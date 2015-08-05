
/******************************************************************

    This is the main module for install_adapt.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/03/21 15:59:32 $
 * $Id: install_adapt.c,v 1.13 2005/03/21 15:59:32 jing Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include <orpg.h>
#include <orpgadpt.h>
#include <infr.h>

/* command line options */
static char *Source_dir = "";
static char *Source_date = "";
static char *Source_time = "";
static char *Text_out = "";
static int Need_file_move = 0;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {
    int ret;

    if (Read_options (argc, argv) != 0)
	exit (1);

    if ((ret = LE_set_option ("LB type", LB_SINGLE_WRITER)) < 0 || 
	(ret = LE_init (argc, argv)) < 0) {
	LE_send_msg (0, "LE_init failed (%d)", ret);
	exit (1);
    }

    if (strlen (Source_dir) == 0) {	/* The default source dir */
	if ((Source_dir = malloc (ADPTU_NAME_SIZE)) == NULL) {
	    LE_send_msg (0, "malloc failed");
	    exit (1);
	}
	ret = MISC_get_cfg_dir (Source_dir, ADPTU_NAME_SIZE - 8);
	if (ret < 0) {
	    LE_send_msg (0, "MISC_get_cfg_dir failed (%d)", ret);
	    exit (1);
	}
	strcat (Source_dir, "/adapt");
    }

    if (strlen (Text_out) > 0)
	printf ("%s\n", Text_out);
    ORPGADPTU_install_adapt (Source_dir, Source_date, 
				Source_time, Need_file_move);

    exit (0);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "D:d:t:s:mOx:qh?")) != EOF) {
	switch (c) {

	    case 'D':
		Source_dir = optarg;
		break;

	    case 'd':
		Source_date = optarg;
		break;

	    case 't':
		Source_time = optarg;
		break;

	    case 'x':
		Text_out = optarg;
		break;

	    case 'm':
		Need_file_move = 1;
		break;

	    case 's':
	    case 'O':
	    case 'q':
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
    char *usage = "\
    Searches an RPG adaptation data archive in a specified directory,\n\
    which must be local, and installs it in the current RPG environment.\n\
    The file that best matches the specified date and time is used.\n\
    Adaptation data is only installed on the RPG nodes. RPG adaptation\n\
    version number and site name (e.g. KTLX) must match.\n\
    Options:\n\
	-D dir (Specifies the directory where RPG adapt archive files \n\
	   to be found. Default: $CFG_DIR/adapt.)\n\
	-d date (mm/dd/yyyy - Archive file date to install. Default: Today)\n\
	-t time (hh:mm:ss - Archive file time to install. Default: The \n\
	   current time.)\n\
	   Note: If date is specified but not time, the latest file of the \n\
	   date is the best match. Otherwise, the file closest to the \n\
	   specified time is the best match.\n\
	-m (Moves adaptation archive files to installed and not installed \n\
	   sub dirs after installation.)\n\
	-x text (Prints \"text\" on the screen when installing.)\n\
	-h (Print usage info)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}


