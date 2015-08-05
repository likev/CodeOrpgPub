/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:16 $
 * $Id: prcpacum_normalize_times.c,v 1.1 2005/03/09 15:43:16 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcpacum_normalize_times.c 

   Description
   ===========
      This function normalizes time in the hour header; period header; and 
   time precipitation last detected in the precipitation status message. The
   definition of normalized time is time in seconds converted to a twenty
   four hour plus orientation. Basically, the headers are searched to for
   the minimum date and maximum dates. If, after the search these dates are
   equal, then normalization is not required since there has not been a day
   turn over. If however there has been a date change then the beginning and
   ending times of each header and the time precipitation last detected in the
   precipitation status message is adjusted. The method of adjustment includes
   adding the current header time to the product of total seconds per day(86400)
   and the difference between the current header day and the minimum day
   number. This will ensure that all ending times are greater than beginning 
   times even for the cases where there has been a day turn over between
   beginning and ending time.

   Change History
   ==============
   02/21/89      0000      P. Pisani            spr # 90067
   02/22/91      0001      Bayard Johnston      spr # 91254
   02/15/91      0001      John Dephilip        spr # 91762
   12/03/91      0002      Steve Anderson       spr # 92740
   12/10/91      0003      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0004      Toolset              spr 91895
   03/25/93      0005      Toolset              spr na93-06801
   01/28/94      0006      Toolset              spr na94-01101
   03/03/94      0007      Toolset              spr na94-05501
   04/11/96      0008      Toolset              ccr na95-11802
   12/23/96      0009      Toolset              ccr na95-11807
   03/16/99      0010      Toolset              ccr na98-23803
   06/30/03      0011      D. Miller            ccr na02-06508
   01/06/05      0012      Cham Pham            ccr NA05-01303
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313h.h>
#include <a313hbuf.h>

/* Local include file */
#include "prcprtac_Constants.h"

void normalize_times( )
{
int curr_per, hour, prd_search; 
int max_init = -1;
int min_init = 999999;
int first = 0;

  if ( DEBUG ) {fprintf(stderr,"A3135K__NORMALIZE_TIMES\n");}

/* Initialze max and min day numbers*/
  blka.max_day_no = max_init;
  blka.min_day_no = min_init;

/* Cycle through new and previous period headers to determine
   whether there has been a change of date. start with first period
   if case is interp but third period if case is extrap.
*/
  if ( blka.cases == EXTRAP ) 
  {
    prd_search = n3;
  }
  else
  {
    prd_search = n1;
  } 

  for ( curr_per=first; curr_per<=prd_search; curr_per++ ) 
  {
    if ( PerdHdr[curr_per].p_beg_date > INIT_VALUE ) 
    { 
       if ( PerdHdr[curr_per].p_beg_date < blka.min_day_no ) 
       {
         blka.min_day_no = PerdHdr[curr_per].p_beg_date;
       }

       if ( PerdHdr[curr_per].p_end_date > blka.max_day_no )
       {
         blka.max_day_no = PerdHdr[curr_per].p_end_date;
       }
    }
  }/* End loop prd_search */

  if (DEBUG) 
  {
   fprintf(stderr,"111: MIN_DAY_NO=%d  MAX_DAY_NO=%d\n",
                  blka.min_day_no,blka.max_day_no);
  }

/* Cycle through hour header to determine whether there has been
   a change of date.
 */
  for ( hour=first; hour<ACZ_TOT_HOURS; hour++ ) 
  {
    if ( HourHdr[hour].h_beg_date > INIT_VALUE ) 
    {
       if ( HourHdr[hour].h_beg_date < blka.min_day_no )
       {
         blka.min_day_no = HourHdr[hour].h_beg_date;
       }

       if ( HourHdr[hour].h_end_date > blka.max_day_no )
       {
         blka.max_day_no = HourHdr[hour].h_end_date;
       }
    }
  }/* End loop ACZ_TOT_HOURS */

  if (DEBUG) 
  {
    fprintf(stderr,"222: MIN_DAY_NO=%d  MAX_DAY_NO=%d\n",
                  blka.min_day_no,blka.max_day_no);
  }

/* Determine whether there has been a change of date for the
   precipitation status message.
 */
  if ( EpreSupl.last_date_rain > INIT_VALUE ) 
  {
    if ( EpreSupl.last_date_rain < blka.min_day_no )
    {
      blka.min_day_no = EpreSupl.last_date_rain;
    }
  }

  blka.time_last_prcp = EpreSupl.last_time_rain;

  if (DEBUG) 
  {
    fprintf(stderr,"333: MIN_DAY_NO=%d\n",blka.min_day_no);
    fprintf(stderr,"TIME_LAST_PRCP = %d\n",blka.time_last_prcp);
  }

/* If there has been a change of date then all begin and end times in the
   period headers (new and previous) are adjusted by adding these times to
   the following factor: number of seconds per day (86400) multiplied by the
   difference between he current begin or end date in the header and the
   minimum day number.
 */
  if ( blka.max_day_no > blka.min_day_no ) 
  {

    for ( curr_per=first; curr_per<=prd_search; curr_per++ ) 
    {
      if ( PerdHdr[curr_per].p_beg_date > INIT_VALUE ) 
      {
        PerdHdr[curr_per].p_beg_time += (PerdHdr[curr_per].p_beg_date-
   			                 blka.min_day_no)*SEC_P_DAY;
        PerdHdr[curr_per].p_end_time += (PerdHdr[curr_per].p_end_date-
                                         blka.min_day_no)*SEC_P_DAY;
        if (DEBUG) 
        {
          fprintf(stderr,"P_BEG_TIME[%d]=%d  P_END_TIME[%d]=%d\n",curr_per,
          PerdHdr[curr_per].p_beg_time,curr_per,PerdHdr[curr_per].p_end_time);
        }
      }
    }/* End loop prd_search */

/* If there has been a date change then the begin and end hours of both current
   and previous hours  will have the following factor added in: number of
   seconds per day multiplied by the difference between the current period
   begin or end time and minimum day number.
 */
    for ( hour=first; hour<ACZ_TOT_HOURS; hour++ ) 
    {
      if ( HourHdr[hour].h_beg_date > INIT_VALUE ) 
      {
        HourHdr[hour].h_beg_time += (HourHdr[hour].h_beg_date-
 				     blka.min_day_no)*SEC_P_DAY;
        HourHdr[hour].h_end_time += (HourHdr[hour].h_end_date-
				     blka.min_day_no)*SEC_P_DAY;
        if (DEBUG) 
        {
          fprintf(stderr,"H_BEG_TIME[%d]=%d  H_END_TIME[%d]=%d\n",hour,
               HourHdr[hour].h_beg_time,hour,HourHdr[hour].h_end_time);
        }
      }
    }/* End loop ACZ_TOT_HOURS */

/* If there has been a date change then the time last precipitation detected
   is adjusted by the following factor: the number of seconds per day multiplied
   by the difference between date last precipitation detected and the minimum
   day number.
 */
    if ( EpreSupl.last_date_rain > INIT_VALUE ) 
    {
      blka.time_last_prcp = EpreSupl.last_time_rain +
                          (EpreSupl.last_date_rain - blka.min_day_no)*SEC_P_DAY;
    }
    else
    {
      blka.time_last_prcp = INIT_VALUE;
    }

  }/* End if block (blka.max_day_no > blka.min_day_no) */

}
