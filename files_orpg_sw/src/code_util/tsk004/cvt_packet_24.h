/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:23 $
 * $Id: cvt_packet_24.h,v 1.1 2002/08/30 16:14:23 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
 
/* cvt_packet_24.h */

#ifndef _PACKET_24_H_
#define _PACKET_24_H_

#include <stdio.h>

void packet_24(char *buffer,int *offset);

extern short read_half(char *buffer,int *offset);

#endif



