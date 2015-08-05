/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/13 18:17:48 $
 * $Id: rpgcs_model_data.h,v 1.10 2012/09/13 18:17:48 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#ifndef RPGCS_MODEL_DATA_H
#define RPGCS_MODEL_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <rpgc.h>
#include <bzlib.h>
#include <zlib.h>
#include <orpg.h>
#include <orpg_product.h>
#include <legacy_prod.h>

/* Model types */
#define RUC_ANY_TYPE		0
#define RUC40 			1
#define RUC13 			2
#define RUC80 			3
#define STR_RUC80 		"RUC 80km"
#define STR_RUC40 		"RUC 40km"
#define STR_RUC13 		"RUC 13km"

/* Model parameters. */
#define RPGCS_MODEL_RUN_DATE	"Model Run Date"
#define RPGCS_MODEL_RUN_TIME	"Model Run Time"
#define RPGCS_VALID_DATE  	"Valid Date"
#define RPGCS_VALID_TIME  	"Valid Time"
#define RPGCS_FORECAST_HOUR	"Forecast Hour"
#define RPGCS_LATITUDE_LLC	"Latitude Lower Left Corner"
#define RPGCS_LONGITUDE_LLC	"Longitude Lower Left Corner"
#define RPGCS_LATITUDE_URC	"Latitude Upper Right Corner"
#define RPGCS_LONGITUDE_URC	"Longitude Upper Right Corner"
#define RPGCS_PROJECTION	"Projection"
#define RPGCS_LATITUDE_TANP	"Latitude of Tangent point"
#define RPGCS_LONGITUDE_TANP	"Longitude of Tangent point"
#define RPGCS_X_DIMENSION	"Number of points in X direction"
#define RPGCS_Y_DIMENSION	"Number of points in Y direction"

/* Fields for models. */
#define RPGCS_MODEL_TEMP    	"Temperature"
#define RPGCS_MODEL_UCOMP      	"U Wind Component"
#define RPGCS_MODEL_VCOMP      	"V Wind Component"
#define RPGCS_MODEL_RH    	"Relative Humidity"
#define RPGCS_MODEL_GH  	"Geopotential Height"
#define RPGCS_MODEL_SFCP 	"Pressure"

/* Fields derived from model. */
#define FREEZING_GRID           "Freezing Grid"
#define FRZ_GRID_RANGE          "Grid Point Range"
#define FRZ_GRID_AZIMUTH        "Grid Point Azimuth"
#define FRZ_GRID_NUM_ZERO_X     "Zero Crosspoint Count"
#define FRZ_GRID_HEIGHT_ZERO    "Height Zero"
#define FRZ_GRID_HEIGHT_ZDR_T1  "Height ZDR Temp 1"
#define FRZ_GRID_HEIGHT_ZDR_T2  "Height ZDR Temp 2"
#define FRZ_GRID_HEIGHT_MINUS20 "Height Minus Twenty"
#define FRZ_GRID_MAX_TEMP_WL    "Max Temp Warm Layer"
#define FRZ_GRID_MIN_TEMP_CL    "Min Temp Cold Layer"
#define FRZ_GRID_SPARE          "Spare"
#define CIP_GRID_PROD           "CIP Product"

typedef struct LatLon {

   double latitude;		/* in degrees */
   double longitude;		/* in degrees */

} LatLon_t;

/* Model data is assumed gridded data using Lambert Conformal projection.  
   The following data structure defines attributes of the grid.  The attributes
   can be used to interpret/transform grid coordinates.  This data structure
   is assumed to be generic enough to handle most projections. */

/* Different projections. */
#define RPGCS_UNKNOWN_PROJECTION 0
#define RPGCS_LAMBERT_CONFORMAL	 1


typedef struct RPGCS_model_attr {

   int model;                   /* Model type (e.g. RUC13, RUC40, etc) */

   int projection;		/* Model projection. */

   time_t model_run_time;	/* Model run time (UTC). */

   time_t valid_time;		/* Time model valid (UTC). */

   time_t forecast_period;	/* Period the model is valid (UTC). */

   LatLon_t grid_lower_left;	/* Lat/Lon of grid lower left. */

   LatLon_t grid_upper_right;	/* Lat/Lon of grid upper right. */

   LatLon_t tangent_point;	/* Grid tangent point .... used for coordinate
				   transformation. */

   int num_dimensions; 		/* Assumes no more than 4 dimensional grid. */

   int dimensions[4];		/* Number of data points in dimension. */

} RPGCS_model_attr_t;

/* Component Parameters. */
#define RPGCS_MODEL_LEVEL  		"Level"
#define RPGCS_PRESSURE_LEVEL		"Pressure level"
#define RPGCS_PRESSURE_LEVEL_UNITS	"mb"

/* Possible values for Component Parameter "level_type". */
#define RPGCS_UNDEFINED_LEVEL_TYPE	0
#define RPGCS_CONST_PRESSURE_LEVEL	1


/* Possible values for units.   Note:  This list is no all inclusive.   Units
   will be added as they are standardized. */

#define RPGCS_UNKNOWN_UNITS		0    

/* Length units. */
#define RPGCS_METER_UNITS       	1
#define RPGCS_METER_STR       		"m"
#define RPGCS_KILOMETER_UNITS       	2
#define RPGCS_KILOMETER_STR       	"km"


/* Velocity units. */
#define RPGCS_MPS_UNITS        		10
#define RPGCS_MPS_STR        		"m/s"

/* Temperature units. */
#define RPGCS_KELVIN_UNITS     		20
#define RPGCS_KELVIN_STR     		"K"
#define RPGCS_CELSIUS_UNITS    		21
#define RPGCS_CELSIUS_STR    		"C"
#define RPGCS_FAHRENHEIT_UNITS 		22
#define RPGCS_FAHRENHEIT_STR 		"F"

/* Pressure units. */
#define RPGCS_MILLIBAR_UNITS		30   
#define RPGCS_MILLIBAR_STR		"mb" 
#define RPGCS_PASCAL_UNITS		31   
#define RPGCS_PASCAL_STR		"Pa" 

/* Relative Humidity units. */
#define RPGCS_PERCENT_UNITS		40   
#define RPGCS_PERCENT_STR		"%" 

/* Conversions.  Other conversions are defined in a309.h. */
#define KELVIN_TO_C            		-273.15

/* Model grid parameters.   */
typedef struct RPGCS_model_grid_params {

   int level_type;		/* Level field type. */

   double level_value;		/* Level value for this field type. */

   int level_units;		/* Units of measure for level value. */

} RPGCS_model_grid_params_t;

#define MAXIMUM_LEVELS		100

/* RPGCS model field data.   By convention, dimension index 0 is always x
   dimension index 1 is always y, dimension index 2 is always z for
   Cartesian grid. */
#define MODEL_BAD_VALUE         -99999999.0
typedef struct RPGCS_model_grid_data {

   char *field;

   int num_levels;
 
   RPGCS_model_grid_params_t *params[ MAXIMUM_LEVELS ];

   int num_dimensions;

   int *dimensions;

   int units;

   double *data[ MAXIMUM_LEVELS ];

} RPGCS_model_grid_data_t;


/* Function prototypes. */
int RPGCS_get_model_data( int prod_id, int model, char **data );
void* RPGCS_get_model_attrs( int model, char *data );
void* RPGCS_get_model_field( int model, char *data, char *field );
double RPGCS_get_data_value( RPGCS_model_grid_data_t *data,
                             int level, int i_ind, int j_ind, 
                             int *units );
void RPGCS_free_model_field( int model, char *data );

#ifdef __cplusplus
}
#endif

#endif
