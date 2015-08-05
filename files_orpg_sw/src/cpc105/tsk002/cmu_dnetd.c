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
 *  @(#)$Id: cmu_dnetd.c,v 1.10 2005/12/06 20:31:54 jing Exp $
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
#ifndef LINUX
#include <sys/stream.h>
#endif
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

#include <infr.h>

#include "xstopts.h"
#include "mpsproto.h"

#include "cmu_dnetd_mps.h"
#include "cmu_dnetd_label.h"

#ifdef SUNOS
#include <uint.h>
#include <ll_proto.h>
#include <ll_control.h>
#include <mlp_proto.h>
#include <mlp_control.h>
#include <x25_proto.h>
#include <x25_control.h>
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#define GL_ERROR	((0x20000000) | (LE_CRITICAL))

#define TYPE(qual) \
((strcmp((qual), "FD") == 0 || strcmp((qual), "fd") == 0)? NETD_FDESC_TYPE : \
(strcmp((qual), "MUX") == 0 || strcmp((qual), "mux") == 0)? NETD_MUXID_TYPE : \
    (-1))

static char netd_message[NC_MAX_MESSAGE+1];    /* message buffer */
static char reply_message[NC_MAX_MESSAGE+1];  /* buffer for returning msg */

static int process_request(char *);
static int reply(char *format, ...);

static char *current_command;

static void set_command(char *);
static int get_command_string(char *);
static unsigned char get_command_byte(void);

extern  void    delete_lower_ref(char *);


#define ok()            	return reply("%s", "OK")
#define ok_length(length)    	return reply("OK %d", (length))
#define ok_version(version)    	return reply("OK %d", (version))
#define ok_label(label)        	return reply("OK %s", (label))
#define ok_value(value)        	return reply("OK %d", (value))

#define err_proto()        	return reply("%s", "ERR PROTOCOL")
#define err_server(msg)        	return reply("ERR SERVER %s", (msg))
#define err_user()        	return reply("%s", "ERR USER")
#define err_label(label)    	return reply("ERR LABEL %s", (label))

/*************************************************************************
* process_request. Returns 0 on success or -1 on failure.
*
*************************************************************************/ 
static int
process_request(char *request)
{
    char 	command[NC_MAX_MESSAGE+1];
    char 	*s;

    reply_message[0] = '\0';	/* clean up reply message */

    /*
     * Set up the command parser, and get the first word,
     * i.e. the command name.
     */

    set_command(request);

    if ( ! get_command_string(command)) {
        LE_send_msg (GL_ERROR, "empty request");
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

        LE_send_msg (LE_VL3, "    open server %s device %d", 
		uconx_device.serverName, uconx_device.device);
        if ((fd = doOpen (&uconx_device)) == -1)
	{
            LE_send_msg (GL_ERROR, "doOpen FAIL errno = %d\n", errno);
            err_server(strerror(errno));
        }
        LE_send_msg (LE_VL3, 
		"        open OK fd %d label %s", fd, new_label);

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

        LE_send_msg (LE_VL3, "    close stream %s", stream);

        if ((fd = netd_lookup_label(stream, NETD_FDESC_TYPE)) == -1)
            err_label(stream);
	if (MPSclose(fd) == -1) {
            LE_send_msg (GL_ERROR, "MPSclose FAIL errno = %d", errno);
            err_server(strerror(errno));
        }

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

        LE_send_msg (LE_VL3, "    push stream %s, module %s", stream, module);

        if ((pControl_label = netd_control_label(stream)))
        {
            LE_send_msg (LE_VL3, "    open server %s proto %s", 
		pControl_label->serverName, pControl_label->protoName);
            if ((fd = doOpen (pControl_label)) == -1)

            {
                LE_send_msg (GL_ERROR, "doOpen FAIL errno = %d\n", errno);
                err_server(strerror(errno));
            }
        }
        else if ((fd = netd_lookup_label(stream,NETD_FDESC_TYPE)) == -1)
            err_label(stream);
        if (MPSioctl(fd, I_PUSH, module) == -1) {
            LE_send_msg (GL_ERROR, "MPSioctl FAIL errno = %d\n", errno);
            err_server(strerror(errno));
        }
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
        
        LE_send_msg (LE_VL3, "    pop stream %s", stream);

        if ((fd = netd_lookup_label(stream, NETD_FDESC_TYPE)) == -1)
            err_label(stream);
        if (MPSioctl(fd, I_POP,0) == -1) {
            LE_send_msg (GL_ERROR, "MPSioctl FAIL errno = %d\n", errno);
            err_server(strerror(errno));
        }

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

        if ((pControl_label = netd_control_label(lower)))
        {
            LE_send_msg (LE_VL3, "    open server %s proto %s)\n", 
		pControl_label->serverName, pControl_label->protoName);
            if ((fd2 = doOpen (pControl_label)) == -1)
            {
                LE_send_msg (GL_ERROR, "doOpen FAIL errno = %d\n", errno);
                err_server(strerror(errno));
            }
        }
        else
	{
	    if ((fd2 = netd_lookup_label(lower,NETD_FDESC_TYPE)) == -1)
                err_label(lower);
	}
        if ((mux = MPSioctl(fd1, I_LINK, fd2)) == -1) {
            LE_send_msg (GL_ERROR, "MPSioctl FAIL errno = %d\n", errno);
            err_server(strerror(errno));
        }
        LE_send_msg (LE_VL3, "    link upper %s lower %s", upper, lower);
        LE_send_msg (LE_VL3, "    close lower %s",lower);
        MPSclose(fd2);
        if ( ! pControl_label)
        {
            netd_delete_label(lower);
            lower[0] = '\0';
        }
        LE_send_msg (LE_VL3, "    mux %d new_label %s", mux, new_label);

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
        
        LE_send_msg (LE_VL3, "    unlink stream %s, label %s", stream, label);

        if ((fd = netd_lookup_label(stream, NETD_FDESC_TYPE)) == -1)
            err_label(stream);

        if (strcmp(label, "ALL") == 0 || strcmp(label, "all") == 0)
            mux = MUXID_ALL;
        else if ((mux = netd_lookup_label(label, NETD_MUXID_TYPE)) == -1)
            err_label(label);
        if (MPSioctl(fd, I_UNLINK, mux) == -1) {
            LE_send_msg (GL_ERROR, "MPSioctl FAIL errno = %d", errno);
            err_server(strerror(errno));
        }

        /*
        * If the lower stream is not a control stream then close
        * it and free the label.
        */
        if ((lower = get_lower_label(label)))
        {
            if ((fd = netd_lookup_label(lower, NETD_FDESC_TYPE)))
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

        LE_send_msg (LE_VL3, 
		"    strioc (stream %s, cmd %s, tmout %s, length %s)",
            stream, cmd, tmout, length);

        if (MPSioctl(fd, I_STR, &ioc) == -1) {
            LE_send_msg (GL_ERROR, "MPSioctl FAIL errno = %d\n", errno);
            if (ioc.ic_dp)
                free(ioc.ic_dp);
            err_server(strerror(errno));
        }

        LE_send_msg (LE_VL3, "    MPSioctl OK (%d bytes)", ioc.ic_len);

        /* check for simply case, no returned data */

        if (ioc.ic_len == 0)
            ok_length(0);

        /* compose a reply with data */

        p = reply_message;

        (void) sprintf(p, "OK %d\n", ioc.ic_len);
        p += strlen(p);

        dptr = ioc.ic_dp;

        while (ioc.ic_len-- > 0)
        {
            (void) sprintf(p, "%02x", (unsigned int)*dptr++);
            p += strlen(p);
        }

        sprintf(p, "\nEND\n");

        free(ioc.ic_dp);

        return 0;
    }

    /*
     * --> SHELL <command>
     * <-- OK | ERR ...
     */

    if (strcmp(command, "shell") == 0)
    {
        int stat;

        LE_send_msg (LE_VL3, "    shell %s\n", current_command);

#ifdef VXWORKS
        if (stat = pt_system(current_command))
#else
        if ((stat = system(current_command)))
#endif /* VXWORKS */
            err_user();

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

        LE_send_msg (LE_VL3, "    rename old %s new %s\n", old, new);

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

        LE_send_msg (LE_VL3, "    alias label %s alias %s", label, alias);

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

        LE_send_msg (LE_VL3, "    delete label %s", label);

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
        LE_send_msg (LE_VL3, "netd shutting down on client request!\n");
        reply("OK");
/*        ipc_detach(netd_queue); */
	netd_delete_all_labels(); /* #5 */
    }

    LE_send_msg (GL_ERROR, "unknown command: %s\n", command);

    err_proto();
} /* end process_request() */

/*************************************************************************
* reply
*
*************************************************************************/
static int reply(char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf(reply_message, format, args);
    va_end(args);

    if (strncmp (reply_message, "OK", 2) == 0)
	return 0;
    return (-1);
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



/* returns 0 on failure */
int nc_shell (struct nc *nc, char *cmd) {
    sprintf (netd_message, "SHELL %s\n", cmd);
    if (process_request (netd_message) < 0)
	return (0);
    return (1);
}

/* returns -1 on failure. The original version also returns info in "dp"
   on success. The data is decoded as in nc_read. I don't use any of the
   return data in "dp". So don't implement it */
int nc_strioc (struct nc *nc, char *stream, int cmd, long tmout, 
						unsigned char *dp, int len) {
    unsigned char *p;
    char *mp;
    int n = len;

    p = dp;
    mp = (char *)netd_message;
    sprintf (mp, "STRIOC %s %d %ld %d\n", stream, cmd, tmout, len);
    mp += strlen (mp);

    while (n--) {
	sprintf (mp, "%02x", *p++);
	mp += strlen (mp);
    }
    if (len > 0)
	sprintf (mp, "%s", "\nEND\n");

    if (process_request(netd_message) < 0)
	return (-1);
    return (0);
}

/* returns NULL on failure. The pointer to the label on success */
char *nc_open (struct nc *nc, char *pServerName, char *pServiceName, 
        char *pCtlrNumber, char *pProtoName, char *pDevice, char *label) {

    sprintf (netd_message, "OPEN %s %s %s %s %s %s\n", pServerName,
	    pServiceName, pCtlrNumber, pProtoName, pDevice,
	    label? label : "ANY");
    if (process_request (netd_message) < 0) {
	LE_send_msg (GL_ERROR, "nc_open failed (%s)\n", reply_message);
	return (NULL);
    }

    return (reply_message + 3);
}

/* returns 0 on failure */
int nc_close (struct nc *nc, char *stream) {

    sprintf (netd_message, "CLOSE %s\n", stream);
    if (process_request (netd_message) < 0)
	return (0);
    return (1);
}

/* returns 0 on failure */
int nc_delete (struct nc *nc, char *label) {
    sprintf (netd_message, "DELETE %s\n", label);
    if (process_request (netd_message) < 0)
	return (0);
    return (1);
}

/* returns -1 on failure or an integer value */
int nc_muxval (struct nc *nc, char *label) {
    int value;

    sprintf (netd_message, "VALUE %s MUX\n", label);
    if (process_request (netd_message) < 0)
	return (-1);

    if (sscanf (reply_message + 3, "%d", &value) != 1) {
	LE_send_msg (GL_ERROR, 
		"bad reply (%s) from VALUE request", reply_message);
	return (-1);
    }
    return (value);
}

/* returns NULL on failure or a pointer to a string on success */
char *nc_link (struct nc *nc, char *upper, char *lower, char *label) {

    sprintf (netd_message, "LINK %s %s %s\n", upper, lower,
            label? label : "ANY");
    if (process_request (netd_message) < 0)
        return NULL;

    return (reply_message + 3);
}


