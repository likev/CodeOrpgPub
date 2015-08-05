/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:12:01 $
 * $Id: qia_acl.c,v 1.5 2012/03/12 13:12:01 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/******************************************************************************
     Filename: qia_acl.c
     Author:   Brian Klein
     Created:  13 September 2007

     Description
     ===========
     This is the algorithm control module for the Quality Index Algorithm.
     This algorithm was spun off of the original Hydrometeor Classification
     algorithm as a front-end processor for HCA input.  It ingests radials
     of DualPol data from the Preprocessor (dpprep). It outputs the same
     input data appending to it the quality index for each input data
     field.         

     Initial implementation is a "simple" version that is based mostly on
     differential phase (PhiDP) and does not require gradient processing.
     The "complex" version does require gradient processing and therefore
     introduces a significant latency in the processing stream.     

     Change History
     ==============
     Brian Klein;   13 September 2007;  CCR TBD;  Initial implementation
     Brian Klein;   24 January 2012;  CCR NA11-00386 Partial Beam Blockage
	
******************************************************************************/

#include <rpgc.h>
#include <rpgcs.h>
#include "qia.h"

#define MAX_LENGTH	40000

/******************************************************************************
     Description:  This is the algoithm control function for the dual-pol
     radar data Quality Index Algorithm.
     Input:    None.
     Output:   None.
     Returns:  0 on success, -1 on error.
     Notes:    
******************************************************************************/
int Qia_acl (void){
 
    int   *obuf_ptr, *ibuf_ptr;
    int   length, in_status, out_status, radial_status;
    int   elev_ind;
    int   vcp_num;
    float elev_ang;
    char  *output;
    Base_data_header *base_data;


    /* Do Until end of elevation or volume. (CPT&E label A) */
    while(1){

       /* Get a radial of DUAL POL BASE DATA. */
       ibuf_ptr = (int*)RPGC_get_inbuf_by_name( "DUALPOL_COMBBASE", &in_status );
       if (in_status == RPGC_NORMAL) {

          /* Get a output buffer of DP_BASE_AND QUALITY. */
	     obuf_ptr = (int*)RPGC_get_outbuf_by_name( "DP_BASE_AND_QUALITY",
                                                   MAX_LENGTH, &out_status );

          if (out_status != RPGC_NORMAL) {

             RPGC_log_msg( GL_INFO, "Aborting...out_status = %d",out_status );

             /* RPGC_get_outbuf_by_name status not RPGC_NORMAL: ABORT */
	        RPGC_abort_because( out_status );

	        /* Release the input buffer */
	        RPGC_rel_inbuf( ibuf_ptr );
	        return -1;

	  }/* end if out_status != RPGC_NORMAL */

          base_data = (Base_data_header *)ibuf_ptr;
          radial_status =  base_data->status;

          /* At the beginning of an elevation, get the beam blockage data   */
          /* for this elevation angle.                                      */
          if (radial_status == BEG_ELEV || radial_status == BEG_VOL){

             vcp_num  = RPGC_get_buffer_vcp_num ( (void*)ibuf_ptr );
             elev_ind = RPGC_get_buffer_elev_index ( (void*)ibuf_ptr );
             elev_ang = RPGCS_get_target_elev_ang ( vcp_num, elev_ind );

             read_Blockage(elev_ang, Beam_Blockage);
          }

	  /* Process this radial. (CPT&E label B)*/
	  radial_status = Qia_process_radial ( (char *)ibuf_ptr, (char **)
                                              &output, &length );

          /* If no dual pol moments, simply copy input radial to output 
             and release. */
          if (radial_status == -1 ) {

             /* Copy input to output and release input and output. */
             memcpy( obuf_ptr, ibuf_ptr, length );
             RPGC_rel_outbuf( obuf_ptr, FORWARD | RPGC_EXTEND_ARGS, length );

          }
	     else if (length > 0) {

	        memcpy( obuf_ptr, output, length );

	        /* Release the output buffer with FORWARD disposition */
	        RPGC_rel_outbuf( obuf_ptr, FORWARD|RPGC_EXTEND_ARGS, length );

	     }
          else {
             RPGC_log_msg(GL_ERROR,"LENGTH <= 0!!! = %d", length);
	        RPGC_rel_outbuf( obuf_ptr, DESTROY, length );
          }/* end if length > 0 */

          /* Release the input buffer */
	     RPGC_rel_inbuf( ibuf_ptr );

	     /* Process data till end of elevation or volume scan. */
	     if( radial_status == GENDEL || radial_status == GENDVOL )
	        return (0);
       }
       else{
          RPGC_log_msg(GL_INFO,"Aborting...in_status = %d",in_status);
          /* RPGC_get_inbuf_by_name status not RPGC_NORMAL: ABORT. */
	     RPGC_abort();
	     return -1;
       }
    }/* end while*/
    return 0;
} 
