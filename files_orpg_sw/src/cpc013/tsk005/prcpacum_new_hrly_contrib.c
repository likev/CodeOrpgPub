/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:12:39 $
 * $Id: prcpacum_new_hrly_contrib.c,v 1.3 2014/03/18 14:12:39 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_new_hrly_contrib.c
    Author: Kelley Miles
    Created: 19 DEC 2004

    Description
    ===========
    This function controls the addition of new hourly contributions to the hour.
    The new contributions are made from the first or the first and third periods
    depending on the case; extrapolation or interpolation. If the case is for
    interpolation, then new hourly contributions are made from the first period
    only. If this case is for extrapolation, then new hourly contributions are
    made first from the third period (current scan) if the p_frac value for the
    period is not zero and then from the first period if the flag zero reference
    is clear.

    Change History
    ============
    DATE          VERSION   PROGRAMMER           NOTES
    ----------    -------   ----------------     ---------------- ----
    02/21/89      0000      P. PISANI            SPR # 90067
    02/22/91      0001      BAYARD JOHNSTON      SPR # 91254
    02/15/91      0001      JOHN DEPHILIP        SPR # 91762
    12/03/91      0002      STEVE ANDERSON       SPR # 92740
    12/10/91      0003      ED NICHLAS           SPR 92637 PDL Re moval
    04/24/92      0004      Toolset              SPR 91895
    03/25/93      0005      Toolset              SPR NA93-06801
    01/28/94      0006      Toolset              SPR NA94-01101
    03/03/94      0007      Toolset              SPR NA94-05501
    04/11/96      0008      Toolset              CCR NA95-11802
    12/23/96      0009      Toolset              CCR NA95-11807
    03/16/99      0010      Toolset              CCR NA98-23803
    12/31/02      0011      D. Miller            CCR NA00-28601
    12/19/04      0012      K. Miles             CCR NA05-01303


Included variable descriptions

   FLAG_CLEAR           Constant. Parameter for a cleared flag
   HYZ_SUPL             Constant. Size of Supplemental Data array in 
                        Hybrid Scan output buffer.
                        SSIZ_PRE + SSIZ_RATE + SSIZ_ACUM +
                        SSIZ_ADJU (= (13+14+16+5) = 48)
   MAX_AZMTHS           Constant. Maximum number of azimuths in a scan 
                        (index into output buffer of adjusted values).
   CASE                 Indicates method for period accumlations,
                        0 indicates method for period accumulations is 
                        INTERPOLATION, 1 indicates method is EXTRAPOLATION.
   PerdHdr              Contains relevant information concerning each period.
   FLG_ZERREF           Constant. Position of reference flag (zero) in 
                        supplemental data array.
   INTERP               Constant. Indicates that method for period
                        accumulations is INTERPOLATION.
   MAX_ACUBINS          Constant. Total number of range bins for accumulation.
   N1                   Constant. Index for first new period out of a possible 
                        three added.
   N3                   Constant. Index for third new period out of a possible 
                        three added.
   NULL0                Constant. A value used for initalization and testing.
   P_FRAC               Constant. Position of fraction of period within past 
                        hour in period header array.

Internal tables/Work Area
  DEL_TIME           Time difference between current & previous volume scans 
                     (sec).
  FRAC               Local variable that contains P_FRAC value from first or 
                     third new period added.

****************************************************************************/
/**  Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/**  Local include file */
#include "prcprtac_Constants.h"

/** Declare function prototype */
void add_scans( int, int, short[MAX_AZMTHS][MAX_RABINS], 
                short[MAX_AZMTHS][MAX_RABINS] );

void new_hrly_contrib( short fprdscan[][MAX_ACUBINS],
                       short acumscan[][MAX_ACUBINS],
                       short acumhrly[][MAX_ACUBINS] )
{

/*
   ACUMHRLY        Hourly Accumulation Scan array (360x115) (units: mmx100.)
   ACUMSCAN        Period Accumulation Scan array (360x115) (units: mmx100.)
   FPRDSCAN        (360x115) I*2 data, contains period scan accumulation data 
                   for the first period when the method for calculating period
                   totals is extrapolation.
*/

int frac, del_time;

   if ( DEBUG ) {fprintf(stderr,"A3135G__NEW_HRLY_CONTRIB\n");}

/* If case is interpolation, then only one new period will be added to the 
   hourly accumulations. */

   if ( blka.cases == INTERP )
   {
      frac = PerdHdr[n1].p_frac;
      del_time = PerdHdr[n1].p_delt_time;
      if (DEBUG) 
      {
       fprintf(stderr,
       "PERIOD SCAN ADDED TO HOURLY ACCUM: FRACTION= %d DEL_TIME (SECS)= %d\n",
                                                      frac,del_time);
      }

      add_scans( frac, del_time, acumscan, acumhrly );
   }

/* Case is extrapolation therefore potentially periods 1 and 3 must be added. */
   
   else
   {

/* Get fraction of period in hour from third new period added. If this value is
   not zero, then third new period will be added to hourly totals. 
 */
      frac = PerdHdr[n3].p_frac;
      del_time = PerdHdr[n3].p_delt_time;

      if ( frac > NULL0 )
      {
         if (DEBUG) 
         {
          fprintf(stderr,
          "PERIOD SCAN ADDED TO HOURLY ACCUM: FRACTION= %d DEL_TIME(SECS)=%d\n",
          frac,del_time);
         }

         add_scans( frac, del_time, acumscan, acumhrly );
      }

/* Prior to adding in first new period, determine whether reference zero rate 
   flag is set. If set, then first new period is not added to hourly totals. 
 */
      if ( RateSupl.flg_zerref == FLAG_CLEAR )
      {
         frac = PerdHdr[n1].p_frac;
         del_time = PerdHdr[n1].p_delt_time;

         if (DEBUG) 
         {
          fprintf(stderr,
           "PERIOD SCAN ADDED TO HOURLY ACCUM: FRACTION=%d DEL_TIME(SECS)=%d\n",
           frac,del_time);
         }

         add_scans( frac, del_time, fprdscan, acumhrly );
      }

   }
}
