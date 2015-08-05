/* packet_20.h */

#ifndef _DECODE_PACKET_20_H_
#define _DECODE_PACKET_20_H_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "global.h"


#define	DEGRAD	(3.14159265/180.0)
/* #define RESCALE_FACTOR (460.0/2048.0) */
/* the RESCALE FACTOR is derived from the ICD */
#define RESCALE_FACTOR (0.25)  /* 1/4 km */

/* Point Feature offset definitions */
#define point_feature_msg_len_offset 1
#define point_feature_header_offset  2
#define point_feature_data_offset    4

/* Point Feature data offset definitions */
#define point_feature_xpos_offset       0
#define point_feature_ypos_offset       1
#define point_feature_type_offset       2
#define point_feature_attribute_offset  3

/* Index into symbol_pkt20.plt           */
#define P20_BLACK  0
#define P20_WHITE  1
#define P20_YELLOW 2
#define P20_RED    3
#define P20_GREEN  4
#define P20_BLUE   5
#define P20_ORANGE 6
#define P20_GRAY   7


extern	Display		*display;
extern	GC		gc;
extern	XColor 	        display_colors[];
extern	Dimension	width,pwidth,pheight;

/* added black_color for undifined symbol */
extern  Pixel           white_color, black_color;

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;

extern assoc_array_i *product_res;

/* prototypes */
void packet_20_skip(char *buffer,int *offset);
void display_packet_20(int packet, int offset);

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
