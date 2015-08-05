/* packet_1_cvg.h */
/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 15:11:42 $
 * $Id: packet_1_cvg.h,v 1.2 2009/08/19 15:11:42 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef _DECODE_PACKET_1_H_
#define _DECODE_PACKET_1_H_

#include <stdio.h>

#include <stdlib.h>

#include <Xm/Xm.h>
#include "global.h"



#define FALSE 0
#define TRUE  1

#define PUP_WIDTH  512
#define PUP_HEIGHT 512

/* CVG 9.0 - added for geo_product support */
#define RESCALE_FACTOR (0.25)  /* 1/4 km */

extern  Display        *display;
extern  GC      gc;
extern  XColor          display_colors[];
extern  Dimension   pwidth, pheight;

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;

/* CVG 9.0 - added for geo_product support */
extern assoc_array_i *product_res;
extern assoc_array_i *msg_type_list;

/* CVG 9.1 - added for coord override for geographic products */
extern assoc_array_i *packet_1_geo_coord_flag;

/* prototypes */
void packet_1_skip(char *buffer,int *offset);
void display_packet_1(int packet, int offset);

/* CVG 9.0 */
extern void non_geo_scale_and_center(float screen_width, float screen_height,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);
/* CVG 9.0 - added for geo_product support */
extern void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);
/* CVG 9.0 - added for geo_product support */
extern float res_index_to_res(int res);

extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer, int *offset);


/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif



