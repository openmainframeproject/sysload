/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file insfile.h
 * \brief General definitions for the insfile loader
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 * \author Michael Loehr   (mloehr@de.ibm.com)
 *
 * $Id: insfile.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#ifndef _INSFILE_H_
#define _INSFILE_H_

#include "config.h"

/**
 * types of components referenced by *.ins-file
 */
enum ins_component_type {
  INS_KERNEL,     //!< must be the first entry of this enum!
  INS_INITRD,     //!< component was identified as initrd
  INS_PARMFILE,   //!< component was identified as parmfile
  INS_INITRDSIZE, //!< component was identified as initrd size
  INS_OTHER,      //!< component was not identified
  INS_LAST //!< must be the last entry of this enum!
};

char *action_insfile_boot(struct cfg_bentry *boot);

#endif /* #ifndef _INSFILE_H_ */
