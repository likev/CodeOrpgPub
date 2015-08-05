/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:44 $
 * $Id: first_triplet_attempt.c,v 1.2 2003/07/17 15:07:44 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   first_triplet_attempt.c

   PURPOSE:

   This routine attempts to derive solutions for gates at which three
   velocities exist. 

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
   check_unf_diffs

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   The basis of this routine is to take the three velocities and check
   if they are close enough to each other within a tight threshold. If
   they are, they are checked against the various seeds as a sanity
   check and assigned a dealiased value. 

   The routine multi_prf_unf is passed the following parameters 
   relavtive to the positions in the call:
   
   seed     - seed value to use in checking for dealiasing (not used in this
               routine)
   average  - average to use in finding solution (not used in this routine)
   vel1     - velocity from scan 1
   nyq1     - Nyquist from scan 1 
   vel2     - velocity from scan 2
   nyq2     - Nyquist from scan 2 
   vel3     - velocity from scan 3
   nyq3     - Nyquist from scan 3 
   add_seed - Use the seed value in the solution?
   add_avg  - Use the avg value in the solution?
   num_hits - minimum number of prfs that must match each other within
              the th_gates threshold

   HISTORY:

   B. Conway, 9/00      - cleanup 
   B. Conway, 5/96      - original development

****************************************************************************/

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

short
check_unf_diffs(short gt_seed, short *avg, short ewt_seed, int ok_gt_seed, 
                int ok, int ok_ewt_seed, short fin_vel, short thres);

Base_data_header rpg_hdr;

void first_triplet_attempt(int direction)
{
   int   i, j, start, end, inc, finish;
   int   have_vel1, have_vel2, have_vel3, tmp_vel;
   int   ok_gt_seed, ok_ewt_seed, ok;

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

   max_diff = TH_OVERLAP_SCALE * th_overlap_size;

   for(i=0; i<tot_prf1_rads; ++i)
      {
/*
      Assign the start, finish and gate increment based on the
      required direction
*/

      initialize_rad_direction(direction, &j_init, &j_inc, &j_finish, i);

      j = j_init;
      inc = j_inc;
      finish = j_finish;
      while (j != finish)
         {

/*
         Check to see if current solution is missing
*/

         if(save.final_vel[i][j]!= MISSING)
            {
            j += inc;
            continue;
            }

/*
  Check to see if the velocities are valid and set the existence
  flags
*/

         have_vel1 = ((vel1 = save.vel1[i][j]) < DATA_THR) ? TRUE : FALSE;
         have_vel2 = ((vel2 = save.vel2[i][j]) < DATA_THR) ? TRUE : FALSE;
         have_vel3 = ((vel3 = save.vel3[i][j]) < DATA_THR) ? TRUE : FALSE;

/*
         If there are less than 3 vels, or a final value has already
         been assigned, just go to the next gate.
*/

         if(have_vel1 + have_vel2 + have_vel3 < TRIPLETS) 
           {
           j += inc;
           continue;
           }
/*
         Try to get a final solution
*/ 
         fin_vel = multi_prf_unf(RNG_FLD, vel1, vel2, vel3, FALSE, TRIPLETS, max_diff);

         if(fin_vel >= DATA_THR)
           {
           j += inc;
           continue;
           }
         tmp_vel = fin_vel;
         fin_vel = MISSING;

/*
         Try to get an EWT value

         Check to see if the value found falls within the threshold amount
         of the EWT estimate.
*/

         if(sounding_for_use)
           if((ewt_seed = (short)(get_ewt_value((float)(j * rpg_hdr.dop_bin_size)
                                  ,i)*FLOAT_10 ))<DATA_THR)
             if(abs(tmp_vel - ewt_seed) <= th_fst_tplt_chk)
               fin_vel = tmp_vel;

         if(fin_vel > DATA_THR)
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
         end   = ( (j+th_seed_chk) <save.prf1_n_dop_bins) ? (j+th_seed_chk)
                                   : save.prf1_n_dop_bins-1;
         start = ( (j-th_seed_chk) > 0) ? (j-th_seed_chk) : 0;
 
         get_seed_value(i,j,start,end,&gt_seed,&ok_gt_seed);  
/*
         Get the various avgs around the point in question
*/
         get_azm_avgs(i, j, avg, &ok);   

         if((ok_gt_seed + ok + sounding_for_use) == FALSE)
           fin_vel = tmp_vel;
         else
           fin_vel = check_unf_diffs(gt_seed, avg, MISSING, ok_gt_seed,
                                     ok, FALSE, tmp_vel, th_fst_tplt_chk);
                  
         }         

         if(fin_vel < DATA_THR)
           {    
           save.final_vel[i][j] = fin_vel;

/* Store the spectrum width from PRF1 in the final SPW array since the
   velocity estimate from PRF1 is what's returned from multi_prf_unf */

           save.final_sw[i][j] = save.sw1[i][j];
           }
          j += inc;
         }
      }
}
