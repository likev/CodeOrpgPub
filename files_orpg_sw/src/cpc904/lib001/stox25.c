static char sccsid[] = "@(#)stox25.c	1.2	12 Aug 1998";
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
 * stox25.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: stox25.c,v 1.4 2000/07/14 19:45:34 john Exp $
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
#include <ctype.h>
#include <sx25.h>

/****************************************************************************
 *	9th Jul. 91 Possible X25 addresses:
 *	LAN	id.LSAP..
 *		id.LSAP.N.NSAP
 *		id.LSAP.NSAP
 *		id.LSAP..NSAP
 *		id.LSAP.X.EXT
 *
 *	WAN88
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
 * 	      is found in file 'subnet.id' i.e. WAN88, WAN84, WAN80 or LAN. 
 ***************************************************************************/

int stox25(unsigned char *cp, struct xaddrf *xad, int lookup)
{
	uint8 c, i, j;
	unsigned char spare;
	int  ntype;

	memset((char *)xad, 0, sizeof(struct xaddrf));

	xad->aflags = 0;
	if (!(xad->sn_id = snidtox25(cp))) 	/* Extract the subnetwork id */
	{
#ifdef DEBUG
		fprintf (stderr, "SNID?\n");
#endif
		return (-1);
	}

	if (lookup)
	{
		if ((ntype = getnettype (cp)) < 0)
		{
#ifdef DEBUG
			fprintf (stderr, "Net Type trouble\n");
#endif
			return (-1);
		}
	}
	else
		ntype = ANY;

	while (*cp && *cp != '.' && !isspace((int)*cp)) cp++;

	if (!(*cp) || isspace((int)*cp))	/* Only a subnetwork id */
	{

		if (ntype != LAN)
			return(0);
		else
			return(-1);
	}

	if (*cp++ != '.')
	{
#ifdef DEBUG
		fprintf(stderr, "No dot after SNID and address not empty\n");
#endif /* DEBUG */
		return (-1);
	}
	
	/* Extract the DTE/LSAP part */
	if (!(*cp) || isspace((int)*cp))
	{
#ifdef DEBUG
		fprintf(stderr,"Nothing after first dot\n");
#endif
		if (ntype != LAN)
			return(0);
		else
			return(-1);
	}
	

	if (*cp == '.')
	{
		if (ntype == LAN)
		{
#ifdef DEBUG
			fprintf(stderr,"Null LSAP not allowed in LAN type address\n");
#endif
			return(-1);
		}
	}
	else
	{
		/* cp points at the character in the addr */
		for (i = 0, j = 0; j < LSAPMAXSIZE; i++, cp++)
		{
			if (isdigit (c = *cp))
				spare = c - '0';
			else
			if (isxdigit (c))
				 spare = c + 10 - (islower(c) ? 'a' : 'A');
			else
				break;

			if (i & 1)
				xad->DTE_MAC.lsap_add[j++] |= spare;
			else
				xad->DTE_MAC.lsap_add[j]  = (spare << 4);
		}

		if (ntype == LAN)
		{
			if (i != 14)
			{
#ifdef DEBUG
				fprintf(stderr, "LAN and len not 14\n");
#endif
				return (-1);
			}
		}
		else if ((int)i > DTEMAXSIZE)
		{
#ifdef DEBUG
			fprintf(stderr, "len > DTEMAXSIZE\n");
#endif
			return (-1);
		}

		xad->DTE_MAC.lsap_len = i;
	}
	
	if (!(*cp) || isspace((int)*cp))
	{
		/* No Extension and no dot after LSAP/DTE */
		return (0);
	}
	
	if (*cp++ != '.')
	{
#ifdef DEBUG
		fprintf (stderr, "No dot after LSAP/DTE\n");
#endif
		return (-1);
	}

	if (! *cp || isspace((int) *cp))
	{
		/* No Extension but dot after LSAP/DTE */
		return (0);
	}
	
	/* printf("X or N or dot = %c\n", *cp); */
	switch (*cp++)
	{
		case 'X':
		case 'x':	
			if ((*cp) && (*cp++ != '.'))
			{
#ifdef DEBUG
				fprintf (stderr, "No dot after 'X'\n");
#endif
				return (-1);
			}
			xad->aflags = EXT_ADDR;
			break;

		case 'N':
		case 'n':
			if ((*cp) && (*cp++ != '.'))
			{
#ifdef DEBUG
				fprintf (stderr, "No dot after 'N'\n");
#endif
				return (-1);
			}
			break;

		case 'P':
		case 'p':	
			xad->aflags = PVC_LCI;
			/* Check that there is nothing after the PVC flag */
			if (*cp++)
			{
#ifdef DEBUG
				fprintf (stderr, "Something follows 'P'\n");
#endif
				return (-1);
			}

			/* LCI must be 3 hex digits */
			if (xad->DTE_MAC.lsap_len != 3)
			{
#ifdef DEBUG
				fprintf (stderr, "LCI is not 3 digits\n");
#endif
				return (-1);
			}
			return (0);

		case '0':	case '1':	case '2':
		case '3':	case '4':	case '5':
		case '6':	case '7':	case '8':
		case '9':	case 'a':	case 'b':
		case 'c':	case 'd':	case 'e':
		case 'f':	case 'A':	case 'B':
		case 'C':	case 'D':	case 'E':
		case 'F':
			cp--;
			break;

		case '.':
			break;

		default:
#ifdef DEBUG
			fprintf(stderr,"option not X, N, digit or .\n");
#endif
			return (-1);
	}

	if (!(*cp) || isspace((int)*cp))	 /* NSAP part is NULL */
	{	
		xad->aflags = 0;
		return(0);		/* NSAP zeroed above */
	}
	
	if (ntype == W80)
	{
#ifdef DEBUG
		fprintf(stderr,"NSAP/EXT not valid in WAN80\n");
#endif
		return(-1);
	}

	/* cp points at the character in the addr */
	for (i = 0, j = 0; j < NSAPMAXSIZE; i++, cp++)
	{
		if (isdigit (c = *cp))
			spare = c - '0';
		else
		if (isxdigit (c))
			 spare = c + 10 - (islower(c) ? 'a' : 'A');
		else
			break;

		if (i & 1)
			xad->NSAP[j++] |= spare;
		else
			xad->NSAP[j]  = (spare << 4);
	}

	if (*cp && !isspace((int) *cp))	 /* End of X25 address? */
	{
#ifdef DEBUG
		fprintf(stderr, "Extra Chars at end of X25 address\n");
#endif
		return (-1);
	}

	xad->nsap_len = i;

	return (0);
}
