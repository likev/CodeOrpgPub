/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:29 $
 * $Id: unfold_vel.c,v 1.2 2003/07/17 15:08:29 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   unfold_vel.c

   PURPOSE:

   This routine attempts to unfold a given velocity.

   CALLED FROM:

   multi_prf_unf
   check_seed_unf

   INPUTS:

   short seed   - velocity to use for unfolding attempt (m/s*10)
   short vel    - velocity value to try and unfold (m/s*10)
   short nyq    - Nyquist of velocity to use for unfolding (m/s*10)
   short thres  - threshold that dealiasing must fall withing (m/s*10)

   CALLS:

   None.
 
   OUTPUTS:

   None.

   RETURNS:

   MISSING if no solution found, or solution if found 

   NOTES:

   This routine uses the incoming seed value to determine what Nyquist
   interval the incoming vel value shoud be dealiased into. It checks
   each interval out to +- 5. 

   HISTORY:

   D. Zittel, 2/02	- improve efficiency
   B. Conway, 9/00      - cleanup
   B. Conway, 5/96      - original development

****************************************************************************/

#include "mpda_constants.h"

short
unfold_vel(short seed, short vel, short twice_nyq, short thres)
{

   int i, diff, vel2, save_diff;

/*
   Check to see if the incoming vel is already within the thres
   value of the velocity to check against
*/

   if( (diff = abs(seed - vel)) <= thres)
      return(vel);
      
   save_diff = 2*MAX_INT_VEL;

/*  Modified for efficiency  */

    if (vel > seed)
       twice_nyq = -twice_nyq;

/*   
   Check for convergence within the Nyquist intervals. If vel can be
   dealiased with the thres value, return the value.
*/

   for(i=1; i<=MAX_NYQ_INTV; i++) 
      {
      vel2 = vel +(i * twice_nyq);
      diff  = abs(seed - vel2);
      if(diff <= thres) 
         return(vel2);
      if(diff > save_diff)
         return(MISSING);
      save_diff = diff;
      }

   return(MISSING);
} 
