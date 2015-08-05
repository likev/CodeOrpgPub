/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:07 $
 * $Id: gauge_radar_storm.c,v 1.3 2011/04/13 22:53:07 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <dp_lib_func_prototypes.h>
#include <gsl/gsl_statistics_double.h>
#include "gauge_radar_common.h"
#include "gauge_radar_consts.h"
#include "gauge_radar_proto.h"
#include "gauge_radar_types.h"

extern int   pps_storm[MAX_AZM][MAX_2KM_RESOLUTION];
extern int   Storm_Total[MAX_AZM][MAX_BINS];

extern float old_dp_storm[MAX_GAUGES];
extern float old_pps_storm[MAX_GAUGES];

extern short zero_scan;
extern short zero_hourly;
extern short no_hourly;
extern short reset_pps_storm;

/******************************************************************************
   Function name: generate_product_for_a_storm()

   Description:
   ============
      generate_product_for_a_storm() determines whether to generate product or
      not based on whether a storm is over or not.

   Inputs:
      long term accumulation buffer.

   Outputs:
      Product 180, if a product was to be generated.

   Return:
      TRUE - product generated;
     FALSE - no product was generated
	
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   24 Feb 2008    0000       Zhan Zhang         Initial implementation
******************************************************************************/	

extern time_t storm_print_time;
extern int    storm_print_volume;

void generate_product_for_a_storm(LT_Accum_Buf_t*       lt_accum_buf,
                                  hyadjscn_large_buf_t* legacy,
                                  User_array_t*         request,
                                  prod_dep_para_t*      prod_dep,
                                  int                   vol_num,
                                  int                   total_vols)
{
    int   i, j;
    short legacy_s;

    short make_storm_product = FALSE;
    short reset_dp_storm     = FALSE;

    static short make_features = FALSE;

    static time_t storm_start = 0L;
    static time_t storm_end   = 0L;

    static short was_stormy            = FALSE;
    static short printed_at_storm_time = FALSE;

    dua_accum_buf_metadata_t metadata; /* holds metadata part of a message */

    static unsigned int metadata_size = sizeof(dua_accum_buf_metadata_t);

    /* Initialize */

    memset(&metadata, 0, metadata_size);

    /* See if a storm is active */

    if(lt_accum_buf->supl.ST_active_flg == FALSE) /* a storm is not active */
    {
       if(was_stormy)
       {
           make_storm_product = TRUE;
           reset_dp_storm     = TRUE;

           RPGC_log_msg (GL_INFO,
                         " Storm: end of a storm, product generated\n");
       }
       else /* a storm was not active */
       {
          make_storm_product = FALSE;

          RPGC_log_msg (GL_INFO,
                        " Storm: no storm active, no product\n");
       }

       was_stormy = FALSE; /* for next call */

    } /* end a storm is not active */
    else /* a storm is active */
    {
       memcpy(Storm_Total,
              lt_accum_buf->Storm_Total,
              MAX_AZM_BINS * sizeof(int));

       storm_end = lt_accum_buf->supl.stmtot_endtime;

       if(was_stormy)
       {
           if((storm_print_time > 0)           &&
              (printed_at_storm_time == FALSE) &&
              (storm_print_time < storm_end))
           {
               /* printed_at_storm_time is a 1 time flag. We only want
                * a storm product the first time the storm_end clicks past
                * the storm_print_time. This should only happen once. */

               printed_at_storm_time = TRUE;
               make_storm_product    = TRUE;

               RPGC_log_msg (GL_INFO,
                             " Storm: %s %ld < %s %ld, %s",
                             "storm_print_time",
                             storm_print_time,
                             "lt_accum_buf->supl.stmtot_endtime",
                             lt_accum_buf->supl.stmtot_endtime,
                             "make product");
           }
           else if(storm_print_volume > 0)
           {
              if(vol_num % storm_print_volume == 0)
              {
                 make_storm_product = TRUE;

                 RPGC_log_msg (GL_INFO,
                               "%s %d, make product",
                               " Storm: middle of a storm, vol divides",
                               storm_print_volume);
              }
              else if(vol_num % storm_print_volume == 1)
              {
                 /* Uncomment make_features here to make a features
                  * product the volume after the storm print volume.
                  *
                  * If we are making a features product, we want to do
                  * it periodically, so it doesn't scroll off the end of
                  * the cvg buffer. */

                 make_features = TRUE;

                 RPGC_log_msg (GL_INFO,
                               "%s, make product",
                               " Storm: making features product");
              }
              else /* no special reason to make product */
              {
                 make_storm_product = FALSE;

                 RPGC_log_msg (GL_INFO,
                               " Storm: middle of a storm, no product\n");
              }
           }
           else /* no special reason to make product */
           {
              make_storm_product = FALSE;

              RPGC_log_msg (GL_INFO,
                            " Storm: middle of a storm, no product\n");
           }
       }
       else /* a storm was not active */
       {
           storm_start        = lt_accum_buf->supl.stmtot_begtime;
           make_storm_product = FALSE;

           RPGC_log_msg (GL_INFO, " Storm: beginning of a storm, no product\n");

           /* Init PPS storm total, so it tracks with DP */

           reset_pps_storm = TRUE;
       }

       was_stormy = TRUE;  /* for next call */

    } /* end a storm is active */

    if(reset_pps_storm == TRUE)
    {
       /* PPS says a storm is no longer active, so reset the legacy
        * storm total */

       memset(pps_storm,     0, MAX_AZM * MAX_2KM_RESOLUTION * sizeof(int));
       memset(old_pps_storm, 0, MAX_GAUGES * sizeof(float));
    }
    else if ((zero_scan == TRUE) &&
             ((zero_hourly == TRUE) || (no_hourly == TRUE)))
    {
       /* We don't have a legacy scan-to-scan, so do nothing.
        * Maybe this is a 1 volume hiccup? */
    }
    else /* we have a legacy scan-to-scan */
    {
       /* Add the legacy scan-to-scan into the legacy storm-total. */

       for(i = 0; i < MAX_AZM; i++)
       {
         for(j = 0; j < MAX_2KM_RESOLUTION; j++)
         {
            legacy_s = legacy->medium.AccumScan[i][j];

            if((legacy_s != INIT_VALUE) && (legacy_s > 0))
            {
               pps_storm[i][j] += legacy_s;
            }
         }
       }
    }

    if(make_storm_product)
    {
       if(storm_start == storm_end) /* no storm range */
       {
          RPGC_log_msg (GL_INFO,
                        ">> start_time %ld == end_time %ld, %s",
                        storm_start,
                        storm_end,
                        "no product");

          RPGC_abort_request(request, PGM_PROD_NOT_GENERATED);
       }
       else /* we have a storm range */
       {
          /* Write to the output product */

          metadata.start_time = storm_start;
          metadata.end_time   = storm_end;

          write_to_output_product(STORM, &metadata, request,
                                  prod_dep, lt_accum_buf,
                                  Storm_Total,
                                  pps_storm,
                                  vol_num, total_vols);
       }
    } /* end if make storm product */
    else if(make_features)
    {
       RPGC_log_msg (GL_INFO, "Making features product\n");

       write_to_output_product(FEATURES, &metadata, request,
                               prod_dep, lt_accum_buf,
                               Storm_Total,
                               pps_storm,
                               vol_num, total_vols);

       make_features = FALSE;
    }
    else
    {
       RPGC_abort_request(request, PGM_PROD_NOT_GENERATED);
    }

    if(reset_dp_storm)
    {
       memset(Storm_Total,  0, MAX_AZM_BINS * sizeof(int));
       memset(old_dp_storm, 0, MAX_GAUGES   * sizeof(float));
    }

} /* end generate_product_for_a_storm() ================================== */
