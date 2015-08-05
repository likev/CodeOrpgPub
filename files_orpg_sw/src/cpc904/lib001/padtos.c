static char sccsid[] = "@(#)padtos.c	1.2	12 Aug 1998";
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
 * padtos.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: padtos.c,v 1.4 2000/07/14 19:45:32 john Exp $
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
#include <string.h>
#include <sx25.h>

int padtos(struct padent *p, char *strp)
{
	char	temp_str[256];
	char	*cp;
	int	i, j;
	int	lp = p->qos.xtras.locpacket;
	int	rp = p->qos.xtras.rempacket;
	int	lw = p->qos.xtras.locwsize;
	int	rw = p->qos.xtras.remwsize;
	
	/*
	 *  Check if return string is NULL
	 */
	 
	if (p == NULL)
	{
		fprintf (stderr, "Passed pad structure is NULL\n");
		return (-1);
	}

	 
	if (strp == NULL)
	{
		fprintf (stderr, "Passed string is NULL\n");
		return (-1);
	}
	*strp = '\0';


	if (lp != 0 || rp != 0)			/* Packet Sizes set */
	{
		/* Check range for remote packet size */
		if ((rp) && (rp < MIN_PKT || rp > MAX_PKT))
		{
			fprintf(stderr, "padtos: Remote packet size out of range\n");
			return (-1);
		}

		/* Check range for local packet size */		
		if ((lp) && (lp < MIN_PKT || lp > MAX_PKT))
		{
			fprintf(stderr, "padtos: Local packet size out of range\n");
			return (-1);
		}

		strcat(strp,"P");

		cp = strp + strlen(strp);
		sprintf (cp, "%d/%d", rp, lp);

	}

	if (lw != 0 || rw != 0)			/* Window Sizes set */
	{
		/* Check range for remote window size */		
		if ((rw) && (rw < MIN_WND || rw > MAX_WND))
		{
			fprintf(stderr, "padtos: Remote window size out of range\n");
			return (-1);
		}

		/* Check range for local window size */		
		if ((lw) && (lw < MIN_WND || lw > MAX_WND))
		{
			fprintf(stderr, "padtos: Local window size out of range\n");
			return (-1);
		}		

		strcat(strp,"W");

		cp = strp + strlen(strp);
		sprintf (cp, "%d/%d", rw, lw);

	}

	if (p->qos.xtras.fastselreq == 1)	/* Fast Select requested */
		strcat(strp,"F");

	if (p->qos.xtras.reversecharges == 1)	/* Reverse charge requested */
		strcat(strp,"R");

	switch (p->x29)
	{
	case 1: 
		strcat(strp," 80");
		break;

	case 2:
		strcat(strp," 84");
		break;

	case 3: 
		strcat(strp," 88");
		break;
	}
		
	cp = temp_str;
	i=0;
	while (i < MAX_CUG_LEN)
	{
		unsigned char ch;

		/*LINTED - deliberate single = */
		if (ch = p->qos.xtras.cug_field[i])
		{
			j = (ch & 0xF0) ? 1 : 0;
			break;
		}
		++i;
	}

	if (i < MAX_CUG_LEN)
	{

		*cp++ = ' ';
		switch (p->qos.xtras.cug_type)
		{
		case CUG:
	 		*cp++ = 'G';		/* Closed User Group */
			break;
		case BCUG:
			*cp++ = 'B';		/* Bilateral CUG */
			break;
		default:
			return(-1);
		}

		for (; i < MAX_CUG_LEN; i++)
		{
			unsigned char tmp, dig1, dig2;
			
			tmp = p->qos.xtras.cug_field[i];
			dig1 = ((tmp >> 4) & 15);
			dig2 = (tmp & 15);
			if (j & 1)
				*cp++ = dig1 + '0';
			*cp++ = dig2 + '0';
			j = 1;
		}
	}

	if (p->qos.xtras.rpoa_len)
	{
		*cp++ = ' ';
	 	*cp++ = 'T';		/* RPOA's */
		for (i = 0; i < (int)(p->qos.xtras.rpoa_len >> 1); i++)
		{
			unsigned char tmp, dig1, dig2;
			
			tmp = p->qos.xtras.rpoa_field[i];
			dig1 = ((tmp >> 4) & 15);
			dig2 = (tmp & 15);
			*cp++ = dig1 + '0';
			*cp++ = dig2 + '0';
		}
	}

	*cp++ = (char)NULL;
	strcat(strp, temp_str);

	/* Check length of CUD is not > 124 chars */
	if (strlen ((char *)p->cud) > (size_t)MAXCUDFSIZE)
	{
		fprintf(stderr, "Call User Data Field is > 124 chars\n");
		return (-1);
	}

	/* If CUD is not empty - copy it */
	if (p->cud[0] && *(p->cud) != '-')
	{
		strcat(strp," ~");		
		strcat(strp, (char *)p->cud);
	}

	return(0);
}

