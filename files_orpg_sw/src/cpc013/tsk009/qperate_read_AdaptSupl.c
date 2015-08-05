/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/29 22:30:00 $
 * $Id: qperate_read_AdaptSupl.c,v 1.14 2014/07/29 22:30:00 dberkowitz Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "qperate_types.h"
#include "qperate_func_prototypes.h"
#include <dpprep_isdp.h>

#define DP_DEA_FILE "alg.dp_precip."

/******************************************************************************
    Filename: qperate_read_AdaptSupl.c

    Description:
    ============
       get_Adapt() copies Exclusion zone and QPE Rate adaptation parameters
    to output buffer.

    Inputs:
       dp_precip_adapt_t* dp_adapt      - Dual Pol            adaptation params
       hydromet_prep_t*   hydprep_adapt - hydromet prep       adaptation params
       hydromet_acc_t*    hydacc_adapt  - hydromet accum      adaptation params
       hydromet_adj_t*    hydadj_adapt  - hydromet adjustment adaptation params
       A3136C3_t*         bias_info     - bias information

    Outputs:
       Rate_Buf_t* rate_out                - rate output buffer filled with
                                             adaptable parameters
       float       exzone[][EXZONE_FIELDS] - exclusion zones

    Change History
    ==============
    DATE          VERSION    PROGRAMMER      NOTES
    ----          -------    ----------      -----
    12/12/06      0000       Cham Pham       Initial implementation for
                                             dual-polarization project
                                             (ORPG Build 11).
    08/10/06      0001       Jihong Liu      Changes based QPE V2
    08/05/08      0002       James Ward      Stripped out supplemental init
                                             and renamed it get_Adapt
    04/07/09      0003       James Ward      Added bias_info
    07/22/09      0004       Zhan Zhang      Comment out the code snippet that
                                             swap the begin/end azimuths
                                             if end azimuth < begin azimuth.
    03/10/10      0005       James Ward      Deleted RhoHV_min_Kdp_rate;
                                             replaced with dpprep.alg
                                             corr_thresh.
    08/30/10      0006       James Ward      Added use_pbb1
    10/31/11      0007       James Ward      For CCR NA11-00372:
                                             PBB method 1 not to be used.
                                             Replaced RhoHV_min_rate
                                             with dpprep.alg art_corr
    20121028      0008       D.Berkowitz     For CCR NA12-00361 and 362
                                             Deleted useMLDAHeights,
                                             replaced with Melting_Layer_Source
    06/20/13      0009       Brian Klein     For CCR TBD.  ISDP estimate in TAB
    20140306      0010       Murnan          Added min_time_period for Top of
                                             Hour accumulation
                                             (CCR NA12-00264)
******************************************************************************/

void get_Adapt(Rate_Buf_t*        rate_out,
               dp_precip_adapt_t* dp_adapt,
               hydromet_prep_t*   hydprep_adapt,
               hydromet_acc_t*    hydacc_adapt,
               hydromet_adj_t*    hydadj_adapt,
               A3136C3_t*         bias_info,
               float              exzone[MAX_NUM_ZONES][EXZONE_FIELDS])
{
   int    i               = 0;
   double value           = 0.0;
   char*  svalue          = NULL;
   float  tmp             = 0.0;
   int    got_bias_source = FALSE;
   char*  buffer          = NULL; /* address of buffer    */
   int    bytes_read      = 0;    /* number of bytes read */
   char   msg[200];               /* stderr message       */

   Bias_table_msg* bias_table_msg = NULL;

   static unsigned int adapt_size      = sizeof(dp_precip_adapt_t);
   static unsigned int hydro_size      = sizeof(hydromet_adj_t);
   static unsigned int bias_size       = sizeof(A3136C3_t);
   static Dpp_isdp_est_t Isdp_est;
   /* static unsigned int bias_msg_size = sizeof(Bias_table_msg); */

   #ifdef QPERATE_DEBUG
      fprintf(stderr, "Beginning get_Adapt() .........\n" );
   #endif

   /* Melting layer adaptation parameters */

   if(RPGC_ade_get_values(ML_DEA_FILE, "ml_depth", &value) != 0)
   {
      RPGC_log_msg(GL_INFO,
                   ">> RPGC_ade_get_values() failed - ml_depth");

      /* Default default_ml_depth: 0.5 Km */
   }
   else
   {
      rate_out->qpe_adapt.mlda.default_ml_depth = (float) value;
   }

   /* Note: Even though the hci user sees 'range = {RPG_0C_Hgt, Radar_Based,
    * Model_Enhanced}', it is defined as an enumeration, 'enum = 0, 1, 2',
    * so either a 0, 1, or 2 will be returned as 'value'. */

   if(RPGC_ade_get_values(ML_DEA_FILE, "Melting_Layer_Source", &value) != 0)
   {
      RPGC_log_msg(GL_INFO,
                   ">> RPGC_ade_get_value(s) failed - Melting_Layer_Source");
   }
   else
   {

    /* Melting_Layer_Source is either -
     * 0 = MLDA_UseType_Default (RPG_0C_Hgt),
     * 1 = MLDA_UseType_Radar (Radar_Based),
     * 2 = MLDA_UseType_Model (Model_Enhanced),
     * where value of 2 is the normal default.
     */

      rate_out->qpe_adapt.mlda.Melting_Layer_Source = (int) value;
   }

   /* Dpprep adaptation parameters */

   if(RPGC_ade_get_values(DPPREP_DEA_FILE, "corr_thresh", &value) != 0)
   {
      RPGC_log_msg(GL_INFO,
        ">> RPGC_ade_get_values() failed - dpprep corr_thresh");
   }
   else /* default corr_thresh: 0.9 */
   {
      rate_out->qpe_adapt.dpprep_adapt.corr_thresh = (float) value;
   }

   if(RPGC_ade_get_values(DPPREP_DEA_FILE, "art_corr", &value) != 0)
   {
      RPGC_log_msg(GL_INFO,
        ">> RPGC_ade_get_values() failed - dpprep art_corr");
   }
   else /* default art_corr: 0.8 */
   {
      rate_out->qpe_adapt.dpprep_adapt.art_corr = (float) value;
   }

   if(RPGC_data_access_read( DP_ISDP_EST, &Isdp_est.isdp_est, 
                             sizeof(Dpp_isdp_est_t), DP_ISDP_EST_MSGID) <= 0)
   {
      RPGC_log_msg(GL_INFO,
        ">> RPGC_data_access_read( DP_ISDP_EST) Failed - dpprep isdp_est");
   }
   else
   {
      rate_out->qpe_adapt.dpprep_adapt.isdp_est = (float) Isdp_est.isdp_est;
      rate_out->qpe_adapt.dpprep_adapt.isdp_yy = (float) Isdp_est.isdp_yy;
      rate_out->qpe_adapt.dpprep_adapt.isdp_mm = (float) Isdp_est.isdp_mm;
      rate_out->qpe_adapt.dpprep_adapt.isdp_dd = (float) Isdp_est.isdp_dd;
      rate_out->qpe_adapt.dpprep_adapt.isdp_hr = (float) Isdp_est.isdp_hr;
      rate_out->qpe_adapt.dpprep_adapt.isdp_min = (float) Isdp_est.isdp_min;
   }

   if(RPGC_ade_get_string_values(DPPREP_DEA_FILE, "isdp_apply", &svalue) == 0 &&
                                  strcasecmp (svalue, "yes") == 0)
   {
      rate_out->qpe_adapt.dpprep_adapt.isdp_apply = 1;
   }
   else
   {
      rate_out->qpe_adapt.dpprep_adapt.isdp_apply = 0;
   }

   /* Dual Pol adaptation parameters */

   memcpy(&(rate_out->qpe_adapt.dp_adapt), dp_adapt, adapt_size);

   /* PPS legacy Accumulation prep parameters
    *
    * Default rain_time_thresh: 60 mins */

   rate_out->qpe_adapt.prep_adapt.rain_time_thresh =
       hydprep_adapt->rain_time_thresh;

   /* PPS legacy Accumulation adaptation parameters
    *
    * Default    restart_time:  60 mins
    * Default max_interp_time:  30 mins
    * Default min_time_period:  54 mins
    * Default  max_hourly_acc: 800 mm
    * Default  max_period_acc: 400 mm */

   rate_out->qpe_adapt.accum_adapt.restart_time    =
       hydacc_adapt->restart_time;
   rate_out->qpe_adapt.accum_adapt.max_interp_time =
       hydacc_adapt->max_interp_time;
   rate_out->qpe_adapt.accum_adapt.min_time_period =
       hydacc_adapt->min_time_period;
   rate_out->qpe_adapt.accum_adapt.max_hourly_acc  =
       hydacc_adapt->max_hourly_acc;
   rate_out->qpe_adapt.accum_adapt.max_period_acc  =
       hydacc_adapt->max_period_acc;

   /* PPS legacy Adjustment adaptation parameters */

   memcpy(&(rate_out->qpe_adapt.adj_adapt),
          hydadj_adapt,
          hydro_size);

   /* Bias info */

   memcpy(&(rate_out->qpe_adapt.bias_info),
          bias_info,
          bias_size);

   /* Force the bias to be applied flag to FALSE because
    * bias for dual-pol is not yet implemented. */

   rate_out->qpe_adapt.adj_adapt.bias_flag = FALSE;

   /* Get the bias source. The source code borrowed from
    * ~/src/cpc102/tsk076/edit_bias.c The bias source is read from the
    * ORPGDAT_ENVIRON_DATA_MSG (a .dat file). This .dat file is also
    * read by prcpadju (~/src/cpc013/tsk006/a3136n.ftn) to fill
    * in the A3136C3 ITC block that the RPGC_itc_read() uses to get
    * the remainder of the bias data. So, the RPGC_itc_read() call could
    * be eliminated by rolling the prcpadju functionality into a
    * function call (picking the first mean field bias that matches the
    * effective number of gauge-radar pairs over the threshold
    * number of gauge-radar pairs). However, Dan Stein estimates that bias
    * won't be implemented until Build 15, and may change, so it's best to
    * leave the code alone for now.
    *
    * For endian reasons, the bias source can be byte swapped.
    * See SOURCE_ID in ~/src/cpc014/tsk006/a31461.ftn.
    *
    * If we are not applying bias, set a default source. */

   memset(&(rate_out->qpe_adapt.bias_source), 0, 5);

   if(rate_out->qpe_adapt.adj_adapt.bias_flag == TRUE)
   {
      bytes_read = RPGC_data_access_read(ORPGDAT_ENVIRON_DATA_MSG,
                                         &buffer,
                                         LB_ALLOC_BUF,
                                         ORPGDAT_BIAS_TABLE_MSG_ID);

      if(bytes_read != 242 /* ?!= bias_msg_size = 282 */)
      {
         sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_read %d\n",
                 "get_Adapt:",
                 "Failed to bias source from data store",
                  ORPGDAT_ENVIRON_DATA_MSG,
                  ORPGDAT_BIAS_TABLE_MSG_ID,
                  bytes_read);

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

         if(bytes_read < 0) /* RPG error */
         {
            if(rpg_err_to_msg(bytes_read, msg) == FUNCTION_SUCCEEDED)
            {
               RPGC_log_msg(GL_INFO, msg);
               #ifdef QPERATE_DEBUG
                  fprintf(stderr, msg);
               #endif
            }
         }

         if(buffer != NULL)
            free(buffer);
      }
      else if(buffer == NULL) /* no buffer to read */
      {
         sprintf(msg, "%s %s, data_id %d, rec_id %d, bytes_read %d\n",
                 "get_Adapt:",
                 "bias source buffer returned is NULL",
                  ORPGDAT_ENVIRON_DATA_MSG,
                  ORPGDAT_BIAS_TABLE_MSG_ID,
                  bytes_read);

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif
      }
      else /* successful read */
      {
         /* The bias source is 3 characters with a NULL terminator. */

         bias_table_msg = (Bias_table_msg *) buffer;

         /* ? For some reason a comparison vs. NULL generates a warning */

         if(bias_table_msg->awips_id[2] == 0)
         {
            /* bias source is byte swapped = "BA0C" */

            rate_out->qpe_adapt.bias_source[0] = ' ';
            rate_out->qpe_adapt.bias_source[1] = bias_table_msg->awips_id[1];
            rate_out->qpe_adapt.bias_source[2] = bias_table_msg->awips_id[0];
            rate_out->qpe_adapt.bias_source[3] = bias_table_msg->awips_id[3];
         }
         else /* bias source is not byte swapped = "ABC0" */
         {
            rate_out->qpe_adapt.bias_source[0] = ' ';
            rate_out->qpe_adapt.bias_source[1] = bias_table_msg->awips_id[0];
            rate_out->qpe_adapt.bias_source[2] = bias_table_msg->awips_id[1];
            rate_out->qpe_adapt.bias_source[3] = bias_table_msg->awips_id[2];
         }

         if(buffer != NULL)
            free(buffer);

         sprintf(msg, "%s %s, data_id %d\n",
                 "get_Adapt:",
                 "Read bias source from data store",
                  ORPGDAT_ENVIRON_DATA_MSG);

         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

         got_bias_source = TRUE;
      }
   }

   if(got_bias_source == FALSE) /* write in a default source */
      strncpy(rate_out->qpe_adapt.bias_source, " XXX", 4);

   /* Exclusion Zones.
    *
    * Since the angles we are comparing have a 1 degree resolution,
    * round the Beg_azm down the nearest 1 degree, and
    * round the End_azm up to the nearest 1 degree.
    *
    * Since the bins we are comparing to have a (minimum) 250m resolution,
    * round the Beg_rng down the nearest 250m, and
    * round the End_rng up to the nearest 250m.
    *
    * Bins will only be excluded below the Elev_ang. */

   for(i=0; i<dp_adapt->Num_zones; i++)
   {
      exzone[i][EXCL_AZM_ANGLE_START] = floorf(dp_adapt->Beg_azm[i]);
      exzone[i][EXCL_AZM_ANGLE_END]   = ceilf(dp_adapt->End_azm[i]);

      /* If set, must exclude at least 1 degree */

      if(exzone[i][EXCL_AZM_ANGLE_START] == exzone[i][EXCL_AZM_ANGLE_END])
         exzone[i][EXCL_AZM_ANGLE_END] += 1.0;

      exzone[i][EXCL_SLANT_RANGE_START] = 250.0 * floorf(
                                          (dp_adapt->Beg_rng[i] * NM_TO_M)
                                          / 250.0);

      exzone[i][EXCL_SLANT_RANGE_END]   = 250.0 * ceilf(
                                          (dp_adapt->End_rng[i] * NM_TO_M)
                                          / 250.0);

      /* If set, must exclude at least 1 bin */

      if(exzone[i][EXCL_SLANT_RANGE_START] == exzone[i][EXCL_SLANT_RANGE_END])
          exzone[i][EXCL_SLANT_RANGE_END] += 250.0;

      exzone[i][EXCL_ELEV_ANGLE] = dp_adapt->Elev_ang[i];
    }

    /* RPGC_log_msg(GL_INFO,
     *             "%s elev=%f, azm1=%f, azm2=%f, rng1=%f, rng2=%f,  %f\n",
     *             "this is in read adapt part!",
     *             exzone[0][EXCL_ELEV_ANGLE],exzone[0][EXCL_AZM_ANGLE_START],
     *             exzone[0][EXCL_AZM_ANGLE_END],exzone[0][EXCL_SLANT_RANGE_START],
     *             exzone[0][EXCL_SLANT_RANGE_END]);
     */

    /* Validate the exclusion zone definitions:
     * 1. (assert) beginning range   < ending range
     * 2.          beginning azimuth < ending azimuth.  */

   for(i = 0; i<dp_adapt->Num_zones; i++)
   {
      /* Swap the begin/end ranges if end range < begin range. */

      if(exzone[i][EXCL_SLANT_RANGE_END] < exzone[i][EXCL_SLANT_RANGE_START])
      {
         tmp                               = exzone[i][EXCL_SLANT_RANGE_END];
         exzone[i][EXCL_SLANT_RANGE_END]   = exzone[i][EXCL_SLANT_RANGE_START];
         exzone[i][EXCL_SLANT_RANGE_START] = tmp;
      }

      /* Swap the begin/end azimuths if end azimuth < begin azimuth.
       *
       * Note: It is assumed there are no zones that cross north.
       *       The following code is commented out.*/

      /* if(exzone[i][EXCL_AZM_ANGLE_END] < exzone[i][EXCL_AZM_ANGLE_START])
       * {
       *    tmp                             = exzone[i][EXCL_AZM_ANGLE_END];
       *    exzone[i][EXCL_AZM_ANGLE_END]   = exzone[i][EXCL_AZM_ANGLE_START];
       *    exzone[i][EXCL_AZM_ANGLE_START] = tmp;
       * }
       */

   } /* end loop over all zones */

} /* end get_Adapt() ==================================== */
