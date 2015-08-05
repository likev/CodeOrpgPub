/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:13:07 $
 * $Id: cvt_packet_0802.h,v 1.1 2002/08/30 16:13:07 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* cvt_packet_0802.h */

#ifndef _PACKET_0802_H_
#define _PACKET_0802_H_

#include <stdio.h>

#define FALSE 0
#define TRUE 1

void packet_0802(char *buffer,int *offset);


extern short read_half(char *buffer,int *offset);


#endif



