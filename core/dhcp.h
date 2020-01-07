/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file dhcp.h
 * \brief Include file for sysload DHCP data module.
 *
 * \author Swen Schillig (swen@vnet.ibm.com)
 *
 * $Id: dhcp.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _DHCP_H_
#define _DHCP_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define DHCP_CMD      "/sbin/dhcpcd"
#define DHCP_TEMP_DIR "/tmp/"
#define DHCP_MAX_RETRIES 5

struct dhcp_request {
	char *ipaddr;
	char *netmask;
	char *network;
	char *broadcast;
	char *gateway;
	char *domain;
	char **dns;      //string list !
	char *dhcpsid;
	char *dhcpgiaddr;
	char *dhcpsiaddr;
	char *dhcpchaddr;
	char *dhcpshaddr;
	char *dhcpsname;
	long leasetime;
	long renewaltime;
	long rebindtime;
	char *interface;
	char *classid;
	char *clientid;
};

struct dhcp_request * dhcp_req_init( struct dhcp_request *);
struct dhcp_request * dhcpcd(char *, struct dhcp_request *, int timeout);

#endif /* #ifndef _DHCP_H_ */
