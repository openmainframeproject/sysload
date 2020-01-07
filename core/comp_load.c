/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file comp_load.c
 * \brief Component loader for System Loader
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 *
 * $Id: comp_load.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include "sysload.h"


/**
 * Verify that URI starts with a valid URI scheme.
 *
 * \param[in] uri  URI to verify.
 * \return         Zero for correct URI scheme, non zero otherwise.
 */

int verify_uri_scheme(const char *uri)
{
	const char *colon_ptr, *ptr;

	// valid URIs it must contain a ':'
	colon_ptr = strchr(uri, ':');
	if (!colon_ptr)
		return -1;

	// valid URIs must start with a alphabetic character
	if (!islower(uri[0]) && !isupper(uri[0]))
		return -1;

	// all remaining characters before ':' must be either alphabetic
	// characters or digits or '+' or '-' or '.'
	for (ptr = uri+1; ptr < colon_ptr; ptr++) {
		if (!islower(*ptr) && !isupper(*ptr) && !isdigit(*ptr) &&
		    *ptr != '+' && *ptr != '-' && *ptr != '.')
			return -1;
	}

	return 0;
}


/**
 * Read data from file descriptor and append to string.
 *
 * \param[in]      fd  File descriptor to read from.
 * \param[in,out]  str Pointer to accumulation string.
 * \return         Zero on successfull read, non zero on read error or
 *                 not data available.
 */

int
read_to_string(int fd, char **str)
{
	char buffer[512];
	int err;

	err = read(fd, buffer, sizeof(buffer)-1);
	if (err>0) {
		buffer[err] = '\0';
		cfg_strcat(str, buffer);
		return 0;
	} else
		return -1;
}


/**
 * Access file specified by a URI and create a copy on a local filesystem.
 *
 * \param[in]   dest    Pathname of local copy
 * \param[in]   uri     URI of source file
 * \param[out]  info    Dynamically allocated buffer with informational
 *                      messages from loader module. If set to \p NULL no
 *                      memory is allocated.
 * \param[out]  errmsg  Dynamically allocated buffer with error messages
 *                      from loader module.If set to \p NULL no memory is
 *                      allocated.
 * \return  On success zero is returned. On error the return value in non-zero.
 */

int
comp_load(const char *dest, const char *uri, char **info, char **errmsg)
{
	char *colon_ptr = NULL, *uri_scheme = NULL, *module = NULL;
	char *int_info = NULL, *int_errmsg = NULL, *defaultpath = NULL;
	int fd_stdout[2], fd_stderr[2], read_flags = 0x0, ret;
	pid_t pid;
	fd_set read_set;

	cfg_strinit(&module);
	cfg_strinit(&int_info);
	cfg_strinit(&int_errmsg);
	cfg_strinit(&defaultpath);

	// extract URI scheme to identify loader module
	if (verify_uri_scheme(uri)) {
		cfg_strcpy(&int_errmsg, "Invalid URI scheme.");
		ret = -1;
		goto cleanup;
	}
	colon_ptr = strchr(uri, ':');
	cfg_strinit(&uri_scheme);
	cfg_strncpy(&uri_scheme, uri, colon_ptr-uri);

	// try to get our installation directory out of CFG_PATH environment
	// variable, otherwise use default path
	cfg_get_env_str(CFG_PATH, &defaultpath);
	if (strlen(defaultpath) == 0) {
		cfg_strcpy(&defaultpath, CFG_DEFAULTPATH);
	}
	cfg_strprintf(&module, "%s/%s/cl_%s",
	    defaultpath, COMP_LOAD_MODULE_PATH, uri_scheme);
	cfg_strfree(&uri_scheme);

	// fork loader module
	pipe(fd_stdout);
	pipe(fd_stderr);
	pid = fork();
	if (pid == 0)
	{
		close(fd_stdout[0]);
		close(fd_stderr[0]);
		dup2(fd_stdout[1], 1);
		dup2(fd_stderr[1], 2);
		execl(module, module, dest, uri, NULL);
		fprintf(stderr, "Error executing loader module '%s' - %s.",
		    module, strerror(errno));
		_exit(127);
	}
	close(fd_stdout[1]);
	close(fd_stderr[1]);

	// read loader module output
	FD_ZERO(&read_set);
	FD_SET(fd_stdout[0], &read_set);
	FD_SET(fd_stderr[0], &read_set);
	do {
		select(FD_SETSIZE, &read_set, NULL, NULL, NULL);
		if (FD_ISSET(fd_stdout[0], &read_set)) {
			if (read_to_string(fd_stdout[0], &int_info))
				read_flags |= 0x01;
		}
		if (FD_ISSET(fd_stderr[0], &read_set)) {
			if (read_to_string(fd_stderr[0], &int_errmsg))
				read_flags |= 0x02;
		}
		FD_ZERO(&read_set);
		if (!(read_flags & 0x01))
			FD_SET(fd_stdout[0], &read_set);
		if (!(read_flags & 0x02))
			FD_SET(fd_stderr[0], &read_set);
	} while (read_flags != 0x03);
	wait4(pid, &ret, 0, NULL);
	if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
		ret = 0;
	else
		ret = -1;

 cleanup:
	if (info && strlen(int_info))
		cfg_strinitcpy(info, int_info);
	if (errmsg && strlen(int_errmsg))
		cfg_strinitcpy(errmsg, int_errmsg);
	cfg_strfree(&module);
	cfg_strfree(&int_info);
	cfg_strfree(&int_errmsg);
	cfg_strfree(&defaultpath);

	return ret;
}

