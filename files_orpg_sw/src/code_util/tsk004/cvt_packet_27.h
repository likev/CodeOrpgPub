/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:36 $
 * $Id: cvt_packet_27.h,v 1.1 2002/08/30 16:14:36 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_27.h */

#ifndef _PACKET_27_H_
#define _PACKET_27_H_

#include <stdio.h>



void packet_27(char *buffer,int *offset);


extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);


#endif



