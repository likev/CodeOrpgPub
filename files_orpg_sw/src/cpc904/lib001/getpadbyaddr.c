static char sccsid[] = "@(#)getpadbyaddr.c	1.2	12 Aug 1998";
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
 * getpadbyaddr.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getpadbyaddr.c,v 1.4 2000/07/14 19:45:25 john Exp $
 * 
 * SpiderX25 Release 8
 */

#ifndef SVR4
#include <sys/types.h>
#endif
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>

struct padent *getpadbyaddr(char *addr)
{
	struct padent *p;

	setpadent(0);
	while (p = getpadent())
	{
		if (equalx25 ((struct xaddrf *) &(p->xaddr),
				(struct xaddrf *) addr))
			break;
	}
	endpadent();
	return (p);
}
