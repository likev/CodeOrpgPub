/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:54 $
 * $Id: prcpacum_subtract_scans.c,v 1.2 2006/02/09 18:19:54 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_subtract_scans.c
    Author: Kelley Miles
    Created: 15 DEC 2004

    Description
    ===========
    This function subtracts the contents of period scans (360 x 115) from the
    running hourly total also (360 x 115). Two loops are provided. The first
    is used if the fraction of the period within the hour is one hundred
    percent. This loop merelyh subtracts the contents of the period scan
    from the hourly scan. The second loop is used if the fraction of the
    period within the hour is less than one hundred percent. Prior to
    subtracting the period from the hourly scan, the fraction of the period
    within the hour, expressed as a decimal fraction, is first multiplied
    by the period scan value. These two loops are maintained to eliminate the
    possibility of multiplying by one when not necessary, thereby reducing
    the amount of computation in the large (360 x 115) loop.

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
    11/27/00      0011      C. Stephenson        CCR NA00-33301
    12/31/02      0012      D. Miller            CCR NA00-28601
    12/15/04      0013      K. Miles             CCR NA05-01303
    10/26/05      0014      C. Pham              CCR NA05-21401

Included variable descriptions
   FLAG_CLEAR           Constant. Parameter for a cleared flag
   MAX_AZMTHS           Constant. Maximum number of azimuths in a scan 
                        (index into output buffer of adjusted values).
   FIRST_BIN            Constant. Starting bin number for 115 bins.
   FIRST_RADIAL         Constant. Starting radial number for 360 radials.
   MAX_ACUBINS          Constant. Total number of range bins for accumulation.
   MAX_PER_CENT         Constant. Maximum amount a period can be
                        within the hour (Indicates 100 percent).
   HBUF_EMPTY           Hourly buffer empty flag.

Internal Tables/Work Area
   bn                   current range bin in loop for processing all range
                        bins (115).
   rn                   current radial number in loop for processing all
                        radials (360).
   dec_frac             fractional part of period that is within the hour
                        expressed as a decimal fraction.

****************************************************************************/
/** Global include files */
#include <rpgc.h>
#include <a313h.h>
#include <a313hbuf.h>

/** Local include file */
#include "prcprtac_Constants.h"

void subtract_scans( int frac,
                     short prdscan[][MAX_ACUBINS],
                     short acumhrly[][MAX_ACUBINS] )
{

/*   frac       fraction of period within the hour as a whole number
                scaled by 1000.
     acumhrly   offset into output buffer of 360 x 115 I*2 hourly
                accumulation totals.
     prdscan    scratch buffer one which contains 360 x 115 I*2 period
                accumulation scan for one of the previous period scans.
*/

int    rn,bn;
double dec_frac;

   if ( DEBUG ) {fprintf(stderr," A3135H__SUBTRACT_SCANS\n");}

/* Get fraction value passed as a real decimal */

   dec_frac = (double)frac / (double)MAX_PER_CENT;

   if (DEBUG) 
   {
     fprintf(stderr," Dec. fraction of period phased out of hour = %f\n",
                                                               dec_frac);
   }

/* Determine whether all or a portion of the period is to be subtracted
   out of the hourly total. A value of 1000 in frac, the passed argument,
   indicates to subtract the entire period from the hourly total; whereas
   a value of frac less than 1000 indicates to subtract only a fraction of
   the period from the hourly total. Hourly totals are computed for each
   radial (360) and range bin (115). 
 */
   if ( frac == MAX_PER_CENT )
   {
      if (DEBUG)
        {fprintf(stderr," Entire period phased out of hourly accum\n");}

      for ( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
      {
         for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
         {

/* Subtract whole period scan. */
            acumhrly[rn][bn] -= prdscan[rn][bn];
         }
      }
   }
/* Only a fraction of the period is to be subtracted from the hourly totals */

   else
   {
      if (DEBUG) 
        {fprintf(stderr," Fraction of period phased out of hourly accum\n");}

      for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
      {
         for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
         {
            /* Note: Changed for LINUX - Used RPGC_NINT library function instead
                     of adding 0.5 for rounding to the nearest integer.
             */

            acumhrly[rn][bn] = RPGC_NINT(acumhrly[rn][bn] - prdscan[rn][bn] *
                                         dec_frac);
         }
      }
   }

   blka.hbuf_empty = FLAG_CLEAR;
}
