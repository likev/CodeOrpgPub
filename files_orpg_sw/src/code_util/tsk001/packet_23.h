/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:45 $
 * $Id: packet_23.h,v 1.4 2009/05/15 17:52:45 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* packet_23.h */

#ifndef _PACKET_23_H_
#define _PACKET_23_H_

#include <stdio.h>
#include "global.h"

extern screen_data *sd1, *sd2, *sd;
extern int verbose_flag;

/* prototype */
void display_packet_23(int packet,int offset);
void packet_23_skip(char *buffer,int *offset);

extern short read_half(char *buffer,int *offset);

extern void dispatch_packet_type(int packet, int offset, int replay);

/* Little Endian byte swap */
extern short read_half_flip(char *buffer,int *offset);

#endif



