/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:12:15 $
 * $Id: biased_dbz_to_rate.h,v 1.1 2004/01/21 20:12:15 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


#ifndef BIASED_DBZ_TO_RATE_H
#define BIASED_DBZ_TO_RATE_H

#include <math.h>

/*********************************************************************  
Method : biased_dBZ_to_rate
Details:Converts the Level II "biased" reflectivity data to rate (in mm/hr).
**********************************************************************/
float biased_dBZ_to_rate(int biased_dBZ, float cf_mult, float cf_power) {

	float	dBZ,
		power,
		rate;

	dBZ 	= (biased_dBZ - 66) / 2.0;  
	power 	= pow(10., (dBZ / 10.));
	rate 	= pow((power/cf_mult),(1.0/cf_power));
	
	return (rate);

}/*end biased_dBZ_to_rate*/

/********************************************************************/
#endif
