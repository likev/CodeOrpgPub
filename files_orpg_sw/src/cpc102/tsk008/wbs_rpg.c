
/******************************************************************

    This is the wb_simulator module that interacts with the RPG.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/09/22 21:04:23 $
 * $Id: wbs_rpg.c,v 1.8 2011/09/22 21:04:23 jing Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <orpgerr.h>
#include <comm_manager.h>
#include <infr.h>

#include "wbs_def.h"

#define MAX_MESSAGE_SIZE 2416	/* max size of ICD messages to RPG */
#define MAX_BUFFER_SIZE 2500	/* buffer size for msgs to RPG */

static int Req_lb;		/* request LB fd */
static int Resp_lb;		/* response LB fd */
static int Data_lb;		/* playback data LB fd */
static int Link_ind;		/* comms link index */

enum {RPG_DISCONNECTED, RPG_CONNECTING, RPG_CONNECTED};
enum {DWS_DONE, DWS_CONGESTED, DWS_WAITING, DWS_NO_DATA};

static int Connect_state = RPG_DISCONNECTED;
				/* The current RPG connection state */
static int Data_write_status = DWS_DONE;
static int Seq_num = 0;
static time_t Cr_time = 0;
static int Last_wreq = 0;	/* the latest comm_manager write seq num */

static int Ctm_hd_s = 12;

static char *Get_full_path (char *name);
static void Connect_to_rpg ();
static int Send_a_request (int type, int parm);
static int Read_comms_resp ();
static void Process_response (char *buf, int len);
static int Send_data (char *msg, int length);
static void Process_RPG_message (char *buf, int len);
static void Send_playback_data ();
static void Send_RDA_loopback_msg ();


/******************************************************************

    Initializes this module. Returns 0 on success or -1 on failure.
	
******************************************************************/

#define MAX_NAME_SIZE 128

int WBSR_init (char *request_lb_name, char *response_lb_name, 
				char *data_lb_name, int no_ctm_header) {
    char *req_name, *resp_name, *data_name;

    if (no_ctm_header)
	Ctm_hd_s = 0;

    if (request_lb_name == NULL) {
	LE_send_msg (GL_ERROR, "comms request data store not specified");
	return (-1);
    }
    if (response_lb_name == NULL) {
	LE_send_msg (GL_ERROR, "comms response data store not specified");
	return (-1);
    }
    if (data_lb_name == NULL) {
	LE_send_msg (GL_ERROR, "playback data store not specified");
	return (-1);
    }

    req_name = Get_full_path (request_lb_name);
    resp_name = Get_full_path (response_lb_name);
    data_name = Get_full_path (data_lb_name);
    if (req_name == NULL || resp_name == NULL || data_name == NULL)
	return (-1);

    Link_ind = 0;

    Req_lb = LB_open (req_name, LB_WRITE, NULL);
    if (Req_lb < 0) {
	LE_send_msg (GL_ERROR, "LB_open (%s) failed (%d)", req_name, Req_lb);
	return (-1);
    }
    Resp_lb = LB_open (resp_name, LB_READ, NULL);
    if (Resp_lb < 0) {
	LE_send_msg (GL_ERROR, "LB_open (%s) failed (%d)", resp_name, Resp_lb);
	return (-1);
    }
    Data_lb = LB_open (data_name, LB_READ, NULL);
    if (Data_lb < 0) {
	LE_send_msg (GL_ERROR, "LB_open (%s) failed (%d)", data_name, Data_lb);
	return (-1);
    }
    LE_send_msg (LE_VL1, "LBs opened: comms req %s, comms resp %s, playback data %s", req_name, resp_name, data_name);
    STR_free (req_name);
    STR_free (resp_name);
    STR_free (data_name);

    return (0);
}

/********************************************************************

    Returns the full path of LB "name". The returned pointer, a STR
    pointer, must be freed by the caller. Returns NULL on failure.
	
********************************************************************/

static char *Get_full_path (char *name) {
    char dir[MAX_NAME_SIZE], *b;

    if (name[0] == '/')
	return (STR_copy (NULL, name));

    if (MISC_get_work_dir (dir, MAX_NAME_SIZE) <= 0) {
	LE_send_msg (GL_ERROR, "$WORK_DIR not found");
	return (NULL);
    }
    b = STR_copy (NULL, dir);
    b = STR_cat (b, "/");
    b = STR_cat (b, name);
    return (b);
}

/********************************************************************

    The main processing loop.
	
********************************************************************/

void WBSR_main () {

    while (1) {
	Cr_time = MISC_systime (NULL);
	if (Connect_state != RPG_CONNECTED)
	    Connect_to_rpg ();
	Read_comms_resp ();
	Send_playback_data ();
	if (Data_write_status == DWS_NO_DATA)
	    msleep (50);
	if (Data_write_status == DWS_CONGESTED)
	    msleep (20);
	if (Data_write_status == DWS_WAITING)
	    msleep (5);
    }
}

/********************************************************************

    Reads and processes all incoming (response) messages from the 
    comms manager. Returns -1 on failure or 0 otherwise.
	
********************************************************************/

static int Read_comms_resp () {
    char *buf;
    int msg_len;

    while (1) {

	msg_len = LB_read (Resp_lb, (char *)&buf, LB_ALLOC_BUF, LB_NEXT);
	if (msg_len == LB_TO_COME)
	    return (0);
	if (msg_len < 0) {
	    LE_send_msg (GL_ERROR, "LB_read Resp_lb failed (ret %d)", msg_len);
	    if (msg_len == LB_EXPIRED)
		continue;
	    else
		return (-1);
	}
	else if (msg_len > 0) {
	    Process_response (buf, msg_len);
	    free (buf);
	}
    }
    return (0);
}

/********************************************************************

    Processes response message "buf" of "len" bytes from the comms 
    manager.
	
********************************************************************/

static void Process_response (char *buf, int len) {
    CM_resp_struct *resp;

    resp = (CM_resp_struct *)buf;
    switch (resp->type) {

	case CM_CONNECT:
	    if (resp->ret_code == CM_SUCCESS) {
		Connect_state = RPG_CONNECTED;
		Send_RDA_loopback_msg ();
		LE_send_msg (LE_VL1, "connected to RPG");
		return;
	    }
	    else if (resp->ret_code != CM_IN_PROCESSING &&
		     resp->ret_code != CM_TERMINATED) {
		Connect_state = RPG_DISCONNECTED;
		LE_send_msg (LE_VL1,  "connecting to RPG failed (ret %d)",
					resp->ret_code);
		return;
	    }
	    break;

	case CM_DISCONNECT:
	    if (resp->ret_code == CM_SUCCESS) {
		Connect_state = RPG_DISCONNECTED;
		LE_send_msg (LE_VL1, "RPG disconnected");
		return;
	    }
	    else if (resp->ret_code != CM_IN_PROCESSING) {
		LE_send_msg (LE_VL1, "disconnecting RPG failed (ret %d)",
					resp->ret_code);
		return;
	    }
	    break;

	case CM_WRITE:
	    if (Last_wreq > resp->req_num + 3)
		LE_send_msg (GL_ERROR, 
			"Unexpeced CM_WRITE response (%d != %d)", 
			resp->req_num, Last_wreq);
	    Data_write_status = DWS_DONE;
	    if (resp->ret_code == CM_SUCCESS)
		return;
	    else if (resp->ret_code == CM_DISCONNECTED) {
		Connect_state = RPG_DISCONNECTED;
		LE_send_msg (LE_VL1, "Connection to RPG lost");
		return;
	    }
	    else if (resp->ret_code == CM_TOO_MANY_REQUESTS) {
		LE_send_msg (GL_ERROR, "CM_WRITE congestion");
		Data_write_status = DWS_CONGESTED;
		return;
	    }
	    else
		LE_send_msg (GL_ERROR, 
				"CM_WRITE error code %d", resp->ret_code);
	    return;

	case CM_DATA:
	    Process_RPG_message ((char *)buf + sizeof (CM_resp_struct) + Ctm_hd_s, 
			len - sizeof (CM_resp_struct) - Ctm_hd_s);
	    return;

	case CM_EVENT:

	    switch (resp->ret_code) {

		case CM_LOST_CONN:
		case CM_LINK_ERROR:
		    Connect_state = RPG_DISCONNECTED;
		    LE_send_msg (GL_INFO,  
			"CM event: CM_LOST_CONN (%d)\n", resp->ret_code);
	            return;

		case CM_TERMINATE:
		    Connect_state = RPG_DISCONNECTED;
		    LE_send_msg (GL_INFO, 
				"CM event: comm_manager terminated\n");
	            return;

		case CM_START:
		    Connect_state = RPG_DISCONNECTED;
		    LE_send_msg (GL_INFO, "CM event: comm_manager started");
	            return;

		case CM_TIMED_OUT:
		    Connect_state = RPG_DISCONNECTED;
		    LE_send_msg (GL_INFO, "CM event: DATA TRANSMIT TIME-OUT");
		    return;

		case CM_STATISTICS:
		    break;

		case CM_EXCEPTION:
		    Connect_state = RPG_DISCONNECTED;
		    LE_send_msg (GL_INFO, "CM event: CM_EXCEPTION\n");
	            return;

		case CM_NORMAL:
		    break;

		default:
		    LE_send_msg (LE_VL1,  
			"CM_EVENT (%d) ignored\n", resp->ret_code);
		    break;
	    }
	    break;

	case CM_STATUS:
	    break;

	case CM_SET_PARAMS:
	    break;

	default:
	    LE_send_msg (0, "response type (%d) ignored", resp->type);
	    break;
    }
    return;
}

/********************************************************************

    Sends request to the comms manager to connect to the RPG.
	
********************************************************************/

static void Connect_to_rpg () {
    static time_t conn_start_time = 0;

    if (Connect_state == RPG_CONNECTED)
	return;

    if (Connect_state == RPG_CONNECTING && Cr_time > conn_start_time + 30)
	Connect_state = RPG_DISCONNECTED;

    if (Connect_state == RPG_DISCONNECTED) {
	if (Send_a_request (CM_CONNECT, 0) != 0)
	    return;
	conn_start_time = MISC_systime (NULL);
	Connect_state = RPG_CONNECTING;
	return;
    }
}

/**************************************************************************

    Sends a request of "type" and "parm" to the comms manager. Return 0 
    on success of -1 on failure.

**************************************************************************/

static int Send_a_request (int type, int parm) {
    CM_req_struct req;
    int ret;

    Seq_num++;
    req.type = type;
    req.parm = parm;
    req.req_num = Seq_num;
    req.link_ind = Link_ind;
    req.time = Cr_time;
    req.data_size = 0;

    ret = LB_write (Req_lb, (char *)&req, sizeof (CM_req_struct), LB_ANY);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "LB_write Req_lb failed (ret %d)", ret);
	return (-1);
    }

    return (0);
}

/**************************************************************************

    Processes message in "buf" of "len" bytes from the RPG.

**************************************************************************/

static void Process_RPG_message (char *buf, int len) {
    RDA_RPG_message_header_t *rda_rpg_msg_header;	/* The ICD msg hdr */

    rda_rpg_msg_header = (RDA_RPG_message_header_t *)buf;
    switch (rda_rpg_msg_header->type) {

	case LOOPBACK_TEST_RPG_RDA:
	    Send_data (buf, len);	/* send back the loopback */
	    Send_RDA_loopback_msg ();
	    break;

	default:		/* other imcoming messages are discarded */
	    break;
    }
}

/**************************************************************************

    Sends a message in "msg" of "length" bytes to the comms manager for
    transmitting the the RPG. Return 0 on success or -1 if the message 
    can not be sent.

**************************************************************************/

static int Send_data (char *msg, int length) {
    static char *buf = NULL;
    CM_req_struct *req;
    int ret;

    buf = STR_reset (buf, length + sizeof (CM_req_struct) + Ctm_hd_s);
    Seq_num++;
    req = (CM_req_struct *)buf;
    req->type = CM_WRITE;
    req->req_num = Seq_num;
    req->link_ind = Link_ind;
    req->time = Cr_time;
    req->data_size = length + Ctm_hd_s;
    req->parm = 1;
    memset (buf + sizeof (CM_req_struct), 0, Ctm_hd_s);
    memcpy (buf + sizeof (CM_req_struct) + Ctm_hd_s, msg, length);

    ret = LB_write (Req_lb, buf, sizeof (CM_req_struct) + length + Ctm_hd_s, LB_ANY);
    if (ret < 0) {
	LE_send_msg (GL_ERROR,  
		"LB_write to-RPG message failed (ret = %d, size = %d)", 
					ret, length);
	return (-1);
    }
    Last_wreq = Seq_num;

    return (0);
}

/**************************************************************************

    Reads playback messages and send to the RPG.

**************************************************************************/

static void Send_playback_data () {
    static int cnt = 0;
    static char buf[NEX_MAX_PACKET_SIZE];
    int msg_len;

    if (Data_write_status == DWS_CONGESTED) {
	LB_seek (Data_lb, -1, LB_CURRENT, NULL);
	Data_write_status = DWS_DONE;
    }
    else if (Data_write_status == DWS_WAITING)
	return;

    while (1) {

	msg_len = LB_read (Data_lb, buf, NEX_MAX_PACKET_SIZE, LB_NEXT);
	if (msg_len == LB_TO_COME) {
	    Data_write_status = DWS_NO_DATA;
	    return;
	}
	if (msg_len < 0) {
	    if (msg_len == LB_EXPIRED) {
		LE_send_msg (GL_ERROR, 
			"Failed in catching up with the playback");
		continue;
	    }
	    else {
		LE_send_msg (GL_ERROR, 
		"LB_read failed (%d) - wb_simulator terminates", msg_len);
		exit (1);
	    }
	}
	else if (msg_len > 0) {
	    if (Connect_state != RPG_CONNECTED)
		continue;
	    if (Send_data (buf, msg_len) == 0) {
		cnt++;
		if (cnt == 1 || (cnt % 10) == 0)
		    LE_send_msg (LE_VL2, 
			"%d playback messages sent to RPG", cnt);
		Data_write_status = DWS_WAITING;
		return;
	    }
	    break;
	}
    }
    Data_write_status = DWS_NO_DATA;
    return;
}

/**************************************************************************

    Sends the loopback message to RPG. This message identifies that this is
    the WBS.

**************************************************************************/

static void Send_RDA_loopback_msg () {
    static int buf[(sizeof (RDA_RPG_message_header_t) + sizeof (short) + 64) / sizeof (int)];
    static int seq_num = 0;
    RDA_RPG_message_header_t *msg_hd;
    short *sp;
    int len, slen;
    char *msg;
    extern int Legacy_RDA;

    msg_hd = (RDA_RPG_message_header_t *)buf;
    msg_hd->type = LOOPBACK_TEST_RDA_RPG;
    msg_hd->rda_channel = 0;
    if (!Legacy_RDA)
	msg_hd->rda_channel |= RDA_RPG_MSG_HDR_ORDA_CFG;
    msg_hd->sequence_num = SHORT_BSWAP_L (seq_num);
    seq_num = (seq_num + 1) % 0x8000;
    msg_hd->julian_date = 0;
    msg_hd->milliseconds = 0;
    msg_hd->num_segs = SHORT_BSWAP_L (1);
    msg_hd->seg_num = SHORT_BSWAP_L (1);

    msg = "Wideband Simulator";
    len = strlen (msg) + 1;
    slen = len / sizeof (short);
    if (len % sizeof (short))
	slen++;
    sp = (short *)((char *)buf + sizeof (RDA_RPG_message_header_t));
    *sp = slen + 1;
    strcpy ((char *)(sp + 1), msg);
    slen = sizeof (RDA_RPG_message_header_t) / sizeof (short) + (*sp);
    msg_hd->size = SHORT_BSWAP_L (slen);
    Send_data ((char *)buf, slen * sizeof (short));
}

