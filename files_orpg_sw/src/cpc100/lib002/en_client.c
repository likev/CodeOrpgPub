/****************************************************************
		
    Description: This module implements the client part of the 
		event notification functions.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/30 20:15:36 $
 * $Id: en_client.c,v 1.44 2012/07/30 20:15:36 jing Exp $
 * $Revision: 1.44 $
 * $State: Exp $
 */

/* System include files */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
/* t_uscalar_t fff; */
#include <stdlib.h> 
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h> 
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef SUNOS
#include <stropts.h>           /* ioctl()                                 */
#include <sys/conf.h>          /* ioctl()                                 */
#endif

/* Local include files */

#include <net.h>
#include <rmt.h>
#include <misc.h>
#include <net.h>
#include <en.h>
#include "en_def.h"

#ifdef EN_THREADED
#include <pthread.h>
#endif

#define MALLOC_WAIT_TIME 200	/* In case of mallloc failure we retry in this
				   module until success. This is the retry
				   period in ms. */

#define CB_REGISTER_NO_ARG	((void *)(0x1))
				/* special value for Cb_regist_t.arg */

typedef struct {			/* client side callback registration */
    int code;				/* event code (LB fd for UN or 
					   EN_AN_CODE for AN regi.) */
    EN_id_t evtid;			/* event id */
    void (*cb_func)(EN_id_t, char *, int, void *);
					/* callback function */
    void *arg;				/* callback argument */
    int redeliver;			/* the previous event needs redeliver */
} Cb_regist_t;

static Cb_regist_t *Cb_regist = NULL;	/* the callback func table */
static void *Cb_tblid = NULL;		/* table id for Cb_regist */
static int N_cb_regists = 0;		/* # of entries in Cb_regist array */

static void *Cr_arg = CB_REGISTER_NO_ARG;
					/* current callback argument */
static void *New_arg = CB_REGISTER_NO_ARG;
					/* new callback argument */

static int Callback_registered = 0;	/* the callback is registered for
					   signal Ntf_signal */
static int Ntf_signal = SIGPOLL;	/* notification signal */
static int Sv_fd = -1;			/* the socket fd to the local server */
static int Pid = -1;			/* this process's pid */
static int Ntf_buf_size	= 1024 + sizeof (Ntf_msg_t);
					/* buffer size for NTF messages */

static unsigned int Local_ip = INADDR_NONE;
					/* IP address of local machine */
static int Local_hid = 0;		/* host index of the local host */
static unsigned short Sender_id = 0;	/* Local NTF sender's ID */
static unsigned short In_sender_id = 0;	/* Incoming NTF sender's ID */
static unsigned int An_self_flags = 0;	/* not-to-self and self_only flags */

static Ntf_msg_t Ack_msg;		/* local server registration ack msg */
static int N_in_buf = 0;		/* number of bytes already read in the
					   local sv sock buf (see Callback). */

static int Lb_msg_expired = 0;		/* LB_MSG_EXPIRED */

static int (*Set_un_req) (int, EN_id_t, unsigned int, 
					int, int *, int *) = NULL;
					/* func setting UN request records */

static void (*Notify_error) (char *) = NULL;
					/* error reporting function */
static char *(*Process_message) (int, EN_id_t, char *, int) = NULL;
					/* message processing function */

static int Ntf_ready = 0;		/* cntl flag: NTF ready for delievery */
static int Wait_for_reg_ack = 0;	/* cntl flag: in state of waiting for
					   NTF reg ack */
static int Ntf_redeliver = 0;		/* the current NTF needs to be 
					   redelivered */

#define AN_GROUP_OFFSET 20		/* offset for group ID */
#define AN_GROUP_MASK 0xfffff		/* mask for user AN ID */
#define MAX_AN_GROUP_NUM 15		/* valid AN group number range is 0 - 
					   this */
static int An_group = -2;		/* The AN group number (-1 undefined,
					   -2 uninitialized) */
static int Received_an_group = 0;	/* The source group number of the 
					   latest received AN */

static int N_cbs = 0;			/* # of NTF callbacks processed - used
					   by EN_control (EN_CTL_WAIT) */

static int Lost_cnt = 0;		/* number of lost NTFs */
static int N_reg_rhosts;		/* number of rhosts in recent AN 
					   registration */

static int Snd_stored_bytes = 0;	/* number of stored bytes managed by
					   Store_NTF_data () */
enum {SND_READ, SND_SAVE, SND_DELETE, SND_BACK, SND_INIT};
					/* values used by argument "func" of
					   Store_NTF_data () */

static int Notify_send = 1;		/* notification send state (boolean) */

static void (*External_sigpoll_cb) (int) = NULL;
					/* the external sigpoll callback */
static int Query_hosts_received = 0;
static char *Saved_query_hosts = NULL;	/* the latest query hosts message */

/* passing parameters for EN_multi_register */
static char *Multi_reg_sent = NULL;

#ifdef EN_THREADED

#include <pthread.h>
#define Callback(a) Mt_Callback (a)
static pthread_mutex_t Lb_ntf_register_mutex = PTHREAD_MUTEX_INITIALIZER;
static void Mt_get_mutex_lock (pthread_mutex_t *mutex);
static void Mt_get_mutex_unlock (pthread_mutex_t *mutex);
static void Mt_Callback (int sig);
static int Is_sig_blocked ();
static int Lock_count = 0;
static pthread_t Mutex_owner = -1;
#else
#define Callback(a) Callback_impl(a)
#endif

static int Register_callback ();
static void Callback_impl (int sig);
static int Register_cb (int code, EN_id_t evtid, 
		void (*notify_func)(EN_id_t, char *, int, void *));
static int Register_local_server (int code, EN_id_t evtid, 
		int a_pid, int a_code, 
		void (*notify_func)(EN_id_t, char *, int, void *));
static void Lb_deregister (int index);
static void Disconnect_server ();
static char *Read_sv_data (int n_bytes);
static int Store_NTF_data (int func, char *buf, int len);
static EN_id_t Get_group_event (EN_id_t event);
static char *Read_sv_failed (int ret);
static int Post_return (int ret);
static int Poll_local_server (int ms);
static int Internal_register (int code, EN_id_t evtid, 
		void (*notify_func)(EN_id_t, char *, int, void *));
static int Sig_wait (int signo, int ms);
static int En_control_internal (int cntl_function, va_list args);
static int Set_signal_callback ();
static int Sig_hold (int yes);
static int Sig_mask (int how, const sigset_t *set, sigset_t *oset);
static int Set_sig_handler (int sig, void (*handler)(int));


/****************************************************************
						
    Accepts LB constants from the LB module.

****************************************************************/

void EN_set_lb_constants (int lb_msg_expired, 
	int (*set_un_req)(int, EN_id_t, unsigned int, int, int *, int *)) {
    Lb_msg_expired = lb_msg_expired;
    Set_un_req = set_un_req;
}

/****************************************************************
						
    Sets/gets EN parameters.

****************************************************************/

void EN_parameters (int func, unsigned short *sender_id, int *notify_send) {
    if (func == EN_GET) {
	if (sender_id != NULL)
	    *sender_id = Sender_id;
	if (notify_send != NULL)
	    *notify_send = Notify_send;
    }
    else {
	if (sender_id != NULL)
	    Sender_id = *sender_id;
	if (notify_send != NULL)
	    Notify_send = *notify_send;
    }
}

/****************************************************************
						
    Description: This registers an EN callback function.

    Input:	event - event id;
		notify_func - the notification callback function;

    Returns:	This function returns EN_SUCCESS on success or a 
		negative LB error number.

****************************************************************/

int EN_register (EN_id_t event, 
		void (*notify_func)(EN_id_t, char *, int, void *)) {

#ifdef TEST_OPTIONS
    if (MISC_test_options ("PROFILE")) {
	char b[128];
	MISC_string_date_time (b, 128, (const time_t *)NULL);
	if (event > 0x10000)
	    fprintf (stderr, "%s PROF: Register AN 0x%x\n", b, event);
	else
	    fprintf (stderr, "%s PROF: Register AN %d\n", b, event);
    }
#endif
    event = Get_group_event (event);

    if (event > EN_MAX_ID && event != EN_ANY && event != EN_REPORT &&
		event != EN_QUERY_HOSTS)
	return (EN_BAD_ARGUMENT);

    return (EN_internal_register (EN_AN_CODE, event, notify_func));
}

/****************************************************************
						
    Registers an EN callback function. It calls Internal_register
    to do the job. Refer to Internal_register.

****************************************************************/

int EN_internal_register (int code, EN_id_t event, void (*notify_func)(EN_id_t, char *, int, void *)) {
    int ret;

    if (Pid < 0)
	Pid = getpid ();
    else if (Pid != getpid ()) {
	int k;
	for (k = 0; k < N_cb_regists; k++)	/* remove parent regists */
	    Cb_regist[k].code = -1;
	Disconnect_server ();
	Pid = getpid ();
    }

    if (!Callback_registered && Ntf_signal == SIGPOLL &&
			getenv ("EN_DEFAULT_SIGUSR1") != NULL)
	Ntf_signal = SIGUSR1;

    Cr_arg = New_arg;
    New_arg = CB_REGISTER_NO_ARG;	/* remove the callback arg */

    EN_internal_block_NTF ();
#ifdef EN_THREADED
    Mt_get_mutex_lock (&Lb_ntf_register_mutex);
#endif
    ret = Internal_register (code, event, notify_func);
#ifdef EN_THREADED
    Mt_get_mutex_unlock (&Lb_ntf_register_mutex);
#endif
    EN_internal_unblock_NTF ();
    return (ret);
}

/********************************************************************
			
    Description: This function is the internal implementation of
		registering an NTF callback function.

    Input:	code - event code;
		evtid - event id;
		notify_func - the notification callback function;

    Returns:	This function returns EN_SUCCESS on success or a 
		negative LB error number.

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

static int Internal_register (int code, EN_id_t evtid, 
			void (*notify_func)(EN_id_t, char *, int, void *))
{
    EN_per_thread_data_t *ptd;
    int ret;

    if (Local_ip == INADDR_NONE) {
	unsigned int lhost;
	Local_hid = RMT_lookup_host_index (RMT_LHI_IX2I, &lhost, 0);
	if (Local_hid < 0)
	    return (EN_LOCAL_IP_NOT_FOUND);
	Local_ip = lhost;
    }

    ptd = EN_get_per_thread_data (); /* init per thread data struct */
    if (ptd->in_callback) {
	MISC_log ("EN: Event register called in callback\n");
	return (EN_REG_FAILED);
    }

    /* connect to the local server */
    if (Sv_fd < 0) {
	int sfd;

	if ((ret = RMT_send_msg (Local_ip, NULL, 0, &sfd)) < 0)
	    return (ret);
	Sv_fd = sfd;

	Store_NTF_data (SND_INIT, NULL, 0);	/* init Store_NTF_data */
	if (Read_sv_data (0) == NULL ||		/* init Read_sv_data */
	    EN_post_msgevent (EN_ANY, NULL, 0) == EN_MALLOC_FAILED)
						/* init EN_publish */
	    return (EN_MALLOC_FAILED);
    }

    if (!Callback_registered) {	/* register the client callback function */
	if ((ret = Register_callback ()) < 0)
	    return (ret);
    }

    if (code < 0 && code != EN_AN_CODE)
	return (EN_BAD_ARGUMENT);

    if (ptd->Deregister) {
	int i;

	ptd->Deregister = 0;
	for (i = N_cb_regists - 1; i >= 0; i--) {
	    if (Cb_regist[i].code == code && Cb_regist[i].evtid == evtid &&
		(notify_func == NULL || Cb_regist[i].cb_func == notify_func))
		Lb_deregister (i);
	}
	return (EN_SUCCESS);
    }

    if (notify_func == NULL)
	return (EN_BAD_ARGUMENT);

    /* register callback function */
    Query_hosts_received = 0;
    ret = Register_cb (code, evtid, notify_func);
    if (ret < 0)
	return (ret);

    if (code == EN_AN_CODE && evtid == EN_QUERY_HOSTS) {
	time_t t;
	int save = ptd->ntf_block_intern;
	if (EN_control (EN_GET_BLOCK_STATE) > 0)
	    return (EN_WAS_BLOCKED);
	ptd->ntf_block_intern = 0;
	if (ret == 0) {
	    t = 0;
	    while (1) {
		Callback (-1);
		if (Query_hosts_received)
		    break;
		if (t == 0)
		    t = MISC_systime (NULL);
		else if (MISC_systime (NULL) > t + 10) {
		    ptd->ntf_block_intern = save;
		    return (EN_REG_FAILED);
		}
		msleep (200);
	    }
	}
	else {
	    if (Saved_query_hosts != NULL)
	        notify_func (evtid, Saved_query_hosts, 
				strlen (Saved_query_hosts) + 1, Cr_arg);
	}
	ptd->ntf_block_intern = save;
    }

    return (EN_SUCCESS);
}

/********************************************************************
			
    See En_control_internal.

********************************************************************/

int EN_control (int cntl_function, ...) {
    va_list args;

    va_start (args, cntl_function);
    return (En_control_internal (cntl_function, args));
}

/********************************************************************
			
    Description: This function sets various control functions for 
		LB update notification.

    Input:	cntl_function - the control function to set;

    Returns:	This function returns EN_SUCCESS on success or a 
		negative LB error number.

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

static int En_control_internal (int cntl_function, va_list args) {
    int ret;

    if (!Callback_registered && Ntf_signal == SIGPOLL &&
			getenv ("EN_DEFAULT_SIGUSR1") != NULL)
	Ntf_signal = SIGUSR1;

    ret = EN_SUCCESS;
    switch (cntl_function) {
	int un_send, ms, group, an_size;
	sigset_t old_mask;
	EN_per_thread_data_t *ptd;

	case EN_BLOCK:
	    Sig_hold (1);
	    ptd = EN_get_per_thread_data ();
	    ret = ptd->ntf_block;
	    ptd->ntf_block++;
	    break;

	case EN_GET_BLOCK_STATE:
	    ptd = EN_get_per_thread_data ();
	    ret = ptd->ntf_block;
	    break;

	case EN_UNBLOCK:
	    ptd = EN_get_per_thread_data ();
	    ret = ptd->ntf_block;
	    if (ptd->ntf_block > 0)
		ptd->ntf_block--;
	    if (ptd->ntf_block == 0) {
		if (!(ptd->in_callback) && Ntf_ready)
		    Callback (-1);
		Sig_hold (0);
	    }
	    break;

	case EN_REDELIVER:
	    ptd = EN_get_per_thread_data ();
	    if (!(ptd->in_callback))
		ret = EN_NOT_SUPPORTED;
	    Sig_hold (1);
	    ptd->ntf_block = 1;
	    Ntf_redeliver = 1;
	    break;

	case EN_WAIT:
	    ptd = EN_get_per_thread_data ();
	    if (ptd->in_callback)
		ret = EN_NOT_SUPPORTED;
	    else {
		ms = va_arg (args, int);
		if (Ntf_signal >= 0) {
		    Sig_mask (0, NULL, &old_mask);
		    if (ms > 0)
			Sig_hold (1);
    
		    N_cbs = 0;
		    ptd->ntf_block = 0;
		    if (ptd->ntf_block_intern != 0)
			MISC_log (
			    "unexpected non-zero ntf_block_intern (%d)\n", 
			    ptd->ntf_block_intern);  
		    Callback (-1);
		    if (N_cbs == 0 && ms > 0) {
			if (Sig_wait (Ntf_signal, ms) < 0)
			    ret = EN_SIG_WAIT_FAILED; 
		    }
		    if (ms > 0)
			Sig_hold (0);
		    Sig_mask (SIG_SETMASK, &old_mask, NULL);
		}
		else {
		    ptd->ntf_block = 0;
		    if (Poll_local_server (ms) > 0 || Ntf_ready) {
			N_cbs = 0;
			while (1) {	/* processes all pending events */
			    int n;
			    n = N_cbs;
			    Callback (-1);
			    if (n == N_cbs)
				break;
			}
		    }
		}
	    }
	    break;

	case EN_DEREGISTER:
	    ptd = EN_get_per_thread_data ();
	    ptd->Deregister = 1;
	    break;

	case EN_SET_ERROR_FUNC:
	    Notify_error = (void (*)(char *))va_arg (args, void *);
	    break;

	case EN_SET_PROCESS_MSG_FUNC:
	    Process_message = (char *(*)(int, EN_id_t, char *, int))
						va_arg (args, void *);
	    break;

	case EN_SET_SIGNAL:
	    if (Callback_registered)
		ret = EN_NOT_SUPPORTED;
	    else
		Ntf_signal = va_arg (args, int);
	    break;

	case EN_GET_SIGNAL:
	    ret = Ntf_signal;
	    break;

	case EN_SET_UN_SEND_STATE:
	    un_send = va_arg (args, int);
	    if (un_send == EN_UN_SEND_GET)
		ret = Notify_send;
	    else if (Notify_send == 0 && 
			un_send == EN_UN_SEND_TMP_DISABLE)
		ret = 0;
	    else {
		ret = Notify_send;
		Notify_send = un_send;
	    }
	    break;

	case EN_SET_SENDER_ID:
	    Sender_id = (va_arg (args, int)) & 0xffff;
	    break;

	case EN_PUSH_ARG:
	    New_arg = va_arg (args, void *);
	    break;

	case EN_TO_SELF_ONLY:
	    An_self_flags |= AN_SELF_ONLY;
	    break;

	case EN_NOT_TO_SELF:
	    An_self_flags |= AN_NOT_TO_SELF;
	    break;

	case EN_GET_N_RHOSTS:
	    ret = N_reg_rhosts;
	    break;

	case EN_SET_AN_GROUP:
	    ret = An_group;
	    group = va_arg (args, int);
	    if (group >= -1 && group <= MAX_AN_GROUP_NUM)
		An_group = group;
	    break;

	case EN_GET_AN_GROUP:
	    ret = Received_an_group;
	    break;

	case EN_SET_AN_SIZE:
	    an_size = va_arg (args, int);
	    if (an_size < 512)
		an_size = 512;
	    Ntf_buf_size = an_size + sizeof (Ntf_msg_t);
	    break;

	case EN_GET_NTF_FD:
	    if (Sv_fd < 0)
		RMT_send_msg (Local_ip, NULL, 0, &Sv_fd);
	    ret = Sv_fd;
	    break;

	case EN_GET_IN_CALLBACK:
	    if (Ntf_signal < 0)
		ret = 0;
	    else {
		ptd = EN_get_per_thread_data ();
		ret = ptd->in_callback;
	    }
	    break;

	default :
	    break;
    }

    va_end (args);
    return (ret);
}

/********************************************************************
			
    Description: This function returns the lost NTF count for the 
		latest NTF callback.

********************************************************************/

int EN_event_lost () {

    return (Lost_cnt);
}

/********************************************************************
			
    Description: This function returns the NTF sender's ID of the 
		latest NTF.

********************************************************************/

int EN_sender_id () {

    return (In_sender_id);
}

/********************************************************************
			
    Description: This function reports an unreachable host for LB NTF.

    Input:	host - the host IP.

********************************************************************/

void EN_print_unreached_host (unsigned int host) {

    if (Notify_error != NULL) {		/* call error reporting */
	char tmp[128];

	sprintf (tmp, "send NTF failed - can not reach %x\n", 
					(unsigned int) ntohl (host));
	Notify_error (tmp);
    }
    return;
}

/********************************************************************
			
    Description: This function blocks the NTF internally. This 
		function must be disabled if called within the callback.

********************************************************************/

void EN_internal_block_NTF () {
    EN_per_thread_data_t *ptd;

    ptd = EN_get_per_thread_data ();
    if (ptd->in_callback)
	return;
    ptd->ntf_block_intern++;
    return;
}

/********************************************************************
			
    Description: This function removes an internal NTF block. This 
		function must be disabled if called within the callback.

********************************************************************/

void EN_internal_unblock_NTF () {
    EN_per_thread_data_t *ptd;

    ptd = EN_get_per_thread_data ();
    if (ptd->in_callback)
	return;
    ptd->ntf_block_intern--;
    if (ptd->ntf_block_intern < 0) 
	MISC_log ("unexpected negative ntf_block_intern\n"); 
    if (ptd->ntf_block_intern > 0)
	return;
    if (Ntf_ready)
	Callback (-1);

    return;
}

/********************************************************************
			
    Description: This function writes a message to the local server.
		It will block if the socket buffer is full. In an
		error condition it disconnects the socket.

    Input:	host - the server host IP address (NBO).
		msg - the message to send.
		msg_len - length of the message.

    Return:	EN_SUCCESS on success or a negative LB error message.

********************************************************************/

int EN_send_to_server (unsigned int host, char *msg, int msg_len) {
    int ret, cnt;

    if (msg_len < 0)
	return (EN_SUCCESS);

    cnt = 0;
    while (1) {				/* blocking write */
	ret = RMT_send_msg (host, msg, msg_len, NULL);
	cnt++;
	if (ret < 0) {
	    if (host == Local_ip)
		Disconnect_server ();
	    else {
		if (cnt <= 1)		/* we try twice */
		    continue;
		RMT_send_msg (host, NULL, -1, NULL);
	    }
	    return (ret);
	}
	if (ret == msg_len)
	    break;
	msleep (100);
	if (cnt > 4 &&
	    RMT_cmd_disconnected (host))
	    return (EN_FAILURE);
    };

    return (EN_SUCCESS);
}

/********************************************************************
			
    Description: This function deregisters all notification entries
		associated with event code "code". This function is called 
		by LB_close.

    Input:	code - event code;

********************************************************************/

void EN_close_notify (int code) {
    int i;

    EN_internal_block_NTF ();
    for (i = N_cb_regists - 1; i >= 0; i--) {
	if (Cb_regist[i].code == code)
	    Lb_deregister (i);
    }
    EN_internal_unblock_NTF ();
    return;
}

/********************************************************************
			
    Description: This function registers a notification item in the 
		local server by sending a message to it. This function
		will wait until an ack is received. If notify_func == 
		NULL, it performs deregisteration and there is no ack.
		Because we use the same socket for NTF and registration,
		while we wait for reg ACK and the NTF is blocked, we 
		have to save the NTF for later processing. This is 
		managed by Store_NTF_data ().

    Input:	code - event code;
		evtid - event id;
		a_pid - aliased pid;
		a_code - aliased event code;
		notify_func - notification callback function.

    Returns:	This function returns EN_SUCCESS on success or a 
		negative LB error number.

********************************************************************/

static int Register_local_server (int code, EN_id_t evtid, int a_pid, 
		int a_code, void (*notify_func)(EN_id_t, char *, int, void *))
{
    Ntf_regist_msg_t msg;
    int ret;

    if (Sv_fd < 0)	/* disable this function (See Disconnect_server) */
	return (EN_LOCAL_SV_NOT_CONN);

    if (notify_func == NULL)
	msg.signal = -1;
    else {
	msg.signal = Ntf_signal;
	if (Ntf_signal < 0)		/* tell server not to send signal */
	    msg.signal = SIGPOLL;
    }

    msg.msg_type = FROM_CLIENT_REG;
    msg.pid = Pid;
    msg.code = code;
    msg.evtid = evtid;
    msg.a_pid = a_pid;
    msg.a_code = a_code;

    Ack_msg.msg_type = 0;		/* clear Ack message buffer */
    ret = EN_send_to_server (Local_ip, (char *)&msg, 
					sizeof (Ntf_regist_msg_t));
    if (ret < 0)
	return (ret);

    if (notify_func == NULL)		/* no ack for deregisteration */
	return (EN_SUCCESS);

    if (Multi_reg_sent != NULL) {
	*Multi_reg_sent = 1;
	return (EN_SUCCESS);
    }

    /* waiting for ack message */
    Wait_for_reg_ack = 1;
    if (Ntf_ready)
	Callback (-1);
    ret = 0;
    while (1) {
	if (Sv_fd < 0) {		/* connection lost */
	    ret = EN_CON_LOST;
	    break;
	}
	if (Ack_msg.msg_type == TO_CLIENT_ACK) {
	    if (Ack_msg.code != code || Ack_msg.evtid != (int)evtid) {
					/* this should never happen */
/*		Disconnect_server (); */
		ret = EN_UNEXP_ACK;
		break;
	    }
	    if (Ack_msg.msg_len == EN_NTF_FAILED)
		ret = EN_REG_FAILED;
	    break;			/* registration completed */
	}
	Poll_local_server (200);	/* if not SIGPOLL, no signals */
	Callback (-1);
    }
    Wait_for_reg_ack = 0;
    if (ret < 0)
	return (ret);
    else
	N_reg_rhosts = Ack_msg.sender_id;

    return (EN_SUCCESS);
}

/********************************************************************
			
    Description: This function registers the callback function to the
		new Ntf_signal.

    Returns:	This function returns EN_SUCCESS on success or a 
		negative error number.

********************************************************************/

static int Register_callback ()
{
    int ret;

    if (External_sigpoll_cb != NULL && Ntf_signal != SIGPOLL)
	return (EN_NOT_SUPPORTED);

    Callback_registered = 0;
    if (Sv_fd < 0)
	return (EN_CON_LOST);

    if ((ret = Set_signal_callback ()) < 0)
	return (ret);

    if (Ntf_signal == SIGPOLL) {
	ret = EN_set_sigpoll_fd (Sv_fd);
	if (ret < 0) {
	    Set_sig_handler (Ntf_signal, SIG_DFL);
	    return (ret);
	}
    }

    Callback_registered = 1;

    return (EN_SUCCESS);
}

/********************************************************************
			
    Sets the signal handler to "handler" for signal "sig". This is 
    different from "sigset" in that it does not change the caller's
    sigmask.

********************************************************************/

static int Set_sig_handler (int sig, void (*handler)(int)) {
    struct sigaction act;
    int ret;

    ret = sigaction (sig, NULL, &act);
    if (ret < 0) {
	MISC_log ("EN: sigaction (get %d) failed (errno %d)\n", sig, errno);
	return (EN_SIGSET_FAILED);
    }
    act.sa_handler = handler;
    act.sa_flags &= ~SA_SIGINFO;
    ret = sigaction (sig, &act, NULL);
    if (ret < 0) {
	MISC_log ("EN: sigaction (set %d) failed (errno %d)\n", sig, errno);
	return (EN_SIGSET_FAILED);
    }
    return (0);
}

/********************************************************************
			
    Adds "callback" as the external sigpoll callback function. This 
    is to support RPC lost connection detection. Returns 0 on success
    or a negative error code.

********************************************************************/

int EN_register_sigpoll_cb (void (*callback) (int)) {
    int ret;

    if (External_sigpoll_cb != NULL)
	return (0);
    if (Ntf_signal != SIGPOLL)
	return (EN_NOT_SUPPORTED);
    ret = Set_signal_callback ();
    if (ret < 0)
	return (ret);
    External_sigpoll_cb = callback;
    return (0);
}

/********************************************************************
			
    Registers multiple application events in one call. This is more
    efficient for satellite connection. When this function fails, some
    of the events may be registered and the future EN_register may not
    work correctly. The caller should terminate if this function fails.
    Returns n_events on success of a negative error code. This cannot be
    called in multi-threaded program since we pass parameters with
    global variables.

********************************************************************/

int EN_multi_register (EN_id_t *events, int n_events,
		void (*notify_func)(EN_id_t, char *, int, void *)) {
    char sent[256];
    int i, ret;

    if (n_events > 256)
	return (EN_BAD_ARGUMENT);
    EN_internal_block_NTF ();
    for (i = 0; i < n_events; i++) {
	int ret;
	sent[i] = 0;
	Multi_reg_sent = sent + i;	/* turn off ACK waiting */
	ret = EN_register (events[i], notify_func);
	Multi_reg_sent = NULL;
	if (ret < 0) {
	    EN_internal_unblock_NTF ();
	    return (ret);
	}
    }

    /* waiting for acks */
    Ack_msg.msg_type = 0;		/* clear Ack message buffer */
    Wait_for_reg_ack = 1;		/* turn on callback processing */
    if (Ntf_ready)
	Callback (-1);
    ret = 0;
    while (1) {
	if (Sv_fd < 0) {		/* connection lost */
	    ret = EN_CON_LOST;
	    break;
	}
	if (Ack_msg.msg_type == TO_CLIENT_ACK) {
	    for (i = 0; i < n_events; i++) {
		if (sent[i] == 0)
		    continue;
		if (Get_group_event (events[i]) == Ack_msg.evtid)
		    break;
	    }
	    if (i >= n_events) {
		ret = EN_UNEXP_ACK;
		break;
	    }
	    sent[i] = 0;
	    if (Ack_msg.msg_len == EN_NTF_FAILED) {
		ret = EN_REG_FAILED;
		break;
	    }
	    Ack_msg.msg_type = 0;	/* clear Ack message buffer */
	    N_reg_rhosts = Ack_msg.sender_id;
	}
	for (i = 0; i < n_events; i++) {
	    if (sent[i])
		break;
	}
	if (i >= n_events)
	    break;			/* registration completed */
	Poll_local_server (200);	/* if not SIGPOLL, no signals */
	Callback (-1);
    }
    Wait_for_reg_ack = 0;
    EN_internal_unblock_NTF ();
    if (ret < 0)
	return (ret);
    return (n_events);
}

/********************************************************************
			
    Sets the local sigpoll callback function.

********************************************************************/

static int Set_signal_callback () {

#ifdef EN_THREADED
    if (Ntf_signal >= 0 && 
	Set_sig_handler (Ntf_signal, Mt_Callback) < 0)
	return (EN_SIGSET_FAILED);
#else
    Sig_hold (0);
    if (Ntf_signal >= 0 &&
	Set_sig_handler (Ntf_signal, Callback_impl) < 0)
	return (EN_SIGSET_FAILED);
#endif
    return (0);
}

/********************************************************************
	
    Adds "fd" to the list of file descriptors that trig sigpoll.
    Returns 0 on success or a negative error code.

********************************************************************/

int EN_set_sigpoll_fd (int fd) {

#ifdef LINUX
    pid_t pid;
    int setown;

    pid = getpid ();
    setown = fcntl (fd, F_SETOWN, pid);
#ifdef EN_THREADED
    if (setown < 0)	/* In a thread, we have to set process group id */
	setown = fcntl (fd, F_SETOWN, -pid);
#endif
    if (setown < 0) {
	MISC_log ("fcntl (F_SETOWN) failed (errno %d)\n", errno);
	return (EN_FCNTL_FAILED);
    }
    if (fcntl (fd, F_SETSIG, SIGPOLL) < 0) {
	MISC_log ("fcntl (F_SETSIG) failed (errno %d)\n", errno);
	return (EN_FCNTL_FAILED);
    }
    if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_ASYNC) < 0) {
	MISC_log ("fcntl (F_SETFL) failed (errno %d)\n", errno);
	return (EN_FCNTL_FAILED);
    }
#endif
#ifdef IRIX
    pid_t pid;

    pid = getpid ();
    if (fcntl (fd, F_SETOWN, pid) < 0 ||
	fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | FASYNC) < 0) {
	MISC_log ("fcntl (F_SETOWN, F_SETFL) failed (errno %d)\n", errno);
	return (EN_FCNTL_FAILED);
    }
#endif
#if (defined (SUNOS) || defined (HPUX))
    if (ioctl (fd, I_SETSIG,
                   S_INPUT  | S_RDNORM | S_RDBAND | S_HIPRI |
                   S_OUTPUT | S_WRNORM | S_WRBAND | S_MSG |
                   S_ERROR  | S_HANGUP) < 0) {
	MISC_log ("ioctl (I_SETSIG) failed (errno %d)\n", errno);
	return (EN_IOCTL_FAILED);
    }
#endif
    return (0);
}

/********************************************************************
	
    Suspends sigpoll for "fd".

********************************************************************/

void EN_suspend_sigpoll_fd (int fd) {

#ifdef LINUX
    fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) & (~O_ASYNC));
#endif
#ifdef IRIX
    fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) & (~FASYNC));
#endif
#if (defined (SUNOS) || defined (HPUX))
    ioctl (fd, I_SETSIG, 0);
#endif
}

/********************************************************************
	
    Resumes sigpoll for "fd".

********************************************************************/

void EN_resume_sigpoll_fd (int fd) {

#ifdef LINUX
    fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_ASYNC);
#endif
#ifdef IRIX
    fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | FASYNC);
#endif
#if (defined (SUNOS) || defined (HPUX))
    ioctl (fd, I_SETSIG,
                   S_INPUT  | S_RDNORM | S_RDBAND | S_HIPRI |
                   S_OUTPUT | S_WRNORM | S_WRBAND | S_MSG |
                   S_ERROR  | S_HANGUP);
#endif
}

/********************************************************************
			
    Description: The Ntf_signal callback function.

    Input:	sig - the signal number;

********************************************************************/

static void Callback_impl (int sig)
{
    static int redelivery_ntf = 0;	/* the current NTF is redelivery */
    char *buf;
    Ntf_msg_t *msg;
    EN_per_thread_data_t *ptd;

    if (Wait_for_reg_ack && sig >= 0)
	return;

    if (External_sigpoll_cb != NULL && sig == SIGPOLL)
	External_sigpoll_cb (SIGPOLL);
    if (sig < 0)			/* not an interrupt */
	sig = Ntf_signal;

    if (Sv_fd < 0 || sig != Ntf_signal)
	return;

    Ntf_ready = 1;
    ptd = EN_get_per_thread_data ();
    if (ptd->in_callback)
	return;

    if (!Wait_for_reg_ack && (ptd->ntf_block_intern || ptd->ntf_block))
	return;

    /* read in all to-client messages and process them */
    ptd->in_callback++;
    Ntf_ready = 0;
    while (1) {
	int code, i, len, ntf_back;
	EN_id_t evtid;
	char emsg[128];

	len = sizeof (Ntf_msg_t);
	buf = Read_sv_data (len);
	if (buf == NULL || N_in_buf < len) {
	    ptd->in_callback--;
	    return;
	}

	msg = (Ntf_msg_t *)buf;
	if (msg->msg_type == TO_CLIENT_AN &&	/* read in AN message */
					msg->msg_len > 0) {
	    len += msg->msg_len;
	    buf = Read_sv_data (len);
	    if (buf == NULL || N_in_buf < len) {
		ptd->in_callback--;
		return;
	    }
	    if (len > Ntf_buf_size) {		/* AN message too large */
		if (Notify_error != NULL)
		    Notify_error ("Unexpected large AN - discarded\n");
		N_in_buf = 0;			/* discarded */
		continue;
	    }
	}

	N_in_buf = 0;
	msg = (Ntf_msg_t *)buf;
	if (msg->msg_type == TO_CLIENT_ACK) {
	    memcpy ((char *)&Ack_msg, buf, sizeof (Ntf_msg_t));
	    ptd->in_callback--;
	    Ntf_ready = 1;
	    return;
	}

	if (msg->msg_type != TO_CLIENT_UN && msg->msg_type != TO_CLIENT_AN) {
	    Disconnect_server ();
	    if (Notify_error != NULL) {	/* call error reporting */
		sprintf (emsg, 
		    "unknown message type (%x, size %d) from local server\n", 
					(unsigned int)msg->msg_type, len);
		Notify_error (emsg);
	    }
	    continue;
	}

	if (Wait_for_reg_ack) {
	    if (len <= Ntf_buf_size)
		Store_NTF_data (SND_SAVE, (char *)msg, len);
	    continue;
	}

	evtid = msg->evtid;
	code = msg->code;
	ntf_back = 0;		/* NTF has been put back */
	for (i = 0; i < N_cb_regists; i++) {
	    Cb_regist_t *cb_reg;
	    int found;

	    found = 0;
	    Ntf_redeliver = 0;
	    cb_reg = Cb_regist + i;
	    if (msg->msg_type == TO_CLIENT_AN && /* AN */
		cb_reg->code == EN_AN_CODE &&
		(cb_reg->evtid == evtid || cb_reg->evtid == EN_ANY)) {
		EN_id_t id;

		if (redelivery_ntf && !cb_reg->redeliver)
		    continue;
		id = evtid;
		if (An_group >= 0 &&
			id <= (AN_GROUP_MASK | (0xf << AN_GROUP_OFFSET))) {
		    Received_an_group = (id >> AN_GROUP_OFFSET) & 0xf;
		    id &= AN_GROUP_MASK;
		}
		Lost_cnt = msg->lost_cnt;
		In_sender_id = EN_SHORT_BSWAP (msg->sender_id);
		if (Process_message != NULL && msg->msg_len > 0)
		    Process_message (EN_MSG_IN, id, 
				buf + sizeof (Ntf_msg_t), msg->msg_len);
		cb_reg->cb_func (id, 
			buf + sizeof (Ntf_msg_t), msg->msg_len, cb_reg->arg);
		if (evtid == EN_QUERY_HOSTS) {
		    if (Saved_query_hosts != NULL)
			free (Saved_query_hosts);
		    Saved_query_hosts = MISC_malloc (msg->msg_len + 1);
		    strcpy (Saved_query_hosts, buf + sizeof (Ntf_msg_t));
		    Query_hosts_received = 1;
		}
		found = 1;
	    }
	    else if (msg->msg_type == TO_CLIENT_UN && 
		cb_reg->code > 0 && code == cb_reg->code && 	/* UN */
		cb_reg->evtid == evtid) {
		if (redelivery_ntf && !cb_reg->redeliver)
		    continue;
		Lost_cnt = msg->lost_cnt;
		In_sender_id = (unsigned int)EN_SHORT_BSWAP (msg->sender_id);
		if (evtid == (EN_id_t)Lb_msg_expired)
		    cb_reg->cb_func (code, 
			(char *)(msg->lbmsgid), Lb_msg_expired, cb_reg->arg);
		else
		    cb_reg->cb_func (code, 
			(char *)(msg->lbmsgid), msg->msg_len, cb_reg->arg);
		found = 1;
	    }
	    if (found) {
		N_cbs++;
		if (Ntf_redeliver)
		    cb_reg->redeliver = 1;
		else
		    cb_reg->redeliver = 0;
	    }
	    if (Ntf_redeliver) {
		if (!ntf_back)
		    Store_NTF_data (SND_BACK, (char *)msg, len);
		ntf_back = 1;
	    }
	}
	if (ntf_back)
	    redelivery_ntf = 1;
	else
	    redelivery_ntf = 0;
	if (ptd->ntf_block) {	/* blocked within callback */
	    Ntf_ready = 1;
	    break;
	}
    }

    ptd->in_callback--;
    return;
}

#ifdef EN_THREADED

/********************************************************************
			
    Description: Thread save Ntf_signal callback function.

    Input:	sig - the signal number;

********************************************************************/

static void Mt_Callback (int sig)
{
     Mt_get_mutex_lock (&Lb_ntf_register_mutex);
     Callback_impl (sig);
     Mt_get_mutex_unlock (&Lb_ntf_register_mutex);
     return;
}
#endif

/********************************************************************
			
    Description: This function reads in data from the local NTF server.
		It reallocates the buffer if necessary. It uses a 
		static buffer. If "n_bytes" is larger than the buffer
		size, extra bytes are read and discarded.
		If a fatal error is encountered when reading the
		the data, it sends NTF error report, discards the
		data and disconnects the connection.

    Input:	n_bytes - expected number of bytes of the message;

    Return:	The pointer to the data on success or NULL on failure.

********************************************************************/

static char *Read_sv_data (int n_bytes)
{
    static char *buffer = NULL;
    int ret, n, l;

    if (buffer == NULL) {
	if ((buffer = (char *)malloc (Ntf_buf_size)) == NULL)
	    return (NULL);
    }
    if (n_bytes <= 0)
	return (buffer);
    n = n_bytes;		/* number of bytes to read in buffer */
    if (n > Ntf_buf_size)
	n = Ntf_buf_size;

    if (Snd_stored_bytes > 0 && !Wait_for_reg_ack) {
	if (Store_NTF_data (SND_READ, 
		buffer + N_in_buf, n - N_in_buf) == EN_SUCCESS)
	    N_in_buf = n;
	else {
	    Disconnect_server ();
	    if (Notify_error != NULL)	/* call error reporting */
		Notify_error ("Store_NTF_data error\n");
	    return (NULL);
	}
    }

    if (N_in_buf >= n_bytes)
	return (buffer);

    ret = NET_read_socket (Sv_fd, buffer + N_in_buf, n - N_in_buf);
    if (ret < 0)
	return (Read_sv_failed (ret));
    N_in_buf += ret;

    /* read and discard extra bytes */
    while (N_in_buf >= n && N_in_buf < n_bytes) {
	char b[128];
	l = n_bytes - N_in_buf;
	if (l > 128)
	    l = 128;
	if ((ret = NET_read_socket (Sv_fd, b, l)) < 0)
	    return (Read_sv_failed (ret));
	N_in_buf += ret;
	if (ret == 0)
	    msleep (100);
    }

    return (buffer);
}

/********************************************************************
			
    Post processing of server data read failure. Returns NULL.

********************************************************************/

static char *Read_sv_failed (int ret) {
    Disconnect_server ();
    if (Notify_error != NULL) {	/* call error reporting */
	if (ret == NET_DISCONNECTED)
	    Notify_error ("connection to local server lost\n");
    }
    N_in_buf = 0;
    return (NULL);
}

/********************************************************************
			
    Description: This function registers a callback function in
		the local table. It then conducts local server regist.
		and NR update if this is a new (code, evtid) entry.

    Input:	code - event code;
		evtid - event id;
		notify_func - the notification callback function;

    Returns:	This function returns EN_SUCCESS or a negative EN
		error number.

********************************************************************/

static int Register_cb (int code, EN_id_t evtid, 
			void (*notify_func)(EN_id_t, char *, int, void *))
{
    int found, i;
    Cb_regist_t *cb;
    int new_ind, err;

    /* create and get the callback func table */
    if (Cb_tblid == NULL &&
	(Cb_tblid = MISC_open_table (sizeof (Cb_regist_t), 
			16, 0, &N_cb_regists, (char **)&Cb_regist)) == NULL)
	return (EN_MALLOC_FAILED);

    found = 0;
    /* look for existing entry */
    for (i = 0; i < N_cb_regists; i++)
	if (code == Cb_regist[i].code && evtid == Cb_regist[i].evtid) {
	    found = 1;
	    break;
	}

    if (found &&
	notify_func == Cb_regist[i].cb_func &&
	Cr_arg == Cb_regist[i].arg) /* duplicated registration */
	    return (EN_SUCCESS);

    /* the following adds a new entry */
    cb = (Cb_regist_t *)MISC_table_new_entry (Cb_tblid, &new_ind);
    if (cb == NULL)
	return (EN_MALLOC_FAILED);

    cb->code = -1;		/* disable this entry first */
    err = EN_SUCCESS;
    if (!found) {
	if (code == EN_AN_CODE)	/* AN regist */
	    err = Register_local_server (code, evtid, 0, 0, notify_func);
	else {			/* UN regist */
	    int a_pid, a_code, ret;

	    /* set/reset the nr record */
	    ret = Set_un_req (code, evtid, Local_hid, Pid, &a_pid, &a_code);
	    if (ret < 0)
		err = ret;
	    else {
		/* register in local server */
		ret = Register_local_server 
				(code, evtid, a_pid, a_code, notify_func);
		if (ret < 0) {	/* failed - we rm nr record */
		    Set_un_req (code, evtid, Local_hid, -1, NULL, NULL);
		    err = ret;
		}
	    }
	}
    }
    if (err < 0) {
	MISC_table_free_entry (Cb_tblid, new_ind);
    }
    else {
	/* complete (activate) local registration */
	cb->evtid = evtid;
	cb->cb_func = notify_func;
	cb->arg = Cr_arg;
	cb->redeliver = 0;
	cb->code = code;		/* activate here */
	err = found;
    }

    return (err);
}

/********************************************************************
			
    Description: This function deregisters a notification callback 
		function.

    Input:	index - entry index to be deregistered;

********************************************************************/

static void Lb_deregister (int index)
{
    Cb_regist_t *cb_reg;
    int code, k;
    EN_id_t evtid;

    cb_reg = Cb_regist + index;
    code = cb_reg->code;
    evtid = cb_reg->evtid;
    cb_reg->code = -1;		/* disable the entry */

    for (k = 0; k < N_cb_regists; k++) {  /* search for other code/evtid */
	if (Cb_regist[k].code == code && Cb_regist[k].evtid == evtid)
	    break;
    }
    if (k >= N_cb_regists) {	/* not found - the last code/evtid */
	if (code != EN_AN_CODE && Set_un_req != NULL)
	    Set_un_req (code, evtid, Local_hid, -1, NULL, NULL);
	Register_local_server (code, evtid, 0, 0, NULL);
					/* deregister local server */
    }
    MISC_table_free_entry (Cb_tblid, index);

    return;
}

/********************************************************************
			
    Description: This function disconnects the socket to the local
		server.

********************************************************************/

static void Disconnect_server ()
{
    int i;

    MISC_close (Sv_fd);
    Sv_fd = -1;		/* This also disables the local sv deregist. */
    Callback_registered = 0;	/* causes new callback reg. */
    N_in_buf = 0;	/* discard left over bytes in input buffer */
    Store_NTF_data (SND_DELETE, NULL, 0);	/* delete save NTF data */

    RMT_send_msg (Local_ip, NULL, -1, NULL);	/* disconn */

    for (i = N_cb_regists - 1; i >= 0; i--) 
	Lb_deregister (i);

    return;
}

/********************************************************************
			
    Description: This function manages storage of the NTF messages
		while waiting for NTF registration ACK messages. It
		is also used for saving the user returned message.

    Input:	func - SND_READ, SND_SAVE, SND_DELETE or SND_BACK.
		data - data to store or buf for reading data.
		len - data length for save and read.

    Return:	EN_SUCCESS on success or EN_FAILURE on failure.

********************************************************************/

#define SND_EXTRA_SIZE 	1024	/* extra size allocated for reducing 
				   frequent memory reallocation */

static int Store_NTF_data (int func, char *data, int len)
{
    static char *buffer = NULL;
    static int buf_size = 0, is_ind_malloc = 0;
    static int data_offset = 0;
    int ret;

    ret = EN_SUCCESS;
    switch (func) {

	case SND_INIT:
	    if (buffer == NULL) {
		while ((buffer = (char *)malloc (Ntf_buf_size)) == NULL)
		    msleep (MALLOC_WAIT_TIME);
	    }
	    return (EN_SUCCESS);

	case SND_READ:
	    if (len <= Snd_stored_bytes) {
		memcpy (data, buffer + data_offset, len);
		data_offset += len;
		Snd_stored_bytes -= len;
		if (Snd_stored_bytes == 0)
		    data_offset = 0;
	    }
	    else {				/* unexpected */
		if (Notify_error != NULL)	/* call error reporting */
		    Notify_error ("unexpected use of Store_NTF_data\n");
		data_offset = Snd_stored_bytes = 0;	/* discard data */
		ret = EN_FAILURE;
	    }
	    break;

	case SND_DELETE:
	    data_offset = Snd_stored_bytes = 0;
	    break;

	case SND_BACK:
	    if (data_offset > 0) {
		if (len <= data_offset) {
		    memcpy (buffer + data_offset - len, data, len);
		    data_offset -= len;
		    Snd_stored_bytes += len;
		}
		else {
		    if (Notify_error != NULL)	/* error reporting */
			Notify_error ("unexpected SND_BACK\n");
		    ret = EN_FAILURE;
		}
		break;
	    }
	    /* otherwise continue to SND_SAVE */

	case SND_SAVE:
	    if (data_offset + Snd_stored_bytes + len > buf_size) {
						/* realloc the buffer */
		char *b;
		int prev_b_size = buf_size;

		buf_size = Snd_stored_bytes + len + 
				SND_EXTRA_SIZE + Ntf_buf_size;
		while ((b = (char *)MISC_ind_malloc (buf_size)) == NULL)
		    msleep (MALLOC_WAIT_TIME);
		if (buffer != NULL) {
		    memcpy (b, buffer + data_offset, Snd_stored_bytes);
		    if (is_ind_malloc)
			MISC_ind_free (buffer, prev_b_size);
		    else
			free (buffer);
		}
		buffer = b;
		data_offset = 0;
		is_ind_malloc = 1;
	    }
	    memcpy (buffer + data_offset + Snd_stored_bytes, data, len);
	    Snd_stored_bytes += len;
	    break;

	default:
	    break;
    }

    return (ret);
}

/********************************************************************
			
    Description: This function posts an AN event of id "event". The 
		AN message is of length "msg_len" bytes and stored 
		in "msg".

    Input:	event_id - AN event id;
		msg - The event message;
		msg_len - length of the event message;

    Returns:	This function returns EN_SUCCESS on success or a 
		negative EN error number.

********************************************************************/

int EN_post_msgevent (EN_id_t event, const char *msg, int msg_len)
{
    static char *buffer = NULL;
    Ntf_msg_t *pmsg;
    const char *msg_to_sent;
    int size, ret;

#ifdef EN_THREADED
    Mt_get_mutex_lock (&Lb_ntf_register_mutex);
#endif

    if (buffer == NULL) {
	if ((buffer = (char *)malloc (Ntf_buf_size)) == NULL)
	    return (Post_return (EN_MALLOC_FAILED));
	memset (buffer, 0, sizeof (Ntf_msg_t));
    }
    if (event > EN_MAX_ID && 
		event != EN_REP_REG_TABLE && event != EN_DISC_HOST &&
		event != EN_QUERY_HOSTS)
	return (Post_return (EN_BAD_ARGUMENT));

    msg_to_sent = NULL;
    if (Process_message != NULL && msg_len > 0 && msg != NULL)
	msg_to_sent = Process_message (EN_MSG_OUT, event, (char *)msg, msg_len);
    if (msg_to_sent == NULL)
	msg_to_sent = msg;

    event = Get_group_event (event);

    size = sizeof (Ntf_msg_t) + msg_len;
    if (size > Ntf_buf_size)
	return (Post_return (EN_MSG_TOO_LARGE));

    pmsg = (Ntf_msg_t *)buffer;
    pmsg->msg_type = FROM_CLIENT_AN;
    pmsg->code = EN_AN_CODE;
    pmsg->evtid = event;
    if (event == EN_QUERY_HOSTS)
	pmsg->msg_len = 0;
    else {
	pmsg->msg_len = msg_len | An_self_flags;
	if (msg_len & An_self_flags)
	    pmsg->sender_id = EN_SHORT_BSWAP (getpid ());
	else
	    pmsg->sender_id = EN_SHORT_BSWAP (Sender_id);
    }
    memcpy (buffer + sizeof (Ntf_msg_t), msg_to_sent, msg_len);
    An_self_flags = 0;

    ret = EN_send_to_server (Local_ip, buffer, size);
    if (ret < 0) 
	return (Post_return (ret));
    return (Post_return (EN_SUCCESS));
}

/********************************************************************
			
    Unlocks register_mutex and returns "ret".

********************************************************************/

static int Post_return (int ret) {
#ifdef EN_THREADED
    Mt_get_mutex_unlock (&Lb_ntf_register_mutex);
#endif
    return (ret);
}

/****************************************************************
						
    Description: Returns the modified event number based on the
		current group number. It reads in the default 
		group number initially.

    Input:	event - application event id;

    Returns:	The modified event number.

****************************************************************/

static EN_id_t Get_group_event (EN_id_t event)
{

    if (An_group == -2) {
	char *env;
	An_group = -1;
	if ((env = getenv ("RMTPORT")) != NULL) {
	    while (*env != '\0') {
		if (*env == '@') {
		    int group;
		    if (sscanf (env + 1, "%d", &group) == 1 &&
			group >= 0 &&  group <= MAX_AN_GROUP_NUM) {
			An_group = group;
			break;
		    }
		}
		env++;
	    }
	}
    }
    if (An_group >= 0 && event <= AN_GROUP_MASK)
	return (event | (An_group << AN_GROUP_OFFSET));
    else
	return (event);
}

/********************************************************************
			
    Polls the local server socket for "ms" milliseconds.

    Returns 1 if data ready, 0 not ready and -1 on poll error.

********************************************************************/

static int Poll_local_server (int ms) {
    struct pollfd fds[1];
    fds[0].fd = Sv_fd;
    fds[0].events = POLL_IN_FLAGS;
    return (poll (fds, 1, ms));
}
 
/********************************************************************
			
    Description: This function sleeps until a signal is 
		available. Signal "signo" is currently blocked and
		needs to be detected.

    Input:	signo - the signal.
		ms - timer value in milli-seconds;

    Return:	0 on success or -1 on failure.

********************************************************************/

static int Sig_wait (int signo, int ms) {
    sigset_t signalSet;
    struct timespec timeout;

    sigemptyset (&signalSet);
    sigaddset(&signalSet, signo);
    timeout.tv_sec = ms / 1000;
    timeout.tv_nsec = (ms % 1000) * 1000000;
#ifdef __WIN32__
    /* FIXME: no equivalent of sigtimedwait on Interix? */
    return -1;
#else /* Unix */
    if (sigtimedwait (&signalSet, NULL, &timeout) >= 0)
	return (0);
    else
	return (-1);
#endif
}

/********************************************************************
			
    Calls either sigprocmask or pthread_sigmask.

********************************************************************/

static int Sig_mask (int how, const sigset_t *set, sigset_t *oset) {

#ifdef EN_THREADED
    return (pthread_sigmask (how, set, oset));
#else
    return (sigprocmask (how, set, oset));
#endif
}

/********************************************************************
			
    Blocks the NTF signal if "yes" is non-zero or unblocks the signal
    if "yes" is zero.

********************************************************************/

static int Sig_hold (int yes) {
    sigset_t set;

    if (Ntf_signal < 0)
	return (0);
    sigemptyset (&set);
    sigaddset (&set, Ntf_signal);
    if (yes)
	return (Sig_mask (SIG_BLOCK, &set, NULL));
    else
	return (Sig_mask (SIG_UNBLOCK, &set, NULL));
}

#ifdef EN_THREADED

/********************************************************************

    Description: Funtion to lock a mutex. Checks for recursive
		calls to pthread_mutex_lock

     Input:     pointer to the mutex

********************************************************************/

static void Mt_get_mutex_lock (pthread_mutex_t *mutex) {
    int blocked;

    if (Lock_count > 0 && Mutex_owner == pthread_self ()) {
	Lock_count++;
	return;
    }
    blocked = Is_sig_blocked ();
    if (!blocked)	
	Sig_hold (1);
    if (pthread_mutex_lock (mutex) == 0) {
     	Mutex_owner = pthread_self ();
	Lock_count++;
    }
    if (!blocked)
	Sig_hold (0);
}

/********************************************************************

    Description: Funtion to unlock the mutex. Checks for recursive
		calls to pthread_mutex_lock and releases the lock only
		when all the recursive calls for unlock are made.

     Input:     pointer to the mutex

********************************************************************/

static void Mt_get_mutex_unlock (pthread_mutex_t *mutex) {

    if (Lock_count > 0 && Mutex_owner == pthread_self ()) {
	if (Lock_count == 1) {
	    int blocked = Is_sig_blocked ();
	    if (!blocked)	
		Sig_hold (1);
	    Lock_count = 0;
	    pthread_mutex_unlock (mutex);
	    if (!blocked)
		Sig_hold (0);
	}
	else
	    Lock_count--;
    }
}

/*******************************************************************

     Returns non-zero if the Ntf_signal is blocked for this thread
     or zero othersise. Returns non-zero if EN signal is not used.

********************************************************************/

static int Is_sig_blocked () {
    int masked = 1;
    if (Ntf_signal >= 0) {
	sigset_t mask;
	Sig_mask (0, NULL, &mask);
	masked = sigismember (&mask, Ntf_signal);
    }
    return (masked);
}
#endif			/* EN_THREADED */

/*******************************************************************

     Additional per-threaded data support.

********************************************************************/

#ifdef EN_THREADED
#include <pthread.h>
static pthread_key_t Ptd_key;
static pthread_once_t Key_init_once = {PTHREAD_ONCE_INIT};

static void Key_init_func ();
static void Free_ptd (void *arg);
#endif

/********************************************************************

    Description: This function returns the pointer to the per thread 
		data structure. This function will retry until success
		if memory is not available.

    Return:	The pointer.

*********************************************************************/

EN_per_thread_data_t *EN_get_per_thread_data (void)
{

#ifndef EN_THREADED
    static EN_per_thread_data_t ptd = 
	{0, 0, 0, 0};
    return (&ptd);

#else
    EN_per_thread_data_t *ptd;
    int blocked;
    blocked = Is_sig_blocked ();
    if (!blocked)
	Sig_hold (1);
    pthread_once (&Key_init_once, Key_init_func);
    ptd = (EN_per_thread_data_t *)pthread_getspecific (Ptd_key);
    if (ptd == NULL) {
	while ((ptd = (EN_per_thread_data_t *)malloc 
				(sizeof (EN_per_thread_data_t))) == NULL)
	    msleep (200);
	ptd->ntf_block_intern = 0;
	ptd->in_callback = 0;
	ptd->Deregister = 0;
	ptd->ntf_block = 0;
	pthread_setspecific (Ptd_key, (void *)ptd);
    }
    if (!blocked)
	Sig_hold (0);
    return (ptd);
#endif
}

#ifdef EN_THREADED

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
    EN_per_thread_data_t *ptd = (EN_per_thread_data_t *)arg;
    free (ptd);
}

#endif

/********************************************************************
			
    Backward compatibility functions.

********************************************************************/

int LB_NTF_control (int cntl_function, ...) {
    va_list args;

    va_start (args, cntl_function);
    return (En_control_internal (cntl_function, args));
}

int LB_AN_register (EN_id_t event, 
		void (*notify_func)(EN_id_t, char *, int, void *)) {
    return (EN_register (event, notify_func));
}

int LB_AN_post (EN_id_t event, const char *msg, int msg_len) {
    return (EN_post_msgevent (event, msg, msg_len));
}

int LB_NTF_lost () {
    return (EN_event_lost ());
}

int LB_NTF_sender_id () {
    return (EN_sender_id ());
}



