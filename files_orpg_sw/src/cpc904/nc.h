/*   @(#)nc.h	1.2	12 Oct 1999	*/
/*
 *  STREAMS Network Control Library - interface
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
 *  @(#)$Id: nc.h,v 1.1 2000/02/25 17:14:47 john Exp $
 */

#ifndef _nc_h
#define _nc_h

#ifndef USE_VARARGS
#include <stdarg.h>
#endif /* USE_VARARGS */
#include <ipc.h>

#define NC_VERSION	1	/* interface version */
#define NC_PROTOCOL	1	/* protocol version */

/* NC "handle" -- an opaque data type */

#ifdef __STDC__
struct nc;
#endif

typedef struct nc NC;

/* protocol limits */

#define NC_MAX_LABEL	256	/* max. label length */
#define NC_MAX_MESSAGE	2000	/* max. message length */
#define NC_MAX_ERROR	256	/* max. length of error string */

/* error values */

#define NC_ERR_MEMORY	1	/* memory allocation error */
#define NC_ERR_CONNECT	2	/* error connecting to server */
#define NC_ERR_IPC	3	/* error connecting to server */
#define NC_ERR_PROTOCOL	4	/* error in protocol */
#define NC_ERR_SERVER	5	/* error on server */
#define NC_ERR_USER	6	/* error in user request */
#define NC_ERR_LABEL	7	/* invalid label specified */

/* default server name */

#ifndef NETD_SERVER_VARIABLE
#ifdef VXWORKS
#define NETD_SERVER_VARIABLE	"X25NETD" /* env var containing server ipc name */
#else
#define NETD_SERVER_VARIABLE	"NETD"	/* env var containing server ipc name */
#endif /* VXWORKS */
#endif

#ifndef NETD_SERVER_DEFAULT
#ifdef VXWORKS
#define NETD_SERVER_DEFAULT	"x25dnetd"	/* default ipc server name */
#else
#define NETD_SERVER_DEFAULT	"netd"	/* default ipc server name */
#endif /* VXWORKS */
#endif

#define nc_default_server() \
	ipc_queue_name(NETD_SERVER_VARIABLE, NETD_SERVER_DEFAULT)

/* error handler (callback) type */

typedef void (nc_handler)(int, char *, struct nc *, char *);

/* functional interface */

extern void nc_set_program_name(char *progname);

extern void nc_set_default_error_handler(nc_handler *handler);
extern void nc_set_error_handler(NC *nc, nc_handler handler);

extern NC *nc_initialise(char *server);		/* note spelling */
extern void nc_terminate(NC *nc);

extern int nc_send(NC *nc);	/* "internal" interface */
#ifdef USE_VARARGS
extern int nc_write();	/* "internal" interface */
#else /* USE_VARARGS */
extern int nc_write(NC *nc, char *format, ...);	/* "internal" interface */
#endif /* USE_VARARGS */
extern int nc_read(NC *nc, char *buf, int len);	/* "internal" interface */
extern int nc_request(NC *nc);			/* "internal" interface */
extern void nc_reset(NC *nc);			/* "internal" interface */

extern int nc_parse_version(NC *nc);		/* "internal" interface */
extern int nc_parse_status(NC *nc);		/* "internal" interface */
extern char *nc_parse_string(NC *nc);		/* "internal" interface */
extern int nc_parse_value(NC *nc);		/* "internal" interface */
extern int nc_parse_error(NC *nc);		/* "internal" interface */

extern int nc_version(NC *nc);
extern char *nc_open(NC *nc, char *server, char *service, char *controller,
        char *protocol, char *device, char *label);
extern int nc_close(NC *nc, char *stream);
extern int nc_push(NC *nc, char *stream, char *module);
extern int nc_pop(NC *nc, char *stream);
extern char *nc_link(NC *nc, char *upper, char *lower, char *label);
extern int nc_unlink(NC *nc, char *multiplexor, char *id);
extern int nc_unlink_all(NC *nc, char *multiplexor);
extern int nc_strioc(NC *nc, char *stream, int command, long timeout,
		     unsigned char *data, int length);
extern int nc_shutdown(NC *nc);

extern int nc_alias(NC *nc, char *label, char *alias);
extern int nc_delete(NC *nc, char *label);
extern int nc_rename(NC *nc, char *old, char *new);
extern int nc_fdval(NC *nc, char *label);
extern int nc_muxval(NC *nc, char *label);
extern int nc_shell(NC *nc, char *cmd);

/* XXX extern char *nc_error(int, char *); */
extern int nc_lookup_error(char *s);
extern char *nc_error_name(int error);
extern char *nc_error_string(int error);

/* XXX diagnostic variables */

extern int nc_trace;			/* trace protocol/build sequence */
extern int nc_dummy;			/* don't send requests/build streams */

#endif /* _nc_h */
