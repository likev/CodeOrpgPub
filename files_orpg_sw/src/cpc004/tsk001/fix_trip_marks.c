/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:49 $
 * $Id: fix_trip_marks.c,v 1.2 2003/07/17 15:07:49 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/**************************************************************************

   fix_trip_marks.c

   PURPOSE:

   This routine set the gates around the trip marks to MISSING.

   CALLED FROM:

   save_mpda_data

   INPUTS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   There tends to be alot of noisy gates at the trip marks. When integrated
   in the other velocity fields, these gates cause big dealiasing problems.
   This routine sets gates within MIN_TRIP_FIX and MAX_TRIP_FIX distance
   of the trip marks at each PRF to MISSING.

   HISTORY:

   D. Zittel, 02/2003   - Implementation phase cleanup and install switch 
                          default case
   B. Conway, 9/00      - cleanup 
   B. Conway, 5/98      - original development

****************************************************************************/

#include <orpg.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"

void
fix_trip_marks(int radial)
{
   int j, trip;
  
   switch (num_prfs)
   {

   case 1 :
     {
     trip = 2*save.prf1_unab_gates;
     for (j=min_trip_fix; j<=max_trip_fix; ++j)
         {
         if( (save.prf1_unab_gates+j) < MAX_GATES)
             {
             save.vel1[radial][save.prf1_unab_gates+j] = MISSING;
             save.sw1[radial][save.prf1_unab_gates+j] = UIF_MISSING;
             }
         if( (trip+j) < MAX_GATES)
             {
             save.vel1[radial][trip+j] = MISSING;
             save.sw1[radial][trip+j] = UIF_MISSING;
             }             
         }

     break;
     } 
   case 2 :
     {
     trip = 2*save.prf2_unab_gates;
     for (j=min_trip_fix; j<=max_trip_fix; ++j)
         {
         if( (save.prf2_unab_gates+j) < MAX_GATES)
             {
             save.prf2[save.prf2_unab_gates+j] = MISSING;
             save.sw_rad[save.prf2_unab_gates+j] = UIF_MISSING;
             }             
         if( (trip+j) < MAX_GATES)
             {
             save.prf2[trip+j] = MISSING;
             save.sw_rad[trip+j] = UIF_MISSING;
             }             
         }

     break;
     }
   case 3 :
     {
     trip = 2*save.prf3_unab_gates;
     for (j=min_trip_fix; j<=max_trip_fix; ++j)
         {
         if( (save.prf3_unab_gates+j) < MAX_GATES)
             {
             save.prf3[save.prf3_unab_gates+j] = MISSING;
             save.sw_rad[save.prf3_unab_gates+j] = UIF_MISSING;
             }             
         if( (trip+j) < MAX_GATES)
             {
             save.prf3[trip+j] = MISSING;
             save.sw_rad[trip+j] = UIF_MISSING;
             }
         }
     break;
     }
   default:
     LE_send_msg(GL_ERROR,"MPDA: fix_trip_marks - invalid prf = %d\n", num_prfs);
   }   
}
