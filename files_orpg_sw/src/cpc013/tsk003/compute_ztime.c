/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/03 14:49:32 $
 * $Id: compute_ztime.c,v 1.5 2014/09/03 14:49:32 dberkowitz Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#include <math.h>
#include "epre_main.h"

/* Define constant variables ----------------------------------------------- */
#define SEC_IN_DAY 86400     /* Number of seconds in a day                   */ 
#define MILSEC 1000          /* Number of milliseconds in a second           */ 
#define FUNCTION_FAILED 1

  int debugit = FALSE;         /* Controls debug output in this file           */

/*****************************************************************************
  get_elev_time() extracts the julian date and time of a radial from the  
                  BASEDATA_ELEV radial header. 
******************************************************************************/

time_t get_elev_time(int zdate, int ztime)
{
    int tot_time_sec;        /* Variable holds total time in second since 
                                midnight of the base data.                   */


   /* We subtract 1 from elev_date because Julian day 1 is UNIX day 0. */

    tot_time_sec = ztime/MILSEC + ((zdate - 1) * SEC_IN_DAY );   

    if (debugit) {
      fprintf( stderr,"ELEV DATE = %d\tTIME = %d\n",zdate,ztime );
      fprintf( stderr,"TOT_TIME_SEC = %d\n",tot_time_sec );
    }
    return tot_time_sec;
}

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
     08/27/2014    0001      R. Murnan          CCR NA14-00268
                                                copied from dp_time_conversion.c
                                                to increase precision to match
                                                dual pol QPE rate calculations.
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
      if(debugit)
         fprintf(stderr, msg);

      return(FUNCTION_FAILED);
   }

   return(avg_time);

} /* end average_time() ===================================== */


/*****************************************************************************
  compute_avg_datetime() computes the average date/time of the radials that
                         built the Hybrid Scan reflectivity data file. The
                         input is the beginning and ending total time.  The 
                         output is stored as intergers in AVGDATE and AVGTIME. 
    Change History
    ============
    DATE          VERSION    PROGRAMMER         NOTES
    ----------    -------    ---------------    ------------------
    01/15/02      0000       Tim O'Bannon       CCR
    10/26/05      0001       C. Pham            CCR NA05-21401
    08/27/2014    0002       R. Murnan          CCR NA14-00268
******************************************************************************/  
void compute_avg_datetime(time_t begtime, time_t endtime, 
                          int *avgdate, int *avgtime)
{
  time_t avg_time_sec; /*Variable holds average time in milseconds        */ 
  char        msg[200];     /* stderr message */

  avg_time_sec = average_time(begtime, endtime);

  if(avg_time_sec == FUNCTION_FAILED)
   {
      sprintf(msg, "%s %s %ld\n",
              "compute_avg_datetime:",
              "average_time() returned FUNCTION_FAILED. avg_time_sec:",
              avg_time_sec);

      RPGC_log_msg(GL_INFO, msg);

      #ifdef EPRE_DEBUG
         fprintf(stderr, msg);
      #endif
   }


  /* Checks on average_time */

     #ifdef EPRE_DEBUG
     {
         fprintf (stderr, "COMPUTE_ZTIME: begtime = %ld, endtime = %ld, avgtime = %ld\n",
                  begtime, endtime, avg_time_sec);
     }
     #endif  

   /* Set average date to Julian date ***/
   /* We add 1 to avgdate because Julian day 1 is UNIX day 0. */
     *avgdate = (avg_time_sec / SEC_IN_DAY) + 1;
     *avgtime = avg_time_sec % SEC_IN_DAY;

     if (debugit) fprintf( stderr,"AVGDATE = %d\tAVGTIME = %d\n",
                                 *avgdate, *avgtime );
} 
