/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:29:43 $
 * $Id: prcpacum_define_hour.c,v 1.1 2005/03/09 15:29:43 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_define_hour.c 

   Description
   ===========
      This function defines the limits of the current hour. The first function 
   performed is to set the end of hour. As a starting point, the end of hour
   is set to the average scan time/date found in the supplemental data array.
   If a clock hour has occurred between the average and reference scan date 
   found in the supplemental data array, then the hour is set back to the clock
   hour. If the end gage accumulation time now occurs between the end of hour
   (clock or average scan time) and the reference scan time, found in the
   supplemental data array, then the hour is set back to the end gage 
   accumulation time. A flag indicating the type of scan is maintained and 
   defined as follows:
     If set to 0, indicates that end of hour is set to current time;
     If set to 1, indicates end of hour set to previous clock hour;
     If set to 2, indicates end of hour set to end gage accumulation time;
     If set to 3, indicates end of hour set to both previous clock hour and 
                  end gage accumulation time.
     Finally the beginning of the hour is determined by subtracting one from
   the end of hour. If the result is negative then the total hours per day (24)
   are added to the result and one subtracted from the current end of day.
   The begnning and ending times of the hour are then converted from hours,
   minutes, and seconds to seconds and stored in the hour header for the current
   hour as are the beginning and ending dates of the hour. The hourly scan type
   is then stored in the hour header for the current hour.

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
   12/31/02      0011      D. Miller            ccr na00-28601
   01/06/05      0012      Cham Pham            ccr NA05-01303

****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313h.h>
#include <a313hbuf.h>

/* Local include files */
#include "prcprtac_Constants.h"

/* Declare function prototype */
void convert_time( int type, int *sec, int *hh, int *mm, int *ss );

void define_hour( )
{         
int day_end, hour_end, min_end, sec_end,
    day_beg, hour_beg, min_beg, sec_beg,
    day_prev, hour_prev, min_prev, sec_prev,
    day_gage, hour_gage, min_gage, sec_gage,
    time_gage, time_prev, end_hour_type;

/* Convert current and previous scan times from seconds to hours, minutes, and
   seconds. Inputs for seconds are in the supplementary data array of the input
   buffer and are the average and current scan times.  End time defaults to
   current system time. 
   Set previous time as reference scan time from i/p buffer.
   Set previous day from reference scan date from i/p buffer.
   Set end day as average scan date from i/p buffer.
   Set end hour flag as 'ends at current hour'.
*/
  if ( DEBUG ) {fprintf(stderr,"A31354__DEFINE_HOUR\n");}

  convert_time( SEC_HMS, &EpreSupl.avgtime, &hour_end, &min_end, &sec_end );

  convert_time( SEC_HMS, &RateSupl.ref_scntim, &hour_prev,
                                    &min_prev, &sec_prev );
  time_prev = RateSupl.ref_scntim;
  day_prev  = RateSupl.ref_scndat;
  day_end   = EpreSupl.avgdate;
  end_hour_type = END_CURR;

  if ( DEBUG ) 
  {
   fprintf(stderr,"Current scan time (Sec) =%d\n",EpreSupl.avgtime);
   fprintf(stderr,"   Day, Hour, Min, Sec =%d  %d  %d  %d\n",
                  day_end, hour_end, min_end, sec_end);
   fprintf(stderr,"Previous scan time (sec) = %d\n",RateSupl.ref_scntim);
   fprintf(stderr,"   Day, Hour, Min, Sec = %d  %d  %d  %d\n",
                  day_prev, hour_prev, min_prev, sec_prev);
   fprintf(stderr," ... End of hour set to current time\n");
  }

/* If there has been a day or hour change since the previous volume scan then
   set the time back to the previous clock hour.  Also set scan type to
   'ends at clock hour' in the hour header.
 */
  if ( (day_end>day_prev) || ((day_end==day_prev) && (hour_end>hour_prev)) ) 
  {                            
    min_end = INIT_VALUE;
    sec_end = INIT_VALUE;
    end_hour_type = END_CLOCK;

    if ( DEBUG ) 
    {
      fprintf(stderr,"...End of Hour set back to end of Clock Hour!\n");
      fprintf(stderr,"   Day, Hour, Min, Sec = %d  %d  %d  %d\n",
            day_end, hour_end, min_end, sec_end);
    }

  }
/* Preset gage accumulation time to current hour with gage accumulation minutes
   inserted. If end minutes is less than end gage minute, the gage hour is set
   back. If there has been a change of day as a result of setting the hour
   back, the gage day is set back.
 */
  day_gage = day_end;
  hour_gage = hour_end;
  min_gage = blka.gage_accum_tim;
  sec_gage = INIT_VALUE;

  if ( min_end < min_gage ) 
  {
    hour_gage = hour_end + DECR;

    if ( hour_gage < INIT_VALUE ) 
    {
      hour_gage = hour_gage + HR_P_DAY;
      day_gage  = day_gage + DECR;
    }

  }
/* Convert gage accumulation time from hours, minutes, and seconds to seconds.
   If gage accumulation time is greater than previous date/time (i.e. if ending
   gage accumulation occurred between previous & current scans) then set hour
   end date/time to gage date/time.  Also set end hour type to either "end gage"
   or "both".  Set to "end gage" if end hour is "current".  Set to "both" if
   end hour is "clock".
 */
  convert_time( HMS_SEC, &time_gage, &hour_gage, &min_gage, &sec_gage );

  if ( (day_gage>day_prev) || ((day_gage==day_prev) && (time_gage>time_prev)) )
  {

    day_end = day_gage;
    hour_end= hour_gage;
    min_end = min_gage;
    sec_end = sec_gage;

    if ( end_hour_type == END_CURR ) 
    {
      end_hour_type = END_GAGE;

      if (DEBUG)
        {fprintf(stderr,"...End of Hour set back to Ending Gage Hour!\n");}
    }
    else 
    {
      end_hour_type = END_BOTH;

      if (DEBUG)
        {fprintf(stderr,"...End of Hour set to Both End Clock & Gage!\n");}
    }

    if ( DEBUG ) 
    {
      fprintf(stderr,"   Day, Hour, Min, Sec = %d  %d  %d  %d\n",
                             day_end, hour_end, min_end, sec_end);
    }

  }

/* Set day begin to day end; hour begin to hour end less one and minute begin
   to minute end. If the beginning of the  hour is now negative, adjust by
   adding twenty four hours and setting the day back by one.
 */
  day_beg = day_end;
  hour_beg= hour_end + DECR;
  min_beg = min_end;
  sec_beg = sec_end;

  if ( hour_beg < INIT_VALUE ) 
  {
    hour_beg = hour_beg + HR_P_DAY;
    day_beg  = day_beg + DECR;
  }

  if ( DEBUG ) 
  {
   fprintf(stderr,"Beginning of Hour specifics:\n");
   fprintf(stderr,"   Day, Hour, Min, Sec = %d  %d  %d  %d\n",
                   day_beg, hour_beg, min_beg, sec_beg);
  }

/* Convert begin and end hour times from hours, minutes, and seconds to seconds.
   Also set begin and end day into hour header and set hourly sca type to proper
   value in hourly header.
 */
  convert_time( HMS_SEC, &HourHdr[curr_hour].h_end_time,
                          &hour_end, &min_end, &sec_end );
  convert_time( HMS_SEC, &HourHdr[curr_hour].h_beg_time,
                          &hour_beg, &min_beg, &sec_beg );
  HourHdr[curr_hour].h_end_date = day_end;
  HourHdr[curr_hour].h_beg_date = day_beg;
  HourHdr[curr_hour].h_scan_type = end_hour_type;

  if ( DEBUG ) 
  {
    fprintf(stderr," End_Hour_Type = %d ",end_hour_type);
    fprintf(stderr," (end_curr=0, end_clock=1, end_gage=2, end_both=3)");
    fprintf(stderr,"\nEND define_hour() .........\n");
  }

}  
