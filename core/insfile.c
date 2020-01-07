/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file insfile.c
 * \brief Analyzes a given insfile and collects all referenced boot files
 *
 * \author Michael Loehr (mloehr@de.ibm.com)
 *
 * $Id: insfile.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sysload.h"
#include "insfile.h"


/**
 * get the path (everything except the filename at the end)
 * from an uri
 *
 * \param[out] uri_path string to be initialized with the path
 * \param[in]  uri      URI to be analyzed
 */
void get_uri_path(char **uri_path, const char *uri)
{
  int i = 0;
  int copylen = strlen(uri);

  cfg_strinit(uri_path);

  for (i = strlen(uri)-1; i >= 0; i--) {
    if (uri[i] == '/') {
      copylen = i+1; // everything including the last '/'
      break;
    }
  }

  cfg_strncpy(uri_path, uri, copylen);
}


/**
 * takes the info from a single insfile line and determines the type
 * of this insfile component. implements a kind of 'voting' mechanism
 * to identify the file type. this allows to add more rules to
 * increase the reliability of the identification.
 *
 * \param[in] path path in the local file system
 * \param[in] name name of the file
 * \param[in] addr load address for this insfile component
 * \return         type of the insfile component
 */
enum ins_component_type identify_file(const char *name, long addr)
{
  FILE *file = NULL;
  struct stat fileinfo;
  int probability[INS_LAST];
  int type = INS_OTHER;
  int highest_probability = 0;
  int most_probable = INS_OTHER;
  int signature = 0;
  int i = 0;
  int ch = 0;

  for (type = INS_KERNEL; type < INS_LAST; type++)
    probability[type] = 0;

  if (stat(name, &fileinfo) != 0) {
    fprintf(stderr, "%s: stat not possible for %s\n", arg0, name);
    goto cleanup;
  }

  // now we have some rules of thumb:

  // is it a big file ?
  if (fileinfo.st_size > (512*1024)) {
    probability[INS_KERNEL]++;
    probability[INS_INITRD]++;

    // ... and first 2 bytes are a 0x1f8b gzip signature ?
    file = fopen(name, "r");
    if (!file) {
      fprintf(stderr, "%s: fopen not possible for %s\n", arg0, name);
      goto cleanup;
    }
    for (i = 0; i < 2; i++) {
      signature = (signature<<8) | fgetc(file);
    }
    fclose(file);
    if (signature == 0x1f8b) {
      probability[INS_INITRD]++;
    }
  }

  // size is exactly 4 bytes
  if (fileinfo.st_size == 4) {
    probability[INS_INITRDSIZE]++;
  }

  // size is in the valid range for a kernel commandline
  if (fileinfo.st_size <= SYSLOAD_MAX_KERNEL_CMDLINE) {
    probability[INS_PARMFILE]++;

    // ... and file contains only valid printable characters
    file = fopen(name, "r");
    if (!file) {
      fprintf(stderr, "%s: fopen not possible for %s\n", arg0, name);
      goto cleanup;
    }
    probability[INS_PARMFILE]++; // assume only valid characters
    while ((ch = fgetc(file)) != EOF) {
      if (!isprint(ch)) {
	probability[INS_PARMFILE]--; // assumption was wrong
	break;
      }
    }
    fclose(file);
  }

  // have a look on the addresses:
  if (addr == 0x00000000)
    probability[INS_KERNEL]++;

  if (addr == 0x00010480)
    probability[INS_PARMFILE]++;

  if (addr == 0x00800000)
    probability[INS_INITRD]++;

  if (addr == 0x00010414)
    probability[INS_INITRDSIZE]++;

  // finally we select the most probable filetype
  for (type = INS_KERNEL; type < INS_LAST; type++) {
    if (probability[type] > highest_probability) {
      highest_probability = probability[type];
      most_probable = type;
    }
  }

 cleanup:

  return most_probable;
}


/**
 * Insfile boot method: Read and analyse INSFILE, copy files to local
 * filesystem and call \p kexec to boot new kernel. On success function
 * does not return. On error a dynamically allocated error message
 * is returned.
 *
 * \param boot  Pointer to boot entry.
 * \return      Dynamically allocated error message.
 */
char *action_insfile_boot(struct cfg_bentry *boot)
{
  char *msg = NULL;
  char *tmp_uri = NULL;
  char *uri_path = NULL;
  char *tmp_msg = NULL;
  char *local_name = NULL;
  char *filename = NULL;
  FILE *insfile = NULL; /*!< insfile to read from */
  char input[CFG_STR_MAX_LEN] = ""; /* input line from insfile */
  char input_name[CFG_STR_MAX_LEN] = "";
  long addr = 0;
  char *cmdline = 0;
  int  linecount = 0;

  cfg_strinit(&msg);
  cfg_strinit(&local_name);
  cfg_strinit(&cmdline);
  cfg_strinit(&filename);

  // get the insfile
  if (strlen(boot->insfile)) {
    tmp_uri = prefix_root(boot->root, boot->insfile);
    if (comp_load(SYSLOAD_FILENAME_INSFILE, tmp_uri, NULL, &tmp_msg)) {
	    cfg_strprintf(&msg, "Error loading insfile '%s' - %s",
		tmp_uri, tmp_msg);
      goto cleanup;
    }

    get_uri_path(&uri_path, tmp_uri);
    cfg_strfree(&tmp_uri);
  }

  // get filenames from insfile
  insfile = fopen(SYSLOAD_FILENAME_INSFILE, "r");
  if (!insfile) {
    cfg_strprintf(&msg, "Unable to open insfile - %s\n",
		  SYSLOAD_FILENAME_INSFILE);
    goto cleanup;
  }

  while (fgets(input, CFG_STR_MAX_LEN, insfile) != NULL) {
    if (input[0] != '*') { // comments start with '*'
	    sscanf(input, "%s %lx", (char *) (&input_name), &addr);
      cfg_strcpy(&filename, input_name);
      linecount++;

      // get the file referenced by the insfile
      if (strlen(filename)) {
	tmp_uri = prefix_root(uri_path, filename);

	cfg_strprintf(&local_name, "/tmp/file_%d", linecount);

	if (comp_load(local_name, tmp_uri, NULL, &tmp_msg)) {
		cfg_strprintf(&msg, "Error loading insfile component '%s' - %s",
		    filename, tmp_msg);
	  goto cleanup;
	}

	switch (identify_file(local_name, addr)) {
	case INS_KERNEL:
	  rename(local_name, SYSLOAD_FILENAME_KERNEL);
	  break;

	case INS_INITRD:
	  rename(local_name, SYSLOAD_FILENAME_INITRD);
	  break;

	case INS_PARMFILE:
	  rename(local_name, SYSLOAD_FILENAME_PARMFILE);
	  break;

	case INS_INITRDSIZE: // from here on nothing usefull
	case INS_OTHER:
	default:
	  unlink(local_name);
	}
      }
    }
  }

  fclose(insfile);
  unlink(SYSLOAD_FILENAME_INSFILE);

  // if all required files exist
  if ((access(SYSLOAD_FILENAME_KERNEL, F_OK) == 0) &&
      (access(SYSLOAD_FILENAME_INITRD, F_OK) == 0) &&
      (access(SYSLOAD_FILENAME_PARMFILE, F_OK) == 0)) {

    msg = compose_commandline(&cmdline, SYSLOAD_FILENAME_PARMFILE,
			      boot->cmdline);
    if (msg)
      goto cleanup;

    msg = kexec(SYSLOAD_FILENAME_KERNEL, SYSLOAD_FILENAME_INITRD, cmdline);
  }

  cfg_strfree(&cmdline);

 cleanup:

  cfg_strfree(&local_name);
  cfg_strfree(&cmdline);
  cfg_strfree(&filename);

  unlink(SYSLOAD_FILENAME_KERNEL);
  unlink(SYSLOAD_FILENAME_INITRD);
  unlink(SYSLOAD_FILENAME_PARMFILE);

  return msg;
}
