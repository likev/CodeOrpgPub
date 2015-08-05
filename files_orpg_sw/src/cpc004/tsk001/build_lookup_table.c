/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/10/04 19:41:24 $
 * $Id: build_lookup_table.c,v 1.5 2005/10/04 19:41:24 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/**************************************************************************

   build_lookup_table.c

   PURPOSE:

   This routine builds the lookup tables for processing of the velocity
   pairs. 

   CALLED FROM:

   initialize_lookup_tables

   INPUTS:

   short   nyq1       - Nyquist velocity of first PRF (m/s*10) 
   short   nyq2       - Nyquist velocity of second PRF (m/s*10)
   int     table_num  - lookup table number

   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   TRUE or FALSE depending on success of filling table.  If the two nyquist velocities
   have the same value, the table cannot be filled.

   NOTES:

   1) This routine builds the lookup tables for 3 Nyquist intervals given
   the Nyquist pair. The effective Nyquist is 1/(1/N1-1/N2). 

   2) The table entries are in table_res increments.

   HISTORY:

   B. Conway, 10/00      - cleanup
   B. Conway, 5/99      - original development

****************************************************************************/

#include <orpg.h>
#include "mpda_constants.h"
#include "mpda_structs.h"

#define  VEL_THRES 30    /* 3 m/s *10 */
#define  NYQ_INTVS 7     /* number of Nyquist intervals */

int
build_lookup_table(short nyq1, short nyq2, int table_num)
{

/*
   The mult array containes the Nyquist intervals for which
   to build table values
*/
   int i, j, k, l; 
   int mult[NYQ_INTVS] = {0,1,-1,2,-2,3,-3};
   short v1, v2, fin_vel, eq_nyq;
   short co_nyq1, co_nyq2, vel_limit;
   short isgn;
   
/* Check to make sure that the nyquists do not exceed MAX_NYQ */
   if(nyq1 > MAX_NYQ)
     LE_send_msg(0, "MPDA: build_lookup_table -- Nyq1 exceeds maximum allowable nyquist value, setting to %4.1f\n",
                 MAX_NYQ/FLOAT_10);

   if(nyq2 > MAX_NYQ)
     LE_send_msg(0, "MPDA: build_lookup_table -- Nyq2 exceeds maximum allowable nyquist value, setting to %4.1f\n",
                 MAX_NYQ/FLOAT_10);

/*
   Find Nyquist co-interval and equivalent nyquist
   Equivalent nyquist is the reciprocal of:
   1/nyq1 - 1/nyq2
*/ 
   if(nyq2 != nyq1)
      {
      eq_nyq = nyq1*nyq2/abs(nyq2 - nyq1);
      LE_send_msg(GL_INFO,"MPDA: Building lookup table; nyq1 = %d, nyq2 = %d\n", nyq1,nyq2);
      }      
   else
      {
      LE_send_msg(GL_ERROR,"MPDA: Can't build lookup table; nyq1 = nyq2 = %d\n", nyq1);
      return TRUE;
      }

   vel_limit = (eq_nyq < MAX_INT_VEL) ? eq_nyq : MAX_INT_VEL;
   co_nyq1 = 2 * nyq1;
   co_nyq2 = 2 * nyq2;   

/*
   Loop through each 1/2 m/s resolution bin and try to get
   set a value that falls within VEL_THRES of each other.
*/

   for (i=-nyq1; i<=nyq1; i+=TABLE_RES)
       {
       for (j=-nyq2; j<=nyq2; j+=TABLE_RES)
           {
           
           fin_vel = MISSING;

/*
           Check each interval and set a table value if possible.
           In the loops below, the nyq1 value is used for comparison
           against the nyq2 intervals. If the interval values match
           the final value is set to that exact match. Otherwise
           if the difference between the interval numbers is less than
           the threshold, the average of the two values is used
           as the table entry.
*/

           for (k=0; k<NYQ_INTVS; ++k) 
               {
               v1 = i + mult[k]*co_nyq1;
               if (abs(v1) > vel_limit) continue;
               for (l=0; l<NYQ_INTVS; ++l)
                   {
                   v2 = j + mult[l]*co_nyq2;
                   if (abs(v2) > vel_limit) continue;
                   if (v1 == v2)
                      {
                      fin_vel = v1;
                      k = NYQ_INTVS;  
                      break;
                      }
                   if (abs(v1-v2) <= VEL_THRES)
                      {
                      fin_vel = (v1 + v2)/2;
                      isgn = (fin_vel < 0) ? -1 : 1;
                      fin_vel = (short)(fin_vel/5. + isgn * 0.5) * 5;
                      k=NYQ_INTVS;
                      break;
                      }
                   }
               }

/*
          Assign the value found to the table entry
*/

          lookup_table[table_num].value[((i+nyq1)/TABLE_RES)*LOOKUP_TABLE_SIZE+(j+nyq2)/TABLE_RES] 
                    = fin_vel;
          }
      }
      return FALSE;
} 
