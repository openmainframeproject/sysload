/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file netbase.h
 * \brief information to communicate network settings
 *
 * \author Michael Loehr (mloehr@de.ibm.com)
 *
 * $Id: netbase.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _NETBASE_H_
#define _NETBASE_H_


/**
 * setup mode for this interface
 */
enum nb_mode {
	NB_STATIC,
	NB_DHCP,
};


/**
 * This structure describes the configuration of a network interface
 */

struct nb_conf {
	enum nb_mode mode;
	char *interface;
	char *address;
	char *mask;
	char *gateway;
	char *nameserver;
	char *domain;
};


struct nb_conf *nb_conf_new();
void nb_conf_init(struct nb_conf *netconf);
void nb_conf_destroy(struct nb_conf *netconf);
void nb_conf_reset(struct nb_conf *netconf);
void nb_conf_free(struct nb_conf **netconf);
void nb_conf_print(struct nb_conf *netconf);
void nb_conf_enable(struct nb_conf *netconf);

int nb_test_mac (const char *macaddr);

#endif /* #ifndef _NETBASE_H_ */
