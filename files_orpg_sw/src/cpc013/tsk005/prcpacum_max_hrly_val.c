/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:12:39 $
 * $Id: prcpacum_max_hrly_val.c,v 1.2 2014/03/18 14:12:39 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_max_hrly_val.c
    Author: Kelley Miles
    Created: 14 DEC 2004

    Description
    ===========
    The function examines the 360 x 115 I*2 hourly scan array to determine the
    maximum hourly value within the array. If the method for computing hourly
    totals is subtraction, then the hourly array is again searched to determine
    whether there are any negative values which are replaced by zeroes.

    Change History
    ============
    DATE          VERSION   PROGRAMMER           NOTES
    ----------    -------   ----------------     --------------------
    02/21/89      0000      P. PISANI            SPR # 90067
    02/22/91      0001      BAYARD JOHNSTON      SPR # 91254
    02/15/91      0001      JOHN DEPHILIP        SPR # 91762
    12/03/91      0002      STEVE ANDERSON       SPR # 92740
    12/10/91      0003      ED NICHLAS           SPR 92637 PDL Removal
    04/24/92      0004      Toolset              SPR 91895
    03/25/93      0005      Toolset              SPR NA93-06801
    01/28/94      0006      Toolset              SPR NA94-01101
    03/03/94      0007      Toolset              SPR NA94-05501
    04/11/96      0008      Toolset              CCR NA95-11802
    12/23/96      0009      Toolset              CCR NA95-11807
    03/16/99      0010      Toolset              CCR NA98-23803
    12/31/02      0011      D. Miller            CCR NA00-28601
    12/14/04      0012      K. Miles, C. Pham    CCR NA05-01303

Included variable descriptions
    HYZ_SUPL       Constant. Size of Supplemental Data array in Hybrid Scan
                   output buffer.
                   SSIZ_PRE + SSIZ_RATE + SSIZ_ACUM + SSIZ_ADJU = 13+14+16+5=48
    MAX_AZMTHS     Constant. Maximum number of azimuths in a scan (index into
                   output buffer of adjusted values).
    MAX_HLYACU     Constant. Offset into supplementary data array in the
                   output buffer for the maximum value in the hourly scan array.
    METHOD         Indicates method of hourly accumulations.
                   0 indicates method for hourly totals is
                   addition, 1 indicates method is
                   subtraction.
    CURR_HOUR      Constant. Current hour offset into hour header.
    FIRST_BIN      Constant. Starting bin number for 115 bins.
    FIRST_RADIAL   Constant. Starting radial number for 360 radials.
    H_MAX_HRLY     Constant. Position of maximum hourly accumulation value in 
                   hourly header array.
    MAX_ACUBINS    Constant. Total number of range bins for accumulation.
    NULL0          Constant. A value used for initalization and testing.
    SUB_HRLY_FLG   Constant. Indicates method for computing hourly totals is 
                   by subtraction.
    HourHdr        Relevant information concerning hourly accumulations.
                   Indexed by one symbolic name listed under value and
                   current or previous hour.

Internal Tables/Work Area
    bn             CURRENT RANGE BIN IN LOOP FOR PROCESSING ALL RANGE BINS (115)
    initval        INITIAL VALUE FOR MAX_HRLY_VAL SET TO -1.
    max_hrly_val   RUNNING VALUE OF MAXIMUM HOURLY VALUE TO BE REPLACED BY 
                   SUBSEQUENT VALUES SHOULD THEY EXCEED THIS CURRENT VALUE.
    rn             CURRENT RADIAL NUMBER IN LOOP FOR PROCESSING ALL RADIALS 
                  (360)
****************************************************************************/
/** Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/** Local include file */
#include "prcprtac_Constants.h" 

void max_hrly_val( short acumhrly[][MAX_ACUBINS] )
{

   int rn, bn, max_hrly_val;
   int initval=-1;

   if ( DEBUG ) 
     {fprintf(stderr," A3135P__MAX_HRLY_VAL\n");}

/* Determine the maximum accumulation value in the hourly buffer */

   max_hrly_val = initval;

   for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
   {
      for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
      {
         if ( acumhrly[rn][bn] > max_hrly_val )
         {
            max_hrly_val = acumhrly[rn][bn];
         }
      }
   }

   if (DEBUG) 
     {fprintf(stderr," Max Hourly Value = %d\n",max_hrly_val);}

/* Store maximum hourly value into the hour header and in supplemental
   data array portion of the output buffer. 
 */
   AcumSupl.max_hlyacu = max_hrly_val;
   HourHdr[curr_hour].h_max_hrly = max_hrly_val;

/* If method is subtraction, ensure that negative values in the hourly
   buffer are set back to zero. 
 */
   if ( blka.method == SUB_HRLY_FLG )
   {
      for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
      {
         for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
         {
            if (acumhrly[rn][bn] < NULL0 )
            {
               acumhrly[rn][bn] = NULL0;
            }
         } /* End loop MAX_ACUBINS */
      }/* End loop MAX_AZMTHS */
   }
}
