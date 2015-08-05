/****************************************************************
		
    Module: lb_extern.h
				
    Description: This module contains the LB external interface
		for implementing the messaging serviced required
		by LB notification and LB_wait.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 1999/03/03 20:28:17 $
 * $Id: lb_extern.h,v 1.6 1999/03/03 20:28:17 jing Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 * $Log: lb_extern.h,v $
 * Revision 1.6  1999/03/03 20:28:17  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.4  1999/03/01 21:31:19  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.3  1998/12/02 18:23:11  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1998/12/01 20:20:10  jing
 * NO COMMENT SUPPLIED
 *
 * 
*/

#ifndef LB_EXTERN_H
#define LB_EXTERN_H

#include <lb.h>

enum {				/* values for the action argument of
				   LB_check_update () */
    LB_FREE_REQ,		/* remove wake-up requests */
    LB_CHECK_AND_FREE,		/* check LB update and remove wake-up requests 
				   */
    LB_CHECK_AND_REQ,		/* check LB update and set one-time wake-up 
				   request; Exclusive LB lock is applied. */
    LB_SET_REQ_ON,		/* turn on wake-up request */
    LB_CHECK_ONLY		/* check LB update only */
};

/* function, to be called by LB, in an external implementation of the LB NTF 
   service. These functions are defined as static functions in LB and to be
   set by LB_*_set_externs functions */

#ifndef LB_NTF_EXTERN_AS_STATIC
int LB_EXT_sv_cntl (int func, int arg);					/*
	- function for various server  controls:
	func = LB_EXT_POLL_WRITE: write available polling needed for 
		socket fd "arg", which is passed from LB_ntf_server.
	func = LB_EXT_LOCAL_READ: local client data read control specified by
		fd = LB_EXT_LOCAL_READ_PAUSE/LB_EXT_LOCAL_READ_RESUME;
	func = LB_EXT_CLOSE_CLIENT: close client of socket fd "arg".
		where "arg" is passed from LB_ntf_server.
	func = LB_EXT_OPEN_CONN: Open a connection to host of IP address 
		"arg".							*/
void LB_EXT_sv_error (char *msg);					/* 
	- function reporting error message "msg" to the server log. 	*/
int LB_EXT_get_client_IP (int buf_size, unsigned int *buf); 		/* 
	- function for getting IP addresses of all configured remote hosts. 
	The IPs are returned in "buf" and the buf_size is "buf_size" (i.e. 
	At most "buf_size" IPs will be returned in "buf". This function returns
	the number of IPs put in "buf". 				*/
char *LB_EXT_pack_msg (char *msg, int len, int *plen);			/*
	- function returns the packed msg that is suitable for sending to
	the messaging server. This is defined identically as RMT_pack_msg. 
	See there for further description.				*/
int LB_EXT_send_msg (unsigned int host, char *msg, int msg_size, int *fd);/* 
	- function sending a message to a remote host by IP address. This
	is defined identically as RMT_send_msg (). See there for further 
	description. 							*/
int LB_EXT_set_un_req (int cmfd, LB_id_t msgid, 
		unsigned int host, int pid, int *a_pid, int *a_fd);	/*
	- function sets a UN wake-up record in LB "cmfd". This is the remote 
	implementation of LB_set_un_req. See LB_set_un_req for further details.
									*/
int LB_EXT_check_update (int fd, unsigned int host, int pid, int action);/*
	- function checks LB update status and sets the LB-wait wake-up request 
	for LB "fd". This is a remote implementation for LB_check_update. 
	See LB_check_update for further details.			*/
#endif

/* external function called by LB if compiler flag LB_NTF_SERVICE is used */
void LB_extern_service ();
			
/* values for the first argument of LB_EXT_sv_cntl */
enum {LB_EXT_POLL_WRITE, LB_EXT_LOCAL_READ, 
				LB_EXT_CLOSE_CLIENT, LB_EXT_OPEN_CONN};

/* values for the second argument of LB_EXT_sv_cntl (LB_EXT_LOCAL_READ, ) */
enum {LB_EXT_LOCAL_READ_PAUSE, LB_EXT_LOCAL_READ_RESUME};
				
/* LB_EXT_sv_cntl (LB_EXT_OPEN_CONN, ... ) return value */
#define LB_EXT_RET_TIMED_OUT		-10

/* value for the second argumet of LB_ntf_server */
#define LB_NTF_WRITE_READY		-1
#define LB_NTF_CONN_LOST		-2
#define LB_NTF_TIMER			-3

#define LB_AN_FD	-2	/* special fd value for AN (must not be -1) */

#define LB_UN_PARAMS_GET 0xffffffff
	/* special value for argument "un_params" of LB_UN_parameters. */

/* functions called by an external implementation of the LB NTF service */
int LB_check_update (int fd, unsigned int host, int pid, int action);
void LB_wait_set_externs (int (*check_update)());

void LB_ntf_server (char *msg, int msg_len, 
				int fd, unsigned int host);
void LB_ntf_sv_set_externs (int (*sv_cntl)(), int (*client_IP)(), 
			void (*sv_error)(), char *(*pack_msg) ());
int LB_set_un_req (int fd, LB_id_t msgid, unsigned int host, 
					int pid, int *a_pid, int *a_fd);
void LB_ntf_set_externs (int (*send_msg)(), int (*set_un_req)());
void LB_internal_block_NTF ();
void LB_internal_unblock_NTF ();
unsigned int LB_UN_parameters (unsigned int un_params);

unsigned int LB_write_failed_host (int lbd);
void LB_print_unreached_host (unsigned int host);

#endif		/* LB_EXTERN_H */
