/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/29 22:30:00 $
 * $Id: dp_s2s_accum_comp_accum.c,v 1.5 2014/07/29 22:30:00 dberkowitz Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#include "dp_s2s_accum_func_prototypes.h"
#include "dp_Consts.h"

/******************************************************************************
    Filename: dp_s2s_accum_comp_accum.c

    Description:
    ============
    dp_compute_accum() will compute scan-to-scan accumulations from two hybrid
    rate grids.  If the the times are further apart than the threshold
    "Maximum Time for Interpolation" (default 30 mins), then the average
    rate of both rate grids is used to compute the accumulation over the
    maximum time, and the missing period flag is set to TRUE.

    If the the times are further apart than the threshold "Threshold Elapsed
    Time to Restart" (default 60 mins), then this function will return the
    value NO_S2S_ACCUM indicating that no accumulation was computed.

    If the function completes successfully, it will return GOOD_S2S_ACCUM.

    This function assumes that the calling function has already determined
    that an accumulation is necessary and valid. This function DOES NOT CHECK
    flag values to determine whether it should compute an accumulation.

    Inputs:
       Rate_Buf_t *rate1 - rate buffer with the first hybrid rate grid.
                           The date/time of this grid will be in the
                           accum_grid.supl.time.

       Rate_Buf_t *rate2 - rate buffer with the second hybrid rate grid.
                           The date/time of this grid will be in the
                           accum_grid.supl.time.
    Outputs:
       S2S_Accum_Buf_t *accum_buf - accum buffer with a grid of accumulation
                                    values.

    Returns: NO_S2S_ACCUM (0), GOOD_S2S_ACCUM (1)

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ------------------
     4 Oct 2007     0000     Stein, Ward        Initial implementation
    11 Apr 2008     0001     Ward               Added check for % hybrid rate
                                                filled.
    18 Nov 2008     0002     Ward               Added logging of flag
                                                transitions
     6 Mar 2014     0003     Berkowitz, Murnan  Add variables for missing period
                                                time and Top of Hour
                                                (CCRs NA12-00223 and NA12-00264)
******************************************************************************/

/* In dp_compute_accum(), rates are in inches/hour, and the accum_grid is
 * in thousandths of inches, so the time_span should be divided by 3600 to
 * convert seconds to hours, and the accum_grid should be multiplied by 1000
 * to convert inches to thousandths of inches. 1000/3600 = 10/36 = 5/18.
 */

#define ACCUM_CONVERT (5.0 / 18.0)

int dp_compute_accum ( Rate_Buf_t *rate1, Rate_Buf_t *rate2,
                       S2S_Accum_Buf_t *accum_buf )
{
   int   max_accum = 0;  /* maximum precip allowed in an accum grid (in) */
   long  time_span = 0L; /* time between rate grids   */
   int   i, j;           /* loop counters             */
   char  msg[200];       /* stderr message            */
   float temp_val = 0.0; /* to guard against overflow */

   /* 02/19/2014 - Initialize variables used in Top of Hour (TOH) calculations. */

   accum_buf->supl.missing_period_time = 0L;
   accum_buf->supl.missing_period_flg = FALSE;
  


   /* Check for NULL pointers */

   if(pointer_is_NULL(rate1, "dp_compute_accum", "rate1"))
      return(NO_S2S_ACCUM);

   if(pointer_is_NULL(rate2, "dp_compute_accum", "rate2"))
      return(NO_S2S_ACCUM);

   if(pointer_is_NULL(accum_buf, "dp_compute_accum", "accum_buf"))
      return(NO_S2S_ACCUM);

   /* Compute the time span between the 2 rate grids */

   time_span = rate2->rate_supl.time - rate1->rate_supl.time;

   /* If the rate grids are too far apart, we can't compute an accum.
    *
    * Default restart_time: 60 mins
    */

   if ((time_span >=
       (rate1->qpe_adapt.accum_adapt.restart_time * SECS_PER_MIN)) ||
       (time_span <= 0))
   {
      accum_buf->supl.null_accum              = NULL_REASON_1;
      accum_buf->supl.missing_period_time     = time_span;
      accum_buf->supl.missing_period_flg      = TRUE;
     
      accum_buf->dua_query.begin_time         = accum_buf->supl.begin_time;
      accum_buf->dua_query.end_time           = accum_buf->supl.end_time;
      accum_buf->dua_query.prcp_detected_flg  = accum_buf->supl.prcp_detected_flg;
      accum_buf->dua_query.ST_active_flg      = accum_buf->supl.ST_active_flg;
      accum_buf->dua_query.missing_period_flg = accum_buf->supl.missing_period_flg;


      if(DP_S2S_ACCUM_DEBUG)
      {
          sprintf(msg, "%s %ld %s %d %s %s\n",
              "dp_compute_accum: accum period (sec) = missing period time =", time_span, ">=",
              rate1->qpe_adapt.accum_adapt.restart_time * SECS_PER_MIN,
              "or <= 0,", "set missing period flag.");
          fprintf(stderr, msg);
      }

      return(NO_S2S_ACCUM); /* this will make a null product */
   }

   /* Default max_interp_time: 30 mins */

   if((time_span >
       (rate1->qpe_adapt.accum_adapt.max_interp_time * SECS_PER_MIN)) &&
      (time_span <
       (rate1->qpe_adapt.accum_adapt.restart_time    * SECS_PER_MIN)))
   {

      /* 03/06/2014 - missing_period_time = the total time of the S2S accumulation
       * period that generated a missing_period_flag minus the max interpolation
       * time. (this is a TOH addition)
       */

      accum_buf->supl.missing_period_time = time_span - 
                                    rate1->qpe_adapt.accum_adapt.max_interp_time;


      /* time_span > max interpolation time, so we have to compute a rate over
       * the max interpolation time and set the missing period flag to TRUE.
       * This is equivalent to using the average rate for the
       * entire max interpolation time. Reset the time span and
       * continue with the accum calculation.
       */

      time_span = rate1->qpe_adapt.accum_adapt.max_interp_time;

      accum_buf->supl.missing_period_flg = TRUE;

      sprintf(msg, "%s %ld %s %d %s %d %s %ld\n",
          "dp_compute_accum: accum period (sec) =", time_span, ">",
          rate1->qpe_adapt.accum_adapt.max_interp_time * SECS_PER_MIN,
          "and <",
          rate1->qpe_adapt.accum_adapt.restart_time    * SECS_PER_MIN,
          ", set missing period flag and missing period time = ",
          accum_buf->supl.missing_period_time);

       RPGC_log_msg(GL_INFO, msg);
       if(DP_S2S_ACCUM_DEBUG)
          fprintf(stderr, msg);
   }
   else
   {
      /* time_span <= max interpolation time, so no extrapolation is required.
       * Just perform a straight accumulation computation.
       *
       * 02/19/2014 - missing_period_time = the total time of the S2S accumulation
       * period that generated a missing_period_flag minus the max interpolation
       * time. (this is a TOH addition)
       */

      accum_buf->supl.missing_period_time = 0L;
      accum_buf->supl.missing_period_flg = FALSE;

      if(DP_S2S_ACCUM_DEBUG)
      {
          sprintf(msg, "%s %ld %s\n",
              "dp_compute_accum: accum period (sec) =", time_span, 
              "missing period flag not set and missing period time =0.");
          fprintf(stderr, msg);
      }
   }

   /* Default max_period_acc: 400 mm. max_accum is in 1000th inches */

   max_accum = accum_buf->qpe_adapt.accum_adapt.max_period_acc *
               (MM_TO_IN * 1000);

   for (i = 0; i < MAX_AZM; i++)
   {
     for (j = 0; j < MAX_BINS; j++)
     {
        /* Note: This QPE_NODATA logic applies to rate grids,
         * not when operating on accum grids. */

        if ((rate1->RateComb[i][j] == QPE_NODATA) ||
            (rate2->RateComb[i][j] == QPE_NODATA))
        {
           accum_buf->accum_grid[i][j] = QPE_NODATA;
           continue;
        }
        else /* both rates were good */
        {
           /* RateComb[][] is an array of floats. temp_val will be in
            * thousandths of an inch. */

           temp_val = ((rate1->RateComb[i][j] + rate2->RateComb[i][j]) / 2.0)
                       * (time_span * ACCUM_CONVERT);
        }

        /* For testing  - adjust i, j to fprintf the bin you want
         *
         * int vsnum;
         *
         * vsnum = RPGC_get_buffer_vol_num( (void*) rate2 );
         *
         * if((i == 356) && (j== 150))
         * {
         *   fprintf(stderr, "%d [%d][%d] rate1 %f, rate2 %f, time_span %ld,
         *           temp_val %f\n",
         *           vsnum, i, j,
         *           rate1->RateComb[i][j],
         *           rate2->RateComb[i][j],
         *           time_span,
         *           temp_val);
         * }
         */

        /* Note: A test against SHRT_MIN is not done because ACCUM_CONVERT
         * is positive, and RateComb/time_span have been tested to be
         * non-negative. */

        if(temp_val <= max_accum) /* a good value */
        {
           accum_buf->accum_grid[i][j] = (short) RPGC_NINT(temp_val);
           if(accum_buf->accum_grid[i][j] == QPE_NODATA) /* accident. landed */
              accum_buf->accum_grid[i][j] += 1;          /* move up 1 */
        }
        else if(temp_val > SHRT_MAX)
        {
           accum_buf->accum_grid[i][j] = SHRT_MAX; /* cap it */

           if(DP_S2S_ACCUM_DEBUG)
           {
              sprintf(msg, "%s %s%d%s%d%s%d%s\n",
              "dp_compute_accum:", "accum_grid[", i, "][", j,
              "] capped at SHRT_MAX (", SHRT_MAX, ")");
 
              fprintf(stderr,msg);
           }
        }
        else /* SHRT_MAX >= temp_val > max_accum */
        {
           accum_buf->accum_grid[i][j] = max_accum; /* cap it */

           if(DP_S2S_ACCUM_DEBUG)
           {
              sprintf(msg, "%s %s%d%s%d%s%d %s\n",
              "dp_compute_accum:", "accum_grid[", i, "][", j,
              "] capped at max_accum (", max_accum, "mm)");
 
              fprintf(stderr,msg);
           }
        }

     } /* end loop over all bins */

   } /* end loop over all radials */

   /* Set the avg and begin/end dates/times inside accum_buf. */

   accum_buf->supl.begin_time = rate1->rate_supl.time;
   accum_buf->supl.end_time   = rate2->rate_supl.time;

   /* Copy values to the DUA query header. These 5 fields are
    * ones in the database that may be SQL queried on.
    */

   accum_buf->dua_query.begin_time         = accum_buf->supl.begin_time;
   accum_buf->dua_query.end_time           = accum_buf->supl.end_time;
   accum_buf->dua_query.prcp_detected_flg  = accum_buf->supl.prcp_detected_flg;
   accum_buf->dua_query.ST_active_flg      = accum_buf->supl.ST_active_flg;
   accum_buf->dua_query.missing_period_flg = accum_buf->supl.missing_period_flg;

   return (GOOD_S2S_ACCUM);

} /* end dp_compute_accum() ========================================*/
