/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:29:39 $
 * $Id: prcpacum_convert_time.c,v 1.1 2005/03/09 15:29:39 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename:  prcpacum_convert_time.c 

   Description
   ===========
     This function converts time from seconds (since midnight)to hours, minutes,
   and seconds or from hours, minutes, and seconds to seconds (since midnight).
   The input argument, type, specifies the conversion as hms_sec or SEC_HMS.

   Change History
   ==============
   02/21/89      0000      P. Pisani            spr# 90067
   07/30/90      0001      Paul Jendrowski      spr# 90845
   11/04/90      0002      Paul Jendrowski      spr# 91254
   12/03/91      0003      Steve Anderson       spr # 92740
   12/10/91      0004      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0005      Toolset              spr 91895
   03/25/93      0006      Toolset              spr na93-06801
   01/28/94      0007      Toolset              spr na94-01101
   03/03/94      0008      Toolset              spr na94-05501
   04/11/96      0009      Toolset              ccr na95-11802
   12/23/96      0010      Toolset              ccr na95-11807
   03/16/99      0011      Toolset              ccr na98-23803
   01/07/05      0012      Cham Pham            ccr NA05-01303
   
   INPUT           TYPE        DESCRIPTION
   -----           ----        -----------
   type            int         Input argument specifying conversion type 
                               SEC_HMS OR HMS_SEC.
   sec             int         Time in seconds since midnight 
   hh              int         Hours portion of time expressed as hours, 
                               minutes and seconds.
   mm              int         Minutes portion of time expressed as hours,
                               minutes and seconds. 
   ss              int         Seconds portion of time expressed as hours,
                               minutes and seconds.

   OUTPUT          TYPE        DESCRIPTION
   ------          ----        -----------
   sec             int         Time in seconds since midnight
   hh              int         Hours portion of time expressed as hours, 
                               minutes and seconds.
   mm              int         Minutes portion of time expressed as hours,
                               minutes and seconds.
   ss              int         Seconds portion of time expressed as hours,
                               minutes and seconds.
****************************************************************************/
/* Global include file */
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

void convert_time( int type, int *sec, int *hh, int *mm, int *ss )
{

  if ( DEBUG ) {fprintf(stderr,"A3135Z__CONVERT_TIME\n");}

/* Determine type of conversion: seconds to hours, minutes, seconds;
   or hours, minutes, seconds to seconds.
 */
  if ( type == SEC_HMS ) 
  {

/* Convert seconds to hours, minutes, seconds.
   Divide total seconds by seconds per hour.
 */
    if ( DEBUG ) {
     fprintf(stderr," CONVERTING SECONDS IN DAY TO HH MM SS FORMAT, SEC=  %d\n",
                                                   *sec); 
    }
    *hh = *sec / SEC_P_HR;

/* Determine total minutes in time.*/
    *mm = (*sec - *hh * SEC_P_HR) / SEC_P_MIN;

/* Determine number of seconds in time.*/
    *ss = *sec - *hh * SEC_P_HR - *mm * SEC_P_MIN;

    if ( DEBUG ) 
      {fprintf(stderr," HH MM SS = %d  %d  %d\n",*hh,*mm,*ss);}

  }
/* Type conversion is hours minutes seconds to seconds.*/
  else 
  {

    if ( DEBUG ) 
      {fprintf(stderr," HH MM SS = %d  %d  %d\n",*hh,*mm,*ss);}

    *sec = *hh * SEC_P_HR + *mm * SEC_P_MIN + *ss;

    if ( DEBUG ) 
      {fprintf(stderr," SEC = %d\n",*sec);}

  }

  if ( DEBUG ) {fprintf(stderr," EXITING A3135Z\n");}
}
