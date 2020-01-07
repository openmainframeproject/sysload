/*!
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file cl_scp.c
 * \brief sysload component_loader module for scp transfer
 *
 * Author(s): Swen Schillig   (swen@vnet.ibm.com)
 *
 * $Id: cl_scp.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <unistd.h>        //->unlink
#include <stdio.h>
#include <stdlib.h>
#include <string.h>        //->strlen
#include <libgen.h>        //->basename
#include <syslog.h>        //->syslog
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

struct ssh_param {
    char *user;
    char *pw;
    char *host;
    char *port;
    char *path;
};

/**
 * decode command-line parameters (e.g. UID, PW, HOST, ...
 *
 * \param[in] input    string which is to be seperated
 * \return    srv_conn ssh_param structure
*/
struct ssh_param* decode_parameters(char * input)
{
    char *ptr = input;
    char *tmp = NULL;
    struct ssh_param *srv_conn;
    struct passwd *pw_entry;

    srv_conn = malloc(sizeof(struct ssh_param));
    memset(srv_conn, 0, sizeof(struct ssh_param));

    if(strncmp(ptr,"scp://",6)) //not a valid source URL
    {
        syslog(LOG_ERR,"Invalid source URL.");
        return NULL;
    }

    ptr += 6;
    tmp = strchr(ptr,'@');

    if(tmp) //user is specified
    {
        srv_conn->user = ptr;
        *tmp = 0;

        ptr = strchr(ptr,':');  //do we have a password
        if(ptr)
        {
            srv_conn->pw = ptr + 1;
            *ptr = 0;
        }
        ptr = tmp + 1;
    }

    if((tmp = strchr(ptr,'/')) == NULL)     //separate hostname
        return NULL;

    if(tmp == ptr)  // URL scp:///path
        srv_conn->host = "localhost";
    else
        srv_conn->host = ptr;

    *tmp = 0;
    tmp++;

    if((ptr = strchr(ptr,':')) != NULL)  //port
    {
        *ptr = 0;
        srv_conn->port = ptr + 1;
    }
    else
        srv_conn->port = "22";

    srv_conn->path = tmp;

    if(srv_conn->user == NULL)          //no user specified
    {
        pw_entry = getpwuid(getuid());
        srv_conn->user = pw_entry->pw_name;
    }

    return srv_conn;
}

/**
 * establish connection to remote SSH server
 *
 * \param[in] ssh_p        pointer to ssh_param structure
 * \return    sftp_session session handle
*/

SFTP_SESSION * connect_ssh(struct ssh_param *ssh_p)
{
    SSH_OPTIONS *options;
    SSH_SESSION *session;
    SFTP_SESSION *sftp_session;
    int result = 0;

    options = options_new();

    if(options_set_wanted_method(options, KEX_HOSTKEY, "ssh-dss,ssh_rsa"))
        return NULL;

    options_set_port(options, atoi(ssh_p->port));
    options_set_host(options, ssh_p->host);
    options_set_username(options, ssh_p->user);
    options_set_timeout(options, 10, 0);

    if((session = ssh_connect(options)) == NULL)
    {
        syslog(LOG_ERR, ssh_get_error(session));
        return NULL;
    }

    result = ssh_userauth_autopubkey(session);

    if(result != SSH_AUTH_SUCCESS)
    {
        if(!(ssh_p->pw &&
             ((result = ssh_userauth_password(session, NULL, ssh_p->pw)) == SSH_AUTH_SUCCESS)))
        {
            syslog(LOG_WARNING,"No password or password doesn't match %d",result);
            return NULL;
        }
        else
            syslog(LOG_INFO,"Password matched. Successful login.");
    }
    else
        syslog(LOG_INFO,"public key matched.");

    if((sftp_session = sftp_new(session)) == NULL)
    {
        syslog(LOG_ERR,ssh_get_error(session));
        return NULL;
    }
    else
        syslog(LOG_INFO,"SFTP subsystem request successful.");

    if(sftp_init(sftp_session))
    {
        syslog(LOG_ERR,ssh_get_error(session));
        return NULL;
    }
    else
        syslog(LOG_INFO,"SFTP initialization successful.");

    return sftp_session;
}

/**
 * open a local file
 *
 * \param[in] name string specifying the local file name
 * \return    fd   file descriptor on success otherwise -1
*/
int open_local(char * name)
{
    int fd = -1;

    if((fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1)
    {
        syslog(LOG_ERR,strerror(errno));
        return -1;
    }
    else
        syslog(LOG_INFO,"Destination opened successfully");
    return fd;
}

/**
 * open a remote file
 * \param[in] name string specifying the remote file name
 * \param[in] sftp pointer to initialized SFTP_SESSION structure
 * \return    fd   file descriptor on success otherwise NULL
*/

SFTP_FILE * open_remote(char * name, SFTP_SESSION *sftp)
{
    SFTP_FILE *fd;

    if((fd = sftp_open(sftp, name, O_RDONLY, NULL)) == NULL)
    {
        syslog(LOG_ERR,ssh_get_error(sftp->session));
        return NULL;
    }
    else
        syslog(LOG_INFO,"Source opened successfully");
    return fd;
}

int main(int argc, char **argv)
{
    struct ssh_param *srv_conn = NULL;
    SFTP_SESSION *sftp;
    SFTP_FILE    *sftp_file;
    const int len = 255;
    int rcvd = 0;
    int acc  = 0;
    char buffer[len+1];
    int local_fd;

    openlog(basename(argv[0]), LOG_PID, LOG_USER);

    if(argc != 3)
    {
	syslog(LOG_ERR,"Invalid number of parameters.");
	return 1;
    }

    if((srv_conn = decode_parameters(argv[1])) == NULL)
        return 1;

    if((sftp = connect_ssh(srv_conn)) == NULL)
        return 2;
    if((local_fd = open_local(argv[2])) == -1)
        return 3;
    if((sftp_file = open_remote(srv_conn->path, sftp)) == NULL)
        return 4;

    do
    {
        rcvd = sftp_read(sftp_file, buffer, len);
        if(rcvd == -1)
        {
            syslog(LOG_ERR,ssh_get_error(sftp->session));
            if(write(local_fd, buffer, rcvd) != rcvd)
                syslog(LOG_ERR,strerror(errno));
        }
        acc += rcvd;
    }while(rcvd == len);

    if(acc > 0)
        syslog(LOG_INFO,"Transferred %d bytes successfully.", acc);
    else
        syslog(LOG_WARNING,"Nothing transferred, please check the remote file-name");

    sftp_file_close(sftp_file);
    close(local_fd);
    sftp_free(sftp);
    free(srv_conn);
    closelog();

    return 0;
}

