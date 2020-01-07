/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file loader.h
 * \brief Include file for sysload loader module.
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 *
 * $Id: loader.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _LOADER_H_
#define _LOADER_H_

#include "config.h"

#define SYSLOAD_MAX_KERNEL_CMDLINE 4096
//!< maximum kernel command line length
#define SYSLOAD_KEXEC_CMD "/sbin/kexec"
//!< path to kexec tool
#define SYSLOAD_REBOOT_CMD "/sbin/reboot"
//!< path to reboot command
#define SYSLOAD_HALT_CMD "/sbin/halt"
//!< path to halt command
#define SYSLOAD_SHELL_CMD "/bin/sh"
//!< path to shell
#define SYSLOAD_FILENAME_KERNEL    "/tmp/kernel.img"
//!< filename for local kernel image
#define SYSLOAD_FILENAME_INITRD    "/tmp/initrd.img"
//!< filename for local initrd image
#define SYSLOAD_FILENAME_PARMFILE  "/tmp/parmfile.txt"
//!< filename for local parmfile copy
#define SYSLOAD_FILENAME_INSFILE  "/tmp/file.ins"
//!< filename for local insfile copy


char *loader(struct cfg_bentry *boot);
char *prefix_root(const char *root, const char *uri);
char *compose_commandline(char **final_cmdline, const char *parmfile,
			  const char *cmdline);
char *kexec(const char *kernel, const char *initrd, const char *cmdline);
char *action_insfile_boot(struct cfg_bentry *boot);
char *action_bootmap_boot(struct cfg_bentry *boot);
char *action_shell();

#endif /* #ifndef _LOADER_H_ */
