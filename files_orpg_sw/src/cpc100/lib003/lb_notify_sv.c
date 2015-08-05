/****************************************************************
		
    Module: lb_notify_sv.c	
				
    Description: This module implements the server part of the
		LB NTF functions. This is the code in the 
		notification server.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2001/06/13 18:52:45 $
 * $Id: lb_notify_sv.c,v 1.46 2001/06/13 18:52:45 jing Exp $
 * $Revision: 1.46 $
 * $State: Exp $
 * $Log: lb_notify_sv.c,v $
 * Revision 1.46  2001/06/13 18:52:45  jing
 * Update
 *
 * Revision 1.44  2000/09/20 21:56:07  jing
 * @
 *
 * Revision 1.42  2000/08/21 20:49:50  jing
 * @
 *
 * Revision 1.39  2000/04/18 19:00:19  jing
 * @
 *
 * Revision 1.38  2000/03/24 22:28:17  jing
 * @
 *
 * Revision 1.33  2000/02/28 00:23:50  jing
 * @
 *
 * Revision 1.21  1999/06/29 21:19:16  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.19  1999/05/27 02:27:03  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.14  1999/05/03 20:57:44  jing
 * NO COMMENT SUPPLIED
 *
 *
*/

#ifdef LB_NTF_SERVICE		/* This file is needed only if we need NTF */
				/* we comment this out to eliminate references 
				   to NET_ functions */

/* System include files */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h> 
#include <fcntl.h>
#include <errno.h>

/* Local include files */

#include <net.h>
#include <misc.h>
#include <lb.h>

#define LB_NTF_EXTERN_AS_STATIC
#include "lb_extern.h"
#include "lb_def.h"

#define WAITING_FOR_MEMORY_TIME 200	/* in case of memory unavailable, the
					   server will retry every this time
					   period (in ms) */
#define RECONN_PERIOD		15	/* reconnection perios (seconds) */
#define CONN_WAIT_TIME		5	/* max waiting time for conn process */

typedef unsigned short Index_t;		/* type for client reg table index */

#define INDEX_INVALID	0xffff		/* special value for invalid index */
#define INDEX_UNKNOWN	0xfffe		/* special values for index */
#define INDEX_DISABLED	0xfffd		/* special values for index */
#define INDEX_MAX	0xfffc		/* maximum valid index */

typedef struct {			/* AN specific client reg data */
    int reg_mask;
    int n_rhosts;			/* number of remote hosts registered */
} An_spec;

typedef struct {			/* UN specific client reg data */
    int a_pid;				/* aliased pid */
    int msg_len;			/* msg length for UN */
    int a_fd;				/* aliased fd (< 0 means no alias) */
} Un_spec;

typedef struct {			/* clients notification registration */
    Index_t prev;			/* index of the previous entry */
    Index_t next;			/* index of the next entry */
    short sock;				/* socket to the client. This is also 
					   used as an ID for each client. */
    short signal;			/* signal for the client; < 0 indicates 
					   signaling is not needed. */
    int pid;				/* local client pid. -1 for remote 
					   clients. */
    int fd;				/* LB fd or LB_AN_FD */
    LB_id_t msgid;			/* LB message ID or the AN event ID */
    int lost_cnt;			/* count of lost events */
    union {				/* AN and UN specific data */
	An_spec an;			/* AN specific data */
	Un_spec un;			/* UN specific data */
    } spec;
} Client_reg_t;

static Client_reg_t *Client_reg = NULL;	/* registers clients notifications */
static void *Cl_tblid = NULL;		/* table id for Client_reg */
static int N_client_regs = 0;		/* # of Client_reg entries */

static Index_t *Indices = NULL;		/* NTF client index table */
static void *Ind_tblid = NULL;		/* table id for Indices */
static int N_indices = 0;		/* size of Indices */

static int Ntfrr_ind = INDEX_DISABLED;	/* NTF report regist index */
static int An_lb_any_ind = INDEX_DISABLED;
					/* LB_ANY AN regist index */

/* external functions */
static int (*LB_EXT_sv_cntl) (int func, int arg);
					/* function for various server 
					   controls */
static void (*LB_EXT_sv_error) (char *) = NULL;
					/* function reporting error messages 
					   to the server log. */
static int (*LB_EXT_get_client_IP) (int, unsigned int *) = NULL;
					/* function for getting IP addresses 
					   of configured remote hosts. */
static char *(*LB_EXT_pack_msg) (char *, int, int *) = NULL;
					/* function for packing msg to be sent
					   to a message server */

static int Lost_ntf = 0;		/* flag indicating that there are lost 
					   notifications */

typedef struct {			/* struct for saved client data */
    int sock;				/* client socket */
    char *data;				/* the queued up messages */
    int buf_size;			/* current data buffer size */
    int n_written;			/* number of bytes written */
    int n_total;			/* total number of bytes in data */
    int signal;				/* the same as Client_reg_t.signal */
    int pid;				/* the same as Client_reg_t.pid */
} Scd_t;


#define MAX_AN_RHOSTS 32		/* maximum number of configured AN
					   hosts. This is limited by the size
					   of Client_reg_t.spec.an.reg_mask */

typedef struct {			/* strust tracking active remote hosts
					   for AN */
    unsigned int ip;			/* IP address of remote server */
    int sock;				/* the socket of an active conn;
					   negative indicates inactive */
    int init_sock;			/* the sock opened for init msg */
    int conn_seq;			/* connection sequence number */
    short failed;			/* error found with the sock */
    short cmd_disc;			/* commanded disconnected */
} Remote_hosts;

static int N_rhosts = 0;		/* array size of Rhosts */
static Remote_hosts *Rhosts = NULL;	/* the remote host table */
static time_t Stop_local_read_time = 0;	/* time of stoping local socket read;
					   0 indicates not stopped */
static unsigned int Local_ip;		/* local IP address */
static int Rhost_failed = 0;		/* connection to a remote server is 
					   detected to be bad */

enum {SCD_GET, SCD_IND_GET, SCD_SAVE, SCD_FREE};
					/* values for "func" argument of 
					   Manage_scd () */
enum {PRIO_LOW, PRIO_HIGH};		/* values for "prio" argument of 
					   Send_msg_to_client () */
enum {SEND_DONE, SEND_QUEUED, SEND_REJECTED, SEND_FAILED};
					/* ret values of Send_msg_to_client */

/* values for argument "func" of Manage_saved_AN */
enum {MSAN_GET, MSAN_SAVE, MSAN_DELETE};


static void Deregister_client (int sock, int need_close);
static Scd_t *Manage_scd (int func, int sock);
static int Send_ack_msg (Client_reg_t *reg, int status);
static void Process_NTF_regist (Ntf_regist_msg_t *msg, int sock);
static void Process_UN_msg (From_lb_write_t *ntf);
static void Resend_lost_ntfs ();
static int Send_msg_to_client (int sock, char *msg, int msg_len, 
				int prio, int pid, int signal);
static void Append_to_scd_buffer (Scd_t *scd, char *msg, int msg_len);
static void Send_sv_ack_msg (int sock, Ntf_regist_msg_t *reg_msg, int status);
static void Send_AN_regist_to_svs (Client_reg_t *new_reg);
static void Process_sv_ack (LB_id_t msgid, int host_ind);
static void Bswap_AN_msg_header (Ntf_msg_t *msg);
static void Process_AN_msg (Ntf_msg_t *msg, int input_len);
static void Send_AN_msg (Ntf_msg_t *msg, Client_reg_t *cl_reg, int msg_len);
static Ntf_msg_t *Manage_saved_AN (int func, LB_id_t event, Ntf_msg_t *msg);
static void Stop_local_read ();
static void Resume_local_read ();
static void Init_rhosts ();
static void Process_conn (Ntf_msg_t *msg, int sock, 
				unsigned int host, int msg_len);
static int Send_conn_init_to_rhost (Remote_hosts *rh, int block, char *reason);
static int Is_AN_registered (Client_reg_t *new_reg);
static void Send_dereg_to_svs (LB_id_t msgid);
static void Ntf_report (const char *format, ...);
static void Report_reg_table ();
static int Search_index_table (LB_id_t msgid);
static void Delete_client_entry (int cl_ind);
static Client_reg_t *Get_new_client_entry (LB_id_t msgid);
static int Get_client_table_ind (LB_id_t msgid);
static void Receive_AN_reg_info (Ntf_msg_t *msg, int sock, int msg_len);
static void Send_AN_reg_info (int sock);
static void Check_rsv_connection (time_t tm);
static void Accept_connection (Remote_hosts *rh, 
				int sock, int seq, char *report);
static void Cmd_disconnect_host (Ntf_msg_t *msg, int input_len);
static int Write_cmd_disc_file (int cmd_disc, unsigned int ip);


/*************************************************************
			
    Description: This function implements the local server 
		of the LB notification services. This function 
		must be registered as a messaging callback 
		function of the messaging server.

    Input:	msg - the incoming message;
		msg_len - message length; 0 indicates no message
			(write available).
		sock - socket fd;
		host - IP address of the sending host.

**************************************************************/

void LB_ntf_server (char *msg, int msg_len, 
				int sock, unsigned int host)
{
    static int cnt = 0;
    From_lb_write_t *lbwntf;
    Ntf_msg_t *ntf;
    int plen, ret;

    if (LB_EXT_sv_cntl == NULL)
	return;

    if (cnt == 0)		/* first call; happens in foreground. */
	Init_rhosts ();		/* initialize the remote host table */
    cnt++;

    if (Lost_ntf)
	Resend_lost_ntfs ();

    if (msg_len == LB_NTF_CONN_LOST) {	/* connection lost */
	Deregister_client (sock, 0);
	return;
    }

    if (Rhost_failed) {
	int i, closed;
	closed = 0;
	for (i = 0; i < N_rhosts; i++) {
	    if (Rhosts[i].failed) {
		if (sock == Rhosts[i].sock)
		    closed = 1;
		Deregister_client (Rhosts[i].sock, 1);
	    }
	}
	Rhost_failed = 0;
	if (closed)
	    return;
    }

    if ((cnt % 5) == 0 || msg_len == LB_NTF_TIMER) { /* check time */
	static time_t prev_tm = 0;
	time_t tm = time (NULL);
	if (tm >= prev_tm + 2) {	/* every two second we do */
	    if (Stop_local_read_time)
		Resume_local_read ();
	    Manage_saved_AN (MSAN_DELETE, 0, NULL);
	    Check_rsv_connection (tm);
	    prev_tm = tm;
	}
	if (msg_len == LB_NTF_TIMER)		/* no event - timer call */
	    return;
    }

    if (msg_len == LB_NTF_WRITE_READY) {
				/* write available - send saved client data */
	ret = Send_msg_to_client (sock, NULL, 0, 0, 0, -1);
	if (ret == SEND_FAILED) {	/* system call failure */
	    MISC_log ("send saved client data failed");
	    Deregister_client (sock, 1);
	    return;
	}
	return;
    }

    if (msg_len < (int)sizeof (Ntf_msg_t)) {
	MISC_log ("unexpected NTF msg length");
	return;
    }
    ntf = (Ntf_msg_t *)msg;

    switch (ntf->msg_type) {

	case FROM_CLIENT_AN:
	case FROM_SV_AN:
	    Process_AN_msg ((Ntf_msg_t *)msg, msg_len);
	    return;

	case FROM_LB_WRITE_UN:
	case FROM_LB_WRITE_WUR:
	    break;

	case FROM_CLIENT_REG:
	case FROM_SV_REG:
	    if (msg_len != sizeof (Ntf_regist_msg_t)) {
		MISC_log ("unexpected NTF reg msg length");
	    }
	    else
		Process_NTF_regist ((Ntf_regist_msg_t *)msg, sock);
	    return;

	case TO_SV_ACK:
	    Process_sv_ack (LB_T_BSWAP (ntf->msgid),
					LB_T_BSWAP (ntf->lost_cnt));
	    return;

	case FROM_SV_CONN:
	    Process_conn ((Ntf_msg_t *)msg, sock, host, msg_len);
	    return;

	default:
	    MISC_log ("unexpected NTF msg received");
	    return;
    }

    /* process each UN and WU in the message */
    lbwntf = (From_lb_write_t *)msg;
    plen = sizeof (From_lb_write_t);	/* length processed or in processing */
    while (plen <= msg_len) {

	if (lbwntf->msg_type == FROM_LB_WRITE_UN) {
	    lbwntf->pid = LB_T_BSWAP (lbwntf->pid);
	    lbwntf->fd = LB_T_BSWAP (lbwntf->fd);
	    lbwntf->msgid = LB_T_BSWAP (lbwntf->msgid);
	    lbwntf->lbmsgid = LB_T_BSWAP (lbwntf->lbmsgid);
	    lbwntf->msg_len = LB_T_BSWAP (lbwntf->msg_len);
	    Process_UN_msg (lbwntf);
	}
	else if (lbwntf->msg_type == FROM_LB_WRITE_WUR)
	    kill (LB_T_BSWAP (lbwntf->pid), LB_SIGNAL);

	plen += sizeof (From_lb_write_t);
	lbwntf++;
    }

    return;
}

/**************************************************************
			
    Description: This function sets external references for
		this module. This must be called in the 
		messaging server initialization.

		sv_cntl - function controlling the message server 
		client_IP - function returning config client IPs.
		sv_error - function for reporting server error.
		pack_msg - function packing msg to be sent to a
			messaging server.

    Input:	poll_write - the function pointer.

***************************************************************/

void LB_ntf_sv_set_externs (int (*sv_cntl)(), int (*client_IP)(), 
			void (*sv_error)(), char *(*pack_msg) ())
{

    LB_EXT_sv_cntl = sv_cntl;
    LB_EXT_get_client_IP = client_IP;
    LB_EXT_sv_error = sv_error;
    LB_EXT_pack_msg = pack_msg;
    return;
}

/*************************************************************
			
    Description: This function processes an UN message from 
		LB_write. If sending message to a client fails, 
		we don't process the exception in order to 
		continue service to other clients.

    Input:	ntf - the received UN message;

**************************************************************/

static void Process_UN_msg (From_lb_write_t *ntf)
{
    int fd, pid, msg_len;
    LB_id_t msgid;
    int next_ind;
    Client_reg_t *cl_reg;

    fd = ntf->fd;
    pid = ntf->pid;
    msg_len = ntf->msg_len;
    msgid = ntf->msgid;

    if (Ntfrr_ind != INDEX_DISABLED)
	Ntf_report ("UN recvd: pid %d, fd %d, msgid %x\n", pid, fd, msgid);

    next_ind = Get_client_table_ind (msgid);
    while (next_ind != INDEX_INVALID) {
	int matched;

	cl_reg = Client_reg + next_ind;
	next_ind = cl_reg->next;
	matched = 0;
	if (cl_reg->spec.un.a_fd < 0) {		/* no alias */
	    if (cl_reg->pid == pid &&
		cl_reg->fd == fd)
		matched = 1;
	}
	else {
	    if (cl_reg->spec.un.a_pid == pid &&
		cl_reg->spec.un.a_fd == fd)
		matched = 1;
	}
	if (matched) {			/* send a msg to the client */
	    Ntf_msg_t msg;
	    int ret, sock;

	    msg.msg_type = TO_CLIENT_UN;
	    msg.fd = cl_reg->fd;
	    msg.msgid = msgid;
	    msg.lbmsgid = ntf->lbmsgid;
	    msg.msg_len = msg_len;
	    msg.lost_cnt = cl_reg->lost_cnt;
	    msg.sender_id = ntf->sender_id;
	    sock = cl_reg->sock;

	    if (cl_reg->lost_cnt <= 0)
		ret = Send_msg_to_client (sock, (char *)&msg, 
				sizeof (Ntf_msg_t), PRIO_LOW, 
				cl_reg->pid, cl_reg->signal);
	    else
		ret = SEND_FAILED;

	    if (Ntfrr_ind != INDEX_DISABLED)
		Ntf_report ("send UN: sock %d, fd %d, msgid %x, ret %d\n", 
				sock, msg.fd, msgid, ret);

	    if (ret == SEND_DONE || ret == SEND_QUEUED) /* send success */
		cl_reg->lost_cnt = 0;
	    else {				/* notification lost */
		cl_reg->lost_cnt++;
		cl_reg->spec.un.msg_len = msg_len;
		Lost_ntf = 1;
	    }
	}
    }

    return;
}

/*************************************************************
			
    Description: This function resends lost events. We don't 
		process disconnected client here.

**************************************************************/

static void Resend_lost_ntfs ()
{
    int i;

    Lost_ntf = 0;
    for (i = 0; i < N_client_regs; i++) {
	Ntf_msg_t tmp, *msg;
	Client_reg_t *cl_reg;
	int size, ret;

	cl_reg = Client_reg + i;
	if (cl_reg->lost_cnt == 0)
	    continue;

	if (cl_reg->fd != LB_AN_FD) {		/* UN */
	    msg = &tmp;
	    msg->msg_type = TO_CLIENT_UN;
	    msg->msg_len = cl_reg->spec.un.msg_len;
	    msg->lbmsgid = cl_reg->msgid;
	    size = sizeof (Ntf_msg_t);
	}
	else {			/* AN; The following modifies the saved AN  
				   message but it should be fine */
	    msg = Manage_saved_AN (MSAN_GET, cl_reg->msgid, NULL);
	    if (msg == NULL)			/* unexpected */
		continue;
	    msg->msg_type = TO_CLIENT_AN;
	    size = sizeof (Ntf_msg_t) + msg->msg_len;
	}
	msg->fd = cl_reg->fd;
	msg->msgid = cl_reg->msgid;
	msg->lost_cnt = cl_reg->lost_cnt - 1;
	msg->sender_id = 0;

	ret = Send_msg_to_client (cl_reg->sock, (char *)msg, size,
			PRIO_LOW, cl_reg->pid, cl_reg->signal);
	if (Ntfrr_ind != INDEX_DISABLED)
	    Ntf_report ("resend NTF: ret %d\n", ret);
	if (ret == SEND_DONE || ret == SEND_QUEUED)
	    cl_reg->lost_cnt = 0;
	else 
	    Lost_ntf = 1;	/* further lost notifications needed */
    }
    return;
}

/********************************************************************
			
    Description: This function processes a NTF registration message.

    Input:	msg - the registration message;
		sock - client socket fd;

********************************************************************/

static void Process_NTF_regist (Ntf_regist_msg_t *msg, int sock)
{
    int pid, fd, signal;
    LB_id_t msgid;
    Client_reg_t *cl_reg;
    int next_ind;

    if (msg->msg_type == FROM_SV_REG) {
	pid = LB_T_BSWAP (msg->pid);
	fd = LB_T_BSWAP (msg->fd);
	msgid = LB_T_BSWAP (msg->msgid);
	signal = LB_T_BSWAP (msg->signal);
    }
    else {
	pid = msg->pid;
	fd = msg->fd;
	msgid = msg->msgid;
	signal = msg->signal;
    }

    if (Ntfrr_ind != INDEX_DISABLED) {
	if (fd == LB_AN_FD)
	    Ntf_report ("AN reg recvd: pid %d, ID %x, signal %d\n", 
						pid, msgid, signal);
	else
	    Ntf_report ("UN reg recvd: pid %d, fd %d, msgid %x, signal %d\n", 
						pid, fd, msgid, signal);
    }

    /* deregistration and check duplicated entry */
    next_ind = Get_client_table_ind (msgid);
    while (next_ind != INDEX_INVALID) {
	cl_reg = Client_reg + next_ind;
	if (sock == cl_reg->sock && fd == cl_reg->fd) {
	    if (signal == -1) {
		if (fd == LB_AN_FD && cl_reg->pid >= 0) /* local AN */
		    Send_dereg_to_svs (msgid);	/* dereg AN on remote sv's */
		Delete_client_entry (next_ind);	/* rm the entry */
		return;
	    }
	    else {				/* already registered */
		if (msg->msg_type == FROM_CLIENT_REG)
		    Send_ack_msg (cl_reg, TO_CLIENT_ACK_DONE);
		else
		    Send_sv_ack_msg (sock, msg, TO_CLIENT_ACK_DONE);
		return;
	    }
	}
	next_ind = cl_reg->next;
    }
    if (signal == -1)		/* deregister */
	return;

    /* the following adds a new entry */
    if (N_client_regs < INDEX_MAX)
	cl_reg = Get_new_client_entry (msgid);
    else {		/* too many clients */
	MISC_log ("NTF client reg table overflow");
	Send_ack_msg (cl_reg, TO_CLIENT_ACK_FAILED);
	return;
    }
    cl_reg->sock = sock;
    cl_reg->signal = signal;
    cl_reg->pid = pid;
    cl_reg->fd = fd;
    cl_reg->msgid = msgid;
    cl_reg->lost_cnt = 0;
    cl_reg->spec.un.msg_len = 0;

    An_lb_any_ind = INDEX_UNKNOWN;
    if (Ntfrr_ind != INDEX_DISABLED)
	Ntfrr_ind = INDEX_UNKNOWN;
    if (fd == LB_AN_FD) {			/* AN reg */
	if (msgid == LB_NTF_REPORT) {		/* reg for NTF status report */
	    if (msg->msg_type == FROM_CLIENT_REG) {
		cl_reg->spec.an.reg_mask = 0;
		Ntfrr_ind = INDEX_UNKNOWN;
		Send_ack_msg (cl_reg, TO_CLIENT_ACK_DONE); 
	    }
	}
	else if (msg->msg_type == FROM_CLIENT_REG) {	/* local AN reg */
	    cl_reg->spec.an.reg_mask = 0xffffffff;	/* disable this reg */
	    /* see if this is already registered by another local client */
	    if (!Is_AN_registered (cl_reg)) {
		cl_reg->signal = -2;		/* remote registration */
		Send_AN_regist_to_svs (cl_reg);	/* send reg to remote sv's */
		cl_reg->signal = signal;	/* recover signal */
	    }
	    if (cl_reg->spec.an.reg_mask == 0)		/* done */
		Send_ack_msg (cl_reg, TO_CLIENT_ACK_DONE);
	}
	else {					/* remote AN reg (from sv) */
	    cl_reg->pid = -1;			/* mark as remote AN reg */
	    cl_reg->signal = -1;		/* no signal necessary */
	    cl_reg->spec.an.reg_mask = 0;
	    Send_sv_ack_msg (sock, msg, TO_CLIENT_ACK_DONE);
	}
    }
    else {					/* UN reg */
	cl_reg->spec.un.a_pid = msg->a_pid;
	cl_reg->spec.un.a_fd = msg->a_fd;
	Send_ack_msg (cl_reg, TO_CLIENT_ACK_DONE);
    }
    return;
}

/****************************************************************
						
    Description: Gets a new client registration entry for "msgid".

    Input:	msgid - UN message ID or AN ID;

    Return:	The pointer to the new entry.

****************************************************************/

static Client_reg_t *Get_new_client_entry (LB_id_t msgid)
{
    int id_ind, new_cl_ind;
    Client_reg_t *new_reg;

    /* create the client tables */
    while (Cl_tblid == NULL) {
	Cl_tblid = MISC_open_table (sizeof (Client_reg_t), 
				100, 0, &N_client_regs, (char **)&Client_reg);
	if (Cl_tblid == NULL)
	    msleep (WAITING_FOR_MEMORY_TIME);
    }
    while (Ind_tblid == NULL) {
	Ind_tblid = MISC_open_table (sizeof (Index_t), 
				32, 1, &N_indices, (char **)&Indices);
	if (Ind_tblid == NULL)
	    msleep (WAITING_FOR_MEMORY_TIME);
    }

    id_ind = Search_index_table (msgid);	/* search msgid */
    while ((new_reg = (Client_reg_t *)	/* get new entry in client table */
		MISC_table_new_entry (Cl_tblid, &new_cl_ind)) == NULL)
	msleep (WAITING_FOR_MEMORY_TIME);

    if (id_ind < 0) {			/* msgid not found */
	int i;
	while (MISC_table_new_entry (Ind_tblid, &id_ind) == NULL)		
					/* get new entry in index table */
	    msleep (WAITING_FOR_MEMORY_TIME);
	for (i = 0; i < N_indices - 1; i++) {	/* sort the table */
	    if (Client_reg[Indices[i]].msgid > msgid) {
		memmove (Indices + i + 1, Indices + i, 
			(N_indices - i - 1) * sizeof (Index_t));
		break;
	    }
	}
	Indices[i] = new_cl_ind;
	new_reg->prev = INDEX_INVALID;
	new_reg->next = INDEX_INVALID;
    }
    else {				/* search for the last entry */
	Client_reg_t *reg;
	int cr_ind;

	cr_ind = Indices[id_ind];
	reg = Client_reg + cr_ind;
	while (reg->next != INDEX_INVALID) {
	    cr_ind = reg->next;		/* the current client entry index */
	    reg = Client_reg + cr_ind;
	}
	reg->next = new_cl_ind;
	new_reg->prev = cr_ind;
	new_reg->next = INDEX_INVALID;
    }
    new_reg->msgid = msgid;
    return (new_reg);
}

/****************************************************************
						
    Description: Deletes client registration entry "cl_ind".

    Input:	cl_ind - index of the client table entry to be 
			deleted;

****************************************************************/

static void Delete_client_entry (int cl_ind)
{
    int last_ind, id_ind;
    Client_reg_t *reg;

    if (cl_ind < 0 || cl_ind >= N_client_regs)
	return;

    /* remove the entry */
    reg = Client_reg + cl_ind;
    if (reg->prev != INDEX_INVALID)
	Client_reg[reg->prev].next = reg->next;
    else {			/* the first client for this msgid */
	id_ind = Search_index_table (reg->msgid); /* search msgid */
	if (id_ind >= 0)
	    Indices[id_ind] = reg->next;
	if (Indices[id_ind] == INDEX_INVALID)
	    MISC_table_free_entry (Ind_tblid, id_ind);	/* rm ID table entry */
    }
    if (reg->next != INDEX_INVALID)
	Client_reg[reg->next].prev = reg->prev;

    /* move entry last_ind to cl_ind */
    last_ind = N_client_regs - 1;
    reg = Client_reg + last_ind;
    if (last_ind != cl_ind) {
	if (reg->prev != INDEX_INVALID)
	    Client_reg[reg->prev].next = cl_ind;
	else {			/* the first client for this msgid */
	    id_ind = Search_index_table (reg->msgid);	/* search */
	    if (id_ind >= 0)
		Indices[id_ind] = cl_ind;
	}
	if (reg->next != INDEX_INVALID)
	    Client_reg[reg->next].prev = cl_ind;
    }

    MISC_table_free_entry (Cl_tblid, cl_ind);	/* rm client table entry */

    if (Ntfrr_ind != INDEX_DISABLED)
	Ntfrr_ind = INDEX_UNKNOWN;
    if (An_lb_any_ind != INDEX_DISABLED)
	An_lb_any_ind = INDEX_UNKNOWN;
    return;
}

/*************************************************************
			
    Description: This function performs a binary search to find
		the "msgid" entry in the NTF index table.

    Input:	msgid - the message id to search for;

    Output:	The table index found on success of -1 on failure.

**************************************************************/

static int Search_index_table (LB_id_t msgid)
{
    int st, end;

    if (N_indices <= 0)
	return (-1);
    st = 0;
    end = N_indices - 1;

    while (1) {
	int ind;

	ind = (st + end) >> 1;				
	if (st == ind) {
	    if (Client_reg[Indices[st]].msgid == msgid)
		return (st);
	    else if (Client_reg[Indices[end]].msgid == msgid)
		return (end);
	    else
		return (-1);
	}
	if (msgid <= Client_reg[Indices[ind]].msgid)
	    end = ind;
	else
	    st = ind;
    }
    return (-1);
}

/*************************************************************
			
    Description: returns the client table index for msgid.

    Input:	msgid - the message id to search for;

    Output:	The client table index found on success of 
		INDEX_INVALID on failure.

**************************************************************/

static int Get_client_table_ind (LB_id_t msgid)
{
    int i;

    if ((i = Search_index_table (msgid)) >= 0)
	return (Indices[i]);
    else
	return (INDEX_INVALID);
}

/********************************************************************
			
    Description: This function checks if any local client still 
		registered for AN "msgid". If not, a deregistration
		msg is sent to all active remote servers.

    Input:	msgid - AN event id;

********************************************************************/

static void Send_dereg_to_svs (LB_id_t msgid)
{
    int next_ind, cnt;
    Client_reg_t reg;

    if (msgid == LB_NTF_REPORT)
	return;

    cnt = 0;
    next_ind = Get_client_table_ind (msgid);
    while (next_ind != INDEX_INVALID) {
	Client_reg_t *cl_reg;

	cl_reg = Client_reg + next_ind;
	next_ind = cl_reg->next;
	if (cl_reg->fd == LB_AN_FD && cl_reg->pid >= 0)
	   cnt++;
	if (cnt >= 2)		/* this AN is still used by a local client */
	    return;
    }

    reg.fd = LB_AN_FD;
    reg.msgid = msgid;
    reg.signal = -1;		/* dereg from sv */
    Send_AN_regist_to_svs (&reg);
    return;
}

/********************************************************************
			
    Description: This function sends a registeration ack message 
		to the client. This function disconnects the client 
		socket if an error is detected in writing the message. 

    Input:	reg - the client to send.
		status - TO_CLIENT_ACK_DONE or TO_CLIENT_ACK_FAILED;

    Return:	0 on success or -1 on failure.

********************************************************************/

static int Send_ack_msg (Client_reg_t *reg, int status)
{
    Ntf_msg_t msg;
    int ret;

    msg.msg_type = TO_CLIENT_ACK;
    msg.fd = reg->fd;
    msg.msgid = reg->msgid;
    msg.msg_len = status;
    msg.sender_id = reg->spec.an.n_rhosts;

    ret = Send_msg_to_client (reg->sock, (char *)&msg, 
		sizeof (Ntf_msg_t), PRIO_HIGH, reg->pid, reg->signal);
    if (Ntfrr_ind != INDEX_DISABLED) {
	if (reg->fd == LB_AN_FD)
	    Ntf_report ("AN ack sent: status %d, ID %x, ret %d\n", 
						status, reg->msgid, ret);
	else
	    Ntf_report ("UN ack sent: status %d, fd %d, msgid %x, ret %d\n", 
					status, reg->fd, reg->msgid, ret);
    }

    if (ret != SEND_DONE && ret != SEND_QUEUED) {
	MISC_log ("Send_ack_msg failed");
	Deregister_client (reg->sock, 1);
	return (-1);
    }

    return (0);
}

/********************************************************************
			
    Description: This function deregisters all client notifications 
		associated with the client of "sock".

    Input:	sock - client socket fd;
		need_close - non-zero indicates that closing the 
			socket is needed.

********************************************************************/

static void Deregister_client (int sock, int need_close)
{
    int host_ind;
    int i;

    if (sock < 0)
	return;

    if (Ntfrr_ind != INDEX_DISABLED)
	Ntf_report ("sock %d disconnected\n", sock);

    host_ind = -1;			/* remote host index */
    for (i = 0; i < N_rhosts; i++) {
	if (Rhosts[i].init_sock == sock) {
	    MISC_log ("init sock %d closed", sock);
	    Rhosts[i].init_sock = -1;
	}
	if (Rhosts[i].sock >= 0 && sock == Rhosts[i].sock) {
	    host_ind = i;
	    Rhosts[i].sock = -1;
	    Rhosts[i].failed = 0;
	    MISC_log ("Deregister_client %x", Rhosts[i].ip);
	    break;
	}
    }

    Manage_scd (SCD_FREE, sock);
    for (i = N_client_regs - 1; i >= 0; i--) {
	Client_reg_t *reg;

	reg = Client_reg + i;
	if (reg->fd == LB_AN_FD && host_ind >= 0)
	    Process_sv_ack (reg->msgid, host_ind);

	if (reg->sock == sock) {
	    if (reg->fd == LB_AN_FD && reg->pid >= 0) /* local AN */
		Send_dereg_to_svs (reg->msgid);	/* dereg AN on remote sv's */
	    Delete_client_entry (i);		/* rm the entry */
	}
    }
    if (need_close)
	LB_EXT_sv_cntl (LB_EXT_CLOSE_CLIENT, sock);

    return;
}

/********************************************************************
			
    Description: This function sends a message to the client.
		This function tries to continue to send any data left
		over from previous messages that could not be sent 
		due to socket buffer full. Two types 
		of messages, high priority and low priority, are 
		processed. The high priority message is saves in
		the SCD data buffer if the socket is full. The low
		priority message is saved only if it is already
		partially sent. Otherwise SEND_FAILED is returned.
		The data buffer is reallocated whenever needed.
		This function requests write polling in case of any 
		left over data. A signal is send to the client when 
		message sending is completed and "signal" >= 0. NTF 
		registration ack msgs do not need signal because
		LB_?N_register will poll for it.

    Input:	sock - client socket fd;
		msg - the message to sent. NULL indicates sending
			the queued messages only.
		msg_len - length of the message.
		prio - priority (PRIO_LOW, PRIO_HIGH).
		pid - the client's pid. < 0 indicates remote clients.
		signal - signal to be sent to the client.
			< 0 indicates no signaling is needed.

    Return:	SEND_DONE - sent with success;
		SEND_QUEUED - msg is queued for later sending;
		SEND_REJECTED - sending failed due to buffer full;
		SEND_FAILED - sending failed due to other reason;

********************************************************************/

static int Send_msg_to_client (int sock, char *msg, int msg_len,
			int prio, int pid, int signal)
{
    Scd_t *scd;
    int ret, plen;

    if (pid < 0 && msg != NULL) {	/* pack msg for remote server */
	msg = LB_EXT_pack_msg ((char *)msg, msg_len, &plen);
	msg_len = plen;
    }

    if ((scd = Manage_scd (SCD_GET, sock)) != NULL) {
	ret = 0;
	if (scd->n_written < scd->n_total) /* continue unfinished write */
	    ret = NET_write_socket (sock, scd->data + scd->n_written, 
					scd->n_total - scd->n_written);
	if (ret < 0)
	    return (SEND_FAILED);
	else
	    scd->n_written += ret;
	if (scd->n_written >= scd->n_total)
	    Manage_scd (SCD_FREE, sock);
	else {
	    LB_EXT_sv_cntl (LB_EXT_POLL_WRITE, sock);
	    if (msg == NULL)
		return (SEND_QUEUED);
	    if (prio == PRIO_HIGH) {
		Append_to_scd_buffer (scd, msg, msg_len);
		return (SEND_QUEUED);
	    }
	    else
		return (SEND_REJECTED);
	}
    }

    if (msg == NULL) {
	if (scd != NULL && scd->signal != SIGPOLL && scd->signal >= 0)
	    kill (scd->pid, scd->signal);
	return (SEND_DONE);
    }

    ret = NET_write_socket (sock, msg, msg_len);
    if (ret < 0)
	return (SEND_FAILED);
    if (ret != msg_len) {
	scd = Manage_scd (SCD_SAVE, sock);
	scd->sock = sock;
	scd->n_total = 0;
	Append_to_scd_buffer (scd, msg, msg_len);
	scd->n_written = ret;
	scd->pid = pid;
	scd->signal = signal;
	LB_EXT_sv_cntl (LB_EXT_POLL_WRITE, sock);
	return (SEND_QUEUED);
    }

    if (signal >= 0 && signal != SIGPOLL)
	kill (pid, signal);

    return (SEND_DONE);
}

/********************************************************************
			
    Description: This function manages the saved client data for 
		each client connection. The current 4 function 
		implementation allows more fields added to SCD.

		func = SCD_GET - returns pointer to the SCD for "sock".
		func = SCD_IND_GET - returns pointer to the SCD of 
				given index.
		func = SCD_FREE - frees SCD for "sock".
		func = SCD_SAVE - returns an empty SCD buffer.

    Input:	func - function selection;
		sock - client socket fd or SCD index if func = 
			SCD_IND_GET;

    Return:	Pointer to a SCD structure or NULL on failure.

********************************************************************/

static Scd_t *Manage_scd (int func, int sock)
{
    static void *tid = NULL;
    static Scd_t *scds = NULL;
    static int n_scds = 0;
    int i;

    if (func == SCD_GET) {
	for (i = 0; i < n_scds; i++)
	    if (scds[i].sock == sock)
		return (scds + i);
	return (NULL);			/* not found */
    }

    if (func == SCD_IND_GET) {
	if (sock >= n_scds)
	    return (NULL);
	else
	    return (scds + sock);
    }

    if (func == SCD_FREE) {
	for (i = 0; i < n_scds; i++) {
	    Scd_t *tscd;

	    tscd = scds + i;
	    if (tscd->sock == sock) {
		if (tscd->data != NULL)
		    free (tscd->data);
		MISC_table_free_entry (tid, i);	/* rm the entry */
		break;
	    }
	}
	return (NULL);			/* not expect to be used */
    }

    if (func == SCD_SAVE) {
	Scd_t *new;

	while (tid == NULL) {			/* create the table */
	    tid = MISC_open_table (sizeof (Scd_t), 
				8, 0, &n_scds, (char **)&scds);
	    if (tid == NULL)
		msleep (WAITING_FOR_MEMORY_TIME);
	}

	while ((new = (Scd_t *)MISC_table_new_entry (tid, NULL)) == NULL)
	    msleep (WAITING_FOR_MEMORY_TIME);
	new->buf_size = 0;
	new->data = NULL;
	return (new);
    }
    return (NULL);			/* unexpected */
}

/********************************************************************
			
    Description: This function appends a new message to the SCD
		data buffer.

    Input:	scd - the SCD structure.
		msg - the message to append.
		msg_len - length of the message.

********************************************************************/

static void Append_to_scd_buffer (Scd_t *scd, char *msg, int msg_len)
{

    if (msg_len + scd->n_total > scd->buf_size) {	/* realloc buffer */
	int new_size;
	char *cpt;

	new_size = scd->buf_size + msg_len * 3;	/* We alloc extra space to 
						   reduce realloc frequency */
	while ((cpt = malloc (new_size)) == NULL)
	    msleep (WAITING_FOR_MEMORY_TIME);
	scd->buf_size = new_size;
	if (scd->n_total > 0)
	    memcpy (cpt, scd->data, scd->n_total);
	if (scd->data != NULL)
	    free (scd->data);
	scd->data = cpt;
    }
    memcpy (scd->data + scd->n_total, msg, msg_len);
    scd->n_total += msg_len;

    return;
}

/********************************************************************
			
    Description: This function sends a registeration ack message 
		to a remote server. This function disconnects the client 
		socket if an error is detected in writing the message. 

    Input:	sock - the remote server's socket fd;
		reg_msg - the received registration msg.
		status - TO_CLIENT_ACK_DONE or TO_CLIENT_ACK_FAILED;

********************************************************************/

static void Send_sv_ack_msg (int sock, Ntf_regist_msg_t *reg_msg, int status)
{
    Ntf_msg_t msg;
    int ret;

    msg.msg_type = TO_SV_ACK;
    msg.fd = reg_msg->fd;
    msg.msgid = reg_msg->msgid;
    msg.lost_cnt = reg_msg->a_pid;	/* echo back the host index */
    msg.msg_len = LB_T_BSWAP (status);

    ret = Send_msg_to_client (sock, (char *)&msg, 
		sizeof (Ntf_msg_t), PRIO_HIGH, -1, -1);

    if (Ntfrr_ind != INDEX_DISABLED)
	Ntf_report (
		"Send ack msg to sv: sock %d, msgid %x, status %d, ret %d\n", 
				sock, LB_T_BSWAP (msg.msgid), status, ret);

    if (ret == SEND_QUEUED)
	Stop_local_read ();
    if (ret != SEND_DONE && ret != SEND_QUEUED) {
	MISC_log ("Send_sv_ack_msg failed");
	Deregister_client (sock, 1);
    }

    return;
}

/********************************************************************
			
    Description: This function checks if an AN event id is already 
		registered in remote hosts. If a registration of the
		same event is in progress, it copies over the remote
		host mask.

    Input:	new_reg - the new client registration entry;

    Return:	LB_TRUE if already registered or registration in progress. 
		Otherwise LB_FALSE.

********************************************************************/

static int Is_AN_registered (Client_reg_t *new_reg)
{
    int next_ind;

    next_ind = Get_client_table_ind (new_reg->msgid);
    while (next_ind != INDEX_INVALID) {
	Client_reg_t *reg;

	reg = Client_reg + next_ind;
	next_ind = reg->next;
	if (reg->fd == LB_AN_FD && reg->pid >= 0 && reg != new_reg) {
								/* found */
	    new_reg->spec.an.reg_mask = reg->spec.an.reg_mask;
	    new_reg->spec.an.n_rhosts = reg->spec.an.n_rhosts;
	    return (LB_TRUE);
	}
    }
    return (LB_FALSE);
}

/********************************************************************
			
    Description: This function sends registration msgs to all active
		remote hosts and sets the remote host mask.

		We don't call Deregister_client in case of failed 
		client writing here because we can not change the 
		Client_reg table (we need to continue and finish the 
		registration of the new entry). We mark the host as
		failed. The clean-up for the client connection will
		be processed later when next event is processed.

    Input:	new_reg - the new client registration entry;

********************************************************************/

static void Send_AN_regist_to_svs (Client_reg_t *new_reg)
{
    int i, cnt;
    unsigned int mask;

    /* sends reg msgs to all active remove hosts */
    mask = 0;
    cnt = 0;
    for (i = 0; i < N_rhosts; i++) {
	int ret;

	if (Rhosts[i].sock >= 0) {
	    Ntf_regist_msg_t rreg;

	    rreg.msg_type = FROM_SV_REG;
	    rreg.signal = LB_T_BSWAP (new_reg->signal);
	    rreg.pid = LB_T_BSWAP (-1);
	    rreg.fd = LB_T_BSWAP (new_reg->fd);
	    rreg.msgid = LB_T_BSWAP (new_reg->msgid);
	    rreg.a_pid = LB_T_BSWAP (i);

	    ret = Send_msg_to_client (Rhosts[i].sock, (char *)&rreg, 
		sizeof (Ntf_regist_msg_t), PRIO_HIGH, -1, -1);

	    if (Ntfrr_ind != INDEX_DISABLED)
		Ntf_report (
			"Send AN regist to sv: sock %d, msgid %x, ret %d\n", 
				Rhosts[i].sock, new_reg->msgid, ret);

	    if (ret == SEND_DONE || ret == SEND_QUEUED) {
		mask |= (1 << i);
		cnt++;
	    }
	    else {
		MISC_log ("Send_AN_regist_to_svs (%x) failed", Rhosts[i].ip);
		Rhosts[i].failed = 1;
		Rhost_failed = 1;
	    }
	    if (ret == SEND_QUEUED)
		Stop_local_read ();
	}
    }
    new_reg->spec.an.reg_mask = mask;
    new_reg->spec.an.n_rhosts = cnt;
    return;
}

/********************************************************************
			
    Description: This function processes a AN reg ACK message from 
		a remote server. If a connection to a remote server
		is lost, it is considered as a server ACK too.

    Input:	msgid - the ACK AN ID;
		host_ind - Rhosts index of the ACK.

********************************************************************/

static void Process_sv_ack (LB_id_t msgid, int host_ind)
{
    unsigned int host_mask;

    host_mask = ~(1 << host_ind);
    while (1) {		/* repeat the procedure in case of deregistration */
	int next_ind, done;

	done = 1;
	next_ind = Get_client_table_ind (msgid);
	while (next_ind != INDEX_INVALID) {
	    Client_reg_t *reg;
	    unsigned int omask, nmask;

	    reg = Client_reg + next_ind;
	    next_ind = reg->next;
	    if (reg->fd == LB_AN_FD) {
		omask = reg->spec.an.reg_mask;
		nmask = omask & host_mask;
		if (nmask != omask) {
		    if (Ntfrr_ind != INDEX_DISABLED)
			Ntf_report (
			    "remote AN reg ACK recvd: ID %x, host ind %d\n", 
						msgid, host_ind);
		    reg->spec.an.reg_mask = nmask;
		    if (nmask == 0) {	/* send reg ack to client */
			if (Send_ack_msg (reg, TO_CLIENT_ACK_DONE) < 0) {
			    done = 0;
			    break;
			}
		    }
		}
	    }
	}
	if (done)
	    break;
    }
    return;
}

/*************************************************************
			
    Description: This function processes an AN message from 
		either a remote server or a local LB_AN_post. 
		If sending message to a client fails, we don't 
		process the exception in order to continue 
		service to other clients.

    Input:	msg - the received AN message;
		input_len - length of the msg as received from 
			the messaging server.

**************************************************************/

#define MAX_RMT_HOSTS	64	/* max number of possible hosts */

static void Process_AN_msg (Ntf_msg_t *msg, int input_len)
{
    LB_id_t event_id;
    int msg_len, rmt_an, next_ind;
    Client_reg_t *cl_reg;
    int n_fd_sent, fd_sent[MAX_RMT_HOSTS];
    int self_only;		/* send this AN to process itself only */
    int not_to_self;		/* not to send this AN to process itself */
    int sender_id;

    if (msg->msg_type == FROM_SV_AN) {
	Bswap_AN_msg_header (msg);
	rmt_an = LB_TRUE; /* we need this bacause msg->msg_type will change */
    }
    else {
	rmt_an = LB_FALSE;
	if (msg->msgid == LB_REP_REG_TABLE) {
	    Report_reg_table ();
	    return;
	}
	if (msg->msgid == LB_DISC_HOST) {
	    Cmd_disconnect_host (msg, input_len);
	    return;
	}
    }

    event_id = msg->msgid;
    msg_len = msg->msg_len;
    self_only = not_to_self = 0;
    if (msg_len & AN_SELF_ONLY)
	self_only = 1;
    if (msg_len & AN_NOT_TO_SELF)
	not_to_self = 1;
    if (self_only || not_to_self) {
	sender_id = LB_SHORT_BSWAP (msg->sender_id);
	msg_len &= AN_MSG_LEN_MASK;
	msg->msg_len = msg_len;
    }

    if (Ntfrr_ind != INDEX_DISABLED)
	Ntf_report ("AN recvd: ID %x, len %d, remote %d\n", 
					event_id, msg_len, rmt_an);

    if (input_len != sizeof (Ntf_msg_t) + msg_len) {
	MISC_log ("unexpected AN msg length");
	return;
    }

    n_fd_sent = 0;
    next_ind = Get_client_table_ind (event_id);
    while (next_ind != INDEX_INVALID) {

	cl_reg = Client_reg + next_ind;
	next_ind = cl_reg->next;
	if (cl_reg->fd != LB_AN_FD || 		/* not an AN */
		cl_reg->spec.an.reg_mask != 0)	/* regist incomplete */
	    continue;

	if (rmt_an && cl_reg->pid < 0)
	    continue;		/* we don't sent remote AN to remote servers */

	if (self_only && cl_reg->pid != sender_id)
	    continue;		/* self_only implies a local AN */
	if (not_to_self && cl_reg->pid == sender_id)
	    continue;		/* not_to_self implies a local AN */

	Send_AN_msg (msg, cl_reg, msg_len);

	if (!rmt_an && cl_reg->pid < 0) {
	    if (n_fd_sent >= MAX_RMT_HOSTS) 
		MISC_log ("Unexpectedly too many hosts");
	    else {		/* record rmt hosts that has been sent */
		fd_sent[n_fd_sent] = cl_reg->fd;
		n_fd_sent++;
	    }
	}
    }

    if (An_lb_any_ind == INDEX_UNKNOWN) {
	An_lb_any_ind = Search_index_table (LB_ANY);
	if (An_lb_any_ind < 0)
	    An_lb_any_ind = INDEX_DISABLED;
    }
    if (An_lb_any_ind == INDEX_DISABLED)
	return;

    next_ind = Indices[An_lb_any_ind];
    while (next_ind != INDEX_INVALID) {		/* process LB_ANY */

	cl_reg = Client_reg + next_ind;
	next_ind = cl_reg->next;
	if (cl_reg->fd != LB_AN_FD || 		/* not an AN */
		cl_reg->spec.an.reg_mask != 0)	/* regist incomplete */
	    continue;

	if (self_only && cl_reg->pid != sender_id)
	    continue;
	if (not_to_self && cl_reg->pid == sender_id)
	    continue;

	if (cl_reg->pid >= 0) {		/* local registration */
	    Send_AN_msg (msg, cl_reg, msg_len);
	}
	else if (!rmt_an) {		/* remote reg and local AN source */
	    int k;
	    for (k = 0; k < n_fd_sent; k++)
		if (cl_reg->fd == fd_sent[k])
		    break;
	    if (k >= n_fd_sent)		/* not yet sent */
		Send_AN_msg (msg, cl_reg, msg_len);
	}
    }

    return;
}

/*************************************************************
			
    Description: This function sends an AN message to a client.

    Input:	msg - the AN message;
		cl_reg - client registration record.
		msg_len - AN message length;

**************************************************************/

static void Send_AN_msg (Ntf_msg_t *msg, Client_reg_t *cl_reg, int msg_len)
{
    int prio, ret;

    msg->msg_type = TO_CLIENT_AN;
    msg->lost_cnt = cl_reg->lost_cnt;
    prio = PRIO_LOW;
    if (cl_reg->pid < 0) {			/* remote client */
	msg->msg_type = FROM_SV_AN;
	prio = PRIO_HIGH;
	Bswap_AN_msg_header (msg);
    }
    if (cl_reg->lost_cnt <= 0)
	ret = Send_msg_to_client (cl_reg->sock, (char *)msg, 
				sizeof (Ntf_msg_t) + msg_len, prio, 
				cl_reg->pid, cl_reg->signal);
    else
	ret = SEND_FAILED;
	 
    if (Ntfrr_ind != INDEX_DISABLED)
	Ntf_report ("send AN: sock %d, ID %x, ret %d\n", 
				cl_reg->sock, cl_reg->msgid, ret);

    if (cl_reg->pid < 0) {
	Bswap_AN_msg_header (msg);		/* swap back */
	if (ret == SEND_QUEUED)
	    Stop_local_read ();
    }
   
    if (ret == SEND_DONE || ret == SEND_QUEUED) 	/* send success */
	cl_reg->lost_cnt = 0;
    else {				/* notification lost */
	cl_reg->lost_cnt++;
	Manage_saved_AN (MSAN_SAVE, 0, msg);
	Lost_ntf = 1;
    }

    return;
}

/*************************************************************
			
    Description: This function performs byte swap on all fields
		of an AN message.

    Input:	msg - the AN message;

**************************************************************/

static void Bswap_AN_msg_header (Ntf_msg_t *msg)
{

    msg->fd = LB_T_BSWAP (msg->fd);
    msg->msgid = LB_T_BSWAP (msg->msgid);
    msg->msg_len = LB_T_BSWAP (msg->msg_len);
    return;
}

/********************************************************************
			
    Description: This function manages the saved latest AN events.

    Input:	func - functions:
			MSAN_GET: for retrieving "event",
			MSAN_SAVE for saving AN "msg",
			MSAN_DELETE for deleting unused ANs.
		event - AN event ID for retrieving an event,
		msg - the AN event msg to be saved;

    Return:	Pointer to an AN event msg in case of retrieving.
		NULL otherwise.

********************************************************************/

#define MSAN_HOUSEKEEP_PERIOD 10

static Ntf_msg_t *Manage_saved_AN (int func, LB_id_t event, Ntf_msg_t *msg)
{
    typedef struct {
	LB_id_t event_id;
	int buf_size;
	Ntf_msg_t *msg;
    } Saved_evt_t;
    static void *tid = NULL;
    static Saved_evt_t *ses = NULL;
    static int n_ses = 0;
    static time_t last_housekeep = 0;
    int i;

    if (func == MSAN_GET) {	/* retrieve */
	for (i = 0; i < n_ses; i++)
	    if (ses[i].event_id == event)
		return (ses[i].msg);
	return (NULL);			/* not found */
    }

    if (func == MSAN_SAVE) {			/* save a message */
	Saved_evt_t *s;
	int size, i;
	LB_id_t event_id;

	event_id = msg->msgid;

	s = NULL;
	for (i = 0; i < n_ses; i++) 
	    if (ses[i].event_id == event_id) {
		s = ses + i;
		break;
	    }
	
	if (s == NULL) {
	    while (tid == NULL) {		/* create the table */
		tid = MISC_open_table (sizeof (Saved_evt_t), 
						8, 0, &n_ses, (char **)&ses);
		if (tid == NULL)
		    msleep (WAITING_FOR_MEMORY_TIME);
	    }

	    while ((s = (Saved_evt_t *)MISC_table_new_entry (tid, NULL)) 
								== NULL)
		msleep (WAITING_FOR_MEMORY_TIME);
	    s->buf_size = 0;
	    s->msg = NULL;
	}

	size = sizeof (Ntf_msg_t) + msg->msg_len;
	if (size > s->buf_size) {		/* enlarge the buffer */
	    if (s->msg != NULL)
		free (s->msg);
	    while ((s->msg = malloc (size)) == NULL)
		msleep (WAITING_FOR_MEMORY_TIME);

	    s->buf_size = size;
	}
	memcpy ((char *)s->msg, (char *)msg, size);
	s->event_id = event_id;

	return (s->msg);
    }

    if (func == MSAN_DELETE) {		/* house keeping - delete msgs */
	time_t tm;

	tm = time (NULL);
	if (tm < last_housekeep + MSAN_HOUSEKEEP_PERIOD)
	    return (NULL);
	last_housekeep = tm;

	for (i = n_ses - 1; i >= 0; i--) {
	    int next_ind, in_use;

	    next_ind = Get_client_table_ind (ses[i].event_id);
	    in_use = 0;
	    while (next_ind != INDEX_INVALID) {
		Client_reg_t *reg;

		reg = Client_reg + next_ind;
		next_ind = reg->next;
		if (reg->fd == LB_AN_FD && reg->lost_cnt > 0) {
		    in_use = 1;
		    break;
		}
	    }

	    if (!in_use) {	/* not in use - we remove it */
		if (ses[i].msg != NULL)
		    free (ses[i].msg);
		MISC_table_free_entry (tid, i);	/* rm the entry */
	    }
	}
    }

    return (NULL);
}

/*************************************************************
			
    Description: This function stops accepting data from all 
		local messaging sockets.

**************************************************************/

static void Stop_local_read ()
{

    if (Stop_local_read_time == 0) {
	Stop_local_read_time = time (NULL);
	LB_EXT_sv_cntl (LB_EXT_LOCAL_READ, LB_EXT_LOCAL_READ_PAUSE);
	MISC_log ("network congestion - local socket read stopped");
    }
    return;
}

/*************************************************************
			
    Description: This function checks whether any remote socket
		is still full. It then tries to send the queued
		data (In case write available callback is missed).
		Finally, if non of them is full, it resumes
		reading local messaging client sockets.

**************************************************************/

static void Resume_local_read ()
{
    int ind, ret;

    ind = 0;
    while (1) {
	Scd_t *scd;

	scd = Manage_scd (SCD_IND_GET, ind);
	if (scd == NULL) 
	    break;

	if (scd->pid < 0) {	/* a remote host */
	    ret = Send_msg_to_client (scd->sock, NULL, 0, 0, -1, -1);
	    if (ret == SEND_FAILED) {	/* system call failure */
		MISC_log ("Resume_local_read failed");
		Deregister_client (scd->sock, 1);
		ind = 0;	/* SCD table updated - start over again */
		continue;
	    }
	    else if (ret == SEND_QUEUED)	/* still full */
		return;
	}
	ind++;
    }

    /* all remote sockets are not full */
    LB_EXT_sv_cntl (LB_EXT_LOCAL_READ, LB_EXT_LOCAL_READ_RESUME);
    Stop_local_read_time = 0;
    MISC_log ("local socket read resumed");
    return;
}

/********************************************************************
			
    Description: This function sets up the remote host table and sends
		an init conn message to each of them. This function
		is called while the server is still in the foreground.
		Thus we exit in case of an error.

********************************************************************/

static void Init_rhosts ()
{
    unsigned int ip[MAX_AN_RHOSTS], *ladd;
    int nl, cnt, i;

    nl = NET_find_local_ip_address (&ladd);
    if (nl <= 0) {
	MISC_log ("NET_find_local_ip_address failed");
	exit (1);
    }
    Local_ip = ladd[0];

    N_rhosts = LB_EXT_get_client_IP (MAX_AN_RHOSTS, ip);
    if ((Rhosts = (Remote_hosts *)malloc 
			(N_rhosts * sizeof (Remote_hosts))) == NULL) {
	MISC_log ("malloc failed");
	exit (1);
    }

    cnt = 0;
    for (i = 0; i < N_rhosts; i++) {
	int k;

	ip[i] = LB_T_BSWAP (ip[i]);
	for (k = 0; k < nl; k++)
	    if (ip[i] == ladd[k])
		break;
	if (k < nl) {			/* a local address */
	    if (k > 0) {		/* Refer to Process_conn prolog */
		MISC_log ("bad local IP address in host conf");
		exit (1);
	    }
	    continue;
	}

	for (k = 0; k < cnt; k++)
	    if (ip[i] == Rhosts[k].ip)
		break;
	if (k < cnt) 			/* a duplicated address */
	    continue;

	Rhosts[cnt].ip = ip[i];
	Rhosts[cnt].sock = -1;
	Rhosts[cnt].init_sock = -1;
	Rhosts[cnt].conn_seq = 0;
	Rhosts[cnt].failed = 0;
	Rhosts[cnt].cmd_disc = 0;

	Send_conn_init_to_rhost (Rhosts + cnt, 1, "init");
	cnt++;
    }
    N_rhosts = cnt;

    return;
}

/********************************************************************
			
    Description: This function processes a remote connection message.
    We don't process error when sending conn resp msg fails. Any bad 
    connection will be processed later.

    The connection procedure must: 1. One and only one connection is
    built between two servers. 2. The servers can start in any order,
    or simmultaneously. 3. The network delay of each socket is 
    independent and may be slow. 4. Any side can reconnect regardless 
    of the other side's state and competing reconnection requests from 
    both sides are resolved. 5. A server, after started, should 
    connect to existing servers as soon as possible and should not 
    provided AN service before all live remote servers are contacted.

    The procedure is: 1. The server of larger IP creates a socket and
    sends an init msg to the other side, the latter accepts the socket
    as the connection socket and responds by sending a resp msg. Upon
    receipt of the resp msg, the initiator completes the procedure.
    2. The server of the smaller IP side opens a socket and sends an 
    init msg for requesting a connection. The server of larger IP, upon
    receiving the msg, closes tha socket. It then starts 1, if it is
    in state of disconnected or in state of connected but a new 
    connection (identified by the connection number) is requested.
    3. No init msg is sent if there is a outstanding init msg. 4. When 
    a remote server conn is accepted, AN regist info message is sent.

    Note that, in order for the above to work, one has to use the host 
    names or IP addresses in the .rssd.conf that are corresponding to the 
    first address returned by NET_find_local_ip_address on each host. And 
    there should be no multiple IP addresses specified for each host in 
    .rssd.conf. If any of these requirements is not met, some of the 
    server may not be started. Mechanisms need to be implemented such 
    that NTF server can support alternative local IP addresses (network 
    interfaces) in the future.

    Input:	msg - the remote conn message;
		sock - conn socket.
		host - remote host IP address.
		msg_len - message length.

********************************************************************/

static void Process_conn (Ntf_msg_t *msg, int sock, 
				unsigned int host, int msg_len)
{
    Remote_hosts *rh;
    int i, fd, seq;
    char buf[128];

    fd = LB_T_BSWAP (msg->fd);
    seq = LB_T_BSWAP (msg->msgid);

    if (fd == SV_CONN_REGS) {
	Receive_AN_reg_info (msg, sock, msg_len);
/*	MISC_log ("AN reg info recv from %x", host); */
	return;
    }

    for (i = 0; i < N_rhosts; i++) {
	rh = Rhosts + i;
	if (rh->ip == host)
	    break;
    }
    if (i >= N_rhosts) {	/* not found */
	MISC_log ("connecting host (%x) not found in rhost conf", host);
	return;
    }

    if (fd == SV_CONN_INIT) {		/* conn init msg */
	if (host > Local_ip) {
	    /* send a conn resp message back */
	    msg->fd = LB_T_BSWAP (SV_CONN_RESP);
	    msg->msgid = LB_T_BSWAP (seq);
	    Send_msg_to_client (sock, (char *)msg, 
			sizeof (Ntf_msg_t), PRIO_HIGH, -1, -1);
	    sprintf (buf, "Sending AN reg to %x - connected - init", host);
	    Accept_connection (rh, sock, seq, buf);
	}
	else {
	    LB_EXT_sv_cntl (LB_EXT_CLOSE_CLIENT, sock);
	    if ((rh->sock < 0 && rh->init_sock < 0) ||	/* not connected */
		(rh->sock >= 0 && rh->conn_seq != seq)) {
		if (rh->sock >= 0) {
		    MISC_log ("New conn request received");
		    Deregister_client (rh->sock, 1);
		}
		Send_conn_init_to_rhost (rh, 1, "counter");
	    }
	}
    }
    else {				/* conn resp msg */
	sprintf (buf, "Sending AN reg to %x - connected - resp", host);
	Accept_connection (rh, sock, seq, buf);
    }

    return;
}

/********************************************************************
			
    Accepts a remote server connection.

    Input:	rh - the remote server.
		sock - the connection sock.
		tm - current time.
		seq - connection sequence number.

********************************************************************/

static void Accept_connection (Remote_hosts *rh, int sock, 
					int seq, char *report) {
    if (rh->sock >= 0) {
	MISC_log ("New init msg received");
	Deregister_client (rh->sock, 1);
    }
    rh->sock = sock;	/* accept connection */
    rh->conn_seq = seq;
    if (rh->init_sock == sock)
	rh->init_sock = -1;
    if (rh->cmd_disc) {
	Write_cmd_disc_file (0, rh->ip);
	rh->cmd_disc = 0;
    }
    MISC_log (report);
    Send_AN_reg_info (sock);
}

/********************************************************************
			
    This function sends a conn init msg to a remote server. It opens a 
    socket to send the message. It closes if sending fails.

    Input:	rh - the remote server.
		block - blocking until success.
		reason - reason this is called.

    Return:	The init sock on success, LB_EXT_RET_TIMED_OUT on 
		timed-out or other negative error number on failure.

********************************************************************/

static int Send_conn_init_to_rhost (Remote_hosts *rh, int block, char *reason)
{
    Ntf_msg_t msg;
    int ret, sock, cnt;
    char buf[128];

    if (rh->init_sock >= 0) {
	MISC_log ("Unclosed init_sock");
	return (-1);
    }

    msg.msg_type = FROM_SV_CONN;
    msg.fd = LB_T_BSWAP (SV_CONN_INIT);
    msg.msgid = LB_T_BSWAP (rh->conn_seq + 1);

    cnt = 0;
    while ((sock = LB_EXT_sv_cntl (LB_EXT_OPEN_CONN, rh->ip)) < 0) {
	if (sock == LB_EXT_RET_TIMED_OUT) {
	    if (!block)
		return (sock);
	    if (cnt >= 20)		/* wait about 4 seconds */
		sprintf (buf, "Connecting %x timed out", rh->ip);
	    else {
		msleep (200);
		cnt++;
		continue;
	    }
	}
	else if (block)
	    sprintf (buf, "Connecting %x not possible", rh->ip);
	if (block)
	    MISC_log (buf);
	return (sock);
    }
    ret = Send_msg_to_client (sock, (char *)&msg, 
			    sizeof (Ntf_msg_t), PRIO_HIGH, -1, -1);
    if (ret != SEND_DONE && ret != SEND_QUEUED) {
	MISC_log ("Send_conn_init_to_rhost failed");
	LB_EXT_sv_cntl (LB_EXT_CLOSE_CLIENT, sock);
	return (-1);
    }
    rh->init_sock = sock;
    MISC_log ("Init n = %d sent to %x (sock %d) - %s", 
				rh->conn_seq + 1, rh->ip, sock, reason);
    return (sock);
}

/*************************************************************
			
    Periodically checks connections to remote servers and 
    sends conn init msg to disconncted remote servers.

    Input:	tm - current time.

**************************************************************/

static void Check_rsv_connection (time_t tm) {
    static time_t pst_tm = 0;		/* current period start time */
    static time_t cst_tm = 0;		/* current conn start time */
    static int check_cnt = 0;		/* check count in the current period */
    static int rh_ind = 0;		/* current rhost index */

    if (pst_tm == 0) {
	pst_tm = tm;
	check_cnt = N_rhosts;		/* disable the first period */
	return;
    }
    if (tm >= pst_tm + RECONN_PERIOD) {	/* start a new period */
	check_cnt = 0;
	pst_tm = tm;
    }

    while (check_cnt < N_rhosts) {
	if (Rhosts[rh_ind].sock < 0 &&	Rhosts[rh_ind].init_sock < 0 &&
	    (cst_tm == 0 || (cst_tm > 0 && tm < cst_tm + CONN_WAIT_TIME)) &&		    Send_conn_init_to_rhost (Rhosts + rh_ind, 0, "reconn")
						 == LB_EXT_RET_TIMED_OUT) {
	    if (cst_tm == 0)
		cst_tm = tm;
	    return;
	}
	cst_tm = 0;
	rh_ind = (rh_ind + 1) % N_rhosts;
	check_cnt++;
    }
}

/********************************************************************
			
    Description: Sends current AN registration info to remote 
		server "sock".

********************************************************************/

static void Send_AN_reg_info (int sock)
{
    unsigned int *id, cr_msgid;
    Ntf_msg_t *msg;
    int cnt, i;

    while ((msg = (Ntf_msg_t *)malloc (sizeof (Ntf_msg_t) + 
			N_client_regs * sizeof (unsigned int))) == NULL)
	msleep (WAITING_FOR_MEMORY_TIME);

    cnt = 0;
    cr_msgid = 0xffffffff;
    id = (unsigned int *)((char *)msg + sizeof (Ntf_msg_t));
    for (i = 0; i < N_indices; i++) {
	Client_reg_t *cl_reg;

	cl_reg = Client_reg + Indices[i];
	while (1) {

	    if (cl_reg->fd == LB_AN_FD && cl_reg->msgid != cr_msgid
		&& cl_reg->msgid <= LB_MAX_ID && cl_reg->pid >= 0) {
		id[cnt] = LB_T_BSWAP (cl_reg->msgid);
		cr_msgid = cl_reg->msgid;
		cnt++;
	    }
	    if (cl_reg->next == INDEX_INVALID)
		break;
	    cl_reg = Client_reg + cl_reg->next;
	}
    }
    if (cnt > 0) {
	msg->msg_type = FROM_SV_CONN;
	msg->msg_len = LB_T_BSWAP (cnt);
	msg->fd = LB_T_BSWAP (SV_CONN_REGS);
	Send_msg_to_client (sock, (char *)msg, 
		sizeof (Ntf_msg_t) + cnt * sizeof (unsigned int), 
		PRIO_HIGH, -1, -1);
    }
    else
	MISC_log ("Empty AN reg info - not sent");

    free (msg);
}

/********************************************************************
			
    Description: Receives the current AN regist info from a remote
		server and update the local NTF reg table.

********************************************************************/

static void Receive_AN_reg_info (Ntf_msg_t *msg, int sock, int msg_len)
{
    unsigned int *idp, id;
    int cnt, i;

    cnt = LB_T_BSWAP (msg->msg_len);
    idp = (unsigned int *)((char *)msg + sizeof (Ntf_msg_t));
    if (msg_len < cnt * (int)sizeof (unsigned int) + (int)sizeof (Ntf_msg_t)) {
	MISC_log ("unexpected NTF reg table msg length");
	return;
    }
    An_lb_any_ind = INDEX_UNKNOWN;
    Ntfrr_ind = INDEX_UNKNOWN;
    for (i = 0; i < cnt; i++) {
	Client_reg_t *cl_reg;

	if (N_client_regs >= INDEX_MAX) {
	    MISC_log ("NTF client reg table overflow (1)");
	    break;
	}
	id = LB_T_BSWAP (idp[i]);
	cl_reg = Get_new_client_entry (id);
	cl_reg->sock = sock;
	cl_reg->signal = -1;
	cl_reg->pid = -1;
	cl_reg->fd = LB_AN_FD;
	cl_reg->msgid = id;
	cl_reg->lost_cnt = 0;
	cl_reg->spec.an.reg_mask = 0;
    }
}

/****************************************************************
						
    Description: This is an LB service funciton. It sends a NTF 
		report message as a special AN
		to a registered client. We only send to the first 
		client in the client registeration table.

    Input:	format - the message format;

****************************************************************/

static void Ntf_report (const char *format, ...)
{
    va_list args;
    ALIGNED_t msg[ALIGNED_T_SIZE (256)];
    char *cpt;
    Ntf_msg_t *hd;
    int len;
    Client_reg_t *cl_reg;
    time_t tm;

    if (Ntfrr_ind == INDEX_UNKNOWN) {
	Ntfrr_ind = Search_index_table (LB_NTF_REPORT);
	if (Ntfrr_ind < 0)
	    Ntfrr_ind = INDEX_DISABLED;
    }

    if (Ntfrr_ind == INDEX_DISABLED)
	return;

    /* send the report as an AN message */
    cpt = (char *)msg + sizeof (Ntf_msg_t);
    tm = time (NULL);
    sprintf (cpt, "%.2d:%.2d ", (int)((tm / 60) % 60), (int)(tm % 60));
    va_start (args, format);
    vsprintf (cpt + 6, format, args);
    va_end (args);
    len = strlen (cpt) + 1;
    hd = (Ntf_msg_t *)msg;
    hd->msg_type = TO_CLIENT_AN;
    hd->fd = LB_AN_FD;
    hd->msgid = LB_NTF_REPORT;
    hd->msg_len = len;
    hd->lbmsgid = 0;
    hd->lost_cnt = 0;
    hd->sender_id = 0;
    cl_reg = Client_reg + Indices[Ntfrr_ind];
    Send_msg_to_client (cl_reg->sock, (char *)msg, 
	len + sizeof (Ntf_msg_t), PRIO_HIGH, cl_reg->pid, cl_reg->signal);
    return;
}

/*************************************************************
			
    Description: Reports the current registration table.

**************************************************************/

static void Report_reg_table ()
{
    int i;

    if (Ntfrr_ind == INDEX_DISABLED)
	return;

    Ntf_report ("  ---- NTF registration table ----\n");
    for (i = 0; i < N_indices; i++) {
	Client_reg_t *cl_reg;
	char text[128];

	cl_reg = Client_reg + Indices[i];
	while (1) {

	    sprintf (text, "AN sock %d, sig %d, pid %d, msgid %x", 
		cl_reg->sock, cl_reg->signal, cl_reg->pid, cl_reg->msgid);
	    if (cl_reg->lost_cnt > 0)
		sprintf (text + strlen (text), 
				", lost_cnt %d", cl_reg->lost_cnt);
	    if (cl_reg->fd == LB_AN_FD)
		sprintf (text + strlen (text), 
			", mask %x", cl_reg->spec.an.reg_mask);
	    else {
		text[0] = 'U';
		sprintf (text + strlen (text), ", fd %d, len %d", 
				cl_reg->fd, cl_reg->spec.un.msg_len);
		if (cl_reg->spec.un.a_fd >= 0) {
		    sprintf (text + strlen (text), ", a_fd %d, a_pid %d", 
				cl_reg->spec.un.a_fd, cl_reg->spec.un.a_pid);
		}	    
	    }

	    Ntf_report ("%s\n", text);
	    if (cl_reg->next == INDEX_INVALID)
		break;
	    cl_reg = Client_reg + cl_reg->next;
	}
    }
    Ntf_report ("\n");

    return;
}

/*************************************************************
			
    Processes the LB_DISC_HOST event of "msg" of length 
    "input_len".

**************************************************************/

#include <rmt.h>
#define MAX_LOCAL_CLIENTS 128

static void Cmd_disconnect_host (Ntf_msg_t *msg, int input_len) {
    int i;
    unsigned int *ipadd;

    if (input_len != sizeof (Ntf_msg_t) + sizeof (int)) {
	MISC_log ("unexpected LB_DISC_HOST msg length");
	return;
    }
    ipadd = (unsigned int *)((char *)msg + sizeof (Ntf_msg_t));

    MISC_log ("Commanded disc: host %x", *ipadd);
    RMT_kill_clients (*ipadd);		/* kill RPC child servers */

    for (i = 0; i < N_rhosts; i++) {	/* remove AN services */
	if (Rhosts[i].ip == *ipadd) {
	    if (Rhosts[i].sock >= 0)
		Deregister_client (Rhosts[i].sock, 1);
	    Rhosts[i].cmd_disc = 1;
	    break;
	}
    }

    /* notify all client applications */
    Write_cmd_disc_file (1, *ipadd);
}

/*************************************************************
			
    Writes a message to the .rssd.disc file and send a signal
    to all client applications. For simplicity, we assume a 
    maxmum number of local clients. If this assumption is not 
    true, more than one signal may be sent to a client.

**************************************************************/

static int Write_cmd_disc_file (int cmd_disc, unsigned int ip) {
    static int fd = -1, file_size;
    int i, n_done, sock_done[MAX_LOCAL_CLIENTS];
    time_t t;
    char buf[128];

    if (fd < 0) {		/* open the file */
	char *h;
	h = getenv ("HOME");
	if (h == NULL)
	    return (-1);
	sprintf (buf, "%s/.rssd.disc", h);
	fd = MISC_open (buf, O_RDWR | O_CREAT | O_TRUNC, 0664);
	if (fd < 0) {
	    MISC_log ("open cmd_disc_file failed (errno %d)\n", errno);
	    return (-1);
	}
	file_size = 0;
    }
    if (file_size > 4000) {
	lseek (fd, 0, SEEK_SET);
	file_size = 0;
    }

    t = time (NULL);
    if (cmd_disc)
	sprintf (buf, "%d CMD_DISCON: %x %d\n", (int)t, ip, (int)t);
    else
	sprintf (buf, "%d RE-ENABLE: %x %d\n", (int)t, ip, (int)t);
    if (MISC_write (fd, buf, strlen (buf)) < 0)
	return (-1);
    file_size += strlen (buf);

    n_done = 0;
    for (i = 0; i < N_client_regs; i++) {	/* send a signal to each local 
						   AN client */
	Client_reg_t *cl_reg;
	int k;

	cl_reg = Client_reg + i;
	if (cl_reg->pid < 0)
	    continue;
	for (k = 0; k < n_done; k++)
	    if (sock_done[k] == cl_reg->sock)
		break;
	if (k < n_done)
	    continue;
/*
	Send_msg_to_client (cl_reg->sock, (char *)msg, 
				input_len, PRIO_LOW, 
				cl_reg->pid, cl_reg->signal);
*/
	kill (cl_reg->pid, SIGUSR2);
	if (n_done >= MAX_LOCAL_CLIENTS)
	    continue;
	sock_done[n_done] = cl_reg->sock;
	n_done++;
    }
    return (0);
}

#endif			/* #ifdef LB_NTF_SERVICE */

