/****************************************************************
		
	File: send_msg.c	
				
	Purpose: This module implements the RMT_send_msg function.

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:34 $
 * $Id: rmt_msg_cl.c,v 1.16 2012/07/27 19:33:34 jing Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */  

/*** System include files ***/

#include <config.h>
#ifdef __WIN32__
#define __INTERIX
#endif
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/socket.h>
#ifdef HPUX
#include <arpa/inet.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>	/* for TCP_NODELAY value */
#include <sys/un.h>
#include <netdb.h>


/*** Local include files ***/

#include <misc.h>
#include <net.h>
#include <rmt.h>
#include "rmt_def.h"


#define MAX_N_HOSTS 32

typedef struct {		/* structure storing open fds */
    unsigned int host;		/* host address (NBO) */
    int fd;			/* socket fd for "host". < 0 indicates unused
				   entry. */
} Host_fd_table;

static Host_fd_table Hosts[MAX_N_HOSTS];
static int N_hosts = 0;

static int Get_fd (unsigned int host, int open, unsigned int *mdf_host);
static void Close_msg_server (int fd);


/****************************************************************

    Description: This function send a message "msg" of length 
		"msg_len" to host "host" (IP address). Refer to
		RMT man-page for further details.

    Input:	host - destination host IP address (NBO)
		msg - the message to send
		msg_len - length of the message

    Output:	sfd - returns the socket fd if not NULL

    Return:	This function returns the number of bytes actually 
		sent or a negative error number.

****************************************************************/

#define FIRST_PART_SIZE 	1024
	/* socket write is a slow process. We set this value a little large.
	   This should be less than  */

int RMT_send_msg (unsigned int host, char *msg, int msg_len, int *sfd)
{
    static int nbytes_written = 0;	/* save the number of bytes written
					   in a previous incomplete call */
    int fd, plen, tlen, ret;
    char *pmsg;
    int hdsize;
    unsigned int mdfh;

    fd = Get_fd (host, msg_len, &mdfh);
    if (sfd != NULL)
	*sfd = fd;
    host = mdfh;

    if (msg_len < 0) {
	if (fd >= 0)
	    Close_msg_server (fd);
	return (0);
    }

    if (fd < 0)
	return (fd);

    if (msg_len == 0)
	return (0);

    /* pack the message */
    pmsg = RMT_pack_msg (msg, msg_len, FIRST_PART_SIZE, &plen);
				/* packed msg - only the first part */
    hdsize = HEAD_SIZE;		/* header size */
    tlen = plen;		/* total bytes to send */
    if (msg_len > FIRST_PART_SIZE)
	tlen += msg_len - FIRST_PART_SIZE;

    /* write the first part */
    if (nbytes_written < plen) {
	ret = NET_write_socket (fd, pmsg + nbytes_written, 
					plen - nbytes_written);
	if (ret < 0) {
	    Close_msg_server (fd);
	    return (ret);
	}
	nbytes_written += ret;
    }
    /* write the second part */
    if (nbytes_written >= plen && nbytes_written < tlen) {
	ret = NET_write_socket (fd, msg + (nbytes_written - hdsize), 
					tlen - nbytes_written);
	if (ret < 0) {
	    Close_msg_server (fd);
	    return (ret);
	}
	nbytes_written += ret;
    }
    if (nbytes_written >= tlen) {
	nbytes_written = 0;
	return (msg_len);
    }
    else {
	if (nbytes_written < hdsize)
	    return (0);
	else
	    return (nbytes_written - hdsize);
    }
}

/***********************************************************************

    Description: This function returns a pointer to the packed version 
		of "msg" of length "len" that is suitable for sending to 
		a server. Since the messaging server processes messages 
		instead of byte stream, any message to be sent to a 
		server must be packed. 

    Input:	msg - the msg to be packed.
		msg_len - length of the message.
		max_len - max length of the first part of the message 
			to be processed.

    Output:	plen - length of the packed msg.

    Return:	pointer to the packed msg.

***********************************************************************/

#define MAX_PERMANENT_BUF	4096
	/* we free the buffer if it is larger than this, assuming extra 
	   large msgs are rare */

char *RMT_pack_msg (char *msg, int msg_len, int max_len, int *plen)
{
    static char *buf = NULL;
    static int buf_size = 0;
    rmt_t *hd;
    int len, slen;

    len = msg_len;
    if (len > max_len)
	len = max_len;
    slen = len + HEAD_SIZE;		/* target packed length */
    if (slen < MIN_MSG_SIZE)
	slen = MIN_MSG_SIZE;
    if (slen > buf_size || 
	buf_size > MAX_PERMANENT_BUF) {
	buf_size = slen + 256;		/* make a little larger to 
					   reduce remalloc frequency */
	if (buf != NULL)
	    free (buf);
	while ((buf = (char *)malloc (buf_size)) == NULL)
	    msleep (WAIT_FOR_MEMORY_TIME);
#ifdef USE_MEMORY_CHECKER
	memset (buf, 0, buf_size); /* useless - for eliminating purify error */
#endif
    }

    hd = (rmt_t *)buf;
    hd[0] = htonrmt (RMT_MSG_ID);
    hd[1] = htonrmt (msg_len);
    memcpy (buf + HEAD_SIZE, msg, len);
    *plen = slen;

    return (buf);
}

/****************************************************************

    Description: This function returns the fd for host "host".

    Input:	host - the host IP address (NBO).
		open - < open client is not needed.

    Return:	This function returns the fd on success or a 
		negative error number.

****************************************************************/

static int Get_fd (unsigned int host, int open, unsigned int *mdf_host)
{
    int i, fd;

    if (host != RMT_LOCAL_HOST) {	/* identify local host */
	unsigned int ip;

	if (RMT_lookup_host_index (RMT_LHI_IX2I, &ip, 0) > 0 && ip == host)
	    host = RMT_LOCAL_HOST;
    }
    *mdf_host = host;

    for (i = 0; i < N_hosts; i++)
	if (Hosts[i].fd >= 0 && Hosts[i].host == host) {
#ifdef DBG
printf ("send_msg.c: Get_fd find existing host %x, fd %d, i %d\n", 
				Hosts[i].host, Hosts[i].fd, i);
#endif
	    return (Hosts[i].fd);
	}
    if (open < 0)
	return (-1);

    for (i = 0; i < N_hosts; i++)
	if (Hosts[i].fd < 0)
	    break;

    if (i >= N_hosts) {
	if (i >= MAX_N_HOSTS)
	    return (RMT_TOO_MANY_MSG_HOSTS);

	Hosts[i].fd = -1;
	N_hosts++;
    }

    /* open the new server */
    fd = RMT_open_msg_host (host, 1);
    if (fd < 0)
	return (fd);
    Hosts[i].host = host;
    Hosts[i].fd = fd;
#ifdef DBG
printf ("send_msg.c: Get_fd new server reg i %d, host %x, fd %d\n", i, host, fd);
#endif

    return (fd);
}

/****************************************************************

    Description: This function closes a message server.

    Input:	fd - fd for the server.

****************************************************************/

static void Close_msg_server (int fd)
{
    int i, last_ind;

    last_ind = -1;
    for (i = 0; i < N_hosts; i++) {
	if (Hosts[i].fd == fd) {
	    close (fd);
	    Hosts[i].fd = -1;
	}
	if (Hosts[i].fd >= 0)
	    last_ind = i;
    }
    N_hosts = last_ind + 1;

    return;
}

/****************************************************************

    Description: This function opens a message server. If "wait" 
		is zero, the function returns without waiting and 
		allows the connection procedure to continue. One 
		can later call this function to check again.

    Input:	host - the host IP address (NBO).
		wait - non-zero for waiting until time-out.

    Return:	This function returns the fd for the new server
		on success or a negative error number.

****************************************************************/

int RMT_open_msg_host (unsigned int host, int wait)
{
    static int cr_fd = -1;	/* current connecting fd */
    static unsigned int cr_host;/* current connecting host */
    int sockfd;			/* socket file descriptor */
    struct sockaddr *add;	/* messaging host address */
    int add_size;
    int wait_cnt;
    int flag;

    if (host == RMT_LOCAL_HOST) {

	add_size = SCSH_local_socket_address ((void **)&add, NULL);
	if (add_size < 0)
	    return (add_size);
	flag = AF_UNIX;
    }
    else {
	struct sockaddr_in rsadd;	/* remote server address */
	int port;			/* port number */

	memset ((char *) &rsadd, 0, sizeof (rsadd));
	rsadd.sin_family = AF_INET;
	port = PNUM_get_port_number ();	/* get the port number */
	if (port == FAILURE)
	    return (RMT_PORT_NUMBER_NOT_FOUND);
	port += 1;
	rsadd.sin_addr.s_addr = host;
	rsadd.sin_port = htons ((unsigned short)port); 
	add = (struct sockaddr *)&rsadd;
	add_size = sizeof (rsadd);
	flag = AF_INET;
    }

    if (cr_fd >= 0 && host == cr_host && !wait) { /* continue connecting */
	while (connect (cr_fd, add, add_size) < 0) {
	    if (errno == EISCONN)
		break;
	    if (errno == EINTR)
		continue;
	    if (errno == EINPROGRESS || errno == EALREADY)
		return (RMT_TIMED_OUT);
	    close (cr_fd);
	    cr_fd = -1;
	    return (RMT_CONNECT_FAILED);
	}
	sockfd = cr_fd;
	cr_fd = -1;
	return (sockfd);
    }
    if (cr_fd >= 0) {
	close (cr_fd);
	cr_fd = -1;
    }

    /* open a socket for connecting to the remote port */
    if ((sockfd = socket (flag, SOCK_STREAM, 0)) == -1)
	return (RMT_OPEN_SOCKET_FAILED);
    fcntl (sockfd, F_SETFD, FD_CLOEXEC);

    /* set socket properties */
    if (flag == AF_INET) {		/* TCP */
	int ret;
	if ((ret = NET_set_TCP_NODELAY (sockfd)) < 0 || 
	    (ret = NET_set_non_block (sockfd)) < 0 ||
	    (ret = RMT_bind_selected_local_ip (sockfd)) < 0 ||
	    (wait && (ret = NET_set_linger_on (sockfd)) < 0) ||
	    (!wait && (ret = NET_set_linger_off (sockfd)) < 0)) {
	    close (sockfd);
	    return (ret);
	}
    }
    else {				/* UNIX */
	int ret;
	if ((ret = NET_set_non_block (sockfd)) < 0) {
	    close (sockfd);
	    return (ret);
	}
    }

    /* Connect the local socket to the remote server */
    wait_cnt = 0;
    while (connect (sockfd, add, add_size) < 0) {
	char buf[64];
	if (errno == EINPROGRESS || errno == EALREADY) {
	    if (wait && RMT_cmd_disconnected (host)) {
		close (sockfd);
		return (RMT_CONNECT_FAILED);
	    }
	    if (!wait && wait_cnt > 2) {
		cr_host = host;
		cr_fd = sockfd;
		return (RMT_TIMED_OUT);
	    }
	    if ((wait_cnt % 60) == 59)
		MISC_log ("RMT: Connect to %x - Attempt %d\n", host, wait_cnt);
	    msleep (100);
	    wait_cnt++;
	    continue;
	}
	if (errno == EISCONN)
	    break;
	if (errno == EINTR)
	    continue;
	if (host == RMT_LOCAL_HOST)
	    MISC_log ("RMT: connect to the local rssd failed (errno %d, %s)\n",
				errno, ((struct sockaddr_un *)add)->sun_path);
	else
	    MISC_log ("RMT: connect (to %s) failed (errno %d)\n", 
				NET_string_IP (host, 1, buf), errno);
	close (sockfd);
	return (RMT_CONNECT_FAILED);
    }

    return (sockfd);
}

/****************************************************************

    Closes all commanded disconnected msg host connection.

****************************************************************/

void RMTSM_close_disc_msg_hosts () {
    int i;

    for (i = 0; i < N_hosts; i++) {
	unsigned int host;
	int k;

	host = Hosts[i].host;
	for (k = 0; k < i; k++) {
	    if (Hosts[k].host == host)
		break;
	}
	if (k < i)
	    continue;
	if (!RMT_cmd_disconnected (host))
	    continue;
	for (k = i; k < N_hosts; k++) {
	    if (Hosts[k].fd >= 0 && Hosts[k].host == host)
		Close_msg_server (Hosts[k].fd);
	}
    }
}

/***********************************************************************

    Closes all remote message hosts. Called in rssd child process.

***********************************************************************/

void RMTSM_close_msg_hosts () {
    int i;

    for (i = 0; i < N_hosts; i++) {
	if (Hosts[i].fd >= 0) {
	    close (Hosts[i].fd);
	    Hosts[i].fd = -1;
	}
    }
}



