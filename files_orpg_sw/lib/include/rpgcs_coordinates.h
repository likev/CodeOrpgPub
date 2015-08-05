#ifndef RPGCS_COORDINATES_H
#define RPGCS_COORDINATES_H

#include <orpg.h>

/* Angle conversions. */
#define PI     		3.14159265
#define TWOPI  		(2*PI)
#define RTD    		(180.0/PI)
#define DTR    		(PI/180.0)

#define M_TO_NMI    	(KM_TO_NM/1000.0)

/* Effective earth's radius, in meters. */
#define EA     		(1.21*6371000)

#define METERS       	1
#define KFEET        	2
#define NMILES       	3

#define DEG          	4
#define DEG10        	5

#ifdef FLOAT
   #define REAL float
   #define RPGCS_FLOAT
#endif

#ifdef DOUBLE
   #define REAL double
   #define RPGCS_DOUBLE
#endif

/* Define the function names */
#ifdef RPGCS_FLOAT

#define RPGCS_xy_to_azran RPGCS_xy_to_azran_f
#define RPGCS_xy_to_azran_u RPGCS_xy_to_azran_uf
#define RPGCS_azranelev_to_xy RPGCS_azranelev_to_xy_f
#define RPGCS_azranelev_to_xy_u RPGCS_azranelev_to_xy_uf
#define RPGCS_azran_to_xy RPGCS_azran_to_xy_f
#define RPGCS_azran_to_xy_u RPGCS_azran_to_xy_uf
#define RPGCS_height RPGCS_height_f
#define RPGCS_height_u RPGCS_height_uf
#define RPGCS_range RPGCS_range_f
#define RPGCS_range_u RPGCS_range_uf
#define RPGCS_set_input_units RPGCS_set_input_units_f
#define RPGCS_get_input_units RPGCS_get_input_units_f
#define RPGCS_set_output_units RPGCS_set_output_units_f
#define RPGCS_get_output_units RPGCS_get_output_units_f

#else
#ifdef RPGCS_DOUBLE

#define RPGCS_xy_to_azran RPGCS_xy_to_azran_d
#define RPGCS_xy_to_azran_u RPGCS_xy_to_azran_ud
#define RPGCS_azranelev_to_xy RPGCS_azranelev_to_xy_d
#define RPGCS_azranelev_to_xy_u RPGCS_azranelev_to_xy_ud
#define RPGCS_azran_to_xy RPGCS_azran_to_xy_d
#define RPGCS_azran_to_xy_u RPGCS_azran_to_xy_ud
#define RPGCS_height RPGCS_height_d
#define RPGCS_height_u RPGCS_height_ud
#define RPGCS_range RPGCS_range_d
#define RPGCS_range_u RPGCS_range_ud
#define RPGCS_set_input_units RPGCS_set_input_units_d
#define RPGCS_get_input_units RPGCS_get_input_units_d
#define RPGCS_set_output_units RPGCS_set_output_units_d
#define RPGCS_get_output_units RPGCS_get_output_units_d

#endif
#endif

int RPGCS_xy_to_azran( REAL x, REAL y, REAL *range, REAL *azm );
int RPGCS_azranelev_to_xy( REAL range, REAL azm, REAL elev,
                           REAL *x, REAL *y );
int RPGCS_azran_to_xy( REAL range, REAL azm, REAL *x, REAL *y );
int RPGCS_height( REAL range, REAL elev, REAL *height );
int RPGCS_range( REAL height, REAL elev, REAL *range );

int RPGCS_xy_to_azran_u( REAL x, REAL y, int xy_units, REAL *range, 
                         int range_units, REAL *azm );
int RPGCS_azranelev_to_xy_u( REAL range, int range_units, REAL azm, 
                             int azm_units, REAL elev, int elev_units,
                             REAL *x, REAL *y, int xy_units );
int RPGCS_azran_to_xy_u( REAL range, int range_units, REAL azm, 
                         int azm_units, REAL *x, REAL *y, int xy_units );
int RPGCS_height_u( REAL range, int range_units, REAL elev, int elev_units,
                    REAL *height, int height_units );
int RPGCS_range_u( REAL height, int height_units, REAL elev, int elev_units,
                    REAL *range, int range_units );

void RPGCS_set_input_units( int units );
int RPGCS_get_input_units( );
void RPGCS_set_output_units( int units );
int RPGCS_get_output_units( );

#endif
