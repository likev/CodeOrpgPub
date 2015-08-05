/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/08 16:27:48 $
 * $Id: rpgcs_latlon_funcs.h,v 1.1 2004/01/08 16:27:48 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef RPGCS_LATLON_FUNCS_H
#define RPGCS_LATLON_FUNCS_H

#include <infr.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <orpgsite.h>
#include <math.h>
#include <a309.h>

#define FLOAT
#include <rpgcs_coordinates.h>

/* Define macros used in this function. */
#define ZDEG2RAD          0.0174533
#define ZCNV2DEG          0.001
#define ZCNV2THNTHS      1000.0 
#define ZCONST1           135.0
#define ZCONST2          6380.0
#define ZCONST3             1.0


/* Function prototypes. */
int RPGCS_latlon_to_xy( float lat, float lon, float *x, float *y );

int RPGCS_latlon_to_azran( float lat, float lon, float *azm, float *rng );

int RPGCS_xy_to_latlon( float x, float y, float *lat, float *lon );

int RPGCS_azran_to_latlon( float azm, float rng, float *lat, float *lon );


#ifdef __cplusplus
}
#endif

#endif
