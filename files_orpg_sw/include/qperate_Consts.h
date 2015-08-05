/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2013/12/19 17:36:26 $
 * $Id: qperate_Consts.h,v 1.6 2013/12/19 17:36:26 dberkowitz Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef QPERATE_CONSTS_H
#define QPERATE_CONSTS_H

/******************************************************************************
    Filename: qperate_Consts.h

    Description:
    ============
    Declare constants for the QPERATE algorithm.

    Change History
    ==============
    DATE       VERSION  PROGRAMMER    NOTES
    ---------- -------  ----------    ---------------
    09/24/2007  0001    Cham Pham     Initial version
    03/10/2010  0002    James Ward    Added DPPREP_DEA_FILE
                                      to fetch dpprep.alg
                                      corr_thresh.
    10/31/2011  0003    James Ward    For CCR NA11-00372:
                                      Deleted MIDDLE_BLOCKED_PERCENTAGE_THRESHOLD
                                      Deleted HIGH_BLOCKED_PERCENTAGE_THRESHOLD
                                      Deleted FSHIELD_FORMULA_COEFFICIENT_1
    20131205    0004    Dan Berkowitz Added BEAM_EDGE_TOP to index definitions 
******************************************************************************/

#include <dp_Consts.h>

#define BLOCK_RAD 3600 /* NormalRes: Max Azimuth angle (tenths of a degree) */

/* Indexes into the exclusion zones */

#define EXCL_AZM_ANGLE_START   0
#define EXCL_AZM_ANGLE_END     1
#define EXCL_SLANT_RANGE_START 2
#define EXCL_SLANT_RANGE_END   3
#define EXCL_ELEV_ANGLE        4

/* BEAM_CENTER_TOP is the index of the beam center top in the
 * melting layer moments. BEAM_EDGE_TOP is the index of the beam edge top
 * (i.e., absolute top of ML) in the melting layer moments.*/

#define BEAM_EDGE_TOP 0
#define BEAM_CENTER_TOP 1

/* DEA file prefixes for parameters not automatically fetched */

#define ML_DEA_FILE     "alg.mlda."
#define DPPREP_DEA_FILE "alg.dpprep."

/* Constant Definitions for  QPE - RATE Algorithm ------------- */

/* #define QPERATE_DEBUG FALSE */

/* Constant Definitions for Regression Testing                       *
 *                                                                   *
 * Our rates are calculated in mm/hr, and the most difference we can *
 * ever detect in our Accum_grid is 1/1000 in = .00254 mm, so set a  *
 * higher tolerance of 0.001 mm                                      */

/* #define REGRESSION_TESTING          FALSE */
/* #define COLLECT_STATS               FALSE */
#define PROTOTYPE_NODATA        -32768.0
#define MAX_ABS_DIFF                 0.001

/* Stats reasons (why a rate was/wasn't collected). The stats   *
 * marked blockage/exclusion won't be collected unless there is *
 * blockage or an exclusion zone.                               */

#define STATS_BI                      1
#define STATS_NE                      2
#define STATS_BLOCKED_USABILITY       3  /* blockage  */
#define STATS_BLOCKED_NO_Z            4  /* blockage  */
#define STATS_BLOCKED_NO_ZPRIME       5  /* blockage  */
#define STATS_BLOCKED_KDP_POS         6  /* blockage  */
#define STATS_BLOCKED_KDP_NEG         7  /* blockage  */
#define STATS_BLOCKED_ZPRIME          8  /* blockage  */
#define STATS_NO_Z                    9
#define STATS_RA                     10
#define STATS_HR                     11
#define STATS_BD                     12
#define STATS_RH_ABOVE_ML            13
#define STATS_RH_BELOW_ML            14
#define STATS_WS                     15
#define STATS_GR                     16
#define STATS_DS_ABOVE_ML            17
#define STATS_DS_BELOW_ML            18
#define STATS_IC_ABOVE_ML            19
#define STATS_IC_BELOW_ML            20
#define STATS_HC_UNMATCHED           21
#define STATS_NO_HC                  22
#define STATS_GC                     23
#define STATS_UK                     24
#define STATS_LOW_RHOHV              25
#define STATS_HIGH_KDP_BLOCK         26  /* blockage  */
#define STATS_IS_EXCLUDED            27  /* exclusion */

#endif
