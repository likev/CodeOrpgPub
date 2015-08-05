/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:51 $
 * $Id: prcpacum_interp1_acum.c,v 1.2 2006/02/09 18:19:51 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_interp1_acum.c
    Author: Kelley Miles
    Created: 06 JAN 2005

    Description
    ===========
    This function computes the new period accumulation scan when the case is
    interpolation and it has just begun to rain (i.e. the previous rate
    scan is flagged zero but the current rate scan is not) by dividing the
    contents of the current rate scan (360 x 115) by two and multiplying 
    by the time difference (fraction of hours) between the current and
    previous scans.
    Note: Input rate scan units are (mmx10/hr).
          Output period accumulation scan units are (mmx100).

    Change History
    ============
    DATE          VERSION   PROGRAMMER           NOTES
    ----------    -------   ----------------     --------------------
    02/21/89      0000      P. PISANI            SPR # 90067
    04/23/90      0001      DAVID M. LYNCH       SPR # 90697
    02/22/91      0002      BAYARD JOHNSTON      SPR # 91254
    02/15/91      0002      JOHN DEPHILIP        SPR # 91762
    12/03/91      0003      STEVE ANDERSON       SPR # 92740
    12/10/91      0004      ED NICHLAS           SPR 92637 PDL Removal
    04/24/92      0005      Toolset              SPR 91895
    03/25/93      0006      Toolset              SPR NA93-06801
    01/28/94      0007      Toolset              SPR NA94-01101
    03/03/94      0008      Toolset              SPR NA94-05501
    04/11/96      0009      Toolset              CCR NA95-11802
    12/23/96      0010      Toolset              CCR NA95-11807
    03/16/99      0011      Toolset              CCR NA98-23803
    11/27/00      0012      C. Stephenson        CCR NA00-33301
    12/31/02      0013      D. Miller            CCR NA00-28601
    01/06/05      0014      K. Miles             CCR NA05-01303
    10/16/05      0015      C. Pham              CCR NA05-21401

Included variable descriptions
   MAX_ACUBINS          Constant. Total number of range bins for
                        accumulation.
   MAX_AZMTHS           Constant. Maximum number of azimuths in a scan.
   MAX_RABINS           Constant. Maximum number of bins along a
                        radial in the rate scan.
   AVG_FACTOR           Constant. Value used for taking an average
                        or dividing by two.
   FIRST_BIN            Constant. Starting bin number for 115 bins.
   FIRST_RADIAL         Constant. Starting radial number for 360 radials.
   RATE_SCALING         Constant. Scaling factor for use in precip
                        rates and accumulations (=10.)
   SEC_P_HOUR           Constant. Number of seconds in one hour.

Internal Tables/Work Area
    BN                  Current Range bin in loop for processing all range
                        bins (115)
    PERIOD_TIME         Max time for interpolation divided by 2 and then
                        divided by seconds per hour. Used to multiply each
                        element of rate scan to determine accumulation scan.
    RN                  Current radial number in loop for processing all
                        radials (360)

****************************************************************************/
/***  Global include files */
#include <rpgc.h>
#include <a313h.h>
#include <a313hbuf.h>

/***  Local include file */
#include "prcprtac_Constants.h"

void interp1_acum( short ratescan[][MAX_RABINS],
                   short acumscan[][MAX_ACUBINS],
                   double *time_scan_dif )
{
/*
   ACUMSCAN             Offset into output buffer of (360 x 115) short
                        period scan accumulation data.
   RATESCAN             Offset into input buffer that contains current
                        rate scan of 360 x 115 short words.
   TIME_SCAN_DIF        Difference between current and previous rate
                        scans in seconds.
*/

int   rn, bn;
double time_dif;

   if (DEBUG) {fprintf(stderr," A3135B__INTERP1_ACUM\n");}

/* Convert time difference (between current and previous scans) in seconds
   to hours (fraction). 
 */
   time_dif = *time_scan_dif / (double)SEC_P_HR;

/* Perform interpolation on the current rate scan (360 x 115).
   Interpolation is performed by: (10.x CUR_SCAN/2.) x time_dif
   Note: Period Accumulations converted to units of .01mm. 
 */
   for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
   {
      for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
      {
         /* Note: Changed for LINUX - Used RPGC_NINT library function instead
                  of adding 0.5 for rounding to the nearest integer.
          */

         acumscan[rn][bn] = RPGC_NINT((RATE_SCALING * ratescan[rn][bn] /
                             AVG_FACTOR) * time_dif);
         if (DEBUG) 
         {
          if ((rn+1)%60 == 1) 
          {
           fprintf(stderr,"bn, rn = (%d, %d): ratescan: %d  acumscan: %d\n",
                           bn+1,rn+1,ratescan[rn][bn],acumscan[rn][bn]);
          }
         }
      }/* End loop MAX_ACUBINS */
   }/* End loop MAX_AZMTHS */
}
