
/******************************************************************

	file: cmu_x25_pvc.c

	This module contains the X25/PVC processing functions for 
	the comm_manager - UCONX version.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/01/30 15:03:16 $
 * $Id: cmu_x25_pvc.c,v 1.32 2008/01/30 15:03:16 jing Exp $
 * $Revision: 1.32 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define X25_ACKS
#define NOTIFY_STATUS
#define USER_RESTART

#include <infr.h>
#include <comm_manager.h>
#include <cmu_def.h>
#ifdef HPUX
#include <sys/errno.h> 
#else
#include <errno.h>	/* MPS include file */
#endif

#include <xstypes.h>
#include <xstpoll.h>
#include <mpsproto.h>
#include <x25_proto.h>
#include <sx25.h>
#include <xstra.h>
#include <ll_mon.h>
#include <dlpi.h>
#include <wan_control.h>
#include <x25_control.h>
#include <streamio.h>
#include <ll_control.h>
#include <x25_monitor.h>


#define REENABLE_TIMER 30
#define WAIT_BEFORE_DISCON_TIMER 1
#define MAX_CONFIG_RETRY_TIME	300	/* max time for configuration retry */

#define RESTART_CONFIRM_WAIT_TIME 30
				/* max waiting time for restart confirmation */
#define TDX_ON_WAIT_TIME 20	/* max waiting time for TDX on event */
#define DISABLE_WAIT_TIME 4	/* time to wait after disable WAN */

static int N_links;		/* Number of links managed by this process */
static Link_struct **Links;	/* link structure list */

static OpenRequest OreqX25;	/* server spec for MPSopen */

static int Restart_wait_time = 5;


/* shared packet buffers */
static char *Cntlbuff;
static char *Databuff;
static struct xstrbuf *Control;
static struct xstrbuf *Data;


static void End_connecting (Link_struct *link);
static void Attach (Link_struct *link);
static void Enable (Link_struct *link);
static void End_disconnecting (Link_struct *link);
static void Disable (Link_struct *link);
static void Detach (Link_struct *link);
static void Get_data (Link_struct *link, int pvc, int more);
static void Process_exception (Link_struct *link);
static int Get_x25_statistics (Link_struct *link, struct persnidstats *pss);
static int Get_lapb_statistics (Link_struct *link, struct lapb_stioc *lapb_s);
static int Zero_lapb_statistics (Link_struct *link);
static int Message_ack_control (Link_struct *link, int enable);


/**************************************************************************

    Description: This function initializes the X25/PVC module.

    Inputs:	n_links - number of links;
		links - the list of the link structure;
		control_buf - shared packet buffer - the control part;
		data_buf - shared packet buffer - the data part;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int XP_initialize (int n_links, Link_struct **links,
			struct xstrbuf *control_buf, struct xstrbuf *data_buf)
{

    N_links = n_links;
    Links = links;

    Control = control_buf;
    Data = data_buf;
    Cntlbuff = Control->buf;
    Databuff = Data->buf;

    memset (&OreqX25, 0, sizeof (OreqX25));
    OreqX25.dev  = 0;
    OreqX25.port = 0;
    OreqX25.ctlrNumber = 0;
    OreqX25.flags = CLONEOPEN;
    strcpy (OreqX25.serverName, CC_get_server_name ());
    strcpy (OreqX25.serviceName, CC_get_X25_service_name ());
    strcpy (OreqX25.protoName, "x25");
    OreqX25.openTimeOut = 10;

    return (0);
}

/**************************************************************************

    Description: Sets the restart waiting time.

**************************************************************************/

void XP_set_restart_wait_time (int t) {
    Restart_wait_time = t;
}

/**************************************************************************

    Description: This function configures the X25 protocol.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int XP_config_protocol ()
{
    int dlpifd;
    time_t st_time;

    st_time = MISC_systime (NULL);
    LE_send_msg (LE_VL0, "Configuring MPS server...\n");
    while ((dlpifd = CCC_x25_config (N_links, Links)) < 0) {
	if (dlpifd == -1) {
	    LE_send_msg (GL_ERROR, "MPS server config failed\n");
	    return (-1);
	}
	if (MISC_systime (NULL) > st_time + MAX_CONFIG_RETRY_TIME) {
	    LE_send_msg (GL_ERROR, "MPS server config failed - timed out\n");
	    return (-1);
	}
	LE_send_msg (GL_ERROR, "MPS server config failed - we will retry\n");
	sleep (10);
    }
    LE_send_msg (LE_VL0, "MPS server config done\n");

    SH_set_dlpi_fd (dlpifd);	/* send DLPI fd for polling */

    return (0);
}

/**************************************************************************

    Description: This function is called periodically for generating timer
		events.

**************************************************************************/

void XP_house_keeping ()
{
    static time_t last_tm = 0;
    time_t t;
    int i;

    t = MISC_systime (NULL);
    if (t <= last_tm)		/* once a second only */
	return;
    last_tm = t;

/* For testing
    if ((t % 20) == 0) {
	if (Links[0]->client_id[0] >= 0) {
	    struct persnidstats pss;
	    struct lapb_stioc lapb_s;
	    Get_x25_statistics (Links[0], &pss);
	    Get_lapb_statistics (Links[0], &lapb_s);
	}
    }
*/

    for (i = 0; i < N_links; i++) {
	Link_struct *link;
	int pvc;

	link = Links[i];
	if (link->wait_before_discon_state) {
	    link->wait_before_discon_state = 0;
	    if (link->conn_activity != NO_ACTIVITY)
		Detach (link);
	}
	if (link->disable_st_time > 0 && 
	    (link->enable_state == DISABLING || 
					link->enable_state == DISABLED))
	    Disable (link);

	if (link->conn_wait_state == CONN_WAIT_FOR_ENABLE) {
	    if (link->conn_activity != CONNECTING) {
		link->conn_wait_state = CONN_WAIT_INACTIVE;
		Detach (link);
	    }
	    else if (t >= link->conn_wait_st_time + REENABLE_TIMER) {
		LE_send_msg (LE_VL1, 
			"    Retry enable WAN link %d", link->link_ind);
		if (CM_max_reenable () > 0 &&
		    link->retry_enable_cnt >= CM_max_reenable ()) {
		    link->retry_enable_cnt = 0;
		    LE_send_msg (GL_INFO, 
			"Disabling link %d before retrying enabling WAN", 
						link->link_ind);
		    if (CCC_wanCommand (link->snid, W_DISABLE) < 0)
			LE_send_msg (GL_ERROR, 
				   "Disabling link %d failed", link->link_ind);
		    else
			link->disable_st_time = MISC_systime (NULL);
		}
		if (link->disable_st_time == 0 ||
			MISC_systime (NULL) >= link->disable_st_time + 
							DISABLE_WAIT_TIME) {
		    link->disable_st_time = 0;
		    link->conn_wait_st_time = t;
		    if (CCC_wanCommand (link->snid, W_ENABLE) < 0) {
			LE_send_msg (GL_ERROR, 
				"Reenabling link %d failed", link->link_ind);
			SH_process_fatal_error ();
			return;
		    }
		    link->retry_enable_cnt++;
		    for (pvc = 0; pvc < link->n_pvc; pvc++) {
			if (link->attach_state[pvc] == ENABLING)
			    link->attach_state[pvc] = DISABLED;
		    }
		}
	    }
	}

	if (link->conn_activity == CONNECTING) {
	    if (link->conn_wait_state == CONN_WAIT_FOR_RESTART &&
		t >= link->conn_wait_st_time + Restart_wait_time) {
		LE_send_msg (LE_VL3, "    Enabling TDX, link %d", 
							link->link_ind);
		if (CCC_control_restart (CCC_ENABLE_TDX, link->snid) < 0) {
		    LE_send_msg (GL_ERROR, "Enabling TDX failed");
		    SH_process_fatal_error ();
		}
		link->conn_wait_state = CONN_WAIT_FOR_TDX_ON;
		link->conn_wait_st_time = t;
		link->clean_tdx = 1;
	    }
	    if (link->conn_wait_state == CONN_WAIT_REST_CONFIRM &&
		t >= link->conn_wait_st_time + RESTART_CONFIRM_WAIT_TIME) {
		LE_send_msg (LE_VL1, 
		    "    RESTART confirmation waiting timed out, link %d", 
						    	link->link_ind);
		Process_exception (link);
		return;
	    }
	    if (link->conn_wait_state == CONN_WAIT_FOR_TDX_ON &&
		t >= link->conn_wait_st_time + TDX_ON_WAIT_TIME) {
		LE_send_msg (LE_VL1, 
		    "    TDX-on waiting timed out, link %d", link->link_ind);
		Process_exception (link);
		return;
	    }
	}
    }
}

/**************************************************************************

    Description: This function processes a packet received from the X25.
		The control and data parts are assumed in Cntlbuff and 
		Databuff respectively.

    Inputs:	link - the link from which the message is received.
		pvs - the PVC index.

    Return:	0 on success or -1 on failure.

**************************************************************************/

int XP_process_packets (Link_struct *link, int pvc)
{
    S_X25_HDR *hd;
    hd = (S_X25_HDR *)Cntlbuff;

    if (hd->xl_type == XL_DAT) {
	switch (hd->xl_command) {		/* control package */
	    struct xdataf *pData;
	    struct xdataackf *ack;
	    unsigned int sequence;

	    case N_Data:
	    pData = (struct xdataf *)Cntlbuff;
/*     
	    if (pData->More)
		printf ("M-bit on\n");
	    if (pData->setQbit)
		printf ("Q-bit on\n");
	    if (pData->setDbit)
		printf ("D-bit set\n");
*/
	    Get_data (link, pvc, pData->More);	/* the data is in Data */
	    break;

	    case N_DatAck:
	    ack = (struct xdataackf *)Cntlbuff;
	    sequence = INT_BSWAP_L (ack->sequence);
	    LE_send_msg (LE_VL3, "    message ACK - sequence %u\n", sequence);
	    if (sequence != link->sequence[pvc])
		LE_send_msg (GL_ERROR, 
			"unexpected message ACK sequence number (%u, %u)", 
					sequence, link->sequence[pvc]);
	    link->w_ack_time = MISC_systime (NULL);
	    link->w_ack[pvc] = link->w_cnt[pvc];
	    XP_write_data (link, pvc);
	    break;

	    default:
	    printf ("XL_DAT: xl_command %d\n", hd->xl_command);
	    break;
	}
	return (0);
    }

    if (hd->xl_type != XL_CTL) {
	LE_send_msg (GL_ERROR, "Unexpected MPS msg type %d: link %d\n", 
						hd->xl_type, link->link_ind);
	return (-1);
    }

    switch (hd->xl_command) {		/* control package */

	case N_PVC_DETACH: {		/* */	
	    struct pvcdetf *cmd;
	    int reason_code;
	    cmd = (struct pvcdetf *)Cntlbuff;
	    reason_code = INT_BSWAP_L (cmd->reason_code);
	    if (reason_code == PVC_SUCCESS) {
					/* we don't receive this normally */
		LE_send_msg (LE_VL1, 
			"    PVC_DETACH_SUCCESS: link %d, pvc %d\n", 
						    link->link_ind, pvc);
		link->attach_state[pvc] = DISABLED;
		Detach (link);
	    }
	    else if (reason_code == PVC_LINKDOWN) { /* connection lost */
		LE_send_msg (LE_VL1, "    link %d, pvc %d, disabled remotely", 
						link->link_ind, pvc);
		Process_exception (link);
	    }
	    else {	/* we receive this sometimes - what does this mean? */
		LE_send_msg (LE_VL1, 
			"    PVC_DETACH_FAILED: link %d, pvc %d\n", 
						    link->link_ind, pvc);
		link->attach_state[pvc] = DISABLED;
	    }
	}
	break;

	case N_RI: {		/* reset indication */	
	    struct xrstf *cmd;
	    cmd = (struct xrstf *)Cntlbuff;
	    LE_send_msg (LE_VL1, "    N_RI: originator %d reason %d \n", 
					cmd->originator, cmd->reason);
	}
	break;

	case N_PVC_ATTACH: {		/* */	
	    struct pvcattf *cmd;
	    int result_code;
	    cmd = (struct pvcattf *)Cntlbuff;
	    result_code = INT_BSWAP_L (cmd->result_code);
	    if (result_code == PVC_SUCCESS) {

		LE_send_msg (LE_VL1,  
			"    PVC_ATTACH_SUCCESS: link %d, pvc %d\n", 
						    link->link_ind, pvc);
		if (link->attach_state[pvc] == ENABLING) {
		    link->attach_state[pvc] = ENABLED;
		    Attach (link);
		}
		else {
		    LE_send_msg (GL_ERROR,  
			    "    ERROR: unexpected PVC_ATTACH_SUCCESS");
		    link->attach_state[pvc] = ENABLED;
		    Process_exception (link);
		}
	    }
	    else {
		LE_send_msg (GL_ERROR, 
		"    PVC_ATTACH failed: result_code %d, link %d, pvc %d\n", 
				result_code, link->link_ind, pvc);
		if (link->attach_state[pvc] == ENABLING)
		    link->attach_state[pvc] = DISABLED;
		else
		    LE_send_msg (GL_ERROR,  
			    "    ERROR: unexpected PVC_ATTACH_FAILED");
		Process_exception (link);
	    }
	}
	break;
    }
    return (0);
}

/**************************************************************************

    Processes DLPI status data.

    Inputs:	dlpifd - The DLPI lisener fd.

**************************************************************************/

#define MAX_CTL_SIZ	1000
#define MAX_DAT_SIZ 	4096

void XP_process_DLPI_status (int dlpifd) {
    struct xstrbuf		ctlblk;
    ALIGNED_t			ctlbuf[ALIGNED_T_SIZE (MAX_CTL_SIZ)];
    struct xstrbuf		datblk;
    ALIGNED_t			datbuf[ALIGNED_T_SIZE (MAX_DAT_SIZ)];
    struct xdataf		*pData;
    S_X25_HDR			*pHdr;
    struct x25_statusmon	*status;
    int				retval, flags;
    char			snid_str[12];

    /* Init to read data */
    memset (&ctlblk, 0, sizeof (ctlblk));
    memset (ctlbuf, 0, sizeof (ctlbuf));
    memset (datbuf, 0, sizeof (datbuf));
    ctlblk.buf = (char *)ctlbuf;
    datblk.buf = (char *)datbuf;
    status = (struct x25_statusmon *)datbuf;
    ctlblk.maxlen	= sizeof (ctlbuf);
    datblk.maxlen	= sizeof (datbuf);

    flags = 0;
    if ((retval = MPSgetmsg (dlpifd, &ctlblk, &datblk, &flags)) < 0) {
	LE_send_msg (GL_ERROR, 
		"MPSgetmsg (DLPI status) failed (errno %d)\n", errno);
	return;
    }

    pHdr = (S_X25_HDR *)ctlbuf;
    pData = (struct xdataf *)ctlbuf;
    if ((pHdr->xl_type == XL_DAT) && (pHdr->xl_command == N_Data)) {
	int i;

	if (datblk.len != sizeof (struct x25_statusmon)) {
	    printf ("Invalid data block (DLPI status) - discarded\n");
	    return;
	}

	status->snid = INT_BSWAP_L (status->snid);
	status->state = INT_BSWAP_L (status->state);
	if (x25tosnid (status->snid, (unsigned char *)snid_str) == -1)
	    strcpy (snid_str, "<UNKNOWN>");
	else
	    snid_str[4] = '\0';

	if (status->state >> 16 != 0xFFFF)
	    LE_send_msg (LE_VL3, 
		"    Subnet '%s' has changed from %d to %d", 
			snid_str, status->state >> 16, status->state & 0xffff);
	else
	    LE_send_msg (LE_VL3, 
		"    Subnet '%s' new restart state %d", 
			snid_str, status->state & 0xffff);

	for (i = 0; i < N_links; i++) {
	    Link_struct *link;
	    int tdx_event, new_state;

	    link = Links[i];
	    tdx_event = new_state = -1;
	    if (strcmp (snid_str, link->snid) == 0) {
		if (status->state >> 16 != 0xFFFF) {
		    new_state = status->state & 0xffff;
		    link->dlpi_state = new_state;
		}
		else
		    tdx_event = status->state & 0xffff;
		if (new_state == DL_DATAXFER) {
						/* link enabled */
		    if (link->conn_wait_state == CONN_WAIT_FOR_ENABLE) {
			link->enable_state = ENABLED;
			link->conn_wait_state = CONN_WAIT_FOR_RESTART;
			link->conn_wait_st_time = MISC_systime (NULL);
			LE_send_msg (LE_VL1, 
			    "    enabling subnet %s done\n", link->snid);
			Enable (link);
		    }
		}
		if (new_state == DL_IDLE) {
						/* link disabled */
		    if (link->enable_state == DISABLING) {
			link->enable_state = DISABLED;
			LE_send_msg (LE_VL1, 
			    "    disabling subnet %s done\n", link->snid);
			Disable (link);
		    }
		}
		if (link->conn_activity == CONNECTING &&
		    (tdx_event == UREV_PVC_CAN_ATTACH ||
		    tdx_event == UREV_TRANSIENT_DXFER_ON)) {
		    if (link->conn_wait_state == CONN_WAIT_FOR_TDX_ON) {
			link->conn_wait_state = CONN_WAIT_REST_CONFIRM;
			link->conn_wait_st_time = MISC_systime (NULL);
		    }
		    else if (link->conn_wait_state == CONN_WAIT_FOR_RESTART) {
			link->conn_wait_state = CONN_WAIT_INACTIVE;
		    }
		    Enable (link);
		}
		if (link->conn_activity == CONNECTING &&
		    link->conn_wait_state == CONN_WAIT_REST_CONFIRM &&
		    tdx_event == UREV_TRANSIENT_DXFER_OFF) {
		    LE_send_msg (LE_VL3, 
				"    RESTART confirm received, link %d", 
						link->link_ind);
		    link->conn_wait_state = CONN_WAIT_INACTIVE;
		    link->clean_tdx = 0;
		    End_connecting (link);
		}
		if (tdx_event == UREV_PVC_CAN_ATTACH &&
		    link->link_state == LINK_CONNECTED) {
		    LE_send_msg (LE_VL3, 
			"    Sending RESTART (in connected state), link %d",
							link->link_ind);
		    if (CCC_control_restart (CCC_SEND_RESTART, 
							link->snid) < 0) {
			LE_send_msg (GL_ERROR, "Sending RESTART failed");
			SH_process_fatal_error ();
		    }
		}
	    }
	}
    }
    else
	LE_send_msg (GL_ERROR, 
		"Invalid DLPI message type received - discarded\n");

    return;
}

/**************************************************************************

    Description: This function starts a connection procedure.

    Inputs:	link - the link involved.

**************************************************************************/

void XP_connection_procedure (Link_struct *link)
{

    if (!(SH_config_OK ()))
	return;

/*    if (link->conn_activity != CONNECTING)
	link->enabling_time = 0; */
    link->link_state = LINK_DISCONNECTED;
    CMPR_cleanup (link);
    Detach (link);

    return;
}

/**************************************************************************

    Description: This function implements the detach step of the connection
		procedure. It detaches and closes all pvcs on a link.

    Inputs:	link - the link involved.

**************************************************************************/

static void Detach (Link_struct *link)
{
    int i;

    if (MISC_systime (NULL) <= link->w_ack_time + WAIT_BEFORE_DISCON_TIMER) {
	link->wait_before_discon_state = 1;
	LE_send_msg (LE_VL1, 
		"waiting before disconnect link %d\n", link->link_ind);
	return;
    }
    link->wait_before_discon_state = 0;
    link->conn_wait_state = CONN_WAIT_INACTIVE;

    /* remove transient datatransfer state */
    if (link->clean_tdx) {
	LE_send_msg (LE_VL3, "    CCC_DISABLE_TDX, link %d\n", link->link_ind);
	CCC_control_restart (CCC_DISABLE_TDX, link->snid);
    }
    link->clean_tdx = 0;

    /* detach all PVCs */
    for (i = 0; i < link->n_pvc; i++) {
	int ret;

	if (link->attach_state[i] == ENABLED) {
	    struct xstrbuf ctlblk;
	    struct pvcdetf detach;

	    link->attach_state[i] = DISABLING;
	    LE_send_msg (LE_VL1, 
			"    Detaching link %d, pvc %d\n", link->link_ind, i);

	    memset (&detach, 0, sizeof (detach));
	    detach.xl_type    = XL_CTL;
	    detach.xl_command = N_PVC_DETACH;
	    ctlblk.len = sizeof (struct pvcdetf);
	    ctlblk.buf = (char *) &detach;
	    while ((ret = MPSputmsg (link->client_id[i], &ctlblk, 0, 0)) != 0 
			&& MPSerrno == EAGAIN) {
		LE_send_msg (GL_INFO, 
			"MPSputmsg for detach returns EAGAIN - retry");
		msleep (100);
	    }
			   
	    if (ret != 0) {
		LE_send_msg (GL_ERROR, 
			"MPSputmsg for detach failed (fd %d, MPSerrno %d)", 
			link->client_id[i], MPSerrno);
		SH_process_fatal_error ();
		return;
	    }
	}
	link->attach_state[i] = DISABLED; /* detach request may not be acked */
    }

    for (i = 0; i < link->n_pvc; i++) {		/* close links */
	if (link->client_id[i] >= 0 &&
	    MPSclose (link->client_id[i]) != 0)
	    LE_send_msg (GL_ERROR, "MPSclose failed, pvc %d, link %d", 
			    i, link->link_ind);
	link->client_id[i] = -1;
    }
    if (link->lapb_fd >= 0)
	MPSclose (link->lapb_fd);
    link->lapb_fd = -1;
    Disable (link);

    return;
}

/**************************************************************************

    Description: This function implements the disable step of the connection
		procedure.

    Inputs:	link - the link involved.

**************************************************************************/

static void Disable (Link_struct *link) {

    if (link->enable_state == ENABLED || link->enable_state == ENABLING) {

	LE_send_msg (LE_VL1, "    Disabling link %d\n", link->link_ind);
	if (link->lapb_enable_state == ENABLED) {
	    if (CCC_lapbCommand (link->snid, L_LINKDISABLE) < 0) {
		LE_send_msg (GL_ERROR, 
			"Disabling lapb link %d failed", link->link_ind);
/*		SH_process_fatal_error ();
		return; This may fail if the PVCs are not attached */
	    }
	    link->lapb_enable_state = DISABLED;
	}
	if (CCC_wanCommand (link->snid, W_DISABLE) < 0) {
	    LE_send_msg (GL_ERROR, "Disabling link %d failed", link->link_ind);
	    SH_process_fatal_error ();
	    return;
	}
	else
	    link->disable_st_time = MISC_systime (NULL);
	link->enable_state = DISABLING;
    }
    if (link->dlpi_state == DL_IDLE)
	link->enable_state = DISABLED;
    if (link->enable_state == DISABLED && 
	(link->disable_st_time == 0 || 
	   MISC_systime (NULL) >= link->disable_st_time + DISABLE_WAIT_TIME)) {
	link->disable_st_time = 0;
	End_disconnecting (link);
    }

    return;
}

/**************************************************************************

    Description: This function implements the end of connecting step of the
		connection procedure. It finishes the connection requests if
		the target is to disconnect. Otherwise it continues the
		procedure to make a new connection.

    Inputs:	link - the link involved.

**************************************************************************/

static void End_disconnecting (Link_struct *link)
{

    if (link->conn_activity == DISCONNECTING) {	/* stop here */

	CMPR_cleanup (link);
	LE_send_msg (LE_VL1 | 1101,  "link %d disconnected\n", link->link_ind);

	link->link_state = LINK_DISCONNECTED;
	CMPR_send_response (link, 
			&(link->req[(int)link->conn_req_ind]), CM_SUCCESS);
    }
    else if (link->conn_activity == CONNECTING)	/* go on to next step */
	Enable (link);
	
    return;
}

/**************************************************************************

    Description: This function implements the enable step of the connection
		procedure. It enables the link.

    Inputs:	link - the link involved.

**************************************************************************/

static void Enable (Link_struct *link)
{

    if (link->enable_state == DISABLED) {

	if (link->conn_activity != CONNECTING) {
	    Detach (link);
	    return;
	}

	LE_send_msg (LE_VL1, "    Enabling link %d\n", link->link_ind);
        if (link->lapb_enable_state != ENABLED) {
	    if (CCC_lapbCommand (link->snid, L_LINKENABLE) < 0) {
		LE_send_msg (GL_ERROR, 
			"Enabling link %d failed", link->link_ind);
		SH_process_fatal_error ();
		return;
	    }
	    link->lapb_enable_state = ENABLED;
	}
        if (CCC_wanCommand (link->snid, W_ENABLE) < 0) {
	    LE_send_msg (GL_ERROR, "Enabling link %d failed", link->link_ind);
	    SH_process_fatal_error ();
	    return;
	}
	link->retry_enable_cnt = 1;
	link->enable_state = ENABLING;
	link->conn_wait_state = CONN_WAIT_FOR_ENABLE;
	link->conn_wait_st_time = MISC_systime (NULL);
    }
    if (link->enable_state == ENABLED) {
	link->disable_st_time = 0;
	if (link->conn_activity == CONNECTING)
	    Attach (link);
	else 
	    Detach (link);
    }

    return;
}

/**************************************************************************

    Description: This function implements the attach step of the connection
		procedure. It opens and attaches all pvcs on a link.

    Inputs:	link - the link involved.

**************************************************************************/

static void Attach (Link_struct *link)
{
    int i, done;

    for (i = 0; i < link->n_pvc; i++) {	/* open links */

	if (link->client_id[i] < 0) {
	    int sock;
    
	    strcpy (OreqX25.protoName, "x25");
	    sock = MPSopen (&OreqX25);
	    if (sock < 0) {
		LE_send_msg (GL_ERROR, 
			    "MPSopen failed (errno %d), pvc %d, link %d", 
				    errno, i, link->link_ind);
		SH_process_fatal_error ();
	    }
	    LE_send_msg (LE_VL3, "    pvc %d open (fd %d), link %d", 
					    i, sock, link->link_ind);
	    /* set non_blocking mode */
	    if (MPSioctl (sock, X_SETIOTYPE, (char *)NONBLOCK) == -1) {
		LE_send_msg (GL_ERROR, 
			    "MPSioctl NONBLOCK failed (errno = %d)\n", errno);
		SH_process_fatal_error ();
	    }
	    link->client_id[i] = sock;
	    link->w_blocked[i] = 0;
	}
    }
    if (link->lapb_fd < 0) {
	strcpy (OreqX25.protoName, "lapb");
	if ((link->lapb_fd = MPSopen (&OreqX25)) < 0) {
	    LE_send_msg (GL_ERROR, 
			    "MPSopen lapb fd failed (errno %d), link %d", 
				    errno, link->link_ind);
	    SH_process_fatal_error ();
	}
    }

    /* make sure it is ready for PVC attach */
    if (link->conn_wait_state == CONN_WAIT_FOR_TDX_ON ||
	link->conn_wait_state == CONN_WAIT_FOR_RESTART)
	return;

    /* attach all PVCs */
    done = 1;
    for (i = 0; i < link->n_pvc; i++) {
	int ret;

	if (link->attach_state[i] == DISABLED) {
	    struct xstrbuf ctlblk;
	    struct pvcattf attach;

	    if (i > 0 && link->attach_state[i - 1] != ENABLED)
		return;

	    link->attach_state[i] = ENABLING;
	    LE_send_msg (LE_VL1, 
			"    Attaching link %d, pvc %d\n", link->link_ind, i);
	    memset (&attach, 0, sizeof (attach));
	    attach.xl_type = XL_CTL;
	    attach.xl_command = N_PVC_ATTACH;
	    attach.sn_id = INT_BSWAP_L (snidtox25 ((unsigned char *)(link->snid)));	/* subnet ID */
	    attach.lci = SHORT_BSWAP_L ((i + 1) * (i + 1));	/* logical channel number */
	    ctlblk.len = sizeof (struct pvcattf);
	    ctlblk.buf = (char *) &attach;
	    ret = MPSputmsg (link->client_id[i], &ctlblk, 0, 0);     
	    if (ret != 0) {
		LE_send_msg (GL_ERROR, "MPSputmsg for attach returns %d", ret);
		SH_process_fatal_error ();
		return;
	    }
	}
	if (link->attach_state[i] != ENABLED)
	    done = 0;   
    }

    if (done) {
	if (link->conn_activity == CONNECTING) {
	    LE_send_msg (LE_VL3, 
			"    Sending RESTART, link %d", link->link_ind);
	    if (CCC_control_restart (CCC_SEND_RESTART, link->snid) < 0) {
		LE_send_msg (GL_ERROR, "Sending RESTART failed");
		SH_process_fatal_error ();
	    }
	    End_connecting (link);
	}
	else 
	    Detach (link);
    }

    return;
}

/**************************************************************************

    Description: This function implements the end of connecting step of the
		connection procedure. It finishes the connection requests.

    Inputs:	link - the link involved.

**************************************************************************/

static void End_connecting (Link_struct *link)
{

    if (link->conn_activity != CONNECTING)
	return;

    /* make sure we received RESTART confirmation */
    if (link->conn_wait_state == CONN_WAIT_REST_CONFIRM) 
	return;
    link->conn_wait_state = CONN_WAIT_INACTIVE;

    /* stop here */
    Message_ack_control (link, 1);	/* enable msg write ACK */
    LE_send_msg (LE_VL1 | 1108,  "link %d connected\n", link->link_ind);
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

void XP_write_data (Link_struct *link, int pvc)
{
    struct xdataf  data;

    if (link->link_state != LINK_CONNECTED) {		/* link failed */
	CMPR_send_response (link, &(link->req[(int)link->w_req_ind[pvc]]), 
					CM_DISCONNECTED);
	return;
    }

    memset (&data, 0, sizeof (data));
    data.xl_type    = XL_DAT;
    data.xl_command = N_Data;
    data.More       = FALSE;
    data.setQbit    = FALSE;
    data.setDbit    = FALSE;
    data.sequence   = INT_BSWAP_L (link->sequence[pvc]);

    if (link->w_cnt[pvc] < link->w_size[pvc]) {		/* write more data */

	while (1) {
	    struct xstrbuf ctlblk;
	    struct xstrbuf datblk;
	    int len, ret;

	    len = link->w_size[pvc] - link->w_cnt[pvc];
	    if (len <= 0)
		break;
/*
	    if (len >= link->packet_size) {
		len = link->packet_size;
		data.More = TRUE;
	    }
	    else 
		data.More = FALSE;
*/
	    LE_send_msg (LE_VL1, 
		"    XP_write_data %d bytes, sequence %d, link %d, pvc %d", 
			len, link->sequence[pvc], link->link_ind, pvc);

	    ctlblk.len = sizeof (data);
	    ctlblk.buf = (char *)&data;
	    datblk.len = len;
	    datblk.buf = link->w_buf[pvc] + link->w_cnt[pvc];

	    link->w_blocked[pvc] = 0;
            ret = MPSputmsg (link->client_id[pvc], &ctlblk, &datblk, 0);
	    if (ret < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
/*		    Check_out_of_band_data (link->client_id[pvc]); */
		    LE_send_msg (LE_VL1, 
				"MPSputmsg blocked, link %d, pvc %d", 
						link->link_ind, pvc);
		    link->w_blocked[pvc] = 1;
		    break;
		}
		else {
		    MPSperror ( "Write Error\n" );
		    LE_send_msg (GL_ERROR, 
			"MPSputmsg failed (errno %d), link %d, pvc %d", 
						errno, link->link_ind, pvc);
		    CMPR_send_response (link, 
			&(link->req[(int)link->w_req_ind[pvc]]), CM_FAILED);
		    return;
		}
	    }

	    link->w_cnt[pvc] += len;
	}
    }

    if (link->w_ack[pvc] >= link->w_size[pvc]) {	/* done */
	CMPR_send_response (link, &(link->req[(int)link->w_req_ind[pvc]]), 
					CM_SUCCESS);
	link->sequence[pvc]++;
    }

    return;
}
 
/**************************************************************************

    Description: This function reads data from the "pvc" on "link".

    Inputs:	link - the link involved.
		pvc - PVC index.

**************************************************************************/

static void Get_data (Link_struct *link, int pvc, int more) {

    /* reallocate the buffer */
    while (link->r_cnt[pvc] + Data->len > link->r_buf_size[pvc]) {
	if (CMC_reallocate_input_buffer (link, pvc, 
					link->r_cnt[pvc] + Data->len) < 0)
	    return;
    }

    if (Data->len <= 0) {
	LE_send_msg (GL_ERROR, "Data->len error (%d)", Data->len);
	return;
    }
    memcpy (link->r_buf[pvc] + link->r_cnt[pvc], Databuff, Data->len);
    link->r_cnt[pvc] += Data->len;
    if (more)
	return;

    /* done */
    CMPR_send_data (link, pvc, link->r_cnt[pvc], link->r_buf[pvc]);
    link->r_cnt[pvc] = 0;

    return;
}

/**************************************************************************

    Description: This function terminates a failed connection.

    Inputs:	link - the link involved.

**************************************************************************/

static void Process_exception (Link_struct *link)
{
    LE_send_msg (LE_VL1 | 1108, "Processing exception...\n");
    CMPR_connect_failure (link);
    Detach (link);
    LE_send_msg (LE_VL1, "disconnected, link %d", link->link_ind);

    return;
}

/**************************************************************************

    Description: This function closes all X25/PVC ports.

**************************************************************************/

void XP_clean_up ()
{

}

/**************************************************************************

    Reads X25/PVC statistics on a port and generates a statistics CM event.

    Inputs:	link - the link involved.

**************************************************************************/

void XP_statistics_request (Link_struct *link)
{
    int i;
    struct lapb_stioc lapb_s;
    ALIGNED_t buf[ALIGNED_T_SIZE (sizeof (CM_resp_struct) + 
					sizeof (X25_statistics))];
    X25_statistics *out;
    CM_resp_struct *resp;
    int *stat;

    if (Get_lapb_statistics (link, &lapb_s) < 0)
	return;

    stat = (int *)lapb_s.lli_stats.lapbmonarray;	/* status array */
    for (i = 0; i < lapbstatmax; i++)
	stat[i] = INT_BSWAP_L (stat[i]);
    out = (X25_statistics *)((char *)buf + sizeof (CM_resp_struct));
    resp = (CM_resp_struct *)buf;
    out->R_I = stat[I_rx_cmd];
    out->X_I = stat[I_tx_cmd];
    out->R_RR = stat[RR_rx_cmd] + stat[RR_rx_rsp];
    out->X_RR = stat[RR_tx_cmd] + stat[RR_tx_rsp];
    out->R_RNR = stat[RNR_rx_cmd] + stat[RNR_rx_rsp];
    out->X_RNR = stat[RNR_tx_cmd] + stat[RNR_tx_rsp];
    out->R_REJ = stat[REJ_rx_cmd] + stat[REJ_rx_rsp];
    out->X_REJ = stat[REJ_tx_cmd] + stat[REJ_tx_rsp];
    out->R_FRMR = stat[FRMR_rx_rsp];
    out->X_FRMR = stat[FRMR_tx_rsp];
    out->R_ERROR = stat[rx_unknown];
    out->X_ERROR = stat[tx_ign] + stat[tx_rtr];
    out->R_FCS = stat[rx_badlen] + stat[rx_bad];

    out->rate = CMRATE_get_rate (link);
/*    out->rate = 0; */
    resp->data_size = sizeof (X25_statistics);

    CMPR_send_event_response (link, CM_STATISTICS, (char *)buf);
    LE_send_msg (LE_VL1, "sending statistics (rate %d)\n", out->rate);

    return;
}

/**************************************************************************

    Description: This function resets X25/PVC statistics on a port.

    Inputs:	link - the link involved.

**************************************************************************/

void XP_statistics_reset (Link_struct *link) {
    Zero_lapb_statistics (link);
    return;
}

/**************************************************************************

    Reads per-subnet x25 statistics.

    Inputs:	link - the link involved.

    Output:	pss - per-subnet statistics.

    Returns 0 on success or -1 on failure.

**************************************************************************/

static int Get_x25_statistics (Link_struct *link, struct persnidstats *pss) {
    struct xstrioctl io;
    int *stat;

    if (link->client_id[0] < 0)
	return (-1);
    pss->snid = snidtox25 ((unsigned char *)(link->snid));
    io.ic_cmd = N_getSNIDstats; /* N_zeroSNIDstats for reset but no return? */
    io.ic_timout = 0; 
    io.ic_len = sizeof (struct persnidstats);
    io.ic_dp = (char *)pss; 

    if (S_IOCTL (link->client_id[0], I_STR, (char *)&io) < 0) { 
	LE_send_msg (GL_ERROR, 
		"getSNIDstats IOCTL failed (errno %d)\n", errno);
	SH_process_fatal_error ();
    } 

    stat = (int *)pss->mon_array;		/* status array */
    /* pss->network_state: network state: L_connecting, WtgRES etc. 
      line 2572 of x25stat.c */

/*    LE_send_msg (0, "X25 ST: id %d, state %d, RRx %d r %d, Ix %d r %d", 
	pss->snid, pss->network_state, 
	stat[rr_out_s], stat[rr_in_s], 
	stat[dt_out_s], stat[dt_in_s]); */
    return (0);
}

/**************************************************************************

    Reads per-subnet lapb statistics. To get lapb statistics, we need to 
    open another fd for each line with OreqX25.protoName = "lapb".

    Inputs:	link - the link involved.

    Output:	lapb_s - per-subnet statistics.

    Returns 0 on success or -1 on failure.

**************************************************************************/

static int Get_lapb_statistics (Link_struct *link, struct lapb_stioc *lapb_s) {
    struct xstrioctl io;
    int *stat;

    if (link->lapb_fd < 0)
	return (-1);
    io.ic_cmd = L_GETSTATS; 
    io.ic_len = sizeof(struct lapb_stioc); 
    io.ic_dp = (char *)lapb_s; 
    lapb_s->lli_snid = INT_BSWAP_L (snidtox25 ((unsigned char *)(link->snid))); 
    lapb_s->lli_type = LI_STATS; 
    if (S_IOCTL (link->lapb_fd, I_STR, (char *)&io) < 0) {
	LE_send_msg (GL_ERROR, "L_GETSTATS IOCTL failed (errno %d)\n", errno);
	SH_process_fatal_error ();
    } 

    stat = (int *)lapb_s->lli_stats.lapbmonarray;	/* status array */
    /* lapb_s->lli_stats.lapbmonarray: network state: 0, 1, ... line 250 of 
       x25stat.c */

/*    LE_send_msg (0, "LAPB ST: id %d, state %d, RR %d %d %d %d, Ix %d r %d", 
	lapb_s->lli_snid, lapb_s->state, 
	stat[RR_rx_cmd], stat[RR_rx_rsp], stat[RR_tx_cmd], stat[RR_tx_rsp], 
	stat[I_tx_cmd], stat[I_rx_cmd]); */
    return (0);
}

/**************************************************************************

    Resets per-subnet lapb statistics.

    Inputs:	link - the link involved.

    Returns 0 on success or -1 on failure.

**************************************************************************/

static int Zero_lapb_statistics (Link_struct *link) {
    struct lapb_stioc lapb_s;
    struct xstrioctl io;

    if (link->lapb_fd < 0)
	return (-1);
    io.ic_cmd = L_ZEROSTATS; 
    io.ic_len = sizeof(lapb_s); 
    io.ic_dp = (char *)&lapb_s; 
    lapb_s.lli_snid = INT_BSWAP_L (snidtox25 ((unsigned char *)(link->snid))); 
    lapb_s.lli_type = LI_STATS; 
    if (S_IOCTL (link->lapb_fd, I_STR, (char *)&io) < 0) {
	LE_send_msg (GL_ERROR, "L_ZEROSTATS IOCTL failed (errno %d)\n", errno);
	return (-1);
    }
    return (0);
}

/**************************************************************************

    Enables/Disables message transmission acknowledgment for all PVCs.

    Inputs:	link - the link involved.
		enable - non-zero for enable and zero for disable.

    Returns 0 on success or -1 on failure.

**************************************************************************/

static int Message_ack_control (Link_struct *link, int enable) {
    int i;

    for (i = 0; i < link->n_pvc; i++) {
	struct xstrioctl io;
	int command;

	if (enable)
	    command = 1;
	else
	    command = 0;
	io.ic_cmd = N_enablelocalACK;
	io.ic_timout = -1; 
	io.ic_len = sizeof (int);
	io.ic_dp = (char *)&command; 

	if (link->client_id[i] >= 0 &&
	    MPSioctl (link->client_id[i], I_STR, (char *)&io) < 0) { 
	    LE_send_msg (GL_ERROR, 
		"N_enablelocalACK IOCTL (pvc %d) failed (MPSerrno %d)\n", 
								i, MPSerrno);
	    return (-1);
	} 
    }
    return (0);
}

