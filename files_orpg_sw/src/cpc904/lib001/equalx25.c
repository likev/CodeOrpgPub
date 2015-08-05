static char sccsid[] = "@(#)equalx25.c	1.2	12 Aug 1998";
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
 * equalx25.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: equalx25.c,v 1.4 2000/07/14 19:45:20 john Exp $
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

/*
 *  Checks if two x25 addresses are equal
 *  Returns: 1 if equal
 *	     0 if not equal
 */
int equalx25 (struct xaddrf *x1, struct xaddrf *x2)
{
	if ((x1->sn_id != x2->sn_id) ||
	    (x1->aflags != x2->aflags) ||
	    (x1->DTE_MAC.lsap_len != x2->DTE_MAC.lsap_len) ||
	    (x1->nsap_len != x2->nsap_len))
		return (0);

	if (!x25add_equal((char *)x1->NSAP, (char *)x2->NSAP, (unsigned int)x1->nsap_len))
		return (0);
	
	if (!x25add_equal((char *)x1->DTE_MAC.lsap_add, (char *)x2->DTE_MAC.lsap_add,
			(unsigned int)x1->DTE_MAC.lsap_len))
		return (0);

	return (1);
}

/*
 * Description : Checks if two Addresses (in the form of strings) are equal
 * Takes       : Two strings and a length (in semi-octets)
 * Returns     : 1 if equal; 0 if not equal
 */
int x25add_equal (char *s1, char *s2, unsigned int len)
{
	if (memcmp (s1, s2, len >> 1) == 0)	/* Compare len/2 bytes */
	{
		if (! (len & 1))		/* If not odd then equal */
			return (1);

		if ((s1 [len >> 1] & 0xF0) ==	/* Check last digit if odd */
			(s2 [len >> 1] & 0xF0))
			return (1);
	}
	return (0);
}
