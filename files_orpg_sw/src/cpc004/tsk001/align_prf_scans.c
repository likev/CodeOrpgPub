/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 14:48:48 $
 * $Id: align_prf_scans.c,v 1.2 2003/07/17 14:48:48 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************

   align_prf_scans.c
 
   PURPOSE:
 
   This routine matches the current radial from the 2nd and 3rd velocity cuts
   to the radial in the 1st velocity cut with the nearest azimuth angle. In
   this manner the scans can be "aligned" as closely as possible for
   dealiasing. 
   
   CALLED FROM:

   save_mpda_data
 
   INPUTS:
 
   None.  

   CALLS:

   get_closest_radial
 
   OUTPUTS:
 
   None.
 
   RETURNS:
 
   None.
 
   HISTORY:
 
   B. Conway, 6/96      - original development
   B. Conway, 8/00      - cleanup 
   D. Zittel, 02/2003   - Implementation phase cleanup and add switch default
                          case
 
****************************************************************************/

#include <memory.h>
#include <orpg.h>
#include "mpda_constants.h"
#include "mpda_structs.h"

int get_closest_radial (int az1, int *az2, int num_rads);

void align_prf_scans( void )
{

  int fnl_az;

/* 
   Using the first collected PRF as the template, match up the closest 
   radial and fill in the gates in the save.vel arrays with the raw
   velocities from the current radial.
*/

  switch(num_prfs)
    {
    case 2:  /* PRF2 */

/* Get the closest radial */              

    fnl_az = get_closest_radial(save.az2, &save.az1[0], save.tot_prf1_rads);

/* Set the flag to indicate the radial was copied and copy the radial */

    if(fnl_az < save.tot_prf1_rads)
       {
       save.az_flag2[fnl_az] = TRUE;
       memcpy(&save.vel2[fnl_az], save.prf2, sizeof(save.prf2));
       memcpy(&save.sw2[fnl_az], save.sw_rad, sizeof(save.sw_rad));
       }
    break;

    case 3:  /* PRF3 */

/* Get the closest radial */

    fnl_az = get_closest_radial(save.az3, &save.az1[0], save.tot_prf1_rads);

/* Set the flag to indicate the radial was copied and copy the radial */

    if(fnl_az < save.tot_prf1_rads)
       {
       save.az_flag3[fnl_az] = TRUE;
       memcpy(&save.vel3[fnl_az], save.prf3, sizeof(save.prf3));
       memcpy(&save.sw3[fnl_az], save.sw_rad, sizeof(save.sw_rad));
       }
    break;
    
    default:  /* Unknown PRF */
      LE_send_msg(GL_ERROR,"MPDA: align_prf_scans - invalid prf = %d\n",num_prfs);
    }
}
