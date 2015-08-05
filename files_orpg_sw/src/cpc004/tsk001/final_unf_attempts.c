/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:43 $
 * $Id: final_unf_attempts.c,v 1.2 2003/07/17 15:07:43 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   final_unf_attempts.c

   PURPOSE:

   This routine attempts to dealias gates that a solution has not been
   found for by this point. 

   CALLED FROM:

   process_mpda_scans

   INPUTS:

   int direction - process the radial either away (FORWARD) or
                   towards (BACKWARD) the radar
   int use_ewt   - Flag to use the ewt or not for dealiasing 

   CALLS:

   get_ewt_value
   get_avg_value
   get_azm_avgs
   get_seed_value
   initialize_unf_seeds
   initialize_rad_direction
    
   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   This routine is similar to the triplet and pair routines excepts that
   it processes every gate regardless of the number of velocites present.

   HISTORY:

   B. Conway, 10/00     - cleanup
   B. Conway, 5/98      - original development

****************************************************************************/

#include <basedata.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"

short 
check_seed_unf(short seed, short vel1, short vel2, short vel3, short thres,
               short sw1,  short sw2,  short sw3,  short *fin_sw);

float 
get_ewt_value (float rng, int azindx);

short 
get_avg_value (int rad, int gate);

void
get_azm_avgs(int az, int gate, short *avg, int *ok);

void
get_seed_value (int az, int bin, int start, int end, short *seed, 
                int *status);

void
initialize_rad_direction(int direction, int *j, int *inc, int *finish, int rad_num);

Base_data_header rpg_hdr;

void final_unf_attempts(int direction, int use_ewt)
{
   int   i, j, start, end, finish, inc;
   int   have_vel1, have_vel2, have_vel3;
   int   ok_gt_seed, ok, loc_ok;

   int   tot_prf1_rads, j_init, j_inc, j_finish;

   short gt_seed, ewt_seed, fin_vel, thres, avg[4];
   short vel1, vel2, vel3;
   short sw1, sw2, sw3, fin_sw;

/*
   Make local copies of global variables
*/
   tot_prf1_rads = save.tot_prf1_rads;

/* 
   Loop through each gate and each radial for a final radial continuity attempt 
*/
 
   for(i = 0; i < tot_prf1_rads; ++i)
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
/*
         Check each gate. If there is at least 1 velocity present
         and a final velocity has not been assigned, process that
         gate.
*/
         if (save.final_vel[i][j] != MISSING) 
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

         if( have_vel1 + have_vel2 + have_vel3 == FALSE )
           {
           j += inc;
           continue;
           }

/*       
         Find a dealiasing threshold to use. Get the smallest Nyquist
         velocity available.
*/

         fin_vel = MISSING;
         thres = MAX_INT_VEL;
         if(have_vel1) thres = (seed_unf_prf1 < thres) ? seed_unf_prf1 : thres;
         if(have_vel2) thres = (seed_unf_prf2 < thres) ? seed_unf_prf2 : thres;
         if(have_vel3) thres = (seed_unf_prf3 < thres) ? seed_unf_prf3 : thres;
         
         sw1 = save.sw1[i][j];
         sw2 = save.sw2[i][j];
         sw3 = save.sw3[i][j];

/*       
         Get the loop bounds along the radial
*/
         end   = ( (j+th_seed_chk) < save.prf1_n_dop_bins) ? (j+th_seed_chk)
                                       : save.prf1_n_dop_bins-1;
         start = ( (j-th_seed_chk) > 0) ? (j-th_seed_chk) : 0;

/*       
         Get the gate closest to the gate in question on the same
         radial
*/

         gt_seed = MISSING;
         ok_gt_seed = FALSE;
         get_seed_value(i,j,start,end,&gt_seed,&ok_gt_seed);
 
/*
        Using each seed try to dealias the velocities in question. The check_seed_unf
        routine uses whatever velocities are present and attempts to dealias the 
        given gate within thres of the gates. In the if tests below, the closest
        gate on the same radial is checked first, followed by each of the averages,
        and finally the ewt.
*/

        if(ok_gt_seed)
          {
          if((fin_vel = check_seed_unf(gt_seed, vel1, vel2, vel3, thres,
                                       sw1, sw2, sw3, &fin_sw))<DATA_THR)
            {
            save.final_vel[i][j] = fin_vel;
            save.final_sw[i][j] = fin_sw;  
            j += inc;
            continue;
            }
          }
          
/*
         Get the various avgs around the point in question
*/

        get_azm_avgs(i, j, avg, &ok );

        loc_ok = ok;
        if(loc_ok & PRE_RAD_AVG_MASK)
          {
          if((fin_vel = check_seed_unf(avg[PRE_RAD_AVG], vel1, vel2, vel3,
                                       thres, sw1, sw2, sw3, &fin_sw)) < DATA_THR)
            {
            save.final_vel[i][j] = fin_vel;
            save.final_sw[i][j] = fin_sw;  
            j += inc;
            continue;
            }
          }

        if(loc_ok & FOR_RAD_AVG_MASK)
          {
          if((fin_vel = check_seed_unf(avg[FOR_RAD_AVG], vel1, vel2, vel3,
                                       thres, sw1, sw2, sw3, &fin_sw)) < DATA_THR)
            {
            save.final_vel[i][j] = fin_vel;
            save.final_sw[i][j] = fin_sw;  
            j += inc;
            continue;
            }
          }

        if(loc_ok & SAME_RAD_AVG1_MASK)
          {
          if((fin_vel = check_seed_unf(avg[SAME_RAD_AVG1], vel1, vel2, vel3,
                                       thres, sw1, sw2, sw3, &fin_sw)) < DATA_THR)
            {
            save.final_vel[i][j] = fin_vel;
            save.final_sw[i][j] = fin_sw;  
            j += inc;
            continue;
            }
          }

        if(loc_ok & SAME_RAD_AVG2_MASK)
          {
          if((fin_vel = check_seed_unf(avg[SAME_RAD_AVG2], vel1, vel2, vel3,
                                       thres, sw1, sw2, sw3, &fin_sw)) < DATA_THR)
            {
            save.final_vel[i][j] = fin_vel;
            save.final_sw[i][j] = fin_sw;  
            j += inc;
            continue;
            }
          }
  
/*
        If the ewt flag is set, get an ewt value
*/

        if(use_ewt)
          if(sounding_for_use)
            if((ewt_seed = (short)(get_ewt_value((float)(j * rpg_hdr.dop_bin_size),i)*FLOAT_10 ))<DATA_THR)
              if((fin_vel = check_seed_unf(ewt_seed, vel1, vel2, vel3,
                                           thres, sw1, sw2, sw3, &fin_sw)) < DATA_THR)
                {
                save.final_vel[i][j] = fin_vel;
                save.final_sw[i][j] = fin_sw;  
                j += inc;
                continue;
                }

   j += inc;
   }
 }

}
