/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:17:00 $
 * $Id: hca_buffer_control.c,v 1.11 2012/03/12 13:17:00 ccalvert Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */


/******************************************************************************
 *      Module:         Hca_buffer_control                                    *
 *      Author:         Brian Klein, Yukuan Song                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    This is the algorithm control loop for the HCA        *
 *                                                                            *
 *      Change History:                                                       *
 *       Jan 2012: NA11-00387 Added partial beam blockage modification. BKlein*
 ******************************************************************************/
#include <unistd.h>
#include <rpgcs.h>
#include <rpgc.h>
#include "hca_local.h"
#include "hca_adapt.h"

#define HAIL_DEA_FILE "alg.hail."
#define QPE_DEA_FILE  "alg.dp_precip."
#define KM_PER_KFT    0.3048
#define HALF_KM       0.5
#define DB            0 
#define EXTERN        extern


#include "hca_adapt_object.h"
#include <orpgsite.h>

 /* melting layer data structure */
typedef struct
{
  float top;
  float bottom;
} ML_data_t;


/* Global variables */
    static float    ML_top[MAXRADIALS];         /* Melting layer top height (km) from MLDA    */
    static float    ML_bottom[MAXRADIALS];      /* Melting layer bottom height (km) from MLDA */
    static double   Default_top;
    static int      first_volume = 1;

 /****************************************************************************
    Description:
       HCA algorithm control loop
    Input:
    Output:
    Returns:
    Globals:
    Notes:
  ***************************************************************************/

int Hca_buffer_control(){

    /* Local variables */
    int *obuf_ptr, *ibuf_ptr, *imelt_ptr;
    int  length, in_status, out_status;
    char *output;

    int   i; /* loop index */
    int   vcp_num;
    int   elev_ind;
    int   radial_status;
    float elev_ang;
    float bottom;
    static float radar_height=0;
   
    Base_data_header *base_data;

    /* Do until end of elevation or volume. */
    /* CPT&E label A */
    while(1){
       /* Get a radial of Quality Index Algorithm output. */
       ibuf_ptr = (int*)RPGC_get_inbuf_by_name( "DP_BASE_AND_QUALITY", &in_status );

       if (in_status == RPGC_NORMAL){

          /* Get the radial status, target elevation angle and bin size */
          base_data = (Base_data_header *)ibuf_ptr;
          radial_status =  base_data->status;

          /* At the beginning of an elevation, compute the ranges where the
             beam top, center and base intersect the melting layer.     
             Also, get the beam blockage data for this elevation angle.  */
          if (radial_status == BEG_ELEV || radial_status == BEG_VOL){

             vcp_num  = RPGC_get_buffer_vcp_num ( (void*)ibuf_ptr );
             elev_ind = RPGC_get_buffer_elev_index ( (void*)ibuf_ptr );
             elev_ang = RPGCS_get_target_elev_ang ( vcp_num, elev_ind );

             read_Blockage(elev_ang, Beam_Blockage);
 
             if (radial_status == BEG_VOL){
                /* Obtain the radar height */
                radar_height = ORPGSITE_get_int_prop(ORPGSITE_RDA_ELEVATION) * FT_TO_KM;

                /* get the adaptable parameter "0 degree height" from HAIL */
                if(RPGC_ade_get_values(HAIL_DEA_FILE, "height_0", &Default_top) != 0){
                   RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value() - Default_top");
                   Default_top = 10.5;
                }

                /* get the adaptable parameter for minimum blockage threshold for FShield */
                double value;
                if(RPGC_ade_get_values(QPE_DEA_FILE, "Min_blockage", &value) != 0){
                   RPGC_log_msg(GL_ERROR, ">> Get_adapt_data(): RPGC_ade_get_value() - Min_blockage_thresh");
                   Min_blockage_thresh = 5;
                }
                else {
                   Min_blockage_thresh = (int) value;
                }

                /* convert default top from kft to km */
                Default_top *= KM_PER_KFT;

                /* The height_0 value is above MSL so make it above radar level */
                Default_top -= radar_height;

                /* Ensure top height is above ground */
                if (Default_top < 0.0) Default_top = 0.0;

                bottom = Default_top - HALF_KM;

                /* Ensure bottom height is above ground */
                if (bottom < 0.0) bottom = 0.0;
    
                if(first_volume == 1) {
                   first_volume = 0;
                   for (i=0;i<MAXRADIALS;i++) {
                      ML_top[i] = Default_top;
                      ML_bottom[i] = bottom;
                   }
                }

                /* Create lookup table by pre-computing f1, f2, f3, g1 and
                   g2 values for all possible input Z index values.             */
                /* CPT&E label B */
/*                MemLookup();*/ /*REMOVED FOR CCR NA11-00387.  Can't precompute */
                                 /* these membership funcitons using indexed Z   */
                                 /* because the fshield adjustment is finer than */
                                 /* the 0.5 dBZ indexed Z resolution and changes */
                                 /* each azimuth.                                */

                /* Access adaptation data to define membership functions and
                   input field weights.                                         */
                /* CPT&E label C */
                DefineMembershipFuncsAndWeights();
             }/* end if beginning of volume */
          }/* end if beginning of elevation or volume */

          /* Process this radial. */
          /* CPT&E label D */
          radial_status = Hca_process_radial((char *)ibuf_ptr, (char **)&output, &length, 
                                              ML_top, ML_bottom);

          obuf_ptr = (int*)RPGC_get_outbuf_by_name( "DP_BASE_HC_AND_ML", length, &out_status );

          if (out_status != RPGC_NORMAL) {
             RPGC_log_msg(GL_INFO,"Aborting...out_status = %d",out_status);
             RPGC_cleanup_and_abort( out_status );
             ibuf_ptr = NULL;
             return (-1);
          }

          if (radial_status < 0) {

             /* All required fields are not present.  Just copy the radial and send it out */
             memcpy (obuf_ptr, ibuf_ptr, length);

             /* Release the output buffer with FORWARD disposition */
             RPGC_rel_outbuf( obuf_ptr, FORWARD | RPGC_EXTEND_ARGS, length );
          }

          else if (length > 0) {
             
             /* CPT*E label E */
             /* Copy the HCA results to the output buffer */
             memcpy (obuf_ptr, output, length);

             /* Release the output buffer with FORWARD disposition */
             RPGC_rel_outbuf( obuf_ptr, FORWARD|RPGC_EXTEND_ARGS, length);

          }/* end if length > 0 */
            
          /* Read melting-layer data at the end of the volume. */
          /* Typically, it is generated at the end of the 10 degree elevation */
          if(radial_status == END_VOL) {

             imelt_ptr = (int*)RPGC_get_inbuf_by_name ("MLDA", &in_status);

             if(in_status == RPGC_NORMAL){
                RPGC_log_msg(GL_INFO,"Received ML data for use in next volume");
                ML_data_t *mlout = (ML_data_t *) imelt_ptr;

                for (i=0;i<MAXRADIALS;i++) {
                 ML_top[i] = mlout[i].top;
                 ML_bottom[i] = mlout[i].bottom;
                 }
             }
             else {
                RPGC_log_msg(GL_INFO,"Using default ML for use in next volume");
                float bottom;
                bottom = Default_top - HALF_KM;
                if (bottom < 0.0) bottom = 0.0;
                for (i=0;i<MAXRADIALS;i++) {
                  ML_top[i] = Default_top;
                  ML_bottom[i] = bottom;
                }
             }
             RPGC_rel_inbuf( (void*)imelt_ptr);
          } /* end if END_VOL */

          /* Release the input buffer */
          RPGC_rel_inbuf( (void*)ibuf_ptr );
          ibuf_ptr = NULL;

          /* Process data till end of elevation or volume scan. */
          if( radial_status == GENDEL ||
              radial_status == GENDVOL ){
             return (0);
          }
       }
       else{
          /* Status returned from RPGC_get_inbuf_by_name was not RPGC_NORMAL so ABORT!. */
          RPGC_log_msg(GL_INFO,"HCA: Failed at RPGC_get_inbuf_by_name (%d)\n",in_status);
          RPGC_cleanup_and_abort(in_status);
          ibuf_ptr = NULL;
       }
    } /* end while */
    return (0);
} /* End of Hca_buffer_control() */

