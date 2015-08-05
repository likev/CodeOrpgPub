/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/06/21 21:30:11 $
 * $Id: rpgcs_latlon.h,v 1.9 2006/06/21 21:30:11 steves Exp $
 * $Revision: 1.9 $
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
#include <rpgcs_model_data.h>

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

/* The following functions are defined in rpgcs_latlon.c */
int RPGCS_latlon_to_xy( float lat, float lon, float *x, float *y );

int RPGCS_latlon_to_azran( float lat, float lon, float *rng, float *azm );

int RPGCS_xy_to_latlon( float x, float y, float *lat, float *lon );

int RPGCS_azran_to_latlon( float rng, float azm, float *lat, float *lon );

/* The following functions are defined in rpgcs_lambert.c */
#define MISSING_LATLON      		-999.0
#define RPGCS_LAMBERT_ERROR    		-1
#define RPGCS_DATA_POINT_NOT_IN_GRID	-2

int RPGCS_lambert_grid_point_xy( int i_ind, int j_ind, double *x,
                                 double *y );
int RPGCS_lambert_grid_point_latlon( int i_ind, int j_ind, double *lat,
                                     double *lon );
int RPGCS_lambert_grid_point_azran( int i_ind, int j_ind, double *azm,
                                    double *ran );
int RPGCS_lambert_latlon_to_grid_point( double lat, double lon, int *i_ind,
                                        int *j_ind );
int RPGCS_lambert_xy_to_grid_point( double x, double y, int *i_ind,
                                    int *j_ind );
int RPGCS_lambert_latlon_to_xy( double lat, double lon,
                                double *x, double *y );
int RPGCS_lambert_xy_to_latlon( double x, double y,
                                double *lat, double *lon );
void RPGCS_lambert_init( RPGCS_model_attr_t *model_attr );
int RPGCS_lambertuv_to_uv( double ur, double vr, double lon,
                           double *u, double *v );


#ifdef __cplusplus
}
#endif

#endif
