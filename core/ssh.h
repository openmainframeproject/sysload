/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file ssh.h
 * \brief Include file for system loader ssh functionality
 *
 * \author Swen Schillig (swen@vnet.ibm.com)
 *
 * $Id: ssh.h,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#ifndef _SSH_H_
#define _SSH_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include "sysload.h"

#define SSH_CMD "/usr/sbin/sshd"
#define SSH_PID "/var/run/ssh_pid"
#define SSH_PORT "22"
#define SSH_CONFIG "/etc/sshd_config"
#define SSH_DSS_HOST_KEY "/etc/ssh_host_dsa_key"
#define SSH_RSA_HOST_KEY "/etc/ssh_host_rsa_key"

#endif /* #ifndef _SSH_H_ */
