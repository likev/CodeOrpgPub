/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:49:37 $
 * $Id: dp_time_conversions.c,v 1.3 2009/10/27 22:49:37 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <errno.h>                  /* errno         */
#include <math.h>                   /* lrintl()      */
#include "dp_Consts.h"              /* FUNCTION_OK   */
#include "dp_lib_func_prototypes.h"
#include "dp_lt_accum_Consts.h"     /* SECS_PER_HOUR */

/******************************************************************************
    Filename: dp_time_conversions.c

    Description:
    ============
    UNIX_time_to_julian_mins() converts a UNIX time_t to an RPG Julian date
    and the number of minutes since midnight. This is used to set various
    final product parameters.

    Inputs: time_t  time               - the time to convert

    Outputs: int*  julian_date         - the julian date
             int*  mins_since_midnight - the minutes since midnight

    Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

    Called by: buildDPR_prod(), append_ascii_layer2(),
               dp4bit_product_header(), dp8bit_product_header(),

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ----------------------
     3 Jan 2008    0000      Ward               Initial implementation
******************************************************************************/

int UNIX_time_to_julian_mins(time_t time, int* julian_date,
                             int* mins_since_midnight)
{
   int  year   = 0;
   int  month  = 0;
   int  day    = 0;
   int  hour   = 0;
   int  minute = 0;
   int  second = 0;
   int  ret    = FUNCTION_SUCCEEDED;
   char msg[200]; /* stderr message */

   /* Check for NULL pointers */

   if(pointer_is_NULL(julian_date, "UNIX_time_to_julian_mins", "julian_date"))
      return(NULL_POINTER);

   if(pointer_is_NULL(mins_since_midnight, "UNIX_time_to_julian_mins", 
                      "mins_since_midnight"))
   {
      return(NULL_POINTER);
   }

   /* Convert time to year, month, day ... */

   ret = RPGCS_unix_time_to_ymdhms(time, &year, &month,  &day,
                                         &hour, &minute, &second);

   if(ret < 0) /* the call to RPGCS_unix_time_to_ymdhms() failed */
   {
      sprintf(msg, "%s %s %ld %s\n",
              "UNIX_time_to_julian_mins:",
              "RPGCS_unix_time_to_ymdhms() says",
              time,
              "is out of range");

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(FUNCTION_FAILED);
   }

   /* Get the Julian date, There is a bug in RPGCS_date_to_julian(),
    * when year = month = day = 0 it returns -719559. */

   ret = RPGCS_date_to_julian(year, month, day, julian_date);

   if(ret < 0) /* the call to RPGCS_date_to_julian() failed */
   {
      sprintf(msg, "%s %s\n",
              "UNIX_time_to_julian_mins:",
              "RPGCS_date_to_julian() call failed");

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(FUNCTION_FAILED);
   }
   else if(*julian_date < MIN_DATE) /* MIN_DATE = 1 = days since 1/1/1970 */
      *julian_date = MIN_DATE;

   /* Get the minutes since midnight. */

   *mins_since_midnight = (hour * MINS_PER_HOUR) + minute;

   return(FUNCTION_SUCCEEDED);

} /* end UNIX_time_to_julian_mins() ===================================== */

/******************************************************************************
    Filename: dp_time_conversions.c

    Description:
    ============
    average_time() averages 2 UNIX time_t's

    lrintl(), in math.h, rounds to the nearest long. Since time_t's are really
    signed longs (31 bits + 1 sign bit), the 'long double' version of the
    lrint suite of algorithms is used to maintain precision.

    We have to be careful of overflow when adding. We are usually subtracting
    to find a difference of time_t's, but not when we average. If you look
    at some current times:

       1187571936 + 1187572122 = 2375144058 > 2^31 = 2147483648.

    To avoid this overflow, we halve the times and then add. If you see a
    large negative time_t in the future, it's probably an overflow bug.

    Inputs: time_t time1 - 1st UNIX time
            time_t time2 - 2nd UNIX time

    Outputs: The average time.

    Called by: (QPE) compute_avg_Time() (for the volume)

    Returns: FUNCTION_FAILED (1) or the average time

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ----------------------
     3 Jan 2008    0000      Ward               Initial implementation
******************************************************************************/

time_t average_time (time_t time1, time_t time2)
{
   long double time1_ld = 0;
   long double time2_ld = 0;
   time_t      avg_time = 0L;
   char        msg[200];     /* stderr message */

   /* To avoid overflow, halve first, add, instead of add first, halve */

   time1_ld  = time1 / 2.0;
   time2_ld  = time2 / 2.0;

   avg_time = (time_t) lrintl(time1_ld + time2_ld);

   if(errno == EDOM) /* Domain Error. See man lrintl() */
   {
      sprintf(msg, "%s %s\n",
              "average_time:",
              "lrintl() returns EDOM (Domain Error)");

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LIB002_DEBUG)
         fprintf(stderr, msg);

      return(FUNCTION_FAILED);
   }

   return(avg_time);

} /* end average_time() ===================================== */
