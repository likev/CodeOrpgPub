
/******************************************************************

	file: psv_wan_access.c

	This module contains functions that access the WAN interface.
	
******************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <rss_replace.h>
#include <orpgdat.h>
#include <prod_user_msg.h>
#include <prod_gen_msg.h>
#include <orpg.h>
#include <infr.h>
#include <comm_manager.h>

#include "psv_def.h"

#define INIT_BUF_SIZE 	5000	/* initial size for response messages */
#define DEFAULT_STT_REP_PERIOD 30	/* default statistics report period */

static int N_users;		/* number of users (links) */
static User_struct **Users;	/* the user structure list */

typedef struct {		/* local part of the User_struct */
    int req_fd;			/* fd for the comm_manager request LB */
    int resp_fd;		/* fd for the comm_manager response LB */
    int n_reqs;			/* number of requests sent; also used as 
				   request sequence number */
    char *buf;			/* buffer for response messages */
    int buf_size;		/* size of buf */
    int last_wreq;		/* request number of the last unacknowledged
				   normal priority write message. 0 means 
				   that there is no such message */
    int last_areq;		/* The same as last_wreq for alert circuit */
} Wan_local;

static int Rep_period = DEFAULT_STT_REP_PERIOD;

/* local functions */
static int Open_lbs ();
static int Check_data (User_struct *usr, char *msg, int len);
static int Send_a_request (User_struct *usr, int type, int parm);
static char *Remove_ip (char *msg);
static int Change_statistics_report_period (User_struct *usr, int period);


/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_users - number of users.
		users - the user structure list.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int WAN_initialize (int n_users, User_struct **users)
{
    int i;

    N_users = n_users;
    Users = users;

    /* allocate local data structure */
    for (i = 0; i < N_users; i++) {
	Wan_local *wan;

	wan = malloc (sizeof (Wan_local));
	if (wan == NULL) {
	    LE_send_msg (GL_ERROR | 213,  "malloc failed");
	    return (-1);
	}
	Users[i]->wan = wan;
	wan->n_reqs = 0;
	wan->buf = malloc (INIT_BUF_SIZE);
	if (wan->buf == NULL) {
	    LE_send_msg (GL_ERROR | 214,  "malloc failed");
	    return (-1);
	}
	wan->buf_size = INIT_BUF_SIZE;
	wan->last_wreq = 0;
	wan->last_areq = 0;
    }

    /* open comm_manager LBs */
    if (Open_lbs () < 0)
	return (-1);

    return (0);
}

/**************************************************************************

    Performs initial contact with the comm_manager to set the adaptation
    parameters and test the initial response from the comm_manager.

**************************************************************************/

void WAN_init_contact () {
    int i;

    for (i = 0; i < N_users; i++) {
	WAN_send_set_params_request (Users[i], Users[i]->line_tbl, 
						Users[i]->info->nb_timeout);
    }
}

/**************************************************************************

    Changes statistics report period for all users.

**************************************************************************/

void WAN_change_statistics_report_period (int period) {
    int i;

    if (period > DEFAULT_STT_REP_PERIOD)
	period = DEFAULT_STT_REP_PERIOD;
    LE_send_msg (GL_INFO, "Statistics report period sets to %d", period);
    Rep_period = period;
    for (i = 0; i < N_users; i++) {
	Change_statistics_report_period (Users[i], Rep_period);
    }
}

/**************************************************************************

    Description: This function reads the next message in "usr"s response LB.

    Inputs:	usr - the user involved.

    Output:	data - the message associated with the event.

    Return:	The event number on success or -1 if there is no message 
		or -2 in other error conditions.

**************************************************************************/

int WAN_read_next_msg (User_struct *usr, char **data)
{
    CM_resp_struct *resp;
    Wan_local *wan;
    int msg_len;

    wan = (Wan_local *)usr->wan;

    *data = NULL;
    while (1) {

	msg_len = LB_read (wan->resp_fd, wan->buf, wan->buf_size, LB_NEXT);
	if (msg_len < 0) {
	    if (msg_len == LB_BUF_TOO_SMALL) {	/* realloc the buffer */
		LB_info info;
		char *tbuf;

		LB_seek (wan->resp_fd, -1, LB_CURRENT, &info);
		tbuf = malloc (info.size);
		if (tbuf == NULL) {
	 	    LE_send_msg (GL_ERROR | 215,  "malloc failed");
		    return (-1);
		}
		free (wan->buf);
		wan->buf = tbuf;
		wan->buf_size = info.size;
		continue;
	    }
	    else if (msg_len == LB_TO_COME)
		return (-1);
	    else if (msg_len == LB_EXPIRED) {
	 	LE_send_msg (GL_ERROR, "LB_read comm_resp - message lost");
		continue;
	    }
	    else {
	 	LE_send_msg (GL_ERROR | 216,  
			"LB_read comm_resp failed (ret %d, link %d)",
						msg_len, usr->line_ind);
		return (-1);
	    }
	}
	else if (msg_len == LB_TO_COME)
	    return (-1);
	else break;
    }

    resp = (CM_resp_struct *)wan->buf;
    *data = wan->buf;

    if (resp->link_ind != usr->line_ind) {
	LE_send_msg (GL_ERROR | 217,  "error - comm response from another link");
	return (-2);
    }

    switch (resp->type) {
	char *c_format;
	int c_len;
	Pd_msg_header *phd;

	case CM_CONNECT:
	    if (resp->ret_code == CM_SUCCESS) {
		wan->last_wreq = 0;
		wan->last_areq = 0;
		LE_send_msg (LE_VL1 | 218,  "connect success, L%d", 
							usr->line_ind);
		WAN_send_set_params_request (usr, usr->line_tbl, 
						usr->info->nb_timeout);
		return (EV_CONNECT_SUCCESS);
	    }
	    else if (resp->ret_code != CM_IN_PROCESSING &&
		     resp->ret_code != CM_TERMINATED) {
		LE_send_msg (LE_VL1 | 219,  "connect failed (ret %d), L%d",
					resp->ret_code, usr->line_ind);
		return (EV_CONNECT_FAILED);
	    }
	    break;

	case CM_DISCONNECT:
	    if (resp->ret_code == CM_SUCCESS) {
		LE_send_msg (LE_VL1 | 220, "disconnect success, L%d", 
							usr->line_ind);
		return (EV_DISCON_SUCCESS);
	    }
	    else if (resp->ret_code != CM_IN_PROCESSING) {
		LE_send_msg (LE_VL1 | 221,  "disconnect failed (ret %d), L%d",
					resp->ret_code, usr->line_ind);
		return (EV_DISCON_FAILED);
	    }
	    break;

	case CM_WRITE:
	    if (wan->last_wreq == resp->req_num)
		wan->last_wreq = 0;
	    if (wan->last_areq == resp->req_num)
		wan->last_areq = 0;

	    if (resp->ret_code != CM_SUCCESS)
		LE_send_msg (GL_ERROR | 222,  
			"CM_WRITE failed (ret %d, seq %d), L%d",
				resp->ret_code, resp->req_num, usr->line_ind);
	    if (resp->ret_code == CM_DISCONNECTED)
		return (EV_LOST_CONN);
	    return (EV_WRITE_COMPLETED);

	case CM_DATA:

	    c_len = UMC_from_ICD (*data + sizeof (CM_resp_struct), 
			msg_len - sizeof (CM_resp_struct), 
			sizeof (CM_resp_struct), (void *)&c_format);
	    if (c_len < 0) {
		LE_send_msg (GL_ERROR, "UMC_from_ICD failed (ret %d), L%d", 
						c_len, usr->line_ind);
		break;
	    }
	    free (wan->buf);
	    wan->buf = c_format;
	    wan->buf_size = c_len + sizeof (CM_resp_struct);
	    *data = wan->buf + sizeof (CM_resp_struct);
	    if (Check_data (usr, *data, c_len) != 0)
		return (-2);
	    phd = (Pd_msg_header *)(*data);
	    LE_send_msg (LE_VL1 | 224,  
			"user msg received, msg_code %d, len %d, L%d", 
			phd->msg_code, msg_len, usr->line_ind);
	    if (usr->line_type == DEDICATED)
		SUS_user_id (usr, phd->src_id);
	    return (EV_USER_DATA);

	case CM_EVENT:

	    switch (resp->ret_code) {
		char *st_msg;

		case CM_LOST_CONN:
		case CM_LINK_ERROR:
		    LE_send_msg (GL_INFO | 225,  
			"CM event: CM_LOST_CONN (%d), L%d\n", 
					resp->ret_code, usr->line_ind);
	            return (EV_LOST_CONN);

		case CM_TERMINATE:
		    LE_send_msg (GL_INFO | 226,  
			"CM event: comm_manager terminated, L%d\n", 
							usr->line_ind);
	            return (EV_COMM_MANAGER_TERMINATE);

		case CM_START:
		    LE_send_msg (GL_INFO | 227,  
			"CM event: comm_manager started, L%d", usr->line_ind);
		    SUS_line_stat_changed (usr, US_LINE_NORMAL);
	            return (EV_COMM_MANAGER_START);

		case CM_TIMED_OUT:
		    LE_send_msg (PSR_status_log_code (usr),  
		       "CM event: DATA TRANSMIT TIME-OUT, L%d", usr->line_ind);
	            PSR_process_status_supression (usr);
		    return (EV_LOST_CONN);

		case CM_STATISTICS:
		    LE_send_msg (GL_INFO,  
			"CM event: CM_STATISTICS, L%d\n", usr->line_ind);
		    SUS_stat_report (usr, (char *)resp, msg_len);
		    break;

		case CM_EXCEPTION:
		    LE_send_msg (GL_INFO,  
			"CM event: CM_EXCEPTION, L%d\n", usr->line_ind);
		    SUS_line_stat_changed (usr, US_LINE_FAILED);
	            return (EV_LOST_CONN);

		case CM_NORMAL:
		    LE_send_msg (GL_INFO,  
			"CM event: CM_NORMAL, L%d\n", usr->line_ind);
		    SUS_line_stat_changed (usr, US_LINE_NORMAL);
		    break;

		case CM_STATUS_MSG:
		    if (msg_len < sizeof (CM_resp_struct) ||
			resp->data_size <= 0 ||
			sizeof (CM_resp_struct) + resp->data_size != msg_len) {
			LE_send_msg (GL_INFO, 
				"Bad CM_STATUS_MSG event received");
			break;
		    }
		    st_msg = (char *)resp + sizeof (CM_resp_struct);
		    LE_send_msg (GL_ERROR, "NB line %d: %s\n", 
					usr->line_ind, st_msg);
		    if (strncmp (st_msg, "CONNECTED", 9) != 0)
		        LE_send_msg (GL_STATUS, "NB line %d: %s\n", 
					usr->line_ind, Remove_ip (st_msg));
		    break;

		default:
		    LE_send_msg (LE_VL1 | 229,  
			"CM_EVENT (%d) ignored, L%d\n", 
					resp->ret_code, usr->line_ind);
		    break;
	    }
	    break;

	case CM_STATUS:
	    break;

	case CM_SET_PARAMS:	/* initially set line status */
	    SUS_line_stat_changed (usr, US_LINE_NORMAL);
	    break;

	default:
	    LE_send_msg (0, "response type (%d) ignored", resp->type);
	    break;
    }

    return (-2);
}

/**************************************************************************

    Removes IP addressed in "msg".

**************************************************************************/

static char *Remove_ip (char *msg) {
    char *p;

    p = msg;
    while (*p != '\0' && *p != '(')
	p++;
    if (*p == '(' && p > msg)
	p[-1] = '\0';
    return (msg);
}

/**************************************************************************

    Description: This function sends a connect request.

    Inputs:	usr - the user involved.
		type - request type.
		parm - request parameter.

    Return:	0 on success of -1 on failure.

**************************************************************************/

static int Send_a_request (User_struct *usr, int type, int parm)
{
    CM_req_struct req;
    Wan_local *wan;
    int ret;

    if (usr->wan == NULL) {
	LE_send_msg (GL_ERROR | 230,  "psv_wan_access is not initialized, L%d)", 
							usr->line_ind);
	return (-1);
    }

    wan = (Wan_local *)usr->wan;
    (wan->n_reqs)++;
    req.type = type;
    req.parm = parm;
    req.req_num = wan->n_reqs;
    req.link_ind = usr->line_ind;
    req.time = time (NULL);
    req.data_size = 0;

    ret = LB_write (wan->req_fd, (char *)&req, 
					sizeof (CM_req_struct), LB_ANY);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 231,  
			"LB_write wan_req failed (ret %d), L%d)", 
					ret, usr->line_ind);
	return (-1);
    }
    EN_post (ORPGEVT_NB_COMM_REQ + usr->cm_ind, NULL, 0, EN_POST_FLAG_LOCAL);

    return (0);
}

/**************************************************************************

    Description: This function sends a CM_SET_PARAMS request to the
		comm_manager.

    Inputs:	usr - the user involved.
		line_tbl - line table for the user.
		rw_time - maximum read/write time.

    Return:	0 on success of -1 on failure.

**************************************************************************/

#define SET_PARAMS_REQ_SIZE (sizeof (CM_req_struct) + sizeof (Link_params))

int WAN_send_set_params_request (User_struct *usr, 
				Pd_line_entry *line_tbl, int rw_time) {
    int buf[(SET_PARAMS_REQ_SIZE / sizeof (int)) + 1];
    Link_params lp;
    CM_req_struct *req;
    Wan_local *wan;
    int ret, type;

    if (usr->wan == NULL) {
	LE_send_msg (GL_ERROR | 232,
		"psv_wan_access is not initialized, L%d)", usr->line_ind);
	return (-1);
    }

    type = line_tbl->line_type;
    if (type == DEDICATED)
	lp.link_type = CM_DEDICATED;
    else if (type == DIAL_IN)
	lp.link_type = CM_DIAL_IN;
    else if (type == DIAL_OUT)
	lp.link_type = CM_DIAL_IN_OUT;
    else
	lp.link_type = CM_WAN;
    lp.line_rate = line_tbl->baud_rate;
    lp.packet_size = line_tbl->packet_size;
    lp.rw_time = rw_time;
    lp.rep_period = Rep_period;
    LE_send_msg (LE_VL2 | 233,  "send link params (%d %d %d %d %d), L%d)",
			lp.link_type, lp.line_rate, lp.packet_size, 
			lp.rw_time, lp.rep_period, usr->line_ind);

    wan = (Wan_local *)usr->wan;
    (wan->n_reqs)++;
    req = (CM_req_struct *)buf;
    req->type = CM_SET_PARAMS;
    req->req_num = wan->n_reqs;
    req->link_ind = usr->line_ind;
    req->time = time (NULL);
    req->data_size = sizeof (Link_params);

    memcpy ((char *)req + sizeof (CM_req_struct), &lp, sizeof (Link_params));

    ret = LB_write (wan->req_fd, (char *)req, SET_PARAMS_REQ_SIZE, LB_ANY);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 234,  
			"LB_write wan_req (set params) failed (ret %d), L%d)", 
					ret, usr->line_ind);
	return (-1);
    }
    EN_post (ORPGEVT_NB_COMM_REQ + usr->cm_ind, NULL, 0, EN_POST_FLAG_LOCAL);

    return (0);
}

/***************************************************************************

    Changes the statistics reporting period to "period" by sending an event
    to the comms server.

***************************************************************************/

static int Change_statistics_report_period (User_struct *usr, int period) {
    int buf[(SET_PARAMS_REQ_SIZE / sizeof (int)) + 1];
    Link_params lp;
    CM_req_struct *req;
    Wan_local *wan;
    int ret;

    if (usr->wan == NULL)
	return (-1);

    lp.link_type = -1;
    lp.line_rate = -1;
    lp.packet_size = -1;
    lp.rw_time = -1;
    lp.rep_period = Rep_period;
    LE_send_msg (LE_VL2,
	"Change statitics period to %d, L%d)", period, usr->line_ind);

    wan = (Wan_local *)usr->wan;
    (wan->n_reqs)++;
    req = (CM_req_struct *)buf;
    req->type = CM_SET_PARAMS;
    req->req_num = wan->n_reqs;
    req->link_ind = usr->line_ind;
    req->time = time (NULL);
    req->data_size = sizeof (Link_params);

    memcpy ((char *)req + sizeof (CM_req_struct), &lp, sizeof (Link_params));

    ret = LB_write (wan->req_fd, (char *)req, SET_PARAMS_REQ_SIZE, LB_ANY);
    if (ret < 0) {
	LE_send_msg (GL_ERROR | 234,  
			"LB_write wan_req (set params) failed (ret %d), L%d)", 
					ret, usr->line_ind);
	return (-1);
    }
    EN_post (ORPGEVT_NB_COMM_REQ + usr->cm_ind, NULL, 0, EN_POST_FLAG_LOCAL);

    return (0);
}

/**************************************************************************

    Description: This function sends a connect request.

    Inputs:	usr - the user involved.

**************************************************************************/

void WAN_connect (User_struct *usr)
{

    if (Send_a_request (usr, CM_CONNECT, 0) != 0)
	return;

    LE_send_msg (LE_VL1 | 235,  "connect request sent, L%d\n", usr->line_ind);

    return;
}

/**************************************************************************

    Description: This function sends a disconnect request.

    Inputs:	usr - the user involved.

**************************************************************************/

void WAN_disconnect (User_struct *usr)
{

    if (Send_a_request (usr, CM_DISCONNECT, 0) != 0)
	return;

    LE_send_msg (LE_VL1 | 236,  "disconnect request sent, L%d\n", usr->line_ind);

    return;
}

/**************************************************************************

    Description: This function discards all messages in the comm manager 
		response LB.

    Inputs:	usr - the user involved.

**************************************************************************/

void WAN_clear_responses (User_struct *usr)
{
    Wan_local *wan;

    wan = (Wan_local *)usr->wan;
    LB_seek (wan->req_fd, 1, LB_LATEST, NULL);

    return;
}

/**************************************************************************

    Description: This function returns whether the normal circuit is ready 
		for accepting a new message. For normal priority message,
		only one outstanding (unacknowledged) message can be written 
		to the comm_manager.

    Inputs:	usr - the user involved.
		priority - HIGH_PRIORITY or NORMAL_PRIORITY;

    Return:	Non-zero if the normal circuit is ready for accepting a 
		new message or zero if it is not.

**************************************************************************/

int WAN_circuit_ready (User_struct *usr, int priority)
{
    Wan_local *wan;

    wan = (Wan_local *)usr->wan;
    if ((priority == NORMAL_PRIORITY && wan->last_wreq > 0) ||
	(priority == HIGH_PRIORITY && wan->last_areq > 0))
	return (0);
    return (1);
}

/**************************************************************************

    Description: This function sends a write request. We assume that there
		is enough space for adding a CM_req_struct header in front
		of the message stored in "buf". For normal priority message,
		only one outstanding (unacknowledged) message can be written 
		to the comm_manager.

    Inputs:	usr - the user involved.
		priority - HIGH_PRIORITY or NORMAL_PRIORITY;
		buf - the message;
		length - length of the message.

     Return:	0 on success or -1 if the message can not be sent.

**************************************************************************/

int WAN_write (User_struct *usr, int priority, char *buf, int length)
{
    CM_req_struct *req;
    Wan_local *wan;
    int ret;

    if (usr->line_tbl->n_pvcs == 1)
	priority = HIGH_PRIORITY;		/* only one pvc */

    wan = (Wan_local *)usr->wan;
    if ((priority == NORMAL_PRIORITY && wan->last_wreq > 0) ||
	(priority == HIGH_PRIORITY && wan->last_areq > 0))
	return (-1);

    (wan->n_reqs)++;
    req = (CM_req_struct *)(buf - sizeof (CM_req_struct));
    req->type = CM_WRITE;
    req->req_num = wan->n_reqs;
    req->link_ind = usr->line_ind;
    req->time = time (NULL);
    req->data_size = length;
    req->parm = priority;

    LE_send_msg (LE_VL2 | 237,  
		"        write to WAN (length 24 + %d, prio %d), L%d", 
		length, req->parm, usr->line_ind);
    ret = LB_write (wan->req_fd, (char *)req, 
				sizeof (CM_req_struct) + length, LB_ANY);
    if (ret < 0)
	LE_send_msg (GL_ERROR | 238,  
			"LB_write wan_req failed (ret = %d, link = %d)", 
					ret, usr->line_ind);
    else
	EN_post (ORPGEVT_NB_COMM_REQ + usr->cm_ind, NULL, 
					0, EN_POST_FLAG_LOCAL);
    if (priority == NORMAL_PRIORITY)
	wan->last_wreq = wan->n_reqs;
    else
	wan->last_areq = wan->n_reqs;

    return (0);
}

/**************************************************************************

    Description: This function opens the comm_manager request and response 
		LBs for all users.

    Return:	0 on success or -1 on failure.

**************************************************************************/

static int Open_lbs ()
{
    Wan_local *wan;
    char name[NAME_SIZE];
    int i, ret;

    for (i = 0; i < N_users; i++) {
	int k;

	wan = (Wan_local *)Users[i]->wan;
	for (k = 0; k < i; k++) {	/* req LB already open */
	    if (Users[k]->cm_ind == Users[i]->cm_ind) {
		wan->req_fd = ((Wan_local *)(Users[k]->wan))->req_fd;
		break;
	    }
	}
	if (k >= i) {			/* we need to open req LB */
	    if (CS_entry ((char *)(ORPGDAT_CM_REQUEST + Users[i]->cm_ind), 
				CS_INT_KEY | 1, NAME_SIZE, name) < 0)
		return (-1);
	    ret = LB_open (name, LB_WRITE, NULL);
	    if (ret < 0) {
		LE_send_msg (GL_ERROR | 239,  
			"LB_open %s failed (ret = %d)", name, ret);
		return (-1);
	    }
	    wan->req_fd = ret;
	}

	/* response LBs */
	if (CS_entry ((char *)(ORPGDAT_CM_RESPONSE + Users[i]->line_ind), 
				CS_INT_KEY | 1, NAME_SIZE, name) < 0)
	    return (-1);
	ret = LB_open (name, LB_READ, NULL);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR | 240,  
			"LB_open %s failed (ret = %d)", name, ret);
	    return (-1);
	}
	wan->resp_fd = ret;
    }

    return (0);
}

/**************************************************************************

    Description: This function allocates a buffer for a message the is to be
		send to the product user. Extra space is reserved in front
		of the buffer for the comm header and the Prod_header header.

    Inputs:	size - size of the message.

    Return:	The pointer to the message area on success or NULL on 
		failure.

**************************************************************************/

#define RESERVED_SIZE sizeof (Prod_header)
		/* since sizeof (Prod_header) > sizeof (CM_req_struct). 
		   Otherwise we need to use sizeof (CM_req_struct) */

char *WAN_usr_msg_malloc (int size)
{
    char *buf;

    buf = malloc (size + RESERVED_SIZE);
    if (buf == NULL) {
	LE_send_msg (GL_ERROR | 241,  "malloc failed");
	return (NULL);
    }

    return (buf + RESERVED_SIZE);
}

/**************************************************************************

    Description: This function frees a product user message buffer allocated 
		by WAN_usr_msg_malloc.

    Inputs:	msg - pointer to the message.

**************************************************************************/

void WAN_free_usr_msg (char *msg)
{

    free (msg - RESERVED_SIZE);
    return;
}

/**************************************************************************

    Description: This function verifies user messages.

    Inputs:	usr - the user involved.
		msg - user message.
		len - length of the user message.

    Return:	0 if no error is found or -1 if an error is detected.

**************************************************************************/

static int Check_data (User_struct *usr, char *msg, int len)
{
    Pd_msg_header *hd;

    if (len < (int)sizeof (Pd_msg_header)) {
	LE_send_msg (GL_ERROR | 242,  "user message without header, L%d", 
						usr->line_ind);
	return (-1);
    }
    hd = (Pd_msg_header *)msg;
    if (UMC_product_length (hd) != len) {
	LE_send_msg (GL_ERROR | 243,  
	    "incorrect user message length (%d, hd->length %d), L%d", 
			len, UMC_product_length (hd), usr->line_ind);
	return (-1);
    }
    switch (hd->msg_code) {
	int size;

	case MSG_PROD_REQUEST:
	case MSG_PROD_REQ_CANCEL:
	    hd->line_ind = usr->line_ind;	/* set the line index */
	    size = ALIGNED_SIZE (sizeof (Pd_msg_header)) +
			(hd->n_blocks - 1) * sizeof (Pd_request_products);
	    if (len != size) {
		LE_send_msg (GL_ERROR | 244,  
		    "prod request msg length error (%d, nblock %d, expecting %d), L%d",
				len, hd->n_blocks, size, usr->line_ind);
		return (-1);
	    }
	    break;

	case MSG_MAX_CON_DISABLE:
	    size = sizeof (Pd_max_conn_time_disable_msg);
	    if (len != size) {
		LE_send_msg (GL_ERROR | 245,  
		    "max conn time disable msg length error (%d, expecting %d), L%d",
				len, size, usr->line_ind);
		return (-1);
	    }
	    break;

	case MSG_ALERT_REQUEST:
	    break;

	case 75:			/* text message from the user */
        case MSG_ENVIRONMENTAL_DATA:	/* environmental data from the user */
	    hd->line_ind = usr->line_ind;	/* set the line index */
	    break;

        case MSG_EXTERNAL_DATA:	            /* external data from the user */
	    hd->line_ind = usr->line_ind;   /* set the line index */
	    break;

	case MSG_SIGN_ON:
	    size = sizeof (Pd_sign_on_msg);
	    if (len != size) {
		LE_send_msg (GL_ERROR | 246,  
		    "sign-on msg length error (%d, expecting %d), L%d",
				len, size, usr->line_ind);
		return (-1);
	    }
	    break;

	case MSG_PROD_LIST:
	    size = sizeof (Pd_msg_header);
	    if (len != size) {
		LE_send_msg (GL_ERROR | 247,  
		    "prod list req msg length error (%d, expecting %d), L%d",
				len, size, usr->line_ind);
		return (-1);
	    }
	break;

	default:
	    LE_send_msg (GL_ERROR | 250,  "unknown user message (code %d), L%d", 
					hd->msg_code, usr->line_ind);
	    usr->bad_msg_cnt++;
	    return (-1);
    }
    return (0);
}




