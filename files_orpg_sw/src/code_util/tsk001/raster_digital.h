/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:47:38 $
 * $Id: raster_digital.h,v 1.5 2008/03/13 22:47:38 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* raster_digital.h */

#ifndef _RASTER_DIGITAL_H_
#define _RASTER_DIGITAL_H_

#include <stdlib.h>
#include <stdio.h>
#include "global.h"


#define	DEGRAD	(3.14159265/180.0)



extern	Display		*display;
extern	GC		gc;
extern	XColor 	        display_colors[];
extern	Dimension	pwidth;


extern screen_data *sd;
/* extern screen_data *sd1, *sd2, *sd2; */

extern int verbose_flag;

extern int palette_size;

/* prototypes */
void digital_raster_skip(char *buffer,int *offset);
void display_digital_raster(int packet, int offset);

extern short read_half(char *buffer,int *offset);


/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);
extern void MISC_short_swap( void *buf, int size);

#endif








