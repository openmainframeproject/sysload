/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file bootmap.c
 * \brief FCP specific functions to load bootmap data
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 *
 * $Id: bootmap_fcp.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <regex.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "loader.h"
#include "bootmap.h"


// definitions to extract URI fields
#define URI_ZFCP_BOOTMAP_RE    "^zfcp://\\(([[:xdigit:]])\\.([[:xdigit:]])" \
	"\\.([[:xdigit:]]{4}),(0x[[:xdigit:]]{16}),(0x[[:xdigit:]]{16})" \
	"(,([[:digit:]]{1,2}))?\\)$"
#define URI_ZFCP_BOOTMAP_BUSID 1
#define URI_ZFCP_BOOTMAP_WWPN  4
#define URI_ZFCP_BOOTMAP_LUN   5
#define URI_ZFCP_BOOTMAP_PROG  7
#define URI_ZFCP_BOOTMAP_MAX   9

#define SYS_PATH_FCP                    "/sys/bus/ccw/drivers/zfcp"
#define SYS_PATH_SCSI                   "/sys/bus/scsi/devices"


/**
 * Find FCP SCSI device identified by bus ID, WWPN and LUN and return host
 * number, channel, SCSI ID and SCSI LUN.
 *
 * \param[in]  busid    Bus ID of the FCP channel through which the SCSI
 *                      device is attached
 * \param[in]  wwpn     WWPN through which the SCSI device is accessed
 * \param[in]  lun      LUN of the SCSI device
 * \param[out] host     Host number of FCP SCSI device
 * \param[out] channel  Channel of FCP SCSI device
 * \param[out] scsi_id  SCSI ID of FCP SCSI device
 * \param[out] scsi_lun SCSI LUN of FCP SCSI device
 * \return              If FCP SCSI device was found 0, otherwise -1.
 *
 */

static int
find_scsi_device(const char *busid, const char *wwpn, const char *lun,
    int *host, int *channel, int *scsi_id, int *scsi_lun)
{
	char *path = NULL, tmpstr[20];
	int ret = -1;
	DIR *dir;
	struct dirent *dirent;

	dir = opendir(SYS_PATH_SCSI);
	if (dir == NULL)
		return -1;
	while ((dirent = readdir(dir)) != NULL) {
		if (strcmp(dirent->d_name, ".") == 0 ||
		    strcmp(dirent->d_name, "..") == 0 )
			continue;

		cfg_strprintf(&path, "%s/%s/hba_id", SYS_PATH_SCSI,
		    dirent->d_name);
		if (read_file(path, tmpstr, sizeof(tmpstr)) == -1 ||
		    strcmp(busid, tmpstr) !=0 )
			continue;

		cfg_strprintf(&path, "%s/%s/wwpn", SYS_PATH_SCSI,
		    dirent->d_name);
		if (read_file(path, tmpstr, sizeof(tmpstr)) == -1 ||
		    strcmp(wwpn, tmpstr) !=0 )
			continue;

		cfg_strprintf(&path, "%s/%s/fcp_lun", SYS_PATH_SCSI,
		    dirent->d_name);
		if (read_file(path, tmpstr, sizeof(tmpstr)) == -1 ||
		    strcmp(lun, tmpstr) !=0 )
			continue;

		if (sscanf(dirent->d_name, "%i:%i:%i:%i", host, channel,
			scsi_id, scsi_lun) == 4) {
			ret = 0;
			break;
		}
	}
	cfg_strfree(&path);
	closedir(dir);

	return ret;
}


/**
 * Set FCP attached SCSI disk online and return device node. On success NULL
 * is returned. On error a dynamically allocated error message is returned.
 *
 * \param[in]  busid  Bus ID of the FCP channel through which the SCSI disk
 *                    is attached
 * \param[in]  wwpn   WWPN through which the SCSI is accessed
 * \param[in]  lun    LUN of the SCSI disk
 * \param[out] dev    Dynamically allocated string with path to block device
 * \return     In case of error dynamically allocated error message.
 */

static char *
set_fcp_disk_online(const char *busid, const char *wwpn, const char *lun,
    char **dev)
{
	char *errmsg = NULL, *syspath = NULL, *echo = NULL;
	char *test = NULL, *pattern = NULL, block[50], nodestr[16];
	int fd, host, channel, scsi_id, scsi_lun, major = 0, minor = 0;

	// check if FCP channel exists
	cfg_strprintf(&syspath, "%s/%s", SYS_PATH_FCP, busid);
	if (access(syspath, F_OK)) {
		cfg_strprintf(&errmsg, "Error setting FCP channel '%s' online"
		    " - no such channel device", busid);
		cfg_strfree(&syspath);
		return errmsg;
	}

	// set FCP channel online
	cfg_strprintf(&echo, "%s/online", syspath);
	if (echo_and_test(echo, "1", 200, echo) != 1) {
		cfg_strprintf(&errmsg,
		    "Error setting FCP channel '%s' online", busid);
		cfg_strfree(&syspath);
		cfg_strfree(&echo);
		return errmsg;
	}

	// configure WWPN
	cfg_strprintf(&test, "%s/%s", syspath, wwpn);
	if (access(test, F_OK)) {
		cfg_strprintf(&echo, "%s/port_add", syspath);
		cfg_strprintf(&test, "%s/%s/failed", syspath, wwpn);
		if (echo_and_test(echo, wwpn, 200, test) != 0) {
			cfg_strprintf(&errmsg,
			    "Error configuring WWPN '%s' on FCP channel '%s'",
			    wwpn, busid);
			cfg_strfree(&syspath);
			cfg_strfree(&echo);
			cfg_strfree(&test);
			return errmsg;
		}
	}

	// configure LUN
	cfg_strprintf(&test, "%s/%s/%s", syspath, wwpn, lun);
	if (access(test, F_OK)) {
		cfg_strprintf(&echo, "%s/%s/unit_add", syspath, wwpn);
		cfg_strprintf(&test, "%s/%s/%s/failed", syspath, wwpn, lun);
		if (echo_and_test(echo, lun, 200, test) != 0) {
			cfg_strprintf(&errmsg,
			    "Error configuring LUN '%s' on WWPN '%s' on FCP "
			    "channel '%s'", lun, wwpn, busid);
			cfg_strfree(&syspath);
			cfg_strfree(&echo);
			cfg_strfree(&test);
			return errmsg;
		}
	}
	cfg_strfree(&syspath);
	cfg_strfree(&echo);
	cfg_strfree(&test);

	// create device node
	if (find_scsi_device(busid, wwpn, lun, &host, &channel,
		&scsi_id, &scsi_lun)) {
		cfg_strprintf(&errmsg, "Error getting device node information"
		    " for FCP disk %s:%s:%s", busid, wwpn, lun);
		return errmsg;
	}

	cfg_strprintf(&pattern, "%s/%i:%i:%i:%i/block:*/dev", SYS_PATH_SCSI,
		      host, channel, scsi_id, scsi_lun);
	if (cfg_glob_filename(pattern, block, sizeof(block)) != 1) {
		cfg_strprintf(&errmsg, "Error getting device node information"
			" for FCP disk %s:%s:%s - could not get block:* "
			"sysfs link", busid, wwpn, lun);
		cfg_strfree(&pattern);
		return errmsg;
	}
	cfg_strfree(&pattern);

	fd = open(block, O_RDONLY);
	if (fd == -1) {
		cfg_strprintf(&errmsg, "Error getting device node information"
		    " for FCP disk %s:%s:%s - %s", busid, wwpn, lun,
		    strerror(errno));
		return errmsg;
	}
	memset(nodestr, 0x0, sizeof(nodestr));
	cfg_read(fd, nodestr, sizeof(nodestr)-1);
	close(fd);
	if (sscanf(nodestr, "%i:%i", &major, &minor) != 2) {
		cfg_strprintf(&errmsg,
		    "Error getting device node information for "
		    "FCP disk %s:%s:%s", busid, wwpn, lun);
		return errmsg;
	}
	cfg_strinit(dev);
	cfg_strprintf(dev, "%s/b%s:%s:%s", BLOCKDEV_PATH, busid, wwpn, lun);
	unlink(*dev);
	if (mknod(*dev, 0660 | S_IFBLK, makedev(major, minor))) {
		cfg_strprintf(&errmsg,
		    "Error creating device node for FCP disk "
		    "%s:%s:%s - %s",
		    busid, wwpn, lun, strerror(errno));
		cfg_strfree(dev);
		return errmsg;
	}

	return NULL;
}


/**
 * Delete FCP SCSI device.
 *
 * \param[in]  busid  Bus ID of the FCP channel through which the SCSI
 *                    device is attached
 * \param[in]  wwpn   WWPN through which the SCSI device is accessed
 * \param[in]  lun    LUN of the SCSI device
 * \return            On success 0, on error -1.
 */

static int
delete_scsi_device(const char *busid, const char *wwpn, const char *lun)
{
	char *echo = NULL;
	int host, scsi_id, channel, scsi_lun, ret;

	if (find_scsi_device(busid, wwpn, lun, &host, &channel, &scsi_id,
		&scsi_lun))
		return -1;
	cfg_strprintf(&echo, "%s/%i:%i:%i:%i/delete", SYS_PATH_SCSI, host,
	    channel, scsi_id, scsi_lun);
	ret = echo_and_test(echo, "1", 0, NULL);
	cfg_strfree(&echo);
	return ret;
}


/**
 * Set FCP attached SCSI disk offline and delete device node.
 *
 * \param[in]  busid  Bus ID of the FCP channel through which the SCSI disk
 *                    is attached
 * \param[in]  wwpn   WWPN through which the SCSI is accessed
 * \param[in]  lun    LUN of the SCSI disk
 * \return     If FCP SCSI disk was set offline 0, otherwise -1.
 */

static int
set_fcp_disk_offline(const char *busid, const char *wwpn, const char *lun)
{
	char *path = NULL;
	int ret;

	// delete FCP SCS disk
	delete_scsi_device(busid, wwpn, lun);

	// remove LUN
	cfg_strprintf(&path, "%s/%s/%s/unit_remove", SYS_PATH_FCP, busid,
	    wwpn);
	echo_and_test(path, lun, 0, NULL);

	// remove WWPN
	cfg_strprintf(&path, "%s/%s/port_remove", SYS_PATH_FCP, busid);
	echo_and_test(path, wwpn, 0, NULL);

	// set FCP channel offline
	cfg_strprintf(&path, "%s/%s/online", SYS_PATH_FCP, busid);
	echo_and_test(path, "0", 0, NULL);

	// delete device node
	cfg_strprintf(&path, "%s/b%s:%s:%s", BLOCKDEV_PATH, busid, wwpn, lun);
	unlink(path);
	cfg_strfree(&path);

	// final check
	cfg_strprintf(&path, "%s/%s/%s/%s", SYS_PATH_FCP, busid, wwpn, lun);
	ret = !access(path, F_OK);
	cfg_strfree(&path);

	return ret;
}


/**
 * Read SCSI MBR and update program table pointer. On success NULL is
 * returned. On error a dynamically allocated error message is returned.
 * Note: currently are only PC BIOS disk layouts supported!
 *
 * \param[in] disk Pointer to partially initialized disk structure
 * \return         In case of error dynamically allocated error message.
 *
 */

char *
disk_read_scsi_mbr(struct disk *disk)
{
	unsigned char *bootrecord;
	char *errmsg = NULL;
	disk_blockptr_t blockptr;

	// load MBR
	bootrecord = malloc(disk->phy_block_size);
	MEM_ASSERT(bootrecord);
	memset(&blockptr, 0x0, sizeof(blockptr));
	blockptr.linear.block = 0;
	blockptr.linear.size = disk->phy_block_size;
	blockptr.linear.blockct = 0;
	errmsg = disk_read_phy_blocks(disk, bootrecord, &blockptr);
	if (errmsg) {
		free(bootrecord);
		return errmsg;
	}

	// verify MBR
	if (bootrecord[510] != 0x55 || bootrecord[511] != 0xaa) {
		cfg_strcpy(&errmsg, "Unsupported SCSI disk layout.");
		return errmsg;
	}
	if (strncmp((char *) bootrecord, MAGIC, MAGIC_LENGTH) !=0) {
		cfg_strcpy(&errmsg,
		    "Missing boot signature on SCSI disk.");
		return errmsg;
	}

	read_packed_blockptr(disk, &disk->program_table_ptr, bootrecord+16);
	free(bootrecord);

	return NULL;
}


/**
 * Handle bootmap boot from FCP SCSI disk devices. On error a dynamically
 * allocated error message is returned.
 *
 * \param[in] boot    Pointer to cfg_bentry structure with boot information.
 * \return    In case of error dynamically allocated error message.
 */

char *
action_bootmap_boot_zfcp(struct cfg_bentry *boot)
{
	char *errmsg = NULL, tmp_buffer[64], busid[16], wwpn[20];
	char lun[20], *dev;
	int ret, n, program = 0;
	regex_t preg;
	regmatch_t pmatch[URI_ZFCP_BOOTMAP_MAX];
	struct disk disk;

	// compile regular expression
	if ((ret = regcomp(&preg, URI_ZFCP_BOOTMAP_RE, REG_EXTENDED))) {
		regerror(ret, &preg, tmp_buffer, sizeof(tmp_buffer));
		cfg_strprintf(&errmsg,
		    "Internal error: unable to compile regular "
		    "expression - %s", tmp_buffer);
		return errmsg;
	}

	// match URI & extract fields
	ret = regexec(&preg, boot->bootmap, URI_ZFCP_BOOTMAP_MAX, pmatch, 0);
	if (ret) {
		regfree(&preg);
		cfg_strcpy(&errmsg,
		    "Error - invalid 'zfcp' boot map URI.");
		return errmsg;
	}
	strncpy(busid, boot->bootmap + pmatch[URI_ZFCP_BOOTMAP_BUSID].rm_so,
	    8);
	busid[8] = '\0';
	for (n = 0; n < strlen(busid); n++)
		busid[n] = tolower(busid[n]);
	strncpy(wwpn, boot->bootmap + pmatch[URI_ZFCP_BOOTMAP_WWPN].rm_so, 18);
	wwpn[18] = '\0';
	for (n = 0; n < strlen(wwpn); n++)
		wwpn[n] = tolower(wwpn[n]);
	strncpy(lun, boot->bootmap + pmatch[URI_ZFCP_BOOTMAP_LUN].rm_so, 18);
	lun[18] = '\0';
	for (n = 0; n < strlen(lun); n++)
		lun[n] = tolower(lun[n]);
	if (pmatch[URI_ZFCP_BOOTMAP_PROG].rm_so != -1)
		sscanf(boot->bootmap+pmatch[URI_ZFCP_BOOTMAP_PROG].rm_so,
		    "%i", &program);
	regfree(&preg);

	// set FCP disk online and boot
	errmsg = set_fcp_disk_online(busid, wwpn, lun, &dev);
	if (errmsg)
		return errmsg;
	errmsg = disk_open(&disk, dev);
	cfg_strfree(&dev);
	if (errmsg)
		return errmsg;
	errmsg = boot_bootmap_disk(boot, &disk, program);

	// if we are still here boot failed
	disk_close(&disk);
	set_fcp_disk_offline(busid, wwpn, lun);

	return errmsg;
}
