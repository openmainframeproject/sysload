/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file sysload.h
 * \brief Include file for System Loader user interface
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 *
 * $Id: sysload.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _SYSLOAD_H_
#define _SYSLOAD_H_

#define SYSLOAD_VERSION "1.0.0"
#define SYSLOAD_UI_VERSION "0.1"  //!< sysload user interface version
#define WORLD_EXISTS 1            //!< otherwise we won't be here

#define COMP_LOAD_MODULE_PATH "cl"     //!< path to loader module directory
#define SETUP_MODULE_PATH     "setup"  //!< path to setup module directory

#include "config.h"
#include "debug.h"
#include "parser.h"
#include "loader.h"


void print_help(const char *arg0);
void redirect_output(const char *device);
int userinterface(const char *startup_msg, struct cfg_toplevel *config,
    struct cfg_bentry *boot);
int comp_load(const char *dest, const char *uri, char **info, char **errmsg);

#endif /* #ifndef _SYSLOAD_H_ */
