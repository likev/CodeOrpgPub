/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/04/18 20:51:41 $
 * $Id: prod_sched.h,v 1.7 2006/04/18 20:51:41 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef PROD_SCHED_H
#define PROD_SCHED_H

/* Product Scheduling information can be stored in DEA file.   The format
   of the elements needs to be as follows:
   
   PROD_SCHED_KEY.<task_name>.<supported_keys>

*/

#define PROD_SCHED_KEY               "Prod_Schedule"

/* The following are supported keywords */
#define PROD_SCHED_DISABLED          "disabled"
#define PROD_SCHED_TIME_OF_DAY       "time_of_day"
#define PROD_SCHED_DAY_OF_YEAR       "day_of_year"
#define PROD_SCHED_PROD_ID           "prod_id"
#define PROD_SCHED_BY_LOCATION       "location"
#define PROD_SCHED_BY_VCP            "vcp"
#define PROD_SCHED_BY_WXMODE         "wxmode"
#define PROD_SCHED_FOR_ICAO          "icao"

/* The following are information should be included in the DEA file:

Prod_Schedule.<task_name>.disabled	value = ;
   					type = string;
					range = { No, Yes };
					enum = 0, 1;
					description = Should product(s) be disabled?;
					default = No;


# The following are optional.   Any or all can be included.   If included,
# then the product is disabled if "disabled" has value "Yes" and all values 
# fall within range. 

Prod_Schedule.<task_name>.prod_id	value = ;
					type = int;
					range = [-1, 2999];
					description = Products to be disabled ... -1 = all;
					default = -1;

Prod_Schedule.<task_name>.time_of_day	value = ;
					type = int;
					range = [0000, 2400];
					description = Begin Time, End Time in hhmm format;
					default = 0000, 2400;

Prod_Schedule.<task_name>.day_of_year	value = ;
					type = int;
					range = [0101, 1231];
					description = Begin Date, End Date in mmdd format;
					default = 0101 1231;

Prod_Schedule.<task_name>.location	value = ;
					type = int;
					range = [ -1800000, 1800000];
					description = Begin Lat, End Lat, Beg Lon, End Lon, in hhmmss format;
					default = -900000 900000 -1800000 1800000;

Prod_Schedule.<task_name>.vcp		value = ;
					type = int;
					range = [0, 999];
					description = Volume Coverage Pattern (VCP);
					default = 0;

Prod_Schedule.<task_name>.wxmode	value = ;
					type = int;
					range = [0,2];
					description = Weather Mode;
					default = 0;

Prod_Schedule.<task_name>.icao		value = ;
					type = string;
					description = ICAO;
*/

#endif /*DO NOT REMOVE!*/
