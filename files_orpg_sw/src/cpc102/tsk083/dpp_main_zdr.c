
/******************************************************************

    This is the main module for the dual-pol radar data preprocesing
    program.
	
******************************************************************/

/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/08/15 19:40:52 $ */
/* $Id: dpp_main_zdr.c,v 1.1 2014/08/15 19:40:52 steves Exp $ */
/* $Revision:  */
/* $State: */

#include <rpgc.h>
#include <rpgcs.h>
#include <orpgsite.h>
#include "dpprep.h"
#include "findBragg.h"
#include <dpprep_isdp.h>

static int  Process_data ();
static void Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Read_Bragg_data ();

/* Global variables for output of system PhiDP estimate */
float       Current_system_phidp = -99;
float       Last_system_phidp = -99;
float       RDA_system_phidp = -99;
time_t      Current_time;
int         Current_hour;
int         Previous_hour;
short       Init_index = 0;      /* 0 means init the queue index */
short       Too_close_count = 0; /* Counts radials with weather too close */
Dpp_isdp_est_t Isdp_est;

/* Global variables for output of Bragg-based ZDR bias estimate */
Bragg_data_t Bragg_data;
int         DPP_ZDR_DYNAMIC_ADJUST = 0;
float       DPP_ZDR_DYNAMIC_ADJUST_VALUE = -99.0;

/************************************************************************

   Description: 
      This is the main function for the dual-pol radar data preprocessing
      program. 

***********************************************************************/

int main( int argc, char *argv[] ) {

/*    int est_sys_phi;*/
/*    int rda_value;*/
#ifdef DPPREP_TEST
    /* DPPL_test (); */
    Test_process_radial (argc, argv);
    exit (0);
#endif

    Read_options (argc, argv);

    /* Initialize the log error services. */
    RPGC_init_log_services( argc, argv );

    /* Specify inputs and outputs. */
    RPGC_reg_io( argc, argv );

    /* Tell system we are radial-based. */
    RPGC_task_init( RADIAL_BASED, argc, argv );

    /* Data needed to determine when to output the estimated system PhiDP message. */
    Current_time = time( NULL );
    Current_hour = Current_time / SECS_IN_HOUR;
    Previous_hour = Current_hour;

    /* Read in any previous Bragg scatter data */
    Read_Bragg_data ();

    /* Waiting for activation. */
    while(1){

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        Process_data ();

    }
}

/************************************************************************ 
   Description: 
      This module reads and processes a radial data.

   Returns:
      Returns -1 on abort, 0 otherwise. 

************************************************************************/
static int Process_data (){

    int *obuf_ptr, *ibuf_ptr;
    int length, in_status, out_status, radial_status, vol_num;
    char *output;

    /* Do Until end of elevation or volume. */
    while(1){

        /* Get a radial of RAW BASE DATA. */
        ibuf_ptr = (int *)RPGC_get_inbuf_by_name( "DPP_INPUT", &in_status );
        if( in_status == RPGC_NORMAL ){

	    /* Process this radial. */
	    radial_status = DPPF_process_radial ( (char *)ibuf_ptr ,
					(char **)&output, &length);

	    if (length > 0) {
		obuf_ptr = (int *)RPGC_get_outbuf_by_name( "DPP_OUTPUT", length, &out_status );

		if (out_status != RPGC_NORMAL) {

		    /* RPGC_get_outbuf_by_name status not RPGC_NORMAL: ABORT */
                    LE_send_msg (GL_ERROR, "dpprep get_outbuf_by_name failed");
		    RPGC_abort_because( out_status );

		    /* Release the input buffer */
		    RPGC_rel_inbuf( ibuf_ptr );
		    return -1;
		}
		memcpy (obuf_ptr, output, length);

		/* Release the output buffer with FORWARD disposition */
		RPGC_rel_outbuf( obuf_ptr, FORWARD );
	    }

	    /* Process data till end of elevation or volume scan. */
	    if( radial_status == GENDEL ||
		radial_status == GENDVOL ) {

               /* Get the current volume scan start date and time */
               vol_num  = RPGC_get_buffer_vol_num((void*)ibuf_ptr);

               /* Perform end-of-elevation processing */
               DPPC_end_of_elev_proc(vol_num, radial_status, Current_system_phidp);

               /* Release the input buffer and return */
	       RPGC_rel_inbuf( ibuf_ptr );
               return 0;
            }
            /* Release the input buffer and continue processing this elevation */
            RPGC_rel_inbuf( ibuf_ptr );

        }
	else{
 
            /* RPGC_get_inbuf_by_name status not RPGC_NORMAL: ABORT. */
            LE_send_msg (GL_ERROR, "dpprep get_inbuf_by_name failed");
	    RPGC_abort();
	    return -1;
        }
    }
    return 0;
} 

/**************************************************************************

    Reads command line arguments. "argc" is the number of command
    arguments and "argv" is the list of command arguments.

**************************************************************************/

static void Read_options (int argc, char **argv) {
    int c;                  /* used by getopt */

    while ((c = getopt (argc, argv, "alh?z:")) != EOF) {

	switch (c) {

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
            case 'z':
                fprintf(stderr,"optarg = %s\n",optarg);
                if (strcmp(optarg,"Bragg") == 0) {
                   DPP_ZDR_DYNAMIC_ADJUST = 1;
                   DPP_ZDR_DYNAMIC_ADJUST_VALUE = -99.0;
                   fprintf(stderr,"ZDR dynamic adjustment active using Bragg estimate\n");
                }
                else {
                   DPP_ZDR_DYNAMIC_ADJUST = 2; 
                   DPP_ZDR_DYNAMIC_ADJUST_VALUE = atof(optarg);
                   fprintf(stderr,"ZDR adjustment active using %f\n",DPP_ZDR_DYNAMIC_ADJUST_VALUE);
                }
                break;
	}
    }
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Pre-processes the dual-pol basedata.\n\
        Options:\n\
          -h (Prints usage info and terminates)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}

/**************************************************************************

    Reads in any previous Bragg scatter state data from bragg.dat.
    If the redundant channel has changed, reinitializes for new estimate.

**************************************************************************/

static void Read_Bragg_data () {
    int   ret, bytes_read;
    char* buffer = NULL;
    int redundant_type = NO_REDUNDANCY;
    int channel_num = 1;
    Redundant_info_t redundant_info;

    ret = RPGC_data_access_open(DP_BRAGG, LB_READ | LB_WRITE);
    if (ret < 0){
       LE_send_msg (GL_INFO, "bragg.dat file not found");
       Bragg_data.Last_zdr_bias = -99;
    }
    else {
       /* Read in the Bragg scatter state data */
       bytes_read = RPGC_data_access_read(DP_BRAGG,
                                      &buffer,
                                      LB_ALLOC_BUF,
                                      (LB_id_t) DP_BRAGG_MSGID);

       if (buffer == NULL) {
          LE_send_msg (GL_INFO, "bragg.dat pointer NULL");
          Bragg_data.Last_zdr_bias = -99;
          Bragg_data.Bragg_channel = 1;
          Bragg_data.Last_report_hour = -99;
       }
       else {
          memcpy(&Bragg_data, buffer, sizeof(Bragg_data_t));
          LE_send_msg (GL_INFO, "Bragg data:  %d %d %f %d %d",
                                Bragg_data.Last_zdr_bias_time,
                                Bragg_data.Last_zdr_bias_date,
                                Bragg_data.Last_zdr_bias,
                                Bragg_data.Bragg_channel,
                                Bragg_data.Last_report_hour);
          if(buffer != NULL)
            free(buffer);

          /* Get redundant type. */
	  if( ORPGSITE_get_redundant_data( &redundant_info ) >= 0 )
	     redundant_type = redundant_info.redundant_type;

          /* Get channel number. */
	  if( (channel_num = ORPGRDA_channel_num()) < 0 )
	     channel_num = 1;

          /* Reset if channel has changed */
          if (channel_num != Bragg_data.Bragg_channel) {
                  Bragg_data.Last_zdr_bias = -99;
                  Bragg_data.Bragg_channel = channel_num;
          }
       }
    }
    return;
}

