/*   @(#)xty.h	1.1	07 Jul 1998	*/

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
 * xty.h of snet module
 *
 * SpiderX25
 * @(#)$Id: xty.h,v 1.1 2000/02/25 17:15:49 john Exp $
 * 
 * SpiderX25 Release 8
 */

#define XTY_STID	204
#define XIN_STID	205
#define XOUT_STID	206

struct xty {
	queue_t *hiqueue;
	queue_t *loqueue;
	queue_t *blqueue;	/* Queue for blocked connection */
	short	strflags;
	short	below_closed;
	short	pgrp;		/* Process group of non-blocking opener */
	short	w_pgrp;		/* Process group of blocking opener */
};

struct xout {
	queue_t *hiqueue;
	queue_t *ctlqueue;
	mblk_t	*xmbl;
};

struct xin {
	queue_t *hiqueue;
	queue_t *ctlqueue;
	mblk_t	*xmbl;
};


#define WBLOCK		1	/* Block user - daemon is slowing up!	*/
#define O_WAIT		2	/* Slave is waiting for master open	*/
#define CALL_OUT	4	/* Making an outgoing call		*/

#define PREV(q)	(OTHERQ(OTHERQ(q)->q_next))

#define XXX_CLOSE 	'C'	/* M_HANGUP field */
#define XXX_HANGUP 	'H'	/* M_HANGUP field */
#define XXX_IGNBRK	'Z'	/* Ignore Breakin */

#ifdef SVR4
#define	XTY_NONBLOCK	0x8000	/* Non-blocking bit in minor device number */
#define	XTY_DEVMASK	0x7FFF	/* Minor device number mask                */
#else
#define	XTY_NONBLOCK	0x80	/* Non-blocking bit in minor device number */
#define	XTY_DEVMASK	0x7F	/* Minor device number mask                */
#endif /* SVR4 */
