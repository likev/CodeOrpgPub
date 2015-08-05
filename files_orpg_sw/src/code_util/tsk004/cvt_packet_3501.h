/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:14:41 $
 * $Id: cvt_packet_3501.h,v 1.1 2002/08/30 16:14:41 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
 
/* cvt_packet_3501.h */

#ifndef _PACKET_3501_H_
#define _PACKET_3501_H_

#include <stdio.h>

#define FALSE 0
#define TRUE 1

void packet_3501(char *buffer,int *offset);


extern short read_half(char *buffer,int *offset);


#endif



