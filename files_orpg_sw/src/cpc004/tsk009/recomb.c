
/******************************************************************

    This is the main module for the super-resolution radar data
    recombination program.
	
******************************************************************/

/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2010/07/12 20:10:02 $ */
/* $Id: recomb.c,v 1.18 2010/07/12 20:10:02 ccalvert Exp $ */
/* $Revision:  */
/* $State: */

#include <rpgc.h>
#include "recomb.h"

static int Range_only = 0;
static int Azi_only = 0;
static int Index_azimuth = 1;	/* use azm_index to detemine indexed radial */
static int Ldm_version = -1;

static int Process_data ();
static void Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int To_be_discarded (void *radial);
static int Get_archive_II_version ();

/************************************************************************

   Description: 
      This is the main function for the super resolution to normal 
      resolution conversion problem. 

***********************************************************************/

int main( int argc, char *argv[] ){

    Read_options( argc, argv );

    /* Initialize the log error services. */
    RPGC_init_log_services( argc, argv );

    /* Specify inputs and outputs. */
    RPGC_reg_io( argc, argv );

    /* Tell system we are radial-based. */
    RPGC_task_init( RADIAL_BASED, argc, argv );

    CR_combine_init (Range_only, Azi_only, Index_azimuth);

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

    int *ibuf_ptr;
    int length, in_status, out_status, radial_status, elev_num;
    char *output;
    static int arch2_v = -1;

    /* Do Until end of elevation or volume. */
    while(1){

        /* Get a radial of RAW BASE DATA. */
        ibuf_ptr = RPGC_get_inbuf_by_name( "RECOMB_INPUT", &in_status );
        if( in_status == RPGC_NORMAL ){

	    Base_data_header *bh = (Base_data_header *)ibuf_ptr;
	    radial_status = bh->status;
            elev_num = bh->elev_num;
	    if (radial_status == GOODBVOL && elev_num == 1 && Ldm_version >= 0)
		arch2_v = Get_archive_II_version ();

	    if (Ldm_version < 0 || Ldm_version == arch2_v) {
		/* Process this radial. */
		radial_status = CR_combine_radials( (char *)ibuf_ptr ,
					    (char **)&output, &length);
    
		if (length > 0 && 
		    (out_status = MAIN_output_radial (output, length)) != 
							    RPGC_NORMAL) {
    
		    /* RPGC_get_outbuf_by_name status not RPGC_NORMAL: ABORT */
		    RPGC_abort_because( out_status );
	
		    /* Release the input buffer */
		    RPGC_rel_inbuf( ibuf_ptr );
		    return -1;
		}
	    }

	    /* Release the input buffer */
	    RPGC_rel_inbuf( ibuf_ptr );

	    /* Process data till end of elevation or volume scan. */
	    if( bh->status == GENDEL ||
		bh->status == GENDVOL )
	        return (0);
        }
	else{
 
            /* RPGC_get_inbuf_by_name status not RPGC_NORMAL: ABORT. */
	    RPGC_abort();
	    return -1;
        }
    }
    return 0;
}

/**************************************************************************

    We must discard some radials so the output radials have an average
    azimuthal separation of 1. degrees and a total number of about
    360. This is needed when a significant number of super-res radials must
    be single-radial recombined. This function returns 1 if the current 
    radial needs to be discarded because its azimuth does not advance 
    sufficiently. It returns 0 otherwise. The azi_num must be reassigned 
    because some radials are discarded.

**************************************************************************/

static int To_be_discarded (void *radial) {
    static double cr_out_azi = -1.;
    static int azi_num = 1;
    Base_data_header *bh;
    int discard;

    bh = (Base_data_header *)radial;
    if (!(bh->msg_type & RECOMBINED_TYPE)|| Range_only)
	return (0);
    if (bh->status == GOODBEL || bh->status == GOODBVOL || bh->azi_num == 1)
	cr_out_azi = -1.;

    discard = 0;
    if (cr_out_azi < 0.) {
	cr_out_azi = bh->azimuth;
	azi_num = 1;
    }
    else {
	double diff = bh->azimuth - cr_out_azi;
	if (diff < -180.)
	    diff += 360.;
	if (diff > 180.)
	    diff -= 360.;
	if (diff < .5 && bh->status == GOODINT)
	    discard = 1;
	else {
	    if (Index_azimuth)
		cr_out_azi = bh->azimuth;
	    else
		cr_out_azi += 1.;
	    azi_num++;
	}
    }
    if (!discard)
	bh->azi_num = azi_num;
    return (discard);
}

/**************************************************************************

    Outputs radial "output" of size "length". Returns out_status from
    RPGC_get_outbuf_by_name.

**************************************************************************/

int MAIN_output_radial (char *output, int length) {
    int *obuf_ptr;
    int out_status;
    Base_data_header *bh;

    if (To_be_discarded (output))
	return (RPGC_NORMAL);

    obuf_ptr = RPGC_get_outbuf_by_name( "RECOMB_OUTPUT", length, &out_status );

    if (out_status != RPGC_NORMAL) {
	return out_status;
    }
    memcpy (obuf_ptr, output, length);
    if (!Azi_only) {
	bh = (Base_data_header *)obuf_ptr;
	if (!Range_only && bh->n_dop_bins > BASEDATA_DOP_SIZE)
	    bh->n_dop_bins = BASEDATA_DOP_SIZE;
	if (bh->n_surv_bins > BASEDATA_REF_SIZE)
	    bh->n_surv_bins = BASEDATA_REF_SIZE;
    }

    /* Release the output buffer with FORWARD disposition */
    RPGC_rel_outbuf( obuf_ptr, FORWARD );
    return out_status;
}

/**************************************************************************

    Returns the value of adaptation data "alg.Archive_II.version".

**************************************************************************/

static int Get_archive_II_version () {
    double d;
    int ret;
    char *id;

    id = "alg.Archive_II.version";
    if ((ret = DEAU_get_values (id, &d, 1)) <= 0) {
	LE_send_msg (GL_ERROR, 
		"DEAU_get_values %s failed (%d)\n", id, ret);
	return (-1);
    }
    return ((int)d);
}

/**************************************************************************

    Reads command line arguments. "argc" is the number of command
    arguments and "argv" is the list of command arguments.

**************************************************************************/

static void Read_options (int argc, char *argv[] ) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */

    while ((c = getopt (argc, argv, "al:T:V:RAh?")) != EOF) {

	switch (c) {

            case 'a':
		Index_azimuth = 0;
                break;

            case 'R':
	       Range_only = 1;
               break;

            case 'A':
	       Azi_only = 1;
               break;

            case 'V':
		if (sscanf (optarg, "%d", &Ldm_version) != 1) {
		    LE_send_msg (GL_ERROR, "Incorrect -V option specified\n");
		    exit (1);
		}
		break;

            case 'l':
               break;

            case 'T':
               break;

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
        Performs recombination of the super-resolution radar data and\n\
        generates the normal resolution data.\n\
        Options:\n\
	  -R (Range recombination only)\n\
	  -A (Azimuth recombination only)\n\
          -a (Assuming non-indexed azimuth)\n\
          -l (Task Log file size)\n\
	  -V ldm_version (specifies the LDM version number)\n\
          -h (Prints usage info and terminates)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
