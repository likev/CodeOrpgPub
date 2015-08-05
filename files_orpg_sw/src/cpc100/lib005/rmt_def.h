/****************************************************************
		
	File: rmt_def.h	
				
	2/22/94

	Purpose: This file defines all global objects that are used by 
	both the server and the RMT library routines but not open to 
	RMT application programs. 

	This file must be included in all RMT library source code files.

	Files used:
	See also: 
	Author: 

****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/08/09 20:06:35 $
 * $Id: rmt_def.h,v 1.43 2012/08/09 20:06:35 jing Exp $
 * $Revision: 1.43 $
 * $State: Exp $
 */  

#ifndef RMT_DEF_H
#define RMT_DEF_H

#include <stdio.h>
#include <rmt.h>

#ifdef DECOSF
#define long int
#endif

#define RMT_SET     1
#define RMT_RESET   0

#define SUCCESS	    0
#define FAILURE     -1


#define RMTD_NO_REQUEST -2     /* return value of SOCD_accept_connection */

#define RMT_INVALID  -1

#define RMT_MSG_ID  9836475	/* a special number to identify a RMT messaging
				   message */

#define WAIT_FOR_MEMORY_TIME 500	/* malloc retry time in ms */

#define TEST_BYTE 254		/* A special byte value used for detecting 
				   the connection status */
#define MAX_NUM_FUNCTIONS 32	/* This can not be larger than 32 */
#define HEAD_SIZE 8		/* Message header size */

/* the following sizes must be large enough for authentication messages (12 bytes) */
/* the encrypted password sent from the client is limited by REQ_MSG_SIZE */
#define REQ_MSG_SIZE 40		/* Request message size */
#define RET_MSG_SIZE 20		/* Return message size */
#define MIN_MSG_SIZE 40		/* min RMT messaging message size */

#define MIN_SERVER_TIMER 1	/* the minimum server's timer */
#define MAX_SERVER_TIMER 10	/* the maximum server's timer */
#define CONNECTION_CHECK_RATE	5
				/* number of connection checks in keep-alive
				   time period */

#define MIN_USER_FUNC_RETN  -999 /* minimum number a user function can return */


#define SELECT_READ   0		/* argument value for SCSH_wait */
#define SELECT_WRITE  1		/* argument value for SCSH_wait */

#define TIME_EXPIRED 0		/* return value of SCSH_wait */
#define IO_READY 1		/* return value of SCSH_wait */

#define MAX_CLIENTS    256      /* maximum number of total pending clients */

#define PASSWORD_SIZE  128

#define LENGTH_MASK 0xfffffff	/* we use 28 bits for the input argument string
				   length and 4 highest bits for user info */
#define ENCRYPT_LEN  24		/* size of encrypt password */
#define KEY_SIZE  8

enum {				/* client sharing mode */
    RMT_SINGLE_CLIENT, 		/* single client per child RMT server */
    RMT_MULTIPLE_CLIENT		/* multiple clients per child RMT server */
};

#define RMT_TIMEDOUT 60		/* default timed out time in seconds */
#define CHECK_PERIOD 2		/* timed out check period (in seconds) */

#define RMT_FUNC_CANCELED -2	/* used for function return value */

typedef struct {		/* authentication message from the client to
				   the server */
    int pid;			/* client ID */
    int type;			/* RMT_SINGLE_CLIENT or RMT_MULTIPLE_CLIENT */
    char password[ENCRYPT_LEN];	/* password */
} Auth_msg_t;

typedef struct {		/* structure for client registration */
    int childPid;		/* child server process pid;*/
    int addr;			/* host address of the client (LBO) */
    int clientPid;		/* client pid */
    int pipeFd;			/* child server stream pipe socket fd*/
} Client_regist;

typedef struct {		/* structure for per-thread static vars */
    int user_buf_size;		/* size of the user provided buffer */
    char *user_buf;		/* pointer to user provided buffer */
    int current_fd;		/* fd for current remote connection */
    char *small_buf;		/* buffer for request and return message used 
				   in RMTc_user_func_general */
    char *large_buf;		/* dynamically allocated large buffer used in 
				   RMTc_user_func_general */
    char *uncompressed_buf;	/* Dynamically allocated uncompressed data 
				   used in RMTc_user_func_general */

    int wait_time;		/* Expiring time of a remote call */
    int start_time;		/* starting time of a remote call */

    rmt_transfer_event_callback_t transfer_callback;
				/* Callback function for notification
				   of data transfer */

    void* rmt_transfer_event_user_data;
				/*  user data for notification callback */
    float prev_clock;
    int compression_status;	/*  compression on or off */
} RMT_per_thread_data_t;

#define AUTH_MSG_LEN ENCRYPT_LEN + 16
				/* message length of client auth msg; must be
				   larger than struct Auth_msg_t */

/* Global functions that are not open to the RMT users */

/* in sec_rmtd.c */
int SECD_initialize_sec_rmtd (void);
void SECD_get_random_key (char *buf, int len);
char *SECD_get_password ();

/* in get_client.c */
int GCLD_initialize_host_table (char *conf_name);
int GCLD_client_sock_ready (int fd);
int GCLD_get_client (int fd, int *cl_type, 
				int *cl_pid, unsigned int *cl_addr);
int GCLD_monitor_net ();

/* in sock_rmtd.c */
int SOCD_open_server (int);
int SOCD_send_msg (int fd, int nmsg, char *msg, int cancel_enabed);
int SOCD_recv_msg (int fd, int nb, char *msg, int cancel_enabed);
int SOCD_accept_connection (int fd, unsigned long *cid);

/* in encrypt.c */
int ENCR_encrypt (char *str, int buf_size, char *buf);

/* in cl_register.c */
int CLRG_register_client (Client_regist *);
void CLRG_store_child_pid (int cpid);
int CLRG_initialize (int n_child);
void CLRG_terminate (char *hname);
int CLRG_get_client_info(int,int);
void CLRG_close_other_client_fd (int fd);
void CLRG_term_client (int cl_pid, int cl_addr);
void CLRG_process_sigchild ();
void CLRG_sigchld ();

/* in rmt.c */
int RMTc_user_func_general (int id, int len, char *input_string, char **output_string);
int RMTc_get_current_fd (void);
int RMT_get_segment_size();
int RMT_report_progress(int event_in, int seg_flags, int no_of_seg_bytes, int no_of_bytes, int total_no_of_bytes);
RMT_per_thread_data_t *RMT_get_per_thread_data (void);

/* in port_number.c */
int PNUM_get_port_number (void);
int PNUM_set_port_number (int port_n);
void PNUM_set_default_port_number (void);
void PNUM_disconnect_time (int t);
unsigned int PNUM_get_local_ip_from_rmtport ();
int PNUM_access_hosts_file (int is_server, char *buf, int buf_size);

/* in sec_rmt.c */
int SEC_pass_security_check (int fd);

/* in sv_register.c */
int SVRG_get_fd_by_name (char *mach_name);
int SVRG_regist_new_server (int nfd, char *mach_name, unsigned long ipa);
void SVRG_remove_a_server (int nfd);
char *SVRG_get_server_by_fd (int fd);
int SVRG_check_ip (unsigned long cid);
int SVRG_test_lost_conns (int fd, void (*cb) (int, char *));
void SVRG_lock_sv_reg_table (int lock);

/* in sock_rmt.c */
int SOC_send_msg (int fd, int msg_len, char *msg, int progress_report);
int SOC_recv_msg (int fd, int nb, char *msg, int progress_report);
int SOC_connect_server (char *hostname, unsigned long *ipa);
int RMT_bind_selected_local_ip (int sockfd);

/* in send_msg.c */
int RMT_open_msg_host (unsigned int host, int wait);
void RMTSM_close_disc_msg_hosts ();
void RMTSM_close_msg_hosts ();

/* in msg_rmtd.c */
int MSGD_open_msg_server (int port_number);
void MSGD_process_msgs (int pfd);
void MSGD_register_rpc_waiting_fds (int n_clfds, int *clfds);
void MSGD_set_max_n_local_senders (int max_n_local_senders);
void MSGD_close_msg_clients ();

/* in sock_shared.c */
int SCSH_wait (int fd, int sw, int time);
int SCSH_local_socket_address (void **add, char **so_name);
char *RMT_compress (int size, char *data, int *csize);
char *RMT_decompress (int size, char *data, int *dsize);
void RMT_free_buffer (void *buf);
int SCSH_get_rssd_home (char *buf, int buf_size);

#endif   /* RMT_DEF_H */
