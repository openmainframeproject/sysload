/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file parser_sysload.c
 * \brief Parser for System Loader config files.
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 * \author Michael Loehr   (mloehr@de.ibm.com)
 *
 * $Id: parser_sysload.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "parser.h"
#include "debug.h"
#include "netbase.h"
#include "sysload.h"

int yyparse(void);
int yy_scan_string(const char *str);
void yyrestart( FILE *input_file );
extern FILE *yyin;


/**
 * Checks if sysload should handle only a specific ui and returns
 * a pointer to the name of the ui to be started. If sysload runs
 * runs in general mode, the function returns NULL.
 */

const char *
parser_uimode(void)
{
	if (strlen(parser_global_context->args->only_ui) == 0) {
		dg_printf(DG_VERBOSE,
		    "%s:return NULL\n", __FUNCTION__);
		return NULL;
	}	
	else {
		dg_printf(DG_VERBOSE,
		    "%s:return %s\n", 
		    __FUNCTION__, parser_global_context->args->only_ui);
		return parser_global_context->args->only_ui;
	}
}


int parser_sysinfo_test(const char *entry, const char *guestname)
{
	FILE *file = NULL;
	char input[CFG_STR_MAX_LEN] = ""; /* input line from file */
	char *pos = NULL;

	file = fopen(SYSINFO_FILENAME, "r");
	if (!file) {
		fprintf(stderr, "%s: fopen not possible for %s\n", 
		    arg0, SYSINFO_FILENAME);
		goto not_found;
	}

	/* find and compare the name of the vmguest */
	while (fgets(input, CFG_STR_MAX_LEN, file) != NULL) {
		if (strncmp(input,entry,strlen(entry)) == 0) {
			pos = &input[strlen(entry)];
			pos += strspn(pos," \t");
			pos[strcspn(pos," \t\n\0")] = '\0';
			dg_printf(DG_MAXIMAL,"%s found\n",entry);
			dg_printf(DG_MAXIMAL,"value <%s>\n",pos);
			if (strcasecmp(guestname,pos) == 0) {
				fclose(file);
				return CFG_RETURN_OK;
			}
		}
	}
	fclose(file);

 not_found:
	return CFG_RETURN_ERROR;
}


/**
 * Check if the name of the vmguest matches
 */

int parser_vmguest_test(const char *guestname)
{
	return parser_sysinfo_test( VMGUEST_ENTRY, guestname);
}


/**
 * Check if the name of the lpar matches
 */

int parser_lpar_test(const char *lparname)
{
	return parser_sysinfo_test( LPAR_ENTRY, lparname);
}


/**
 * Checking for parser state in 'system' section. To be called before 
 * any parser action is executed
 *
 */
enum parse_activity parser_active_system(void)
{
	if (parser_global_context->system_depth >= 
	    parser_global_context->system_first_inactive) {
		return PA_INACTIVE;
	} 
	else {
		return PA_ACTIVE;
	}
}


/**
 * To be called at the beginning of every 'system' section.
 *
 */
void parser_enter_system(enum parse_activity active)
{
	parser_global_context->system_depth++;

	dg_printf( DG_MAXIMAL, 
	    "entering 'system' section %d\n", 
	    parser_global_context->system_depth);
	/* parser is not switched to inactive yet */
	if (parser_active_system() == PA_ACTIVE)
		/* switch to inactive now if we enter an inactive section */
		if (active == PA_INACTIVE) {
			dg_printf( DG_MAXIMAL, 
			    "switch to inactive %d\n", 
			    parser_global_context->system_depth);
			parser_global_context->system_first_inactive =
				parser_global_context->system_depth;
		}
}


/**
 * To be called at the end of every 'system' section.
 *
 */
void parser_exit_system(void)
{
	/* we are leaving the outmost inactive 'system' section */
	if (parser_global_context->system_first_inactive ==
	    parser_global_context->system_depth) {
		parser_global_context->system_first_inactive = 
			MAX_SYSTEM_DEPTH;
		dg_printf( DG_MAXIMAL, 
		    "leaving the outmost inactive 'system' section %d\n",
		    parser_global_context->system_depth);
	}
	else {
		dg_printf( DG_MAXIMAL, 
		    "leaving 'system' section %d\n",
		    parser_global_context->system_depth);
	}
		
	parser_global_context->system_depth--;
}


/**
 * Called to enable a qeth device
 */

void parser_setup_qeth(
    const char *busid1, 
    const char *busid2, 
    const char *busid3)
{
	char *cmd = NULL, *defaultpath = NULL;

	cfg_strinit(&cmd);
	cfg_strinit(&defaultpath);
	cfg_get_defaultpath(&defaultpath);
	cfg_strprintf(&cmd,
	    "%s/%s/setup_qeth %s %s %s", 
	    defaultpath, SETUP_MODULE_PATH,
	    busid1, busid2, busid3);
	
	if ((strlen(busid1) != 8) ||
	    (strlen(busid2) != 8) ||
	    (strlen(busid3) != 8))
		dg_printf( DG_MINIMAL, 
		    "invalid parameter in %s\n",
		    cmd);
	
	cfg_system( cmd);
	
	cfg_strfree(&defaultpath);
	cfg_strfree(&cmd);
}

 
/**
 * Called to enable a dasd device
 */

void parser_setup_dasd(const char *busid)
{
	char *cmd = NULL, *defaultpath = NULL;

	cfg_strinit(&cmd);
	cfg_strinit(&defaultpath);
	cfg_get_defaultpath(&defaultpath);
	cfg_strprintf(&cmd,
	    "%s/%s/setup_dasd %s", 
	    defaultpath, SETUP_MODULE_PATH,
	    busid);
	
	if (strlen(busid) != 8)
		dg_printf( DG_MINIMAL, 
		    "invalid parameter in %s\n", cmd);
	
	cfg_system( cmd);
	
	cfg_strfree(&defaultpath);
	cfg_strfree(&cmd);
}


/**
 * Called to enable a zfcp device
 */

void parser_setup_zfcp(
    const char *busid, 
    const char *wwpn, 
    const char *lun)
{
	char *cmd = NULL, *defaultpath = NULL;

	cfg_strinit(&cmd);
	cfg_strinit(&defaultpath);
	cfg_get_defaultpath(&defaultpath);
	cfg_strprintf(&cmd,
	    "%s/%s/setup_zfcp %s %s %s", 
	    defaultpath, SETUP_MODULE_PATH,
	    busid, wwpn, lun);
	
	if ((strlen(busid) != 8) ||
	    (strlen(wwpn) != 18) ||
	    (strlen(lun)  != 18))
		dg_printf( DG_MINIMAL, 
		    "invalid parameter in %s\n", cmd);
	
	cfg_system( cmd);
	
	cfg_strfree(&defaultpath);
	cfg_strfree(&cmd);
}

 
/**
 * Parse sysload configuration file provided in parser context and update
 * configuration structure.
 *
 * \param[out] config    Pointer to cfg_toplevel structure to be configured.
 * \param[in]  context   Pointer to parser_context structure.
 * \return     \p P_OK on success, \p P_ERR on error
 */

static enum p_return
parse_global(struct cfg_toplevel *config, struct parser_context *context)
{
	enum p_return pret;

        /* flex setup */
	yyin = context->file;

	yyrestart( context->file);

	dg_showfile(DG_VERBOSE, yyin, 8);
	parser_global_context = context;
	parser_global_context->toplevel = config;
	
	if (yyparse() == 0) 
		pret = P_OK;
	else
		pret = P_ERR;;

	return pret;
}


/**
 * Parse sysload configuration file identified by URI. On success \p NULL
 * is returned. If the configuration file contains errors the return
 * value is a dynamically allocated string with the parser error message.
 * The caller is responsible to free this memory buffer with \p free().
 *
 * \param[out] config   Pointer to unintialized cfg_toplevel structure
 *                      to return parser output.
 * \param[in]  uri      String with configuration file URI.
 * \return     \p NULL on success, in case of error dynamically allocated
 *             error message.
 */

char *
parse_sysload(struct cfg_toplevel *config, struct sysload_arguments *sysload_args)
{
	char *errmsg = NULL;
	int idx;
	struct parser_context context;

	DG_ENTER( DG_VERBOSE);
	cfg_init(config);
	parser_init(&context);
	context.args = sysload_args;

	if (parser_open_incl_uri(&context, sysload_args->config_uri)) {
		cfg_strinit(&errmsg);
		cfg_strprintf(&errmsg, "Error accessing config file '%s': %s",
		    sysload_args->config_uri, context.errmsg);
		goto cleanup;
	}

	if (parse_global(config, &context)) {
		cfg_strinit(&errmsg);
		cfg_strprintf(&errmsg,
		    "Error parsing config URI '%s': %s",
		    context.uri,
		    context.errmsg);
		goto cleanup;
	}

	// resolve default boot label
	if (strspn(context.boot_default, "0123456789") ==
	    strlen(context.boot_default) &&
	    sscanf(context.boot_default, "%d", &idx) == 1) {
		if (idx<0 || idx>=config->bentry_count) {
			cfg_strinitcpy(&errmsg, "Invalid default boot index.");
			goto cleanup;
		}
		config->boot_default = idx;
	} else {
		for (idx = 0; idx<config->bentry_count; idx++) {
			if (strcmp(context.boot_default,
				config->bentry_list[idx].label) == 0) {
				config->boot_default = idx;
				break;
			}
		}
		if (idx == config->bentry_count) {
			cfg_strinitcpy(&errmsg, "Invalid default boot label.");
			goto cleanup;
		}
	}

 cleanup:
	if (errmsg != NULL)
		dg_printf(DG_VERBOSE, "%s:errmsg=%s\n", __FUNCTION__, errmsg);

	parser_destroy(&context);
	DG_RETURN( DG_VERBOSE, errmsg);
}


/**
 * Parse sysload boot entry provided in \p str. On success \p NULL
 * is returned. If the boot entry contains errors the return value
 * is a dynamically allocated string with the parser error message.
 * The caller is responsible to free this memory buffer with \p free().
 *
 * \param[out] bentry   Pointer to uninitialized cfg_bentry structure
 *                      to return parser output.
 * \param[in]  str      String with boot entry commands
 * \return     \p NULL on success, in case of error dynamically allocated
 *             error message.
 */

char *parse_sysload_boot_entry(struct cfg_bentry *bentry, const char *str)
{
	char *errmsg = NULL;
	struct parser_context context;

	DG_ENTER( DG_VERBOSE);
	dg_printf(DG_VERBOSE, "%s:str=<%s>\n", __FUNCTION__, str);
	cfg_bentry_init(bentry);
	parser_init(&context);

	/* flex setup */

	yy_scan_string( str);

	parser_global_context = &context;
	parser_global_context->toplevel = cfg_new();
	
	if (yyparse() == 0) {
		cfg_bentry_copy(bentry,
		    &(parser_global_context->toplevel->bentry_list[0]));
	}
	else {
		cfg_strinit(&errmsg);
		cfg_strprintf(&errmsg,
		    "Error parsing boot entry.");
	};

	free(parser_global_context->toplevel);

	if (errmsg != NULL)
		dg_printf(DG_VERBOSE, "%s:errmsg=%s\n", __FUNCTION__, errmsg);

	parser_destroy(&context);
	DG_RETURN( DG_VERBOSE, errmsg);
}


/**
 * Parse arguments passed on the command line to sysload and store parsed
 * arguments in \p sysload_args. In case of command line errors return
 * PA_ERROR.
 *
 * \param[out] sysload_args  Pointer to \p sysload_arguments structure with parsed
 *  command line arguments.
 * \param[in]  argc   Number of arguments passed to program.
 * \param[in]  argv   Array with arguments passed to program.
 */

enum parse_result
parse_arguments(struct sysload_arguments *sysload_args, int argc, char **argv)
{
	char *optstring="ou:vh";
	int c;

	// initialize arguments
	memset(sysload_args, 0x0, sizeof(*sysload_args));
	cfg_strinit(&sysload_args->config_uri);
	cfg_strinit(&sysload_args->console);
	cfg_strinit(&sysload_args->only_ui);

	opterr=1;
	optind=1;
	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'o':
			cfg_strcpy(&sysload_args->console, optarg);
			break;

		case 'u':
			cfg_strcpy(&sysload_args->only_ui, optarg);
			dg_printf(DG_VERBOSE,
			    "%s:starting with only_ui = %s\n", 
			    __FUNCTION__, sysload_args->only_ui);
			break;

		case 'v':
			printf("sysload user interface version %s\n"
			    "Written by Ralph Wuerthner and Michael Loehr.\n",
			    SYSLOAD_UI_VERSION);
			return PA_DONE;

		case 'h':
			print_help(argv[0]);
			return PA_DONE;

		case ':':
			fprintf(stderr, "%s: option requires an argument"
			    "-- %c\n", argv[0], optopt);
			fprintf(stderr, "Try `%s -h' for more information.\n",
			    argv[0]);
			return PA_ERROR;
		}
	}

	if (optind != argc-1) {
		fprintf(stderr, "%s: invalid argument(s). "
		    "Try `%s -h' for more information.\n",
		    argv[0], argv[0]);
		return PA_ERROR;
	}

	cfg_strcpy(&sysload_args->config_uri, argv[optind]);
	return PA_VALID;
}


void parse_kernel_argument(const char *karg)
{
	char *errmsg = NULL;
	struct parser_context context;

	dg_printf(DG_VERBOSE, "%s:karg=<%s>\n", __FUNCTION__, karg);
	parser_init(&context);

	/* flex setup */

	yy_scan_string( karg);

	parser_global_context = &context;
	parser_global_context->toplevel = cfg_new();
	
	if (yyparse() == 0) {
		dg_printf(DG_VERBOSE, "%s:yyparse successfull\n",
		    __FUNCTION__);
	}
	else {
		cfg_strinit(&errmsg);
		cfg_strprintf(&errmsg,
		    "Error parsing kernel argument.");
	};

	free(parser_global_context->toplevel);

	if (errmsg != NULL)
		dg_printf(DG_VERBOSE, "%s:errmsg=%s\n", __FUNCTION__, errmsg);

	parser_destroy(&context);
	parser_global_context = NULL; // just for safety reasons	
}


/**
 * search, evaluate and apply additional sysload config info on the kernel
 * command line. 
 */

void parse_kernel_cmdline(void)
{
	FILE *file = NULL;
	char input[CFG_STR_MAX_LEN] = ""; /* input line from cmdline */
	char *pos = NULL;
	char *param = NULL;
	int paramlen = 0;

	file = fopen(CMDLINE_FILENAME, "r");
	if (!file) {
		fprintf(stderr, "%s: fopen not possible for %s\n", 
		    arg0, CMDLINE_FILENAME);
		return;
	}

	dg_printf(DG_MAXIMAL,"fopen done\n");
	cfg_strinit(&param);
	/* find and compare the name of the vmguest */
	while (fgets(input, CFG_STR_MAX_LEN, file) != NULL) {
		dg_printf(DG_VERBOSE,"input:%s",input);
		pos = input;
		while ((pos = strstr(pos,"kset=")) != NULL) {
			paramlen = strcspn(pos," \t\n\0");
			cfg_strncpy(&param,pos,paramlen);
			pos += paramlen;
			dg_printf(DG_VERBOSE,"value <%s>\n",param);
			parse_kernel_argument(param);
		}
		pos = input;
		while ((pos = strstr(pos,"knet=")) != NULL) {
			paramlen = strcspn(pos," \t\n\0");
			cfg_strncpy(&param,pos,paramlen);
			pos += paramlen;
			dg_printf(DG_VERBOSE,"value <%s>\n",param);
			parse_kernel_argument(param);
		}
	}
	cfg_strfree(&param);
	fclose(file);

}
