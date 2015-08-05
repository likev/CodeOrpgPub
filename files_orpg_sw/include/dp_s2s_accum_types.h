/*
 * RCS info
 * $Author:
 * $Locker:
 * $Date:
 * $Id:
 * $Revision:
 * $State:
 */

#ifndef DP_S2S_ACCUM_TYPES_H
#define DP_S2S_ACCUM_TYPES_H

/******************************************************************************
    Filename: dp_s2s_accum_types.h

    Description:
    ============
    Declare structures for the DualPol Scan-to-scan accumulation algorithm.

    Note: S2S_Accum_Buf_t is the structure for the DP_S2S_ACCUM
          linear buffer. If you change it, also change its max_size in
          ~/cfg/extensions/product_attr_table.dp_precip

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         -----
    10/15/2007  0000       James Ward         Initial version.

    02/19/2014  0001       Murnan/Berkowitz   Missing time accounting added.
                                              (CCR NA12-00264)
******************************************************************************/

#include <qperate_types.h>   /* QPE_Adapt_t */
#include <precip_grid_rec.h> /* dua_query_t */

/* S2S_Accum_Supl_t is the data structure for supplemental data. */

typedef struct
{
  time_t begin_time;          /* time of 1st rate grid            */
  time_t end_time;            /* time of 2nd rate grid            */
  time_t last_time_prcp;      /* last time of precipitation       */
  time_t start_time_prcp;     /* start time of precipitation      */
  time_t missing_period_time; /* when missing_period_flag is TRUE */
                              /* and the total S2S accumulation   */
                              /* period is < restart time (60mins)*/
                              /* then this time is the remainder  */
                              /* of the S2S accumulation period   */
                              /* not accounted for by             */
                              /* interpolation.                   */
  int    prcp_detected_flg;   /* TRUE -> precip. detected         */
  int    ST_active_flg;       /* TRUE -> a storm is active        */
  int    prcp_begin_flg;      /* TRUE -> precip. has just begun   */
  int    missing_period_flg;  /* TRUE -> s2s has a missing period */
  float  pct_hybrate_filled;  /* % hybrid rate grid filled        */
  float  highest_elev_used;   /* highest elevation angle used     */
  float  sum_area;            /* sum of the area with precip.     */
  int    vol_sb;              /* volume spot blank                */
  int    null_accum;          /* > 0 -> no accum_grid was made    */
} S2S_Accum_Supl_t;

/* Note: Rate grids are in inches/hour, but         *
 *       accum_grid[][] is in thousands of inches.  *
 *       accum_grid[][] is signed, for subtraction. */

typedef struct
{
  dua_query_t      dua_query;             /* Must be first in S2S_Accum_Buf_t */
  QPE_Adapt_t      qpe_adapt;                     /* QPE adaptation data      */
  S2S_Accum_Supl_t supl;                          /* S2S supplemental data    */
  short            accum_grid[MAX_AZM][MAX_BINS]; /* grid of accumulation     */
} S2S_Accum_Buf_t;

#endif /* DP_S2S_ACCUM_TYPES_H */
