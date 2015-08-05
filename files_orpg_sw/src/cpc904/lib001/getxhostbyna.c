static char sccsid[] = "@(#)getxhostbyna.c	1.2	12 Aug 1998";
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
 * getxhostbyna.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getxhostbyna.c,v 1.4 2000/07/14 19:45:30 john Exp $
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

struct xhostent *getxhostbyname(register char *name)
{
	register struct xhostent *p = NULL;
	register char **cp;

	setxhostent(0);
	while (p = getxhostent())
	{
		if (strcmp(p->h_name, name) == 0)
			break;
		for (cp = p->h_aliases; *cp != 0; cp++)
			if (strcmp(*cp, name) == 0)
				goto found;
	}
found:
	endxhostent();
	return (p);
}
