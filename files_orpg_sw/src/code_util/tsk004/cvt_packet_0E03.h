/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:13:28 $
 * $Id: cvt_packet_0E03.h,v 1.1 2002/08/30 16:13:28 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* cvt_packet_0E03.h */

#ifndef _PACKET_0E03_H_
#define _PACKET_0E03_H_

#include <stdio.h>

#define FALSE 0
#define TRUE 1

void packet_0E03(char *buffer,int *offset);


extern short read_half(char *buffer,int *offset);


#endif



