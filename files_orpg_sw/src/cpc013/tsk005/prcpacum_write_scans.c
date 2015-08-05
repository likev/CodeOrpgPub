/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:36 $
 * $Id: prcpacum_write_scans.c,v 1.1 2005/03/09 15:43:36 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_write_scans.c
    Author: Kelley Miles
    Created: 09 DEC 2004

    Description
    ===========
    This function writes various information to the file HYACCUMS.DAT as part
    of the final housekeeping function of function prepare_outputs().  The
    following data is written: 1) current reate scan in the input buffer
    to become the previous rate scan the next time this task is executed;
    2) first period accumulation scan if case is extrapolation; 3) period
    accumulation scan or third period accumulation scan if case is 
    extrapolation; and 4) hourly accumulation scan.

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
    12/09/04      0012      K. Miles             CCR NA05-01303

Included variable descriptions
 FLAG_CLEAR            Constant parameter for a cleared flag   
 HYZ_SUPL              Constant size of supplemental data array
                       in Hybrid Scan output buffer. SSIZ_PRE +
                       SSIZ_RATE + SSIZ_SCUM + SSIZ_ADJU
                       (13+14+16+5) = 48   
 MAX_AZMTHS            Constant maximum number of azimuths in a
                       scan (index into output buffer of adjusted values)   
 MAX_RABINS            Constant maximum number of bins along a
                       radial in the rate scan   
 CASE                  Indicates method for period accumulations,
                       0 indicates method for period accumulations
                       is INTERPOLATION, 1 indicates method is
                       EXTRAPOLATION   
 CURRENT_INDEX         Current index into previous period headers
                       maintained on disk   
 HOUR_HEADER           Relevant information concerning hourly
                       accumulations. Indexed by one symbolic name
                       listed under value, and current or previous hour   
 SCN_FLAG              Defines rain state for current period and
                       past hour. 0 indicates rain for current period
                       and past hour. 1 indicates no rain for current
                       period and past hour.   
 CURR_HOUR             Constant current hour offset into hour header   
 DECR                  Constant value used for decrementing   
 EXTRAP                Constant indicates that method for period
                       accumulations is EXTRAPOLATION   
 FLG_ZERATE            Constant position of flag (zero) in supplemental 
                       data array   
 FLG_ZERREF            Constant position of reference flag (zero) in
                       supplemental data array   
 HLYSCN                Constant flag indicating I/O to be performed on
                       hourly accumulation scan   
 H_FLAG_NHRLY          Constant position of flag (not hourly) in hourly
                       header array   
 H_FLAG_ZERO           Constant position of flag (zero hourly) in hourly
                       header array   
 IO_OK                 Constant parameter defining valid I/O return code   
 MAX_ACUBINS           Constant total number of range bins for accumulation   
 NUM_PREV_PRD          Constant number of previous periods maintained on disk   
 RATSCN                Constant flag indicating I/O to be performed on
                       reference rate scan   
 WRITEREC              Constant write operation parameter for SYSIO call   

****************************************************************************/
/** Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/** Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

void write_scans( short ratescan[][MAX_ACUBINS],
                  short fprdscan[][MAX_ACUBINS],
                  short acumscan[][MAX_ACUBINS],
                  short acumhrly[][MAX_ACUBINS],
                  int *iostat )
{

/*
   acumhrly     Hourly Accumulation Scan array (360x115) (units: mmx100.)
   acumscan     Period Accumulation Scan array (360x115) (units: mmx100.)
   fprdscan     (360x115) I*2 data, contains period accumulation scan data 
                for the first period when the method for calculating period 
                totals is extrapolation.
   iostat       indicates the status of most recent disk I/O. Non-zero
                indicates an error.
   ratescan     offset into input buffer that contains current rate scan of
                360 x 115 I*2 words.
*/
   const int first = 1;    /* Value representing first period index */
   const int second = 2;   /* Constant paramter indicating second period index*/
   const int frst_prd = 2; /* Constant (2) used to back space 2 periods from
                              current index in order to write first period when
                              method for computing period totals is 
                              extrapolation */
   int rec_idx;            /* Indicates which period (1/13) is to be written to
                              disk (HYACCUMS.DAT) */
   int writerec = 1;

   if ( DEBUG ) {fprintf(stderr,"A3135X__WRITE_SCANS\n");}

   *iostat = IO_OK;

   if (DEBUG) 
   {
     fprintf(stderr," ======> CURRENT INDEX ( %d )\n",blka.current_index);
   }
 
/* If current flag zero is clear then write the rate scan from the
   input buffer to disk.
*/
   if ( RateSupl.flg_zerate == FLAG_CLEAR )
   {
      if (DEBUG) 
        {fprintf(stderr,"\nCurrent Rate Scan written to disk\n");}

      *iostat = Scan_IO( writerec, ratscn, ratescan );

/* If I/O status is good and flag zero scan to scan is clear and case
   is extrapolate then write the first period accumulation scan to one
   of {[60min/VCP_RATE]+1} records on disk (i.e., CURRENT_INDEX less two)
 */
      if ( *iostat == IO_OK )
      {
         if ( blka.scn_flag == FLAG_CLEAR )
         {
            if ( (blka.cases == EXTRAP) && (RateSupl.flg_zerref == FLAG_CLEAR) )
            {
             /* Back up to first period index */
               if ( blka.current_index == second )
               {
                  rec_idx = NUM_PREV_PRD;
               }
               else if ( blka.current_index == first )
               {
                  rec_idx = NUM_PREV_PRD + DECR;
               }
               else
               {
                  rec_idx = blka.current_index - frst_prd;
               }

               if (DEBUG) 
               {
                fprintf(stderr,"\nExtrap: first period scan written to disk\n");
               }
               *iostat = Scan_IO( writerec, rec_idx, fprdscan );
            }
         }/* End if block (blka.scn_flag == FLAG_CLEAR) */

/* If I/O status is good then write the last period (for extrap case)
   or new period (interp case) to the current slot for accumulation
   scans on disk
 */
         if ( *iostat == IO_OK )
         {
            rec_idx = blka.current_index;

            if (DEBUG) 
              {fprintf(stderr,"\nPeriod Accum Scan written to disk\n");}

            *iostat = Scan_IO( writerec, rec_idx, acumscan );
         }
      }
   }/* End if block (RateSupl.flg_zerate == FLAG_CLEAR) */

/* If I/O status is good and zero hourly flag is clear and no hourly
   flag is clear then write hourly accumulation scan to disk
 */
   if ( *iostat == IO_OK )
   {
      if ( (HourHdr[curr_hour].h_flag_zero == FLAG_CLEAR ) &&
           (HourHdr[curr_hour].h_flag_nhrly == FLAG_CLEAR ) )
      {
         if (DEBUG) 
           {fprintf(stderr,"\nHourly Accum Scan written to disk\n");}

         *iostat = Scan_IO( writerec, hlyscn, acumhrly );
      }
   }

}
