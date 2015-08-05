/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:39 $
 * $Id: radar_rng.h,v 1.2 2008/01/04 20:54:39 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/*********************************************************
File	: radar_rng.h
Created	: By Tim O'Bannon , 2003
Modification History:
	Reji Zachariah 8/2003 : fixed a calculation error;
			 	added comments explaning the calculation.
**********************************************************/

#ifndef RADAR_RNG_H
#define RADAR_RNG_H

#include <math.h>
#include "saaConstants.h"
/***********************************************************  
Method:  radar_range
Details: Computes and returns the radar range (km) for a given height (km) 
	 and sine of elevation angle. 
 **********************************************************/
float radar_range(float height, float sin_el) 
{
	/*making ER static const to prevent recalculations */
	/*when this function is called multiple times. */
	static const float ER	= EARTH_RADIUS * PROP_FCTR;
	
	float range;

	/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	//Explanation for reference:
	
	height = range*sin_el +range^2/(2*ER)
	range^2/(2*ER)+ range*sin_el-height = 0
	
	Therefore (using the quadratic equation solution),
	range 	= [-sin_el +/- sqrt(sin_el^2 - 4*(-height)/(2*ER))]/[1/ER]
		= ER[-sin_el +/- sqrt(sin_el^2 + 2*height/ER)]
		
	For our purposes (for a valid range), we need to use:
	range 	= ER[-sin_el + sqrt(sin_el^2 + 2*height/ER)] 
		= -ER[sin_el - sqrt(sin_el^2 + 2*height/ER)] 
		
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
	
	range = -ER* (sin_el - sqrt(sin_el * sin_el + 2. * height / ER));
	return (range);
	
}/*end radar_range*/
/************************************************************/

#endif
