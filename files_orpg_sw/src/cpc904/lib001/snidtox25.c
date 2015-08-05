static char sccsid[] = "@(#)snidtox25.c	1.2	12 Aug 1998";
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
 * snidtox25.c of sx25 module
 *
 * SpiderX25
 * @(#)$Id: snidtox25.c,v 1.4 2000/07/14 19:45:33 john Exp $
 * 
 * SpiderX25 Release 8
 */



#include <stdio.h>
#include <ctype.h>
#ifndef SVR4
#include <sys/types.h>
#endif
#include <uint.h>
#include <x25_proto.h>
#include <xnetdb.h>
#include <sx25.h>

/*
 *  snidtox25 - Takes between a 1 and 4 character string
 *              identifier and converts it to a unsigned int
 */
uint32 snidtox25( unsigned char *snid)
{
	uint8	 posn = 0;
	uint32   new_snid = 0;

	if (snid == (unsigned char *) NULL)
		return(0);

	while (*snid && isalnum((int)*snid) && posn < (uint8) SN_ID_LEN)
	{
		/*
		 * First char should be in LSB, 
		 * Next char should be bit-shifted 8 bits left,
		 * Next char should be bit-shifted 16 bits left etc.
		 */
		new_snid += (((uint32) *snid) << (posn * 8));
		posn++;
		snid++;
	}
	return (new_snid);
}
