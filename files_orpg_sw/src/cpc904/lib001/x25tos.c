static char sccsid[] = "@(#)x25tos.c	1.2	12 Aug 1998";
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
 * x25tos.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: x25tos.c,v 1.4 2000/07/14 19:45:34 john Exp $
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
#include <ctype.h>
#include <string.h>
#include <sx25.h>

/****************************************************************************
 *	22nd Feb. 90 Possible X25 addresses:
 *	LAN	id.LSAP..
 *		id.LSAP.N.NSAP
 *		id.LSAP.NSAP
 *		id.LSAP..NSAP
 *		id.LSAP.X.EXT
 *
 *	WAN84	id.DTE..
 *		id.DTE.N.NSAP
 *		id.LSAP.NSAP
 *		id.LSAP..NSAP
 *		id.DTE.X.EXT
 *		id..N.NSAP
 *		id..NSAP
 *		id...NSAP
 *		id..X.EXT
 *              id.LCI.P
 *
 *	WAN80	id.DTE..
 *		id...
 *              id.LCI.P
 *
 *	NOTE: 'id' is a logical subnetwork identifier.  A matching sub-net type
 * 		is found in file 'subnet.id' i.e. WAN84, WAN80 or LAN. 
 ***************************************************************************/

int x25tos (struct xaddrf *xad, unsigned char *cp, int lookup)
{
	unsigned char nsap_len = xad->nsap_len;
	unsigned char lsap_len = xad->DTE_MAC.lsap_len;
	unsigned char new_snid[5];
	int ntype, lp;
	int flagn = 0;
	int flagx = 0;
	int flagp = 0;
	int i = 0;


	if (cp == NULL)
	{
#ifdef DEBUG
		fprintf (stderr, "Passed string is NULL\n");
#endif
		return (-1);
	}

	/* Check maximum length of SAPs */
	
	if (lsap_len > (unsigned char)(LSAPMAXSIZE * 2))
	{
#ifdef DEBUG
		fprintf (stderr, "LSAP too long\n");
#endif
		return (-1);
	}

	if (nsap_len > (unsigned char)(NSAPMAXSIZE * 2))
	{
#ifdef DEBUG
		fprintf (stderr, "NSAP too long\n");
#endif
		return (-1);
	}
	
	if (x25tosnid(xad->sn_id, new_snid) < 0)
		return(-1);

	if (lookup)
	{
		if ((ntype = getnettype (new_snid)) < 0)
		{
#ifdef DEBUG
			fprintf (stderr, "Net Type trouble\n");
#endif
			return (-1);
		}
	}
	else
		ntype = ANY;

	switch (xad->aflags)		/* Check last bit of flag */
	{
	case NSAP_ADDR: flagn = 1;
			break;

	case EXT_ADDR:  flagx = 1;
			break;

	case PVC_LCI:   flagp = 1;
			break;

	default:
#ifdef DEBUG
			fprintf (stderr, "Invalid aflag\n");
#endif
			return (-1);
	}

	if ((flagn || flagx) && (ntype == W80) && nsap_len)
	{
#ifdef DEBUG
		fprintf (stderr, "No NSAP or EXT flag not allowed in WAN80\n");
#endif
		return (-1);
	}

	if ((ntype == LAN && lsap_len != 14) ||
	    ((ntype == W80 || ntype == W84) && ((int)lsap_len > (DTEMAXSIZE - 2))) ||
	    ((ntype == W88 || ntype == ANY) && (int)lsap_len > DTEMAXSIZE))
	{
#ifdef DEBUG
		fprintf (stderr, "LSAP/DTE length incorrect length\n");
#endif
		return (-1);
	}		

	while (new_snid[i] != '\0')
		*cp++ = new_snid[i++];			/* Get subnet_id */

	if (nsap_len == 0 && lsap_len == 0)	/* Check if we've finished */
	{
		*cp = '\0';
		return (0);
	}
		
	*cp++ = '.';

	if (lsap_len != 0)
	{
		for (lp = 0; lp < (int) lsap_len; lp++)
		{
			unsigned char tmp, hex1, hex2;
			
			tmp = xad->DTE_MAC.lsap_add [lp >> 1];
			hex1 = ((tmp >> 4) & 15);
			if (hex1 <= 9)
				*cp++ = hex1 + '0';
			else
				*cp++ = hex1 + 'A' - 10;
			lp++;
			if (lp >= (int) lsap_len)
				break;
			hex2 = (tmp & 15);
			if (hex2 <= 9)
				*cp++ = hex2 + '0';
			else
				*cp++ = hex2 + 'A' - 10;
		}
	}

	*cp++ = '.';

	if (flagp)
	{
		if (nsap_len != 0)
			return (-1);
		*cp++ = 'P';
		*cp = '\0';
		return (0);
	}

	if (nsap_len == 0)
	{
		*--cp = '\0';
		return (0);
	}
	
	if (flagx)
		*cp++ = 'X';
	else
	if (flagn)
		*cp++ = 'N';
		
	*cp++ = '.';
		
	for (lp = 0; lp < (int) nsap_len; lp++)
	{
		unsigned char tmp, hex1, hex2;
			
		tmp = xad->NSAP [lp >> 1];
		hex1 = ((tmp >> 4) & 15);
		if (hex1 <= 9)
			*cp++ = hex1 + '0';
		else
			*cp++ = hex1 + 'A' - 10;
		lp++;
		if (lp >= (int) nsap_len)
			break;
		hex2 = (tmp & 15);
		if (hex2 <= 9)
			*cp++ = hex2 + '0';
		else
			*cp++ = hex2 + 'A' - 10;
	}

	*cp = '\0';

	return (0);
}
