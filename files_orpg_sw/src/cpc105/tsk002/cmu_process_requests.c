
/******************************************************************

	file: cmu_process_requests.c

	This module contains the functions that process the requests
	- UCONX version.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 1998/01/30 22:53:42 $
 * $Id: cmu_process_requests.c,v 1.5 1998/01/30 22:53:42 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <comm_manager.h>
#include <orpgevt.h>
#include "cmu_def.h"

extern int CMU_verbose;		/* defined in the main module */

static int Cr_time;		/* current time */

static int N_req_processed = 0; /* number of requests processed */


/* local functions */
static int Search_req (Link_struct *link);
static int Pending_request (Link_struct *link, int type, int ind);
static void Process_connect (Link_struct *link, int ind);
static void Process_disconnect (Link_struct *link, int ind);
static void Process_dial_out (Link_struct *link, int ind);
static void Process_write (Link_struct *link, int ind);
static void Connection_procedure (Link_struct *link);
static void Process_status (Link_struct *link, int ind);


/**************************************************************************

    Description: This function processes any pending requests.

    Return:	It returns the total number of requests processed by this
		process.

**************************************************************************/

int PR_process_requests (int nlinks, Link_struct **links)
{
    int cnt, i;

    cnt = 0;
    Cr_time = time (NULL);
    for (i = 0; i < nlinks; i++) {
	Req_struct *req;
	int ind;

	if ((ind = Search_req (links[i])) < 0)
	    continue;

	cnt++;
	req = &(links[i]->req[ind]);
	if (CMU_verbose && req->type != CM_WRITE)
	    LE_send_msg (0, "request %d received on link %d", 
				req->type, links[i]->link_ind);
	switch (req->type) {

	    case CM_CONNECT:
		if (links[i]->link_type == DIAL_IN_OUT)
		    PR_send_response (links[i], req, CM_NOT_CONFIGURED);
		else 
		    Process_connect (links[i], ind);
		break;

	    case CM_DIAL_OUT:
		if (links[i]->link_type != DIAL_IN_OUT)
		    PR_send_response (links[i], req, CM_NOT_CONFIGURED);
		else
		    Process_dial_out (links[i], ind);
		break;

	    case CM_DISCONNECT:
		Process_disconnect (links[i], ind);
		break;

	    case CM_WRITE:
		Process_write (links[i], ind);
		break;

	    case CM_STATUS:
		Process_status (links[i], ind);
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

void PR_send_response (Link_struct *link, Req_struct *req, int ret_code)
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
	    cpt += SH_align_bytes (link->n_added_bytes);
	for (pvc = 0; pvc < link->n_pvc; pvc++)
	    if (cpt == link->w_buf[pvc])
		break;
	if (pvc < link->n_pvc)
	    link->w_buf[pvc] = NULL;

	if (req->data != NULL)
	    free (req->data);
	req->data = NULL;
    }

    if ((req->type == CM_CONNECT || req->type == CM_DISCONNECT) &&
	ret_code != CM_IN_PROCESSING)
	link->conn_activity = NO_ACTIVITY;

    /* send the response message */
    resp.type = req->type;
    resp.req_num = req->req_num;
    resp.link_ind = req->link_ind;
    resp.ret_code = ret_code;
    resp.data_size = 0;
    resp.time = time (NULL);

    ret = LB_write (link->respfd, (char *)&resp, 
				sizeof (CM_resp_struct), LB_NEXT);
    if (ret != sizeof (CM_resp_struct)) {
	LE_send_msg (0, "LB_write (response) failed (ret = %d)", ret);
	CM_terminate ();
    }
    EN_post (ORPGEVT_NB_COMM_RESP + link->link_ind, NULL, 0, 1);

    if (CMU_verbose && req->type != CM_WRITE)
	LE_send_msg (0, "response of type %d sent on link %d", 
				req->type, link->link_ind);

    req->state = CM_DONE;
    N_req_processed++;

    return;
}

/**************************************************************************

    Description: This function sends a received data message to the user.

    Inputs:	link - the link structure corresponding to the request;
		pvc - the PVC index;
		len - data length;
		data - pointer to the data;

**************************************************************************/

void PR_send_data (Link_struct *link, int pvc, int len, char *data)
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
    resp->data_size = len;
    resp->time = time (NULL);

    if (link->n_added_bytes != 0) {
	cpt = data - sizeof (CM_resp_struct) - link->n_added_bytes;
	memcpy (cpt, (char *)&tmp_resp, sizeof (CM_resp_struct));
    }
    else
	cpt = data - sizeof (CM_resp_struct);

    ret = LB_write (link->respfd, cpt, 
				sizeof (CM_resp_struct) + len, LB_NEXT);
    if (ret != sizeof (CM_resp_struct) + len) {
	LE_send_msg (0, "LB_write (send data) failed (ret = %d)", ret);
	CM_terminate ();
    }
    if (link->data_en)
	EN_post (ORPGEVT_NB_COMM_RESP + link->link_ind, NULL, 0, 1);

    return;
}

/**************************************************************************

    Description: This function sends a CM_LOST_CONN message to the user.

    Inputs:	link - the link structure corresponding to the request;
		ret_code - event code.

**************************************************************************/

void PR_send_event_response (Link_struct *link, int ret_code)
{
    CM_resp_struct resp;
    int ret;

    /* send the response message */
    resp.type = CM_EVENT;
    resp.link_ind = link->link_ind;
    resp.ret_code = ret_code;
    resp.time = time (NULL);

    ret = LB_write (link->respfd, (char *)&resp, 
				sizeof (CM_resp_struct), LB_NEXT);
    if (ret != sizeof (CM_resp_struct)) {
	LE_send_msg (0, "LB_write (response) failed (ret = %d)", ret);
	CM_terminate ();
    }
    EN_post (ORPGEVT_NB_COMM_RESP + link->link_ind, NULL, 0, 1);

    if (CMU_verbose)
	LE_send_msg (0, "event of code %d sent on link %d", 
				ret_code, link->link_ind);

    return;
}

/**************************************************************************

    Description: This function processes the CM_DIAL_OUT request.

    Inputs:	link - the link structure corresponding to the request;
		req - the CM_CONNECT request;

**************************************************************************/

static void Process_dial_out (Link_struct *link, int ind)
{
    Req_struct *req;

    req = &(link->req[ind]);
fprintf (stderr, "CM_DIAL_OUT is not implemented yet\n");

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
    if (link->link_state == LINK_CONNECTED)
	PR_send_response (link, req, CM_CONNECTED);
    else 
	PR_send_response (link, req, CM_DISCONNECTED);

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
	link->w_buf[pvc] += SH_align_bytes (link->n_added_bytes);
    link->w_size[pvc] = req->data_size - link->n_added_bytes;
    link->w_cnt[pvc] = 0;
    link->w_ack[pvc] = 0;
    link->w_req_ind[pvc] = ind;

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

    req = &(link->req[ind]);

    if (link->conn_activity == CONNECTING ||
	link->conn_activity == DISCONNECTING) {
	PR_send_response (link, req, CM_IN_PROCESSING);
	return;
    }

    if (link->link_state == LINK_CONNECTED) {
	PR_send_response (link, req, CM_SUCCESS);
	return;
    }

    if (CMU_verbose)
	LE_send_msg (0, "process connect on link %d", link->link_ind);

    req->state = Cr_time;
    link->conn_activity = CONNECTING;
    link->conn_req_ind = ind;
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
	PR_send_response (link, &(link->req[i]), CM_TERMINATED);

    if (link->conn_activity != NO_ACTIVITY) {
	PR_send_response (link, req, CM_IN_PROCESSING);
	return;
    }

    if (CMU_verbose)
	LE_send_msg (0, "process disconnect on link %d", link->link_ind);

    req->state = Cr_time;
    link->conn_activity = DISCONNECTING;
    link->conn_req_ind = ind;
    Connection_procedure (link);

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
		processed. The cenceled requests are removed from the "req" 
		list.

    Inputs:	link - the link structure corresponding to the request;
		new_req - the CM_CANCEL request;

**************************************************************************/

void PR_process_cancel (Link_struct *link, Req_struct *new_req)
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
		    cpt += SH_align_bytes (link->n_added_bytes);
		for (k = 0; k < link->n_pvc; k++) {
		    if (cpt == link->w_buf[k])
			link->w_buf[k] = NULL;
		}
		if (req->data != NULL) {
		    free (req->data);
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
    PR_send_response (link, new_req, cnt);

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

	if (req[ind].state == CM_NEW) {

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

    Description: This function is called when a link is disconnected. It
		terminate all unfinished write requests on that link and
		discard all incomplete incoming data.

    Inputs:	link - the link involved.

**************************************************************************/

void PR_disconnect_cleanup (Link_struct *link)
{
    int ind, i;

    while ((ind = Pending_request (link, CM_WRITE, -1)) >= 0) {
	if (link->req[ind].state > CM_DONE)
	    PR_send_response (link, &(link->req[ind]), CM_DISCONNECTED);
	link->req[ind].state = CM_DONE;
    }

    for (i = 0; i < link->n_pvc; i++)
	link->r_cnt[i] = 0;

    return;
}


