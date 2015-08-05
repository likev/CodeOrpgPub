static char sccsid[] = "@(#)nc_client.c	1.3	09/04/98";
/*   @(#) nc_client.c 00/04/11 Version 1.7   */
/*
 *  STREAMS Network Control Library - client implementation
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
 *  @(#)$Id: nc_client.c,v 1.3 2000/07/14 15:57:54 jing Exp $
 */

/* 
Modification history:
Chg Date	Init Description
 1.  6-jul-98   rjp  Alan Robertson of Spider provided these fixes.
 2. 18-aug-98   rjp  Convert returned data from ascii to binary.
 3. 14-sep-98   rjp  Added NC_ERR_LABEL to nc_default_error_handler.

*/

#include <stdlib.h>
#ifdef USE_VARARGS
#include <varargs.h>
#else /* USE_VARARGS */
#include <stdarg.h>
#endif /* USE_VARARGS */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if 0
#ifdef POSIX
#include <unistd.h>
#endif
#endif

#ifdef WINNT
#include <winsock.h>
#endif /* WINNT */

#include <xstopts.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <nc.h>

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

int nc_trace = 0;

int nc_dummy = 0;
#ifndef DEBUG
int nc_debug = 0;
#else
int nc_debug = 1;
#endif

struct nc {
    char    *server;        /* name of server */
    QID    queue;            /* queue connected to server */
    int    error;            /* indicates error on connection */
    nc_handler *error_handler;    /* error handler for connection */
    char    buf[NC_MAX_MESSAGE+1];    /* request buffer */
    char    *ptr;            /* current read/write offset in buf */
    struct nc *next;        /* next active connection */
};

static struct nc *nc_servers = NULL;    /* list of active connections */

static char *nc_progname = NULL;

static void nc_default_error_handler(int, char *, struct nc *, char *);
static void nc_call_error_handler(int, char *, struct nc *, char *);
#ifdef USE_VARARGS
static int nc_varargs(va_alist);
#else /* USE_VARARGS */
static int nc_varargs(int (*func)(), struct nc * nc, char * format, ...);
#endif /* USE_VARARGS */
int nc_send(struct nc *);

static nc_handler *nc_error_handler = nc_default_error_handler;


/*
 * If varargs is all that is available then we need to do the following ...
 */
static int
#ifdef USE_VARARGS
nc_varargs(va_alist) va_dcl
{
    va_list args;
    struct nc * nc;
    char * format;
    int * (*func)();

    va_start(args);

    func = (int *(*)())va_arg(args, int *);
    nc = (struct nc *)va_arg(args, struct nc *);
    format = va_arg(args, char *);
    (void)vsprintf(nc->buf, format, args);

    va_end(args);
    return((int)(*func)(nc));
}
#else /* USE_VARARGS */
nc_varargs(int (*func)(), struct nc * nc, char * format, ...)
{
    va_list args;

    va_start(args, format);
    (void)vsprintf(nc->buf, format, args);

    va_end(args);
    return((int)(*func)(nc));
}
#endif /* USE_VARARGS */

void
nc_set_program_name(char *progname)
{
    ipc_set_program_name(nc_progname = progname);
}

void
nc_set_default_error_handler(nc_handler *handler)
{
    nc_error_handler = (handler? handler : nc_default_error_handler);
}

void
nc_set_error_handler(struct nc *nc, nc_handler handler)
{
    nc->error_handler = (handler? handler : nc_default_error_handler);
}

static void
nc_default_error_handler(int error, char *progname, struct nc *nc,
             char *err_str)
{
#if 0
    int saved_errno = errno;
#endif

    if (progname)
        fprintf(stderr, "%s: ", progname);

    switch (error)
    {
    case NC_ERR_LABEL:
        fprintf(stderr, "invalid label %s",err_str);
        break;
    case NC_ERR_MEMORY:
        fprintf(stderr, "can't allocate %s",err_str);
        break;
#if 0
    case NC_ERR_SYSTEM:
        fprintf(stderr, "%s : %s", err_str, strerror(saved_errno));
        break;
#endif
    case NC_ERR_CONNECT:
        /* XXX ipc library also prints a message */
        fprintf(stderr, "%s", err_str);
        break;
    default:
        fprintf(stderr, "%s", err_str);
        break;
    }

    fprintf(stderr, "\n");

#ifdef ABORT_ON_PROTO_ERROR
    nc_terminate(NULL);
    exit(EXIT_FAILURE);
#endif /* ABORT_ON_PROTO_ERROR */
}

static void
#ifdef USE_VARARGS
nc_handle_error(va_alist) va_dcl
{
    va_list    args;
    struct nc * nc;
    char * format;
    char err_str[128];
    int err;

    va_start(args);

    nc = (struct nc *)va_arg(args, struct nc *);
    err = (int)va_arg(args, int *);
    format = va_arg(args, char *);
#else /* USE_VARARGS */
nc_handle_error(struct nc * nc, int err, char * format, ...)
{
    va_list args;
    char err_str[128];

    va_start(args, format);
#endif /* USE_VARARGS */
    (void)vsprintf(&err_str[0], format, args);
    va_end(args);

    nc_call_error_handler(err, nc_progname, nc, err_str);
}

static void
nc_call_error_handler(int error, char *progname, struct nc *nc, char *err_str)
{
    nc_handler *handler;

    if (nc)
        nc->error = error;

    if (nc && nc->error_handler)
        handler = nc->error_handler;
    else
        handler = nc_error_handler;

    (*handler)(error, progname, nc, err_str);
}

/*
 * nc_initialise() -- make a connection to a network control daemon (netd)
 *
 * Normal operation is just to connect to the default (only) daemon, but
 * in some environments a single management entity might control several
 * daemons (e.g. on remote systems/front end processors).
 */

struct nc *
nc_initialise(char *server)
{
    struct nc *nc;

    if (server == NULL)
        server = nc_default_server();

    if ((nc = malloc(sizeof(struct nc))) == NULL) {
        nc_handle_error(NULL, NC_ERR_MEMORY, "%s",
                 "connection structure");
        return NULL;
    }

    /*
     * Store the server name for future reference (for example, in
     * formatting error messages).
     *
     * On systems with a global address space, we must identify
     * each connection in the global list as belonging to this
     * process/task.
     */

#ifdef VXWORKS
    nc->server = malloc(strlen(server)+1);
    strcpy(nc->server, server);
#else
    nc->server = strdup(server);
#endif /* VXWORKS */
    nc->error = 0;
    nc->error_handler = NULL;

    if ( ! nc_dummy)
    {
        if ((nc->queue = ipc_connect(server)) == QID_FAIL) {
            nc_handle_error(nc, NC_ERR_CONNECT, "%s",
                     "can't connect to server");
#ifndef VXWORKS
	    free (nc);			/* #1.				*/
#endif
	    return (NULL);		/* #1. 				*/
        }

        TRACE(("[NC] connect - %s\n", server? server : "<DEFAULT>"));
    }

    /* initialise the buffer pointer for "output" */

    nc->ptr = nc->buf;

    /* simply prepend to global list of connections */

    nc->next = nc_servers;
    nc_servers = nc;

    return nc;
}

/*
 * nc_terminate() -- terminate specified connection, or all
 *
 * Clients should normally use nc_terminate() for each connection;
 * in case of emergency, specifing a NULL connection structure will
 * terminate all of this process's connections.
 *
 * Take special care here on systems with a global address space (i.e.
 * where the connection list is shared among all clients).
 */

void
nc_terminate(struct nc *nc)
{
    struct nc *nct = nc_servers, *ncp = NULL;

    TRACE(("[NC] terminate - %s\n", nc? nc->server : "<ALL>"));

    /*
     * Since the list of connections is only singly linked, we
     * have to grub around looking for the preceding entry in
     * order to remove this one.
     *
     * It's not worth writing a special (simplified) case for
     * removing all entries (nc == NULL), due to having to deal
     * with a shared list on a system with a global name space.
     */

    while (nct != NULL)
    {
        if (nct != nc) {    /* can't be NULL */
            nct = nct->next;
            continue;
        }


        /*
         * Close down the IPC layer.
         */

        if (!nc_dummy)
            ipc_terminate(nc->queue);

        /*
         * Remove the structure from the list, and free it.
         */

        if (nct == nc_servers) {
            nc_servers = nct->next;
            ncp = NULL;
        } else {
            ncp->next = nct->next;
            ncp = nct;
        }

        ncp = nct->next;

        free(nct->server); 
        free(nct);

        if (nc)            /* there can only be one */
            return;

        nct = ncp->next;
    }

    if (nc)
        TRACE(("nc %p invalid\n", nc));

    /*
     * If no connections remain for this client, do any necessary
     * IPC layer cleanup.
     */

    if (!nc_dummy && nc_servers == NULL)
        ipc_cleanup();
}

/*
 * nc_reset() -- reset the data pointer for a request buffer
 */

void
nc_reset(struct nc *nc)
{
    nc->ptr = nc->buf;
}

/*
 * nc_write() -- format message and append to request buffer
 */

int
#ifdef USE_VARARGS
nc_write(va_alist) va_dcl
{
    va_list    args;
    int n;
    struct nc *nc;
    char *format;

    va_start(args);

    nc = (struct nc *)va_arg(args, struct nc *);
    format = va_arg(args, char *);
#else /* USE_VARARGS */
nc_write(struct nc *nc, char *format, ...)
{
    va_list args;
    int n;

    va_start(args, format);
#endif /* USE_VARARGS */
#ifndef NON_ANSI_SPRINTF
    n = vsprintf(nc->ptr, format, args);
#else
    (void) vsprintf(nc->ptr, format, args);
    n = strlen(nc->ptr);
#endif
    va_end(args);

    nc->ptr += n;

    return n;
}

/*
 * nc_read() -- read returned data into user buffer
 */

int
nc_read(struct nc *nc, char *buf, int len)
{
    unsigned    left;                   /* #2                           */
    unsigned    right;                  /* #2                           */
    int n = 0;
    char *lim = nc->buf + NC_MAX_MESSAGE + 1;

#if 0					/* #2				*/
    while (len-- && nc->ptr < lim) {
        *buf++ = *nc->ptr++;
        n++;
    }
#else					/* #2 				*/
    if (((len/2)*2) != len)		/* If len is odd		*/
    {
	*(nc->ptr+2*len) = '0';		/* Pad with zeros		*/
	*(nc->ptr+2*len+1) = '0';	/* Pad with zeros		*/
	len++;				/* Make it even			*/
    }
    len = len/2;
    
    while (len-- && nc->ptr < lim)
    {
        if (*nc->ptr <= '9')
            left = (unsigned) (*nc->ptr - '0');
        else
            if (*nc->ptr <= 'F')
                left = 10 + (unsigned) (*nc->ptr - 'A');
            else
                left = 10 + (unsigned) (*nc->ptr - 'a');
        nc->ptr++;                      /* Next char                    */
        if (*nc->ptr <= '9')
            right = (unsigned) (*nc->ptr - '0');
        else
            if (*nc->ptr <= 'F')
                right = 10 + (unsigned) (*nc->ptr - 'A');
            else
                right = 10 + (unsigned) (*nc->ptr - 'a');
        nc->ptr++;                      /* Next char                    */
        *buf = (unsigned char) left*16 + right;
        n++;
        buf++;
    }
#endif

    return n;
}

/*
 * nc_send() -- send a request, wait for response
 */

int
nc_send(struct nc *nc)
{
    if (nc_trace)
        fputs(nc->buf, stdout);

    nc->ptr = nc->buf;        /* reset for reading reply */

    if (nc_dummy)
    {
        return 0;
    }

    /* send the full null-terminated string */
    if (ipc_send(nc->queue, nc->buf, nc->buf,
                strlen(nc->buf)+1, sizeof(nc->buf)) == -1)
    {
        nc_handle_error(nc, NC_ERR_IPC, "%s", "communication error");
        return nc->error;
    }
    if (nc_trace)
        fputs(nc->buf, stdout);
    return 0;
}

/*
 * nc_parse_version() -- parse a response to the VERSION request
 */

int
nc_parse_version(struct nc *nc)
{
    int version;

    nc->ptr = nc->buf;

    if (strncmp(nc->ptr, "OK ", strlen("OK ")) == 0)
    {
        version = atoi(nc->ptr + strlen("OK "));

        while (*nc->ptr++ != '\n')
            ;

        return version;
    }

    (void) nc_parse_error(nc);

    return 0;
}

/*
 * nc_parse_status() -- parse a simple OK/ERR response from the server
 */

int
nc_parse_status(struct nc *nc)
{
    nc->ptr = nc->buf;

    if (nc_dummy)
        return 1;

    /* we'll let anything with an "OK" prefix pass */

    if (strncmp(nc->ptr, "OK", strlen("OK")) == 0)
    {
        while (*nc->ptr++ != '\n')
            ;

        return 1;
    }

    (void) nc_parse_error(nc);

    return 0;
}

/*
 * nc_parse_string() -- parse a returned string (e.g. a label)
 */

char *
nc_parse_string(struct nc *nc)
{
printf("%s-%d: nc_parse_string() string = %s\n",
   __FILE__, __LINE__, nc->buf);
    nc->ptr = nc->buf;

    if (nc_dummy)
        return((char *)1);

    if (strncmp(nc->ptr, "OK", strlen("OK")) == 0)
    {
        char *t, *s = nc->ptr + strlen("OK");

        if (*s == ' ')
        {
printf("%s-%d: nc_parse_string() s = 0x%x\n",
   __FILE__, __LINE__, *s);

            ++s;
printf("%s-%d: nc_parse_string() now s = 0x%x\n",
   __FILE__, __LINE__, *s);
        }

        if ((t = strpbrk(s, " \r\n")) != NULL)
            *t++ = '\0';

        while (*nc->ptr++ != '\n')
            ;

printf("%s-%d: nc_parse_string() returning = 0x%x\n",
   __FILE__, __LINE__, *s);
        return s;
    }

    (void) nc_parse_error(nc);

    return NULL;
}

/*
 * nc_parse_value() -- parse a returned label value
 */

int
nc_parse_value(struct nc *nc)
{
    nc->ptr = nc->buf;

    if (nc_dummy)
        return 0;

    if (strncmp(nc->ptr, "OK ", strlen("OK ")) == 0)
    {
        char *value = nc->ptr + strlen("OK ");

        while (*nc->ptr++ != '\n')
            ;

        if (isdigit(value[0]))
            return atoi(value);

        return -1;
    }

    (void) nc_parse_error(nc);

    return -1;
}

/*
 * nc_parse_error() -- parse an ERR response and flag the error
 *
 * Error responses must include an error code, and optional string.
 */

int
nc_parse_error(struct nc *nc)
{
    char *s, *t;

    if (strncmp(nc->ptr, "ERR", strlen("ERR"))) {
        nc_handle_error(nc, NC_ERR_PROTOCOL, "%s", "invalid response");
        return nc->error;
    }

    if ((s = strchr(nc->ptr, ' ')) == NULL) {
        nc_handle_error(nc, NC_ERR_PROTOCOL, "%s",
                 "missing error code");
        return nc->error;
    }

    *s++ = '\0';

    if ((t = strpbrk(s, " \r\n")) == NULL) {
        nc_handle_error(nc, NC_ERR_PROTOCOL, "%s",
                 "incomplete error response");
        return nc->error;
    }

    *t++ = '\0';

    if ((nc->error = nc_lookup_error(s)) == 0) {
        nc_handle_error(nc, NC_ERR_PROTOCOL, "%s", "unknown error");
        return nc->error;
    }

    nc_handle_error(nc, nc->error, "%s", t);

    return nc->error;
}

/*
 * nc_version() -- return protocol version supported by server
 */

int
nc_version(struct nc *nc)
{
    if (nc_varargs(nc_send, nc, "%s", "VERSION\n"))
        return 0;

    return nc_parse_version(nc);
}

/*
 * nc_open() -- open a STREAMS device in the server
 */

char *
nc_open(struct nc *nc, char *pServerName, char *pServiceName, 
        char *pCtlrNumber, char *pProtoName, char *pDevice, char *label)
{
printf("%s-%d: nc_open()\n",
   __FILE__, __LINE__);

    if (nc_varargs(nc_send, nc, "OPEN %s %s %s %s %s %s\n", pServerName,
	    pServiceName, pCtlrNumber, pProtoName, pDevice,
	    label? label : "ANY"))
    {
printf("%s-%d: nc_open()- bad return from nc_varags()\n",
   __FILE__, __LINE__);
        return NULL;
    }

    if (nc_dummy)
        return (label)? label: pServerName;

printf("%s-%d: nc_open() - calling nc_parse_string()\n",
   __FILE__, __LINE__);
    return nc_parse_string(nc);
}

/*
 * nc_close() - close a stream in the server
 */

int
nc_close(struct nc *nc, char *stream)
{

    if (nc_varargs(nc_send, nc, "CLOSE %s\n", stream))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_push() -- push a STREAMS module on the specified stream
 */

int
nc_push(struct nc *nc, char *stream, char *module)
{
    if (nc_varargs(nc_send, nc, "PUSH %s %s\n", stream, module))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_pop() -- pop the module at the head of the specified stream
 */

int
nc_pop(struct nc *nc, char *stream)
{
    if (nc_varargs(nc_send, nc, "POP %s\n", stream))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_link() -- link one stream under another
 */

char *
nc_link(struct nc *nc, char *upper, char *lower, char *label)
{
    if (nc_varargs(nc_send, nc, "LINK %s %s %s\n", upper, lower,
            label? label : "ANY"))
        return NULL;

    if (nc_dummy)
        return (label)? label: upper;

    return nc_parse_string(nc);
}

/*
 * nc_unlink() -- unlink a previously linked stream
 */

int
nc_unlink(struct nc *nc, char *driver, char *mux)
{
    if (nc_varargs(nc_send, nc, "UNLINK %s %s\n", driver, mux))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_unlink_all() -- unlink ALL streams below a multiplexor
 */

int
nc_unlink_all(struct nc *nc, char *driver)
{
    if (nc_varargs(nc_send, nc, "UNLINK %s ALL\n", driver))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_strioc() -- send an I_STR ioctl down a stream
 *
 * This allows arbitrary data to be sent AND received.  The data is
 * encoded; this encoding scheme is a simple ASCII encoding of the
 * binary data, which must have the same meaning on both the client
 * and server.
 *
 * In the future, hooks may be added to allow portable encoding (ala
 * XDR?).
 *
 * The data in each direction is followed by an END line, in order to
 * facilitate error recovery.  Note that if no data is required in either
 * direction (len == 0), then no END line is sent in that direction.
 */

int
nc_strioc(struct nc *nc, char *stream, int cmd, long tmout, unsigned char *dp, int len)
{
    unsigned char *p = dp;
    int n = len;

    nc_reset(nc);

    /*
     * Write the command line, followed by any data.
     */

    nc_write(nc, "STRIOC %s %d %ld %d\n", stream, cmd, tmout, len);

    while (n--)
    {
       nc_write(nc, "%02x", *p++);
    }
    if (len > 0)
        nc_write(nc, "%s", "\nEND\n");

    /*
     * Send the IPC request, wait for response.
     */

    if (nc_send(nc))
        return -1;

    /*
     * Find out if there's any data to get back.
     * If so, copy it out (user must have supplied enough space.
     */

    n = nc_parse_value(nc);

    /* XXX check for END/ABORT */

    if (n > 0)
        (void) nc_read(nc, (char *)dp, n);

    /*
     * Return the amount of data returned.
     */

    return n;
}

/*
 * nc_shutdown() -- request the network control daemon to terminate
 */

int
nc_shutdown(struct nc *nc)
{
    if (nc_varargs(nc_send, nc, "%s", "SHUTDOWN\n"))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_rename() -- rename an existing label
 */

int
nc_rename(struct nc *nc, char *old, char *new)
{
    if (nc_varargs(nc_send, nc, "RENAME %s %s\n", old, new))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_alias() -- create an alias for an existing label
 */

int
nc_alias(struct nc *nc, char *label, char *alias)
{
    if (nc_varargs(nc_send, nc, "ALIAS %s %s\n", label, alias))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_delete() -- remove an existing label
 *
 * No action is taken with respect to the object referred to by the
 * label, even if no references to it remain.
 */

int
nc_delete(struct nc *nc, char *label)
{
    if (nc_varargs(nc_send, nc, "DELETE %s\n", label))
        return 0;

    return nc_parse_status(nc);
}

/*
 * nc_fdval() -- retrieve value of FD label
 */

int
nc_fdval(struct nc *nc, char *label)
{
    if (nc_varargs(nc_send, nc, "VALUE %s FD\n", label))
        return -1;

    return nc_parse_value(nc);
}

/*
 * nc_muxval() -- retrieve value of MUX label
 */

int
nc_muxval(struct nc *nc, char *label)
{
    if (nc_varargs(nc_send, nc, "VALUE %s MUX\n", label))
        return -1;

    if (nc_dummy)
        return 1;

    return nc_parse_value(nc);
}

/*
 * nc_shell() -- Pass command to be executed by system()
 */

int
nc_shell(struct nc *nc, char *cmd)
{
    if (nc_varargs(nc_send, nc, "SHELL %s\n", cmd))
        return -1;

    return nc_parse_status(nc);
}

