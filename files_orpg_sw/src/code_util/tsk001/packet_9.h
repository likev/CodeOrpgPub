/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:50 $
 * $Id: packet_9.h,v 1.5 2009/05/15 17:52:50 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* packet_9.h */

#ifndef _PACKET_9_H_
#define _PACKET_9_H_

#include <stdio.h>
#include <Xm/Xm.h>
#include "global.h"

#define PUP_WIDTH  512
#define PUP_HEIGHT 512
/* #define RESCALE_FACTOR (460.0/2048.0) */
#define RESCALE_FACTOR (0.25)  /* 1/4 km */

/* cvg 9.0 - added for GEOGRAPHIC_PRODUCT support */
extern assoc_array_i *msg_type_list;
extern assoc_array_i *product_res;

extern  Display        *display;
extern  GC      gc;
extern  XColor          display_colors[];
extern  Dimension   pwidth, pheight;

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;

/* prototypes */
void display_packet_9(int packet,int offset);
void packet_9_skip(char *buffer,int *offset);

/* CVG 9.0 */
extern void non_geo_scale_and_center(float screen_width, float screen_height,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);

/* CVG 9.0 - added for GEOGRAPHIC_PRODUCT support */
extern float res_index_to_res(int res);
extern void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);



extern short read_half(char *buffer,int *offset);


/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif



