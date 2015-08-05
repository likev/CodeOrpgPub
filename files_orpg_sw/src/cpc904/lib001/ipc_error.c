/*   @(#) ipc_error.c 99/12/23 Version 1.2   */

/*
 *  SpiderSTREAMS IPC Library - Common Error Routines
 *
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  @(#)$Id: ipc_error.c,v 1.3 2000/07/14 15:57:50 jing Exp $
 */


/* 
Modification history:
Chg Date	     Init Description
 1. 13-AUG-98 mpb  Port to windows NT.
*/

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef WINNT
#include <winsock.h>
#endif /* WINNT */
#include <ipc.h>
#include "ipc_error.h"

#ifdef NEED_STRERROR
extern char *sys_errlist[];
extern int sys_nerr;
 
#define strerror(err) ((err) < sys_nerr? sys_errlist[(err)] : "Unknown error")
#endif /* NEED_STRERROR */

/* XXX make these work on systems with a global address space */

static char *ipc_progname = NULL;
static int ipc_error_value;

void
ipc_set_program_name(char *progname)
{
	ipc_progname = progname;
}

int
ipc_error(void)
{
	return ipc_error_value;
}

void
ipc_warning(char *format, ...)
{
	va_list args;

	if (ipc_progname)
		fprintf(stderr, "%s: ", ipc_progname);
	fprintf(stderr, "WARNING: ");

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, "\n");
}

void
ipc_system_error(char *format, ...)
{
	va_list args;
	int error = errno;

	if (ipc_progname)
		fprintf(stderr, "%s: ", ipc_progname);
	
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, ": %s\n", strerror(error));

	ipc_error_value = IPC_ERR_SYSTEM;
	errno = error;
}

void
ipc_memory_error(char *format, ...)
{
	va_list args;

	if (ipc_progname)
		fprintf(stderr, "%s: ", ipc_progname);

	fprintf(stderr, "failed to allocate memory for ");

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, "\n");

	ipc_error_value = IPC_ERR_MEMORY;
}

void
ipc_user_error(int error, char *format, ...)
{
	va_list args;

	if (ipc_progname)
		fprintf(stderr, "%s: ", ipc_progname);

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, "\n");

	ipc_error_value = error;
}
