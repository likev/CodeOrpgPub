
/******************************************************************

	file: cmt_sock.c

	This module contains socket access functions for the TCP
	comm_manager.
	
******************************************************************/

/* 
 * RCS info
 * $Author Jing$
 * $Locker:  $
 * $Date: 2008/04/15 20:26:46 $
 * $Id: cmt_sock.c,v 1.15 2008/04/15 20:26:46 jing Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 *
 * History:
 *  02Feb2002 - C. Gilbert - CCR #NA01-34801 Issue 1-886 Part 1. Add FAA support to 
 *                           the client cm_tcp.
 *
 * 20MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Fix some minor problems
 *                           found in unit testing.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#ifdef HPUX
#include <arpa/inet.h>
#endif
#include <sys/un.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>

#include <netinet/in.h>		/* for IPPROTO_TCP value */
#include <netinet/tcp.h>	/* for TCP_NODELAY value */
#include <arpa/inet.h>

#include <infr.h>
#include <comm_manager.h>
#include <cmt_def.h>

static int Sig_pipe = 0;


static int Set_sock_properties (int fd);
static int Set_nonblock (int fd);
static void Sigpipe_int ();


/*********************************************************************
			
    Description: This function opens a socket for use as a server 
		and puts it into listen mode. The socket option 
		SO_REUSEADDR is set to allow a new server to use the 
		port in case the port is not released by a hanging 
		process. 

    Inputs:	link - the link involved.

    Return:	This function returns the server socket fd on success or
		-1 on failure.
            
********************************************************************/

int SOCK_open_server (Link_struct *link) 
{
    int fd;		        /* socket file descriptor */
    struct sockaddr_in loc_soc;	/* local socket info */
    int i;


    /* get a file descriptor for the connection */
    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	LE_send_msg (GL_ERROR | 1023,  "open socket failed (errno %d)\n", errno);
	return (-1);
    }

    loc_soc.sin_family = AF_INET;
    if (SOCK_set_address (&loc_soc, link->server_name) < 0) {
	close (fd);
	return (-1);
    }
    loc_soc.sin_port = htons ((unsigned short)link->port_number);

    i = 1;
    if (setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, 
					(char *)&i, sizeof (int)) < 0) {
	LE_send_msg (GL_ERROR | 1024,  "SO_REUSEADDR failed (errno %d)\n", errno);
	close (fd);
	return (-1);
    }

    /* bind to a local port */
    errno = 0;
    if (bind (fd, (struct sockaddr *) &loc_soc, sizeof (loc_soc)) < 0) {
	LE_send_msg (GL_ERROR | 1025,  "bind failed on port %d (errno %d)\n", 
				link->port_number, errno);
	close (fd);
	return (-1);
    }

    if (Set_sock_properties (fd) < 0 ||
	Set_nonblock (fd) < 0) {
	close (fd);
	return (-1);
    }

    /* Wait for remote connection request */
    errno = 0;
    if (listen (fd, 5) < 0) {
	LE_send_msg (GL_ERROR | 1026,  "listen failed on port %d (errno %d)\n", 
						link->port_number, errno);
	close (fd);
	return (-1);
    }

    return (fd);
}

/*********************************************************************
			
    Description: This function opens a socket for a client PVC. 

    Inputs:	link - the link involved.

    Return:	This function returns the socket fd on success or
		-1 on failure.
            
********************************************************************/

int SOCK_open_client (void **address, char *server_name, int port)
{
    int sockfd;			/* socket file descriptor */

    if (*address == NULL) {

	struct sockaddr_in *rem_soc;	/* remote host address */

	rem_soc = (struct sockaddr_in *)malloc (sizeof (struct sockaddr_in));
	if (rem_soc == NULL) {
	    LE_send_msg (GL_ERROR | 1027,  "malloc failed\n");
	    return (-1);
	}

	memset ((char *)rem_soc, 0, sizeof (rem_soc));
	rem_soc->sin_family = AF_INET;
	if (SOCK_set_address (rem_soc, server_name) < 0) {
	    free (rem_soc);
	    return (-1);
	}
	rem_soc->sin_port = htons ((unsigned short)port);
						/* fill in port number */
	*address = (void *)rem_soc;

    }


    /* open a socket for connecting to the remote port */
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
	LE_send_msg (GL_ERROR | 1028,  "open client socket failed (errno %d)\n", errno);
	return (-1);
    }

    if (Set_sock_properties (sockfd) < 0 ||
	Set_nonblock (sockfd) < 0) {
	close (sockfd);
	return (-1);
    }

    return (sockfd);
}

/*********************************************************************
			
    Description: This function sets the socket address field in the
		addr structure in terms of the host name (or Internet 
		address). 

    Inputs:	host - the host name or the Internet address.

    Outputs:	addr - the struct sockaddr_in structure.

    Return:	This function returns 0 on success or -1 on failure.
            
********************************************************************/

int SOCK_set_address (struct sockaddr_in *addr, char *host)
{

    if (strcmp (host, "INADDR_ANY") == 0) {
	addr->sin_addr.s_addr = htonl (INADDR_ANY);
	return (0);
    }

    if ((addr->sin_addr.s_addr = inet_addr (host)) == -1) {
	struct hostent *hostport;	/* host port info */

        hostport = gethostbyname (host);
					/* get the remote host info */
	if (!hostport) {
	    LE_send_msg (GL_ERROR | 1029,  "gethostbyname failed (host: %s)\n", host);
	    return (-1);
	}

        /* copy the remote sockets internet address to soc */
        memcpy ((char *)&(addr->sin_addr), (char *) hostport->h_addr,
	        hostport->h_length);
    }

    return (0);
}
/*********************************************************************
			
    Description: This function tries to connect to the server. 

    Inputs:	link - the link involved.
		pvc - the PVC number.

    Return:	This function returns 0 on success, SOCK_NOT_CONNECTED 
		if connection failed or SOCK_CONNECT_ERROR on error.
            
********************************************************************/

int SOCK_connect (int *fd, void *address)
{

    if (connect (*fd, (struct sockaddr *)address,
		 		sizeof (struct sockaddr)) < 0) {
	if (errno == EINPROGRESS || errno == EALREADY || errno == EPIPE ||
	    errno == EINTR || errno == ECONNREFUSED || errno == EAGAIN) {
	    if (errno == ECONNREFUSED || errno == EPIPE) {
                close (*fd);
		*fd = -1;
	    }
	    return (SOCK_NOT_CONNECTED);
	}

	if (errno == EISCONN)
	    return (0);
	LE_send_msg (GL_ERROR | 1030,  "connect failed (errno %d)\n", errno);
	return (SOCK_CONNECT_ERROR);
    }

    return (0);
}

/**************************************************************************

    Description: This function accepts a client connection request.

    Inputs:	link - the link involved.

    Output:	cadd - client's host address.

    Return:	returns the client sock id on success, SOCK_NOT_CONNECTED 
		if there is no connection request or SOCK_CONNECT_ERROR 
		if an error is detected.

**************************************************************************/

int SOCK_accept_client (Link_struct *link, unsigned int *cadd)
{
    int sockfd;			/* client socket fd */
    int len;			/* address buffer length */
    static union sunion {
	struct sockaddr_in sin;
	struct sockaddr_un sund;
    } sadd;			/* the client address */

    len = sizeof (struct sockaddr_in);

    while (1) {
	errno = 0;
	sockfd = accept (link->server_fd, 
			(struct sockaddr *) &sadd, (socklen_t *)&len);
	if (sockfd < 0) {   /* failed */
	    if (errno == EINTR) continue;   /* retry */
	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		return (SOCK_NOT_CONNECTED);

	    LE_send_msg (GL_ERROR | 1031,  "accept failed (errno %d)", errno);
	    return (SOCK_CONNECT_ERROR);
	}
	break;
    }

    /* set socket properties */
    if (Set_sock_properties (sockfd) < 0 ||
	Set_nonblock (sockfd) < 0) {
	close (sockfd);
	return (SOCK_CONNECT_ERROR);
    }

    *cadd = ntohl (sadd.sin.sin_addr.s_addr);	/* get client host address */

    return (sockfd);
}

/**************************************************************************

    Description: This function sets additional socket properties.

    Inputs:	fd - the socket file descriptor.

    Return: 	returns 0 on success or -1 on failure.

**************************************************************************/

static int Set_sock_properties (int fd)
{
    int i;
    struct linger lig;		/* parameter for setting SO_LINGER */

    /* disable buffering of short data in TCP level */
    i = 1;
    errno = 0;
    if (setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, 
					(char *)&i, sizeof (int)) < 0) {
	LE_send_msg (GL_ERROR | 1032,  "TCP_NODELAY failed (errno %d)", errno);
	return (-1);
    }

    /* turn off linger */
    lig.l_onoff = 0;
    lig.l_linger = 0;
    if (setsockopt (fd, SOL_SOCKET, SO_LINGER, 
			(char *)&lig, sizeof (struct linger)) < 0) {
	LE_send_msg (GL_ERROR | 1033,  "SO_LINGER failed (errno %d)", errno);
	return (-1);
    }

    return (0);
}

/**************************************************************************

    Description: This function sets the socket "fd" to non-blocking mode.

    Inputs:	fd - the socket file descriptor.

    Return: 	returns 0 on success or -1 on failure.

**************************************************************************/

static int Set_nonblock (int fd)
{

    /* set non-block IO */
#ifdef HPUX
    if (fcntl (fd, F_SETFL, O_NONBLOCK) < 0) {
#else
    if (fcntl (fd, F_SETFL, O_NDELAY) < 0) {
#endif
	LE_send_msg (GL_ERROR | 1034,  
		"fcntl O_NDELAY (server) failed (errno %d)\n", errno);
	return (-1);
    }
    return (0);
}

/**************************************************************************

    Description: This function reads at most buf_size bytes from the 
		socket "fd".

    Inputs:	link - the link involved.
		fd - the socket file descriptor.
		buf_size - size of "buf".

    Output:	buf - the bytes read.

    Return:	returns the number of bytes read or -1 if the socket is
		disconnected or an error is encountered.

**************************************************************************/

int SOCK_read (Link_struct *link, int fd, char *buf, int buf_size)
{

    while (1) {
	int n_read;			/* number of bytes read */

	if ((n_read = read (fd, buf, buf_size)) == 0) {		
					/* socket disconnected */
	    LE_send_msg (LE_VL0 | 1035,  "socket (read) disconnected on link %d", 
						link->link_ind);
	    return (-1);
	}
	if (n_read < 0) {

	    if (errno == EWOULDBLOCK || errno == EAGAIN)
		return (0);

	    if (errno == EINTR) 
		continue;

	    LE_send_msg (GL_ERROR | 1036,  "read socket failed (errno %d)", errno);
	    return (-1);
	}
	return (n_read);
    }
}

/**************************************************************************

    Description: This function writes at most buf_size bytes to the 
		socket "fd".

    Inputs:	link - the link involved.
		pvc - the pvc number.
		buf - buffer containing the data to write.
		n_bytes - number of bytes to write.

    Return:	returns the number of bytes written or -1 if the socket is
		disconnected or an error is encountered.

**************************************************************************/

int SOCK_write (Link_struct *link, int pvc, char *buf, int n_bytes)
{
    int fd;

    fd = link->pvc_fd[pvc];
    link->w_blocked[pvc] = 0;
    while (1) {
	int n_written;			/* number of bytes written */

	Sig_pipe = 0;
	n_written = write (fd, buf, n_bytes);
	if (Sig_pipe) {			/* socket disconnected */
	    LE_send_msg (LE_VL0, "socket (write) disconnected on link %d", 
						link->link_ind);
	    return (-1);
	}
	if (n_written < 0) {

	    if (errno == EWOULDBLOCK || errno == EAGAIN) {
		link->w_blocked[pvc] = 1;
		return (0);
	    }

	    if (errno == EINTR) 
		continue;

	    LE_send_msg (GL_ERROR, "write socket failed (errno %d)", errno);
	    return (-1);
	}
	if (n_written < n_bytes)
	    link->w_blocked[pvc] = 1;
	return (n_written);
    }
}

/**************************************************************************

    Description: This function registers the SIGPIPE call back function.

    Return:	returns 0 on success or -1 on failure.

**************************************************************************/

int SOCK_init ()
{
    if (sigset (SIGPIPE, Sigpipe_int) == SIG_ERR)
	return (-1);
    return (0);
}

/**************************************************************************

    Description: This is the SIGPIPE call back function, which sets the 
		"Sig_pipe" flag.

**************************************************************************/

static void Sigpipe_int ()
{

    Sig_pipe = 1;
    return;
}


