/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:22:22 $
 * $Id: packet_8_cvg.h,v 1.1 2009/05/15 17:22:22 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* packet_8_cvg.h */

#ifndef _DECODE_PACKET_8_H_
#define _DECODE_PACKET_8_H_

#include <stdio.h>
#include <Xm/Xm.h>
#include <prod_gen_msg.h>
#include "global.h"

#define PUP_WIDTH  512
#define PUP_HEIGHT 512

#define RESCALE_FACTOR (0.25)  /* 1/4 km */

extern  Display        *display;
extern  GC      gc;
extern  XColor          display_colors[];
extern  Dimension   pwidth, pheight;
extern  int             printing_gab;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;

extern assoc_array_i *msg_type_list;
extern assoc_array_i *product_res;


extern Boolean norm_format1, norm_format2, bkgd_format1, bkgd_format2;
extern Boolean norm_format3, bkgd_format3;

/* prototypes */
void packet_8_display(int packet,int offset);
void packet_8_skip(char *buffer,int *offset);

/* CVG 9.0 */
extern void non_geo_scale_and_center(float screen_width, float screen_height,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);
/* CVG 9.0 - added for GEOGRAPHIC_PRODUCT support */
extern void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);

extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);


extern float res_index_to_res(int res);

/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif



