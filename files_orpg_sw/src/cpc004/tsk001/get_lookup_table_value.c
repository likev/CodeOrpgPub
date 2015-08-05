/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:56 $
 * $Id: get_lookup_table_value.c,v 1.2 2003/07/17 15:07:56 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

 /**************************************************************************

   get_lookup_table_value.c 

   PURPOSE:

   This routine gets the values from the lookup tables.

   CALLED FROM:

   pairs_and_trips_attempts

   INPUTS:

   short  nyq1  - Nyquist velocity of one PRF (m/s*10)  
   short  nyq2  - Nyquist velocity of another PRF (m/s*10)
   short  pos1  - table position of first PRF (m/s*10)
   short  pos2  - table position of other PRF (m/s*10)

   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   The table positions passed to this routine are of the form
   (vel+nyq)/5. The units are m/s*10. The division by 5 gives
   1/2 m/s resolution for the table lookup.

   HISTORY:

   B. Conway, 10/00     - cleanup
   B. Conway, 5/99      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"

short
get_lookup_table_value(short num1, short num2, short pos1, short pos2)
{

   if(num1 > num2)
     return (lookup_table[prf_pairs_table[num1*MAX_PRFS+num2]].value[pos2*LOOKUP_TABLE_SIZE+pos1]);
   else
     return (lookup_table[prf_pairs_table[num2*MAX_PRFS+num1]].value[pos1*LOOKUP_TABLE_SIZE+pos2]);
}
