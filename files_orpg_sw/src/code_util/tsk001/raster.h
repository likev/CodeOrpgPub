/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:55 $
 * $Id: raster.h,v 1.7 2009/05/15 17:52:55 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* raster.h */

#ifndef _DECODE_RASTER_H_
#define _DECODE_RASTER_H_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <prod_gen_msg.h>
#include "global.h"

#define DEGRAD  (3.14159265/180.0)

#define PUP_WIDTH  512
#define PUP_HEIGHT 512

/* Raster data packet header offset definitions */
#define i_start_offset              3
#define j_start_offset              4
#define x_scale_offset              5
#define y_scale_offset              7
#define number_of_rows_offset       9
#define raster_rle_offset           11

/* Raster packet data offset definitions */
#define bytes_in_row_offset          1
#define raster_rle_header_offset     1

/* Symbology Block Offset defintions */
#define block1_size_offset      2
#define num_layers_offset       4
#define layer1_size_offset      6
#define image_type_offset       8

extern assoc_array_i *msg_type_list;

extern  Display     *display;
extern  GC      gc;
extern  XColor          display_colors[];
extern  Dimension   pwidth, pheight, barwidth;
extern  float pixel_int;
extern  float scanl_int;
extern screen_data *sd;
/* extern screen_data *sd1, *sd2, *sd3; */
extern int verbose_flag;


/* prototypes */
void packet_BA07_skip(char *buffer,int *offset);
void decode_raster(short *product_data, int offset);

void display_raster(int packet, int offset, int replay);
void output_raster();

void delete_raster();

/* CVG 9.0 */
extern void non_geo_scale_and_center(float screen_width, float screen_height,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);


extern short read_half(char *buffer,int *offset);


/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif




