/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:06 $
 * $Id: gauge_radar_func.c,v 1.3 2011/04/13 22:53:06 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <dp_lib_func_prototypes.h>
#include <gsl/gsl_statistics_double.h>
#include "gauge_radar_common.h"
#include "gauge_radar_consts.h"
#include "gauge_radar_proto.h"
#include "gauge_radar_types.h"
#include "koun_features.h"

extern int data_stream;      /* realtime or replay                 */
extern int stats_everywhere; /* TRUE -> print stats to file        */
extern int use_only_nonzero; /* TRUE -> use only nonzero g-r pairs */

extern short feature_x[NUM_FEATURES];
extern short feature_y[NUM_FEATURES];
extern char* feature_name[NUM_FEATURES];

extern int   pps_hourly[MAX_AZM][MAX_2KM_RESOLUTION];

extern float old_gauge_hourly[MAX_GAUGES];
extern float old_dp_hourly[MAX_GAUGES];
extern float old_pps_hourly[MAX_GAUGES];

extern float old_gauge_storm[MAX_GAUGES];
extern float old_dp_storm[MAX_GAUGES];
extern float old_pps_storm[MAX_GAUGES];

extern short zero_scan;
extern short zero_hourly;
extern short no_hourly;
extern short reset_pps_storm;

extern Siteadp_adpt_t site_adapt; /* site adaptation data */

extern Gauge gauges[MAX_GAUGES];
extern short num_gauges;

/******************************************************************************
   Function name: task_handler()

   Description:
   ============
      task_handler() reads user requests; then for every request, queries the
      database; then it computes the accumulation during user-specified time
      period; then it builds the output product.

      The Compact_dp_basedata_elev is only needed if we need to find
      the range to the first bin for gauge location purposes. Since the range
      to the first bin is currently 0, it has been commented out. If you
      wish to revive the Compact_dp_basedata_elev, add DP_MOMENTS_ELEV (406)
      as the first buffer to your 179/180 products in your
      product_attr_table/task_attr_table.

   Inputs:
      None.

   Outputs:
      None.

   Return:
      1 - OK
     -1 - error

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2010    0000       Zhan Zhang         Initial implementation
******************************************************************************/

int task_handler(void)
{
    int   i, j;
    short legacy_s;

    int num_requests = 0;
    int opstat       = RPGC_NORMAL;
    int vol_num      = 0;        /* volume scan number      */
    int total_vols   = 0;        /* total number of volumes */

    static int old_vol_num = 0;  /* old vol num         */
    static int eighty_vols  = 0; /* eighty volume count */

    /* Compact_dp_basedata_elev* cdbe = NULL; -* base data header */
    LT_Accum_Buf_t*           lt_accum_buf = NULL; /* DP  lt accum buffer */
    hyadjscn_large_buf_t*     legacy       = NULL; /* PPS lt accum buffer */

    User_array_t* request_list = NULL;
    User_array_t* request      = NULL;

    prod_dep_para_t prod_dep; /* structure of product dependent parameters */

    /* start_time/end_time are user-specified start/end time
     *
     * time_t start_time = 0L;
     * time_t end_time   = 0L; */

    int ret; /* hold return value of a function for program logic control */

    dua_accum_buf_metadata_t metadata;  /* hold the metadata part of a message
                                           sent to the output buffer */

    static unsigned int metadata_size = sizeof(dua_accum_buf_metadata_t);

    /* year, month, day, hour, minute, second are only used once in converting
     * unix time to YMDHMS, of which, year, month, and day are passed to
     * user_time_to_unix_time() later on */

    int year   = 0;
    int month  = 0;
    int day    = 0;
    int hour   = 0;
    int minute = 0;
    int second = 0;

    /* Get Base data header -/
     *
     * cdbe = (Compact_dp_basedata_elev*) RPGC_get_inbuf_by_name(CDBE_BUF_NAME,
                                                                 &opstat);
     *
     * if((cdbe == NULL) || (opstat != RPGC_NORMAL))
     * {
     *     RPGC_log_msg(GL_INFO,
     *                  "RPGC_get_inbuf_by_name() %s error", CDBE_BUF_NAME);
     *
     *     if(cdbe != NULL)
     *        RPGC_rel_inbuf(cdbe);
     *
     *     /- Replace RPGC_cleanup_and_abort() with RPGC_abort() until
     *      * ROC fixes bug.
     *      * RPGC_cleanup_and_abort(opstat); -/
     *
     *     RPGC_abort();
     *     return -1;
     * }
     * else
     * {
     *     RPGC_log_msg(GL_INFO,
     *                  "surv_range %d bins, range_beg_surv %d meters",
     *                  cdbe->radial[0].bdh.surv_range,
     *                  cdbe->radial[0].bdh.range_beg_surv);
     *
     *    if(cdbe->radial[0].bdh.range_beg_surv > 0)
     *    {
     *       RPGC_log_msg(GL_INFO, "%s",
     *                    "range_beg_surv > 0, recalculate gauge distances");
     *    }
     * } */

    /* Get Long Term Accum buffer */

    lt_accum_buf = (LT_Accum_Buf_t*) RPGC_get_inbuf_by_name(DP_BUF_NAME,
                                                            &opstat);

    if((lt_accum_buf == NULL) || (opstat != RPGC_NORMAL))
    {
        RPGC_log_msg(GL_INFO,
                     "RPGC_get_inbuf_by_name() %s error", DP_BUF_NAME);

        /* if(cdbe != NULL)
         *    RPGC_rel_inbuf(cdbe); */

        if(lt_accum_buf != NULL)
           RPGC_rel_inbuf(lt_accum_buf);

        /* Replace RPGC_cleanup_and_abort() with RPGC_abort() until
         * ROC fixes bug.
         * RPGC_cleanup_and_abort(opstat); */

        RPGC_abort();
        return -1;
    }

    /* Get a legacy accum buffer. HYADJSCN (104) is an unbiased grid;
     * it is produced in the legacy before bias is applied. We're
     * not going to check flags = we're going to assume that there is
     * some scan-to-scan in the legacy -> it's a medium buffer. */

    zero_scan       = TRUE;
    zero_hourly     = TRUE;
    no_hourly       = TRUE;
    reset_pps_storm = TRUE;

    legacy = (hyadjscn_large_buf_t*) RPGC_get_inbuf_by_name(PPS_BUF_NAME,
                                                            &opstat);

    if((legacy == NULL) || (opstat != RPGC_NORMAL))
    {
        RPGC_log_msg(GL_INFO,
                     "RPGC_get_inbuf_by_name() %s error", PPS_BUF_NAME);

        /* if(cdbe != NULL)
         *    RPGC_rel_inbuf(cdbe); */

        if(lt_accum_buf != NULL)
           RPGC_rel_inbuf(lt_accum_buf);

        if(legacy != NULL)
           RPGC_rel_inbuf(legacy);

        /* Replace RPGC_cleanup_and_abort() with RPGC_abort() until
         * ROC fixes bug.
         * RPGC_cleanup_and_abort(opstat); */

        RPGC_abort();
        return -1;
    }
    else /* read the legacy flags */
    {
       zero_scan       = legacy->medium.small.Supl[FLG_ZERSCN];
       zero_hourly     = legacy->medium.small.Supl[FLG_ZERHLY];
       no_hourly       = legacy->medium.small.Supl[FLG_NOHRLY];
       reset_pps_storm = legacy->medium.small.Supl[FLG_RESTP];
    }

    /* Check to see if we have any legacy hourly */

    if((zero_hourly == TRUE) || (no_hourly == TRUE))
    {
       /* We don't have any legacy hourly, so reset the hourly buffer. */

       memset(pps_hourly,     0, MAX_AZM * MAX_2KM_RESOLUTION * sizeof(int));
       memset(old_pps_hourly, 0, MAX_GAUGES * sizeof(float));
    }
    else /* we have legacy hourly */
    {
       /* Copy the legacy hourly. */

       for(i = 0; i < MAX_AZM; i++)
       {
         for(j = 0; j < MAX_2KM_RESOLUTION; j++)
         {
            legacy_s = legacy->AccumHrly[i][j];

            if((legacy_s != INIT_VALUE) && (legacy_s > 0))
            {
               pps_hourly[i][j] = legacy_s;
            }
         }
       }
    }

    /* Get volume scan number, which is used when building final products */

    vol_num = RPGC_get_buffer_vol_num((void *) lt_accum_buf);

    if(vol_num < old_vol_num)
       eighty_vols += 80;

    total_vols = eighty_vols + vol_num;

    old_vol_num = vol_num;

    RPGC_log_msg(GL_INFO, "---------- volume: %d total %d ----------\n",
                 vol_num, total_vols);

    /* Add REALTIME/REPLAY handling difference later
     * if(data_stream == PGM_REALTIME_STREAM) ... */

    /* Read user requests */

    num_requests = get_user_requests(&request_list);

    if(num_requests <= 0)
       RPGC_log_msg (GL_INFO, "num_requests %d <= 0", num_requests);

    /* Process request loop */

    for(i = 0; i < num_requests; i++)
    {
       request = (User_array_t*) request_list + i;

       /* Set product dependent parameter structure to 0 */

       reset_prod_dep(&prod_dep);

       /* Put user request start/end times into prod_dep.
        * We're not using the user request time.
        *
        * ret = user_time_to_unix_time(request, &start_time, &end_time,
        *                              &prod_dep,
        *                              year, month, day,
        *                              hour, minute, second);
        * if(ret == -1)
        * {
        *   RPGC_log_msg (GL_INFO, ">> user_time_to_unix_time() error");
        *
        *   RPGC_abort_request(request, PGM_PROD_NOT_GENERATED);
        *   return -1;
        * } */

       if(request->ua_prod_code == HOURLY_PROD_ID)
       {
          /* We need lt_accum_buf->supl.hrly_endtime
           * to get a time for hourly comparison. If hrly_endtime
           * is 0, then there is no DP hourly buffer to do a gauge
           * comparison with. We could start keeping track of hourly
           * buffers in a circular queue like DP long term accum does,
           * but it was decided that was too much effort. */

          if(lt_accum_buf->supl.hrly_endtime <= 0L)
          {
             RPGC_log_msg(GL_INFO,
                          "hourly end time %ld, %s",
                          lt_accum_buf->supl.hrly_endtime,
                          "no hourly buffer to compare against");

             memset(old_dp_hourly, 0, MAX_GAUGES * sizeof(float));

             RPGC_abort_request(request, PGM_PROD_NOT_GENERATED);
             continue;
          }

          /* Use the time info in lt_accum_buf to obtain year, month, day,
           * hour, minute, second, and pass them to user_time_to_unix_time()
           * later on */

          ret = RPGCS_unix_time_to_ymdhms(lt_accum_buf->supl.hrly_endtime,
                                          &year, &month, &day,
                                          &hour, &minute, &second);
          if(ret != 0)
          {
             RPGC_log_msg(GL_INFO, "error calling RPGCS_unix_time_to_ymdhms");

             RPGC_abort_request(request, PGM_PROD_NOT_GENERATED);
             continue;
          }

          RPGC_log_msg(GL_INFO, "  Time: %02d/%02d/%04d %02d:%02d:%02d",
                       month, day, year, hour, minute, second);

          /* Generate the product only if in a different hour */

          if(generate_product_hourly(year, month, day, hour) == FALSE)
          {
             RPGC_abort_request(request, PGM_PROD_NOT_GENERATED);
             continue;
          }

          if(lt_accum_buf->supl.hrly_begtime ==
             lt_accum_buf->supl.hrly_endtime) /* no hourly range */
          {
             RPGC_log_msg (GL_INFO, ">> no hourly range");

             RPGC_abort_request(request, PGM_PROD_NOT_GENERATED);
             continue;
          }

          /* Write to the output product */

          memset(&metadata, 0, metadata_size);

          metadata.start_time = lt_accum_buf->supl.hrly_begtime;
          metadata.end_time   = lt_accum_buf->supl.hrly_endtime;

          write_to_output_product(HOURLY, &metadata, request,
                                  &prod_dep, lt_accum_buf,
                                  lt_accum_buf->One_Hr_unbiased,
                                  pps_hourly,
                                  vol_num, total_vols);

       } /* end hourly request */
       else if(request->ua_prod_code == STORM_PROD_ID)
       {
          /* Generate a storm product, if required. */

          generate_product_for_a_storm(lt_accum_buf, legacy,
                                       request, &prod_dep,
                                       vol_num, total_vols);
       } /* end storm request */

    } /* end loop over num_requests */

    if(request_list != NULL)
       free(request_list);

    /* RPGC_rel_inbuf(cdbe); */
    RPGC_rel_inbuf(lt_accum_buf);
    RPGC_rel_inbuf(legacy);

    return 1;

} /* end task_handler() ================================== */

/******************************************************************************
   Function name: get_user_requests()

   Description:
   ============
      Read user request information, i.e. end time/time span in minutes.

   Inputs:
      request list, to be filled

   Outputs:
      request list, filled

   Return:
      number of requests; -1 - error

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2010    0000       Zhan Zhang         Initial implementation
******************************************************************************/

int get_user_requests(User_array_t * * request_list)
{
    int num_reqs = 0;

    /* Read user requests */

    *request_list = (User_array_t *) RPGC_get_customizing_data(-1, &num_reqs);

    if(request_list == NULL)
    {
        RPGC_log_msg(GL_INFO, ">>Get_user_requests(): no requests found\n");
        return -1;
    }

    return num_reqs;

} /* end get_user_requests() ================================== */

/******************************************************************************
   Function name: user_time_to_unix_time()

   Description:
   ============
      It converts a user request to start time and end time in the format
      of Unix time.

   Inputs:
      user request; year, month, day, hour, minute, second,
      pointer of type prod_dep_para_t

   Outputs:
      start time and end time in Unix format, prod_dep parameters filled.

   Return:
      0 - OK, -1 - error

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2010    0000       Zhan Zhang         Initial implementation
******************************************************************************/

int user_time_to_unix_time(User_array_t * user_request,
                           time_t *start_time, time_t *end_time,
                           prod_dep_para_t * prod_dep,
                           int year, int month, int day,
                           int hour, int minute, int second)
{
    int user_specified_end_time_in_second;
    int span = user_request->ua_dep_parm_1;

    int ret;

    int julian_date;

    /* Used ONLY for function that converts the format of start time from
     * Unix Time to YMDHMS, so as to fill in prod_dep->param_6 */

    int year_temp, month_temp, day_temp, hour_temp, minute_temp, second_temp;

    if (user_request->ua_dep_parm_0 == -1)
        user_specified_end_time_in_second = hour * MINS_PER_HOUR *
                          SECS_PER_MINUTE + minute * SECS_PER_MINUTE + second;
    else
        user_specified_end_time_in_second = user_request->ua_dep_parm_0 *
                                            SECS_PER_MINUTE;

    /* Compute the end_time by using YMD from the lt_accum_buf and
     * user_specified_end_time_in_second  */

    ret = RPGCS_ymdhms_to_unix_time(end_time, year, month, day,
           user_specified_end_time_in_second / SECS_PER_MINUTE / MINS_PER_HOUR,
           user_specified_end_time_in_second / SECS_PER_MINUTE % MINS_PER_HOUR,
           user_specified_end_time_in_second % SECS_PER_MINUTE);

    if (ret == -1)
    {
        RPGC_log_msg(GL_INFO,
                     ">>user specified time error, ret %d\n",
                     ret);
        return -1;
    }

    /* Compute the end_time in Unix_time format
     *
     * If user_specified_end_time_in_second > the end_time quantity
     * obtained from the triggering lt_accum_buf, which is represented using
     * hour-minute-second. In this case, we simply subtract 1 day from
     * the already computed end_time */

    if (hour * MINS_PER_HOUR * SECS_PER_MINUTE + minute * SECS_PER_MINUTE
             + second < user_specified_end_time_in_second)
    {
        *end_time -= SECONDS_PER_DAY;

        /* Recompute year_temp, month_temp, day_temp, which are used
         * by halfword 48. */

        RPGCS_unix_time_to_ymdhms(*end_time, &year_temp, &month_temp, &day_temp,
                                  &hour_temp, &minute_temp, &second_temp);
    }
    else /* use year, month, day passed in */
    {
        year_temp  = year;
        month_temp = month;
        day_temp   = day;
    }

    /* compute start_time */

    *start_time = *end_time - span * SECS_PER_MINUTE;

    /* put time related info into the product dependent structure */

    /* halfword 27 - End time in minutes */

    prod_dep->param_1 = check_time((short) (user_specified_end_time_in_second /
                                    SECS_PER_MINUTE));

    /* halfword 28 - Time span in minutes */

    prod_dep->param_2 = check_time_span((short) span);

    /* halfword 48 - End date (Julian date) */

    RPGCS_date_to_julian(year_temp, month_temp, day_temp, &julian_date);

    prod_dep->param_5 = check_date((short) julian_date);

    /* halfword 49 - Start time, in minutes */

    RPGCS_unix_time_to_ymdhms(*start_time, &year_temp, &month_temp, &day_temp,
                              &hour_temp, &minute_temp, &second_temp);

    prod_dep->param_6 = check_time((short) (minute_temp + (60 * hour_temp)));

    return 0;

} /* end user_time_to_unix_time() ================================== */

/******************************************************************************
   Function name: calculate_statistics()

   Description:
   ============
      Calculate statistics for a set of gauge-radar pairs.

   Inputs:
      double gauge_d[MAX_GAUGES] - the gauges, as doubles
      double radar_d[MAX_GAUGES] - the radars, as doubles

   Outputs:
      stats_t* stats - stats structure filled.

   Return:
      None

   Excel functions for comparison use are:

                                 mean: =AVERAGE($F2:$F39)/AVERAGE($G2:$G39)
                   standard deviation: =STDEV($F2:$F39)
 									
                    gauge-radar pairs: =COUNT($H2:$H39)
                      mean field bias: =AVERAGE($F2:$F39)/AVERAGE($G2:$G39)
                        additive bias: =SUM($H2:$H39)
                           mean error: =AVERAGE($H2:$H39)
                  mean absolute error: =SUMPRODUCT(ABS($H2:$H39))/
                                        COUNT($H2:$H39)
               root mean square error: =SQRT(SUMSQ($H2:$H39)/COUNT($H2:$H39))
                   standard deviation: =STDEV($H2:$H39)
              correlation coefficient: =CORREL($F2:$F39,$G2:$G39)
    fractional                   bias: =AVERAGE($H2:$H39)/AVERAGE($F2:$F39)
    fractional root mean square error: =SQRT(SUMSQ($H2:$H39)/COUNT($H2:$H39))/
                                        AVERAGE($F2:$F39)
    fractional     standard deviation: =SQRT($H53*$H53-$H52*$H52)

    The gauges run from cells F2 to F39 and the DP data runs from cells
    G2 to G39. DP-Gauge is from cells H2 to H39. The fractional bias is at H52
    and the fractional root mean square error is at H53.

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   12 Apr 2010    0000       James Ward         Initial version
******************************************************************************/

void calculate_statistics(double   gauge_d[MAX_GAUGES],
                          double   radar_d[MAX_GAUGES],
                          stats_t* stats)
{
    int    i;
    double rmg[MAX_GAUGES];   /* radar minus gauge       */
    double ab;                /* additive bias           */
    double cov;               /* radar, gauge covariance */

    /* ----- means ---- */

    stats->gauge_mean = gsl_stats_mean(gauge_d, 1, stats->gr_pairs);
    stats->radar_mean = gsl_stats_mean(radar_d, 1, stats->gr_pairs);

    stats->got_gauge_mean = TRUE;
    stats->got_radar_mean = TRUE;

    /* ----- standard deviations ---- */

    if(stats->gr_pairs < 2)
    {
       stats->got_gauge_sd = FALSE;
       stats->got_radar_sd = FALSE;
    }
    else
    {
       stats->gauge_sd = gsl_stats_sd_m(gauge_d, 1, stats->gr_pairs,
                                                    stats->gauge_mean);
       stats->radar_sd = gsl_stats_sd_m(radar_d, 1, stats->gr_pairs,
                                                    stats->radar_mean);

       stats->got_gauge_sd = TRUE;
       stats->got_radar_sd = TRUE;
    }

    /* ----- mean field bias ----- */

    if(fabs(stats->radar_mean) < TOLERANCE)
    {
       stats->got_mfb = FALSE;
    }
    else if(fabs(stats->gauge_mean) < TOLERANCE)
    {
       stats->mfb = 0.0;

       stats->got_mfb = TRUE;
    }
    else /* radar_mean, gauge_mean > 0 */
    {
       stats->mfb = stats->gauge_mean / stats->radar_mean;

       stats->got_mfb = TRUE;
    }

    /* ----- additive bias ----- */

    memset(rmg, 0, MAX_GAUGES * sizeof(double));

    ab = 0.0;

    for(i=0; i<stats->gr_pairs; i++)
    {
        rmg[i] = radar_d[i] - gauge_d[i];
        ab    += rmg[i];
    }

    stats->ab = ab;

    stats->got_ab = TRUE;

    /* ----- mean error ----- */

    stats->me = gsl_stats_mean(rmg, 1, stats->gr_pairs);

    stats->got_me = TRUE;

    /* ----- mean absolute error ----- */

    stats->mae = gsl_stats_absdev_m(rmg, 1, stats->gr_pairs, 0.0);

    stats->got_mae = TRUE;

    /* ----- root mean square error ----- */

    stats->rmse = gsl_stats_sd_with_fixed_mean(rmg, 1, stats->gr_pairs, 0.0);

    stats->got_rmse = TRUE;

    /* ----- standard deviation ----- */

    if((stats->gr_pairs < 2) || (stats->got_me == FALSE))
    {
       stats->got_sd = FALSE;
    }
    else
    {
       stats->sd = gsl_stats_sd_m(rmg, 1, stats->gr_pairs, stats->me);

       stats->got_sd = TRUE;
    }

    /* ----- correlation coefficient -----
     *
     * Correlation coefficient formula is from:
     *
     * http://en.wikipedia.org/wiki/Correlation_and_dependence */

    if(stats->gr_pairs < 2)
    {
       stats->got_cc = FALSE;
    }
    else /* stats->gr_pairs > 1 */
    {
       cov = gsl_stats_covariance_m(radar_d, 1,
                                    gauge_d, 1,
                                    stats->gr_pairs,
                                    stats->radar_mean,
                                    stats->gauge_mean);


       if((fabs(stats->radar_sd) < TOLERANCE) ||
          (fabs(stats->gauge_sd) < TOLERANCE))
       {
          stats->got_cc = FALSE;
       }
       else
       {
          stats->cc = cov / (stats->radar_sd * stats->gauge_sd);

          stats->got_cc = TRUE;
       }
    } /* end stats->gr_pairs > 1 */

    /* Fractional formulas are from "Rainfall Estimation with a Polarimetric
     * Prototype of WSR-88D" by Ryzhkov, Giangrande, and Schurr, Journal of
     * Applied Meteorology, Volume 44, 2005. */

    /* ----- fractional bias ----- */

    if((stats->got_me) && (stats->gauge_mean >= TOLERANCE))
    {
       stats->fb = stats->me / stats->gauge_mean;

       stats->got_fb = TRUE;
    }
    else
    {
       stats->got_fb = FALSE;
    }

    /* ----- fractional root mean square error ----- */

    if((stats->got_rmse) && (stats->gauge_mean >= TOLERANCE))
    {
       stats->frmse = stats->rmse / stats->gauge_mean;

       stats->got_frmse = TRUE;
    }
    else
    {
       stats->got_frmse = FALSE;
    }

    /* ----- fractional standard deviation ----- */

    if((stats->got_frmse) && (stats->got_fb))
    {
       stats->fsd = sqrt((stats->frmse * stats->frmse) -
                         (stats->fb    * stats->fb));

       stats->got_fsd = TRUE;
    }
    else
    {
       stats->got_fsd = FALSE;
    }

} /* end calculate_statistics() ================================== */

/******************************************************************************
   Function name: calculate_dp_pps_stats()

   Description:
   ============
      Calculate dp and pps statistics. calculate_statistics() is called
      twice, once for the dp-gauge pairs, and once for the pps-gauge
      pairs.

   Inputs:
      int do_stats                          - TRUE -> calculate the stats
      Gauges_Buf_t* gauges_buf              - gauge values
      int dp[MAX_AZM][MAX_BINS]             - dp    values
      int pps[MAX_AZM][MAX_2KM_RESOLUTION]  - pps   values

   Outputs:
      messages - array filled with statistics

   Return:
      None.

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   24 Mar 2010    0000      James Ward         Initial implementation
******************************************************************************/

void calculate_dp_pps_stats(int do_stats,
                            Gauges_Buf_t* gauges_buf,
                            int dp[MAX_AZM][MAX_BINS],
                            int pps[MAX_AZM][MAX_2KM_RESOLUTION],
                            char messages[NUM_MSGS][MSG_LEN])
{
    int   i;

    int dp_i;  /* dual pol as an int */
    int pps_i; /* pps      as an int */

    double dp_d[MAX_GAUGES];    /* dual pol as a double */
    double pps_d[MAX_GAUGES];   /* pps      as a double */

    double gauges_match_dp[MAX_GAUGES];  /* gauge that matched dp  */
    double gauges_match_pps[MAX_GAUGES]; /* gauge that matched pps */

    stats_t dp_stats;
    stats_t pps_stats;

    char gauge_msg[40];
    char dp_msg[40];
    char pps_msg[40];

    memset(&dp_stats,  0, sizeof(stats_t));
    memset(&pps_stats, 0, sizeof(stats_t));

    if(do_stats == TRUE)
    {
       memset(dp_d, 0, MAX_GAUGES * sizeof(double));
       memset(pps_d,0, MAX_GAUGES * sizeof(double));

       memset(gauges_match_dp, 0, MAX_GAUGES * sizeof(double));
       memset(gauges_match_pps,0, MAX_GAUGES * sizeof(double));

       for(i=0; i<num_gauges; i++)
       {
          if(gauges_buf->gauges[i] >= TOLERANCE)
          {
             /* Check Dual Pol */

             dp_i = dp[gauges[i].azm_s][gauges[i].dp_bin];

             if((dp_i != QPE_NODATA) && (dp_i > 0))
             {
                 gauges_match_dp[dp_stats.gr_pairs] = gauges_buf->gauges[i];

                 /* Convert DP from (1000s of inches) to inches */

                 dp_d[dp_stats.gr_pairs] = dp_i / 1000.0;

                 dp_stats.gr_pairs++;
             }

             /* Check PPS */

             pps_i = pps[gauges[i].azm_s][gauges[i].pps_bin];

             if((pps_i != INIT_VALUE) && (pps_i > 0))
             {
                 gauges_match_pps[pps_stats.gr_pairs] = gauges_buf->gauges[i];

                 /* Convert from (tenths of mm) to inches */

                 pps_d[pps_stats.gr_pairs] = (pps_i / 10.0) * MM_TO_IN;

                 pps_stats.gr_pairs++;
             }
          }
      }

      if(dp_stats.gr_pairs > 0)
      {
         calculate_statistics(gauges_match_dp,
                              dp_d,
                              &dp_stats);
      }

      if(pps_stats.gr_pairs > 0)
      {
         calculate_statistics(gauges_match_pps,
                              pps_d,
                              &pps_stats);
      }

    } /* end do_stats == TRUE */

    /* Fill in the messages */

    /* ----- Header ----- */

    sprintf(&(messages[0][0]), "%s%s%s%s%s%s",
            "                                       ",
            "Gauges",
            "         ",
            "DP",
            "           ",
            "PPS");

    /* ----- Means ----- */

    if(dp_stats.got_gauge_mean == TRUE)
    {
       sprintf(gauge_msg, "%10.2f",
               RPGC_NINT(dp_stats.gauge_mean * 100.0) / 100.0);
    }
    else
    {
       sprintf(gauge_msg, "%13s", "  UNAVAILABLE");
    }

    if(dp_stats.got_radar_mean == TRUE)
    {
       sprintf(dp_msg, "%13.2f",
               RPGC_NINT(dp_stats.radar_mean * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_radar_mean == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.radar_mean * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[1][0]), "%s%s:%s%s%s",
            "                              ",
            "mean",
            gauge_msg,
            dp_msg,
            pps_msg);

    /* ----- Standard deviations ----- */

    if(dp_stats.got_gauge_sd == TRUE)
    {
       sprintf(gauge_msg, "%10.2f",
               RPGC_NINT(dp_stats.gauge_sd * 100.0) / 100.0);
    }
    else
    {
       sprintf(gauge_msg, "%13s", "  UNAVAILABLE");
    }

    if(dp_stats.got_radar_sd == TRUE)
    {
       sprintf(dp_msg, "%13.2f",
               RPGC_NINT(dp_stats.radar_sd * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_radar_sd == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.radar_sd * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[2][0]), "%s%s:%s%s%s",
            "                ",
            "standard deviation",
            gauge_msg,
            dp_msg,
            pps_msg);

    /* ----- Spacer ----- */

    sprintf(&(messages[3][0]), " ");

    /* ----- Pairs Header ----- */

    sprintf(&(messages[4][0]), "%s%s%s%s",
            "                                      ",
            "DP-Gauge",
            "     ",
            "PPS-Gauge");

    /* ----- Gauge-Radar pairs ----- */

    sprintf(dp_msg,  "%8d",  dp_stats.gr_pairs);
    sprintf(pps_msg, "%13d", pps_stats.gr_pairs);

    sprintf(&(messages[5][0]), "%s%s:%s%s",
            "                 ",
            "gauge-radar pairs",
            dp_msg,
            pps_msg);

    /* All stats are rounded to nearest 0.01 */

    /* ----- mean_field_bias ----- */

    if(dp_stats.got_mfb == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.mfb * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_mfb== TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.mfb * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[6][0]), "%s%s:%s%s",
            "                   ",
            "mean field bias",
            dp_msg,
            pps_msg);

    /* ----- additive_bias ----- */

    if(dp_stats.got_ab == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.ab * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_ab== TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.ab * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[7][0]), "%s%s:%s%s",
            "                     ",
            "additive bias",
            dp_msg,
            pps_msg);

    /* ----- mean_error ----- */

    if(dp_stats.got_me == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.me * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_me == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.me * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[8][0]), "%s%s:%s%s",
            "                        ",
            "mean error",
            dp_msg,
            pps_msg);

    /* ----- mean_absolute_error ----- */

    if(dp_stats.got_mae == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.mae * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_mae == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.mae * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[9][0]), "%s%s:%s%s",
            "               ",
            "mean absolute error",
            dp_msg,
            pps_msg);

    /* ----- root_mean_square_error ----- */

    if(dp_stats.got_rmse == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.rmse * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_rmse == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.rmse * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[10][0]), "%s%s:%s%s",
            "            ",
            "root mean square error",
            dp_msg,
            pps_msg);

    /* ----- standard_deviation ----- */

    if(dp_stats.got_sd == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.sd * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_sd == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.sd * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[11][0]), "%s%s:%s%s",
            "      ",
            "g-r error standard deviation",
            dp_msg,
            pps_msg);

    /* ----- correlation_coefficient ----- */

    if(dp_stats.got_cc == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.cc * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_cc == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.cc * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[12][0]), "%s%s:%s%s",
            "           ",
            "correlation coefficient",
            dp_msg,
            pps_msg);

    /* ----- fractional_bias ----- */

    if(dp_stats.got_fb == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.fb * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_fb == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.fb * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[13][0]), "%s%s:%s%s",
            " ",
            "fractional                   bias",
            dp_msg,
            pps_msg);

    /* ----- fractional_rmse ----- */

    if(dp_stats.got_frmse == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.frmse * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_frmse == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.frmse * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[14][0]), "%s%s:%s%s",
            " ",
            "fractional root mean square error",
            dp_msg,
            pps_msg);

    /* ----- fractional_sd ----- */

    if(dp_stats.got_fsd == TRUE)
    {
       sprintf(dp_msg, "%10.2f",
               RPGC_NINT(dp_stats.fsd * 100.0) / 100.0);
    }
    else
    {
       sprintf(dp_msg, "%13s", "  UNAVAILABLE");
    }

    if(pps_stats.got_fsd == TRUE)
    {
       sprintf(pps_msg, "%13.2f",
               RPGC_NINT(pps_stats.fsd * 100.0) / 100.0);
    }
    else
    {
       sprintf(pps_msg, "%13s", "  UNAVAILABLE");
    }

    sprintf(&(messages[15][0]), "%s%s:%s%s",
            " ",
            "fractional     standard deviation",
            dp_msg,
            pps_msg);

    /* Print statistics for debugging */

    for(i=0; i < NUM_MSGS; i++)
       RPGC_log_msg (GL_INFO, messages[i]);

} /* end calculate_dp_pps_stats() ================================== */

/******************************************************************************
   Function name: write_to_output_product()

   Description:
   ============
      It builds the output product, 179 or 180

   Inputs:
      dua_accum_grid, metadata of type dua_accum_buf_metadata_t, request_list,
      pointer of type prod_dep_para_t, vol_num, total_vols

   Outputs:
      Product 179 or 180 filled in.

   Return:
      0 - OK, -1 - error

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   -----------    -------    -----------------  ----------------------
    2 Jan 2010    0000       Zhan Zhang         Initial implementation
   13 Apr 2010    0001       James Ward         Added product 179
******************************************************************************/

int write_to_output_product(int type,
                            dua_accum_buf_metadata_t* metadata,
                            User_array_t* request_list,
                            prod_dep_para_t* prod_dep,
                            LT_Accum_Buf_t* lt_accum_buf,
                            int dp[MAX_AZM][MAX_BINS],
                            int pps[MAX_AZM][MAX_2KM_RESOLUTION],
                            int vol_num,
                            int total_vols)
{
    int   iostat = 0;
    void* outbuf = NULL; /* pointer to the output buffer */

    Graphic_product* hdr;  /* graphic product header */
    Symbology_block* sym;  /* symbology block        */

    float product_scale  = 1.0;
    float product_offset = 0.0;

    unsigned char* temp_p; /* temporary pointer to facilitate writing of
                            * the code segment that writes missing period
                            * flag/null product flag into the final product */

    Gauges_Buf_t gauges_buf;

    char header[HEADER_LEN];
    char messages[NUM_MSGS][MSG_LEN];

    int positive_gauges;

    static unsigned int graphic_size   = sizeof(Graphic_product);
    static unsigned int sym_size       = sizeof(Sym_block_t);
    static unsigned int tab_hdr_size   = sizeof(TAB_header_t);
    static unsigned int accum_buf_size = sizeof(dua_accum_buf_t);

    char* tab_start = NULL;
    int   tab_size  = 0;
    int   msg_len   = 0; /* RPGC_prod_hdr() wants a pointer */

    Graphic_product* hdr_copy; /* product header copy for TAB */

    /* The following are used to build the gauge product packet1s */

    int   hourly_num_packet1;
    short hourly_packet1_index[MAX_GAUGES];
    char  hourly_packet1_txt[MAX_GAUGES][MAX_PACKET1];

    int   storm_num_packet1;
    short storm_packet1_index[MAX_GAUGES];
    char  storm_packet1_txt[MAX_GAUGES][MAX_PACKET1];

    char outbuf_name[20];

    int  prod_id;

    /* Initialize buffers */

    memset(header,   0, HEADER_LEN);
    memset(messages, 0, NUM_MSGS * MSG_LEN);

    if(type != FEATURES)
    {
       /* Get the positive gauges. So we do one web query, we
        * also store the gauges in thousands of inches. The gauge
        * buffer is built right after the radar buffer. */

       positive_gauges = query_gauges_web(metadata->start_time,
                                          metadata->end_time,
                                          gauges,
                                          num_gauges,
                                          &gauges_buf);
       if(positive_gauges == 0)
       {
          memset(old_gauge_hourly, 0, MAX_GAUGES * sizeof(float));
          memset(old_gauge_storm,  0, MAX_GAUGES * sizeof(float));
       }

       /* Don't calculate statistics on a null product. */

       if(metadata->null_product_flag == FALSE)
          calculate_dp_pps_stats(TRUE,  &gauges_buf, dp, pps, messages);
       else
          calculate_dp_pps_stats(FALSE, &gauges_buf, dp, pps, messages);
    }

    if(type == HOURLY)
    {
       write_to_files(&gauges_buf, dp, pps,
                      metadata, type,
                      header, messages,
                      vol_num, total_vols,
                      &hourly_num_packet1,
                      hourly_packet1_index,
                      hourly_packet1_txt);

        strcpy(outbuf_name, HOURLY_PRODUCT_NAME);

        prod_id = HOURLY_PROD_ID;
    }
    else if(type == STORM)
    {
       write_to_files(&gauges_buf, dp, pps,
                      metadata, type,
                      header, messages,
                      vol_num, total_vols,
                      &storm_num_packet1,
                      storm_packet1_index,
                      storm_packet1_txt);

       strcpy(outbuf_name, STORM_PRODUCT_NAME);

       prod_id = STORM_PROD_ID;
    }
    else /* type == FEATURES */
    {
       strcpy(outbuf_name, FEATURES_PRODUCT_NAME);

       prod_id = FEATURES_PROD_ID;
    }

    /* Get the output buffer */

    if(request_list != NULL)
    {
       outbuf = RPGC_get_outbuf_by_name_for_req(outbuf_name,
                                                accum_buf_size,
                                                request_list,
                                                &iostat);
    }
    else /* no request list */
    {
       outbuf = RPGC_get_outbuf_by_name(outbuf_name,
                                        accum_buf_size,
                                        &iostat);
    }

    if(outbuf == NULL)
    {
        RPGC_log_msg (GL_INFO, ">> %s, prod_id %d, outbuf == NULL"
                      "RPGC_get_outbuf failed", prod_id);
        RPGC_abort_request(request_list, iostat);
        return -1;
    }
    else if(iostat != RPGC_NORMAL)
    {
        RPGC_log_msg (GL_INFO, ">> %s, prod_id %d, iostat != RPGC_NORMAL"
                      "RPGC_get_outbuf failed", prod_id);
        RPGC_abort_request(request_list, iostat);
        return -1;
    }

    sym = (Symbology_block *) ((char *) outbuf + graphic_size);

    if(metadata->null_product_flag) /* make a null product */
    {
       make_null_symbology_block((char *) outbuf,
                          metadata->null_product_flag,
                          outbuf_name,
                          lt_accum_buf->qpe_adapt.accum_adapt.restart_time,
                          lt_accum_buf->supl.last_time_prcp,
                          lt_accum_buf->qpe_adapt.prep_adapt.rain_time_thresh);

       prod_dep->param_4 = 0; /* when null_product_flag == 1, do not write to
                               * symbology, just set the max_value component in
                               * prod_dep structure to 0 for later on writing of
                               * product description block.
                               */
    }
    else if(type == HOURLY)
    {
       add_gauges_block((char*) outbuf,
                         hourly_num_packet1,
                         hourly_packet1_index,
                         hourly_packet1_txt);
    }
    else if(type == STORM)
    {
       add_gauges_block((char*) outbuf,
                         storm_num_packet1,
                         storm_packet1_index,
                         storm_packet1_txt);
    }
    else /* type == FEATURES */
    {
       add_features((char*) outbuf);
    }

    /* Add the TAB right after the symbology block */

    RPGC_get_product_int((void *) &(sym->block_len), &sym_size);

    tab_start = (char*) (outbuf + graphic_size + sym_size);

    tab_size = gauges_tab(tab_start, header, messages);

    /************* start filling in PDB of Graphic Product Header *************/

    hdr = (Graphic_product *) outbuf; /* cast the output buffer pointer to
                                         * a graphic product header pointer */

    /* Initialize graphic product header to NULL */

    memset(hdr, 0, graphic_size);

    /* Initialize some fields in the product description block */

    RPGC_prod_desc_block((void *) outbuf, prod_id, vol_num);

    /* This is a digital, 8-bit, 256-level product */

    RPGC_set_product_float((void*) &(hdr->level_1), product_scale);
    RPGC_set_product_float((void*) &(hdr->level_3), product_offset);

    /* According to Brian Klein, level_5 = hw 35 is reserved
     * by the FAA to store a logarithmic scale */

    hdr->level_6 = UCHAR_MAX; /* hw 36, 255 = max data level */
    hdr->level_7 =         1; /* hw 37 */
    hdr->level_8 =         0; /* hw 38 */

    /* Set halfword 29 - elevation number to 0 for volume based product */

    hdr->elev_ind = 0;

    /* Get values of some unset product dependent parameters */

    /*  We'll need this later when bias gets implemented for dual pol
     *
     *  prod_dep->param_7 =  (short) RPGC_NINT(SCALE_MEAN_FIELD_BIAS *
     *                               check_mean_field_bias(metadata->bias));
     */

    /* WARNING - This needs to be changed once bias is actually being applied
     *   to the products
     */

    prod_dep->param_7 =  (short) 100;

    /* The low byte of param_3 is the null product flag, the high byte
     * is the missing period flag. */

    /* temp_p is a temporary pointer to facilitate code writing */

    temp_p = (unsigned char *) (&(prod_dep->param_3));

    if(metadata->null_product_flag)
        *temp_p = (unsigned char) metadata->null_product_flag;
    else
        *temp_p = (unsigned char) 0;

    temp_p++; /* move up 1 byte */

    *temp_p = (unsigned char) metadata->missing_period_flag;

    /* Fill in product dependent parameters in product description block */

    RPGC_set_dep_params((void *) outbuf, (short *) prod_dep);

    /* Set three WORDs that holds offsets (in half words) of
     * three blocks, ie, Symbology block, GAB, and TAB, respectively,
     * from beginning of the product.
     *
     * Note 60 = graphic_size / sizeof(short) */

    RPGC_set_prod_block_offsets((void *) outbuf,
            60, /* offset to symbology block */
             0, /* offset to GAB */
            (graphic_size + sym_size) / sizeof(short)); /* offset to TAB */

    /************** end filling in PDB of Graphic Product Header **********/

    msg_len = sym_size + tab_size;

    RPGC_prod_hdr(outbuf, prod_id, &msg_len);

    /* Copy the main msg header to the TAB */

    hdr_copy = (Graphic_product*) (tab_start + tab_hdr_size);

    memcpy (hdr_copy, hdr, graphic_size);

    /* Correct TAB msg_len, it's not the main msg_len */

    msg_len = tab_size - tab_hdr_size;

    RPGC_set_product_int(&(hdr_copy->msg_len), msg_len);

    /* End of NEW */

    RPGC_rel_outbuf((void *) outbuf, FORWARD);

    return 0;

} /* end write_to_output_product() ================================== */

/******************************************************************************
   Function name: reset_prod_dep()

   Description:
   ============
      Resets each member of a product-dependent-parameter structure to 0

   Inputs:
      pointer to a product-dependent-parameter structure

   Outputs:
      product-dependent-parameter structure that has been reset

   Return:
      None.

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   2 Jan 2010    0000       Zhan Zhang         Initial implementation
******************************************************************************/

void reset_prod_dep(prod_dep_para_t * prod_dep)
{
    prod_dep->param_1  = 0;
    prod_dep->param_2  = 0;
    prod_dep->param_3  = 0;
    prod_dep->param_4  = 0;
    prod_dep->param_5  = 0;
    prod_dep->param_6  = 0;
    prod_dep->param_7  = 0;
    prod_dep->param_8  = 0;
    prod_dep->param_9  = 0;
    prod_dep->param_10 = 0;

    return;

} /* end reset_prod_dep() ================================== */

/******************************************************************************
   Function name: write_to_files()

   Description:
   ============
      It appends to gauge radar files on disk. One file is a descriptive text,
      the other is comma separated for easy spreadsheet import.

   Inputs:
      Gauges_Buf_t* gauges_buf             - gauge data
      int dp[MAX_AZM][MAX_BINS]            - dp    data
      int pps[MAX_AZM][MAX_2KM_RESOLUTION] - pps   data
      dua_accum_buf_metadata_t* metadata   - metadata, with times
      int type                             - HOURLY or STORM (no FEATURES)
      char header[HEADER_LEN]              - text/TAB header
      char messages[NUM_MSGS][MSG_LEN]     - stats messages
      int vol_num                          - volume number
      int total_vols                       - total volume number

   Outputs:
     Appends to the file.

   Return:
      0 - OK,
     -1 - cannot write to the file

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   -----------   -------    -----------------  ----------------------
   06 Mar 2010    0000       Zhan Zhang        Initial implementation
   29 Mar 2010    0001       James Ward        Added messages for debugging
   07 Feb 2011    0002       James Ward        Added human readable ending
                                               hour to hourly CSV.
******************************************************************************/

int write_to_files(Gauges_Buf_t* gauges_buf,
                   int dp[MAX_AZM][MAX_BINS],
                   int pps[MAX_AZM][MAX_2KM_RESOLUTION],
                   dua_accum_buf_metadata_t* metadata,
                   int type,
                   char header[HEADER_LEN],
                   char messages[NUM_MSGS][MSG_LEN],
                   int vol_num,
                   int total_vols,
                   int* num_packet1,
                   short packet1_index[MAX_GAUGES],
                   char packet1_txt[MAX_GAUGES][MAX_PACKET1])
{
    int   i, azm_i;
    int   dp_i, pps_i;
    float gauge_f, dp_f, pps_f;          /* gauge/dual pol/pps as floats  */
    float gauge_100, dp_100, pps_100;    /* gauge/dual pol/pps in .01  in */
    float gauge_1000, dp_1000, pps_1000; /* gauge/dual pol/pps in .001 in */
    float dp_diff, pps_diff;

    int start_year, start_month,  start_day;
    int start_hour, start_minute, start_second;

    int  end_year, end_month,  end_day;
    int  end_hour, end_minute, end_second;

    char end_str[40];

    char path[80];
    char txt_file[80];
    char dp_csv_file[80];
    char pps_csv_file[80];
    char dp_pps_csv_file[80];

    FILE* txt_fp        = NULL;
    FILE* dp_csv_fp     = NULL;
    FILE* pps_csv_fp    = NULL;
    FILE* dp_pps_csv_fp = NULL;

    short is_a_gauge_DP_pair  = FALSE;
    short is_a_gauge_PPS_pair = FALSE;

    static short hourly_first_time = TRUE;
    static short storm_first_time  = TRUE;

    char packet1_msg[20];

    char temp[HEADER_LEN];

    /* Build the path to the files */

    strcpy(path, getenv("HOME"));
    strcat(path, "/src/cpc102/tsk022/");
    strncat(path, site_adapt.rpg_name, 4);
    strcat(path, "_");

    if(type == HOURLY)
       strcat(path, "hourly");
    else if(type == STORM)
       strcat(path, "storm");

    strcat(path, "_gauge_");

    sprintf(txt_file,        "%sdp_pps.txt", path);
    sprintf(dp_csv_file,     "%sdp.csv",     path);
    sprintf(pps_csv_file,    "%spps.csv",    path);
    sprintf(dp_pps_csv_file, "%sdp_pps.csv", path);

    /* Open the .txt and .csv files. */

    txt_fp        = fopen(txt_file, "a+");
    dp_pps_csv_fp = fopen(dp_pps_csv_file, "a+");

    /* Uncomment the next 2 lines if you want pairs only .csv files
     *
     * dp_csv_fp  = fopen(dp_csv_file, "a+");
     * pps_csv_fp = fopen(pps_csv_file, "a+"); */

    /* Begin writing to the files */

    if((type == HOURLY) && hourly_first_time)
    {
       if(dp_csv_fp != NULL)
       {
          fprintf(dp_csv_fp, "Gauge ID,Latitude,Longitude,AZM,KM,");
          fprintf(dp_csv_fp, "End Year,End Month,End Day,");
          fprintf(dp_csv_fp, "End Hour,End Minute,End Second,");
          fprintf(dp_csv_fp, "Gauge,DP,DP-Gauge\n");
       }

       if(pps_csv_fp != NULL)
       {
          fprintf(pps_csv_fp, "Gauge ID,Latitude,Longitude,AZM,KM,");
          fprintf(pps_csv_fp, "End Year,End Month,End Day,");
          fprintf(pps_csv_fp, "End Hour,End Minute,End Second,");
          fprintf(pps_csv_fp, "Gauge,PPS,PPS-Gauge\n");
       }

       if(dp_pps_csv_fp != NULL)
       {
          fprintf(dp_pps_csv_fp, "Gauge ID,Latitude,Longitude,AZM,KM,");
          fprintf(dp_pps_csv_fp, "End Year,End Month,End Day,");
          fprintf(dp_pps_csv_fp, "End Hour,End Minute,End Second,");
          fprintf(dp_pps_csv_fp, "Gauge,DP,DP-Gauge,PPS,PPS-Gauge\n");
       }

       memset(old_gauge_hourly, 0, MAX_GAUGES * sizeof(float));
       memset(old_dp_hourly,    0, MAX_GAUGES * sizeof(float));
       memset(old_pps_hourly,   0, MAX_GAUGES * sizeof(float));

       hourly_first_time = FALSE;
    }
    else if((type == STORM) && storm_first_time)
    {
       if(dp_csv_fp != NULL)
       {
          fprintf(dp_csv_fp, "Gauge ID,Latitude,Longitude,AZM,KM,");
          fprintf(dp_csv_fp, "Gauge,DP,DP-Gauge\n");
       }

       if(pps_csv_fp != NULL)
       {
          fprintf(pps_csv_fp, "Gauge ID,Latitude,Longitude,AZM,KM,");
          fprintf(pps_csv_fp, "Gauge,PPS,PPS-Gauge\n");
       }

       if(dp_pps_csv_fp != NULL)
       {
          fprintf(dp_pps_csv_fp, "Gauge ID,Latitude,Longitude,AZM,KM,");
          fprintf(dp_pps_csv_fp, "Gauge,DP,DP-Gauge,PPS,PPS-Gauge\n");
       }

       memset(old_gauge_storm, 0, MAX_GAUGES * sizeof(float));
       memset(old_dp_storm,    0, MAX_GAUGES * sizeof(float));
       memset(old_pps_storm,   0, MAX_GAUGES * sizeof(float));

       storm_first_time = FALSE;
    }

    /* Construct the header, which is used by the text files
     * and in the TAB */

    RPGCS_unix_time_to_ymdhms(metadata->start_time,
                              &start_year, &start_month,  &start_day,
                              &start_hour, &start_minute, &start_second);

    sprintf(header, "From %02d/%02d/%04d %02d:%02d:%02dZ",
                     start_month, start_day,    start_year,
                     start_hour,  start_minute, start_second);

    RPGCS_unix_time_to_ymdhms(metadata->end_time,
                              &end_year, &end_month,  &end_day,
                              &end_hour, &end_minute, &end_second);

    sprintf(temp, " to %02d/%02d/%04d %02d:%02d:%02dZ",
                   end_month, end_day,    end_year,
                   end_hour,  end_minute, end_second);

    sprintf(end_str, "%04d,%02d,%02d,%02d,%02d,%02d",
                      end_year, end_month,  end_day,
                      end_hour, end_minute, end_second);

    strcat(header, temp);

    if(type == HOURLY)
        strcat(header, " Hourly");
    else if(type == STORM)
        strcat(header, " Storm");

    sprintf(temp, " vol %d total %d", vol_num, total_vols);

    strcat(header, temp);

    if(txt_fp != NULL)
    {
       fprintf(txt_fp,"%s\n", header);

       fprintf(txt_fp, "Gauge ID Latitude   Longitude  AZM    KM");
       fprintf(txt_fp, "    Gauge  DP     DP-G   PPS    PPS-G\n");
    }

    /* Reset the packet 1 info */

    *num_packet1 = 0;
    memset(packet1_index, 0, MAX_GAUGES * sizeof(short));
    memset(packet1_txt,   0, MAX_GAUGES * MAX_PACKET1 * sizeof(char));

    for(i=0; i < num_gauges; i++)
    {
       gauge_f = gauges_buf->gauges[i];

       azm_i = gauges[i].azm_s;

       /* Convert both DP (thousands of inches) and
        * PPS (tenths of mm) to inches */

       dp_i  = dp[azm_i][gauges[i].dp_bin];
       pps_i = pps[azm_i][gauges[i].pps_bin];

       dp_f  = dp_i / 1000.0;
       pps_f = (pps_i / 10.0) * MM_TO_IN;

       /* Do some gauge/radar consistency checks,
        * even if we're not going to print out the gauge */

       if(type == HOURLY)
       {
          #ifdef THIS_WILL_ONLY_WORK_IF_HOURLY_AND_STORM_SYNCED
          /* Hourly gauges/radars can decrease, but hourly
           * gauges/radars should be <= storm gauges/radars */

          if(txt_fp != NULL)
          {
             if(gauge_f > old_gauge_storm[i])
             {
                fprintf(txt_fp,
                        "[%d] hourly gauge_f %f > old_gauge_storm %f\n",
                        i, gauge_f, old_gauge_storm[i]);
             }

             if(dp_f > old_dp_storm[i])
             {
                fprintf(txt_fp,
                        "[%d] hourly dp_f %f > old_dp_storm %f\n",
                        i, dp_f, old_dp_storm[i]);
             }

             if(pps_f > old_pps_storm[i])
             {
                fprintf(txt_fp,
                        "[%d] hourly pps_f %f > old_pps_storm %f\n",
                        i, pps_f, old_pps_storm[i]);
             }
          }
          #endif

          old_gauge_hourly[i] = gauge_f;
          old_dp_hourly[i]    = dp_f;
          old_pps_hourly[i]   = pps_f;

       } /* end type == HOURLY */
       else if(type == STORM)
       {
          /* Storm gauges/radars should not decrease
           * over the life of the storm. We allow for the fact
           * that they might be reset (to 0) if a storm ends
           * and restarts again. */

          if(txt_fp != NULL)
          {
             if((fabsf(gauge_f) >= TOLERANCE) &&
                (gauge_f < old_gauge_storm[i]))
             {
                fprintf(txt_fp,
                        "%8s [%d] gauge_f %f < old_gauge_storm %f\n",
                        gauges[i].id, i, gauge_f, old_gauge_storm[i]);
             }

             if((dp_i != QPE_NODATA) &&
                (dp_i  > 0)          &&
                (dp_f < old_dp_storm[i]))
             {
                fprintf(txt_fp,
                        "%8s [%d] dp_f %f < old_dp_storm %f\n",
                        gauges[i].id, i, dp_f, old_dp_storm[i]);
             }

             if((pps_i != INIT_VALUE)    &&
                (fabsf(pps_f) >= TOLERANCE) &&
                (pps_f < old_pps_storm[i]))
             {
                fprintf(txt_fp,
                        "%8s [%d] pps_f %f < old_pps_storm %f\n",
                        gauges[i].id, i, pps_f, old_pps_storm[i]);
             }
          }

          old_gauge_storm[i] = gauge_f;
          old_dp_storm[i]    = dp_f;
          old_pps_storm[i]   = pps_f;

       } /* end type == STORM */

       /* Determine if it's a gauge-DP/gauge-PPS pair.
        * Note that DP or PPS could be NO DATA while
        * the other has a valid value. */

       if(fabsf(gauge_f) >= TOLERANCE)
       {
          if((dp_i != QPE_NODATA) && (dp_i > 0))
             is_a_gauge_DP_pair = TRUE;
          else
             is_a_gauge_DP_pair = FALSE;

          if((pps_i != INIT_VALUE) && (pps_i > 0))
             is_a_gauge_PPS_pair = TRUE;
          else
             is_a_gauge_PPS_pair = FALSE;
       }
       else /* gauge is 0, so not a DP or PPS pair */
       {
          is_a_gauge_DP_pair  = FALSE;
          is_a_gauge_PPS_pair = FALSE;
       }

       if((use_only_nonzero)             &&
          (is_a_gauge_DP_pair  == FALSE) &&
          (is_a_gauge_PPS_pair == FALSE))
       {
          continue; /* go to the next gauge */
       }

       /* If we got here, print the gauge-radar pair.
        * Since DP is only stored to 0.001, we round
        * Gauge/DP/PPS to the nearest 0.001 */

       gauge_100  = RPGC_NINT(gauge_f * 100.0)  / 100.0;
       gauge_1000 = RPGC_NINT(gauge_f * 1000.0) / 1000.0;

       if(txt_fp != NULL)
       {
          fprintf(txt_fp,
                  "%8s %9.6f%11.6f%6.1f%7.2f%7.3f",
                  gauges[i].id,
                  gauges[i].lat,
                  gauges[i].lon,
                  gauges[i].azm,
                  gauges[i].km,
                  gauge_1000);
       }

       if(is_a_gauge_DP_pair)
       {
          if(dp_csv_fp != NULL)
          {
             fprintf(dp_csv_fp,
                     "%s,%.6f,%.6f,%.1f,%.2f,",
                     gauges[i].id,
                     gauges[i].lat,
                     gauges[i].lon,
                     gauges[i].azm,
                     gauges[i].km);
             if(type == HOURLY)
             {
                fprintf(dp_csv_fp, "%s,", end_str);
             }
             fprintf(dp_csv_fp,"%.3f", gauge_1000);
          }
       }

       if(is_a_gauge_PPS_pair)
       {
          if(pps_csv_fp != NULL)
          {
             fprintf(pps_csv_fp,
                     "%s,%.6f,%.6f,%.1f,%.2f,",
                     gauges[i].id,
                     gauges[i].lat,
                     gauges[i].lon,
                     gauges[i].azm,
                     gauges[i].km);
             if(type == HOURLY)
             {
                fprintf(pps_csv_fp, "%s,", end_str);
             }
             fprintf(pps_csv_fp,"%.3f", gauge_1000);
          }
       }

       /* dp_pps_csv_fp matches the txt file */

       if(dp_pps_csv_fp != NULL)
       {
          fprintf(dp_pps_csv_fp,
                  "%s,%.6f,%.6f,%.1f,%.2f,",
                  gauges[i].id,
                  gauges[i].lat,
                  gauges[i].lon,
                  gauges[i].azm,
                  gauges[i].km);
          if(type == HOURLY)
          {
             fprintf(dp_pps_csv_fp, "%s,", end_str);
          }
          fprintf(dp_pps_csv_fp,"%.3f", gauge_1000);
       }

       packet1_index[*num_packet1] = i;

       /* Add gauge values to text strings */

       if(gauge_100 < (TRACE - TOLERANCE))
       {
          if(gauge_100 >= TOLERANCE) /* T is for Trace */
          {
             sprintf(packet1_txt[*num_packet1], "%s T",
                     gauges[i].id);
          }
          else /* gauge is 0 */
          {
             sprintf(packet1_txt[*num_packet1], "%s 0",
                     gauges[i].id);
          }
       }
       else /* gauge is non-zero */
       {
          sprintf(packet1_txt[*num_packet1], "%s %.2f",
                  gauges[i].id, gauge_100);
       }

       /* Add DP values to text strings. */

       dp_100  = RPGC_NINT(dp_f * 100.0)  / 100.0;
       dp_1000 = RPGC_NINT(dp_f * 1000.0) / 1000.0;

       dp_diff = dp_1000 - gauge_1000;
       if(fabsf(dp_diff) < TOLERANCE)
          dp_diff = 0.0;

       if(is_a_gauge_DP_pair)
       {
          if(txt_fp != NULL)
          {
             fprintf(txt_fp,
                     "%7.3f%7.3f",
                     dp_1000, dp_diff);
          }

          if(dp_csv_fp != NULL)
          {
             fprintf(dp_csv_fp,
                     ",%.3f,%.3f\n",
                     dp_1000, dp_diff);
          }

          if(dp_100 < (TRACE - TOLERANCE))
          {
             if(dp_100 >= TOLERANCE)
                sprintf(packet1_msg, " T");
             else
                sprintf(packet1_msg, " 0");
          }
          else
             sprintf(packet1_msg, " %.2f", dp_100);
       }
       else /* not a gauge-DP pair */
       {
          if(dp_i == QPE_NODATA)
          {
             if(txt_fp != NULL)
             {
                fprintf(txt_fp, "  NO DATA    NO DATA ");
             }

             sprintf(packet1_msg, " ND");
          }
          else /* dp_f must be 0 */
          {
             /* dp_diff = 0 - gauge_1000 = -gauge_1000 */

             if(txt_fp != NULL)
             {
                fprintf(txt_fp,
                        "%7.3f%7.3f",
                        0.0, -gauge_1000);
             }

             sprintf(packet1_msg, " 0");
          }
       }

       strcat(packet1_txt[*num_packet1], packet1_msg);

       /* Add PPS values to text strings */

       pps_100  = RPGC_NINT(pps_f * 100.0)  / 100.0;
       pps_1000 = RPGC_NINT(pps_f * 1000.0) / 1000.0;

       pps_diff = pps_1000 - gauge_1000;
       if(fabsf(pps_diff) < TOLERANCE)
          pps_diff = 0.0;

       if(is_a_gauge_PPS_pair)
       {
          if(txt_fp != NULL)
          {
             fprintf(txt_fp,
                     "%7.3f%7.3f",
                     pps_1000, pps_diff);
          }

          if(pps_csv_fp != NULL)
          {
             fprintf(pps_csv_fp,
                     ",%.3f,%.3f\n",
                     pps_1000, pps_diff);
          }

          if(pps_100 < (TRACE - TOLERANCE))
          {
             if(pps_100 >= TOLERANCE)
                sprintf(packet1_msg, " T");
             else
                sprintf(packet1_msg, " 0");
          }
          else
             sprintf(packet1_msg, " %.2f", pps_100);
       }
       else /* not a gauge-PPS pair */
       {
          /* INIT_VALUE is 0 for PPS, so we can't distinguish
           * a 0 value from NO DATA. Assume PPS means 0, because
           * ND should be rare. */

          /* pps_diff = 0 - gauge_1000 = -gauge_1000 */

          if(txt_fp != NULL)
          {
             fprintf(txt_fp,
                     "%7.3f%7.3f",
                     0.0, -gauge_1000);
          }

          sprintf(packet1_msg, " 0");
       }

       strcat(packet1_txt[*num_packet1], packet1_msg);

       (*num_packet1)++;

       if(txt_fp != NULL)
       {
          fprintf(txt_fp, "\n");
       }

       /* dp_pps_csv_fp matches the txt file */

       if(dp_pps_csv_fp != NULL)
       {
          fprintf(dp_pps_csv_fp,
                  ",%.3f,%.3f,%.3f,%.3f\n",
                  dp_1000,  dp_diff,
                  pps_1000, pps_diff);
       }

    } /* end loop over all gauges */

    /* Add the statistics to the text file. */

    if(txt_fp != NULL)
    {
       if(stats_everywhere)
       {
          fprintf(txt_fp, "\n");

          for(i=0; i < NUM_MSGS; i++)
          {
             fprintf(txt_fp, "%s\n", messages[i]);
          }
       }

       fprintf(txt_fp, "\n");

       fclose(txt_fp);
   }

   if(dp_csv_fp != NULL)
      fclose(dp_csv_fp);

   if(pps_csv_fp != NULL)
      fclose(pps_csv_fp);

   if(dp_pps_csv_fp != NULL)
      fclose(dp_pps_csv_fp);

    return 0;

} /* end write_to_files() ================================== */

/******************************************************************************
   Function name: add_gauges_block()

   Description:
   ============
      Makes the gauges block. It is based on make_null_symbology_block()

   Inputs:
      char* outbuf             - output buffer
      Gauges_Buf_t* gauges_buf - gauge data

   Outputs:
      Current gauge values written to output buffer.

   Return:
      None

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   -----------   -------    -----------------  ----------------------
   16 Apr 2010    0000      James Ward         Initial version
******************************************************************************/

void add_gauges_block(char* outbuf,
                      int num_packet1,
                      short packet1_index[MAX_GAUGES],
                      char packet1_txt[MAX_GAUGES][MAX_PACKET1])
{
   int   i;
   short i_start, j_start;
   int   data_layer_len  = 0;
   int   num_bytes_added = 0;

   char*            layer1_ptr = NULL;
   Symbology_block* sym_ptr    = NULL;

   static unsigned int graphic_size = sizeof(Graphic_product);

   sym_ptr    = (Symbology_block*) (outbuf + graphic_size);
   layer1_ptr = (char*) sym_ptr;

   /* The Symbology_block includes both the layer divider and the data_len
    * for the first layer. */

   sym_ptr->divider  = (short) -1; /* 2 bytes */
   sym_ptr->block_id = (short)  1; /* 2 bytes */

   /* We will fill in the (4 byte) sym_ptr->block_len later */

   sym_ptr->n_layers      = (short)  1; /* 2 bytes */
   sym_ptr->layer_divider = (short) -1; /* 2 bytes */

   /* We will fill in the (4 byte) sym_ptr->data_len length later */

   layer1_ptr += 16; /* 16 = 2 + 2 + 4 + 2 + 2 + 4 bytes */

   for(i=0; i<num_packet1; i++)
   {
      /* For geographic products, the i and j coordinates are
       * 1/4 Km screen coordinates relative to the radar location
       * (the center of the screen). cvg puts the upper left hand corner
       * of the text box at the screen location. */

      i_start = gauges[packet1_index[i]].x;
      j_start = gauges[packet1_index[i]].y;

      /* CVG appears to wrap lines after 80 characters,
       * so we try to keep ours under 80 in case AWIPS does also. */

      num_bytes_added = add_packet_1(layer1_ptr,
                                     i_start,
                                     j_start,
                                     packet1_txt[i]);

      data_layer_len += num_bytes_added;
      layer1_ptr     += num_bytes_added;

      /* Add a symbol at the gauge location. */

      num_bytes_added = add_packet_25(layer1_ptr,
                                      i_start,
                                      j_start,
                                      1);

      data_layer_len += num_bytes_added;
      layer1_ptr     += num_bytes_added;
   }

   /* If you want to test cvg's resolution, uncomment this block.
    *
    * int  j;
    * char test_text[80];
    *
    * for(i=-10; i<=10; i++)
    * {
    *    for(j=-10; j<=10; j++)
    *    {
    *       i_start = i * 4;
    *       j_start = j * 4;
    *
    *       sprintf(test_text, "%d %d", i_start / 4, j_start / 4);
    *
    *       num_bytes_added = add_packet_1(layer1_ptr,
    *                                      i_start,
    *                                      j_start,
    *                                      test_text);
    *
    *       data_layer_len += num_bytes_added;
    *       layer1_ptr     += num_bytes_added;
    *
    *       num_bytes_added = add_packet_25(layer1_ptr,
    *                                       i_start,
    *                                       j_start,
    *                                       1);
    *
    *       data_layer_len += num_bytes_added;
    *       layer1_ptr     += num_bytes_added;
    *    }
    * }
   */

   /* Write the length of the data layer to the product as an int (4 bytes).
    * According to the Code Manual, Vol. 2, p. 74, the length of
    * the data layer includes everything AFTER the length of
    * the data layer. */

   RPGC_set_product_int ((void *) &(sym_ptr->data_len), data_layer_len);

   /* Set the block length of the symbology block to account for all
    * the layer1 we've just written. The block length includes everything
    * in the symbology block, INCLUDING the divider and block_id. */

   RPGC_set_product_int ((void *) &(sym_ptr->block_len), 16 + data_layer_len);

} /* end add_gauges_block() ===================================== */

/* add_features writes features instead of gauges */

void add_features(char* outbuf)
{
   int   i;
   short i_start, j_start;
   int   data_layer_len  = 0;
   int   num_bytes_added = 0;
   char  packet1_txt[MAX_PACKET1];

   char*            layer1_ptr = NULL;
   Symbology_block* sym_ptr    = NULL;

   static unsigned int graphic_size = sizeof(Graphic_product);

   sym_ptr    = (Symbology_block*) (outbuf + graphic_size);
   layer1_ptr = (char*) sym_ptr;

   /* The Symbology_block includes both the layer divider and the data_len
    * for the first layer. */

   sym_ptr->divider  = (short) -1; /* 2 bytes */
   sym_ptr->block_id = (short)  1; /* 2 bytes */

   /* We will fill in the (4 byte) sym_ptr->block_len later */

   sym_ptr->n_layers      = (short)  1; /* 2 bytes */
   sym_ptr->layer_divider = (short) -1; /* 2 bytes */

   /* We will fill in the (4 byte) sym_ptr->data_len length later */

   layer1_ptr += 16; /* 16 = 2 + 2 + 4 + 2 + 2 + 4 bytes */

   for(i=0; i<NUM_FEATURES; i++)
   {
      /* For geographic products, the i and j coordinates are
       * 1/4 Km screen coordinates relative to the radar location
       * (the center of the screen). cvg puts the upper left
       * hand corner of the text box at the screen location. */

      i_start = feature_x[i];
      j_start = feature_y[i];

      /* CVG appears to wrap lines after 80 characters,
       * so we try to keep ours under 80 in case AWIPS does also.
       *
       * The text array passed into add_packet_1 can't be a
       * constant, as it might be expanded by 1 char for
       * byte alignment. */

      memset(packet1_txt, 0, MAX_PACKET1);

      strcpy(packet1_txt, feature_name[i]);

      num_bytes_added = add_packet_1(layer1_ptr,
                                     i_start,
                                     j_start,
                                     packet1_txt);

      data_layer_len += num_bytes_added;
      layer1_ptr     += num_bytes_added;

      /* Add a symbol at the gauge location. */

      num_bytes_added = add_packet_25(layer1_ptr,
                                      i_start,
                                      j_start,
                                      1);

      data_layer_len += num_bytes_added;
      layer1_ptr     += num_bytes_added;
   }

   /* Write the length of the data layer to the product as an int (4 bytes).
    * According to the Code Manual, Vol. 2, p. 74, the length of
    * the data layer includes everything AFTER the length of
    * the data layer. */

   RPGC_set_product_int ((void *) &(sym_ptr->data_len), data_layer_len);

   /* Set the block length of the symbology block to account for all
    * the layer1 we've just written. The block length includes everything
    * in the symbology block, INCLUDING the divider and block_id. */

   RPGC_set_product_int ((void *) &(sym_ptr->block_len), 16 + data_layer_len);

} /* end add_features() ===================================== */

/******************************************************************************
    Filename: gauge_radar_func.c

    Description:
    ============
    add_packet_25() adds a packet 25 to a buffer. It is based on add_packet_1()
    in cpc014/lib002. Hopefully, add_packet_25() can be moved inside dp_packet.c
    one day.

    Inputs: char* buffer  - where to place the text
            short i_start - i starting point
            short j_start - j starting point
            short radius  - radius of circle (1 to 512 pixels)

    Outputs: int number of bytes added

    See also Figure 3-14. Text and Special Symbol Packets - Packet Code 25
    (Sheet 1) in the Class I User ICD.

    Returns: number of bytes added, or NULL_POINTER (2)

    Called by: add_gauges_block

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ----------------------
    03 May 2010    0000      Ward               Initial implementation
******************************************************************************/

int add_packet_25(char* buffer, short i_start, short j_start, short radius)
{
   int    num_bytes = 0;
   short* short_ptr = NULL;

   /* Check for NULL pointers */

   if(pointer_is_NULL(buffer, "add_packet_25", "buffer"))
      return(NULL_POINTER);

   short_ptr = (short *) buffer;

   /**** Write packet header ****/

   *short_ptr = (short) 25; /* Write the packet code (25) */
   ++short_ptr;             /* Move up 1 short            */
   num_bytes += 2;

   /**** Write packet data length ****/

   *short_ptr = (short) 6; /* Write packet data length */
   ++short_ptr;            /* Move up 1 short          */
   num_bytes += 2;

   /**** Write packet data block ****/

   *short_ptr = i_start;  /* Write the i starting point */
   ++short_ptr;           /* Move up 1 short            */
   num_bytes += 2;

   *short_ptr = j_start;  /* Write the j starting point */
   ++short_ptr;           /* Move up 1 short            */
   num_bytes += 2;

   *short_ptr = radius;  /* Write the circle radius */
   ++short_ptr;          /* Move up 1 short         */
   num_bytes += 2;

   return(num_bytes);

} /* end add_packet_25() ===================================== */
