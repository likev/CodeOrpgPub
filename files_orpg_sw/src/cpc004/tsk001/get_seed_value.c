/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:58 $
 * $Id: get_seed_value.c,v 1.2 2003/07/17 15:07:58 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   get_seed_value.c

   PURPOSE:

   This routine returns the closest gate with a solution on the same
   radial as the bin in question. 

   CALLED FROM:

   first_triplet_attempt
   second_triplet_attempt
   pairs_and_trips_attempts
   final_unf_attempts

   INPUTS:

   int  az      - current azimuth number
   int  bin     - current bin number
   int  start   - start bin for search
   int  end     - end bin for search
   int  inc     - bin increment for search
   short *seed  - velocity found, if any 
   int  status  - set to TRUE if a seed was found, FALSE otherwise

   CALLS:
 
   None.

   OUTPUTS:

   None.

   RETURNS:

   Whether routine was successful or not (TRUE or FALSE)

   NOTES:

   HISTORY:

   R. May, 2/02         - modified for efficiency
   B. Conway, 10/00     - cleanup
   B. Conway, 6/96      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"


void
get_seed_value (int az, int bin, int start, int end, short *seed, int *status)
{
  int bin1, bin2; 
  *status = FALSE;
  *seed = MISSING;
  bin1 = bin2 = bin;

/*
  Go along the radial between the start and end gates. Hold the closest
  velocity on the radial to the gate in question.
*/

  while (bin1 <=end || bin2 >=start)
      {
      if (bin2 >= start)
         if (save.final_vel[az][bin2] < DATA_THR)
            {
            *seed = save.final_vel[az][bin2];
            *status = TRUE;
            return;   
            }
      if (bin1 <= end)
         if(save.final_vel[az][bin1] < DATA_THR)
            {
            *seed = save.final_vel[az][bin1];
            *status = TRUE;
            return;   
            }
      ++bin1;
      --bin2;
      }
  return;
}
