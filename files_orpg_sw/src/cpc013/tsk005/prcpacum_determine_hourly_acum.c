/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:12:38 $
 * $Id: prcpacum_determine_hourly_acum.c,v 1.3 2014/03/18 14:12:38 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_determine_hourly_acum.c
    Author: Kelley Miles
    Created: 05 JAN 2005

    Description
    ===========
    This function controle the computations for hourly totals. The first
    function performed is to determine the method of computing hourly
    totals. This is accomplished by a call to determine_hourly_method()
    Next, the new hourly contributions are added to the current hourly
    total. These new contributions can be from one or three periods
    depending on whether the case is for extrapolation or interpolation.
    This is accomlished by a call to new_hrly_contrib(). Next, control is 
    passed to add_scans() if there is a fraction of the previous hour that
    was not included in the hourly totals followed by a call to
    subtract_prev_hrly() or add_prev_hrly() depending on whether the method
    for determining hourly totals is for subtraction  or addition, respectively.
    Finally, function hrly_outli_corr() and max_hrly_val() are called to
    compute the outlier correction and determine the maximum hourly value,
    respectively.

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
    04/01/95      0008      Toolset              CCR NA95-11802
    06/02/95      0009      R. RIERSON           CCR NA94-35301
    12/23/96      0010      Toolset              CCR NA95-11807
    03/16/99      0011      Toolset              CCR NA98-23803
    12/31/02      0012      D. Miller            CCR NA00-28601
    01/05/05      0013      K. Miles             CCR NA05-01303

Included variable descriptions
   FLAG_CLEAR            Constant. Parameter for a cleared flag
   FLAG_SET              Constant. Parameter for a set flag.
   FLG_SPOT_BLANK        Constant. Parameter for position of Spot 
                         Blanking flag within Supplemental Data array
   FLG_ZERHLY            Constant. Index to flag indicating hourly
                         scan data is zero-filled.
   HYZ_SUPL              Constant. Size of Supplemental
                         Data array in Hybrid Scan output buffer.
                         SSIZ_PRE + SSIZ_RATE + SSIZ_ACUM +
                         SSIZ_ADJU (= (13+14+16+5) = 48)
   MAX_AZMTHS            Constant. Maximum number of azimuths in a
                         scan (index into output buffer of
                         adjusted values).
   CURRENT_INDEX         Current index into previous period
                         headers maintained on disk.
                         (Cyclic, from 1 to [60min/VCP_rate]+1).
   HBUF_EMPTY            Hourly buffer empty flag.
   HourHdr               Relevant information concerning hourly
                         accumulations. Indexed by one symbolic
                         name listed under value, and current or
                         previous hour.
   MAX_HRLY_POSS         Maximum value possible for hourly totals.
   METHOD                Indicates method of hourly accumulations.
                         0 indicates method for hourly totals is
                         addition, 1 indicates method is subtraction.
   PerdHdr               Contains relevant information concerning
                         each period.
   CURR_HOUR             Constant. Current hour offset into hour header.
   FLG_ZERATE            Constant. Position of flag (zero) in
                         supplemental data array.
   HLYSCN                Constant. Flag indicating I/O to be
                         performed on hourly accumulation scan.
   H_FLAG_NHRLY          Constant. Position of flag (not hourly)
                         in hourly header array.
   H_FLAG_ZERO           Constant. Position of flag (zero hourly)
                         in hourly header array.
   H_MAX_HRLY            Constant. Position of maximum hourly
                         accumulation value in hourly header array.
   H_SPOT_BLANK          Constant. (Constant) Index location for
                         Spot Blanking status for hour
   IO_OK                 Constant. Parameter defining valid I/O
                         return code.
   MAX_ACUBINS           Constant. Total number of range bins for
                         accumulation.
   MAX_PER_CENT          Constant. Maximum amount a period can be
                         within the hour (Indicates 100 percent).
   NULL0                 Constant. A value used for initalization
                         and testing.
   PREV_HOUR             Constant. Previous hour offset into hour header
   P_FLAG_MISS           Constant. Position of flag (missing) in
                         period header array.
   P_FLAG_ZERO           Constant. Position of flag (zero period)
                         in period header array.
   P_FRAC                Constant. Position of fraction of period
                         within past hour in period header array.
   READ                  Constant. Read operation parameter for
                         SYSIO call.
   SUB_HRLY_FLG          Constant. Indicates method for computing
                         hourly totals is by subtraction.

Internal Tables/Work Area
   DEL_TIME              Time difference between current & previous
                         volume scans (sec).
   FRAC_DIF              Difference between max_per_cent (1000) and
                         P_FRAC value of period header for current index.

****************************************************************************/
/**  Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/***  Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

/** Declare function prototypes */
void determine_hourly_method(void);
void new_hrly_contrib(short[MAX_AZMTHS][MAX_RABINS],
                      short[MAX_AZMTHS][MAX_RABINS],
                      short[MAX_AZMTHS][MAX_RABINS]);
void add_scans(int,int,short[MAX_AZMTHS][MAX_RABINS],
               short[MAX_AZMTHS][MAX_RABINS]);
void subtract_prev_hrly(short[MAX_AZMTHS][MAX_RABINS],
                        short[MAX_AZMTHS][MAX_RABINS],int*);
void add_prev_hrly(short[MAX_AZMTHS][MAX_RABINS],
                   short[MAX_AZMTHS][MAX_RABINS],int*);
void hrly_outli_corr(short[MAX_AZMTHS][MAX_RABINS]);
void max_hrly_val(short[MAX_AZMTHS][MAX_RABINS]);

void determine_hourly_acum( short prdscan[][MAX_ACUBINS],
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
   prdscan      scratch buffer one which contains 360 x 115 I*2 period
                accumulation scan data for one of previous periods stored
                on disk
*/

int del_time, frac_dif;
int readrec = 0;

   if ( DEBUG ) {fprintf(stderr," A3135E__DETERMINE_HOURLY_ACUM\n");}

/* Determine the method (add/subtract) by which hourly totals will be
   accumulated. Clear I/O status flag to "NO ERROR". Set "FIRST IN
   OUTPUT BUFFER" flag. This flag indicates that the first information
   put in the hourly accumulation buffer will be copied into it.
   Thereafter, new data will be added to existing values. 
 */
   determine_hourly_method();

   if ( DEBUG ) 
   {
     fprintf(stderr,
             "METHOD = %d  (0 = Addition, 1 = Subtraction)\n",blka.method);
   }

   *iostat = IO_OK;
   blka.frst_in_outbuf = FLAG_SET;
   blka.max_hrly_poss = NULL0;

/* If the method is subtraction, then read the previous hourly scan and
   clear the "FIRST IN OUTPUT BUFFER" flag. Also, adjust the maximum
   hourly possible variable. 
 */
   if ( blka.method == SUB_HRLY_FLG )
   {
      if (( HourHdr[prev_hour].h_flag_zero == FLAG_CLEAR ) &&
          ( HourHdr[prev_hour].h_flag_nhrly == FLAG_CLEAR ))
      {
            *iostat = Scan_IO( readrec, hlyscn, acumhrly );

            blka.hbuf_empty = FLAG_CLEAR;
            blka.frst_in_outbuf = FLAG_CLEAR;
            blka.max_hrly_poss += HourHdr[prev_hour].h_max_hrly;
       }
   }

/* If IO_STATUS from previous read is good, then call subroutine to
   control the new contributions to the hour. Possibly one or two new
   periods will be added depending on case; extrapolate or interpolate.
   If there is an I/O error from the previous disk read, then control is
   returned to the calling routine. 
 */
   if ( *iostat == IO_OK )
   {
      if (DEBUG) 
        {fprintf(stderr,"RateSupl.flg_zerate: %d\n",RateSupl.flg_zerate);}

      if ( RateSupl.flg_zerate == FLAG_CLEAR )
      {
         new_hrly_contrib( fprdscan, acumscan, acumhrly );
      }

/* If method is "subtraction" and the current period's fraction is less
   than 1000, then that period's accumulation is read from disk and the
   fraction not added in previously is currently added. 
 */
      if ( HourHdr[prev_hour].h_flag_zero == FLAG_CLEAR )
      {

         if ( blka.method == SUB_HRLY_FLG )
         {
            if (( PerdHdr[blka.current_index].p_frac < MAX_PER_CENT ) &&
                ( PerdHdr[blka.current_index].p_flag_zero == FLAG_CLEAR ) &&
                ( PerdHdr[blka.current_index].p_flag_miss == FLAG_CLEAR ))
            {
               frac_dif = MAX_PER_CENT - PerdHdr[blka.current_index].p_frac;
               del_time = PerdHdr[blka.current_index].p_delt_time;

               *iostat = Scan_IO( readrec, blka.current_index, prdscan );
               if ( *iostat == IO_OK )
               {
                  add_scans( frac_dif, del_time, prdscan, acumhrly );
               }
            }

/* Continue to phase out periods by subtraction once the special case for
   addition above has been executed. 
 */
           if (( *iostat == IO_OK ) && ( blka.hbuf_empty == FLAG_CLEAR ))
           {
              subtract_prev_hrly( prdscan, acumhrly, iostat );
           }
         }
         else
         {

/* Method for hourly totals is "addition" so call the subroutine to control
   this process. 
 */
            add_prev_hrly( prdscan, acumhrly, iostat );
         }

      }
   }

/* Set current zero hour flag to value in hbuf_empty. */

   HourHdr[curr_hour].h_flag_zero = blka.hbuf_empty;
   AcumSupl.flg_zerhly = blka.hbuf_empty;

/* Do hourly outlier correction and calculate maximum hourly value. */

   if ( *iostat == IO_OK )
   {
      if ( HourHdr[curr_hour].h_flag_zero == FLAG_CLEAR )
      {
         hrly_outli_corr( acumhrly );
         max_hrly_val( acumhrly );
      }
   }

/* Set spot blank status. */

   AcumSupl.flg_spot_blank = HourHdr[curr_hour].h_spot_blank;
}
