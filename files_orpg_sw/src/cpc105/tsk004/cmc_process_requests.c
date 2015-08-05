
/******************************************************************

	file: cmc_process_requests.c

	This module contains the functions that process the requests
	- shared version.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2013/01/02 17:06:39 $
 * $Id: cmc_process_requests.c,v 1.38 2013/01/02 17:06:39 jing Exp $
 * $Revision: 1.38 $
 * $State: Exp $
 *
 * 12MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Add support for TCP Dial-out.
 *
 * 20MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Fix some minor problems
 *                           found in unit testing.
 *
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include <infr.h>
#include <comm_manager.h>
#include <cmc_def.h>

static int Cr_time;		/* current time */

static int N_req_processed = 0; /* number of requests processed */

extern LB_id_t Msg_id;		/* defined in cmc_common.c */
extern int CMC_comm_resp_event;	/* defined in cmc_common.c */ 

typedef struct {
    char method;		/* compression method */
    char packed;		/* 1 if packed or 0 if not */
    char unused;
    char unused1;
    int org_length;		/* data size before compression */
} Compress_hd_t;

/* If thr first bit of Tcp_msg_header.length is set, the incoming data is 
   compressed. The compressed data starts with Compress_hd_t. All fields
   in Compress_hd_t are in network byte order */

static void (*Additional_processing) (Link_struct *link) = NULL;

/* local functions */
static int Search_req (Link_struct *link);
static int Pending_request (Link_struct *link, int type, int ind);
static void Process_connect (Link_struct *link, int ind);
static void Process_disconnect (Link_struct *link, int ind);
static void Process_dialout_disconnect (Link_struct *link, int ind);
static void Process_dialout (Link_struct *link, int ind);
static void Process_write (Link_struct *link, int ind);
static void Connection_procedure (Link_struct *link);
static void Process_status (Link_struct *link, int ind);
static int Process_statistics_request (Link_struct *link, int rep_period);
static void Process_set_params (Link_struct *link, int ind);
static int Compress_data (Link_struct *link, int pvc, Req_struct *req);


void CMPR_set_additional_processing (void (*ap) (Link_struct *link)) {
    Additional_processing = ap;
}

/**************************************************************************

    Description: This function processes any pending requests.

    Return:	It returns the total number of requests processed by this
		process.

**************************************************************************/

int CMPR_process_requests (int nlinks, Link_struct **links)
{
    int cnt, i;

    cnt = 0;
    Cr_time = MISC_systime (NULL);
    for (i = 0; i < nlinks; i++) {
	Req_struct *req;
	int ind;

	if ((ind = Search_req (links[i])) < 0)
	    continue;

	cnt++;
	req = &(links[i]->req[ind]);
	if (req->type != CM_WRITE)
	    LE_send_msg (LE_VL1 | 32,  "request %d received on link %d", 
				req->type, links[i]->link_ind);
	switch (req->type) {

	    case CM_CONNECT:
		if (links[i]->link_type == CM_DIAL_IN_OUT)
		    CMPR_send_response (links[i], req, CM_NOT_CONFIGURED);
		else 
		    Process_connect (links[i], ind);
		break;

	    case CM_DIAL_OUT:
		if (links[i]->link_type != CM_DIAL_IN_OUT)
		    CMPR_send_response (links[i], req, CM_NOT_CONFIGURED);
		else
		    Process_dialout (links[i], ind);
		break;

	    case CM_DISCONNECT:
		if (links[i]->link_type != CM_DIAL_IN_OUT)
			Process_disconnect (links[i], ind);
		else
			Process_dialout_disconnect (links[i], ind);
		break;

	    case CM_WRITE:
		Process_write (links[i], ind);
		break;

	    case CM_STATUS:
		Process_status (links[i], ind);
		break;

	    case CM_SET_PARAMS:
		Process_set_params (links[i], ind);
		break;

	    default:
		break;
	}
    }

    return (N_req_processed);
}


/**************************************************************************

    Description: This function forms a response message and sends to the 
		comm_manager user. It frees the allocated memory segments 
		for CM_WRITE request. It also sets the req->state to CM_DONE.

    Inputs:	link - the link structure corresponding to the request;
		req - the original request;
		ret_code - the return code;

**************************************************************************/

void CMPR_send_response (Link_struct *link, Req_struct *req, int ret_code)
{
    CM_resp_struct resp;
    int ret;

    /* clean up the resources allocated for this request */
    if (req->type == CM_WRITE) {
	char *cpt;
	int pvc;

	/* find the pvc */
	cpt = req->data + sizeof (CM_req_struct);
	if (link->n_added_bytes != 0)
	    cpt += CMC_align_bytes (link->n_added_bytes);
	for (pvc = 0; pvc < link->n_pvc; pvc++)
	    if (cpt == link->w_buf[pvc])
		break;
	if (pvc < link->n_pvc)
	    link->w_buf[pvc] = NULL;

	if (req->data != NULL && req->data != link->pack_buf)
	    CMC_free (req->data);
	req->data = NULL;
    }

    if (req->type == CM_SET_PARAMS || req->type == CM_CONNECT) {
	if (req->data != NULL)
	    CMC_free (req->data);
	req->data = NULL;
    }

    if ((req->type == CM_CONNECT || req->type == CM_DISCONNECT || 
		req->type == CM_DIAL_OUT) &&
	(ret_code != CM_IN_PROCESSING && ret_code != CM_TRY_LATER)){
	link->conn_activity = NO_ACTIVITY;
	link->dial_activity = NO_ACTIVITY;
	link->dial_state = NORMAL;
    }

    if (link->proto == PROTO_PVC)
	CMRATE_write_done (link);

    /* send the response message. We don't send for packed write. */
    if (req->type != CM_WRITE || req->msg_id != LB_ANY) {
	resp.type = req->type;
	resp.req_num = req->req_num;
	resp.link_ind = req->link_ind;
	resp.ret_code = ret_code;
	resp.data_size = 0;
	resp.time = time (NULL);
    
	ret = LB_write (link->respfd, (char *)&resp, 
				    sizeof (CM_resp_struct), LB_NEXT);
	if (ret != sizeof (CM_resp_struct)) {
	    LE_send_msg (GL_ERROR | 33,  "LB_write (response) failed (ret = %d)", ret);
	    CM_terminate ();
	}
	if (CMC_get_en_flag () >= 0)
	    EN_post (CMC_comm_resp_event + link->link_ind, 
					    NULL, 0, CMC_get_en_flag ());
    
	if (req->type != CM_WRITE)
	    LE_send_msg (LE_VL1 | 34,  
		    "response of type %d (ret_code %d) sent on link %d", 
				    req->type, ret_code, link->link_ind);
    }

    req->state = CM_DONE;
    N_req_processed++;
    if (Additional_processing != NULL)
	Additional_processing (link);

    return;
}

/**************************************************************************

    Returns sizeof (Compress_hd_t);

**************************************************************************/

int CMPR_comp_hd_size () {
    return (sizeof (Compress_hd_t));
}

/**************************************************************************

    Description: This function sends a received data message to the user if
		the data is packed and/or compressed.

    Inputs:	link - the link structure corresponding to the request;
		pvc - the PVC index;
		len - data length;
		data - pointer to the data;

**************************************************************************/

void CMPR_send_compressed_data (Link_struct *link, int pvc) {
    static char *dest = NULL;
    int method, org_size, ret, extra_bytes;
    char *st_p;

    Compress_hd_t *hd = (Compress_hd_t *)(link->r_buf[pvc]);
    if (link->r_cnt[pvc] < sizeof (Compress_hd_t)) {
	LE_send_msg (GL_ERROR, "Compression header missing");
	return;
    }
    method = hd->method;
    extra_bytes = sizeof (CM_resp_struct) + 
			    CMC_align_bytes (link->n_added_bytes);
    st_p = NULL;
    if (method >= 0) {
	org_size = ntohl (hd->org_length);
	dest = STR_reset (dest, org_size + extra_bytes);
	ret = MISC_decompress (method, 
		link->r_buf[pvc] + sizeof (Compress_hd_t), 
		(unsigned int)(link->r_cnt[pvc] - sizeof (Compress_hd_t)),
		dest + extra_bytes, org_size);
	if (ret > 0) {
	    st_p = dest + extra_bytes;
	    link->org_bytes += org_size;
	    link->comp_bytes += link->r_cnt[pvc];
	    LE_send_msg (LE_VL2, 
		"    Message read and decompressed %d->%d bytes, pvc %d, link %d", 
			link->r_cnt[pvc], org_size, pvc, link->link_ind);
	}
	else {
	    LE_send_msg (GL_ERROR, 
		"Decompress payload failed (%d->%d), pvc %d, link %d", 
			    link->r_cnt[pvc], org_size, pvc, link->link_ind);
	}
    }
    else {
	st_p = link->r_buf[pvc] + sizeof (Compress_hd_t);
	org_size = link->r_cnt[pvc];
    }

    if (st_p != 0) {
	if (hd->packed == 0)
	    CMPR_send_data (link, pvc, org_size - sizeof (Compress_hd_t), st_p);
	else {
	    static char *sizes = NULL;
	    int n_msgs, *szs, off, i;
	    org_size -= sizeof (Compress_hd_t);
	    n_msgs = ntohl (*((int *)st_p));
	    sizes = STR_reset (sizes, 128);
	    sizes = STR_append (sizes, 
			st_p + org_size - n_msgs * sizeof (int), 
						n_msgs * sizeof (int));
	    szs = (int *)sizes;
	    off = sizeof (int);
	    org_size -= n_msgs * sizeof (int);
	    for (i = 0; i < n_msgs; i++) {
		int s;
		s = ntohl (szs[i]);
		if (s + off > org_size) {
		    LE_send_msg (GL_ERROR, 
		"Size error in unpacking msgs (%d %d %d %d), pvc %d, link %d", 
			    1, s, off, org_size, pvc, link->link_ind);
		    break;
		}
		CMPR_send_data (link, pvc, s, st_p + off);
		LE_send_msg (LE_VL3, "    Message from pack %d bytes, pvc %d, link %d", 
			    s, pvc, link->link_ind);
		off += s;
		if (i == n_msgs - 1 && off != org_size)
		    LE_send_msg (GL_ERROR, 
		    "Unused bytes in unpacking msgs (%d), pvc %d, link %d", 
			    org_size - off, pvc, link->link_ind);
	    }
	}
    }
    return;
}

/**************************************************************************

    Description: This function sends a received data message to the user.

    Inputs:	link - the link structure corresponding to the request;
		pvc - the PVC index;
		len - data length;
		data - pointer to the data;

**************************************************************************/

void CMPR_send_data (Link_struct *link, int pvc, int len, char *data)
{
    CM_resp_struct *resp;
    CM_resp_struct tmp_resp;
    char *cpt;
    int ret;
   
    /* the response header */
    if (link->n_added_bytes == 0)
	resp = (CM_resp_struct *)(data - sizeof (CM_resp_struct));
    else
	resp = &tmp_resp;

    resp->type = CM_DATA;
    resp->req_num = link->r_seq_num;
    link->r_seq_num++;
    resp->link_ind = link->link_ind;
    resp->ret_code = pvc + 1;		/* priority */
    resp->data_size = len + link->n_added_bytes;
    resp->time = time (NULL);

    if (link->n_added_bytes != 0) {
	cpt = data - sizeof (CM_resp_struct) - link->n_added_bytes;
	memcpy (cpt, (char *)&tmp_resp, sizeof (CM_resp_struct));
    }
    else
	cpt = data - sizeof (CM_resp_struct);

    ret = LB_write (link->respfd, cpt, 
				sizeof (CM_resp_struct) + len + link->n_added_bytes, LB_NEXT);
    if (ret != sizeof (CM_resp_struct) + len + link->n_added_bytes) {
	LE_send_msg (GL_ERROR,  "LB_write (send data) failed (ret = %d)", ret);
	CM_terminate ();
    }
    if (link->data_en && CMC_get_en_flag () >= 0)
	EN_post (CMC_comm_resp_event + link->link_ind, 
					NULL, 0, CMC_get_en_flag ());

    return;
}

/**************************************************************************

    Description: This function sends a CM_EVENT message to the user.

    Inputs:	link - the link structure corresponding to the request;
		ret_code - event code.
		buf - caller provided buffer for the response message.

**************************************************************************/

void CMPR_send_event_response (Link_struct *link, int ret_code, char *buf)
{
    CM_resp_struct resp_buf;
    CM_resp_struct *resp;
    int len, ret;

    if (buf == NULL) {
	resp = &resp_buf;
	resp->data_size = 0;
    }
    else
	resp = (CM_resp_struct *)buf;

    /* send the response message */
    resp->type = CM_EVENT;
    resp->link_ind = link->link_ind;
    resp->ret_code = ret_code;
    resp->time = time (NULL);
    resp->req_num = 0;
    len = sizeof (CM_resp_struct);
    if (ret_code == CM_STATISTICS || ret_code == CM_STATUS_MSG)
	len += resp->data_size;

    ret = LB_write (link->respfd, (char *)resp, len, LB_NEXT);
    if (ret != len ) {
	LE_send_msg (GL_ERROR | 36,  "LB_write (response) failed (ret = %d)", ret);
	exit (1);
    }
    if (CMC_get_en_flag () >= 0)
	EN_post (CMC_comm_resp_event + link->link_ind, 
					NULL, 0, CMC_get_en_flag ());

    LE_send_msg (LE_VL1 | 37,  "event of code %d sent on link %d", 
				ret_code, link->link_ind);

    return;
}

/**************************************************************************

    Description: This function processes the CM_DIAL_OUT request.

    Inputs:	link - the link structure corresponding to the request;
		ind - link->req[ind] is the CM_CONNECT request;
    Note:	There can only be one on-going dialout/disconnect request
		under processing. link->dial_activity indicates the
		current connection activity: CONNECTING, DISCONNECTING
		or NO_ACTIVITY. link->req[link->conn_req_ind] is the
		current connect/disconnect request under processing.
		link->conn_activity is used for internal state mapping.
**************************************************************************/

static void Process_dialout (Link_struct *link, int ind)
{
    Req_struct *req;

#ifdef SIMPACT

    Dial_params *p;

    req = &(link->req[ind]);

    p = (Dial_params *)(req->data + sizeof (CM_req_struct));
    if (strlen(p->phone_no) <= 0){
	LE_send_msg (GL_ERROR, "Phone no. needed for dialout...\n");
	CMPR_send_response (link, req, CM_INVALID_PARAMETER);
	return;
    }

    if (link->dial_activity == DISCONNECTING ){
	CMPR_send_response (link, req, CM_TRY_LATER);
	return;
    }

    if (link->dial_activity == CONNECTING ){
	if (strcmp(link->phone_no,p->phone_no) == 0){
		CMPR_send_response (link, req, CM_IN_PROCESSING);
	}
	else{
		LE_send_msg (GL_ERROR, 
			"Port in use;Connected to %s on link no:%d ..\n",
			link->phone_no,link->link_ind);
		CMPR_send_response (link, req, CM_PORT_IN_USE);
	}
	return;
    }


    if (link->link_state == LINK_CONNECTED) {
	if (strcmp(link->phone_no,p->phone_no) == 0 ){
		CMPR_send_response (link, req, CM_SUCCESS);
	}
	else{
		LE_send_msg (GL_ERROR, "Port in use on link no:%d %s:%s..\n",
			link->link_ind);
		CMPR_send_response (link, req, CM_PORT_IN_USE);
	}
	return;
    }


    strcpy(link->phone_no, p->phone_no);
    LE_send_msg (LE_VL1, "process dialout connect on link %d", link->link_ind);
    req->state = Cr_time;
    link->conn_req_ind = ind;
    link->dial_activity = CONNECTING;
    link->conn_activity = CONNECTING;
    DO_dialout_procedure(link);
    return;

#elif defined (CM_TCP)
    Dial_params *p;
    int ph_index;
    int k;

    req = &(link->req[ind]);

    p = (Dial_params *)(req->data + sizeof (CM_req_struct));
    if (strlen(p->phone_no) <= 0){
	LE_send_msg (GL_ERROR, "Phone no. needed for dialout...\n");
	CMPR_send_response (link, req, CM_INVALID_PARAMETER);
	return;
    }

    if (link->dial_activity == DISCONNECTING ){
	CMPR_send_response (link, req, CM_TRY_LATER);
	return;
    }

    if (link->dial_activity == CONNECTING ){
	if (strcmp(link->phone_no,p->phone_no) == 0){
		CMPR_send_response (link, req, CM_IN_PROCESSING);
	}
	else{
		LE_send_msg (GL_ERROR, 
			"Port in use;Connected to %s on link no:%d ..\n",
			link->phone_no,link->link_ind);
		CMPR_send_response (link, req, CM_PORT_IN_USE);
	}
	return;
    }


    if (link->link_state == LINK_CONNECTED) {
	if (strcmp(link->phone_no,p->phone_no) == 0 ){
		CMPR_send_response (link, req, CM_SUCCESS);
	}
	else{
		LE_send_msg (GL_ERROR, "Port in use on link no:%d %s:%s..\n",
			link->link_ind);
		CMPR_send_response (link, req, CM_PORT_IN_USE);
	}
	return;
    }


    strcpy(link->phone_no, p->phone_no);

    ph_index = DO_search_dialout_table(link);
    if (ph_index < 0) {
        LE_send_msg (LE_VL1, "dialout not configured %s not found in phone table",
                              link->phone_no);
	CMPR_send_response (link, req, CM_NOT_CONFIGURED);
	return;
    }


    link->server = link->phone_nums[ph_index]->server;
    link->port_number = link->phone_nums[ph_index]->port_num;

    if (link->address != NULL) free (link->address);
    link->address = NULL;
    if (link->server_name != NULL) free (link->server_name);
    link->server_name = malloc (strlen (link->phone_nums[ph_index]->server_name) + 1);
    if (link->server_name == NULL) {
       LE_send_msg (GL_ERROR | 1007,  "malloc failed");
       return;
    }
    strcpy (link->server_name, link->phone_nums[ph_index]->server_name);

    if (link->dialout_name != NULL) free (link->dialout_name);
    link->dialout_name = malloc (strlen (link->phone_nums[ph_index]->dialout_name) + 1);
    if (link->dialout_name == NULL) {
       LE_send_msg (GL_ERROR | 1007,  "malloc failed");
       return;
    }
    strcpy (link->dialout_name, link->phone_nums[ph_index]->dialout_name);

    if (link->ch2_address != NULL) free (link->ch2_address);
    link->ch2_address = NULL;
    if (link->ch2_name != NULL) free (link->ch2_name);

    /* this is an optional parameter */
    if (link->phone_nums[ph_index]->ch2_name != NULL ) {
       link->ch2_name = malloc (strlen (link->phone_nums[ph_index]->ch2_name) + 1);
       if (link->ch2_name == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
          return;
       }
       strcpy (link->ch2_name, link->phone_nums[ph_index]->ch2_name);
    }


    if (link->password != NULL) free (link->password);
    link->password = malloc (strlen (link->phone_nums[ph_index]->password) + 1);
    if (link->password == NULL) {
       LE_send_msg (GL_ERROR | 1007,  "malloc failed");
       return;
    }
    strcpy (link->password, link->phone_nums[ph_index]->password);

    link->line_rate = link->phone_nums[ph_index]->line_rate;

    if ( link->server == CMT_FAACLIENT ) {

 
       link->ch1_link = NULL;
       if (link->ch2_link != NULL)
          free (link->ch2_link);
       link->ch2_link = malloc (sizeof (Link_struct));

       if (link->ch2_link == NULL) {
          LE_send_msg (GL_ERROR | 1007,  "malloc failed");
          return;
       }

       /* init ch2_link */           
       link->ch2_link->n_pvc = link->n_pvc;
       link->ch2_link->ch_num = 2;
       link->ch2_link->ch1_link = link;
       link->ch2_link->link_state = LINK_DISCONNECTED;
       link->ch2_link->conn_activity = NO_ACTIVITY;
       link->ch2_link->dial_state = NORMAL;
       link->ch2_link->dial_activity = NO_ACTIVITY;
       link->ch2_link->rep_period = 0;
       link->ch2_link->rw_time = 0;
       link->ch2_link->n_added_bytes = 0;
       link->ch2_link->r_seq_num = 0;

       for (k = 0; k < link->n_pvc; k++) {
          link->ch2_link->r_buf[k] = NULL;
          link->ch2_link->r_cnt[k] = 0;
          link->ch2_link->r_buf_size[k] = 0;
          link->ch2_link->w_buf[k] = NULL;
       }

    }

    LE_send_msg (LE_VL1, "process dialout connect on link %d", link->link_ind);
    req->state = Cr_time;
    link->conn_req_ind = ind;
    link->dial_activity = CONNECTING;
    link->conn_activity = CONNECTING;
    DO_dialout_procedure(link);
    return;

#else
	req = &(link->req[ind]);
    	LE_send_msg (GL_ERROR | 38,  "CM_DIAL_OUT is not implemented for this protocol");
	CMPR_send_response (link, req, CM_NOT_CONFIGURED);
#endif

}

/**************************************************************************

    Description: This function processes the CM_STATUS request.

    Inputs:	link - the link structure corresponding to the request;
		ind - link->req[ind] is the CM_STATUS request;

**************************************************************************/

static void Process_status (Link_struct *link, int ind)
{
    Req_struct *req;

    req = &(link->req[ind]);
    if (link->link_state == LINK_CONNECTED) {
	if (link->pack_time > 0 &&
	    (link->pack_cnt > 0 || Pending_request (link, CM_WRITE, -1) >= 0))
	    CMPR_send_response (link, req, CM_WRITE_PENDING);
	else
	    CMPR_send_response (link, req, CM_CONNECTED);
    }
    else 
	CMPR_send_response (link, req, CM_DISCONNECTED);

    return;
}

/**************************************************************************

    Description: This function sets the statistics reporting period.

    Inputs:	link - the link structure corresponding to the request;
		rep_period - reporting period;

    Return:	error status.

**************************************************************************/

static int Process_statistics_request (Link_struct *link, int rep_period)
{

    if (rep_period <= 0) {
	link->rep_period = 0;
	return (CM_SUCCESS);
    }
    else {
	if (link->link_state == LINK_CONNECTED) {
	    link->rep_period = rep_period;
	    link->rep_time = MISC_systime (NULL);
	    XP_statistics_reset (link);
	    return (CM_SUCCESS);
	}
	else {
	    return (CM_REJECTED);
	}
    }
}

/**************************************************************************

    Description: This function processes the CM_SET_PARAMS request.

    Inputs:	link - the link structure corresponding to the request;
		ind - link->req[ind] is the CM_STATUS request;

**************************************************************************/

static void Process_set_params (Link_struct *link, int ind)
{
    Req_struct *req;
    Link_params *p;
    int ret;

    req = &(link->req[ind]);
    p = (Link_params *)(req->data + sizeof (CM_req_struct));

    if (p->link_type >= 0)
	link->link_type = p->link_type;
    if (p->line_rate >= 0) 
	link->line_rate = p->line_rate;
    if (p->packet_size >= 0)
	link->packet_size = p->packet_size;
    if (p->rw_time >= 0)
	link->rw_time = p->rw_time;
    ret = CM_SUCCESS;
    if (p->rep_period >= 0)
	ret = Process_statistics_request (link, p->rep_period);

    CMPR_send_response (link, req, ret);
    return;
}

/**************************************************************************

    Description: This function processes the CM_WRITE request.

    Inputs:	link - the link structure corresponding to the request;
		ind - link->req[ind] is the CM_WRITE request;

**************************************************************************/

static void Process_write (Link_struct *link, int ind)
{
    Req_struct *req;
    int pvc;

    req = &(link->req[ind]);
    pvc = req->parm - 1;		/* the circuit for this write */

    /* make sure there is no unfinished write of the same priority */
    if (link->w_buf[pvc] != NULL)
	return;

    link->w_buf[pvc] = req->data + sizeof (CM_req_struct);
    if (link->n_added_bytes != 0)
	link->w_buf[pvc] += CMC_align_bytes (link->n_added_bytes);
    link->w_size[pvc] = req->data_size - link->n_added_bytes;
    link->w_cnt[pvc] = 0;
    link->w_ack[pvc] = 0;
    link->w_req_ind[pvc] = ind;
    link->w_time_out[pvc] = 0;
    Compress_data (link, pvc, req);

    if (link->proto == PROTO_PVC)
	CMRATE_start_write (link, req->data_size);

    req->state = Cr_time;
    if (link->proto == PROTO_HDLC)
	HA_write_data (link, 0);
    else
	XP_write_data (link, pvc);

    return;
}

/**************************************************************************

    Description: This function processes the CM_CONNECT request.

    Inputs:	link - the link structure corresponding to the request;
		ind - link->req[ind] is the CM_CONNECT request;

    Note:	There can only be one on-going connect/disconnect request
		under processing. link->conn_activity indicates the
		current connection activity: CONNECTING, DISCONNECTING
		or NO_ACTIVITY. link->req[link->conn_req_ind] is the
		current connect/disconnect request under processing.

**************************************************************************/

static void Process_connect (Link_struct *link, int ind)
{
    Req_struct *req;
    int i;
    int current_activity;

    req = &(link->req[ind]);
    current_activity = link->conn_activity;
	/* remove pending disconnect requests */
    while ((i = Pending_request (link, CM_DISCONNECT, ind)) >= 0)
	CMPR_send_response (link, &(link->req[i]), CM_SUCCESS);

    if (link->conn_activity == CONNECTING) {
	CMPR_send_response (link, req, CM_IN_PROCESSING);
	return;
    }

    if (link->link_state == LINK_CONNECTED) {
	CMPR_send_response (link, req, CM_SUCCESS);
	return;
    }

    LE_send_msg (LE_VL1 | 39,  "process connect on link %d", link->link_ind);

    req->state = Cr_time;
    link->conn_activity = CONNECTING;
    link->conn_req_ind = ind;
    link->pack_cnt = 0;
    if ( current_activity == NO_ACTIVITY)
    	Connection_procedure (link);

    return;
}

/**************************************************************************

    Description: This function calls appropriate protocol functions to 
		start the connection procedure.

    Inputs:	link - the link structure corresponding to the request;

**************************************************************************/

static void Connection_procedure (Link_struct *link)
{

    if (link->proto == PROTO_HDLC)
	HA_connection_procedure (link);
    else
	XP_connection_procedure (link);

}

/**************************************************************************

    Description: This function processes the CM_DISCONNECT request.

    Inputs:	link - the link structure corresponding to the request;
		ind - link->req[ind] is the CM_DISCONNECT request;

    Note:	Refer to Note in Process_connect.

**************************************************************************/

static void Process_disconnect (Link_struct *link, int ind)
{
    Req_struct *req;
    int i;

    req = &(link->req[ind]);

    /* stop all pending connect request */
    while ((i = Pending_request (link, CM_CONNECT, ind)) >= 0)
	CMPR_send_response (link, &(link->req[i]), CM_TERMINATED);

    if (link->conn_activity == DISCONNECTING ) {
	CMPR_send_response (link, req, CM_IN_PROCESSING);
	return;
    }

    LE_send_msg (LE_VL1 | 40,  "Process disconnect on link %d",link->link_ind);

    req->state = Cr_time;
    link->conn_activity = DISCONNECTING;
    link->conn_req_ind = ind;
    Connection_procedure (link);

    return;
}

/**************************************************************************

    Description: This function processes the CM_DISCONNECT request on a dial line

    Inputs:	link - the link structure corresponding to the request;
		ind - link->req[ind] is the CM_DISCONNECT request;

    Note:	Refer to Note in Process_connect.

**************************************************************************/

static void Process_dialout_disconnect (Link_struct *link, int ind)
{
    Req_struct *req;
    int i;
    int current_activity;

    req = &(link->req[ind]);
    current_activity = link->conn_activity;

    if (link->dial_activity == CONNECTING && 
		link->dial_state >= WAITING_FOR_MODEM_RESPONSE &&
		link->dial_state <= PROCESS_EXCEPTION ) {
	CMPR_send_response (link, req, CM_TRY_LATER);
	return;
    }
    /* stop all pending connect request */
    while ((i = Pending_request (link, CM_CONNECT, ind)) >= 0)
		CMPR_send_response (link, &(link->req[i]), CM_TERMINATED);

    if (link->dial_activity == DISCONNECTING) {
	CMPR_send_response (link, req, CM_IN_PROCESSING);
	return;
    }

    LE_send_msg (LE_VL1, "process disconnect on dialout link %d",link->link_ind);

    req->state = Cr_time;
    link->dial_activity = DISCONNECTING;
    link->conn_req_ind = ind;
    if ( link->dial_state == X25_DIAL_CONNECT_PENDING)
    	link->dial_state = X25_DIAL_DISCONNECT;
    
    if (current_activity == NO_ACTIVITY){
    	link->conn_activity = DISCONNECTING;
    	link->dial_state = X25_DIAL_DISCONNECT;
    	Connection_procedure (link);
    }
    return;
}

/**************************************************************************

    Description: This function searches for any pending request of "type"
		on "link" received before request req["ind"]. If ind = -1, 
		all requests are searched.

    Inputs:	link - the link structure;
		ind - link->req[ind] is the request in processing;

    Return:	The index in array link->req of the pending  request of 
		"type" if it is found, or -1 if not found.

**************************************************************************/

static int Pending_request (Link_struct *link, int type, int ind)
{
    int req_ind, i;

    req_ind = link->st_ind - 1;
    for (i = 0; i < link->n_reqs; i++) {
	Req_struct *req;

	req_ind = (req_ind + 1) % MAX_N_REQS;
	if (req_ind == ind)
	    break;

	req = &(link->req[req_ind]);
	if (req->type == type && req->state != CM_DONE)
	    return (req_ind);
    }

    return (-1);
}

/**************************************************************************

    Description: This function processes the CM_CANCEL request. It removes
		those requests that match req->parm and are not yet being 
		processed. The canceled requests are removed from the "req" 
		list.

    Inputs:	link - the link structure corresponding to the request;
		new_req - the CM_CANCEL request;

**************************************************************************/

void CMPR_process_cancel (Link_struct *link, Req_struct *new_req)
{
    int ind, i, cnt;

    cnt = 0;
    ind = link->st_ind - 1;
    for (i = 0; i < link->n_reqs; i++) {
	Req_struct *req;

	ind = (ind + 1) % MAX_N_REQS;
	req = &(link->req[ind]);
	if ((new_req->parm == -1 || new_req->parm == req->req_num) &&
						/* match the req number */
	    req->state == CM_NEW) {		/* not started */
	    int rm_ind, k;

	    if (req->type == CM_WRITE) {	/* free the data buffer */
		char *cpt;

		cpt = req->data + sizeof (CM_req_struct);
		if (link->n_added_bytes != 0)
		    cpt += CMC_align_bytes (link->n_added_bytes);
		for (k = 0; k < link->n_pvc; k++) {
		    if (cpt == link->w_buf[k])
			link->w_buf[k] = NULL;
		}
		if (req->data != NULL && req->data != link->pack_buf) {
		    CMC_free (req->data);
		    req->data = NULL;
		}
	    }
	
	    if (req->type == CM_SET_PARAMS || req->type == CM_CONNECT) {
						/* free the data buffer */
		if (req->data != NULL) {
		    CMC_free (req->data);
		    req->data = NULL;
		}
	    }

	    /* remove req from the list */
	    rm_ind = ind;
	    for (k = 1; k < link->n_reqs - i; k++) {
		memcpy (&(link->req[rm_ind]), 
			&(link->req[(rm_ind + 1) % MAX_N_REQS]), 
			sizeof (Req_struct));
		rm_ind = (rm_ind + 1) % MAX_N_REQS;
	    }
	    link->n_reqs--;
	    i--;
	    cnt++;
	}
    }

    /* send a response */
    CMPR_send_response (link, new_req, cnt);

    return;
}

/**************************************************************************

    Description: This function searches for the next request to be 
		processed in the "req" list.

    Inputs:	link - the link structure corresponding to the request;

    Return:	Returns the index in "req" of the request to be processed
		or -1 if there is nothing can be processed.

**************************************************************************/

static int Search_req (Link_struct *link)
{
    Req_struct *req;
    int i, ind;
    int wind [MAX_N_STATIONS];

    if (link->n_reqs <= 0)
	return (-1);

    req = link->req;
    ind = link->st_ind - 1;
    for (i = 0; i < MAX_N_STATIONS; i++)
	wind [i] = -1;
    for (i = 0; i < link->n_reqs; i++) {	/* go through all requests */

	ind = (ind + 1) % MAX_N_REQS;

	if (req[ind].state == CM_NEW || req[ind].state == CM_NEW_PACKED) {

	    if (req[ind].type != CM_WRITE)
		return (ind);	/* return the first non-write request */
	    else {
		if (wind[req[ind].parm - 1] < 0)
		    wind[req[ind].parm - 1] = ind;
	    }

	}
    }

    /* return the write request of highest priority level */
    for (i = 0; i < MAX_N_STATIONS; i++) {
	if (wind[i] >= 0)
	    return (wind[i]);
    }

    return (-1);
}

/**************************************************************************

    Description: This function is called when a connection is lost or a 
		connect/disconnect procedure failed. It terminates all 
		unfinished requests on that link, cleans up the link and 
		sends lost a connection event when it is appropriate.

    Inputs:	link - the link involved.

**************************************************************************/

void CMPR_connect_failure (Link_struct *link)
{

    if (link->conn_activity == NO_ACTIVITY &&
		link->link_state == LINK_CONNECTED)
	CMPR_send_event_response (link, CM_LOST_CONN, NULL);

    link->link_state = LINK_DISCONNECTED;
    link->dial_wait_time = 0;

    CMPR_cleanup (link);

    if (link->conn_activity != NO_ACTIVITY)
    	CMPR_send_response (link, 
			&(link->req[(int)link->conn_req_ind]), CM_FAILED);

    return;
}

/**************************************************************************

    Description: This function is called when a failure happens during
		dialout phase.  It terminates all 
		unfinished requests on that link, cleans up the link and 
		sends an appropriate message.

    Inputs:	link - the link involved.
		ret_value: failure return code from the modem

**************************************************************************/

void CMPR_dialout_failure (Link_struct *link, int ret_value)
{
    if (link->conn_activity == NO_ACTIVITY &&
		link->link_state == LINK_CONNECTED)
	CMPR_send_event_response (link, CM_LOST_CONN, NULL);

    link->link_state = LINK_DISCONNECTED;
    link->dial_wait_time =0;

    CMPR_cleanup (link);

    if (link->conn_activity != NO_ACTIVITY)
    	CMPR_send_response (link, 
			&(link->req[(int)link->conn_req_ind]), ret_value);

    return;
}

/**************************************************************************

    Description: This function performs clean-up work on a failed or
		discarded connetion. It terminates all unfinished write 
		request and discards all incomplete incoming data.

    Inputs:	link - the link involved.

**************************************************************************/

void CMPR_cleanup (Link_struct *link)
{
    int ind, i;

    while ((ind = Pending_request (link, CM_WRITE, -1)) >= 0) {
	if (link->req[ind].state > CM_DONE)
	    CMPR_send_response (link, &(link->req[ind]), CM_DISCONNECTED);
	link->req[ind].state = CM_DONE;
	if (Additional_processing != NULL)
	    Additional_processing (link);
    }

    for (i = 0; i < link->n_pvc; i++)
	link->r_cnt[i] = 0;

    link->rep_period = 0;

    CMRATE_reset (link);

    return;
}

/********************************************************************

    Compresses outgoing data for "pvc" of "link". The request is "req".
    Returns 0.

********************************************************************/

static int Compress_data (Link_struct *link, int pvc, Req_struct *req) {
    static char *dest = NULL;
    char *w_buf;
    int method, clen, org_length;

    method = link->compress_method;
    link->w_compressed = 0;
    if (method < 0 && req->state != CM_NEW_PACKED)
	return (0);
    org_length = link->w_size[pvc];
    w_buf = link->w_buf[pvc];

    if (method >= 0 && org_length > 256) {
	dest = STR_reset (dest, org_length);
	if (req->state == CM_NEW_PACKED)
	    clen = MISC_compress (method, w_buf + sizeof (Compress_hd_t), 
			org_length - sizeof (Compress_hd_t), dest, org_length);
	else
	    clen = MISC_compress (method, w_buf, org_length, dest, org_length);
    }
    else
	clen = -1;
    if (clen > 0 && clen + sizeof (Compress_hd_t) <= org_length) {
	Compress_hd_t *hd = (Compress_hd_t *)w_buf;
	hd->method = method;
	if (req->state == CM_NEW_PACKED)
	    hd->packed = 1;
	else {
	    hd->packed = 0;
	    org_length += sizeof (Compress_hd_t);
	}
	hd->org_length = htonl (org_length);
	memcpy (w_buf + sizeof (Compress_hd_t), dest, clen);
	clen += sizeof (Compress_hd_t);
	LE_send_msg (LE_VL2, "    Outgoing payload compressed (%d->%d)", 
		org_length, clen);
	link->org_bytes += org_length;
	link->w_size[pvc] = clen;
	link->comp_bytes += clen;
	link->w_compressed = 1;
    }
    else if (req->state == CM_NEW_PACKED) {
	Compress_hd_t *hd = (Compress_hd_t *)(link->w_buf[pvc]);
	hd->method = -1;
	hd->packed = 1;
 	hd->org_length = 0;
	link->w_compressed = 1;
	LE_send_msg (LE_VL2, "    Outgoing pack not compressed");
    }

    return (0);
}
