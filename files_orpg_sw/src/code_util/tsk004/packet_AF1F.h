/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:55 $
 * $Id: packet_AF1F.h,v 1.5 2009/05/15 17:37:55 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/* packet_AF1F.h */

#ifndef _PACKET_AF1F_H_
#define _PACKET_AF1F_H_

#include <stdio.h>
#include <stdlib.h>
#include "misc_functions.h"

#include "cvt.h"

#define FALSE 0
#define TRUE 1


void packet_AF1F(char *buffer,int *offset,int *flag);

/* CVT 4.4 - added rad number parameter */
void decode_rle(char *msg,int rad, short start_angle,short delta,char *buffer,
                int *offset,short num_bytes,short num_bins, int flag);

/* CVT 4.4 - added rad number parameter */  
void print_rle(char *msg,int rad,short start_angle,short delta,char *buffer,
               int *offset,short num_bytes);


#endif

