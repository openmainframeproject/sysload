/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file modbase.h
 * \brief Information to communicate module settings
 *
 * \author Michael Loehr (mloehr@de.ibm.com)
 *
 * $Id: modbase.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _MODBASE_H_
#define _MODBASE_H_


/**
 * This structure describes the information to load a module
 */

struct mb_conf {
	char *name;
	char *param;
	char *kernelversion;
};


struct mb_conf *mb_conf_new();
void mb_conf_init(struct mb_conf *modconf);
void mb_conf_destroy(struct mb_conf *modconf);
void mb_conf_reset(struct mb_conf *modconf);
void mb_conf_free(struct mb_conf **modconf);
void mb_conf_print(struct mb_conf *modconf);
void mb_conf_load(struct mb_conf *modconf);


#endif /* #ifndef _MODBASE_H_ */
