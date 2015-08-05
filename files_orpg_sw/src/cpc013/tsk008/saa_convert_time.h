/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/21 20:12:38 $
 * $Id: saa_convert_time.h,v 1.1 2004/01/21 20:12:38 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */


/**************************************************************
File	: saa_convert_time.h
Details	: THIS MODULE CONVERTS TIME FROM SECONDS (SINCE MIDNIGHT) TO
    	  HOURS, MINUTES, AND SECONDS OR FROM HOURS, MINUTES, AND
    	 SECONDS TO SECONDS (SINCE MIDNIGHT). THE INPUT ARGUMENT,
         Time_Conv_Type, SPECIFIES THE CONVERSION AS HMS_SEC OR SEC_HMS.
         
         This module also contains a function to convert the julian
         date to dd,dm,dy.
***************************************************************/

#ifndef SAA_CONVERT_TIME_H
#define SAA_CONVERT_TIME_H

#include "saaConstants.h"

enum Time_Conv_Type{
	SAA_To_Seconds = 0,
	SAA_From_Seconds = 1
};


/**************************************************************
Method	: saa_convert_time
Details : converts time
***************************************************************/
static void saa_convert_time (enum Time_Conv_Type type, int* seconds, int* hh,
			int* mm, int* ss){
			
	//Check for NULL pointers 
	if ( (seconds == NULL) 	||
	     (hh == NULL) 	||
	     (mm == NULL)	||
	     (ss == NULL) )	{
		LE_send_msg(GL_ERROR,"SAA:saa_convert_time - NULL pointers passed in as arguments.\n");
	  	return;
	}//end if
	
	if(type == SAA_From_Seconds){
	
		*hh = *seconds/SEC_IN_HOUR;
		*mm = (*seconds - *hh * SEC_IN_HOUR)/SEC_IN_MIN;
		*ss = *seconds - *hh * SEC_IN_HOUR - *mm * SEC_IN_MIN; 
		return;
	}
	else{
		*seconds = *hh * SEC_IN_HOUR + *mm * SEC_IN_MIN + *ss;
		return;
	}//end else
			
}//saa_convert_time

/**************************************************************
Method	: saa_convert_from_julian
Details : converts from julian date to dd,dm,dy
***************************************************************/
static void saa_convert_from_julian (
    int		julian,				/* days since January 1, 1970		*/
    int		*dd,				/* OUT:  day				*/
    int		*dm,				/* OUT:  month				*/
    int		*dy )				/* OUT:  year				*/
{

   	 int	l,n;

	//Check for NULL pointers 
	if ( (dd == NULL) 	||
	     (dm == NULL) 	||
	     (dy == NULL)) {
		LE_send_msg(GL_ERROR,"SAA:saa_convert_from_julian - NULL pointers passed in as arguments.\n");
	  	return;
	}//end if

    	/* Convert modified julian to year/month/day */
    	julian += 2440587;
    	l = julian + 68569;
    	n = 4*l/146097;
    	l = l -  (146097*n + 3)/4;
    	*dy = 4000*(l+1)/1461001;
    	l = l - 1461*(*dy)/4 + 31;
    	*dm = 80*l/2447;
    	*dd= l -2447*(*dm)/80;
    	l = *dm/11;
    	*dm = *dm+ 2 - 12*l;
    	*dy = 100*(n - 49) + *dy + l;
	/*    *dy = *dy - 1900;*/   

    	return;
}//end saa_convert_from_julian

#endif
