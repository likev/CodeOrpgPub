/*
 * RCS info
 * $Author:
 * $Locker:
 * $Date:
 * $Id:
 * $Revision:
 * $State:
 */

/*** Local Include Files  ***/

#include <math.h>
#include <stdio.h>
#include "qperate_func_prototypes.h"

/*****************************************************************************
  Filename: qperate_precip_accum_init.c

  Description:
  ============

  If the area is greater than or equal to a defined precipitation area threshold
  (Precip_Area_Thresh), then the rain detected flag is set to be TRUE.

  Other supplemental fields related to the precipitation detection are computed
  based on the rain detection situation.

  This module replaces a portion of the old Precipitation Detection Function
  (PDF).

  Inputs:  time_t first_elev_time - first elevation time
           time_t last_elev_time  - first elevation time

  Outputs: Rate_Buf_t* rate_out    - rate output buffer, with the following
                                     Rate_Supl_t (include/qperate_types.h)
                                     fields set:

     time_t time;               * time of rate grid                 *
     time_t last_time_prcp;     * last time of precipitation        *
     time_t start_time_prcp;    * start time of precipitation       *
     int    prcp_detected_flg;  * TRUE -> precip. has been detected *
     int    ST_active_flg;      * TRUE -> a storm is active         *
     int    prcp_begin_flg;     * TRUE -> precipitation has begun   *

     The remaining 4 Rate_Supl_t fields are set outside precip_accum_init():

     float  pct_hybrate_filled; * % hybrid rate grid filled         *
     float  highest_elang;      * highest elevation angle used      *
     float  sum_area;           * sum of the area with precip.      *
     int    vol_sb;             * volume spot blank                 *

  Change History
  ==============
  DATE        VERSION  PROGRAMMER         NOTES
  ----------  -------  ---------------    ------------------
  02/22/07     0000     Jihong Liu        Initial implementation for
                                          dual-polarization project
                                          (ORPG Build 11).
  03/26/07     0001     Jihong Liu        Move some logic to caller func.
                                          qperate_build_RainfallRate
  06/20/07     0002     Jihong Liu        Accumulation beginning time and
                                          latest rain time are based on
                                          average date/time for the current
                                          volume instead of the current
                                          elevation when the rain is
                                          detected
  20081117     0003     James Ward        Added logging of flag transitions

  20110823     0004     Steve Smith       See notes below.
                        James Ward

  precip_accum_init() was previously called during the building of the
  hybrid scan, but it was discovered that an aborted volume,
  KVNX20110308_180518_V06.gz, could cause the storm start time to erroneously
  be set.

  The solution was to call precip_accum_init() right before the rate grid
  product is ready to be built, at the end of the volume. Now the storm times,
  storm active flags, and the rate grid are always in sync.

  To facilitate QPE restarts (CCR NA11-0284), update_supl_rainTime() was
  folded into precip_accum_init().
******************************************************************************/

extern int restart_qpe; /* see qperate_main.c */

void precip_accum_init(time_t first_elev_time, time_t last_elev_time,
                       Rate_Buf_t* rate_out)
{
   static time_t storm_start_time   = 0L;
   static time_t last_rain_time     = 0L;
   static int    prev_ST_active_flg = FALSE; /* TRUE -> a storm was active */

   long noRainDuration    = 0L;
   int  durationThreshold = 0;

   time_t average_elev_time = 0L; /* average elevation time */

   char msg[200]; /* stderr message */

   /* 29 Aug 2011 Ward Check to see if the restart button was
    * hit. If so, reset the times. CCR NA11-0284. Resetting the
    * last_rain_time mimics the PPS, which RPG restarts the precip
    * accum task on an hci button push. */

   if(restart_qpe == TRUE)
   {
      sprintf(msg, "Got %s event, initializing precip times\n",
              "ORPGEVT_RESTART_LT_ACCUM");
      RPGC_log_msg(GL_INFO, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      storm_start_time   = 0L;
      last_rain_time     = 0L;
      prev_ST_active_flg = FALSE;

      restart_qpe = FALSE;
   }

   /* Init flags */

   rate_out->rate_supl.prcp_detected_flg = FALSE;
   rate_out->rate_supl.prcp_begin_flg    = FALSE;
   rate_out->rate_supl.ST_active_flg     = FALSE;

   /* See if enough precip has been detected to trip the
    * precip detected flag.
    *
    * Default Paif_area: 80 km^2 */

   if(rate_out->rate_supl.sum_area >=
      rate_out->qpe_adapt.dp_adapt.Paif_area)
   {
      rate_out->rate_supl.prcp_detected_flg = TRUE;
      rate_out->rate_supl.ST_active_flg     = TRUE;
      last_rain_time                        = first_elev_time; /* AEL 3.1.3 */

      sprintf(msg, "Precip area %f >= %d km^2, %s %s\n",
              rate_out->rate_supl.sum_area,
              rate_out->qpe_adapt.dp_adapt.Paif_area,
              "precip detected in this volume.",
              "A storm is active.");

      RPGC_log_msg(GL_INFO, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      /* Is this new precip? */

      if(prev_ST_active_flg == FALSE)
      {
         rate_out->rate_supl.prcp_begin_flg = TRUE;
         storm_start_time                   = first_elev_time; /* AEL 3.1.3 */

         sprintf(msg, "Precip is beginning.\n");

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }
   } /* precip detected in this volume */
   else /* precip not detected in this volume */
   {
      sprintf(msg, "Precip area %f < %d km^2, %s\n",
              rate_out->rate_supl.sum_area,
              rate_out->qpe_adapt.dp_adapt.Paif_area,
              "precip not detected in this volume.");

      RPGC_log_msg(GL_INFO, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      noRainDuration = first_elev_time - last_rain_time; /* secs */

      /* Default rain_time_thresh: 60 mins */

      durationThreshold = rate_out->qpe_adapt.prep_adapt.rain_time_thresh
                          * SECS_PER_MIN; /* we want it in secs */

      if(noRainDuration >= durationThreshold) /* AEL 3.1.3 */
      {
         /* The last_rain_time mimics the PPS, which does not
          * reset the last_date_rain/last_time_rain when the
          * rain_time_thresh is exceeded. */

         storm_start_time                  = 0L;
         rate_out->rate_supl.ST_active_flg = FALSE;

         sprintf(msg, "noRainDuration %ld >= %d secs, %s\n",
                 noRainDuration,
                 durationThreshold,
                 "no storm is active");

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

      } /* end precip not detected, storm not active */
      else /* a storm must be active */
      {
         rate_out->rate_supl.ST_active_flg = TRUE;

         sprintf(msg, "noRainDuration %ld < %d secs, %s\n",
                 noRainDuration,
                 durationThreshold,
                 "a storm is active");

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

      } /* end precip detected, storm active */
   }

   /* Update rate times. AEL 3.2.2 I */

   rate_out->rate_supl.start_time_prcp = storm_start_time;
   rate_out->rate_supl.last_time_prcp  = last_rain_time;

   /* The rate time used is an average of times of the beginning
    * elevation and the ending elevation. This mimics the legacy PPS.
    * AEL 3.1.1 */

   compute_avg_Time(first_elev_time, last_elev_time, &average_elev_time);

   rate_out->rate_supl.time = average_elev_time;

   /* Save the ST_active_flg for next time */

   prev_ST_active_flg = rate_out->rate_supl.ST_active_flg;

} /* end precip_accum_init() =============================== */
