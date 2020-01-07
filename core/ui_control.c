/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file ui_control.c
 * \brief Starts all userinterfaces and collects the results.
 *
 * \author Michael Loehr (mloehr@de.ibm.com)
 * \author Swen Schillig (swen@vnet.ibm.com)
 *
 * $Id: ui_control.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "ui_control.h"
#include "parser.h"
#include "debug.h"

static int rcvd_sig = 0;


/**
 * For local tracking of ui client status
 */

enum ui_status {
	UI_RUNNING,      /**< still going strong */
	UI_CLEAN_EXIT,   /**< has exited without problem */
	UI_PROBLEM_EXIT  /**< has exited with problem or crashed */
};


/**
 * This structure contains information about a single user interface client.
 */

struct ui_info {
	pid_t pid;             /**< pid of a child */
	int pipe_fd[2];        /**< pipe coming from the child */
	char *collected_str;   /**< collected input from the child */
	enum ui_status status; /**< local tracking of running clients */
};


/**
 * Build a set of all clients that could write to the input pipes.
 *
 * \param[out] wset  the resulting waitset
 * \param[in]  c     the current client info array
 * \param[in]  count the number of clients in the client info array
 */

void build_waitset(fd_set *wset, struct ui_info *c, int count)
{
	int i = 0;

	FD_ZERO(wset); // initialize empty waitset

	for (i = 0; i < count; i++) {
		if (kill(c[i].pid, 0) == 0) { // only existing clients!
			// add client input to waitset
			FD_SET(c[i].pipe_fd[0], wset);
		}
	}
}


/**
 * Remove all clients that have been terminated up to now.
 *
 * \param[in]  c     the current client info array
 * \param[in]  count the number of clients in the client info array
 */

void remove_terminated_clients(struct ui_info *c, int count)
{
	pid_t return_pid;         /* pid returned from exiting ui */
	int status = 0;           /* status from returning processes */
	int cnr = 0;

	do {
		/* wait nonblocking to remove zombies */
		return_pid = waitpid (WAIT_ANY, &status, WNOHANG);

		/* search for the process and mark as finished */
		for (cnr = 0; cnr < count; cnr++) {
			if (c[cnr].pid == return_pid) {
				if (WIFEXITED(status) && (WEXITSTATUS(status)
					== 0))
					c[cnr].status = UI_CLEAN_EXIT;
				else
					c[cnr].status = UI_PROBLEM_EXIT;
	
				continue;
			}
		}
	} while (return_pid > 0); // until no more terminated clients left
}

void default_sig_hdlr(int sig_no)
{
    rcvd_sig = sig_no;
    return; //just leave the select() function.
}

/**
 * This function will be triggered when input is available or the endtime
 * is reached. If endtime is 0 no timeout is set!
 *
 * \param[in] size    the size parameter of the original select system call
 * \param[in] readset the waitset of the original select system call
 * \param[in] time_out the time after which the default entry should be booted.
 * \return    equivalent to the select system call
 */

int ui_select(int size, fd_set *readset, int time_out)
{
    if(time_out < 0)
        time_out = 0;

    alarm(time_out);

    return select(size, readset, NULL, NULL, NULL);
}


/**
 * This function is called after a timeout occured. It initializes the output
 * boot entry and copies the content of the default selection to the
 * output boot entry.
 *
 * \param[out] boot the output boot entry to be initialized
 * \param[in]  config the config structure from which the default will be
 *             copied
 * \return     CFG_RETURN_OK if successfull, CFG_RETURN_ERROR if not
 */

int ui_timeout(struct cfg_bentry *boot, struct cfg_toplevel *config)
{
	/* no valid default entry was set */
	if ((config->boot_default < 0) ||
	    (config->boot_default >= config->bentry_count)) {
		return CFG_RETURN_ERROR;
	}

	/* just copy the default entry */
	cfg_bentry_init(boot);
	cfg_bentry_copy(boot, &(config->bentry_list[config->boot_default]));
	return CFG_RETURN_OK;
}


/**
 * Reads all available data from the input pipe referenced by filedes and
 * appends the input data to collstr.
 *
 * \param [in,out] collstr string to collect input data
 * \param [in]     filedes references pipe to read from
 * \return         equivalent to the read system call
 */

int read_from_client (char **collstr, int filedes)
{
	char input[CFG_STR_MAX_LEN+1] = "";
	int input_size = 0;

	/* get input from filedes */
	input_size = read(filedes, input, CFG_STR_MAX_LEN);
	if (input_size >= 0) {
		input[ input_size] = '\0';
		dg_printf(DG_VERBOSE,"%s:<%s>\n",__FUNCTION__,input);
		cfg_strcat(collstr, input);
	} else {
		syslog(LOG_ERR,"error reading from pipe - %s",
		    strerror(errno));
	}

	return input_size;
}


/**
 * Searches for the client that has data available on fd.
 *
 * \param [in] fd filedescriptor of the pipe that has data available
 * \param[in]  c the current client info array
 * \param[in]  count the number of clients in the client info array
 * \return     position of the client in the client info array
 */

int search_for_client(int fd, struct ui_info *c, int count)
{
	int cnr = 0;

	for (cnr = 0; cnr < count; cnr++) {
		if (c[cnr].pipe_fd[0] == fd) {
			return cnr;
		}
	}

	/* this can never happen unless there is something really wrong */
	syslog(LOG_ERR, "client not found!");
	return 0;
}

/**
 * Check if we have a complete and valid input from a client.
 *
 * \param [in,out] boot  the boot information, if the input was complete and
 *                       valid
 * \param [in]     fd    input filedescriptor from the client to check
 * \param [in]     c     the current client info array
 * \param [in]     count the number of clients in the client info array
 * \return         CFG_RETURN_OK if input is complete and valid,
 *                 CFG_RETURN_ERROR otherwise
 */

int check_input_from_client(struct cfg_bentry *boot, int fd,
    struct ui_info *c, int count)
{
	char *errmsg = NULL;
	int cnr = 0;

	/* search for the client with this fd */
	cnr = search_for_client(fd, c, count);

	read_from_client(&(c[cnr].collected_str), fd);

	remove_terminated_clients(c, count);
	
	if (c[cnr].status == UI_CLEAN_EXIT) { // client ended with succes?
		errmsg = parse_sysload_boot_entry(boot, c[cnr].collected_str);
		if (errmsg == NULL) { // parse ok
			return CFG_RETURN_OK;
		} else { // parse error
			/* throw away the message and hope for another */
			/*  usefull input */
			cfg_strfree(&errmsg);
			syslog(LOG_ERR, "parse error in:%s", 
			    c[cnr].collected_str);
		}
	}
	return CFG_RETURN_ERROR;
}


/**
 * Start all userinterfaces and wait for timeout or the first successfull
 * result.
 *
 * \param[in]  startup_msg  message to be displayed by all user interfaces.
 * \param[in]  config       complete config to be handled by every user
 *                          interface.
 * \param[out] boot         contains this selected boot entry on
 *                          successfull return
 * \return     CFG_RETURN_OK if successfull, CFG_RETURN_ERROR if not.
 */

int userinterface(const char *startup_msg, struct cfg_toplevel *config,
    struct cfg_bentry *boot)
{
	struct ui_info *u = NULL; /* info for all uis */
	int i = 0;                /* multi purpose counter */
	struct timeval timeout;   /* for waiting with select */
	char *module_call = NULL; /* module call with params */
	char *defaultpath = NULL;
	fd_set read_fd_set;       /* set of input files to read from */
	int fd = 0;
	int reason = 0;           /* reason for return from select */
	int retval = CFG_RETURN_ERROR;
	time_t endtime;           /* when should the timeout happen */

	DG_ENTER( DG_VERBOSE);
	/* initialize */
	dg_printf(DG_VERBOSE, 
	    "%s: %d userinterfaces with %d bootentries to start\n", 
	    __FUNCTION__, config->ui_count, config->bentry_count);

	u = malloc(sizeof(struct ui_info) * config->ui_count);
	if (u == NULL) {
		syslog(LOG_ERR,"malloc failed");
		return CFG_RETURN_ERROR;
	}
	cfg_strinit(&module_call);
	cfg_strinit(&defaultpath);
	for (i = 0; i < config->ui_count; i++) {
		u[i].pid = 0;
		u[i].pipe_fd[0] = 0;
		u[i].pipe_fd[1] = 0;
		cfg_strinit(&(u[i].collected_str));
		u[i].status = UI_PROBLEM_EXIT;
	}

	/* useless to continue if no timeout and no ui */
	if (config->timeout <= 0 && config->ui_count <=0) {
		syslog(LOG_WARNING,"%s", startup_msg);
		syslog(LOG_WARNING,"nothing to do for the userinteface");
		retval = CFG_RETURN_ERROR;
		goto cleanup_and_return;
	}

	/* set environment variables */
	cfg_set_env_str(CFG_STARTUP_MSG, (char *) startup_msg);
	cfg_set_env(config);

	/* start all clients */
	for (i = 0; i < config->ui_count; i++) {
		/* create pipe */
		if(pipe(u[i].pipe_fd) < 0) {
			syslog(LOG_ERR, "error creating pipe - %s", 
			    strerror(errno));
			retval = CFG_RETURN_ERROR;
			goto cleanup_and_return;
		}

		u[i].status = UI_RUNNING; // assume the fork will work

		switch(u[i].pid = fork()) {
		case -1: /* fork failed */
			syslog(LOG_ERR, "fork failed - %s", strerror(errno));
			retval = CFG_RETURN_ERROR;
			goto cleanup_and_return;

		case 0: /* child */
			/* we do not need the read end of the pipe */
			close(u[i].pipe_fd[0]);
			/* child i closes stdout and duplicates pipefd[1]
			   ==> pipefd[1] is new "stdout" */
			dup2(u[i].pipe_fd[1], 1);

			/* start process */
			cfg_get_env_str(CFG_PATH, &defaultpath);
			if (strlen(defaultpath) == 0) {
				cfg_strcpy(&defaultpath, CFG_DEFAULTPATH);
			}

                        cfg_strprintf(&module_call, "%s/%s/ui_%s",
			    defaultpath, UI_MODULE_PATH,
			    config->ui_list[i].module);
			dg_printf( DG_VERBOSE, module_call);

                        execl(module_call, strrchr(module_call,'/') + 1,
                              config->ui_list[i].cmdline, (char *) 0);

			u[i].status = UI_PROBLEM_EXIT;
			syslog(LOG_ERR, "cannot start process - %s", 
			    strerror(errno));
			retval = CFG_RETURN_ERROR;
			goto cleanup_and_return;

		default: /* parent */
			/* we are only reading and do not need the write end */
			close(u[i].pipe_fd[1]);
		}
	}

        signal(SIGALRM, default_sig_hdlr);
        signal(SIGUSR1, default_sig_hdlr);

	/* set timeout */
        endtime = (config->timeout > 0) ? config->timeout : 0 ;

	/* wait for several children or timeout */
	while (0==0) {

		/* Block until input arrives on one or more active clients. */
		build_waitset(&read_fd_set, u, config->ui_count);

		reason = ui_select(FD_SETSIZE, &read_fd_set, endtime);

		if (reason <= 0) { // problem or interrupt
			if (errno != EINTR) { // problem!
				syslog(LOG_ERR, "select failed - %s", 
				    strerror(errno));
				retval = CFG_RETURN_ERROR;
				goto cleanup_and_return;
                        }
                        if(rcvd_sig == SIGUSR1) { //switch off timeout
                            if(endtime)
                            {
                                syslog(LOG_INFO, "TIMEOUT STOPPED BY USER");
                                config->timeout = 0; //change config in case of re-entrance
                            }
                            endtime = 0;
                            continue;
                        }
			//we received an interrupt
                        retval = ui_timeout(boot, config); 
			goto cleanup_and_return;

		} else {
			/* Service all pending input. */
			for (fd = 0; fd < FD_SETSIZE; ++fd) {
				if (FD_ISSET (fd, &read_fd_set)) {
					if (check_input_from_client(boot, fd,
						u, config->ui_count) == 
					    CFG_RETURN_OK) {
						retval = CFG_RETURN_OK;
						goto cleanup_and_return;

					}
				} /* if (FD_ISSET ... */
			} /* for (fd ... */
		} /* else */
	} /* while (0==0) */

 cleanup_and_return:

	for (i = 0; i < config->ui_count; i++) {
		if (u[i].status == UI_RUNNING) { // process still exists!
			kill(u[i].pid, SIGTERM);
		}
	}

	remove_terminated_clients(u, config->ui_count);

	for (i = 0; i < config->ui_count; i++) {
		// at least one process still exists!
		if (u[i].status == UI_RUNNING) {
			timeout.tv_usec = 0;  /* milliseconds */
			timeout.tv_sec  = UI_KILL_TIMEOUT;  /* seconds */
			select(0, NULL, NULL, NULL, &timeout);
			break; // wait only once
		}
	}

	for (i = 0; i < config->ui_count; i++) {
		// at least one process still exists!
		if (u[i].status == UI_RUNNING) {
			/* kill remaining processes */
			kill(u[i].pid, SIGKILL);
		}
	}

	remove_terminated_clients(u, config->ui_count);

	/* clean up */
	for (i = 0; i < config->ui_count; i++) {
		cfg_strfree(&(u[i].collected_str));
	}
	cfg_strfree(&module_call);
	cfg_strfree(&defaultpath);
	free(u);

	DG_RETURN( DG_VERBOSE, retval);
}


