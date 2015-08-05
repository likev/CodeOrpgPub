/****************************************************************
		
	File: sock_rmt.c	
				
	2/24/94

	Purpose: This module contains the communication routines 
	for the RMT client library. This implementation uses UNIX
	socket functions.

	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:35 $
 * $Id: rmt_sock_cl.c,v 1.20 2012/07/27 19:33:35 jing Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#ifdef __WIN32__
#define __INTERIX
#endif
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HPUX
#include <arpa/inet.h>
#endif
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>		/* for IPPROTO_TCP value */
#include <netinet/tcp.h>	/* for TCP_NODELAY value */
#include <arpa/inet.h>

/*** Local include files ***/

#include <rmt.h>
#include <net.h>
#include <misc.h>
#include "rmt_def.h" 


/*** Definitions / macros / types ***/


/*** External references / external global variables ***/


static void (*Cmd_disc_cb) (void) = NULL;
					/* user cmd_disc callback function */

/*** Local references / local variables ***/
static int Send_oob (int fd, RMT_per_thread_data_t *ptd);


/*******************************************************************
				
    Sets up the timed out value to "time" for the subsequent remote 
    calls.

*******************************************************************/

int RMT_set_time_out (int time) {
    RMT_per_thread_data_t *ptd;
    int previous;

    ptd = RMT_get_per_thread_data ();
    previous = ptd->wait_time;
    if (time <= 0)
	ptd->wait_time = RMT_TIMEDOUT;
    else
	ptd->wait_time = time;
    return (previous);
}

/*******************************************************************
				
    An older version of RMT_set_time_out.

*******************************************************************/

void RMT_time_out (int time) {

    RMT_set_time_out (time);
    return;
}

/****************************************************************
			
	SOC_connect_server()			Date: 2/24/94

	This function makes a connection to the RMT server on a
	remote host "hostname". Refer to "UNIX Network Programming"
	for how to use the socket functions.

	If the connection is made, suitable socket properties are 
	set and an authentication procedure is conducted by calling
	SEC_pass_security_check.

	If everything is OK, the function returns the socket file
	descriptor. Otherwise it returns a negative value indicating
	one the following error conditions:

	RMT_PORT_NUMBER_NOT_FOUND : Failed in finding a port number.
	RMT_OPEN_SOCKET_FAILED : Failed in opening a socket.
	RMT_CONNECT_FAILED : The connect call failed. The server is probably 
		not running.
	RMT_SET_SOCK_PROP_FAILED : Failed in setting socket properties.
	RMT_TIMED_OUT: Failed because of timed out.
*/

int
  SOC_connect_server
  (
      char *hostname,		/* remote host name */
      unsigned long *ipa	/* returns IP address for the connection */
) {
    int sockfd;			/* socket file descriptor */
    struct sockaddr_in rem_soc;	/* remote host address */
    int port;			/* port number of RMT server */
    int tm, ret;
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();

    memset ((char *) &rem_soc, 0, sizeof (rem_soc));
    rem_soc.sin_family = AF_INET;

    port = PNUM_get_port_number ();	/* get the port number */
    if (port == FAILURE) {
	MISC_log ("RMT: Failed in finding the RMT port number\n");
	return (RMT_PORT_NUMBER_NOT_FOUND);
    }

    rem_soc.sin_addr.s_addr = NET_get_ip_by_name (hostname);
    if (rem_soc.sin_addr.s_addr == INADDR_NONE) {
	return (RMT_GETHOSTBYNAME_FAILED);
    }

    /* if the IP address is connected since an aliased name, we don't create
	a new connection */
    *ipa = rem_soc.sin_addr.s_addr;
    if ((sockfd = SVRG_check_ip (rem_soc.sin_addr.s_addr)) >= 0)
	return (sockfd);

    rem_soc.sin_port = htons ((unsigned short)port); /* fill in port number */

    /* open a socket for connecting to the remote port */
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
	MISC_log ("RMT: socket failed (errno = %d)\n", errno);
	return (RMT_OPEN_SOCKET_FAILED);
    }
    fcntl (sockfd, F_SETFD, FD_CLOEXEC);

    /* set socket properties */
    if ((ret = NET_set_TCP_NODELAY (sockfd)) < 0 ||
				/* This is not important for this module */
	(ret = NET_set_non_block (sockfd)) < 0 ||
	(ret = NET_set_linger_off (sockfd)) < 0 ||
	(ret = RMT_bind_selected_local_ip (sockfd)) < 0) {
	close (sockfd);
	return (ret);
    }

    /* Connect the local socket to the remote server */
    tm = MISC_systime (NULL);
    while (connect (sockfd, (struct sockaddr *) &rem_soc,
		 sizeof (rem_soc)) < 0)
    {

	int saved_errno = errno;

	if (saved_errno == EINPROGRESS || saved_errno == EALREADY)
	{
	    /* not available for write - retry I/O  */
	    RMT_report_progress(RMT_IO_RETRY, 0, 0, 0, 0);

	    if (MISC_systime (NULL) > tm + ptd->wait_time) { /* connect time out */
		close (sockfd);
		RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
		return (RMT_TIMED_OUT);
	    }
	    msleep (100);
	    continue;
	}
	if (saved_errno == EISCONN)
	    break;

	if (saved_errno == EINTR)
        {
	    continue;
	}
	MISC_log ("RMT: connect to %s failed (errno = %d)\n", 
					hostname, saved_errno);

	close (sockfd);
	RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
	return (RMT_CONNECT_FAILED);
    }

    if ((ret = SEC_pass_security_check (sockfd)) != SUCCESS) {
	MISC_log ("RMT: authentication failed in connecting to %s\n", 
						hostname);
	close (sockfd);
	return (ret);
    }

    return (sockfd);
}

/****************************************************************
			
    Sets the selected local address for socket "sockfd".

****************************************************************/

int RMT_bind_selected_local_ip (int sockfd) {
    char buf[64];
    unsigned int ip;

    if (PNUM_get_local_ip_from_rmtport () == INADDR_NONE) /* no need to bind */
	return (0);
    if (RMT_lookup_host_index (RMT_LHI_IX2I, &ip, 0) > 0) {
	static struct sockaddr_in l_soc;	/* local socket info */
	l_soc.sin_addr.s_addr = ip;
	l_soc.sin_port = 0;			/* any unused port number */
	l_soc.sin_family = AF_INET;
	if (bind (sockfd, (struct sockaddr *) &l_soc, sizeof (l_soc)) < 0) {
	    MISC_log ("NET: bind selected local IP (%s) failed (%d)\n", 
				NET_string_IP (ip, 1, buf), errno);
	    return (NET_BIND_LOCAL_IP_FAIED);
	}
    }
    return (0);
}

/****************************************************************
			
	This function writes "len" bytes data in "data" to socket "fd".

	If the connection to the server is broken, this function can
	cause the application to exit due to a SIGPIPE signal. Note 
	that we can not protect the application by blocking the signal
	since after sigrelse, the process will exit. If we use sig 
	ignore, we might also ignore the calling program's signal, 
	which will cause confusion for the calling program. Thus the 
	best thing we can do is probably doing nothing to protect the 
	process from exiting and letting the application to protect 
	itself if there is a need. The application can do this by 
	calling sigignore.

	The starting time of the request is recorded in ptd->start_time.

	This function can be called with msg_len = 0 for setting
	ptd->start_time. This is useful for authentication messaging.

	The function returns RMT_SUCCESS on success one of the following
	negative numbers on failure:
	RMT_SELECT_FAILED, RMT_WRITE_FAILED ,RMT_SERVER_DISCONNECTED,
	RMT_TIMED_OUT, or RMT_CANCELLED.
*/

int
  SOC_send_msg
  (
      int fd,			/* socket fd */
      int len,			/* length of the message to be sent */
      char *data,		/* the message to be sent */
      int progress_report	/* progress report and cancelation enabled */ 
				   
) {
    int n_written;		/* number of byte written */
    int ret;
    int cancel_transfer;
    RMT_per_thread_data_t *ptd;
    time_t b_time;		/* latest block start time */

    ptd = RMT_get_per_thread_data ();
/*    ptd->start_time = MISC_systime (NULL);*/	/* prepare for times out */

    if (len == 0) 
	return (RMT_SUCCESS);	/* see SOC_send_msg note */

    cancel_transfer = 0;
    if (progress_report) 
	cancel_transfer = RMT_report_progress(RMT_DATA_SENT, 0, 0, 0, len);	

    n_written = 0;
    b_time = 0;
    while (!cancel_transfer) {
	int k;

	errno = 0;
	k = write (fd, &data[n_written], len - n_written);

	if (k < 0) {

	    int saved_errno = errno;

	    if (saved_errno == EWOULDBLOCK || saved_errno == EAGAIN)
	    {
	        /* not available for write - retry I/O */
	        RMT_report_progress(RMT_IO_RETRY, 0, 0, 0, 0);

		if (b_time == 0)
		    b_time = MISC_systime (NULL);
		else if (MISC_systime (NULL) > b_time + ptd->wait_time)
		{
		    RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
		    return (RMT_TIMED_OUT);		/* timed out */
		}

		if ((ret = SCSH_wait (fd, SELECT_WRITE, CHECK_PERIOD)) < 0)
		{
		    RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
		    return (RMT_SELECT_FAILED);	/* select call error */
		}

		continue;
	    }

	    else if (saved_errno == EBADF) {	/* socket disconnected */
		RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
		return (RMT_SERVER_DISCONNECTED);
	    }

	    else if (saved_errno != EINTR) {	/* fatal write error */
		MISC_log ("RMT: write failed (SOC_send_msg) (error = %d)\n",
			    saved_errno);
		RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
		return (RMT_WRITE_FAILED);
	    }
	}

	if (k > 0) {
	    b_time = 0;
	    n_written += k;
	}

	if (n_written >= len)
	{
	    if (progress_report)
		cancel_transfer = RMT_report_progress(RMT_DATA_SENT, 
					RMT_LAST_SEGMENT, k, n_written, len);
	    break;
	}
	if (progress_report)
	    cancel_transfer = RMT_report_progress(RMT_DATA_SENT, 
					0, k, n_written, len);
    }

    if (cancel_transfer) {
	Send_oob (fd, ptd);
        return(RMT_CANCELLED);
    }
    else
	return (RMT_SUCCESS);
}

/****************************************************************
			
	SOC_recv_msg()				Date: 2/24/94

	This function reads "n_bytes" byte data from socket "fd"
	and puts it in "msg".

	The function calls "select" to wait for data to come. It 
	wakes up every CHECK_PERIOD seconds to check if the remote
	call time is expired. 

	It returns RMT_SUCCESS on success and a negative number 	
	indicating the error condition. Possible error numbers are: 
	RMT_SELECT_FAILED, RMT_SERVER_DISCONNECTED, RMT_TIMED_OUT
	RMT_READ_FAILED, and RMT_CANCELLED
*/

int
  SOC_recv_msg
  (
      int fd,			/* socket fd */
      int n_bytes,		/* number of bytes to read */
      char *msg,		/* the buffer for output */
      int progress_report	/* progress report and cancelation enabled */ 			   
) {
    int n_read;			/* number of bytes read */
    int ret;
    int cancel_transfer;	/* 1 if this transfer should be cancelled,
				   0 otherwise */
    RMT_per_thread_data_t *ptd;
    time_t b_time;		/* latest block start time */

    ptd = RMT_get_per_thread_data ();

    /* wait until a message comes */
    if ((ret = SCSH_wait (fd, SELECT_READ, CHECK_PERIOD)) < 0)
	return (RMT_SELECT_FAILED);

    cancel_transfer = 0;
    if (progress_report) 
	cancel_transfer = RMT_report_progress(RMT_DATA_RECEIVED, 
							0, 0, 0, n_bytes);

    n_read = 0;
    b_time = 0;
    while (!cancel_transfer) {
	int k;			/* read call return value */

	errno = 0;
	if ((k = read (fd, &msg[n_read], n_bytes - n_read)) == 0) {	/* socket
							   disconnected */
	    RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
	    return (RMT_SERVER_DISCONNECTED);
	}

	if (k < 0) {

	    int saved_errno = errno;

	    if (saved_errno == EWOULDBLOCK || saved_errno == EAGAIN)
	    {
		RMT_report_progress(RMT_IO_RETRY, 0, 0, 0, 0);

		if (b_time == 0)
		    b_time = MISC_systime (NULL);
		else if (MISC_systime (NULL) > b_time + ptd->wait_time)
		{
		    RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
		    return (RMT_TIMED_OUT);		/* timed out */
		}

		if ((ret = SCSH_wait (fd, SELECT_READ, CHECK_PERIOD)) < 0)
		{
		    RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
		    return (RMT_SELECT_FAILED);	/* select call error */
		}

		continue;
	    }

	    else if (saved_errno != EINTR) {	/* fatal read error */
		MISC_log ("RMT: read failed (SOC_recv_msg) (errno = %d)\n", 
							saved_errno);
		RMT_report_progress(RMT_DISCONNECT, 0, 0, 0, 0);
		return (RMT_READ_FAILED);
	    }

	    continue;
	}

	n_read += k;
	if (k > 0)
	    b_time = 0;

	if (n_read >= n_bytes)
	{
	    if (progress_report)
		cancel_transfer = RMT_report_progress(RMT_DATA_RECEIVED, 
				RMT_LAST_SEGMENT, k, n_read, n_bytes);	
	    break;
	}
	if (progress_report)
	    cancel_transfer = RMT_report_progress(RMT_DATA_RECEIVED, 
				0, k, n_read, n_bytes);
	
    }

    if (cancel_transfer) {
	Send_oob (fd, ptd);
        return(RMT_CANCELLED);
    }
    else
       return(RMT_SUCCESS);
}

/****************************************************************
			
	This function writes a out-of-band byte.

    Input:	fd - socket fd;
		ptd - per thread data.

	The function returns RMT_SUCCESS on success one of the following
	negative numbers on failure:
	RMT_SELECT_FAILED, RMT_WRITE_FAILED ,RMT_SERVER_DISCONNECTED,
	RMT_TIMED_OUT, or RMT_CANCELLED.
*/

static int Send_oob (int fd, RMT_per_thread_data_t *ptd)
{
    int b_time = 0;

    while (1) {
	int ret;
	char data = 0;

	ret = send (fd, &data, 1, MSG_OOB);

	if (ret < 0) {

	    if (errno == EWOULDBLOCK || errno == EAGAIN) {
					/* not available for write */
		if (b_time == 0)
		    b_time = MISC_systime (NULL);
		else if (MISC_systime (NULL) > b_time + ptd->wait_time) {
		    return (RMT_TIMED_OUT);		/* timed out */
		}

		if ((ret = SCSH_wait (fd, SELECT_WRITE, CHECK_PERIOD)) < 0)
		    return (RMT_SELECT_FAILED);	/* select call error */

		continue;
	    }

	    else if (errno == EBADF) {	/* socket disconnected */
		return (RMT_SERVER_DISCONNECTED);
	    }

	    else if (errno != EINTR) {	/* fatal write error */
		MISC_log ("RMT: send failed (SOC_send_msg) (error = %d)\n",
			    errno);
		return (RMT_WRITE_FAILED);
	    }
	}
	else				/* done */
	    return (RMT_SUCCESS);
    }
}

/********************************************************************
			
    Returns 1 if host IP "ipaddr" (NBO)) is commanded disconnected. It 
    returns 0 otherwise. This function reread rssd_disc to get the 
    list of hosts that are detected to be disconnected. If there is
    a newly disconnected hosts, it closes all existing messaging sockets
    to the disconnected hosts.

********************************************************************/

#define MAX_DISC_IPS 32

int RMT_cmd_disconnected (unsigned int ipaddr) {
    static time_t last_read_time = 0;
    static int recursive_call = 0, n_disc_ips = 0;
    static unsigned int disc_ips[MAX_DISC_IPS];
    int i;
    time_t cr_t;

    if (ipaddr == 0 || ipaddr == RMT_LOCAL_HOST)
	return (0);

    cr_t = MISC_systime (NULL);
    if (cr_t >= last_read_time + 2 &&
	!recursive_call) {		/* re-read the file */
	unsigned int ips[MAX_DISC_IPS];
	char buf[MAX_DISC_IPS * 20];
	int n_ips, k, new_disc;

	last_read_time = cr_t;
	if (RMT_access_disc_file (0, buf, MAX_DISC_IPS * 20) < 0)
	    return (0);
	n_ips = 0;
	while (n_ips < MAX_DISC_IPS &&
	       MISC_get_token (buf, "Cx", n_ips, ips + n_ips, 0) > 0)
	    n_ips++;
	new_disc = 0;
	for (i = 0; i < n_ips; i++) {
	    for (k = 0; k < n_disc_ips; k++) {
		if (disc_ips[k] == ips[i])
		    break;
	    }
	    if (k >= n_disc_ips)
		new_disc = 1;
	}
	for (i = 0; i < n_ips; i++)
	    disc_ips[i] = ips[i];
	n_disc_ips = n_ips;
	if (new_disc) {			/* New disc host found */
	    recursive_call = 1;
	    RMTSM_close_disc_msg_hosts ();
	    if (Cmd_disc_cb != NULL)
		Cmd_disc_cb ();
	    recursive_call = 0;
	}
    }

    for (i = 0; i < n_disc_ips; i++) {
	if (ipaddr == disc_ips[i])
	    return (1);
    }
    return (0);
}
