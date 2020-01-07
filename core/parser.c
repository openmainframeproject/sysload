/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file parser.c
 * \brief General functions for parsing config files.
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 * \author Michael Loehr   (mloehr@de.ibm.com)
 *
 * $Id: parser.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "sysload.h"


#define INITIAL_LINE_BUF 2 //!< initial size of buffer for reading lines

struct parser_context *parser_global_context = NULL;


/**
 * Initialize a parser_context structure.
 *
 * \param[in] context Pointer to parser_context structure to be initialized.
 */

void
parser_init(struct parser_context *context)
{
	memset(context, 0x0, sizeof(*context));

	// initialize string members
	cfg_strinit(&context->uri);
	cfg_strinit(&context->filename);
	cfg_strinit(&context->errmsg);
	cfg_strinitcpy(&context->boot_default, "0");

	// initialize member structures
	context->system_depth = 0;
	context->system_first_inactive = MAX_SYSTEM_DEPTH;
	
	context->bentry = cfg_bentry_new();
	context->netconf = nb_conf_new();
	context->modconf = mb_conf_new();
}


/**
 * Destroy a parser_context structure and free all alocated memory.
 *
 * \param[in] context Pointer to parser_context structure to be destroyed.
 */

void
parser_destroy(struct parser_context *context)
{
	// destroy string members
	cfg_strfree(&context->uri);
	cfg_strfree(&context->filename);
	cfg_strfree(&context->errmsg);
	cfg_strfree(&context->boot_default);

	// destroy member structures
	nb_conf_destroy(context->netconf);
	mb_conf_destroy(context->modconf);
	cfg_bentry_destroy(context->bentry);
	nb_conf_free(&context->netconf);
	mb_conf_free(&context->modconf);
	cfg_bentry_free(&context->bentry);
}


/**
 * Set error message string in parser context.
 *
 * \param[in] context Pointer to parser_context structure
 * \param[in] str Error message string to be set in parser context
 */

void parser_set_errmsg(struct parser_context *context, const char *str)
{
	cfg_strcpy(&context->errmsg, str);
}


/**
 * Set error message string in parser context using a printf style interface.
 *
 * \param[in] context  Pointer to parser_context structure.
 * \param[in] format   printf format string.
 * \param[in] ...      Additional printf arguments.
 */

void parser_printf_errmsg(struct parser_context *context,
    const char *format, ...)
{
	va_list arg;

	va_start(arg, format);
	cfg_strvprintf(&context->errmsg, format, arg);
}


FILE *open_incl_uri(const char *uri, const char *localname)
{
	FILE *urifile = NULL;
	char *errmsg = NULL;

	// create local copy of URI
	if (comp_load(localname, uri, NULL, &errmsg)) {
		if (errmsg) {
			fprintf( stderr,
			    "unable to open URI - %s", errmsg);
			free(errmsg);
		} 
		else {
			fprintf( stderr,
			    "error while opening URI");
		}
		return NULL;
	}

	// open local copy
	urifile = fopen(localname, "r");
	if (urifile == NULL) {
		fprintf( stderr,
		    "unable to open local URI copy '%s' - %s",
		    localname, strerror(errno));
		return NULL;
	}

	return urifile;
}

/**
 * Open include file. Any subsequent calls to \p parser_getstr will
 * read data from this URI.
 *
 * \param[in] context Pointer to parser_context structure.
 * \param[in] uri     String with URI to be opened.
 * \return    \p P_OK on success, \p P_ERR on error.
 */

enum p_return
parser_open_incl_uri(struct parser_context *context, const char *uri)
{
	char *errmsg = NULL;

	// create local copy of URI
	cfg_strprintf(&context->filename, URI_TEMP_FILENAME, 999);
	if (comp_load(context->filename, uri, NULL, &errmsg)) {
		if (errmsg) {
			parser_printf_errmsg(context,
			    "unable to open URI - %s", errmsg);
			free(errmsg);
		} else
			parser_set_errmsg(context, "error while opening URI");
		return P_ERR;
	}

	// open local copy
	context->file = fopen(context->filename, "r");
	if (context->file == NULL) {
		parser_printf_errmsg(context,
		    "unable to open local URI copy '%s' - %s",
		    context->filename, strerror(errno));
		return P_ERR;
	}
	cfg_strcpy(&context->uri, uri);

	return P_OK;
}


