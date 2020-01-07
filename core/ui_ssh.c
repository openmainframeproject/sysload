/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file ui_ssh.c
 * \brief System Loader user interface module for remote ssh-clients
 *
 * \author Swen Schillig    (swen@vnet.ibm.com)
 * \author Christof Schmitt (christof.schmitt@de.ibm.com)
 *
 * $Id: ui_ssh.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <unistd.h>        //->unlink
#include <string.h>        //->strlen
#include <libgen.h>        //->basename
#include <syslog.h>        //->syslog
#include "ssh.h"
#include "config.h"
#include "sysload.h"


char *arg0; //!< global variable with pointer to argv[0]

/**
 * get value for key from a string list
 *
 * \param[in] key  :to search a value for
 *            list :of strings where to search for key and value
 * \return    CFG_RETURN_OK if the user entered the correct password,
 *            CFG_RETURN_ERROR otherwise
 */
char *get_value(char const *key, char **list)
{
    char * t_ptr;

    if(list == NULL) return(NULL);

    while((t_ptr = *list++) != NULL)
    {
        if(strncmp(key,t_ptr,strlen(key)) != 0)
            continue;
        t_ptr += strlen(key);

        if(*t_ptr == '=')           //proper assignment
            t_ptr++;
        else if(*t_ptr == '\0')     //blank instead of '=' separator
            t_ptr = *list++;

        return(t_ptr);
    }
    return(NULL);
}

/**
 * assemble cmd-line (host-key file) parameter for SSH server
 *
 * \param[in]  type     : DSS or RSA
 *             key_param: file name string from cmd-line
 * \param[out] key_param: final cmd-line parameter for SSH server
 *
 * \return     ON SUCCESS final cmd-line parameter for SSH server
 *             ELSE       NULL
 */
char * assemble_key_param(char const *type, char *key_param, char *portnumber)
{
    char *res_str;       //resulting key_param string
    char *info;          //dummy pointer for comp_load function
    char *error;         //dummy pointer for comp_load function
    char *def_str = (strcmp(type,"DSS") == 0) ? 
	    SSH_DSS_HOST_KEY : SSH_RSA_HOST_KEY; //default host key location

    if(key_param)
    {
        res_str = malloc(strlen(def_str) + strlen(portnumber) + 2);
        sprintf(res_str,"%s.%s", def_str, portnumber);

        if(comp_load(res_str, key_param, &info, &error) != 0)
        {
            syslog(LOG_ERR,
		"Cannot retrieve %s host key file. Using default instead.",
		type);
            return(def_str);
        }
        else
            return(res_str);
    }
    else return(def_str);
}

int main(int argc,char **argv)
{
    char const *cmd_name = basename(argv[0]);
    char *portnumber     = get_value("port",argv) ;
    char *rsa_key        = get_value("rsa_key",argv);
    char *dss_key        = get_value("dss_key",argv);
    char *temp_key;

    char *temp_fn;                    //pid-file name
    char *ex_str = NULL;              //execution string

    FILE *temp_fh;                    //pid-file handle
    
    struct stat buf;                  //result buffer for stat() call

    pid_t ssh_pid;                    //sshd-pid
    int   ex_code;                    //execution return code

    openlog(cmd_name, LOG_PID, LOG_USER);

    dg_level = DG_MINIMAL;
    dg_init();

    portnumber = portnumber ? portnumber : SSH_PORT;     //assign port number

    syslog(LOG_INFO,"Starting SSH on port %s", portnumber);

    temp_fn = malloc(strlen(SSH_PID) + strlen(portnumber) + 2);
    sprintf(temp_fn,"%s.%s",SSH_PID,portnumber);

    //check for existence of PID file
    if(stat(temp_fn,&buf) == 0)                          
    {
        if((temp_fh = fopen(temp_fn,"r")))
        {
		//read PID for SSHD which is serving our port
		fscanf(temp_fh,"%ld",(long *) &ssh_pid);     
		fclose(temp_fh);
		syslog(LOG_WARNING,"PID file already exists");
		if(kill(ssh_pid, 0) == 0)                    //PID is running
		{
			syslog(LOG_ERR,"already running on port %s", 
			    portnumber);
			exit(10);
		}
		syslog(LOG_INFO,
		    "not running on port %s. Removing stale PID file\n", 
		    portnumber);
        }
	//delete stale PID file
        unlink(temp_fn);                                 
    }

    free(temp_fn);

    temp_key = assemble_key_param("RSA", rsa_key, portnumber);
    rsa_key  = malloc(strlen(temp_key) + 5);
    sprintf(rsa_key,"-h %s", temp_key);

    temp_key = assemble_key_param("DSS", dss_key, portnumber);
    dss_key  = malloc(strlen(temp_key) + 5);
    sprintf(dss_key,"-h %s", temp_key);

    cfg_strinit(&ex_str);

    // Note: Do not run sshd in the background. sysload expects a valid
    //   boot entry on stdout, so don't return in the normal case and
    //   sysload won't receive an empty response.
    cfg_strprintf(&ex_str,
	"%s -D -p %s -f %s -oPidFile=%s.%s %s %s -h /etc/ssh_host_key", 
	SSH_CMD, portnumber, SSH_CONFIG, 
	SSH_PID, portnumber, dss_key, rsa_key);

    ex_code = cfg_system(ex_str);
    cfg_strfree(&ex_str);

    closelog();
    return( (ex_code <= 0) ? ex_code : WEXITSTATUS(ex_code) );
}
