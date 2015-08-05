/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:06 $
 * $Id: cvt_packet_18.h,v 1.1 2002/08/30 16:14:06 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_18.h */

#ifndef _PACKET_18_H_
#define _PACKET_18_H_

#include <stdio.h>



void packet_18(char *buffer,int *offset,int *flag);


extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);


#endif



