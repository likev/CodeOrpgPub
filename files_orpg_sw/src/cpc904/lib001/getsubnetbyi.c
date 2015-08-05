static char sccsid[] = "@(#)getsubnetbyi.c	1.2	12 Aug 1998";
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
 * getsubnetbyi.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getsubnetbyi.c,v 1.4 2000/07/14 19:45:27 john Exp $
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

struct subnetent *getsubnetbyid(uint32 snid)
{
	register struct subnetent *p = NULL;

	setsubnetent(0);
	while (p = getsubnetent())
	{
		if (snid == p->xaddr.sn_id)
			break;
	}
	endsubnetent();
	return (p);
}
