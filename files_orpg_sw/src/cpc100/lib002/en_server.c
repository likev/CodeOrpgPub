/****************************************************************
				
    Description: This module implements the server part of the
		event notification functions. This is the code in the 
		notification server.

*****************************************************************/

/*
 * RCS info 
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/08/07 19:41:22 $
 * $Id: en_server.c,v 1.31 2013/08/07 19:41:22 steves Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 * $Log: en_server.c,v $
 * Revision 1.31  2013/08/07 19:41:22  steves
 * CCR NA13-00181
 *
 * Revision 1.14  2006/04/24 18:54:15  jing
 * Update
 *
 * Revision 1.1  2002/03/12 16:39:49  jing
 * Initial revision
 *
*/

/* System include files */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h> 
#include <fcntl.h>
#include <errno.h>
#include <time.h>

/* Local include files */

#include <net.h>
#include <misc.h>
#include <rmt.h>
#include <en.h>

#include "en_def.h"

#define WAITING_FOR_MEMORY_TIME 200	/* in case of memory unavailable, the
					   server will retry every this time
					   period (in ms) */
#define RECONN_PERIOD		15	/* reconnection perios (seconds) */
#define CONN_WAIT_TIME		5	/* max waiting time for conn process */

#define MAX_MSG_SIZE 0x7fffffff		/* assuming that the server's msgs 
					   are always less than this */

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
    int a_code;				/* aliased code (< 0 means no alias) */
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
    int code;				/* event code (LB fd or EN_AN_CODE) */
    EN_id_t evtid;			/* LB message ID or the AN event ID */
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
					/* EN_ANY AN regist index */

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
    unsigned int ip;			/* IP address of remote server, NBO */
    int sock;				/* the socket of an active conn;
					   negative indicates inactive */
    int init_sock;			/* the sock opened for init msg */
    int init_timer;			/* the init sock timer so we can 
					   reinit connection procedure. */
    char failed;			/* error found with the sock */
    char conn_retry;			/* in retrying connection mode */
    short cmd_disc;			/* not used */
    time_t t_sent;			/* time sending the latest message */
    time_t t_recv;			/* time receiving the latest message */
} Remote_hosts;

static int N_rhosts = 0;		/* array size of Rhosts */
static Remote_hosts *Rhosts = NULL;	/* the remote host table */
static time_t Stop_local_read_time = 0;	/* time of stoping local socket read;
					   0 indicates not stopped */
static int Rhost_failed = 0;		/* connection to a remote server is 
					   detected to be bad */
static int No_resp_time = 0;		/* time limit for no-response test. 0
					   disables no-resp checking. */

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
static void Process_sv_ack (EN_id_t evtid, int host_ind);
static void Bswap_AN_msg_header (Ntf_msg_t *msg);
static void Process_AN_msg (Ntf_msg_t *msg, int input_len);
static void Send_AN_msg (Ntf_msg_t *msg, Client_reg_t *cl_reg, int msg_len);
static Ntf_msg_t *Manage_saved_AN (int func, EN_id_t event, Ntf_msg_t *msg);
static void Stop_local_read ();
static void Resume_local_read ();
static void Init_rhosts ();
static void Process_conn (Ntf_msg_t *msg, int sock, 
				unsigned int host, int msg_len);
static int Send_conn_init_to_rhost (Remote_hosts *rh, int block, char *reason);
static int Is_AN_registered (Client_reg_t *new_reg);
static void Send_dereg_to_svs (EN_id_t evtid);
static void Ntf_report (const char *format, ...);
static void Report_reg_table ();
static int Search_index_table (EN_id_t evtid);
static void Delete_client_entry (int cl_ind);
static Client_reg_t *Get_new_client_entry (EN_id_t evtid);
static int Get_client_table_ind (EN_id_t evtid);
static void Receive_AN_reg_info (Ntf_msg_t *msg, int sock, int msg_len);
static void Send_AN_reg_info (int sock);
static void Check_rsv_connection (time_t tm);
static void Accept_connection (Remote_hosts *rh, int sock, char *report);
static void Cmd_disconnect_host (Ntf_msg_t *msg, int input_len);
static int Remote_rssd_conn_change ();
static char *Get_rssd_hosts (Ntf_msg_t *src_msg);
void Disconnect_remote_host (unsigned int ipadd);

/*************************************************************
			
    Description: This function implements the local server 
		of the LB notification services. This function 
		must be registered as a messaging callback 
		function of the messaging server.

    Input:	msg - the incoming message;
		msg_len - message length; 0 indicates no message
			(write available).
		sock - socket fd;
		host - IP address of the sending host (NBO).

**************************************************************/

void EN_ntf_server (char *msg, int msg_len, 
				int sock, unsigned int host)
{
    static int cnt = 0;
    From_lb_write_t *lbwntf;
    Ntf_msg_t *ntf;
    int plen, ret;
    time_t tm;

    if (cnt == 0)		/* first call; happens in foreground. */
	Init_rhosts ();		/* initialize the remote host table */
    cnt++;

    tm = MISC_systime (NULL);
    if (No_resp_time > 0 && msg_len >= (int)sizeof (Ntf_msg_t) && 
					host != RMT_LOCAL_HOST) {
	int i;
	for (i = 0; i < N_rhosts; i++) {
	    if (Rhosts[i].sock == sock) {
		Rhosts[i].t_recv = tm;
		break;
	    }
	}
    }

    { 		/* house keeping */
	static time_t prev_tm = 0, prev_cc_tm = 0;
	if (tm >= prev_tm + 2) {	/* every two second we do */
	    if (Stop_local_read_time)
		Resume_local_read ();
	    Manage_saved_AN (MSAN_DELETE, 0, NULL);
	    prev_tm = tm;
	}
	if (tm >= prev_cc_tm + 1) {	/* every second we do */
	    Check_rsv_connection (tm);
	    prev_cc_tm = tm;
	}
    }

    if (msg_len == RMT_MSG_CONFIG_UPDATE) {
	Init_rhosts ();
	return;
    }

    if (Lost_ntf)
	Resend_lost_ntfs ();

    if (msg_len == RMT_MSG_CONN_LOST) {	/* connection lost */
	int i;
	char buf[128];
	for (i = 0; i < N_rhosts; i++) {
	    if (Rhosts[i].sock >= 0 && sock == Rhosts[i].sock) {
		MISC_log ("lost rssd connection to %s", 
				NET_string_IP (Rhosts[i].ip, 1, buf));
		break;
	    }
	}
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
    if (msg_len == RMT_MSG_TIMER)		/* timer call */
	return;

    if (msg_len == RMT_MSG_WRITE_READY) {
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
	    Process_sv_ack (EN_T_BSWAP (ntf->evtid),
					EN_T_BSWAP (ntf->lost_cnt));
	    return;

	case FROM_SV_CONN:
	    Process_conn ((Ntf_msg_t *)msg, sock, host, msg_len);
	    return;

	case KEEP_ALIVE_TEST:
	    return;

	default:
	    MISC_log ("unexpected NTF msg received (type %d, len %d)", 
					ntf->msg_type, msg_len);
	    return;
    }

    /* process each UN and WU in the message */
    lbwntf = (From_lb_write_t *)msg;
    plen = sizeof (From_lb_write_t);	/* length processed or in processing */
    while (plen <= msg_len) {

	if (lbwntf->msg_type == FROM_LB_WRITE_UN) {
	    lbwntf->pid = EN_T_BSWAP (lbwntf->pid);
	    lbwntf->fd = EN_T_BSWAP (lbwntf->fd);
	    lbwntf->msgid = EN_T_BSWAP (lbwntf->msgid);
	    lbwntf->lbmsgid = EN_T_BSWAP (lbwntf->lbmsgid);
	    lbwntf->msg_len = EN_T_BSWAP (lbwntf->msg_len);
	    Process_UN_msg (lbwntf);
	}

	plen += sizeof (From_lb_write_t);
	lbwntf++;
    }

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
    EN_id_t msgid;
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
	if (cl_reg->spec.un.a_code < 0) {		/* no alias */
	    if (cl_reg->pid == pid &&
		cl_reg->code == fd)
		matched = 1;
	}
	else {
	    if (cl_reg->spec.un.a_pid == pid &&
		cl_reg->spec.un.a_code == fd)
		matched = 1;
	}
	if (matched) {			/* send a msg to the client */
	    Ntf_msg_t msg;
	    int ret, sock;

	    msg.msg_type = TO_CLIENT_UN;
	    msg.code = cl_reg->code;
	    msg.evtid = msgid;
	    msg.lbmsgid = ntf->lbmsgid;
	    msg.msg_len = msg_len;
	    msg.lost_cnt = cl_reg->lost_cnt;
	    msg.sender_id = ntf->sender_id;
	    msg.reserved = 0;
	    sock = cl_reg->sock;

	    if (cl_reg->lost_cnt <= 0)
		ret = Send_msg_to_client (sock, (char *)&msg, 
				sizeof (Ntf_msg_t), PRIO_LOW, 
				cl_reg->pid, cl_reg->signal);
	    else
		ret = SEND_FAILED;

	    if (Ntfrr_ind != INDEX_DISABLED)
		Ntf_report ("send UN: sock %d, fd %d, msgid %x, ret %d\n", 
				sock, msg.code, msgid, ret);

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

	if (cl_reg->code != EN_AN_CODE) {		/* UN */
	    msg = &tmp;
	    msg->msg_type = TO_CLIENT_UN;
	    msg->msg_len = cl_reg->spec.un.msg_len;
	    msg->lbmsgid = cl_reg->evtid;
	    size = sizeof (Ntf_msg_t);
	}
	else {			/* AN; The following modifies the saved AN  
				   message but it should be fine */
	    msg = Manage_saved_AN (MSAN_GET, cl_reg->evtid, NULL);
	    if (msg == NULL)			/* unexpected */
		continue;
	    msg->msg_type = TO_CLIENT_AN;
	    size = sizeof (Ntf_msg_t) + msg->msg_len;
	}
	msg->code = cl_reg->code;
	msg->evtid = cl_reg->evtid;
	msg->lost_cnt = cl_reg->lost_cnt - 1;
	msg->sender_id = 0;
	msg->reserved = 0;

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
    int pid, code, signal;
    EN_id_t evtid;
    Client_reg_t *cl_reg;
    int next_ind;

    cl_reg = NULL;		/* not necessary - turn off gcc warning */
    if (msg->msg_type == FROM_SV_REG) {
	pid = EN_T_BSWAP (msg->pid);
	code = EN_T_BSWAP (msg->code);
	evtid = EN_T_BSWAP (msg->evtid);
	signal = EN_T_BSWAP (msg->signal);
    }
    else {
	pid = msg->pid;
	code = msg->code;
	evtid = msg->evtid;
	signal = msg->signal;
    }

    if (Ntfrr_ind != INDEX_DISABLED) {
	if (code == EN_AN_CODE)
	    Ntf_report ("AN reg recvd: pid %d, ID %x, signal %d\n", 
						pid, evtid, signal);
	else
	    Ntf_report ("UN reg recvd: pid %d, code %d, evtid %x, signal %d\n", 
						pid, code, evtid, signal);
    }

    /* deregistration and check duplicated entry */
    next_ind = Get_client_table_ind (evtid);
    while (next_ind != INDEX_INVALID) {
	cl_reg = Client_reg + next_ind;
	if (sock == cl_reg->sock && code == cl_reg->code) {
	    if (signal == -1) {
		if (code == EN_AN_CODE && cl_reg->pid >= 0) /* local AN */
		    Send_dereg_to_svs (evtid);	/* dereg AN on remote sv's */
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
	cl_reg = Get_new_client_entry (evtid);
    else {		/* too many clients */
	MISC_log ("NTF client reg table overflow");
	Send_ack_msg (cl_reg, TO_CLIENT_ACK_FAILED);
	return;
    }
    cl_reg->sock = sock;
    cl_reg->signal = signal;
    cl_reg->pid = pid;
    cl_reg->code = code;
    cl_reg->evtid = evtid;
    cl_reg->lost_cnt = 0;
    cl_reg->spec.un.msg_len = 0;

    An_lb_any_ind = INDEX_UNKNOWN;
    if (Ntfrr_ind != INDEX_DISABLED)
	Ntfrr_ind = INDEX_UNKNOWN;
    if (code == EN_AN_CODE) {			/* AN reg */
	if (msg->msg_type == FROM_CLIENT_REG) {	/* local AN reg */
	    if (evtid == EN_REPORT) {
		cl_reg->spec.an.reg_mask = 0;
		Ntfrr_ind = INDEX_UNKNOWN;
		Send_ack_msg (cl_reg, TO_CLIENT_ACK_DONE);
	    }
	    else if (evtid == EN_QUERY_HOSTS) {
		Ntf_msg_t msg;
		cl_reg->spec.an.reg_mask = 0;
		Send_ack_msg (cl_reg, TO_CLIENT_ACK_DONE);
#ifdef USE_MEMORY_CHECKER
		memset (&msg, 0, sizeof (Ntf_msg_t));
#endif
		msg.evtid = EN_QUERY_HOSTS;
		msg.msg_type = FROM_CLIENT_AN;
		msg.msg_len = AN_SELF_ONLY;
		msg.sender_id = EN_SHORT_BSWAP (pid);
		Process_AN_msg (&msg, sizeof (Ntf_msg_t));
	    }
	    else {
		cl_reg->spec.an.reg_mask = 0xffffffff;	/* disable this reg */
		/* see if this is already registered by another local client */
		if (!Is_AN_registered (cl_reg)) {
		    cl_reg->signal = -2;		/* remote regist */
		    Send_AN_regist_to_svs (cl_reg);/* send to remote sv's */
		    cl_reg->signal = signal;		/* recover signal */
		}
		if (cl_reg->spec.an.reg_mask == 0)	/* done */
		    Send_ack_msg (cl_reg, TO_CLIENT_ACK_DONE);
	    }
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
	cl_reg->spec.un.a_code = msg->a_code;
	Send_ack_msg (cl_reg, TO_CLIENT_ACK_DONE);
    }
    return;
}

/****************************************************************
						
    Description: Gets a new client registration entry for "evtid".

    Input:	evtid - UN message ID or AN ID;

    Return:	The pointer to the new entry.

****************************************************************/

static Client_reg_t *Get_new_client_entry (EN_id_t evtid)
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

    id_ind = Search_index_table (evtid);	/* search evtid */
    while ((new_reg = (Client_reg_t *)	/* get new entry in client table */
		MISC_table_new_entry (Cl_tblid, &new_cl_ind)) == NULL)
	msleep (WAITING_FOR_MEMORY_TIME);

    if (id_ind < 0) {			/* evtid not found */
	int i;
	while (MISC_table_new_entry (Ind_tblid, &id_ind) == NULL)		
					/* get new entry in index table */
	    msleep (WAITING_FOR_MEMORY_TIME);
	for (i = 0; i < N_indices - 1; i++) {	/* sort the table */
	    if (Client_reg[Indices[i]].evtid > evtid) {
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
    new_reg->evtid = evtid;
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
    else {			/* the first client for this evtid */
	id_ind = Search_index_table (reg->evtid); /* search evtid */
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
	else {			/* the first client for this evtid */
	    id_ind = Search_index_table (reg->evtid);	/* search */
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
		the "evtid" entry in the NTF index table.

    Input:	evtid - the event id to search for;

    Output:	The table index found on success of -1 on failure.

**************************************************************/

static int Search_index_table (EN_id_t evtid)
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
	    if (Client_reg[Indices[st]].evtid == evtid)
		return (st);
	    else if (Client_reg[Indices[end]].evtid == evtid)
		return (end);
	    else
		return (-1);
	}
	if (evtid <= Client_reg[Indices[ind]].evtid)
	    end = ind;
	else
	    st = ind;
    }
    return (-1);
}

/*************************************************************
			
    Description: returns the client table index for evtid.

    Input:	evtid - the event id to search for;

    Output:	The client table index found on success of 
		INDEX_INVALID on failure.

**************************************************************/

static int Get_client_table_ind (EN_id_t evtid)
{
    int i;

    if ((i = Search_index_table (evtid)) >= 0)
	return (Indices[i]);
    else
	return (INDEX_INVALID);
}

/********************************************************************
			
    Description: This function checks if any local client still 
		registered for AN "evtid". If not, a deregistration
		msg is sent to all active remote servers.

    Input:	evtid - AN event id;

********************************************************************/

static void Send_dereg_to_svs (EN_id_t evtid)
{
    int next_ind, cnt;
    Client_reg_t reg;

    if (evtid > EN_MAX_ID)
	return;

    cnt = 0;
    next_ind = Get_client_table_ind (evtid);
    while (next_ind != INDEX_INVALID) {
	Client_reg_t *cl_reg;

	cl_reg = Client_reg + next_ind;
	next_ind = cl_reg->next;
	if (cl_reg->code == EN_AN_CODE && cl_reg->pid >= 0)
	   cnt++;
	if (cnt >= 2)		/* this AN is still used by a local client */
	    return;
    }

    reg.code = EN_AN_CODE;
    reg.evtid = evtid;
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

#ifdef USE_MEMORY_CHECKER
    memset (&msg, 0, sizeof (Ntf_msg_t));
#endif
    msg.msg_type = TO_CLIENT_ACK;
    msg.code = reg->code;
    msg.evtid = reg->evtid;
    msg.msg_len = status;
    msg.sender_id = reg->spec.an.n_rhosts;

    ret = Send_msg_to_client (reg->sock, (char *)&msg, 
		sizeof (Ntf_msg_t), PRIO_HIGH, reg->pid, reg->signal);
    if (Ntfrr_ind != INDEX_DISABLED) {
	if (reg->code == EN_AN_CODE)
	    Ntf_report ("AN ack sent: status %d, ID %x, ret %d\n", 
						status, reg->evtid, ret);
	else
	    Ntf_report ("UN ack sent: status %d, code %d, evtid %x, ret %d\n", 
					status, reg->code, reg->evtid, ret);
    }

    if (ret != SEND_DONE && ret != SEND_QUEUED) {
	MISC_log ("Send_ack_msg failed (to pid %d)", reg->pid);
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
/*	    MISC_log ("init sock %d closed", sock); */
	    Rhosts[i].init_sock = -1;
	}
	if (Rhosts[i].sock >= 0 && sock == Rhosts[i].sock) {
	    host_ind = i;
	    Rhosts[i].sock = -1;
	    Rhosts[i].failed = 0;
/*	    MISC_log ("Deregister_client %x", Rhosts[i].ip); */
	    Remote_rssd_conn_change ();
	    break;
	}
    }

    Manage_scd (SCD_FREE, sock);
    for (i = N_client_regs - 1; i >= 0; i--) {
	Client_reg_t *reg;

	reg = Client_reg + i;
	if (reg->code == EN_AN_CODE && host_ind >= 0)
	    Process_sv_ack (reg->evtid, host_ind);

	if (reg->sock == sock) {
	    if (reg->code == EN_AN_CODE && reg->pid >= 0) /* local AN */
		Send_dereg_to_svs (reg->evtid);	/* dereg AN on remote sv's */
	    Delete_client_entry (i);		/* rm the entry */
	}
    }
    if (need_close)
	RMT_close_msg_client (sock);
    return;
}

/********************************************************************
			
    Description: This function sends a message to the client.
		This function tries to continue to send any data left
		over from previous messages that could not be sent 
		due to socket buffer full. Two types 
		of messages, high priority and low priority, are 
		processed. The high priority message is saved in
		the SCD data buffer if the socket is full. The low
		priority message is saved only if it is already
		partially sent. Otherwise SEND_FAILED is returned.
		The data buffer is reallocated whenever needed.
		This function requests write polling in case of any 
		left over data. A signal is send to the client when 
		message sending is completed and "signal" >= 0. NTF 
		registration ack msgs do not need signal because
		EN register will poll for it.

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
	msg = RMT_pack_msg ((char *)msg, msg_len, MAX_MSG_SIZE, &plen);
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
	    RMT_poll_write (sock);
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

    if (No_resp_time > 0 && pid < 0) {		/* set t_sent */
	int i;
	for (i = 0; i < N_rhosts; i++) {
	    if (Rhosts[i].sock == sock) {
		Rhosts[i].t_sent = MISC_systime (NULL);
		break;
	    }
	}
    }

    if (ret != msg_len) {
	scd = Manage_scd (SCD_SAVE, sock);
	scd->sock = sock;
	scd->n_total = 0;
	Append_to_scd_buffer (scd, msg, msg_len);
	scd->n_written = ret;
	scd->pid = pid;
	scd->signal = signal;
	RMT_poll_write (sock);
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
	Scd_t *new_ent;

	while (tid == NULL) {			/* create the table */
	    tid = MISC_open_table (sizeof (Scd_t), 
				8, 0, &n_scds, (char **)&scds);
	    if (tid == NULL)
		msleep (WAITING_FOR_MEMORY_TIME);
	}

	while ((new_ent = (Scd_t *)MISC_table_new_entry (tid, NULL)) == NULL)
	    msleep (WAITING_FOR_MEMORY_TIME);
	new_ent->buf_size = 0;
	new_ent->data = NULL;
	return (new_ent);
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
	while ((cpt = (char *)malloc (new_size)) == NULL)
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

#ifdef USE_MEMORY_CHECKER
    memset (&msg, 0, sizeof (Ntf_msg_t));
#endif
    msg.msg_type = TO_SV_ACK;
    msg.code = reg_msg->code;
    msg.evtid = reg_msg->evtid;
    msg.lost_cnt = reg_msg->a_pid;	/* echo back the host index */
    msg.msg_len = EN_T_BSWAP (status);

    ret = Send_msg_to_client (sock, (char *)&msg, 
		sizeof (Ntf_msg_t), PRIO_HIGH, -1, -1);

    if (Ntfrr_ind != INDEX_DISABLED)
	Ntf_report (
		"Send ack msg to sv: sock %d, evtid %x, status %d, ret %d\n", 
				sock, EN_T_BSWAP (msg.evtid), status, ret);

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

    Return:	non-zero if already registered or registration in progress. 
		Otherwise 0.

********************************************************************/

static int Is_AN_registered (Client_reg_t *new_reg)
{
    int next_ind;

    next_ind = Get_client_table_ind (new_reg->evtid);
    while (next_ind != INDEX_INVALID) {
	Client_reg_t *reg;

	reg = Client_reg + next_ind;
	next_ind = reg->next;
	if (reg->code == EN_AN_CODE && reg->pid >= 0 && reg != new_reg) {
								/* found */
	    new_reg->spec.an.reg_mask = reg->spec.an.reg_mask;
	    new_reg->spec.an.n_rhosts = reg->spec.an.n_rhosts;
	    return (1);
	}
    }
    return (0);
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
	    rreg.signal = EN_T_BSWAP (new_reg->signal);
	    rreg.pid = EN_T_BSWAP (-1);
	    rreg.code = EN_T_BSWAP (new_reg->code);
	    rreg.evtid = EN_T_BSWAP (new_reg->evtid);
	    rreg.a_pid = EN_T_BSWAP (i);
	    rreg.a_code = 0;

	    ret = Send_msg_to_client (Rhosts[i].sock, (char *)&rreg, 
		sizeof (Ntf_regist_msg_t), PRIO_HIGH, -1, -1);

	    if (Ntfrr_ind != INDEX_DISABLED)
		Ntf_report (
			"Send AN regist to sv: sock %d, evtid %x, ret %d\n", 
				Rhosts[i].sock, new_reg->evtid, ret);

	    if (ret == SEND_DONE || ret == SEND_QUEUED) {
		mask |= (1 << i);
		cnt++;
	    }
	    else {
		char buf[64];
		MISC_log ("Send_AN_regist_to_svs (%s) failed", 
				NET_string_IP (Rhosts[i].ip, 1, buf));
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

    Input:	evtid - the ACK AN ID;
		host_ind - Rhosts index of the ACK.

********************************************************************/

static void Process_sv_ack (EN_id_t evtid, int host_ind)
{
    unsigned int host_mask;

    host_mask = ~(1 << host_ind);
    while (1) {		/* repeat the procedure in case of deregistration */
	int next_ind, done;

	done = 1;
	next_ind = Get_client_table_ind (evtid);
	while (next_ind != INDEX_INVALID) {
	    Client_reg_t *reg;
	    unsigned int omask, nmask;

	    reg = Client_reg + next_ind;
	    next_ind = reg->next;
	    if (reg->code == EN_AN_CODE) {
		omask = reg->spec.an.reg_mask;
		nmask = omask & host_mask;
		if (nmask != omask) {
		    if (Ntfrr_ind != INDEX_DISABLED)
			Ntf_report (
			    "remote AN reg ACK recvd: ID %x, host ind %d\n", 
						evtid, host_ind);
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
		either a remote server or a local EN_post. 
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
    EN_id_t event_id;
    int msg_len, rmt_an, next_ind;
    Client_reg_t *cl_reg;
    int n_sock_sent, sock_sent[MAX_RMT_HOSTS];
    int self_only;		/* send this AN to process itself only */
    int not_to_self;		/* not to send this AN to process itself */
    int sender_id;

    not_to_self = 0;		/* not necessary - turn off gcc warning */
    rmt_an = 0;
    if (msg->msg_type == FROM_SV_AN) {
	Bswap_AN_msg_header (msg);
	rmt_an = 1; /* we need this bacause msg->msg_type will change */
    }
    else if (msg->evtid > (int)(EN_MAX_ID)) {
	if (msg->evtid == (int)(EN_REP_REG_TABLE)) {
	    Report_reg_table ();
	    return;
	}
	if (msg->evtid == (int)(EN_DISC_HOST)) {
	    Cmd_disconnect_host (msg, input_len);
	    return;
	}
	if (msg->evtid == (int)(EN_QUERY_HOSTS)) {
	    unsigned int self = msg->msg_len & AN_SELF_ONLY;
	    if ((msg = (Ntf_msg_t *)Get_rssd_hosts (msg)) == NULL)
		return;
	    input_len = (msg->msg_len & AN_MSG_LEN_MASK) + sizeof (Ntf_msg_t);
	    msg->msg_len |= self;
	}
    }

    event_id = msg->evtid;
    msg_len = msg->msg_len;
    self_only = not_to_self = 0;
    if (msg_len & AN_SELF_ONLY)
	self_only = 1;
    if (msg_len & AN_NOT_TO_SELF)
	not_to_self = 1;
    sender_id = 0;		/* not necessary - turn off gcc warning */
    if (self_only || not_to_self) {
	sender_id = EN_SHORT_BSWAP (msg->sender_id);
	msg_len &= AN_MSG_LEN_MASK;
	msg->msg_len = msg_len;
    }

    if (Ntfrr_ind != INDEX_DISABLED)
	Ntf_report ("AN recvd: ID %x, len %d, remote %d\n", 
					event_id, msg_len, rmt_an);

    if (input_len != (int)sizeof (Ntf_msg_t) + msg_len) {
	MISC_log ("unexpected AN msg length");
	return;
    }

    n_sock_sent = 0;
    next_ind = Get_client_table_ind (event_id);
    while (next_ind != INDEX_INVALID) {

	cl_reg = Client_reg + next_ind;
	next_ind = cl_reg->next;
	if (cl_reg->code != EN_AN_CODE || 		/* not an AN */
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
	    if (n_sock_sent >= MAX_RMT_HOSTS) 
		MISC_log ("Unexpectedly too many hosts");
	    else {		/* record rmt hosts that has been sent */
		sock_sent[n_sock_sent] = cl_reg->sock;
		n_sock_sent++;
	    }
	}
    }

    if (An_lb_any_ind == INDEX_UNKNOWN) {
	An_lb_any_ind = Search_index_table (EN_ANY);
	if (An_lb_any_ind < 0)
	    An_lb_any_ind = INDEX_DISABLED;
    }
    if (An_lb_any_ind == INDEX_DISABLED)
	return;

    next_ind = Indices[An_lb_any_ind];
    while (next_ind != INDEX_INVALID) {		/* process EN_ANY */

	cl_reg = Client_reg + next_ind;
	next_ind = cl_reg->next;
	if (cl_reg->code != EN_AN_CODE || 		/* not an AN */
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
	    for (k = 0; k < n_sock_sent; k++)
		if (cl_reg->sock == sock_sent[k])
		    break;
	    if (k >= n_sock_sent)		/* not yet sent */
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
				cl_reg->sock, cl_reg->evtid, ret);

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

    msg->code = EN_T_BSWAP (msg->code);
    msg->evtid = EN_T_BSWAP (msg->evtid);
    msg->msg_len = EN_T_BSWAP (msg->msg_len);
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

static Ntf_msg_t *Manage_saved_AN (int func, EN_id_t event, Ntf_msg_t *msg)
{
    typedef struct {
	EN_id_t event_id;
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
	EN_id_t event_id;

	event_id = msg->evtid;

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
	    while ((s->msg = (Ntf_msg_t *)malloc (size)) == NULL)
		msleep (WAITING_FOR_MEMORY_TIME);

	    s->buf_size = size;
	}
	memcpy ((char *)s->msg, (char *)msg, size);
	s->event_id = event_id;

	return (s->msg);
    }

    if (func == MSAN_DELETE) {		/* house keeping - delete msgs */
	time_t tm;

	tm = MISC_systime (NULL);
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
		if (reg->code == EN_AN_CODE && reg->lost_cnt > 0) {
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
	Stop_local_read_time = MISC_systime (NULL);
	RMT_local_read (RMT_LOCAL_READ_PAUSE);
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
    RMT_local_read (RMT_LOCAL_READ_RESUME);
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
    unsigned int ip, l_ip;
    int lhind, cnt, i, new_n_rh, k;
    Remote_hosts *new_hs;

    new_n_rh = 0;
    while (RMT_lookup_host_index (RMT_LHI_IX2I, &ip, new_n_rh + 1) > 0)
	new_n_rh++;
    if ((lhind = RMT_lookup_host_index (RMT_LHI_IX2I, &l_ip, 0)) <= 0) {
	MISC_log ("en_server: Local host not found\n");
	exit (1);
    }
    new_n_rh--;
    new_hs = (Remote_hosts *)MISC_malloc (new_n_rh * sizeof (Remote_hosts));

    cnt = 0;
    for (i = 0; i <= new_n_rh; i++) {

	if (i + 1 == lhind)
	    continue;
	RMT_lookup_host_index (RMT_LHI_IX2I, &ip, i + 1);
	new_hs[cnt].ip = ip;
	new_hs[cnt].sock = -1;
	new_hs[cnt].init_sock = -1;
	new_hs[cnt].init_timer = 0;
	new_hs[cnt].failed = 0;
	new_hs[cnt].conn_retry = 0;
	new_hs[cnt].cmd_disc = 0;
	new_hs[cnt].t_sent = MISC_systime (NULL);
	new_hs[cnt].t_recv = new_hs[cnt].t_sent;
	cnt++;
    }

    for (i = 0; i < N_rhosts; i++) {
	char buf[128];
	unsigned int tip = Rhosts[i].ip;
	for (k = 0; k < new_n_rh; k++)
	    if (new_hs[k].ip == tip)
		break;
	if (k >= new_n_rh) {	/* an existing rhost is removed */
	    if (Rhosts[i].sock >= 0)
		MISC_log ("%s no longer in conf - disconnected", 
					NET_string_IP (tip, 1, buf));
	    Disconnect_remote_host (EN_T_BSWAP (tip));
	}
    }

    for (i = 0; i < new_n_rh; i++) {
	unsigned int tip = new_hs[i].ip;
	for (k = 0; k < N_rhosts; k++) {
	    if (Rhosts[k].ip == tip)
		break;
	}
	if (k < N_rhosts)	/* existing rhost still in config */
	    memcpy (new_hs + i, Rhosts + k, sizeof (Remote_hosts));
	else {
	    int t = 0;
	    if (Rhosts == NULL)	/* initially try to connect in fg */
		t = 20;
	    Send_conn_init_to_rhost (new_hs + i, t, "init");
	}
    }

    if (Rhosts != NULL)
	MISC_free (Rhosts);
    Rhosts = new_hs;
    N_rhosts = new_n_rh;

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

    Input:	msg - the remote conn message;
		sock - conn socket.
		host - remote host IP address (NBO).
		msg_len - message length.

********************************************************************/

static void Process_conn (Ntf_msg_t *msg, int sock, 
				unsigned int host, int msg_len)
{
    Remote_hosts *rh;
    int i, code, lh_ind;
    char buf[128], b[64];

    code = EN_T_BSWAP (msg->code);

    if (code == SV_CONN_REGS) {
	Receive_AN_reg_info (msg, sock, msg_len);
/*	MISC_log ("AN reg info recv from %x", host); */
	return;
    }

    rh = NULL;			/* not useful - turn off gcc warning */
    for (i = 0; i < N_rhosts; i++) {
	rh = Rhosts + i;
	if (rh->ip == host)
	    break;
    }
    if (i >= N_rhosts) {	/* not found */
	MISC_log ("connecting host (%s) not found in rhost conf", 
					NET_string_IP (host, 1, b));
	return;
    }

    lh_ind = RMT_lookup_host_index (RMT_LHI_LIX, NULL, 0) - 1;
    if (code == SV_CONN_INIT) {		/* conn init msg */
	if (i >= lh_ind) {
	    /* send a conn resp message back */
	    msg->code = EN_T_BSWAP (SV_CONN_RESP);
	    msg->evtid = 0;
	    Send_msg_to_client (sock, (char *)msg, 
			sizeof (Ntf_msg_t), PRIO_HIGH, -1, -1);
	    sprintf (buf, "Sending AN reg to %s - connected - init", 
				NET_string_IP (host, 1, b));
	    Accept_connection (rh, sock, buf);
	}
	else {
	    char b[128];
	    int ret;
	    RMT_close_msg_client (sock);
	    if (rh->init_sock < 0) {
		if (rh->sock >= 0) {
		    MISC_log ("Unusable init conn request recvd (from %s)",
					NET_string_IP (rh->ip, 1, b));
		    Deregister_client (rh->sock, 1);
		}
		ret = Send_conn_init_to_rhost (rh, 2, "counter");
		if (ret < 0)
		    MISC_log ("Counter conn to %s failed (%d)\n", 
					NET_string_IP (rh->ip, 1, b), ret);
	    }
	}
    }
    else {				/* conn resp msg */
	sprintf (buf, "Sending AN reg to %s - connected - resp",
					NET_string_IP (host, 1, b));
	Accept_connection (rh, sock, buf);
    }

    return;
}

/********************************************************************
			
    Accepts a remote server connection.

    Input:	rh - the remote server.
		sock - the connection sock.
		tm - current time.

********************************************************************/

static void Accept_connection (Remote_hosts *rh, int sock, char *report) {
    char buf[64];

    if (rh->sock >= 0) {
/*	MISC_log ("New init msg received"); */
	Deregister_client (rh->sock, 1);
    }
    rh->sock = sock;	/* accept connection */
    if (rh->init_sock == sock)
	rh->init_sock = -1;
    Remote_rssd_conn_change ();

    MISC_log ("Connection to %s accepted (%s)\n",
				NET_string_IP (rh->ip, 1, buf), report);
    rh->conn_retry = 0;
    if (No_resp_time > 0)
	rh->t_recv = MISC_systime (NULL);
    Send_AN_reg_info (sock);
}

/********************************************************************
			
    This function sends a conn init msg to a remote server. It opens a 
    socket to send the message. It closes if sending fails.

    Input:	rh - the remote server.
		block - blocking until success.
		reason - reason this is called.

    Return:	The init sock on success, RMT_TIMED_OUT on 
		timed-out or other negative error number on failure.

********************************************************************/

static int Send_conn_init_to_rhost (Remote_hosts *rh, int block, char *reason)
{
    Ntf_msg_t msg;
    int ret, sock, cnt;
    char buf[64];

    if (rh->init_sock >= 0) {
	MISC_log ("Unclosed init_sock");
	return (-1);
    }

#ifdef USE_MEMORY_CHECKER
    memset (&msg, 0, sizeof (Ntf_msg_t));
#endif
    msg.msg_type = FROM_SV_CONN;
    msg.code = EN_T_BSWAP (SV_CONN_INIT);
    msg.evtid = 0;

    cnt = 0;

    while ((sock = RMT_connect_host ((unsigned int)rh->ip)) < 0) {
	if (sock == RMT_TIMED_OUT) {
	    if (block == 0)
		return (sock);
	    if (cnt >= block) {		/* wait about .2 * block seconds */
		MISC_log ("Connecting %s timed out", 
				NET_string_IP (rh->ip, 1, buf));
		rh->conn_retry = 1;
	    }
	    else {
		msleep (200);
		cnt++;
		continue;
	    }
	}
	else if (block > 0)
	    rh->conn_retry = 1;
	return (sock);
    }
    ret = Send_msg_to_client (sock, (char *)&msg, 
			    sizeof (Ntf_msg_t), PRIO_HIGH, -1, -1);
    if (ret != SEND_DONE && ret != SEND_QUEUED) {
	MISC_log ("Send init to (%s) failed", NET_string_IP (rh->ip, 1, buf));
	RMT_close_msg_client (sock);
	return (-1);
    }
    MISC_log ("Init sent to %s (%s)\n", 
				NET_string_IP (rh->ip, 1, buf), reason);
    rh->init_sock = sock;
    rh->init_timer = RECONN_PERIOD + 2;
    rh->conn_retry = 0;
    return (sock);
}

/*************************************************************
			
    Periodically checks connections to remote servers and 
    sends conn init msg to disconnected remote servers.

    Input:	tm - current time.

**************************************************************/

static void Check_rsv_connection (time_t tm) {
    static time_t pst_tm = 0;		/* current period start time */
    static time_t cst_tm = 0;		/* current conn start time */
    static int check_cnt = 0;		/* check count in the current period */
    static int rh_ind = 0;		/* current rhost index */
    int i;

    if (pst_tm == 0) {
	pst_tm = tm;
	check_cnt = N_rhosts;		/* disable the first period */
	return;
    }

    if (No_resp_time > 0) {
	for (i = 0; i < N_rhosts; i++) {
	    Remote_hosts *rh;
	    rh = Rhosts + i;
	    if (rh->sock < 0)
		continue;
	    if (tm >= rh->t_sent + (No_resp_time >> 1)) { /* send keep alive */
		Ntf_msg_t msg;
		memset (&msg, 0, sizeof (Ntf_msg_t));
		msg.msg_type = KEEP_ALIVE_TEST;
		Send_msg_to_client (rh->sock, (char *)&msg, sizeof (Ntf_msg_t),
						PRIO_HIGH, -1, -1);
	    }
	    if (tm > rh->t_recv + ((3 * No_resp_time) >> 1)) {/* check alive */
		char buf[64];
		unsigned int ip = rh->ip;
		MISC_log ("No-resp disc: host %s", NET_string_IP (ip, 1, buf));
		Disconnect_remote_host (EN_T_BSWAP (ip));
	    }
else {		/* test code - to be removed later */
    static int pr_df = 0;
    int df = tm - rh->t_recv;
    if (df >= No_resp_time) {
	if (df > pr_df) {
	    MISC_log ("    Slow response: %d seconds\n", df);
	    pr_df = df;
	}
    }
    else {
	if (pr_df > 0)
	    MISC_log ("    Slow response ends\n");
	pr_df = 0;
    }
}
	}
    }

    if (tm >= pst_tm + RECONN_PERIOD) {	/* start a new period */
	check_cnt = 0;
	pst_tm = tm;
	for (i = 0; i < N_rhosts; i++) {
	    Remote_hosts *rh;
	    rh = Rhosts + i;
	    if (rh->init_sock >= 0) {
		rh->init_timer -= RECONN_PERIOD;
		if (rh->init_timer <= 0) {
		    RMT_close_msg_client (rh->init_sock);
		    rh->init_sock = -1;
		}
	    }
	}
    }

    while (check_cnt < N_rhosts) {
	Remote_hosts *rh;
	rh = Rhosts + rh_ind;
	if (rh->sock < 0 && rh->init_sock < 0 &&
	    (cst_tm == 0 || (cst_tm > 0 && tm < cst_tm + CONN_WAIT_TIME))) {
	    int ret;
	    if (rh->conn_retry)
		MISC_log_disable (1);
	    ret = Send_conn_init_to_rhost (rh, 0, "reconn");
	    MISC_log_disable (0);
	    if (ret == RMT_TIMED_OUT) {
		char buf[64];
		if (!rh->conn_retry)
		    MISC_log ("Connecting to %s timed out - will retry\n", 
					NET_string_IP (rh->ip, 1, buf));
		rh->conn_retry = 1;
		if (cst_tm == 0)
		    cst_tm = tm;
		return;
	    }
	    if (ret == RMT_CONNECT_FAILED)
		rh->conn_retry = 1;
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
    unsigned int *id, cr_evtid;
    Ntf_msg_t *msg;
    int cnt, i;

    while ((msg = (Ntf_msg_t *)malloc (sizeof (Ntf_msg_t) + 
			N_client_regs * sizeof (unsigned int))) == NULL)
	msleep (WAITING_FOR_MEMORY_TIME);

    cnt = 0;
    cr_evtid = 0xffffffff;
    id = (unsigned int *)((char *)msg + sizeof (Ntf_msg_t));
    for (i = 0; i < N_indices; i++) {
	Client_reg_t *cl_reg;

	cl_reg = Client_reg + Indices[i];
	while (1) {

	    if (cl_reg->code == EN_AN_CODE && cl_reg->evtid != cr_evtid
		&& cl_reg->evtid <= EN_MAX_ID && cl_reg->pid >= 0) {
		id[cnt] = EN_T_BSWAP (cl_reg->evtid);
		cr_evtid = cl_reg->evtid;
		cnt++;
	    }
	    if (cl_reg->next == INDEX_INVALID)
		break;
	    cl_reg = Client_reg + cl_reg->next;
	}
    }
    if (cnt > 0) {
	memset (msg, 0, sizeof (Ntf_msg_t));
	msg->msg_type = FROM_SV_CONN;
	msg->msg_len = EN_T_BSWAP (cnt);
	msg->code = EN_T_BSWAP (SV_CONN_REGS);
	Send_msg_to_client (sock, (char *)msg, 
		sizeof (Ntf_msg_t) + cnt * sizeof (unsigned int), 
		PRIO_HIGH, -1, -1);
    }
/*
    else
	MISC_log ("Empty AN reg info - not sent");
*/

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

    cnt = EN_T_BSWAP (msg->msg_len);
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
	id = EN_T_BSWAP (idp[i]);
	cl_reg = Get_new_client_entry (id);
	cl_reg->sock = sock;
	cl_reg->signal = -1;
	cl_reg->pid = -1;
	cl_reg->code = EN_AN_CODE;
	cl_reg->evtid = id;
	cl_reg->lost_cnt = 0;
	cl_reg->spec.an.reg_mask = 0;
    }
}

/****************************************************************
						
    Creates the EN_QUERY_HOSTS event which contains the IP addresses 
    of all remote hosts, and their connection status. Local host is 
    not included. Returns the msg on success or NULL if nobody 
    registers this message.

****************************************************************/

#define T_BUF_SIZE 256

static char *Get_rssd_hosts (Ntf_msg_t *src_msg) {
    static char *buffer = NULL;
    int len, i;
    char *p, *msg;
    Ntf_msg_t *hd;

    if (Search_index_table (EN_QUERY_HOSTS) < 0)
				/* nobody registered for this event */
	return (NULL);

    if (buffer == NULL)
	buffer = MISC_malloc (sizeof (Ntf_msg_t) + T_BUF_SIZE);

    msg = buffer + sizeof (Ntf_msg_t);
    p = msg;
    sprintf (p, "Remote_hosts: %d", N_rhosts);
    p += strlen (p);
    for (i = 0; i < N_rhosts; i++) {
	Remote_hosts *rh;
	unsigned int ui, connected;
	char buf[64];
	if ((p - msg) + 18 >= T_BUF_SIZE) {	/* truncated if too many */
	    MISC_log ("EN_QUERY_HOSTS result truncated\n");
	    break;
	}
	rh = Rhosts + i;
	ui = EN_T_BSWAP (rh->ip);
	connected = 1;
	if (rh->sock < 0)
	    connected = 0;
	sprintf (p, " %s %d", NET_string_IP (ui, 0, buf), connected);
	p += strlen (p);
    }
    len = p - msg + 1;
    hd = (Ntf_msg_t *)buffer;
    hd->msg_type = TO_CLIENT_AN;
    hd->code = EN_AN_CODE;
    hd->evtid = EN_QUERY_HOSTS;
    hd->msg_len = len;
    hd->lbmsgid = 0;
    hd->lost_cnt = 0;
    hd->sender_id = src_msg->sender_id;
    hd->reserved = 0;
    return (buffer);
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
	Ntfrr_ind = Search_index_table (EN_REPORT);
	if (Ntfrr_ind < 0)
	    Ntfrr_ind = INDEX_DISABLED;
    }

    if (Ntfrr_ind == INDEX_DISABLED)
	return;

    /* send the report as an AN message */
    cpt = (char *)msg + sizeof (Ntf_msg_t);
    tm = MISC_systime (NULL);
    sprintf (cpt, "%.2d:%.2d ", (int)((tm / 60) % 60), (int)(tm % 60));
    va_start (args, format);
    vsprintf (cpt + 6, format, args);
    va_end (args);
    len = strlen (cpt) + 1;
    hd = (Ntf_msg_t *)msg;
    hd->msg_type = TO_CLIENT_AN;
    hd->code = EN_AN_CODE;
    hd->evtid = EN_REPORT;
    hd->msg_len = len;
    hd->lbmsgid = 0;
    hd->lost_cnt = 0;
    hd->sender_id = 0;
    hd->reserved = 0;
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

	    sprintf (text, "AN sock %d, sig %d, pid %d, evtid %x", 
		cl_reg->sock, cl_reg->signal, cl_reg->pid, cl_reg->evtid);
	    if (cl_reg->lost_cnt > 0)
		sprintf (text + strlen (text), 
				", lost_cnt %d", cl_reg->lost_cnt);
	    if (cl_reg->code == EN_AN_CODE)
		sprintf (text + strlen (text), 
			", mask %x", cl_reg->spec.an.reg_mask);
	    else {
		text[0] = 'U';
		sprintf (text + strlen (text), ", code %d, len %d", 
				cl_reg->code, cl_reg->spec.un.msg_len);
		if (cl_reg->spec.un.a_code >= 0) {
		    sprintf (text + strlen (text), ", a_code %d, a_pid %d", 
				cl_reg->spec.un.a_code, cl_reg->spec.un.a_pid);
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
			
    Processes the EN_DISC_HOST event of "msg" of length 
    "input_len".

**************************************************************/

#include <rmt.h>
#define MAX_LOCAL_CLIENTS 128

static void Cmd_disconnect_host (Ntf_msg_t *msg, int input_len) {
    unsigned int *ipadd;
    char buf[64];

    if (input_len != sizeof (Ntf_msg_t) + sizeof (int)) {
	MISC_log ("unexpected EN_DISC_HOST msg length");
	return;
    }
    ipadd = (unsigned int *)((char *)msg + sizeof (Ntf_msg_t));

    MISC_log ("Commanded disc: host %s", NET_string_IP (*ipadd, 1, buf));
    Disconnect_remote_host (EN_T_BSWAP (*ipadd));
}

/*************************************************************
			
    Disconnets a remote host "ipadd" (in LBO).

**************************************************************/

void Disconnect_remote_host (unsigned int ipadd) {
    int i;

    RMT_kill_clients (ipadd);		/* kill RPC child servers */
    for (i = 0; i < N_rhosts; i++) {	/* remove AN services */
	if (Rhosts[i].ip == EN_T_BSWAP (ipadd)) {
	    if (Rhosts[i].sock >= 0)
		Deregister_client (Rhosts[i].sock, 1);
	    break;
	}
    }
}

/*************************************************************
			
    Sets No_resp_time.

**************************************************************/

void EN_disconnect_time (unsigned int seconds) {

    No_resp_time = seconds;
    if (No_resp_time != 0 && No_resp_time < 5)
	No_resp_time = 5;
}

/*************************************************************
			
    Routine called when the connection to a remote rssd is 
    established or lost. It sends a EN_QUERY_HOSTS event and
    updates the rssd_disc file.

**************************************************************/

#define MAX_DISC_IPS 32

static int Remote_rssd_conn_change () {
    unsigned int disc_ips[MAX_DISC_IPS];
    int n_disc_ips, i;
    Ntf_msg_t msg;
    char buf[MAX_DISC_IPS * 20], *p;

    /* post EN_QUERY_HOSTS */
    memset (&msg, 0, sizeof (Ntf_msg_t));
    msg.evtid = EN_QUERY_HOSTS;
    msg.msg_type = FROM_CLIENT_AN;
    msg.msg_len = 0;
    Process_AN_msg (&msg, sizeof (Ntf_msg_t));

    n_disc_ips = 0;
    for (i = 0; i < N_rhosts; i++) {
	Remote_hosts *rh;
	rh = Rhosts + i;
	if (rh->sock < 0 && n_disc_ips < MAX_DISC_IPS) {
	    disc_ips[n_disc_ips] = rh->ip;
	    n_disc_ips++;
	}
    }

    /* update rssd_disc */
    p = buf;
    for (i = 0; i < n_disc_ips; i++) {
	sprintf (p, " %x", disc_ips[i]);
	p += strlen (p);
    }
    if (n_disc_ips == 0)
	sprintf (p, " ");
    strcat (p, "\n");
    RMT_access_disc_file (1, buf, strlen (buf) + 1);

    return (0);
}

