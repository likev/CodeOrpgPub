/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:22:21 $
 * $Id: packet_15_cvg.h,v 1.1 2009/05/15 17:22:21 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* packet_15_cvg.h */

#ifndef _DECODE_PACKET_15_H_
#define _DECODE_PACKET_15_H_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "global.h"


#define DEGRAD  (3.14159265/180.0)
/* #define RESCALE_FACTOR (460.0/2048.0) old PUP method */
#define RESCALE_FACTOR (0.25)  /* 1/4 km */
#define PUP_WIDTH  512
#define PUP_HEIGHT 512

/* Storm ID offset definitions */
#define storm_id_msg_len_offset 1
#define storm_id_header_offset  2
#define storm_id_data_offset    3

/* Storm ID data offset definitions */
#define storm_id_xpos_offset    0
#define storm_id_ypos_offset    1
#define storm_id_str_offset     2

extern Dimension pwidth, pheight;
extern  Display     *display;
extern  GC      gc;
extern  XColor          display_colors[];
extern  Dimension   width;
extern  Pixel white_color;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;

extern Boolean norm_format1, norm_format2, bkgd_format1, bkgd_format2;
extern Boolean norm_format3, bkgd_format3;

extern assoc_array_i *product_res;

/* prototypes */
void packet_15_skip(char *buffer,int *offset);
void display_storm_id_data(int packet, int offset);

extern short read_half(char *buffer,int *offset);

extern float res_index_to_res(int res);

/* CVG 9.0 - added new function supporting GEOGRAPHIC_PRODUCT */
extern void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);


/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif





