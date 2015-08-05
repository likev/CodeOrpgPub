/*   @(#) dnetd.c 00/01/05 Version 1.10   */

/*
 *  STREAMS Network Daemon
 *
 *  Copyright (c) 1988-1997 Spider Software Limited
 *
 *  All Rights Reserved.
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  Made in Scotland.
 *
 *  @(#)$Id: dnetd.c,v 1.2 2000/07/14 15:42:32 jing Exp $
 */
 
/*
Modification history:
 
Chg Date       Init Description
1.   4-jun-98  rjp  Change strioctl to xstrioctl.
2.  12-AUG-98  mpb  Compile on Windows NT (WINNT).
3.  18-AUG-98  mpb  Took out prototype of exit_program().  It is done in mps.h
4.  08-oct-99  tdg  Added VxWorks support.
5.  28-oct-99  tdg  Added call to netd_delete_all_labels() for cleanup
*/
 



#define DEBUG
int	debug=1;
/* int	debug=0; */

#include <stdlib.h>
#ifdef USE_VARARGS
#include <varargs.h>
#else /* USE_VARARGS */
#include <stdarg.h>
#endif /* USE_VARARGS */
#include <stdio.h>
#ifdef POSIX
#ifndef SUNOS5
#include <unistd.h>
#endif /* SUNOS5 */
#include <fcntl.h>
#endif
#ifdef SVR4
#include <fcntl.h>
#endif /* SVR4 */

#ifndef WINNT
#ifdef VXWORKS
#include <fcntl.h>
#include <signal.h>
#include <streams/stream.h>
#else
#include <sys/fcntl.h>
#include <sys/signal.h>
#include <sys/stream.h>
#endif /* VXWORKS */
#else
typedef char *    caddr_t;
#include <winsock.h>
int    getopt ( int, char *const *, const char * );
#endif /* ! WINNT */

#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <debug.h>
#include <ipc.h>
#include <nc.h>
#include <streamio.h>


#include "xstopts.h"
#include "mpsproto.h"

#include "mps.h"
#include "label.h"

#include <sys/snet/uint.h>
#include <sys/snet/ll_proto.h>
#include <sys/snet/ll_control.h>
#include <sys/snet/mlp_proto.h>
#include <sys/snet/mlp_control.h>
#include <sys/snet/x25_proto.h>
#include <sys/snet/x25_control.h>

#ifdef NEED_STRERROR
extern char *sys_errlist[];
extern int sys_nerr;

#define strerror(err) ((err) < sys_nerr? sys_errlist[(err)] : "Unknown error")
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#define OFLAGS(s)    	(strcmp((flags), "r") == 0? O_RDONLY : \
            		(strcmp((flags), "w") == 0? O_WRONLY : \
            		(strcmp((flags), "rw") == 0)? O_RDWR : \
                	(-1)))

#define TYPE(qual) \
((strcmp((qual), "FD") == 0 || strcmp((qual), "fd") == 0)? NETD_FDESC_TYPE : \
(strcmp((qual), "MUX") == 0 || strcmp((qual), "mux") == 0)? NETD_MUXID_TYPE : \
    (-1))

static QID netd_queue;                /* server queue handle */
static IPC *netd_request;            /* current request */
static char netd_message[NC_MAX_MESSAGE+1];    /* message buffer */

#ifdef DEBUG
int netd_debug = 1;
#endif

extern char netd_version[];
extern char netd_copyright[];

static int process_request(char *);
#ifdef USE_VARARGS
static int reply();
#else /* USE_VARARGS */
static int reply(char *format, ...);
#endif /* USE_VARARGS */

static char *current_command;

static void set_command(char *);
static int get_command_string(char *);
static unsigned char get_command_byte(void);


#ifndef WINNT
static void terminate(int);
#ifdef VXWORKS
static void exit_program();
extern int pt_system(char *);
#else
static void daemon();
extern  void    signal();
#endif /* VXWORKS */
#endif /* ! WINNT */


extern  void    delete_lower_ref(char *);

static int  forever=1;

/*************************************************************************
* main
*
*************************************************************************/

#ifdef VXWORKS
int x25dnetd (int argc, char *argv[], pti_TaskGlobals *ptg)
#else
int main(int argc, char *argv[])
#endif /* VXWORKS */
{
    char *server;

#ifdef WINNT
    LPSTR        lpMsg;
    WSADATA      WSAData;
    int          optionValue = SO_SYNCHRONOUS_NONALERT;
#endif    /* WINNT */

#ifdef    WINNT         /* #4 */
/* For NT, need to initialize the Windows Socket DLL */
    if ( WSAStartup ( 0x0101, &WSAData) )
    {
        FormatMessage (
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, WSAGetLastError ( ), LANG_USER_DEFAULT,
            ( LPSTR ) &lpMsg, 0, NULL );
        fprintf ( stderr, "\n\nWSAStartup() failed:\n%s\n", lpMsg );

        LocalFree ( lpMsg );
        exit ( -1 );
    }
#endif    /* WINNT */

#ifdef VXWORKS
    init_mps_thread (ptg);
    if (!debug)
      {
        semGive ( ptg->tskSem );
        ptg->tskSem = NULL;
      }
#endif /* VXWORKS */

#if defined ( DEBUG ) || defined ( WINNT ) || defined ( VXWORKS )
    printf("%s\n%s\n", netd_version, netd_copyright);

#else /* DEBUG || WINNT || VXWORKS */
    /*
     * Run as a daemon.
     */

    daemon();
#endif /* DEBUG || WINNT || VXWORKS */

    TRACE(("Setting signal handlers...\n"));

#ifndef WINNT
#ifdef VXWORKS
    signal(SIGUSR1, terminate);
#else
    signal(SIGHUP, terminate);
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);
#endif /* VXWORKS */
#endif /* WINNT */

    /*
     * Create a server queue on which to accept client requests.
     */
    server = ipc_queue_name(NETD_SERVER_VARIABLE, NETD_SERVER_DEFAULT);

    TRACE(("[NETD] Binding IPC server \"%s\"...\n", server));

    if ((netd_queue = ipc_attach(server)) == QID_FAIL)
        S_EXIT(EXIT_FAILURE);

    /*
     * Process client requests.
     */
    while (forever)
    {
        TRACE(("----------------------------------------\n"));

        if ((netd_request = ipc_receive(netd_queue,
            netd_message, sizeof(netd_message))) == IPC_FAIL)
        {
            ipc_detach(netd_queue);
            S_EXIT(EXIT_FAILURE);
        }

        TRACE(("%s", netd_message));

        (void) process_request(netd_message);
    }
    return (0);
} /* end main() */


#if !defined ( WINNT ) && !defined ( VXWORKS )
/*************************************************************************
* daemon
*
* To disable the daemon processing compile application with -DDEBUG.
*
*************************************************************************/
static void
daemon()
{
    /*
    * Detach ourselves from the controlling tty...
    */
    setpgrp();
#if 0
    fclose(stdin);        /* finished with input */
    fclose(stdout);        /* bin this too */
#endif
    if (S_FORK()) S_EXIT(0);
} /* end daemon() */
#endif /* !WINNT && !VXWORKS */


#define ok()            	return reply("%s", "OK\n")
#define ok_length(length)    	return reply("OK %d\n", (length))
#define ok_version(version)    	return reply("OK %d\n", (version))
#define ok_label(label)        	return reply("OK %s\n", (label))
#define ok_value(value)        	return reply("OK %d\n", (value))

#define err_proto()        	return reply("%s", "ERR PROTOCOL\n")
#define err_server(msg)        	return reply("ERR SERVER %s\n", (msg))
#define err_user()        	return reply("%s", "ERR USER\n")
#define err_label(label)    	return reply("ERR LABEL %s\n", (label))

/*************************************************************************
* process_request
*
*************************************************************************/ 
static int
process_request(char *request)
{
    char 	command[NC_MAX_MESSAGE+1];
    char 	*s;

    /*
     * Set up the command parser, and get the first word,
     * i.e. the command name.
     */

    set_command(request);

    if ( ! get_command_string(command)) {
        TRACE(("[NETD] empty request\n"));
        err_proto();
    }

    /* convert to lower case for case insensitive match */

    for (s = command; *s != '\0'; s++)
        *s = tolower(*s);

    /*
     * --> VERSION
     * <-- OK <version> | ERR ...
     */

    if (strcmp(command, "version") == 0)
        ok_version(NC_PROTOCOL);

    /*
     * --> OPEN <device> <flags> <label>
     * <-- OK <label> | ERR ...
     */

    if (strcmp(command, "open") == 0)
    {
        char 		serverName	[NC_MAX_MESSAGE+1];
        char 		serviceName	[NC_MAX_MESSAGE+1];
        char 		ctlrNumber 	[NC_MAX_MESSAGE+1];
        char 		protoName     	[NC_MAX_MESSAGE+1];
        char 		device        	[NC_MAX_MESSAGE+1];
	char 		label         	[NC_MAX_MESSAGE+1];
	char  		*new_label;
        int 		fd;
  	UCONX_DEVICE	uconx_device;

        new_label = label;
        if ( ! get_command_string(serverName))
            err_proto();
        if ( ! get_command_string(serviceName))
            err_proto();
        if ( ! get_command_string(ctlrNumber))
            err_proto();
        if ( ! get_command_string(protoName))
            err_proto();
        if ( ! get_command_string(device))
            err_proto();
        if ( ! get_command_string(label))
            err_proto();
        if (strcmp(label, "ANY") == 0
	||  strcmp(label, "any") == 0)
            new_label = netd_generate_label();
        else
	    if ((fd = netd_lookup_label(label, NETD_FDESC_TYPE)) != -1)
                err_label(label);
	/*
 	* Init device structure with caller's parameters.
 	*/
	if (uconx_device_create (&uconx_device, serverName, serviceName,
		ctlrNumber, protoName, device) == -1) /* Failed		*/
            err_server(strerror(errno));

        TRACE(("[NETD] open(\"%s\", %d)\n", uconx_device.serverName,
		uconx_device.device));
        if ((fd = doOpen (&uconx_device)) == -1)
	{
            TRACE(("[NETD] => FAIL errno=%d\n", errno));
            err_server(strerror(errno));
        }
        TRACE(("[NETD] => %d (%s)\n", fd, new_label));

        if (netd_store_label(new_label, fd, &uconx_device, "",
                NETD_FDESC_TYPE, "") == -1)
            err_label(new_label);
        ok_label(new_label);
    }

    /*
     * --> CLOSE <stream>
     * <-- OK
     */

    if (strcmp(command, "close") == 0)
    {
        char stream[NC_MAX_MESSAGE+1];
        int fd;
        
        if ( ! get_command_string(stream))
            err_proto();

        TRACE(("[NETD] close(%s)\n", stream));

        if ((fd = netd_lookup_label(stream, NETD_FDESC_TYPE)) == -1)
            err_label(stream);
	if (MPSclose(fd) == -1) {
            TRACE(("[NETD] => FAIL errno=%d\n", errno));
            err_server(strerror(errno));
        }
        TRACE(("[NETD] => OK\n"));

        (void) netd_delete_label(stream);

        ok();
    }

    /*
     * --> PUSH <stream> <module> <label>
     * <-- OK | ERR ...
     */

    if (strcmp(command, "push") == 0)
    {
        char		stream[NC_MAX_MESSAGE+1];
 	char		module[NC_MAX_MESSAGE+1];
        UCONX_DEVICE	*pControl_label;
        int fd;
        
        if ( ! get_command_string(stream))
            err_proto();

        if ( ! get_command_string(module))
            err_proto();

        TRACE(("[NETD] push(%s, \"%s\")\n", stream, module));

        if (pControl_label = netd_control_label(stream))
        {
            TRACE(("[NETD] open(%s, %s)\n", pControl_label->serverName,
		    pControl_label->protoName));
            if ((fd = doOpen (pControl_label)) == -1)

            {
                TRACE(("[NETD] => FAIL errno=%d\n", errno));
                err_server(strerror(errno));
            }
        }
        else if ((fd = netd_lookup_label(stream,NETD_FDESC_TYPE)) == -1)
            err_label(stream);
        if (MPSioctl(fd, I_PUSH, module) == -1) {
            TRACE(("[NETD] => FAIL errno=%d\n", errno));
            err_server(strerror(errno));
        }
        TRACE(("[NETD] => OK\n"));

        ok();
    }

    /*
     * --> POP <stream>
     * <-- OK | ERR ...
     */

    if (strcmp(command, "pop") == 0)
    {
        char stream[NC_MAX_MESSAGE+1];
        int fd;
        
        if ( ! get_command_string(stream))
            err_proto();
        
        TRACE(("[NETD] pop(%s)\n", stream));

        if ((fd = netd_lookup_label(stream, NETD_FDESC_TYPE)) == -1)
            err_label(stream);
        if (MPSioctl(fd, I_POP,0) == -1) {
            TRACE(("[NETD] => FAIL errno=%d\n", errno));
            err_server(strerror(errno));
        }
        TRACE(("[NETD] => OK\n"));

        ok();
    }

    /*
     * --> LINK <upper> <lower> <label>
     * <-- OK <label> | ERR ...
     */

    if (strcmp(command, "link") == 0)
    {
        char 		upper[128];
	char 		lower[128];
	char 		label[128];
        UCONX_DEVICE	*pControl_label;
	char 		*new_label;
        int 		fd1, fd2, mux;
        
        if ( ! get_command_string(upper))
            err_proto();
        
        if ( ! get_command_string(lower))
            err_proto();
        
        if ( ! get_command_string(label))
            err_proto();

        new_label = label;

        if (strcmp(label, "ANY") == 0 || strcmp(label, "any") == 0)
            new_label = netd_generate_label();

        if ((fd1 = netd_lookup_label(upper, NETD_FDESC_TYPE)) == -1)
            err_label(upper);

        netd_mark_control_label(upper);

        if (pControl_label = netd_control_label(lower))
        {
            TRACE(("[NETD] open(%s, %s)\n", pControl_label->serverName,
                    pControl_label->protoName));
            if ((fd2 = doOpen (pControl_label)) == -1)
            {
                TRACE(("[NETD] => FAIL errno=%d\n", errno));
                err_server(strerror(errno));
            }
        }
        else
	{
	    if ((fd2 = netd_lookup_label(lower,NETD_FDESC_TYPE)) == -1)
                err_label(lower);
	}
        if ((mux = MPSioctl(fd1, I_LINK, fd2)) == -1) {
            TRACE(("[NETD] => FAIL errno=%d\n", errno));
            err_server(strerror(errno));
        }
        TRACE(("[NETD] link(%s, %s)\n", upper, lower));
        TRACE(("[NETD] close(%s)\n",lower));
        MPSclose(fd2);
        if ( ! pControl_label)
        {
            netd_delete_label(lower);
            lower[0] = '\0';
        }
        TRACE(("[NETD] => %d (%s)\n", mux, new_label));

        if (netd_store_label(new_label, mux, (UCONX_DEVICE *) 0, lower,
                     NETD_MUXID_TYPE, upper) == -1)
            err_label(new_label);

        ok_label(new_label);
    }

    /*
     * --> UNLINK <stream> ( <mux> | ALL )
     * <-- OK | ERR ...
     */

    if (strcmp(command, "unlink") == 0)
    {
        char stream[NC_MAX_MESSAGE+1], label[NC_MAX_MESSAGE+1],
             *lower;
        int fd, mux;
        
        if ( ! get_command_string(stream))
            err_proto();
        
        if ( ! get_command_string(label))
            err_proto();
        
        TRACE(("[NETD] unlink(%s, %s)\n", stream, label));

        if ((fd = netd_lookup_label(stream, NETD_FDESC_TYPE)) == -1)
            err_label(stream);

        if (strcmp(label, "ALL") == 0 || strcmp(label, "all") == 0)
            mux = MUXID_ALL;
        else if ((mux = netd_lookup_label(label, NETD_MUXID_TYPE)) == -1)
            err_label(label);
        if (MPSioctl(fd, I_UNLINK, mux) == -1) {
            TRACE(("[NETD] => FAIL errno=%d\n", errno));
            err_server(strerror(errno));
        }
        TRACE(("[NETD] => OK\n"));

        /*
        * If the lower stream is not a control stream then close
        * it and free the label.
        */
        if (lower = get_lower_label(label))
        {
            if (fd = netd_lookup_label(lower, NETD_FDESC_TYPE))
                MPSclose(fd);
            delete_lower_ref(label);
        }

        ok();
    }

    /*
     * --> STRIOC <stream> <ioctl> <timeout> <length>
     * --> <data>
     * --> END | ABORT ...
     * <-- OK <length> || ERR ...
     * <-- <data>
     * <-- END | ABORT ...
     */

    if (strcmp(command, "strioc") == 0)
    {
        char	stream [NC_MAX_MESSAGE+1];
	char	cmd [NC_MAX_MESSAGE+1];
	char	tmout [NC_MAX_MESSAGE+1];
	char	length [NC_MAX_MESSAGE+1];
        int 	fd, mux;
        struct xstrioctl ioc;		/* #1 strioctl xstrioctl	*/
        int 	len;
        char 	*p, *dptr;
        
        if ( ! get_command_string(stream))
            err_proto();
        
        if ( ! get_command_string(cmd))
            err_proto();
        
        if ( ! get_command_string(tmout))
            err_proto();
        
        if ( ! get_command_string(length))
            err_proto();

        /*
        * Firstly we assume that the label is a muxid. If this fails
        * then go for the default of fd.
        *
        * If it succeeds we get the muxid returned, but we also need
        * the fd. Hence do search again but ask for fd by changing type.
        */
        mux = netd_lookup_label(stream, NETD_MUXID_TYPE);

        if ((fd = netd_lookup_label(stream, NETD_FDESC_TYPE)) == -1)
            err_label(stream);

        ioc.ic_cmd = atoi(cmd);
        ioc.ic_timout = atoi(tmout);
        ioc.ic_len = atoi(length);

        if ((len = ioc.ic_len) > 0)
        {
            if ((ioc.ic_dp = malloc(ioc.ic_len)) == NULL)
                err_server("out of memory");

            dptr = ioc.ic_dp;

            while (len-- > 0)
	    {
                *dptr++ = get_command_byte();
	    }
        }
        else
            ioc.ic_dp = NULL;

        TRACE(("[NETD] strioc(%s, %s, %s, %s)\n",
            stream, cmd, tmout, length));

        if (MPSioctl(fd, I_STR, &ioc) == -1) {
            TRACE(("[NETD] => FAIL errno=%d\n", errno));
            if (ioc.ic_dp)
                free(ioc.ic_dp);
            err_server(strerror(errno));
        }

        TRACE(("[NETD] => OK (%d bytes)\n", ioc.ic_len));

        /* check for simply case, no returned data */

        if (ioc.ic_len == 0)
            ok_length(0);

        /* compose a reply with data */

        p = netd_message;

#ifndef NON_ANSI_SPRINTF
        p += sprintf(p, "OK %d\n", ioc.ic_len);
#else
        (void) sprintf(p, "OK %d\n", ioc.ic_len);
        p += strlen(p);
#endif

        dptr = ioc.ic_dp;

        while (ioc.ic_len-- > 0)
        {
#ifndef NON_ANSI_SPRINTF
            p += sprintf(p, "%02x", (unsigned int)*dptr++);
#else
            (void) sprintf(p, "%02x", (unsigned int)*dptr++);
            p += strlen(p);
#endif
        }

        sprintf(p, "\nEND\n");

        free(ioc.ic_dp);
	
       (void) ipc_reply(netd_request,
                netd_message, strlen(netd_message)+1);

        return 0;
    }

    /*
     * --> SHELL <command>
     * <-- OK | ERR ...
     */

    if (strcmp(command, "shell") == 0)
    {
        int stat;

        TRACE(("[NETD] shell(\"%s\")\n", current_command));

#ifdef VXWORKS
        if (stat = pt_system(current_command))
#else
        if (stat = system(current_command))
#endif /* VXWORKS */
            err_user();

        TRACE(("[NETD] => OK\n"));

        ok();
    }

    /*
     * --> RENAME <old> <new>
     * <-- OK | ERR ...
     */

    if (strcmp(command, "rename") == 0)
    {
        char old[NC_MAX_MESSAGE+1], new[NC_MAX_MESSAGE+1];
        
        if ( ! get_command_string(old))
            err_proto();
        
        if ( ! get_command_string(new))
            err_proto();

        TRACE(("[NETD] new(%s, %s)\n", old, new));

        if (netd_rename_label(old, new) != 0)
            err_label(new);

        ok();
    }

    /*
     * --> ALIAS <label> <alias>
     * <-- OK | ERR ...
     */

    if (strcmp(command, "alias") == 0)
    {
        char label[NC_MAX_MESSAGE+1], alias[NC_MAX_MESSAGE+1];
        
        if ( ! get_command_string(label))
            err_proto();
        
        if ( ! get_command_string(alias))
            err_proto();

        TRACE(("[NETD] alias(%s, %s)\n", label, alias));

        if (netd_copy_label(label, alias) != 0)
            err_label(alias);

        ok();
    }

    /*
     * --> DELETE <label>
     * <-- OK | ERR ...
     */

    if (strcmp(command, "delete") == 0)
    {
        char label[NC_MAX_MESSAGE+1];
        
        if ( ! get_command_string(label))
            err_proto();

        TRACE(("[NETD] delete(%s)\n", label));

        if (netd_delete_label(label) == -1)
            err_label(label);

        ok();
    }

    /*
     * --> VALUE <label> ( FD | MUX )
     * <-- OK <value> | ERR ...
     */

    if (strcmp(command, "value") == 0)
    {
        char label[NC_MAX_MESSAGE+1], qual[NC_MAX_MESSAGE+1];
        int type, value;

        if ( ! get_command_string(label))
            err_proto();

        if ( ! get_command_string(qual))
            err_proto();

        if ((type = TYPE(qual)) == -1)
            err_user();

        if ((value = netd_lookup_label(label, type)) == -1)
            err_label(label);

        ok_value(value);
    }

    /*
     * --> DIAG labels | ...
     * <-- OK <n> | ERR ...
     * <-- ( FD | MUX ) <label> <value>
     * <--  ...
     * <-- END | ABORT ...
     */

    if (strcmp(command, "diag") == 0)
    {
        char what[NC_MAX_MESSAGE+1];

        if ( ! get_command_string(what))
            err_user();

        if (strcmp(what, "labels") == 0)
            /* ... send formatted label table ... */;
        else
            err_user();

        ok();
    }

#ifdef DEBUG
    /*
     * --> PRINT labels | ...
     * <-- OK | ERR ...
     */

    if (strcmp(command, "print") == 0)
    {
        char what[NC_MAX_MESSAGE+1];

        if ( ! get_command_string(what))
            err_user();

        if (strcmp(what, "labels") == 0) {
            netd_print_labels();
            ok();
        }

        err_user();
    }
#endif

    /*
     * --> SHUTDOWN
     * <-- OK | ERR ...
     */

    if (strcmp(command, "shutdown") == 0)
    {
        TRACE(("NOTICE: netd shutting down on client request!\n"));
        reply("OK\n");
        ipc_detach(netd_queue);
	netd_delete_all_labels(); /* #5 */
        S_EXIT(EXIT_SUCCESS);
    }

    TRACE(("unknown command: %s\n", command));

    err_proto();
} /* end process_request() */

/*************************************************************************
* reply
*
*************************************************************************/
static int
#ifdef USE_VARARGS
reply(va_alist) va_dcl
{
    va_list args;
    char *format;

    va_start(args);

    format = va_arg(args, char *);
    (void)vsprintf(netd_message, format, args);
    va_end(args);
#else /* USE_VARARGS */
reply(char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf(netd_message, format, args);
    va_end(args);
#endif /* USE_VARARGS */

    TRACE(("%s", netd_message));

    if (ipc_reply(netd_request, netd_message, strlen(netd_message)+1) == -1)
    {
        ipc_detach(netd_queue);
        S_EXIT(EXIT_FAILURE);
    }

    return 0;
} /* end reply() */

/*************************************************************************
* set_command
*
*************************************************************************/
static void
set_command(char *s)
{
    current_command = s;
} /* end set_command() */

/*************************************************************************
* get_command_string
*
*************************************************************************/
static int
get_command_string(char *dptr)
{
    char *s, *p;

    /* skip whitespace */

    while (*current_command == ' ')
        ++current_command;

    if ((s = strpbrk(current_command, " \r\n")) == NULL)
        return 0;

    *s++ = '\0';

    p = s;
    s = current_command;
    current_command = p;

    (void)strcpy(dptr, s);

    return ((int)s);
} /* end get_command_string() */

/*************************************************************************
* get_command_byte
*
*************************************************************************/
static unsigned char
get_command_byte(void)
{
    unsigned int byte;

    /* XXX catch premature end of data (ABORT), check for valid END */

    if (current_command[0] && current_command[1])
    {
        sscanf(current_command, "%02x", &byte);
        current_command += 2;
        return (char) byte;
    }

    return '\0';
} /* end get_command_byte() */

#ifndef WINNT
/*************************************************************************
* terminate
*
*************************************************************************/
static void
terminate(int sig)
{
    TRACE(("\nOuch!\n"));

    ipc_detach(netd_queue);
 
    netd_delete_all_labels(); /* #5 */

    S_EXIT(EXIT_FAILURE);
} /* end terminate() */
#endif /* ! WINNT */



/**********

exit_program() ---

Instead of just exiting, clean up system before exit.

**********/
#ifdef    ANSI_C
void exit_program ( int error_val )
#else
void exit_program ( error_val )
int error_val;
#endif	/* ANSI_C */
{
#ifdef	WINNT
   LPSTR lpMsg;
#endif	/* WINNT */

#ifdef	WINNT
   if ( WSACleanup ( ) == SOCKET_ERROR )
   {
      FormatMessage ( 
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL, WSAGetLastError ( ), LANG_USER_DEFAULT, 
         ( LPSTR ) &lpMsg, 0, NULL );

      fprintf ( stderr, "\n\nWSACleanup() failed:\n%s\n", lpMsg );

      LocalFree ( lpMsg );
   }
#endif	/* WINNT */

#ifdef VXWORKS
   cleanup_mps_thread ();
#endif /* VXWORKS */

   exit ( error_val );

} /* end exit_program() */
