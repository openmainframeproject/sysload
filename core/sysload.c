/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file sysload.c
 * \brief Main function for System Loader
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 *
 * $Id: sysload.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>       //basename
#include <syslog.h>       //syslog
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "sysload.h"


char *arg0; //<! global variable pointing to argv[0] (used in MEM_ASSERT)

/**
 * Print help message
 *
 * \param[in] arg0 \p argv[0] from \p main()
 */

void
print_help(const char *arg0)
{
	printf("Usage: %s [OPTION] <configuration file URI>\n"
	    "Start System Loader user interface.\n\n"
	    " -o <device>   Output System Loader status messages to <device>. "
	    "Default is stdout.\n"
	    " -u <uitype>   Ignore global definitions/Start only this ui. \n"
	    " -v            Output version information and exit.\n"
	    " -h            Display this help and exit.\n",
	    arg0);
}


/**
 * Redirect stdout & stderr to different device if requested on command line.
 *
 * \param[in]  device Path to output device.
 */

void
redirect_output(const char *device)
{
	int fd;
	struct stat st;

	if (strlen(device)) {
		if (stat(device, &st)) {
			printf("Warning: unable to check output device '%s'"
			    " - %s\n", device, strerror(errno));
			goto error;
		}
		if ((st.st_mode & S_IFMT) != S_IFCHR) {
			printf("Warning: '%s' is not a character device\n",
			    device);
			goto error;
		}
		fd = open(device, O_RDWR | O_NONBLOCK, 0);
		if (fd != -1) {
			dup2(fd, 1);
			dup2(fd, 2);
			close(fd);
			return;
		}
		printf("Warning: error opening output device '%s' - %s\n",
		    device, strerror(errno));

	error:
		printf("Continuing using stdout as output device.\n");
	}
}


/**
 * \p main -- the entry point into a world of wonderful possibilities.
 *
 * \param[in]  argc Number of arguments passed to program.
 * \param[in]  argv Array with arguments passed to program.
 */

int
main(int argc, char **argv)
{
	char *errmsg, *startup_msg;
	struct sysload_arguments sysload_args;
	struct cfg_toplevel config;
	struct cfg_bentry boot;
	enum parse_result pa_return;
	int retval = 0; 
	FILE *pidfile = NULL;

	arg0 = argv[0];

        openlog(basename(argv[0]), LOG_PID | LOG_CONS, LOG_USER);

        dg_init();

	pa_return = parse_arguments(&sysload_args, argc, argv);
	dg_printf(DG_VERBOSE, "%s:parse_arguments done.\n", __FUNCTION__);

	switch (pa_return) {

	case PA_DONE:
		retval = 0;
		break;

	case PA_ERROR:
		retval = 1;
		break;

	case PA_VALID:
		redirect_output(sysload_args.console);
		dg_printf(DG_VERBOSE, 
		    "%s:redirect output done.\n", __FUNCTION__);
		cfg_strinit(&startup_msg);

		// print startup message to redirected console
		syslog(LOG_INFO,"sysload user interface is starting.");
		syslog(LOG_INFO,"Configuration file source: %s",
		    sysload_args.config_uri);

		// if we are the primary sysload
		if (strlen(sysload_args.only_ui) == 0) {
			// handle sysload config info on the kernel command line
			parse_kernel_cmdline();
			// write our pid to a file
			pidfile = fopen(CFG_PIDFILE, "w+");
			if (!pidfile) {
				fprintf(stderr, 
				    "%s: fopen not possible for %s\n", 
				    arg0, CFG_PIDFILE);
			}
			else {
				fprintf(pidfile,"%d", getpid());
				fclose(pidfile);
			}
		}

		// parse configuration file
		errmsg = parse_sysload(&config, &sysload_args);
		if (errmsg) {
			cfg_strprintf(&startup_msg,
			    "Error loading configuration data:\n"
			    "%s\n"
			    "Continuing with blank configuration\n", errmsg);
			free(errmsg);
			cfg_init(&config);
		}

		while (WORLD_EXISTS) {
			// launch user interface modules
			if (userinterface(startup_msg, &config, &boot)) {
				syslog(LOG_ERR,
				    "unable to find boot configuration\n"
				    "sysload is exiting - goodbye!");
				retval = 1;
				break;
			}
			cfg_strcpy(&startup_msg, "");

			// start new kernel
			errmsg = loader(&boot);
			if (errmsg) {
				cfg_strprintf(&startup_msg,
				    "Unable to start selected configuration:\n"
				    "%s\n"
				    "Please try again.\n",
				    errmsg);
				free(errmsg);
			} else
				cfg_strprintf(&startup_msg,
				    "Error starting selected configuration. "
				    "Please try again.\n");
			cfg_bentry_destroy(&boot);
		}
		break;
	}

	// enter a shell for debugging purposes
	if (dg_level != DG_OFF) {
		action_shell();
        	closelog();
	}

	return retval;
}
