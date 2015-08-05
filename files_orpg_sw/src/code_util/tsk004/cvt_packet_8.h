/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:15:12 $
 * $Id: cvt_packet_8.h,v 1.1 2002/08/30 16:15:12 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* cvt_packet_8.h */

#ifndef _PACKET_8_H_
#define _PACKET_8_H_

#include <stdio.h>



void packet_8(char *buffer,int *offset);


extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);


#endif



