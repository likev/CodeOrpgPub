/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:43:46 $
 * $Id: prcpacum_prepare_outputs.c,v 1.3 2008/01/04 20:43:46 aamirn Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_prepare_outputs.c
    Author: Kelley Miles
    Created: 14 DEC 2004

    Description
    ===========
    This function controls the final housekeeping functions that are performed
    just prior to putting the task back into the trap wait state. The basic
    function accomplished include: 1) restoring period and hour header times
    to a 24-hour orientation; 2) writing the rate header to disk; 3) writing
    period and hour headers to disk; 4) writing the current rate scan, current
    period accumulation scan, and hourly scan to disk; 5) performing the
    scan-to-scan function if the method for computing period totals is
    extrapolation; and 6) restoring the scan-to-scan and hourly accumulation
    scans in the output buffer to units of MMX10 (from their internal units
    of MMX100 int he rate/accumulation algorithm-task) just prior to release
    of the buffer.

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
    05/23/01      0011      Cheryl Stephenson    CCR NA00-33301,
                                                  issue 1-552
    07/31/02      0012      Dennis Miller        CCR NA02-20701
    12/31/02      0013      D. Miller            CCR NA00-28601
    06/30/03      0014      D. Miller            CCR NA02-06508
    12/14/04      0015      K. Miles             CCR NA05-01303
    10/26/05      0016      C. Pham              CCR NA05-21401

Included variable definitions
   FLAG_CLEAR           Constant. Parameter for a cleared flag
   HYZ_SUPL             Constant. Size of Supplemental Data array in 
                        Hybrid Scan output buffer.
                        SSIZ_PRE + SSIZ_RATE + SSIZ_ACUM +
                        SSIZ_ADJU (= (13+14+16+5) = 48)
   MAX_ACUBINS          Constant. Total number of range bins for accumulation.
   MAX_AZMTHS           Constant. Maximum number of azimuths in a scan 
                        (index into output buffer of adjusted values).
   MAX_RABINS           Constant. Maximum number of bins along a radial in 
                        the rate scan.
   CASE                 Indicates method for period accumlations,
                        0 indicates method for period accumulations is 
                        INTERPOLATION, 1 indicates method is EXTRAPOLATION.
   CURR_HOUR            Constant. Current hour offset into hour header.
   EXTRAP               Constant. Indicates that method for period
                        accumulations is EXTRAPOLATION.
   FLG_ZERATE           Constant. Position of flag (zero) in supplemental data array.
   FLG_ZERREF           Constant. Position of reference flag (zero)
                        in supplemental data array.
   HOUR_HEADER          Relevant information concerning hourly accumulations. 
                        Indexed by one symbolic name listed under value, 
                        and current or previous hour.
   H_FLAG_NHRLY         Constant. Position of flag (not hourly)
                        in hourly header array.
   H_FLAG_ZERO          Constant. Position of flag (zero hourly)
                        in hourly header array.
   RATE_SCALING         Constant. Scaling factor for use in precip
                        rates and accumulations (=10.)

****************************************************************************/
/** Global include files */
#include <rpgc.h>
#include <a313h.h>
#include <a313hbuf.h>

/** Local include file */
#include "prcprtac_Constants.h"

/* Declare function prototypes */
extern void restore_times( void );
extern void write_rate_hdr( int* );
extern void write_hdr_fields( int* );
extern void write_scans( short[MAX_AZMTHS][MAX_RABINS], 
                         short[MAX_AZMTHS][MAX_RABINS], 
                         short[MAX_AZMTHS][MAX_RABINS], 
                         short[MAX_AZMTHS][MAX_RABINS], int* );
extern void scan_to_scan( short[MAX_AZMTHS][MAX_RABINS], 
                          short[MAX_AZMTHS][MAX_RABINS] );

void prepare_outputs( short ratescan[][MAX_RABINS],
                      short fprdscan[][MAX_ACUBINS],
                      short acumscan[][MAX_ACUBINS],
                      short acumhrly[][MAX_ACUBINS],
                      int *iostat )
{
/* acumhrly     Hourly Accumulation Scan array (360x115) (units: mmx100.)
   acumscan     Period Accumulation Scan array (360x115) (units: mmx100.)
   fprdscan     (360x115) I*2 data, contains period accumulation scan data 
                for the first period when the method for calculating period 
                totals is extrapolation.
   iostat       indicates the status of most recent disk I/O. Non-zero
                indicates an error.
   ratescan     offset into input buffer that contains current rate scan of
                360 x 115 I*2 words.
*/

int bn, rn;

   if ( DEBUG ) {fprintf(stderr," A3135U__PREPARE_OUTPUTS\n");}

/* Initialize iostat flag to good status. */
   *iostat = FLAG_CLEAR;

/* Restore times to normal to 24-hour period. */
   restore_times( );

/* Write rate scan header to disk. */
   write_rate_hdr( iostat );

/* If I/O status ok, then write headers period and hourly to disk,
   write new period accumulation scans to disk, and, if current and
   previous zero flags are clear and case is extrapolation, then also
   do scan-to-scan to combine periods one and three. 
 */
   if ( *iostat == FLAG_CLEAR )
   {
      write_hdr_fields( iostat );

      if ( *iostat == FLAG_CLEAR )
      {
         write_scans( ratescan, fprdscan, acumscan, acumhrly, iostat );
      }

      if ( *iostat == FLAG_CLEAR )
      {
         if ( blka.cases == EXTRAP )
         {
            if ( RateSupl.flg_zerate == FLAG_CLEAR &&
                 RateSupl.flg_zerref == FLAG_CLEAR )
            {
               scan_to_scan( fprdscan, acumscan );
            }
         }

/* Now that all disk i/o for present volume scan is complete and the
   output buffers are ready for release, rescale contents of output
   buffer (i.e., Period Accum Scan and Hourly Scan) to mm*10 by dividing
   by 10. 
 */
         /* Period Scan: */
         if ( RateSupl.flg_zerate == FLAG_CLEAR )
         {
            for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
            {
               for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
               {
                /* Note: Changed for LINUX - Used RPGC_NINT library function 
                   instead of adding 0.5 for rounding to the nearest integer.
                 */

                  acumscan[rn][bn] = (short)RPGC_NINT(acumscan[rn][bn]/RATE_SCALING); 
               }
            }
         }

         /* Hourly Scan: */
         if ( HourHdr[curr_hour].h_flag_zero == FLAG_CLEAR &&
              HourHdr[curr_hour].h_flag_nhrly == FLAG_CLEAR )
         {
            for( rn=FIRST_RADIAL; rn<MAX_AZMTHS; rn++ )
            {
               for ( bn=FIRST_BIN; bn<MAX_ACUBINS; bn++ )
               {
               /* Note: Changed for LINUX - Used RPGC_NINT library function 
                  instead of adding 0.5 for rounding to the nearest integer.
                */

                  acumhrly[rn][bn] = (short)RPGC_NINT(acumhrly[rn][bn]/RATE_SCALING);
               }
            }
         }

      }/* End inner if block (*iostat == FLAG_CLEAR) */
   }/* End outer if block (*iostat == FLAG_CLEAR) */
}
