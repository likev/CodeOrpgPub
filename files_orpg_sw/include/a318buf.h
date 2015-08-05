/*
 * Module: a318buf.h
 * cpc018 Meso Rapid Update Product buffer definitions
 *
 *      The contents in this file are derived from both A317BUF.INC and 
 *      A315BUF.INC. The structures must be consistent with those defined
 *      there. Thus, if either A317BUF.INC or A315BUF.INC are modified,
 *      this file has to be updated accordingly.
 *
 * Notes:
 *	MESORUATTR - MESO 3D RAPID UPDATE ATTRIBUTES (A copy of MSTV3ATR
 *                   buffer #41, made after each Doppler elevation cut but
 *                   with the elevation time added.) DO NOT CHANGE ANY PART
 *                   OF THIS STRUCTURE WITHOUT MAKING COORESPONDING CHANGES
 *                   TO MSTV3ATR!!!
 *
 *      TRFRCATR   - Storm Tracking and Forecast Attributes Buffer. (See
 *                   A315BUF.INC)
 *
 *      MESORUPROD - Meso rapid update output product structure.  This is
 *                   similar to MESORUATTR but without the adaptation data.
 *
 *   Care should be taken when reading these structure and field names as
 *   many look similar but have different meanings.  Note the mesoruattr_hdr
 *   is not the same as the mesoruprod_hdr.  Also note that the term features,
 *   when applied to the attribute structure means only non-meso features,
 *   whereas when applied to the product structure refers to all feature types. 
 *
 */

/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/05/24 16:31:16 $
 * $Id: a318buf.h,v 1.4 2007/05/24 16:31:16 ryans Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef A318BUF_H
#define A318BUF_H

/*
 * Include for STA_COSP and STA_MAXT values
*/
#include <a315buf.h>

/*
 * Include for Graphic_product header
 */
#include "product.h"

/*
 * Array sizes for Feature and Mesocyclone Tables and other defines
 */
#define FMAX        650         /* maximum number of attribute features    */
#define MMAX         20         /* maximum number of attribute mesos       */
#define RUMAX       100         /* maximum number of rapid update features */

#define NO            0
#define YES           1
#define MESO_TYPE     1         /* used in type field of feature attribute */
#define CORSHR_TYPE   2         /* used in type field of feature attribute */
#define UNCORSHR_TYPE 3         /* used in type field of feature attribute */
#define INC_FLAG      3         /* used in new_flag for increasing strength*/
#define UPD_FLAG      2         /* used in new_flag for updated (excldg azran)*/
#define NEW_FLAG      1         /* used in new_flag for new features       */
#define EXT_FLAG      0         /* used in new_flag for extrapolated feats */
#define SEC_PER_DAY   86400     /* seconds per day                         */
#define NUM_GAB_ROWS  5;        /* number of rows in the GAB table         */


/*
 * MESORUATTR Header
 */
typedef struct
{
   int   vol_time;               /* Volume scan start time (msec)        */
   int   num_feat;               /* Number of features detected          */
   int   num_meso;               /* Number of Mesocyclones detected      */
   int   elev_time;              /* Elevation scan start time (msec)     */
   int   last_elev_flag;         /* Set on last elevation of volume      */
} mesoruattr_hdr_t;

/*
 * MESORUATTR Adaptation Data
 */
typedef struct
{
   int   max_num_feats;          /* Maximum number of Features           */
   int   max_num_meso;           /* Maximum number of Mesocyclones       */
   int   ptrn_vktr_th;           /* Pattern Vector Threshold             */
   float hi_mom_th;              /* high momentum threshold      (units?)*/
   float lo_mom_th;              /* low momentum threshold       (units?)*/
   float hi_shr_th;              /* high shear threshold           (1/s?)*/
   float lo_shr_th;              /* low shear threshold            (1/s?)*/
   float max_dia_ratio_th;       /* max diameter ratio threshold         */
   float far_max_dia_ratio_th;   /* far max diameter ratio threshold     */
   float min_dia_ratio_th;       /* min diameter ratio threshold         */
   float far_min_dia_ratio_th;   /* far min diameter ratio threshold     */
   float rng_th;                 /* range threshold                  (km)*/
   float rdl_dist_th;            /* radial distance threshold        (km)*/
   float az_th;                  /* azimuth threshold               (deg)*/
   float max_meso_ht;            /* maximum mesocyclone height       (km)*/
   float tvs_srch_pct;           /* TVS Search Percentage(no longer used)*/
   float tvs_smr_th;		 /* TVS smear threshold  (no longer used)*/
} mesoruattr_adapt_t;

/*
 * MESORUATTR Features Attribute Data
 */
typedef struct
{
   int   type[FMAX];             /* Feature Type (1=MESO,2=3D_COR,3=UNC) */
   int   storm_num[FMAX];        /* Numeric Storm ID(index into tracking)*/
   float cntr_az[FMAX];          /* Center Azimuth                  (deg)*/
   float cntr_rng[FMAX];         /* Center Range                     (km)*/
   float el[FMAX];               /* Elevation Angle                 (deg)*/
   float ht[FMAX];               /* Height                           (km)*/
   float az_dia[FMAX];           /* Azimuth Diameter                 (km)*/
   float rng_dia[FMAX];          /* Range Diameter                   (km)*/
   float avg_st[FMAX];           /* Average Shear                   (1/s)*/
   float max_st[FMAX];           /* Maximum Shear                   (1/s)*/
   float avg_spdr[FMAX];         /* Average Rotational Speed        (m/s)*/
   float max_spdr[FMAX];         /* Maximum Rotational Speed        (m/s)*/
   float top_ht[FMAX];           /* Top Height                       (km)*/
   float bot_ht[FMAX];           /* Bottom Height                    (km)*/
   float bot_az[FMAX];           /* Bottom Azimuth                  (deg)*/
   float bot_rng[FMAX];          /* Bottom Range                     (km)*/
   float bot_el[FMAX];           /* Bottom Elevation Angle          (deg)*/
   float max_ts[FMAX];           /* Maximum Tangential Shear (TS)   (1/s)*/
   float max_ts_rng[FMAX];       /* Maximum TS Range                 (km)*/
   float max_ts_begaz[FMAX];     /* Maximum TS Begin Azimuth        (deg)*/
   float max_ts_endaz[FMAX];     /* Maximum TS End Azimuth          (deg)*/
   float beg_rng[FMAX];          /* (Extrema) Begin Range            (km)*/
   float end_rng[FMAX];          /* (Extrema) End Range              (km)*/
   float begaz[FMAX];            /* (Extrema) Begin Azimuth         (deg)*/
   float endaz[FMAX];            /* (Extrema) End Azimuth           (deg)*/
} mesoruattr_feat_t;

/*
 * MESORUATTR Mesocyclone Attribute Data
 */
typedef struct
{
   int   storm_num[MMAX];        /* Numeric Storm ID(index into tracking)*/
   float hi_avg_shr[MMAX];       /* Highest Average Shear           (1/s)*/
   float cntr_az[MMAX];          /* Center Azimuth                  (deg)*/
   float cntr_rng[MMAX];         /* Center Range                     (km)*/
   float el[MMAX];               /* Elevation Angle                 (deg)*/
   float ht[MMAX];               /* Height                           (km)*/
   float az_dia[MMAX];           /* Azimuth Diameter                 (km)*/
   float rng_dia[MMAX];          /* Range Diameter                   (km)*/
   float avg_spdr[MMAX];         /* Average Rotational Speed        (m/s)*/
   float max_spdr[MMAX];         /* Maximum Rotational Speed        (m/s)*/
   float top_ht[MMAX];           /* Top Height                       (km)*/
   float bot_ht[MMAX];           /* Bottom Height                    (km)*/
   float bot_az[MMAX];           /* Bottom Azimuth                  (deg)*/
   float bot_rng[MMAX];          /* Bottom Range                     (km)*/
   float bot_el[MMAX];           /* Bottom Elevation Angle          (deg)*/
   float max_ts[MMAX];           /* Maximum Tangential Shear (TS)   (1/s)*/
   float max_ts_rng[MMAX];       /* Maximum TS Range                 (km)*/
   float max_ts_begaz[MMAX];     /* Maximum TS Begin Azimuth        (deg)*/
   float max_ts_endaz[MMAX];     /* Maximum TS End Azimuth          (deg)*/
   float beg_rng[MMAX];          /* (Extrema) Begin Range            (km)*/
   float end_rng[MMAX];          /* (Extrema) End Range              (km)*/
   float begaz[MMAX];            /* (Extrema) Begin Azimuth         (deg)*/
   float endaz[MMAX];            /* (Extrema) End Azimuth           (deg)*/
} mesoruattr_meso_t;

/*
 * MESORUATTR (comes from meso3d task)
 */
typedef struct
{
   mesoruattr_hdr_t   hdr;
   mesoruattr_adapt_t adapt;
   mesoruattr_feat_t  feat;
   mesoruattr_meso_t  meso;
} mesoruattr_t;


/*
 * Data Array Header
 */
typedef struct
{
   int   j_date;           /* Volume scan start date (NEXRAD julian date)*/
   int   vol_time;         /* Volume scan start time (msec)        */
   int   elev_time;        /* Elevation scan start time (msec)     */
   float elev;             /* Elevation angle  (deg)               */
   int   num_RUfeat;       /* Number of Features in RU product     */
} mesorudata_hdr_t;


/*
 * Data array feature information (used for all feature types)
 */
typedef struct
{
   int   feat_num;         /* index into original feat or meso table   */
   char  storm_id[3];      /* alphanumeric id of associated storm cell */
   int   type;             /* 1=MESO, 2=3D COR, 3=UNC                  */
   int   new_flag;         /* 1=new (not in previous volume)           */
   int   storm_num;        /* index into SCIT tracking table           */
   float cntr_az;          /* Center Azimuth                      (deg)*/
   float cntr_rng;         /* Center Range                         (km)*/
   float el;               /* Elevation Angle                     (deg)*/
   float ht;               /* Height                               (km)*/
   float az_dia;           /* Azimuth Diameter                     (km)*/
   float rng_dia;          /* Range Diameter                       (km)*/
   float avg_st;           /* Average Shear                       (1/s)*/
   float max_st;           /* Maximum Shear                       (1/s)*/
   float avg_spdr;         /* Average Rotational Speed            (m/s)*/
   float max_spdr;         /* Maximum Rotational Speed            (m/s)*/
   float top_ht;           /* Top Height                           (km)*/
   float bot_ht;           /* Bottom Height                        (km)*/
   float bot_az;           /* Bottom Azimuth                      (deg)*/
   float bot_rng;          /* Bottom Range                         (km)*/
   float bot_el;           /* Bottom Elevation Angle              (deg)*/
   float max_ts;           /* Maximum Tangential Shear (TS)       (1/s)*/
   float max_ts_rng;       /* Maximum TS Range                     (km)*/
   float max_ts_begaz;     /* Maximum TS Begin Azimuth            (deg)*/
   float max_ts_endaz;     /* Maximum TS End Azimuth              (deg)*/
   float beg_rng;          /* (Extrema) Begin Range                (km)*/
   float end_rng;          /* (Extrema) End Range                  (km)*/
   float begaz;            /* (Extrema) Begin Azimuth             (deg)*/
   float endaz;            /* (Extrema) End Azimuth               (deg)*/
   float hi_avg_shr;       /* Highest Average Shear(MESOs only)   (1/s)*/
} mesorudata_feat_t;

/*
 * Data array update flag information (used for all feature types)
     Each TAB displayed field has an update flag set to '^' to indicate
     that the field was updated.  Its blank otherwise.
 */
typedef struct
{
   char  type_f[2];           /* Feature type                             */
   char  ht_f[2];             /* Height                                   */
   char  dia_f[2];            /* Diameter                                 */
   char  top_ht_f[2];         /* Top Height                               */
   char  bot_az_rng_ht_f[2];  /* Bottom Azimuth/range/height              */
   char  max_ts_f[2];         /* Maximum Tangential Shear (TS)            */
} mesorudata_flags_t;
 
 
/*
 * Structure for data array style output
 */
typedef struct
{
   mesorudata_hdr_t   hdr;
   mesorudata_feat_t  RUFeatures[RUMAX];
   mesorudata_flags_t RUFlags[RUMAX];
} mesorudata_t;


/*
 * Array size for Storm Tracking and Forecast Tables.
 */
#define NSTF_MAX    100         /* maximum number of storm        */
#define NSTF_INT      4         /* number of forecast intervals   */
#define NSTF_MPV     13         /* number of past volumes tracked */
#define NSTF_ADP     57         /* elements in storm tracking adaptation data */

/*
 * TRFRCATR Header (See analogous fortran parameters in A315BUF.INC)
 */
typedef struct
{
   int   bnt;      /* Number of storms                     */
   int   bno;      /* Number of storms with motion determined*/
   float bvs;      /* Average speed of all storms          */
   float bvd;      /* Average direction of all storms      */
} tracking_hdr_t;


/*
 * TRFRCATR ID-Type Table
 */
typedef struct
{
   int   num_id;   /* Numerical ID                            */
   int   type;     /* Storm type (new or continuing)          */
   int   nvl;      /* Number of volumes cell has been tracked */
} tracking_bsi_t;

/*
 * TRFRCATR Storm Cell Motion Table
 */
typedef struct
{
   float x0;       /* x-coordinate location (km)              */
   float y0;       /* y-coordinate location (km)              */
   float xsp;      /* x-coordinate speed (m/s)                */
   float ysp;      /* y-coordinate speed (m/s)                */
   float spd;      /* polar coordinate speed (m/s)            */
   float dir;      /* polar coordinate direction (deg)        */
   float err;      /* forecast error (km)                     */
   float mfe;      /* mean forecast error (km)                */
} tracking_bsm_t;

/*
 * TRFRCATR Storm Cell Forward Table
 */
typedef struct
{
   float xf[NSTF_INT]; /* forecast x-coordinate location (km) */
   float yf[NSTF_INT]; /* forecast y-coordinate location (km) */
} tracking_bsf_t;

/*
 * TRFRCATR Storm Cell Backward Table
 */
typedef struct
{
   float xb[NSTF_MPV]; /* past volume x-coordinate location (km)*/
   float yb[NSTF_MPV]; /* past volume y-coordinate location (km)*/
} tracking_bsb_t;

/*
 * TRFRCATR Storm Tracking Adaptation data
 */
typedef union
{
   int adp_i[NSTF_ADP];   /* integer based adaptation data  */
   float adp_f[NSTF_ADP]; /* float based adaptation data    */
} tracking_bfa_t;


/*
 * TRFRCATR
 */
typedef struct
{
   tracking_hdr_t   hdr;            /* Header                  */
   tracking_bsi_t   bsi[NSTF_MAX];  /* ID_type table (numerical IDs, types & # volumes tracked)*/
   tracking_bsm_t   bsm[NSTF_MAX];  /* Storm Cell Motion Table (current positions & speed) */
   tracking_bsf_t   bsf[NSTF_MAX];  /* Storm Cell Foward Table (forecast positions)*/
   tracking_bsb_t   bsb[NSTF_MAX];  /* Storm Cell Backward Table (past positions) */
   tracking_bfa_t   bfa;            /* Adaptation data         */
} tracking_t;

#endif

