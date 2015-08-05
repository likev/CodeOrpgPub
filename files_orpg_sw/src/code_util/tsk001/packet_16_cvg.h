/* packet_16_cvg.h */
/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:22:21 $
 * $Id: packet_16_cvg.h,v 1.1 2009/05/15 17:22:21 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
 
#ifndef _DECODE_PACKET_16_H_
#define _DECODE_PACKET_16_H_


#include <Xm/MessageB.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "global.h"
/* CVG 9.0 */
#include "packet_16.h"

#define	DEGRAD	(3.14159265/180.0)

/* Radial rle data packet header offset defintions */
#define number_of_bins_offset       2
#define range_interval_offset       5
#define number_of_radials_offset    6
#define rad_num_bytes_offset        7

/* Radial rle packet data offset definitions */
#define azimuth_angle_offset         1
#define azimuth_delta_offset         2
#define radial_data_array_offset     3

/* Symbology Block Offset defintions */
#define block1_size_offset      2
#define num_layers_offset       4
#define layer1_size_offset      6
#define image_type_offset       8

extern  Dimension barwidth;
extern	Display	*display;
extern	GC	gc;
extern	XColor	display_colors[];
extern  int     palette_size;
extern  Dimension pwidth, pheight;

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;
extern char config_dir[255];
extern assoc_array_i *digital_legend_flag;
extern assoc_array_s *digital_legend_file;
extern assoc_array_s *dig_legend_file_2;

void packet_16_skip(char *buffer,int *offset);

void display_packet_16(int packet, int offset, int replay);
void decode_packet_16(short *product_data, int offset);
void delete_packet_16();
void output_packet_16();
                                 
extern int read_digital_legend(FILE *leg_file, char *digital_legend_filename, 
                                               int method, int frame_location);

extern int read_generic_legend(FILE *leg_file, char *generic_legend_filename, 
                                                int method, int frame_location);
double calculate_color_scale();
int data_level_to_color(unsigned int d_level);
int lookup_gen_color(unsigned int d_level);


extern short read_half(char *buffer,int *offset);

/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

extern Widget shell;

#endif





