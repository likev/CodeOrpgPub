/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:00 $
 * $Id: get_trip_num.c,v 1.2 2003/07/17 15:08:00 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************
 
   get_trip_num.c

   PURPOSE:
 
   This routine assigns the gate for the appropriate velocity in question
   depending on which trip range has the most power and can be classified
   a unambiguous.
 
   CALLED FROM:
 
   range_unfold_prf_scans 
 
   INPUTS:
 
   int     cur_az       - current radial number
   int     bin_num      - current bin number
   int     unabg_gates  - number of gate in unambiguous range for the given PRF
   float   tover        - amount of power over other trips to be declared unambig
   int     veldis       - velocity gate spacing (m)
 
   CALLS:
 
   None.
 
   OUTPUTS:
 
   None.
 
   RETURNS:
 
   Gate number along radial for corresponding trip if found, MISSING
   otherwise
 
   NOTES:

   The trip location that can be returned from this routine can only be
   the first or second trip locations. However, to be unambiguous, these
   locations must also have more power (+tover) than the 3rd and 4th 
   trip locations. 

   HISTORY:

   B. Conway, 10/96      - original development
   B. Conway, 11/00      - cleanup
 
****************************************************************************/

#include "mpda_constants.h"
#include "mpda_range_unf_data.h"

int
get_trip_num(int cur_az, int bin_num, int unabg_gates, float tover, int veldis)
   {

   float pwr_trip1, pwr_trip2, pwr_trip3, pwr_trip4;

   int   vel_trip1, vel_trip2, ref_trip1, ref_trip2, ref_trip3, ref_trip4;
   int   pwr_gates;

/*
   Set up each trip location. Note pwr_gates is the max gates associated with
   this azimuth.
*/

   vel_trip1 = bin_num;
   vel_trip2 = ( (unabg_gates + vel_trip1) < MAX_GATES) ? unabg_gates + vel_trip1 :
                                                          MAX_GATES-1;
   pwr_gates = base_scan.num_gates[cur_az];

   ref_trip1 = vel_trip1/veldis +1;

   ref_trip2 = ( ((unabg_gates + vel_trip1)/veldis +1) < pwr_gates) ? 
                  (unabg_gates + vel_trip1)/veldis +1  : -1;

   ref_trip3 = ( ((2*unabg_gates + vel_trip1)/veldis +1) < pwr_gates) ? 
                  (2*unabg_gates + vel_trip1)/veldis +1  : -1;

   ref_trip4 = ( ((3*unabg_gates + vel_trip1)/veldis +1) < pwr_gates) ? 
                  (3*unabg_gates + vel_trip1)/veldis +1  : -1;

/*
   Assign a power value to each possible trip location
*/

   pwr_trip1 = base_scan.pwr[cur_az][ref_trip1];

   pwr_trip2 = (ref_trip2 > -1) ? base_scan.pwr[cur_az][ref_trip2] : 
                                  MISSING_PWR;

   pwr_trip3 = (ref_trip3 > -1) ? base_scan.pwr[cur_az][ref_trip3] :
                                  MISSING_PWR;

   pwr_trip4 = (ref_trip4 > -1) ? base_scan.pwr[cur_az][ref_trip4] :
                                  MISSING_PWR;

/*  
   Check each power value (+tover) and assign the velocity to the
   appropiate location.
*/
     
/* 
   Check for 1st trip echos 
*/

   if( (pwr_trip1 > (pwr_trip2+tover) ) && (pwr_trip1 > (pwr_trip3+tover) ) &&
       (pwr_trip1 > (pwr_trip4+tover) ) )
       return (vel_trip1);

/* 
   Check for 2nd trip echos 
*/

   if( (pwr_trip2 > (pwr_trip1+tover) ) && (pwr_trip2 > (pwr_trip3+tover) ) &&
       (pwr_trip2 > (pwr_trip4+tover) ) )
       return (vel_trip2);

/* 
   Otherwise, data are ambiguous 
*/ 

       return (int) MISSING_PWR;
}
