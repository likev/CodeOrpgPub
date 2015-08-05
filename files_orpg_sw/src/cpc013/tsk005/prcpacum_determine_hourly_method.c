/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:42:56 $
 * $Id: prcpacum_determine_hourly_method.c,v 1.1 2005/03/09 15:42:56 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/****************************************************************************
    Filename: prcpacum_determine_hourly_method.c
    Author: Kelley Miles
    Created: 20 DEC 2004

    Description
    ===========
    This function determines the method for calculating hourly totals. The 
    result will cause flag 'method' to be set to either of two methods;
    addition or subtraction. The addition method will cause the period scans
    for all periods within the current hour to be added. The subtraction
    method will cause the period scans for the new hourly contributions to
    be added to arunning hourly total followed by the location of all periods
    between the previous and current begin hour which will be phased out by
    subtracting these period totals from the running hourly total. The first
    step in this process is to determine the existance of the following
    unique cases which will cause the method for computing hourly totals to
    be set to 'addition'.

       1) Current case is extrapolation and previous end hour type is
          'ends at current hour'.

       2) Either FLAG ZERO or FLAG NO HOURLY for the previous hour set.

       3) Previous case is extraploation and previous end hour type is
          'not ends at current hour'.

    If one of these three cases is not present, then the period headers are
    searched to determine the total number of periods between the current
    index and the current beginning of the hour and the total number of periods
    between the current index and the current beginning of the hour. These two
    counts are then compared and if the total number of periods between the
    current index and the current beginningof the hour is greater than the
    number of periods between the previous and current beginning of the hour,
    then the method is set to subtraction. Otherwise, it is set to addition.
    If the counts are equal, then the method is set to addition.

    Change History
    ============
    DATE          VERSION   PROGRAMMER           NOTES
    ----------    -------   ----------------     --------------------
    02/21/89      0000      P. PISANI            SPR # 90067
    02/22/91      0001      PAUL JENDROWSKI      SPR # 91254
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
    07/31/02      0011      Dennis Miller        CCR NA02-20701
    12/31/02      0012      D. Miller            CCR NA00-28601
    12/20/04      0013      K. Miles             CCR NA05-01303


Included variable descriptions

   END_CURR             Constant. Used to initialize scan type indicator for 
                        normal scan.
   FLAG_CLEAR           Constant. Parameter for a cleared flag
   FLAG_SET             Constant. Parameter for a set flag.
   CASE                 Indicates method for period accumlations, 0 indicates 
                        method for period accumulations is INTERPOLATION, 1
                        indicates method is EXTRAPOLATION.
   CURRENT_INDEX        Current index into previous period headers maintained on 
                        disk.  (Cyclic, from 1 to [60min/VCP_rate]+1).
   HourHdr              Relevant information concerning hourly accumulations. 
                        Indexed by one symbolic name listed under value, and 
                        current or previous hour.
   PerdHdr              Contains relevant information concerning each period.
   CURR_HOUR            Constant. Current hour offset into hour header.
   DECR                 Constant. A value used for decrementing.
   EXTRAP               Constant. Indicates that method for period 
                        accumulations is EXTRAPOLATION.
   H_BEG_TIME           Constant. Offset into hour header for current or 
                        previous hour for beginning time (sec) of the hour.
   H_FLAG_CASE          Constant. Position of case indicator in hourly header 
                        array.
   H_FLAG_NHRLY         Constant. Position of flag (not hourly) in hourly 
                        header array.
   H_FLAG_ZERO          Constant. Position of flag (zero hourly) in hourly 
                        header array.
   H_SCAN_TYPE          Constant. Position of scan type in hourly header array.
   INCR                 Constant. A value used for incrementing by one.
   MAX_PER_CENT         Constant. Maximum amount a period can be within the hour 
                        (Indicates 100 percent).
   NULL                 Constant. A value used for initalization and testing.
   NUM_PREV_PRD         Constant. Number of previous periods maintained on disk.
   PREV_HOUR            Constant. Previous hour offset into hour header
   P_BEG_TIME           Constant. Position of beginning time in period header 
                        array.
   P_END_TIME           Constant. Position of end time in period header array.
   P_FLAG_MISS          Constant. Position of flag (missing) in period header 
                        array.
   P_FLAG_ZERO          Constant. Position of flag (zero period) in period 
                        header array.
   P_FRAC               Constant. Position of fraction of period within past 
                        hour in period header array.
   ADD_HRLY_FLG         Indicates that method for computing hourly totals is 
                        addition.
   SUB_HRLY_FLG         Constant. Indicates method for computing hourly totals 
                        is by subtraction.
   METHOD               Indicates method of hourly accumulations.
                        0 indicates method for hourly totals is addition,
                        1 indicates method is subtraction.

Internal Tables/Work Area
 ADD_PRD_CNT         Counter of the number of periods between the period pointed 
                     to by the current index and period that contains the 
                     current begin hour.
 CHECK_INDEX         Variable used to check the local index so that it does not 
                     overflow maximum number of periods.
 FIRST               Variable used to initialize the local index to one.
 LOCAL_INDEX         Value set to current index used to control searching  
                     periods in the previous hour.
 SUB_PRD_CNT         Counter of number of periods between the previous begin 
                     hour and the current begin hour.

****************************************************************************/
/* Global include files */
#include <a313h.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

void determine_hourly_method( )
{

/* The hardwire fix employed in this module is to correct "Precipitation 
   Residuals" - small, anomalous precipitation amounts that sometimes remained 
   in accumulation products after a discrete rainfall event was over. These 
   were only manifested when the method of "Addition and Subtraction" was 
   employed (as opposed to "Addition" only). Both methods are compliant with 
   the requirements of the Precipitation Accumulation Algorithm Report. 
   The old, untested code is left in place, commented out, in the consideration 
   that the method of Addition and Subtraction is far superior, computationally,
   and perhaps could be resurrected some time in the future, if the problems  
   that cause the appearance of the Residuals are overcome.  
*/
/*----------------------
   int local_index, prd_index, first=1;
   int add_prd_cnt, sub_prd_cnt;
   int check_index;

   Evaluate past period header to determine whether more processing would have
   to be done to determine new hourly totals by adding periods shared with
   previous hour or by subtracting periods phased out. The following code
   determines some special cases for the "add" method. These include:

   1) No 'previous hour' or 'zero previous hour' flag set.

   if (( HourHdr[prev_hour].h_flag_zero  == FLAG_SET ) ||
      ( HourHdr[prev_hour].h_flag_nhrly  == FLAG_SET ))
   {
      blka.method = ADD_HRLY_FLG;
   }

   2) Current case is for extrapolation and end hour type is "ends at current 
      hour"

   else if (( blka.cases == EXTRAP ) &&
            ( HourHdr[curr_hour].h_scan_type == END_CURR ))
   {
      blka.method = ADD_HRLY_FLG;
   }

   3) Previous case is extrapolate and previous end hour type is not "ends at
      current hour"

   else if (( HourHdr[prev_hour].h_flag_case == EXTRAP ) &&
            ( HourHdr[prev_hour].h_scan_type != END_CURR ))
   {
      blka.method = ADD_HRLY_FLG;
   }
   else
   {

   Cycle through the period headers starting with the most current period and
   working backwards while periods end after the beginning time of the current
   hour. If this condition is met and FLAG ZERO and FLAG MISSING are both clear,
   "ADD" counter is incremented by one. At the end of the function, the "ADD"
   and "SUB" counters are compared to determine the method for hourly 
   accumulations. 

      add_prd_cnt = NULL;

   Do for all periods from current to oldest while periods end after the
   beginning of the new hour. 

      while ( local_index != blka.current_index )
      {
         local_index = blka.current_index;
         if ( HourHdr[curr_hour].h_beg_time  < 
                        PerdHdr[local_index].p_end_time )
         {
            if (( PerdHdr[local_index].p_flag_zero == FLAG_CLEAR ) &&
                ( PerdHdr[local_index].p_flag_miss == FLAG_CLEAR ))
            {
               add_prd_cnt = add_prd_cnt + INCR;
            }
            local_index = local_index + DECR;
            if ( local_index == NULL )
            {
               local_index = NUM_PREV_PRD;
            }
         }
      }
   }

   Cycle through the periods starting with the oldest working forward while
   periods are between the current begin hour and the previous begin hour.
   If this condition is met and the zero and missing flags for the period are
   clear, and frac is non-zero, then a "sub" counter is incremented. This
   counter will be compared with an "add" counter at the end of the function
   to determine the method of hourly accumulation.

   local_index = blka.current_index + INCR;
   if ( local_index > NUM_PREV_PRD )
   {
      local_index = first;
   }
   sub_prd_cnt = NULL;

   Do from oldest period to most current while the period end is after the
   beginning of the previous hour and the period start is before the
   beginning of the new hour. 

   while ( local_index != check_index )
   {
      check_index = local_index;
      if ( PerdHdr[local_index].p_beg_time <
           HourHdr[curr_hour].h_beg_time )
      {
         if ( PerdHdr[local_index].p_end_time >
              HourHdr[prev_hour].h_beg_time )
         {
            if (( PerdHdr[local_index].p_flag_zero == FLAG_CLEAR ) &&
                ( PerdHdr[local_index].p_flag_miss == FLAG_CLEAR ) &&
                ( PerdHdr[local_index].p_frac > NULL ))
            {
               sub_prd_cnt = sub_prd_cnt + INCR;
            }
         }
        local_index = local_index + INCR;
        if ( local_index > NUM_PREV_PRD )
        {
           local_index = first;
        }
      }

   Special case (for subtraction only) - Determine whether the current period's
   fraction value in the period header is less than 1000. If it is, then 
   increment the subtraction count by one. The reason for this is that some 
   percentage of the current period must be added into the total. Therefore, 
   the "addition method" must be appropriately more favorably weighted.

      if ( PerdHdr[blka.current_index].p_frac < MAX_PER_CENT )
      {
         if (( PerdHdr[blka.current_index].p_flag_zero == FLAG_CLEAR ) &&
              (PerdHdr[blka.current_index].p_flag_miss == FLAG_CLEAR ))
         {
            sub_prd_cnt = sub_prd_cnt + INCR;
         }
      }

   Compare the "addition counter" with the "subtraction counter". Whichever is
   less determines the method. 

      if ( sub_prd_cnt > add_prd_cnt )
      {
         blka.method = ADD_HRLY_FLG;
      }
      else
      {
         blka.method = SUB_HRLY_FLG;
      }
   }
----------------------*/

/* The hardwire fix employed in this module is to correct "Precipitation 
   Residuals" - small, anomalous precipitation amounts that sometimes remained 
   in accumulation products after a discrete rainfall event was over. These 
   were only manifested when the method of "Addition and Subtraction" was 
   employed (as opposed to "Addition" only). Both methods are compliant with 
   the requirements of the Precipitation Accumulation Algorithm Report. 

   The old, untested code is left in place, commented out, in the consideration 
   that the method of Addition and Subtraction is far superior, computationally,
   and perhaps could be resurrected some time in the future, if the problems  
   that cause the appearance of the Residuals are overcome.  
 */

   blka.method = ADD_HRLY_FLG;
}
