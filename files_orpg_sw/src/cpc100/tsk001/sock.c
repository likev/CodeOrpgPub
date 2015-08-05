/******************************************************************

    Description: This module contains common socket access utility
	functions for the BCAST tools.

******************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2000/12/18 21:05:07 $
 * $Id: sock.c,v 1.12 2000/12/18 21:05:07 jing Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>		/* for IPPROTO_TCP value */
#include <netinet/tcp.h>	/* for TCP_NODELAY value */
#include <unistd.h>

#include <bcast_def.h>
#include <misc.h>

#define PAUSE_TIME	20	/* pause time when waiting on TCP port */

extern char Prog_name [];	/* program name defined in the main
				   module */

static int Sig_pipe = 0;	/* flag indicating a SIGPIPE is received */



/*********************************************************************
			
    Description: This function sets TCP socket properties. It disables 
		short data buffering in TCP level. For efficiency 
		reason, the TCP buffers frequent short messages and 
		tries to send them in large chunk. However this can 
		reduce performance and is not acceptable. It turns off 
		the linger feature to avoid hanging around stopped 
		processes.It sets the socket to be non-blocking.

    Returns:	This function returns 0 on success or -1 on failure.

*********************************************************************/

int SOCK_set_TCP_properties (int fd)
{
    int i = 1;
    struct linger lig;		/* parameter for setting SO_LINGER */

    /* disable buffering of short data in TCP level */
    i = 1;
    errno = 0;
    if (setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, (char *)&i, 
					sizeof (int)) < 0) {
	fprintf (stderr, "setsockopt TCP_NODELAY failed (errno = %d) - %s\n", 
					errno, Prog_name);
	return (-1);
    }

    /* turn off linger */
    lig.l_onoff = 0;
    lig.l_linger = 0;
    if (setsockopt (fd, SOL_SOCKET, SO_LINGER, (char *)&lig, 
			sizeof (struct linger)) < 0) {
	fprintf (stderr, "setsockopt SO_LINGER failed (errno = %d) - %s\n", 
					errno, Prog_name);
	return (-1);
    }

    /* set non-block IO */
#ifdef HPUX
    if (fcntl (fd, F_SETFL, O_NONBLOCK) < 0) {
#else
    if (fcntl (fd, F_SETFL, O_NDELAY) < 0) {
#endif
	close (fd);
	fprintf (stderr, "fcntl O_NDELAY failed (errno = %d) - %s\n", 
					errno, Prog_name);
	return (-1);
    }

    return (0);
}

/********************************************************************
			
    Description: This function reads "n_bytes" bytes of data form the 
		socket "fd" into "msg_buf". This function will retry 
		until the required number of bytes is read if some 
		data is read.

    Return:	It returns the number of bytes read on success, 0 if 
		there is no data or -1 if the socket is disconnected.

    Notes: 	This function terminates the program if a fatal error 
 		is detected.

********************************************************************/

int SOCK_read_tcp (int fd, int n_bytes, char *msg_buf) 
{
    int n_read;				/* number of bytes read */

    n_read = 0;
    while (1) {
	int k;

	if (n_read >= n_bytes)
	    return (n_read);

	errno = 0;
	if ((k = read (fd, &msg_buf[n_read], n_bytes - n_read)) == 0) {		
					/* socket disconnected */
	    fprintf (stderr, "socket read returns 0\n");
	    return (-1);
	}
	if (k < 0) {
	    if (errno == ECONNRESET) {
		fprintf (stderr, "errno = ECONNRESET\n");
		return (-1);
	    }
	    else if (errno == EWOULDBLOCK || errno == EAGAIN) {
		if (n_read == 0)
		    return (0);
		msleep (PAUSE_TIME);
		continue;
	    }
	    else if (errno != EINTR) {
		fprintf (stderr, "read failed (errno = %d) - %s\n",
				errno, Prog_name);
		MAIN_exit ();
	    }
	    continue;
	}
	n_read += k;
    }
}

/******************************************************************
			
    Description: This function writes "msg_size" bytes of data in 
		"msg" to the socket "fd". A SIGPIPE signal received, 
		when calling write(), indicates that the socket is 
		disconnected by the other end. If the socket is 
		blocked, it will retry until the socket is 
		available for write. 

    Return:	It returns 0 on success or -1 if the socket is 
		disconnected.

    Notes: 	This function terminates the program if a fatal 
		error is detected.

******************************************************************/

int SOCK_write_tcp (int fd, int msg_size, char *msg) 
{
    int n_written;

    Sig_pipe = 0;
    n_written = 0;
    while (1) {
	int k;

	errno = 0;
	k = write (fd, &msg[n_written], msg_size - n_written);
	if (Sig_pipe) {			/* socket disconnected */
	    fprintf (stderr, "SIGPIPE received\n");
	    return (-1);
	}
	if (k < 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN) {
		msleep (PAUSE_TIME);
		continue;
	    }
	    if (errno != EINTR) {	/* fatal error */
		fprintf (stderr, "write failed (errno = %d) - %s\n",
			 		errno, Prog_name);
		MAIN_exit ();
	    }
	    continue;
	}
	if (k > 0)
	    n_written += k;
	if (n_written >= msg_size)
	    return (0);
    }
}

/******************************************************************
			
    Description: The SIGPIPE interrupt call back function. This 
		function sets up the Sig_pipe flag.

******************************************************************/

void SOCK_sigpipe_int ()
{

    Sig_pipe = 1;
    return;
}

/******************************************************************
			
    Compares two sequence numbers and returns true (non-zero) or
    false.

******************************************************************/

static unsigned int Max_seq_num = 1, Seq_check1, Seq_check2;

int SOCK_seq_cmp (int op, unsigned int arg1, unsigned int arg2) {
    int less;		/* at least less than */

    less = 0;
    if (arg1 == 0 || arg2 == 0) {
	if (arg1 == 0)
	    less = 1;
    }
    else {
	if (arg1 < arg2) {
	    less = 1;
	    if (arg1 < Seq_check1 && arg2 > Seq_check2)
		less = 0;
	}
	else {
	    if (arg2 < Seq_check1 && arg1 > Seq_check2)
		less = 1;
	}
    }
    switch (op) {
	case LESS_THAN:
	    if (less && arg1 != arg2)
		return (1);
	    else 
		return (0);
	case GREATER_THAN:
	    if (!less && arg1 != arg2)
		return (1);
	    else 
		return (0);
	case GREATER_EQUAL:
	    if (!less || arg1 == arg2)
		return (1);
	    else 
		return (0);
	case LESS_EQUAL:
	    if (less || arg1 == arg2)
		return (1);
	    else 
		return (0);
    }
    return (0);
}

/******************************************************************
			
    Increments a sequence number by "inc" which is always positive.
    inc is assumed to be <= pb_size.

******************************************************************/

unsigned int SOCK_seq_add (unsigned int seq, int inc) {
    unsigned int t;
    t = seq + inc;
    if (t >= Max_seq_num)
	t = (t % Max_seq_num) + 1;
    return (t);
}

/******************************************************************
			
    Evaluates Max_seq_num - the max sequence number is 
    Max_seq_num - 1. Note that the sequence number starts with 0.
    Sequence number of 0 is not used for a normal seq number. Rather
    it is used for control purposes. When the sequence number reaches
    Max_seq_num, it falls back to 1. Note that Max_seq_num must be
    a multiple of pb_size plus 1. We also need to reserve a small
    range (at least pb_size) on top.

******************************************************************/

void SOCK_set_max_seq (int pb_size) {

    if (pb_size == 0)
	return;
    Max_seq_num = pb_size * ((0xffffffff / pb_size) - 2) + 1;
/*    Max_seq_num = pb_size * 5 + 1; for testing */
    Seq_check1 = Max_seq_num / 4;
    Seq_check2 = Seq_check1 * 3;
    return;
}









