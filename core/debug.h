/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file debug.h
 * \brief Include file for debug functionality
 *
 * \author Michael Loehr (mloehr@de.ibm.com)
 *
 * $Id: debug.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <syslog.h>        //->syslog

#define DG_POSITION(level) {						\
			dg_printf(level,				\
			    "%s: '%s', '%s', line %d ",			\
			    arg0, __FILE__, __FUNCTION__, __LINE__);	\
	} //!< print position message


#define DG_ENTER(level) {						\
		DG_POSITION(level);					\
		dg_printf(level, "entering\n");				\
	} //!< entering code area


#define DG_EXIT(level) {						\
		DG_POSITION(level);					\
		dg_printf(level, "exiting\n");				\
	} //!< exiting code area


#define DG_RETURN(level, retval) {					\
		DG_POSITION(level);					\
		dg_printf(level, "returning\n");			\
		return(retval);						\
	} //!< exiting code area via return


/**
 * debug levels
 */
enum debug_level {
	DG_OFF,
	DG_MINIMAL,
	DG_VERBOSE,
	DG_MAXIMAL,
};


extern enum debug_level dg_level; //!< active debug level
extern int dg_indlevel;           //!< indentation level

void dg_init();
void dg_printf( enum debug_level level,  const char *format, ...);
void dg_wait( enum debug_level level, int seconds);
int dg_redirect( const char *name);
void dg_showfile( enum debug_level level, FILE *f, int lines);

#endif /* #ifndef _DEBUG_H_ */
