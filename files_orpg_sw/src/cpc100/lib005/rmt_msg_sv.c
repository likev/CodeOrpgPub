/****************************************************************
		
	File: msg_rmtd.c	
				
	2/23/94

	Purpose: This module contains messaging functions for 
	the RMT server. 

	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/08/07 19:41:22 $
 * $Id: rmt_msg_sv.c,v 1.23 2013/08/07 19:41:22 steves Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <rmt.h>
#include <net.h>
#include <misc.h>
#include "rmt_def.h"


#define MAX_N_CONNS 200			/* max number of connections */
#define N_SVFDS 3			/* number of server fds */
#define MAX_N_MSG_PROCESSED 10		/* max number messages processed for 
					   each sender to avoid being blocked
					   too long by any single sender */

static struct pollfd *Fds = NULL;	/* socket fds to listen;
					   0-th: The RMT server fd 
					   1-st: The TCP msg server fd; */
static int *Sender_index;		/* Senders index corresponding to the
					   fds in Fds. */
static int N_fds = 0;			/* number of elements in Fds */
static int N_msg_fds = 0;		/* number Fds excluding waiting RPC 
					   child fds */

typedef struct {			/* register a message sender */
    int fd;				/* socket fd (-1: void entry) */
    unsigned int cid;			/* sender's host (NBO) */
    int msg_len;			/* expected message length */
    int msg_in;				/* bytes received */
    int buf_size;			/* buf_size for "msg" */
    char *msg;				/* received message */
    int write;				/* write available flag - non-zero 
					   indicates that poll write needed */
} Sender_regist;

static Sender_regist *Senders = NULL;	/* message sender registration */
static int N_senders = 0;		/* number of elements in Senders */
static int Max_n_local_senders = 0;	/* maximum number of local senders */
static int Cr_n_local_senders = 0;	/* current number of local senders */

static int Msvfd;			/* message server socket fd */
static int Lmsvfd;			/* local message server socket fd */

static int Sender_changed = 1;		/* flag indicating a sender has been 
					   added or removed */

static int Local_socket_read = 1;	/* non-zero: reading all sockets
					   remote as well as local; zero: 
					   reading remote sockets only. */

static int Current_fd = -1;		/* the current fd in calling the user
					   callback function; -1 means the
					   fd is closed during callback. */

static int N_clfds = 0;			/* number of RPC client fds that need 
					   to be monitored for input*/
static int *Clfds;			/* list of RPC client fds */

/* The callback function receiving the messages */
static void (*Callback) (char *, int , int, unsigned int) = NULL;

/* static functions */
static int Wait_input ();
static void Process_connection (int fd);
static int Process_message (int sind);
static void Close_sender (Sender_regist *sender, int call_cb);
static void Register_new_fd (int fd, unsigned int cid);


/***********************************************************************

    Description: Set the max number of local senders.

    Input:	max_n_local_senders - The max number of local senders.

***********************************************************************/

void MSGD_set_max_n_local_senders (int max_n_local_senders)
{

    Max_n_local_senders = max_n_local_senders;
    return;
}

/***********************************************************************

    Description: This function accepts a callback function for receiving
		messages. Refer to RMT man-page for more details.

    Input:	callback - The callback function.

***********************************************************************/

void RMT_register_msg_callback (void (*callback)(char *, int, int, unsigned int))
{

    Callback = callback;
    return;
}

/***********************************************************************

    Description: This function accepts a request for polling 
		write-availability.

    Input:	fd - the msg client fd.

***********************************************************************/

int RMT_poll_write (int fd)
{
    int i;

    if (fd < 0)
	return (RMT_FAILURE);
    for (i = 0; i < N_senders; i++)
	if (Senders[i].fd == fd) {
	    Senders[i].write = 1;
	    Sender_changed = 1;
	    return (RMT_SUCCESS);
	}

    return (RMT_FAILURE);
}

/***********************************************************************

    Description: This function tells the RMT messaging server to remove
		the msg client of "fd".

    Input:	fd - the msg client fd.

***********************************************************************/

void RMT_close_msg_client (int fd)
{
    int i;

    if (fd == Current_fd)
	Current_fd = -1;		/* fd used in callback is closed */
    for (i = 0; i < N_senders; i++) {
	Sender_regist *sender;

	sender = Senders + i;
	if (sender->fd == fd) {
	    Close_sender (sender, 0);
	    break;
	}
    }
    return;
}

/***********************************************************************

    Description: This function sets the Local_socket_read flag.

    Input:	func - set/reset function (RMT_LOCAL_READ_PAUSE or
			RMT_LOCAL_READ_RESUME).

***********************************************************************/

void RMT_local_read (int func)
{
    if (func == RMT_LOCAL_READ_PAUSE)
	Local_socket_read = 0;
    else
	Local_socket_read = 1;
    Sender_changed = 1;
    return;
}

/***********************************************************************

    Description: This function opens the messaging server sockets for 
		the RMT messaging service and initialize the local
		data structures.

    Input:	port_number - The RMT server port number.

    Return:	SUCCESS on success or FAILURE ON failure.

***********************************************************************/

int MSGD_open_msg_server (int port_number)
{
    char *name, dir_buf[256];
    struct sockaddr *lsadd;	/* local server address */
    int add_size;

    /* open the msg server socket */
    Msvfd = -1;
    if (port_number >= 1024 && 
	(Msvfd = SOCD_open_server (port_number + 1)) == FAILURE) {
	MISC_log ("Opening msg server failed\n");
	return (FAILURE);
    }

    /* open the local msg server socket */
    add_size = SCSH_local_socket_address ((void **)&lsadd, &name);
    if (add_size < 0) {
	return (FAILURE);
    }

    MISC_mkdir (MISC_dirname (name, dir_buf, 256));
    MISC_unlink (name);
    Lmsvfd = socket (AF_UNIX, SOCK_STREAM, 0);
    if (Lmsvfd < 0) {
	MISC_log ("socket (local) failed (errno = %d)\n", errno);
	return (FAILURE);
    }

    MISC_log ("The UNIX socket is %s\n", name);
    if (bind (Lmsvfd, lsadd, add_size) == -1) {
	MISC_log ("bind (local) failed (errno = %d)\n", errno);
	close (Lmsvfd);
	return (FAILURE);
    }

    /* Wait for connection request */
    if (listen (Lmsvfd, 32) < 0) {
	MISC_log ("Listen (local) failed (errno = %d)\n", errno);
	close (Lmsvfd);
	return (FAILURE);
    }

    Fds = (struct pollfd *)malloc (
			(MAX_N_CONNS + N_SVFDS) * sizeof (struct pollfd));
    Sender_index = (int *)malloc (
			(MAX_N_CONNS + N_SVFDS) * sizeof (int));
    Senders = (Sender_regist *)malloc (
			MAX_N_CONNS * sizeof (Sender_regist));
    if (Fds == NULL || Sender_index == NULL || Senders == NULL) {
	MISC_log ("malloc failed\n");
	return (FAILURE);
    }

    N_fds = N_msg_fds = N_SVFDS;
    N_senders = 0;
    Fds[1].fd = Msvfd;
    Fds[2].fd = Lmsvfd;
    Fds[1].events = POLL_IN_FLAGS;
    Fds[0].events = POLL_IN_FLAGS;
    Fds[2].events = POLL_IN_FLAGS;

    if (Callback != NULL) {
	MISC_log ("Connecting to other server for AN ...\n");
	Callback (NULL, RMT_MSG_TIMER, -1, 0);
    }

    return (SUCCESS);
}

/***********************************************************************

    Description: This function performs RMT messaging services. It 
		returns when a new RMT client requests connection. If
		poll fails, it will also return. This provides a chance
		that the RPC service will continue.

    Input:	pfd - The RMT parent fd.

***********************************************************************/

void MSGD_process_msgs (int pfd)
{
    extern int RMT_reread_config;

    if (pfd < 0) {
	if (RMT_reread_config && Callback != NULL)
	    Callback (NULL, RMT_MSG_CONFIG_UPDATE, 0, 0);
	return;
    }

    Fds[0].fd = pfd;

    while (1) {
	int cb_called;			/* the callback has been called */
	int ret, i;
	int rpccl_ready;		/* RPC client ready */
	int rpccl_called;		/* RPC client called */

	if (RMT_reread_config)
	    return;
	ret = Wait_input ();
	if (ret == FAILURE)
	    return;

	if (Fds[1].revents & (POLL_IN_RFLAGS))
	    Process_connection (Fds[1].fd);

	if (Fds[2].revents & (POLL_IN_RFLAGS))
	    Process_connection (Fds[2].fd);

	cb_called = 0;
	rpccl_ready = 0;
	rpccl_called = 0;
	for (i = N_SVFDS; i < N_fds; i++) {
	    int revents;

	    revents = Fds[i].revents;
	    if (revents & (POLL_IN_RFLAGS)) {
		if (i < N_msg_fds) {		/* messaging fd */
		    cb_called = Process_message (Sender_index[i]);
		}
		else {				/* RPC client fd */
		    rpccl_called = 1;
		    rpccl_ready = GCLD_client_sock_ready (Fds[i].fd);
		}
	    }
	    if ((revents & POLLOUT) && i < N_msg_fds) {
		Sender_regist *sender;

		sender = Senders + Sender_index[i];
		sender->write = 0;
		Sender_changed = 1;
		if (Callback != NULL && sender->fd >= 0) {
		    Callback (NULL, RMT_MSG_WRITE_READY, 
						sender->fd, sender->cid);
		    cb_called = 1;
		}
	    }
	    if (revents && !(revents & (POLLOUT | POLL_IN_RFLAGS))) {
		MISC_log ("Bad fd polled %d, revent %x\n", Fds[i].fd, revents);
		if (i < N_msg_fds)
		    Close_sender (Senders + Sender_index[i], 1);
		else
		    rpccl_ready = GCLD_client_sock_ready (Fds[i].fd);
		Sender_changed = 1;
	    }
	    if (Sender_changed)
		break;
	}
	if (!cb_called && Callback != NULL)
	    Callback (NULL, RMT_MSG_TIMER, -1, 0);

	if (!rpccl_called && N_msg_fds < N_fds)
	    GCLD_client_sock_ready (-1);

 	if ((Fds[0].revents & (POLL_IN_RFLAGS)) || rpccl_ready)
	    return;			/* proceed to accept new RPC client */
   }
}

/***********************************************************************

    Description: This function waits for data ready on one of the RMT
		inputs. I use poll here instead of select assuming poll 
		is more portable.

    Return:	SUCCESS on success or FAILURE ON failure.

***********************************************************************/

static int Wait_input ()
{

    if (Sender_changed) {		/* recreate the Fds array */
	int cnt, i;

	cnt = N_SVFDS;
	for (i = 0; i < N_senders; i++) {
	    if (Senders[i].fd >= 0) {
		if (Local_socket_read || Senders[i].cid != RMT_LOCAL_HOST) {
		    Fds[cnt].fd = Senders[i].fd;
		    Fds[cnt].events = POLL_IN_FLAGS;
		    if (Senders[i].write)
			Fds[cnt].events |= POLLOUT;
		    Sender_index[cnt] = i;
		    cnt++;
		}
		else if (Senders[i].write) {
		    Fds[cnt].fd = Senders[i].fd;
		    Fds[cnt].events = POLLOUT;
		    Sender_index[cnt] = i;
		    cnt++;
		}
	    }
	}
	N_msg_fds = cnt;
	for (i = 0; i < N_clfds; i++) {
	    Fds[cnt].fd = Clfds[i];
	    Fds[cnt].events = POLL_IN_FLAGS;
	    cnt++;
	}
	N_fds = cnt;
	Sender_changed = 0;
    }

    while (1) {
	int ret, to_ret;

	to_ret = SUCCESS;
	ret = poll (Fds, N_fds, 1000);
	if (ret < 0) {
	    if (errno != EINTR && errno != EAGAIN) {
		MISC_log ("poll failed (errno %d)\n", errno);
		to_ret = FAILURE;
	    }
	}
	CLRG_process_sigchild ();
	return (to_ret);
    }

}

/***********************************************************************

    Description: This function accepts a message senders connection
		and registers this sender.

***********************************************************************/

static void Process_connection (int fd)
{
    int sockfd;
    unsigned long cid;		/* LBO */

    sockfd = SOCD_accept_connection (fd, &cid);
    if (sockfd == FAILURE)
	return;

    if (cid == RMT_LOCAL_HOST) {
	if (Cr_n_local_senders > Max_n_local_senders) {
	    MISC_log ("Too many local message senders");
	    close (sockfd);
	    return;
	}
	else
	    Cr_n_local_senders++;
    }

    /* register the new connection */
    Register_new_fd (sockfd, htonl (cid));

    return;
}

/***********************************************************************

    Description: This function registers the client fds that need to
		be monitored for input.

    Input:	n_clfds - number of such fds.
		clfds - list of the fds.

***********************************************************************/

void MSGD_register_rpc_waiting_fds (int n_clfds, int *clfds)
{
    N_clfds = n_clfds;
    Clfds = clfds;
    Sender_changed = 1;
    return;
}

/***********************************************************************

    Description: This function makes a connection to host of IP address
		"host_ip".

    Input:	host_ip - host IP address (NBO).

    Return:	fd of the connection on success or a negative RMT 
		error number.

***********************************************************************/

int RMT_connect_host (unsigned int host_ip)
{
    int fd;

    fd = RMT_open_msg_host (host_ip, 0);
    if (fd < 0)
	return (fd);

    /* register the new connection */
    Register_new_fd (fd, host_ip);

    return (fd);
}

/***********************************************************************

    Description: This function registers a new fd for monitoring.

    Inputs:	fd - the new socket fd.
		cid - the host address (NBO).

***********************************************************************/

static void Register_new_fd (int fd, unsigned int cid)
{
    Sender_regist *sender;
    int i;

    /* register the new connection */
    for (i = 0; i < N_senders; i++) {

	sender = Senders + i;
	if (sender->fd < 0) {
	    sender->fd = fd;
	    sender->cid = cid;
	    sender->msg_in = 0;
	    sender->msg_len = 0;
	    sender->write = 0;
	    break;
	}
    }
    if (i >= N_senders) {

	if (i >= MAX_N_CONNS) {
	    close (fd);
	    MISC_log ("Too many registered message senders");
	    return;
	}

	sender = Senders + N_senders;
	sender->fd = fd;
	sender->cid = cid;
	sender->msg = NULL;
	sender->buf_size = 0;
	sender->msg_in = 0;
	sender->msg_len = 0;
	sender->write = 0;
	N_senders++;
    }
    Sender_changed = 1;

    return;
}

/***********************************************************************

    Description: This function reads input from the socket "fd" and
		calls the callback function when a message is ready.
		To maximize efficiency (minimize the number of socket 
		read calls) we introduce MIN_MSG_SIZE and try to read  
		N_EX_BYTES bytes from next message.

    Input:	sind - the sender's index in Senders array.

***********************************************************************/

#define N_EX_BYTES 1

static int Process_message (int sind)
{
    Sender_regist *sender;
    int cnt, called;

    sender = Senders + sind;
    cnt = called = 0;
    while (1) {
	int rlen, ret;

	/* number of bytes to read */
	if (sender->msg_in >= HEAD_SIZE) {
	    int size;

	    size = sender->msg_len + HEAD_SIZE;
	    if (size < MIN_MSG_SIZE)
		size = MIN_MSG_SIZE;
	    rlen = size - sender->msg_in;
	}
	else if (sender->msg_in > 0)
	    rlen = MIN_MSG_SIZE - sender->msg_in;
	else
	    rlen = MIN_MSG_SIZE;

	/* prepare the buffer space */
	if (rlen + sender->msg_in > sender->buf_size) {
	    int new_size;
	    char *new_buf;

	    new_size = (sender->buf_size + 64) * 2;
	    if (rlen + sender->msg_in > new_size)
		new_size = rlen + sender->msg_in;
	    while ((new_buf = (char *)malloc (new_size + N_EX_BYTES)) == NULL)
		msleep (WAIT_FOR_MEMORY_TIME);
	    sender->buf_size = new_size;
	    if (sender->msg_in > 0)
		memcpy (new_buf, sender->msg, sender->msg_in);
	    if (sender->msg != NULL) {
		free (sender->msg);
		sender->msg = NULL;
	    }
	    sender->msg = new_buf;
	}

	ret = NET_read_socket (sender->fd, sender->msg + sender->msg_in, 
						rlen + N_EX_BYTES);
	if (ret <= 0) {
	    if (ret < 0) {
		Close_sender (sender, 1);
	    }
	    return (called);
	}
	else {
	    sender->msg_in += ret;
	    if (sender->msg_in >= HEAD_SIZE) {
		int msg_len;
		if (sender->msg_len == 0) {/* verify and get msg length */
		    rmt_t *hd;
		    int mlen;

		    hd = (rmt_t *)sender->msg;
		    mlen = ntohrmt (hd[1]);
		    if (ntohrmt (hd[0]) != RMT_MSG_ID || mlen <= 0) {
					/* bad message */
			char buf[128];
			MISC_log ("Bad RMT user message (%s, %d %x %d, i %d)", NET_string_IP (sender->cid, 1, buf), mlen, hd[0], sender->fd, sind);
			Close_sender (sender, 1);
			return (called);
		    }
		    sender->msg_len = mlen;
		}
		msg_len = sender->msg_len + HEAD_SIZE;
		if (msg_len < MIN_MSG_SIZE)
		    msg_len = MIN_MSG_SIZE;
		if (sender->msg_in >= msg_len) {
		    Current_fd = sender->fd;
		    if (Callback != NULL && sender->fd >= 0) {
			Callback (sender->msg + HEAD_SIZE, 
			    sender->msg_len, sender->fd, sender->cid);
			called = 1;
			if (Current_fd < 0)
			    return (called);
		    }
		    if (sender->msg_in == msg_len) {
			sender->msg_len = 0;
			sender->msg_in = 0;
		    }
		    else if (sender->msg_in == msg_len + N_EX_BYTES) {
			char c;

			c = sender->msg[sender->msg_in - 1];
			sender->msg[0] = c;
			sender->msg_in = 1;
			sender->msg_len = 0;
		    }
		    else {
			MISC_log ("Internal error in Process_message");
			Close_sender (sender, 1);
			return (called);
		    }
		}
	    }
	    if (ret < rlen + N_EX_BYTES)
		return (called);
	}
	cnt++;
	if (cnt >= MAX_N_MSG_PROCESSED)
	    return (called);
    }
    return (called);
}

/***********************************************************************

    Description: This function closes the sender "sender". The socket 
		closed and the sender is removed from the table. A
		message is sent to the user function telling that the 
		connection is lost.

    Input:	sender - the sender's registration.
		call_cb - non-zero indicates calling the user callback
			is required.

***********************************************************************/

static void Close_sender (Sender_regist *sender, int call_cb)
{
    int fd, i, cnt, last_ind;

#ifdef DBG
printf ("msg_rmtd.c: Close_sender fd %d\n", sender->fd);
#endif

    fd = sender->fd;
    if (Callback != NULL && call_cb && fd >= 0) 
	Callback (NULL, RMT_MSG_CONN_LOST, fd, sender->cid);

    /* find the sender */
    last_ind = -1;
    cnt = 0;
    for (i = 0; i < N_senders; i++) {
	if (Senders[i].fd == fd) {
	    close (fd);
	    Senders[i].fd = -1;
	    if (Senders[i].msg != NULL) {
		free (Senders[i].msg);
		Senders[i].msg = NULL;
	    }
	    Senders[i].buf_size = 0;
	    cnt++;
	    if (Senders[i].cid == RMT_LOCAL_HOST)
		Cr_n_local_senders--;
	}
	if (Senders[i].fd >= 0)
	    last_ind = i;
    }
    Sender_changed = 1;
    N_senders = last_ind + 1;

    if (cnt != 1) {
	MISC_log ("Bad sender registeration (cnt %d)", cnt);
    }

    return;
}

/***********************************************************************

    Closes all message clients. Called in rssd child process.

***********************************************************************/

void MSGD_close_msg_clients () {
    int i;

    close (Msvfd);
    close (Lmsvfd);
    for (i = 0; i < N_senders; i++) {
	if (Senders[i].fd >= 0) {
	    close (Senders[i].fd);
	    Senders[i].fd = -1;
	}
    }
}



