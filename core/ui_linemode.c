/**
 * Copyright IBM Corp. 2005, 2008
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (version 2 only)
 * as published by the Free Software Foundation.
 *
 * \file ui_linemode.c
 * \brief System Loader user interface module for line mode terminals
 *
 * \author Michael Loehr    (mloehr@de.ibm.com)
 * \author Swen Schillig    (swen@vnet.ibm.com)
 * \author Christof Schmitt (christof.schmitt@de.ibm.com)
 *
 * $Id: ui_linemode.c,v 1.2 2008/05/16 07:35:52 schmichr Exp $
 */


#include <sys/time.h>
#include <unistd.h>
#include <libgen.h>      //->basename
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <syslog.h>
#include <pwd.h>
#include "config.h"
#include "sysload.h"

#define assign_input(X,Y)  { if(! strlen(X))\
                                 X = Y;\
                              else if(strlen(X) == strspn(X," "))\
                                 X = '\0'; }

static struct termios term_orig;
static int            term_status = 0;

extern char *crypt(char *, char *);

char *arg0; //!< global variable with pointer to argv[0]

FILE *c_in = NULL;          /*!< for screen input */
FILE *c_out = NULL;         /*!< for screen output */


/**
 * get the PID of the main sysload process.
 * This sysload process is responsible for timeout handling
 *
 * \return  PID of main sysload process (PID of parent process as fallback)
 */
pid_t getkppid()
{
	FILE *pidfile = NULL;
	int pid = -1;

	pidfile = fopen(CFG_PIDFILE, "r");
	if (!pidfile) {
		fprintf(stderr, 
		    "%s: fopen not possible for %s\n", 
		    arg0, CFG_PIDFILE);
		pid = getppid();
	}
	else {
		fscanf(pidfile,"%d", &pid);
		fclose(pidfile);
	}
	return pid;
}


/**
 * Set the terminal input to non-canonical and no echo or reset to default
 *
 * \param[in] on  if true switch to special mode otherwise reset
 * \return    CFG_RETURN_OK on success,
 *            CFG_RETURN_ERROR otherwise
 */
int set_terminal(int on)
{
    struct termios term_new;
    int fd = fileno(c_in);

    if(! term_status && on)
    {
        if(tcgetattr(fd, &term_orig) < 0 )    //get current terminal attributes
            return(CFG_RETURN_ERROR);

        term_new = term_orig;

	//turn off echo and canonical mode -> no buffering
        term_new.c_lflag &= ~(ECHO|ICANON);   
        term_new.c_cc[VMIN]  = 1;             //get at least one char
        term_new.c_cc[VTIME] = 0;             //wait

        tcflush(fd, TCIFLUSH);

        if(tcsetattr(fd, TCSAFLUSH, &term_new) < 0)
            return(CFG_RETURN_ERROR);

        term_status = 1;
    }
    else if(term_status && ! on)
    {
	    //reset terminal attributes with original values
	    if(tcsetattr(fd, TCSAFLUSH, &term_orig) < 0)    
		    return(CFG_RETURN_ERROR);

	    term_status = 0; //clear status flag
    }

    return(CFG_RETURN_OK);
}

/*
 * Default signal handler (SIGINT, SIGQUIT)
 */

void default_sig_handler(int sig_no)
{
    set_terminal(0);
    exit(0);
}

/**
 * Signal handler for SIGTERM
 */

void sigtermhandler()
{
    set_terminal(0); //reset original terminal settings if changed
    fprintf(c_out, 
	"entry has been selected via timeout or another user interface!\n");
    exit(0);
}

/**
 * Ask the user to enter a password and test against passwd
 *
 * \param[in] passwd  Password to compare the input against
 * \return    CFG_RETURN_OK if the user entered the correct password,
 *            CFG_RETURN_ERROR otherwise
 */
int password(char* passwd)
{
    char input[CFG_STR_MAX_LEN] = "";         // input line from user */
    char single;
    char *str_ptr;

    set_terminal(1);

    fprintf(c_out, "\nPlease enter the password: ");
    fflush(c_out);

    str_ptr = input;
    while( (single = getc(c_in)) != '\n')     //read char until new-line
        if(str_ptr < &input[CFG_STR_MAX_LEN]) //if we still have enough space
            *str_ptr++ = single;              //save char in string

    *str_ptr = 0;                             //string end
    putc('\n', c_out);                        //print new-line on terminal

    set_terminal(0);
                                              //compare the 2 encrypted values
    if(strcmp(passwd, input) == 0)       
    {
        fprintf(c_out, "\nPassword OK!\n");
        return CFG_RETURN_OK;
    }

    fprintf(c_out, "\nPassword incorrect!\n");
    return CFG_RETURN_ERROR;
}

/**
 * Wait until one key was pressed
 *
 * \param[in] none
 * \return    code of the pressed key
 *            
 */
int keypressed()
{
    char single;

    set_terminal(1);
    single = getc(c_in);
    set_terminal(0);

    return(single);
}


/**
 * More flexible user-input-check routine
 *
 * \param[in] line raw user input
 * \return    0 success  !0 error
 * \modified  'cmd' character specifying the command, 'selection' bootentry
 *
**/
int compute_selection(char * line, char * cmd, int * selection)
{
    char * input = line;
    *cmd       = '\0';                             //reset all parameters
    *selection = 0;                                //to default values

    if(input == NULL || *input == '\0')             //NULL or empty string ?
        return(CFG_RETURN_OK);

    while(*input == ' ')                            //leading blanks !?
        input++;

    if(sscanf(input,"%d", selection) != 1)
        if(sscanf(input, "%c", cmd) != 1)
            return(CFG_RETURN_ERROR);               //neither decimal nor char
        else
        {
            input++;                                //successful read a char
            while(*input == ' ') input++;           //blanks !?
	    //only cmd and no selection
            if(*input == '\0') return(CFG_RETURN_OK);  
            sscanf(input, "%d", selection);
        }
    else
    {
        input += strspn(input,"0123456789");        //how long is the decimal
        while(*input == ' ') input++;               //blanks !?
        if(*input == '\0') return(CFG_RETURN_OK);   //only cmd and no selection
        sscanf(input,"%c", cmd);
    }

    return(CFG_RETURN_OK);
}

/**
 * Print selected boot-entry
 *
 * \param[in] bentry boot-entry structure
 * \return    void
 *
**/

void print_selection(struct cfg_bentry* bentry)
{
    fprintf(c_out,"\n*** BOOT ENTRY PRINTOUT ***\n");
    if(bentry->title    && strlen(bentry->title))
        fprintf(c_out,"%-9s %s\n","title",bentry->title);
    if(bentry->label    && strlen(bentry->label))
        fprintf(c_out,"%-9s %s\n","label",bentry->label);
    if(bentry->root     && strlen(bentry->root))
        fprintf(c_out, "%-9s %s\n","root",bentry->root);
    if(bentry->kernel   && strlen(bentry->kernel))
        fprintf(c_out, "%-9s %s\n","kernel",bentry->kernel);
    if(bentry->initrd   && strlen(bentry->initrd))
        fprintf(c_out, "%-9s %s\n","initrd",bentry->initrd);
    if(bentry->cmdline  && strlen(bentry->cmdline))
        fprintf(c_out, "%-9s %s\n","cmdline",bentry->cmdline);
    if(bentry->parmfile && strlen(bentry->parmfile))
        fprintf(c_out, "%-9s %s\n","parmfile",bentry->parmfile);
    if(bentry->insfile  && strlen(bentry->insfile))
        fprintf(c_out, "%-9s %s\n","insfile",bentry->insfile);
    if(bentry->bootmap  && strlen(bentry->bootmap))
        fprintf(c_out, "%-9s %s\n","bootmap",bentry->bootmap);
    if(bentry->pause    && strlen(bentry->pause))
        fprintf(c_out, "%-9s %s\n","pause",bentry->pause);

    switch (bentry->action)
    {
    case KERNEL_BOOT:
        fprintf(c_out, "%-9s %s\n","action","KERNEL_BOOT\n");
        break;
    case INSFILE_BOOT:
        fprintf(c_out, "%-9s %s\n","action","INSFILE_BOOT\n");
        break;
    case BOOTMAP_BOOT:
        fprintf(c_out, "%-9s %s\n","action","BOOTMAP_BOOT\n");
        break;
    case REBOOT:
        fprintf(c_out, "%-9s %s\n","action","REBOOT\n");
        break;
    case HALT:
        fprintf(c_out, "%-9s %s\n","action","HALT\n");
        break;
    case SHELL:
        fprintf(c_out, "%-9s %s\n","action","SHELL\n");
        break;
    case EXIT:
        fprintf(c_out, "%-9s %s\n","action","EXIT\n");
        break;
    }

    return;
}

/**
 * Low level input routine for interactive boot option
 *
 * \param[in] out_str string to be printed specifying the requested input
 *            input   pointer to string where user-input is stored
 * \return    CFG_RETURN_OK on success,
 *            CFG_RETURN_ERROR otherwise
 *
**/
int get_input(char const *out_str, char **input)
{
    char line[CFG_STR_MAX_LEN + 1];

    fprintf(c_out,"%-9s ", out_str);
    fflush(c_out);
    fgets(line, CFG_STR_MAX_LEN, c_in);

    if((input == NULL) || ((*input = malloc(strlen(line))) == NULL))
        return(CFG_RETURN_ERROR);

    memcpy(*input,line,strlen(line));
    (*input)[strlen(line) - 1] = '\0';

    return(CFG_RETURN_OK);

}

/**
 * High level input routine for interactive boot option
 *
 * \param[in]  bentry  address of cfg_bentry structure
 * \param[out] bentry filled structure
 * \return    CFG_RETURN_OK on success,
 *            CFG_RETURN_ERROR otherwise
 *
**/
int get_man_boot(struct cfg_bentry **bentry, int type)
{
    char input[CFG_STR_MAX_LEN + 1];
    int  action = 0;
    int  result = 0;

    if( (bentry == NULL) || 
	((*bentry = malloc(sizeof(struct cfg_bentry))) == NULL))
	    return(CFG_RETURN_ERROR);

    memset(*bentry, 0, sizeof(struct cfg_bentry));    //initialize bentry

    if(type < 0)
    {
        fprintf(c_out,
	    "%-9s=> 1->KERNEL_BOOT 2->INSFILE_BOOT 3->BOOTMAP_BOOT\n",
	    "ACTION");
        do
        {
            fprintf(c_out, "%-9s ","action");
            fflush(c_out);
            fgets(input, CFG_STR_MAX_LEN, c_in);
            action = atoi(input);
        }while( (action < 1) || (action > 3) );
        action--;
    }

    switch(action)
    {
    case 0:  (*bentry)->action = KERNEL_BOOT;
             result |= get_input("root",    &((*bentry)->root));
             result |= get_input("kernel",  &((*bentry)->kernel));
             result |= get_input("initrd",  &((*bentry)->initrd));
             result |= get_input("cmdline", &((*bentry)->cmdline));
             result |= get_input("parmfile",&((*bentry)->parmfile));
             break;

    case 1:  (*bentry)->action = INSFILE_BOOT;
             result |= get_input("insfile", &((*bentry)->insfile));
             break;

    case 2:  (*bentry)->action = BOOTMAP_BOOT;
             result |= get_input("bootmap", &((*bentry)->bootmap));
             break;

    default: (*bentry)->action = KERNEL_BOOT;
             result |= get_input("root",    &((*bentry)->root));
             result |= get_input("kernel",  &((*bentry)->kernel));
             result |= get_input("initrd",  &((*bentry)->initrd));
             result |= get_input("cmdline", &((*bentry)->cmdline));
             break;
    }

    return( (result) ? CFG_RETURN_ERROR : CFG_RETURN_OK);
}


/**
 * This program implements a simple linemode user interface. It collects all
 * input data that is required to display a boot selection menu from
 * environment variables and prints the selected entry to stdout.
 */

int main(int argc, char *argv[])
{
    arg0 = argv[0];                          // make MEM_ASSERT happy
    struct cfg_toplevel* my_toplevel = NULL; // boot info from env
    struct cfg_bentry*   bentry      = NULL;
    struct cfg_bentry*   temp_ptr    = NULL;
    char const *cmd_name = basename(argv[0]);
    int i            = 0;                 // multi purpose loop counter
    int selected     = 0;                 // selected boot menu entry
    int selection_ok = CFG_RETURN_ERROR;
    char input[CFG_STR_MAX_LEN] = "";     // input line from user
    char *device_name           = NULL;   // for user interaction
    char *message               = NULL;   // message to be displayed
    char command                = '\0';   // bootloader command
                                          // [d,D] for displaying a menu entry

    openlog(cmd_name, LOG_PID, LOG_USER);
    syslog(LOG_INFO,"Starting '%s' interface",basename(argv[0]));

    signal(SIGTERM, sigtermhandler);
    signal(SIGINT,  default_sig_handler);// whatever happens make sure we are
    signal(SIGQUIT, default_sig_handler);// able to reset the terminal settings

    cfg_strinit(&message);
    cfg_strinit(&device_name);

    my_toplevel = cfg_new();

    cfg_get_env(my_toplevel);
    cfg_get_env_str(CFG_STARTUP_MSG, &message);

    /* now we know which device to open! */
    cfg_strcpy(&device_name, (argv[1]) ? argv[1] : "/dev/console");
                                              // open the terminal keyboard
    if( (c_in  = fopen(device_name, "r")) == NULL)  
        c_in = fopen(ctermid(NULL),"r");
                                              // open the terminal screen
    if( (c_out = fopen(device_name, "w")) == NULL)   
        c_out = fopen(ctermid(NULL), "w");

    if (!c_in || !c_out) {
        syslog(LOG_ERR, "Unable to open terminal");
        exit(1);
	}

    do {
        selection_ok = CFG_RETURN_ERROR;
        fprintf(c_out, "\nWelcome to System Loader " SYSLOAD_VERSION "\n\n");
        if (strlen(message))
            fprintf(c_out, "%s\n\n", message);
        fprintf(c_out,"The following boot options are available:\n\n");
        /*print the boot selection menu */

        for (i = 0; i < my_toplevel->bentry_count; i++) {
            bentry = &(my_toplevel->bentry_list[i]);

            fprintf(c_out,"%s%c%d\t%s%s"
                    ,(i == my_toplevel->boot_default) ? "->" : "  "
                    ,(bentry->locked)        ? '['  : ' '
                    ,i+1
                    ,bentry->title
                    ,(bentry->locked)        ? "]\n"  : "\n");
        }

        fprintf(c_out,
	    "   d<n>\tDisplay boot parameters of the selected entry\n");
        fprintf(c_out,"   m<n>\tModify and boot selected entry\n");
        fprintf(c_out,"   i\tEnter boot parameters interactively\n");

        fprintf(c_out, "\nPlease enter your selection:\n");
        if (my_toplevel->timeout > 0) {
            fprintf(c_out,"(default will be selected in %d seconds) \n", 
		my_toplevel->timeout);
        }

        fgets(input, CFG_STR_MAX_LEN, c_in);

        kill(getkppid(), SIGUSR1); //send USR1 to parent to stop timeout

        input[strlen(input) - 1] = '\0'; //replace CR with string-term char

        //no input or only blanks
        if((strlen(input) <= 0) || (strlen(input) == strspn(input," ")))   
            continue;

        compute_selection(input, &command, &selected);

        // if no selection was made use default
        selected = (selected) ? selected - 1 : my_toplevel->boot_default;  

        if (selected < 0 || selected > my_toplevel->bentry_count - 1)
        {
            fprintf(c_out, "Invalid input.\n");
            continue;
        }

        bentry = &(my_toplevel->bentry_list[selected]);

        if(bentry->locked && password(my_toplevel->password) == CFG_RETURN_ERROR)
            continue;

        switch(command)
        {
        case 'd':
        case 'D':
            print_selection(bentry);
            fprintf(c_out,"*** Press RETURN to continue ***\n");
            if(keypressed() == (char) CFG_RETURN_ERROR)
                fprintf(c_out,"Terminal-I/O error !\n");
            continue;
            break;

        case 'i':
        case 'I':
            fprintf(c_out,"*** Please enter the values for manual boot***\n");

            get_man_boot(&bentry, -1);
            print_selection(bentry);
            fprintf(c_out,
		"*** Press 'B' to boot this entry or any other key to cancel ***\n");

            if((command = keypressed()) == (char) CFG_RETURN_ERROR)
            {
                free(bentry);
                fprintf(c_out,"Terminal-I/O error !\n");
                continue;
            }
            if(command == 'B' || command == 'b')
            {
                fprintf(c_out,"Booting \n");

                // Required by sysload parser, so just add something here
                bentry->title = "manual_title";
                bentry->label = "manual_label";
            }
            else
            {
                free(bentry);
                continue;
            }
            break;

        case 'm':
        case 'M':
            temp_ptr = bentry;

            switch(temp_ptr->action)
            {
            case KERNEL_BOOT :
                print_selection(bentry);

                fprintf(c_out,
		    "*** Please enter the new values for each line ***\n");
                fprintf(c_out,
		    "*** or hit ENTER to leave it unchanged.       ***\n");

                get_man_boot(&bentry, temp_ptr->action);

                assign_input(bentry->root,     temp_ptr->root);
                assign_input(bentry->kernel,   temp_ptr->kernel);
                assign_input(bentry->initrd,   temp_ptr->initrd);
                assign_input(bentry->cmdline,  temp_ptr->cmdline);
                assign_input(bentry->parmfile, temp_ptr->parmfile);
                break;

            case INSFILE_BOOT:
                print_selection(bentry);

                fprintf(c_out,
		    "*** Please enter the new values for each line ***\n");
                fprintf(c_out,
		    "*** or hit ENTER to leave it unchanged.       ***\n");

                get_man_boot(&bentry, temp_ptr->action);

                if(! strlen(bentry->insfile))
                    bentry->insfile = temp_ptr->insfile;
                break;

            case BOOTMAP_BOOT:
                print_selection(bentry);

                fprintf(c_out,
		    "*** Please enter the new values for each line ***\n");
                fprintf(c_out,
		    "*** or hit ENTER to leave it unchanged.       ***\n");

                get_man_boot(&bentry, temp_ptr->action);

                if(! strlen(bentry->bootmap) ||
                   (strspn(bentry->bootmap," ") == strlen(bentry->bootmap)) )
                    bentry->bootmap = temp_ptr->bootmap;
                break;

            default:
                fprintf(c_out,"This boot entry cannot be modified.\n");
                continue;      //if anything else
                break;
            }

            print_selection(bentry);
            fprintf(c_out,
		"*** Press 'B' to boot this entry or any other key to cancel ***\n");

            if((command = keypressed()) == (char) CFG_RETURN_ERROR)
            {
                free(bentry);
                fprintf(c_out,"Terminal-I/O error !\n");
                continue;
            }
            if(command == 'B' || command == 'b')
            {
                fprintf(c_out,"Booting \n");

                // Required by sysload parser, so just add something here
                bentry->title = "manual_title";
                bentry->label = "manual_label";
            }
            else
            {
                free(bentry);
                continue;
            }
            break;

        case '\0':
            break;

        default:
            fprintf(c_out,"Unknown command !\n");
            continue;
            break;
        }

        selection_ok = CFG_RETURN_OK;

    } while (selection_ok != CFG_RETURN_OK);

    /* check if pause exists and wait for user input and continue */
    if (bentry->pause && strlen(bentry->pause) > 0)
    {
        fprintf(c_out, "\n%s\n", bentry->pause);
        fprintf(c_out, "(press enter to continue)\n");
        fgets(input, CFG_STR_MAX_LEN, c_in);
    }

	/* now write selected entry to stdout */
    cfg_bentry_print(bentry);

    fclose(c_in);
    fclose(c_out);
    cfg_strfree(&message);

    cfg_strfree(&device_name);

    closelog();
    exit(0);
}

