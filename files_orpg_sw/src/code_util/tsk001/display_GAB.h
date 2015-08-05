/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:36 $
 * $Id: display_GAB.h,v 1.7 2009/05/15 17:52:36 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* display_GAB.h */

#ifndef _DISPLAY_GAB_H_
#define _DISPLAY_GAB_H_

#include <stdio.h>
#include <product.h>
#include "global.h"

int printing_gab=0; /* we need to know when we're printing the gab so that we can
		     * scale some things differently - state variable for packet
		     * drawing functions */

extern screen_data *sd;
/* extern screen_data *sd2, *sd1; */
/* extern screen_data *sd3; */

extern int verbose_flag;
extern Display *display;
extern GC gc;
extern Pixel black_color;
/* CVG 9.0 added */
extern Pixel white_color;

extern  Dimension pwidth, pheight;
extern Dimension barwidth;


extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);
extern int read_word(char *buffer,int *offset);


extern void dispatch_packet_type(int packet, int offset, int replay);

void display_GAB(int offset);


#endif



