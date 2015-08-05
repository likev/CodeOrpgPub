/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/03/03 17:59:33 $
 * $Id: cvt_display_TAB.h,v 1.3 2004/03/03 17:59:33 cheryls Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/* cvt_display_TAB.h */

#ifndef _DISPLAY_TAB_H_
#define _DISPLAY_TAB_H_

#include <stdio.h>
#include <product.h>
#include "cvt_dispatcher.h"

#define MAX_NUM_LINES 17

extern msg_data md;
extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);
extern int print_pdb_header(char* buffer);
extern int print_message_header(char* buffer);
void display_TAB(char *buffer,int *offset, int verbose);


#endif



