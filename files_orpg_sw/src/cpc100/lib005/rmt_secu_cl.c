/****************************************************************
		
	File: sec_rmt.c	
				
	2/24/94

	Purpose: This module contains the authentication routines 
	for the RMT client library.

	Files used: rmt.h
	See also: 
	Author: 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:58 $
 * $Id: rmt_secu_cl.c,v 1.6 2012/06/14 18:57:58 jing Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  


/*** System include files ***/
#include <config.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#if !defined(LINUX) && !defined(__WIN32__)
#include <inttypes.h>
#endif

/*** Local include files ***/

#include <infr.h>
#include "rmt_def.h"


/*** Definitions / macros / types ***/

#define PASSWORD_LENGTH 120	/* maximum length of client password */

#ifdef THREADED 
static int Cl_type = RMT_MULTIPLE_CLIENT;
#else
static int Cl_type = RMT_SINGLE_CLIENT;
				/* shared server mode: RMT_SINGLE_CLIENT or 
				   RMT_MULTIPLE_CLIENT */
#endif
static int Cl_gid = -1;		/* ID for server sharing */

/*** External references / external global variables ***/

/*** Local references / local variables ***/

static char Password[PASSWORD_LENGTH] = "";	/* the password */



/****************************************************************
			
	This function sets the sharing server mode.

    Input:	gid - group ID for server sharing.

****************************************************************/

void RMT_sharing_client (int gid)
{

    if (gid >= 0) {
	Cl_type = RMT_MULTIPLE_CLIENT;
 	Cl_gid = gid;
    }
    else
	Cl_type = RMT_SINGLE_CLIENT;
}

/****************************************************************
			
	RMT_set_password()			Date: 2/24/94

	This function sets up the password for subsequent 
	RMT_creat_connection calls. The password is used until
	next call of this function.

	The length of the password is limited by 
	PASSWORD_LENGTH - 1. Any extra characters are truncated.
*/

void
  RMT_set_password
  (
      char *passwd		/* password to be used */
) {

    strncpy (Password, passwd, PASSWORD_LENGTH);
    Password[PASSWORD_LENGTH - 1] = '\0';

    return;
}

/****************************************************************
			
	SEC_pass_security_check()		Date: 2/24/94

	This function responds to the servers authentication
	messages.

	Although the first message for authentication came from
	the server we can not start by receiving the message. We
	have to first call a SOC_send_msg to start the timed out.
	We do this by sending an empty message, which causes 
	the request processing timer to be set without actually
	send anything to the server. (This may not need anymore
	since we redefined the time-out, 05/02/00).

	It returns SUCCESS on success or an RMT error code on
	failure.

*/

int
  SEC_pass_security_check
  (
      int fd			/* socket fd */
) {
    ALIGNED_t msg_send[ALIGNED_T_SIZE (AUTH_MSG_LEN)];
    char msg_recv[RET_MSG_SIZE];
    char pswd_buf[PASSWORD_LENGTH + 8];   /* key size is 8 */
    int encrypt_len, pw_needed;
    Auth_msg_t *a;
    int sock_status;

    /* we send an empty msg to set up Start_time for timed out */
#ifdef USE_MEMORY_CHECKER
    memset (msg_send, 0, sizeof (msg_send));
#endif
    sock_status = SOC_send_msg (fd, 0, (char *)msg_send, 0);
    if (sock_status == RMT_CANCELLED)
        return(sock_status);
    if (sock_status < 0)
	return (RMT_AUTHENTICATION_FAILED);

    /* get server's request */
    sock_status = SOC_recv_msg (fd, RET_MSG_SIZE, msg_recv, 0);
    if (sock_status == RMT_CANCELLED)
        return(sock_status);
    if (sock_status < 0)
	return (RMT_AUTHENTICATION_FAILED);

    a = (Auth_msg_t *)msg_send;
    a->type = htonl (Cl_type);
    if (Cl_gid < 0) {
	int pid;
	pid = getpid ();
        a->pid = htonl (pid);
    }
    else
        a->pid = htonl (Cl_gid);

    if (strcmp ("Rejected_auth", msg_recv) == 0) {
	MISC_log ("RMT: Connection rejected because of host auth failure\n");
	return (RMT_REJECTED_BAD_AUTH_HOST);
    }

    pw_needed = 0;
    if (strcmp ("No_password", msg_recv) != 0) {

	pw_needed = 1;
	if (strncmp ("Key:", msg_recv, 4) != 0) {
	    msg_recv[RET_MSG_SIZE - 1] = '\0';
	    MISC_log ("RMT: Unrecognized auth message; %s\n", msg_recv);
	    return (RMT_AUTHENTICATION_FAILED);
	}

	/* get password, encrypt and send it */
	memcpy (pswd_buf, msg_recv + 4, 8); /* get the key */
	strncpy (pswd_buf + 8, Password, PASSWORD_LENGTH);
	pswd_buf[PASSWORD_LENGTH + 8 - 1] = 0;
	encrypt_len = ENCR_encrypt (pswd_buf, ENCRYPT_LEN, a->password);
    }

    sock_status = SOC_send_msg (fd, AUTH_MSG_LEN, (char *)msg_send, 0);
    if (sock_status == RMT_CANCELLED)
        return(sock_status);
    if (sock_status < 0)
	return (RMT_AUTHENTICATION_FAILED);

    if (!pw_needed)
	return (SUCCESS);	/* ok */

    /* wait */
    sock_status = SOC_recv_msg (fd, RET_MSG_SIZE, msg_recv, 0);
    if (sock_status == RMT_CANCELLED)
        return(sock_status);
    if (sock_status < 0)
	return (RMT_AUTHENTICATION_FAILED);
    if (strcmp ("Password_OK", msg_recv) == 0) {
	return (SUCCESS);	/* ok */
    }
    return (RMT_AUTH_REJECTED);
}
