/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/07/30 19:26:11 $
 * $Id: dp_lt_accum_types.h,v 1.5 2014/07/30 19:26:11 dberkowitz Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef DP_LT_ACCUM_TYPES_H
#define DP_LT_ACCUM_TYPES_H

/******************************************************************************
    Filename: dp_lt_accum_types.h

    Description:
    ============
    Declare structures for the Dual Pol Long Term accumulation algorithm.

    Note: LT_Accum_Buf_t is the structure for the DP_LT_ACCUM
          linear buffer. If you change it, also change its max_size in
          ~/cfg/extensions/product_attr_table.dp_precip

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----------  -------    ----------         ----------------
    10/25/2007  0000       Cham Pham          Initial version.
    03/06/2014  0001       Murnan             Added Top of Hour parameters
                                              (CCRs NA12-00223 and NA12-00264)
******************************************************************************/

#include "a313hparm.h"          /* HYZ_MESG                */
#include "dp_lt_accum_Consts.h" /* DP_HRLY_ACCUM_MAXN_MSGS */
#include "qperate_types.h"      /* QPE_Adapt_t             */

/* 2007119 Ward                                                       *
 *                                                                    *
 * 1. last_time_prcp is used by DSA in append_ascii_layer2().         *
 *                                                                    *
 * 2. hrly_begtime is unused, to be added to the Build 12  ICD        *
 *    for the 1 hour product.                                         *
 *                                                                    *
 * 3. We always make a DOD, so the null_One_hr_diff flag was deleted. */

typedef struct
{
  time_t last_time_prcp;                  /* copied from Accum_Supl_t      */
  int    ST_active_flg;                   /* copied from Accum_Supl_t      */
  int    prcp_begin_flg;                  /* copied from Accum_Supl_t      */
  int    prcp_detected_flg;               /* copied from Accum_Supl_t      */
  float  pct_hybrate_filled;              /* copied from Accum_Supl_t      */
  float  highest_elev_used;               /* copied from Accum_Supl_t      */
  float  sum_area;                        /* copied from Accum_Supl_t      */
  int    vol_sb;                          /* copied from Accum_Supl_t      */
  time_t stmtot_begtime;                  /* set by Storm Total            */
  time_t stmtot_endtime;                  /* set by Storm Total            */
  time_t TOH_begtime;                     /* set by Top of Hour            */
  time_t TOH_endtime;                     /* set by Top of Hour            */
  time_t total_TOH_missing_time;          /* set by Top of Hour            */
  time_t total_TOH_accum_time;            /* set by Top of Hour            */
  time_t hrly_begtime;                    /* set by Hourly - UNUSED        */
  time_t hrly_endtime;                    /* set by Hourly                 */
  time_t hrlydiff_begtime;                /* set by Hourly Diff            */
  time_t hrlydiff_endtime;                /* set by Hourly Diff            */
  time_t stmdiff_begtime;                 /* set by Storm Diff             */
  time_t stmdiff_endtime;                 /* set by Storm Diff             */
  int    TOH_min_time_period;             /* time required to exceed before
                                             generating a TOH product(mins)
                                             currently based on Legacy      
                                             min_time_period = 54 mins     */
  int    null_TOH_unbiased;               /* >0/FALSE set by Top of Hour   */
  int    null_One_Hr_biased;              /* >0/FALSE set by Hourly        */
  int    null_One_Hr_unbiased;            /* >0/FALSE set by Hourly        */
  int    null_Storm_Total;                /* >0/FALSE set by Storm Total   */
  int    null_Storm_Total_diff;           /* >0/FALSE set by Storm Diff    */
  int    missing_period_TOH_unbiased;     /* TRUE/FALSE set by Top of Hour */
  int    missing_period_One_Hr_biased;    /* TRUE/FALSE set by Hourly      */
  int    missing_period_One_Hr_unbiased;  /* TRUE/FALSE set by Hourly      */
  int    missing_period_Storm_Total;      /* TRUE/FALSE set by Storm Total */
  int    missing_period_One_hr_diff;      /* TRUE/FALSE set by Hourly Diff */
  int    missing_period_Storm_Total_diff; /* TRUE/FALSE set by Storm Diff  */
} LT_Accum_Supl_t;

/* Note: All 5 LT_Accum_Buf_t grids are in thousandths of inches */

typedef struct
{
  QPE_Adapt_t     qpe_adapt;               /* QPE adaptation data               */
  LT_Accum_Supl_t supl;                    /* Long term accum supplemental data */
  int TOH_unbiased[MAX_AZM][MAX_BINS];     /* Top of Hour unbiased grid         */
  int One_Hr_biased[MAX_AZM][MAX_BINS];    /* One hour biased grid              */
  int One_Hr_unbiased[MAX_AZM][MAX_BINS];  /* One hour unbiased grid            */
  int Storm_Total[MAX_AZM][MAX_BINS];      /* storm total grid                  */
  int One_Hr_diff[MAX_AZM][MAX_BINS];      /* one hour difference grid          */
  int Storm_Total_diff[MAX_AZM][MAX_BINS]; /* storm total difference grid       */
} LT_Accum_Buf_t;

typedef struct
{
   int data_id; /* the linear buffer id that stores the associated accum buffers */

   int first; /* index of the first accum grid in the queue. */
   int last;  /* index of the last accum grid in the queue.  */

   int max_queue; /* maximum number of accum grids tracked. If max_queue < 1 or
                   * max_queue > DP_HRLY_ACCUM_MAXN_MSGS (maximum stored)
                   * an error will be generated. */

   /* begin_time and end_time store beginning/ending Unix times for the
    * accum grids being tracked by the circular queue. Since these are read
    * from disk on a restore, they are set as fixed sized arrays instead of
    * malloc'd pointers. */

   time_t begin_time[DP_HRLY_ACCUM_MAXN_MSGS];
   time_t end_time[DP_HRLY_ACCUM_MAXN_MSGS];

   /* trim_time is the last time the queue was trimmed to an hour by 
    * CQ_Trim_To_Hour(). It is usually set to the end time of the last 
    * accum buffer received. */

   time_t trim_time;

   /* missing_period keeps track of the missing period flags so
    * CQ_Get_Missing_Period() doesn't have to read_from_LB() to check a
    * flag. Since this is read from disk on a restore, it is set as a
    * fixed sized array instead of malloc'd pointers. */

   int missing_period[DP_HRLY_ACCUM_MAXN_MSGS];

} Circular_Queue_t;

/* hyadjscn_large_buf_t holds the contents of a legacy accumulation grid.    *
 * It is needed to compute the difference products.                          *
 *                                                                           *
 * 20071126 Ward - Cham says there are 3 types of legacy buffers, small,     *
 * medium and large, so we split hyadjscn_buf_t into 3 types.                *
 * Flags in the Supl[] can be used to tell which one we have. I also changed *
 *                                                                           *
 *  short Lfm4[13][13];                                                      *
 *                                                                           *
 *  to:                                                                      *
 *                                                                           *
 *  int   Lfm4[85];                                                          *
 *                                                                           *
 * because although Lfm4[13][13] captures the *sense* of what the Lfm4       *
 * is trying to do, in the definition of the Lfm4 grid,                      *
 *                                                                           *
 * ~/include/a313hparm.h has:                                                *
 *                                                                           *
 * #define HYZ_LFM4     13                                                   *
 * #define HYO_BHEADER  (HYO_LFM4+(HYZ_LFM4*HYZ_LFM4+1)/2)                   *
 *         = 85 ints = 169 shorts + 1 int.                                   *
 *                                                                           *
 * HYO_LFM4 is the (int) offset into the buffer of Lfm4.                     *
 * The extra int is probably for alignment.                                  *
 *                                                                           *
 * HYZ_MESG (6), HYZ_ADAP, HYZ_SUPL, HYZ_LFM4 (13), N_BIAS_LINES (10),       *
 * N_BIAS_FLDS (5), MAX_ADJBINS (115), MAX_AZMTHS (360)                      *
 * are defined in a313hparm.h                                                *
 *                                                                           *
 * Note that the row/column order of BiasTable/AccumScan/AccumHrly is        *
 * reversed from the FORTRAN. This is because C stores arrays in row-major   *
 * order, and FORTRAN stores arrays in column-major order                    *
 * -> FORTRAN arrays are transposed.                                         */

typedef struct
{
   int   Mesg[HYZ_MESG];
   float Adapt[HYZ_ADAP];
   int   Supl[HYZ_SUPL];
   int   Lfm4[85];
   char  BiasSource[4];
   int   BiasTable[N_BIAS_LINES][N_BIAS_FLDS];
} hyadjscn_small_buf_t;

typedef struct
{
   hyadjscn_small_buf_t small;
   short AccumScan[MAX_AZMTHS][MAX_ADJBINS]; /* Cham 20071113 - 10s of mms */
} hyadjscn_medium_buf_t;

typedef struct
{
   hyadjscn_medium_buf_t medium;
   short AccumHrly[MAX_AZMTHS][MAX_ADJBINS]; /* Cham 20071113 - 10s of mms */
} hyadjscn_large_buf_t;

/* Storm_Backup_t holds the contents of DP_STORM.DAT disk file */

typedef struct
{
  int    StormTotal[MAX_AZM][MAX_BINS]; /* storm accum in thousands of inches */
  time_t storm_begtime;                 /* storm start                        */
  time_t storm_endtime;                 /* storm end                          */
  int    missing_period;                /* missing period flag                */
  int    num_bufs;                      /* number of buffers in StormTotal    */
} Storm_Backup_t;

#endif /* DP_LT_ACCUM_TYPES_H */
