/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 14:49:01 $
 * $Id: check_unf_diffs.c,v 1.2 2003/07/17 14:49:01 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   check_unf_diffs.c

   PURPOSE:

   This routine checks a dealiased velocity to see if it is within the 
   seed values.

   CALLED FROM:

   first_triplet_attempt
   pairs_and_trips_attempts

   INPUTS:

   short   gt_seed          - closest gate on same radial as gate 
                              in question (m/s*10) 
   short   *avg             - array of avgs for gate in question (m/s*10)
   short   ewt_seed         - ewt seed for checks (m/s*10)
   int     ok_gt_seed       - existance flag for gt_seed
   int     ok               - existance bit map for averages
   int     ok_ewt_seed      - existance flag for ewt_seed
   short   fin_vel          - dealiased value (m/s*10)
   short   thres            - dealiasing threshold (m/s*10)

   CALLS:
 
   None. 

   OUTPUTS:

   None.

   RETURNS:

   short fin_vel  - velocity that falls within the threhold of one of the
                    seeds, or MISSING
   NOTES:

   1) This routine just checks each seed, starting with the closest gate
      on the same radial and runs down the averages seeing if the dealiased
      solution falls within the given threshold of any of these seed values.

   HISTORY:

   B. Conway, 9/00      - original development

**************************************************************************/
#include "mpda_constants.h"

short
check_unf_diffs(short gt_seed, short *avg, short ewt_seed, int ok_gt_seed, 
                int ok, int ok_ewt_seed, short fin_vel, short thres)
{

/*
   Check each seed against the dealiased velocity. If one is within the
   dealiasing threshold, return
*/

   if(ok_gt_seed && (abs(fin_vel-gt_seed) <= thres) )
     return fin_vel;

   if( (ok & PRE_RAD_AVG_MASK) && (abs(fin_vel-avg[PRE_RAD_AVG]) <= thres) )
     return fin_vel;

   if( (ok & FOR_RAD_AVG_MASK) && (abs(fin_vel-avg[FOR_RAD_AVG]) <= thres) )
     return fin_vel;

   if( (ok & SAME_RAD_AVG1_MASK) && (abs(fin_vel-avg[SAME_RAD_AVG1]) <= thres) )
     return fin_vel;

   if( (ok & SAME_RAD_AVG2_MASK) && (abs(fin_vel-avg[SAME_RAD_AVG2]) <= thres) )
     return fin_vel;

   if(ok_ewt_seed && (abs(fin_vel-ewt_seed) <= thres) )
     return fin_vel;

/*
   Nothing worked... return MISSING
*/

   return MISSING;
}
