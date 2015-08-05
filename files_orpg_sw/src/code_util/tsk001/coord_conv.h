/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:22 $
 * $Id: coord_conv.h,v 1.2 2008/03/13 22:45:22 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


#ifndef	_88D_COORD_CONV_H
#define	_88D_COORD_CONV_H

#include "global.h"


#define RADAR_POS_OFFSET (96+20)

extern screen_data *sd;
/* extern screen_data *sd1, *sd2, *sd3; */

extern int read_word(char *buffer,int *offset);


/* /////////////////////////////////////// */
/*  from a309.h */
#ifndef CFT_TO_KM
#define CFT_TO_KM  0.000304799 /*  0.0003048 in a309.h */
#endif

#ifndef CFT_TO_M
#define CFT_TO_M   0.3047999
#endif

#ifndef CKM_TO_FT
#define CKM_TO_FT  3280.85 /*  3280.84 in a309.h */
#endif

#ifndef CKM_TO_NM
#define CKM_TO_NM  0.53996
#endif

#ifndef CM_TO_FT
#define CM_TO_FT  3.28085  /*  3.28084 in a309.h */
#endif

#ifndef CNM_TO_KM
#define CNM_TO_KM  1.85198  /*  1.852 in a309.h */
#endif

#ifndef CNM_TO_M
#define CNM_TO_M   1851.98  /*  1852.0 in a309.h */
#endif
/* ////////////////////////////////////// */


/* //#include <orpg.h>  // WHY? */

/* Angle conversions. */
#define PI     3.14159265
#define TWOPI  (2*PI)
#define RTD    (180.0/PI)
#define DTR    (PI/180.0)

/* #define M_TO_NMI    (KM_TO_NM/1000.0) */
#define M_TO_NMI    (CKM_TO_NM/1000.0)

/* Effective earth's radius, in meters. */
/* used by get height */
/* this calculates to 15,417.82 KM, not very good! */
#define EA     (1.21*6371000)

#define METERS       1
/* //#define KFEET        2 */
/* //#define NMILES       3 */

/* //#define DEG          4 */
/* //#define DEG10        5 */

/* //#ifdef FLOAT */
/* //   #define REAL float */
/* //   #define CVG_FLOAT */
/* //#endif */
/* // */
/* //#ifdef DOUBLE */
/* //   #define REAL double */
/* //   #define CVG_DOUBLE */
/* //#endif */
/* // */
/* //*/ /* Define the function names */
/* //#ifdef CVG_FLOAT */
/* // */
/* //#define _88D_xy_to_azran _88D_xy_to_azran_f */
/* //////#define _88D_xy_to_azran_u _88D_xy_to_azran_uf */
/* //#define _88D_azranelev_to_xy _88D_azranelev_to_xy_f */
/* //////#define _88D_azranelev_to_xy_u _88D_azranelev_to_xy_uf */
/* //#define _88D_azran_to_xy _88D_azran_to_xy_f */
/* //////#define _88D_azran_to_xy_u _88D_azran_to_xy_uf */
/* //#define _88D_get_height _88D_get_height_f */
/* //////#define _88D_get_height_u _88D_get_height_uf */
/* //////#define _88D_set_input_units _88D_set_input_units_f */
/* //////#define _88D_get_input_units _88D_get_input_units_f */
/* //////#define _88D_set_output_units _88D_set_output_units_f */
/* //////#define _88D_get_output_units _88D_get_output_units_f */
/* // */
/* //#else */
/* //#ifdef CVG_DOUBLE */
/* // */
/* //#define _88D_xy_to_azran _88D_xy_to_azran_d */
/* //////#define _88D_xy_to_azran_u _88D_xy_to_azran_ud */
/* //#define _88D_azranelev_to_xy _88D_azranelev_to_xy_d */
/* //////#define _88D_azranelev_to_xy_u _88D_azranelev_to_xy_ud */
/* //#define _88D_azran_to_xy _88D_azran_to_xy_d */
/* //////#define _88D_azran_to_xy_u _88D_azran_to_xy_ud */
/* //#define _88D_get_height _88D_get_height_d */
/* //////#define _88D_get_height_u _88D_get_height_ud */
/* //////#define _88D_set_input_units _88D_set_input_units_d */
/* //////#define _88D_get_input_units _88D_get_input_units_d */
/* //////#define _88D_set_output_units _88D_set_output_units_d */
/* //////#define _88D_get_output_units _88D_get_output_units_d */
/* // */
/* //#endif */
/* //#endif */

/* int _88D_xy_to_azran( REAL x, REAL y, REAL *range, REAL *azm ); */
int _88D_xy_to_azran( float x, float y, float *range, float *azm );
/* int _88D_azranelev_to_xy( REAL range, REAL azm, REAL elev, */
/*                            REAL *x, REAL *y ); */
int _88D_azranelev_to_xy( float range, float azm, float elev,
                           float *x, float *y );                           
/* int _88D_azran_to_xy( REAL range, REAL azm, REAL *x, REAL *y ); */
int _88D_azran_to_xy( float range, float azm, float *x, float *y );
/* int _88D_get_height( REAL range, REAL elev, REAL *height ); */
int _88D_get_height( float range, float elev, float *height );

/* // NOT USED BY CVG */
/* //int _88D_xy_to_azran_u( REAL x, REAL y, int xy_units, REAL *range,  */
/* //                         int range_units, REAL *azm ); */
/* //int _88D_azranelev_to_xy_u( REAL range, int range_units, REAL azm,  */
/* //                             int azm_units, REAL elev, int elev_units, */
/* //                             REAL *x, REAL *y, int xy_units ); */
/* //int _88D_azran_to_xy_u( REAL range, int range_units, REAL azm,  */
/* //                         int azm_units, REAL *x, REAL *y, int xy_units ); */
/* //int _88D_get_height_u( REAL range, int range_units, REAL elev, int elev_units, */
/* //                    REAL *height, int height_units ); */

/* // NOT USED BY CVG */
/* //void _88D_set_input_units( int units ); */
/* //int _88D_get_input_units( ); */
/* //void _88D_set_output_units( int units ); */
/* //int _88D_get_output_units( ); */




/* ///////////////////////////////////////////////////////////////////// */
/* //////////////////////////////////////////////////////////////////// */
/* #include <orpgsite.h> */
#include <math.h>
/* #include <a309.h>  // MISC CONV FACTORS */


/* Define macros used in this function. */
#define ZDEG2RAD          0.0174533
#define ZCNV2DEG          0.001
#define ZCNV2THNTHS      1000.0 
#define ZCONST1           135.0
#define ZCONST2          6380.0
#define ZCONST3             1.0


/* Function prototypes. */
int _88D_latlon_to_xy( float lat, float lon, float *x, float *y );

int _88D_latlon_to_azran( float lat, float lon, float *rng, float *azm );

int _88D_xy_to_latlon( float x, float y, float *lat, float *lon );

int _88D_azran_to_latlon( float rng, float azm, float *lat, float *lon );




/* ///////////////////////////////////////////////////////////////////// */
/* //////////////////////////////////////////////////////////////////// */
/*  CVG TEST FUNCTION */
void test_coord_conv();


#endif	/* _88D_COORD_CONV_H */

