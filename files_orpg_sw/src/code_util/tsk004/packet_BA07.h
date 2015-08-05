/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:47:10 $
 * $Id: packet_BA07.h,v 1.4 2008/03/13 22:47:10 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* packet_BA07.h */

#ifndef _PACKET_BA07_H_
#define _PACKET_BA07_H_

#include <stdio.h>
#include <stdlib.h>
#include "misc_functions.h"

/*  CVT 4.3 */
#include "cvt.h"


#define FALSE 0
#define TRUE 1

void packet_BA07(char *buffer, int *offset, int *flag);
void decode_raster_rle(char *msg, short row, char *buffer, int *offset,
                       short num_bytes, short n_cols, int flag);
void print_raster_rle(char *msg, short row, char *buffer, int *offset,
                      short num_bytes);


#endif

