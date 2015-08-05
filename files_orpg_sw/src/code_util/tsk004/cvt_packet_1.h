/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:13:30 $
 * $Id: cvt_packet_1.h,v 1.1 2002/08/30 16:13:30 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* cvt_packet_1.h */

#ifndef _PACKET_1_H_
#define _PACKET_1_H_

#include <stdio.h>

#define FALSE 0
#define TRUE 1

void packet_1(char *buffer,int *offset);


extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);


#endif



