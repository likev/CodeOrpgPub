/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:49 $
 * $Id: packet_6.h,v 1.6 2009/05/15 17:52:49 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_6.h */

#ifndef _PACKET_6_H_
#define _PACKET_6_H_

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
/* CVG 9.0 */
extern assoc_array_i *msg_type_list;



/* prototypes */
void display_packet_6(int packet, int offset);
void packet_6_skip(char *buffer,int *offset);

/* CVG 9.0 */
extern void non_geo_scale_and_center(float screen_width, float screen_height,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);
extern void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);

extern short read_half(char *buffer,int *offset);


extern float res_index_to_res(int res);

/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif



