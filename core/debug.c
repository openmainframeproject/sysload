/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file debug.c
 * \brief debugging functions
 *
 * \author Michael Loehr (mloehr@de.ibm.com)
 *
 * $Id: debug.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <sys/types.h> 
#include <unistd.h> 
#include "config.h"
#include "debug.h"


enum debug_level dg_level = DG_OFF; //!< active debug level
int dg_indlevel = 0;                //!< indentation level


void dg_init()
{
	syslog(LOG_INFO, "dg_init()");
}


void dg_strvprintf(char **str, const char *format, va_list arg)
{
	va_list arg2;
	size_t len;

	va_copy(arg2, arg);
	len = vsnprintf(NULL, 0, format, arg2)+1;
	va_end(arg2);
	if (len >= 0) {
		*str = realloc(*str, len);
		MEM_ASSERT(*str);
		vsnprintf(*str, len, format, arg);
		va_end(arg);
	}
}


void dg_printf( enum debug_level level,  const char *format, ...)
{
	va_list arg;
	char *outstr;

	if (level <= dg_level) {
		/*
		for (i=0; i<dg_indlevel; i++) {
			fprintf(dg_output, " ");
		}
		*/

		cfg_strinit(&outstr);
		va_start(arg, format);
		dg_strvprintf(&outstr, format, arg);
		va_end(arg);
		syslog(LOG_INFO, outstr);
		printf(outstr);
		cfg_strfree(&outstr);
	}
}


void dg_wait( enum debug_level level, int seconds)
{
	struct timeval timeout;   /* for waiting with select */

	if (level <= dg_level) {
		syslog(LOG_INFO, "waiting for %d seconds!", seconds);
		timeout.tv_usec = 0;  /* milliseconds */
		timeout.tv_sec = seconds;  /* seconds */

		select( 0, NULL, NULL, NULL, &timeout);
	}
}


int dg_redirect( const char *name)
{
	return CFG_RETURN_OK;
}


void dg_showfile( enum debug_level level, FILE *f, int lines)
{
	int i = 0;
	char input[CFG_STR_MAX_LEN] = ""; /* input line from f */

	dg_printf(level,"vvv file vvv\n");
	while ((fgets(input, CFG_STR_MAX_LEN, f) != NULL) &&
	    (i < lines)) {
		i++;
		dg_printf(level,input);
	}
	dg_printf(level,"^^^ file ^^^\n");
	rewind(f);
}
