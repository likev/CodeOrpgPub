static char sccsid[] = "@(#)getsubnetent.c	1.2	12 Aug 1998";
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
 * getsubnetent.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getsubnetent.c,v 1.4 2000/07/14 19:45:29 john Exp $
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

static FILE *snidf = NULL;
static char line[BUFSIZ+1];
static struct subnetent subnetentry;
static int stayopen = 0;

void setsubnetent(int f)
{
	if (snidf == NULL)
		snidf = fopen(SUBNETS, "r" );
	else
		rewind(snidf);
	stayopen |= f;
}

void endsubnetent(void)
{
	if (snidf && !stayopen) {
		fclose(snidf);
		snidf = NULL;
	}
}

struct subnetent *getsubnetent(void)
{
	char *p;
	register char *cp;

	if (snidf == NULL && (snidf = fopen(SUBNETS, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(line, BUFSIZ, snidf)) == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';

	while (*p == ' ' || *p == '\t')
		p++;

	cp = any(p, " \t");
	if (cp != NULL)
		*cp++ = '\0';

	memset((char *)&subnetentry, 0, sizeof(struct subnetent));

	if ((stox25 ((unsigned char *) p, &subnetentry.xaddr, ADDR_CHECK)) < 0)
	{
#ifdef DEBUG
		fprintf(stderr, "getsnident: Invalid X.25 address\n");
#endif /* DEBUG */
		goto again;
	}
        if ( ! cp )
                return(&subnetentry);

	p = any(cp, " \t");
	subnetentry.alias = cp;
	if (p == NULL)
		return(&subnetentry);

	*p++ = '\0';

	while (*p == ' ' || *p == '\t')
		p++;

	cp = any(p, "\t");
	if (cp != NULL)
	{
		while ((*cp != '\n') && (*cp))
			cp++;
		*cp++ = '\0';
	}

	subnetentry.descrip = p;

	return (&subnetentry);
}
