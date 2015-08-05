
/******************************************************************

	file: cmu_shared.c

	This module contains the functions shared by all protocols 
	- UCONX version.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/01/30 15:03:15 $
 * $Id: cmu_shared.c,v 1.36 2008/01/30 15:03:15 jing Exp $
 * $Revision: 1.36 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/poll.h>

#include <orpgerr.h>
#include <infr.h>
#include <comm_manager.h>
#include <cmu_def.h>
#ifdef HPUX
#include <sys/errno.h> 
#else
#include <errno.h>	/* MPS include file */
#endif
#include <mpsproto.h>
#include <xstpoll.h>

extern int errno;

static int N_links;		/* Number of links managed by this process */
static Link_struct **Links;	/* link structure list */

static int Cmsv_addr;		/* comms server internet address */

static int Config_OK = 0;	/* protocol configuration is in good 
				   condition */

static int Dlpi_fd = -1;	/* DLPI lisener fd */

/* shared packet buffers */
static ALIGNED_t Cntlbuff [ALIGNED_T_SIZE (1024)];
				/* See x25pvcsnd.c for the sizing */
static ALIGNED_t Databuff [ALIGNED_T_SIZE (8192)];
				/* See x25pvcsnd.c for the sizing */
static struct xstrbuf Control;
static struct xstrbuf Data;

/* local functions */
static int Configure_protocols ();
static void Init_state_flags ();
static void Reboot_MPS ();
static void Wait_seconds (int seconds);


/**************************************************************************

    Description: This function initializes all protocol modules and
		configures the protocols.

    Inputs:	n_links - number of links;
		links - the list of the link structure;
		cmsv_addr - comms server internet address.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int SH_initialize (int n_links, Link_struct **links, int cmsv_addr)
{

    Control.maxlen = sizeof (Cntlbuff);
    Control.buf = (char *)Cntlbuff;
    Data.maxlen = sizeof (Databuff);
    Data.buf = (char *)Databuff;

    N_links = n_links;
    Links = links;
    Cmsv_addr = cmsv_addr;

    /* For new cm_manager instance we need to reboot the MPS */
    if (CMC_get_new_instance ())
	Reboot_MPS ();

    Init_state_flags ();

    if (HA_initialize (n_links, links, &Control, &Data) != 0 ||
	XP_initialize (n_links, links, &Control, &Data) != 0)
	return (-1);

    return (Configure_protocols ());
}

/***********************************************************************

    Sets the DLPI listener fd for polling.

************************************************************************/

void SH_set_dlpi_fd (int dlpifd) {
    Dlpi_fd = dlpifd;
}

/***********************************************************************

    Description: This function returns the configuration OK flag.

    Return:	the configuration OK flag.

************************************************************************/

int SH_config_OK ()
{
    return (Config_OK);
}

/***********************************************************************

    Description: This function configures the protocols.

    Return:	It returns 0 on success or -1 on failure.

************************************************************************/

static int Configure_protocols ()
{

    if (HA_config_protocol () != 0 ||
	XP_config_protocol () != 0)
	return (-1);

    Config_OK = 1;

    return (0);
}

/**************************************************************************

    Description: This function initializes all state flags.

**************************************************************************/

static void Init_state_flags ()
{
    int i;

    for (i = 0; i < N_links; i++) {
	int k;
	Link_struct *link;

	link = Links[i];
	link->enable_state = DISABLED;
	link->lapb_enable_state = DISABLED;
	link->connect_state = DISABLED;
	link->conn_wait_state = CONN_WAIT_INACTIVE;
	link->conn_wait_st_time = 0;
	link->dlpi_state = 0;		/* 0 is an unused state */
	link->clean_tdx = 0;
	link->w_ack_time = 0;
	link->wait_before_discon_state = 0;
	link->disable_st_time = 0;

	link->lapb_fd = -1;
	for (k = 0; k < link->n_pvc; k++) {
	    link->client_id[k] = -1;
	    link->bind_state[k] = DISABLED;
	    link->attach_state[k] = DISABLED;
	    link->sequence[k] = 0;
	    link->w_blocked[k] = 0;
	}
    }

    return;
}

/**************************************************************************

    Description: This function polls the IO for reply messages. Only the
		HDLC version is implemented at this time.

    Return:	It returns the number messages read.

**************************************************************************/

#define MAN_N_LINKS_MANAGED 8

int SH_poll ()
{
    struct xpollfd pfds[MAX_N_STATIONS * MAN_N_LINKS_MANAGED + 2];
    int ilink [MAX_N_STATIONS * MAN_N_LINKS_MANAGED];
    int i, nfds, cnt, ret, k, tfd;
    int ntf_sd;

    nfds = 0;
    pfds[nfds].sd = Dlpi_fd;
    pfds[nfds].events = POLLIN | POLLPRI;
    nfds++;

    for (i = 0; i < N_links; i++) {
	for (k = 0; k < Links[i]->n_pvc; k++) {
	    if (Links[i]->client_id[k] >= 0) {
		pfds[nfds].sd = Links[i]->client_id[k];
		pfds[nfds].events = POLLIN | POLLPRI;
		if (Links[i]->w_blocked[k])
		    pfds[nfds].events |= POLLOUT;
		ilink[nfds] = i;
		nfds++;
	    }
	}
    }

    tfd = LB_NTF_control (LB_GET_NTF_FD);	/* Sync NTF section */
    ntf_sd = -1;
    if (tfd >= 0) {
	ntf_sd = tfd;
	pfds[nfds].sd = ntf_sd;
	pfds[nfds].events = POLLIN | POLLPRI | POLLSYS;
	nfds++;
    }

    if (nfds == 0) {
	msleep (1000);
	return (0);
    }

    cnt = 0;
    CM_block_signals (0);
    ret = MPSpoll (pfds, nfds, 1000);
    if (ret < 0) {
	if (errno == EINTR) {
	    CM_block_signals (1);
	    return (0);
	}
	LE_send_msg (0, "MPSpoll failed (errno %d)", errno);
	SH_process_fatal_error ();
    }
    CM_block_signals (1);

    if (ret == 0)
	return (cnt);

    for (k = 0; k < nfds; k++) {
	Link_struct *link;
	int pvc, sd;

	if (!(pfds[k].revents & (POLLIN | POLLPRI | POLLOUT)))
	    continue;

	if (pfds[k].sd == ntf_sd && (pfds[k].events & POLLSYS)) {
						/* Sync NTF section */
	    LB_NTF_control (LB_NTF_WAIT, 0);
	    continue;
	}

	if (pfds[k].sd == Dlpi_fd) {	/* DLPI status change */
	    XP_process_DLPI_status (Dlpi_fd);
	    continue;
	}

	sd = pfds[k].sd;
	link = Links[ilink[k]];
	for (pvc = 0; pvc < link->n_pvc; pvc++)
	    if (sd == link->client_id[pvc])
		break;
	if (pvc >= link->n_pvc) {
	    LE_send_msg (LE_VL1, "fd %d already closed\n", sd);
	    continue;
	}
	cnt++;

	if (pfds[k].revents & (POLLIN | POLLPRI)) { 
	    int gflg;

	    gflg = 0;
	    if (link->client_id[pvc] >= 0) {
			/* client_id may change in this while loop */
		while ((ret = MPSgetmsg (sd, &Control, &Data, &gflg)) < 0
				&& errno == 4)
		    msleep (20);
		if (ret < 0) {
		    if (MPSerrno == EAGAIN || MPSerrno == EWOULDBLOCK)
			break;
		    LE_send_msg (GL_ERROR, 
			"MPSgetmsg (SA_poll) failed (sd %d, ret %d, errno %d \
			link %d, pvc %d)",
				sd, ret, MPSerrno, link->link_ind, pvc);
		    SH_process_fatal_error ();
		}
		else {
		    if (link->proto == PROTO_HDLC)
			HA_process_packets (link);
		    else
			XP_process_packets (link, pvc);
		}
	    }
	}

	if ((pfds[k].revents & POLLOUT) && link->client_id[pvc] >= 0) {
	    link->w_blocked[pvc] = 0;
	    if (link->proto == PROTO_HDLC)
		HA_write_data (link, 0);
	    else
		XP_write_data (link, pvc);
	}
    }
    return (cnt);
}

/**************************************************************************

    Description: This function is the specific house keeping function for
		this comm_manager. It checks the connection status to
		the comms server and performs appropriate actions.

    Inputs:	link - the link structure;
		cr_time - current time;

**************************************************************************/

#define RESTART_REQUEST_TIME 10	/* request restart period */
#define RESTART_WAIT_TIME 20	/* waiting time before restart */
#define CMSV_DOWN_TIME 12	/* max TCP connection lost time */

void SH_house_keeping (Link_struct *link, time_t cr_time)
{
    static int last_request_time = 0;
    int qtime;

    if (cr_time < RESTART_REQUEST_TIME + last_request_time)
	return;

    CMC_start_new_instance (-1);	/* cancel previous request */
    CMC_start_new_instance (RESTART_WAIT_TIME);	/* send a new request */
    last_request_time = cr_time;

    qtime = CMMON_get_connect_status (Cmsv_addr);
    if (qtime >= CMSV_DOWN_TIME && Config_OK) {
	LE_send_msg (0, "Comms server %s connection lost for %d seconds", 
				CC_get_server_name (), qtime);
	SH_process_fatal_error ();
    }

    return;
}

/**************************************************************************

    Description: This function is called when a fatal error is detected.
		It sends a CM_EXCEPTION event to every line user, requests
		for new comm_manager instance and then terminates.

**************************************************************************/

void SH_process_fatal_error ()
{
    int i;

    for (i = 0; i < N_links; i++)
	CMPR_send_event_response (Links[i], CM_EXCEPTION, NULL);
    CMC_start_new_instance (10);
    LE_send_msg (GL_ERROR, "Terminating because of a fatal error\n");
    LE_send_msg (0, "Starting a new process is requested\n");
    exit (0);

    return;
}

/**************************************************************************

    Reboots the comms server and waits until the server is back.

**************************************************************************/

#define MPS_AFTER_REBOOT_TIME 20
#define REBOOT_TRY_TIME 6
#define MAX_REBOOT_TIME 80

static void Reboot_MPS () {
    int qtime, cmd_cnt, ret;

    LE_send_msg (GL_STATUS, 
	"Comms Server %s Error - Recovery Initiated", CC_get_server_name ());

    LE_send_msg (LE_VL0, "Waiting for 5 seconds before rebooting MPS ... \n");
    Wait_seconds (5);

    LE_send_msg (0, "Rebooting the MPS comms server %s\n", 
					CC_get_server_name ());
    cmd_cnt = 0;
    while (1) {
	char buf[128], *cmd, out[128];

	if ((cmd = CC_get_reboot_command ()) == NULL) {
	    sprintf (buf, "commreset %s", CC_get_server_name ());
	    cmd = buf;
	}
	LE_send_msg (0, "        using command: %s\n", cmd);
	ret = MISC_system_to_buffer (cmd, out, 128, NULL);
	if (ret == 0) {
	    time_t st_t;

	    LE_send_msg (LE_VL0, "Waiting for MPS to come up ... \n");
	    Wait_seconds (10);		/* wait until MPS started to reboot */

	    CMMON_update (MISC_systime (NULL));
	    LE_send_msg (LE_VL1, "Pinging MPS ... \n");
	    st_t = MISC_systime (NULL);
	    while (1) {
		qtime = CMMON_get_connect_status (Cmsv_addr);
		if (qtime == 0) {
		    LE_send_msg (LE_VL0, 
			"MPS comes back, wait for %d seconds ... \n", 
					MPS_AFTER_REBOOT_TIME);
		    Wait_seconds (MPS_AFTER_REBOOT_TIME);
						/* wait after reboot */
		    return;
		}
		if (MISC_systime (NULL) >= st_t + MAX_REBOOT_TIME) {
		    LE_send_msg (LE_VL0, 
				"Failed in waiting for MPS to come up\n");
		    break;
		}
		Wait_seconds (3);
		CMMON_update (MISC_systime (NULL));
	    }
	}
	if (cmd_cnt < REBOOT_TRY_TIME) {
	    if (ret != 0)
		LE_send_msg (GL_INFO, 
		"Failed in running MPS reboot cmd (%d) - will retry\n", ret);
	    Wait_seconds (10);
	    cmd_cnt++;
	    continue;
	}
	LE_send_msg (GL_ERROR, 
			"Failed in rebooting MPS - cm_uconx terminates\n");
	exit (1);
    }
}

/**************************************************************************

    Waits for "seconds" before return.

**************************************************************************/

static void Wait_seconds (int seconds) {
    time_t st_time;

    st_time = MISC_systime (NULL);
    while (1) {
	msleep (1000);
	if (MISC_systime (NULL) >= st_time + seconds)
	    return;
    }
}



