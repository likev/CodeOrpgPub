/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:13:00 $
 * $Id: cvt_display_GAB.h,v 1.1 2002/08/30 16:13:00 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/* cvt_display_GAB.h */

#ifndef _DISPLAY_GAB_H_
#define _DISPLAY_GAB_H_

#include <stdio.h>
#include <product.h>
#include "cvt_dispatcher.h"

extern msg_data md;
extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);

void display_GAB(char *buffer,int *offset);
extern void packet_8(char *buffer,int *offset);
extern void packet_10(char *buffer,int *offset);



#endif



