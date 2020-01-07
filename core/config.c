/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file config.c
 * \brief General functions for handling of configuration data structures.
 *
 * \author Ralph Wuerthner (rwuerthn@de.ibm.com)
 * \author Michael Loehr   (mloehr@de.ibm.com)
 *
 * $Id: config.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <glob.h>
#include "config.h"
#include "debug.h"

#define BOOL2STR(bool) ((bool) ? "True" : "False")


/**
 * Create and initialize a cfg_toplevel structure.
 *
 * \return Pointer to cfg_toplevel structure created and initialized.
 */

struct cfg_toplevel *cfg_new()
{
	struct cfg_toplevel *config_to_init = NULL;

	config_to_init = malloc(sizeof(*config_to_init));
	MEM_ASSERT(config_to_init);

	cfg_init(config_to_init);
	return(config_to_init);
}


/**
 * Initialize a cfg_toplevel structure.
 *
 * \param[in] config Pointer to cfg_toplevel structure to be initialized.
 */

void cfg_init(struct cfg_toplevel *config)
{
	memset(config, 0x0, sizeof(*config));

	// intialize string members
	cfg_strinit(&config->password);
}


/**
 * Destroy a cfg_toplevel structure and free all alocated memory.
 *
 * \param[in] config Pointer to cfg_toplevel structure to be destroyed.
 */

void cfg_destroy(struct cfg_toplevel *config)
{
	// destroy lists
	cfg_userinterface_list_destroy(config);
	cfg_bentry_list_destroy(config);

	// free string members
	cfg_strfree(&config->password);
}


/**
 * Destroy user interface list in cfg_toplevel structure.
 *
 * \param[in] config Pointer to cfg_toplevel structure.
 */

void cfg_userinterface_list_destroy(struct cfg_toplevel *config)
{
	int n;

	for (n = 0; n < config->ui_count; n++)
		cfg_userinterface_destroy(&config->ui_list[n]);
	free(config->ui_list);
	config->ui_list = NULL;
	config->ui_count = 0;
}


/**
 * Destroy boot entry list in cfg_toplevel structure.
 *
 * \param[in] config Pointer to cfg_toplevel structure.
 */

void cfg_bentry_list_destroy(struct cfg_toplevel *config)
{
	int n;

	for (n = 0; n < config->bentry_count; n++)
		cfg_bentry_destroy(&config->bentry_list[n]);
	free(config->bentry_list);
	config->bentry_list = NULL;
	config->bentry_count = 0;
}


/**
 * Copy a cfg_toplevel structure to an initialized destination
 * cfg_toplevel structure.
 *
 * \param[out] dest Pointer to initialized destination cfg_toplevel structure.
 * \param[in]  src  Pointer to source cfg_toplevel structure.
 */

void cfg_copy(struct cfg_toplevel *dest, const struct cfg_toplevel *src)
{
	int n;

	// destroy old lists
	cfg_userinterface_list_destroy(dest);
	cfg_bentry_list_destroy(dest);

	// copy user interface list
	for (n = 0; n < src->ui_count; n++)
		cfg_add_userinterface(dest, &src->ui_list[n]);

	// copy boot entry list
	for (n = 0; n < src->bentry_count; n++)
		cfg_add_bentry(dest, &src->bentry_list[n]);
}


/**
 * Copy a cfg_toplevel structure to an uninitialized destination
 * cfg_toplevel structure.
 *
 * \param[out] dest Pointer to uninitialized destination cfg_toplevel
 *                  structure.
 * \param[in]  src  Pointer to source cfg_toplevel structure.
 */

void cfg_initcopy(struct cfg_toplevel *dest, const struct cfg_toplevel *src)
{
	cfg_init(dest);
	cfg_copy(dest, src);
}


/**
 * Create and initialize a cfg_userinterface structure.
 *
 * \return a pointer to the cfg_userinterface structure
 */

struct cfg_userinterface *cfg_userinterface_new()
{
	struct cfg_userinterface *ui_to_init = NULL;

	ui_to_init = malloc(sizeof(*ui_to_init));
	MEM_ASSERT(ui_to_init);

	cfg_userinterface_init(ui_to_init);
	return(ui_to_init);
}


/**
 * Initialize a cfg_userinterface structure.
 *
 * \param[in] ui Pointer to cfg_userinterface structure to be initialized.
 */

void cfg_userinterface_init(struct cfg_userinterface *ui)
{
	memset(ui, 0x0, sizeof(*ui));

	// intialize string members
	cfg_strinit(&ui->module);
	cfg_strinit(&ui->cmdline);
}


/**
 * Initialize a cfg_userinterface structure.
 *
 * \param[in] ui Pointer to cfg_userinterface structure to be initialized.
 */

void cfg_userinterface_set(struct cfg_userinterface *ui,
    char *module_name, char *cmd)
{
	memset(ui, 0x0, sizeof(*ui));

	// intialize string members
	cfg_strinitcpy(&(ui->module), module_name);
	cfg_strinitcpy(&(ui->cmdline), cmd);
}


/**
 * Destroy a cfg_userinterface structure and free all alocated memory.
 *
 * \param[in] ui Pointer to cfg_userinteface structure to be destroyed.
 */

void cfg_userinterface_destroy(struct cfg_userinterface *ui)
{
	// free string members
	cfg_strfree(&ui->module);
	cfg_strfree(&ui->cmdline);
}


/**
 * Copy a cfg_userinterface structure to an already initialized destination
 * cfg_userinterface structure.
 *
 * \param[out] dest Pointer to initialized destination cfg_userinteface
 * structure.
 * \param[in]  src  Pointer to source cfg_userinteface structure.
 */

void cfg_userinterface_copy(struct cfg_userinterface *dest,
    const struct cfg_userinterface *src)
{
	// copy string members
	cfg_strcpy(&dest->module, src->module);
	cfg_strcpy(&dest->cmdline, src->cmdline);
}


/**
 * Copy a cfg_userinterface structure to an uninitialized destination
 * cfg_userinterface structure.
 *
 * \param[out] dest Pointer to uninitialized destination cfg_userinteface
 * structure.
 * \param[in]  src  Pointer to source cfg_userinteface structure.
 */

void cfg_userinterface_initcopy(struct cfg_userinterface *dest,
    const struct cfg_userinterface *src)
{
	cfg_userinterface_init(dest);
	cfg_userinterface_copy(dest, src);
}


/**
 * Create and initialize a cfg_bentry structure.
 *
 * \return a pointer to the cfg_bentry structure
 */

struct cfg_bentry *cfg_bentry_new()
{
	struct cfg_bentry *bentry_to_init = NULL;

	bentry_to_init = malloc(sizeof(*bentry_to_init));
	MEM_ASSERT(bentry_to_init);

	cfg_bentry_init(bentry_to_init);
	return(bentry_to_init);
}


/**
 * Initialize a cfg_bentry structure.
 *
 * \param[in] bentry Pointer to cfg_bentry structure to be initialized.
 */

void cfg_bentry_init(struct cfg_bentry *bentry)
{
	memset(bentry, 0x0, sizeof(*bentry));

	// intialize string members
	cfg_strinit(&(bentry->title));
	cfg_strinit(&(bentry->label));
	cfg_strinit(&(bentry->root));
	cfg_strinit(&(bentry->kernel));
	cfg_strinit(&(bentry->initrd));
	cfg_strinit(&(bentry->cmdline));
	cfg_strinit(&(bentry->parmfile));
	cfg_strinit(&(bentry->insfile));
	cfg_strinit(&(bentry->bootmap));
	cfg_strinit(&(bentry->pause));
}


/**
 * Destroy a cfg_bentry structure and free all alocated memory.
 *
 * \param[in] bentry Pointer to cfg_bentry structure to be destroyed.
 */

void cfg_bentry_destroy(struct cfg_bentry *bentry)
{
	cfg_strfree(&(bentry->title));
	cfg_strfree(&(bentry->label));
	cfg_strfree(&(bentry->root));
	cfg_strfree(&(bentry->kernel));
	cfg_strfree(&(bentry->initrd));
	cfg_strfree(&(bentry->cmdline));
	cfg_strfree(&(bentry->parmfile));
	cfg_strfree(&(bentry->insfile));
	cfg_strfree(&(bentry->bootmap));
	cfg_strfree(&(bentry->pause));
}


/**
 *
 * \param[in] bentry
 */

void cfg_bentry_free(struct cfg_bentry **bentry)
{
	if (*bentry == NULL) return;
	free(*bentry);
	*bentry = NULL;
}


/**
 * Copy a cfg_bentry structure to an already initialized destination
 * cfg_bentry structure.
 *
 * \param[out] dest Pointer to initialized destination cfg_bentry structure.
 * \param[in]  src  Pointer to source cfg_bentry structure.
 */

void cfg_bentry_copy(struct cfg_bentry *dest, const struct cfg_bentry *src)
{
	// copy non string members
	dest->locked = src->locked;
	dest->action = src->action;

	// copy string members
	cfg_strcpy(&dest->title, src->title);
	cfg_strcpy(&dest->label, src->label);
	cfg_strcpy(&dest->root, src->root);
	cfg_strcpy(&dest->kernel, src->kernel);
	cfg_strcpy(&dest->initrd, src->initrd);
	cfg_strcpy(&dest->cmdline, src->cmdline);
	cfg_strcpy(&dest->parmfile, src->parmfile);
	cfg_strcpy(&dest->insfile, src->insfile);
	cfg_strcpy(&dest->bootmap, src->bootmap);
	cfg_strcpy(&dest->pause, src->pause);
}


/**
 * Copy a cfg_bentry structure to an uninitialized destination
 * cfg_bentry structure.
 *
 * \param[out] dest Pointer to uninitialized destination cfg_bentry structure.
 * \param[in]  src  Pointer to source cfg_bentry structure.
 */

void cfg_bentry_initcopy(struct cfg_bentry *dest, const struct cfg_bentry *src)
{
	cfg_bentry_init(dest);
	cfg_bentry_copy(dest, src);
}


/**
 * Create a copy a user interface element and add copy to user interface
 * list in config.
 * structure.
 *
 * \param[in,out]  config  Pointer to cfg_toplevel structure.
 * \param[in]      ui      Pointer to cfg_userinterface structure.
 */

void cfg_add_userinterface(struct cfg_toplevel *config,
    const struct cfg_userinterface *ui)
{
	// increase ui_list
	config->ui_list =
		realloc(config->ui_list,
		    sizeof(struct cfg_userinterface)*(config->ui_count+1));
	MEM_ASSERT(config->ui_list);

	// copy ui to ui_list
	cfg_userinterface_initcopy(&config->ui_list[config->ui_count], ui);
	config->ui_count++;
	dg_printf(DG_VERBOSE, "%s\n", __FUNCTION__);
}


/**
 * Create a copy a boot entry element and add copy to boot entry list
 * in config.
 *
 * \param[in,out]  config  Pointer to cfg_toplevel structure.
 * \param[in]      bentry  Pointer to cfg_bentry structure.
 */

void cfg_add_bentry(struct cfg_toplevel *config,
    const struct cfg_bentry *bentry)
{
	// increase bentry_list
	config->bentry_list =
		realloc(config->bentry_list,
		    sizeof(struct cfg_bentry)*(config->bentry_count+1));
	MEM_ASSERT(config->bentry_list);

	// copy bentry to bentry_list
	cfg_bentry_initcopy(&config->bentry_list[config->bentry_count],
	    bentry);
	config->bentry_count++;
	// This line breaks the UI in debug mode
	// dg_printf(DG_VERBOSE, "%s\n", __FUNCTION__);
}


/**
 * Print contents of a cfg_toplevel structure as debug output.
 *
 * \param[in] config Pointer to cfg_toplevel structure to be printed.
 */

void cfg_print(struct cfg_toplevel *config)
{
	int n;

	dg_printf(DG_VERBOSE, "  timeout=%d\n", config->timeout);
	dg_printf(DG_VERBOSE, "  default=%d\n", config->boot_default);
	dg_printf(DG_VERBOSE, "  password='%s'\n", config->password);
	for (n = 0; n < config->ui_count; n++) {
		dg_printf(DG_VERBOSE,"  userinterface[%d]='%s' '%s'\n", n,
		    config->ui_list[n].module, config->ui_list[n].cmdline);
	}
	for (n = 0; n < config->bentry_count; n++) {
		dg_printf(DG_VERBOSE,"\n  bentry[%d]:\n", n);
		dg_printf(DG_VERBOSE,
		    "    title='%s'\n", config->bentry_list[n].title);
		dg_printf(DG_VERBOSE,
		    "    label='%s'\n", config->bentry_list[n].label);
		dg_printf(DG_VERBOSE,
		    "    root='%s'\n", config->bentry_list[n].root);
		dg_printf(DG_VERBOSE,
		    "    kernel='%s'\n", config->bentry_list[n].kernel);
		dg_printf(DG_VERBOSE,
		    "    initrd='%s'\n", config->bentry_list[n].initrd);
		dg_printf(DG_VERBOSE,
		    "    cmdline='%s'\n", config->bentry_list[n].cmdline);
		dg_printf(DG_VERBOSE,
		    "    parmfile='%s'\n", config->bentry_list[n].parmfile);
		dg_printf(DG_VERBOSE,
		    "    insfile='%s'\n", config->bentry_list[n].insfile);
		dg_printf(DG_VERBOSE,
		    "    bootmap='%s'\n", config->bentry_list[n].bootmap);
		dg_printf(DG_VERBOSE,
		    "    locked=%s\n",BOOL2STR(config->bentry_list[n].locked));
		dg_printf(DG_VERBOSE,
		    "    pause='%s'\n", config->bentry_list[n].pause);
		dg_printf(DG_VERBOSE,
		    "    action=%d\n", config->bentry_list[n].action);
	}
}


/**
 * Print a 'key' 'value' pair to stdout if both strings are not empty.
 *
 * \param[in] ident  key string.
 * \param[in] value  value string.
 */

static void print_if_available(const char *ident, const char *value)
{
	if (ident == 0 || value == 0)
		return;

	if (strlen(ident) == 0 || strlen(value) == 0)
		return;

	printf("%s %s\n", ident, value);
}


/**
 * Print contents of a cfg_bentry structure to stdout.
 * The print format is identical to the format of a boot_entry
 * section in the config file.
 *
 * \param[in] bentry Pointer to cfg_bentry structure to be printed.
 */

void cfg_bentry_print(struct cfg_bentry *bentry)
{
	printf("boot_entry {\n");
	print_if_available("title", bentry->title);
	print_if_available("label", bentry->label);
	switch (bentry->action) {
	case REBOOT:
		printf("reboot\n");
		break;
	case HALT:
		printf("halt\n");
		break;
	case SHELL:
		printf("shell\n");
		break;
	case EXIT:
		printf("exit\n");
		break;
	default:
		print_if_available("root", bentry->root);
		print_if_available("kernel", bentry->kernel);
		print_if_available("initrd", bentry->initrd);
		print_if_available("cmdline", bentry->cmdline);
		print_if_available("parmfile", bentry->parmfile);
		print_if_available("insfile", bentry->insfile);
		print_if_available("bootmap", bentry->bootmap);
	}
	printf("}\n");
	return;
}


/**
 * Write a string variable to the environment.
 *
 * \param[in] variable name of the variable to be written.
 * \param[in] value value of the variable to be written.
 */

void cfg_set_env_str(const char *variable, const char *value)
{
	char *lost_str = NULL;

	cfg_strinit(&lost_str);
	cfg_strprintf(&lost_str, "%s_%s=%s", CFG_PREFIX, variable, value);
	dg_printf(DG_MAXIMAL, "%s:%s\n", __FUNCTION__, lost_str);
	/* lost_str becomes 'forever' part of the environment! */
	if (putenv(lost_str) !=0 ) {
		fprintf(stderr, "%s:putenv failed\n", arg0);
	}
}


/**
 * Read a string variable from the environment.
 *
 * \param[in] variable name of the variable to be read.
 * \param[out] value value of the variable to be read.
 */

void cfg_get_env_str(const char *variable, char **value)
{
	char *varstr = NULL;
	char *outstr = NULL;

	cfg_strinit(&varstr);
	cfg_strprintf(&varstr, "%s_%s", CFG_PREFIX, variable);
	outstr = getenv(varstr);
	if (outstr != NULL) {
		cfg_strcpy(value, outstr);
	}
	cfg_strfree(&varstr);
}


/**
 * Write an integer variable to the environment.
 *
 * \param[in] variable name of the variable to be written.
 * \param[in] value value of the variable to be written.
 */

void cfg_set_env_int(const char *variable, int value)
{
	char *lost_str = NULL;

	cfg_strinit(&lost_str);
	cfg_strprintf(&lost_str, "%s_%s=%d", CFG_PREFIX, variable, value);
	dg_printf(DG_MAXIMAL, "%s:%s\n", __FUNCTION__, lost_str);
	/* lost_str becomes forever part of the environment! */
	if (putenv(lost_str) != 0) {
		printf("putenv failed\n");
	}
}


/**
 * Read an integer variable from the environment.
 *
 * \param[in] variable name of the variable to be read.
 * \param[out] value value of the variable to be read.
 */

void cfg_get_env_int(const char *variable, int *value)
{
	char *varstr = NULL;
	char *outstr = NULL;

	cfg_strinit(&varstr);
	cfg_strprintf(&varstr, "%s_%s", CFG_PREFIX, variable);
	outstr = getenv(varstr);
	if (outstr != NULL) {
		sscanf(outstr, "%d", value);
	}
	cfg_strfree(&varstr);
}


/**
 * Write a string with index number to the environment.
 *
 * \param[in] variable name of the environment variable to be written.
 * \param[in] index index number of the variable to be written.
 * \param[in] value value of the variable to be written.
 */

void cfg_set_env_str_i(const char *variable, int index, const char *value)
{
	char *outstr = NULL;

	cfg_strinit(&outstr);
	cfg_strprintf(&outstr, "B%d_%s", index, variable);
	cfg_set_env_str(outstr, value);
	cfg_strfree(&outstr);
}


/**
 * Write an integer with index number to the environment.
 *
 * \param[in] variable name of the environment variable to be written.
 * \param[in] index index number of the variable to be written.
 * \param[in] value value of the variable to be written.
 */

void cfg_set_env_int_i(const char *variable, int index, int value)
{
	char *outstr = NULL;

	cfg_strinit(&outstr);
	cfg_strprintf(&outstr, "B%d_%s", index, variable);
	cfg_set_env_int(outstr, value);
	cfg_strfree(&outstr);
}


/**
 * Read a string with index number from the environment.
 *
 * \param[in] variable name of the environment variable to be read.
 * \param[in] index index number of the variable to be read.
 * \param[in] value value of the variable to be read.
 */

void cfg_get_env_str_i(const char *variable, int index, char **value)
{
	char *outstr = NULL;

	cfg_strinit(&outstr);
	cfg_strprintf(&outstr, "B%d_%s", index, variable);
	cfg_get_env_str(outstr, value);
	cfg_strfree(&outstr);
}


/**
 * Read an integer with index number from the environment.
 *
 * \param[in] variable name of the environment variable to be read.
 * \param[in] index index number of the variable to be read.
 * \param[in] value value of the variable to be read.
 */

void cfg_get_env_int_i(const char *variable, int index, int *value)
{
	char *outstr = NULL;

	cfg_strinit(&outstr);
	cfg_strprintf(&outstr, "B%d_%s", index, variable);
	cfg_get_env_int(outstr, value);
	cfg_strfree(&outstr);
}


/**
 * Write a complete cfg_toplevel structure to the environment.
 *
 * \param[in] config cfg_toplevel structure to be written.
 */

void cfg_set_env(struct cfg_toplevel *config)
{
	int i = 0;
	struct cfg_bentry* bentry_i = NULL;

	cfg_set_env_str(CFG_VERSION, CFG_CONFIG_VERSION);
	cfg_set_env_int(CFG_DEFAULT, config->boot_default);
	cfg_set_env_int(CFG_TIMEOUT, config->timeout);
	cfg_set_env_str(CFG_PASSWORD, config->password);
	cfg_set_env_int(CFG_BENTRY_COUNT, config->bentry_count);
	for (i = 0; i < config->bentry_count; i++) {
		bentry_i = &(config->bentry_list[i]);
		cfg_set_env_str_i(CFG_TITLE, i, bentry_i->title);
		cfg_set_env_str_i(CFG_LABEL, i, bentry_i->label);
		cfg_set_env_str_i(CFG_ROOT, i, bentry_i->root);
		cfg_set_env_str_i(CFG_KERNEL, i, bentry_i->kernel);
		cfg_set_env_str_i(CFG_INITRD, i, bentry_i->initrd);
		cfg_set_env_str_i(CFG_CMDLINE, i, bentry_i->cmdline);
		cfg_set_env_str_i(CFG_PARMFILE, i, bentry_i->parmfile);
		cfg_set_env_str_i(CFG_INSFILE, i, bentry_i->insfile);
		cfg_set_env_str_i(CFG_BOOTMAP, i, bentry_i->bootmap);
		cfg_set_env_int_i(CFG_LOCKED, i, bentry_i->locked);
		cfg_set_env_str_i(CFG_PAUSE, i, bentry_i->pause);
		cfg_set_env_int_i(CFG_ACTION, i, bentry_i->action);
	}
}


/**
 * Read a complete cfg_toplevel structure from the environment.
 *
 * \param[in] config cfg_toplevel structure to be read.
 */

void cfg_get_env(struct cfg_toplevel *config)
{
	int i = 0;
	struct cfg_bentry* bentry_i = NULL;
	int max_entry_count = 0;

	/* insert version check here ... */

	cfg_get_env_int(CFG_DEFAULT, &(config->boot_default));
	cfg_get_env_int(CFG_TIMEOUT, &(config->timeout));
	cfg_get_env_str(CFG_PASSWORD, &(config->password));
	cfg_get_env_int(CFG_BENTRY_COUNT, &(max_entry_count));
	for (i = 0; i < max_entry_count; i++) {
		bentry_i = cfg_bentry_new();
		cfg_get_env_str_i(CFG_TITLE, i, &(bentry_i->title));
		cfg_get_env_str_i(CFG_LABEL, i, &(bentry_i->label));
		cfg_get_env_str_i(CFG_ROOT, i, &(bentry_i->root));
		cfg_get_env_str_i(CFG_KERNEL, i, &(bentry_i->kernel));
		cfg_get_env_str_i(CFG_INITRD, i, &(bentry_i->initrd));
		cfg_get_env_str_i(CFG_CMDLINE, i, &(bentry_i->cmdline));
		cfg_get_env_str_i(CFG_PARMFILE, i, &(bentry_i->parmfile));
		cfg_get_env_str_i(CFG_INSFILE, i, &(bentry_i->insfile));
		cfg_get_env_str_i(CFG_BOOTMAP, i, &(bentry_i->bootmap));
		cfg_get_env_int_i(CFG_LOCKED, i, &(bentry_i->locked));
		cfg_get_env_str_i(CFG_PAUSE, i, &(bentry_i->pause));
		cfg_get_env_int_i(CFG_ACTION, i, (int*)&(bentry_i->action));
		cfg_add_bentry(config, bentry_i);
	}
}


/**
 * Initialize a dynamically allocated string by allocating and
 * assigning memory for the empty string \p "".
 *
 * \param[in] str Pointer to string to be initialized.
 */

void cfg_strinit(char **str)
{
	*str = malloc(1);
	MEM_ASSERT(*str);
	**str = '\0';
}


/**
 * Free a dynamically allocated string and assign \p NULL.
 *
 * \param[in] str Pointer to string to be freed.
 */

void cfg_strfree(char **str)
{
	if (*str == NULL) return;
	free(*str);
	*str = NULL;
}


/**
 * Copy string to dynamically allocated memory. Destination
 * string must be already initialized as \p realloc() is used
 * to resize memory buffer.
 *
 * \param[in,out] dest Pointer to initialized destination string.
 * \param[in]     src  Source string.
 */

void cfg_strcpy(char **dest, const char *src)
{
	size_t len = strlen(src)+1;

	*dest = realloc(*dest, len);
	MEM_ASSERT(*dest);
	memcpy(*dest, src, len);
}


/**
 * Append source string to the content of the destination string.
 * Destination string must be already initialized.
 *
 * \param[in,out] dest Pointer to destination string.
 * \param[in]     src  Source string.
 */

void cfg_strcat(char **dest, const char *src)
{
	size_t oldlen = strlen(*dest);
	size_t srclen  = strlen(src);
	size_t newlen  = oldlen+srclen;
	char *newdest = malloc(newlen+1);

	MEM_ASSERT(newdest);
	memcpy(newdest, *dest, oldlen);
	memcpy(&(newdest[oldlen]), src, srclen);
	newdest[newlen] = '\0';
	cfg_strfree(dest);
	*dest = newdest;
}


/**
 * Copy at most \p len characters from source string to dynamically
 * allocated memory. Destination string must be already initialized
 * as \p realloc() is used to resize memory buffer.
 *
 * \param[in,out] dest Pointer to initialized destination string.
 * \param[in]     src  Source string.
 * \param[in]     len  Maximum number of characters to be copied
 */

void cfg_strncpy(char **dest, const char *src, size_t len)
{
	size_t slen = strlen(src);

	if (slen < len)
		len = slen;

	*dest = realloc(*dest, len+1);
	MEM_ASSERT(*dest);
	memcpy(*dest, src, len);
	(*dest)[len] = '\0';
}


/**
 * Copy string to dynamically allocated memory. Destination
 * string must be uninitialized.
 *
 * \param[in,out] dest Pointer to uninitialized destination string.
 * \param[in]     src  Source string.
 */

void cfg_strinitcpy(char **dest, const char *src)
{
	size_t len = strlen(src)+1;

	*dest = malloc(len);
	MEM_ASSERT(*dest);
	memcpy(*dest, src, len);
}


/**
 * Print string to dynamically allocated memory. Format string and
 * additional arguments to be passed must follow printf rules.
 * Destination string must be initialized.
 *
 * \param[in,out] str Pointer to initialized destination string.
 * \param[in]     format  printf format string.
 * \param[in]     ...     additional printf arguments
 */

void cfg_strprintf(char **str, const char *format, ...)
{
	va_list arg;

	va_start(arg, format);
	cfg_strvprintf(str, format, arg);
	// dg_printf(DG_VERBOSE, "%s:%s\n", __FUNCTION__, *str);
}


/**
 * Print string to dynamically allocated memory. Format string and
 * additional arguments to be passed must follow printf rules.
 * Destination string must be initialized.
 *
 * \param[in,out] str Pointer to initialized destination string.
 * \param[in]     format  printf format string.
 * \param[in]     arg     additional printf arguments
 */

void cfg_strvprintf(char **str, const char *format, va_list arg)
{
	va_list arg2;
	size_t len;

	va_copy(arg2, arg);
	len = vsnprintf(NULL, 0, format, arg2)+1;
	va_end(arg2);
	if (len >= 0) {
		*str = realloc(*str, len);
		MEM_ASSERT(*str);
		vsnprintf(*str, len, format, arg);
		va_end(arg);
	}
}


/**
 * A modified read that always reads until the buffer is filled up.
 *
 * \return read_len == count, 0 for EOF, -1 on error
 */

ssize_t cfg_read(int fd, void *buf, size_t count)
{
	ssize_t filled = 0;
	ssize_t read_len = 0;
	
	do {
		read_len = read( fd, buf+filled, count-filled);
		if (read_len > 0) {
			filled += read_len;
		} else if (read_len == 0) {
			// 0==EOF
			break;
		} else {
			filled = read_len;
			break;
		}

	} while( filled < count);

	return filled;
}


/**
 * enhanced system() call with output to syslog
 */
int cfg_system(const char *cmd)
{
	int retval = 0;

	dg_printf(DG_VERBOSE, "cfg_system(%s)->", cmd);
	retval = system(cmd);
	dg_printf(DG_VERBOSE, "%d\n", retval);
	return retval;
}


/**
 * get the defaultpath for the sysload package
 */
void cfg_get_defaultpath(char **defpath)
{
	// try to get our installation directory 
	// from CFG_PATH environment
	// variable, otherwise use default path
	cfg_get_env_str(CFG_PATH, defpath);
	if (strlen(*defpath) == 0) {
		cfg_strcpy(defpath, CFG_DEFAULTPATH);
	}
}

/**
 * extend the pattern with glob() and return first match in buffer
 *
 * Return: 1 for success, 0 if something went wrong
 */
int cfg_glob_filename(char* pattern, char* buffer, size_t buffer_size)
{
	int ret;
	glob_t globbuf;
	size_t size;

	ret = glob(pattern, 0, NULL, &globbuf);

	if(ret!=0)
		return 0;

	if(globbuf.gl_pathc == 0)
		return 0;

	size=strlen(globbuf.gl_pathv[0]);
	if(size+1 > buffer_size)
		return 0;

	strcpy(buffer, globbuf.gl_pathv[0]);

	return 1;
}

