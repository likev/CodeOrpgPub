
/******************************************************************

	file: nds_main.c

	This is the main module for the nds program - the node
	service.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/03/07 22:36:15 $
 * $Id: nds_main.c,v 1.9 2007/03/07 22:36:15 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 

#include <nds.h>
#include "nds_def.h"

static int Verbose = 0;			/* verbose level */
static int Manage_mrpg = 0;

static char Nds_lb_name[NDS_NAME_SIZE];	/* nds LB name */
static int Nds_fd;			/* nds LB file descriptor */
static int Max_no_processes = 256;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Redirect_to_le (char *msg);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    int ret, n_files;

    /* Initialize the LE service */
    ret = LE_create_lb (argv[0], 1000, 0, -1);
    if (ret < 0) {
	LE_send_msg (0,  "LE_create_lb failed (ret %d)", ret);
	exit (1);
    }
    ret = LE_init (argc, argv);
    if (ret < 0) {
	LE_send_msg (0,  "LE_init failed (ret %d)", ret);
	exit (1);
    }

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (1);
    LE_local_vl (Verbose);

    MISC_log_reg_callback (Redirect_to_le);

    /* open the nds LB */
    Nds_fd = LB_open (Nds_lb_name, LB_WRITE, NULL);
    if (Nds_fd < 0) {
	LE_send_msg (0, "LB_open %s failed (ret %d)", 
						Nds_lb_name, Nds_fd);
	exit (1);
    }

    /* set max number of open file descriptors */
    n_files = Max_no_processes * 2 + 64;
    if ((ret = MISC_rsrc_nofile (n_files)) < n_files) {
        LE_send_msg (0, "MISC_rsrc_nofile failed (ret %d)", ret);
        exit (1);
    }
    LE_send_msg (0, "max number of open files set to %d", ret);

    if (NDSPS_init () != 0)
	exit (1);

    /* apply lock to make sure only one instance is running */
    if (LB_lock (Nds_fd, LB_EXCLUSIVE_LOCK, LB_LB_LOCK) == 
						LB_HAS_BEEN_LOCKED) {
	LE_send_msg (0, "Another nds is running - stop");
	exit (0);
    }

    /* the main loop */
    while (1) {
	NDSPS_update ();
	msleep (1000);
    }

    exit (0);
}

/**************************************************************************

    Redirects the libinfr error message to LE.

**************************************************************************/

static void Redirect_to_le (char *msg) {
    LE_send_msg (0, "%s", msg);
}

/**************************************************************************

    Description: Returns the nds LB file descriptor.

**************************************************************************/

int NDS_get_nds_fd () {
    return (Nds_fd);
}

/**************************************************************************

    Restart mrpg for management.

**************************************************************************/

void NDS_resume_mrpg () {
    int ret;
    char b[80];

    if (!Manage_mrpg)
	return;
    ret = MISC_system_to_buffer ("mrpg -b resume &", b, 80, NULL);
    if (ret < 0)
	LE_send_msg (0, "MISC_system_to_buffer mrpg resume returns %d", ret);
    else
	LE_send_msg (0, "\"mrpg resume\" invoked");
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;	/* used by getopt */
/*    extern int optind; */
    int c;			/* used by getopt */
    int err;			/* error flag */

    err = 0;
    strcpy (Nds_lb_name, "nds.lb");
    while ((c = getopt (argc, argv, "hf:n:mv?")) != EOF) {
	switch (c) {

	    case 'f':
		strncpy (Nds_lb_name, optarg, NDS_NAME_SIZE);
		Nds_lb_name[NDS_NAME_SIZE - 1] = '\0';
		break;

	    case 'n':
		sscanf (optarg, "%d", &Max_no_processes);
		break;

	    case 'm':
		Manage_mrpg = 1;
		break;

	    case 'v':
		Verbose = 3;
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

**************************************************************************/

static void Print_usage (char **argv) {
    printf ("Usage: %s (options)\n", argv[0]);
    printf ("       Options:\n");
    printf ("       -f nds_lb_name (nds LB name. default: nds.lb)\n");
    printf ("       -n max_num_procs (maximum number of processes monitored. default: 256)\n");
    printf ("       -v (sets initial verbose level to 3. default: 0)\n");
    printf ("       -m (managing mrpg - restart mrpg when it is missing)\n");
    exit (0);
}

