
/******************************************************************

	file: cmt_login.c

	This module contains login functions for the TCP
	comm_manager.
	
******************************************************************/

/* 
 * RCS info
 * $Author Jing$
 * $Locker:  $
 * $Date: 2010/08/18 22:30:34 $
 * $Id: cmt_login.c,v 1.12 2010/08/18 22:30:34 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 *
 * History:
 *
 * 11JUN2003 - Chris Gilbert - CCR #NA03-06201 Issue 2-150. Add "faaclient"
 *             support going through a proxy firewall.
 *
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <infr.h>
#include <comm_manager.h>
#include "cmt_def.h"


/**************************************************************************

    Description: This function sends a login message to the server.

    Inputs:	link - the link involved.
		pvc - the PVC number.

    Return:	returns 0 on success, -1 on failure.

**************************************************************************/

int LOGIN_send_login (Link_struct *link, int pvc)
{
    int msg[TCM_MSG_BUF_SIZE / sizeof (int)];
    Tcp_msg_header *hd;
    int len, ret;

    if (link->ch_num > 0)
	LE_send_msg (LE_VL2, 
		"    Sending login, ch %d, pvc %d, link %d", link->ch_num, pvc, link->link_ind); /*CDG*/
    else
	LE_send_msg (LE_VL2, 
		"    Sending login, pvc %d, link %d", pvc, link->link_ind);
    sprintf ((char *)msg + sizeof (Tcp_msg_header), 
		"%d %d %d %s", link->link_ind, link->n_pvc, 
						pvc, link->password);
    len = strlen ((char *)msg + sizeof (Tcp_msg_header)) + 1;
    hd = (Tcp_msg_header *)msg;
    hd->param = htonl (TCM_ID);
    hd->type = htonl (TCM_LOGIN);
    hd->length = htonl (len);
    len += sizeof (Tcp_msg_header);	/* total length */
    if ((ret = SOCK_write (link, pvc, (char *)msg, len)) != 
								len) {
	LE_send_msg (GL_ERROR, 
		"SOCK_write login failed (ret %d), pvc %d, link %d", 
						ret, pvc, link->link_ind);
	return (-1);
    }

    return (0);
}

/**************************************************************************

    Description: This function reads a client login message and verifies
		the login. The message buffer is cleared after the message
		is verified and accepted.

    Inputs:	link - the link involved.
		pvc - the PVC number.

    Returns 0 on success, TCM_MSG_INCOMPLETE if the message 
    is incomplete or TCM_BAD_MSG if an error is detected.

**************************************************************************/

int LOGIN_accept_login (Link_struct *link, int pvc)
{
    char *msg, *msgbody;
    char password[TCM_MSG_BUF_SIZE];
    Tcp_msg_header *hd;
    int type;
    int link_ind, pvc_ind, n_pvc;

    type = SH_read_message (link, pvc);
    if (type < 0)
	return (type);
    if (type != TCM_LOGIN) {
	LE_send_msg (GL_INFO, 
		"Not a login message (type %d), pvc %d, link %d", 
					type, pvc, link->link_ind);
	return (TCM_BAD_MSG);
    }
    LE_send_msg (LE_VL2, 
		"    Login received, pvc %d, link %d", pvc, link->link_ind);

    msg = link->r_tcp_hd[pvc];
    hd = (Tcp_msg_header *)msg;
    msg[TCM_MSG_BUF_SIZE - 1] = '\0';
    msgbody = msg + sizeof (Tcp_msg_header);
    if (sscanf (msgbody, "%d %d %d %s", 
			&link_ind, &n_pvc, &pvc_ind, password) != 4) {
	LE_send_msg (GL_INFO,  "Bad login message (%s, pvc %d, link %d)", 
						msgbody, pvc, link->link_ind);
	return (TCM_BAD_MSG);
    }


    /* Remove link number check -CDG L1-755 */
    if (n_pvc != link->n_pvc || /*** link_ind != link->link_ind || ***/
	pvc_ind < 0 || pvc_ind >= link->n_pvc) {
	LE_send_msg (GL_INFO, "Bad login msg field (%s), pvc %d, link %d", 
					msgbody, pvc, link->link_ind);
	return (TCM_BAD_MSG);
    }

    if (link->login_state[pvc] == ENABLED) {
	LE_send_msg (GL_INFO, "Duplicated login message, pvc %d, link %d", 
						pvc, link->link_ind);
	return (TCM_BAD_MSG);
    }

    if (strcmp (password, link->password) != 0) {
	LE_send_msg (GL_INFO, "Password check failed, pvc %d, link %d", 
						pvc, link->link_ind);
	return (TCM_BAD_MSG);
    }
    link->r_hd_cnt[pvc] = 0;

    return (pvc_ind);
}

/**************************************************************************

    Description: This function sends an acknowledgement to the client.

    Inputs:	link - the link involved.

    Return:	returns 0 on success or -1 or failure.

**************************************************************************/

int LOGIN_send_server_ack (Link_struct *link)
{
    int msg[TCM_MSG_BUF_SIZE / sizeof (int)];
    Tcp_msg_header *hd;
    int len, ret;

    LE_send_msg (LE_VL2, "    Sending login ACK, link %d", link->link_ind);
    sprintf ((char *)msg + sizeof (Tcp_msg_header), 
		"%d %d %s", link->link_ind, link->n_pvc, "connected");
    len = strlen ((char *)msg + sizeof (Tcp_msg_header)) + 1;
    hd = (Tcp_msg_header *)msg;
    hd->param = htonl (TCM_ID);
    hd->type = htonl (TCM_LOGIN_ACK);
    hd->length = htonl (len);
    len += sizeof (Tcp_msg_header);	/* total length */
    if ((ret = SOCK_write (link, 0, (char *)msg, len)) != len) {
	LE_send_msg (GL_ERROR, 
		"SOCK_write login ACK failed (ret %d), link %d", 
						ret, link->link_ind);
	return (-1);
    }

    return (0);
}

/**********************************************************************

    Description: This function receives and verifies the login 
		acknowledgement from the server. The message buffer is 
		cleared after the message is verified and accepted.

    Inputs:	link - the link involved.

    Returns 0 on success, TCM_MSG_INCOMPLETE if the message 
    is incomplete or TCM_BAD_MSG if an error is detected.

***********************************************************************/

int LOGIN_receive_ack (Link_struct *link)
{
    char *msg, *msgbody;
    Tcp_msg_header *hd;
    int pvc, type;
    int link_ind, pvc_num;
    char connected[TCM_MSG_BUF_SIZE];

    pvc = 0;
    type = SH_read_message (link, pvc);
    if (type < 0)
	return (type);
    if (type != TCM_LOGIN_ACK) {
	LE_send_msg (GL_INFO, 
		"Unexpected login ACK (type %d), pvc %d, link %d", 
					type, pvc, link->link_ind);
	return (TCM_BAD_MSG);
    }
    LE_send_msg (LE_VL2, 
		"    Login ACK received, link %d", link->link_ind);

    msg = link->r_tcp_hd[pvc];
    hd = (Tcp_msg_header *)msg;
    msg[TCM_MSG_BUF_SIZE - 1] = '\0';
    msgbody = msg + sizeof (Tcp_msg_header);

    if (sscanf (msgbody, "%d %d %s", &link_ind, &pvc_num, connected) != 3) {
	LE_send_msg (GL_INFO, "Bad login ack message (%s), pvc %d, link %d", 
					msgbody, pvc, link->link_ind);
	return (TCM_BAD_MSG);
    }

    /* Remove link number check -CDG L1-755 */
    if (/*** link_ind != link->link_ind || ***/
	pvc_num != link->n_pvc || strcmp (connected, "connected") != 0) {
	LE_send_msg (GL_INFO, "Bad login ack msg field (%s), pvc %d, link %d", 
					msgbody, pvc, link->link_ind);
	return (TCM_BAD_MSG);
    }
    link->r_hd_cnt[pvc] = 0;

    return (0);
}
