/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/01 22:17:21 $
 * $Id: initialize_uif_table.c,v 1.3 2005/12/01 22:17:21 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/**************************************************************************

   initialize_uif_table.c

   PURPOSE:

   This routine intitializes a lookup table to convert UIF values to floating
   point values.
   
   CALLED FROM:

   save_mpda_data

   INPUTS:

   None.

   CALLS:

   None.
   
   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   HISTORY:

   R. May, 2/02      - original development

****************************************************************************/
#include "mpda_constants.h"
#include "mpda_structs.h"

#define MAX_VALID_UIF         255
#define MIN_VALID_UIF           2

void
initialize_uif_table(int resolution)
{
   short velocity;

   uif_table[UIF_MISSING]=MISSING;
   uif_table[UIF_RNG_FLD]=RNG_FLD;
   uif_table[MAX_NUM_UIF]=MISSING;
   
   scale_factor = 3 - resolution;

   max_uif_vel = ((float) (MAX_VALID_UIF - UIF_VEL_BIAS))/scale_factor;
   min_uif_vel = ((float) (MIN_VALID_UIF - UIF_VEL_BIAS))/scale_factor;

   for(velocity=2;velocity<MAX_NUM_UIF;++velocity)
      {
      uif_table[velocity] = (short) (((float)(velocity-UIF_VEL_BIAS)/scale_factor)*FLOAT_10);
      }

   /* Used in vel_short_to_UIF to check maximum/minimum velocity */
   max_uif_vel = uif_table[MAX_NUM_UIF-1];
   min_uif_vel = uif_table[2];

   sf_velocity = (float) scale_factor / FLOAT_10;
   return;   
}

