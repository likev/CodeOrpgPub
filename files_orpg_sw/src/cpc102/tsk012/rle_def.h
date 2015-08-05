/*
 * RCS info
 * $Author: eddief $
 * $Locker:  $
 * $Date: 2002/08/07 21:30:17 $
 * $Id: rle_def.h,v 1.31 2002/08/07 21:30:17 eddief Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>

#include <rle.h>


/* Flag value for slow moving storms */
#define slow_mover                -999
#define max_level_length      7
#define max_data_levels      16

/* Define Pointer to radial and raster data structures */
extern struct radial_rle *radial_image;
extern struct raster_rle *raster_image;
extern struct dhr_rle *dhr_image;

/* Define Pointer to VWP data structure */
extern struct vwp *vwp_image;

/* Define Pointer to STI data structure */
extern struct sti *sti_image;

/* Define Pointer to HI data structure */
extern struct hi *hi_image;

/* Define Pointer to MESO data structure */
extern struct meso *meso_image;

/* Define Pointer to TVS data structure */
extern struct tvs *tvs_image;

/* Define Pointer to SWP data structure */
extern struct swp *swp_image;

/* Define pointer to product_pertinent data structure */
extern struct product_pertinent* attribute;   

/* Define pointer to graphic attribute data structure */
extern struct graphic_attr *gtab;

/* Define pointer to tabular alphanumeric data structure */
extern struct tabular_attr *ttab;



/* Function Prototypes */
int decode_product( short *product_data );
int polar_slice( char *filename );
int read_concurrent_product_file( char *filename, short **product_data );
int decode_header( short *product_data );
int decode_radial_rle( short *product_data );
int decode_raster_rle( short *product_data );
void decode_data_level( short level, char *decoded_level );
void level_to_ASCII( float scale_factor, unsigned char value, char *string);
void convert_time( int timevalue, int *hours, int *minutes, int *seconds );
void calendar_date( short date, int *day, int *month, int *year );
void free_memory( short product_code );
void decode_grafattr_block( short *product_data );
void decode_tabular_block( short *product_data,
                           unsigned short product_type );
void decode_text_packet_8( short *product_data,
                           char **packet_text,
                           int *line_number,
                           int *col_number );
void product_dependent( short *product_data );
void degrees_minutes_seconds( float latlong,
                              int *deg,
                              int *min,
                              int *sec );
void decode_vwp( short * product_data, short product_offset, int layer1_size );
void decode_sti( short * product_data, short product_offset, int layer1_size );
void decode_hi( short * product_data, short product_offset, int layer1_size );
void decode_meso( short * product_data, short product_offset, int layer1_size );
void decode_tvs( short * product_data, short product_offset, int layer1_size );
void decode_swp( short * product_data, short product_offset, int layer1_size );
void decode_dhr( short * product_data );
