/*   @(#)xxx_control.h	1.1	07 Jul 1998	*/

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
 * xxx_control.h of snet module
 *
 * SpiderX25
 * @(#)$Id: xxx_control.h,v 1.1 2000/02/25 17:15:50 john Exp $
 * 
 * SpiderX25 Release 8
 */

#define XXX_STID	203

#ifdef SVR4
#define PAD_CMD		(('X' << 8) | 127)
#else
#define PAD_CMD		(('X' << 8) | 1)
#endif

/*  Maximum number of PAD arguments allowed */

#define	MAXARGC		24	/* To set all 22 pars+flag+command */

#define MAX_BUF_SZ	256

