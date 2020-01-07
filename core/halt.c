/**
 * Copyright IBM Corp. 2006, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file halt.c
 * \brief halt, reboot, power_off
 *
 * \author Swen Schillig (swen@vnet.ibm.com)
 *
 * $Id: halt.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */

#include <sys/reboot.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>


int main(int argc, char **argv)
{
    const int flag[] = { RB_AUTOBOOT,
                         RB_HALT_SYSTEM,
                         RB_POWER_OFF
                       };

    const char *mode = "rhp";
    int cc ;
    char * calling = basename(argv[0]);

    for(cc=0; (cc < 3) && (mode[cc] != *calling); cc++);

    if(cc > 2)
    {
        fprintf(stderr,"Unknown 'halt' variation.\n");
        return(1);
    }
    sync();
    return reboot(flag[cc]);
}
