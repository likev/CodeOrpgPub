
/******************************************************************

	file: cmu_main.c

	This is the main module for the comm_manager - UCONX
	version.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2001/04/17 22:25:31 $
 * $Id: cmu_main.c,v 1.24 2001/04/17 22:25:31 jing Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <comm_manager.h>
#include <orpgerr.h>
#include <orpgdat.h>
#include <cmu_def.h>

static int Verbosity_level = -1;

int CMU_need_unbind;		/* unbind port upon disconnect (HDLC only) */

static int N_links;		/* Number of links managed by this process */
static Link_struct *Links [MAX_N_LINKS];
				/* link struct list */

static int Boot_time;		/* maximum server boot time */

static int Max_enable_retry = 0;

/* local functions */
static void Le_callback (char *msg, int msgsz);


/******************************************************************

    Description: The main function of comm_manager.

    Input:	argc - argument count;
		argv - argument array;

******************************************************************/

int main (int argc, char **argv)
{
    int device_number;	/* device number managed by this process */
    int cmsv_addr;	/* comms server internet address */
    int vl, i;

    LB_NTF_control (LB_NTF_SIGNAL, LB_NTF_NO_SIGNAL);

    /* set default state */
    CMU_need_unbind = 1;
    Boot_time = 10;

    CMC_start_up (argc, argv, &vl);
    if (Verbosity_level >= 0)
	LE_local_vl (Verbosity_level);
    LE_set_callback (Le_callback);	/* for sending status message */

    /* read link configuration file */
    if ((N_links = CC_read_link_config (Links, 
				&device_number, &cmsv_addr)) < 0)
	exit (1);
    if (N_links == 0) {
	LE_send_msg (LE_VL1, "no link to be managed by this comm_manager\n");
	exit (0);
    }

    /* initialize this task */
    if (CMC_initialize (N_links, Links) != 0)
	exit (1);

    /* catch the signals */
    CMC_register_termination_signals (CM_terminate); 

    if (CMC_get_new_instance ()) {
	for (i = 0; i < N_links; i++)
	    CMPR_send_event_response (Links[i], CM_EXCEPTION, NULL);
    }

    /* initialize the protocol modules */
    CM_block_signals (1);
    if (SH_initialize (N_links, Links, cmsv_addr) < 0) {
	if (!CMC_get_new_instance ())
	    SH_process_fatal_error ();
	for (i = 0; i < N_links; i++)
	    CMPR_send_event_response (Links[i], CM_EXCEPTION, NULL);
	LE_send_msg (GL_ERROR, "Initialization failed - terminate\n");
	CM_terminate ();
    }

    /* send CM_START message */
    for (i = 0; i < N_links; i++)
	CMPR_send_event_response (Links[i], CM_START, NULL);

    /* the main loop */
    while (1) {
	int ret;

	ret = CMRR_get_requests (N_links, Links);
	if (ret < 0)
	    SH_process_fatal_error ();
	CMPR_process_requests (N_links, Links);
	XP_house_keeping ();
	CMC_house_keeping (N_links, Links);
	SH_poll (Links, N_links);
    }
}

/************************************************************************

    Block/unblock event signals.

************************************************************************/

int CM_block_signals (int block) {
    static sigset_t old;
    sigset_t new;

    if (block) {
	sigemptyset (&new);
	sigaddset (&new, SIGIO);
	sigaddset (&new, SIGUSR1);
	sigaddset (&new, SIGUSR2);
	if (sigprocmask (SIG_BLOCK, &new, &old) < 0) {
	    return (-1);
	}
    }
    else {
	sigprocmask (SIG_SETMASK, &old, NULL);
    }
    return (0);
}

/**************************************************************************

    Description: This function returns Boot_time.

    Return:	Boot_time.

**************************************************************************/

int CM_boot_time ()
{

    return (Boot_time);
}

/**************************************************************************

    Description: This function returns Max_enable_retry.

**************************************************************************/

int CM_max_reenable ()
{

    return (Max_enable_retry);
}

/**************************************************************************

    Description: This function returns additional specific option flags
		for this comm_manager.

    Return:	The flag string.

**************************************************************************/

char *CM_additional_flags ()
{

    return ("d:l:w:m:b");
}

/**************************************************************************

    Description: This function reads additional specific command line 
		arguments for this comm_manager.

    Inputs:	c - option flag;
		optarg - option string;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int CM_parse_additional_args (int c, char *optarg)
{

    switch (c) {
	int restart_wait_time;

	case 'd':
	    CMMON_cm_ping_lb_dir (optarg);
	    break;

	case 'l':
	    if (sscanf (optarg, "%d", &Verbosity_level) != 1 ||
		Verbosity_level < 0 || Verbosity_level > 3) {
		LE_send_msg (GL_ERROR, "option -l error");
		return (-1);
	    }
	    break;

	case 'm':
	    if (sscanf (optarg, "%d", &Max_enable_retry) != 1 ||
		Max_enable_retry < 1 || Max_enable_retry > 255) {
		LE_send_msg (GL_ERROR, "option -m error");
		return (-1);
	    }
	    break;

	case 'w':
	    if (sscanf (optarg, "%d", &restart_wait_time) != 1 ||
		restart_wait_time < 0) {
		LE_send_msg (GL_ERROR, "option -w error");
		return (-1);
	    }
	    XP_set_restart_wait_time (restart_wait_time);
	    break;

	case 'b':
	    CMU_need_unbind = 0;
	    break;

    }
    return (0);
}

/**************************************************************************

    Description: This function prints the additional specific usage info
		for this comm_manager.

**************************************************************************/

void CM_print_additional_usage ()
{
    printf ("       -b (keep port bound upon disconnect (HDLC only))\n");
    printf ("       -d cm_ping LB dir (directory for cm_ping LBs;\n");
    printf ("          The default is the project work directory)\n");
    printf ("       -l verbosity level (0 - 3, default is 0, -v sets to 1)\n");
    printf ("       -w restart wait time (in seconds, default is 5)\n");
    printf ("       -m max number of WAN enabling before disabling (default is no limit)\n");
    return;
}

/**************************************************************************

    Description: This function performs clean-up works and then 
		terminates this process.

**************************************************************************/

void CM_terminate ()
{

    CMC_terminate ();
    HA_clean_up ();
    XP_clean_up ();
    LE_send_msg (GL_INFO, "Terminating ...");
    exit (0);
}

/***********************************************************************

    The LE callback function. It sends critical messages to the 
    RPG system log file.

***********************************************************************/

#define MAX_NAME_SIZE 128

static void Le_callback (char *msg, int msgsz) {
    static int lbfd = -1;
    LE_message *le_msg_p;
    int ret;

    le_msg_p = (LE_message *)msg;
    if (!(le_msg_p->code & GL_CRIT_BIT))
	return;

    /* We process only GLOBAL and STATUS messages */
    if (!(le_msg_p->code & (GL_GLOBAL_BIT | GL_STATUS_BIT)))
	return;

    if (lbfd < 0) {
	char name[MAX_NAME_SIZE];

	CS_cfg_name ("");
	ret = CS_entry_int_key (ORPGDAT_SYSLOG, ORPGSC_LBNAME_COL, 
					MAX_NAME_SIZE, name);
	if (ret <= 0) {
	    LE_send_msg (0, 
		    "CS_entry_int_key ORPGDAT_SYSLOG failed (ret %d)\n", ret);
	    return;
	}
	lbfd = LB_open (name, LB_WRITE, NULL);
	if (lbfd < 0) {
	    LE_send_msg (0, "LB_open %s failed (ret %d)\n", name, lbfd);
	    return;
	}
    }
    ret = LB_write (lbfd, (char *)msg, msgsz, LB_ANY);
    if (ret < 0)
	LE_send_msg (0, "LB_write RPG status failed (ret %d)\n", ret);
}

