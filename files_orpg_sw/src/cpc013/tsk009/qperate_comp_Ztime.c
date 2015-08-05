/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:42:06 $
 * $Id: qperate_comp_Ztime.c,v 1.4 2009/10/27 22:42:06 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* System Include file */

#include <math.h>

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

#define MILSEC 1000 /* Number of milliseconds in a second */

/******************************************************************************
    Filename: qperate_comp_Ztime.c

    Description:
    ============
       get_vol_BegEndTime() extracts the Julian date from 1/1/70 and time
    in millisecs of day past midnight (GMT) of a elevation from
    the Base_data_header (read by read_MomentData()):

       unsigned short date;  - Radial date, modified Julian.
                  int time;  - Radial time, in millisecs since midnight.

    Inputs:
       elev_date - modified Julian date
       elev_time - elevation time in millisecs since midnight (GMT).

    Return:
       The total time (in UNIX seconds).

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----        -------    ----------      -----
    12/21/06    0000       Cham Pham       Initial implementation for
                                           dual-polarization project
                                           (ORPG Build 11).
******************************************************************************/

time_t get_elevation_time (int elev_date, int elev_time)
{
   time_t tot_time_sec = 0L; /* Total time in seconds since midnight (GMT) */

   #ifdef QPERATE_DEBUG
      fprintf ( stderr, "Beginning get_vol_BegEndTime() ...........\n" );
   #endif

   /* This logic mimics the legacy PPS. We subtract 1 from elev_date because
    * Julian day 1 is UNIX day 0. */

   tot_time_sec = ((elev_date - 1) * SECS_PER_DAY) + (elev_time / MILSEC);

   #ifdef QPERATE_DEBUG
   {
      fprintf (stderr, "ELEV DATE = %d\tTIME = %d\n", elev_date, elev_time);
      fprintf (stderr, "TOT_TIME_SEC = %ld\n", tot_time_sec);
   }
   #endif

   /* 20080423 You can use http://www.onlineconversion.com/unix_time.htm
    *          to convert UNIX secs to a normal date. */

   return (tot_time_sec);

} /* end get_vol_BegEndTime() ==================================== */

/******************************************************************************
    Filename: qperate_comp_Ztime.c

    Description:
    ============
       compute_avg_Time() computes the average date/time of the elevations
    used in construction of the  hybrid rate scan for the present volume scan.

    Inputs:
       time_t  begtime - the beginning time.
       time_t  endtime - the ending time.

    Outputs:
       time_t* avgtime - the average time.

    Returns: FUNCTION_SUCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER_INPUT (2)

    Change History
    ============
    DATE        VERSION    PROGRAMMER      NOTES
    ----        -------    ----------      -----
    12/21/06    0000       Cham Pham       Initial implementation for
                                           dual-polarization project
                                           (ORPG Build 11).
    01/03/08    0000       Ward            Converted to time_t
******************************************************************************/

int compute_avg_Time (time_t begtime, time_t endtime, time_t* avgtime)
{
   char msg[200]; /* stderr message */

   /* Check for NULL pointer */

   if(pointer_is_NULL(avgtime, "compute_avg_Time", "avgtime"))
      return(NULL_POINTER);

   /* The average time is the average of the elevation scan times.
    * This logic mimics the legacy PPS. */

   *avgtime = average_time(begtime, endtime);

   if(*avgtime == FUNCTION_FAILED)
   {
      sprintf(msg, "%s %s %ld\n",
              "compute_avg_Time:",
              "average_time() returned FUNCTION_FAILED. avgtime:",
              *avgtime);

      RPGC_log_msg(GL_INFO, msg);

      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      return(FUNCTION_FAILED);
   }

   #ifdef QPERATE_DEBUG
   {
      fprintf (stderr, "begtime = %ld, endtime = %ld, avgtime = %ld\n",
               begtime, endtime, *avgtime);
   }
   #endif

   return(FUNCTION_SUCCEEDED);

} /* compute_avg_Time() ================================================ */
