/*   @(#)sld.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 * sld.h of snet module
 *
 * SpiderX25
 * @(#)$Id: sld.h,v 1.1 2000/02/25 17:14:56 john Exp $
 * 
 * SpiderX25 Release 8
 */

#define SLD_STID	209

#define VALID_IBITS (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK) 
typedef struct sld	{
	ushort	sld_iflag;		/* input modes			*/
	ushort	sld_oflag;		/* output modes			*/
	ushort	sld_cflag;		/* control modes		*/
	ushort	sld_lflag;		/* ? modes			*/
	char	sld_col;		/* current column		*/
	ushort	sld_state;		/* local data space		*/
	unsigned char	sld_cc[8];	/* settable control chars	*/
	int	sld_to;
	ushort	sld_pgrp;		/* Process group		*/
	struct sld  *sld_next;		/* next tty on free list	*/
} SLD;
/* 4-byte masks to ensure CS7, etc have bits masked */
#define STRING_CS7_MASK 0x7f7f7f7fL
#define STRING_CS6_MASK 0x3f3f3f3fL
#define STRING_CS5_MASK 0x1f1f1f1fL

