/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:05 $
 * $Id: initialize_unf_seeds.c,v 1.2 2003/07/17 15:08:05 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************
 
   initialize_unf_seeds.c
 
   PURPOSE:
 
   This routine intializes the seed values and their TRUE/FALSE existence
   flags to be used in unfolding.
  
   CALLED FROM:
 
   first_triplet_attempt
   second_triplet_attempt
   pairs_and_trips_attempts
   final_unf_attempts
   replace_orig_vals
   despeckle_results

   INPUTS:
 
   short *gt_seed        - closest gate along given radial 
   short *ewt_seed       - ewt value being used as a seed
   short *fin_vel        - final velocity value assigned to the gate in question

   int   *ok_gt_seed       - existence flag for gt_seed
   int   *ok_ewt_seed      - existence flag for ewt_seed

   CALLS:

   None.

   OUTPUTS:
 
   None. 
 
   RETURNS:

   None. 
 
   HISTORY:
 
   R. May, 2/03         - For improvement of get_azm_avgs, move initialization
                          of average variables and flags
   B. Conway, 9/00      - original development

****************************************************************************/

#include "mpda_constants.h"

void
initialize_unf_seeds(short *gt_seed, short *ewt_seed, short *fin_vel,
                     int *ok_gt_seed, int *ok_ewt_seed)
{

/*
   Set all values and return
*/
   *gt_seed        = MISSING;
   *ewt_seed       = MISSING;
   *fin_vel        = MISSING;

   *ok_gt_seed       = FALSE;
   *ok_ewt_seed      = FALSE;
}
