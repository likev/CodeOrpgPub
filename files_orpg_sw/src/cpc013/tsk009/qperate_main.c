/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 20:08:42 $
 * $Id: qperate_main.c,v 1.13 2014/09/02 20:08:42 dberkowitz Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

#include "dp_lib_func_prototypes.h"
#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: qperate_main.c

    Description:
    ============
          This module contains the main function for the Dual-Pol QPE Rate
    algorithm.  The algorithm receives DP_MOMENTS_ELEV data as input and
    produce two intermediate linear buffers and a final product.
    First intermediate LB is Instantaneous Rainfall Rate (QPERATE) including
    (QPE Adaptation data, Rate Supplemental data, and the polar grid of Rainfall
    Rate (Rate(combined)).  Second intermediate LB is Hybrid Scan Hydrometeor-
    Class (QPEHHC).  A final product is Digital Inst. Precip Rate (DPR) product
    for each volume.

    The output is required for the Dual-Pol QPE Accumulation Algorithm.

    Input:   int   argc   - argument count (unused)
             char* argv[] - arguments      (unused)

    Output:  none

    Returns: none

    Globals: none

    Notes:   All input and output for this module are provided through
             ORPG API services calls.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    --------    -------    ----------         --------------------------
    01/10/07    0000       Cham Pham          Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).

    06/20/07    0001       Jihong Liu         Added one function to update
                                              the rain time in the supplemental
                                              fields, which is based on
                                              the result obtained from
                                              accumulation initiation function.

    08/29/11    0002       James Ward         Added global restart, similar
                                              to dp_lt_accum. CCR NA11-0284.
******************************************************************************/

/* According to Chris Calvert of the ROC, it is necessary for restart_qpe,
   the callback variable, global. */

int restart_qpe = FALSE;

int main(int argc, char* argv[])
{
   char* inbuf_name    = "DP_MOMENTS_ELEV";
   char* rate_out_name = "QPERATE";
   char* hhc_out_name  = "QPEHHC";

   Compact_dp_basedata_elev* inbuf = NULL; /* Pointer to input buffer of
                                            * base elevation data */

   Rate_Buf_t*   rate_out    = NULL; /* pointer to rate output buffer   */
   HHC_Buf_t*    hhc_out     = NULL; /* pointer to hhc  output buffer   */
   Regression_t* rates       = NULL; /* regression rates for comparison */

   int         ret           = 0;     /* function return code  */
   int         iostat        = 0;     /* Status from API calls */

   int         new_vol       = FALSE; /* TRUE -> this is a new volume   */
   int         get_next_elev = TRUE;  /* TRUE -> get the next elevation */
   int         got_inbuf     = FALSE; /* TRUE -> we got the inbuf       */

   int         vol_num       = QPE_NODATA; /* Current volume number         */
   int         vcp_num       = QPE_NODATA; /* Current VCP number            */
   int         elev_ind      = 0;          /* elevation index               */
   int         last_elev_ind = 0;          /* last elevation index          */
   int         max_ntilts    = 0;          /* max. tilts to end processing  */
   int         elev_angle_tenths = QPE_NODATA; /* elevation angle in tenths *
                                                * of a degree               */

   int         surv_bin_size   = 0;  /* Reflectivity data gate size METERS */
   int         spotblank       = 0;  /* Spot Blank flag                    */
   int         start_elev_date = 0;  /* start of surv elevation date       */
   int         start_elev_time = 0;  /* start of surv elevation time       */
   int         end_elev_date   = 0;  /* end of surv elevation date         */
   int         end_elev_time   = 0;  /* end of surv elevation time         */
   int         last_elev_flag  = 0;  /* last elevation flag, set by RPG    */

   time_t      first_elev_time = 0L; /* first elevation time               */
   time_t      last_elev_time  = 0L; /* last  elevation time               */

   float*      RateZ_table     = NULL; /* table of R(Z) values    */

   int i, j;                 /* for loop counters for init buffers */
   Rate_Buf_t init_rate_buf; /* initialized rate buffer */
   HHC_Buf_t  init_HHC_buf;  /* initialized HHC buffer */

   unsigned int rate_size  = sizeof(Rate_Buf_t);
   unsigned int hhc_size   = sizeof(HHC_Buf_t);

   /* QPE Rate algorithm variables */

   int  num_bins_filled  = 0; /* Number bin to fill in hybrid Rate grid,
                               * AEL 3.1.2.4 */
   char msg[200];             /* stderr message                          */

   /* 06082008 Ward - Moved global variables to local. */

   hydromet_prep_t   hydprep_adapt; /* hydromet preparation  data */
   hydromet_acc_t    hydacc_adapt;  /* hydromet accumulation data */
   hydromet_adj_t    hydadj_adapt;  /* hydromet adjustment   data */
   dp_precip_adapt_t dp_adapt;      /* dual pol adaptation   data */
   A3136C3_t         bias_info;     /* bias info                  */
   Siteadp_adpt_t    sadpt;         /* site adaptation       data */
   float             exzone[MAX_NUM_ZONES][EXZONE_FIELDS]; /* exclusion zones */
   unsigned short    max_no_of_gates;    /* maximum gates that can be filled,
                                          * based upon the data presented     */

   int vol_year, vol_month, vol_day, vol_hr, vol_min;

   /* ===================================================================== */

   /* Initialize log_error services. */

   RPGC_init_log_services ( argc, argv );

   /* Register inputs and output */

   RPGC_reg_io ( argc, argv );

   /* Register for ITC input */

   ret = RPGC_itc_in(A3136C3, (char *) &bias_info, sizeof(A3136C3_t),
                     RPGC_get_id_from_name(inbuf_name));
   if(ret < 0)
   {
      sprintf(msg, "Cannot register for A3136C3 (bias) ITC input\n");

      RPGC_log_msg(GL_ERROR, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }

      RPGC_abort_task();
   }

   /* Register for adaptation data */

   ret = RPGC_reg_ade_callback (dp_precip_callback_fx,
                                &dp_adapt,
                                DP_PRECIP_ADAPT_DEA_NAME,
                                BEGIN_VOLUME );
   if(ret < 0)
   {
      sprintf(msg, "Cannot register %s adaptation data callback function\n",
                   "DP_PRECIP_ADAPT");

      RPGC_log_msg(GL_ERROR, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }

      RPGC_abort_task();
   }

   ret = RPGC_reg_ade_callback( hydromet_prep_callback_fx,
                                &hydprep_adapt,
                                HYDROMET_PREP_DEA_NAME,
                                BEGIN_VOLUME );
   if(ret < 0)
   {
      sprintf(msg, "Cannot register %s adaptation data callback function\n",
                   "HYDROMET_PREP");

      RPGC_log_msg(GL_ERROR, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }

      RPGC_abort_task();
   }

   ret = RPGC_reg_ade_callback( hydromet_acc_callback_fx,
                                &hydacc_adapt,
                                HYDROMET_ACC_DEA_NAME,
                                BEGIN_VOLUME );
   if(ret < 0)
   {
      sprintf(msg, "Cannot register %s adaptation data callback function\n",
                   "HYDROMET_ACC");

      RPGC_log_msg(GL_ERROR, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }

      RPGC_abort_task();
   }

   ret = RPGC_reg_ade_callback( hydromet_adj_callback_fx,
                                &hydadj_adapt,
                                HYDROMET_ADJ_DEA_NAME,
                                BEGIN_VOLUME );
   if(ret < 0)
   {
      sprintf(msg, "Cannot register %s adaptation data callback function\n",
                   "HYDROMET_ADJ");

      RPGC_log_msg(GL_ERROR, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }

      RPGC_abort_task();
   }

   /* Register for site adaptation data */

   ret = RPGC_reg_site_info(&sadpt);

   if(ret < 0)
   {
      sprintf(msg, "Cannot register %s adaptation data callback function\n",
                   "SITE_INFO");

      RPGC_log_msg(GL_ERROR, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }

      RPGC_abort_task();
   }

   /* 08/29/11 James Ward  Register restart callback.
    * ORPGEVT_RESTART_LT_ACCUM is set in ~/include/orpgevt.h */

   ret = RPGC_reg_for_external_event(ORPGEVT_RESTART_LT_ACCUM,
                                     restart_qpe_cb,
                                     ORPGEVT_RESTART_LT_ACCUM);
   if(ret < 0)
   {
      sprintf(msg,
              "ORPGEVT_RESTART_LT_ACCUM register failed, ret = %d",
              ret);

      RPGC_log_msg(GL_ERROR, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      exit(GL_EXIT_FAILURE);
   }

   /* Open the data store with the bias source */

   ret = RPGC_data_access_open(ORPGDAT_ENVIRON_DATA_MSG, LB_READ);

   if(ret < 0)
   {
      sprintf(msg, "%s, data_id %d, ret %d\n",
              "Failed to open ORPGDAT_ENVIRON_DATA_MSG for bias source",
               ORPGDAT_ENVIRON_DATA_MSG, ret);

      RPGC_log_msg(GL_ERROR, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif

      if(rpg_err_to_msg(ret, msg) == FUNCTION_SUCCEEDED)
      {
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }

      RPGC_abort_task();
   }
   else
   {
      sprintf(msg, "Opened ORPGDAT_ENVIRON_DATA_MSG for bias source\n");

      RPGC_log_msg(GL_INFO, msg);
      #ifdef QPERATE_DEBUG
         fprintf(stderr, msg);
      #endif
   }

   /* ORPG task initialization routine. Input parameters argc/argv are
    * not used in this algorithm */

   RPGC_task_init(VOLUME_BASED, argc, argv);

   /* Register a Termination Handler. Code cribbed from:
    * ~/src/cpc001/tsk024/hci_agent.c
    *
    * 20080213 Chris Calvert says not to use termination
    * handlers. Keeping the code here in case we ever get
    * permission to turn it back on.
    *
    * if(ORPGTASK_reg_term_handler(qperate_terminate) < 0)
    * {
    *    RPGC_log_msg(GL_ERROR, "Could not register for termination signals");
    *    exit(GL_EXIT_FAILURE);
    * } */

   sprintf(msg, "BEGIN QPE-RATE ALGORITHM, CPC013/TSK009\n");
   RPGC_log_msg(GL_INFO, msg);
   #ifdef QPERATE_DEBUG
     fprintf(stderr, msg);
   #endif

   /* Initialize our output buffers.
    * Initializing rate_out to all 0s also sets:
    *
    * rate_out->rate_supl.sum_area       to 0 (AEL 3.1.1)
    * rate_out->rate_supl.prcp_begin_flg to FALSE (0) */

   memset((void *) &init_rate_buf, 0, rate_size);
   memset((void *) &init_HHC_buf,  0, hhc_size);

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         init_rate_buf.RateComb[i][j] = QPE_NODATA; /* AEL 3.1.1 */
         init_HHC_buf.HybridHCA[i][j] = QPE_NODATA; /* AEL 3.1.1 */
      }
   }

   /* This volume-based task will execute forever. */

   while(TRUE)
   {
      /* Suspend until driving input available.
       * This should be a new volume */

      RPGC_wait_act(WAIT_DRIVING_INPUT);

      /* If we've still got an output buffer open, the last
       * volume must've terminated early. Process the buffers. */

      if((rate_out != NULL) || (hhc_out != NULL))
      {
         End_of_volume(&rate_out, &hhc_out,
                       first_elev_time, last_elev_time,
                       vol_num, vcp_num,
                       &sadpt, TRUE);
      }

      /* Get a new Rate output buffer */

      rate_out = (Rate_Buf_t*) RPGC_get_outbuf_by_name (rate_out_name,
                                                        rate_size,
                                                        &iostat);
      if((rate_out == NULL) || (iostat != RPGC_NORMAL))
      {
          sprintf(msg, "RPGC_get_outbuf_by_name %s failed %p, iostat %d\n",
                  rate_out_name,
                  (void*) rate_out,
                  iostat);

          RPGC_log_msg(GL_INFO, msg);
          #ifdef QPERATE_DEBUG
             fprintf(stderr, msg);
          #endif

         if(rate_out != NULL)
         {
            RPGC_rel_outbuf ((void*) rate_out, DESTROY);
            rate_out = NULL;
         }

         /* Also release hhc_out, if it is non-NULL */

         if(hhc_out != NULL)
         {
            RPGC_rel_outbuf ((void*) hhc_out, DESTROY);
            hhc_out = NULL;
         }

         RPGC_abort_because(iostat);
         continue;
      }

      /* Get a new Hybrid Hydrometeor Class output buffer */

      hhc_out = (HHC_Buf_t*) RPGC_get_outbuf_by_name (hhc_out_name,
                                                      hhc_size,
                                                      &iostat);
      if((hhc_out == NULL) || (iostat != RPGC_NORMAL))
      {
         sprintf(msg, "RPGC_get_outbuf_by_name %s failed %p, iostat %d\n",
                 hhc_out_name,
                 (void*) hhc_out,
                 iostat);

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

         if(hhc_out != NULL)
         {
            RPGC_rel_outbuf ((void*) hhc_out, DESTROY);
            hhc_out = NULL;
         }

         /* Also release rate_out, which is non-NULL */

         RPGC_rel_outbuf ((void*) rate_out, DESTROY);
         rate_out = NULL;

         RPGC_abort_because(iostat);
         continue;
      }

      /* Initialize new volume */

      new_vol           = TRUE;
      get_next_elev     = TRUE;       /* AEL 3.1.1 */
      elev_ind          = 0;
      elev_angle_tenths = QPE_NODATA;
      max_ntilts        = MAX_ELEVS; /* max elevations/volume over all VCPs */
      num_bins_filled   = 0;         /* AEL 3.1.1 */
      max_no_of_gates   = 0;
      first_elev_time   = 0L;
      last_elev_time    = 0L;

      memcpy((void *) rate_out, (void *) &init_rate_buf, rate_size);
      memcpy((void *) hhc_out,  (void *) &init_HHC_buf,  hhc_size);

      /* Get bias items from Global COMMON block A3136C3. This is the
       * only time this volume we collect the bias, so the same bias
       * will be applied to all accums/products from now on. */

      RPGC_itc_read(A3136C3, &iostat);

      /* Copy adaptation data, bias_info, and bias source to the output buffer,
       * and get the exclusion zones (not copied to output buffers). */

      get_Adapt(rate_out,
                &dp_adapt,
                &hydprep_adapt,
                &hydacc_adapt,
                &hydadj_adapt,
                &bias_info,
                exzone);

      /* See if we need to create a new RateZ_table. This needs to be checked
       * every volume in case the adaptable parameters change. */

      create_RateZ_table(&RateZ_table, rate_out);

      /* Start while loop that controls elevation processing.
       * We process 1 elevation at a time - AEL 3.1.2
       * The AEL calls get_next_elev - "GO TO next SAMPLE VOLUME" */

      while(get_next_elev == TRUE)
      {
         got_inbuf = FALSE; /* start off pessimistic */

         /* Read an elevation of Dual-pol Moments */

         inbuf = (Compact_dp_basedata_elev*)
                  RPGC_get_inbuf_by_name (inbuf_name, &iostat);

         if((inbuf == NULL) || (iostat != RPGC_NORMAL))
         {
            sprintf(msg, "RPGC_get_inbuf_by_name %s failed %p, iostat %d\n",
                    inbuf_name,
                    (void*) inbuf,
                    iostat);

            RPGC_log_msg(GL_INFO, msg);
            #ifdef QPERATE_DEBUG
               fprintf(stderr, msg);
            #endif

            if(inbuf != NULL)
            {
               RPGC_rel_inbuf ((void*) inbuf);
               inbuf = NULL;
            }

            RPGC_cleanup_and_abort(iostat);

            break; /* skip the remainder of elevation processing */
         } /* end didn't get input buffer */
         else
         {
            got_inbuf = TRUE;
         }

         /* Get some parameters from the input buffer.
          * The maximum tilt number for this VCP is used to halt
          * QPE Rate processing at the next to last tilt. */

         vol_num = RPGC_get_buffer_vol_num((void*) inbuf);
         vcp_num = RPGC_get_buffer_vcp_num((void*) inbuf);

         /* Replaced RPGC_get_buffer_elev_index(),
          * (deprecated) RPGCS_get_last_elev_index(),
          * and check of base data header last elevation flag
          * with RPGC_is_buffer_from_last_elev(). Old code:
          *
          * elev_ind       = RPGC_get_buffer_elev_index ((void*) inbuf);
          * max_ntilts     = RPGCS_get_last_elev_index(vcp_num) - 1;
          * last_elev_flag = inbuf->radial[0].bdh.last_ele_flag; */

         last_elev_flag = RPGC_is_buffer_from_last_elev((void*) inbuf,
                                                        &elev_ind,
                                                        &last_elev_ind);
         max_ntilts = last_elev_ind - 1;

         elev_angle_tenths = RPGCS_get_target_elev_ang(vcp_num, elev_ind);

         RPGCS_julian_to_date(inbuf->radial[0].bdh.begin_vol_date,
                              &vol_year, &vol_month, &vol_day);

         vol_hr  = inbuf->radial[0].bdh.begin_vol_time / 3600000;                      /* millisecs/hour */
         vol_min = (inbuf->radial[0].bdh.begin_vol_time - (3600000 * vol_hr)) / 60000; /* millisecs/min  */

         sprintf(msg, "---------- %4.4d/%2.2d/%2.2d %02d:%02d volume: %d elevation: %d ----------\n",
                      vol_year, vol_month, vol_day, vol_hr, vol_min, vol_num, elev_ind);
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

         /* Read the elevation header */

         read_HeaderData (inbuf, &start_elev_date, &start_elev_time,
			  &end_elev_date, &end_elev_time,
                          &surv_bin_size, &spotblank);

         /* If this is the 1st elevation in the volume, capture its time.
          * new_vol will be set to FALSE in build_RR_Polar_Grid */

         if((new_vol == TRUE) || (elev_ind == 1))
         {
            /* AEL 3.1.1 */

            first_elev_time = get_elevation_time(start_elev_date, start_elev_time);
         }

         /* Add the elevation to the Instantaneous Rainfall Rate grid */

         build_RR_Polar_Grid(inbuf,            &new_vol,
                             elev_ind,         elev_angle_tenths,
                             max_ntilts,       &num_bins_filled,
                             surv_bin_size,
                             rate_out,         hhc_out,
                             &get_next_elev,   exzone,
                             &max_no_of_gates,
                             rates,            RateZ_table);

         /* In case we terminate the volume early,
          * save some supplemental data.
          * Default Mode_filter_len: 9 */

         rate_out->qpe_adapt.dp_adapt.Mode_filter_len =
             inbuf->mode_filter_length;

         /* Compute % rate grid filled. AEL 3.2.2 D */

         rate_out->rate_supl.pct_hybrate_filled =
             (num_bins_filled / (float) MAX_AZM_BINS) * 100.0;
         rate_out->rate_supl.vol_sb             =
             spotblank;
         rate_out->rate_supl.highest_elang      =
             (float) elev_angle_tenths / 10.0;

         /* Save the last elevation time. AEL 3.1.1 */

         last_elev_time = get_elevation_time(end_elev_date, end_elev_time);

         /* We no longer need the input buffer,
          * so release it and reset pointer */

         if(inbuf != NULL)
         {
            RPGC_rel_inbuf ((void*) inbuf);
            inbuf = NULL;
         }

         /* Print out our data fill.
          * 20110908 Ward Removed 'pct of data fill' as no longer needed */

         /* if(max_no_of_gates > 0)
          * {
          *    if(max_no_of_gates > MAX_BINS)
          *    {
          *       -* Reduce for comparison with hybrid rate grid *-
          *
          *       max_no_of_gates = MAX_BINS;
          *    }
          *
          *    sprintf(msg, "%s %f %%, pct of data fill = %f %%\n",
          *            "pct_hybrate_filled:",
          *            (num_bins_filled / (float) MAX_AZM_BINS) * 100.0,
          *            (num_bins_filled / (360.0 * max_no_of_gates)) * 100.0);
          * }
          * else
          * {
          *   sprintf(msg, "%s %f %%, pct of data fill = 0.0 %%\n",
          *            "pct_hybrate_filled:",
          *            (num_bins_filled / (float) MAX_AZM_BINS) * 100.0);
          * } */

         sprintf(msg, "%s %f %%\n",
                 "pct_hybrate_filled:",
                 (num_bins_filled / (float) MAX_AZM_BINS) * 100.0);

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

      } /* end while(get_next_elev == TRUE) */

      /* If we got here, then build_RR_Polar_Grid()
       * set (get_next_elev == FALSE) or
       * the RPG aborted the volume early (got_inbuf == FALSE). */

      if(got_inbuf) /* the RPG didn't abort the volume early */
      {
         /* If we're in Clear Air mode, then AVSET should not trigger.
          * AVSET triggers when > 
          * (*rate_out)->qpe_adapt.dp_adapt.Min_early_term_ang
          * (5 degrees), and in Clear Air mode, the elevation angle 
          * never gets above 4.5 degrees.
          * We should terminate our volume scan at max_ntilts
          * (= next to last elevation) and last_elev_flag
          * should always be FALSE.
          *
          * If we're not in Clear Air mode and last_elev_flag == TRUE,
          * this must be an AVSET early termination. so we want to check the
          * elevation angle before forwarding the buffers. */

          End_of_volume(&rate_out, &hhc_out,
                        first_elev_time, last_elev_time,
                        vol_num, vcp_num, &sadpt,
                        last_elev_flag);
      }

      rate_out = NULL;
      hhc_out  = NULL;

      /* We probably quit before the end of the volume, <= the next to last
       * elevation, so abort the remaining elevations in the scan.
       * Don't write out a GL_INFO in this case. */

      RPGC_abort_remaining_volscan();

   } /* end while loop forever */

   /* Bye, bye! */

   #ifdef QPERATE_DEBUG
      fprintf (stderr, "\nQPERATE Program Terminated\n");
   #endif

   return 0;

} /* end main() ============================================ */

/******************************************************************************
    Filename: qperate_main.c

    Description:
    ============
       End_of_volume() does end of volume processing

    Inputs: Rate_Buf_t*     *rate_out        - rate output buffer
            HHC_Buf_t*      *hhc_out         - HHC output buffer
            time_t          first_elev_time  - first elevation time
            time_t          last_elev_time   - last  elevation time
            int             vol_num          - volume number
            int             vcp_num          - VCP number
            Siteadp_adpt_t* sadpt            - site adaptation data
            int             check_elev_angle - TRUE -> check elev angle

    Note: To initialize rate_out and hhc_out at the end we have the
          pass them in as double pointers. hhc_out is dependent on rate_out.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    --------    -------    ----------      ---------------
    20080807     0000      James Ward      Initial version
    20110830     0001      James Ward      Only compute rate times right
                                           before we build an output
                                           buffer.
    20140826     0002	   Nicholas Cooper Removed check_mean_filter_size
					   for .Mode_filter_len since min/max
					   values checked in DEA.
******************************************************************************/

void End_of_volume(Rate_Buf_t* *rate_out, HHC_Buf_t* *hhc_out,
                   time_t first_elev_time, time_t last_elev_time,
                   int vol_num, int vcp_num,
                   Siteadp_adpt_t* sadpt, int check_elev_angle)
{
   int    ret = 0;  /* function return code   */
   char   msg[200]; /* stderr message         */

   static unsigned int rate_size = sizeof(Rate_Buf_t);
   static unsigned int hhc_size  = sizeof(HHC_Buf_t);

   if(*rate_out == NULL)
   {
      /* Since hhc_out depends on rate_out, destroy it too */

      if(*hhc_out != NULL)
         RPGC_rel_outbuf((void*) *hhc_out, DESTROY);
   }
   else /* rate_out is not NULL */
   {
      /* Check to see if we scanned high enough.
       * Default Min_early_term_ang: 5.0 degrees */

      if((check_elev_angle == TRUE) &&
         ((*rate_out)->rate_supl.highest_elang <
          (*rate_out)->qpe_adapt.dp_adapt.Min_early_term_ang))
      {
         sprintf(msg, "%s %f < Min_early_term_ang %d, %s\n",
                      "rate_out->rate_supl.highest_elang",
                       (*rate_out)->rate_supl.highest_elang,
                       (*rate_out)->qpe_adapt.dp_adapt.Min_early_term_ang,
                       "destroying output buffers");

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

         /* Destroy the output buffers */

         RPGC_rel_outbuf((void*) *rate_out, DESTROY);

         if(*hhc_out != NULL)
            RPGC_rel_outbuf((void*) *hhc_out, DESTROY);

       } /* end we didn't scan high enough to FORWARD the buffers */
       else /* we scanned high enough */
       {
          /* Run PAIF. Filling last_rain_time is AEL 3.1.3 */

          precip_accum_init(first_elev_time, last_elev_time, *rate_out);

          /* Build the DPR product in generic radial product format.
           * The output buffer will be built inside buildDPR_prod(). */

          ret = buildDPR_prod(vol_num, vcp_num, *rate_out, sadpt);

          if(ret == FUNCTION_FAILED)
          {
             RPGC_log_msg(GL_INFO, "Did not generate a DPR (176) product!");

             /* Destroy the output buffers */

             RPGC_rel_outbuf((void*) *rate_out, DESTROY);

             if(*hhc_out != NULL)
                RPGC_rel_outbuf((void*) *hhc_out, DESTROY);
          }
          else /* we built a DPR product */
          {
             /* Copy info from the Rate buffer to the HHC buffer,
              * then release the HHC buffer. */

             if(*hhc_out != NULL)
             {
                /* Default Mode_filter_len: 9 */

                (*hhc_out)->time               =
                   (*rate_out)->rate_supl.time;
                (*hhc_out)->pct_hybrate_filled =
                   check_hybr_rate_filled(
                        (*rate_out)->rate_supl.pct_hybrate_filled);
                (*hhc_out)->highest_elang      =
                   check_highest_elev_used(
                        (*rate_out)->rate_supl.highest_elang);
                (*hhc_out)->mode_filter_length =
                        (*rate_out)->qpe_adapt.dp_adapt.Mode_filter_len; 

                RPGC_rel_outbuf((void*) *hhc_out,
                                 FORWARD | EXTENDED_ARGS_MASK,
                                 hhc_size);

              } /* end hhc_out not NULL */

              /* Release the rate buffer */

              RPGC_rel_outbuf((void*) *rate_out,
                               FORWARD | EXTENDED_ARGS_MASK,
                               rate_size);

          } /* end we built a DPR product */

       } /* end we scanned high enough */

   } /* end rate_out not NULL */

   /* We've forwarded/destroyed the buffers, so initialize the pointers. */

   *rate_out = NULL;
   *hhc_out  = NULL;

} /* end End_of_volume() ==================================== */

/*****************************************************************************
   Filename: qperate_main.c

   Description
   ===========
   restart_qpe_cb() is the Dual Pol QPE callback function

   Input: int fx_parm - the event that caused the call.

   Output: none

   As per Chris Calvert guidance, the callback sets a flag and exits.
   To test, try something like this in an unrelated task:

   if(vsnum % 3 == 0)
   {
      EN_post(ORPGEVT_RESTART_LT_ACCUM, "Hello", strlen("Hello") + 1, 0);
      RPGC_log_msg(GL_INFO, "ORPGEVT_RESTART_LT_ACCUM event posted");
   }

   To generate the event in the hci, select:

       RPG Control
       Unlock (click on lock in urh corner)
       Reset-Hydromet, click Dual-Pol QPE
       Activate

   Change History
   ==============
    DATE           VERSION    PROGRAMMERS        NOTES
    -----------    -------    ---------------    ----------------------
    29 Aug 2011    0001       James Ward         Initial implementation
                                                 CCR NA11-0284.
******************************************************************************/

void restart_qpe_cb(int fx_parm)
{
   restart_qpe = TRUE;

} /* end restart_qpe_cb() =================================== */
