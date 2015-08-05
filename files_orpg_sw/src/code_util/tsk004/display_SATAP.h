/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2002/08/30 16:15:16 $
 * $Id: display_SATAP.h,v 1.1 2002/08/30 16:15:16 cheryls Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* display_SATAP.h */

#ifndef _DISPLAY_SATAP_H_
#define _DISPLAY_SATAP_H_

#include <stdio.h>
#include <product.h>
#include "cvt_dispatcher.h"

extern msg_data md;
extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);

void display_SATAP(char *buffer);


#endif



