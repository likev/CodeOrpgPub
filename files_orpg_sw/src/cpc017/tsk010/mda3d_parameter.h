/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/02/24 18:17:47 $
 * $Id: mda3d_parameter.h,v 1.5 2006/02/24 18:17:47 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda3d_parameter.h

Description:    include all parameters used for mda 3-D 
************************************************************************/

#ifndef MDA3D_PARAMETER_H 
#define MDA3D_PARAMETER_H 

#include <basedata.h>

/* -------------------------------------------------------------------- */
/* Define MACROS for MAX and MIN */
#define MIN(x,y)  ( (x) < (y) ? (x) : (y) ) /* minimum of two values */
#define MAX(x,y)  ( (x) > (y) ? (x) : (y) ) /* minimum of two values */

#define MESO_MAX_LENGTH    400     /* max length of shear vector */ 

#define MESO_MAX_FEAT   500     /* max num of allowable 2D features */
#define MESO_NUM_ELEVS  22      /* number of the elevation levels */
#define MESO_MAX_NCPLT  500     /* Maximum number MDA couplets/mesos per volume scan*/ 
#define MESO_MAX_MAG_HT 12.0    /* Maximum height to compute mesocyclone magnitudes */
#define NX_MISSING_DATA -32768  /* Misssing data */
#define MESO_MAX_DEPTH  8.0     /* Maximum allowable top for mesocyclones (km) */
#define RE              6371.0 /* Radius of Earch in km */
#define IR              1.21   /* constant for height calculation */
#define DTR     0.017453        /* factor to convert degree to radian */
#define MESO_MAX_BASE  5.0      /* Maximum allowable base height for mesocyclones (km) */
#define MESO_MIN_DEPTH 3.0      /* Minimum allowable depth for mesocyclones (km) */
#define MESO_MIN_DEPTH_LT 3.0   /* Minumum absolute depth threshold for a low-topped mesocyclone */
#define MESO_MIN_BASE_LT  3.0   /* Minumum base threshold for a low-topped mesocyclone */
#define MESO_MIN_RELDEPTH 0.25  /* Minimum storm-relative depth threshold for classifying
                                 * low-topped mesocyclones. */
#define MESO_MIN_RANK_LT 5.0    /* Minumum rank threshold for a low-topped mesocyclone */
#define MESO_MIN_RANK_WEAK_LT 3.0/* Minumum rank threshold for a weak low-topped mesocyclone */
#define MDA_USER_CLASS 0        /* When .TRUE., mesosyclones are classified using USER-
                                 * specified thresholds (MESO_MIN_RANK_USER,
                                 * MESO_MAX_BASE_USER, MESO_MIN_DEPTH_USER) instead
                                 * of the "Burgess" Mesocyclone criteria. */
#define MESO_MIN_RANK_USER 5.0  /* User-defined minimum rank to classify as MESO */
#define MESO_MAX_BASE_USER 5.0  /* User-defined maximum base to classify as MESO */
#define MESO_MIN_DEPTH_USER 3.0  /* User-defined minimum depth to classify as MESO */
#define MESO_MIN_DISP_RANK 1.0
                          /* Minimum 3D strength rank of detections that will be
                           * displayed.  Ideally, this should be function tied
                           * directly to the display device, and not the
                           * algorithm.  Detections of ranks less than this
                           * value are still found, stored, and tracked. */
#define MESO_MIN_RANK	1	/* the min rank of shear vector */ 
#define MESO_MAX_RANK	25	/* the max rank of shear vector */ 
#endif


