#ifndef DATA_CONVERSION_H
#define DATA_CONVERSION_H

#include <basedata.h>
#include <generic_basedata.h>
#include <stdlib.h>

/* Macro definitions. */
#define BELOW_THRESHOLD     -999.0f
#define RANGE_FOLDED        -888.0f
#define INVALID_DATA        -777.0f

#define DATA_OUT_OF_RANGE        -1
#define MIN_VALUE                 0
#define MIN_DATA_VALUE            2
#define MAX_DATA_VALUE          255
#define MAX_VALUE               255

#define Z_MIN_VALUE          -32.0f
#define Z_MAX_VALUE           94.5f

#define W_MIN_VALUE            0.0f
#define W_MAX_VALUE           10.0f

/* Enumerations. */
typedef enum { HIGH_RES, LOW_RES } RESO;

/* Function Prototypes */
RESO RPGCS_get_velocity_reso( int reso );
RESO RPGCS_set_velocity_reso( int reso );

float RPGCS_reflectivity_to_dBZ( int value );
int RPGCS_dBZ_to_reflectivity( float value );

float RPGCS_velocity_to_ms( int value );
int RPGCS_ms_to_velocity( RESO reso, float value );

float RPGCS_spectrum_width_to_ms( int value );
int RPGCS_ms_to_spectrum_width( float value );

int RPGCS_radar_data_conversion( void* data, Generic_moment_t *data_block,
                                 float below_threshold, float range_folded,
                                 float **converted_data );

#endif
