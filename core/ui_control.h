/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file ui_control.h
 * \brief General definitions for the sysload user interface starter
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 * \author Michael Loehr   (mloehr@de.ibm.com)
 *
 * $Id: ui_control.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _UI_CONTROL_H_
#define _UI_CONTROL_H_

#define UI_CONTROL_H_REVISION		"$Revision: 1.2 $"

#include "config.h"

#define UI_MODULE_PATH  "ui" //!< path to loader module directory
#define UI_KILL_TIMEOUT 2    //!< delay in seconds before sending SIGKILL
                             //!< to non responding user interface modules

int userinterface(const char *startup_msg, struct cfg_toplevel *config,
    struct cfg_bentry *boot);

#endif /* #ifndef _UI_CONTROL_H_ */
