/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file loader.c
 * \brief Functions to load and start new kernel
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 * \author Michael Loehr   (mloehr@de.ibm.com)
 *
 * $Id: loader.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "sysload.h"
#include "insfile.h"
#include "bootmap.h"
#include "debug.h"


/**
 * Prefix URI with \p root if \p uri is a relative path. A relative
 * path is either marked by surrounded single quotes or it doesn't
 * start with a URI scheme.
 *
 * \param[in] root  Root prefix
 * \param[in] uri   URI to be prefixed
 * \return          Dynamically allocated string with final URI
 */

char *
prefix_root(const char *root, const char *uri)
{
	char *final_uri;
	const char *colon_ptr, *ptr;
	size_t uri_len = strlen(uri);

	cfg_strinit(&final_uri);
	if (!uri_len)
		return final_uri;

	// check if URI is surrounded by single quotes
	if (uri_len > 1 && uri[0] == '\'' && uri[uri_len-1] == '\'') {
		cfg_strprintf(&final_uri, "%s%s", root, uri+1);
		final_uri[strlen(final_uri)-1] = '\0';
		return final_uri;
	}

	cfg_strprintf(&final_uri, "%s%s", root, uri);

	// valid URIs it must contain a ':'
	colon_ptr = strchr(uri, ':');
	if (!colon_ptr)
		return final_uri;

	// valid URIs must start with a alphabetic character
	if (!islower(uri[0]) && !isupper(uri[0]))
		return final_uri;

	// all remaining characters before ':' must be either alphabetic
	// characters or digits or '+' or '-' or '.'
	for (ptr = uri+1; ptr < colon_ptr; ptr++) {
		if (!islower(*ptr) && !isupper(*ptr) && !isdigit(*ptr) &&
		    *ptr != '+' && *ptr != '-' && *ptr != '.')
			return final_uri;
	}

	cfg_strcpy(&final_uri, uri);
	return final_uri;
}


/**
 * Compose kernel command line from parmfile contents and command
 * line string. On success \p NULL will be returned. On error a
 * dynamically allocated error message is returned.
 *
 * \param[out] final_cmdline  Uninitialized string where final command line
 *                            will be stored.
 * \param[in]  parmfile       String with parmfile filename.
 * \param[in]  cmdline        String with kernel command line.
 * \return                    Dynamically allocated string with final kernel
 *                            command line
 */

char *
compose_commandline(char **final_cmdline, const char *parmfile,
    const char *cmdline)
{
	char *errmsg;
	int fd;
	ssize_t count = 0, err;
	struct stat st;

	cfg_strinit(&errmsg);

	if (stat(parmfile, &st)) {
		cfg_strprintf(&errmsg, "Unable to access local parmfile '%s' "
		    "- %s", parmfile, strerror(errno));
		return errmsg;
	}
	if (st.st_size+strlen(cmdline) > SYSLOAD_MAX_KERNEL_CMDLINE) {
		cfg_strcpy(&errmsg, "Kernel command line too long.");
		return errmsg;
	}

	fd = open(parmfile, O_RDONLY);
	if (fd == -1) {
		cfg_strprintf(&errmsg, "Unable to open local parmfile '%s' "
		    "- %s", parmfile, strerror(errno));
		return errmsg;
	}
	*final_cmdline = malloc(st.st_size+strlen(cmdline)+1);
	MEM_ASSERT(*final_cmdline);
	while (count<st.st_size) {
		err = read(fd, (*final_cmdline)+count, st.st_size-count);
		if (err == -1) {
			cfg_strprintf(&errmsg,
			    "Unable to read local parmfile '%s' - %s",
			    parmfile, strerror(errno));
			close(fd);
			free(*final_cmdline);
			return errmsg;
		}
		count += err;
	}
	close(fd);
	(*final_cmdline)[count] = '\0';
	cfg_strcat(final_cmdline, " ");
	cfg_strcat(final_cmdline, cmdline);
	cfg_strfree(&errmsg);

	return NULL;
}


/**
 * Replacement for \p system() where command line arguments can be
 * passed via string array \p argv . The value returned is -1 on
 * error (e.g. fork failed), and the return status of the command
 * otherwise.
 *
 * \param path[in] Pathname of file to be executed.
 * \param argv[in] Array with strings used as argument list for program.
 *                 The string array must be terminated by a \p NULL pointer.
 * \return         Return status of program.
 */

static int
systemv(const char *path, char *const argv[])
{
	sig_t sigint, sigquit, sigchld;
	int status;
	pid_t pid;

	// backup signal handler
	sigint = signal(SIGINT, SIG_IGN);
	sigquit = signal(SIGQUIT, SIG_IGN);
	sigchld = signal(SIGCHLD, SIG_DFL);

	pid = fork();
	if (pid < 0) {
		signal(SIGINT, sigint);
		signal(SIGQUIT, sigquit);
		signal(SIGCHLD, sigchld);
		fprintf(stderr, "%s: fork failed - %s\n", arg0,
		    strerror(errno));
		return -1;
	}
	if (pid == 0) {
		signal(SIGQUIT, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);

		execv(path, argv);
		fprintf(stderr, "exec '%s' failed - %s;\n", path,
		    strerror(errno));
		_exit(127);
	}

	if (wait4(pid, &status, 0, NULL) == -1)
		status = -1;

	// restore signal handler
	signal(SIGINT, sigint);
	signal(SIGQUIT, sigquit);
	signal(SIGCHLD, sigchld);

	return status;
}


/**
 * By using kexec tool load new kernel image and switch to new kernel.
 * On success function does not return. On error a dynamically
 * allocated error message is returned.
 *
 * \param[in] kernel    Pathname to kernel image.
 * \param[in] initrd    Pathname to initrd image.
 * \param[in] cmdline   Kernel command line.
 * \return    In case of error dynamically allocated error message.
 */

char *
kexec(const char *kernel, const char *initrd, const char *cmdline)
{
	char *msg, *kernel_arg, *initrd_arg, *cmdline_arg;
	char *argv[10];
	int index = 0, status;

	cfg_strinit(&msg);
	cfg_strinit(&kernel_arg);
	cfg_strinit(&initrd_arg);
	cfg_strinit(&cmdline_arg);

	// load kernel into memory
	argv[index++] = SYSLOAD_KEXEC_CMD;
	argv[index++] = "-l";
	if (strlen(initrd)) {
		cfg_strprintf(&initrd_arg, "--initrd=%s", initrd);
		argv[index++] = initrd_arg;
	}
	if (strlen(cmdline)) {
		cfg_strprintf(&cmdline_arg, "--command-line=%s", cmdline);
		argv[index++] = cmdline_arg;
	}
	cfg_strcpy(&kernel_arg, kernel);
	argv[index++] = kernel_arg;
	argv[index] = NULL;
	status = systemv(argv[0], argv);
	if (status) {
		cfg_strprintf(&msg, "kexec load failed with return code %i.",
		    status);
		goto cleanup;
	}

	// execute new kernel
	argv[0] = SYSLOAD_KEXEC_CMD;
	argv[1] = "-e";
	argv[2] = NULL;
	status = systemv(argv[0], argv);

	// if we are still here something is wrong
	cfg_strprintf(&msg, "kexec execute failed with return code %i.",
	    status);

 cleanup:
	cfg_strfree(&kernel_arg);
	cfg_strfree(&initrd_arg);
	cfg_strfree(&cmdline_arg);

	return msg;
}


/**
 * Regular kernel boot method: copy files to local filesystem and call
 * \p kexec to boot new kernel. On success function does not return.
 * On error a dynamically allocated error message is returned.
 *
 * \param boot  Pointer to boot entry.
 * \return      Dynamically allocated error message.
 */

static char *
action_kernel_boot(struct cfg_bentry *boot)
{
	char *msg = NULL, *tmp_uri = NULL, *tmp_msg = NULL, *cmdline = NULL;

	if (strlen(boot->kernel)>0) {
		tmp_uri = prefix_root(boot->root, boot->kernel);
		if (comp_load(SYSLOAD_FILENAME_KERNEL, tmp_uri, NULL,
			&tmp_msg)) {
			cfg_strprintf(&msg, "Error loading kernel image - %s",
			    tmp_msg);
			goto cleanup;
		}
		cfg_strfree(&tmp_uri);
	} else {
		cfg_strinitcpy(&msg,  "No kernel specified.");
		goto cleanup;
	}

	if (strlen(boot->initrd)>0) {
		tmp_uri = prefix_root(boot->root, boot->initrd);
		if (comp_load(SYSLOAD_FILENAME_INITRD, tmp_uri, NULL,
			&tmp_msg)) {
			cfg_strprintf(&msg, "Error loading initrd image - %s",
			    tmp_msg);
			goto cleanup;
		}
		cfg_strfree(&tmp_uri);
	}

	if (strlen(boot->parmfile)>0) {
		tmp_uri = prefix_root(boot->root, boot->parmfile);
		if (comp_load(SYSLOAD_FILENAME_PARMFILE, tmp_uri, NULL,
			&tmp_msg)) {
			cfg_strprintf(&msg, "Error loading parmfile - %s",
			    tmp_msg);
			goto cleanup;
		}
		msg = compose_commandline(&cmdline, SYSLOAD_FILENAME_PARMFILE,
		    boot->cmdline);
		if (msg)
			goto cleanup;
		cfg_strfree(&tmp_uri);
	} else
		cfg_strinitcpy(&cmdline, boot->cmdline);

	if (strlen(boot->initrd))
		msg = kexec(SYSLOAD_FILENAME_KERNEL, SYSLOAD_FILENAME_INITRD, cmdline);
	else
		msg = kexec(SYSLOAD_FILENAME_KERNEL, "", cmdline);
	cfg_strfree(&cmdline);

 cleanup:
	cfg_strfree(&tmp_msg);
	cfg_strfree(&tmp_uri);
	unlink(SYSLOAD_FILENAME_KERNEL);
	unlink(SYSLOAD_FILENAME_INITRD);
	unlink(SYSLOAD_FILENAME_PARMFILE);

	return msg;
}


/**
 * Boot map boot method: Read and analyse boot map, copy files to local
 * filesystem and call \p kexec to boot new kernel. On success function
 * does not return. On error a dynamically allocated error message
 * is returned.
 *
 * \param boot  Pointer to boot entry.
 * \return      Dynamically allocated error message.
 */

char *
action_bootmap_boot(struct cfg_bentry *boot)
{
	char *msg;

	if (strstr(boot->bootmap, "dasd://") == boot->bootmap)
		msg = action_bootmap_boot_dasd(boot);
	else if (strstr(boot->bootmap, "zfcp://") == boot->bootmap)
		msg = action_bootmap_boot_zfcp(boot);
	else
		cfg_strinitcpy(&msg, "Unsupported bootmap URI scheme.");

	return msg;
}


/**
 * Reboot method: reboot system by calling /sbin/reboot. On success function
 * does not return. On error a dynamically allocated error message
 * is returned.
 *
 * \return
 */

static char *
action_reboot()
{
	char *msg, *argv[2];
	int status;

	argv[0] = SYSLOAD_REBOOT_CMD;
	argv[1] = NULL;
	status = systemv(argv[0], argv);

	cfg_strinit(&msg);
	cfg_strprintf(&msg, "reboot command failed with return code %i.",
	    status);

	return msg;
}


/**
 * Halt method: halt system by calling /sbin/halt. On success function
 * does not return. On error a dynamically allocated error message
 * is returned.
 *
 * \return
 */

static char *
action_halt()
{
	char *msg, *argv[2];
	int status;

	argv[0] = SYSLOAD_HALT_CMD;
	argv[1] = NULL;
	status = systemv(argv[0], argv);

	cfg_strinit(&msg);
	cfg_strprintf(&msg, "halt command failed with return code %i.",
	    status);

	return msg;
}


/**
 * start a shell
 *
 * \return
 */

char *
action_shell()
{
	char *msg;
	int status;

	status = cfg_system(SYSLOAD_SHELL_CMD);

	cfg_strinit(&msg);
	cfg_strprintf(&msg, "shell command returned with return code %i.",
	    status);

	return msg;
}


/**
 * exit from sysload
 *
 * \return
 */

static char *
action_exit()
{
	char *msg, *argv[2];
	int status;

	exit( 0); // TODO: replace by a better implementation

	argv[0] = SYSLOAD_SHELL_CMD;
	argv[1] = NULL;
	status = systemv(argv[0], argv);

	cfg_strinit(&msg);
	cfg_strprintf(&msg, "shell command returned with return code %i.",
	    status);

	return msg;
}


/**
 * Analyse boot entry and call appropriate handler function to perform boot
 * action. On success function does not return. On error a dynamically
 * allocated error message is returned.
 *
 * \param[in] boot Pointer to cfg_bentry structure with boot information.
 * \return         In case of error dynamically allocated error message.
 */

char *
loader(struct cfg_bentry *boot)
{
	char *msg = NULL;

	DG_ENTER( DG_VERBOSE);
	// fork to function handling request
	// on success handler functions do not return
	switch (boot->action) {
	case KERNEL_BOOT:
		msg = action_kernel_boot(boot);
		break;

	case INSFILE_BOOT:
		msg = action_insfile_boot(boot);
		break;

	case BOOTMAP_BOOT:
		msg = action_bootmap_boot(boot);
		break;

	case REBOOT:
		msg = action_reboot();
		break;

	case HALT:
		msg = action_halt();
		break;

	case SHELL:
		msg = action_shell();
		break;

	case EXIT:
		msg = action_exit();
		break;

	default:
		cfg_strinit(&msg);
		cfg_strprintf(&msg, "Invalid boot action %i selected.",
		    boot->action);
	}

	DG_RETURN( DG_VERBOSE, msg);
}
