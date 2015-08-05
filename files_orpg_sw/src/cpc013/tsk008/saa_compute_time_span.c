/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:48 $
 * $Id: saa_compute_time_span.c,v 1.2 2008/01/04 20:54:48 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/**************************************************************
File	: saa_compute_time_span.c
Details	: Computes the time span between end time and start time (given
	  the start time after midnight in seconds, the start date (Julian),
	  the end date, and end time. )
***************************************************************/

#include "saa_compute_time_span.h"


/**************************************************************
Method	: saa_compute_time_span
Details	: Computes the time span between end time and start time (given
	  the start time after midnight in seconds, the start date (Julian),
	  the end date, and end time. )
***************************************************************/

float saa_compute_time_span(
int startDate,
int startTime,
int endDate,
int endTime){

	float timeDifference;
	int dateDifference = endDate - startDate;
	
	/*check if end date falls before start date */
	if(dateDifference < 0){
		LE_send_msg(GL_ERROR,"SAA:saa_compute_time_span - End Date comes before Start Date.\n");
	  	return (float)dateDifference ;  /* Modified 12/15/2003  WDZ  */
	}
	/*compute the time difference in fractional hour */
	timeDifference = dateDifference * HOURS_IN_DAY + (endTime - startTime)/(float)SEC_IN_HOUR;
	
	return timeDifference;
	
}/*end saa_compute_time_span */
