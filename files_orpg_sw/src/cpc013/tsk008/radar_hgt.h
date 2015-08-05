/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:38 $
 * $Id: radar_hgt.h,v 1.2 2008/01/04 20:54:38 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


#ifndef RADAR_HGT_H
#define RADAR_HGT_H

#include "saaConstants.h"

/***********************************************************  
Method:  radar_height
Details: Computes the radar height (km) for a given range (km) and 
    	 sine of elevation angle.  
 **********************************************************/

float radar_height(float range, float sin_el) 
{
	
	/* making ER static const to prevent recalculations
	   when this function is called multiple times. */
	static const float ER	= EARTH_RADIUS * PROP_FCTR;
	
	float height;
	    
	height = (range * sin_el) + (range * range / (2. * ER));
	
	return height;
	
}/*end radar_height*/
/************************************************************/

#endif
