/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 15:06:55 $
 * $Id: dp_lt_accum_Consts.h,v 1.2 2009/03/03 15:06:55 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef DP_LT_ACCUM_CONSTS_H
#define DP_LT_ACCUM_CONSTS_H

/******************************************************************************
    Filename: dp_lt_accum_Consts.h

    Description:
    ============
    Declare constants for the Dual Pol Long Term accumulation algorithm.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----------  -------    ----------         --------------------------
    10/25/2007  0000       Cham Pham          Initial version.
    11/21/2008  0001       James Ward         Removed BEGIN_TIME/END_TIME
******************************************************************************/

/* Circular Queue constants. CQ_EMPTY is set to -1, which can't
 * be the index of any non-negative slot */

#define CQ_EMPTY   -1

#define CQ_SUCCESS  0
#define CQ_FAILURE  1

/* DP_HRLY_ACCUM_MAXN_MSGS should match what's in the data_attr_table
 * for the DP_HRLY_ACCUM data store maxn_msgs. 30 messages > an
 * AVSET VCP of 2.3 min/volume. The lowest value we expect is
 * 15 messages > a VCP of 5 min/volume. */

#define DP_HRLY_ACCUM_MAXN_MSGS 30

/* INTERP_BACKWARD and INTERP_FORWARD are for Interpolate_grid() */

#define INTERP_BACKWARD -1
#define INTERP_FORWARD   1

/* LB message ids for DP hourly data store */

#define HOURLY_ID      1
#define HOURLY_DIFF_ID 2

/* LB message id for DP storm total data store */

#define STORM_ID      1
#define STORM_DIFF_ID 2

#define DP_LT_ACCUM_DEBUG FALSE

#endif /* DP_LT_ACCUM_CONSTS_H */
