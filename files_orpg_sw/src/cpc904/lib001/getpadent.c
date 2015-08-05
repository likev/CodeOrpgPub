static char sccsid[] = "@(#)getpadent.c	1.2	12 Aug 1998";
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
 * getpadent.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getpadent.c,v 1.5 2002/05/14 19:09:05 eddief Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history.
Chg Date	Init Description
 1.  6-jul-98	rjp  Changed uint8 to char so compiler wouldn't complain about
		     comparison, 'less than zero', always being false.
*/


#include <stdio.h>
#include <ctype.h>
#ifndef SVR4
#include <sys/types.h>
#endif
#include <string.h>
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>


static FILE *padf = NULL;
static char line[BUFSIZ+1];
static struct padent pad;
static int stayopen = 0;

#ifndef LINUX
char	* strncpy();
char	* strcpy();
char	* strcat();
#endif

void setpadent(int f)
{
	if (padf == NULL)
		padf = fopen(PADMAP, "r" );
	else
		rewind(padf);
	stayopen |= f;
}

void endpadent(void)
{
	if (padf && !stayopen)
	{
		fclose(padf);
		padf = NULL;
	}
}

struct padent * getpadent(void)
/*
 *  Gets one VALID pad entry from the PADMAP file
 *  Returns a pointer to a padent structure or
 *  NULL for an error
 */
{
	char *p;
	register char *cp, *eol_ptr;
	int chr, more_facs;
	int i, j, c;
	unsigned char spare;

	if (padf == NULL && (padf = fopen(PADMAP, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(line, BUFSIZ, padf)) == NULL)
		return (NULL);

	if (*p == '#')			/* Ignore comment lines */
		goto again;

	/* Replace trailing hash or newline with '\0' */
	cp = any(p, "#\n");

	/* If not there we have an invalid line */
	if (cp == NULL)
		goto again;

	memset((char*)&pad, 0, sizeof(struct padent));

	eol_ptr = cp;
	*cp = '\0';

	cp = any(p, " \t");

	if (cp != NULL)
		*cp++ = '\0';
	
	if ((stox25 ((unsigned char *) p, &pad.xaddr, ADDR_CHECK)) < 0)
	{
#ifdef DEBUG
		fprintf(stderr, "getpadent : X25 Address error\n");
#endif
		goto again;
	}

	while (*cp == ' ' || *cp == '\t')
		++cp;

	p = any(cp, " \t");
	if (p == NULL)
		return(&pad);

	*p++ = '\0';

	if (*cp == (char)NULL)
		return(&pad);
	
	if (strcmp(cp, "80") == 0)
		pad.x29 = 1;
	else
	if (strcmp(cp, "84") == 0)
		pad.x29 = 2;
	else
	if (strcmp(cp, "88") == 0)
		pad.x29 = 3;
	else
		/* Use configured default X.29 type */
		pad.x29 = 0;

	while (*p == ' ' || *p == '\t')
		++p;

	if (*p == (char)NULL)			
		return(&pad);

	if (*p != '-')
	{
		more_facs = 1;
		while (more_facs)
		{
			switch (*p++)
			{
			int 	val;	/* #1. Was uint8 val; 		*/
	
			case 'F':
			case 'f': pad.qos.xtras.fastselreq = 1;
				  break;
		
			case 'R':
			case 'r': pad.qos.xtras.reversecharges = 1;
				  break;
		
			case 'P':
			case 'p': val = 0;
				  while (isdigit (*p))
				  	val = val * 10 + (*p++ - '0');
				  if (val < MIN_PKT || val > MAX_PKT)
				  	goto again;
				  if (*p++ != '/')
				  	goto again;
				  pad.qos.xtras.rempacket = val;
	
				  val = 0;
				  while (isdigit (*p))
				  	val = val * 10 + (*p++ - '0');
				  if (val < MIN_PKT || val > MAX_PKT)
				  	goto again;		  
				  pad.qos.xtras.locpacket = val;
				  break;
		
		
			case 'W':
			case 'w': val = 0;
				  while (isdigit (*p))
				  	val = val * 10 + (*p++ - '0');
				  if (val < MIN_WND || val > MAX_WND)
				  	goto again;
	
				  if (*p++ != '/')
				  	goto again;
				  pad.qos.xtras.remwsize = val;
	
				  val = 0;
				  while (isdigit (*p))
				  	val = val * 10 + (*p++ - '0');
				  if (val < MIN_WND || val > MAX_WND)
				  	goto again;		  
				  pad.qos.xtras.locwsize = val;
				  break;

			case 'C':
			case 'c': val = 0;
				  while (isdigit (*p))
					val = val * 10 + (*p++ - '0');
				  if (val < MIN_TCLASS || val > MAX_TCLASS)
					goto again;

				  if (*p++ != '/')
					goto again;
				  pad.qos.remthroughput = val;

				  val = 0;
				  while (isdigit (*p))
					val = val * 10 + (*p++ - '0');
				  if (val < MIN_TCLASS || val > MAX_TCLASS)
					goto again;
				  pad.qos.locthroughput = val;
				  pad.qos.reqtclass = 1;
				  break;

			default :
#ifdef DEBUG
				  fprintf (stderr, "Invalid facilty\n");
#endif
				  goto again;
	
			}	/* End Switch */
			chr = *p;
			if (chr == ' ' || chr == '\t' || chr == '\0')
				more_facs = 0;
			else if (chr == ',')
				p++;
		}
	}
	else
		p++;
		
	if (p >= eol_ptr)
		return (&pad);

	while (*p == ' ' || *p == '\t') ++p;
	if (*p == (char)NULL)			
		return(&pad);

	if (*p != '-')
	{

		pad.qos.xtras.cug_type = CUG;
		switch (*p++)
		{
		case 'b':
		case 'B':
			pad.qos.xtras.cug_type = BCUG;
			/*FALLTHROUGH - lint directive */
		case 'g':
		case 'G':

		 	/* p points at the character in the CUG */
			for (i = 0, j = 0; j < MAX_CUG_LEN; i++, p++)
			{
				if (isdigit (c = *p))
					spare = c - '0';
				else
					break;

				if (i & 1)
					pad.qos.xtras.cug_field[j++] |= spare;
				else
					pad.qos.xtras.cug_field[j]  = (spare << 4);
			}

			if (*p && isdigit(*p))
			 /* End of CUG? */
			{
#ifdef DEBUG
fprintf(stderr, "Extra Chars at end of CUG\n");
#endif
				return(NULL);
			}

			/* right justify */
			while (i < (2*MAX_CUG_LEN))
			{
				for (j=MAX_CUG_LEN-1; j>0; --j)
				{
					pad.qos.xtras.cug_field[j] >>= 4;
					pad.qos.xtras.cug_field[j] |=
					    (pad.qos.xtras.cug_field[j-1] << 4);
				}
				pad.qos.xtras.cug_field[0] >>= 4;
				i++;
			}
			break;
		default :
			pad.qos.xtras.cug_type = 0;
#ifdef DEBUG
				fprintf (stderr, "Invalid CUG\n");
#endif
			goto again;


		}	/* End Switch */
		chr = *p;
		if (chr == ' ' || chr == '\t' || chr == '\0')
			more_facs = 0;
	}
	else
		p++;

	if (p >= eol_ptr)
		return (&pad);

	while (*p == ' ' || *p == '\t') ++p;
	if (*p == (char)NULL)		
		return(&pad);

	if (*p != '-')
	{
		switch (*p++)
		{
		case 't':
		case 'T':

		 	/* p points at the character in the RPOA */
			for (i = 0, j = 0; j < MAX_RPOA_LEN; i++, p++)
			{
				if (isdigit (c = *p))
					spare = c - '0';
				else
					break;

				if (i & 1)
				    pad.qos.xtras.rpoa_field[j++] |= spare;
				else
				    pad.qos.xtras.rpoa_field[j]  = (spare << 4);
			}

			if (*p && isdigit(*p))
			 /* End of RPOA? */
			{
#ifdef DEBUG
fprintf(stderr, "Extra Chars at end of RPOA\n");
#endif
				return (NULL);
			}

			if ( (i%4) || (i > 16) )
			{
#ifdef DEBUG
					fprintf (stderr, "Invalid RPOA\n");
#endif
				goto again;
			}
			pad.qos.xtras.rpoa_len = (uint8) i;
			break;

		default :
#ifdef DEBUG
				  fprintf (stderr, "Invalid RPOA\n");
#endif
			  goto again;

		}	/* End Switch */
		chr = *p;
		if (chr == ' ' || chr == '\t' || chr == '\0')
			more_facs = 0;
	}
	else
		p++;

	if (p >= eol_ptr)
		return (&pad);

	while (*p == ' ' || *p == '\t') ++p;
	if (*p == (char)NULL)			
		return(&pad);

	cp = (char *)pad.cud;
	/* Only copy maximum of MAXCUDFSIZE chars */
	if (strlen(p) > MAXCUDFSIZE)
	{
		strncpy(cp, p, MAXCUDFSIZE);
		strcat(cp, "\0");
	}
	else
		strcpy(cp, p);

	return (&pad);
}
