/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:53 $
 * $Id: get_closest_radial.c,v 1.2 2003/07/17 15:07:53 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************
 
   get_closest_radial.c
 
   PURPOSE:
 
   This routine the radial closest in azimuth to the base PRF radial
   in question.
 
   CALLED FROM:

   align_prf_scans
   range_unfold_prf_scans
   fill_radials

   INPUTS:
 
   int  az1     - azimuthal value from base PRF to match
   int *az2     - pointer to first azimuthal value in PRF to match
   int num_rads - number of radials in PRF to match    
 
   CALLS:

   None.

   OUTPUTS:
 
   None.
 
   RETURNS:
 
   Radial number closest in azimuth to base PRF radial 
 
   NOTES:
 
   1) Azimuthal values are integer values to the nearest 1/100th of a degree.
      i.e. 12984 is equivalent to 129.84 deg.
 
   HISTORY:
 
   B. Conway, 6/96      - original development
   B. Conway, 8/00      - cleanup 
 
****************************************************************************/

#include "mpda_adapt_params.h"
#include "mpda_constants.h"

int
get_closest_radial (int az1, int *az2, int num_rads)
{

   int rad_num, chk_dis, chk_az, hld_loc = MAX_RADS - 1;
   int reflx_deg;

/*  
   Set an large initial distance for between radial checking to
   overcome.
*/
   
   reflx_deg = FULL_CIRC - max_delta_az;
   chk_dis = max_delta_az;

/*
   The following loop goes through all the radials and finds the
   radial closest in azimuth to the base PRF radial passed in. 
   hld_loc is the integer location of the closest radial to az1
   of the az2 array.
    
*/

   for (rad_num=0; rad_num<num_rads; ++rad_num)
       {
       chk_az = abs(az1 - *(az2+rad_num));

/* If the difference is too large, check to see if the two radials are
   close across the zero degree radial */

       if (chk_az > reflx_deg)
         chk_az = abs(FULL_CIRC - chk_az);
       if ( chk_az < chk_dis)
          {
          chk_dis = chk_az;
          hld_loc = rad_num; 
          }
       }
   return(hld_loc);
}
