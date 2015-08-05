/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:27 $
 * $Id: second_triplet_attempt.c,v 1.2 2003/07/17 15:08:27 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   second_triplet_attempt.c

   PURPOSE:

   This routine attempts to derive solutions for gates at which three
   velocities exist using seeds and a relaxed threshold.

   CALLED FROM:

   process_mpda_scans

   INPUTS:

   int direction - process the radial either away (FORWARD) or
                   towards (BACKWARD) the radar

   CALLS:

   get_azm_avgs
   get_ewt_value
   get_seed_value
   initialize_unf_seeds
   initialize_rad_direction
   multi_prf_unf

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:
 
   The basis of this routine is to take the three velocities and check
   if they are close enough to each other using seeds and averages 
   within a looser threhold than the first check. If they are within
   the thresholds, they are assigned a dealiased value.
 
   The routine multi_prf_unf is passed the following parameters
   relavtive to the positions in the call:
 
   seed     - seed value to use in checking for dealiasing 
   average  - average to use in finding solution
   vel1     - velocity from scan 1
   nyq1     - Nyquist from scan 1
   vel2     - velocity from scan 2
   nyq2     - Nyquist from scan 2
   vel3     - velocity from scan 3
   nyq3     - Nyquist from scan 3
   add_seed - Use the seed value in the solution?
   num_hits - minimum number of prfs that must match each other within   
              the th_gates threshold 
   The difference between this routine and the first_triplet_attempt
   routine is that in this one the seed value is passed to the unfold
   routine along with a relaxed unfolding threshold. 

   HISTORY:

   B. Conway, 9/00      - cleanup
   B. Conway, 5/96      - original development

*************************************************************************/

#include <basedata.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"

void
get_azm_avgs(int az, int gate, short *avg, int *ok);

float
get_ewt_value(float rng, int azindx);

void
get_seed_value (int az, int bin, int start, int end, short *seed,
                int *status);

void
initialize_unf_seeds(short *gt_seed, short *ewt_seed, short *fin_vel,
                     int *ok_gt_seed, int *ok_ewt_seed);

void
initialize_rad_direction(int direction, int *j, int *inc, int *finish, int rad_num);

short
multi_prf_unf(short seed, short vel1, short vel2, short vel3,
              int add_seed, int num_hits, short th_gates);

Base_data_header rpg_hdr;

void second_triplet_attempt(int direction)
{
   int   i, j, start, end, inc, finish;
   int   have_vel1, have_vel2, have_vel3;
   int   ok_gt_seed, ok_ewt_seed, ok, loc_ok;
   int   tot_prf1_rads, j_init, j_inc, j_finish;
 
   short gt_seed, ewt_seed, fin_vel, max_diff, avg[4];
   short vel1, vel2, vel3;
 
/*
  Make local copy of global variables
*/

   tot_prf1_rads = save.tot_prf1_rads;
   
/*
   The following main loop processes all the radials of the
   3 scans. There must be 3 velocities present at each gate
   (the while loop below) before a final assignment is
   attempted.
*/
 
   max_diff = TH_OVERLAP_SCALE * th_overlap_relax;
   for(i=0; i < tot_prf1_rads; ++i)
      {  
/*
      Assign the start, finish and gate increment based on the
      required direction
*/
 
      initialize_rad_direction(direction, &j_init, &j_inc, &j_finish, i);
      j = j_init;
      inc = j_inc;
      finish = j_finish;

      while(j != finish)
         {

         if(save.final_vel[i][j] == MISSING)
           {

/*
  Check to see if the velocities are valid and set the existence
  flags
*/

           have_vel1 = ((vel1 = save.vel1[i][j]) < DATA_THR) ? TRUE : FALSE;
           have_vel2 = ((vel2 = save.vel2[i][j]) < DATA_THR) ? TRUE : FALSE;
           have_vel3 = ((vel3 = save.vel3[i][j]) < DATA_THR) ? TRUE : FALSE;

/*
           If there are less than 3 vels just go to the next gate.
*/
 
           if(have_vel1 + have_vel2 + have_vel3 == TRIPLETS )
             {
 
/*
         Initialize all seeds to MISSING, and the checks to FALSE
*/

             initialize_unf_seeds(&gt_seed, &ewt_seed, &fin_vel, &ok_gt_seed,
                                  &ok_ewt_seed);
 
/*
         Set distance along radial to look for a seed gate and try
         to find a seed
*/
 
             end   = ( (j+th_seed_chk) < save.prf1_n_dop_bins) ? (j+th_seed_chk) 
                                       : save.prf1_n_dop_bins-1;
             start = ( (j-th_seed_chk) > 0) ? (j-th_seed_chk) : 0;
 
             get_seed_value(i,j,start,end,&gt_seed,&ok_gt_seed);
 

/*       
       First try the closest single gate found along the same radial as a seed
*/

             if (ok_gt_seed)  
                if((fin_vel = multi_prf_unf(gt_seed, vel1, vel2, vel3,
                                            TRUE, TRIPLETS+1, max_diff))< DATA_THR)
                  {
                  save.final_vel[i][j] = fin_vel;

/* Store the spectrum width from PRF1 in the final SPW array since the
   velocity estimate from PRF1 is what's returned from multi_prf_unf */

                  save.final_sw[i][j] = save.sw1[i][j];
                  j += inc;
                  continue;
                  }

/*
         Get the various avgs around the point in question
*/

             get_azm_avgs(i, j, avg, &ok);
 
/*
         Next try the previous radial average
*/
             loc_ok = ok;
             if (loc_ok & PRE_RAD_AVG_MASK)  
                if((fin_vel = multi_prf_unf(avg[PRE_RAD_AVG], vel1, vel2, vel3,
                                            TRUE, TRIPLETS+1, max_diff) ) < DATA_THR)
                  {
                  save.final_vel[i][j] = fin_vel;

/* Store the spectrum width from PRF1 in the final SPW array since the
   velocity estimate from PRF1 is what's returned from multi_prf_unf */

                  save.final_sw[i][j] = save.sw1[i][j];
                  j += inc;
                  continue;
                  }
/*
         Next try the forward radial average
*/

             if (loc_ok & FOR_RAD_AVG_MASK)  
                if((fin_vel = multi_prf_unf(avg[FOR_RAD_AVG], vel1, vel2, vel3,
                                            TRUE, TRIPLETS+1, max_diff) ) < DATA_THR)
                  {
                  save.final_vel[i][j] = fin_vel;

/* Store the spectrum width from PRF1 in the final SPW array since the
   velocity estimate from PRF1 is what's returned from multi_prf_unf */

                  save.final_sw[i][j] = save.sw1[i][j];
                  j += inc;
                  continue;
                  }
/*
         Next try the average of gates along the same radial prior to the
         gate in question
*/

             if (loc_ok & SAME_RAD_AVG1_MASK)  
                if((fin_vel = multi_prf_unf(avg[SAME_RAD_AVG1], vel1,
                                            vel2, vel3, TRUE, TRIPLETS+1, max_diff) )
                                            < DATA_THR)
                  {
                  save.final_vel[i][j] = fin_vel;

/* Store the spectrum width from PRF1 in the final SPW array since the
   velocity estimate from PRF1 is what's returned from multi_prf_unf */

                  save.final_sw[i][j] = save.sw1[i][j];
                  j += inc;
                  continue;
                  }

/*
         Next try the average of gates along the same radial prior to the
         gate in question
*/

             if (loc_ok & SAME_RAD_AVG2_MASK)  
                if((fin_vel = multi_prf_unf(avg[SAME_RAD_AVG2], vel1, vel2, vel3,
                                            TRUE,TRIPLETS+1,max_diff) ) < DATA_THR)
                  {
                  save.final_vel[i][j] = fin_vel;

/* Store the spectrum width from PRF1 in the final SPW array since the
   velocity estimate from PRF1 is what's returned from multi_prf_unf */

                  save.final_sw[i][j] = save.sw1[i][j];
                  j += inc;
                  continue;
                  }

/*
         Get the EWT value for this gate
*/

             if(sounding_for_use)
               if((ewt_seed = (short)(get_ewt_value((float)(j * rpg_hdr.dop_bin_size),i)*FLOAT_10 ))<DATA_THR)

/*
         Finally, if a solution still has not been found, try finding one
         using the EWT 
*/

                  if((fin_vel = multi_prf_unf(ewt_seed, vel1, vel2, vel3,
                                              TRUE, TRIPLETS+1, max_diff) ) < DATA_THR)
                    {
                    save.final_vel[i][j] = fin_vel;

/* Store the spectrum width from PRF1 in the final SPW array since the
   velocity estimate from PRF1 is what's returned from multi_prf_unf */

                    save.final_sw[i][j] = save.sw1[i][j];
                    j += inc;
                    continue;
                    }

             }

           }

/*
         If it makes it this far, no solution was found, change the counter
         and process the next gate.
*/

         j += inc;           
         }
      }
}
