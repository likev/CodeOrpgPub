static char sccsid[] = "@(#)getnettype.c	1.2	12 Aug 1998";
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
 * getnettype.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: getnettype.c,v 1.4 2000/07/14 19:45:24 john Exp $
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

FILE *fopen ();

void exit();

/*
 *  getnettype reads from file X25CONF to determine what type of network
 *  that a passed subnetwork refers to.
 */


int getnettype(unsigned char *snid)

{
	FILE	*finp=NULL;				/* INPUT FILE */
	char	line [MAXLINE];

	unsigned char	small_snid[MAXHOSTNAMESIZE];
	int		len = 0;
	int		i = 0;


 	if ((finp = fopen(X25CONF,"r")) == NULL)
		return(-1);

	while (fgets(line, MAXLINE, finp) != NULL)
	{
		char *subnet_id, *netid, *junk;
		
		/* strip comments */
		if (*line == '#')
			continue;

		if (*line == '\n')
			continue;

		/* get device prefix */
		if ((junk = strtok(line," \t\n")) == NULL)
		{
			fprintf(stderr, "File format error - Column 1\n");
			exit (-1);
		}

		/* ignore interface entries */
		if (strcmp(junk, (char *)"interface") == 0)
			continue;

		/* get user friendly name */
		if ((junk = strtok(NULL," \t\n")) == NULL)
		{
			fprintf(stderr, "File format error - column 2\n");
			exit (-1);
		}

		/* get class */
		if ((junk = strtok(NULL," \t\n")) == NULL)
		{
			fprintf(stderr, "File format error - column 3\n");
			exit (-1);
		}

		/* find appropriate subnet-id */
		if ((subnet_id = strtok(NULL," \t\n")) != NULL)
		{
			/*
			 * strip off the address
			 */

			strcpy( (char*)small_snid, (char*)snid );

			len = strlen( (char *)small_snid );

			for( i=0; i<len; ++i )
			{
				if ( small_snid[i] == '.' )
				{
					small_snid[i] = (unsigned char)NULL;
					break;
				}
			}

			if (strcmp((char*)small_snid, (char *)subnet_id) != 0)
				continue;

			/* examine net-type - column 5 */
			if ((netid = strtok(NULL," \t\n")) != NULL)
			{
				(void) fclose(finp);
				if (strcmp (netid, "LAN") == 0)
					return (LAN);
				else if (strcmp (netid, "WAN80") == 0)
					return (W80);
				else if (strcmp (netid, "WAN84") == 0)
					return (W84);
				else if (strcmp (netid, "WAN88") == 0)
					return (W88);
				else
					return (-1);
			}
		}
	}
	(void) fclose (finp);
	return (-1);
}
