/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file bootmap.h
 * \brief Include file for reading boot maps
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 *
 * $Id: bootmap.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _BOTTMAP_H_
#define _BOOTMAP_H_

#include <stdint.h>
#include <sys/types.h>


// Most of the following definitions are take from zipl

// Type for representing disk block numbers
typedef uint64_t blocknum_t;

// Pointer to block on a disk with cyl/head/sec layout
struct disk_blockptr_chs {
	int cyl;
	int head;
	int sec;
	int size;
	int blockct;
};

// Pointer to a block on a disk with linear layout
struct disk_blockptr_linear {
	blocknum_t block;
	int size;
	int blockct;
};

// Pointer to a block on disk
typedef union {
	struct disk_blockptr_chs chs;
	struct disk_blockptr_linear linear;
} disk_blockptr_t;

// Disk type identifier
typedef enum {
	disk_type_scsi,
	disk_type_fba,
	disk_type_diag,
	disk_type_eckd_classic,
	disk_type_eckd_compatible,
	disk_type_unknown
} disk_type_t;

// from linux/hdregs.h
struct hd_geometry {
	unsigned char heads;
	unsigned char sectors;
	unsigned short cylinders;
	unsigned long start;
};

// Disk type
struct disk {
	int fd;
	disk_type_t type;
	int phy_block_size;
	uint64_t phy_blocks;
	struct hd_geometry geo;
	disk_blockptr_t program_table_ptr;
};

// path to create private block device nodes
#define BLOCKDEV_PATH                   "/dev"

// magic number to idetify boot map data structures
#define MAGIC "zIPL"
#define MAGIC_LENGTH 4

int echo_and_test(const char *echo, const char *data, unsigned long msleep,
    const char *test);
ssize_t read_file(const char *path, char *buffer, size_t buffer_len);
void disk_close(struct disk *disk);
char *disk_read_dasd_boot_record(struct disk *disk);
char *disk_read_scsi_mbr(struct disk *disk);
char *disk_open(struct disk *disk, const char *path);
void read_packed_blockptr(struct disk *disk, disk_blockptr_t *ptr,
    void *buffer);
char *disk_read_phy_blocks(struct disk *disk, void *buffer,
    disk_blockptr_t *blockptr);
int get_program_table_size(struct disk *disk);
char *boot_bootmap_disk(struct cfg_bentry *boot, struct disk *disk,
    int program);
char *action_bootmap_boot_dasd(struct cfg_bentry *boot);
char *action_bootmap_boot_zfcp(struct cfg_bentry *boot);

#endif /* #ifndef _BOOTMAP_H_ */
