/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file parser.h
 * \brief Include file for sysload config file parser.
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 * \author Michael Loehr   (mloehr@de.ibm.com)
 *
 * $Id: parser.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _PARSER_H_
#define _PARSER_H_

#include "config.h"
#include "netbase.h"
#include "modbase.h"
#include "sysload.h"

#define MAX_SYSTEM_DEPTH 999
#define P_COMMENT '#'       //!< comment character in config files
#define P_WHITESPACE " \t"  //!< string with white space characters
#define P_URI_PARAM_DELIMITER " \t,)"  //!< delimiters for URI params
#define URI_TEMP_FILENAME "/tmp/sysloadparser-%d.cfg"
//!< filename for local copies of URIs

#define SYSINFO_FILENAME "/proc/sysinfo" //!< filename for s390 sysinfo file
#define CMDLINE_FILENAME "/proc/cmdline" //!< filename for kernel commandline

#define VMGUEST_ENTRY "VM00 Name:"  //!< line containing the vmguest name
#define LPAR_ENTRY    "LPAR Name:"  //!< line containing the lpar name

#define YYSTYPE char*


/**
 * Return codes for parser functions.
 */

enum p_return {
  P_ERR = -1,  //!< parsing error detected
  P_OK = 0,    //!< success
  P_EOF = 1,   //!< end of file reached
};


/**
 * result of parse_arguments
 */
enum parse_result {
	PA_VALID,
	PA_DONE,
	PA_ERROR,
};


/**
 * activity status of the parser
 */
enum parse_activity {
	PA_ACTIVE,
	PA_INACTIVE,
};


/**
 * This structure is used to store all arguments passed on the command
 * line when sysload is started.
 */

struct sysload_arguments {
	char *config_uri;    //!< URI to access configuration file
	char *console;       //!< Device node used to output sysload messages
	char *only_ui;       //!< ignode globals and start only this ui
};


/**
 *
 * This structure defines the context reqired by the parser to work.
 */

struct parser_context {
  FILE *file;         //!< config file
  char *uri;          //!< original URI used to access file
  char *filename;     //!< filename used for local URI copy
  char *errmsg;              //!< buffer for parser error messages
  char *boot_default;        //!< buffer for default boot label during parsing
  int  system_depth;         //!< depth of nested system statements
  int  system_first_inactive;    //!< depth of first inactive system statement
  struct sysload_arguments *args;  //!< commandline arguments
  struct cfg_toplevel *toplevel; //!< toplevel built by parser
  struct cfg_bentry *bentry;     //!< temp. bentry info collected by parser
  struct nb_conf *netconf;       //!< temp. netconf info collected by parser
  struct mb_conf *modconf;   //!< temp. module info collected by parser
};

extern struct parser_context *parser_global_context;

void parser_init(struct parser_context *context);
void parser_destroy(struct parser_context *context);
void parser_set_errmsg(struct parser_context *context,const char *str);
void parser_printf_errmsg(struct parser_context *context,
			  const char *format,...);
FILE *open_incl_uri(const char *uri, const char *localname);
enum p_return parser_open_incl_uri(struct parser_context *context,
			       const char *filename);
const char *parser_uimode(void);
int parser_vmguest_test(const char *guestname);
int parser_lpar_test(const char *lparname);
enum parse_activity parser_active_system(void);
void parser_enter_system(enum parse_activity active);
void parser_exit_system(void);
char *parse_sysload(struct cfg_toplevel *config, 
                  struct sysload_arguments *sysload_args);
char *parse_sysload_boot_entry(struct cfg_bentry *config, const char *str);
void parse_kernel_cmdline(void);
enum parse_result parse_arguments(struct sysload_arguments *sysload_args, int argc,
    char **argv);
void parser_setup_qeth(
    const char *busid1, 
    const char *busid2, 
    const char *busid3);
void parser_setup_dasd(const char *busid);
void parser_setup_zfcp(
    const char *busid, 
    const char *wwpn, 
    const char *lun);



#endif /* #ifndef _PARSER_H_ */
