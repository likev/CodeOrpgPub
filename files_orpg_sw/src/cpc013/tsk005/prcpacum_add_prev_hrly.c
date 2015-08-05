/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/18 14:12:38 $
 * $Id: prcpacum_add_prev_hrly.c,v 1.3 2014/03/18 14:12:38 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_add_prev_hrly.c
    Author: Kelley Miles
    Created: 15 DEC 2004

    Description
    ===========
    This function controls the addition of period scan accumulations to hourly
    accumulations. Function Scan_IO() is called to read in a previous
    accumulation scan and function add_scans() is called to actually add the
    contents of the period to hourly scan. This process is performed for all
    non-missing, non-zero periods that are between the current beginning of
    the hour and the current index.

    Change History
    ============
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
    02/04/05      0012      K. Miles, C. Pham    CCR NA05-01303

Included variable descriptions
   FLAG_CLEAR           Constant. Parameter for a cleared flag
   MAX_AZMTHS           Constant. Maximum number of azimuths in a scan 
                        (index into output buffer of adjusted values).
   CURRENT_INDEX        Current index into previous period headers 
                        maintained on disk.
                        (Cyclic, from 1 to [60min/VCP_rate]+1).
   HourHdr              Relevant information concerning hourly
                        accumulations. Indexed by one symbolic name 
                        listed under value, and current or previous hour.
   NEW_FRAC             Contains fraction each period is in the hour, 
                        for previous period scans.
                        Decimal real fraction multiplied by 1000;
                        stored as an integer value.
   PerdHdr              Contains relevant information concerning each period.
   CURR_HOUR            Constant. Current hour offset into hour header.
   DECR                 Constant. A value used for decrementing.
   H_BEG_TIME           Constant. Offset into hour header for current or 
                        previous hour for beginning time (in seconds) of the 
                        hour.
   IO_OK                Constant. Parameter defining valid I/O return code.
   MAX_ACUBINS          Constant. Total number of range bins for accumulation.
   NULL0                Constant. A value used for initalization and testing.
   NUM_PREV_PRD         Constant. Number of previous periods maintained on disk.
   P_END_TIME           Constant. Position of end time in period header array.
   P_FLAG_MISS          Constant. Position of flag (missing) in period header 
                        array.
   P_FLAG_ZERO          Constant. Position of flag (zero period) in period 
                        header array.
   READ                 Constant. Read operation parameter for SYSIO call.

****************************************************************************/
/** Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/**  Local include files */
#include "prcprtac_file_io.h"
#include "prcprtac_Constants.h"

/* Declare function prototype */
extern void add_scans( int, int, short[MAX_AZMTHS][MAX_RABINS], 
                       short[MAX_AZMTHS][MAX_RABINS] );

void add_prev_hrly( short prdscan[][MAX_ACUBINS],
                    short acumhrly[][MAX_ACUBINS],
                    int *iostat )
{

/*   acumhrly   offset into output buffer of 360 x 115 I*2 hourly
                accumulation totals.
     iostat     indicates the status of most recent disk I/O. non-zero
                indicates an error.
     prdscan    scratch buffer one which contains 360 x 115 I*2 period
                accumulation scan for one of the previous period scans.
*/

   int frac_diff, del_time;
   int curr_beg_hour;
   int local_index;
   int readrec=0;

   if ( DEBUG ) 
     {fprintf(stderr,"  A3135F__ADD_PREV_HRLY\n");}

/* Set the current begin hour from hour header into a local variable.
   Set up a local index to search the period headers starting from the
   current period and going back in time to the oldest period. Subroutines
   to read a previous scan and add the scans are called respectively while
   each period's end time is greater than the beginning of the current
   hour and the period's zero and missing flags are clear. 
 */
   curr_beg_hour = HourHdr[curr_hour].h_beg_time;
   local_index = blka.current_index;;

   if ( DEBUG ) 
   {
     fprintf(stderr,"CURR_BEG_HOUR = %d   LOCAL_INDEX = %d\n",
                     curr_beg_hour,local_index);
   }

   *iostat = IO_OK;

/* Do for iostat is zero and period header's end time is greater than
   the beginning of the current hour. 
 */
   for (; ;)
   {
      if ( ( *iostat == IO_OK ) &&
           ( PerdHdr[local_index].p_end_time > curr_beg_hour ) )
      {
         if ( ( PerdHdr[local_index].p_flag_zero == FLAG_CLEAR ) &&
              ( PerdHdr[local_index].p_flag_miss == FLAG_CLEAR ) )
         {
            frac_diff = blka.new_frac[local_index];
            del_time = PerdHdr[local_index].p_delt_time;

            if (DEBUG) 
              {fprintf(stderr,"Period Scan read from disk to be added\n");}

          /* Read period scan off of disk */
            *iostat = Scan_IO( readrec, local_index, prdscan );

            if ( *iostat == IO_OK )
            {
             /* Add portion of period scan to hourly accumulation */
               add_scans( frac_diff, del_time, prdscan, acumhrly );
            }
         }

       /* Decrement local index and test. If index is zero, then set ahead to
          max number of periods in hour. 
        */
         local_index = local_index + DECR;

         if ( local_index == NULL0 )
         {
            local_index = NUM_PREV_PRD;
         }

         if ( local_index != blka.current_index ) continue;
      }
    /* Exit infinite loop when local index equals to current index */
      break;

   }/*End infinite for loop */
}
