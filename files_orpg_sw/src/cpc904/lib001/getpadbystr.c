static char sccsid[] = "@(#)getpadbystr.c	1.2	12 Aug 1998";
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
 * getpadbystr.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getpadbystr.c,v 1.5 2002/05/14 19:09:04 eddief Exp $
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

#define MAX_LENGTH	256

#ifndef LINUX
char	* strcat();
char	* strncpy();
char	* strcpy();
#endif

static char buf[MAX_LENGTH];

char *getpadbystr(char *s)
/* Routine takes a string which can be a name eg fred, which refers to
 * a pad address or an actual X25 (pad) address in dot format
 */
{
	char		buf2[MAX_LENGTH];
	char		facilities[MAX_LENGTH];
	struct padent	*pad;
	struct xhostent	*xhost;
	struct xaddrf	*xaddr;
	struct xaddrf	xad;
	char		*host;
	uint32		snid;

	buf[0] = '\0';
	buf2[0] = '\0';
	facilities[0] = '\0';

	if (xhost = getxhostbyname(s))
	{
		host = xhost->h_addr;
		snid = ((struct xaddrf *)host)->sn_id;
#ifdef DEBUG
			printf("Valid X25 host name\n");
#endif
	}
	else
	{
		if (stox25 ((unsigned char *) s, &xad, ADDR_CHECK) < 0)
		{
#ifdef DEBUG
			printf("Not a valid X25 string type address... ");
#endif
			return(NULL);
		}
		else
		{
#ifdef DEBUG
			printf("Is a valid X25 string type address... ");
#endif
			host = (char *)&xad;
			snid = xad.sn_id;
		}
	}
#ifdef DEBUG
	printf("We now have a valid pad address/name\n");
#endif

	x25tos((struct xaddrf *)host, (unsigned char *)buf, 0);

	if (xaddr = getaddrbyid(snid))
	{
		x25tos(xaddr, (unsigned char*)buf2, 0);
		strcat(buf, " +");
		strcat(buf, buf2);
	}

	if ((pad = getpadbyaddr(host)) != NULL)
	{
#ifdef DEBUG
		printf("Pad entry for address %s\n", s);
#endif
		if (padtos(pad, &facilities[0]) == 0)
		{
#ifdef DEBUG
			printf("Facilities = %s(%d)\n", facilities,
							strlen(facilities));
#endif
			strcat(buf, " ");
			strcat(buf, facilities);
		}
	}
#ifdef DEBUG
printf("return buf = %s\n", buf);
#endif
	/*LINTED - could fix with 'return (*(& buf))'	*/
	return (buf);
}
