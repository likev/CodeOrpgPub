/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:08 $
 * $Id: gauge_radar_types.h,v 1.3 2011/04/13 22:53:08 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef GAUGE_RADAR_TYPES_H
#define GAUGE_RADAR_TYPES_H

/******************************************************************************
    Filename: gauge_radar_types.h

    Description:
    ============
    These are the structures for the DualPol gauges algorithm.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----------  -------    ----------         ----------------
    20091204    0000       James Ward         Initial version.
******************************************************************************/

#include "gauge_radar_consts.h" /* MAX_GAUGES          */
#include "gauge_radar_query.h"  /* gauge_radar_query_t */
#include <dp_Consts.h>          /* MAX_AZM             */

typedef struct
{
  gauge_radar_query_t query;              /* Must be first in Gauges_Buf_t */
  float               gauges[MAX_GAUGES]; /* gauges data */
} Gauges_Buf_t;

/* output buffer metadata structure */

typedef struct
{
    time_t start_time;	
    time_t end_time;
    int missing_period_flag;
    int null_product_flag;
    int max_accum;
    float bias;	
} dua_accum_buf_metadata_t;

/* output buffer structure */

typedef struct
{
    dua_accum_buf_metadata_t metadata;     /* metadata part of the message */
    int dua_accum_grid[MAX_AZM][MAX_BINS]; /* grid data for accumulation   */
} dua_accum_buf_t;

/* stats hold the stats for convenience */

typedef struct
{
    float gauge_mean; /* gauge mean               */
    float gauge_sd;   /* gauge standard deviation */
    float radar_mean; /* radar mean               */
    float radar_sd;   /* radar standard deviation */

    short got_gauge_mean; /* TRUE -> got gauge mean               */
    short got_gauge_sd;   /* TRUE -> got gauge standard deviation */
    short got_radar_mean; /* TRUE -> got radar mean               */
    short got_radar_sd;   /* TRUE -> got radar standard deviation */

    short gr_pairs; /* gauge-radar pairs */

    float mfb;   /* mean field bias                  */
    float ab;    /* additive bias                    */
    float me ;   /* mean error                       */
    float mae;   /* mean absolute error              */
    float rmse;  /* root mean square error           */
    float sd;    /* standard deviation               */
    float cc;    /* correlation coefficient          */
    float fb;    /* fractional bias                  */
    float frmse; /* fractional root mean square error*/
    float fsd;   /* fractional standard deviation    */

    short got_mfb;   /* TRUE -> got mean field bias         */
    short got_ab;    /* TRUE -> got additive bias           */
    short got_me;    /* TRUE -> got mean error              */
    short got_mae;   /* TRUE -> got mean absolute error     */
    short got_rmse;  /* TRUE -> got root mean square error  */
    short got_sd;    /* TRUE -> got standard deviation      */
    short got_cc;    /* TRUE -> got correlation coefficient */
    short got_fb;    /* TRUE -> got fractional bias         */
    short got_frmse; /* TRUE -> got fractional rmse         */
    short got_fsd;   /* TRUE -> got fractional sd           */

} stats_t;

typedef struct
{
  char   id[ID_LEN]; /* name                         */
  float  lat;        /* latitude                     */
  float  lon;        /* longitude                    */
  float  km;         /* Km from radar                */
  float  azm;        /* azimuth                      */
  short  azm_s;      /* azimuth as a short           */
  short  dp_bin;     /* bin at 1/4 Km DP  resolution */
  short  pps_bin;    /* bin at   2 Km PPS resolution */
  short  x;          /* 1/4 Km screen x coordinate   */
  short  y;          /* 1/4 Km screen y coordinate   */
} Gauge;


/* product dependent parameters in PDB */

typedef struct
{
  short param_1;	/* halfword 27 - End Time */
  short param_2;	/* halfword 28 - Time Span  */
  short param_3;	/* halfword 30 - Missing flag (high byte) + Null flag
                                                              (low byte)*/
  short param_4;	/* halfword 47 - Max Accum Value */
  short param_5;	/* halfword 48 - End Date */
  short param_6;	/* halfword 49 - Start Time */
  short param_7;	/* halfword 50 - Mean-field Bias */
  short param_8;	/* halfword 51 - Not used */
  short param_9;	/* halfword 52 - Not used */
  short param_10;	/* halfword 53 - Not used */
} prod_dep_para_t;

/* packet 1 header structure   */
typedef struct {
  short packet_code;
  short data_block_length; /* length of data block in bytes */
}Packet1_hdr_t;

/* packet 1 data block structure  */
typedef struct {
  short I_start; /* screen coordinates */
  short J_start;
  char data[80];
}Packet1_data_t;

#endif /* GAUGE_RADAR_TYPES_H */
