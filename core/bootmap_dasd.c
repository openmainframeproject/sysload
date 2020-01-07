/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file bootmap.c
 * \brief DASD specific functions to load bootmap data
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 *
 * $Id: bootmap_dasd.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "loader.h"
#include "bootmap.h"


// definitions to extract URI fields
#define URI_DASD_BOOTMAP_RE    "^dasd://\\(([[:xdigit:]])\\.([[:xdigit:]])" \
	"\\.([[:xdigit:]]{4})(,([[:digit:]]{1,2}))?\\)$"
#define URI_DASD_BOOTMAP_BUSID 1
#define URI_DASD_BOOTMAP_PROG  5
#define URI_DASD_BOOTMAP_MAX   7

#define SYS_PATH_DASD                   "/sys/bus/ccw/devices"


/**
 * Set DASD Channel device online and return device node. On success NULL
 * is returned. On error a dynamically allocated error message is returned.
 *
 * \param[in]  busid  Bus ID of channel device to be set online
 * \param[out] dev    Dynamically allocated string with path to block device
 * \return     In case of error dynamically allocated error message.
 */

static char *
set_dasd_online(const char *busid, char **dev)
{
	char *errmsg = NULL, *syspath = NULL, *echo = NULL;
	char nodestr[16], *pattern = NULL, block[50];
	int fd, major = 0, minor = 0;

	// check if channel device exists
	cfg_strprintf(&syspath, "%s/%s", SYS_PATH_DASD, busid);
	if (access(syspath, F_OK)) {
		cfg_strprintf(&errmsg, "Error setting DASD '%s' online - "
		    "no such channel device", busid);
		cfg_strfree(&syspath);
		return errmsg;
	}

	// set channel device online
	cfg_strprintf(&echo, "%s/online", syspath);
	if (echo_and_test(echo, "1", 100, echo) != 1) {
		cfg_strprintf(&errmsg, "Error setting DASD '%s' online",
		    busid);
		cfg_strfree(&syspath);
		cfg_strfree(&echo);
		return errmsg;
	}
	cfg_strfree(&echo);

	// create device node
	cfg_strprintf(&pattern, "%s/block:*/dev", syspath);
	if (cfg_glob_filename(pattern, block, sizeof(block)) != 1) {
		cfg_strprintf(&errmsg, "Error setting DASD '%s' online - "
			"could not get block:* sysfs link", busid);
		cfg_strfree(&pattern);
		cfg_strfree(&syspath);
		return errmsg;
	}
	cfg_strfree(&pattern);
	cfg_strfree(&syspath);

	fd = open(block, O_RDONLY);
	if (fd == -1) {
		cfg_strprintf(&errmsg, "Error getting device node information"
		    " for DASD '%s' - %s", busid, strerror(errno));
		return errmsg;
	}
	memset(nodestr, 0x0, sizeof(nodestr));
	cfg_read(fd, nodestr, sizeof(nodestr)-1);
	close(fd);
	if (sscanf(nodestr, "%i:%i", &major, &minor) != 2) {
		cfg_strprintf(&errmsg,
		    "Error getting device node information for DASD '%s'",
		    busid);
		return errmsg;
	}
	cfg_strinit(dev);
	cfg_strprintf(dev, "%s/b%s", BLOCKDEV_PATH, busid);
	unlink(*dev);
	if (mknod(*dev, 0660 | S_IFBLK, makedev(major, minor))) {
		cfg_strprintf(&errmsg,
		    "Error creating device node for DASD '%s' - %s",
		    busid, strerror(errno));
		cfg_strfree(dev);
		return errmsg;
	}

	return NULL;
}


/**
 * Set DASD Channel device offline and delete device node.
 *
 * \param[in]  busid  Bus ID of channel device to be set offline
 */

static void
set_dasd_offline(const char *busid)
{
	char *echo = NULL, *dev = NULL;

	// set channel device offline
	cfg_strprintf(&echo, "%s/%s/online", SYS_PATH_DASD, busid);
	echo_and_test(echo, "0", 0, NULL);
	cfg_strfree(&echo);

	// delete device node
	cfg_strprintf(&dev, "%s/b%s", BLOCKDEV_PATH, busid);
	unlink(dev);
	cfg_strfree(&dev);

}


/**
 * Read DASD boot record and update program table pointer. On success NULL is
 * returned. On error a dynamically allocated error message is returned.
 *
 * \param[in] disk Pointer to partially initialized disk structure
 * \return         In case of error dynamically allocated error message.
 *
 */

char *
disk_read_dasd_boot_record(struct disk *disk)
{
	char *bootrecord;
	char *errmsg = NULL;
	disk_blockptr_t blockptr;

	bootrecord = malloc(disk->phy_block_size);
	MEM_ASSERT(bootrecord);
	memset(&blockptr, 0x0, sizeof(blockptr));
	blockptr.chs.cyl = 0;
	blockptr.chs.head = 0;
	blockptr.chs.sec = 2;
	blockptr.chs.size = disk->phy_block_size;
	blockptr.chs.blockct = 0;
	errmsg = disk_read_phy_blocks(disk, bootrecord, &blockptr);
	if (!errmsg)
		read_packed_blockptr(disk, &disk->program_table_ptr,
		    bootrecord+4);
	free(bootrecord);

	return errmsg;
}


/**
 * Handle bootmap boot from DASD devices. On error a dynamically allocated
 * error message is returned.
 *
 * \param[in] boot    Pointer to cfg_bentry structure with boot information.
 * \return    In case of error dynamically allocated error message.
 */

char *
action_bootmap_boot_dasd(struct cfg_bentry *boot)
{
	char *errmsg = NULL, tmp_buffer[64], busid[16], *dev;
	int ret, n, program = 0;
	regex_t preg;
	regmatch_t pmatch[URI_DASD_BOOTMAP_MAX];
	struct disk disk;

	// compile regular expression
	if ((ret = regcomp(&preg, URI_DASD_BOOTMAP_RE, REG_EXTENDED))) {
		regerror(ret, &preg, tmp_buffer, sizeof(tmp_buffer));
		cfg_strprintf(&errmsg, "Internal error: unable to compile "
		    "regular expression - %s", tmp_buffer);
		return errmsg;
	}

	// match URI & extract fields
	ret = regexec(&preg, boot->bootmap, URI_DASD_BOOTMAP_MAX, pmatch, 0);
	if (ret) {
		regfree(&preg);
		cfg_strcpy(&errmsg,
		    "Error - invalid 'dasd' boot map URI.");
		return errmsg;
	}
	strncpy(busid, boot->bootmap + pmatch[URI_DASD_BOOTMAP_BUSID].rm_so,
	    8);
	busid[8] = '\0';
	for (n = 0; n < strlen(busid); n++)
		busid[n] = tolower(busid[n]);
	if (pmatch[URI_DASD_BOOTMAP_PROG].rm_so != -1)
		sscanf(boot->bootmap+pmatch[URI_DASD_BOOTMAP_PROG].rm_so,
		    "%i", &program);
	regfree(&preg);

	// set DASD online and boot
	errmsg = set_dasd_online(busid, &dev);
	if (errmsg)
		return errmsg;
	errmsg = disk_open(&disk, dev);
	cfg_strfree(&dev);
	if (errmsg)
		return errmsg;
	errmsg = boot_bootmap_disk(boot, &disk, program);

	// if we are still here boot failed
	disk_close(&disk);
	set_dasd_offline(busid);

	return errmsg;
}
