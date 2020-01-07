/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file modbase.c
 * \brief Information to communicate module settings
 *
 * \author Michael Loehr (mloehr@de.ibm.com)
 *
 * $Id: modbase.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <string.h>
#include "config.h"
#include "debug.h"
#include "modbase.h"


struct mb_conf *mb_conf_new()
{
	struct mb_conf *conf_to_init = NULL;

	conf_to_init = malloc(sizeof(*conf_to_init));
	MEM_ASSERT(conf_to_init);

	mb_conf_init(conf_to_init);
	return(conf_to_init);
}


void mb_conf_init(struct mb_conf *modconf)
{
	// intialize all members
	cfg_strinit(&modconf->name);
	cfg_strinit(&modconf->param);
	cfg_strinit(&modconf->kernelversion);
}


void mb_conf_destroy(struct mb_conf *modconf)
{
	// free string members
	cfg_strfree(&modconf->name);
	cfg_strfree(&modconf->param);
	cfg_strfree(&modconf->kernelversion);
}


void mb_conf_reset(struct mb_conf *modconf)
{
	mb_conf_destroy(modconf);
	mb_conf_init(modconf);
}


void mb_conf_free(struct mb_conf **modconf)
{
	if (*modconf == NULL) return;
	free(*modconf);
	*modconf = NULL;
}


void mb_conf_print(struct mb_conf *modconf)
{
	printf("mb_conf:\n");
	printf("  name=%s\n", modconf->name);
	printf("  param=%s\n", modconf->param);
	printf("  kernelversion=%s\n", modconf->kernelversion);
}


void mb_conf_load(struct mb_conf *modconf)
{
	char *cmd = NULL;

	cfg_strinit(&cmd);
	cfg_strprintf(&cmd,
	    "/sbin/modprobe %s %s", modconf->name, modconf->param);
	cfg_system(cmd);
	cfg_strfree(&cmd);
}
