
/******************************************************************

	file: psv_main.c

	This is the main module for the p_server - the product server.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/07/14 20:13:57 $
 * $Id: psv_main.c,v 1.77 2011/07/14 20:13:57 jing Exp $
 * $Revision: 1.77 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <mrpg.h> 
#include <infr.h> 
#include <orpgerr.h>

#include "psv_def.h"

static int Psv_verbose;		/* starting verbose mode level */

static int P_server_index;	/* index of this product server */

static int N_users;		/* number of users (links) */
static User_struct *Users[MAX_N_USERS];
				/* user structure list */
static int Max_n_files;		/* max number of open files */

static int Log_size;		/* log file size */

/* local functions */
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Print_cs_error (char *msg);
static int Terminate (int signal, int sigtype);


/******************************************************************

    Description: The main function of p_server.

******************************************************************/

int main (int argc, char **argv)
{
    int ret;

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (1);

    LB_NTF_control (LB_NTF_SIGNAL, LB_NTF_NO_SIGNAL);

    /* Initialize the LE and CS services */
    ret = ORPGMISC_init (argc, argv, Log_size, 
				LB_SINGLE_WRITER, -1, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 68,  
				"ORPGMISC_init failed (ret %d)", ret);
	exit (EXIT_FAILURE) ;
    }
    LE_local_vl (Psv_verbose);
    CS_error (Print_cs_error);	/* asking CS to print error via 
				   Print_cs_error */

    ret = ORPGTASK_reg_term_handler (Terminate);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGTASK_reg_term_hdlr failed: %d", ret) ;
	exit (EXIT_FAILURE) ;
    }

    if (Max_n_files > 0	&&	/* increases OS resource limit */
	(ret = MISC_rsrc_nofile (Max_n_files)) < Max_n_files) {
	LE_send_msg (GL_ERROR | 72,  
		"MISC_rsrc_nofile failed (ret %d < req %d)", ret, Max_n_files);
	ORPGTASK_exit (1);
    }

    /* initialize Users by reading product distribution info */
    if ((N_users = RPI_initialize (P_server_index, Users)) <= 0)
	ORPGTASK_exit (1);
    LE_send_msg (LE_VL1, "%d lines served by this product server", N_users);

    /* initialize modules */
    if (GE_initialize (N_users, Users, P_server_index) != 0 ||
	RRS_initialize (N_users, Users) != 0 ||
	PE_initialize (N_users, Users) != 0 ||
	MT_initialize (N_users, Users) != 0 ||
	SUS_initialize (N_users, Users) != 0 ||
	WAN_initialize (N_users, Users) != 0 ||
	PSO_initialize (N_users, Users) != 0 ||
	PSAI_initialize (N_users, Users) != 0 ||
	PSR_initialize (N_users, Users) != 0 ||
	HWQ_initialize (N_users, Users) != 0 ||
	PWA_initialize (N_users, Users, P_server_index) != 0 ||
	HP_initialize (N_users, Users, P_server_index) != 0)
	ORPGTASK_exit (1);

    /* report ready-for-operation and wait until RPG is in operational mode */
    ORPGMGR_report_ready_for_operation ();
    LE_send_msg (GL_INFO, "Waiting for RPG to enter OP state");
    if (ORPGMGR_wait_for_op_state (300) != 0) {
	LE_send_msg (GL_ERROR, "ORPGMGR_wait_for_op_mode failed");
	ORPGTASK_exit (1);
    }

    /* actions before routine processing */
    RRS_init_read ();		/* read RDA and RPG status initially */
    WAN_init_contact ();	/* make initial contact to comm_manager */

    /* the main loop */
    LE_send_msg (GL_INFO, "p_server starts operation");
    GE_main_loop ();
    ORPGTASK_exit (0);
    exit (0);
}

/**************************************************************************

    Description: This function terminates this process.

**************************************************************************/

static int Terminate (int signal, int sigtype)
{
/*    int i; */

    /* disconnect all lines */
/*
    for (i = 0; i < N_users; i++) {
	if (Users[i]->wan != NULL)
	    WAN_disconnect (Users[i]);
    }
*/
    if (signal == SIGTERM) {
	LE_send_msg (GL_INFO,  "SIGTERM received");
	return (1);
    }

    LE_send_msg (GL_INFO | 74,  "p_server terminates");
    return (0);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    P_server_index = -1;
    Max_n_files = 0;
    Log_size = 2000;
    Psv_verbose = 0;
    while ((c = getopt (argc, argv, "hv:f:s:T:?")) != EOF) {
	switch (c) {

	    case 'v':
		if (sscanf (optarg, "%d", &Psv_verbose) != 1 ||
		    Psv_verbose < 0)
		    err = -1;
		break;

	    case 'f':
		if (sscanf (optarg, "%d", &Max_n_files) != 1)
		    err = -1;
		break;

	    case 's':
		if (sscanf (optarg, "%d", &Log_size) != 1)
		    err = -1;
		break;

	    case 'T':	/* ignore this (for task name) */
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind == argc - 1)       /* get the comm manager index  */
	sscanf (argv[optind], "%d", &P_server_index);

    if (err == 0 && P_server_index < 0) {
	fprintf (stderr, "P_server_index not specified or incorrect\n");
	err = -1;
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv)
{
    printf ("Usage: %s (options) p_server_index\n", argv[0]);
    printf ("       Options:\n");
    printf ("       -f max_number_open_files (default: system default)\n");
    printf ("       -v LE_message_verbosity_level (default: 0)\n");
    printf ("       -s log_size (size of the log-error (LE) data store;\n");
    printf ("          default: 500 messages)\n");
    exit (0);
}

/**************************************************************************

    Description: This function sends error messages while reading the link 
		configuration file.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static void Print_cs_error (char *msg)
{

    LE_send_msg (GL_ERROR | 76,  msg);
}

