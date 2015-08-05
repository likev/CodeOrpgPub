/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:43 $
 * $Id: packet_18.h,v 1.5 2009/05/15 17:52:43 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* packet_18.h */

#ifndef _DECODE_PACKET_18_H_
#define _DECODE_PACKET_18_H_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "global.h"


typedef struct {
   int             number_of_rows;
   int             number_of_columns;
   unsigned short *raster_data[464];
} precip_rate_data;

#define	DEGRAD	(3.14159265/180.0)

/* Packet 18 header offset definitions */
#define number_of_columns_offset    3
#define number_of_rows_offset       4
#define p18_header_offset           5

/* Packet 18 data offset definitions */
#define bytes_in_row_offset          1
#define p18_row_header_offset        1

int p18_orig_x;
int p18_orig_y;
int p18_g_size;

extern	Display		*display;
extern	GC		gc;
extern	XColor 	        display_colors[];
extern	Dimension	pwidth, pheight;

extern Pixel white_color, black_color;

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;


/* prototypes */
void packet_18_skip(char *buffer,int *offset);
void display_packet_18(int packet, int offset);

extern short read_half(char *buffer,int *offset);

/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif








