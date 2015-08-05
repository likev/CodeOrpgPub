/****************************************************************
		
	File: rmt.c	
				
	2/24/94

	Purpose: This module contains the basic routines for the 
	RMT client library.

	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:57 $
 * $Id: rmt_client.c,v 1.24 2012/06/14 18:57:57 jing Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */  

/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>


/*** Local include files ***/

#include <rmt.h>
#include "rmt_def.h"
#include <misc.h>
#include <en.h>


/*** Definitions / macros / types ***/

/*** Global functions / variables ***/

/*** Local references / local variables ***/
static int Need_check_all_fds = 0;
static int Lost_conn_in_rpc = 0;	/* flag for Lost_conn_cb control */
static int In_rpc = 0;			/* flag for detecting recursive RPC in 
					   non-threaded mode */

#ifdef THREADED
#include <pthread.h>
static pthread_key_t Ptd_key;
static pthread_once_t Key_init_once = {PTHREAD_ONCE_INIT};

static void Key_init_func ();
static void Free_ptd (void *arg);
static pthread_mutex_t countMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static void (*Lost_conn_cb) (int fd, char *host) = NULL;

static int Process_failure (int ret);
static void Sigpoll_callback (int sig);
static void Resume_sigpoll_fd (int fd);


/********************************************************************

    Description: This function returns the pointer to the per thread 
		data structure. This function will retry until success
		if memory is not available.

    Return:	The pointer.

*********************************************************************/

RMT_per_thread_data_t *RMT_get_per_thread_data (void) {

#ifndef THREADED
    static RMT_per_thread_data_t ptd = 
	{0, NULL, RMT_INVALID, NULL, NULL, NULL, RMT_TIMEDOUT, 0,
			NULL, NULL, 0.0, RMT_COMPRESSION_OFF};
    return (&ptd);

#else
    RMT_per_thread_data_t *ptd;

    pthread_once (&Key_init_once, Key_init_func);
    ptd = (RMT_per_thread_data_t *)pthread_getspecific (Ptd_key);
    if (ptd == NULL) {
	while ((ptd = (RMT_per_thread_data_t *)malloc 
				(sizeof (RMT_per_thread_data_t))) == NULL)
	    msleep (200);
	ptd->user_buf_size = 0;
	ptd->user_buf = NULL;
	ptd->current_fd = RMT_INVALID;
	ptd->small_buf = NULL;
	ptd->large_buf = NULL;
	ptd->uncompressed_buf = NULL;
	ptd->wait_time = RMT_TIMEDOUT;
	ptd->start_time = 0;
	ptd->transfer_callback = NULL;
	ptd->rmt_transfer_event_user_data = NULL;
	ptd->prev_clock = 0.0;
	ptd->compression_status = RMT_COMPRESSION_OFF;
	pthread_setspecific (Ptd_key, (void *)ptd);
    }
    return (ptd);
#endif
}

/****************************************************************

    Registers a progress callback function "callback_in" and 
    associated user data "user_data".
	
****************************************************************/

int RMT_listen_for_progress (
	rmt_transfer_event_callback_t callback_in, void *user_data) {
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();

    ptd->transfer_callback = callback_in;
    ptd->rmt_transfer_event_user_data = user_data;
    return (0);
}

/****************************************************************
			
    Reports transfer progress to the user by calling registered
    callback function "transfer_callback". See Man rmt for more
    details. Returns 0 if the transfer operation should continue,
    or 1 otherwise.
	
****************************************************************/

int RMT_report_progress (int event_in, int seg_flags, 
	int no_of_seg_bytes, int no_of_bytes, int total_no_of_bytes) {
    int ret_value;
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();

    /*  Notify all progress listeners */
    ret_value = 0;
    if (ptd->transfer_callback != NULL) {
	float new_clock;
	rmt_transfer_event_t event;
	time_t t;
	int ms;
 
	t = MISC_systime (&ms);
	new_clock = (float)(t & 0xffff) + ms * .001f;
	event.event = event_in;
	event.segment_flags = seg_flags;
	event.no_of_segment_bytes = no_of_seg_bytes;
	event.no_of_bytes = no_of_bytes;
	event.total_no_of_bytes = total_no_of_bytes;
	event.user_data = ptd->rmt_transfer_event_user_data;
	if ((no_of_bytes == 0) && (no_of_seg_bytes == 0))
	    event.no_of_seconds = 0.0;
	else {
	    event.no_of_seconds = new_clock - ptd->prev_clock;
	    if (event.no_of_seconds < -1.f)
		event.no_of_seconds += 65536.f;
	}
	ptd->prev_clock = new_clock;
 
	if ((*ptd->transfer_callback)(&event) == 1)
	    ret_value = 1;
    }
    return (ret_value);
}

/****************************************************************
			
    Sets compression status of the RMT library. Returns the 
    previous compression status.
	
****************************************************************/

int RMT_set_compression (int compression_flag) {
    int prev_compression;
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();
    prev_compression = ptd->compression_status;
    ptd->compression_status = compression_flag;
    return (prev_compression);
}

/****************************************************************
			
    Returns the compression status of the RMT library
	
****************************************************************/

int RMT_compression () {
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();
    return (ptd->compression_status);
}

/****************************************************************

	This function receives a user specified buffer for the return
	string in the next remote function call.

****************************************************************/

void RMT_use_buffer (int size, char *buf)
{
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();
    if (size > 0) {
	ptd->user_buf_size = size;
	ptd->user_buf = buf;
    }
}

/***************************************************************************

    Returns 1 if the connection to "mach_name" exists or 0 otherwise.

***************************************************************************/

int RMT_is_connected (char *mach_name) { 
    if (SVRG_get_fd_by_name (mach_name) == FAILURE)
	return (0);
    return (1);
}

/****************************************************************
			
	RMT_create_connection()			Date: 2/24/94

	This is the RMT lib function for creating a child server on
	host "mach_name" and making a connection to it. It returns 
	the socket fd of the connection on success.

	Because only a single child on a remote host is created 
	for a client process, the remote host name is registered.
	This function checks all existing connections before trying to 
	generate a new child server and a new connection. If the 
	connection to "mach_name" exists the function simply 
	returns the existing fd.

	This function sets the Current_fd to the fd associated with
	host "mach_name" if the connection is created or exists.

	The function returns one of the following negative values on 
	failure:

	RMT_BAD_REMOTE_HOST_NAME : Null remote_host_name.
	RMT_TOO_MANY_CONNECTIONS : Failed in registering the new server. 
		Probably too many connections have been made.
	RMT_PORT_NUMBER_NOT_FOUND : Failed in finding the RMT port number.
	RMT_OPEN_SOCKET_FAILED : Failed in opening a socket.
	RMT_CONNECT_FAILED : The connect call failed. The server is 
		probably not running.
	RMT_SET_SOCK_PROP_FAILED : Failed in setting socket properties.
	RMT_AUTHENTICATION_FAILED : Failed in passing authentication check.
	RMT_TIMED_OUT: Failed because of timed out.
*/

int
  RMT_create_connection
  (
      char *mach_name		/* name of the remote host to connect  */
) {
    int fd, ret;
    unsigned long ipa;
    RMT_per_thread_data_t *ptd;

    if (mach_name[0] == '\0') {	/* bad name */
	MISC_log ("RMT: Empty host name\n");
	return (RMT_BAD_REMOTE_HOST_NAME);
    }

    if ((fd = SVRG_get_fd_by_name (mach_name)) == FAILURE) {	
				/* if not connected */
	fd = SOC_connect_server (mach_name, &ipa);	
				/* make a new connection */
	if (fd < 0)
	    return (fd);	/* failed in creation */

	if (SVRG_regist_new_server (fd, mach_name, ipa) == FAILURE) {
	    close (fd);
	    return (RMT_TOO_MANY_CONNECTIONS);	/* failed in registration */
	}
	if (Lost_conn_cb != NULL && (ret = EN_set_sigpoll_fd (fd)) < 0) {
	    close (fd);
	    return (ret);	/* failed in set sigpoll */
	}
    }

    ptd = RMT_get_per_thread_data ();
    ptd->current_fd = fd;		/* set up Current_fd */	

    return (fd);
}

/****************************************************************

    Register a callback function so it is called when a remote 
    "host" is detected to be disconnected (e.g. OS shutdown, server
    killed and so on). Returns 0 on success or a negative error 
    code.

****************************************************************/

int RMT_set_lost_conn_callback (void (*cb) (int, char *)) {
    int ret;

    if (Lost_conn_cb != NULL || cb == NULL)
	return (0);
    ret = EN_register_sigpoll_cb (Sigpoll_callback);
    if (ret < 0)
	return (ret);
    Lost_conn_cb = cb;
    return (0);
}

/****************************************************************

    The SIGPOLL callback function.

****************************************************************/

static void Sigpoll_callback (int sig) {
    if (Lost_conn_cb != NULL) {
	if (Lost_conn_in_rpc)
	    Need_check_all_fds = 1;
	else
	    SVRG_test_lost_conns (-1, Lost_conn_cb);
    }
}

/****************************************************************
	
	RMT_set_current()			Date: 2/24/94

	This function sets up the current remote connection to "fd".

	It first makes a valid check on "fd". If the "fd" is
	invalid, it returns RMT_INVALID_FD. The function returns 
	RMT_SUCCESS on success.
*/

int
  RMT_set_current
  (
      int fd			/* socket fd to be set */
) {
    RMT_per_thread_data_t *ptd;

#ifdef THREADED
    if (!RMT_is_fd_valid (fd))
	return (RMT_INVALID_FD);
#else
    if (SVRG_get_server_by_fd (fd) == NULL)
	return (RMT_INVALID_FD);
#endif

    ptd = RMT_get_per_thread_data ();
    ptd->current_fd = fd;		/* set up Current_fd */

    return (RMT_SUCCESS);
}

/****************************************************************
			
	RMT_port_number()			Date: 2/24/94

	This function sets up a user specified port number by 
	calling PNUM_set_port_number.
*/

int
  RMT_port_number
  (
      int port			/* the user specified port number */
) {
    int p;

    if ((p = PNUM_set_port_number (port)) < 0)
	return (RMT_FAILURE);
    else
	return (p);
}

/****************************************************************
			
	RMT_messages()			Date: 2/24/94

	This function is removed.
*/

void
  RMT_messages
  (
      int sw			/* RMT_OFF or RMT_ON */
) {

    return;
}

/****************************************************************
			
	This function closes the current connection if the 
	connection is not locked. The closing will terminate the
	server process on the remote side of the connection.

	This function returns RMT_SUCCESS on success or RMT_FAILURE 
	if "fd" is invalid or the connection is locked. 
*/

int
RMT_close_connection ()
{
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();

    if (ptd->current_fd == RMT_INVALID)
	return (RMT_FAILURE);

    close (ptd->current_fd);
    SVRG_remove_a_server (ptd->current_fd);
    ptd->current_fd = RMT_INVALID;
    return (RMT_SUCCESS);
}

/****************************************************************
			
*/

int RMT_check_connection ()
{
    char *out;
    int ret;

    ret = RMTc_user_func_general (0, 5, "NOOP", &out);
				/* strlen ("NOOP")  + 1 = 5 */
    if (ret < 0)
	return (ret);

    return (RMT_SUCCESS);
}

/****************************************************************
			
*/

#define TMP_BUF_SIZE	256

int RMT_terminate_service (char *host_name, char *host_list)
{
    int fd;
    char str [TMP_BUF_SIZE];
    char *out;
    int ret;

    if (SVRG_get_fd_by_name (host_name) != FAILURE)
	return (RMT_ALREADY_CONNECTED);

    fd = RMT_create_connection (host_name);
    if (fd < 0)
	return (fd);

    strcpy (str, "TERMINATE ");
    strncat (str, host_list, TMP_BUF_SIZE - 10);
					/* strlen ("TERMINATE ") = 10 */
    str [TMP_BUF_SIZE - 1] = '\0';
    ret = RMTc_user_func_general (0, strlen (str) + 1, str, &out);

    /* close the connection */
    close (fd);
    SVRG_remove_a_server (fd);

    if (ret < 0)
	return (ret);

    return (RMT_SUCCESS);
}

/****************************************************************
			
	RMTc_user_func_general()		Date: 2/24/94

	This is the prototype remote procedure call processing routine.
	Every remote function call actually calls this routine.

	The function first checks the calling arguments. If a fatal
	error is found in the arguments, it returns RMT_BAD_ARG_CLIENT_SIDE.
	Note that the RMT tool interprets the lower 24 bit in the 
	"length" argument as the actual length of the input byte string. 

	It then checks if the current socket fd, Current_fd, is valid. 
	This function uses Current_fd for sending request and receiving
	return messages. Before invoking a remote procedure call the client
	program must first set up Current_fd. If Current_fd is not set,
	the function returns RMT_CONNECTION_NOT_SET.

	Then the function generates the request message. It uses a static
	buffer for sending the request. The size of the static buffer,
	STATIC_BUFFER, must be larger than REQ_MSG_SIZE. The first part
	of the input byte string is put is the request message. The remaining
	part of the input byte string, if any, will be sent to the server
	directly after sending the request. 

	After sending the request, the function waits for a return message.
	If the return message is received, the length of the output string
	is found from the message and the remaining part, if any, of the 
	output string is read from the socket. The returned function id is
	compared with the original id. If they are different, a fatal error 
	is detected and the function returns.

	The output byte string is put in the buffer "buf" and returns to the
	caller. If the output byte string is too large for the static buffer,
	a large buffer is dynamically allocated for the output string. 
	The large buffer area is freed next time this function is called.

	The function returns the length of the output string of the 
	remote function on success or a negative number on failure.
	
*/

int
  RMTc_user_func_general
  (
      int id,			/* function number (id) */
      int inp_len,		/* length of the input byte including info 
				   in the highest byte */
      char *in_string,		/* the input byte string */
      char **output_string	/* pointer to output byte string - output */
) {

    RMT_per_thread_data_t *ptd;

    int in_req_len;		/* maximum input string size in the request */
    int ret_id;			/* returned function id */
    int out_len;		/* output string length */
    int in_out_len;		/* maximum size of output string in the
				   return message */
    int input_len;			/* true input data length */
    int ub_size;
    int ret;
    int compressed;		/*  RMT_COMPRESSION_ON or RMT_COMPRESSION_OFF */
    char* input_string;		/*  input string (compressed or not ) */
    int in_len;			/*  input string length (compressed or not) */
    char *buf;
    int progress_report;	/* progress report and cancel enabled */

#ifndef THREADED
    if (In_rpc)
	return (RMT_RPC_REENTRY);
#endif
    In_rpc++;

    ptd = RMT_get_per_thread_data ();
    input_len = inp_len & LENGTH_MASK;
    compressed = RMT_compression();
    if (compressed && input_len >= 256)
	compressed |= 2;

    progress_report = 0;
    if (ptd->transfer_callback != NULL)
	progress_report = 1;

    /* free the large buffer allocated previously */
    if (ptd->large_buf != NULL) {
	free (ptd->large_buf);
	ptd->large_buf = NULL;
    }

    /* user buffer for return - use only once so we reset it here */
    ub_size = ptd->user_buf_size;
    ptd->user_buf_size = 0;

    /* check calling arguments */
    if (id < 0 || id > MAX_NUM_FUNCTIONS) {
	MISC_log ("RMT: Bad function id: %d\n", id);
	In_rpc--;
	return (RMT_BAD_ARG_CLIENT_SIDE);
    }

    /* check if ptd->current_fd is set */
    if (ptd->current_fd == RMT_INVALID) {
	MISC_log ("RMT: Current connection is not yet set\n");
	In_rpc--;
	return (RMT_CONNECTION_NOT_SET);
    }

    if (Lost_conn_cb != NULL) {
	EN_suspend_sigpoll_fd (ptd->current_fd);
#ifdef THREADED
	pthread_mutex_lock (&countMutex);
#endif
	Lost_conn_in_rpc++;
#ifdef THREADED
	pthread_mutex_unlock (&countMutex);
#endif
    }
#ifdef THREADED
    RMT_mt_lock_sv_fd (ptd->current_fd, 1);
#endif

    buf = ptd->small_buf;
    if (buf == NULL) {
	buf = (char *)malloc (STATIC_BUFFER);
	if (buf == NULL) {
	    MISC_log ("RMT: Failed in allocating work space. Size = %d\n",
			STATIC_BUFFER);
	    return (Process_failure (RMT_MALLOC_FAILED));
	}
#ifdef USE_MEMORY_CHECKER
	memset (buf, 0, (REQ_MSG_SIZE <= STATIC_BUFFER)? REQ_MSG_SIZE : STATIC_BUFFER);
#endif
	ptd->small_buf = buf;
    }

    /*  Compress input string */
    if (compressed == 3) {
    	input_string = RMT_compress (input_len, in_string, &in_len);
	if (input_string == NULL) {
	    if (in_len < 0)
		return (Process_failure(RMT_COMPRESSION_FAILED));
	    compressed = 1;
	}
	else {
            inp_len &= (~LENGTH_MASK);
            inp_len |= (in_len & LENGTH_MASK);
	}
    }
    if (compressed != 3) {	/* not compressed */
        input_string = in_string;
        in_len = input_len;
    }

    /* form and send the request message */

    buf [0] = '*';
    buf [1] = compressed;
    buf [2] = progress_report;
    buf [3] = id;
    *((rmt_t *)(buf + 4)) = htonrmt (inp_len);
    in_req_len = REQ_MSG_SIZE - HEAD_SIZE;

    if (in_len < in_req_len) {	/* input string fits in request */
	memcpy (buf + HEAD_SIZE, input_string, in_len);
	ret = SOC_send_msg (ptd->current_fd, 
					REQ_MSG_SIZE, buf, progress_report);
    }
    else {		/* put part of input byte string in request  */
	memcpy (buf + HEAD_SIZE, input_string, in_req_len);
	ret = SOC_send_msg (ptd->current_fd, 
					REQ_MSG_SIZE, buf, progress_report);
	if (ret >= 0)
	   ret = SOC_send_msg (ptd->current_fd, in_len - in_req_len, 
			input_string + in_req_len, progress_report);
    }
    if (ret < 0) {
       if (compressed == 3)
           RMT_free_buffer (input_string);
       return (Process_failure (ret));
    }

#ifdef TEST_OPTIONS
    {
	char buf[1024];

	if (MISC_test_options ("PROFILE")) {
	    static int cnt = 0;
	    MISC_string_date_time (buf, 128, (const time_t *)NULL);
	    fprintf (stderr, "%s: RPC count %d\n", buf, cnt);
	    cnt++;
	}
	if (MISC_test_options ("SIMULATE_SAT_RPC")) {
	    int ms;
	    double st, d;
	    time_t t = MISC_systime (&ms);
	    st = (double)t + ms * .001;
	    while (1) {
		msleep (50);
		t = MISC_systime (&ms);
		d = (double)t + ms * .001;
		if (d >= st + .8)
		    break;
	    }
	}

	if (MISC_test_options ("PRINT_STACK")) {
	    MISC_proc_printstack (getpid (), 1024, buf);
	    fprintf (stderr, "%s\n", buf);
	}
    }
#endif

    /* receive the return message */

    /* read message */
    if ((ret = SOC_recv_msg (ptd->current_fd, 
			RET_MSG_SIZE, buf, progress_report)) < 0)
	return (Process_failure (ret));

    /* parse and check message */
    compressed = buf[1];
    ret_id = buf[3];
    out_len = ntohrmt(*(rmt_t *)(buf + 4));
    if (ret_id != id) {
	MISC_log ("RMT: Incorrect return id: ret_id = %d id = %d\n", 
							ret_id, id);
	return (Process_failure (RMT_BAD_RESPONSE));
    }

    /* read remaining part of output byte string */

    in_out_len = RET_MSG_SIZE - HEAD_SIZE;
    if (in_out_len > out_len)
	in_out_len = out_len;

    if (ub_size > 0) {			/* use user provided buffer */
	if (ub_size < out_len)
	    return (Process_failure (RMT_BAD_USER_BUFFER));
	memcpy (ptd->user_buf, buf + HEAD_SIZE, in_out_len);
	if (out_len > in_out_len &&
	    (ret = SOC_recv_msg (ptd->current_fd, out_len - in_out_len,
			   ptd->user_buf + in_out_len, progress_report)) < 0)
	    return (Process_failure (ret));
	*output_string = ptd->user_buf;
    }
    else {
       /* we need to use internal buffer */
       if (out_len + HEAD_SIZE > STATIC_BUFFER) {	/* need large buffer */
	    if ((ptd->large_buf = (char *) malloc (out_len)) == NULL) {
		MISC_log ("RMT: Failed in allocating work space. Size = %d\n", 
						out_len);
	        return (Process_failure (RMT_MALLOC_FAILED));
	    }

	    memcpy (ptd->large_buf, buf + HEAD_SIZE, in_out_len);
	    if (out_len > in_out_len &&
		(ret = SOC_recv_msg (ptd->current_fd, out_len - in_out_len, 
			ptd->large_buf + in_out_len, progress_report)) < 0)
	       return (Process_failure (ret));
	   *output_string = ptd->large_buf;
       }
       else if (out_len > in_out_len) {	/* use static buffer */
  	   if ((ret = SOC_recv_msg (ptd->current_fd, out_len - in_out_len,
			   buf + RET_MSG_SIZE, progress_report)) < 0)
	       return (Process_failure (ret));
  	   *output_string = buf + HEAD_SIZE;
       }
       else			/* no remaining part of output string */
	   *output_string = buf + HEAD_SIZE;
    }

    if (compressed) {

	/* free the ptd->uncompressed_buf allocated previously */
	if (ptd->uncompressed_buf != NULL) {
	    RMT_free_buffer (ptd->uncompressed_buf);
	    ptd->uncompressed_buf = NULL;
        }
        
	ptd->uncompressed_buf = RMT_decompress (out_len, *output_string, 
							&out_len);
	if (ptd->uncompressed_buf == NULL)
	   return (Process_failure (RMT_DECOMPRESSION_FAILED));
	
	if (ub_size > 0 && ub_size >= out_len) {
	    memcpy(ptd->user_buf, ptd->uncompressed_buf, out_len);
	    *output_string = ptd->user_buf;
	}
	else
	    *output_string = ptd->uncompressed_buf;	
     }
     if (Lost_conn_cb != NULL)
	 Resume_sigpoll_fd (ptd->current_fd);
     In_rpc--;
#ifdef THREADED
     RMT_mt_lock_sv_fd (ptd->current_fd, 0);
#endif

    return(out_len);
}

/****************************************************************

    Resumes sigpoll for fd.

****************************************************************/

static void Resume_sigpoll_fd (int fd) {

    EN_resume_sigpoll_fd (fd);
#ifdef THREADED
    pthread_mutex_lock (&countMutex);
#endif
    if (Lost_conn_in_rpc > 0)
	Lost_conn_in_rpc--;
    if (Lost_conn_in_rpc == 0) {
	if (Need_check_all_fds)
	    SVRG_test_lost_conns (-1, Lost_conn_cb);
	else
	    SVRG_test_lost_conns (fd, Lost_conn_cb);
	Need_check_all_fds = 0;
    }
    else
	Need_check_all_fds = 1;
#ifdef THREADED
    pthread_mutex_unlock (&countMutex);
#endif
}

/****************************************************************
			
	Process_failure ()			Date: 2/24/94

	This function is called if the remote function call failed
	in sending the request or receiving the response. It
	closes the child server and the connection and returns the
	error number.
*/

static int
Process_failure (int ret)
{
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();
    if (ret != RMT_CANCELLED) {
	SVRG_remove_a_server (ptd->current_fd);
	close (ptd->current_fd);
	ptd->current_fd = RMT_INVALID;
    }
    else if (Lost_conn_cb != NULL)
	Resume_sigpoll_fd (ptd->current_fd);
    In_rpc--;
#ifdef THREADED
    RMT_mt_lock_sv_fd (ptd->current_fd, 0);
#endif

    return (ret);
}

/*****************************************************************

	This function returns the fd of the current connection.
*/

int RMTc_get_current_fd (void)
{
    RMT_per_thread_data_t *ptd;

    ptd = RMT_get_per_thread_data ();
    return (ptd->current_fd);
}

#ifdef THREADED

/********************************************************************

    Description: This function initializes thread specific data key 
		for the per thread data structure. Because it is 
		difficult to return to the caller, the function 
		terminates the application in certain error contitions.

*********************************************************************/

static void Key_init_func (void)
{
    int status;

    status = 0;
    while ((status = pthread_key_create (&Ptd_key, Free_ptd)) != 0) {
	if (status == ENOMEM) {
	    msleep (200);
	    continue;
	}
	MISC_log ("pthread_key_create failed (ret %d)\n", status);
	exit (1);
    }
}

/********************************************************************

    Description: This function frees thread specific data key for 
		the per thread data structure.

    Input:	arg - pointer to the data.

*********************************************************************/

static void Free_ptd (void *arg)
{
    RMT_per_thread_data_t *ptd = (RMT_per_thread_data_t *)arg;
    if (ptd->small_buf != NULL)
	free (ptd->small_buf);
    if (ptd->large_buf != NULL)
	free (ptd->large_buf);
    if (ptd->uncompressed_buf != NULL) {
	RMT_free_buffer (ptd->uncompressed_buf);
    }
    ptd->small_buf = ptd->large_buf = ptd->uncompressed_buf = NULL;

    free (ptd);
}

#endif

