/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:08 $
 * $Id: cvt_packet_19.h,v 1.1 2002/08/30 16:14:08 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_19.h */

#ifndef _PACKET_19_H_
#define _PACKET_19_H_

#include <stdio.h>



void packet_19(char *buffer,int *offset,int *flag);


extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);


#endif



