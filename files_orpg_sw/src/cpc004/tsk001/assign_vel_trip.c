/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 14:48:51 $
 * $Id: assign_vel_trip.c,v 1.2 2003/07/17 14:48:51 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************

   assign_vel_trip

   PURPOSE:

   This routine assigns the velocity in question to the correct trip

   CALLED FROM:

   range_unfold_prf_scans 

   INPUTS:

   int    trip_gate        - gate for assignment of velocity
   int    num_trip1_gates  - unambiguous gates for given prf
   float  trip1_pwr        - power at trip1 (db)
   float  trip2_pwr        - power at trip2 (db)
   short  *trip1_vel       - velocity at first trip range (m/s*10)
   short  *tripgt_vel      - velocity at trip_gate trip range (m/s*10)
   short  *trip2_vel       - velocity at second trip range (m/s*10)
   short  *trip1_sw        - spectrum width at first trip range (UIF)
   short  *tripgt_sw       - spectrum width at trip_gate trip range (UIF)
   short  *trip2_sw        - spectrum width at second trip range (UIF)

   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   Values for which addresses are pass in. 

   NOTES:

   If the trip gate is found to be in the second trip, the velocity is 
   moved to that gate. The first trip velocity gate is then set to RNG_FLD.

   HISTORY:

   B. Conway, 10/96      - original development
   B. Conway, 11/00      - cleanup
   R. May, 04/2003	 - Added support for creating spectrum width field

****************************************************************************/

#include "mpda_constants.h"

void
assign_vel_trip(int trip_gate, int num_trip1_gates, float trip1_pwr, 
                float trip2_pwr, short *trip1_vel, short *tripgt_vel,
                short *trip2_vel, unsigned short *trip1_sw,
                unsigned short *tripgt_sw, unsigned short *trip2_sw)
{

   if (trip_gate > num_trip1_gates)
      {
      *tripgt_vel = *trip1_vel;
      *tripgt_sw = *trip1_sw;
      *trip1_vel = (trip1_pwr > PWR_THR) ? RNG_FLD : MISSING;
      *trip1_sw = (trip1_pwr > PWR_THR) ? UIF_RNG_FLD : UIF_MISSING;
      return;
      }

   if (trip2_pwr > PWR_THR)
      {
      *trip2_vel = RNG_FLD;
      *trip2_sw = UIF_RNG_FLD;
      }

   return; 
}
