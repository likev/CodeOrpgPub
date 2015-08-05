static char sccsid[] = "@(#)getxhostent.c	1.2	12 Aug 1998";
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
 * getxhostent.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getxhostent.c,v 1.4 2000/07/14 19:45:31 john Exp $
 * 
 * SpiderX25 Release 8
 */

#include <stdio.h>
#include <ctype.h>
#ifndef SVR4
#include <sys/types.h>
#endif
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>

#define	MAXALIASES	35

static FILE *xhostf = NULL;
static char line[BUFSIZ+1];
static char xhostaddr[MAXXADDRSIZE];
static struct xhostent xhost;
static char *xhost_aliases[MAXALIASES];
static int stayopen = 0;

void setxhostent(int f)
{
	if (xhostf == NULL)
		xhostf = fopen(XHOSTS, "r" );
	else
		rewind(xhostf);
	stayopen |= f;
}

void endxhostent(void)
{
	if (xhostf && !stayopen) {
		fclose(xhostf);
		xhostf = NULL;
	}
}

struct xhostent *getxhostent()
{
	char *p;
	register char *cp, **q;

	if (xhostf == NULL && (xhostf = fopen(XHOSTS, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(line, BUFSIZ, xhostf)) == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	
	xhost.h_addr = xhostaddr;
	if ((stox25 ((unsigned char *) p, (struct xaddrf *) xhostaddr,
					ADDR_CHECK)) < 0)
	{
#ifdef DEBUG
		fprintf(stderr, "getxhostent : X25 Address error\n");
#endif
		goto again;
	}
	
	xhost.h_length = sizeof (struct xaddrf);
	xhost.h_addrtype = CCITT_X25;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	xhost.h_name = cp;
	q = xhost.h_aliases = xhost_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &xhost_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&xhost);
}
