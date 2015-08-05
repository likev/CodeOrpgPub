
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <stdlib.h>

#include <infr.h>
#include <orpg.h>
#include "simulate.h"

extern int Cur_link_ind;
extern int Little_endian;
extern int Src_id;
extern int Dest_id;
extern int No_connection;
extern int Use_comms_manager;
extern int Allow_ICD_send;

static int Input_LB, Output_LB;

static int N_chars = 40;	/* number of chars after MHB printed */
static int Bytes_expected = 0;	/* number of bytes expected to complete the
				   the current message */

/* connection state */
enum {CS_DISCONNECTED, CS_CONNECTING, CS_CONNECTED, CS_DISCONNECTING};
static int Connect_state = CS_DISCONNECTED;

/* message write state */
enum {WS_WRITE_DONE, WS_WRITING};
static int Write_state = WS_WRITE_DONE;

static int N_reqs = 0;		/* request sequence number */

#define BUF_SIZE 20000

static char Output_buf[BUF_SIZE];


static void Send_response (CM_req_struct *req);
static void Send_icd_msg (char *msg, int size);
static int Set_message_header_block (char *msg);
static int Send_product_request_msg ();
static int Send_bias_table_msg ();
static void Display_msg (char *msg, int size);
static void Print_bytes (char *text, char *msg, int size);
static void Int_array_byte_swap (void *buf, int size);
static void Write_response_msg (char *msg, int size);
static void Send_event (int event_code);
static int Read_params (int *params);
static void Process_cm_response ();
static int Send_a_request (int type, int parm);
static int Send_icd_msg_to_cm (char *msg, int size);
static int Read_bias_rows (int n_rows, int *mem_span, int *n_pairs, 
                           int *avg_gage, int *avg_radar, int *bias);



/***********************************************************************

    Description: This function initializes this module.

***********************************************************************/

int SIM_init (char *in_lb, char *out_lb) {

    Input_LB = LB_open (in_lb, LB_READ, NULL);
    if (Input_LB < 0) {
	LE_send_msg (0, "LB_open %s failed (ret = %d)", in_lb, Input_LB);
	exit (1);
    }

    Output_LB = LB_open (out_lb, LB_WRITE, NULL);
    if (Output_LB < 0) {
	LE_send_msg (0, "LB_open %s failed (ret = %d)", out_lb, Output_LB);
	exit (1);
    }

    if (!Use_comms_manager) {
	Send_event (CM_START);
    }

    return (0);
}

/***********************************************************************

    Description: This function reads all incoming messages in the 
		comm_manager request LB and process them.

***********************************************************************/

void Md_search_input_LB () {
    static unsigned char input_buf[BUF_SIZE];

    if (Use_comms_manager) {
	Process_cm_response ();
	return;
    }

    while (1) {
	int ret, msg_len;
	CM_req_struct *req;

	ret = LB_read (Input_LB, (char *) input_buf, BUF_SIZE, LB_NEXT);

	if (ret > 0) {

	    msg_len = ret;
	    req = (CM_req_struct *)input_buf;

	    Int_array_byte_swap (req, sizeof (CM_req_struct) / sizeof (int));

	    if (msg_len < sizeof(CM_req_struct)) {
		printf ("unexpected msg length %d\n", msg_len);
		continue;
	    }

	    if (req->link_ind != Cur_link_ind) {
/*	        printf("A wrong link ind comes,[%d] != cur [%d]\n",
		       req->link_ind, Cur_link_ind);
*/
	        continue;
	    }

	    switch (req->type) {

	        case RQ_WRITE:

		if (msg_len - sizeof(CM_req_struct) != req->data_size) {
		    printf ("bad RQ_WRITE message size\n");
		    continue;
		}
		else {
	            Display_msg ((char *)req + sizeof(CM_req_struct), 
							req->data_size);
		    Send_response (req);
		}
		break;

	        case RQ_CONNECT:
		if (!No_connection) {
	            Send_response (req);
		    Bytes_expected = 0;
		}
	        break;

	        case RQ_DISCONNECT:
	        Send_response (req);
		Bytes_expected = 0;
	        break;

	        default:
	        printf("request message type %d ignored\n", req->type);
	        break;
	    }

        } 
	else if (ret == LB_TO_COME) 
	    return;
	else {
	    printf("LB_read cm_request failed (ret %d)\n", ret);
	    exit (1);
        }
    }
}

/***********************************************************************

    Processes comm_manager response messages.

***********************************************************************/

static void Process_cm_response () {

    while (1) {
	static char *buf = NULL;
	int msg_len;

	if (buf != NULL)
	    free (buf);
	buf = NULL;
	msg_len = LB_read (Input_LB, (char **)&buf, LB_ALLOC_BUF, LB_NEXT);

	if (msg_len > 0) {
	    CM_resp_struct *resp;

	    if (msg_len < sizeof (CM_resp_struct)) {
		printf ("unexpected comm_manager response size %d\n", msg_len);
		continue;
	    }
	    resp = (CM_resp_struct *)buf;
	    if (resp->link_ind != Cur_link_ind) {
		printf ("error - comm response from another link\n");
		continue;
	    }
printf ("response link_ind %d type %d ret_code %d\n", resp->link_ind, resp->type, resp->ret_code);
	    switch (resp->type) {
	
		case CM_CONNECT:
		    if (Connect_state == CS_CONNECTING &&
					resp->ret_code == CM_SUCCESS)
			Connect_state = CS_CONNECTED;
		    else 
			printf ("unexpected CM_CONNECT resp (code %d)\n", 
							resp->ret_code);
		    break;
	
		case CM_DISCONNECT:
		    if (Connect_state == CS_DISCONNECTING &&
					resp->ret_code == CM_SUCCESS)
			Connect_state = CS_DISCONNECTED;
		    else 
			printf ("unexpected CM_DISCONNECT resp (code %d)\n", 
							resp->ret_code);
		    break;

		case CM_WRITE:
		    if (Write_state == WS_WRITING &&
					resp->ret_code == CM_SUCCESS)
			Write_state = WS_WRITE_DONE;
		    else 
			printf ("unexpected CM_WRITE resp (code %d)\n", 
							resp->ret_code);
		    break;

		case CM_DATA:
		    if (msg_len - sizeof (CM_resp_struct) != resp->data_size) {
			printf ("bad CM_DATA message size\n");
			continue;
		    }
		    else {
			Display_msg ((char *)resp + sizeof(CM_resp_struct), 
							    resp->data_size);
		    }
		    break;

		case CM_EVENT:
	
		    switch (resp->ret_code) {
	
			case CM_LOST_CONN:
			case CM_LINK_ERROR:
			    printf ("CM event: CM_LOST_CONN (%d)\n", 
							resp->ret_code);
			    Connect_state = CS_DISCONNECTED;
			    Write_state = WS_WRITE_DONE;
			    break;
	
			case CM_TERMINATE:
			    printf ("CM event: comm_manager terminated\n");
			    Connect_state = CS_DISCONNECTED;
			    Write_state = WS_WRITE_DONE;
			    break;
	
			case CM_START:
			    printf ("CM event: comm_manager started\n");
			    Connect_state = CS_DISCONNECTED;
			    Write_state = WS_WRITE_DONE;
			    break;
	
			case CM_TIMED_OUT:
			    printf ("CM event: DATA TRANSMIT TIME-OUT\n");
			    Write_state = WS_WRITE_DONE;
			    break;
	
			case CM_STATISTICS:
			    printf ("CM event: CM_STATISTICS\n");
			    break;
	
			case CM_EXCEPTION:
			    printf ("CM event: CM_EXCEPTION\n");
			    Connect_state = CS_DISCONNECTED;
			    Write_state = WS_WRITE_DONE;
			    break;
	
			case CM_NORMAL:
			    printf ("CM event: CM_NORMAL\n");
			    Connect_state = CS_DISCONNECTED;
			    Write_state = WS_WRITE_DONE;
			    break;
	
			default:
			    printf ("CM_EVENT (%d) ignored\n", resp->ret_code);
			    break;
		    }
		    break;

		default:
		    printf ("comm_manager resp type (%d) ignored\n", 
								resp->type);
		    break;
	    }
        } 
	else if (msg_len == LB_TO_COME) 
	    return;
	else {
	    printf ("LB_read comm_manager response failed (ret %d)\n", 
							msg_len);
	    exit (1);
        }
    }
}

/***********************************************************************

    Description: This function simulates the comm_manager response to
		request "req".

    Input:	req - The request message.

***********************************************************************/

static void Send_response (CM_req_struct *req)
{
    CM_resp_struct resp;

    resp.ret_code = RET_SUCCESS;
    resp.link_ind = req->link_ind;
    resp.type = req->type;
    resp.time = time(NULL);
    resp.req_num = req->req_num;
    resp.data_size = 0;

    Int_array_byte_swap (&resp, sizeof (CM_resp_struct) / sizeof (int));

    Write_response_msg ((char *)&resp, sizeof(CM_resp_struct));

    return;
}

/***********************************************************************

    Description: This function simulates a comm_manager event.

    Input:	event_code - The event code.

***********************************************************************/

static void Send_event (int event_code)
{
    CM_resp_struct resp;

    resp.ret_code = event_code;
    resp.link_ind = Cur_link_ind;
    resp.type = CM_EVENT;
    resp.time = time(NULL);
    resp.req_num = 0;
    resp.data_size = 0;

    Int_array_byte_swap (&resp, sizeof (CM_resp_struct) / sizeof (int));

    Write_response_msg ((char *)&resp, sizeof(CM_resp_struct));

    return;
}

/***********************************************************************

    Description: This function sends a product user message to the 
		p_server.

    Input:	msg - pointer to the user message including the comm header.
		size - size of the user message excluding the comm header.

***********************************************************************/

static void Send_icd_msg (char *msg, int size)
{
    CM_resp_struct *resp;

    if (Use_comms_manager) {
	Send_icd_msg_to_cm (msg, size);
	return;
    }

    resp = (CM_resp_struct *)msg;
    resp->type = RQ_DATA;
    resp->ret_code = RET_SUCCESS;
    resp->link_ind = Cur_link_ind;
    resp->time = time (NULL);
    resp->req_num = 0;
    resp->data_size = size;

    /* swap comm header */
    Int_array_byte_swap (resp, sizeof (CM_resp_struct) / sizeof (int));

    /* swap user message */
    MISC_short_swap (msg + sizeof (CM_resp_struct), size / sizeof (short));

    Write_response_msg ((char *)resp, sizeof(CM_resp_struct) + size);

    return;
}

/**************************************************************************

    Sends an ICD message "msg" of size "size" to the comm_manager.

    Return:	0 on success of -1 on failure.

**************************************************************************/

static int Send_icd_msg_to_cm (char *msg, int size) {
    CM_req_struct *req;
    int ret;

    if (!Allow_ICD_send && Connect_state != CS_CONNECTED) {
	printf ("Cannot send ICD message - not connected (%d)\n", 
							Connect_state);
	return (-1);
    }
    if (!Allow_ICD_send && Write_state != WS_WRITE_DONE) {
	printf ("Cannot send ICD message - writing in progress\n");
	return (-1);
    }

    N_reqs++;
    req = (CM_req_struct *)msg;
    req->type = CM_WRITE;
    req->parm = 1;
    req->req_num = N_reqs;
    req->link_ind = Cur_link_ind;
    req->time = time (NULL);
    req->data_size = size;

    ret = LB_write (Output_LB, (char *)req, 
				sizeof (CM_req_struct) + size, LB_ANY);
    if (ret < 0) {
	printf ("LB_write comm_manager req failed (ret %d))", ret);
	return (-1);
    }
    EN_post_event (/* ORPGEVT_NB_COMM_REQ */ 200 + Cur_link_ind);
    Write_state = WS_WRITING;

    return (0);
}

/***********************************************************************

    Description: This function writes a response msg to p_server
		and sends an event.

    Input:	msg - pointer to the user message including the comm header.
		size - size of the user message excluding the comm header.

***********************************************************************/

static void Write_response_msg (char *msg, int size)
{
    int ret;

    ret = LB_write (Output_LB, msg, size, LB_ANY);
    if (ret <= 0) {
	LE_send_msg(0, "LB_write failed (ret %d)", ret);
	return;
    }

    ret = EN_post (/* ORPGEVT_NB_COMM_RESP */ 300 + Cur_link_ind, NULL, 0, 0);
    if (ret < 0) {
	printf("Post ORPGEVT_NB_COMM_RESP evnt failed (ret = %d)", ret);
	return;
    }

    return;
}

/***********************************************************************

    Description: This function initialize the message header block of
		a product user message.

    Return:	The size,  in halfword, of the message header block.

***********************************************************************/

static int Set_message_header_block (char *msg)
{
    halfword *spt;
    time_t tm;
    int rpg_sec;

    tm = time (NULL);

    spt = (halfword *)msg;
    spt[1] = RPG_JULIAN_DATE (tm);
    rpg_sec = RPG_TIME_IN_SECONDS (tm);
    spt[2] = rpg_sec << 16;
    spt[3] = rpg_sec & 0xffff;
    spt[6] = Src_id;
    spt[7] = Dest_id;

    return (SIZE_OF_PD_MSG_HEADER / sizeof (halfword));
}

/***********************************************************************

    Description: This function generates a sign-on message and sends
		it to the p_server.

***********************************************************************/

int Md_Sign_on ()
{
    int flag, len, ret1, ret2;
    unsigned char u_pass[8], l_pass[8];
    char *msg;
    halfword *spt;

    CS_cfg_name ("pup_emu/sign_on.msg");
    CS_control (CS_RESET);

    if (Use_comms_manager) 
	msg = Output_buf + sizeof (CM_req_struct);
    else
	msg = Output_buf + sizeof (CM_resp_struct);
    spt = (halfword *)msg;
    Set_message_header_block (msg);

    len = 36;				/* message length in bytes */
    spt[0] = 11;			/* message code */
    spt[4] = len << 16;
    spt[5] = len & 0xffff;
    spt[8] = 2;				/* number of blocks */

    spt[9] = -1;
    spt[10] = 18;
    if (CS_entry ("disconn_override_flag", 1 | CS_INT, 0, (void *)&flag) <= 0 ||
	(ret1 = CS_entry ("user_passwd", 1, 8, (void *)u_pass)) < 0 ||
	(ret2 = CS_entry ("port_passwd", 1, 8, (void *)l_pass)) < 0) {
	printf ("read sign_on.msg failed\n");
	CS_cfg_name ("");
	return -1;
    }
    if (ret1 == 0)
	u_pass[0] = '\0';
    if (ret2 == 0)
	l_pass[0] = '\0';

    CS_cfg_name ("");

    spt[11] = (u_pass[0] << 8) | u_pass[1];
    spt[12] = (u_pass[2] << 8) | u_pass[3];
    spt[13] = (u_pass[4] << 8) | u_pass[5];
    spt[14] = (l_pass[0] << 8) | l_pass[1];
    spt[15] = (l_pass[2] << 8) | l_pass[3];
    spt[16] = flag;

    Send_icd_msg ((char *)Output_buf, len);
    return 0;
}

/***********************************************************************

    Description: This function generates a maximum connect time disable
		message and sends it to the p_server.

***********************************************************************/

int Md_max_conn_dis()
{
    int add_tm, len;
    char *msg;
    halfword *spt;

    CS_cfg_name ("pup_emu/max_conn.msg");
    CS_control (CS_RESET);

    if (Use_comms_manager) 
	msg = Output_buf + sizeof (CM_req_struct);
    else
	msg = Output_buf + sizeof (CM_resp_struct);
    spt = (halfword *)msg;
    Set_message_header_block (msg);

    len = 28;				/* message length in bytes */
    spt[0] = 4;				/* message code */
    spt[4] = len << 16;
    spt[5] = len & 0xffff;
    spt[8] = 2;				/* number of blocks */

    spt[9] = -1;
    spt[10] = 10;
    if (CS_entry ("add_conn_time", 1 | CS_INT, 0, (void *)&add_tm) <= 0) {
	printf ("read max_conn.msg failed\n");
	CS_cfg_name ("");
	return -1;
    }

    CS_cfg_name ("");

    spt[11] = add_tm;

    Send_icd_msg ((char *)Output_buf, len);
    return 0;
}

/***********************************************************************

    Description: This function generates a PUP/RPGOP status message and 
		sends it to the p_server.

***********************************************************************/

int Md_puprpgop_to_rpg_status_msg ()
{
    int len;
    char *msg;
    halfword *spt;
    int state, err[30];
    int cnt, i;

    CS_cfg_name ("pup_emu/pup_status.msg");
    CS_control (CS_RESET);

    if (Use_comms_manager) 
	msg = Output_buf + sizeof (CM_req_struct);
    else
	msg = Output_buf + sizeof (CM_resp_struct);
    spt = (halfword *)msg;
    Set_message_header_block (msg);

    if (CS_entry ("pup_rpgop_state", 1 | CS_INT, 0, (void *)&state) <= 0) {
	printf ("read pup status failed\n");
	CS_cfg_name ("");
	return -1;
    }
    cnt = 0;
    CS_control (CS_KEY_OPTIONAL);
    while (cnt < 30 &&
	   CS_entry ("pup_rpgop_error", (cnt + 1) | CS_INT, 0, 
					(void *)&(err[cnt])) > 0)
	cnt++;
    CS_control (CS_KEY_REQUIRED);
    CS_cfg_name ("");

    len = (20 + cnt) * 2;		/* message length in bytes */
    spt[0] = 14;			/* message code */
    spt[4] = len << 16;
    spt[5] = len & 0xffff;
    spt[8] = 2;				/* number of blocks */

    spt[9] = -1;
    spt[10] = (9 + cnt) * 2;
    spt[11] = state;
    for (i = 0; i < cnt; i++)
	spt[20 + i] = err[i];

    Send_icd_msg ((char *)Output_buf, len);
    return 0;
}

/***********************************************************************

    Description: This function generates a routine product request 
		message and sends it to the p_server.

***********************************************************************/

int Md_Routine_product()
{

    CS_cfg_name ("pup_emu/routine_prod_req.msg");
    CS_control (CS_RESET);

    return (Send_product_request_msg ());
}

/***********************************************************************

    Description: This function generates a one-time product request 
		message and sends it to the p_server.

***********************************************************************/

int Md_Class1_one_time ()
{

    CS_cfg_name ("pup_emu/one_time_req.msg");
    CS_control (CS_RESET);
    return (Send_product_request_msg ());
}

/***********************************************************************

    Description: This function generates a bias table message 
		 and sends it to the p_server.

***********************************************************************/

int Md_bias_table_msg()
{

    CS_cfg_name ("./msg/bias_table.msg");
    return (Send_bias_table_msg ());
}

/***********************************************************************

    Description: This function generates a product request message and 
		sends it to the p_server.

***********************************************************************/

static int Send_product_request_msg ()
{
    int len;
    char *msg;
    halfword *spt;
    int cnt;

    if (Use_comms_manager) 
	msg = Output_buf + sizeof (CM_req_struct);
    else
	msg = Output_buf + sizeof (CM_resp_struct);
    spt = (halfword *)msg;
    spt += Set_message_header_block (msg);

    cnt = 0;
    while (1) {
	int prod_code, flag, seq_number, num_prods, interval;
	int vol_date, vol_time;
	int params[6];
	halfword *pt;

	if (CS_entry (CS_NEXT_LINE, 0, 0, NULL) < 0)
	    break;

	if (CS_level (CS_DOWN_LEVEL) < 0)
	    continue;

	if (CS_entry ("prod_code", 1 | CS_INT, 0, (void *)&prod_code) <= 0 ||
	    CS_entry ("flag", 1 | CS_HEXINT, 0, (void *)&flag) <= 0 ||
	    CS_entry ("seq_number", 1 | CS_INT, 0, (void *)&seq_number) <= 0 ||
	    CS_entry ("num_prods", 1 | CS_INT, 0, (void *)&num_prods) <= 0 ||
	    CS_entry ("interval", 1 | CS_INT, 0, (void *)&interval) <= 0 ||
	    CS_entry ("vol_date", 1 | CS_INT, 0, (void *)&vol_date) <= 0 ||
	    CS_entry ("vol_time", 1 | CS_INT, 0, (void *)&vol_time) <= 0) {
	    printf ("read product request failed\n");
	    CS_cfg_name ("");
	    return -1;
	}
 	if (Read_params (params) != 0) {
	    printf ("read product request failed\n");
	    CS_cfg_name ("");
	    return -1;
	}
	if (CS_level (CS_UP_LEVEL) < 0)
	    continue;

	pt = spt + (cnt * 16);
	pt[0] = -1;
	pt[1] = 32;
	pt[2] = prod_code;
	pt[3] = flag;
	pt[4] = seq_number;
	pt[5] = num_prods;
	pt[6] = interval;
	pt[7] = vol_date;
	pt[8] = vol_time >> 16;
	pt[9] = vol_time & 0xffff;
	pt[10] = params[0];
	pt[11] = params[1];
	pt[12] = params[2];
	pt[13] = params[3];
	pt[14] = params[4];
	pt[15] = params[5];

	cnt++;
    }
    CS_cfg_name ("");

    spt = (halfword *)msg;
    len = (9 + cnt * 16) * 2;		/* message length in bytes */
    spt[0] = 0;				/* message code */
    spt[4] = len << 16;
    spt[5] = len & 0xffff;
    spt[8] = 1 + cnt;			/* number of blocks */

    Send_icd_msg ((char *)Output_buf, len);
    return 0;
}

/***********************************************************************

    Description: This function generates a bias table message and 
		sends it to the p_server.

***********************************************************************/

static int Send_bias_table_msg ()
{
    int len, i;
    char *msg, buf[128];
    halfword *spt;

    char awips_id[10], radar_id[10];
    int obs_year, obs_mon, obs_day, obs_hr, obs_min, obs_sec;
    int gen_year, gen_mon, gen_day, gen_hr, gen_min, gen_sec;
    int n_rows, mem_span[12], n_pairs[12], avg_gage[12];
    int avg_radar[12], bias[12];

    halfword *pt, *t;

    msg = Output_buf + sizeof (CM_resp_struct);
    spt = (halfword *)msg;
    spt += Set_message_header_block (msg);

    if( (CS_entry ("Bias_table", 0, 128, buf ) <= 0 )
                       ||
        (CS_level( CS_DOWN_LEVEL ) < 0) ){

        printf ("can not find bias table section\n");
        CS_cfg_name ("");
        return -1;
 
    }

    if (CS_entry ("site_id", 1, 4, (void *)&awips_id[1]) <= 0 ||
        CS_entry ("radar_id", 1, 4, (void *)&radar_id[1]) <= 0 ||
        CS_entry ("obs_year", 1 | CS_INT, 0, (void *)&obs_year) <= 0 ||
        CS_entry ("obs_month", 1 | CS_INT, 0, (void *)&obs_mon) <= 0 ||
        CS_entry ("obs_day", 1 | CS_INT, 0, (void *)&obs_day) <= 0 ||
        CS_entry ("obs_hour", 1 | CS_INT, 0, (void *)&obs_hr) <= 0 ||
        CS_entry ("obs_min", 1 | CS_INT, 0, (void *)&obs_min) <= 0 ||
        CS_entry ("obs_sec", 1 | CS_INT, 0, (void *)&obs_sec) <= 0 ||
        CS_entry ("gen_year", 1 | CS_INT, 0, (void *)&gen_year) <= 0 ||
        CS_entry ("gen_month", 1 | CS_INT, 0, (void *)&gen_mon) <= 0 ||
        CS_entry ("gen_day", 1 | CS_INT, 0, (void *)&gen_day) <= 0 ||
        CS_entry ("gen_hour", 1 | CS_INT, 0, (void *)&gen_hr) <= 0 ||
        CS_entry ("gen_min", 1 | CS_INT, 0, (void *)&gen_min) <= 0 ||
        CS_entry ("gen_sec", 1 | CS_INT, 0, (void *)&gen_sec) <= 0 ||
        CS_entry ("n_rows", 1 | CS_INT, 0, (void *)&n_rows) <= 0 ){

        printf ("read bias table failed\n");
        CS_cfg_name ("");
        return -1;

    }

    if (Read_bias_rows (n_rows, mem_span, n_pairs, avg_gage, avg_radar, bias) != 0) {

       printf ("read bias table rows failed\n");
       CS_cfg_name ("");
       return -1;

    }

    if (CS_level (CS_UP_LEVEL) < 0){

       printf("read bias table rows failed\n" );
       CS_cfg_name ("");
       return -1;

    }

    pt = spt;
    pt[0] = -1;
    pt[1] = 1;
    pt[2] = 0;
    pt[3] = 42 + (n_rows * 20);

    awips_id[0] = 0x20;
    memcpy( &pt[4], &awips_id[0], 2 );
    memcpy( &pt[5], &awips_id[2], 2 );

    radar_id[0] = 0x20;
    memcpy( &pt[6], &radar_id[0], 2 );
    memcpy( &pt[7], &radar_id[2], 2 );

    pt[8] = obs_year;
    pt[9] = obs_mon;
    pt[10] = obs_day;
    pt[11] = obs_hr;
    pt[12] = obs_min;
    pt[13] = obs_sec;

    pt[14] = gen_year;
    pt[15] = gen_mon;
    pt[16] = gen_day;
    pt[17] = gen_hr;
    pt[18] = gen_min;
    pt[19] = gen_sec;

    pt[20] = n_rows;

    t = &pt[21];
    for( i = 0; i < n_rows; i++ ){

        t[0] = (mem_span[i] & 0xffff0000) >> 16;
        t[1] = mem_span[i] & 0xffff;
        t[2] = (n_pairs[i] & 0xffff0000) >> 16;
        t[3] = n_pairs[i] & 0xffff;
        t[4] = (avg_gage[i] & 0xffff0000) >> 16;
        t[5] = avg_gage[i] & 0xffff;
        t[6] = (avg_radar[i] & 0xffff0000) >> 16;
        t[7] = avg_radar[i] & 0xffff;
        t[8] = (bias[i] & 0xffff0000) >> 16;
        t[9] = bias[i] & 0xffff;

	t += 10;

    }
    CS_cfg_name ("");

    spt = (halfword *)msg;
    len = (9 + 21 + n_rows * 10) * 2;	/* message length in bytes */
    spt[0] = 15;			/* message code */
    spt[4] = (len & 0xffff0000) >> 16;
    spt[5] = len & 0xffff;
    spt[8] = 2;				/* number of blocks */

    Send_icd_msg ((char *)Output_buf, len);
    return 0;

}

/**************************************************************************

    Description: This function reads and parses the six product parameters.
		This is adapted from a similar function in ipi.c.

    Output:	params: The parameter array.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

#define TBUF_SIZE 16

static int Read_params (int *params)
{
    char tmp[TBUF_SIZE];
    int i;

    for (i = 0; i < 6; i++) {
	if (CS_entry ("params", i + 1, TBUF_SIZE, (void *)tmp) > 0) {
	    int v;

	    if (sscanf (tmp, "%d", &v) == 1)
		params[i] = v;
	    else if (strcmp (tmp, "UNU") == 0)
		params[i] = PARAM_UNUSED;
	    else if (strcmp (tmp, "ANY") == 0)
		params[i] = PARAM_ANY_VALUE;
	    else if (strcmp (tmp, "ALG") == 0)
		params[i] = PARAM_ALG_SET;
	    else if (strcmp (tmp, "ALL") == 0)
		params[i] = PARAM_ALL_VALUES;
	    else if (strcmp (tmp, "EXS") == 0)
		params[i] = PARAM_ALL_EXISTING;
	    else
		return (-1);
	}
	else 
	    return (-1);
    }
    return (0);
}

/**************************************************************************

    Description: This function reads and parses the up to 12 rows in 
                 bias table.

    Input:      n_rows - number of bias table rows.

    Output:	params: The parameter array.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

#define TBUF_SIZE 16

static int Read_bias_rows (int n_rows, int *mem_span, int *n_pairs, int *avg_gage,
                           int *avg_radar, int *bias)
{
    int i;

    for (i = 0; i < n_rows; i++) {

	if (CS_entry ("mem_span", (i + 1) | CS_INT, 0, (void *) &mem_span[i]) <= 0) 
	   return (-1);

    }

    for (i = 0; i < n_rows; i++) {

	if (CS_entry ("n_pairs", (i + 1) | CS_INT, 0, (void *) &n_pairs[i]) <= 0) 
	   return (-1);

    }

    for (i = 0; i < n_rows; i++) {

	if (CS_entry ("avg_gage", (i + 1) | CS_INT, 0, (void *) &avg_gage[i]) <= 0) 
	   return (-1);

    }

    for (i = 0; i < n_rows; i++) {

	if (CS_entry ("avg_radar", (i + 1) | CS_INT, 0, (void *) &avg_radar[i]) <= 0) 
	   return (-1);

    }

    for (i = 0; i < n_rows; i++) {

	if (CS_entry ("bias", (i + 1) | CS_INT, 0, (void *) &bias[i]) <= 0) 
	   return (-1);

    }

    return (0);
}

/***********************************************************************

    Description: This function sends a connect request.

***********************************************************************/

int Md_connect () {

    if (!Allow_ICD_send && Connect_state != CS_DISCONNECTED) {
	printf ("cannot connect in state %d\n", Connect_state);
	return (0);
    }

    Send_a_request (CM_CONNECT, 0);
    Connect_state = CS_CONNECTING;
    return (0);
}

/***********************************************************************

    Description: This function sends a disconnect request.

***********************************************************************/

int Md_disconnect () {

    Send_a_request (CM_DISCONNECT, 0);
    Connect_state = CS_DISCONNECTING;
    return (0);
}

/**************************************************************************

    Sends a request message to the comm_manager.

    Inputs:	type - request type.
		parm - request parameter.

    Return:	0 on success of -1 on failure.

**************************************************************************/

static int Send_a_request (int type, int parm) {
    CM_req_struct req;
    int ret;

    N_reqs++;
    req.type = type;
    req.parm = parm;
    req.req_num = N_reqs;
    req.link_ind = Cur_link_ind;
    req.time = time (NULL);
    req.data_size = 0;

    ret = LB_write (Output_LB, (char *)&req, 
					sizeof (CM_req_struct), LB_ANY);
    if (ret < 0) {
	printf ("LB_write comm_manager req failed (ret %d))", ret);
	return (-1);
    }
    EN_post_event (/* ORPGEVT_NB_COMM_REQ */ 200 + Cur_link_ind);

    return (0);
}

/***********************************************************************

    Description: This function generates a alert request message and 
		sends it to the p_server.

***********************************************************************/

int Md_alert ()
{

    printf ("alert is not implemented\n");
    return (0);
}

/***********************************************************************

    Description: This function generates a product list request message and 
		sends it to the p_server.

***********************************************************************/

int Md_product_list_req ()
{
    int len;
    char *msg;
    halfword *spt;

    if (Use_comms_manager) 
	msg = Output_buf + sizeof (CM_req_struct);
    else
	msg = Output_buf + sizeof (CM_resp_struct);
    spt = (halfword *)msg;
    Set_message_header_block (msg);

    len = 18;				/* message length in bytes */
    spt[0] = 8;				/* message code */
    spt[4] = len << 16;
    spt[5] = len & 0xffff;
    spt[8] = 1;				/* number of blocks */

    Send_icd_msg ((char *)Output_buf, len);
    return (0);
}

/***********************************************************************

    Description: This function generates the text for displaying a
		p_server message. The text is then sent to the text
		window.

***********************************************************************/

static void Display_msg (char *msg, int size)
{
    static char text[10000];
    halfword *hpt;
    int pcode, pc_ind;
    static char *pcode_name[] = {
	"", "", "GEN STATUS", "REQ RESPONSE", "",
	"", "ALERT PARAMS", "", "PROD LIST", "ALERT", 
	"RCM EDIT REQ", "", "REQ APUP STATUS", "", "",
	"", "PRODUCT"
    };
    time_t tm;
    int len, date, t, yy, mon, dd, hh, mm, ss;

    if (Bytes_expected > 0) {		/* a continued message */
	Print_bytes (text, msg, size);
	Bytes_expected -= size;
	if (Bytes_expected < 0)
	    Bytes_expected = 0;
	Output_text (text);
	return;
    }

    /* swap the message header block */
    MISC_short_swap (msg, SIZE_OF_PD_MSG_HEADER / sizeof (short));

    hpt = (halfword *)msg;
    pcode = hpt[0];
    pc_ind = pcode;
    if (pc_ind < 0)
	pc_ind = 0;
    if (pc_ind > 16)
	pc_ind = 16;
    date = hpt[1];
    t = (hpt[2] << 16) | (hpt[3] & 0xffff);
    len = (hpt[4] << 16) | (hpt[5] & 0xffff);
    tm = UNIX_SECONDS_FROM_RPG_DATE_TIME (date, t * 1000);
    unix_time (&tm, &yy, &mon, &dd, &hh, &mm, &ss);
    sprintf (text, 
	"MHB: code %d (%s), %.2d/%.2d/%.2d %.2d:%.2d:%.2d, len %d, (%d %d), nb %d\n", 
	pcode, pcode_name[pc_ind], mon, dd, yy, hh, mm, ss, 
	len, hpt[6], hpt[7], hpt[8]);
    if (size == 10240)
	Bytes_expected = len;
    else if (size != len) 
	printf ("p_server message length error: (%d, %d)\n", size, len);

    /* print following bytes */
    Print_bytes (text + strlen (text), msg + 18, size - 18);

    Bytes_expected -= size;
    if (Bytes_expected < 0)
	Bytes_expected = 0;

    Output_text (text);
    return;
}

/***********************************************************************

    Description: This function prints an array of bytes in buffer "text".

    Inputs:	msg - the byte array;
		n_bytes - size of the byte array.

    Output:	text - pointer to the buffer for printed text.

***********************************************************************/

static void Print_bytes (char *text, char *msg, int size)
{
    int nt, off, i;

    nt = size;
    if (nt > N_chars)
	nt = N_chars;
    off = 0;
    for (i = 0; i < nt; i++) {
	sprintf (text + off, "%.2x ", (unsigned int)msg[i] & 0xff);
	off += 3;
	if ((i % 20) == 9) {
	    strcpy (text + off, "   ");
	    off += 3;
	}
	if ((i % 20) == 19) {
	    text[off] = '\n';
	    off += 1;
	}
    }
    if (text[off - 1] != '\n') {
	text[off] = '\n';
	off++;
    }
    text[off] = '\n';
    off++;
    text[off] = '\0';
    return;
}

/***********************************************************************

    Description: This function swaps bytes of an integer array.

    Inputs:	buf - the integer array;
		size - size of the array.

***********************************************************************/

static void Int_array_byte_swap (void *buf, int size)
{
    if (Little_endian) {
	unsigned int i, *ipt;

	ipt = (unsigned int *)buf;
	for (i = 0; i < size; i++) {
	    *ipt = INT_BSWAP(*ipt);
	    ipt++;
	}
    }
    return;
}
