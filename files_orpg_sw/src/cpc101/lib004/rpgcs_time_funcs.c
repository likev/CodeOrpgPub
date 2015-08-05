/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/11 13:59:16 $
 * $Id: rpgcs_time_funcs.c,v 1.4 2006/09/11 13:59:16 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#include <rpgcs.h>

/* Needed and set by tzset function. */
extern long timezone;

#define BASEYR        2440587 

/********************************************************************

   Description:
      Returns the time zone value for the site.  Uses the tzset 
      function which returns the number of seconds west of GMT.  
      The number of seconds is converted to hours and returned to
      the user.

********************************************************************/
int RPGCS_get_time_zone(){

   /* Call this function so that timezone value is set. */
   tzset();

   return( (int) (timezone / SECS_IN_HOUR) );
 
/* End of RPGCS_get_time_zone() */
}
 
/********************************************************************

   Description:
      Returns the current time, in seconds since midnight, and the
      current modified Julian date.

   Outputs:
      ctime - current time, in seconds since midnight.
      cdate - current modified Julian date.

   Returns:
      0 on success, -1 on error.

********************************************************************/
int RPGCS_get_date_time( int *ctime, int *cdate ){

   time_t curr_time = time( NULL );
   int year, month, day, hour, minute, second;

   /* Get the calendar date and clock time from the UNIX time. */
   RPGCS_unix_time_to_ymdhms( curr_time, &year, &month, &day, 
                              &hour, &minute, &second );

   /* Convert the clock time to seconds since midnight. */
   *ctime = hour*3600 + minute*60 + second;

   /* Convert the calendar date to modifed Julian date. */
   RPGCS_date_to_julian( year, month, day, cdate );

   return 0;

/* End of RPGCS_get_date_time() */
}

/********************************************************************
  Description:
     This takes as input the modified Julian date, and returns
     the year, month, and day.   

  Inputs:
     julian_date - Modified Julian date.  Assumes day 1 is 
                   Jan 1, 1970.

  Outputs:
     year - whole year
     month - month number
     day - day number

  Note:
     Reference algorithm from Fleigel and Van Flandern,
     Communications of the ACM, Volume 11, no. 10,
     (October 1968), p 657.

  Assumption:
     Day 1 is Jan 1, 1970.  This routine developed for use with
     Julian date as provided in RDA/RPG radial header.

********************************************************************/
int RPGCS_julian_to_date( int julian_date, int *year, int *month,
                          int *day ){

   int julian_1;
   int   l, n;

   /* Convert Julian date to base year of Julian calendar */
   julian_1 = BASEYR + julian_date;

   /* Compute year, month, and day */
   l = julian_1 + 68569;
   n = 4*l/146097;
   l = l -  (146097*n + 3)/4;
   *year = 4000*(l+1)/1461001;
   l = l - 1461*(*year)/4 + 31;
   *month = 80*l/2447;
   *day = l - 2447*(*month)/80;
   l = *month/11;
   *month = *month + 2 - 12*l;
   *year = 100*(n - 49) + (*year) + l;

   return 0;

/* End of RPGCS_julian_to_date() */
}

/*****************************************************************************

  Description:
     This function converts Gregorian year, month, day to modified Julian
     date.  

  Inputs:
     year - whole year
     month - month number
     day - day number

  Outputs:
     julian_date - Modified Julian date relative to Jan 1, 1970.

  Note:
     Reference algorithm from Fleigel and Van Flandern.

  Assumption:
     Jan 1, 1970 is day 1 of the modified Julian calendar.

*****************************************************************************/
int RPGCS_date_to_julian( int year, int month, int day, int *julian_date ){

   *julian_date = ( 1461*(year + 4800 + (month - 14)/12))/4 +
                  ( 367*(month - 2 - 12 * ( (month - 14)/12 )))/12 -
                  ( 3*((year + 4900 + (month - 14)/12 )/100))/4 +
                    day - 32075;

   /* Must subtract base year to convert from Julian date to Modified
      Julian date. */
   *julian_date -= BASEYR;

   return 0;

/* End of RPGCS_date_to_julian() */
}
   
/******************************************************************************
   Description: 
      Conversion between the UNIX time and the (year, month, day, hour, 
      minute, second) representation. "year", "month", "day", "hour",
      "minute" and "second" are converted to UNIX time and the result is
      stored in "time".

   Inputs:
      year - year (e.g. 1995; >= 1970)
      month - month number (1 through 12)
      day - day number (1, 2, 3, ...)
      hour - hour (0 through 23)
      minute - minute (0 through 59)
      second - second (0 through 59)
   
   Outputs:
      time - the UNIX time;
    
   Returns: 
      The function returns 0 on success or -1 on failure (an
      argument is out of range).

   Notes:
      See misc_unix_time.c in cpc100/lib004.

******************************************************************************/
int RPGCS_ymdhms_to_unix_time( time_t *time, int year, int month, int day, 
                               int hour, int minute, int second ){
   
   /* Ensure that the time value is zero. */
   if( *time != 0 )
      *time = 0;

   return( unix_time( time, &year, &month, &day, &hour, &minute, &second ) );

/* End of RPGCS_ymdhms_to_unix_time() */
}

/******************************************************************************
   Description: 
      Conversion between the UNIX time and the (year, month,
      day, hour, minute, second) representation. If "time" is
      non-zero, this function returns the year, month,
      day, hour, minute and second as converted from "time".
   
   Inputs:
      time - the UNIX time;

   Outputs:
      year - year (e.g. 1995; >= 1970)
      month - month number (1 through 12)
      day - day number (1, 2, 3, ...)
      hour - hour (0 through 23)
      minute - minute (0 through 59)
      second - second (0 through 59)
    
   Returns: 
      The function returns 0 on success or -1 on failure (an
      argument is out of range).

   Notes:
      See misc_unix_time.c in cpc100/lib004.

******************************************************************************/
int RPGCS_unix_time_to_ymdhms( time_t time, int *year, int *month, int *day, 
                               int *hour, int *minute, int *second ){

   /* Insure the time value is not zero. */
   if( time == 0 )
      return(-1);

   return( unix_time( &time, year, month, day, hour, minute, second ) );

/* End of RPGCS_unix_time_to_ymdhms() */
}

/******************************************************************************

   Description:
      Function to convert radial time, in milliseconds, the hours, minutes, 
      and seconds.

   Inputs:
      time - time, in number of milliseconds since mignight

   Outputs:
      hour - clock hour (0 - 23)
      minutes - number of minutes past hour (0 - 59)
      seconds - number of seconds past minute (0 - 59)
      mills - number of milliseconds

   Returns:
      Returns 0 on success and -1 on error.

*******************************************************************************/
int RPGCS_convert_radial_time( unsigned int time, int *hour, int *minute, 
                               int *second, int *mills ){
   
   int timevalue = time;

   /* Time value must be in the range 0 - 86400000. */
   if( time > 86400000 )
     return(-1);

   /* Extract the number of hours. */
   *hour = timevalue/3600000;
  
   /* Extract the number of minutes. */
   timevalue -= (*hour)*3600000;
   *minute = timevalue/60000;
   
   /* Extract the number of seconds. */
   timevalue -= (*minute)*60000;
   *second = timevalue/1000;

   /* Extract the number of milliseconds. */
   *mills = time % 1000;
   
   /* Put hour, minute, and second in range. */
   if( *hour == 24 )
      *hour = 0;

   if( *minute == 60 )
      *minute = 0;

   if( *second == 60 )
      *second = 0;

   return 0;

/* End of RPGCS_convert_radial_time() */
}

/******************************************************************

   Description:
      Computes the time span between end time and start time (given
      the start time after midnight in seconds, the start date (Julian),
      the end date, and end time. )

   Inputs:
      start_date - starting Date (modified Julian)
      start_time - starting time (seconds since midnight)
      end_date - ending Date (modified Julian)
      end_time - ending time (seconds since midnight)

   Outputs:

   Returns:
      Returns the number of seconds between starting date/time
      and ending date time, or RPGCS_ERROR on error.

***************************************************************/
time_t RPGCS_time_span( int start_date, int start_time,
                        int end_date, int end_time ){

   int date_difference = end_date - start_date;
   time_t time_difference = 0;

   /*check if end date falls before start date */
   if( date_difference < 0 )
      return( RPGCS_ERROR );

   /*compute the time difference in seconds */
   time_difference = (time_t) ((date_difference-1)*SECS_IN_DAY + 
                     (SECS_IN_DAY - start_time) + end_time);
   return( time_difference );

/* End RPGCS_time_span() */
}

