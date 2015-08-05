/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/19 19:55:58 $
 * $Id: mda1d_parameter.h,v 1.5 2009/03/19 19:55:58 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda1d_paramer.h

Description:    include all parameres used for mda 1-D 
************************************************************************/

#ifndef MDA1D_PARAMETER_H 
#define MDA1D_PARAMETER_H 

#include <basedata.h>

/* -------------------------------------------------------------------- */
#define MESO_MAX_LENGTH    BASEDATA_MAX_SR_RADIALS  /* max length of shear vector */ 
#define DOP_BIN_SIZE       0.25 /* Bin size in kilometers */ 
#define MESO_MIN_RANK	   1    /* the min rank of shear vector */ 
#define MESO_MAX_RANK      25   /* the max rank of shear vector */ 
#define MDA_LOOK_AHD	   1    /* 1: turn on the look-ahead mode */
#define DEGREE_TO_RADIAN   3.1416/180.0 /* factor to convert degree to radian */
#define MESO_MAX_VECT      10000 /* max number of vectors per tilts */
#define CONV_MAX_VECT	   2000 /* max number of vectors per tilts */
#define CONV_DELTV_TH	   5.0  /* threshold of velocity difference */
#define CONV_SHR_TH        1.0  /* threshold of convergence shear */
#define CONV_MAX_HGT_TH	   8.0  /* Maximum height to process velocity data. Uses this
                                 * one value to compute CONV_MAX_RNG_TH for each
                                 * elevation angle */
#define RE                 6371.0 /* Radius of Earch in km */
#define IR                 1.21 /* constant for height calculation */
#define MESO_AZOVLP_THR    355  /* threshold of overlaped checking */
#define MESO_AZOVLP_THR_SR 710  /* threshold of overlaped checking */
#define HEM	           1    /* 1: Northern hemispher; 0: Southern Hemisphere */
#endif


