
/******************************************************************

	file: read_request.c

	This module reads in the new requests.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 1997/12/19 23:36:21 $
 * $Id: cmu_read_request.c,v 1.3 1997/12/19 23:36:21 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <comm_manager.h>
#include "cmu_def.h"


/* local functions */
static int Check_request (Link_struct *link, CM_req_struct *req);
static void Rm_done_requests (Link_struct *link);


/**************************************************************************

    Description: This function reads all new requests from the request LBs.
		The requests for each link is sorted in terms of their
		priorities. If a request can not be read because the "req"
		array is full for a link, this and all following requests
		are left untouched in the LB. CM_CANCEL request is processed
		here. If a read error is encountered, this function sends 
		and LE message and terminates.

    Inputs:	n_links - number of links;
		links - the link structure list;

    Return:	It returns the total number of requests read.

**************************************************************************/

int RR_get_requests (int n_links, Link_struct **links)
{
    int req_cnt;
    int i;

    req_cnt = 0;
    for (i = 0; i < n_links; i++) {

	if (links[i]->reqfd < 0)
	    continue;

	while (1) {
	    Req_struct req;
	    CM_req_struct *cmreq;
	    int ind, k, ret;

	    cmreq = (CM_req_struct *)&(req.type);
	    ret = LB_read (links[i]->reqfd, (char *)cmreq, 
					sizeof (CM_req_struct), LB_NEXT);
	    if (ret == LB_TO_COME)
		break;
	    if (ret < 0) {
		if (ret == LB_BUF_TOO_SMALL) {
		    if (req.type != CM_WRITE || req.data_size <= 0) {
			LE_send_msg (0, "incorrect request length");
			continue;
		    }
		}
		else {
		    LE_send_msg (0, 
			"LB_read request msg failed (ret = %d)", ret);
		    CM_terminate ();
		}
	    }

	    /* check the link number */
	    ind = req.link_ind;
	    for (k = 0; k < n_links; k++)
		if (ind == links[k]->link_ind)
		    break;
	    if (k >= n_links) {		/* request not due this manager */
		LE_send_msg (0, 
			"request with wrong link index (%d) - ignored", ind);
		continue;
	    }

	    req.state = CM_NEW;
	    req.data = NULL;
	    req.msg_id = LB_previous_msgid (links[i]->reqfd);

	    /* check the request parameters */
	    if (Check_request (links[k], cmreq) != 0) {
		PR_send_response (links[k], &req, CM_INVALID_PARAMETER);
		continue;
	    }

	    /* process CM_CANCEL */
	    if (req.type == CM_CANCEL) {
		PR_process_cancel (links[k], &req);
		continue;
	    }

	    /* remove all finished requests */
	    Rm_done_requests (links[k]);

	    /* check if there is room for the request */
	    if (links[k]->n_reqs >= MAX_N_REQS) {
					/* too many pending requests */
		PR_send_response (links[k], &req, CM_TOO_MANY_REQUESTS);
		continue;
	    }

	    /* read the full message */
	    if (req.type == CM_WRITE) {
		char *buf;
		int size, read_size;
		int offset;

		size = sizeof (CM_req_struct) + req.data_size;
		read_size = size;
		if (links[k]->n_added_bytes > 0)
		    size += SH_align_bytes (links[k]->n_added_bytes);
		buf = malloc (size);
		if (buf == NULL) {
		    LE_send_msg (0, "malloc failed (size = %d)", size);
		    CM_terminate ();
		}
		LB_seek (links[i]->reqfd, -1, LB_CURRENT, NULL);
		offset = SH_align_bytes (links[k]->n_added_bytes) - 
						links[k]->n_added_bytes;
		    
		ret = LB_read (links[i]->reqfd, buf + offset, 
			read_size, LB_NEXT);
		if (ret != read_size) {
		    LE_send_msg (0, "Bad request (type CM_WRITE) - ignored");
		    continue;
		}
		req.data = buf;
		if (links[k]->n_added_bytes != 0)
		    memcpy (buf, cmreq, sizeof (CM_req_struct));
	    }

	    /* put the request in the list */
	    memcpy (
	      &(links[k]->req[(links[k]->st_ind + links[k]->n_reqs) % MAX_N_REQS]), 
						&req, sizeof (Req_struct));
	    links[k]->n_reqs++;

	    req_cnt++;
	}
    }

    return (req_cnt);
}

/**************************************************************************

    Description: This function remove all request in the request list that
		are marked as "CM_DONE".

    Inputs:	link - the link structure;

**************************************************************************/

static void Rm_done_requests (Link_struct *link)
{
    int ind, i, cnt;

    /* remove those that do not need copy */
    cnt = 0;
    ind = link->st_ind - 1;
    for (i = 0; i < link->n_reqs; i++) {
	Req_struct *req;

	ind = (ind + 1) % MAX_N_REQS;
	req = &(link->req[ind]);
	if (req->state == CM_DONE) 		/* done */
	    cnt++;
	else
	    break;
    }
    link->st_ind = (link->st_ind + cnt) % MAX_N_REQS;
    link->n_reqs -= cnt;

    /* remove those that need copy */
    ind = link->st_ind - 1;
    for (i = 0; i < link->n_reqs; i++) {
	Req_struct *req;

	ind = (ind + 1) % MAX_N_REQS;
	req = &(link->req[ind]);
	if (req->state == CM_DONE) {		/* done */
	    int rm_ind, k;
	
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
	}
    }

    return;
}

/**************************************************************************

    Description: This function checks whether or not parameters in a 
		request are legal.

    Inputs:	link - link structure corresponding to the request;
		req - the request to be checked;

    Return:	returns 0 if no error is found or -1 is an error is found.

**************************************************************************/

static int Check_request (Link_struct *link, CM_req_struct *req)
{

    if (req->type < CM_CONNECT || req->type >= CM_DATA)
	return (-1);

    if (req->type == CM_WRITE) {
	if (link->proto == PROTO_HDLC)
	    req->parm = 1;
	if (req->parm < 1 || req->parm > link->n_pvc ||	/* priority */
	    req->data_size <= 0)		/* data size */
	    return (-1);
	if (req->data_size > link->packet_size)
	    return (-1);
    }

    return (0);
}

