/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:15 $
 * $Id: multi_prf_unf.c,v 1.2 2003/07/17 15:08:15 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   multi_prf_unf.c

   PURPOSE:

   This routine attempts to find dealiased solution from gates that have
   three velocit estimates present.

   CALLED FROM:

   first_triplet_attempt
   second_triplet_attempt

   INPUTS:

   short seed        - seed value to use in dealiasing (m/s*10)
   short avg         - avg value to use in dealiasing (m/s*10)
   short vel1        - velocity from first prf  (m/s*10)
   short vel2        - velocity from second prf  (m/s*10)
   short vel3        - velocity from third prf  (m/s*10)
   int   add_seed    - flag whether to use seed in dealiasing
   int   num_hits    - threshold that velocites must fall within to be counted
   int   th_overlap  - threshold that velocites must overlap within
   int   unf_attempt - number of attempts at dealiasing the given scan
   

   CALLS:

   None.
   
   OUTPUTS:
   
   None.

   RETURNS:

   MISSING if no solution was found, dealiased velocity otherwise

   HISTORY:

   R. May    02/05/02   - Increased Efficiency
   D. Zittel 01/23/01   - corrected logic 
   B. Conway, 9/00      - cleanup 
   B. Conway, 5/96      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"

#define   LOW_INT -MAX_NYQ_INTV
#define   HIGH_INT MAX_NYQ_INTV
#define   MAX_SIZE (HIGH_INT - LOW_INT + 1) * TRIPLETS + 1

short
multi_prf_unf(short seed, short vel1,  short vel2, short vel3, 
          int add_seed, int num_hits, short max_diff)
{
    int   i, j, k, vel_hits[MAX_SIZE], vel_prf[MAX_SIZE];
    int   tmp_vel, tmp_prf, min_loc;           
    short prf1_twice_nyq, prf2_twice_nyq, prf3_twice_nyq, hits_cnt;

    if(add_seed == FALSE){

       if( (abs(vel1-vel2) <= max_diff) && (abs(vel2-vel3) <= max_diff) && 
           (abs(vel1-vel3) <= max_diff) )
          return vel1;

    }

/* 
    Calculate all values of velocities between -100 m/s and 100 m/s
*/

    hits_cnt = 0;

    prf1_twice_nyq = save.prf1_twice_nyq;
    prf2_twice_nyq = save.prf2_twice_nyq;
    prf3_twice_nyq = save.prf3_twice_nyq;

    vel1 += prf1_twice_nyq*LOW_INT;
    vel2 += prf2_twice_nyq*LOW_INT;
    vel3 += prf3_twice_nyq*LOW_INT;

    for( j = LOW_INT; j <= HIGH_INT; j++ ){

       if (abs(vel1) < MAX_INT_VEL){

          vel_hits[hits_cnt] = vel1;
          vel_prf[hits_cnt] = 1;
          hits_cnt++;

       }
       vel1 += prf1_twice_nyq;

       if (abs(vel2) < MAX_INT_VEL){

          vel_hits[hits_cnt] = vel2;
          vel_prf[hits_cnt] = 2;
          hits_cnt++;

       }
       vel2 += prf2_twice_nyq;

       if (abs(vel3) < MAX_INT_VEL){

          vel_hits[hits_cnt] = vel3;
          vel_prf[hits_cnt] = 3;
          hits_cnt++;

       }
       vel3 += prf3_twice_nyq;

    }

/* 
    Check to see if we are using a seed 
*/

    if (add_seed){

       vel_hits[hits_cnt] = seed;
       vel_prf[hits_cnt] = 4;

    } 
    else
       hits_cnt--;

/* Sort array of possible velocities */

    for(i=0;i<hits_cnt;++i){

       min_loc = i;
       for(j=i+1;j<=hits_cnt;++j){

          if(vel_hits[j]<vel_hits[min_loc])
             min_loc = j;

       }

       tmp_vel = vel_hits[min_loc];
       tmp_prf = vel_prf[min_loc];
       vel_hits[min_loc] = vel_hits[i];
       vel_prf[min_loc] = vel_prf[i];
       vel_hits[i] = tmp_vel;
       vel_prf[i] = tmp_prf;

    }

/* Search for 3 (4 for second triplet) values within the given threshold */

    for(i = 0, k = num_hits-1; i < hits_cnt - num_hits + 2; ++i, ++k ){

       if (vel_hits[k] - vel_hits[i] <= max_diff){

/* Return the velocity corresponding to highest PRF */

          for(j=i; j<=k; ++j){

             if (vel_prf[j] == vel_max_nyq)
                return vel_hits[j];

          }        
       }
    }

/* No 3 (or 4 for second triplet) values within threshold, return MISSING */

    return MISSING;
}
