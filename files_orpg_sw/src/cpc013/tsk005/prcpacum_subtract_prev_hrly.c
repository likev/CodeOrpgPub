/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:12:39 $
 * $Id: prcpacum_subtract_prev_hrly.c,v 1.3 2014/03/18 14:12:39 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_subtract_prev_hrly.c
    Author: Kelley Miles
    Created: 17 DEC 2004

    Description
    ===========
    This function controls the function of subtracting out period scans from
    an hourly running total. The previous period scans are read from disk and
    control is passed to a3135h__subtract_scans to actually do the subtraction.
    This process is performed for all periods within the previous and current
    beginning of the hour.

    Change History
    ============
    DATE          VERSION    PROGRAMMER         NOTES
    ----------    -------    ---------------    ------------------
    02/21/89      0000       P. PISANI          SPR # 90067
    02/22/91      0001       BAYARD JOHNSTON    SPR # 91254
    02/15/91      0002       JOHN DEPHILIP      SPR # 91762
    12/03/91      0003       STEVE ANDERSON     SPR # 92740
    12/10/91      0004       ED NICHLAS         SPR 92637 PDL Removal
    04/24/92      0005       Toolset            SPR 91895
    03/25/93      0006       Toolset            SPR NA93-06801
    01/28/94      0007       Toolset            SPR NA94-01101
    03/03/94      0008       Toolset            SPR NA94-05501
    04/11/96      0009       Toolset            CCR NA95-11802
    12/23/96      0010       Toolset            CCR NA95-11807
    03/16/99      0011       Toolset            CCR NA98-23803
    12/31/02      0012       D. Miller          CCR NA00-28601
    12/17/04      0013       K. Miles           CCR NA05-01303

Included variable descriptions
   FLAG_CLEAR         Constant. Parameter for a cleared flag
   MAX_AZMTHS         Constant. Maximum number of azimuths in a scan 
                      (index into output buffer of adjusted values).
   CURRENT_INDEX      Current index into previous period headers maintained 
                      on disk. (Cyclic, from 1 to [60min/VCP_rate]+1).
   HourHdr            Relevant information concerning hourly accumulations. 
                      Indexed by one symbolic name listed under value, and 
                      current or previous hour.
   new_frac           Contains fraction each period is in the hour, for 
                      previous period scans. Decimal real fraction multiplied 
                      by 1000; stored as an integer value.
   PerdHdr            Contains relevant information concerning each period.
   CURR_HOUR          Constant. Current hour offset into hour header.
   H_BEG_TIME         Constant. Offset into hour header for current or previous 
                      hour for beginning time (in seconds) of the hour.
   INCR               Constant. A value used for incrementing by one.
   IO_OK              Constant. Parameter defining valid I/O return code.
   MAX_ACUBINS        Constant. Total number of range bins for accumulation.
   NULL0              Constant. A value used for initalization and testing.
   NUM_PREV_PRD       Constant. Number of previous periods maintained on disk.
   PREV_HOUR          Constant. Previous hour offset into hour header
   P_BEG_TIME         Constant. Position of beginning time in period header 
                      array.
   P_END_TIME         Constant. Position of end time in period header array.
   P_FLAG_MISS        Constant. Position of flag (missing) in period header 
                      array.
   P_FLAG_ZERO        Constant. Position of flag (zero period) in period header
                      array.
   P_FRAC             Constant. Position of fraction of period within past hour 
                      in period header array.
   readrec            Constant. Read operation parameter for SYSIO call.
****************************************************************************/
/** Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/** Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

/** Declare function prototype */
void subtract_scans( int, short[MAX_AZMTHS][MAX_RABINS], 
                     short[MAX_AZMTHS][MAX_RABINS] );

void subtract_prev_hrly( short prdscan[][MAX_ACUBINS],
                         short acumhrly[][MAX_ACUBINS],
                         int *iostat )
{

/*
     prdscan    scratch buffer one which contains 360 x 115 I*2 period
                accumulation scan for one of the previous period scans.
     acumhrly   offset into output buffer of 360 x 115 I*2 hourly
                accumulation totals.
     iostat     indicates the status of most recent disk I/O. non-zero
                indicates an error.
*/

   int frac_diff, local_index,check_index,
       prev_beg_hour, curr_beg_hour;
   int readrec=0;
   int first=1;
   
   if ( DEBUG ) {fprintf(stderr," A31359__SUBTRACT_PREV_HRLY\n");}

/* Set both current and previous begin hour for local use. Set a local index
   to start at oldest period to work forward while each period start time is
   less than the beginning of the hour and disk I/O status is normal. If these
   condition are true, and each period end time is greater than the beginning
   of the previous hour, and each periods fraction value is greater than zero
   and flags zero and missing are both clear; then the fractional difference
   between value in new_frac and the current period is determined and
   subroutines to read in the requested scan and to subtract scans are called
   respectively.
 */
   prev_beg_hour = HourHdr[prev_hour].h_beg_time;
   curr_beg_hour = HourHdr[curr_hour].h_beg_time;
   *iostat = IO_OK;

/* Initialize testing index (i.e. local index) to oldest index by adding one. */

   local_index = blka.current_index + INCR;
   if ( local_index > NUM_PREV_PRD )
   {
      local_index = first;
   }

   check_index = local_index;

/* Do for iostat is zero and the beginning of each period header is less than
   the beginning of the current hour. */
   for (; ;)
   {
       if ( ( *iostat == IO_OK ) &&
            ( PerdHdr[local_index].p_beg_time < curr_beg_hour ) )
       {
         if ( PerdHdr[local_index].p_end_time > prev_beg_hour )
         {
            if ( PerdHdr[local_index].p_frac > NULL0 )
            {
               if ( ( PerdHdr[local_index].p_flag_zero == FLAG_CLEAR ) &&
                    ( PerdHdr[local_index].p_flag_miss == FLAG_CLEAR ) )
               {
                  frac_diff = PerdHdr[local_index].p_frac -
                              blka.new_frac[local_index];
 
                /* Read period scan off of disk */
                  if (DEBUG) 
                  {
                    fprintf(stderr,
                            "'Period Scan read from disk to be subtracted'\n");
                    fprintf(stderr,"   FRACTIONAL DIFFERENCE %d\n",frac_diff);
                  }

                  *iostat = Scan_IO( readrec, local_index, prdscan );
                  if ( *iostat == IO_OK )
                  {
                     subtract_scans( frac_diff, prdscan, acumhrly );
                  }

               }
            }/* End if (p_frac > NULL0) */
         }

/* Increment index and test. If index greather than 16, set back to 1. */

         local_index = local_index + INCR;
         if ( local_index > NUM_PREV_PRD )
         {
            local_index = first;
         }
         
         if ( local_index != check_index) continue;

/* Exit the infinite loop when local index equal to check index */
         break;
      }

   }/* End infinite for loop */
}
