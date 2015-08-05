/*   @(#) nc_error.c 99/12/23 Version 1.2   */
/*
 *  STREAMS Network Control Library
 *
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  Made in Scotland.
 *
 *  @(#)$Id: nc_error.c,v 1.3 2000/07/14 15:57:56 jing Exp $
 */

/* 
Modification history:
Chg Date	     Init Description
1.  13-AUG-98 mpb  Port to Windows NT.
*/

#ifdef WINNT
#include <winsock.h>
#endif /* WINNT */

#include <nc.h>

struct nc_error {
	int	error;
	char	*name;
	char	*desc;
};

static struct nc_error nc_errors[] =
{
	{ NC_ERR_CONNECT,	"CONNECT",	"connection error" },
	{ NC_ERR_PROTOCOL,	"PROTOCOL",	"protocol error" },
	{ NC_ERR_SERVER,	"SERVER",	"server error" },
	{ NC_ERR_USER,		"USER",		"client error" },
	{ NC_ERR_LABEL,		"LABEL",	"invalid label" },
};

#define NERR	(sizeof(nc_errors) / sizeof(struct nc_error))

int
nc_lookup_error(char *s)
{
	int i;

	for (i = 0; i < NERR; i++) {
		if (strcmp(nc_errors[i].name, s) == 0)
			return nc_errors[i].error;
	}

	return 0;
}

char *
nc_error_name(int error)
{
	int i;

	for (i = 0; i < NERR; i++) {
		if (nc_errors[i].error == error)
			return nc_errors[i].name;
	}

	return "<UNKNOWN>";
}

char *
nc_error_string(int error)
{
	int i;

	for (i = 0; i < NERR; i++) {
		if (nc_errors[i].error == error)
			return nc_errors[i].desc;
	}

	return "unknown error";
}
