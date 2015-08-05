/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:45 $
 * $Id: packet_25.h,v 1.6 2009/05/15 17:52:45 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_25.h */

#ifndef _PACKET_25_H_
#define _PACKET_25_H_

#include <stdio.h>
#include <Xm/Xm.h>
#include "global.h"

#define PUP_WIDTH  512
#define PUP_HEIGHT 512
/* #define RESCALE_FACTOR (460.0/2048.0) */
#define RESCALE_FACTOR (0.25)  /* 1/4 km */


extern	Display	       *display;
extern	GC		gc;
extern	XColor 	        display_colors[];
extern	Dimension	pwidth, pheight;

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;
extern assoc_array_i *product_res;

/* prototypes */
void packet_25_skip(char *buffer,int *offset);
void display_packet_25(int packet, int offset);

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



