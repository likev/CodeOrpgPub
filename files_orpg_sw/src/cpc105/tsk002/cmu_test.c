
/******************************************************************

	file: cmu_test.c

	This is the test program for comm_managers.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2000/07/14 15:30:53 $
 * $Id: cmu_test.c,v 1.7 2000/07/14 15:30:53 jing Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <comm_manager.h>


#define TEST_BUFFER_SIZE 32000

static char Req_name [NAME_SIZE];
static char Resp_name_1 [NAME_SIZE];
static char Resp_name_2 [NAME_SIZE];

static int Req_fd;
static int Resp_fd_1;
static int Resp_fd_2;

static int Line_ind[2];

static int N_reqs = 0;			/* number of requests sent */

static char Buf[TEST_BUFFER_SIZE];

static char St_msg[] = "This is the start";
static char End_msg[] = "This is the end";


/* local functions */
static void Send_request (int type, int tind, int parm, 
					int data_size, char *data);
static int Read_responses (int tind, int n);
static int Read_response_of_type (int tind, int type);
static void Print_data (int len, char *buf);


/******************************************************************

    Description: The main function of comm_manager.

******************************************************************/

void main (int argc, char **argv)
{
    int num;
    int cm_id;
    int tm;

    cm_id = 2;
    Line_ind[0] = 8;
    Line_ind[1] = 9;
    sprintf (Req_name, "%s.%d", "/home/jing/uconx/req", cm_id);
    sprintf (Resp_name_1, "%s.%d", "/home/jing/uconx/resp", Line_ind[0]);
    sprintf (Resp_name_2, "%s.%d", "/home/jing/uconx/resp", Line_ind[1]);

    if (argc < 2 || strcmp (argv[1], "-h") == 0 ||
	sscanf (argv[1], "%d", &num) != 1 ||
	num <= 0 || num > 13) {
	printf ("usage: %s test_number\n", argv[0]);
	printf ("    test 1: A general test\n");
	printf ("    test 2: Disconnect two links\n");
	printf ("    test 3: connect two link\n");
	printf ("    test 4: Disconnect two links with delay\n");
	printf ("    test 5: Write test (link %d)\n", Line_ind[0]);
	printf ("    test 6: Read test (link %d)\n", Line_ind[1]);
	printf ("    test 7: Disconnect (link %d)\n", Line_ind[0]);
	printf ("    test 8: Disconnect (link %d)\n", Line_ind[1]);
	printf ("    test 9: Connect (link %d)\n", Line_ind[0]);
	printf ("    test 10: Connect (link %d)\n", Line_ind[1]);
	printf ("    test 11: NEXRAD loopback (link %d)\n", Line_ind[0]);
	printf ("    test 12: Write test (link %d)\n", Line_ind[1]);
	printf ("    test 13: Read test (link %d)\n", Line_ind[0]);
	exit (0);
    }

    /* open LBs */
    Req_fd = LB_open (Req_name, LB_WRITE, NULL);
    if (Req_fd < 0) {
	LE_send_msg (0, "LB_open %s failed (ret = %d)", Req_name, Req_fd);
	exit (1);
    }
    Resp_fd_1 = LB_open (Resp_name_1, LB_READ, NULL);
    if (Resp_fd_1 < 0) {
	LE_send_msg (0, "LB_open %s failed (ret = %d)", Resp_name_1, Resp_fd_1);
	exit (1);
    }
    Resp_fd_2 = LB_open (Resp_name_2, LB_READ, NULL);
    if (Resp_fd_2 < 0) {
	LE_send_msg (0, "LB_open %s failed (ret = %d)", Resp_name_2, Resp_fd_2);
	exit (1);
    }

    switch (num) {
	static char buf [TEST_BUFFER_SIZE];
	int i, ret;

	case 7:
	printf ("CM_DISCONNECT, link %d\n", Line_ind[0]);
	Send_request (CM_DISCONNECT, 0, 0, 0, NULL); 
	Read_response_of_type (0, CM_DISCONNECT);
	break;

	case 8:
	printf ("CM_DISCONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_DISCONNECT, 1, 0, 0, NULL); 
	Read_response_of_type (1, CM_DISCONNECT);
	break;

	case 9:
	printf ("CM_CONNECT, link %d\n", Line_ind[0]);
	Send_request (CM_CONNECT, 0, 0, 0, NULL); 
	Read_response_of_type (0, CM_CONNECT);
	break;

	case 10:
	printf ("CM_CONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_CONNECT, 1, 0, 0, NULL); 
	Read_response_of_type (1, CM_CONNECT);
	break;

	case 11:
	{
	    char buf [1000];
	    short *spt;
	    int *ipt;
	    int len;

	    spt = (short *)&buf[12];
	    ipt = (int *)spt;
	    spt[0] = 100;		/* message_len  */
	    spt[1] = 12;		/*  message_type */
	    spt[2] = 3;		/* seq_num  */
	    spt[3] = 9896;		/*  julian_date */
	    spt[6] = 1;		/* num_message_segs  */
	    spt[7] = 1;		/* message_seg_num  */
	    ipt[2] = 43566;		/* millisecs */

	    spt[8] = spt[0] - 8;		/*   */
	    for (i = 0; i < 120; i++)
		spt [9 + i] = i;

printf ("0 - %d, 1 - %d, 91 - %d, 92 - %d, 93 - %d, 94 - %d\n", spt[8 + 0], spt[8 + 1], spt[8 + 91], spt[8 + 92], spt[8 + 93], spt[8 + 94]);

	    len = (spt[0] + 6) * 2;
	    Send_request (CM_WRITE, 0, 1, len, buf + 10);
	    printf ("loop back message (len = %d) sent to link %d\n", len, Line_ind[0]);
	}
	break;

	case 5:
	for (i = 0; i < 5; i++) {
	    int len;

	    len = (rand () % 1000) + 50;
len = 2096;
	    strcpy (buf, St_msg);
	    strcpy (buf + len - strlen (End_msg) - 1, End_msg);
	    printf ("CM_WRITE, len %d, link %d\n", len, Line_ind[0]);
	    Send_request (CM_WRITE, 0, 1, len, buf);
	    ret = Read_responses (0, 1);
	    if (ret != CM_SUCCESS) {
		printf ("CM_WRITE failed (ret = %d), link %d\n", ret, Line_ind[0]);
		break;
	    }
exit (0);
	}
	tm = time (NULL);
	printf ("Fast write ... \n");
for (i = 0; i < 20000; i++) {
int k;
	for (k = 0; k < 5; k++) {
	    int len;

	    len = (rand () % 1000) + 50;
len = 4096;
	    strcpy (buf, St_msg);
	    strcpy (buf + len - strlen (End_msg) - 1, End_msg);
	    printf ("CM_WRITE, len %d, link %d\n", len, Line_ind[0]);
	    Send_request (CM_WRITE, 0, 1, len, buf);
	}
	for (k = 0; k < 5; k++) {
	    ret = Read_responses (0, 1);
	    if (ret != CM_SUCCESS) {
		printf ("CM_WRITE failed (ret = %d), link %d\n", ret, Line_ind[0]);
		break;
	    }
	}
	msleep (500);
}
	printf ("time used: %d\n", (int)(time(NULL) - tm));
	break;

	case 12:
	for (i = 0; i < 5; i++) {
	    int len;

	    len = (rand () % 1000) + 50;
len = 2096;
	    strcpy (buf, St_msg);
	    strcpy (buf + len - strlen (End_msg) - 1, End_msg);
	    printf ("CM_WRITE, len %d, link %d\n", len, Line_ind[1]);
	    Send_request (CM_WRITE, 1, 1, len, buf);
	    ret = Read_responses (1, 1);
	    if (ret != CM_SUCCESS) {
		printf ("CM_WRITE failed (ret = %d), link %d\n", ret, Line_ind[1]);
		break;
	    }
exit (0);
	}
	break;

	case 6:
	printf ("Reading data on link %d\n", Line_ind[1]);
	Read_responses (1, 100);
	break;

	case 13:
	printf ("Reading data on link %d\n", Line_ind[0]);
	Read_responses (0, 100);
	break;

	case 4:
	printf ("CM_CONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_CONNECT, 1, 0, 0, NULL); 
	printf ("wait 20 seconds ... \n");
	sleep (20);
	printf ("CM_CONNECT, link %d\n", Line_ind[0]);
	Send_request (CM_CONNECT, 0, 0, 0, NULL); 
	Read_responses (-1, 2);
	break;

	case 2:
	printf ("CM_DISCONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_DISCONNECT, 1, 0, 0, NULL); 
/*	sleep (4); */
	printf ("CM_DISCONNECT, link %d\n", Line_ind[0]);
	Send_request (CM_DISCONNECT, 0, 0, 0, NULL); 
	Read_responses (-1, 2);
	break;
 
	case 3:
	printf ("CM_CONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_CONNECT, 1, 0, 0, NULL); 
	printf ("CM_CONNECT, link %d\n", Line_ind[0]);
	Send_request (CM_CONNECT, 0, 0, 0, NULL); 
	Read_responses (-1, 2);
	break;

	case 1:
	printf ("CM_CONNECT, link %d\n", Line_ind[0]);
	Send_request (CM_CONNECT, 0, 0, 0, NULL);
	printf ("CM_CONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_CONNECT, 1, 0, 0, NULL); 
	printf ("CM_DISCONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_DISCONNECT, 1, 0, 0, NULL); 
	Read_responses (-1, 3);

	printf ("wait 8 seconds ... \n");
	sleep (8);
	printf ("CM_CONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_CONNECT, 1, 0, 0, NULL); 
	Read_responses (-1, 1);

	printf ("wait 8 seconds ... \n");
	sleep (8);
	printf ("CM_DISCONNECT, link %d\n", Line_ind[1]);
	Send_request (CM_DISCONNECT, 1, 0, 0, NULL); 
	Read_responses (-1, 1);
	break;

	default:
	printf ("test is not implemented\n");
	break;
    }

    exit (0);
}


/******************************************************************

    Description: This function sends a request to the request LB.

    Inputs:	type - request type;
		tind - test index;
		parm - request parameter;
		data_size - size of the data (CM_WRITE only);
		data - data to be written (CM_WRITE only);

******************************************************************/

static void Send_request (int type, int tind, int parm, 
					int data_size, char *data)
{
    CM_req_struct *req;
    int ret;

    N_reqs++;
    req = (CM_req_struct *)Buf;
    req->type = type;
    req->req_num = N_reqs;
    req->link_ind = Line_ind[tind];
    req->parm = parm;
    if (type == CM_WRITE) {
	req->data_size = data_size;
	if (data_size + (int)sizeof (CM_req_struct) > TEST_BUFFER_SIZE) {
	    LE_send_msg (0, "data too large (size = %d)", data_size);
	    exit (1);
	}
	memcpy (Buf + sizeof (CM_req_struct), data, data_size);
    }
    else {
	req->data_size = 0;
    }

    ret = LB_write (Req_fd, Buf, 
			req->data_size + sizeof (CM_req_struct), LB_ANY);
    if (ret < 0) {
	LE_send_msg (0, "LB_write failed (ret = %d)", ret);
	exit (1);
    }

    return;
}

/******************************************************************

    Description: This function reads n responses before it returns.

    Inputs:	tind - test index to be processed; -1 means responses 
			from all links are processed.
		n - number of responses received before return;

    Return:	It returns the ret_code of the last response.

******************************************************************/

static int Read_responses (int tind, int n)
{
    int n_read;
    int value;
    int first, second;

    if (tind == 0)
	first = second = Resp_fd_1;
    else if (tind == 1)
	first = second = Resp_fd_2;
    else if (tind < 0) {
	first = Resp_fd_1; 
	second = Resp_fd_2;
    }
    else
	first = second = -1;

    n_read = 0;
    while (n_read < n) {
	int ret;

	ret = LB_read (first, Buf, TEST_BUFFER_SIZE, LB_NEXT);
	if (ret == LB_TO_COME && first != second)
	    ret = LB_read (second, Buf, TEST_BUFFER_SIZE, LB_NEXT);
	if (ret == LB_TO_COME) {
	    msleep (50);
	    continue;
	}
	if (ret > 0) {
	    CM_resp_struct *resp;

	    if (ret < (int)sizeof (CM_resp_struct)) {
		LE_send_msg (0, "Bad response message (len = %d)", ret);
		exit (1);
	    }
	    resp = (CM_resp_struct *)Buf;

	    if (tind >= 0 && tind <= 1 && Line_ind[tind] != resp->link_ind)
		continue;

	    printf ("Res: type %d, num %d, link %d, ret_val %d, d_size %d\n", 
			resp->type, resp->req_num, resp->link_ind, 
			resp->ret_code, resp->data_size);
	    value = resp->ret_code;

	    if (resp->type == CM_DATA)
		Print_data (resp->data_size, Buf + sizeof (CM_resp_struct));
	}
	if (ret < 0) {
	    LE_send_msg (0, "LB_read failed (ret = %d)", ret);
	    exit (1);
	}
	n_read++;
    }

    return (value);
}

/******************************************************************

    Description: This function reads 1 responses of type "type" 
		before it returns.

    Inputs:	tind - test index to be processed; -1 means responses 
			from all links are processed.
		type - responses type to read;

    Return:	It returns the ret_code of the response.

******************************************************************/

static int Read_response_of_type (int tind, int type)
{
    int n_read;
    int value;
    int first, second;

    if (tind == 0)
	first = second = Resp_fd_1;
    else if (tind == 1)
	first = second = Resp_fd_2;
    else if (tind < 0) {
	first = Resp_fd_1; 
	second = Resp_fd_2;
    }
    else
	first = second = -1;

    n_read = 0;
    while (n_read < 1) {
	int ret;

	ret = LB_read (first, Buf, TEST_BUFFER_SIZE, LB_NEXT);
	if (ret == LB_TO_COME && first != second)
	    ret = LB_read (second, Buf, TEST_BUFFER_SIZE, LB_NEXT);
	if (ret == LB_TO_COME) {
	    sleep (1);
	    continue;
	}
	if (ret > 0) {
	    CM_resp_struct *resp;

	    if (ret < (int)sizeof (CM_resp_struct)) {
		LE_send_msg (0, "Bad response message (len = %d)", ret);
		exit (1);
	    }
	    resp = (CM_resp_struct *)Buf;

	    if (tind >= 0 && tind <= 1 && Line_ind[tind] != resp->link_ind)
		continue;
	    if (resp->type != type)
		continue;

	    printf ("Res: type %d, num %d, link %d, ret_val %d, d_size %d\n", 
			resp->type, resp->req_num, resp->link_ind, 
			resp->ret_code, resp->data_size);
	    value = resp->ret_code;

	    if (resp->type == CM_DATA)
		Print_data (resp->data_size, Buf + sizeof (CM_resp_struct));
	}
	if (ret < 0) {
	    LE_send_msg (0, "LB_read failed (ret = %d)", ret);
	    exit (1);
	}
	n_read++;
    }

    return (value);
}

/******************************************************************

    Description: This function reads n responses before it returns.
		It returns the ret_code of the last response.

******************************************************************/

static void Print_data (int len, char *buf)
{

    if (len <= 0 || len + (int)sizeof (CM_resp_struct) > TEST_BUFFER_SIZE) {
	printf ("Bad data length (%d)\n", len);
	return;
    }
    buf [TEST_BUFFER_SIZE - sizeof (CM_resp_struct) - 1] = '\0';

    printf ("len = %d:-%s...%s-\n", len, buf, 
			buf + len - strlen (End_msg) - 1);
    return;
}
