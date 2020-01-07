/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file dhcp_request.c
 * \brief sysload user interface module for dhcp requests
 *
 * \author Swen Schillig (swen@vnet.ibm.com)
 *
 * $Id: dhcp_request.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <unistd.h>        //->unlink
#include <string.h>        //->strlen
#include "dhcp.h"
#include "config.h"
#include "debug.h"

/**
 * Initialize dhcp_request structure
 *
 * \param[in] *dhcp pointer to dhcp_request instance
 * \return    *dhcp pointer to initialized dhcp_request instance
*/
struct dhcp_request * dhcp_req_init(struct dhcp_request *dhcp)
{
    if(dhcp == NULL) return(dhcp);

    memset(dhcp,0,sizeof(*dhcp));

    return(dhcp);
}


/**
 * send request to DHCP server
 *
 * \param[in] interface string specifying network interface
 * \param[in] dhcp      pointer to dhcp_request structure where results are stored
 * \param[in] timeout   integer specifying the timeout
 * \return    dhcp      on success pointer to dhcp_request structure otherwise NULL
*/
struct dhcp_request * dhcpcd(char *interface, struct dhcp_request *dhcp, int timeout)
{
    char                *dhcp_ex;
    char                *temp_name;
    FILE                *temp_file;
    char                line[CFG_STR_MAX_LEN];
    char                *t_ptr;
    char                *v_ptr;
    int                 counter = 0;
    int                 len;

    cfg_strinit(&dhcp_ex);
    cfg_strprintf(&dhcp_ex,"%s -T -t %d -NYRG -L %s -c /bin/true %s", 
	DHCP_CMD, timeout, DHCP_TEMP_DIR, interface);

    if(cfg_system(dhcp_ex) != 0)                         //dhcp request
    {
        cfg_strfree(&dhcp_ex);
        return (void *) 0;
    }

    cfg_strfree(&dhcp_ex);
    cfg_strinit(&temp_name);
    cfg_strprintf(&temp_name,"%s/dhcpcd-%s.info", DHCP_TEMP_DIR, interface);

    if((temp_file = fopen(temp_name,"r")) == NULL)
    {
        cfg_strfree(&temp_name);
        return (void *) 0;
    }

    unlink(temp_name);
    cfg_strfree(&temp_name);

    while(fgets(line,CFG_STR_MAX_LEN,temp_file))
    {
        line[strlen(line) - 1] = '\0';     //replace CR with \0 character
        t_ptr = index(line,'=') + 1;       //move ptr to first char after '='
        len   = (int)(t_ptr - line) - 1;   //calculate length of key

      if(strncmp(line,"IPADDR", len) == 0)
      {
          dhcp->ipaddr = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->ipaddr,t_ptr);
      }
      else if(strncmp(line,"NETMASK", len) == 0)
      {
          dhcp->netmask = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->netmask,t_ptr);
      }
      else if(strncmp(line,"NETWORK", len) == 0)
      {
          dhcp->network = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->network,t_ptr);
      }
      else if(strncmp(line,"BROADCAST", len) == 0)
      {
          dhcp->broadcast = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->broadcast,t_ptr);
      }
      else if(strncmp(line,"GATEWAY", len) == 0)
      {
          dhcp->gateway = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->gateway,t_ptr);
      }
      else if(strncmp(line,"DOMAIN", len) == 0)
      {
          dhcp->domain = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->domain,t_ptr);
      }
      else if(strncmp(line,"DNS", len) == 0)
      {
	      do {
		  // allocate more memory for additional DNS entry
		  dhcp->dns = realloc(dhcp->dns, 
		      (counter + 1) * sizeof(char *) );               
		  // do we have a list, point to the seperator
		  v_ptr = index(t_ptr,',');     
		  // get the length of the DNS entry
		  len = (v_ptr == NULL) ? 
			  strlen(t_ptr) : (int) (v_ptr - t_ptr);     
		  // allocate memory for DNS entry
		  dhcp->dns[counter] = malloc(len + 1);
		  // set last char to \0 , might have been ','
		  t_ptr[len] = '\0';      
		  strcpy(dhcp->dns[counter], t_ptr);     
		  // move t_ptr to next DNS entry
		  t_ptr = v_ptr + 1;        
		  counter++;
	      }while(v_ptr != NULL);

	      dhcp->dns = realloc(dhcp->dns, (counter + 1) * sizeof(char *) );
	      // set last one to NULL to identify end of char list
	      dhcp->dns[counter] = NULL; 
      }
      else if(strncmp(line,"DHCPSID", len) == 0)
      {
          dhcp->dhcpsid = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->dhcpsid,t_ptr);
      }
      else if(strncmp(line,"DHCPGIADDR", len) == 0)
      {
          dhcp->dhcpgiaddr = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->dhcpgiaddr,t_ptr);
      }
      else if(strncmp(line,"DHCPSIADDR", len) == 0)
      {
          dhcp->dhcpsiaddr = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->dhcpsiaddr,t_ptr);
      }
      else if(strncmp(line,"DHCPCHADDR", len) == 0)
      {
          dhcp->dhcpchaddr = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->dhcpchaddr,t_ptr);
      }
      else if(strncmp(line,"DHCPSHADDR", len) == 0)
      {
          dhcp->dhcpshaddr = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->dhcpshaddr,t_ptr);
      }
      else if(strncmp(line,"DHCPSNAME", len) == 0)
      {
          dhcp->dhcpsname = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->dhcpsname,t_ptr);
      }
      else if(strncmp(line,"LEASETIME", len) == 0)
      {
          dhcp->leasetime = atol(t_ptr);
      }
      else if(strncmp(line,"RENEWALTIME", len) == 0)
      {
          dhcp->renewaltime = atol(t_ptr);
      }
      else if(strncmp(line,"REBINDTIME", len) == 0)
      {
          dhcp->rebindtime = atol(t_ptr);
      }
      else if(strncmp(line,"INTERFACE", len) == 0)
      {
          dhcp->interface = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->interface,t_ptr);
      }
      else if(strncmp(line,"CLASSID", len) == 0)
      {
          dhcp->classid = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->classid,t_ptr);
      }
      else if(strncmp(line,"CLIENTID", len ) == 0)
      {
          dhcp->clientid = malloc(strlen(t_ptr) + 1);
          strcpy(dhcp->clientid,t_ptr);
      }
    }

    return dhcp;
}
