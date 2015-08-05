/*   @(#)uint.h	1.2	08 Jul 1998	*/
/*
 *  Copyright (c) 1991 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 *  @(#)$Spider: uint.h,v 1.1 1997/05/26 13:44:29 mark Exp $
 */

/*
Modification History
Chg Date	Init Description
 1.  7-jul-98	rjp  Added some types.

*/

#ifndef unchar				/* #1				*/
#define unchar unsigned char
#endif
#ifndef ushort				/* #1				*/
#define ushort unsigned short
#endif
#ifndef caddr_t				/* #1				*/
#define caddr_t char *
#endif

#ifndef _SNET_UINT
#define _SNET_UINT

/*
 * Fixed-length types
 */

typedef char   int8;
typedef short  int16;
#ifdef __LP64__
typedef int    int32;
#else  /* 32 bit model */
typedef long   int32;
#endif /* __LP64__ */

typedef unsigned char   uint8;
typedef unsigned short  uint16;
#ifdef __LP64__
typedef unsigned int    uint32;
#else  /* 32 bit model */
typedef unsigned long   uint32;
#endif /* __LP64__ */

#endif /* _SNET_UINT */
