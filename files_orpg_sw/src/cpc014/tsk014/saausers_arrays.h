/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2006/04/05 15:51:41 $
 * $Id: saausers_arrays.h,v 1.3 2006/04/05 15:51:41 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


/************************************************************************
Module:         saausers_arrays.h  

Description:    Include file containing arrays and structures unique to the
		Snow Accumulation Algorithm product task, saausers.         
                
Authors:        Dave Zittel, Meteorologist, ROC/Applications
                    walter.d.zittel@noaa.gov
                Version 1.0, December 2003
                Version 2.0, March 2006   CCR NA06-06603  Correct USD/USW Start 
                			  Time display WDZ

*************************************************************************/
#ifndef SAAUSER_ARRAYS_H
#define SAAUSER_ARRAYS_H

#define MAX_HOURS    	     30 /* Number of hours allowed in snow user select Product */
#define MAX_USP_HOURS        30
#define MAX_USR_RQSTS        10 /* Maxmimum number of user requests allowed  */

struct Saausrhskp
{
	short usr_date[MAX_HOURS];
	short usr_time[MAX_HOURS];
	short usr_start_date[MAX_HOURS];
	short usr_start_time[MAX_HOURS];
	short data_avail_flag[MAX_HOURS];
	short usr_first_date;
	short usr_first_time;
	short usr_last_date;
	short usr_last_time;
} hskp_data;

	int   julian_minutes[MAX_HOURS];

struct Usr_data
{
	short swu_data[MAX_SAA_RADIALS][MAX_SAA_BINS];
	short sdu_data[MAX_SAA_RADIALS][MAX_SAA_BINS];
} ;

struct Usr_data usraccum[MAX_HOURS];

struct USP_data
{
	short total_swu[MAX_SAA_RADIALS][MAX_SAA_BINS];
	short total_sdu[MAX_SAA_RADIALS][MAX_SAA_BINS];
	short swu_avail_hrs[MAX_HOURS];
	short sdu_avail_hrs[MAX_HOURS];
	short time_order[MAX_HOURS];
	short swu_hour_cnt;
	short sdu_hour_cnt;
	int   max_swu;
	int   max_sdu;
	short all_avail_hrs[MAX_HOURS];
	short all_hour_cnt;
} usp;

struct User_requests
{
	short swu_num_rqsts;
	short swu_req_indx[MAX_USR_RQSTS];
	short swu_end_hour[MAX_USR_RQSTS];
	short swu_num_hours[MAX_USR_RQSTS];
	int   swu_beg_minutes[MAX_USR_RQSTS];
	int   swu_end_minutes[MAX_USR_RQSTS];
	short sdu_num_rqsts;
	short sdu_req_indx[MAX_USR_RQSTS];
	short sdu_end_hour[MAX_USR_RQSTS];
	short sdu_num_hours[MAX_USR_RQSTS];
	int   sdu_beg_minutes[MAX_USR_RQSTS];
	int   sdu_end_minutes[MAX_USR_RQSTS];
} userreq;
	
#endif
