/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:12:17 $
 * $Id: deg_to_rad.h,v 1.1 2004/01/21 20:12:17 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


#ifndef DEG_TO_RAD_H
#define DEG_TO_RAD_H

/*include math.h to access M_PI, the definition of PI*/
#include <math.h>

/***************************************************  
Method : deg_to_rad
Details: Converts from degrees to radians.
Modified: Aug. 6,2003 by Reji Zachariah to include M_PI
	  instead of defining PI inside the method.
 ***************************************************/
float deg_to_rad(float degrees) {

	return (degrees * M_PI / 180.0);
	
}/*end deg_to_rad*/

/*****************************************************/

#endif
