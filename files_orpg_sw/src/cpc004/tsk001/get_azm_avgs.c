/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:52 $
 * $Id: get_azm_avgs.c,v 1.2 2003/07/17 15:07:52 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   get_azm_avgs.c

   PURPOSE:

   Given a radial and gate, this routine finds the various averages
   surrounding that location.

   CALLED FROM:

   first_triplet_attempt
   second_triplet_attempt
   pairs_and_trips_attempts
   final_unf_attempts
   replace_arig_vals
   unf_azimuthal_errors
   despeckle_results
   
   INPUTS:

   int rad               - radial number of gate location
   int gate              - gate number of location

   short *pre_rad_avg    - average along the previous radial
   short *for_rad_avg    - average along the forward radial
   short *same_rad_avg1  - average along current radial prior to gate
   short *same_rad_avg2  - average along current radial beyond current gate

   int *ok_pre_rad_avg    - flag for average along the previous radial
   int *ok_for_rad_avg    - flag for average along the forward radial
   int *ok_same_rad_avg1  - flag for average along current radial prior to gate
   int *ok_same_rad_avg2  - flag for average along current radial beyond current gate
   
   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   The distance along the radials to use for averaging purposes is defined
   by the adapatable parameters gate_for and gates_back.

   HISTORY:

   R. May, 02/03        - corrected to account for center bin on forward and
                          previous radials, check that forward and previous
                          radials are close enough to the current one
   B. Conway, 10/00     - cleanup
   B. Conway, 5/99      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"

void get_azm_avgs(int rad, int gate, short *avg, int *ok ){

   int start, end, i, rad_for, rad_prev;
   int loc_ok, tot_prf1_rads, rad_offset;

   short num_avg1, num_avg2, num_avg3, num_avg4;
   short pra, fra, sra1, sra2;
   
/* 
   Set loop bounds to search for good velocities 
*/
   end   = ( (gate+gates_for) >= MAX_GATES) ? MAX_GATES-1 : (gate+gates_for);
   start = ( (gate-gates_back) > 0) ? (gate-gates_back) : 0;

/*
   Find the averages along the radials on either side of the 
   gate in question. Note - if this is the first or last radial, the
   radial numbers must be set correctly
*/
   tot_prf1_rads = save.tot_prf1_rads;
   rad_offset = save.rad_offset; 

   if(rad == 0)
      rad_prev = tot_prf1_rads - (rad_offset + 1); 
   else
      rad_prev = rad - 1;

   if(rad == tot_prf1_rads - 1)
      rad_for = rad_offset;
   else
      rad_for  = rad + 1;

/* 
   Reset the counters and the variables that hold the sums 
*/
   num_avg1 = 0;
   num_avg2 = 0;
   num_avg3 = 0;
   num_avg4 = 0;
   pra = fra = sra1 = sra2 = 0;

   for(i=start; i<=end; ++i){
      if(save.final_vel[rad_prev][i] < DATA_THR){

         pra += save.final_vel[rad_prev][i];
         ++num_avg1;

       }

      if(save.final_vel[rad_for][i] < DATA_THR){

         fra += save.final_vel[rad_for][i];
         ++num_avg2;

      }

      /*
         Find the average of the gates on the same radial prior
         to the gate in question
      */
      if( save.final_vel[rad][i] < DATA_THR ){

         if( i < gate ){

            sra1 += save.final_vel[rad][i];
            ++num_avg3;

         }

     /*
        Find the average of the gates on the same radial beyond the
        gate in question. Note - if the gate in question is the last
        gate on the radial, break out of this loop
     */
         else if( i > gate ){

            sra2 += save.final_vel[rad][i];
            ++num_avg4;

         }

      }

   }

/*
   Set the averages if they exist and set the flags for each average
*/
   loc_ok =  FALSE;

   if( num_avg1 > 0 ){

      loc_ok |= PRE_RAD_AVG_MASK;
      avg[PRE_RAD_AVG] = pra/num_avg1;

   }

   if( num_avg2 > 0 ){

      loc_ok |= FOR_RAD_AVG_MASK;
      avg[FOR_RAD_AVG] = fra/num_avg2;

   }


   if( num_avg3 > 0  ){

      loc_ok |= SAME_RAD_AVG1_MASK;
      avg[SAME_RAD_AVG1] = sra1/num_avg3;

   }

   if( num_avg4 > 0 ){

      loc_ok |= SAME_RAD_AVG2_MASK;
      avg[SAME_RAD_AVG2] = sra2/num_avg4;

   }

   *ok = loc_ok;

 }
