
/******************************************************************

	file: cmu_hdlc_abm.c

	This module contains the HDLC/ABM processing functions for 
	the comm_manager - UCONX version.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/01/30 15:03:14 $
 * $Id: cmu_hdlc_abm.c,v 1.13 2008/01/30 15:03:14 jing Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/poll.h>

#include <infr.h>
#include <comm_manager.h>
#include <cmu_def.h>
#ifdef HPUX
#include <sys/errno.h> 
#else
#include <errno.h>	/* MPS include file */
#endif
#include <xstopts.h>
#include <xstypes.h>
#include <xstpoll.h>
#include <mpsproto.h>
#include <dlpiabm.h>

extern int CMU_need_unbind;	/* defined in cmu_main.c */

static int N_links;		/* Number of links managed by this process */
static Link_struct **Links;	/* link structure list */


/* shared packet buffers */
static char *Cntlbuff;
static char *Databuff;
static struct xstrbuf *Control;
static struct xstrbuf *Data;

/* The following are for printing messages */
static char		Responses   [ ] [ 30 ] =
{
   "NULL", "DL_BIND_ACK", "DL_BIND_REQ", "DL_CONNECT_CON", "DL_CONNECT_REQ",
   "DL_DISCONNECT_IND", "DL_DISCONNECT_REQ", "DL_ERROR_ACK",
   "DL_GET_STATISTICS_ACK", "DL_GET_STATISTICS_REQ", "DL_OK_ACK",
   "DL_RESET_IND", "DL_RESET_REQ", "DL_RESET_CON", "DL_UNBIND_REQ",
   "DL_TEST_IND", "DL_DATA_REQ", "DL_DATA_IND"
};

static char		Error_acks  [ ] [ 30 ] =
{
   "DL_DLSAP_IN_USE", "DL_LL2_REJECT", "DL_NOTINIT", "DL_TIMEDOUT",
   "DL_NOBUFFERS", "DL_LINK_DOWN", "NULL", "DL_BADDATA", 
   "DL_REJ_AND_SREJ", "DL_INVALID_ADDRESS"
};

static char		Disc_orig   [ ] [ 30 ] =
{
   "DL_REMOTE", "DL_LOCAL"
};

static char		Disc_reason [ ] [ 30 ] =
{
   "DL_START_FAIL", "DL_CONNECT_FAIL", "DL_DISC_RCVD", "DL_NO_SIG"
};

/* local functions */
static void Disconnect (Link_struct *link);
static void End_hdlc_disconnecting (Link_struct *link);
static void Connect (Link_struct *link);
static void End_hdlc_connecting (Link_struct *link);
static void Read_data (Link_struct *link, int pvc);
static void Unbind (Link_struct *link);
static void Do_unbind (Link_struct *link);
static void Bind (Link_struct *link);
static void Process_exception (Link_struct *link);


/**************************************************************************

    Description: This function initializes this module.

    Inputs:	n_links - number of links;
		links - the list of the link structure;
		control_buf - shared packet buffer - the control part;
		data_buf - shared packet buffer - the data part;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int HA_initialize (int n_links, Link_struct **links,
		struct xstrbuf *control_buf, struct xstrbuf *data_buf)
{

    N_links = n_links;
    Links = links;

    Control = control_buf;
    Data = data_buf;
    Cntlbuff = Control->buf;
    Databuff = Data->buf;

    return (0);
}

/**************************************************************************

    Description: This function opens and binds all HDLC/ABM links.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int HA_config_protocol ()
{
    int i;

    /* open and bind HDLC/ABM links */
    for (i = 0; i < N_links; i++) {
	Link_struct *link;
	int sock;

	link = Links[i];
	if (link->proto != PROTO_HDLC)
	    continue;

	if ((sock = MPSopen (link->oreq)) == ERROR) {
	    MPSperror ( "Unable to open connection to the MPS server" );
	    return (-1);
	}
	link->client_id[0] = sock;
	LE_send_msg (LE_VL1, "MPSopen succeeded (client_id %d)", sock);

	/* set non_blocking mode */
	if (MPSioctl (sock, X_SETIOTYPE, (char *)NONBLOCK) == -1) {
	    LE_send_msg (GL_ERROR, 
			"MPSioctl NONBLOCK failed (errno = %d)\n", errno);
	    MPSclose (sock);
	    link->client_id[0] = -1;
	    return (-1);
	}
    }

    return (0);
}

/**************************************************************************

    Description: This function processes a packet received from the HDLC.
		The control and data parts are assumed in Cntlbuff and 
		Databuff respectively.

    Inputs:	link - the link from which the message is received.

    Return:	It returns the packet primitive.

**************************************************************************/

int HA_process_packets (Link_struct *link)
{
    dlpi_primitive      *p_req;
    dl_error_ack_t      *p_err;
    dl_disconnect_ind_t *p_disc;
    dl_ok_ack_t         *p_ok;
    dl_data_t *p_data;
    bit32 *ipt;

    p_req = (dlpi_primitive *) Cntlbuff;
    p_req->dl_primitive = htonl (p_req->dl_primitive);
    ipt = (bit32 *)Cntlbuff + 1;

/*
LE_send_msg (0, "Received %s\n", Responses[p_req->dl_primitive]);
*/

    switch (p_req->dl_primitive) {
	int i;

	case DL_BIND_ACK:	/* bind success */
	    LE_send_msg (LE_VL1, 
			"        bind success (link %d)\n", link->link_ind);
	    if (link->bind_state[0] == ENABLING) {
		link->bind_state[0] = ENABLED;
		Bind (link);
	    }
	    else {
		LE_send_msg (GL_ERROR, 
			"        ERROR: unexpected CS_BIND_SUCCESS");
		link->bind_state[0] = ENABLED;
	    }
	    break;

	case DL_CONNECT_CON:	/* connection built */
	    LE_send_msg (LE_VL1, 
			"        DL_CONNECT_CON: link %d\n", link->link_ind);

	    if (link->connect_state == ENABLING) { /* originated locally */
	        link->connect_state = ENABLED;
		Connect (link);
	    }
	    else {				/* originated remotely */
	        link->connect_state = ENABLED;
		LE_send_msg (LE_VL1, 
			"        connection resumed remotely: link %d\n", 
							link->link_ind);
	    }
	    break;

	case DL_DISCONNECT_IND:
	    p_disc = (dl_disconnect_ind_t *)Cntlbuff;
	    for (i = 1; i < sizeof (dl_disconnect_ind_t) / sizeof (bit32); i++)
		{*ipt = ntohl (*ipt); ipt++;}
	    LE_send_msg (LE_VL1, 
		    "DL_DISCONNECT_IND: link %d; Originator %s; Reason %s\n", 
			link->link_ind, Disc_orig[p_disc->dl_originator], 
			Disc_reason[p_disc->dl_reason]);
	    if (p_disc->dl_originator == DL_REMOTE) {	/* connection lost */
		LE_send_msg (LE_VL1, 
			"        connection lost: link %d\n", link->link_ind);
		link->connect_state = DISABLED;
		Process_exception (link);
		break;
	    }
	    else {	/* locally originated connecting command failed - This 
			   is time-out */
		/* different procedures could be processed in terms of the 
		   failure reasons (p_disc->dl_reason); See p 18 of HDLC/ABM 
		   programmer's guide */
		LE_send_msg (LE_VL1, 
				"CS_CONN_TIMEOUT: link %d\n", link->link_ind);
		link->connect_state = DISABLED;
		Process_exception (link);
	    }
	    break;

	case DL_ERROR_ACK:
	    p_err = (dl_error_ack_t *)Cntlbuff;
	    for (i = 1; i < sizeof (dl_error_ack_t) / sizeof (bit32); i++)
		{*ipt = ntohl (*ipt); ipt++;}

LE_send_msg (0, "        Error response to %s\n", 
                   		Responses[p_err->dl_error_primitive]);
LE_send_msg (0, "        Error was %s\n", 
				Error_acks[p_err->dl_errno - 0xe0]);

	    if (p_err->dl_error_primitive == DL_DISCONNECT_REQ &&
			p_err->dl_errno == DL_LINK_DOWN) {
				/* connecting process terminated */
		LE_send_msg (LE_VL1, 
			"        connect process terminated: link %d\n", 
						link->link_ind); 
		link->connect_state = DISABLED;
		Process_exception (link);
		break;
 	    }

	    if (p_err->dl_error_primitive == DL_BIND_REQ) {
		LE_send_msg (LE_VL1, "        DL_BIND_REQ failed: link %d\n", 
						link->link_ind);
		if (link->bind_state[0] != ENABLING)
		    LE_send_msg (GL_ERROR, 
			"        Unexpected DL_BIND_REQ failure: link %d",
							link->link_ind);
		link->bind_state[0] = DISABLED;
		Process_exception (link);
	    }

	    if (p_err->dl_error_primitive == DL_UNBIND_REQ) {
		LE_send_msg (LE_VL1, 
			"        DL_UNBIND_REQ failed: link %d\n", 
						link->link_ind);
		if (link->bind_state[0] == DISABLING)
		    link->bind_state[0] = ENABLED;
		else
		    LE_send_msg (GL_ERROR, 
			"        Unexpected DL_UNBIND_REQ failure");
		Process_exception (link);
	    }

	    if (p_err->dl_error_primitive == DL_CONNECT_REQ) {
		LE_send_msg (LE_VL1, "        CONN_REJECT: link %d\n", 
						link->link_ind);
		link->connect_state = DISABLED;
		Process_exception (link);
	    }

	    if (p_err->dl_error_primitive == DL_DISCONNECT_REQ) {
		LE_send_msg (LE_VL1, 
				"        DL_DISCONNECT_REQ failed: link %d\n", 
							link->link_ind);
		if (link->connect_state == DISABLING) {	/* originated locally */
		    /* The other side does not responding - probably unbound.
		       We clean up our side and will try reconnect anyway. So
		       we set all states to disconnected. */
		    link->connect_state = DISABLED;
		}
		else
		    LE_send_msg (0, "        Unexpected DISCONN_REJECT");
		link->connect_state = DISABLED;
		Process_exception (link);
	    }

	    break;

	case DL_OK_ACK:
	    p_ok = (dl_ok_ack_t *)Cntlbuff;
	    for (i = 1; i < sizeof (dl_ok_ack_t) / sizeof (bit32); i++)
		{*ipt = ntohl (*ipt); ipt++;}

	    if (p_ok->dl_correct_primitive == DL_DISCONNECT_REQ) {
		LE_send_msg (LE_VL1, "        DISCONN SUCCESS: link %d\n", 
							link->link_ind);
		if (link->connect_state == DISABLING) {
		    link->connect_state = DISABLED;
		    Disconnect (link);
		}
		else
		    LE_send_msg (0, "        Unexpected DISCONN_SUCCESS");
		link->connect_state = DISABLED;
		break;
	    }
	    else if (p_ok->dl_correct_primitive == DL_UNBIND_REQ) {
		LE_send_msg (LE_VL1, "        UNBIND SUCCESS: link %d\n", 
							link->link_ind);
		if (link->bind_state[0] == DISABLING) {
		    link->bind_state[0] = DISABLED;
		    Unbind (link);
		}
		else
		    LE_send_msg (GL_ERROR, 
				"        Unexpected UNBIND SUCCESS");
		link->bind_state[0] = DISABLED;
		break;
	    }
	    else {
		LE_send_msg (LE_VL1, "        unexpected DL_OK_ACK to %s\n", 
				Responses[p_ok->dl_correct_primitive]);
	    }

	    break;
 
	case DL_RESET_IND:
LE_send_msg (0, "             RESET_IND: link %d\n", link->link_ind);
	    /* we may need to send a response to the user */
	    break;

	case DL_DATA_IND:
/*
LE_send_msg (0, "              DATA_IND: link %d\n", link->link_ind);
*/
	    p_data = (dl_data_t *)Cntlbuff;
	    for (i = 1; i < sizeof (dl_data_t) / sizeof (bit32); i++)
		{*ipt = ntohl (*ipt); ipt++;}
	    if(p_data->dl_data_type != DL_I_FORMAT)
		LE_send_msg (GL_ERROR, "        Not an I frame - discarded\n");
	    else
		Read_data (link, 0);
	    break;

	default:
	    LE_send_msg (GL_ERROR, "        unexpected HDLC packet (%d)", 
						p_req->dl_primitive);
	    break;
    }

    return (p_req->dl_primitive);
}

/**************************************************************************

    Description: This function starts a connection procedure.

    Inputs:	link - the link involved.

**************************************************************************/

void HA_connection_procedure (Link_struct *link)
{

    if (!(SH_config_OK ()))
	return;

    link->link_state = LINK_DISCONNECTED;
    CMPR_cleanup (link);
    Disconnect (link);

    return;
}

/**************************************************************************

    Description: This function implements the first step of the HDLC
		connection procedure. It disconnects the connection.

    Inputs:	link - the link involved.

**************************************************************************/

static void Disconnect (Link_struct *link)
{

    /* HDLC disconnection */
    if (link->connect_state == ENABLED || link->connect_state == ENABLING) {
	dlpi_primitive *p_req;

	link->connect_state = DISABLING;
	LE_send_msg (LE_VL1, "disconnecting link %d, client id %d\n", 
					link->link_ind, link->client_id[0]);

	p_req = (dlpi_primitive *)Control->buf;
	p_req->dl_primitive = htonl (DL_DISCONNECT_REQ);
        Control->len = sizeof (dlpi_primitive);
	if (MPSputmsg (link->client_id[0], Control, NULL, 0) == -1) {
	    MPSperror ( "MPSputmsg DL_DISCONNECT_REQ failed" );
	    return;
	}
    }
    else if (link->connect_state == DISABLED)
	Unbind (link);

    return;
}

/**************************************************************************

    Description: This function implements the second step of the connection
		procedure. It unbinds all HDLC/ABM clients on a link.

    Inputs:	link - the link involved.

**************************************************************************/

static void Unbind (Link_struct *link)
{

    if (CMU_need_unbind)
	Do_unbind (link);
    else
	End_hdlc_disconnecting (link);

    return;
}

/**************************************************************************

    Description: This function implements the second step of the connection
		procedure. It unbinds all HDLC/ABM clients on a link.

    Inputs:	link - the link involved.

**************************************************************************/

static void Do_unbind (Link_struct *link)
{

    /* unbind the client */
    if (link->bind_state[0] == ENABLED) {
	dlpi_primitive *p_req;

	LE_send_msg (LE_VL1, "Unbind HDLC link %d\n", link->link_ind);
	link->bind_state[0] = DISABLING;

	/* send unbind command */
	p_req = (dlpi_primitive *)Cntlbuff;
	p_req->dl_primitive = htonl (DL_UNBIND_REQ);
	Control->len = sizeof (dlpi_primitive);
	if (MPSputmsg (link->client_id[0], Control, NULL, 0 ) == -1) {
	    MPSperror ("MPSputmsg unbind failed");
	    return;
	}
    }

    if (link->bind_state[0] == DISABLED)
	End_hdlc_disconnecting (link);

    return;
}

/**************************************************************************

    Description: This function implements the third step of the HDLC
		connection procedure. It finishes the connection requests 
		if the target is to disconnect. Otherwise it continues the 
		procedure to make a new connection.

    Inputs:	link - the link involved.

**************************************************************************/

static void End_hdlc_disconnecting (Link_struct *link)
{

    if (link->conn_activity == DISCONNECTING) {	/* stop here */

	CMPR_cleanup (link);
	LE_send_msg (LE_VL1, "link %d disconnected\n", link->link_ind);

	link->link_state = LINK_DISCONNECTED;
	CMPR_send_response (link, 
			&(link->req[(int)link->conn_req_ind]), CM_SUCCESS);
    }
    else if (link->conn_activity == CONNECTING) /* go on to next step */
	Bind (link);

    return;
}

/**************************************************************************

    Description: This function implements the 4-th step of the connection
		procedure. It binds all CS clients on a link. A non-zero 
		cs_bind return indicates a fatal error and the process is 
		terminated. 

    Inputs:	link - the link involved.

**************************************************************************/

static void Bind (Link_struct *link)
{

    if (link->bind_state[0] == DISABLED) {
	struct xstrbuf cntl;

	link->bind_state[0] = ENABLING;
	LE_send_msg (LE_VL1, "Bind link %d, client id %d\n", 
			    link->link_ind, link->client_id[0]);

	cntl.len = sizeof (dl_bind_req_t);
	cntl.buf = (char *)link->p_bind;
	if (MPSputmsg (link->client_id[0], &cntl, NULL, 0) == (-1)) {
	    MPSperror ( "MPSputmsg failed" );
	    return;
	}
    }

    if (link->bind_state[0] == ENABLED) {
	if (link->conn_activity == CONNECTING)
	    Connect (link);
	else 
	    Disconnect (link);
    }

    return;
}

/**************************************************************************

    Description: This function implements the 5-th step of the HDLC 
		connection procedure. It connects the link.

    Inputs:	link - the link involved.

**************************************************************************/

static void Connect (Link_struct *link)
{

    if (link->connect_state == DISABLED) {
	dlpi_primitive *p_req;

	if (link->conn_activity != CONNECTING) {
	    Disconnect (link);
	    return;
	}

	LE_send_msg (LE_VL1, "connect link %d, client id %d\n", 
				link->link_ind, link->client_id[0]);
	link->connect_state = ENABLING;

	p_req = (dlpi_primitive *)Cntlbuff;
	p_req->dl_primitive = htonl (DL_CONNECT_REQ);
        Control->len = sizeof (dlpi_primitive);
	if (MPSputmsg (link->client_id[0], Control, NULL, 0) == -1) {
	    MPSperror ( "MPSputmsg DL_CONNECT_REQ failed" );
	    return;
	}
    }

    if (link->connect_state == ENABLED) {
	if (link->conn_activity == CONNECTING)
	    End_hdlc_connecting (link);
	else 
	    Disconnect (link);
    }

    return;
}

/**************************************************************************

    Description: This function implements the last step of the HDLC 
		connection procedure. It finishes the procedure.

    Inputs:	link - the link involved.

**************************************************************************/

static void End_hdlc_connecting (Link_struct *link)
{

    if (link->conn_activity != CONNECTING)
	return;

    /* stop here */
    LE_send_msg (LE_VL1, "link %d connected\n", link->link_ind);
    CMPR_send_response (link, 
			&(link->req[(int)link->conn_req_ind]), CM_SUCCESS);
    link->link_state = LINK_CONNECTED;

    return;
}

/**************************************************************************

    Description: This function writes data to the "pvc" on "link".

    Inputs:	link - the link involved.
		pvc - PVC index.

**************************************************************************/

void HA_write_data (Link_struct *link, int pvc)
{

    if (link->link_state != LINK_CONNECTED) {		/* link failed */
	CMPR_send_response (link, &(link->req[(int)link->w_req_ind[pvc]]), 
					CM_DISCONNECTED);
	return;
    }

    if (link->w_ack[pvc] < link->w_size[pvc]) {		/* write more data */

	while (1) {
	    dl_data_t *p_data;
	    struct xstrbuf data;
	    int len, ret;

	    len = link->w_size[pvc] - link->w_cnt[pvc];
	    if (len <= 0)
		break;
	    if (len > link->packet_size) {
		len = link->packet_size;
	    }

	    p_data = (dl_data_t *)Control->buf;
	    p_data->dl_primitive = htonl (DL_DATA_REQ);
	    p_data->dl_data_type = htonl (DL_I_FORMAT);
	    Control->len = sizeof(dl_data_t);
	    data.buf = link->w_buf[pvc] + link->w_cnt[pvc];
	    data.len = len;
	
	    ret = MPSputmsg (link->client_id[pvc], Control, &data, 0);
	    if (ret == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
/*		    Check_out_of_band_data (link->client_id[pvc]); */
		    break;
		}
		else {
		    MPSperror ( "Write Error\n" );
		    CMPR_send_response (link, 
			&(link->req[(int)link->w_req_ind[pvc]]), CM_FAILED);
		    return;
		}
	    }

	    link->w_cnt[pvc] += len;
	    link->w_ack[pvc] += len;
	}
    }

    if (link->w_ack[pvc] >= link->w_size[pvc])	/* done */
	CMPR_send_response (link, &(link->req[(int)link->w_req_ind[pvc]]), 
					CM_SUCCESS);

    return;
}
 
/**************************************************************************

    Description: This function reads data from the "pvc" on "link".

    Inputs:	link - the link involved.
		pvc - PVC index.

**************************************************************************/

static void Read_data (Link_struct *link, int pvc)
{

    /* reallocate the buffer */
    while (link->r_cnt[pvc] + link->packet_size > link->r_buf_size[pvc]) {
	if (CMC_reallocate_input_buffer (link, pvc, 0) < 0)
	    return;
    }

    if (Data->len <= 0) {
	LE_send_msg (0, "Data->len error (%d)", Data->len);
	return;
    }
    memcpy (link->r_buf[pvc] + link->r_cnt[pvc], Data->buf, Data->len);
    link->r_cnt[pvc] += Data->len;

    /* done */
    CMPR_send_data (link, pvc, link->r_cnt[pvc], link->r_buf[pvc]);
    link->r_cnt[pvc] = 0;

    return;
}

/**************************************************************************

    Description: This function closes all HDLC ports.

**************************************************************************/

void HA_clean_up ()
{
    int i;

    /* unbind and close HDLC/ABM links */
    for (i = 0; i < N_links; i++) {
	int cnt;
	dlpi_primitive *p_req;

	if (Links[i]->proto != PROTO_HDLC)
	    continue;

	if (Links[i]->bind_state[0] != DISABLED) {

	    /* send unbind command */
	    p_req = (dlpi_primitive *)Cntlbuff;
	    p_req->dl_primitive = htonl (DL_UNBIND_REQ);
	    Control->len = sizeof (dlpi_primitive);
	    if (MPSputmsg (Links[i]->client_id[0], Control, NULL, 0 ) == -1) {
		MPSperror ("MPSputmsg unbind failed");
		return;
	    }

	    /* Wait for the reply ack */
	    cnt = 0;
	    while (1) {
		dlpi_primitive *p_req;
		dl_ok_ack_t *p_ok;
		int gflg;

		gflg = 0;
		if (MPSgetmsg (Links[i]->client_id[0], Control, Data, &gflg) < 
									0) {
		    if (errno == EAGAIN || errno == EWOULDBLOCK) {
			msleep (500);
			cnt++;
			if (cnt > 10) {
			    LE_send_msg (LE_VL1, "HA link %d unbind failed", 
						Links[i]->link_ind);
			    break;
			}
			continue;
		    }
		    MPSperror ("MPSgetmsg unbind failed");
		    break;
		}
		p_req = (dlpi_primitive *)Cntlbuff;
		if (htonl (p_req->dl_primitive) != DL_OK_ACK)
		    continue;

		p_ok = (dl_ok_ack_t *)Cntlbuff;
		if (htonl (p_ok->dl_correct_primitive) == DL_UNBIND_REQ) {
		    LE_send_msg (LE_VL1, "HA link %d unbound", 
							Links[i]->link_ind);
		    break;
		}
	    }
	}

	/* close the link */
	MPSclose (Links[i]->client_id[0]);
    }

    return;
}

/**************************************************************************

    Description: This function processes lost connection and other
		exception conditions.

    Inputs:	link - the link involved.

**************************************************************************/

static void Process_exception (Link_struct *link)
{

    CMPR_connect_failure (link);
    Disconnect (link);

    return;
}

