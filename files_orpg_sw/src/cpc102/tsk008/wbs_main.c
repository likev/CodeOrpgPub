
/******************************************************************

    This is the main module for wb_simulator - The wide-band 
    simulator.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/09/22 21:04:23 $
 * $Id: wbs_main.c,v 1.3 2011/09/22 21:04:23 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <orpgerr.h>
#include <infr.h>

#include "wbs_def.h"

static int Verbose;		/* verbose mode */
static int Log_size;		/* size of the LE LB */
static char *Request_lb_name;	/* LB name for request comms messages */
static char *Response_lb_name;	/* LB name for response comms messages */
static char *Data_lb_name;	/* LB name for playback data */
static int No_ctm_header = 1;
int Legacy_RDA = 0;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);


/******************************************************************

    Description: The main function of interface_irt.

    Input:	argc - argument count;
		argv - argument array;

******************************************************************/

int main (int argc, char **argv) {
    int ret;

    LB_NTF_control (LB_NTF_SIGNAL, LB_NTF_NO_SIGNAL);

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (1);

    /* Initialize the LE service */
    LE_set_option ("LB type", LB_SINGLE_WRITER);
    LE_set_option ("LB size", Log_size);
    ret = LE_init (argc, argv);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "LE_init failed (ret %d)", ret);
	exit (1);
    }
    if (Verbose)
	LE_set_option ("verbose level", 3);

    /* initialize modules */
    if (WBSR_init (Request_lb_name, Response_lb_name, 
				Data_lb_name, No_ctm_header) < 0)
	exit (1);

    /* the main loop */
    LE_send_msg (GL_INFO, "wb_simulator starts");
    WBSR_main ();
    exit (0);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;	/* used by getopt */
    extern int optind;
    int c;			/* used by getopt */
    int err;			/* error flag */
    char *opt_flags = "l:q:r:d:cLhv?";
    char tc;

    err = 0;
    Verbose = 0;
    Request_lb_name = NULL;
    Response_lb_name = NULL;
    Data_lb_name = NULL;
    Log_size = 500;
    while ((c = getopt (argc, argv, opt_flags)) != EOF) {

	switch (c) {

	    case 'q':
		Request_lb_name = malloc (strlen (optarg) + 1);
		if (Request_lb_name == NULL) {
		    LE_send_msg (GL_ERROR, "malloc failed");
		    err = -1;
		}
		else
		    strcpy (Request_lb_name, optarg);
		break;

	    case 'r':
		Response_lb_name = malloc (strlen (optarg) + 1);
		if (Response_lb_name == NULL) {
		    LE_send_msg (GL_ERROR, "malloc failed");
		    err = -1;
		}
		else
		    strcpy (Response_lb_name, optarg);
		break;

	    case 'd':
		Data_lb_name = malloc (strlen (optarg) + 1);
		if (Data_lb_name == NULL) {
		    LE_send_msg (GL_ERROR, "malloc failed");
		    err = -1;
		}
		else
		    strcpy (Data_lb_name, optarg);
		break;

	    case 'l':
		if (sscanf (optarg, "%d%c", &Log_size, &tc) != 1)
		    err = -1;
		break;

	    case 'v':
		Verbose = 1;
		break;

	    case 'c':
		No_ctm_header = 0;
		break;

	    case 'L':
		Legacy_RDA = 1;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

    Inputs:	argv - the list of command arguments

**************************************************************************/

static void Print_usage (char **argv) {
    printf ("Usage: %s [options] -q request_lb -r response_lb -d data_lb\n", argv[0]);
    printf ("       The wide band simulator for testing RPG. This application\n");
    printf ("       reads playback data from \"play_a2\" through \"data_lb\"\n");
    printf ("       and communicates with \"cm_tcp\" through \"request_lb\"\n");
    printf ("       and \"response_lb\". The LBs are assumed to be in $WORK_DIR\n");
    printf ("       if not explicitly specified.\n");
    printf ("       Options:\n");
    printf ("       -l log_size (size of the log-error (LE) data store;\n");
    printf ("          default: 500 messages)\n");
    printf ("       -c (CTM header is assumed - RPG cm_tcp.0 without -a 12)\n");
    printf ("       -L (Legacy RDA; The default is ORDA)\n");
    printf ("       -v (verbose mode)\n");

    exit (0);
}
