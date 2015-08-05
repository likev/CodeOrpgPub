/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:57 $
 * $Id: time_functions.c,v 1.6 2009/05/15 17:37:57 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/* time_functions.c */
#include "time_functions.h"


#define SECS_PER_DAY    86400

short orpg_date (
    time_t  seconds)            /* time_t (seconds since 1/1/70)    */
{
    return (seconds/SECS_PER_DAY + 1);
}



short orpg_time (
    time_t  seconds)            /* seconds since 1/1/70         */
{
    return (seconds % SECS_PER_DAY * 1000);
}



time_t unix_time (
    short   date,               /* days since January 1, 1970       */
                                /* (1/1/70 is represented as 1, not 0)  */
    int     time)               /* milliseconds past midnight       */
{
    return ((time_t) ((date-1)*SECS_PER_DAY + time/1000));
}


void calendar_date (
    short   date,               /* days since January 1, 1970       */
    int     *dd,                /* OUT:  day                */
    int     *dm,                /* OUT:  month              */
    int     *dy )               /* OUT:  year               */
{

    int         l,n, julian;

    /* Convert modified julian to type integer */
    julian = date;

    /* Convert modified julian to year/month/day */
    julian += 2440587;
    l = julian + 68569;
    n = 4*l/146097;
    l = l -  (146097*n + 3)/4;
    *dy = 4000*(l+1)/1461001;
    l = l - 1461*(*dy)/4 + 31;
    *dm = 80*l/2447;
    *dd= l -2447*(*dm)/80;
    l = *dm/11;
    *dm = *dm+ 2 - 12*l;
    *dy = 100*(n - 49) + *dy + l;
/*    *dy = *dy - 1900;*/   

    return;
}


char *LatLon_to_DdotD (
    int     LatLon)             /* Angle in degrees         */
{
    static char     LatLon_str[10];

    (void) sprintf(LatLon_str, "%3.3f\"", LatLon / 1000.0);
    return(LatLon_str);
}


char *LatLon_to_DDDMMSS (
    int     LatLon)             /* Angle in degrees         */
{
    int         degrees;
    int         minutes;
    int         seconds;
    int         remainder;
    static char     LatLon_str[20];

    degrees = LatLon / 1000;
    remainder   = LatLon % 1000;

    minutes = remainder / 60;
    remainder   = remainder % 60;

    seconds = remainder / 60;

    (void) sprintf(LatLon_str, "%3d %2d' %2d\"", degrees, minutes, seconds);
    return(LatLon_str);
}



char *msecs_to_string (
    int     time)               /* milliseconds since midnight      */
{
    int         h, m, s, frac;
    static char     stime[20];

    frac =  time - 1000*(time/1000);
    time =  time/1000;
    h =     time/3600;
    time =  time - h*3600;
    m =     time/60;
    s =     time - m*60;

    (void) sprintf(stime, "%2d:%02d:%02d", h, m, s);
    
    return(stime);
}



char *date_to_string (
    short   date)               /* days since January 1, 1970       */
{
    int         dd, dm, dy;
    char        *month = "UNDEFINED";

    static char     sdate[30];


    calendar_date( date, &dd, &dm, &dy );
    switch (dm) {
    case 1:     month = "January";  break;
    case 2:     month = "February"; break;
    case 3:     month = "March";    break;
    case 4:     month = "April";    break;
    case 5:     month = "May";      break;
    case 6:     month = "June";     break;
    case 7:     month = "July";     break;
    case 8:     month = "August";   break;
    case 9:     month = "September";    break;
    case 10:    month = "October";  break;
    case 11:    month = "November"; break;
    case 12:    month = "December"; break;
    default:    (void) sprintf(sdate, "** %02d/%02d/%02d **\n", dm, dd, dy);
    
    }
/* CVT 4.4 BUG FIX - us 'dy' directly in output */
/*    year = 1900 + dy; */
    (void) sprintf(sdate, "%s %d, %d", month, dd, dy);
    return(sdate);
}


/*** this function is not used anywhere in cvt ***/
/* char* format_date_time(time_t time) { */
/*   #define TIME_FORMAT_STRING "%m/%d/%y %H:%M" */
/*   #define TIME_ARRAY_LEN 15 */
/*  */
/*   struct tm *tm_p; */
/*   char time_array[TIME_ARRAY_LEN]; */
/*  */
/*   tm_p=localtime((const time_t *) &time); */
/*   (void)strftime(time_array,(size_t) TIME_ARRAY_LEN,TIME_FORMAT_STRING,tm_p); */
/*   return(time_array); */
/* } */


/****************************************************************************************
Function:   _88D_secs_to_string
Description:    Given time in seconds since midnight, return HH:MM:SS string.
Returns:    time formatted as a string
Globals:    none
Notes:      returned string is STATIC, not malloc'd
*****************************************************************************************/
char *_88D_secs_to_string (
    int     time)               /* seconds since midnight       */
{
    int         h, m, s;
    static char     stime[20];

    h =     time/3600;
    time =  time - h*3600;
    m =     time/60;
    s =     time - m*60;

    (void) sprintf(stime, "%02d:%02d:%02d", h, m, s);
    return(stime);
}
