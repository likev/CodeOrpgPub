/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:18 $
 * $Id: pairs_and_trips_attempts.c,v 1.2 2003/07/17 15:08:18 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   pairs_and_trips_attempts.c

   PURPOSE:

   This routine attempts to dealias gates at which pairs are present. It
   also attempts solutions from pairs with the triplets that no solution
   has been found.

   CALLED FROM:

   process_mpda_scans

   INPUTS:

   int direction - process the radial either away (FORWARD) or
                   towards (BACKWARD) the radar
   
   int type      - either PAIRS or TRIPLETS. Tells how many 
                   velocities must be present for processing
   CALLS:

   get_ewt_value
   get_seed_value
   get_azm_avgs
   initialize_rad_direction
   initialize_unf_seeds
   get_lookup_table_value
   check_unf_diffs

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   The basis of this routine is to use a lookup table built from the 
   Nyquist velocities to find a solution to each gate that a pair of
   velocities exists. In the get_table_lookup_value routine two 
   Nyquists are passed in with the two velocities in question. A
   dealiased value from the lookup table is returned.
 
   HISTORY:

   B. Conway, 10/00     - cleanup
   B. Conway, 5/98      - original development

****************************************************************************/

#include <basedata.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"

float get_ewt_value(float rng, int azindx);

void
get_seed_value (int az, int bin, int start, int end, short *seed, 
                int *status);

void
get_azm_avgs(int az, int gate, short *avg, int *ok);
void
initialize_rad_direction(int direction, int *j, int *inc, int *finish, int rad_num);

void
initialize_unf_seeds(short *gt_seed, short *ewt_seed, short *fin_vel,
                     int *ok_gt_seed, int *ok_ewt_seed);

short
get_lookup_table_value(short nyq1, short nyq2, short pos1, short pos2);

short
check_unf_diffs(short gt_seed, short *avg, short ewt_seed, int ok_gt_seed, 
                int ok, int ok_ewt_seed, short fin_vel, short thres);

Base_data_header rpg_hdr;

void pairs_and_trips_attempts(int direction)
{
   int   i, j, inc, finish;
   int   have_vel1, have_vel2, have_vel3 ;
   int   start,end;
   int   ok_gt_seed, ok_ewt_seed, ok, tot_ok;
   int   j_init, j_inc, j_finish, tot_prf1_rads;
 
   short fin_vel, thres;
   short prf1_nyq, prf2_nyq, prf3_nyq, gt_seed, ewt_seed, avg[4];
   short vel1, vel2, vel3;

/*
   Make local copy of global variables
*/
   tot_prf1_rads = save.tot_prf1_rads;

/*
   Process each gate along each radial. This routine only processes gates
   at which two velocities are present.
*/

   for(i=0; i<tot_prf1_rads; ++i)
      {

/*
      Initialize the loop counters
*/

      initialize_rad_direction(direction, &j_init, &j_inc, &j_finish, i);
      j = j_init;
      inc = j_inc; 
      finish = j_finish;

/*
      Go through each gate and attempt dealiased solutions
*/
      while(j != finish)
         {
         if(save.final_vel[i][j] != MISSING)
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

         if(have_vel1 + have_vel2 + have_vel3 < PAIRS)
            {
            j += inc;
            continue;
            }

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
         Get the various avgs around the point in question
*/
         get_azm_avgs(i, j, avg, &ok);

/*
         See if an EWT value at this gate exists
*/

         if(sounding_for_use)
           if((ewt_seed = (short)(get_ewt_value((float)(j * rpg_hdr.dop_bin_size),i)*FLOAT_10 ))<DATA_THR)
               ok_ewt_seed = TRUE; 

         tot_ok = ok + ok_ewt_seed + ok_gt_seed;

/*       
         There are 3 if blocks below. Each do the same thing except with
         different velocities. Each test checks to see if the velocites
         exist, then gets a dealiased value from the lookup tables, sets
         the comparison threshold to the lower Nyquist velocity, and checks
         to see if the dealiased solution falls within the threshold 
         compared to the various seeds. If so, the final value is set,
         otherwise it is set to missing for further processing.
*/

/*
         Check the first two pair combinations
*/

         if(have_vel1 && have_vel2){

            prf1_nyq = save.prf1_nyq;
            prf2_nyq = save.prf2_nyq;

            if((fin_vel = get_lookup_table_value(save.prf1_number, save.prf2_number,
                                             (vel1+prf1_nyq)/TABLE_RES, (vel2+prf2_nyq)/TABLE_RES))<DATA_THR)
              {

              if(tot_ok > 0 || sounding_for_use == TRUE)
                {
                if(prf1_nyq > prf2_nyq)
                   thres = seed_unf_prf2;
                else
                   thres = seed_unf_prf1;

                fin_vel = check_unf_diffs(gt_seed, avg, ewt_seed, ok_gt_seed, 
                                          ok, ok_ewt_seed, fin_vel, thres);
                }

              if(fin_vel < DATA_THR){

                save.final_vel[i][j] = fin_vel;

/* Save the spectrum width corresponding to the PRF with the highest nyquist */

                if(prf1_nyq > prf2_nyq)
                  save.final_sw[i][j] = save.sw1[i][j];
                else
                  save.final_sw[i][j] = save.sw2[i][j];
               j += inc;
               continue;

              }
            }

         }

/*
         Check the next two pair combinations
*/

         if(have_vel1 && have_vel3){

            prf1_nyq = save.prf1_nyq; 
            prf3_nyq = save.prf3_nyq; 
            if((fin_vel = get_lookup_table_value(save.prf1_number, save.prf3_number,
                                             (vel1+prf1_nyq)/TABLE_RES, (vel3+prf3_nyq)/TABLE_RES))<DATA_THR)
              {
              if(tot_ok > 0 || sounding_for_use == TRUE)
                {
                if(prf1_nyq > prf3_nyq)
                   thres = seed_unf_prf3;
                else
                   thres = seed_unf_prf1;

                fin_vel = check_unf_diffs(gt_seed, avg, ewt_seed, ok_gt_seed, 
                                          ok, ok_ewt_seed, fin_vel, thres);
                }

              if(fin_vel < DATA_THR){
                save.final_vel[i][j] = fin_vel;

/* Save the spectrum width corresponding to the PRF with the highest nyquist */

                if(prf1_nyq > prf3_nyq)
                  save.final_sw[i][j] = save.sw1[i][j];
                else
                  save.final_sw[i][j] = save.sw3[i][j];
               j += inc;
               continue;
             }

           }
         }



/*
         Check the final two pair combinations
*/
         if(have_vel2 && have_vel3){

            prf2_nyq = save.prf2_nyq; 
            prf3_nyq = save.prf3_nyq; 
            if((fin_vel = get_lookup_table_value(save.prf2_number, save.prf3_number,
                                             (vel2+prf2_nyq)/TABLE_RES, (vel3+prf3_nyq)/TABLE_RES))<DATA_THR)
              {
              if(tot_ok > 0 || sounding_for_use == TRUE)
                {
                if(prf2_nyq > prf3_nyq)
                   thres = seed_unf_prf3;
                else
                   thres = seed_unf_prf2;

                fin_vel = check_unf_diffs(gt_seed, avg, ewt_seed, ok_gt_seed, 
                                          ok, ok_ewt_seed, fin_vel, thres);
                }

              if(fin_vel < DATA_THR){

                save.final_vel[i][j] = fin_vel;

/* Save the spectrum width corresponding to the PRF with the highest nyquist */

                if(prf2_nyq > prf3_nyq)
                  save.final_sw[i][j] = save.sw2[i][j];
                else
                  save.final_sw[i][j] = save.sw3[i][j];
               j += inc;
               continue;

               }
             }
         }
 
         j += inc;
      }

   }
}
