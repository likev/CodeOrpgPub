static char sccsid[] = "@(#)any.c	1.2	12 Aug 1998";
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
 * @(#)$Id: any.c,v 1.3 2000/07/14 15:57:17 jing Exp $
 * 
 */

char * any(register char *cp, char *match)
{
	register char *mp, c;

	/*LINTED - deliberate single = */
	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}
