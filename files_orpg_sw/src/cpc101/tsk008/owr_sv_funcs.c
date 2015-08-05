
/****************************************************************
		
    This module implements the various functions of the server part
    of the one-way-replicator (owr).

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/06/13 20:12:16 $
 * $Id: owr_sv_funcs.c,v 1.5 2011/06/13 20:12:16 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/* System include files */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

#include <orpg.h>
#include <infr.h>
#include "owr_def.h"

extern int Verbose;

#define UPD_LOCALLY 1
#define UPD_REMOTELY 2

#define N_MSGS_MNGD 32		/* max number of messages update-managed */

typedef struct {
    int id;			/* data store id */
    int fd;			/* LB file descriptor */
    char rep_type;		/* replication type */
    char w_perm;		/* write permission */
    short upd_flag;		/* the LB update bit flags */
    short n_msgs;		/* number of elements in msg_id */
    short maxn_msgs;		/* array size of msg_id */
    LB_id_t *msg_id;		/* message ids tracked for update-managed */
    char *mu_flag;		/* message update bit flags tracked */
} Data_store_t;

static Not_rep_msg_t *Not_to_cls = NULL;
static int N_not_to_cls = 0;

static void Lb_callback (int fd, LB_id_t msg_id, int msg_info, void *arg);
static void Send_lb_update_msg (Client_t *cl, int id, 
				LB_id_t msg_id, int msg_len);
static void Send_error_resp (int code, Message_t *req, Client_t *cl);
static void Log_msg_updated_by_both (Data_store_t *ds, LB_id_t msg_id);
static int Set_upd_flag (Data_store_t *ds, LB_id_t msg_id, int bit);
static int Get_upd_flag (Data_store_t *ds, LB_id_t msg_id, int bit);


/************************************************************************

    Processes a REQ_LB_REP request of "req_len" bytes from client "cl".
    The response message is generated and send to the client.

************************************************************************/

void SF_process_req_lb_rep (Client_t *cl, int req_len, Message_t *req) {
    char *rbuf;
    Data_store_t *dsp, ds;
    int did, i, fd, ret, len, ms, write_perm;
    char *buf, *name;
    Message_t *mh;
    LB_status status;
    LB_attr attr;

    if (req_len != sizeof (Message_t) || req_len != req->size) {
	MISC_log ("Bad REQ_LB_REP requet - Wrong size %d\n", req_len);
	Send_error_resp (BAD_MESSAGE, NULL, cl);
	return;
    }

    did = req->id;
    dsp = (Data_store_t *)cl->ds;
    for (i = 0; i < cl->n_ds; i++) {
	if (dsp[i].id == did) {
	    MISC_log ("REQ_LB_REP requet - %d already requested\n", did);
	    Send_error_resp (ALREADY_PROCESSED, req, cl);
	    return;
	}
    }

    write_perm = 0;
    fd = ORPGDA_open (did, LB_READ);
    if (fd < 0) {
	MISC_log ("Bad REQ_LB_REP requet - ORPGDA_open %d failed (%d)\n",
						did, fd);
	Send_error_resp (BAD_DID, req, cl);
	return;
    }

    status.n_check = 0;
    status.attr = &attr;
    ret = ORPGDA_stat (did, &status);
    if (ret < 0) {
	MISC_log ("ORPGDA_stat %d failed (%d)\n", did, ret);
	Send_error_resp (BAD_DID, req, cl);
	return;
    }
    if (!(attr.types & LB_SINGLE_WRITER)) {

	ORPGDA_close (did);
	fd = ORPGDA_open (did, LB_WRITE);
	if (fd < 0) {
	    MISC_log ("ORPGDA_open LB_WRITE %d failed (%d)\n", did, fd);
	    Send_error_resp (BAD_DID, req, cl);
	    return;
	}
	write_perm = 1;
    }

    if (req->code & (REP_TO_SERVER | REP_TO_NONE))
	ret = 0;
    else
	ret = ORPGDA_UN_register (did, LB_ANY, Lb_callback);
    if (ret < 0) {
	ORPGDA_close (did);
	MISC_log (" - ORPGDA_UN_register %d failed (%d)\n", did, ret);
	Send_error_resp (SERVER_ERROR, req, cl);
	return;
    }

    if ((req->code & REP_CCOCM) || 
		(!(attr.types & LB_DB) && !(req->code & REP_COPY_LB)))
	len = 0;
    else {
	len = ORPGDA_read (did, &buf, LB_ALLOC_BUF, LB_READ_ENTIRE_LB);
	if (len <= 0) {
	    ORPGDA_close (did);
	    MISC_log (" - ORPGDA_read ENTIRE_LB %d failed\n", did);
	    Send_error_resp (SERVER_ERROR, req, cl);
	    return;
	}
    }

    name = ORPGDA_lbname (did);
    if (name == NULL) {
	ORPGDA_close (did);
	if (len > 0)
	    free (buf);
	MISC_log (" - ORPGDA_lbname %d failed\n", did);
	Send_error_resp (SERVER_ERROR, req, cl);
	return;
    }

    rbuf = OC_compress_payload (
		strlen (name) + 1 + sizeof (Message_t) + sizeof (LB_attr),
		buf, len, &ms);
    if (len > 0)
	free (buf);

    mh = (Message_t *)rbuf;
    mh->type = RESP_LB_REP;
    mh->id = did;
    memcpy (rbuf + sizeof (Message_t), &attr, sizeof (LB_attr));
    strcpy (rbuf + sizeof (Message_t) + sizeof (LB_attr), name);

    memset (&ds, 0, sizeof (Data_store_t));
    ds.id = did;
    ds.fd = fd;
    ds.rep_type = req->code;
    ds.w_perm = write_perm;
    if (len == 0)
	ds.rep_type |= REP_LB_ANY;
    else {
	ds.maxn_msgs = attr.maxn_msgs;
	if (ds.maxn_msgs > N_MSGS_MNGD)
	    ds.maxn_msgs = N_MSGS_MNGD;
	ds.msg_id = (LB_id_t *)MISC_malloc (ds.maxn_msgs * 
				(sizeof (LB_id_t) + sizeof (char)));
	ds.mu_flag = (char *)(ds.msg_id + ds.maxn_msgs);
    }

    cl->ds = STR_append (cl->ds, &ds, sizeof (Data_store_t));
    cl->n_ds++;
    if (Verbose)
	MISC_log ("Data store %d rep info sent to client\n", did);

    SV_send_msg_to_client (cl, rbuf, ms);
    OS_free_buffer ();

    return;
}

/************************************************************************

    Generates and returns an error response of code "code".

************************************************************************/

static void Send_error_resp (int code, Message_t *req, Client_t *cl) {
    static Message_t mh;

    if (req == NULL)
	memset (&mh, 0, sizeof (Message_t));
    else
	memcpy (&mh, req, sizeof (Message_t));
    mh.type = RESP_LB_REP;
    mh.code = code;
    SV_send_msg_to_client (cl, (char *)&mh, mh.size);
}

/************************************************************************

    Processes a REQ_NOT_REP message of "len" bytes from client "req_cl".

************************************************************************/

void SF_procedss_not_rep (Client_t *req_cl, int len, Message_t *msg) {

    if (len < sizeof (Message_t) ||
	msg->code * sizeof (Not_rep_msg_t) + sizeof (Message_t) != len) {
	MISC_log ("Bad REQ_NOT_REP requet - Wrong size %d\n", len);
	return;
    }

    if (Not_to_cls != NULL)
	free (Not_to_cls);
    Not_to_cls = MISC_malloc (msg->code * sizeof (Not_rep_msg_t));
    memcpy (Not_to_cls, (char *)msg + sizeof (Message_t), 
				msg->code * sizeof (Not_rep_msg_t));
    N_not_to_cls = msg->code;
}

/************************************************************************

    Processes a LB_UPDATE message of "len" bytes from client "req_cl".

************************************************************************/

void SF_lb_update (Client_t *req_cl, int len, Message_t *msg) {
    Client_t *clients;
    int n_clients, i,  req_cid, ucnt, fcnt;

    if (len < sizeof (Message_t)) {
	MISC_log ("Bad LB_UPDATE requet - Wrong size %d\n", len);
	return;
    }

    n_clients = SV_get_clients (&clients);

    req_cid = req_cl->client.cid;
    ucnt = fcnt = 0;
    for (i = 0; i < n_clients; i++) {
	Data_store_t *ds;
	int k;
	Client_t *cl = clients + i;
	if (cl->stat == ST_DELETED)
	    continue;
	ds = (Data_store_t *)cl->ds;
	for (k = 0; k < cl->n_ds; k++) {
	    if (ds[k].id == msg->id) {

		if (cl->client.cid == req_cid) { /* the client sent req */
		    LB_id_t msg_id;
		    int ret, set, dlen;
		    char *data;

		    if (!(ds[k].w_perm)) {
			MISC_log ("Data store %d has no write premission\n", 
					msg->id);
			continue;
		    }

		    if (ds[k].rep_type & REP_LB_ANY)
			msg_id = LB_ANY;
		    else
			msg_id = msg->msg_id;
		    data = OC_decompress_payload (msg, 
					sizeof (Message_t), len, &dlen);
		    ret = ORPGDA_write (msg->id, data, dlen, msg_id);
		    OS_free_buffer ();
		    if (ret <= 0) {
			MISC_log ("ORPGDA_write %d failed (%d)\n", 
					msg->id, ret);
			continue;
		    }
		    if (Verbose)
			MISC_log ("Data store %d (msg %d) updated (from cl %d)\n", msg->id, msg_id, cl->client.cid);

		    set = Set_upd_flag (ds + k, msg_id, UPD_REMOTELY);
		    if (Get_upd_flag (ds + k, msg_id, UPD_LOCALLY) &&
			!(ds[k].rep_type & (REP_TO_SERVER | REP_TO_NONE))) {
			msg->cl_upd = 1;
			SV_send_msg_to_client (cl, (char *)msg, len);
			msg->cl_upd = 0;
			if (set)
			    Log_msg_updated_by_both (ds + k, msg_id);
		    }
		    ucnt++;
		}
		else if (!(ds[k].rep_type & (REP_TO_SERVER | REP_TO_NONE))) {
					/* tell other clients to update */
		    SV_send_msg_to_client (cl, (char *)msg, len);
		    fcnt++;
		    if (Verbose)
			MISC_log ("Data store %d (msg %d) update (from cl %d) forwarded to cl %d\n", msg->id, msg->msg_id, req_cid, cl->client.cid);
		}
	    }
	}
    }

    if (ucnt == 0 && fcnt == 0)
 	MISC_log ("Bad LB_UPDATE - did (%d) not found\n", msg->id);
    else if (ucnt == 0)
 	MISC_log ("Warning - LB_UPDATE - did (%d) not updated\n", msg->id);
}

/************************************************************************

    Frees resources allocation for client "cl".

************************************************************************/

void SF_free_resources (Client_t *cl) {
    int i;

    for (i = 0; i < cl->n_ds; i++) {
	Data_store_t *ds = (Data_store_t *)cl->ds;
	ORPGDA_close (ds[i].id);
	if (ds[i].msg_id != NULL)
	    free (ds[i].msg_id);
	ds[i].n_msgs = ds[i].maxn_msgs = 0;
    }
}

/************************************************************************

    The LB event callback function.

************************************************************************/

static void Lb_callback (int fd, LB_id_t msg_id, int msg_info, void *arg) {
    static int pid = 0;
    Client_t *clients;
    int n_clients, i;
    extern int Terminating;

    if (Terminating)
	return;

    if (msg_info <= 0)
	return;
    if (pid == 0)
	pid = getpid ();
    if (EN_sender_id () == pid)		/* ignore event posted by myself */
	return;

    n_clients = SV_get_clients (&clients);

    for (i = 0; i < n_clients; i++) {
	Data_store_t *ds;
	int k;
	Client_t *cl = clients + i;
	if (cl->stat == ST_DELETED)
	    continue;
	ds = (Data_store_t *)cl->ds;
	for (k = 0; k < cl->n_ds; k++) {
	    if (ds[k].fd == fd) {
		int set, n;

		if (ds[k].rep_type & (REP_TO_SERVER | REP_TO_NONE))
		    continue;
		for (n = 0; n < N_not_to_cls; n++) {
		    if (Not_to_cls[n].did == ds[k].id && 
					Not_to_cls[n].msg_id == msg_id)
			break;
		}
		if (n < N_not_to_cls)
		    continue;
		Send_lb_update_msg (cl, ds[k].id, msg_id, msg_info);
		set = Set_upd_flag (ds + k, msg_id, UPD_LOCALLY);
		if (set && Get_upd_flag (ds + k, msg_id, UPD_REMOTELY))
		    Log_msg_updated_by_both (ds + k, msg_id);
	    }
	}
    }
}

/************************************************************************

    Sends updated message "msg_id" ("msg_len" bytes) of Lb "fd' to client
    "cl". The data store id is "id".

************************************************************************/

static void Send_lb_update_msg (Client_t *cl, int id, 
				LB_id_t msg_id, int msg_len) {
    static int bs = 0;
    static char *rbuf = NULL;
    Message_t *mh;
    int len, ms;
    char *cbuf;

    if (msg_len + sizeof (Message_t) > bs) {
	if (rbuf != NULL)
	    free (rbuf);
	bs = msg_len + sizeof (Message_t);
	rbuf = MISC_malloc (bs);
    }

    len = ORPGDA_read (id, rbuf + sizeof (Message_t), msg_len, msg_id);
    if (len == LB_BUF_TOO_SMALL) {
	char *buf;
	len = ORPGDA_read (id, &buf, LB_ALLOC_BUF, msg_id);
	if (len > 0) {
	    if (rbuf != NULL)
		free (rbuf);
	    bs = len + sizeof (Message_t);
	    rbuf = MISC_malloc (bs);
	    memcpy (rbuf + sizeof (Message_t), buf, len);
	    free (buf);
	}
    }
    if (len <= 0) {
	MISC_log ("ORPGDA_read updated %d (msg %d, len %d) failed (%d)\n",
					id, msg_id, msg_len, len);
	return;
    }

    cbuf = OC_compress_payload (sizeof (Message_t), 
				rbuf + sizeof (Message_t), len, &ms);
    mh = (Message_t *)cbuf;
    mh->type = LB_UPDATE;
    mh->id = id;
    mh->msg_id = msg_id;
    SV_send_msg_to_client (cl, cbuf, ms);

    if (bs > LARGE_BUF_SIZE) {
	free (rbuf);
	rbuf = NULL;
	bs = 0;
    }
    OS_free_buffer ();
}

/************************************************************************

    Returns bit "bit" of the update flag of data store ds and msg_id.

************************************************************************/

static int Get_upd_flag (Data_store_t *ds, LB_id_t msg_id, int bit) {
    int i;

    if (ds->maxn_msgs == 0)
	return (ds->upd_flag & bit);
    for (i = 0; i < ds->n_msgs; i++) {
	if (ds->msg_id[i] == msg_id)
	    return (ds->mu_flag[i] & bit);
    }
    return (ds->upd_flag & bit);
}

/************************************************************************

    Adds bit "bit" to the update flag of data store ds and msg_id.
    Returns 0 if the bit has already been set or 1 otherwise.

************************************************************************/

static int Set_upd_flag (Data_store_t *ds, LB_id_t msg_id, int bit) {
    int i, set;

    set = 1;
    if (ds->maxn_msgs == 0) {
	if (ds->upd_flag & bit)
	    set = 0;
	else
	    ds->upd_flag |= bit;
    }
    else {
	for (i = 0; i < ds->n_msgs; i++) {
	    if (ds->msg_id[i] == msg_id) {
		if (ds->mu_flag[i] & bit)
		    set = 0;
		else
		    ds->mu_flag[i] |= bit;
		break;
	    }
	}
	if (i >= ds->n_msgs) {
	    if (i < ds->maxn_msgs) {
		ds->msg_id[i] = msg_id;
		ds->mu_flag[i] = bit;
		ds->n_msgs++;
	    }
	    else {
		if (ds->upd_flag & bit)
		    set = 0;
		else
		    ds->upd_flag |= bit;
	    }
	}
    }
    return (set);
}

/************************************************************************

    Logs the event that the message of "msg_id" in data store "ds" has 
    been updated from both the client and the server sides.

************************************************************************/

static void Log_msg_updated_by_both (Data_store_t *ds, LB_id_t msg_id) {
    int i;

    for (i = 0; i < ds->n_msgs; i++) {
	if (ds->msg_id[i] == msg_id) {
	    MISC_log ("Data store (%d, msg %d) updated from both cl and sv sides\n", ds->id, msg_id);
	    return;
	}
    }
    if (ds->maxn_msgs > 0)
	MISC_log ("Data store (%d, msg %d - not tracked) updated from both cl and sv sides\n", ds->id, msg_id);
    else
	MISC_log ("Data store (%d) updated from both cl and sv sides\n", ds->id, msg_id);
}










