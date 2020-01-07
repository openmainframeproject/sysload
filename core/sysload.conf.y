/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file sysload.conf.y
 * \brief Grammar for the sysload config file
 *
 * $Id: sysload.conf.y,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */
%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "parser.h"
#include "sysload.h"

	int yylex(void);
	void yyerror(char const *msg);
%}

/* BISON Declarations */
%error-verbose
%debug

%token T_DEFAULT
%token T_TIMEOUT
%token T_PASSWORD
%token T_INCLUDE
%token T_EXEC
%token T_SETUP
%token T_KSET
%token T_USERINTERFACE
%token T_SSH
%token T_MODULE
%token T_MOD
%token T_NAME
%token T_PARAM
%token T_KERNELVERSION
%token T_BOOT_ENTRY
%token T_TITLE
%token T_LABEL
%token T_ROOT
%token T_KERNEL
%token T_INITRD
%token T_CMDLINE
%token T_PARMFILE
%token T_INSFILE
%token T_BOOTMAP
%token T_LOCK
%token T_PAUSE
%token T_HALT
%token T_SHELL
%token T_EXIT
%token T_REBOOT
%token T_LINEMODE
%token T_STRING
%token T_IDENT
%token T_NUMBER
%token T_HEXNUM
%token T_BUSID
%token T_BUSIDVAL
%token T_DASD
%token T_ZFCP
%token T_WWPN
%token T_LUN
%token T_QETH
%token T_NETWORK
%token T_KNET
%token T_DHCP
%token T_STATIC
%token T_MODE
%token T_MASK
%token T_IPADDR
%token T_URISTART
%token T_URIBRACKET
%token T_ADDRESS
%token T_GATEWAY
%token T_NAMESERVER
%token T_INTERFACE
%token T_SYSTEM
%token T_UUID
%token T_MAC
%token T_MACVAL
%token T_VMGUEST
%token T_LPAR
%token T_TRUE
%token T_FALSE
%token T_NOT
%token T_UNRECOGNIZED
/* Note: include will be handled by the scanner */

/* Grammar follows */
%%

/* 
 * a system loader config file is a list of definitions
 */

input: 

  deflist
    {
	    /* cfg_print(parser_global_context->toplevel); */
    }  
    /* knet=dhcp,interface or
       knet=static,interface,address[,mask[,gateway[,nameserver]]] */
  | T_KNET '=' knet_params
    {
	    nb_conf_enable(parser_global_context->netconf);
	    /* reset for the next netconf */
	    nb_conf_reset(parser_global_context->netconf);
    }
  | T_KSET '=' kset_paramlist
;

kset_paramlist:

    kset_param
  | kset_paramlist ',' kset_param
;

kset_param:

    T_DASD '(' T_BUSIDVAL ')'
    {
	    parser_setup_dasd($3);
	    cfg_strfree(&$3);
    }  
  | T_QETH '(' T_BUSIDVAL ',' T_BUSIDVAL ',' T_BUSIDVAL ')'
    {
	    parser_setup_qeth($3,$5,$7);
	    cfg_strfree(&$3);
	    cfg_strfree(&$5);
	    cfg_strfree(&$7);
    }  
  | T_ZFCP '(' T_BUSIDVAL ',' T_HEXNUM ',' T_HEXNUM ')'
    {
	    parser_setup_zfcp($3,$5,$7);
	    cfg_strfree(&$3);
	    cfg_strfree(&$5);
	    cfg_strfree(&$7);
    }  
  | T_DHCP '(' net_rest ')'
    {
	    parser_global_context->netconf->mode = NB_DHCP;
	    nb_conf_enable(parser_global_context->netconf);
	    /* reset for the next netconf */
	    nb_conf_reset(parser_global_context->netconf);
    }
  | T_STATIC '(' net_rest ')'  
    {
	    parser_global_context->netconf->mode = NB_STATIC;
	    nb_conf_enable(parser_global_context->netconf);
	    /* reset for the next netconf */
	    nb_conf_reset(parser_global_context->netconf);
    }
  | T_MOD '(' T_IDENT ')'
    {
	    cfg_strcpy(&parser_global_context->modconf->name, $3);
	    mb_conf_load(parser_global_context->modconf);
	    /* reset for the next modconf */
	    mb_conf_reset(parser_global_context->modconf);
	    cfg_strfree(&$3);
    }  
;

net_rest:

    T_IDENT addr_rest
    {
	    cfg_strcpy(&parser_global_context->netconf->interface, $1);
	    cfg_strfree(&$1);
    }
;

addr_rest:

    /* empty */
  | ',' T_IPADDR mask_rest
    {
	    cfg_strcpy(&parser_global_context->netconf->address, $2);
	    cfg_strfree(&$2);
    }
;

knet_params:

    T_DHCP   ',' T_IDENT
    {
	    parser_global_context->netconf->mode = NB_DHCP;
	    cfg_strcpy(&parser_global_context->netconf->interface, $3);
	    cfg_strfree(&$3);
    }
  | T_STATIC ',' T_IDENT ',' T_IPADDR mask_rest
    {
	    parser_global_context->netconf->mode = NB_STATIC;
	    cfg_strcpy(&parser_global_context->netconf->interface, $3);
	    cfg_strcpy(&parser_global_context->netconf->address, $5);
	    cfg_strfree(&$3);
	    cfg_strfree(&$5);
    }
;

mask_rest:

    /* empty */
  | ',' T_IPADDR gateway_rest
    {
	    cfg_strcpy(&parser_global_context->netconf->mask, $2);
	    cfg_strfree(&$2);
    }
;

gateway_rest:

    /* empty */
  | ',' T_IPADDR nameserver_rest
    {
	    cfg_strcpy(&parser_global_context->netconf->gateway, $2);
	    cfg_strfree(&$2);
    }
;

nameserver_rest:

    /* empty */
  | ',' T_IPADDR
    {
	    cfg_strcpy(&parser_global_context->netconf->nameserver, $2);
	    cfg_strfree(&$2);
    }
;

deflist:   

    def
  | deflist def
;


/* 
 * possible definition types are: 
 * global definitions, boot entries or a list of system dependent definitions
 */

def:

    globaldef
  | bootentry
  | system '{' deflist '}'
    {
	    parser_exit_system();
    }  
| system '{' '}' /* handle include in inactive system section */
    {
	    parser_exit_system();
    }  
;

globaldef:   

    T_DEFAULT T_STRING
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->boot_default, $2);
	    }
	    cfg_strfree(&$2);
    }
  | T_TIMEOUT T_NUMBER
    {
	    if ((parser_uimode()!=NULL) ||
		(parser_active_system() != PA_ACTIVE)) { 
		    dg_printf(DG_VERBOSE,"%s:ignoring timeout\n",
			__FUNCTION__);
		    parser_global_context->toplevel->timeout = 0;
	    }
	    else {
		    sscanf($2, "%d", 
			&parser_global_context->toplevel->timeout);
		    dg_printf(DG_VERBOSE,"%s:timeout set to %d\n",
			__FUNCTION__, 
			parser_global_context->toplevel->timeout);
	    }
	    cfg_strfree(&$2);
    }
  | T_PASSWORD T_STRING
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->toplevel->password, $2);
	    }
	    cfg_strfree(&$2);
    }
  | setup
  | network
  | userinterface
  | exec
;


/*
 * the setup statements can be used to load modules
 * and to set dasd, qeth and zfcp devices online
 */

setup:   

    setup_module
  | setup_dasd
  | setup_qeth
  | setup_zfcp
;

setup_module:

  T_SETUP T_MODULE '{' modparamlist '}'
    {
	    if ((parser_uimode()==NULL) && 
		(parser_active_system() == PA_ACTIVE)) {
		    mb_conf_load(parser_global_context->modconf);
	    }
	    else {
		    dg_printf(DG_VERBOSE,"%s:ignoring setup module\n",
			__FUNCTION__);
	    }
	    /* reset for the next modconf */
	    mb_conf_reset(parser_global_context->modconf);
    }  
;

modparamlist:   

    modparam
  | modparamlist modparam
;

modparam:   

    T_NAME T_STRING
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->modconf->name, $2);
	    }
	    cfg_strfree(&$2);
    } 
  | T_PARAM T_STRING
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->modconf->param, $2);
	    }
	    cfg_strfree(&$2);
    } 
  | T_KERNELVERSION T_STRING
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->modconf->
			kernelversion, $2);
	    }
	    cfg_strfree(&$2);
    } 
  | system '{' modparamlist '}'
    {
	    parser_exit_system();
    }  
;

setup_dasd: 

  T_SETUP T_DASD '{' T_BUSID T_BUSIDVAL '}'
    {
	    if ((parser_uimode() == NULL) &&
		(parser_active_system() == PA_ACTIVE)) {
		    parser_setup_dasd($5);
	    }
	    else {
		    dg_printf(DG_VERBOSE,"%s:ignoring setup dasd\n",
			__FUNCTION__);
	    }
	    cfg_strfree(&$5);
    } 
;

setup_qeth: 

  T_SETUP T_QETH '{'
	T_BUSID T_BUSIDVAL 
	T_BUSID T_BUSIDVAL 
	T_BUSID T_BUSIDVAL 
  '}'
    {
	    if ((parser_uimode() == NULL) &&
		(parser_active_system() == PA_ACTIVE)) {
		    parser_setup_qeth($5,$7,$9);
	    }
	    else {
		    dg_printf(DG_VERBOSE,"%s:ignoring setup qeth\n",
			__FUNCTION__);
	    }
	    cfg_strfree(&$5);
	    cfg_strfree(&$7);
	    cfg_strfree(&$9);
    } ;

setup_zfcp: 

  T_SETUP T_ZFCP '{' 
	T_BUSID T_BUSIDVAL 
	T_WWPN T_HEXNUM 
	T_LUN T_HEXNUM 
  '}'
    {
	    if ((parser_uimode() == NULL) &&
		(parser_active_system() == PA_ACTIVE)) {
		    parser_setup_zfcp($5,$7,$9);
	    }
	    else {
		    dg_printf(DG_VERBOSE,"%s:ignoring setup zfcp\n",
			__FUNCTION__);
	    }
	    cfg_strfree(&$5);
	    cfg_strfree(&$7);
	    cfg_strfree(&$9);
    } ;


/*
 * network setup can be specified with a variety of parameters.
 * parameters can be dependent from a system statement.
 */

network: 

  T_SETUP T_NETWORK '{' netparamlist '}'
    {
	    if ((parser_uimode() == NULL) &&
		(parser_active_system() == PA_ACTIVE)) {
		    nb_conf_enable(parser_global_context->netconf);
	    }
	    else {
		    dg_printf(DG_VERBOSE,"%s:ignoring network\n",
			__FUNCTION__);
	    }
	    /* reset for the next netconf */
	    nb_conf_reset(parser_global_context->netconf);
    }  
;

netparamlist:   

    netparam
  | netparamlist netparam
;

netparam:   

    T_MODE T_STATIC
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->netconf->mode = NB_STATIC;
	    }
    } 
  | T_MODE T_DHCP
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->netconf->mode = NB_DHCP;
	    }
    } 
  | T_ADDRESS T_IPADDR
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->netconf->address, $2);
	    }
	    cfg_strfree(&$2);
    } 
  | T_MASK T_IPADDR
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->netconf->mask, $2);
	    }
	    cfg_strfree(&$2);
    } 
  | T_GATEWAY T_IPADDR
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->netconf->gateway, $2);
	    }
	    cfg_strfree(&$2);
    } 
  | T_NAMESERVER T_IPADDR
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->netconf->
			nameserver, $2);
	    }
	    cfg_strfree(&$2);
    } 
  | T_INTERFACE T_IDENT
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->netconf->
			interface, $2);
	    }
	    cfg_strfree(&$2);
    } 
  | system '{' netparamlist '}'
    {
	    parser_exit_system();
    }  
;


/*
 * the start of userinterfaces can be defined via an identifier that
 * specifies the ui. the rest of the line will be given to the specific
 * ui module as a parameter line. the handling of the ssh userinterface is
 * somewhat special because it has to start another ui inside the ssh.
 */

userinterface:

    T_USERINTERFACE T_IDENT T_STRING
    {
	    struct cfg_userinterface ui;
	    const char *only_ui = NULL;
	    
	    dg_printf( DG_MAXIMAL, "p:ui: <%s> <%s>\n", $2, $3);
	    
	    if (parser_active_system() == PA_ACTIVE) {
		    only_ui = parser_uimode();
		    if (only_ui==NULL) {
			    cfg_userinterface_init(&ui);
			    cfg_strcpy(&ui.module,$2);
			    cfg_strcpy(&ui.cmdline,$3);
			    cfg_add_userinterface(parser_global_context->
				toplevel, &ui);
			    cfg_userinterface_destroy(&ui);
		    }
	    }
	    cfg_strfree(&$2);
	    cfg_strfree(&$3);
    }  
  | T_USERINTERFACE T_SSH T_IDENT T_STRING
    {
	    struct cfg_userinterface ui;
	    const char *only_ui = NULL;
	    
	    dg_printf( DG_MAXIMAL, "p:ui_ssh: <%s> <%s>\n", $3, $4);
	    
	    if (parser_active_system() == PA_ACTIVE) {
		    only_ui = parser_uimode();
		    /* start ssh in normal mode */
		    if (only_ui==NULL) {
			    cfg_userinterface_init(&ui);
			    cfg_strcpy(&ui.module,"ssh");
			    cfg_strcpy(&ui.cmdline,$4);
			    cfg_add_userinterface(parser_global_context->
				toplevel, &ui);
			    cfg_userinterface_destroy(&ui);
		    }	  
		    /* start ui on /dev/tty provided by ssh in only_ui mode */
		    else if (strcmp(only_ui,$3)==0) {
			    cfg_userinterface_init(&ui);
			    cfg_strcpy(&ui.module,$3);
			    cfg_strcpy(&ui.cmdline,"/dev/tty");
			    cfg_add_userinterface(parser_global_context->
				toplevel, &ui);
			    cfg_userinterface_destroy(&ui);
		    }
	    }
	    cfg_strfree(&$3);
	    cfg_strfree(&$4);
    }  
;


exec:

  T_EXEC T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:exec <%s>\n", $2);
	    if ((parser_uimode() == NULL) &&
		(parser_active_system() == PA_ACTIVE)) {
		    cfg_system($2);
	    }
	    cfg_strfree(&$2);
    }  
;


/*
 * bootentries start with the keyword 'boot_entry' and are followed by
 * a detailed specification in '{' '}'. bootentries will be ignored inside
 * an inactive 'system' section.
 */

bootentry:

  T_BOOT_ENTRY '{' becontent '}'
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    /* put collected settings into toplevel */
		    cfg_add_bentry(parser_global_context->toplevel, 
			parser_global_context->bentry);
		    /* cfg_bentry_print(parser_global_context->bentry); */
	    }
	    /* clean up for collecting the next entry */
	    cfg_bentry_destroy(parser_global_context->bentry);
	    cfg_bentry_init(parser_global_context->bentry);
    }  
;

becontent:

  title label optionlist
;

title: 

  T_TITLE T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:title <%s>\n", $2);
	    cfg_strcpy(&parser_global_context->bentry->title, $2);
	    cfg_strfree(&$2);
    }  
;

label:

    /* empty */
  | T_LABEL T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:label <%s>\n", $2);
	    cfg_strcpy(&parser_global_context->bentry->label, $2);
	    cfg_strfree(&$2);
    }  
;

optionlist:   

    option
  | optionlist option
;


/*
 * only specific combinations of options are valid!
 */

option:   

    T_ROOT uri
    {
	    dg_printf( DG_MAXIMAL, "p:root uri <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->bentry->root, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_LOCK
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->locked = 1;
	    }
    }  
  | T_PAUSE T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:pause <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->bentry->pause, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_KERNEL T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:kernel <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = KERNEL_BOOT;
		    cfg_strcpy(&parser_global_context->bentry->kernel, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_KERNEL uri
    {
	    dg_printf( DG_MAXIMAL, "p:kernel uri <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = KERNEL_BOOT;
		    cfg_strcpy(&parser_global_context->bentry->kernel, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_INITRD T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:initrd <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->bentry->initrd, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_INITRD uri
    {
	    dg_printf( DG_MAXIMAL, "p:initrd uri <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->bentry->initrd, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_CMDLINE T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:cmdline <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->bentry->cmdline, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_PARMFILE T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:parmfile <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->bentry->parmfile, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_PARMFILE uri
    {
	    dg_printf( DG_MAXIMAL, "p:parmfile uri <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    cfg_strcpy(&parser_global_context->bentry->parmfile, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_INSFILE T_STRING
    {
	    dg_printf( DG_MAXIMAL, "p:insfile <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = INSFILE_BOOT;
		    cfg_strcpy(&parser_global_context->bentry->insfile, $2);
	    }
	    cfg_strfree(&$2);
    }
  | T_INSFILE uri
    {
	    dg_printf( DG_MAXIMAL, "p:insfile uri <%s>\n", $2);
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = INSFILE_BOOT;
		    cfg_strcpy(&parser_global_context->bentry->insfile, $2);
	    }
	    cfg_strfree(&$2);
    }
  | T_BOOTMAP T_STRING
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = BOOTMAP_BOOT;
		    cfg_strcpy(&parser_global_context->bentry->bootmap, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_BOOTMAP uri
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = BOOTMAP_BOOT;
		    cfg_strcpy(&parser_global_context->bentry->bootmap, $2);
	    }
	    cfg_strfree(&$2);
    }  
  | T_REBOOT
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = REBOOT;
	    }
    }
  | T_HALT
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = HALT;
	    }
    }
  | T_SHELL
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = SHELL;
	    }
    }
  | T_EXIT
    {
	    if (parser_active_system() == PA_ACTIVE) {
		    parser_global_context->bentry->action = EXIT;
	    }
    }
  | system '{' optionlist '}'
    {
	    parser_exit_system();
    }  
;

uri:   

   T_URISTART T_STRING
   {
	   char *uri_str = NULL;

	   cfg_strinitcpy(&uri_str,$1);
	   cfg_strfree(&$1);
	   cfg_strcat(&uri_str,$2);
	   cfg_strfree(&$2);

	   $$ = uri_str;
   }
 | T_URISTART T_URIBRACKET T_STRING
   {
	   char *uri_str = NULL;

	   cfg_strinitcpy(&uri_str,$1);
	   cfg_strfree(&$1);
	   cfg_strcat(&uri_str,$2);
	   cfg_strfree(&$2);
	   cfg_strcat(&uri_str,$3);
	   cfg_strfree(&$3);

	   $$ = uri_str;
   }
;


/*
 * the system statement checks if we are on the specified system.
 * if this is not the case the content of the system bracket will be ignored.
 */

system:    

  T_SYSTEM systemlist 
    {
	    /* we have evaluated the system statement */
	    dg_printf( DG_MAXIMAL, "system statement is %s\n", $2);
	    if (strcmp($2,"true")!=0) {
		    parser_enter_system(PA_INACTIVE);
	    }
	    else {
		    parser_enter_system(PA_ACTIVE);
	    }
	    cfg_strfree(&$2);
    }  
;

systemlist:   

    systemid
    {
	    $$ = $1;
    }  
  | systemlist systemid
    {
	    /* if one of both is "true" the result is "true" */
	    if (strcmp($1,"true")==0) {
		    $$ = $1;
		    cfg_strfree(&$2);
	    }
	    else if (strcmp($2,"true")==0) {
		    $$ = $2;
		    cfg_strfree(&$1);
	    }
	    else{ /* both are not "true" */
		    $$ = $1;
		    cfg_strfree(&$2);
	    }
    }  
  | T_NOT '(' systemlist ')'
    {
	    char *negstr = NULL;
	    
	    if (strcmp($3,"true")==0) {
		    cfg_strinitcpy(&negstr,"false");
	    }
	    else{
		    cfg_strinitcpy(&negstr,"true");
	    }    
	    $$ = negstr;
	    cfg_strfree(&$3);
    }
;

systemid:

    T_UUID    '(' T_IDENT ')'
    {
	    $$ = $3;
    }  
  | T_MAC     '(' T_MACVAL ')'
    {
	    char *mac_str = NULL;
	    
	    if (nb_test_mac($3) == CFG_RETURN_OK) {
		    cfg_strinitcpy(&mac_str,"true");
	    }
	    else {
		    cfg_strinitcpy(&mac_str,"false");
	    }
	    cfg_strfree(&$3);
	    $$ = mac_str;
    }  
  | vmguest
    {
	    $$ = $1;
    }  
;

vmguest:

    T_VMGUEST '(' T_IDENT ')'
    {
	    char *result_str = NULL;

	    if (parser_vmguest_test( $3) == CFG_RETURN_OK) {
		    cfg_strinitcpy(&result_str,"true");
	    }
	    else {
		    cfg_strinitcpy(&result_str,"false");
	    }
	    cfg_strfree(&$3);
	    $$ = result_str;
    }  
  | T_VMGUEST '(' T_IDENT ',' T_IDENT ')'
    {
	    char *result_str = NULL;

	    if ((parser_vmguest_test( $3) == CFG_RETURN_OK) &&
		(parser_lpar_test( $5) == CFG_RETURN_OK)) {
		    cfg_strinitcpy(&result_str,"true");
	    }
	    else {
		    cfg_strinitcpy(&result_str,"false");
	    }
	    cfg_strfree(&$3);
	    cfg_strfree(&$5);
	    $$ = result_str;
    }
  | T_LPAR '(' T_IDENT ')'
    {
	    char *result_str = NULL;

	    if (parser_lpar_test( $3) == CFG_RETURN_OK) {
		    cfg_strinitcpy(&result_str,"true");
	    }
	    else {
		    cfg_strinitcpy(&result_str,"false");
	    }
	    cfg_strfree(&$3);
	    $$ = result_str;
    }
;

%%
