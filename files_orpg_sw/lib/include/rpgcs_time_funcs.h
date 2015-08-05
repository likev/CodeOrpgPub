/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/11 14:03:33 $
 * $Id: rpgcs_time_funcs.h,v 1.4 2006/09/11 14:03:33 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef RPGCS_TIME_FUNCS_H
#define RPGCS_TIME_FUNCS_H

#include <infr.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define HOURS_IN_DAY       24
#define SECS_IN_DAY     86400
#define SECS_IN_HOUR     3600

/* Function prototypes. */
int RPGCS_get_time_zone();

int RPGCS_get_date_time( int *ctime, int *cdate );

int RPGCS_julian_to_date( int julian_date, int *year, int *month,
                           int *day );

int RPGCS_date_to_julian( int year, int month, int day, int *julian_date );

int RPGCS_ymdhms_to_unix_time( time_t *time, int year, int month, int day,
                                int hour, int minute, int second );

int RPGCS_unix_time_to_ymdhms( time_t time, int *year, int *month, int *day,
                                int *hour, int *minute, int *second );

int RPGCS_convert_radial_time( unsigned int time, int *hour, int *minute,
                                int *second, int *mills );

time_t RPGCS_time_span( int start_time, int start_date, int end_time,
                        int end_date );

#ifdef __cplusplus
}
#endif

#endif
