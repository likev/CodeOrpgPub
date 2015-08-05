/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/03/08 17:21:27 $
 * $Id: qperate_excluded_Zone.c,v 1.7 2011/03/08 17:21:27 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: qperate_excluded_Zone.c

    Description:
    ============
       is_Excluded() checks dual-pol data which should not be used from user
    selected bounds.

    Inputs:
       float       azm_angle               - azimuth angle in degrees
       float       slant_rng_m             - slant range in meters
       float       elev_angle_deg          - elevation angle in degrees
       Rate_Buf_t* rate_out                - rate output buffer
       float       exzone[][EXZONE_FIELDS] - exclusion zones

    Returns:
       The excluded zone flag. TRUE  - the bin is within an exclusion zone
                               FALSE - "     "    outside     "          "

    Change History
    ==============
    DATE        VERSION    PROGRAMMER        NOTES
    ----        -------    ----------        -----
    12/07/06    0000       Cham Pham         Initial implementation for
                                             dual-polarization project
                                             (ORPG Build 11).

    06/25/09    0001       Zhan Zhang        Add validation for range inputs
                                             Add special handling for 0 crossing
                                             exclusion zones (CCR NA09-00174)
******************************************************************************/

int is_Excluded(float azm_angle, float slant_rng_m, float elev_angle_deg,
                Rate_Buf_t* rate_out,
                float exzone[MAX_NUM_ZONES][EXZONE_FIELDS])
{
   int i;
   float tmp;
   static int Not_Validated = TRUE;

   /*
   RPGC_log_msg(GL_INFO, "elev=%f, azm1=%f, azm2=%f, rng1=%f, rng2=%f\n",
                       exzone[0][EXCL_ELEV_ANGLE],exzone[0][EXCL_AZM_ANGLE_START],
                       exzone[0][EXCL_AZM_ANGLE_END],exzone[0][EXCL_SLANT_RANGE_START],
                       exzone[0][EXCL_SLANT_RANGE_END]);
   */

   /* Validate the exclusion zone definitions for range*/

   if(Not_Validated)
   {
      for(i = 0; i < rate_out->qpe_adapt.dp_adapt.Num_zones; i++)
      {
         /* Swap the begin/end ranges if end range < begin range. */

         if(exzone[i][EXCL_SLANT_RANGE_END] < exzone[i][EXCL_SLANT_RANGE_START])
         {
            tmp                               = exzone[i][EXCL_SLANT_RANGE_END];
            exzone[i][EXCL_SLANT_RANGE_END]   = exzone[i][EXCL_SLANT_RANGE_START];
            exzone[i][EXCL_SLANT_RANGE_START] = tmp;
         }

      } /* end loop over all zones */

      Not_Validated = FALSE;
   }

   /* This logic mimics the legacy PPS. AEL 3.1.2.1 */

   for(i = 0; i < rate_out->qpe_adapt.dp_adapt.Num_zones; i++)
   {
      if ( exzone[i][EXCL_AZM_ANGLE_START] < exzone[i][EXCL_AZM_ANGLE_END] )
      {
      /* EXCL_AZM_ANGLE_END and EXCL_SLANT_RANGE_END comparisons
       * are < instead of <= to avoid overlapping exclusion zones.
       *
       * Note: 4 bit products are 8 bin averages, so an exclusion
       * zone may appear on an 8 bit product, but will be averaged
       * away on a 4 bit product. */

         /* handling normally defined exclusion zones */
         /* note that we use AND in azimuth term */
         if(elev_angle_deg <= exzone[i][EXCL_ELEV_ANGLE]        &&
            azm_angle      >= exzone[i][EXCL_AZM_ANGLE_START]   &&
            azm_angle      <= exzone[i][EXCL_AZM_ANGLE_END]     &&
            slant_rng_m    >= exzone[i][EXCL_SLANT_RANGE_START] &&
            slant_rng_m    <= exzone[i][EXCL_SLANT_RANGE_END])
         {
          /*  RPGC_log_msg(GL_INFO, "inside No. %d Normal exclusion zone \n", i);
            RPGC_log_msg(GL_INFO, "elev=%f, azm1=%f, azm2=%f, rng1=%f, rng2=%f this is new added\n",
                       exzone[i][EXCL_ELEV_ANGLE],exzone[i][EXCL_AZM_ANGLE_START],
                       exzone[i][EXCL_AZM_ANGLE_END],exzone[i][EXCL_SLANT_RANGE_START],
                       exzone[i][EXCL_SLANT_RANGE_END]);
           */
            return (TRUE);
         }
      }
      else
      {
         /* special handling for 0-degree-crossing  exclusion zone
            (CCR NA09-00174) */
         /* note that we use OR in azimuth term */
         if(elev_angle_deg <= exzone[i][EXCL_ELEV_ANGLE]
              &&
            (azm_angle      >= exzone[i][EXCL_AZM_ANGLE_START] ||
             azm_angle      <= exzone[i][EXCL_AZM_ANGLE_END] )
              &&
            slant_rng_m    >= exzone[i][EXCL_SLANT_RANGE_START]
              &&
            slant_rng_m    <= exzone[i][EXCL_SLANT_RANGE_END])
         {
            /* RPGC_log_msg(GL_INFO, "inside No. %d 0 degree crossing exclusion zone \n", i);
            RPGC_log_msg(GL_INFO, "elev=%f, azm1=%f, azm2=%f, rng1=%f, rng2=%f\n",
                       exzone[i][EXCL_ELEV_ANGLE],exzone[i][EXCL_AZM_ANGLE_START],
                       exzone[i][EXCL_AZM_ANGLE_END],exzone[i][EXCL_SLANT_RANGE_START],
                       exzone[i][EXCL_SLANT_RANGE_END]);
             */
            return (TRUE);
         }
      }
   } /* end loop over all zones */

   return(FALSE);

} /* end is_Excluded() ===================================== */
