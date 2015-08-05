/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:40 $
 * $Id: despeckle_results.c,v 1.2 2003/07/17 15:07:40 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************
 
   despeckle_results.c
 
   PURPOSE:

   This routine resets final velocity gates that are noisy compared to
   gates around them.  It attempts to re-unfold the noisy gate; if this
   attempt fails, the gate is set to missing.

   CALLED FROM:
 
   process_mpda_scans
 
   INPUTS:

   None.
 
   CALLS:

   get_azm_avgs
   check_seed_unf
   
   OUTPUTS:

   None.

   RETURNS:
 
   None.
 
   NOTES:
 
   This routine checks whether each gate fits between forward and previous
   radials and forward and previous gates on the same radial.
   If it is not, an attempt is made to dealias it within one of those 
   averages. The checking of the averages in this manner hopefully preserves 
   shear regions but smooths areas where continuity should exist.

   HISTORY:
 
   B. Conway, 2/99      - original development
   B. Conway, 10/00     - cleanup
 
****************************************************************************/
 
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"

void
get_azm_avgs(int az, int gate, short *avg, int *ok);

short 
check_seed_unf(short seed, short vel1, short vel2, short vel3, short thres,
               short sw1,  short sw2,  short sw3, short *sw_sol);

void
despeckle_results()
{

   int   i, j, jtemp, ok, loc_ok;
   int   tot_prf1_rads;

   short avg[4], unf_thres, seed, fst_chk_thresh;
   short vel1, vel2, vel3;
   short sw1, sw2, sw3, fin_sw;

/*
   Make local copies of global variables.
*/
   fst_chk_thresh = th_qc_chk;
   tot_prf1_rads = save.tot_prf1_rads;

/*
   Check each gate on each radial to see if it fits reasonably within
   its surrounding gates
*/

   for(i=0; i<tot_prf1_rads; ++i){

      jtemp = save.vel_limit[i];
      for(j=0; j<jtemp; ++j){

         if( save.final_vel[i][j] >= DATA_THR )
             continue;

/*
  Check to see if the velocities are valid and set the existence
  flags; if none exist, continue.

  If a velocity exists, find the lowest Nyquist velocity available
  and use that as the checks for despeckling. First set the unf_thres
  to use at a very high number. 
*/

         unf_thres = MAX_INT_VEL;

         if( (vel1 = save.vel1[i][j]) < DATA_THR )
            unf_thres = (seed_unf_prf1 < unf_thres) ? seed_unf_prf1 : unf_thres;

         if( (vel2 = save.vel2[i][j]) < DATA_THR )
            unf_thres = (seed_unf_prf2 < unf_thres) ? seed_unf_prf2 : unf_thres;

         if( (vel3 = save.vel3[i][j]) < DATA_THR )
            unf_thres = (seed_unf_prf3 < unf_thres) ? seed_unf_prf3 : unf_thres;

         sw1 = save.sw1[i][j];
         sw2 = save.sw2[i][j];
         sw3 = save.sw3[i][j];
/*
 Get the surrounding averages of the gate in question
*/

         get_azm_avgs(i, j, avg, &ok );

/*
   First see if the gate fits between the previous and forward radial
   average. If not, try to dealias it between those values.
*/

         loc_ok = ok;
         if( (loc_ok & PRE_RAD_AVG_MASK) && (loc_ok & FOR_RAD_AVG_MASK) ){

            if( (abs(save.final_vel[i][j]-avg[PRE_RAD_AVG]) > fst_chk_thresh) && 
                (abs(save.final_vel[i][j]-avg[FOR_RAD_AVG]) > fst_chk_thresh) ){
 
               seed = (avg[PRE_RAD_AVG]+avg[FOR_RAD_AVG])/2;
               if( (save.final_vel[i][j] = check_seed_unf(seed, vel1, vel2, vel3, 
                                                          unf_thres, sw1, sw2,
                                                          sw3, &fin_sw) ) < DATA_THR)
                  {
                  save.final_sw[i][j] = fin_sw;
                  continue;
                  }

            }

         } 

/*
   Check the averages on either side of the gate on the same radial.
*/

         if( (loc_ok & SAME_RAD_AVG1_MASK) && (loc_ok & SAME_RAD_AVG2_MASK) ){

            if( (abs(save.final_vel[i][j]-avg[SAME_RAD_AVG1]) > fst_chk_thresh) && 
                (abs(save.final_vel[i][j]-avg[SAME_RAD_AVG2]) > fst_chk_thresh) ){

               seed = (avg[SAME_RAD_AVG1]+avg[SAME_RAD_AVG2])/2;
               if( (save.final_vel[i][j] = check_seed_unf(seed, vel1, vel2, vel3,
                                                          unf_thres, sw1, sw2,
                                                          sw3, &fin_sw) ) < DATA_THR)
                  {
                  save.final_sw[i][j] = fin_sw;
                  continue;
                  }
            }

         } 

      }

   }

}
