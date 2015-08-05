/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:48 $
 * $Id: prcpacum_extrap_acum.c,v 1.2 2006/02/09 18:19:48 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_extrap_acum.c
    Author: Kelley Miles
    Created: 06 JAN 2005

    Description
    ===========
    This function computes period accumulations when the case is extrapolation.
    The precip rate data for the previous or current rate scan is multiplied
    by one-half the maximum time for interpolation and converted to (fraction 
    of) hours in order to generate the accumulation for the first or third 
    periods, respectively.
    Note: There are three periods in existance when the case is extrapolation. 
    The middle period contains missing data.
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
    10/26/05      0015      C. Pham              CCR NA05-21401

Included variable descriptions
   MAX_ACUBINS           Constant. Total number of range bins for
                         accumulation.
   MAX_AZMTHS            Constant. Maximum number of azimuths in a scan.
   MAX_RABINS            Constant. Maximum number of bins along a
                         radial in the rate scan.
   MAX_INTERP_TIM        Maximum time allowed for interpolation.
                         Default is 30 minutes or 1,800 seconds.
                         Value can range to 3,600 seconds or one hour.
   AVG_FACTOR            Constant. Value used for taking an
                         average or dividing by two.
   FIRST_BIN             Constant. Starting bin number for 115 bins.
   FIRST_RADIAL          Constant. Starting radial number for 360 radials.
   RATE_SCALING          Constant. Scaling factor for use in precip
                         rates and accumulations (=10.)
   SEC_P_HOUR            Constant. Number of seconds in one hour.


Internal Tables/Work Area
    BN                   Current Range bin in loop for processing all range
                         bins (115)
    PERIOD_TIME          Max time for interpolation divided by 2 and then
                         divided by seconds per hour. Used to multiply each
                         element of rate scan to determine accumulation scan.
    RN                   Current radial number in loop for processing all
                         radials (360)


****************************************************************************/
/***  Global include file */
#include <rpgc.h>
#include <a313h.h>
#include <a313hbuf.h>

/***  Local include file */
#include "prcprtac_Constants.h"

void extrap_acum( short gratescan[][MAX_RABINS],
                  short prdscan[][MAX_ACUBINS] )
{

/*
   GRATESCAN     General rate scan of 360 x 115 I*2 data used as input.
                 The source is either the input buffer or scratch buffer one.
   PRDSCAN       Scratch buffer two or accumulation scan to scan offset into
                 output buffer of 360 x 115 I*2 data.
*/

double period_time;
int    rn,bn;

/* Determine time period of scan as one-half the maximum interpolation time and
   convert to hours (fraction). */

   period_time = ( blka.max_interp_tim /(double)AVG_FACTOR ) / (double)SEC_P_HR;
   if (DEBUG) {fprintf(stderr,"period_time = %f\n",period_time);}

/* Perform extrapolation on the general input rate scan. Extrapolation is
   performed by: (10.x scan) x period_time.
   (Note: Period accumulations converted to units of .01 mm.) */

   for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
   {
      for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
      {
         /* Note: Changed for LINUX - Used RPGC_NINT library function instead of
                  adding 0.5 for rounding to the nearest integer.
          */

         prdscan[rn][bn] = RPGC_NINT((RATE_SCALING * (double)gratescan[rn][bn])*
                                      period_time);
         if ( DEBUG ) {
           /*if ((rn+1)%60 == 1) {
              fprintf(stderr,"bn, rn = (%d, %d): gratescan: %d  prdscan: %d\n",
                           bn+1,rn+1,gratescan[rn][bn],prdscan[rn][bn]);
           }*/
         }
      }
   }
}
