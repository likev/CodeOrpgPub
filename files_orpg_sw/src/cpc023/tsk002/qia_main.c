/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:12:02 $
 * $Id: qia_main.c,v 1.4 2012/03/12 13:12:02 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
*/
/******************************************************************************
     Filename: qia_main.c
     Author:   Brian Klein
     Created:  13 September 2007

     Description
     ===========
     This is the main module for the Quality Index Algorithm. This
     algorithm was spun off of the original Hydrometeor Classification
     algorithm as a front-end processor for HCA input.

     Change History
     ==============
     Brian Klein;   13 September 2007;  CCR TBD;  Initial implementation
     Brian Klein;   26 January 2012;   CCR NA11-00386; Partial Beam Blockage
	
******************************************************************************/

#include <rpgc.h>
#include "qia.h"

#define QIA_DEA_NAME "alg.qia."
#define QPE_DEA_FILE  "alg.dp_precip."

static void Read_options (int argc, char **argv);
static void Print_usage (char **argv);

/******************************************************************************

     Description:  This is the main function for the dual-pol radar data
                   Quality Index Algorithm.
     Input:    None.
     Output:   None.
     Returns:  Nothing.
     Notes:    Currently, no input arguments are defined. 

******************************************************************************/

int main( int argc, char *argv[] ) {

    double get_value;

    Read_options (argc, argv);

    /* Initialize the log error services. */
    RPGC_init_log_services( argc, argv );

    /* Specify inputs and outputs. */
    RPGC_reg_io( argc, argv );

    /* Tell system we are volume-based. */
    RPGC_task_init( VOLUME_BASED, argc, argv );

    /* Waiting for activation. */
    while(1){

        if(RPGC_ade_get_values(QIA_DEA_NAME, "z_atten_thresh", &get_value) != 0) {
           RPGC_log_msg(GL_INFO, ">> qia.alg not found, using default Z_atten_thresh value");
           Z_atten_thresh = (float) 25.0;
        }
        else
           Z_atten_thresh = (float) get_value;

        /* Get the parameter for minimum blockage threshold for FShield */
        if(RPGC_ade_get_values(QPE_DEA_FILE, "Min_blockage", &get_value) != 0){
           RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value() - Min_blockage_thresh");
           Min_blockage_thresh = 5;
        }
        else {
           Min_blockage_thresh = (int) get_value;
        }

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        Qia_acl();
    }
}

/**************************************************************************

    Reads command line arguments. "argc" is the number of command
    arguments and "argv" is the list of command arguments.

**************************************************************************/

static void Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */

    while ((c = getopt (argc, argv, "alh?")) != EOF) {

	switch (c) {

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Computes DP Quality Index for the dual-pol basedata.\n\
        Options:\n\
          -h (Prints usage info and terminates)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
