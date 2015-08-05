/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/19 19:56:48 $
 * $Id: mda2d_parameter.h,v 1.5 2009/03/19 19:56:48 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda2d_paramer.h
Author: 	Yukuan Song
Created:	Oct. 2002 

Description:    include all parameres used for mda 2-D 
************************************************************************/

#ifndef MDA2D_PARAMETER_H 
#define MDA2D_PARAMETER_H 

#include <basedata.h>

/* -------------------------------------------------------------------- */
#define MESO_MIN_RANK	1	/* the min rank of shear vector */ 
#define MESO_MAX_RANK	25	/* the max rank of shear vector */ 
#define MAX_RANK_RANGE	30	/* Range of the rank*/ 
#define MESO_MAX_FEAT   500     /* max num of allowable 2D features */
#define MESO_MAX_NSV	200     /* Maximum no. MDA shear vectors allowed per 2D feature */
#define YES_DO_SMOOTH   1       /* 1: do 3-point smooth, otherwise do 1-point smooth */
#define RE              6371.0  /* Radius of Earch in km */
#define IR	        1.21    /* constant for height calculation */
#define MESO_MAX_FHT	12.0    /* maximum allowable height of features */
#define MESO_MAX_VECT	10000    /* max number of vectors per tilts */

#endif

