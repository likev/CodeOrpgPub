/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 15:06:56 $
 * $Id: precip_grid_rec.h,v 1.3 2009/03/03 15:06:56 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef PRECIP_GRID_REC_H
#define PRECIP_GRID_REC_H

/******************************************************************************
    Filename: precip_grid_rec.h

    Description: 
    ============
    Declare structure for the DualPol database query.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----------  -------    ----------         ----------------
    10/15/2007  0000       Zhan Zhang         Initial version.
******************************************************************************/

typedef struct {
  time_t begin_time;         /* beginning time of accum           */
  time_t end_time;           /* ending time of accum              */
  int    prcp_detected_flg;  /* TRUE -> precip. has been detected */
  int    ST_active_flg;      /* TRUE -> a storm is active         */
  int    missing_period_flg; /* TRUE -> a period is missing       */
} dua_query_t;

#endif /* PRECIP_GRID_REC_H */
