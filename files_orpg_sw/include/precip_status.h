/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/12/17 21:45:43 $
 * $Id: precip_status.h,v 1.3 2004/12/17 21:45:43 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef PRECIP_STATUS_H
#define PRECIP_STATUS_H

/* current_precip_status values. */
#define PRECIP_ACCUM                1
#define PRECIP_NOT_ACCUM            0
#define PRECIP_STATUS_UNKNOWN      -1

/* rain_area_trend values. */
#define TREND_UNKNOWN              -2
#define TREND_DECREASING           -1
#define TREND_INCREASING            0
#define TREND_STEADY                1

/* time_last_exceeded_raina undefined value. */
#define TIME_LAST_EXC_RAINA_UNKNOWN -1

/* time_remaining_to_reset_accum undefined value. */
#define RESET_ACCUM_UNKNOWN        -1

/* rain_area undefined value. */
#define PRECIP_AREA_UNKNOWN        -1

/* rain_area_diff undefined value. */
#define PRECIP_AREA_DIFF_UNKNOWN   -1

/* rain_dbz_thresh_rainz undefined value. */
#define RAIN_DBZ_THRESH_UNKNOWN    -1.0

/* rain_area_thresh_raina undefined value. */
#define RAIN_AREA_THRESH_UNKNOWN   -1

/* rain_time_thresh_raint undefined value. */
#define RAIN_TIME_THRESH_UNKNOWN   -1

/* positional parameters for precipitation pre-processing adaptation data. */
#define RAIN_DBZ_THRESH             6
#define RAIN_AREA_THRESH            7
#define RAIN_TIME_THRESH            8


typedef struct {

   int    current_precip_status;		/* Either 1 if accumulating, or 0 is not accumulating */
   int    rain_area_trend;			/* -1 if decreasing, 0 is steady, or 1 if increasing. */
   time_t time_last_exceeded_raina;		/* UNIX time since area exceeded RAINA. */
   time_t time_remaining_to_reset_accum;	/* UNIX time. */
   int  rain_area;				/* Precipitation area, in km^2. */
   int    rain_area_diff;                       /* Precipitation area difference, in whole km^2. */
   float  rain_dbz_thresh_rainz;		/* rain dBZ thresh, in dBZ. */
   int    rain_area_thresh_raina;		/* rain area threshold, in km^2. */
   int    rain_time_thresh_raint;		/* rain time threshold, in seconds. */

} Precip_status_t;

#endif
