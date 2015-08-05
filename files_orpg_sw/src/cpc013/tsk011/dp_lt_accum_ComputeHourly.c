/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/11/15 16:28:47 $
 * $Id: dp_lt_accum_ComputeHourly.c,v 1.6 2010/11/15 16:28:47 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include "dp_lt_accum_func_prototypes.h"

/*****************************************************************************
   Filename: dp_lt_accum_ComputeHourly.c

   Description
   ===========
   compute_hourly() computes the grids for the two hourly products:

       OHA (169) One-Hour Accumulation      (can be biased)
       DAA (170) Digital Accumulation Array (always unbiased)

    Like compute_diff(), if the scan-to-scan accumulation grid is unavailable,
    we will give the user the best available data by writing out the
    latest version of the product.

    Note: A null product means that the product could have been
    generated, but was not, due to a condition. It does not indicate
    some sort of system failure. When we make a null product, we don't
    initialize the grid because it is not displayed.

   Inputs: S2S_Accum_Buf_t*  s2s_accum_buf - latest scan to scan accum
           LT_Accum_Buf_t*   lt_accum_buf  - our saved data
           Circular_Queue_t* hourly_circq  - circq that tracks hourly

   Outputs: lt_accum_buf.One_Hr_biased    - with latest s2s accum added
            lt_accum_buf.One_Hr_unbiased  - with latest s2s accum added

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Change History
   ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------
    29 Oct 2007     0000       Ward              Initial implementation
    04 Mar 2009     0001       Ward              Changed prcp_detected_flg to
                                                 ST_active_flg.
    21 Oct 2010     0002       Ward              Null products now use current
                                                 time.
******************************************************************************/

int compute_hourly(S2S_Accum_Buf_t*  s2s_accum_buf,
                   LT_Accum_Buf_t*   lt_accum_buf,
                   Circular_Queue_t* hourly_circq)
{
  int    max_hourly     = 0;     /* maximum hourly value in inches     */
  time_t hourly_begtime = 0L;    /* hourly beginning time              */
  time_t hourly_endtime = 0L;    /* hourly ending time                 */
  short  missing_period = FALSE; /* TRUE -> hourly is missing a period */
  char   msg[200];               /* stderr msg                         */

  if(DP_LT_ACCUM_DEBUG)
     fprintf(stderr, "Beginning compute_hourly()\n");

  /* Check for NULL pointers. */

  if(pointer_is_NULL(s2s_accum_buf, "compute_hourly", "s2s_accum_buf"))
     return(NULL_POINTER);

  if(pointer_is_NULL(lt_accum_buf, "compute_hourly", "lt_accum_buf"))
     return(NULL_POINTER);

  if(pointer_is_NULL(hourly_circq, "compute_hourly", "hourly_circq"))
     return(NULL_POINTER);

  /* Default max_hourly_acc: 800 mm
   *
   * max_hourly is in thousandths of inches, the same as the
   * One_Hr_biased/One_Hr_unbiased.
   */

  max_hourly = s2s_accum_buf->qpe_adapt.accum_adapt.max_hourly_acc *
               MM_TO_IN * 1000;

  /* 20090304 In the old code, we only added a new accum if precip has been
   * detected in this volume. This was different from the legacy which always
   * added precip, however small, as long as a storm was active. After the IRR,
   * we changed to match the legacy behavior, so both the hourly and the storm
   * should add any amount of precip, even if it's below the detection
   * threshold, as long as a storm is active. Old code:
   *
   * if((s2s_accum_buf->supl.null_accum == FALSE) &&
   *    (s2s_accum_buf->supl.prcp_detected_flg == TRUE)) */

   if(s2s_accum_buf->supl.ST_active_flg == TRUE)
   {
      /* A storm is active. If we got an accum buffer, update our totals.
       * If we didn't get an accum buffer, do nothing, and the
       * hourly total will be unchanged. */

      if(s2s_accum_buf->supl.null_accum == FALSE)
      {
         /* Add the new accum buf to the queue. If the queue is empty,
          * this will also initialize One_Hr_biased/One_Hr_unbiased
          * so the accum_grid can be added to these running totals. */

         if(CQ_Add_to_back(hourly_circq,
                           s2s_accum_buf,
                           lt_accum_buf->One_Hr_biased,
                           lt_accum_buf->One_Hr_unbiased) != CQ_SUCCESS)
         {
            sprintf(msg, "%s %s\n",
                         "compute_hourly:",
                         "CQ_Add_to_back() did not return CQ_SUCCESS!!!");

            RPGC_log_msg(GL_INFO, msg);
            if(DP_LT_ACCUM_DEBUG)
               fprintf(stderr, msg);
         }

         /* Update our running total grids. One_Hr_biased CAN be biased,
          * but is does not forced to be; the bias could be turned
          * off by the user. */

         if(s2s_accum_buf->qpe_adapt.adj_adapt.bias_flag == TRUE)
         {
            add_biased_short_to_int(lt_accum_buf->One_Hr_biased,
                                    s2s_accum_buf->accum_grid,
                                    s2s_accum_buf->qpe_adapt.bias_info.bias,
                                    max_hourly);

            /* Because Dual Pol is not applying bias, This should not happen,
             * so print an error */

            if(DP_LT_ACCUM_DEBUG)
            {
               fprintf(stderr, "%s %s %s\n",
                       "compute_hourly:",
                       "s2s_accum_buf->qpe_adapt.adj_adapt.bias_flag",
                       " == TRUE!!!");
            }
         }
         else /* the bias is off */
         {
            add_unbiased_short_to_int(lt_accum_buf->One_Hr_biased,
                                      s2s_accum_buf->accum_grid,
                                      max_hourly);
         }

         add_unbiased_short_to_int(lt_accum_buf->One_Hr_unbiased,
                                   s2s_accum_buf->accum_grid,
                                   max_hourly);

      } /* end if accum buf not NULL */

   } /* end if ST_active */

   /* No storm is active. We could init the hourly product here,
    * but what if the ST_active flag is set to a < 1 hour trigger?
    * In either case, the hourly product will be trimmed to an hour
    * by the next CQ_Trim_To_Hour(). Old code:
    *
    * else
    * {
    *   init_hourly(lt_accum_buf, hourly_circq,
    *               s2s_accum_buf->qpe_adapt.dp_adapt.Max_vols_per_hour);
    * } */

   /* Delete every accum buf in the queue older than an hour.
    * This will also subtract them from One_Hr_biased/One_Hr_unbiased
    * along the way. We want to do this even if we don't get a new
    * accum grid or have precip so our queue stays current to the hour. */

   if(CQ_Trim_To_Hour(hourly_circq,
                      lt_accum_buf->One_Hr_biased,
                      lt_accum_buf->One_Hr_unbiased,
                      max_hourly,
                      s2s_accum_buf->supl.end_time) != CQ_SUCCESS)
   {
      sprintf(msg, "%s %s\n",
                   "compute_hourly:",
                   "CQ_Trim_To_Hour() did not return CQ_SUCCESS!!!");

      RPGC_log_msg(GL_INFO, msg);
      if(DP_LT_ACCUM_DEBUG)
         fprintf(stderr, msg);
   }

   /* If the queue is empty, store the reason and flag for later creation of
    * null products. */

   if(hourly_circq->first == CQ_EMPTY)
   {
      if(s2s_accum_buf->supl.null_accum)
      {
         lt_accum_buf->supl.null_One_Hr_biased   = NULL_REASON_1;
         lt_accum_buf->supl.null_One_Hr_unbiased = NULL_REASON_1;
      }
      else
      {
         lt_accum_buf->supl.null_One_Hr_biased   = NULL_REASON_5;
         lt_accum_buf->supl.null_One_Hr_unbiased = NULL_REASON_5;
      }

      /* We don't need to init One_Hr_biased/One_Hr_unbiased
       * as they are not displayed for null products.
       *
       * 20101028 Ward - Old code, which used 0 as a volume time
       *                 for NULL products:
       *
       * hourly_begtime = 0L;
       * hourly_endtime = 0L;
       *
       * New code uses current QPE grid time for NULL products.
       *
       * The supl.end_time is the time of the 2nd rate grid that made
       * the scan-to-scan accumulation. The supl.begin_time is the
       * time of the 1st rate grid that made the scan-to-scan accumulation.
       * The time of a rate grid is an average time of all the elevations
       * that contributed to the hybrid scan. All these are different
       * from the current volume time, which is in the Scan Summary. */

      hourly_begtime = s2s_accum_buf->supl.end_time;
      hourly_endtime = s2s_accum_buf->supl.end_time;

      missing_period = FALSE;
   }
   else /* queue is not empty, flag to make non-null products later */
   {
      lt_accum_buf->supl.null_One_Hr_biased   = FALSE;
      lt_accum_buf->supl.null_One_Hr_unbiased = FALSE;

      hourly_begtime = hourly_circq->begin_time[hourly_circq->first];
      hourly_endtime = hourly_circq->end_time[hourly_circq->last];

      missing_period = CQ_Get_Missing_Period(hourly_circq);
   }

   /* Fill in product metadata */

   lt_accum_buf->supl.hrly_begtime = hourly_begtime;
   lt_accum_buf->supl.hrly_endtime = hourly_endtime;

   lt_accum_buf->supl.missing_period_One_Hr_biased   = missing_period;
   lt_accum_buf->supl.missing_period_One_Hr_unbiased = missing_period;

   return(FUNCTION_SUCCEEDED);

} /* end compute_hourly() =================================== */
