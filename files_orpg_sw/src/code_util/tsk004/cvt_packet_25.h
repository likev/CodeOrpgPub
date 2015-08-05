/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:26 $
 * $Id: cvt_packet_25.h,v 1.1 2002/08/30 16:14:26 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* cvt_packet_25.h */

#ifndef _PACKET_25_H_
#define _PACKET_25_H_

#include <stdio.h>



void packet_25(char *buffer,int *offset);


extern short read_half(char *buffer,int *offset);


#endif



