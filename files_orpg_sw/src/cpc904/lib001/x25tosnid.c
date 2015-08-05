static char sccsid[] = "@(#)x25tosnid.c	1.2	12 Aug 1998";
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
 * x25tosnid.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: x25tosnid.c,v 1.4 2000/07/14 19:45:35 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
	Modification history:
 
	Chg	Date			Init	Description
   1.    10-AUG-98   mpb   Compile on Windows NT.
*/



#if defined (KERNEL) || defined (_KERNEL) || defined (STREAMS_KERNEL)
#define bufp_t  caddr_t

#define isalnum(n)	((n > '0' && n < '9') || (n > 'a' && n < 'z') || \
			 (n > 'A' && n < 'Z'))

#else
#include <ctype.h>
#endif
#ifndef SVR4
#include <sys/types.h>
#endif

#include <uint.h>
#include <x25_proto.h>

#if defined (KERNEL) || defined (_KERNEL) || defined (STREAMS_KERNEL)
#include <sys/stream.h>
#endif

#if !defined (KERNEL) && !defined (_KERNEL) && !defined (STREAMS_KERNEL)
#include <xnetdb.h>
#endif
#include <sx25.h>

#ifndef NULL
#define NULL	0
#endif

#if defined (KERNEL) || defined (_KERNEL) || defined (STREAMS_KERNEL)
#define isnum(unknown) ( (unknown >= 'A' && unknown <= 'Z') || \
		  (unknown >= 'a' && unknown <= 'z') || \
		  (unknown >= '0' && unknown <= '0') )

#endif
/*
 *  x25tosnid - converts a subnetwork identifier represented
 *              as an unsigned int to a string 
 */
int x25tosnid(uint32 snid, unsigned char *str_snid)
{
	uint8 posn = 0;
	int i;

        if (str_snid == (unsigned char *)NULL)
			return(-1);

	/* First zero array */
	for (i = 0 ; i < SN_PRINT_LEN; i++)
		str_snid[i] = '\0';

    	while (snid && posn < SN_ID_LEN)
	{
		if (!isalnum((int)(*str_snid = ( unsigned char ) (snid & 0xFF))))
			/* If char is not alphanumeric, return failure */
			return(-1);
		str_snid++;
		snid = snid >> 8;
		posn++;
	}
	return(0);
}

