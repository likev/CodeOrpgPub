/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 19:58:45 $
 * $Id: dp_precip_adapt.h,v 1.5 2014/09/02 19:58:45 dberkowitz Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef DP_PRECIP_ADAPT_H
#define DP_PRECIP_ADAPT_H

/******************************************************************************
    Filename: dp_precip_adapt.h

    Description:
    ============
    Declare a structure to hold adaptation data for the DualPol Precip
    algorithm.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER   NOTES
    ----------  -------    ----------   ------------------------
    01/22/2007    0000     Cham Pham    Initial implementation for
                                        dual-polarization project
                                        (ORPG Build 11).
    08/07/2007    0001     Jihong Liu   Changes based on QPE version 2
    01/24/2008    0002     James Ward   Merged them all together
    08/21/2008    0003     James Ward   Adopted consistent names and
                                        commented units.
    10/31/2011    0004     James Ward   For CCR NA11-00372:
                                        Removed  Use_pbb1
                                        Removed  Z_max_beam_blk (pbb1)
                                        Added    Min_blockage
                                        Added    Kdp_min_beam_blk
                                        Replaced RhoHV_min_rate
                                        with     art_corr
                                        Removed RhoHV_min_Kdp_rate which
                                        previously had been replaced with
                                        corr_thresh.
    08/12/2014    0005     Berkowitz,   For CCR NA13-00260 and 00261;
                           Murnan       Added Hr_HighZThresh
                                        Added Ds_BelowMLTop_mult
******************************************************************************/

#include "dp_Consts.h" /* MAX_NUM_ZONES */

#define DP_PRECIP_ADAPT_DEA_NAME "alg.dp_precip."

typedef struct {
short Mode_filter_len;         /* Mode N Filter Size               unitless  */
float Kdp_mult;                /* Kdp Multiplier coeff             unitless  */
float Kdp_power;               /* Kdp Power coeff                  unitless  */
float Z_mult;                  /* Z-R Multiplier coeff             unitless  */
float Z_power;                 /* Z-R Power coeff                  unitless  */
float Zdr_z_mult;              /* Zdr/Z Multiplier coeff for Z     unitless  */
float Zdr_z_power;             /* Zdr/Z Power coeff for Z          unitless  */
float Zdr_zdr_power;           /* Zdr/Z Power coeff for Zdr        unitless  */
float Gr_mult;                 /* Graupel multiplier coeff         unitless  */
float Rh_mult;                 /* Rain/Hail multiplier coeff       unitless  */
float Ds_mult;                 /* Dry snow multiplier coeff        unitless  */
float Ds_BelowMLTop_mult;      /* Dry snow multiplier coeff        unitless  *
                                * below melting layer top                    */
float Ic_mult;                 /* Crystal Multiplier coeff         unitless  */
float Ws_mult;                 /* Wet snow multiplier coeff        unitless  */
float Grid_is_full;            /* Percent of rate grid needed      %         *
                                * to be full to stop filling                 */
int   Min_blockage;            /* Min Beam Blockage to augment Z   %         */
int   Kdp_min_beam_blk;        /* Min Beam Blockage to Use Kdp     %         */
float Hr_HighZThresh;          /* Reflectivity threshold for use   dBZ       *
                                * of R(Kdp) in Heavy Rain                    */
int   Kdp_max_beam_blk;        /* Max Beam Blockage to Use Kdp     %         */
float Kdp_min_usage_rate;      /* Min Z Rate to use Kdp Rate       mm/hr     */
float Refl_min;                /* Min Reflectivity                 dBZ       */
float Refl_max;                /* Max Reflectivity                 dBZ       */
int   Paif_area;               /* PAIF Precipitation Area          km^2      */
float Paif_rate;               /* PAIF Precipitation Rate          mm/hr     */
int   Max_vols_per_hour;       /* Max volumes per hour.            unitless  *
                                * We could have a 2.3 min VCP                */
int   Min_early_term_ang;      /* Min angle for (AVSET) early      degrees   *
                                * volume termination                         */
float Max_precip_rate;         /* Maximum Rate                     mm/hr     */
int   Num_zones;               /* Number of Exclusion Zones        unitless  */
float Beg_azm[MAX_NUM_ZONES];  /* Exclusion Zone - Begin Azimuth   degrees   */
float End_azm[MAX_NUM_ZONES];  /* Exclusion Zone - End   Azimuth   degrees   */
int   Beg_rng[MAX_NUM_ZONES];  /* Exclusion Zone - Begin Range     nm        */
int   End_rng[MAX_NUM_ZONES];  /* Exclusion Zone - End   Range     nm        */
float Elev_ang[MAX_NUM_ZONES]; /* Exclusion Zone - Elevation Angle degrees   */
} dp_precip_adapt_t;

#endif /* DP_PRECIP_ADAPT_H */
