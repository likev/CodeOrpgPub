/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:46:26 $
 * $Id: packet_0802.h,v 1.5 2008/03/13 22:46:26 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* packet_0802.h */

#ifndef _PACKET_0802_H_
#define _PACKET_0802_H_

#include <stdio.h>
#include "global.h"

#define FALSE 0
#define TRUE 1

int contour_color=0;      /* color contour packets have their color level set by a
			   * specific packet (0802x) so we need persistance of
			   * the last set color level so that the countours know
			   * how to be drawn as
			   */

extern screen_data  *sd;
/* extern screen_data *sd1, *sd2, *sd3; */
extern int verbose_flag;

void display_packet_0802(int packet,int offset);
void packet_0802_skip(char *buffer,int *offset);

extern short read_half(char *buffer,int *offset);

/* Little Endian byte swap */
extern void MISC_short_swap( void *buf, int size);

#endif



