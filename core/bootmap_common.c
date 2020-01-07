/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file bootmap.c
 * \brief Common functions to load bootmap data
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 *
 * $Id: bootmap_common.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include "sysload.h"
#include "loader.h"
#include "bootmap.h"


// Most of the following definitions are take from zipl

// from linux/fs.h
#define BLKGETSIZE              _IO(0x12, 96)
#define BLKSSZGET               _IO(0x12, 104)

// from linux/hdregs.h
#define HDIO_GETGEO             0x0301

#define DASD_IOCTL_LETTER       'D'
#define BIODASDINFO             _IOR(DASD_IOCTL_LETTER, 1,	\
	    struct dasd_information)

// Layout of SCSI disk block pointer
struct scsi_blockptr {
	uint64_t blockno;
	uint16_t size;
	uint16_t blockct;
	uint8_t reserved[4];
} __attribute((packed));

// Layout of FBA disk block pointer
struct fba_blockptr {
	uint32_t blockno;
	uint16_t size;
	uint16_t blockct;
} __attribute((packed));

// Layout of ECKD disk block pointer
struct eckd_blockptr {
	uint16_t cyl;
	uint16_t head;
	uint8_t sec;
	uint16_t size;
	uint8_t blockct;
} __attribute((packed));

// Layout of component table entry
struct component_entry {
	uint8_t data[23];
	uint8_t type;
	union {
		uint64_t load_address;
		uint64_t load_psw;
	} address;
} __attribute((packed));

typedef enum {
	component_execute = 0x01,
	component_load = 0x02
} component_type;

// Component table entry
struct component {
	disk_blockptr_t segment_ptr;
	component_type type;
	union {
		uint64_t load_address;
		uint64_t load_psw;
	} address;
};


// Definitions for dasd device driver, taken from linux/include/asm/dasd.h
struct dasd_information {
        unsigned int devno;             /* S/390 devno */
        unsigned int real_devno;        /* for aliases */
        unsigned int schid;             /* S/390 subchannel identifier */
        unsigned int cu_type : 16;      /* from SenseID */
        unsigned int cu_model : 8;      /* from SenseID */
        unsigned int dev_type : 16;     /* from SenseID */
        unsigned int dev_model : 8;     /* from SenseID */
        unsigned int open_count;
        unsigned int req_queue_len;
        unsigned int chanq_len;         /* length of chanq */
        char type[4];                   /* from discipline.name */
        unsigned int status;            /* current device level */
        unsigned int label_block;       /* where to find the VOLSER */
        unsigned int FBA_layout;        /* fixed block size (like AIXVOL) */
        unsigned int characteristics_size;
        unsigned int confdata_size;
        char characteristics[64];       /* from read_device_characteristics */
        char configuration_data[256];   /* from read_configuration_data */
};


// Data structure at beginning of zipl's stage 3 boot loader
struct zipl_stage3_params {
        uint64_t parm_addr;
        uint64_t initrd_addr;
        uint64_t initrd_len;
        uint64_t load_psw;
};
#define PSW_ADDRESS_MASK                0x000000007fffffffLL
#define PSW_DISABLED_WAIT               0x000a000000000000LL
#define KERNEL_HEADER_SIZE              65536

// IPL type in component table
typedef enum {
        component_header_ipl = 0x00,
        component_header_dump = 0x01
} component_header_type;


/**
 * Open file \p echo, write \p data and sleep \p msleep milliseconds.
 * Next open file \p test for reading and return received data converted
 * to integer.
 *
 * \param[in] echo     File path to open for writing
 * \param[in] data     Data to be writen to echo file
 * \param[in] msleep   Sleep time in milliseconds
 * \param[in] test     File path for testing
 * \return    Received data from \p test converted to integer or -1 if an
 *            file I/O error occurred
 */

int
echo_and_test(const char *echo, const char *data, unsigned long msleep,
    const char *test)
{
	char buffer[32];
	int fd, len, ret;
	struct timespec req, rem;

	// echo data
	if (echo && data && strlen(echo) && strlen(data)) {
		fd = open(echo, O_WRONLY);
		if (fd == -1) {
			return -1;
		}
		if (write(fd, data, strlen(data)) != strlen(data)) {
			close(fd);
			return -1;
		}
		close(fd);
	}

	// sleep
	if (usleep > 0) {
		rem.tv_sec = msleep / 1000;
		rem.tv_nsec = (msleep % 1000) * 1000000;
		do {
			req = rem;
		} while (nanosleep(&req, &rem) == -1 && errno == EINTR);
	}

	// test
	if (test && strlen(test)) {
		fd = open(test, O_RDONLY);
		if (fd == -1)
			return -1;
		len = cfg_read(fd, buffer, sizeof(buffer)-1);
		close(fd);
		if (len == -1)
			return -1;
		buffer[len] = '\0';
		if (sscanf(buffer, "%i", &ret) != 1)
			return -1;
		else
			return ret;
	}

	return 0;
}


/**
 * Read file contents into provided buffer. A trailing \p '\n' character
 * is removed and read data is terminated by \p '\0'.
 *
 * \param[in] path        Path to file to be read
 * \param[in] buffer      Pointer to buffer to store file data
 * \param[in] buffer_len  Size of buffer in bytes
 * \return    On success number of bytes read. On error -1.
 */

ssize_t
read_file(const char *path, char *buffer, size_t buffer_len)
{
	int fd;
	ssize_t read_len;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -1;
	read_len = cfg_read(fd, buffer, buffer_len - 1);
	if (read_len == -1) {
		close(fd);
		return -1;
	}
	close(fd);
	if (read_len && buffer[read_len-1] == '\n')
		read_len--;
	buffer[read_len] = '\0';

	return read_len;
}


/**
 * Return block count element in \p blockptr structure.
 *
 * \param[in]  disk  Pointer to disk structure for identifing \p blockptr
 *                   format
 * \param[in]  ptr   Block pointer to be used
 * \return     block count element
 */

static int
get_blockptr_blockct(struct disk *disk, disk_blockptr_t *ptr)
{
	int blockct;

	switch (disk->type) {
	case disk_type_scsi:
	case disk_type_fba:
		blockct = ptr->linear.blockct;
		break;

	case disk_type_eckd_classic:
	case disk_type_eckd_compatible:
		blockct = ptr->chs.blockct;
		break;

	case disk_type_diag:
	case disk_type_unknown:
		blockct = 0;
		break;
	}

	return blockct;
}


/**
 * Calculate size of \p struct \p blockptr structure depended on disk type.
 *
 * \param[in]  disk  Reference to disk type.
 * \return     size of \p struct \p blockptr structure in bytes.
 */

static int
get_blockptr_size(struct disk *disk)
{
	switch (disk->type) {
	case disk_type_scsi:
		return sizeof(struct scsi_blockptr);

	case disk_type_fba:
		return sizeof(struct fba_blockptr);

	case disk_type_eckd_classic:
	case disk_type_eckd_compatible:
		return sizeof(struct eckd_blockptr);

	case disk_type_diag:
	case disk_type_unknown:
		break;
	}

	return 0;
}


/**
 * Test if disk block pointer is \p NULL.
 *
 * \param[in] disk     Pointer to disk structure for identifing \p blockptr
 *                     format
 * \param[in] blockptr Block pointer to be tested
 * \return    -1 if \p blockptr is \p NULL, 0 if \p blockptr is not \p NULL
 */

static int
blockptr_is_null(struct disk *disk, disk_blockptr_t *blockptr)
{
	char *ptr = (char *) blockptr;
	int n, size = get_blockptr_size(disk);

	for (n = 0; n < size; n++) {
		if (*ptr != 0x0)
			return 0;
		ptr++;
	}

	return -1;
}


/**
 * Convert packed block pointer in \p buffer into regular representation.
 *
 * \param[in]  disk   Pointer to disk structure for identifing pointer
 *                    format
 * \param[in]  buffer Pointer to data buffer with packed block pointer
 * \param[out] ptr    Pointer to destination for converted block pointer
 */

void
read_packed_blockptr(struct disk *disk, disk_blockptr_t *ptr, void *buffer)
{
	struct scsi_blockptr *scsi;
	struct eckd_blockptr *eckd;
	struct fba_blockptr *fba;

	memset(ptr, 0x0, sizeof (*ptr));
	switch (disk->type) {
	case disk_type_scsi:
		scsi = (struct scsi_blockptr *) buffer;
		ptr->linear.block = scsi->blockno;
		ptr->linear.size = scsi->size;
		ptr->linear.blockct = scsi->blockct;
		break;

	case disk_type_eckd_classic:
	case disk_type_eckd_compatible:
		eckd = (struct eckd_blockptr *) buffer;
		ptr->chs.cyl = eckd->cyl;
		ptr->chs.head = eckd->head;
		ptr->chs.sec = eckd->sec;
		ptr->chs.size = eckd->size;
		ptr->chs.blockct = eckd->blockct;
		break;

	case disk_type_fba:
		fba = (struct fba_blockptr *) buffer;
		ptr->linear.block = fba->blockno;
		ptr->linear.size = fba->size;
		ptr->linear.blockct = fba->blockct;
		break;

	case disk_type_diag:
	case disk_type_unknown:
		break;
	}
}


/**
 * Print disk_blockptr_t structure.
 *
 * \param[in] disk   Pointer to disk structure for identifing pointer format
 * \param[in] ptr    Pointer to block pointer to be printed
 */

void print_blockptr(struct disk *disk, disk_blockptr_t *blockptr)
{
	switch (disk->type) {
	case disk_type_scsi:
	case disk_type_fba:
		printf("block=%i, size=%i, blockct=%i\n",
		    (int) blockptr->linear.block,
		    blockptr->linear.size,
		    blockptr->linear.blockct);
		break;

	case disk_type_eckd_classic:
	case disk_type_eckd_compatible:
		printf("cyl=%i, head=%i, sec=%i, size=%i, blockct=%i\n",
		    blockptr->chs.cyl,
		    blockptr->chs.head,
		    blockptr->chs.sec,
		    blockptr->chs.size,
		    blockptr->chs.blockct);
		break;

	case disk_type_diag:
	case disk_type_unknown:
		break;
	}
}


/**
 * Check if block pointer is valid.
 *
 * \param[in] disk      Pointer to initialized disk structure
 * \param[in] blockptr  Block pointer to be checked
 * \return    \p 0 in case block pointer is valid, \p -1 in case block
 *            pointer is not valid
 */

static int
check_blockptr(struct disk *disk, disk_blockptr_t *blockptr)
{
	switch (disk->type) {
	case disk_type_scsi:
	case disk_type_fba:
		if (blockptr->linear.blockct < 0 ||
		    blockptr->linear.size != disk->phy_block_size ||
		    blockptr->linear.block >= disk->phy_blocks)
			return -1;
		break;

	case disk_type_eckd_classic:
	case disk_type_eckd_compatible:
		if (blockptr->chs.blockct < 0 ||
		    blockptr->chs.size != disk->phy_block_size ||
		    blockptr->chs.cyl < 0 ||
		    blockptr->chs.cyl >= disk->geo.cylinders ||
		    blockptr->chs.head < 0 ||
		    blockptr->chs.head >= disk->geo.heads ||
		    blockptr->chs.sec < 1 ||
		    blockptr->chs.sec > disk->geo.sectors)
			return -1;
		break;

	case disk_type_diag:
	case disk_type_unknown:
		break;
	}

	return 0;
}


/**
 * Open specified block device and read vital disk information. On
 * success NULL is returned. On error a dynamically allocated error message
 * is returned.
 *
 * \param[in]  disk  Pointer to disk structure
 * \param[in]  path  Path to DASD block device
 * \return     In case of error dynamically allocated error message.
 */

char *
disk_open(struct disk *disk, const char *path)
{
	char *errmsg = NULL;
	long devsize;
	struct dasd_information dasd_info;

	memset(disk, 0x0, sizeof (*disk));

	disk->fd = open(path, O_RDONLY);
	if (disk->fd == -1) {
		cfg_strprintf(&errmsg, "Error opening disk - %s",
		    strerror(errno));
		return errmsg;
	}

	// get geometry information
	if (ioctl(disk->fd, HDIO_GETGEO, &(disk->geo))) {
		cfg_strprintf(&errmsg, "Error getting disk geometry - %s",
		    strerror(errno));
		close(disk->fd);
		return errmsg;
	}

	// determine disk type
	if (!ioctl(disk->fd, BIODASDINFO, &dasd_info)) {
		if (strncmp(dasd_info.type, "FBA ", 4) == 0)
			disk->type = disk_type_fba;
		else if (strncmp(dasd_info.type, "DIAG", 4) == 0)
			disk->type = disk_type_diag;
		else if (strncmp(dasd_info.type, "ECKD", 4) == 0) {
			if (dasd_info.FBA_layout)
				disk->type = disk_type_eckd_classic;
			else
				disk->type = disk_type_eckd_compatible;
		}
		else
			disk->type = disk_type_unknown;
	} else
		disk->type = disk_type_scsi;

	// get physical block size
	if (ioctl(disk->fd, BLKSSZGET, &disk->phy_block_size)) {
		cfg_strprintf(&errmsg, "Error getting blocksize - %s",
		    strerror(errno));
		close(disk->fd);
		return errmsg;
	}

	// get size of device in sectors (512 byte)
	if (ioctl(disk->fd, BLKGETSIZE, &devsize)) {
		cfg_strprintf(&errmsg, "Error getting device size - %s",
		    strerror(errno));
		close(disk->fd);
		return errmsg;
	}

	// convert device size to size in physical blocks
	disk->phy_blocks = devsize / (disk->phy_block_size / 512);

	// read boot record
	switch (disk->type) {
	case disk_type_scsi:
	case disk_type_fba:
		errmsg = disk_read_scsi_mbr(disk);
		break;

	case disk_type_eckd_classic:
	case disk_type_eckd_compatible:
		errmsg = disk_read_dasd_boot_record(disk);
		break;

	case disk_type_diag:
	case disk_type_unknown:
		cfg_strcpy(&errmsg, "Unsupported disk type.");
		break;
	}
	if (errmsg) {
		close(disk->fd);
		return errmsg;
	}

	// check program table pointer
	if (check_blockptr(disk, &disk->program_table_ptr)) {
		cfg_strcpy(&errmsg,
		    "Error - invalid program table pointer in boot record");
		close(disk->fd);
		return errmsg;
	}
	if (get_blockptr_blockct(disk, &disk->program_table_ptr) !=0) {
		cfg_strcpy(&errmsg,
		    "Error - invalid program table pointer in boot record");
		close(disk->fd);
		return errmsg;
	}

	return NULL;
}


/**
 * Close specified disk.
 *
 * \param[in]  disk  Pointer to disk structure
 * \param[in]  path  Path to DASD block device
 * \return     In case of error dynamically allocated error message.
 */

void
disk_close(struct disk *disk)
{
	close(disk->fd);
}


/**
 * Read physical blocks addressed by \p blockptr from disk into
 * memory. \p blockptr format must match disk type. On success NULL is
 * returned. On error a dynamically allocated error message is returned.
 *
 * \param[in]  disk      Pointer to initialized disk structure
 * \param[out] buffer    Pointer to allocated memory buffer where blocks
 *                       will be stored
 * \param[in]  blockptr  Block address of 1st block to be read from disk
 * \return     In case of error dynamically allocated error message.
 */

char *
disk_read_phy_blocks(struct disk *disk, void *buffer,
    disk_blockptr_t *blockptr)
{
	char *errmsg = NULL;
	off_t offset;
	ssize_t read_len, read_total, read_cnt = 0;

	if (check_blockptr(disk, blockptr)) {
		cfg_strcpy(&errmsg, "Error reading block from disk "
		    "- invalid block pointer");
		return errmsg;
	}

	// convert block pointer to byte offset
	switch (disk->type) {
	case disk_type_scsi:
	case disk_type_fba:
	case disk_type_diag:
		offset = blockptr->linear.block;
		read_total = disk->phy_block_size *
			(blockptr->linear.blockct+1);
		break;

	case disk_type_eckd_classic:
	case disk_type_eckd_compatible:
		offset = blockptr->chs.sec + disk->geo.sectors *
			(blockptr->chs.head +
			    blockptr->chs.cyl*disk->geo.heads) - 1;
		read_total = disk->phy_block_size * (blockptr->chs.blockct+1);
		break;

	default:
		cfg_strcpy(&errmsg,
		    "Invalid disk type for physical block read");
		return errmsg;
	}

	offset *= disk->phy_block_size;
	if (lseek(disk->fd, offset, SEEK_SET) == (off_t)-1) {
		cfg_strprintf(&errmsg,
		    "Error seeking physical block - %s",
		    strerror(errno));
		return errmsg;
	}
	while (read_cnt < read_total) {
		read_len = read(disk->fd, buffer+read_cnt,
		    read_total - read_cnt);
		if (read_len == -1) {
			cfg_strprintf(&errmsg,
			    "Error reading data from disk - %s",
			    strerror(errno));
			return errmsg;
		}
		read_cnt += read_len;
	}

	return NULL;
}


/**
 * Calculate maximum number of program entries in program table based
 * on disk type.
 *
 * \param[in]  disk  Pointer to initialized disk structure
 * \return     maximum number of program entries in program table
 */

int
get_program_table_size(struct disk *disk)
{
	return 512 / get_blockptr_size(disk) - 1;
}


/**
 * Read program table from disk. On success NULL is returned. On error a
 * dynamically allocated error message is returned.
 *
 * \param[in]  disk           Pointer to initialized disk structure
 * \param[out] program_table  Pointer to dynamically allocated memory buffer
 *                            with program table
 * \return     In case of error dynamically allocated error message.
 */

static char *
get_program_table(struct disk *disk, disk_blockptr_t **program_table)
{
	char *errmsg = NULL, *buffer;
	int table_size, n;

	// verify program table pointer
	if (get_blockptr_blockct(disk, &disk->program_table_ptr) != 0) {
		cfg_strcpy(&errmsg,
		    "Error - invalid program table pointer");
		return errmsg;
	}

	// read physical program table from disk
	buffer = malloc(disk->phy_block_size);
	MEM_ASSERT(buffer);
	errmsg = disk_read_phy_blocks(disk, buffer, &disk->program_table_ptr);
	if (errmsg) {
		free(buffer);
		return errmsg;
	}

	// verify program table magic number
	if (strncmp(buffer, MAGIC, MAGIC_LENGTH) != 0) {
		cfg_strcpy(&errmsg,
		    "Error - invalid magic number in program table");
		free(buffer);
		return errmsg;
	}

	// convert program table entries to regular format
	table_size = get_program_table_size(disk);
	*program_table = malloc(sizeof (disk_blockptr_t) * table_size);
	MEM_ASSERT(*program_table);
	for (n = 0; n < table_size; n++) {
		read_packed_blockptr(disk, &((*program_table)[n]),
		    buffer + (n+1) * get_blockptr_size(disk));
	}
	free(buffer);

	return NULL;
}


/**
 * Calculate maximum number of component entries in component table based
 * on disk type.
 *
 * \param[in]  disk  Pointer to initialized disk structure
 * \return     maximum number of component entries in component table
 */

static int
get_component_table_size(struct disk *disk)
{
	return disk->phy_block_size / get_blockptr_size(disk) - 1;
}


/**
 * Read component table from disk. On success NULL is returned. On error a
 * dynamically allocated error message is returned.
 *
 * \param[in]  disk             Pointer to initialized disk structure
 * \param[out] component_table  Pointer to dynamically allocated memory buffer
 *                              with component table
 * \param[out] opt              \p opt field from component table
 * \param[in]  table_ptr        Block pointer to requested component table
 * \return     In case of error dynamically allocated error message.
 */

static char *
get_component_table(struct disk *disk, struct component **component_table,
    int *opt, disk_blockptr_t *table_ptr)
{
	char *errmsg = NULL;
	int table_size, n;
	struct component_entry *buffer;

	// verify block pointer to component table
	if (get_blockptr_blockct(disk, table_ptr) != 0) {
		cfg_strcpy(&errmsg,
		    "Error - invalid component table pointer");
		return errmsg;
	}

	// read component table from disk
	buffer = malloc(disk->phy_block_size);
	MEM_ASSERT(buffer);
	errmsg = disk_read_phy_blocks(disk, buffer, table_ptr);
	if (errmsg) {
		free(buffer);
		return errmsg;
	}

	// verify component table magic number
	if (strncmp((char *) buffer, MAGIC, MAGIC_LENGTH) != 0) {
		cfg_strcpy(&errmsg,
		    "Error - invalid magic number in component table");
		free(buffer);
		return errmsg;
	}
	*opt = ((uint8_t *) buffer)[4];

	// convert program table entries to regular format
	table_size = get_component_table_size(disk);
	*component_table = malloc(sizeof (disk_blockptr_t) * table_size);
	MEM_ASSERT(*component_table);
	for (n = 0; n < table_size; n++) {
		read_packed_blockptr(disk,
		    &((*component_table)[n].segment_ptr), &buffer[n+1]);
		(*component_table)[n].type = buffer[n+1].type;
		(*component_table)[n].address.load_address =
			buffer[n+1].address.load_address;
	}
	free(buffer);

	return NULL;
}


/**
 * Copy component from disk into memory. On success NULL is returned. On error
 * a dynamically allocated error message is returned.
 *
 * \param[in]  disk       Pointer to initialized disk structure
 * \param[in]  component  Pointer to component to be loaded
 * \param[out] buffer     Dynamically allocated memory buffer with component
 * \return     In case of error dynamically allocated error message.
 *
 */

static char *
copy_component_to_memory(struct disk *disk, void **buffer,
    int *buffer_size, disk_blockptr_t *component_ptr)
{
	char *errmsg, *segment_table;
	int pointer_in_segment_table, load_position = 0, entry = 0;
	disk_blockptr_t code_ptr;

	// read 1st segment table for component
	segment_table = malloc(disk->phy_block_size*
	    (get_blockptr_blockct(disk, component_ptr)+1));
	MEM_ASSERT(segment_table);
	errmsg = disk_read_phy_blocks(disk, segment_table, component_ptr);
	if (errmsg) {
		free(segment_table);
		return errmsg;
	}
	pointer_in_segment_table = disk->phy_block_size *
		(get_blockptr_blockct(disk, component_ptr)+1) /
		get_blockptr_size(disk);
	read_packed_blockptr(disk, &code_ptr, segment_table);

	// walk through all segment tables and load code segments
	*buffer = NULL;
	*buffer_size = 0;
	do {
		// load code segment
		*buffer_size += disk->phy_block_size *
			(get_blockptr_blockct(disk, &code_ptr)+1);
		*buffer = realloc(*buffer, *buffer_size);
		MEM_ASSERT(*buffer);
		errmsg = disk_read_phy_blocks(disk, (*buffer)+load_position,
		    &code_ptr);
		if (errmsg) {
			free(segment_table);
			free(*buffer);
			return errmsg;
		}
		load_position += disk->phy_block_size *
			(get_blockptr_blockct(disk, &code_ptr)+1);
		entry++;
		read_packed_blockptr(disk, &code_ptr,
		    segment_table + entry * get_blockptr_size(disk));

		// read new segment table if end of current segment
		// table is reached
		if (entry == pointer_in_segment_table-1 &&
		    !blockptr_is_null(disk, &code_ptr)) {
			segment_table = realloc(segment_table,
			    disk->phy_block_size *
			    (get_blockptr_blockct(disk, &code_ptr)+1));
			MEM_ASSERT(segment_table);
			errmsg = disk_read_phy_blocks(disk, segment_table,
			    &code_ptr);
			if (errmsg) {
				free(segment_table);
				free(*buffer);
				return errmsg;
			}
			pointer_in_segment_table = disk->phy_block_size *
				(get_blockptr_blockct(disk, &code_ptr)+1) /
				get_blockptr_size(disk);
			read_packed_blockptr(disk, &code_ptr, segment_table);
			entry = 0;
		}
	} while (!blockptr_is_null(disk, &code_ptr));

	free(segment_table);

	return NULL;
}


/**
 * Identify boot objects (kernel image, initrd, parmfile) by looking at
 * zipl's stage 3 parameter structure. On success NULL is returned. On
 * error a dynamically allocated error message is returned.
 *
 * \param[in]  disk             Pointer to initialized disk structure
 * \param[in]  component_table  Component table to be booted
 * \param[out] kernel           Block pointer to kernel component
 * \param[out] initrd           Block pointer to initrd component
 * \param[out] initrd_len       Length of initrd
 * \param[out] parmfile         Block pointer to parmfile component
 * \return     In case of error dynamically allocated error message.
 */

static char *
identify_boot_objects(struct disk *disk, struct component *component_table,
    disk_blockptr_t *kernel, disk_blockptr_t *initrd,
    int *initrd_len, disk_blockptr_t *parmfile)
{
	void *buffer;
	char *errmsg = NULL;
	int comp_num, buffer_size, n, table_size =
		get_component_table_size(disk);
	uint64_t kernel_addr;
	struct zipl_stage3_params stage3;

	memset(kernel, 0x0, sizeof (disk_blockptr_t));
	memset(initrd, 0x0, sizeof (disk_blockptr_t));
	*initrd_len = 0;
	memset(parmfile, 0x0, sizeof (disk_blockptr_t));

	// find stage 3 boot loader component
	for (comp_num = 0; comp_num < table_size &&
		     component_table[comp_num].type != component_execute;
	     comp_num++);
	if (comp_num == 0 || comp_num == table_size) {
		cfg_strcpy(&errmsg,
		    "Error - invalid component table found");
		return errmsg;
	}
	if (component_table[comp_num].address.load_psw == PSW_DISABLED_WAIT) {
		cfg_strcpy(&errmsg,
		    "Error - data segment load is not supported");
		return errmsg;
	}
	errmsg = copy_component_to_memory(disk, &buffer, &buffer_size,
	    &component_table[comp_num-1].segment_ptr);
	if (errmsg)
		return errmsg;
	stage3 = *((struct zipl_stage3_params *) buffer);
	free(buffer);

	// lookup kernel component
	kernel_addr = stage3.load_psw & PSW_ADDRESS_MASK;
	for (n = 0; n < comp_num &&
		     component_table[n].address.load_address != kernel_addr;
	     n++);
	if (n == comp_num) {
		cfg_strcpy(&errmsg,
		    "Error - invalid component table found");
		return errmsg;
	}
	*kernel = component_table[n].segment_ptr;

	// lookup initrd component
	for (n = 0; n < comp_num && component_table[n].address.load_address
		     != stage3.initrd_addr; n++);
	if (n != comp_num) {
		*initrd = component_table[n].segment_ptr;
		*initrd_len = stage3.initrd_len;
	}

	// lookup parmfile component
	for (n = 0; n < comp_num && component_table[n].address.load_address
		     != stage3.parm_addr; n++);
	if (n != comp_num)
		*parmfile = component_table[n].segment_ptr;

	return NULL;
}

/**
 * Copy kernel component to local filesystem. On success NULL is returned. On
 * error a dynamically allocated error message is returned.
 *
 * \param[in] disk   Pointer to initialized disk structure
 * \param[in] kernel Block pointer to kernel component
 * \return    In case of error dynamically allocated error message.
 */

static char *
read_kernel_component(struct disk *disk, disk_blockptr_t *kernel)
{
	void *buffer;
	char *errmsg = NULL;
	int fd, n, buffer_size, zero = 0x0;

	// open local kernel file and write kernel header because kernel header
	// was removed by zipl
	fd = creat(SYSLOAD_FILENAME_KERNEL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		cfg_strprintf(&errmsg, "Error writing kernel file - %s",
		    strerror(errno));
		return errmsg;
	}
	for (n = 0; n < KERNEL_HEADER_SIZE / sizeof(zero); n++) {
		if (write(fd, &zero, sizeof zero) != sizeof(zero)) {
			cfg_strprintf(&errmsg,
			    "Error writing kernel file - %s", strerror(errno));
			close(fd);
			unlink(SYSLOAD_FILENAME_KERNEL);
			return errmsg;
		}
	}

	// write kernel image
	errmsg = copy_component_to_memory(disk, &buffer, &buffer_size, kernel);
	if (errmsg) {
		close(fd);
		return errmsg;
	}
	if (write(fd, buffer, buffer_size) != buffer_size) {
		cfg_strprintf(&errmsg, "Error writing kernel file - %s",
		    strerror(errno));
		free(buffer);
		close(fd);
		unlink(SYSLOAD_FILENAME_KERNEL);
		return errmsg;
	}
	free(buffer);
	if (close(fd)) {
		cfg_strprintf(&errmsg, "Error writing kernel file - %s",
		    strerror(errno));
		unlink(SYSLOAD_FILENAME_KERNEL);
		return errmsg;
	}

	return NULL;
}


/**
 * Copy initrd component to local filesystem. On success NULL is returned. On
 * error a dynamically allocated error message is returned.
 *
 * \param[in] disk   Pointer to initialized disk structure
 * \param[in] kernel Block pointer to initrd component
 * \param[in] initrd length found in stage 3 parameter structure
 * \return    In case of error dynamically allocated error message.
 */

static char *
read_initrd_component(struct disk *disk, disk_blockptr_t *initrd,
    int initrd_len)
{
	void *buffer;
	char *errmsg = NULL;
	int fd, buffer_size;

	// open local initrd file
	fd = creat(SYSLOAD_FILENAME_INITRD, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		cfg_strprintf(&errmsg, "Error writing initrd file - %s",
		    strerror(errno));
		return errmsg;
	}

	// write initrd image
	errmsg = copy_component_to_memory(disk, &buffer, &buffer_size, initrd);
	if (errmsg) {
		close(fd);
		unlink(SYSLOAD_FILENAME_INITRD);
		return errmsg;
	}
	if (buffer_size < initrd_len) {
		cfg_strcpy(&errmsg, "Error writing initrd file "
		    "- invalid initrd component length");
		free(buffer);
		close(fd);
		unlink(SYSLOAD_FILENAME_INITRD);
		return errmsg;
	}
	if (write(fd, buffer, initrd_len) != initrd_len) {
		cfg_strprintf(&errmsg, "Error writing initrd file - %s",
		    strerror(errno));
		free(buffer);
		close(fd);
		unlink(SYSLOAD_FILENAME_INITRD);
		return errmsg;
	}
	free(buffer);
	if (close(fd)) {
		cfg_strprintf(&errmsg, "Error writing kernel file - %s",
		    strerror(errno));
		unlink(SYSLOAD_FILENAME_INITRD);
		return errmsg;
	}

	return NULL;
}


/**
 * Read parmfile component. On success NULL is returned. On error a
 * dynamically allocated error message is returned.
 *
 * \param[in]  disk     Pointer to initialized disk structure
 * \param[in]  parmfile Block pointer to parmfile component
 * \param[out] cmdline  Dynamically allocated string with kernel command line
 * \return    In case of error dynamically allocated error message.
 */

static char *
read_parmfile_component(struct disk *disk, disk_blockptr_t *parmfile,
    char **cmdline)
{
	char *errmsg = NULL, *buffer;
	int buffer_size;

	errmsg = copy_component_to_memory(disk, (void **) &buffer,
	    &buffer_size,
	    parmfile);
	if (errmsg)
		return errmsg;
	buffer[buffer_size-1] = '\0';
	cfg_strinitcpy(cmdline, buffer);

	return NULL;
}


/**
 * Prepare command line passed via config file's parmfile and cmdline
 * commands. On success NULL is returned. On error a dynamically allocated
 * error message is returned.
 *
 * \param[in]  boot    Pointer to cfg_bentry structure with boot information.
 * \param[out] cmdline Dynamically allocated string with assembled command line
 * \return     In case of error dynamically allocated error message.
 *
 */

static char *
prepare_config_command_line(struct cfg_bentry *boot, char **cmdline)
{
	char *errmsg = NULL, *tmp_uri, *tmp_msg;

	if (strlen(boot->parmfile)) {
		tmp_uri = prefix_root(boot->root, boot->parmfile);
		if (comp_load(SYSLOAD_FILENAME_PARMFILE, tmp_uri, NULL,
			&tmp_msg)) {
			cfg_strprintf(&errmsg, "Error loading parmfile - %s",
			    tmp_msg);
			cfg_strfree(&tmp_msg);
			cfg_strfree(&tmp_uri);
			return errmsg;
		}
		cfg_strfree(&tmp_uri);
		errmsg = compose_commandline(cmdline, SYSLOAD_FILENAME_PARMFILE,
		    boot->cmdline);
		if (errmsg)
			return errmsg;
	} else
		cfg_strinitcpy(cmdline, boot->cmdline);

	return NULL;
}


/**
 * Load boot objects from disk and boot new system by calling \p kexec()
 * On success NULL is returned. On error a dynamically allocated error
 * message is returned.
 *
 * \param[in] boot    Pointer to cfg_bentry structure with boot information.
 * \param[in] disk    Pointer to initialized disk structure
 * \param[in] program Program number in program table to be booted
 * \return    In case of error dynamically allocated error message.
 */

char *
boot_bootmap_disk(struct cfg_bentry *boot, struct disk *disk, int program)
{
	char *errmsg = NULL, *cmdline, *extra_cmdline;
	int opt, initrd_len;
	disk_blockptr_t *program_table, kernel, initrd, parmfile;
	struct component *component_table;

	// check program entry
	if (program < 0 || program >= get_program_table_size(disk)) {
		cfg_strcpy(&errmsg,
		    "Error - program number out of valid range.");
		return errmsg;
	}

	// load component table for requested program
	errmsg = get_program_table(disk, &program_table);
	if (errmsg)
		return errmsg;
	if (blockptr_is_null(disk, &program_table[program])) {
		cfg_strprintf(&errmsg,
		    "Error - program entry %i is empty.", program);
		return errmsg;
	}
	errmsg = get_component_table(disk, &component_table, &opt,
	    &program_table[program]);
	free(program_table);
	if (errmsg)
		return errmsg;
	if (opt == component_header_dump) {
		cfg_strcpy(&errmsg, "Error - load-with-dump-list-directed"
		    " IPL is not supported.");
		return errmsg;
	}
	if (opt != component_header_ipl) {
		cfg_strcpy(&errmsg, "Error - invalid list directed IPL"
		    " type.");
		return errmsg;
	}

	// load boot objects
	identify_boot_objects(disk, component_table, &kernel, &initrd,
	    &initrd_len, &parmfile);
	free(component_table);
	errmsg = read_kernel_component(disk, &kernel);
	if (errmsg)
		return errmsg;
	if (!blockptr_is_null(disk, &initrd)) {
		errmsg = read_initrd_component(disk, &initrd, initrd_len);
		if (errmsg)
			return errmsg;
	}
	if (!blockptr_is_null(disk, &parmfile)) {
		errmsg = read_parmfile_component(disk, &parmfile, &cmdline);
		if (errmsg)
			return errmsg;
	} else
		cfg_strinit(&cmdline);
	errmsg = prepare_config_command_line(boot, &extra_cmdline);
	if (errmsg) {
		free(cmdline);
		return errmsg;
	}
	cfg_strcat(&cmdline, " ");
	cfg_strcat(&cmdline, extra_cmdline);
	cfg_strfree(&extra_cmdline);

	// call kexec
	if (!blockptr_is_null(disk, &initrd))
		errmsg = kexec(SYSLOAD_FILENAME_KERNEL, SYSLOAD_FILENAME_INITRD,
		    cmdline);
	else
		errmsg = kexec(SYSLOAD_FILENAME_KERNEL, "", cmdline);
	cfg_strfree(&cmdline);

	// if we are still here, something went wrong
	return errmsg;
}
