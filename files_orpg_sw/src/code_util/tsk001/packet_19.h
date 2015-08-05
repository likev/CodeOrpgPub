/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:43 $
 * $Id: packet_19.h,v 1.6 2009/05/15 17:52:43 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_19.h */

#ifndef _DECODE_PACKET_19_H_
#define _DECODE_PACKET_19_H_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "global.h"


#define	DEGRAD	(3.14159265/180.0)
/* #define RESCALE_FACTOR (460.0/2048.0) */
#define RESCALE_FACTOR (0.25)  /* 1/4 km */

/* Hail offset definitions */
#define hail_msg_len_offset 1
#define hail_header_offset  2
#define hail_data_offset    5

/* Hail data offset definitions */
#define hail_xpos_offset     0
#define hail_ypos_offset     1
#define hail_prob_offset     2
#define hail_svr_prob_offset 3
#define hail_size_offset     4


extern	Display		*display;
extern	GC		gc;
extern	XColor 	        display_colors[];
extern	Dimension	width, pwidth, pheight;
extern  Pixel white_color;

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;

extern assoc_array_i *product_res;

/* these probabilities are supposed to be user-modifiable, but we can handle
   that later */
int svr_hail_thrsh_2 = 50;
int svr_hail_thrsh_1 = 30;
int hail_thrsh_2 = 50;
int hail_thrsh_1 = 30;

/* prototypes */
void packet_19_skip(char *buffer,int *offset);
void display_hail_data(int packet, int offset);

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



