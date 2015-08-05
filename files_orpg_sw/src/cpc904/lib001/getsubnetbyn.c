static char sccsid[] = "@(#)getsubnetbyn.c	1.2	12 Aug 1998";
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
 * getsubnetbyn.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getsubnetbyn.c,v 1.4 2000/07/14 19:45:28 john Exp $
 * 
 * SpiderX25 Release 8
 */

#include <stdio.h>
#ifndef SVR4
#include <sys/types.h>
#endif
#include <string.h>
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>

struct subnetent *getsubnetbyname( register char *name)
{
	register struct subnetent *p = NULL;

	setsubnetent(0);
	while (p = getsubnetent())
	{
		if (strcmp(p->alias, name) == 0)
			goto found;
	}
found:
	endsubnetent();
	return (p);
}
