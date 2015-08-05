/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 12:50:50 $
 * $Id: dp_lt_accum_ComputeDiff.c,v 1.9 2012/03/12 12:50:50 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#include <a313hparm.h> /* FLG_RESTP - legacy flag "Reset Storm Total Precip" */
#include "dp_lt_accum_func_prototypes.h"

extern time_t start_time_storm_diff;

/******************************************************************************
    Filename: dp_lt_accum_diffs.c

    Description:
    ============
    compute_diff() computes the grids for the two difference products.  This is
    always done by subtracting the legacy scan-to-scan accumulation from the
    dual-pol grid (DP - Legacy).  Note that this function reads the legacy
    scan-to-scan accumulation grid (resolution 2Km X 1 degree) and converts
    it to the same resolution as dual-pol (250m X 1 degree).

       DOD (174) Digital One-Hour Difference Accumulation
       DSD (175) Digital Storm Total Difference Accumulation

    Note: A null product means that the product could have been
    generated, but was not, due to a condition. It does not indicate
    some sort of system failure. When we make a null product, we don't
    initialize the grid because it is not displayed.

    For the Storm Total Difference, we will use this logic:

    -----------------------|--------------------------
      Is a storm active?   |
                           |
     Legacy     Dual Pol   | DSD
    -----------------------|--------------------------
       F     |     F       | make a null product
    ---------|-------------|--------------------------
           else            | make a difference product
    --------------------------------------------------

    Note: The dual pol ST_active flag will become F if no rain has been
          detected in rain_time_thresh (60) min. This is set in qperate
          build_RR_Polar_Grid().

    Note: By initializing the legacy_ST_active to FALSE, if we don't
          get a legacy buffer, its defaults to the dual pol ST_active flag
          for resetting the difference totals.

    Other legacy flags that could have been used, but weren't:

       legacy_prcp_detec (FLG_RAINDET = 3)

    Inputs: S2S_Accum_Buf_t*  s2s_accum_buf     - latest scan to scan accum
            LT_Accum_Buf_t*   lt_accum_buf      - our saved data
            Circular_Queue_t* hourly_diff_circq - hourly diff circular queue
            Storm_Backup_t*   storm_diff_backup - storm diff backup

    Outputs: lt_accum_buf.One_Hr_diff      - with latest s2s accum added
             lt_accum_buf.Storm_Total_diff - with latest s2s accum added

    Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

    Change History
    ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------
    16 Oct 2007    0000       Stein, Ward        Initial implementation
    20 Oct 2010    0001       Ward               Added legacy_storm_begtime
******************************************************************************/

int compute_diff(S2S_Accum_Buf_t*  s2s_accum_buf,
                 LT_Accum_Buf_t*   lt_accum_buf,
                 Circular_Queue_t* hourly_diff_circq,
                 Storm_Backup_t*   storm_diff_backup)
{
   hyadjscn_large_buf_t*  legacy = NULL; /* legacy scan-to-scan accum buffer */
   S2S_Accum_Buf_t        diff_buf;      /* difference buffer                */
   int   opstat           = RPGC_NORMAL; /* legacy buffer error code         */
   short zero_scan        = FALSE;       /* TRUE -> zero legacy scan-to_scan */
   short zero_hourly      = FALSE;       /* TRUE -> zero legacy hourly accum */
   short no_hourly        = FALSE;       /* TRUE -> no   legacy hourly accum */
   short got_legacy_accum = FALSE;       /* TRUE -> got a legacy accum       */
   short legacy_ST_active = FALSE;       /* TRUE -> legacy storm is active   */
   short hi_res_legacy[MAX_AZM][MAX_BINS]; /* legacy in hi-res               */
   char  msg[200];                         /* stderr message                 */

   int year  = 0;
   int month = 0;
   int day   = 0;
   int hour  = 0;
   int min   = 0;
   int extra = 0;

   static unsigned int s2s_size = sizeof(S2S_Accum_Buf_t);

   static time_t legacy_storm_begtime = 0L;

   if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning compute_diff()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(s2s_accum_buf, "compute_diff", "s2s_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "compute_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(hourly_diff_circq, "compute_diff", "hourly_diff_circq"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_diff_backup, "compute_diff", "storm_diff_backup"))
      return(NULL_POINTER);

   /* Get a legacy accum buffer. HYADJSCN (104) is an unbiased grid;
    * it is produced in the legacy before bias is applied.
    *
    * 20080617 Note: If a task upstream of HYADJSCN does not abort correctly,
    * then RPGC_get_inbuf_by_name() may hang forever. There is no timeout.
    */

   legacy = (hyadjscn_large_buf_t*) RPGC_get_inbuf_by_name("HYADJSCN", &opstat);

   if((opstat != RPGC_NORMAL) || (legacy == NULL))
   {
      sprintf(msg, "%s %p or opstat = %d\n",
                   "RPGC_get_inbuf_by_name HYADJSCN failed:",
                   (void*) legacy, opstat);

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);

      legacy_ST_active     = FALSE;
      legacy_storm_begtime = 0L;
      got_legacy_accum     = FALSE;
   }
   else /* got a legacy buffer */
   {
      /* The legacy buffer can be: small  (no scan-to-scan or hourly accum)
       *                           medium (scan-to-scan, but no hourly accum)
       *                           large  (scan-to-scan, and hourly accum)
       *
       * We want scan-to-scan, so medium and large buffers are OK.
       *
       * Three flags: FLG_ZERSCN, FLG_ZERHLY, and FLG_NOHRLY,
       * from a313hparm.h, will help us figure out what size buffer we have.
       *
       * For debugging, the flag values are:
       *
       *   AVG_SCNDAT =  0
       *   AVG_SCNTIM =  1
       *   FLG_RESTP  =  4
       *   FLG_PCPBEG =  5
       *   FLG_ZERSCN = 25
       *   FLG_ZERHLY = 31
       *   FLG_NOHRLY = 32 */

      zero_scan   = legacy->medium.small.Supl[FLG_ZERSCN];
      zero_hourly = legacy->medium.small.Supl[FLG_ZERHLY];
      no_hourly   = legacy->medium.small.Supl[FLG_NOHRLY];

      if ((zero_scan == TRUE) && ((zero_hourly == TRUE) || (no_hourly == TRUE)))
      {
         /* We have a small_buf_t with no scan-to-scan/hourly accum. */

         if (DP_LT_ACCUM_DEBUG)
         {
            fprintf(stderr, "%s zero_scan: %hd, zero_hourly: %hd, "
		            "no_hourly: %hd\n", "Got a small_buf_t",
                            zero_scan, zero_hourly, no_hourly);
         }

         legacy_ST_active     = FALSE;
         legacy_storm_begtime = 0L;
         got_legacy_accum     = FALSE;
       }
      else /* medium or large buffer */
      {
         /* 20101020 Ward Legacy storm start borrowed from
          * ~/src/cpc014/tsk006/a3146e.ftn :
          *
          * IF (HYDRSUPL(FLG_RESTP).EQ.FLAG_SET .OR. STMTOT_FLG_FRST) THEN
          *    STMTOT_BDAT = HYDRSUPL(AVG_SCNDAT)
          *    STMTOT_BTIM = HYDRSUPL(AVG_SCNTIM)
          *
          * If FLG_RESTP is TRUE, there is no storm, so reset the storm
          * date/time. */

         if (legacy->medium.small.Supl[FLG_RESTP] == TRUE)
         {
            legacy_ST_active     = FALSE;
            legacy_storm_begtime = 0L;
         }
         else /* PPS has an active storm */
         {
            legacy_ST_active = TRUE;

            /* Note: After startup, dp_lt_accum will be called on the 2nd
             *       volume, so FLG_PCPBEG may be FALSE, as precip. began
             *       in the first volume. We also check if the storm begin
             *       time is 0. */

            if((legacy->medium.small.Supl[FLG_PCPBEG] == TRUE) ||
               (legacy_storm_begtime == 0L))
            {
               /* Convert the AVG_SCNDAT Julian date to YYYYMMDD */

               RPGCS_julian_to_date(legacy->medium.small.Supl[AVG_SCNDAT],
                                    &year, &month, &day);

               /* Convert the AVG_SCNTIM, seconds since midnight, to HHMMSS */

               extra = legacy->medium.small.Supl[AVG_SCNTIM];
               hour  = extra / SECS_PER_HOUR;

               extra -= (hour * SECS_PER_HOUR);
               min    = extra / SECS_PER_MIN;

               extra -= (min * SECS_PER_MIN);
               /* seconds is now extra */

               /* Convert the YYYYMMDD, HHMMSS to Unix time */

               RPGCS_ymdhms_to_unix_time(&legacy_storm_begtime,
                                         year, month, day,
                                         hour, min, extra);
            }
         }

         got_legacy_accum = TRUE;

      } /* end else medium/large buffer */

   } /* end got a legacy buffer */

   /* We need both a DP and a legacy grid to product a diff product. */

   if((s2s_accum_buf->supl.null_accum == FALSE) &&
      (got_legacy_accum == TRUE))
   {
      /* Convert low res legacy (2 Km X 1 degree) to high res legacy
       * (250 m * 1 degree). This also converts from 10s of millimeters
       * to thousandths of inches. */

      convert_grid_to_DP_res(legacy->medium.AccumScan, hi_res_legacy);

      /* Subtract the two grids to produce a difference grid.
       * Difference grids are never biased. */

      memcpy(&diff_buf, s2s_accum_buf, s2s_size);

      subtract_unbiased_short_grids(diff_buf.accum_grid, hi_res_legacy);

      /* Compute the hourly difference. */

      compute_hourly_diff(&diff_buf, s2s_accum_buf, lt_accum_buf,
                          hourly_diff_circq, legacy_ST_active);

      /* Compute the storm total difference. */

      compute_storm_diff(&diff_buf, s2s_accum_buf, lt_accum_buf,
                         storm_diff_backup, legacy_ST_active,
                         legacy_storm_begtime);

  } /* end if got both legacy and DP grids */
  else /* diff buff could not be computed */
  {
     /* Compute the hourly difference. */

     compute_hourly_diff(NULL, s2s_accum_buf, lt_accum_buf,
                         hourly_diff_circq, legacy_ST_active);

     /* Compute the storm total difference. */

     compute_storm_diff(NULL, s2s_accum_buf, lt_accum_buf,
                        storm_diff_backup, legacy_ST_active, 0);
  }

  /* We no longer need the legacy buffer, so release it. */

  if(legacy != NULL)
     RPGC_rel_inbuf((void*) legacy);

  return(FUNCTION_SUCCEEDED);

} /* end compute_diff() ================================================== */

/******************************************************************************
    Filename: dp_build_accum_diffs.c

    Description:
    ============
    compute_hourly_diff() computes the one-hour difference grid by adding the
    current scan-to-scan difference grid into the running 1-hr difference
    product.

       DOD (174) Digital One-Hour Difference Accumulation

    Inputs: S2S_Accum_Buf_t*  diff_buf          - difference buffer
            S2S_Accum_Buf_t*  s2s_accum_buf     - latest scan to scan accum
            LT_Accum_Buf_t*   lt_accum_buf      - our saved data
            Circular_Queue_t* hourly_diff_circq - hourly diff circular queue
            short             legacy_ST_active  - TRUE -> legacy storm is active

    Outputs: lt_accum_buf.One_Hr_diff      - with latest s2s accum added

    Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

    Change History
    ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------
    17 Dec 2007    0000       Ward               Initial implementation
    04 Mar 2009    0001       Ward               Added ST_Active check
    21 Oct 2010    0002       Ward               Null products now use current
                                                 time.
    27 Oct 2011    0003       Ward               Convert One_Hr_diff buffer to
                                                 8-bin resolution.
                                                 CCR: NA11-00324
******************************************************************************/

int compute_hourly_diff(S2S_Accum_Buf_t*  diff_buf,
                        S2S_Accum_Buf_t*  s2s_accum_buf,
                        LT_Accum_Buf_t*   lt_accum_buf,
                        Circular_Queue_t* hourly_diff_circq,
                        short             legacy_ST_active)
{
   int    i, j;                        /* indices                            */
   char   msg[200];                    /* stderr msg                         */
   time_t hourly_diff_begtime = 0L;    /* hourly diff beginning time         */
   time_t hourly_diff_endtime = 0L;    /* hourly diff ending time            */
   short  missing_period      = FALSE; /* TRUE -> 1-hr diff missing a period */

   if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning compute_hourly_diff()\n");

   /* Check for NULL pointers. diff_buf can be NULL. */

   if(pointer_is_NULL(lt_accum_buf, "compute_hourly_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(hourly_diff_circq, "compute_hourly_diff",
                                         "hourly_diff_circq"))
   {
      return(NULL_POINTER);
   }

   /* If one of the systems has a storm active, add in the precip */

   if((legacy_ST_active == TRUE) || (s2s_accum_buf->supl.ST_active_flg == TRUE))
   {
      /* One of the systems has a storm active. If we got a difference buffer,
       * update our totals. If we didn't get a difference buffer, do nothing,
       * and the hourly difference will be unchanged. */

      if(diff_buf != NULL) /* got a good difference buffer */
      {
         /* Add the new diff buffer to our hourly diff circular queue.
          * If the queue is empty, this will also initialize the 1-hr diff
          * so the accum_grid can be added to this running total. */

         if(CQ_Add_to_back (hourly_diff_circq,
                            diff_buf,
                            NULL,
                            lt_accum_buf->One_Hr_diff) != CQ_SUCCESS)
         {
           sprintf(msg, "%s %s\n",
                   "compute_hourly_diff:",
                   "CQ_Add_to_back() did not return CQ_SUCCESS!!!");

           RPGC_log_msg(GL_INFO, msg);
           if(DP_LT_ACCUM_DEBUG)
              fprintf(stderr, msg);
         }

         /* Update our running hourly diff. Unlike the hourly accumulation,
          * there is no max_hourly_acc threshold for a difference grid.
          * Note: The hourly difference is always unbiased, so no bias_flag
          * check is done. */

         add_unbiased_short_to_int(lt_accum_buf->One_Hr_diff,
                                   diff_buf->accum_grid,
                                   INT_MAX);

      } /* end if diff_buf not NULL */

   } /* end if one of the systems has a storm active */

   /* No storm is active. We could init the hourly diff product here,
    * but what if an ST_active flag is set to a < 1 hour trigger?
    * In either case, the hourly diff product will be trimmed to an hour
    * by the next CQ_Trim_To_Hour(). Old code:
    *
    * else
    * {
    *   init_hourly_diff(lt_accum_buf, hourly_diff_circq,
    *                    s2s_accum_buf->qpe_adapt.dp_adapt.Max_vols_per_hour);
    * } */

   /* Delete every accum_buf in the circular queue older than an hour
    * This will also subtract them from One_Hr_diff along the way.
    * We want to do this even if we don't get a diff_buf so our queue
    * stays current to an hour. */

   if(CQ_Trim_To_Hour(hourly_diff_circq,
                      NULL,
                      lt_accum_buf->One_Hr_diff,
                      INT_MAX,
                      s2s_accum_buf->supl.end_time) != CQ_SUCCESS)
   {
      sprintf(msg, "%s %s\n",
              "compute_hourly_diff:",
              "CQ_Trim_To_Hour() did not return CQ_SUCCESS!!!");

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   if(hourly_diff_circq->first == CQ_EMPTY) /* queue is empty */
   {
      /* Init the grid because there is no difference
       * product for this hour. It is not a null product, though.
       *
       * We always make a one-hour diff product (DOD 174), even if
       * we're not getting any legacy buffers. It's the only product
       * without a null product flag in halfword 30.
       *
       * In cvg the screen will be a solid white disk, but this is
       * an artifact of the cvg FL 1 color choice. Black is used
       * for the 0.0 level */

      for(i = 0; i < MAX_AZM; i++)
        for(j = 0; j < MAX_BINS; j++)
            lt_accum_buf->One_Hr_diff[i][j] = QPE_NODATA;

      /* 20101028 Ward - Old code, which used 0 as a volume time
       *                 for NULL products:
       *
       * hourly_diff_begtime = 0L;
       * hourly_diff_endtime = 0L;
       *
       * New code uses current QPE grid time for NULL products.
       *
       * The supl.end_time is the time of the 2nd rate grid that made
       * the scan-to-scan accumulation. The supl.begin_time is the
       * time of the 1st rate grid that made the scan-to-scan accumulation.
       * The time of a rate grid is an average time of all the elevations
       * that contributed to the hybrid scan. All these are different
       * from the current volume time, which is in the Scan Summary. */

      hourly_diff_begtime = s2s_accum_buf->supl.end_time;
      hourly_diff_endtime = s2s_accum_buf->supl.end_time;

      missing_period = FALSE;
   }
   else /* queue is not empty, make non-null product */
   {
      /* 20111027 Ward Convert One_Hr_diff buffer to 8-bin resolution.
       * CCR: NA11-00324 */

      convert_to_low_res(lt_accum_buf->One_Hr_diff);

      hourly_diff_begtime =
                   hourly_diff_circq->begin_time[hourly_diff_circq->first];
      hourly_diff_endtime =
                   hourly_diff_circq->end_time[hourly_diff_circq->last];

      missing_period = CQ_Get_Missing_Period(hourly_diff_circq);
   }

   /* Fill in product metadata */

   lt_accum_buf->supl.hrlydiff_begtime = hourly_diff_begtime;
   lt_accum_buf->supl.hrlydiff_endtime = hourly_diff_endtime;

   lt_accum_buf->supl.missing_period_One_hr_diff = missing_period;

   return(FUNCTION_SUCCEEDED);

} /* end compute_hourly_diff() =========================================== */

/******************************************************************************
    Filename: dp_build_accum_diffs.c

    Description:
    ============
    compute_storm_diff() computes the storm-total difference grid by adding the
    current scan-to-scan difference grid into the running storm-total difference
    product.

      DSD (175) Digital Storm Total Difference Accumulation

    Inputs: S2S_Accum_Buf_t* diff_buf          - difference buffer
            S2S_Accum_Buf_t* s2s_accum_buf     - latest scan to scan accum
            LT_Accum_Buf_t*  lt_accum_buf      - our saved data
            Storm_Backup_t*  storm_diff_backup - storm diff buffer
            short            legacy_ST_active  - TRUE -> legacy storm is active

    Outputs: lt_accum_buf.Storm_Total_diff     - with latest s2s accum added

    Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

    Change History
    ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------
    17 Dec 2007    0000       Ward               Initial implementation
    04 Mar 2009    0001       Ward               Switched order of ST_Active
                                                 and diff_buf checks so to match
                                                 storm logic
    21 Oct 2010    0002       Ward               Added legacy storm begin time.
                                                 Null products now use current
                                                 time.
    27 Oct 2011    0003       Ward               Convert One_Hr_diff buffer to
                                                 8-bin resolution.
                                                 CCR: NA011-00324
******************************************************************************/

int compute_storm_diff(S2S_Accum_Buf_t*  diff_buf,
                       S2S_Accum_Buf_t*  s2s_accum_buf,
                       LT_Accum_Buf_t*   lt_accum_buf,
                       Storm_Backup_t*   storm_diff_backup,
                       short             legacy_ST_active,
                       time_t            legacy_storm_begtime)
{
   time_t storm_diff_begtime = 0L;    /* storm diff beginning time           */
   time_t storm_diff_endtime = 0L;    /* storm diff ending time              */
   short  missing_period     = FALSE; /* TRUE -> storm diff missing a period */

   if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning compute_storm_diff()\n");

   /* Check for NULL pointers. diff_buf can be NULL. */

   if(pointer_is_NULL(s2s_accum_buf, "compute_storm_diff", "s2s_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "compute_storm_diff", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_diff_backup, "compute_storm_diff",
                                         "storm_diff_backup"))
   {
      return(NULL_POINTER);
   }

   /* If one of the systems has a storm active, add in the precip */

   if((legacy_ST_active == TRUE) || (s2s_accum_buf->supl.ST_active_flg == TRUE))
   {
      /* One of the systems has a storm active. If we got a difference buffer,
       * update our totals. If we didn't get an difference buffer, do nothing,
       * and the storm difference will be unchanged. */

      if(diff_buf != NULL) /* got a good difference buffer */
      {
         /* Add the difference grid to the storm-total difference product
          * Note: The storm difference is always unbiased,
          * so no bias_flag check is done. */

         add_unbiased_short_to_int(lt_accum_buf->Storm_Total_diff,
                                   diff_buf->accum_grid,
                                   INT_MAX);

         /* Add the difference grid to the storm-total backup */

         add_unbiased_short_to_int(storm_diff_backup->StormTotal,
                                   diff_buf->accum_grid,
                                   INT_MAX);

         /* Set the storm diff start time to the earliest of the PPS/QPE
          * storm starts. */

         storm_diff_backup->storm_begtime = 0L;

         if(s2s_accum_buf->supl.ST_active_flg == TRUE) /* QPE has a storm */
         {
            /* Use the QPE storm start time */

            storm_diff_backup->storm_begtime = s2s_accum_buf->supl.start_time_prcp;
         }

         if((legacy_ST_active == TRUE) && (legacy_storm_begtime > 0L))
         {
            /* PPS has a storm with a valid start time */

            if(storm_diff_backup->storm_begtime > 0L) /* QPE has a storm */
            {
               if(legacy_storm_begtime < storm_diff_backup->storm_begtime)
               {
                  /* PPS storm started earlier, so use it. */

                  storm_diff_backup->storm_begtime = legacy_storm_begtime;
               }
            }
            else /* QPE has no storm, so use PPS time */
            {
               storm_diff_backup->storm_begtime = legacy_storm_begtime;
            }
         }

         /* The storm start time is now set to the earliest of the PPS/QPE
          * storm starts. If the the difference product was reset after
          * this time, use it. This also happens when PPS/QPE have no storm,
          * as storm_begtime will be 0L. */

         if(storm_diff_backup->storm_begtime < start_time_storm_diff)
            storm_diff_backup->storm_begtime = start_time_storm_diff;

         /* For endtime, use the QPE end time, if it's later than the
          * storm_begtime. On one example, the PPS storm begin time was
          * 2010/10/07 08:49, but the QPE end time was 2010/10/07 08:48.
          * The two times were off by several seconds because QPE/PPS compute
          * their times a little differently in a volume, but it looked odd
          * when the times are truncated to the minute in the product. */

         if(s2s_accum_buf->supl.end_time > storm_diff_backup->storm_begtime)
            storm_diff_backup->storm_endtime = s2s_accum_buf->supl.end_time;
         else
            storm_diff_backup->storm_endtime = storm_diff_backup->storm_begtime;

         /* If the dual pol has a storm missing period, the legacy
          * should also have a storm missing period since they
          * are sharing the same data stream. */

         storm_diff_backup->missing_period |=
                                        s2s_accum_buf->supl.missing_period_flg;

         storm_diff_backup->num_bufs++;

      } /* end if diff_buf is good, not null */

   } /* end one of the systems has a storm active */
   else /* neither system has a storm active -> reset storm_diff_backup */
   {
      init_storm_diff(s2s_accum_buf, lt_accum_buf, storm_diff_backup);
   }

   if(storm_diff_backup->num_bufs <= 0) /* storm_diff_backup is empty */
   {
      /* Store the reason and set flags, etc. for a future null product */

      if(s2s_accum_buf->supl.null_accum)
         lt_accum_buf->supl.null_Storm_Total_diff = NULL_REASON_1;
      else
         lt_accum_buf->supl.null_Storm_Total_diff = NULL_REASON_4;

      /* We don't need to init Storm_Total_diff as it is not displayed
       * for null products. In cvg the screen will be blank.
       *
       * for(i = 0; i < MAX_AZM; i++)
       *   for(j = 0; j < MAX_BINS; j++)
       *       lt_accum_buf->Storm_Total_diff[i][j] = QPE_NODATA;
       *
       * 20101028 Ward - Old code used 0 as a volume time
       *                 for NULL products:
       *
       * storm_diff_begtime = 0L;
       * storm_diff_endtime = 0L;
       *
       * New code uses current QPE grid time for NULL products.
       *
       * The supl.end_time is the time of the 2nd rate grid that made
       * the scan-to-scan accumulation. The supl.begin_time is the
       * time of the 1st rate grid that made the scan-to-scan accumulation.
       * The time of a rate grid is an average time of all the elevations
       * that contributed to the hybrid scan. All these are different
       * from the current volume time, which is in the Scan Summary. */

      storm_diff_begtime = start_time_storm_diff;
      storm_diff_endtime = s2s_accum_buf->supl.end_time;

      missing_period = FALSE;
   }
   else /* make a non-null product */
   {
       /* 20111027 Ward Convert Storm_Total_diff buffer to 8-bin
        * resolution. CCR: NA11-00324 */

       convert_to_low_res(lt_accum_buf->Storm_Total_diff);

       lt_accum_buf->supl.null_Storm_Total_diff = FALSE;

       storm_diff_begtime = storm_diff_backup->storm_begtime;
       storm_diff_endtime = storm_diff_backup->storm_endtime;

       missing_period = storm_diff_backup->missing_period;
   }

   /* Fill in product metadata */

   lt_accum_buf->supl.stmdiff_begtime = storm_diff_begtime;
   lt_accum_buf->supl.stmdiff_endtime = storm_diff_endtime;

   lt_accum_buf->supl.missing_period_Storm_Total_diff = missing_period;

   return(FUNCTION_SUCCEEDED);

} /* end compute_storm_diff() ============================== */

/******************************************************************************
    Filename: dp_build_accum_diffs.c

    Description:
    ============
    convert_to_low_res() converts a high res DP (250 m * 1 degree) to
    a low res DP (2 Km X 1 degree). PPS is in low res, DP grids are in
    high res. This causes stair-stepping (difference values step up and down
    over 8 bins) in the DOD/DSD, so CCR NA11-00324 was written to reduce
    the DP to 8 bit. Conversion code was borrowed from the 4 bit
    convert_Resolution().

    Dan Berkowitz, in a 20111018 e-mail, says:

    "I don't know whether it is a good idea to ignore the "no data" bins when
     doing an average of the eight 0.25 km DP bins. The only alternative to this
     would be to make those bins have a value of zero, which may not be good,
     either. So, I would be in favor of accepting this method unless a better
     one can be found soon."

     Inputs: int grid[][MAX_BINS] - integer grid in high resolution
    Outputs: int grid[][MAX_BINS] - integer grid in low  resolution

    Returns: None.

    Change History
    ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------
    27 Oct 2011    0000       Ward               Initial version.
******************************************************************************/

void convert_to_low_res(int grid[][MAX_BINS])
{
   int   rad, bin, avgbin;
   float sum_8bin = 0.0;
   int   num_gooddata;
   int   cnt, cnt_avg;

   /* debug printing
    *
    *  int   j;
    *  fprintf(stderr, "------------------------------------------------\n");
    *  fprintf(stderr, "Before:\n");
    *  for(j = 0; j < MAX_BINS; j++)
    *  {
    *     fprintf(stderr, " [%3d] %6d", j, grid[316][j]);
    *     if(j % 8 == 7)
    *        fprintf(stderr, "\n");
    *  }
    */

   for(rad = 0; rad < MAX_AZM; rad++)
   {
      avgbin       = 0;
      sum_8bin     = 0.0;
      num_gooddata = 0;

      for(bin = avgbin; bin < MAX_BINS ; ++bin)
      {
         /* Only add to the sum if the data is valid.
          * Note: since these are difference products, negative
          *       values are allowed. */

         if(grid[rad][bin] != QPE_NODATA)
         {
            sum_8bin += (float) grid[rad][bin];
            num_gooddata++;
         }

         if((bin % 8) == 7) /* last bin in the set of 8 = (2 Km / 250 m) */
         {
            if(num_gooddata > 0)
               cnt_avg = (int) RPGC_NINT(sum_8bin / num_gooddata);
            else
               cnt_avg = QPE_NODATA;

            for(cnt = 0; cnt < 8; cnt++)
            {
               grid[rad][bin-cnt] = cnt_avg;
            }

            avgbin      += 8;   /* skip to next octet */
            sum_8bin     = 0.0;
            num_gooddata = 0;
            continue;
         }
      } /* end bin loop */
   } /* end rad loop */

   /* debug printing
    *
    *  fprintf(stderr, "After:\n");
    *  for(j = 0; j < MAX_BINS; j++)
    *  {
    *     fprintf(stderr, " [%3d] %6d", j, grid[316][j]);
    *     if(j % 8 == 7)
    *        fprintf(stderr, "\n");
    *  }
    */

} /* end convert_to_low_res() ============================== */
