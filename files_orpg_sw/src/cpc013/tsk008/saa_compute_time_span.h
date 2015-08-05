/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:12:37 $
 * $Id: saa_compute_time_span.h,v 1.1 2004/01/21 20:12:37 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/**************************************************************
File	: saa_compute_time_span.h
Details	: Computes the time span between end time and start time (given
	  the start time after midnight in seconds, the start date (Julian),
	  the end date, and end time. )
***************************************************************/

#ifndef SAA_COMPUTE_TIME_SPAN_H
#define SAA_COMPUTE_TIME_SPAN_H

#include <stdlib.h>
#include "saa.h"

/********************FUNCTION DECLARATIONS *********************/

float saa_compute_time_span(int startDate,int startTime,int endDate,int endTime);

#endif
