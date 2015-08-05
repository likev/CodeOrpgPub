
/**************************************************************************

    Event Notification (EN) library public include file.

 **************************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:49 $
 * $Id: en.h,v 1.66 2012/06/14 18:57:49 jing Exp $
 * $Revision: 1.66 $
 * $State: Exp $
 */

#ifndef EN_H
#define EN_H

typedef int en_t;
typedef unsigned int EN_id_t;    /* type for event ID */

#define EN_MAX_ID	(EN_id_t)0xfffcffff
#define EN_ANY		(EN_id_t)0xffffffff
#define EN_REPORT	(EN_id_t)0xfffffffa		/* local reg only */
#define EN_REP_REG_TABLE	(EN_id_t)0xfffffff9	/* post only */
#define EN_DISC_HOST	(EN_id_t)0xfffffff8		/* post only */
#define EN_QUERY_HOSTS	(EN_id_t)0xfffffff7	/* post and local reg */
#define EN_AN_CODE	-2	/* EN code value for AN (must not be -1) */

#define EN_SUCCESS 0	/* function return value on success */
#define EN_FAILURE -1	/* function return value on failure */


/* values for argument "cntl_function" of EN_control () */
enum {
    EN_BLOCK,			/* suspends notification */
    EN_UNBLOCK,			/* resumes notification */
    EN_WAIT,			/* resumes notification and waits until a 
				   notification comes. */
    EN_SET_ERROR_FUNC,		/* registers an error callback function. */
    EN_SET_SIGNAL,		/* registers an alternative notifi. signal. */
    EN_DEREGISTER,		/* switch EN registeration to deregistration */
    EN_SET_UN_SEND_STATE,	/* set LB notification send state */
    EN_CTL_FAILED_IP,		/* get IP of the unaccessible host in NTY */
    EN_SET_SENDER_ID,		/* set the NTY sender's ID */
    EN_PUSH_ARG,		/* push an argument pointer to the stack for
				   next NTF registration to use */
    EN_TO_SELF_ONLY,		/* the next AN posted will be sent to this 
				   process only */
    EN_NOT_TO_SELF,		/* the next AN posted will not be sent to this 
				   process */
    EN_REDELIVER,		/* this NTF needs to be re-deliver */
    EN_GET_N_RHOSTS,		/* get the number of remote hosts in latest
				   AN registration */
    EN_GET_BLOCK_STATE,		/* returns the block state (boolean) */
    EN_SET_AN_GROUP,		/* set a new AN group number */
    EN_GET_AN_GROUP,		/* get AN sender's group number */
    EN_SET_AN_SIZE,		/* set max AN message size */
    EN_GET_NTF_FD,		/* get socket fd to local NTF server */
    EN_SET_PROCESS_MSG_FUNC,	/* set message processing function */
    EN_GET_IN_CALLBACK,		/* get in callback status */
    EN_GET_SIGNAL		/* get the current NTF signal */
};

/* argument values for EN_control (EN_SET_UN_SEND_STATE, ...) and 
   EN_un_send_state () */
enum {EN_UN_SEND_TMP_DISABLE = 2, EN_UN_SEND_GET};

/* values for the first argument of the process message function set by
   EN_control (EN_SET_PROCESS_MSG_FUNC, ...) */
enum {EN_MSG_IN, EN_MSG_OUT};

#define EN_NTF_NO_SIGNAL -1	/* for EN_control (EN_CTL_SIGNAL, ...) */

#define EN_MAX_AN_MSG	128	/* default max size of EN messages */

typedef struct {		/* per-thread variables */
    int ntf_block_intern;	/* cntl flag: NTF delievery blocked 
				   internally. */
    int in_callback;		/* cntl flag: the thread is in NTF callback */
    int Deregister;		/* deregisteration flag for EN register */
    int ntf_block;		/* cntl flag: NTF delievery blocked 
				   externally. */
} EN_per_thread_data_t;

/* error return values (should started with 200 if RPG code compiles) */
#define EN_BAD_ARGUMENT		-341
#define EN_MALLOC_FAILED	-342
#define EN_MSG_TOO_LARGE	-354
#define EN_NOT_SUPPORTED	-373
#define EN_SIGSET_FAILED	-378

#define EN_SEND_FAILED	-381
#define EN_IOCTL_FAILED	-382
#define EN_UNEXP_ACK	-383
#define EN_REG_FAILED	-384
#define EN_CON_LOST	-385
#define EN_WAS_BLOCKED	-386

#define EN_NTF_FAILED           -387
#define EN_LOCAL_IP_NOT_FOUND	-388
#define EN_LOCAL_SV_NOT_CONN	-389
#define EN_SIG_WAIT_FAILED	-399
#define EN_FCNTL_FAILED         -390
#define EN_NOT_USED		-391	/* not used */

/* EN_post flags macros */
#define EN_POST_FLAG_LOCAL  1  /* no longer supported - this flag causes no
				  effect. post event locally (i.e., do not
				  disseminate throughout system)          */
#define EN_POST_FLAG_DONT_NTFY_SENDER  2
                               /* if sender is registered for the event   */
                               /* being posted, do not deliver this event */
                               /* message to the sender                   */
#define EN_POST_FLAG_NTFY_SENDER_ONLY  4
                               /* notify only the sender of this event ...*/
                               /* sender must be registered for the event */
                               /* in order to receive notification of it  */

enum {EN_GET, EN_SET};		/* "func" values for EN_parameters */

/* EN library routine prototypes */
#ifdef __cplusplus
extern "C"
{
#endif
/* EN public functions */
int EN_register (EN_id_t event, void (*notify_func)(EN_id_t, char *, int, void*));
int EN_multi_register (EN_id_t *events, int n_events,
			void (*notify_func)(EN_id_t, char *, int, void*));
int EN_control (int cntl_function, ...);
int EN_post_msgevent (EN_id_t event, const char *msg, int msg_len);
int EN_post_event (EN_id_t event);
int EN_event_lost ();
int EN_sender_id ();

/* functions used internally within libinfr */
void EN_internal_block_NTF ();
void EN_internal_unblock_NTF ();
EN_per_thread_data_t *EN_get_per_thread_data (void);
int EN_internal_register (int code, EN_id_t event, 
		void (*notify_func)(EN_id_t, char *, int, void *));
void EN_close_notify (int code);
int EN_send_to_server (unsigned int host, char *msg, int msg_len);
void EN_parameters (int func, unsigned short *sender_id, int *notify_send);
void EN_set_lb_constants (int lb_msg_expired, 
	int (*set_un_req)(int, EN_id_t, unsigned int, int, int *, int *));
void EN_ntf_server (char *msg, int msg_len, int sock, unsigned int host);
void EN_print_unreached_host (unsigned int host);
int EN_set_sigpoll_fd (int fd);
int EN_register_sigpoll_cb (void (*callback) (int));
void EN_suspend_sigpoll_fd (int fd);
void EN_resume_sigpoll_fd (int fd);
void EN_disconnect_time (unsigned int seconds);


/* backward compatibility functions */
int EN_cntl (int cmd, ...) ;
int EN_cntl_cancel_wait (void) ;
int EN_cntl_block (void) ;
int EN_cntl_block_with_resend (void) ;
int EN_cntl_get_state (void) ;
int EN_cntl_unblock (void) ;
int EN_cntl_wait (unsigned int wait_sec) ;
int EN_deregister (EN_id_t event, void (*encallback)());
int EN_register_obj (EN_id_t event, void (*encallback)(), void *obj_ptr);
int EN_post (EN_id_t event, const void *msg, int msglen, int msg_flags);

int LB_NTF_control (int cntl_function, ...);
int LB_AN_register (EN_id_t event, 
		void (*notify_func)(EN_id_t, char *, int, void *));
int LB_AN_post (EN_id_t event, const char *msg, int msg_len);
int LB_NTF_lost ();
int LB_NTF_sender_id ();

#ifdef __cplusplus
}
#endif


/* for backward compatibility */

#ifdef LB_THREADED
#define EN_THREADED
#endif

#define LB_NTF_REPORT		EN_REPORT
#define LB_AN_FD		EN_AN_CODE

#define LB_NTF_BLOCK		EN_BLOCK
#define LB_NTF_UNBLOCK		EN_UNBLOCK
#define LB_NTF_WAIT 		EN_WAIT
#define LB_NTF_ERROR		EN_SET_ERROR_FUNC
#define LB_NTF_SIGNAL		EN_SET_SIGNAL
#define LB_NTF_DEREG		EN_DEREGISTER
#define LB_UN_SEND		EN_SET_UN_SEND_STATE
#define LB_NTF_FAILED_IP	EN_FAILED_IP	
#define LB_NTF_SET_SENDER_ID	EN_SET_SENDER_ID	
#define LB_NTF_PUSH_ARG		EN_PUSH_ARG
#define LB_AN_SELF_ONLY		EN_TO_SELF_ONLY
#define LB_AN_NOT_TO_SELF	EN_NOT_TO_SELF	
#define LB_NTF_REDELIVER	EN_REDELIVER	
#define LB_GET_N_RHOSTS		EN_GET_N_RHOSTS
#define LB_NTF_BLOCK_STATE	EN_GET_BLOCK_STATE	
#define LB_AN_GROUP		EN_SET_AN_GROUP
#define LB_GET_AN_GROUP		EN_GET_AN_GROUP
#define LB_SET_AN_SIZE		EN_SET_AN_SIZE
#define LB_GET_NTF_FD		EN_GET_NTF_FD

#define LB_UN_SEND_TMP_DISABLE	EN_UN_SEND_TMP_DISABLE
#define LB_UN_SEND_GET		EN_UN_SEND_GET
#define LB_NTF_NO_SIGNAL	EN_NTF_NO_SIGNAL
#define LB_MAX_AN_MSG		EN_MAX_AN_MSG

#define LB_REP_REG_TABLE	EN_REP_REG_TABLE
#define LB_DISC_HOST		EN_DISC_HOST

#define EV_id_t EN_id_t

    /* The following LB_* are used only by infrerr.c and 
					oclbbaseobjectiostream.cpp */
#define LB_NTF_SEND_FAILED EN_SEND_FAILED
#define LB_IOCTL_FAILED EN_IOCTL_FAILED
#define LB_NTF_UNEXP_ACK EN_UNEXP_ACK
#define LB_NTF_REG_FAILED EN_REG_FAILED
#define LB_NTF_CON_LOST EN_CON_LOST
#define LB_CON_LOCAL_SV_FAILED EN_LOCAL_SV_NOT_CONN
#define LB_NTF_FAILED EN_NTF_FAILED
#define LB_LOCAL_IP_NOT_FOUND EN_LOCAL_IP_NOT_FOUND
#define LB_LOCAL_SV_NOT_CONN EN_LOCAL_SV_NOT_CONN
#define LB_SIG_WAIT_FAILED EN_SIG_WAIT_FAILED
#define LB_SIGSET_FAILED EN_SIGSET_FAILED

#define EN_WRITE_FAILED EN_SEND_FAILED
#define EN_EVTREG_ENTRY_NOT_FOUND EN_NOT_USED

/* for EN_cntl argument */
#define EN_STATE_BLOCKED	1
#define EN_STATE_UNBLOCKED	2
#define EN_STATE_WAIT		3
#define EN_CNTL_STATE			1
#define EN_CANCEL_WAIT	((EN_STATE_WAIT) + 1)
#define EN_QUERY_STATE	((EN_CANCEL_WAIT) + 1)

/* The following are used in RPG but do not make any difference */
#define EN_POST_ERR_EVTCD 0
#define EN_POST_NUM_RESERVED_EVTCDS 15
#define EN_POST_MIN_RESERVED_EVTCD 1
#define EN_POST_MAX_RESERVED_EVTCD ((EN_POST_MIN_RESERVED_EVTCD) + \
                                    (EN_POST_NUM_RESERVED_EVTCDS))
#define EN_NTFYR_EXCEPTION_EVTCD ((EN_POST_MIN_RESERVED_EVTCD) + 1)

/* end of for backward compatibility */

#endif			/* end of #ifndef EN_H */
