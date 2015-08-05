/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/10 20:42:19 $
 * $Id: dp_lt_accum_ComputeST.c,v 1.7 2012/01/10 20:42:19 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#include <a313hparm.h> /* FLG_RESTP */
#include "dp_lt_accum_func_prototypes.h"

/******************************************************************************
   Filename: dp_lt_accum_ComputeST.c

   Description
   ===========
    compute_storm() adds the current scan-to-scan accumulation grid to the
    storm-total intermediateg product which is used to generate both the
    DSA and the STA:

        DSA (172) 8-bit Digital Storm Total Accumulation (can be biased)
        STA (171) 4-bit Storm Total Accumulation (can be biased)

    Inputs: S2S_Accum_Buf_t* s2s_accum_buf - latest scan to scan accum
            LT_Accum_Buf_t*  lt_accum_buf  - our running total
            Storm_Backup_t*  storm_backup  - storm backup

    Outputs: lt_accum_buf.Storm_Total      - with latest s2s accum added

    Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

    Note: The dual pol ST_active flag will become F if no rain has been
          detected in rain_time_thresh (60) min. This is set in
          qperate build_RR_Polar_Grid().

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         ----------------------------------
    10/2007     0000       Cham Pham          Initial implementation
    03/2008     0001       James Ward         Fixed spot blank handling, changed
                                              storm start to start_time_precip
    10/2010     0002       Ward               Null products now use current
                                              time.
******************************************************************************/

int compute_storm(S2S_Accum_Buf_t*  s2s_accum_buf,
                  LT_Accum_Buf_t*   lt_accum_buf,
                  Storm_Backup_t*   storm_backup)
{
   time_t storm_begtime  = 0L;    /* storm beginning time              */
   time_t storm_endtime  = 0L;    /* storm ending time                 */
   short  missing_period = FALSE; /* TRUE -> storm is missing a period */

   if(DP_LT_ACCUM_DEBUG)
      fprintf(stderr, "Beginning compute_storm()\n");

   /* Check for NULL pointers. */

   if(pointer_is_NULL(s2s_accum_buf, "compute_storm", "s2s_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(lt_accum_buf, "compute_storm", "lt_accum_buf"))
      return(NULL_POINTER);

   if(pointer_is_NULL(storm_backup, "compute_storm", "storm_backup"))
      return(NULL_POINTER);

   if(s2s_accum_buf->supl.ST_active_flg == TRUE)
   {
     /* A storm is active. If we got an accum buffer, update our totals.
      * If we didn't get an accum buffer, do nothing, and the
      * storm total will be unchanged. */

     if(s2s_accum_buf->supl.null_accum == FALSE)
     {
        /* StormTotal CAN be biased, but is not forced to be;
         * the bias could be turned off by the user. */

        if(s2s_accum_buf->qpe_adapt.adj_adapt.bias_flag == TRUE)
        {
            /* Add the scan-to-scan accumulation grid to the storm-total
             * intermediate product. */

            add_biased_short_to_int(lt_accum_buf->Storm_Total,
                                    s2s_accum_buf->accum_grid,
                                    s2s_accum_buf->qpe_adapt.bias_info.bias,
                                    INT_MAX);

            /* Add the scan-to-scan accumulation grid to the storm-total
             * backup. */

            add_biased_short_to_int(storm_backup->StormTotal,
                                    s2s_accum_buf->accum_grid,
                                    s2s_accum_buf->qpe_adapt.bias_info.bias,
                                    INT_MAX);

           /* Because Dual Pol is not applying bias,
            * this should not happen, so print an error */

           if(DP_LT_ACCUM_DEBUG)
           {
              fprintf(stderr, "%s %s%s\n",
                              "compute_storm:",
                              "s2s_accum_buf->qpe_adapt.adj_adapt.bias_flag",
                              " == TRUE!!!");
           }
        }  /* If bias_flag == TRUE */
        else /* no bias */
        {
           add_unbiased_short_to_int(lt_accum_buf->Storm_Total,
                                     s2s_accum_buf->accum_grid,
                                     INT_MAX);

           add_unbiased_short_to_int(storm_backup->StormTotal,
                                     s2s_accum_buf->accum_grid,
                                     INT_MAX);
        }

        /* The start_time_prcp, set in qperate, is always the
         * storm start. */

        storm_backup->storm_begtime = s2s_accum_buf->supl.start_time_prcp;
        storm_backup->storm_endtime = s2s_accum_buf->supl.end_time;

        storm_backup->missing_period |= s2s_accum_buf->supl.missing_period_flg;

        storm_backup->num_bufs++;

     }  /* If null_accum == FALSE */

   }  /* If ST_active_flg (if the storm-total is active) */
   else /* no storm active -> reset storm_backup */
   {
      init_storm(lt_accum_buf, storm_backup);
   }

   if(storm_backup->num_bufs <= 0) /* storm_backup is empty */
   {
      /* Make a null product */

      if(s2s_accum_buf->supl.null_accum)
         lt_accum_buf->supl.null_Storm_Total = NULL_REASON_1;
      else
         lt_accum_buf->supl.null_Storm_Total = NULL_REASON_4;

      /* We don't need to init Storm_Total
       * as it is not displayed for null products.
       *
       * 20101028 Ward - Old code, which used 0 as a volume time
       *                 for NULL products:
       *
       * storm_begtime = 0L;
       * storm_endtime = 0L;
       *
       * New code uses current QPE grid time for NULL products.
       *
       * The supl.end_time is the time of the 2nd rate grid that made
       * the scan-to-scan accumulation. The supl.begin_time is the
       * time of the 1st rate grid that made the scan-to-scan accumulation.
       * The time of a rate grid is an average time of all the elevations
       * that contributed to the hybrid scan. All these are different
       * from the current volume time, which is in the Scan Summary. */

      storm_begtime = s2s_accum_buf->supl.end_time;
      storm_endtime = s2s_accum_buf->supl.end_time;

      missing_period = FALSE;
   }
   else /* make a non-null product */
   {
       lt_accum_buf->supl.null_Storm_Total = FALSE;

       storm_begtime = storm_backup->storm_begtime;
       storm_endtime = storm_backup->storm_endtime;

       missing_period = storm_backup->missing_period;
   }

   /* Fill in product metadata */

   lt_accum_buf->supl.stmtot_begtime = storm_begtime;
   lt_accum_buf->supl.stmtot_endtime = storm_endtime;

   lt_accum_buf->supl.missing_period_Storm_Total = missing_period;

   return(FUNCTION_SUCCEEDED);

} /* end compute_storm() =================================== */
