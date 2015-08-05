/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:32 $
 * $Id: byteswap.h,v 1.5 2009/05/15 17:52:32 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef _BYTESWAP_H_
#define _BYTESWAP_H_

#include <misc.h>


#define SHORT_BSWAP(a) ((((a) & 0xff) << 8) | (((a) >> 8) & 0xff))
#define INT_BSWAP(a) ((((a) & 0xff) << 24) | (((a) & 0xff00) << 8) | \
		                (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))
#define INT_SSWAP(a) ((((a) & 0xffff) << 16) | (((a) >> 16) & 0xffff))
#define SHORT_SSWAP(a) {short z; z = a[0]; a[0] = a[1]; a[1] = z;}



int write_orpg_product_int( void *loc, void *value );
int read_orpg_product_int( void *loc, void *value );
int write_orpg_product_float( void *loc, void *value );
int read_orpg_product_float( void *loc, void *value );
	
#endif
