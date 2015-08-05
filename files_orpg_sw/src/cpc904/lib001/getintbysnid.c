static char sccsid[] = "@(#)getintbysnid.c	1.2	12 Aug 1998";
/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Modified by: Ian Lartey
 *
 * getintbysnid.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getintbysnid.c,v 1.4 2000/07/14 19:45:23 john Exp $
 * 
 * SpiderX25 Release 8
 */



/*
 *  GETINTBYSNID.C   Get an interface entry with the supplied ID 
 *                   from the x25conf file
 */

#include <stdio.h>
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>

/*
* #define TRACE
*/

struct confinterface *getintbysnid(char *confname, uint32 snid)
{
	register struct confinterface *p = NULL;

	setconfintent(confname, 0);
	while (p = getconfifacent(confname))
	{
#ifdef TRACE
		printf ("getintbysnid: snid %x %x\n", snid, p->snid);
#endif
		if (snid == p->snid)
			break;
	}
	endconfintent();
	return (p);
}

struct confinterface *getnextintbysnid(char *confname, uint32 snid)
{
	register struct confinterface *p = NULL;

	setconfintent(confname, 2);
	while (p = getconfifacent(confname))
	{
#ifdef TRACE
                printf ("getnextintbysnid: snid %x %x\n", snid, p->snid);
#endif

		if (snid == p->snid)
			break;
	}
	if(p == NULL)
	{
		setconfintent(confname, 0);
		endconfintent();

	}
	return (p);
}
