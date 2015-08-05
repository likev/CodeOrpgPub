/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:03 $
 * $Id: initialize_rad_direction.c,v 1.2 2003/07/17 15:08:03 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   initialize_rad_direction.c

   PURPOSE:

   This routine sets the loop bounds for searching radials depending on
   which direction along the radial to process.

   CALLED FROM:

   first_triplet_attempt
   second_triplet_attempt
   pairs_and_trips_attempts
   final_unf_attempts

   INPUTS:

   int   direction - direction to process along radials
   int   *start    - first gate to start at
   int   *int      - increment along radial
   int   *finish   - final gate to process

   CALLS:

   None.

   OUTPUTS:

   int   *start    - first gate to start at
   int   *int      - increment along radial
   int   *finish   - final gate to process

   RETURNS:

   None.

   NOTES:

   Here forward means from the radar center outward. Backward means to
   process from the last gate inward to the radar center.

   HISTORY:

   B. Conway, 10/00     - cleanup
   B. Conway, 5/99      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"

void
initialize_rad_direction(int direction, int *start, int *inc,
                         int *finish, int rad_num)
{

/* 
   Depending on which directions to go along the radial set the
   start, inc and finish values
*/
 
   if(direction == GO_FORWARD)
     {
     *start  = 0;
     *inc    = 1;
     *finish = save.vel_limit[rad_num];
     }
   else
     {
     *start  = save.vel_limit[rad_num];
     *inc    = -1;
     *finish = 0;
     }
}
