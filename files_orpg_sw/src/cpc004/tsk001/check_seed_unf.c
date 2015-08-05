/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 14:49:00 $
 * $Id: check_seed_unf.c,v 1.2 2003/07/17 14:49:00 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   check_seed_unf.c

   PURPOSE:

   This routine attempts to unfold gates to within a threshold value of a
   seed.

   CALLED FROM:

   despeckle_results
   final_unf_attempts
   replace_orig_vels

   INPUTS:

   short   seed   - velocity seed for unfolding (m/s*10)
   short   vel1   - velocity from 1st prf (m/s*10)
   short   vel2   - velocity from 2nd prf (m/s*10)
   short   vel3   - velocity from 3st prf (m/s*10)
   short   thres  - threshold for unfold (m/s*10)
   short   sw1    - spectrum width from 1st prf (UIF)
   short   sw2    - spectrum width from 2nd prf (UIF)
   short   sw3    - spectrum width from 3rd prf (UIF)
   short  *sw_sol - spectrum width value for the returned solution (UIF)

   CALLS:

   unfold_vel

   OUTPUTS:

   None.

   RETURNS:

   short   hldvel - unfolded velocity or MISSING

   NOTES:

   1) If three velocities are present, it is possible to have 3 dealiased
      values coming back from unfold_vel. This routine then returns the 
      dealiased velocity that is closest to the input seed.

   HISTORY:

   D. Zittel, 02/2003   - Implementation phase final cleanup
   B. Conway, 9/00      - cleanup 
   B. Conway, 5/96      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"

short 
unfold_vel(short val, short vel, short nyq, short thres);

short
check_seed_unf(short seed, short vel1, short vel2, short vel3, short thres,
               short sw1,  short sw2,  short sw3,  short *sw_sol)
{

   short hldvel, hld1, hld2, hld3, diff;

   hldvel = MISSING;
   diff   = 2 * MAX_INT_VEL;

/*  
   Unfold each incoming velocity that is not missing
*/

   hld1 = (vel1 < DATA_THR) ? unfold_vel(seed,vel1,save.prf1_twice_nyq,thres) : MISSING;
   hld2 = (vel2 < DATA_THR) ? unfold_vel(seed,vel2,save.prf2_twice_nyq,thres) : MISSING;
   hld3 = (vel3 < DATA_THR) ? unfold_vel(seed,vel3,save.prf3_twice_nyq,thres) : MISSING;

/* 
   Find the unfolded velocity that is closest to the seed and return
   that value
*/

   if(hld1 < MAX_INT_VEL)
     {
     diff = abs(hld1-seed);
     hldvel = hld1;
     *sw_sol = sw1;
     }

   if(hld2 < MAX_INT_VEL && (abs(hld2-seed) < diff) )
     {
     diff = abs(hld2-seed);
     hldvel = hld2;
     *sw_sol = sw2;
     }

   if(hld3 < MAX_INT_VEL && (abs(hld3-seed) < diff) )
     {
     hldvel = hld3;
     *sw_sol = sw3;
     }

   return(hldvel);
}
