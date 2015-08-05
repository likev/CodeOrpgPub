
/*******************************************************************

    Module: css.h

    Description: Public header file for the Client/Server Support
		(CSS) module.

*******************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 14:37:18 $
 * $Id: css.h,v 1.14 2005/09/14 14:37:18 jing Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 * $Log: css.h,v $
 * Revision 1.14  2005/09/14 14:37:18  jing
 * Update
 *
 * Revision 1.12  2002/03/12 16:44:53  jing
 * Update
 *
*/

#ifndef CSS_H
#define CSS_H

typedef struct {		/* for passing request info on server side */
    int cid;			/* connection ID */
    int sv_id;			/* server ID */
    int conn_n;			/* connection number */
    unsigned int seq;		/* request/response sequence number */
    int user_id;		/* user id */
} CSS_client_t;

#define CSS_ANY_LINK -1		/* special value for CSS_get_response's 
				   argument "conn_n" */
#define CSS_SERVER_MSG 0	/* req number for unsolicit server msgs */

enum {REQUEST_READY = 1, RESPONSE_READY};
				/* for argument "type" of CSS_wait */

#define CSS_S_NAME_NOT_FOUND		-160
#define CSS_MALLOC_FAILED		-161
#define CSS_TIMED_OUT			-162

#define CSS_SERVER_ADDR_RESET		-163
#define CSS_DUPLICATED_SERVER		-164
#define CSS_MSG_OUT_SEQ			-165
#define CSS_BAD_ARGUMENT		-166

#define CSS_SV_ADDR_ERROR		-167
#define CSS_HOST_NAME_ERROR		-168
#define CSS_OPEN_SOCKET_FAILED		-169
#define CSS_SET_SOCK_PROP_FAILED	-170

#define CSS_CONNECT_FAILED		-171
#define CSS_WRITE_FAILED		-172
#define CSS_BIND_FAILED			-173
#define CSS_LISTEN_FAILED		-174

#define CSS_BAD_MSG_HD			-175
#define CSS_BUFFER_ERROR		-176
#define CSS_LOST_CONN			-177
#define CSS_SERVER_DOWN			-178

#define CSS_ADD_POLL_FD_FAILED		-179


#ifdef __cplusplus
extern "C"
{
#endif

int CSS_sv_main (char *sv_addr, 
		int maxn_conns, int hk_seconds,
		int (*proc_req_func)(char *, int, char **), 
		int (*housekeep_func)(void));
int CSS_set_server_address (char *s_name, char *sv_addr, int conn_n);
int CSS_get_service (char *s_name, char *req, int req_len, 
				int wait_seconds, char **resp);
int CSS_get_cid (char *s_name);
int CSS_send_request (int cid, char *req, int req_len);
int CSS_get_response (int cid, char **resp, unsigned int *req_seq);

int CSS_sv_init (char *sv_addr, int maxn_conns);
int CSS_sv_get_request (int cid, CSS_client_t **client, char **req);
int CSS_sv_send_response (CSS_client_t *client, 
					int msg_len, char *msg);
int CSS_sv_send_msg (int sv_id, int conn_n, unsigned int seq, 
					int msg_len, char *msg);
int CSS_wait (int wait_ms, int *type);
int CSS_close (int id);
int CSS_get_poll_fds (int array_size, int *fds, int *poll_flag);
int CSS_add_poll_fd (int fd, int poll_flag, 
			void (*cb_func)(int fd, int ready_flag));
int CSS_set_user_id (int cid, int user_id, char *password);
int CSS_sv_user_id (int sv_id, int user_id, char *password);

/* for backward compatibility */
int CSS_sv_main_loop (char *sv_addr, int n_preqs, 
		int n_conns, int hk_seconds,
		int (*proc_req_func)(char *, int, char **), 
		int (*housekeep_func)(void),
		void (*err_func)(char *));
int CSS_set_server_addr (char *s_name, char *sv_addr, int conn_n);
void CSS_convert_address (char *old, char *new_addr);
int CSS_close_conn (int cid);

#ifdef __cplusplus
}
#endif


#endif		/* #ifndef CSS_H */
