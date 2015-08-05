/**************************************************************************
		
      Module: rmt.h

 Description:
	Purpose: This is the header file for the RMT module. This file 
	defines the objects that used by both the RMT lib and the RMT 
	application programs. It contains the function templates that are 
	open to the RMT library user. 

	This file must be included in all RMT source code files and all 
	application programs that uses RMT library routines.

 **************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:34 $
 * $Id: rmt.h,v 1.45 2012/07/27 19:33:34 jing Exp $
 * $Revision: 1.45 $
 * $State: Exp $
 */

#ifndef RMT_H
#define RMT_H

#include <stdio.h>


#define RMT_ON      1
#define RMT_OFF     0
#define RMT_LOCK    1
#define RMT_GLOBAL_LOCK 2
#define RMT_UNLOCK  0
#define RMT_GLOBAL_UNLOCK  3
#define RMT_TRUE    1
#define RMT_FALSE   0
#define RMT_COMPRESSION_ON  1
#define RMT_COMPRESSION_OFF 0

#define RMT_SUCCESS     0
#define RMT_FAILURE     -1

#define NAME_SIZE 80		/* Name string size */

#define RMT_USE_DEFAULT_PORT 0	/* argument for RMT_port_number() for 
			         setting the default port number */

#define STATIC_BUFFER 3056	/* static buffer size for messages- To fit 
				   in two IP packets */
#define RMT_PREFERRED_SIZE STATIC_BUFFER-256
				/* preferred size for input/output string */

typedef int rmt_t;		/* type for header fields */

#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff	/* for unavailable inet address */
#endif

/* The following two functions need to be redefined for little endian machine */
/* byte swap for hardware independence */
#ifdef LITTLE_ENDIAN_MACHINE
#define htonrmt(a) RMT_byte_swap(a)
#else
#define htonrmt(a) (a)
#endif
#define ntohrmt(a) htonrmt(a)

enum {  RMT_DATA_RECEIVED, RMT_DATA_SENT, RMT_DISCONNECT, RMT_IO_RETRY  };

#define RMT_LAST_SEGMENT 1		/* segment_flag that is set if a segment is the last segment */

typedef struct rmt_transfer_event
{
    int event;				/*  data transfer event */
    int segment_flags;			/*  flags for this segment */
    int no_of_segment_bytes;		/*  number of bytes transferred this segment */
    int no_of_bytes;    		/*  number of bytes transferred so far  */
    int total_no_of_bytes;		/*  Total number of bytes to be transferred */
    float no_of_seconds;		/*  Number of seconds for the transfer */
    void* user_data;			/*  User data */
} rmt_transfer_event_t;

typedef int (*rmt_transfer_event_callback_t)(rmt_transfer_event_t* event);

#ifdef __cplusplus
extern "C"
{
#endif  

int RMT_create_connection (char *remote_host_name);
int RMT_set_current (int fd);
int RMT_port_number (int port);
void RMT_messages (int sw);
void RMT_time_out (int time);
int RMT_set_time_out (int time);
void RMT_ask_password (char *prompt, int pw_size, char *password);
int RMT_close_connection (void);
void RMT_set_password (char *password);
void RMT_send_log (char *msg, int beep);
char *RMT_get_conf_file (int *update_cnt);
void RMT_use_buffer (int size, char *buf);
void RMT_free_user_buffer (char **buf);
void RMT_register_msg_callback (void (*callback)(char *, int, int, unsigned int));
int RMT_send_msg (unsigned int host, char *msg, int msg_len, int *fd);
int RMT_poll_write (int fd);
void RMT_close_msg_client (int fd);
void RMT_local_read (int func);
int RMT_connect_host (unsigned int ip);
char *RMT_pack_msg (char *msg, int msg_len, int max_len, int *plen);
void RMT_sharing_client (int gid);
int RMT_set_lost_conn_callback (void (*cb) (int fd, char *host));
int RMT_is_connected (char *mach_name);

int RMT_set_compression(int compression_flag);
int RMT_compression();
int RMT_cmd_disconnected (unsigned int ipaddr);
int RMT_is_reserved_port ();

int RMT_listen_for_progress(rmt_transfer_event_callback_t callback, void* user_data);

int RMT_initialize_user_funcs ();
void RMT_kill_clients (int addr);
void RMT_mt_lock_sv_fd (int fd, int lock);
int RMT_is_fd_valid (int fd);
int RMT_is_fd_of_this_thread (int fd);
int RMT_access_disc_file (int is_server, char *buf, int buf_size);
int RMT_lookup_host_index (int func, void *h_name, int index);
unsigned int RMT_bind_address ();

#ifdef __cplusplus
}
#endif  


/* Error code */
#define RMT_BAD_REMOTE_HOST_NAME  -1001
#define RMT_TOO_MANY_CONNECTIONS  -1002
#define RMT_PORT_NUMBER_NOT_FOUND -1004
#define RMT_GETHOSTBYNAME_FAILED  -1005
#define RMT_OPEN_SOCKET_FAILED    -1006
#define RMT_CONNECT_FAILED        -1007
#define RMT_SET_SOCK_PROP_FAILED  -1009
#define RMT_AUTHENTICATION_FAILED -1010		/* auth procedure failed */

#define RMT_BAD_ARG_CLIENT_SIDE   -1003
#define RMT_CONNECTION_NOT_SET    -1008
#define RMT_TOO_MANY_LC_HOSTS	  -1011

#define RMT_BAD_ARG_SERVER_SIDE   -1012
#define RMT_USER_FUNC_UNDEFINED   -1013
#define RMT_BAD_USER_FUNC_RETN    -1014
#define RMT_REQ_MSG_TOO_LARGE     -1015

#define RMT_RPC_REENTRY		  -1016
#define RMT_INVALID_FD            -1017
#define RMT_TIMED_OUT             -1018
#define RMT_SELECT_FAILED	  -1019
#define RMT_WRITE_FAILED	  -1020
#define RMT_SERVER_DISCONNECTED	  -1021
#define RMT_READ_FAILED		  -1022

#define RMT_BAD_RESPONSE	  -1023
#define RMT_MALLOC_FAILED	  -1024

#define RMT_BAD_CONTROL_MSG	  -1025
#define RMT_BAD_USER_BUFFER	  -1026
#define RMT_ALREADY_CONNECTED	  -1027
#define RMT_OPEN_HOST_FAILED	  -1028
#define RMT_TOO_MANY_MSG_HOSTS	  -1029

#define RMT_ABORT		  -1030
#define RMT_HOME_UNDEFINED	  -1031
#define RMT_SOCKET_DISCONNECTED	  -1032
#define RMT_REJECTED_BAD_AUTH_HOST	-1033	/* auth failed because of the
						   the host is rekected */
#define RMT_AUTH_REJECTED	-1034		/* auth failed because of bad
						   password */
#define RMT_CANCELLED		  -1035		/* RPC operation was cancelled */
#define RMT_COMPRESSION_FAILED	  -1036		/* Compression of data failed */
#define RMT_DECOMPRESSION_FAILED  -1037		/* Decompression of data failed */
#define RMT_PTHREAD_KEY_CREATE_FAILED  -1041	/* pthread_key_create failed  */
#define RMT_PROGRESS_CALLBACK_NOT_FOUND  -1038  /* Could not remove the specified callback based on user_data address */
#define RMT_TOO_MANY_PROGRESS_CALLBACKS  -1039  /* Could not register the specified callback because too many callbacks
							are already registered */
#define RMT_CLOSE_IN_OTHER_THREAD	  -1040		/*  */

/* special values for argument "msg_len" passed to user's callback in
   RMT server for messaging service */
#define RMT_MSG_WRITE_READY		-1
#define RMT_MSG_CONN_LOST		-2
#define RMT_MSG_TIMER			-3
#define RMT_MSG_CONFIG_UPDATE		-4

/* values for argument "func" of function RMT_local_read */
enum {RMT_LOCAL_READ_RESUME, RMT_LOCAL_READ_PAUSE};

#define RMT_LOCAL_HOST 0xffffffff
				/* special IP address for local host */

enum {RMT_LHI_H2IX, RMT_LHI_I2IX, RMT_LHI_IX2I, RMT_LHI_IX2H,
						RMT_LHI_SET, RMT_LHI_LIX};
		/* values for arg func of RMT_lookup_host_index */

/* in rmtd.c */
#ifdef __cplusplus
extern "C"
{
#endif  

int RMTD_server_init (int argc, char **argv, char **envp,
    void (*set_up_user_functions) (int (**user_func) (int,char *,char **)));
void RMTD_server_main ();
int RMT_i_am_server (void);
int RMT_is_in_backgroud ();

rmt_t RMT_byte_swap (rmt_t x);

#ifdef __cplusplus
}
#endif  


#endif   /* RMT_H */
