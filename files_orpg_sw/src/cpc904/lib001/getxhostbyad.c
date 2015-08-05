static char sccsid[] = "@(#)getxhostbyad.c	1.2	12 Aug 1998";
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
 * getxhostbyad.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getxhostbyad.c,v 1.4 2000/07/14 19:45:30 john Exp $
 * 
 * SpiderX25 Release 8
 */

#include <stdio.h>
#ifndef SVR4
#include <sys/types.h>
#endif
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>

struct xhostent *getxhostbyaddr(char *addr, int len, int type)
{
	register struct xhostent *p = NULL;

	setxhostent(0);
	while (p = getxhostent())
	{
		if (p->h_addrtype != type || p->h_length != len)
			continue;

		if (equalx25 ((struct xaddrf *) p->h_addr,
				(struct xaddrf *) addr))
			break;
	}
	endxhostent();
	return (p);
}
