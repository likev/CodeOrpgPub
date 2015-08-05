/****************************************************************
		
    This is the local include file for owr_client and owr_server.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/06/13 20:12:16 $
 * $Id: owr_def.h,v 1.5 2011/06/13 20:12:16 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef OWR_DEF_H
#define OWR_DEF_H

/* System include files */

#define MAX_N_CLIENTS 32receive_tm
#define LARGE_BUF_SIZE 102400
#define MAX_STR_SIZE 256
#define KEEP_ALIVE_TIME 6

enum {ST_CONN, ST_DELETED};	/* for Client_t.stat */

typedef struct {	/* server-side struct for each connected client */
    int stat;		/* status */
    CSS_client_t client;/* CSS client struct for the client */

    int n_ds;		/* number of data stores for the client */
    void *ds;		/* the data stores */
    time_t receive_tm;	/* time receiving the latest msg */
    time_t send_tm;	/* time sending the latest msg */
} Client_t;

enum {			/* Message types for Message_t.type */
    REQ_LB_REP,		/* Request an LB rep from client. Data store ID in 
			   Message_t.id. */
    RESP_LB_REP,	/* Response on REQ_LB_REP. LB attr, full-path of the
			   LB and the compressed LB data follow. */
    LB_UPDATE,		/* sends an updated LB message */
    KEEP_ALIVE,		/* keep alive message */
    REQ_NOT_REP		/* request not replicating certain messages */
};

typedef struct {	/* header for messages between client and server */
    char type;		/* message type */
    char code;		/* return code or rep type for REQ_LB_REP */
    char cl_upd;	/* LB updated from client side */
    char cmpr_method;	/* compression method */
    int size;		/* Size of the message. The original size if 
			   compressed */
    int id;		/* data store id */
    LB_id_t msg_id;	/* message id */
} Message_t;

enum {			/* Error response code */
    RESP_NORMAL,
    BAD_MESSAGE,
    ALREADY_PROCESSED,
    BAD_DID,
    SERVER_ERROR,
};

typedef struct {
    int did;
    int msg_id;
} Not_rep_msg_t;

/* LB replication types */
#define REP_LB_ANY 0x1		/* msgs are replicated as creation (LB_ANY) */
#define REP_TO_SERVER 0x2	/* replicate from client to server only */
#define REP_TO_CLIENT 0x4	/* replicate from server to client only */
#define REP_TO_NONE 0x8		/* no replication in any direction */
#define REP_CCOCM 0x10		/* client_can_only_create_message */
#define REP_COPY_LB 0x20	/* initially copy LB over */

/* global functions */
int OC_get_env_port ();
void SF_process_req_lb_rep (Client_t *cl, int req_len, Message_t *req);
int SV_get_clients (Client_t **clients);
void SV_send_msg_to_client (Client_t *cl, char *msg, int len);
void SF_free_resources (Client_t *cl);
int CF_read_config ();
int CF_process_resp_lb_rep (int len, Message_t *msg);
int CF_send_lb_reqs ();
char *CL_get_local_path (char *path);
int CF_lb_update (int len, Message_t *msg);
int CL_send_to_server (char *msg, int msg_size);
void SF_lb_update (Client_t *cl, int req_len, Message_t *req);
char *CF_get_local_path (int did);
int CF_prereg_ans ();
void OC_restart ();
void OC_prepare_for_restart (int argc, char **argv);
void CF_report_statistics ();
char *OC_compress_payload (int hd_len, char *pload, int pl_size, int *msp);
char *OC_decompress_payload (Message_t *msg, 
				int hd_size, int msg_len, int *ds);
void OS_free_buffer ();
void SF_procedss_not_rep (Client_t *req_cl, int len, Message_t *msg);

#endif
