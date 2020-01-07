/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file config.h
 * \brief Include file for System Loader configuration data module.
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 * \author Michael Loehr   (mloehr@de.ibm.com)
 *
 * $Id: config.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


/*
 * IMPORTANT NOTICE:
 *
 * - if members of configuration structures are added or removed, don't
 *   forget to modify the appropriate _init, _destroy, _copy, ...
 *   functions in config.c
 *
 * - if modifications to configuration structures result in changes to
 *   the interface to user interface modules don't forget to update
 *   CFG_CONFIG_VERSION.
 */

#define CFG_CONFIG_VERSION "1.0" //!< version of the configuration structure

#define CFG_STR_MAX_LEN 512 //!< max. length for simple strings

#define CFG_RETURN_OK     0 //!< returned on success
#define CFG_RETURN_ERROR -1 //!< returned on error

#define CFG_PREFIX       "SYSLOAD"         //!< prefix for environment variables
#define CFG_STARTUP_MSG  "STARTUP_MESSAGE" //!< strings for env. variables
#define CFG_VERSION      "VERSION"
#define CFG_DEFAULT      "DEFAULT"
#define CFG_TIMEOUT      "TIMEOUT"
#define CFG_PASSWORD     "PASSWORD"
#define CFG_BENTRY_COUNT "BENTRY_COUNT"
#define CFG_TITLE        "TITLE"
#define CFG_LABEL        "LABEL"
#define CFG_ROOT         "ROOT"
#define CFG_KERNEL       "KERNEL"
#define CFG_INITRD       "INITRD"
#define CFG_CMDLINE      "CMDLINE"
#define CFG_PARMFILE     "PARMFILE"
#define CFG_INSFILE      "INSFILE"
#define CFG_BOOTMAP      "BOOTMAP"
#define CFG_LOCKED       "LOCKED"
#define CFG_PAUSE        "PAUSE"
#define CFG_ACTION       "ACTION"

#define CFG_PATH         "PATH"             //!< to set the sysload base path from
                                            //!< environment (export SYSLOAD_PATH=...)
#define CFG_DEFAULTPATH  "/usr/sysload"     //!< if no environment variable is set
#define CFG_PIDFILE      "/tmp/sysload.pid" //!< pid of the primary sysload process

#define MEM_ASSERT(ptr) {						\
		if ((ptr) == NULL) {					\
			fprintf(stderr,					\
			    "%s: memory allocation failed in module '%s'," \
			    " function '%s', line %d\n", arg0,		\
			    __FILE__, __FUNCTION__, __LINE__);		\
			abort();					\
		}							\
	} //!< assert a failed memory allocation
extern char *arg0; //!< global variable with pointer to argv[0]


/**
 * Enumeration of all possible boot actions
 */
enum boot_action {
	KERNEL_BOOT,
	INSFILE_BOOT,
	BOOTMAP_BOOT,
	REBOOT,
	HALT,
	SHELL,
	EXIT,
};


/**
 * This structure defines one user interface instance.
 */

struct cfg_userinterface {
	char *module;  //!< module name
	char *cmdline; //!< module arguments
};


/**
 * This structure describes one boot menu entry.
 */

struct cfg_bentry {
	char *title;    //!< boot item name
	char *label;    //!< label to be referenced with \p default command
	char *root;     //!< URI prefix for paths
	char *kernel;   //!< kernel image URI
	char *initrd;   //!< initrd image URI
	char *cmdline;  //!< kernel command line
	char *parmfile; //!< parmfile URI
	char *insfile;  //!< insfile URI
	char *bootmap;  //!< boot table URI
	int locked;     //!< entry is locked
	char *pause;    //!< display message and wait for user input
	enum boot_action action; //!< boot action
};


/**
 * This structure describes all toplevel configuration settings.
 */

struct cfg_toplevel {
	int boot_default; //!< default boot entry
	int timeout;      //!< timeout in seconds before booting default entry
	char *password;   //!< password to access locked boot entries
	struct cfg_userinterface *ui_list; //!< list of user interface
	                                   //!< instances
	int ui_count;     //!< number of user interface instance entries
	struct cfg_bentry *bentry_list; //!< list of boot entries
	int bentry_count; //!< number of boot entries
};

struct cfg_toplevel* cfg_new();
void cfg_init(struct cfg_toplevel *config);
void cfg_destroy(struct cfg_toplevel *config);
void cfg_userinterface_list_destroy(struct cfg_toplevel *config);
void cfg_bentry_list_destroy(struct cfg_toplevel *config);

void cfg_copy(struct cfg_toplevel *dest, const struct cfg_toplevel *src);
void cfg_initcopy(struct cfg_toplevel *dest, const struct cfg_toplevel *src);
void cfg_add_userinterface(struct cfg_toplevel *config,
    const struct cfg_userinterface *ui);
void cfg_add_bentry(struct cfg_toplevel *config,
    const struct cfg_bentry *bentry);

struct cfg_userinterface *cfg_userinterface_new();
void cfg_userinterface_init(struct cfg_userinterface *ui);
void cfg_userinterface_destroy(struct cfg_userinterface *ui);
void cfg_userinterface_copy(struct cfg_userinterface *dest,
    const struct cfg_userinterface *src);
void cfg_userinterface_initcopy(struct cfg_userinterface *dest,
    const struct cfg_userinterface *src);
void cfg_userinterface_set(struct cfg_userinterface *dest,
    char* module_name, char* cmd);

struct cfg_bentry *cfg_bentry_new();
void cfg_bentry_init(struct cfg_bentry *bentry);
void cfg_bentry_destroy(struct cfg_bentry *bentry);
void cfg_bentry_free(struct cfg_bentry **bentry);
void cfg_bentry_copy(struct cfg_bentry *dest, const struct cfg_bentry *src);
void cfg_bentry_initcopy(struct cfg_bentry *dest,
    const struct cfg_bentry *src);
void cfg_print(struct cfg_toplevel *config);
void cfg_bentry_print( struct cfg_bentry* bentry);
void cfg_set_env_str(const char *variable, const char *value);
void cfg_get_env_str(const char *variable, char **value);
void cfg_set_env_int(const char *variable, int value);
void cfg_get_env_int(const char *variable, int *value);
void cfg_set_env(struct cfg_toplevel *config);
void cfg_get_env(struct cfg_toplevel *config);

void cfg_strinit(char **str);
void cfg_strfree(char **str);
void cfg_strcpy(char **dest, const char *src);
void cfg_strcat(char **dest, const char *src);
void cfg_strncpy(char **dest, const char *src, size_t len);
void cfg_strinitcpy(char **dest, const char *src);
void cfg_strprintf(char **str, const char *format, ...);
void cfg_strvprintf(char **str, const char *format, va_list arg);

ssize_t cfg_read(int fd, void *buf, size_t count);
int cfg_system(const char *cmd);
void cfg_get_defaultpath(char **defpath);

int cfg_glob_filename(char* pattern, char* buffer, size_t max_length);

#endif /* #ifndef _CONFIG_H_ */
