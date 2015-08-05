/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:44 $
 * $Id: packet_2.h,v 1.6 2009/05/15 17:52:44 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* packet_2.h */

#ifndef _PACKET_2_H_
#define _PACKET_2_H_

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

/* added black_color for undifined symbol */
extern  Pixel           white_color, black_color;

/* Index into symbol_pkt2.plt           */
#define P2_BLACK  0
#define P2_WHITE  1
#define P2_YELLOW 2
#define P2_RED    3
#define P2_GREEN  4
#define P2_BLUE   5
#define P2_ORANGE 6
#define P2_GRAY   7

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;

extern assoc_array_i *product_res;

/* cvg 9.0 - added for NON_GEOGRAPHIC_PRODUCT support */
extern assoc_array_i *msg_type_list;

/* prototypes */
void packet_2_skip(char *buffer,int *offset);
void packet_2_display(int packet,int offset);

extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);

extern float res_index_to_res(int res);


/* CVG 9.0 - added new function supporting GEOGRAPHIC_PRODUCT */
extern void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);

/* CVG 9.0 added to provide support for non-geographic products */
extern void non_geo_scale_and_center(float screen_width, float screen_height,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);


/* Little Endian byte swap */
extern void MISC_short_swap( void *buf, int size);

#endif



