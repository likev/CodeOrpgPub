/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:04 $
 * $Id: cvt_packet_17.h,v 1.1 2002/08/30 16:14:04 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_17.h */

#ifndef _PACKET_17_H_
#define _PACKET_17_H_

#include <stdio.h>



void packet_17(char *buffer,int *offset,int *flag);


extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);


#endif



