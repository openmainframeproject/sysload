/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file sysload.conf.l
 * \brief Scanner for sysload.conf files
 *
 * $Id: sysload.conf.l,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */
%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "config_parser.h"
int lineno = 1;

%}
%x STRMODE
%x URIMODE
%x URIRESTMODE
%x UIMODE
%x INCLUDEMODE
%x MODMODE

%{
#define MAX_INCL_DEPTH 20

struct include_stack_entry {
  char *uri;          //!< original URI used to access file
  char *filename;     //!< filename used for local URI copy
  int  lineno;        //!< backup of lineno
  YY_BUFFER_STATE buffer; //!< backup of YY_CURRENT_BUFFER
};

struct include_stack_entry stack[MAX_INCL_DEPTH];
int top = 0;

%}

X [xX]

DIGIT       [0-9]
HEXCH    [0-9A-Fa-f]
ID          [a-zA-Z_][a-zA-Z0-9_.-]*
URIMIDDLE   "://"
WHITESPACE  [ \t]

%%

<STRMODE>.*$ {
  int start;
  dg_printf( DG_MAXIMAL, "string:<%s>\n", yytext );
  BEGIN(INITIAL);
  start = strspn( yytext, " \t");
  cfg_strinitcpy(&yylval,&yytext[start]);
  return T_STRING;
}

<INCLUDEMODE>.*$ {
  int start = 0;
  char *uri = NULL;
  char *localname = NULL;
  FILE *inclfile = NULL;

  start = strspn( yytext, " \t");
  cfg_strinitcpy(&uri,&yytext[start]);
  dg_printf( DG_VERBOSE, "include uri:<%s>\n", uri );
  cfg_strinit(&localname);
  cfg_strprintf(&localname, URI_TEMP_FILENAME, top);

  /* check if max. include depth is already reached */
  if (top >= MAX_INCL_DEPTH) {
    dg_printf( DG_VERBOSE, "max. include depth reached - ignoring include\n");
    goto include_handled;
  }
  else if (parser_active_system() != PA_ACTIVE) {
    dg_printf( DG_VERBOSE, "inactive 'system' section - ignoring include\n");
    goto include_handled;
  }
  else {
    /* save current state */
    stack[top].buffer = YY_CURRENT_BUFFER;
    stack[top].lineno = lineno;

    /* open include uri */
    inclfile = open_incl_uri( uri, localname);

    if (inclfile) { /* OK */
      dg_printf( DG_VERBOSE, "open_incl_uri OK\n");

      yyin = inclfile;
      cfg_strinitcpy(&stack[top].uri, uri);
      cfg_strinitcpy(&stack[top].filename, localname);
      yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE));
      lineno = 1;
      top++;
    }
    else { /* ERROR */
      dg_printf( DG_VERBOSE, "open_incl_uri ERROR\n");

      goto include_handled;
    }
  }

include_handled:
  cfg_strfree(&uri);
  cfg_strfree(&localname);
  BEGIN(INITIAL);
}

<<EOF>> {
  top--;

  if (top < 0) {
    top = 0;
    yyterminate();
  }
  else {
    dg_printf( DG_VERBOSE, "%s: EOF reached\n", stack[top].uri);
    yy_delete_buffer( YY_CURRENT_BUFFER);
    yy_switch_to_buffer( stack[top].buffer);
    lineno = stack[top].lineno;
    cfg_strfree(&stack[top].uri);
    cfg_strfree(&stack[top].filename);
  }
}

<URIMODE>{ID}{URIMIDDLE} {
  dg_printf( DG_MAXIMAL, "uristart: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  BEGIN(URIRESTMODE);
  return T_URISTART;
}

<URIRESTMODE>"("[^)]+")" {
  dg_printf( DG_MAXIMAL, "uribracket: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  BEGIN(STRMODE);
  return T_URIBRACKET;
}

<URIRESTMODE>[^(].*$ {
  dg_printf( DG_MAXIMAL, "uristring: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  BEGIN(INITIAL);
  return T_STRING;
}

{DIGIT}+ { 
  dg_printf( DG_MAXIMAL, "number: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  return T_NUMBER;
}

"0"{X}{HEXCH}+ {
  dg_printf( DG_MAXIMAL, "hexnum: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  return T_HEXNUM;
}

{HEXCH}+"."{HEXCH}+"."{HEXCH}+ {
  dg_printf( DG_MAXIMAL, "busid: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  return T_BUSIDVAL;
}

{HEXCH}+":"{HEXCH}+":"{HEXCH}+":"{HEXCH}+":"{HEXCH}+":"{HEXCH}+ {
  dg_printf( DG_MINIMAL, "macval: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  return T_MACVAL;
}

{DIGIT}+"."{DIGIT}+"."{DIGIT}+"."{DIGIT}+ {
  dg_printf( DG_MAXIMAL, "ipaddr: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  return T_IPADDR;
}

#.*$ { /* comments are not returned to the parser */
  dg_printf( DG_MAXIMAL, "commment: %s\n", yytext );
}

default{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "default: %s\n", yytext );
  BEGIN(STRMODE);
  return T_DEFAULT;
}

timeout{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "timeout: %s\n", yytext );
  return T_TIMEOUT;
}

password{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "password: %s\n", yytext );
  BEGIN(STRMODE);
  return T_PASSWORD;
}

exec{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "exec: %s\n", yytext );
  BEGIN(STRMODE);
  return T_EXEC;
}

userinterface{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "userinterface: %s\n", yytext );
  BEGIN(UIMODE);
  return T_USERINTERFACE;
}

pause{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "pause: %s\n", yytext );
  BEGIN(STRMODE);
  return T_PAUSE;
}

include {
  dg_printf( DG_MAXIMAL, "include: %s\n", yytext );
  BEGIN(INCLUDEMODE);
}

mod {
  BEGIN(MODMODE);
  return T_MOD;
}

boot_entry return T_BOOT_ENTRY;
lock       return T_LOCK;
reboot     return T_REBOOT;
halt       return T_HALT;
exit       return T_EXIT;
shell      return T_SHELL;
network    return T_NETWORK;
knet       return T_KNET;
mode       return T_MODE;
dhcp       return T_DHCP;
static     return T_STATIC;
address    return T_ADDRESS;
mask       return T_MASK;
gateway    return T_GATEWAY;
nameserver return T_NAMESERVER;
interface  return T_INTERFACE;
setup      return T_SETUP;
kset       return T_KSET;
qeth       return T_QETH;
zfcp       return T_ZFCP;
dasd       return T_DASD;
busid      return T_BUSID;
wwpn       return T_WWPN;
lun        return T_LUN;
system     return T_SYSTEM;
uuid       return T_UUID;
mac        return T_MAC;
vmguest    return T_VMGUEST;
lpar       return T_LPAR;
module     return T_MODULE;
true       return T_TRUE;
false      return T_FALSE;
not        return T_NOT;

name{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "name: %s\n", yytext );
  BEGIN(STRMODE);
  return T_NAME;
}

param{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "param: %s\n", yytext );
  BEGIN(STRMODE);
  return T_PARAM;
}

kernelversion{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "kernelversion: %s\n", yytext );
  BEGIN(STRMODE);
  return T_KERNELVERSION;
}

title{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "title: %s\n", yytext );
  BEGIN(STRMODE);
  return T_TITLE;
}

label{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "label: %s\n", yytext );
  BEGIN(STRMODE);
  return T_LABEL;
}

cmdline{WHITESPACE}+ {
  dg_printf( DG_MAXIMAL, "cmdline: %s\n", yytext );
  BEGIN(STRMODE);
  return T_CMDLINE;
}

kernel{WHITESPACE}+ { /* kernel line with string */
  dg_printf( DG_MAXIMAL, "kernel: %s\n", yytext );
  BEGIN(STRMODE);
  return T_KERNEL;
}

kernel{WHITESPACE}+{ID}{URIMIDDLE} { /* kernel line with uri */
  dg_printf( DG_MAXIMAL, "kerneluri: %s\n", yytext );
  yyless(strlen("kernel"));
  BEGIN(URIMODE);
  return T_KERNEL;
}

initrd{WHITESPACE}+ { /* initrd line with string */
  dg_printf( DG_MAXIMAL, "initrd: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  BEGIN(STRMODE);
  return T_INITRD;
}

initrd{WHITESPACE}+{ID}{URIMIDDLE} { /* initrd line with uri */
  dg_printf( DG_MAXIMAL, "initrduri: %s\n", yytext );
  yyless(strlen("initrd"));
  BEGIN(URIMODE);
  return T_INITRD;;
}

root{WHITESPACE}+{ID}{URIMIDDLE}  { /* always an uri */
  dg_printf( DG_MAXIMAL, "rooturi: %s\n", yytext );
  /* remove 'root' and read uri-scheme with next call */
  yyless(strlen("root")); 
  BEGIN(URIMODE);
  return T_ROOT;
}

parmfile{WHITESPACE}+ { /* parmfile line with string */
  dg_printf( DG_MAXIMAL, "parmfile: %s\n", yytext );
  BEGIN(STRMODE);
  return T_PARMFILE;
}

parmfile{WHITESPACE}+{ID}{URIMIDDLE} { /* parmfile line with uri */
  dg_printf( DG_MAXIMAL, "parmfileuri: %s\n", yytext );
  /* remove 'parmfile' - read uri-scheme with next call */
  yyless(strlen("parmfile")); 
  BEGIN(URIMODE);
  return T_PARMFILE;
}

insfile{WHITESPACE}+{ID}{URIMIDDLE}  { /* always an uri */
  dg_printf( DG_MAXIMAL, "insfileuri: %s\n", yytext );
  /* remove 'root' and read uri-scheme with next call */
  yyless(strlen("insfile")); 
  BEGIN(URIMODE);
  return T_INSFILE;
}

bootmap{WHITESPACE}+{ID}{URIMIDDLE}  { /* always an uri */
  dg_printf( DG_MAXIMAL, "bootmapuri: %s\n", yytext );
  /* remove 'root' and read uri-scheme with next call */
  yyless(strlen("bootmap")); 
  BEGIN(URIMODE);
  return T_BOOTMAP;
}

<MODMODE,INITIAL>{ID} {
  dg_printf( DG_MAXIMAL, "identifier: %s\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  return T_IDENT;
}

<UIMODE>ssh {
  return T_SSH;
}

<UIMODE>{ID} {
  cfg_strinit(&yylval);
  dg_printf( DG_MAXIMAL, "uiidentifier:<%s>\n", yylval );
  BEGIN(STRMODE);
  cfg_strinitcpy(&yylval,yytext);
  return T_IDENT;
}

<URIMODE,INITIAL>[ \t]+       /* eat up whitespace */

\n {
  lineno++;
  BEGIN(INITIAL); /* return to normal mode */
}

<MODMODE,INITIAL>[(,/{}=] {
  return yytext[0];
}

<MODMODE>[)] {
  BEGIN(INITIAL);
  return yytext[0];
}

<INITIAL>[)] {
  return yytext[0];
}

<URIMODE,INITIAL>. {
  fprintf( stderr, "unrecognized character '%s'\n", yytext );
  cfg_strinitcpy(&yylval,yytext);
  return T_UNRECOGNIZED;
}

%%

int yywrap()
{
  return 1;
}

void yyerror(char const *msg)
{
  char *tmpstr;
  
  cfg_strinit(&tmpstr);
  cfg_strprintf(&tmpstr,
		"uri:%s:"
		"%s near symbol '%s' in line %d\n", 
		"???URI???",
		msg, yytext, lineno);
  parser_set_errmsg(parser_global_context,tmpstr);
  fprintf(stderr,tmpstr);
  cfg_strfree(&tmpstr);
}
