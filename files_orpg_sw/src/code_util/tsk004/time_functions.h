/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2003/02/06 18:13:27 $
 * $Id: time_functions.h,v 1.2 2003/02/06 18:13:27 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/* time_functions.h */

#ifndef _TIME_FUNCTIONS_H_
#define _TIME_FUNCTIONS_H_

/*#include <data_mgt.h>*/
#include <time.h>
#include <stdio.h>

short orpg_date (time_t	seconds);
short orpg_time (time_t	seconds);
time_t unix_time (short date,int time);
void calendar_date (short date,int *dd,int *dm,int *dy);
char *LatLon_to_DdotD (int LatLon);
char *LatLon_to_DDDMMSS (int LatLon);
char *msecs_to_string (int time);
char *date_to_string (short date);
char *format_date_time(time_t time);
char *_88D_secs_to_string(int time);

#endif

