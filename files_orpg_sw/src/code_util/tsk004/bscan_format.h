/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:09 $
 * $Id: bscan_format.h,v 1.4 2008/03/13 22:45:09 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* bscan_format.h */

#ifndef _BSCAN_FORMAT_H_
#define _BSCAN_FORMAT_H_

/*  CVT 4.3 */
#include "cvt.h"

extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);
/*  cvt 4.2 */
/* extern short scale_parameter(short val,int flag); */
extern float scale_parameter(short val,int flag); /*  not used */

void generate_BSCAN(short num_radials,short range_bins,short code,char *buffer,
     int *offset,int *flag);

#endif

