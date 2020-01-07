/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file netbase.c
 * \brief information to communicate network settings
 *
 * \author Michael Loehr (mloehr@de.ibm.com)
 *
 * $Id: netbase.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include "config.h"
#include "debug.h"
#include "dhcp.h"
#include "netbase.h"


/**
 * Create and initialize a nb_conf structure.
 *
 * \return Pointer to nb_conf structure created and initialized.
 */

struct nb_conf *nb_conf_new()
{
	struct nb_conf *conf_to_init = NULL;

	conf_to_init = malloc(sizeof(*conf_to_init));
	MEM_ASSERT(conf_to_init);

	nb_conf_init(conf_to_init);
	return(conf_to_init);
}


/**
 * Initialize a nb_conf structure (mode to NB_STATIC, all strings to "").
 *
 * \param[in] netconf Pointer to nb_conf structure to be initialized.
 */

void nb_conf_init(struct nb_conf *netconf)
{
	// intialize all members
	netconf->mode = NB_STATIC;
	cfg_strinit(&netconf->interface);
	cfg_strinit(&netconf->address);
	cfg_strinit(&netconf->mask);
	cfg_strinit(&netconf->gateway);
	cfg_strinit(&netconf->nameserver);
	cfg_strinit(&netconf->domain);
}


/**
 * Destroy a nb_conf structure and free all alocated memory.
 *
 * \param[in] netconf Pointer to nb_conf structure to be destroyed.
 */

void nb_conf_destroy(struct nb_conf *netconf)
{
	// free string members
	cfg_strfree(&netconf->interface);
	cfg_strfree(&netconf->address);
	cfg_strfree(&netconf->mask);
	cfg_strfree(&netconf->gateway);
	cfg_strfree(&netconf->nameserver);
	cfg_strfree(&netconf->domain);
}


/**
 * Destroy and re-initialize a nb_conf structure.
 *
 * \param[in] netconf Pointer to structure to be destroyed/re-initialized.
 */

void nb_conf_reset(struct nb_conf *netconf)
{
	nb_conf_destroy(netconf);
	nb_conf_init(netconf);
}


/**
 * Free a dynamically allocated nb_conf structure and assign \p NULL.
 *
 * \param[in] str Pointer to nb_conf structure to be freed.
 */

void nb_conf_free(struct nb_conf **netconf)
{
	if (*netconf == NULL) return;
	free(*netconf);
	*netconf = NULL;
}


/**
 * Print contents of a nb_conf structure to debug facility.
 * The print format is identical to the format of a setup network
 * section in the config file.
 *
 * \param[in] netconf Pointer to nb_conf structure to be printed.
 */

void nb_conf_print(struct nb_conf *netconf)
{
	dg_printf(DG_VERBOSE,"setup network {\n");
	dg_printf(DG_VERBOSE,"  mode ");
	switch (netconf->mode) {
	case NB_STATIC:
		dg_printf(DG_VERBOSE,"static\n");
		break;
	case NB_DHCP:
		dg_printf(DG_VERBOSE,"dhcp\n");
		break;
	}
	dg_printf(DG_VERBOSE,"  interface %s\n", netconf->interface);
	dg_printf(DG_VERBOSE,"  address %s\n", netconf->address);
	dg_printf(DG_VERBOSE,"  mask %s\n", netconf->mask);
	dg_printf(DG_VERBOSE,"  gateway %s\n", netconf->gateway);
	dg_printf(DG_VERBOSE,"  nameserver %s\n", netconf->nameserver);
	dg_printf(DG_VERBOSE,"  domain %s\n", netconf->nameserver);
	dg_printf(DG_VERBOSE,"}\n");
}


/**
 * Enable network interface according to the given network configuration
 *
 * \param[in] netconf Pointer to network configuration to be enabled
 */

void nb_conf_enable(struct nb_conf *netconf)
{
	char *netcmd = NULL;
	FILE *resolvconf = NULL;
	struct dhcp_request my_request;
	struct dhcp_request *dhcp_return = NULL;
	int timeout = 10; /* dhcp timeout in seconds */
	int retries = 0;

	cfg_strinit(&netcmd);

	nb_conf_print( netconf);

	if (netconf->mode == NB_DHCP) {		
		dhcp_req_init(&my_request);
		do {
			dhcp_return = dhcpcd( netconf->interface,
			    &my_request, timeout);
			retries++;
			if (dhcp_return != NULL) {
				dg_printf( DG_VERBOSE,
				    "dhcp request %d successful (timeout %d)\n",
				    retries, timeout);
				cfg_strcpy(&netconf->address,
				    dhcp_return->ipaddr);
				cfg_strcpy(&netconf->mask,
				    dhcp_return->netmask);
				cfg_strcpy(&netconf->gateway,
				    dhcp_return->gateway);
				cfg_strcpy(&netconf->nameserver,
				    dhcp_return->dns[0]);
				cfg_strcpy(&netconf->domain,
				    dhcp_return->domain);
			}
			else {
				dg_printf( DG_VERBOSE,
				    "dhcp request %d failed (timeout %d)\n",
				    retries, timeout);
				timeout = timeout*2;
			}
		} while ((dhcp_return == NULL) && 
		    (retries <= DHCP_MAX_RETRIES));
	}

	resolvconf = fopen("/etc/resolv.conf", "a+");
	if (resolvconf != NULL) {
		/* set domain if available */
		if (strlen(netconf->domain) > 0)
			fprintf(resolvconf, "domain %s\n",
			    netconf->domain);
		/* set nameserver if available */
		if (strlen(netconf->nameserver) > 0)
			fprintf(resolvconf, "nameserver %s\n",
			    netconf->nameserver);
		fclose(resolvconf);
	}

	/* configure loopback interface */
	cfg_strprintf(&netcmd, "ifconfig lo 127.0.0.1");
	cfg_system( netcmd);

	/* check for minimal necessary parameters */
	if (strlen(netconf->interface) == 0 || strlen(netconf->address) == 0) {
		fprintf(stderr, "%s:invalid network interface or address\n",
		    arg0);
		goto cleanup_and_return;
	}

	/* assume 255.255.255.0 if netmask is missing */
	if (strlen(netconf->mask) == 0) {
		cfg_strprintf(&netconf->mask, "255.255.255.0");
	}

	cfg_strprintf(&netcmd, "ifconfig %s %s netmask %s up", 
	    netconf->interface, netconf->address, netconf->mask);
	cfg_system( netcmd);

	/* set gateway if available */
	if (strlen(netconf->gateway) > 0) {
		cfg_strprintf(&netcmd, "route add default gw %s",
		    netconf->gateway);
		cfg_system( netcmd);
	}

 cleanup_and_return:

	cfg_strfree(&netcmd);
}


/**
 * Test if a network device with the given MAC address exists
 *
 * \return CFG_RETURN_OK if a network device with this MAC exists, 
 *         CFG_RETURN_ERROR otherwise
 */

int nb_test_mac (const char *macaddr)
{
  struct ifreq ifRequest;             // request to the interface
  struct if_nameindex* ifTmp  = NULL; // tmp pointer
  struct if_nameindex* ifList = NULL; // list of interfaces
  int fh     = -1;                    // handle for the socket
  int result = CFG_RETURN_ERROR;
  char *localmac = NULL;

  cfg_strinit(&localmac);
  // open socket
  fh = socket (PF_INET, SOCK_STREAM, 0);

  if (fh < 0) {
	  printf ("File %s, line %d: socket () failed\n", __FILE__, __LINE__);
	  result = CFG_RETURN_ERROR;
	  goto cleanup_and_return;
  }

  // now get a list of all interfaces
  ifList = if_nameindex ();
  
  if (!ifList) {
	  printf ("File %s, line %d: if_nameindex () failed\n", 
	      __FILE__, __LINE__);
	  result = CFG_RETURN_ERROR;
	  goto cleanup_and_return;
  }

  // get mac address of all devices
  for (ifTmp = ifList; *(char*)ifTmp != 0; ifTmp++) {
	  // set request for ioctl and get mac
	  strncpy (ifRequest.ifr_name, ifTmp->if_name, IF_NAMESIZE);
	  if (ioctl (fh, SIOCGIFHWADDR, &ifRequest) == -1)
		  // error, skip this device
		  continue;

	  cfg_strprintf( &localmac, "%02x:%02x:%02x:%02x:%02x:%02x",
	      (unsigned char)ifRequest.ifr_ifru.ifru_hwaddr.sa_data[0],	      
	      (unsigned char)ifRequest.ifr_ifru.ifru_hwaddr.sa_data[1],	      
	      (unsigned char)ifRequest.ifr_ifru.ifru_hwaddr.sa_data[2],	      
	      (unsigned char)ifRequest.ifr_ifru.ifru_hwaddr.sa_data[3],	      
	      (unsigned char)ifRequest.ifr_ifru.ifru_hwaddr.sa_data[4],	      
	      (unsigned char)ifRequest.ifr_ifru.ifru_hwaddr.sa_data[5]);

	  dg_printf( DG_MINIMAL, "%s:localmac=%s\n", ifTmp->if_name, localmac);
	      
	  /*
	  if ((ifRequest.ifr_addr.sa_family != ARPHRD_ETHER) &&
	      (ifRequest.ifr_addr.sa_family != ARPHRD_EETHER))
		  // skip, no ethernet device
		  continue;
	  */

	  // test for valid mac
	  if (strcmp( localmac, macaddr) == 0) {
		  result = CFG_RETURN_OK;
		  goto cleanup_and_return;
	  }
  }

cleanup_and_return:

  cfg_strfree(&localmac);

  if (fh >= 0)
	  close (fh);
  if (ifList)
	  if_freenameindex (ifList);
  
  return result;
} 
