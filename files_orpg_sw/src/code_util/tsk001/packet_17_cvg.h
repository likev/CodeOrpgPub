/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:22:21 $
 * $Id: packet_17_cvg.h,v 1.1 2009/05/15 17:22:21 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* packet_17_cvg.h */

#ifndef _DECODE_PACKET_17_H_
#define _DECODE_PACKET_17_H_

#include <Xm/MessageB.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "global.h"
/* CVG 9.0 */
#include "packet_17.h"


#define	DEGRAD	(3.14159265/180.0)
#define PRECIP_ARRAY_IMAGE 2

/* Packet 17 header offset definitions */
#define number_of_columns_offset    3
#define number_of_rows_offset       4
#define p17_header_offset           5

/* Packet 17 data offset definitions */
#define bytes_in_row_offset          1
#define p17_row_header_offset        1

    
int p17_orig_x;
int p17_orig_y;
int p17_g_size;



extern	Display		*display;
extern	GC		gc;
extern	XColor	display_colors[];
extern  int     palette_size;
extern	Dimension	pwidth, pheight;
extern  Dimension barwidth;

extern Pixel white_color, black_color;

extern Widget shell;
extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;

extern char config_dir[255];
extern assoc_array_i *digital_legend_flag;
extern assoc_array_s *digital_legend_file;

void packet_17_skip(char *buffer,int *offset);

void display_packet_17(int packet, int offset, int replay);
void decode_packet_17(short *product_data, int offset);
void delete_packet_17();
void output_packet_17();


extern int read_digital_legend(FILE *leg_file, char *digital_legend_filename, 
                                               int method, int frame_location);


extern short read_half(char *buffer,int *offset);


/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif
