/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:47 $
 * $Id: fix_nyq_value.c,v 1.2 2003/07/17 15:07:47 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************
 
   fix_nyq_value.c
 
   PURPOSE:

   This routine rounds the nyquist velocity to the nearest TABLE_RES.
 
   CALLED FROM:

   save_mpda_data
   initialize_lookup_tables

   INPUTS:

   short  *nyq   - nyquist velocity in m/s*10 

   CALLS:

   None.
 
   OUTPUTS:
 
   None. 

   RETURNS:
 
   Modified nyquist velocity 

   HISTORY:

   D Zittel, 02/2003 - Implementation phase cleanup 
   R May,    5/02    - Correct bug and simplified to one formula
   B Conway, 5/00    - cleanup
   B Conway, 5/98    - orginal development
 
***************************************************************************/
#include "mpda_constants.h"

void fix_nyq_value(short *nyq)
{

/* Set any value below MIN_NYQ to MIN_NYQ */

   if (*nyq < MIN_NYQ)
      {
      *nyq = MIN_NYQ;
      return;
      }
   
/* Set any value above MAX_NYQ to MAX_NYQ. */

   else if (*nyq > MAX_NYQ)
      {
      *nyq = MAX_NYQ;
      return;
      }

/* If the Nyquist value is already a round number, do nothing */

   else if (*nyq%TABLE_RES == 0)
     return;

/* Otherwise, return the nyquist to the nearest TABLE_RES */

   else
     *nyq = ((short)((float) *nyq/TABLE_RES + 0.5) * TABLE_RES);
}

   
