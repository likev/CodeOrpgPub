/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/09/30 22:17:54 $
 * $Id: readMda3dInput.c,v 1.7 2011/09/30 22:17:54 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/******************************************************************************
 *	Module:         readMda3dInput.c		                      *
 *	Author:		Brian Klein					      *
 *   	Created:	Feb. 19, 2003					      *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file contains logic taken from the               *
 *                      NSSL MDA Tracking algorithm.                          *
 *									      *
 *      Notes:       	Returns the number of couplets                        *
 ******************************************************************************/
#include <stdio.h>

#include "a309.h"
#include "rpgc.h"
#include "orpg.h"
#include "mda3d_input.h"
#include "mdattnn_params.h"

#define DEBUG 0

int readMda3dInput(cplt_t* new_cplt, int* elev_time, int *vol_num, int *overflow_flg, char* inbuf)
{
        int        i, j, nbr_new_cplts, ii, one_time;
        char*      inbuf_ptr;
        new_cplt_t in_cplt;        /* mda3d output couplet       */
        time_height_xs_t in_th_xs; /* mda3d output time-height cross section  */

        inbuf_ptr = inbuf;
       
        if (DEBUG) fprintf(stderr,"Obtained input buffer...\n");
        
/*      Get the volume number.  It is used in the acl for getting the volume time */

        *vol_num = RPGC_get_buffer_vol_num((void*)inbuf);

/*      The first item in the buffer is the number of couplets. */

        memcpy(&nbr_new_cplts, inbuf_ptr, sizeof(int));
        inbuf_ptr += sizeof(int);

/*      The next item in the buffer is the overflow flag */
        memcpy(overflow_flg, inbuf_ptr, sizeof(int));
        inbuf_ptr += sizeof(int);

/*      The next item in the buffer is the array of elevation times. */

	for(i= 0; i< MESO_NUM_ELEVS; i++) {
	 memcpy(&one_time, inbuf_ptr, sizeof(int)); 
         inbuf_ptr += sizeof(int);
         elev_time[i] = one_time;
        }

        if (DEBUG) fprintf(stderr,"nbr_new_cplts = %d\n",nbr_new_cplts);

        if (nbr_new_cplts < 0) {
          RPGC_log_msg(GL_ERROR,
              "mdattnn: Invalid number of mda3d couplets!  nbr_new_cplts = %d\n",
                                                  nbr_new_cplts);
            RPGC_rel_inbuf((void*)inbuf);
            inbuf = inbuf_ptr = NULL;
            RPGC_abort();
            return (int)0;
        }
        
        if (nbr_new_cplts > MAX_MDA_FEAT) {
          RPGC_log_msg(GL_ERROR,
              "mdattnn: Too many mda3d couplets!  nbr_new_cplts = %d\n",
                                                  nbr_new_cplts);
          nbr_new_cplts = MAX_MDA_FEAT;
        }
        
        for (i = 0; i < nbr_new_cplts; i++) {

/*        The next item in the buffer is the number of elevations  */
/*        (2d features) in the current couplet.                    */

          memcpy(&(new_cplt[i].num2D), inbuf_ptr, sizeof(int));
          inbuf_ptr += sizeof(int);

          if (new_cplt[i].num2D <= 0 || new_cplt[i].num2D > MESO_NUM_ELEVS) {
            if (DEBUG) fprintf(stderr,"Invalid num2D[%d]=%d\n",i,
                                       new_cplt[i].num2D);
            RPGC_log_msg(GL_INFO,
              "mdattnn: Error reading input LB!  [%3d].num2D = %d\n",
                                                    i,new_cplt[i].num2D);
            if(DEBUG) fprintf(stderr,"Invalid num2D, aborting...\n");
            RPGC_rel_inbuf((void*)inbuf);
            inbuf = inbuf_ptr = NULL;
            RPGC_abort();
            return (int)0;           /* go to the next volume */
          }

          for (j = 0; j < new_cplt[i].num2D; j++) {

/*          Copy the time-height cross-section array for this couplet.  */

            memcpy(&in_th_xs, inbuf_ptr, sizeof(time_height_xs_t));
            inbuf_ptr += sizeof(time_height_xs_t);

/*          Copy the items from the input time-height cross section     */
/*          structure to tracking's structure.                          */

            new_cplt[i].mda_th_xs[j].tilt_num = in_th_xs.tilt_num;
            new_cplt[i].mda_th_xs[j].height   = in_th_xs.height;
            new_cplt[i].mda_th_xs[j].diam     = in_th_xs.diam;
            new_cplt[i].mda_th_xs[j].rot_vel  = in_th_xs.rot_vel;
            new_cplt[i].mda_th_xs[j].shear    = in_th_xs.shear;
            new_cplt[i].mda_th_xs[j].gtgmax   = in_th_xs.gtgmax;
            new_cplt[i].mda_th_xs[j].rank     = in_th_xs.rank;
            new_cplt[i].mda_th_xs[j].ca       = in_th_xs.ca;
            new_cplt[i].mda_th_xs[j].cr       = in_th_xs.cr;
            
          } /* end for */

/*        Copy the couplet structure.                                   */
/*        Note that the input structure was defined within mda3d.       */
/*        Its name comes from the NSSL Fortran code array name.         */

          memcpy(&in_cplt, inbuf_ptr, sizeof(new_cplt_t));
          inbuf_ptr += sizeof(new_cplt_t);

/*        Need to copy items from the input couplet structure to tracking's */
/*        couplet structure.                                                */

          new_cplt[i].meso_event_id = in_cplt.meso_id;
          new_cplt[i].circ_class    = in_cplt.cir_type;
          new_cplt[i].tvs_near      = TVS_UNKNOWN;
          new_cplt[i].user_meso     = in_cplt.user_meso; /* old nssl_meso   */
          new_cplt[i].ll_azm        = in_cplt.ll_center_azm;
          new_cplt[i].ll_rng        = in_cplt.ll_center_rng;
          new_cplt[i].llx           = in_cplt.ll_center_x;
          new_cplt[i].lly           = in_cplt.ll_center_y;
          new_cplt[i].nssl_base     = in_cplt.nssl_base;
          new_cplt[i].nssl_top      = in_cplt.nssl_top;
          new_cplt[i].nssl_depth    = in_cplt.nssl_depth;
          new_cplt[i].core_base     = in_cplt.core_base;
          new_cplt[i].base          = in_cplt.base;
          new_cplt[i].strength_rank = in_cplt.strength_rank;

/*        The following elements are included only to be passed on to */
/*        neural network and trending processes.                      */
          
          new_cplt[i].ll_diam               = in_cplt.low_level_dia;
          new_cplt[i].max_diam              = in_cplt.max_level_dia;
          new_cplt[i].ll_rot_vel            = in_cplt.low_level_rot_vel;
          new_cplt[i].max_rot_vel           = in_cplt.max_level_rot_vel;
          new_cplt[i].ll_shear              = in_cplt.low_level_shear;
          new_cplt[i].max_shear             = in_cplt.max_level_shear;
          new_cplt[i].ll_gtg_vel_dif        = in_cplt.low_level_gtg_vel_diff;
          new_cplt[i].max_gtg_vel_dif       = in_cplt.max_level_gtg_vel_diff;
          new_cplt[i].height_max_diam       = in_cplt.height_max_level_dia;
          new_cplt[i].height_max_rot_vel    = in_cplt.height_max_level_rot_vel;
          new_cplt[i].height_max_shear      = in_cplt.height_max_level_shear;
          new_cplt[i].height_max_gtg_vel_dif=
                                       in_cplt.height_max_level_gtg_vel_diff;
          new_cplt[i].depth                 = in_cplt.depth;
          new_cplt[i].core_top              = in_cplt.core_top;
          new_cplt[i].core_depth            = in_cplt.core_depth;
          new_cplt[i].display_filter        = in_cplt.ll_elev_sweep;
          new_cplt[i].msi                   = in_cplt.msi;
          new_cplt[i].msi_rank              = in_cplt.msi_rank;
          new_cplt[i].storm_rel_depth       = in_cplt.storm_relative_depth;
          new_cplt[i].ll_convergence        = in_cplt.low_level_convergence;
          new_cplt[i].ml_convergence        = in_cplt.mid_level_convergence;
          new_cplt[i].vert_int_rot_vel      = in_cplt.vert_integ_rot_vel;
          new_cplt[i].vert_int_shear        = in_cplt.vert_integ_shear;
          new_cplt[i].vert_int_gtg_vel_dif  = in_cplt.vert_integ_gtg_vel_diff;
          new_cplt[i].delta_v_slope         = in_cplt.delt_v_slope;
          new_cplt[i].trend_delta_v_slope   = in_cplt.trend_delt_v_slope;
	  new_cplt[i].detection_status = in_cplt.topped;
          
          if (DEBUG) fprintf(stderr,"id=%d, %3.0f/%3.0f depth=%5.1f, nssl_depth=%5.1f, stm rel depth=%f\n",
            new_cplt[i].meso_event_id,
            new_cplt[i].ll_azm,
            new_cplt[i].ll_rng,
            new_cplt[i].depth,
            new_cplt[i].nssl_depth,
            new_cplt[i].storm_rel_depth);
          
          if (DEBUG) {
          
            for (ii = 0; ii < new_cplt[i].num2D; ii++) 
              fprintf(stderr,"  %6.2f/%6.2f hgt=%d, diam=%d\n",
               new_cplt[i].mda_th_xs[ii].ca,
               new_cplt[i].mda_th_xs[ii].cr,
               new_cplt[i].mda_th_xs[ii].height,
               new_cplt[i].mda_th_xs[ii].diam);
          }
          
        } /* end for */

        if (DEBUG) fprintf(stderr,"Completed reading input buffer...\n");

        return (int)nbr_new_cplts;

} /* end of function readMda3dInput() */

