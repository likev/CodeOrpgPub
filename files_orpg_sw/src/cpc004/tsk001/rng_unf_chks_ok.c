/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:23 $
 * $Id: rng_unf_chks_ok.c,v 1.2 2003/07/17 15:08:23 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************

   rng_unf_checks.c

   PURPOSE:

   This routine simple checks to make sure the velocity and powers
   exist within the valid trip ranges.

   CALLED FROM:

   range_unfold_prf_scans

   INPUTS:

   short   *trip1_vel  - original velocity (m/s*10)
   char    *trip1_sw   - original spectrum width (UIF)
   int     vel2_loc    - 2nd trip gate location 
   float   trip1_pwr   - power at the trip 1 location (db)
   float   trip2_pwr   - power at the trip 2 location (db)

   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   HISTORY:

   B. Conway, 10/96      - original development
   B. Conway, 11/00      - cleanup
   D. Zittel, 02/2003    - Implementation phase cleanup per Unit Testing
   R. May, 04/2003	 - Added support for creating spectrum width field

****************************************************************************/

#include "mpda_constants.h"

int
rng_unf_chks_ok(short *trip1_vel, unsigned short *trip1_sw, int vel2_loc,
                float trip1_pwr, float trip2_pwr)
{

   if (vel2_loc >= MAX_GATES && trip1_pwr < PWR_THR)
      {
      *trip1_vel = MISSING;
      *trip1_sw = UIF_MISSING;
      return FALSE;
      }

   if (trip1_pwr < PWR_THR && trip2_pwr < PWR_THR)
      {
      *trip1_vel = MISSING;
      *trip1_sw = UIF_MISSING;
      return FALSE;
      }

   return TRUE;
}
