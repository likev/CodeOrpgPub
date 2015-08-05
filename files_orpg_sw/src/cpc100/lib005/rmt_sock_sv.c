/****************************************************************
		
	File: sock_rmtd.c	
				
	2/22/94

	Purpose: The module containing basic networking functions for
	RMT server.

	This is a socket based implementation.

	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:35 $
 * $Id: rmt_sock_sv.c,v 1.12 2012/07/27 19:33:35 jing Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#ifdef __WIN32__
#define __INTERIX
#endif
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#ifdef HPUX
#include <arpa/inet.h>
#endif
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

#include <netinet/in.h>		/* for IPPROTO_TCP value */
#include <netinet/tcp.h>	/* for TCP_NODELAY value */

/*** Local include files ***/

#include <misc.h>
#include <rmt.h>
#include <net.h>
#include "rmt_def.h"

/*** Definitions / macros / types ***/

#define TEST_BYTE_PERIOD 10	/* time interval sending test byte */


/*** External references / external global variables ***/

/*** Local references / local variables ***/


/****************************************************************
			
	SOCD_open_server()		Date: 2/16/94

	This function opens up an Internet Stream socket for use 
	as a server and puts it into listen mode. Refer to "UNIX
	Network Programming" for a description for how to use
	the socket. The socket option SO_REUSEADDR is set to allow
	a new server to use the port in case the port is not released
	by a hanging process. The function sets up necessary socket 
	properties.
	
	This function returns the server socket fd on success. If an
	error is encountered and the function can not finish the task, an
	appropriate message is written to the log file and the function
	returns FAILURE.               
*/

int
  SOCD_open_server
  (
      int port			/* the RMT port number */
) {
    int fd, uid, ret;		        /* socket file descriptor */
    static struct sockaddr_in loc_soc;	/* local socket info */
    

    /* get a file descriptor for the connection to the RMT port */
    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	MISC_log ("Could not open socket (errno = %d)\n", errno);
	return (FAILURE);
    }

    loc_soc.sin_port = htons ((unsigned short)port);
    loc_soc.sin_family = AF_INET;
    loc_soc.sin_addr.s_addr = RMT_bind_address ();

    if (NET_set_SO_REUSEADDR (fd) < 0) {
	close (fd);
	return (FAILURE);
    }

    /* bind to a local port */
    uid = getuid ();
    errno = 0;
    seteuid ((uid_t)0);
    ret = bind (fd, (struct sockaddr *) &loc_soc, sizeof (loc_soc));
    setuid ((uid_t)uid);
    if (ret < 0) {
	MISC_log ("Couldn't bind port %d (errno = %d)\n", port, errno);
	close (fd);
	return (FAILURE);
    }

    if (NET_set_TCP_NODELAY (fd) < 0 ||
	NET_set_keepalive_on (fd) < 0 ||
	NET_set_linger_off (fd) < 0 ||
	NET_set_non_block (fd) < 0) {
	close (fd);
	return (FAILURE);
    }

    /* Wait for remote connection request */
    errno = 0;
    if (listen (fd, 16) < 0) {
	MISC_log ("Listen failure on port %d (errno = %d)\n", port, errno);
	close (fd);
	return (FAILURE);
    }

    return (fd);
}

/******************************************************************
			
	SOCD_accept_connection()			Date: 4/22/94

	This function accepts a client connection request. When there 
	is a connection
	request, the accept call returns a client socket fd. Then
	appropriate socket properties are set for the client socket.

	This function returns the client socket fd and the client host 
	address on success. If there is no waiting connection request in the
	socket this function returns RMTD_NO_REQUEST. If an error is 
	encountered, an appropriate message is written to the log file 
	and the function returns FAILURE.   
*/

int
  SOCD_accept_connection
  (
      int fd,		/* The server socket fd */
      unsigned long *cid  /* returns client's host internet address (LBO) */
) {

    int sockfd;			/* client socket fd */
    int len;			/* address buffer length */
    static union sunion {
	struct sockaddr_in sin;
	struct sockaddr_un sund;
    } sadd;			/* the client address */

    len = sizeof (struct sockaddr_in);

    sadd.sin.sin_addr.s_addr = 0;
    while (1) {
        errno = 0;
        sockfd = accept (fd, (struct sockaddr *) &sadd, (socklen_t *)&len);

	if (sockfd < 0) {   /* failed */

	    if (errno == EINTR) continue;   /* retry */
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		return (RMTD_NO_REQUEST);

	    MISC_log ("accept failed (errno %d)", errno);
	    return (FAILURE);
	}
	break;
    }

    if (sadd.sund.sun_family == AF_UNIX)
	*cid = RMT_LOCAL_HOST;
    else
	*cid = ntohl (sadd.sin.sin_addr.s_addr);	/* get host id */

    /* set socket properties */
    if (*cid != RMT_LOCAL_HOST) {	/* for TCP */
	if (NET_set_TCP_NODELAY (sockfd) < 0 ||
	    NET_set_keepalive_on (sockfd) < 0 ||
	    NET_set_linger_off (sockfd) < 0 ||
	    NET_set_non_block (sockfd) < 0) {   /* set non-block IO */
	    close (sockfd);
	    return (FAILURE);
	}
    }
    else {				/* for UNIX socket */
	if (NET_set_non_block (sockfd) < 0) {   /* set non-block IO */
	    close (sockfd);
	    return (FAILURE);
	}
    }

    return (sockfd);
}

/******************************************************************
			
	SOCD_send_msg()			Date: 2/22/94

	This function writes "msg_size" bytes of data in "msg" to the 
	socket "fd". A SIGPIPE signal received, when calling write(), 
	indicates that the socket is disconnected by the other end. 
	The write system call may return various
	error conditions. If the socket is blocked, a select call
	is performed on the socket until the socket is available 
	for write. The select call will block at most 5 seconds 
	before another write try is conducted. 

	The function returns SUCCESS if all data are written to the socket.
	It returns FAILURE if a fatal error is detected or the socket is
	disconnected. It returns RMT_FUNC_CANCELED if OOB data is recieved
	indicating a cancelation from the client. Otherwise it will keep 
	trying to send the data and not return. Error messages are written 
	to the log file on error conditions.
*/

int
  SOCD_send_msg
  (
      int fd,			/* The socket fd */
      int msg_size,		/* date size in bytes */
      char *msg,		/* the pointer to the date */
      int cancel_enabled	/* RPC cancelation enabled */
) {
    int n_written;

    n_written = 0;
    while (1) {
	int k;			/* return value */
	char oob;

	errno = 0;
	if (cancel_enabled && recv (fd, &oob, 1, MSG_OOB) > 0)
	    return (RMT_FUNC_CANCELED);
	k = write (fd, &msg[n_written], msg_size - n_written);
	if (k < 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN) {
		if (SCSH_wait (fd, SELECT_WRITE, 5) == FAILURE)
		    return (FAILURE);
		continue;
	    }
	    else if (errno == EBADF || errno == EPIPE) {
						/* socket disconnected */
		MISC_log ("Client %d disconnected - write", fd);
		return (FAILURE);
	    }
	    else if (errno != EINTR) {
		MISC_log ("write failed (SOCD_send_msg) (errno %d)", errno);
		return (FAILURE);
	    }
	    continue;
	}
	if (k > 0)
	    n_written += k;
	if (n_written >= msg_size)
	    return (SUCCESS);
    }
}

/******************************************************************
			
	SOCD_recv_msg()			Date: 2/22/94

	This function reads "n_bytes" bytes of data from the socket "fd" 
	into "msg_buf". This function calls SCSH_wait to wait for
	socket data available. A 0 return from the read system call 
	indicates that the socket is disconnected.

	The function returns SUCCESS if required data are read from the socket.
	It returns FAILURE if a fatal error is detected or the socket is
	disconnected. Otherwise it will keep trying to read the data
	and not return. Error messages are written to the log file on
	error conditions.

*/

int
  SOCD_recv_msg
  (
      int fd,			/* socket fd */
      int n_bytes,		/* number of bytes to read */
      char *msg_buf,		/* the buffer for holding the data */
      int cancel_enabled	/* RPC cancelation enabled */
) {
    int n_read;			/* number of bytes read */

    /* wait until data available */
    if (SCSH_wait (fd, SELECT_READ, 5) == FAILURE)
	return (FAILURE);

    n_read = 0;
    while (1) {
	int k;			/* read return value */
	int ret;		/* return value */
	char oob;

	errno = 0;
	if (cancel_enabled && recv (fd, &oob, 1, MSG_OOB) > 0)
	    return (RMT_FUNC_CANCELED);

	errno = 0;
	if ((k = read (fd, &msg_buf[n_read], n_bytes - n_read)) == 0) {
					/* socket disconnected */
	    return (FAILURE);
	}
	if (k < 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN) {
		if ((ret = SCSH_wait (fd, SELECT_READ, TEST_BYTE_PERIOD))
		    == FAILURE)
		    return (FAILURE);

		continue;
	    }
	    if (errno != EINTR) {
		MISC_log ("read failed (SOCD_recv_msg) (errno %d)", errno);
		return (FAILURE);
	    }
	    continue;
	}
	n_read += k;
	if (n_read >= n_bytes)
	    return (SUCCESS);
    }

}

/******************************************************************
			
	This function reads at most "n_bytes" bytes of data from 
	socket "fd" into "msg_buf". This function will not wait.

	The function returns the number of bytes read or -1 on failure.
	If the connection is lost, it returns -1.

******************************************************************/

int
  SOCD_read_socket
  (
      int fd,			/* socket fd */
      int n_bytes,		/* number of bytes to read */
      char *msg_buf		/* the buffer for holding the data */
) {
    int n_read;			/* number of bytes read */

    n_read = 0;
    while (1) {
	int k;			/* read return value */

	if ((k = read (fd, &msg_buf[n_read], n_bytes - n_read)) == 0) {		
					/* socket disconnected */
	    return (-1);
	}
	if (k < 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		break;
	    if (errno != EINTR) {
		MISC_log ("read failed (SOCD_read_socket) (errno %d)", errno);
		return (-1);
	    }
	}
	else {
	    n_read += k;
	    if (n_read >= n_bytes)
		break;
	}
    }
    return (n_read);
}

