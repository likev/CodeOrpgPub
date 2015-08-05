/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/30 19:24:06 $
 * $Id: qperate_types.h,v 1.13 2014/07/30 19:24:06 dberkowitz Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

#ifndef QPERATE_TYPES_H
#define QPERATE_TYPES_H

/******************************************************************************
    Filename: qperate_types.h

    Description:
    ============
    Declare structures for the QPERATE algorithm.

    Note: Rate_Buf_t is the structure for the QPERATE
          linear buffer. If you change it, also change its max_size in
          ~/cfg/extensions/product_attr_table.dp_precip

    Note: HHC_Buf_t is the structure for the QPEHHC
          linear buffer. If you change it, also change its max_size in
          ~/cfg/extensions/product_attr_table.dp_precip

    Change History
    ==============
    DATE        VERSION    PROGRAMMER     NOTES
    ----------  -------    ----------     ---------------
    20070924     0000      Cham Pham      Initial version
    20080811     0001      James Ward     Deleted num_exzone from qpe_adapt,
                                          dp_adapt.num_zone is used instead.
    20100310     0002      James Ward     Deleted RhoHV_min_Kdp_rate,
                                          replaced with dpprep.alg
                                          corr_thresh
    20101221     0003      James Ward     Added attenuation flag
    20111031     0004      James Ward     For CCR NA11-00372:
                                          Deleted RhoHV_min_rate,
                                          replaced with dpprep.alg
                                          art_corr
    20121028     0005      D.Berkowitz    For CCR NA12-00361 and 362
                                          Deleted useMLDAHeights,
                                          replaced with Melting_Layer_Source
    20140306     0006      Murnan         Added hourly min_time_period for
                                          Top of Hour accumulation.
                                          (CCR NA12-00264)
******************************************************************************/

/* System and ORPG Includes----------------------------------------- */

#include <math.h>
#include <stdlib.h>

#include <rpg_globals.h>
#include <orpgsite.h>
#include <generic_basedata.h>

/* Required by rpgc after API enhancement patch -------------------- */

#include <gen_stat_msg.h>
#include <orpgctype.h>
#include <product.h>
#include <orpgpat.h>
#include <vcp.h>

/* A new support library provided in API enhancement patch --------- */

#include <rpgc.h>
#include <rpgcs.h>

/* DUALPOL Adaptation Includes ------------------------------------- */

#include <alg_adapt.h>
#include <dp_precip_adapt.h>
#include <hydromet_prep.h>
#include <hydromet_acc.h>
#include <hydromet_adj.h>
#include <itc.h>             /* A3136C3_t */
#include <siteadp.h>

#include <qperate_Consts.h>  /* MAX_AZM                  */
#include <dp_elev_types.h>   /* Compact_dp_basedata_elev */

/* QPE Moment Data  =================================== */

typedef struct
{
  float Z;           /* reflectivity          */
  float Kdp;         /* KDP                   */
  float CorrelCoef;  /* RhoHv                 */
  float Zdr;         /* ZDR                   */
  short HydroClass;  /* HCA hydroclass        */
  short attenuated;  /* TRUE -> is attenuated */
} Moments_t;

/* QPE Adaptation Data  =================================== */

/* prep_adapt_t is filled from hydromet_prep.alg */

typedef struct {
  int   rain_time_thresh; /* (RAINT) Threshold to reset storm total */
} prep_adapt_t;

/* accum_adapt_t is filled from hydromet_acc.alg */

typedef struct {
  int   restart_time;     /* (TIMRS) Threshold Elapsed Time to Restart */
  int   max_interp_time;  /* (MXTIN) Maximum Time for Interpolation    */
  int   min_time_period;  /* (MXTIP) Minimum Hourly Time period        */
  int   max_hourly_acc;   /* (MXHAC) Maximum Hourly Accumulation Value */
  int   max_period_acc;   /* (MXPAC) Maximum Period Accumulation Value */
} accum_adapt_t;

/* adj_adapt_t is filled from hydromet_adj.alg */

typedef struct {
  int   time_bias;        /* (TBIES) Time Bias Estimation            */
  int   num_grpairs;      /* (NGRPS) Threshold # of Gage-Radar Pairs */
  float reset_bias;       /* (RESBI) Reset Bias Value                */
  int   longst_lag;       /* (LGLAG) Longest Allowable Lag (Hours)   */
  int   bias_flag;        /* (empty) Bias Applied Flag               */
} adj_adapt_t;

/* dpprep_adapt_t is filled from dpprep.alg and dpprep_isdp.h */

typedef struct {
  float  corr_thresh; /* Correlation threshold for Kdp calculation      */
  float  art_corr;    /* Correlation threshold for high-attenuation bin */
  int    isdp_apply;  /* 1=apply isdp estimate adjustment to base data  */
  int    isdp_est;    /* estimated ISDP (deg)                           */
  int    isdp_yy;     /* 2-digit year of ISDP estimate                  */
  int    isdp_mm;     /* month of ISDP estimate                         */
  int    isdp_dd;     /* day of ISDP estimate                           */
  int    isdp_hr;     /* hour of ISDP estimate                          */
  int    isdp_min;    /* minute of ISDP estimate                        */
} dpprep_adapt_t;

/* mlda_adapt_t is filled from mlda.alg. It is not used, *
 * only displayed in the TAB/layer2.                     */

typedef struct
{
  float default_ml_depth;      /* (ml_depth) default melting layer depth */
  int   Melting_Layer_Source;  /* MLDA heights are either -
                                * 0 = MLDA_UseType_Default (RPG_0C_Hgt),
                                * 1 = MLDA_UseType_Radar (Radar_Based),
                                * 2 = MLDA_UseType_Model (Model_Enhanced),
                                * where value of 2 is the normal default.
                                */
} mlda_adapt_t;

/* dp_precip.alg is all of the new Dual-Pol adaptation data
 * collected into one file. The other *.alg are legacy files,
 * with the exception of mlda.alg, which is new by Brian, and
 * the bias_info. */

typedef struct
{
  prep_adapt_t       prep_adapt;     /* from hydromet_prep.alg */
  accum_adapt_t      accum_adapt;    /* from hydromet_acc.alg  */
  adj_adapt_t        adj_adapt;      /* from hydromet_adj.alg  */
  dpprep_adapt_t     dpprep_adapt;   /* from dpprep.alg        */
  mlda_adapt_t       mlda;           /* from mlda.alg          */
  dp_precip_adapt_t  dp_adapt;       /* from dp_precip.alg     */
  A3136C3_t          bias_info;      /* bias info              */
  char               bias_source[5]; /* bias source            */
} QPE_Adapt_t;

/* QPE Supplemental Data  =================================== */

typedef struct
{
  time_t time;               /* time of rate grid                 */
  time_t last_time_prcp;     /* last time of precipitation        */
  time_t start_time_prcp;    /* start time of precipitation       */
  int    prcp_detected_flg;  /* TRUE -> precip. has been detected */
  int    ST_active_flg;      /* TRUE -> a storm is active         */
  int    prcp_begin_flg;     /* TRUE -> precipitation has begun   */
  float  pct_hybrate_filled; /* % hybrid rate grid filled         */
  float  highest_elang;      /* highest elevation angle used      */
  float  sum_area;           /* sum of the area with precip.      */
  int    vol_sb;             /* volume spot blank                 */
} Rate_Supl_t;

/* Two intermediate output buffers =================================== */

typedef struct
{
  QPE_Adapt_t qpe_adapt;             /* QPE adaptation data    */
  Rate_Supl_t rate_supl;             /* Rate supplemental data */
  float RateComb[MAX_AZM][MAX_BINS]; /* Combined rate grid     */
} Rate_Buf_t;

typedef struct
{
   time_t time;                         /* time of rate grid            */
   float  pct_hybrate_filled;           /* % hybrid rate grid filled    */
   float  highest_elang;                /* highest elevation angle used */
   short  mode_filter_length;           /* Length of the mode filter    */
   short  HybridHCA[MAX_AZM][MAX_BINS]; /* hybrid hydroclass used       */
} HHC_Buf_t;

/* For regression testing =================================== */

typedef struct
{
  float Rz[MAX_AZM][MAX_BINS];
  float Rkdp[MAX_AZM][MAX_BINS];
  float Rec[MAX_AZM][MAX_BINS];
  float Rzdr[MAX_AZM][MAX_BINS];
  float rhohv[MAX_AZM][MAX_BINS];
} Regression_t;

/* For stats collection =================================== */

typedef struct
{
  float rate[MAX_AZM][MAX_BINS];
  short reason[MAX_AZM][MAX_BINS];
} Stats_t;

/* The Bias_table_msg is the Prod_bias_table_msg from ~/include/legacy_prod.h
 * without the initial message header:
 *
 * Prod_msg_header_icd mhb;    -* message header block *-
 */

typedef struct {                /* Bias Table Environmental Data Message */
    Block_id_t block;           /* environmental data block header */
    char awips_id[4];           /* AWIPS site ID ... NOT NULL TERMINATED */
    char radar_id[4];           /* radar site ID ... NOT NULL TERMINATED */
    short obs_yr;               /* observation year (1970 - 2099) */
    short obs_mon;              /* observation month (1 - 12) */
    short obs_day;              /* observation day (1 - 31) */
    short obs_hr;               /* observation hour (0 - 23) */
    short obs_min;              /* observation minute (0 - 59) */
    short obs_sec;              /* observation second (0 - 59) */
    short gen_yr;               /* generation year (1970 - 2099) */
    short gen_mon;              /* generation month (1 - 12) */
    short gen_day;              /* generation day (1 - 31) */
    short gen_hr;               /* generation hour (0 - 23) */
    short gen_min;              /* generation minute (0 - 59) */
    short gen_sec;              /* generation second (0 - 59) */
    short n_rows;               /* number of bias table rows (2 - 12) */
    Memory_span_t span[12];     /* memory span data. */
} Bias_table_msg;

#endif /* QPERATE_TYPES_H */
