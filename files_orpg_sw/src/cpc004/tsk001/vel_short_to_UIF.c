/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/08 12:52:55 $
 * $Id: vel_short_to_UIF.c,v 1.4 2006/09/08 12:52:55 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/***********************************************************************
 
   vel_float_to_UIF.c

   PURPOSE:

   This routine converts a float to Unisys Integer Format.
 
   CALLED FROM:

   output_mpda_data

   INPUTS:

   float velocity - value to convert to UIF 

   CALLS:

   None.

   OUTPUTS:
 
   None  

   RETURNS:
 
   short equivalent of UIF value  

   NOTES:
 
   1) This replacement for this routine will likely be something in
      the ORPG API. 
  
   HISTORY:
 
   R. May, 2/03      - Renamed and converted to take a short as input
   B Conway, 5/00    - cleanup
   B Conway, 5/97    - orginal development
 
***************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"
#include <rpgc.h>

unsigned short int
vel_short_to_UIF(short velocity)
{
   float tstvel;
   int tstvel_int;

   if(velocity == RNG_FLD)
     return(UIF_RNG_FLD);
   else if(velocity == MISSING)
     return(UIF_MISSING);
  
   tstvel = (float) velocity;
   if(tstvel > max_uif_vel)
     tstvel = max_uif_vel;
   else if(tstvel < min_uif_vel)
     tstvel = min_uif_vel;

   tstvel = tstvel*sf_velocity + UIF_VEL_BIAS;
   tstvel_int = (int) RPGC_NINT( tstvel ); 

   return( (unsigned short int) tstvel_int );

}

