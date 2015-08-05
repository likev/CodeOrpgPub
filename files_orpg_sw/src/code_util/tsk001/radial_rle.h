/* radial_rle.h */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:55 $
 * $Id: radial_rle.h,v 1.8 2009/05/15 17:52:55 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

#ifndef _DECODE_16_LEVEL_H_
#define _DECODE_16_LEVEL_H_

#include <Xm/MessageB.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "global.h"

#define	DEGRAD	(3.14159265/180.0)

/* Radial rle data packet header offset defintions */
#define number_of_bins_offset       2
#define range_interval_offset       5
#define number_of_radials_offset    6
#define radial_rle_offset           7

/* Radial rle packet data offset definitions */
#define azimuth_angle_offset         1
#define azimuth_delta_offset         2
#define radial_data_array_offset     3

/* Symbology Block Offset defintions */
#define block1_size_offset      2
#define num_layers_offset       4
#define layer1_size_offset      6
#define image_type_offset       8

extern Dimension barwidth;

extern	Display	*display;
extern  Window window;
extern	GC	gc;
extern	XColor	display_colors[];
extern  int     palette_size;
extern  Dimension pwidth, pheight;
extern  float pixel_int;
extern  float scanl_int;
extern screen_data *sd;
/* extern screen_data *sd1, *sd2, *sd3; */
extern int verbose_flag;
extern assoc_array_i *digital_legend_flag;

extern Widget shell;

/* prototypes */
void packet_AF1F_skip(char *buffer,int *offset);
void decode_radial_rle(short *product_data, int offset);
void output_radial_rle();

void display_radial_rle(int packet, int offset, int replay);

void delete_radial_rle();

extern short read_half(char *buffer,int *offset);


/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif
