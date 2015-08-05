
/******************************************************************

	file: cmt_shared.c

	This module contains the functions shared by all protocols 
	- TCP version.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2013/01/24 19:37:49 $
 * $Id: cmt_shared.c,v 1.22 2013/01/24 19:37:49 jing Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 *
 * History:
 *
 *
 * 11JUN2003 - Chris Gilbert - CCR #NA03-06201 Issue 2-150. Add "faaclient"
 *             support going through a proxy firewall.
 *
 *
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/poll.h>
#include <stropts.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>

#include <infr.h>
#include <comm_manager.h>
#include "cmt_def.h"

extern int Simple_code;

static int N_links;		/* Number of links managed by this process */
static Link_struct **Links;	/* link structure list */

static int Verify_header (char *msg, int pvc, int link_ind);
static int Is_waiting_connect (Link_struct *link, int pvc);


/**************************************************************************

    Description: This function initialize this module and other modules 
		specific to this comm_manager.

    Inputs:	n_links - number of links;
		links - the list of the link structure;

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int SH_initialize (int n_links, Link_struct **links)
{

    N_links = n_links;
    Links = links;

    if (TCP_initialize (n_links, links) != 0 ||
	SOCK_init () != 0)
	return (-1);

    return (0);
}

/**************************************************************************

    Description: This function polls all sockets and calls appropriate 
		processing functions when any of them is ready.

    Return:	0 if no activity on all sockets. Non-zero otherwise.

**************************************************************************/

int SH_poll (int ms) {
    struct pollfd pfds[MAX_N_LINKS * MAX_N_STATIONS + 4];
    int ilink [MAX_N_LINKS * MAX_N_STATIONS + 4];
    int i, nfds, ret, ntf_fd, input_first;

    nfds = 0;
    for (i = 0; i < N_links; i++) {
	int pvc, flow_available;
	Link_struct *link;

	link = Links[i];
	if (link->server_fd >= 0) {
	    pfds[nfds].fd = link->server_fd;
	    pfds[nfds].events = POLLIN | POLLPRI;
	    ilink[nfds] = i;
	    nfds++;
	}
	if (Simple_code && link->server == CMT_FAACLIENT) {
	    int k;		/* poll for completed test connection */
	    for (k = 0; k < link->n_rss; k++) {
		if (link->tfd[k] >= 0) {
		    pfds[nfds].fd = link->tfd[k];
		    pfds[nfds].events = POLLOUT;
		    ilink[nfds] = i;
		    nfds++;
		}
	    }
	}
	flow_available = 0;	/* not needed - to disable gcc warning */
	if (link->link_state == LINK_CONNECTED)
	    flow_available = TCP_flow_cntl_len (link, 1);
	for (pvc = 0; pvc < link->n_pvc; pvc++) {
	    if (link->link_state == LINK_CONNECTED && 
		!flow_available) {
		continue;			/* flow control */
            }

	    if (link->pvc_fd[pvc] >= 0) {
		int poll_events;

		poll_events = 0;
		if (link->read_ready[pvc])
		    poll_events = POLLIN | POLLPRI;
		if (link->w_blocked[pvc])
		    poll_events |= POLLOUT;
		if (Is_waiting_connect (link, pvc))
		    poll_events = POLLOUT;
		if (poll_events) {
		    pfds[nfds].fd = link->pvc_fd[pvc];
		    pfds[nfds].events = poll_events;
		    ilink[nfds] = i;
		    nfds++;

		}
	    }
	}

        /* check for second channel */
        if (link->server == CMT_FAACLIENT &&
             link->ch2_link != NULL) {

	   for (pvc = 0; pvc < link->ch2_link->n_pvc; pvc++) {

	       if (link->ch2_link->link_state == LINK_CONNECTED && 
                   link->ch2_link->line_rate > 0 &&
                 link->ch2_link->n_bytes_in_window >
                 (link->ch2_link->line_rate >> 3)
                   - (link->ch2_link->line_rate >> 4)) {
		   continue;			/* flow control */
                }

                if (link->ch2_link->pvc_fd[pvc] >= 0) {
		   int poll_events;

		   poll_events = 0;
		   if (link->ch2_link->read_ready[pvc])
		       poll_events = POLLIN | POLLPRI;
		   if (link->ch2_link->w_blocked[pvc] || 
				Is_waiting_connect (link->ch2_link, pvc))
		       poll_events |= POLLOUT;
		   if (poll_events) {
		       pfds[nfds].fd = link->ch2_link->pvc_fd[pvc];
		       pfds[nfds].events = poll_events;
		       ilink[nfds] = i;
		       nfds++;

		   }
                }
            } /* end for */
         } /* end if */
    }

    ntf_fd = -1;
    if (CMC_get_en_flag () >= 0)
	ntf_fd = LB_NTF_control (LB_GET_NTF_FD);	/* Sync NTF section */
    if (ntf_fd >= 0) {
	pfds[nfds].fd = ntf_fd;
	pfds[nfds].events = POLLIN | POLLPRI;
	nfds++;
    }

    if (nfds == 0) {
	msleep (ms);
	return (0);
    }

    while ((ret = poll (pfds, nfds, ms)) < 0) {
	if (errno != EINTR && errno != EAGAIN) {
	    LE_send_msg (GL_ERROR,  "poll failed (errno %d)", errno);
	    CM_terminate ();
	}
    }

    if (ret == 0) {
	return (ret);
    }

    input_first = rand () % 2;	/* randomize the order of processing input 
				   and output */
    for (i = 0; i < nfds; i++) {

	if (!(pfds[i].revents & (POLLIN | POLLPRI | POLLOUT)))
	    pfds[i].revents |= POLLIN;

	if (pfds[i].fd == ntf_fd) {		/* Sync NTF */
	    LB_NTF_control (LB_NTF_WAIT, 0);
            ret = 0;
	    continue;
	}

	if (input_first && (pfds[i].revents & (POLLIN | POLLPRI))) {
	    TCP_process_input (Links[ilink[i]], pfds[i].fd);
        }

	if (pfds[i].revents & POLLOUT) {
	    Link_struct *link;

	    link = Links[ilink[i]];
	    if (SH_is_test_fd (link, pfds[i].fd))
		TCP_process_input (link, pfds[i].fd);
	    else {
		int pvc;
		for (pvc = 0; pvc < link->n_pvc; pvc++) {
		    if (pfds[i].fd == link->pvc_fd[pvc]) {
			if (Is_waiting_connect (link, pvc))
			    TCP_process_input (link, pfds[i].fd);
			else if (link->w_blocked[pvc])
			    TCP_write_data (link, pvc);
			break;
		    }
		}
	    }
	}

	if (!input_first && (pfds[i].revents & (POLLIN | POLLPRI))) {
	    TCP_process_input (Links[ilink[i]], pfds[i].fd);
        }
    }
    return (ret);
}

/**************************************************************************

    Returns true if the pvc of link is in waiting for connect to complete
    state or false otherwise. This function is needed because we must use 
    POLLOUT for the socket to detect connect call completion.

**************************************************************************/

static int Is_waiting_connect (Link_struct *link, int pvc) {
    if (link->conn_activity == CONNECTING &&
	(link->server == CMT_CLIENT || link->server == CMT_FAACLIENT) &&
	link->connect_state[pvc] == ENABLING)
	return (1);
    return (0);
}

/**************************************************************************

    Returns true if the fd is a redundant server test fd of link or false 
    otherwise. This function is needed because we must use POLLOUT for the
    socket to detect connect call completion.

**************************************************************************/

int SH_is_test_fd (Link_struct *link, int fd) {
    int k;
    if (link->server != CMT_FAACLIENT)
	return (0);
    for (k = 0; k < link->n_rss; k++) {
	if (fd == link->tfd[k])
	    return (1);
    }
    return (0);
}

/**************************************************************************

    Description: This function is the specific house keeping function for
		this comm_manager.

    Inputs:	link - the link structure;
		cr_time - current time;

**************************************************************************/

void SH_house_keeping (Link_struct *link, time_t cr_time)
{

    TCP_house_keeping (link, cr_time);
    return;
}

/**************************************************************************

    Description: This function reads a TCP massage from the "pvc" 
		on "link" and verifies the message. For TCM_DATA, only
		the message header is read. The data read are in 
		link->r_tcp_hd[pvc] and the number of bytes is 
		link->r_hd_cnt[pvc].

    Inputs:	link - the link involved.
		pvc - PVC index.

    Return:	type of the message if message is completely read, 
		TCM_MSG_INCOMPLETE if part of the message is read or 
		TCM_BAD_MSG on failure or the message is bad.

**************************************************************************/

int SH_read_message (Link_struct *link, int pvc) {
    Tcp_msg_header *hd;
    int ret, len;

    while (1) {
    
	/* length of the message to read */
	if (link->r_hd_cnt[pvc] < sizeof (Tcp_msg_header))
	    len = sizeof (Tcp_msg_header);
	else {
	    hd = (Tcp_msg_header *)link->r_tcp_hd[pvc];
	    if (hd->type == TCM_DATA) {
		link->r_msg_size[pvc] = (hd->length & TCM_LENGTH_MASK);
		return (hd->type);
	    }
	    else if (link->r_hd_cnt[pvc] >= hd->length + 
						sizeof (Tcp_msg_header)) {
		return (hd->type);
	    }
	    len = sizeof (Tcp_msg_header) + hd->length;
	    if (len > TCM_MSG_BUF_SIZE) {
		LE_send_msg (GL_ERROR, 
		    "Bad message (type %d) size (%d) on pvc %d link %d",
		    hd->type, hd->length, pvc, link->link_ind);
		return (TCM_BAD_MSG);
	    }
	}
    
	ret = SOCK_read (link, link->pvc_fd[pvc],
		    link->r_tcp_hd[pvc] + link->r_hd_cnt[pvc], 
			    len - link->r_hd_cnt[pvc]);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
			    "Message read failed (ret %d), pvc %d, link %d",
			    ret, pvc, link->link_ind);
	    return (TCM_BAD_MSG);
	}
	if (ret == 0)
	    return (TCM_MSG_INCOMPLETE);
	link->r_hd_cnt[pvc] += ret;
	if (link->r_hd_cnt[pvc] < (int)sizeof (Tcp_msg_header))
	    return (TCM_MSG_INCOMPLETE);

	if (len == sizeof (Tcp_msg_header) &&
		link->r_hd_cnt[pvc] == sizeof (Tcp_msg_header)) {
	    if (Verify_header (link->r_tcp_hd[pvc], pvc, link->link_ind) < 0)
		return (TCM_BAD_MSG);
	}
    }

    return (TCM_MSG_INCOMPLETE);
}

/*********************************************************************

    Byte swaps head fields and verifies the header.

    Inputs:	msg - ponter to the incoming message.
		pvc - PVC index.
		link_ind - link index.

    Return:	0 on success or -1 on failure.

*********************************************************************/

static int Verify_header (char *msg, int pvc, int link_ind) {
    Tcp_msg_header *hd;

    hd = (Tcp_msg_header *)msg;
    hd->type = ntohl (hd->type);
    hd->param = ntohl (hd->param);
    hd->length = ntohl (hd->length);

    switch (hd->type) {

	case TCM_LOGIN:
	case TCM_LOGIN_ACK:
	    if (hd->param != TCM_ID) {
		LE_send_msg (GL_INFO,
			"Bad received message (TCM_ID %d), pvc %d, link %d", 
						hd->param, pvc, link_ind);
		return (-1);
	    }
	case TCM_DATA_ACK:
	case TCM_KEEP_ALIVE:
	case TCM_DATA:
	    if ((hd->type != TCM_DATA && hd->length > TCM_MSG_BUF_SIZE) ||
		((hd->length & TCM_LENGTH_MASK) < 0) ||
		((hd->type == TCM_DATA_ACK || hd->type == TCM_KEEP_ALIVE) &&
					hd->length != 0)) {
		LE_send_msg (GL_INFO,
		"Bad received message (type %d, length %d), pvc %d, link %d", 
					hd->type, hd->length, pvc, link_ind);
		return (-1);
	    }
	    return (0);

	default:
	    LE_send_msg (GL_INFO,
		"Bad received message (type %d), pvc %d, link %d", 
						hd->type, pvc, link_ind);
	    return (-1);
    }
}
