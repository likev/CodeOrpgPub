
/******************************************************************

	file: read_request.c

	This module reads in the new requests.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/04/22 14:20:31 $
 * $Id: cmc_read_request.c,v 1.21 2013/04/22 14:20:31 steves Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>

#include <infr.h>
#include <comm_manager.h>
#include <cmc_def.h>

static int Need_delayed_free = 0;

/* local functions */
static int Check_request (Link_struct *link, CM_req_struct *req);
static void Rm_done_requests (Link_struct *link);
static int Add_pack_msg (Link_struct *link, Req_struct *req);
static int Add_pack_msg_no_seek (Link_struct *link, Req_struct *req,
						char *msgp, int msize);
static void Pack_unsent_message (Link_struct *link, int fd);
static void Pack_unsent_message_no_seek (Link_struct *link);
static int Lpw_done (Link_struct *link);


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

    Return:	It returns the total number of requests read or a negative 
		error number.

    This function uses LB_seek when the request LB is not LB_MUST_READ.
    Otherwise, each message is read only once and the messages to pack are
    stored in a buffer. In the latter case, reading requests is stopped when
    the packing buffer is full.

**************************************************************************/

int CMRR_get_requests (int n_links, Link_struct **links)
{
    static char *msgp = NULL;
    int req_cnt;
    int i;
/* static int cnt = 0; */

    req_cnt = 0;
    for (i = 0; i < n_links; i++) {
	Link_struct *linki;

	linki = links[i];
	if (linki->link_state != LINK_CONNECTED)
	    linki->pack_cnt = 0;
	if (linki->pack_cnt > 0) {
	    if (linki->must_read) {
		if (Lpw_done (linki))
		    Pack_unsent_message_no_seek (linki);
		if (linki->pack_cnt == 0)	/* packing performed */
		    req_cnt++;
/* LE_send_msg (0, "  pack_cnt %d  max_pack %d", linki->pack_cnt, linki->max_pack); */
		if (linki->pack_cnt >= linki->max_pack)
		    return (req_cnt);	/* stop reading request */
	    }
	    else {
		int n_reqs = linki->n_reqs;
		int reqfd = linki->reqfd;
		if (reqfd < 0)
		    reqfd = -reqfd;
		if (Lpw_done (linki))
		    Pack_unsent_message (linki, reqfd);
		if (linki->n_reqs > n_reqs)
		    req_cnt++;
	    }
	}
    }

    for (i = 0; i < n_links; i++) {
	Link_struct *linki;
	int reqfd;

	linki = links[i];
	reqfd = linki->reqfd;
	if (reqfd < 0)
	    continue;

	while (1) {
	    Req_struct req;
	    CM_req_struct *cmreq;
	    int ind, k, ret, msize;
	    Link_struct *link;

	    if (msgp != NULL)
		free (msgp);
	    msgp = NULL;
	    cmreq = (CM_req_struct *)&(req.type);
	    if (linki->must_read)
		ret = LB_read (reqfd, (char *)&msgp, 
					LB_ALLOC_BUF, LB_NEXT);
	    else
		ret = LB_read (reqfd, (char *)cmreq, 
					sizeof (CM_req_struct), LB_NEXT);
/* if (ret != LB_TO_COME)
   LE_send_msg (0, "  LB_read read request ret %d", ret); */
	    if (ret == LB_TO_COME)
		break;
	    req_cnt++;
	    if (ret == LB_EXPIRED) {
		LE_send_msg (GL_ERROR, 
		"Cannot catch up with the request flow - request buffer too small");
		CM_terminate ();
	    }
	    if (!linki->must_read) {
		for (k = 0; k < n_links; k++) {
		    Link_struct *linkk;
		    int fd;
		    linkk = links[k];
		    fd = linkk->reqfd;
		    if (fd < 0)
			fd = -fd;
		    if (linkk->pack_cnt > 0 && fd == reqfd)
			(linkk->pack_cnt)++;
		}
	    }
	    if (msgp != NULL && ret >= sizeof (CM_req_struct))
		memcpy (cmreq, msgp, sizeof (CM_req_struct));
	    if (ret == LB_BUF_TOO_SMALL || ret > sizeof (CM_req_struct)) {
		if ((req.type != CM_WRITE && req.type != CM_SET_PARAMS &&
		    req.type != CM_DIAL_OUT && req.type != CM_CONNECT)
		    || req.data_size <= 0) {
		    LE_send_msg (GL_ERROR,  "incorrect request length ret %d req.type %d req.data_size %d", ret, req.type, req.data_size);
		    continue;
		}
	    }
	    else if (ret < sizeof (CM_req_struct)) {
		LE_send_msg (GL_ERROR | 42, 
		    "LB_read request msg failed (ret = %d)", ret);
		CM_terminate ();
	    }
	    msize = ret;

	    /* check the link number */
	    ind = req.link_ind;
	    for (k = 0; k < n_links; k++)
		if (ind == links[k]->link_ind)
		    break;
	    if (k >= n_links) {		/* request not due this manager */
		LE_send_msg (GL_ERROR | 43,  
			"request with wrong link index (%d) - ignored", ind);
		continue;
	    }
	    link = links[k];

	    req.state = CM_NEW;
	    req.data = NULL;
	    req.msg_id = LB_previous_msgid (reqfd);
/* cnt++;
   if ((cnt % 200) > 100)
   msleep (100); */

	    /* check the request parameters */
	    if (Check_request (link, cmreq) != 0) {
		LE_send_msg (GL_ERROR | 44,  
			"request with invalid type or parameter");
		CMPR_send_response (link, &req, CM_INVALID_PARAMETER);
		continue;
	    }

	    /* process CM_CANCEL */
	    if (req.type == CM_CANCEL) {
		CMPR_process_cancel (link, &req);
		continue;
	    }

	    if (link->pack_time > 0 && 
		req.type == CM_WRITE && req.parm == link->n_pvc) {
		if (link->must_read) {
		    Add_pack_msg_no_seek (link, &req, msgp, msize);
/* LE_send_msg (0, "       pack_cnt %d\n", link->pack_cnt); */
		    if (link->pack_cnt >= link->max_pack)
			return (req_cnt);
		}
		else
		    Add_pack_msg (link, &req);
		continue;
	    }

	    /* remove all finished requests */
	    Rm_done_requests (link);

	    /* check if there is room for the request */
	    if (link->n_reqs >= MAX_N_REQS) {
					/* too many pending requests */
		CMPR_send_response (link, &req, CM_TOO_MANY_REQUESTS);
		continue;
	    }

	    /* read the full message */
	    if (req.type == CM_WRITE || req.type == CM_SET_PARAMS ||
		req.type == CM_DIAL_OUT || 
		(req.type == CM_CONNECT && 
		  (ret == LB_BUF_TOO_SMALL || ret > sizeof (CM_req_struct)))) {
		char *buf;
		int size, read_size;
		int offset;

		size = sizeof (CM_req_struct) + req.data_size;
		read_size = size;
		if (link->n_added_bytes > 0)
		    size += CMC_align_bytes (link->n_added_bytes);
		buf = malloc (size);
		if (buf == NULL) {
		    LE_send_msg (GL_ERROR, "malloc failed (size = %d)", size);
		    CM_terminate ();
		}
		offset = CMC_align_bytes (link->n_added_bytes) - 
						link->n_added_bytes;

		if (msgp == NULL) {
		    LB_seek (reqfd, -1, LB_CURRENT, NULL);
		    ret = LB_read (reqfd, buf + offset, read_size, LB_NEXT);
		}
		else
		    ret = msize;
		if (ret != read_size) {
		    LE_send_msg (LE_VL0 | 46,  
				"Bad request (type %d) - ignored", req.type);
		    CMC_free (buf);
		    continue;
		}
		if (msgp != NULL)
		    memcpy (buf + offset, msgp, read_size);
		if (req.type == CM_CONNECT)
		    buf[offset + read_size - 1] = '\0';
		req.data = buf;
		if (link->n_added_bytes != 0)
		    memcpy (buf, cmreq, sizeof (CM_req_struct));
	    }

	    /* put the request in the list */
	    memcpy (
	      &(link->req[(link->st_ind + link->n_reqs) % 
			MAX_N_REQS]), &req, sizeof (Req_struct));
	    link->n_reqs++;
	}
    }

    return (req_cnt);
}

/**********************************************************************

    Reads unsent low priority write requests and creates a request for 
    "link" if the packing time is expired. link->pack_cnt > 0 when called.
    If more than one message packed, the data is in the packed format:
     CM_req_struct - compr_header - n_message - messages - message lengths.
    If only one message exists, the data is not packed: CM_req_struct - 
    message. The packed data is marked by req.state = CM_NEW_PACKED
    instead of CM_NEW.

**********************************************************************/

static void Pack_unsent_message (Link_struct *link, int fd) {
    static char *msg_sizes = NULL;
    int cnt, len0, data_size, cmp_h_s, msg_expired, sb_msgs;
    Req_struct req;
    char *p0;

    if (link->pack_time > 1) {
	int cr_ms;
	time_t cr_tm = MISC_systime (&cr_ms);
	if (((double)cr_tm + .001 * cr_ms) - link->pack_st_time < 
						    .001 * link->pack_time)
	    return;
    }

    if (link->n_added_bytes > 0) {
	LE_send_msg (GL_ERROR, 
		"n_added_bytes > 0 and pack_time > 0 not supported");
	CM_terminate ();
    }

    Rm_done_requests (link);
    if (link->n_reqs >= MAX_N_REQS) {
	LE_send_msg (GL_ERROR, "Too many unprocessed requests");
	return;
    }

    LB_seek (fd, -(link->pack_cnt), LB_CURRENT, NULL);
    sb_msgs = link->pack_cnt;

    cmp_h_s = CMPR_comp_hd_size () + sizeof (int);
    link->pack_buf = STR_reset (link->pack_buf, 40000);
    link->pack_buf = STR_append (link->pack_buf, NULL, 
				sizeof (CM_req_struct) + cmp_h_s);
    msg_sizes = STR_reset (msg_sizes, 128);
    data_size = 0;
    msg_expired = 0;
    cnt = len0 = 0;
    p0 = NULL;
    while (1) {
	CM_req_struct *cmreq;
	int len, tmp;
	char *p;
	len = LB_read (fd, &p, LB_ALLOC_BUF, LB_NEXT);
	if (len == LB_EXPIRED) {
	    link->pack_cnt--;
	    msg_expired = 1;
	    if (link->pack_cnt == 0)
		break;
	    continue;
	}
	else if (len == LB_TO_COME) {
	    LE_send_msg (GL_ERROR, "LB_read pack - unexpected LB_TO_COME");
	    link->pack_cnt = 0;
	    break;
	}
	else if (len < sizeof (CM_req_struct)) {
	    LE_send_msg (GL_ERROR, "LB_read msg to pack failed (%d)", len);
	    CM_terminate ();
	}
	link->pack_cnt--;
	cmreq = (CM_req_struct *)p;
	if (cmreq->link_ind != link->link_ind ||
	    cmreq->type != CM_WRITE || cmreq->parm != link->n_pvc) {
	    CMC_free (p);
	    continue;
	}

	len -= sizeof (CM_req_struct);
	if (cnt == 0)
	    link->pack_buf = STR_append (link->pack_buf, NULL, len);
	else
	    link->pack_buf = STR_append (link->pack_buf, 
					p + sizeof (CM_req_struct), len);
	data_size += len;
	tmp = htonl (len);
	msg_sizes = STR_append (msg_sizes, (char *)&tmp, sizeof (int));

	memcpy (&(req.type), p, sizeof (CM_req_struct));
	if (cnt == 0) {
	    p0 = p;
	    len0 = data_size;
	}
	else
	    CMC_free (p);

	cnt++;
	if (cnt >= link->max_pack)
	    break;
	if (link->pack_cnt == 0)
	    break;
    }
    if (cnt > 1)
	LE_send_msg (LE_VL3, "    Seek back %d, pack %d, seek forward %d", 
					sb_msgs, cnt, link->pack_cnt);
    else
	LE_send_msg (LE_VL3, "    Seek back %d, read %d, seek forward %d", 
					sb_msgs, cnt, link->pack_cnt);

    if (link->pack_cnt > 0)
	LB_seek (fd, link->pack_cnt, LB_CURRENT, NULL);

    if (msg_expired) {
	LE_send_msg (GL_ERROR, "Message to pack is expired");
	CMPR_send_event_response (link, CM_BUFFER_OVERFLOW, NULL);
    }
    if (cnt == 0)
	return;

    if (cnt == 1) {
	memcpy (link->pack_buf, p0, data_size + sizeof (CM_req_struct));
	req.state = CM_NEW;
    }
    else {
	memcpy (link->pack_buf + sizeof (CM_req_struct) + cmp_h_s, 
					p0 + sizeof (CM_req_struct), len0);
	*((int *)(link->pack_buf + 
	    sizeof (CM_req_struct) + cmp_h_s - sizeof (int))) = htonl (cnt);
	link->pack_buf = STR_append (link->pack_buf, msg_sizes, 
						    cnt * sizeof (int));
	data_size += cnt * sizeof (int) + cmp_h_s;
	req.data_size = data_size;
	memcpy (link->pack_buf, &(req.type), sizeof (CM_req_struct));
	req.state = CM_NEW_PACKED;
    }
    req.data = link->pack_buf;
    req.msg_id = LB_ANY;
    if (p0 != NULL)
	CMC_free (p0);

    memcpy (&(link->req[(link->st_ind + link->n_reqs) % MAX_N_REQS]), 
					&req, sizeof (Req_struct));
    link->n_reqs++;
    return;
}

/**********************************************************************

    Processes a low priority write "req" for packing.

**********************************************************************/

static int Add_pack_msg (Link_struct *link, Req_struct *req) {
    int ret;
    CM_resp_struct resp;
    extern int CMC_comm_resp_event;	/* defined in cmc_common.c */ 

    if (link->pack_cnt == 0) {		/* set start time */
	int ms;
	time_t cr_t = MISC_systime (&ms);
	link->pack_st_time = (double)cr_t + .001 * ms;
	link->pack_cnt = 1;
    }

    resp.type = req->type;
    resp.req_num = req->req_num;
    resp.link_ind = req->link_ind;
    resp.ret_code = CM_SUCCESS;
    resp.data_size = 0;
    resp.time = time (NULL);

    ret = LB_write (link->respfd, (char *)&resp, 
				sizeof (CM_resp_struct), LB_NEXT);
    if (ret != sizeof (CM_resp_struct)) {
	LE_send_msg (GL_ERROR,  "LB_write (response 1) failed (ret = %d)", ret);
	CM_terminate ();
    }
    if (CMC_get_en_flag () >= 0)
	EN_post (CMC_comm_resp_event + link->link_ind, 
					NULL, 0, CMC_get_en_flag ());
    return (1);
}

/**********************************************************************

    Packs unsent low priority write requests and creates a request for
    "link" if the packing time is expired or the number of saved
    messages reaches the maximum. If more than one message is packed,
    the data is in the packed format: CM_req_struct - compr_header -
    n_message - messages - message lengths. If only one message exists,
    the data is not packed: CM_req_struct - message. The packed data is
    marked by req.state = CM_NEW_PACKED instead of CM_NEW.

    This version does not use LB_seek so we can support LB_MUST_READ
    of the request LB. This however stops processing other links when
    comms on one of the links is blocked.

**********************************************************************/

static void Pack_unsent_message_no_seek (Link_struct *link) {
    static char *msg_sizes = NULL;
    int cnt, len0, data_size, cmp_h_s;
    Req_struct req;
    char *p0, *msgs;

    if (link->pack_time > 1) {
	int cr_ms;
	time_t cr_tm = MISC_systime (&cr_ms);
	if (link->pack_cnt < link->max_pack &&
	    ((double)cr_tm + .001 * cr_ms) - link->pack_st_time < 
						    .001 * link->pack_time)
	    return;
    }

    if (link->n_added_bytes > 0) {
	LE_send_msg (GL_ERROR, 
		"n_added_bytes > 0 and pack_time > 0 not supported");
	CM_terminate ();
    }

    Rm_done_requests (link);
    if (link->n_reqs >= MAX_N_REQS) {
	LE_send_msg (GL_ERROR, "Too many unprocessed requests");
	return;
    }

    cmp_h_s = CMPR_comp_hd_size () + sizeof (int);
    link->pack_buf = STR_reset (link->pack_buf, 40000);
    link->pack_buf = STR_append (link->pack_buf, NULL, 
				sizeof (CM_req_struct) + cmp_h_s);
    msg_sizes = STR_reset (msg_sizes, 128);
    data_size = 0;
    cnt = len0 = 0;
    p0 = NULL;
    msgs = link->saved_msgs;
    while (cnt < link->pack_cnt) {
	int len, tmp;
	char *p;

	p = msgs + sizeof (int);
	len = *((int *)msgs);
	msgs = msgs + ALIGNED_SIZE (len) + sizeof (int);

	len -= sizeof (CM_req_struct);
	if (cnt == 0)
	    link->pack_buf = STR_append (link->pack_buf, NULL, len);
	else
	    link->pack_buf = STR_append (link->pack_buf, 
					p + sizeof (CM_req_struct), len);
	data_size += len;
	tmp = htonl (len);
	msg_sizes = STR_append (msg_sizes, (char *)&tmp, sizeof (int));

	memcpy (&(req.type), p, sizeof (CM_req_struct));
	if (cnt == 0) {
	    p0 = p;
	    len0 = data_size;
	}

	cnt++;
    }

    if (cnt == 1) {
	memcpy (link->pack_buf, p0, data_size + sizeof (CM_req_struct));
	req.state = CM_NEW;
    }
    else {
	memcpy (link->pack_buf + sizeof (CM_req_struct) + cmp_h_s, 
					p0 + sizeof (CM_req_struct), len0);
	*((int *)(link->pack_buf + 
	    sizeof (CM_req_struct) + cmp_h_s - sizeof (int))) = htonl (cnt);
	link->pack_buf = STR_append (link->pack_buf, msg_sizes, 
						    cnt * sizeof (int));
	data_size += cnt * sizeof (int) + cmp_h_s;
	req.data_size = data_size;
	memcpy (link->pack_buf, &(req.type), sizeof (CM_req_struct));
	req.state = CM_NEW_PACKED;
    }
    req.data = link->pack_buf;
    req.msg_id = LB_ANY;

    memcpy (&(link->req[(link->st_ind + link->n_reqs) % MAX_N_REQS]), 
					&req, sizeof (Req_struct));
    link->n_reqs++;
    LE_send_msg (LE_VL3, "    %d messages packed", link->pack_cnt);

    link->pack_cnt = 0;
    return;
}

/**********************************************************************

    Processes a low priority write "req" for packing.

    This version does not use LB_seek. Refer to another *_no_seek.

**********************************************************************/

static int Add_pack_msg_no_seek (Link_struct *link, Req_struct *req,
						char *msgp, int msize) {
    int ret, size;
    CM_resp_struct resp;
    extern int CMC_comm_resp_event;	/* defined in cmc_common.c */ 

    if (link->pack_cnt == 0) {		/* set start time */
	int ms;
	time_t cr_t = MISC_systime (&ms);
	link->pack_st_time = (double)cr_t + .001 * ms;
/*	if (link->saved_msgs == NULL) */
	    link->saved_msgs = STR_reset (link->saved_msgs, 40000);
	link->n_saved_bytes = 0;
    }

    /* read the full message and save it */
    size = sizeof (CM_req_struct) + req->data_size;
    link->saved_msgs = STR_append (link->saved_msgs, NULL, 
				ALIGNED_SIZE (size) + sizeof (int));
    if (msgp == NULL) {
	LB_seek (link->reqfd, -1, LB_CURRENT, NULL);
	ret = LB_read (link->reqfd,
		link->saved_msgs + link->n_saved_bytes + sizeof (int),
							size, LB_NEXT);
    }
    else
	ret = msize;

    if (ret != size) {
	LE_send_msg (LE_VL0, "Bad request (type %d) - ignored", req->type);
	return (0);
    }
    if (msgp != NULL)
	memcpy (link->saved_msgs + link->n_saved_bytes + sizeof (int),
								msgp, size);

    *((int *)(link->saved_msgs + link->n_saved_bytes)) = size;
    link->n_saved_bytes += ALIGNED_SIZE (size) + sizeof (int);
    link->pack_cnt++;

    resp.type = req->type;
    resp.req_num = req->req_num;
    resp.link_ind = req->link_ind;
    resp.ret_code = CM_SUCCESS;
    resp.data_size = 0;
    resp.time = time (NULL);

    ret = LB_write (link->respfd, (char *)&resp, 
				sizeof (CM_resp_struct), LB_NEXT);
    if (ret != sizeof (CM_resp_struct)) {
	LE_send_msg (GL_ERROR,  "LB_write (response 1) failed (ret = %d)", ret);
	CM_terminate ();
    }
    if (CMC_get_en_flag () >= 0)
	EN_post (CMC_comm_resp_event + link->link_ind, 
					NULL, 0, CMC_get_en_flag ());
    return (1);
}

/**********************************************************************

    Processes a request "req" that to be packed for link "link".

**********************************************************************/

static int Lpw_done (Link_struct *link) {
    int ind, i;

    if (link->pack_time <= 0)
	return (1);
    ind = link->st_ind - 1;
    for (i = 0; i < link->n_reqs; i++) {
	Req_struct *req;
	ind = (ind + 1) % MAX_N_REQS;
	req = &(link->req[ind]);
	if (req->type == CM_WRITE && req->state != CM_DONE &&
			req->parm == link->n_pvc)	/* LPW in progress */
	    return (0);
    }
    return (1);
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

    if (req->type < CM_CONNECT || req->type > CM_SET_PARAMS ||
	req->type == CM_DATA || req->type == CM_EVENT)
	return (-1);

    if (req->type == CM_WRITE) {
	if (link->proto == PROTO_HDLC) {
	    if (req->data_size > link->packet_size)
		return (-1);
	    req->parm = 1;
	}
	if (req->parm < 1 || req->parm > link->n_pvc ||	/* priority */
	    req->data_size <= 0)		/* data size */
	    return (-1);
    }

    if (req->type == CM_SET_PARAMS) {
	if (req->data_size != sizeof (Link_params))
	    return (-1);
    }
    if (req->type == CM_DIAL_OUT) {
	if (req->data_size != sizeof (Dial_params))
	    return (-1);
    }

    return (0);
}

/**************************************************************************

    Description: Enables the delayed free feature.

**************************************************************************/

void CMC_need_delayed_free () {
    Need_delayed_free = 1;
}

/**************************************************************************

    Description: Implements the delayed memory free. Pointer "pt" is
		saved for later freeing when this is called with pt
		= NULL. This function is enabled by Need_delayed_free.

    Inputs:	pt - the pointer to be freed.

**************************************************************************/

#define MAX_SAVED_PTS 256

void CMC_free (char *pt) {
    static int n_pts = 0;
    static char *pts[MAX_SAVED_PTS];

    if (!Need_delayed_free) {
	if (pt != NULL)
	    free (pt);
	return;
    }
    if (pt == NULL) {		/* free all saved pointers */
	int i;

	for (i = 0; i < n_pts; i++)
	    free (pts[i]);
	n_pts = 0;
	return;
    }
    if (n_pts >= MAX_SAVED_PTS) {
	LE_send_msg (GL_ERROR, "Too many pointers saved for free");
	free (pt);
	return;
    }
    pts[n_pts] = pt;		/* save for later free */
    n_pts++;
}


