/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:22 $
 * $Id: replace_orig_vals.c,v 1.2 2003/07/17 15:08:22 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   replace_orig_vals.c

   PURPOSE:

   This routine replaces the original velocity values at gates that no
   solution has been found for.

   CALLED FROM:

   process_mpda_scans

   INPUTS:

   None.

   CALLS:

   get_azm_avgs
   initialize_unf_seeds
   short check_seed_unf

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   This routine replaces the original velocity with the one closest to the
   ewt or the averages of all the gate around it. In this manner it is hoped
   that any errors will be minimized.  Otherwise, the bin with the highest
   nyquist is used as a last resort.

   HISTORY:

   R. May, 5/03         - more cleanup, implementation of UDF findings
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

short check_seed_unf(short seed, short vel1, short vel2, short vel3, short thres,
                     short sw1, short sw2, short sw3, short *fin_sw);

Base_data_header rpg_hdr;

void replace_orig_vals()
{
   int i, j, jtemp;
   int have_vel1, have_vel2,have_vel3;
   int ok, loc_ok;

   short tmp_nyq, tot_avg, num_avg, thres, diff;
   short vel1, vel2, vel3, fin_vel, ewt_seed, avg[4];
   short sw1, sw2, sw3, fin_sw;

/*
   Check each radial and each gate
*/

   for(i=0; i<save.tot_prf1_rads; ++i)
      {
      jtemp = save.vel_limit[i];
      for(j=0; j < jtemp; ++j)
         {
/*
         If a solution already exits or no velocities are present
         continue to the next gate
*/
         if(save.final_vel[i][j] != MISSING )
           continue;

/*
  Check to see if the velocities are valid and set the existence
  flags
*/

         have_vel1 = ((vel1 = save.vel1[i][j]) < DATA_THR) ? TRUE : FALSE;
         have_vel2 = ((vel2 = save.vel2[i][j]) < DATA_THR) ? TRUE : FALSE;
         have_vel3 = ((vel3 = save.vel3[i][j]) < DATA_THR) ? TRUE : FALSE;

         if(have_vel1 + have_vel2 + have_vel3 == FALSE)
            continue;

/*
          Use the lowest Nyquist velocity as the threshold for
          dealiasing
*/

         thres = MAX_INT_VEL;
         if(have_vel1) thres = (save.prf1_nyq < thres) ?
                                save.prf1_nyq : thres;
         if(have_vel2) thres = (save.prf2_nyq < thres) ?
                                save.prf2_nyq : thres;
         if(have_vel3) thres = (save.prf3_nyq < thres) ?
                                save.prf3_nyq : thres;

         sw1 = save.sw1[i][j];
         sw2 = save.sw2[i][j];
         sw3 = save.sw3[i][j];

         fin_vel = MISSING;

/*
         Get the various avgs around the point in question
*/

         get_azm_avgs(i, j, avg, &ok);

/* 
         Find the average of all the averages
*/

         num_avg = 0;
         tot_avg = 0;
         loc_ok = ok;

         if(loc_ok & PRE_RAD_AVG_MASK){

            num_avg ++;
            tot_avg += avg[PRE_RAD_AVG];

         }

         if(loc_ok & FOR_RAD_AVG_MASK){

            num_avg ++;
            tot_avg += avg[FOR_RAD_AVG];

         }
           
         if(loc_ok & SAME_RAD_AVG1_MASK){

            num_avg ++;
            tot_avg += avg[SAME_RAD_AVG1];

         }

         if(loc_ok & SAME_RAD_AVG2_MASK){

            num_avg ++;
            tot_avg += avg[SAME_RAD_AVG2];

         }
/*
       Try to dealias the gate using the averages one last time
*/

       if(num_avg != 0)
         {
         tot_avg /= num_avg;
         fin_vel = check_seed_unf(tot_avg,vel1, vel2, vel3, thres,
                                   sw1, sw2, sw3, &fin_sw);
         if(fin_vel < DATA_THR)
           {
           save.final_vel[i][j] = fin_vel;
           save.final_sw[i][j] = fin_sw;
           continue;
           }

/*
      Dealiasing didn't work - just replace the velocity with the one
      closest to the average
*/

         diff = 2*MAX_INT_VEL;
         if(have_vel1)
           {
             fin_vel = save.vel1[i][j];
             fin_sw = sw1; 
             diff = abs(save.vel1[i][j]-tot_avg);
           }
         if(have_vel2)
           {
           if(abs(save.vel2[i][j]-tot_avg)<diff)
             {
             fin_vel = save.vel2[i][j];
             fin_sw = sw2;
             diff = abs(save.vel2[i][j]-tot_avg);
             }
           }
         if(have_vel3)
           {
           if(abs(save.vel3[i][j]-tot_avg)<diff)
             {
             fin_vel = save.vel3[i][j];
             fin_sw = sw3;
             }
           }

/*
      Found a solution -- assign it and continue
*/

         save.final_vel[i][j] = fin_vel;
         save.final_sw[i][j] = fin_sw;
         continue;
         }

/*
         Try to get an EWT value
*/
         if(sounding_for_use)
           if((ewt_seed = (short)(get_ewt_value((float)(j * rpg_hdr.dop_bin_size),i)*FLOAT_10 ))<DATA_THR)
             if((fin_vel = check_seed_unf(ewt_seed,vel1, vel2, vel3, thres,
                                          sw1, sw2, sw3, &fin_sw)) < DATA_THR)
             {
             save.final_vel[i][j] = fin_vel;
             save.final_sw[i][j] = fin_sw;
             continue;
             }

/*
      No solution found. Just assign the final velocity to that which
      has the highest Nyquist
*/ 
     
    
      tmp_nyq = 0;
      if (have_vel1)
         {
         tmp_nyq = save.prf1_nyq;
         save.final_vel[i][j] = save.vel1[i][j];
         save.final_sw[i][j] = sw1;
         }
 
      if (have_vel2 && (save.prf2_nyq > tmp_nyq))
         {
         tmp_nyq = save.prf2_nyq;
         save.final_vel[i][j] = save.vel2[i][j];
         save.final_sw[i][j] = sw2;
         }
 
      if (have_vel3 && (save.prf3_nyq > tmp_nyq))
         {
         save.final_vel[i][j] = save.vel3[i][j];
         save.final_sw[i][j] = sw3;
         }
      }
   }
}
