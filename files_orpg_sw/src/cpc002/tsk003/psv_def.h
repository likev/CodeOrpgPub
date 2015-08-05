
/***********************************************************************

    Description: Internal include file for p_server.

***********************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/05/09 20:47:15 $
 * $Id: psv_def.h,v 1.86 2012/05/09 20:47:15 jing Exp $
 * $Revision: 1.86 $
 * $State: Exp $
 */  

#ifndef PSV_DEF_H

#define PSV_DEF_H

#include <time.h>
#include <prod_distri_info.h>
#include <prod_status.h>
#include <prod_gen_msg.h>
#include <legacy_prod.h>
#include <a309.h>
#include <comm_manager.h>

#define MAX_N_USERS	50	/* max number of users served by a p_server 
				   instance */
#define MAX_STATUS_MSGS 10	/* maximum number of certain status
				   messages generated in a session */

#define CONNECT_TRY_TIME 30	/* waiting time after sending a connect 
				   request */
#define AUTH_TIME 60		/* max time for authentication */ 
#define CONNECT_FAILURE_TIME 30	/* waiting time after a connect failure */
#define WAIT_TIME 2		/* waiting time after a suspended event 
				   processing */
#define DISCON_TIME 20		/* time for completing a line disconnect 
				   procedure */
#define PUS_PUBLISH_TIME 10	/* product user line util publish interval */

#define MAX_NB_UTIL_PERIOD 5	/* max NB util reporting period */

enum {				/* events processed by p_server; listed in 
				   decreasing priorities */
    EV_NEXT_STATE,		/* special p_server internal event */

    EV_USER_CMD,		/* user control command received */

    EV_APUP_STAT_REQ,		/* (2) command req APUP status received */

    EV_WX_ALERT,		/* alert message ready, RPG data ready */

    EV_NEW_VOL_SCAN,		/* new volume scan started; RPG status */

    EV_WRITE_COMPLETED,		/* (5) write to user completed; WAN status */
    EV_LOST_CONN,		/* link to the user unexpectedly disconnected; 
				   WAN status */
    EV_COMM_MANAGER_TERMINATE,	/* comm_manager terminates */
    EV_COMM_MANAGER_START,	/* comm_manager started */

    EV_CONNECT_SUCCESS,		/* connection request succeeded */
    EV_CONNECT_FAILED,		/* (10) connection request failed */
    EV_DISCON_SUCCESS,		/* disconnect request succeeded */
    EV_DISCON_FAILED,		/* disconnect request failed */

    EV_USER_DATA,		/* user message available */
    EV_TEXT_MSG,		/* text message available */

    EV_TRANS_PROD,		/* (15) transformed product ready */
    EV_ONETIME_PROD,		/* one-time product ready */
    EV_ROUTINE_PROD,		/* new routine product generated */

    EV_ENTER_TEST,		/* enter test mode */
    EV_ENTER_INACTIVE,		/* enter inactive mode */
    EV_EXIT_TEST,		/* (20) exit from test mode */
    EV_EXIT_INACTIVE,		/* exit from inactive mode */

    EV_PROD_GEN_UPDATED		/* prod generation list updated */
};

#define FIRST_TIMER_EVENT 100	/* first timer event number */
#define MAX_N_TIMERS	9	/* number of timers defined; must >= the 
				   number of timer events defined. */
enum {				/* timer events */
    EV_CONNECT_TIMER = FIRST_TIMER_EVENT,
				/* connect timer expired */
    EV_AUTH_TIMER,		/* authentication timer expired */
    EV_CONNECT_FAILURE_TIMER,	/* connect failure timer expired */
    EV_WAIT_TIMER,		/* wait timer expired */
    EV_DISCON_TIMER,		/* disconnect timer expired */
    EV_SESSION_TIMER,		/* session timer expired */
    EV_AFTER_CONN_TIMER,	/* after connection timer expired */
    EV_RPS_TIMER,		/* waiting for RPS timer expired */
    EV_PUS_PUBLISH_TIMER	/* product user line util publishing time */
};

enum {				/* p_server states; psv_state in User_struct */
    ST_DISCONNECTED,		/* state disconnected */
    ST_CONNECT_PENDING,		/* state connect pending */
    ST_AUTHENTICATION,		/* state authentication */
    ST_INITIAL_PROCEDURE,	/* state initial procedure */
    ST_ROUTINE,			/* state routine procedure */
    ST_DISCONNECTING,		/* (5) state disconnection pending */
    ST_TERMINATING,		/* state termination pending */
    ST_TERMINATED,		/* state termination completed */
};

typedef struct {		/* user data structure */
    char line_ind;		/* line index */
    char cm_ind;		/* comm manager index */
				/* line_ind and cm_ind can not change 
				   while this p_server version is running */

    char line_type;		/* line type DEDICATED, DIAL_IN, DIAL_OUT. This
				   field determines how the user profile is 
				   found (using line_ind for DEDICATED and user
				   supplied uid for others). */
				/* line_type can be updated on the fly. The 
				   update becomes effect when a new user is 
				   connected */
    char first_psv;		/* this is the first p_server instance */

    Pd_user_entry *up;		/* user profile */

    char psv_state;		/* the current p_server state: 
				   ST_DISCONNECTED ... */
    char next_state;		/* continue to process the next state; yes 
				   (non-zero) or no (zero) */
    char link_state;		/* link connection state: LINK_ENABLED or
				   LINK_DISABLED */

    char discon_reason;		/* disconnect reason as defined in 
				   prod_status.h */
    char user_msg_block;	/* block the user message reading */
    unsigned char bad_msg_cnt;	/* number of consecutive bad user messages */

    char msg_queued;		/* new message queued */
    char status_msg_cnt;	/* status message count for suppressing extra
				   number of status messages */

    time_t session_time;	/* time of the connection established */
    time_t time;		/* current time */

    int cur_event;		/* used for storing the current event when the 
				   event processing has to be suspended. */
    char *ev_data;		/* data associated with cur_event */

    int status_received;	/* status message reported from comm_manager */

    /* adaptation info */
    Pd_distri_info *info;	/* prod distri info table currently in use */
    Pd_line_entry *line_tbl;	/* entry of the line table for this link */

    /* module level data structures */
    void *rrs;			/* local data structure of RRS */
    void *sus;			/* local data structure of SUS */
    void *wan;			/* local data structure of WAN */
    void *hwq;			/* local data structure of HWQ */
    void *pso;			/* local data structure of PSO */
    void *mt;			/* local data structure of MT */
    void *psai;			/* local data structure of PSAI */
    void *psr;			/* local data structure of PSR */
    void *hp;			/* local data structure of HP */
} User_struct;


enum {				/* Values used for the priority argument of 
				   function WAN_write; Specifying WAN  
				   write circuit; Must be consistent with
				   requirements defined in comm_manager.h */
    HIGH_PRIORITY = 1,		/* writing to the high priority circuit */
    NORMAL_PRIORITY		/* writing to the normal priority circuit */
};


/* Argument "type" of HWQ_put_in_queue () is an OR of the following values */
#define HWQ_TYPE_INDIRECT 0x1	/* the user message pointed by "data" can be 
				   in one of the following two formats: 
				   direct - "data" points to the user message; 
				   TYPE_INDIRECT - "data" points to the user 
				   message header block followed by the 
				   Indirect_rpg_msg structure */
#define HWQ_TYPE_ALERT	0x2	/* "data" is an alert product */
#define HWQ_TYPE_ONETIME 0x4	/* "data" is a one-time requested product */
#define HWQ_TYPE_HIGH_PRIO 0x8	/* product in "data" was requested with high 
				   priority */
#define HWQ_TYPE_STATIC 0x10	/* pointer "data" is static and can not be 
				   freed */
#define HWQ_TYPE_CMHD_FREE 0x20
				/* pointer "data" needs to be freed by "free".
				   "data" follows the comm_manager header of 
				   sizeof (CM_req_struct) */
#define HWQ_TYPE_ALERT_PRIO 0x40 /*"data" is an alert paired product */


typedef struct {		/* data struct used for representing a product
				   distribution message that is partially stored
				   in an LB message (HWQ_TYPE_INDIRECT type) */
    int orpgdat;		/* ORPG data type number */
    int msg_id;			/* message ID as stored in the ORPG data */
    unsigned int vol_number;	/* product volume number */
    int seq_number;		/* request sequence number */
    int elev_angle;		/* in .1 degrees; -10 - 450 */
    time_t req_time;              /* requested time for OTRs */
} Indirect_rpg_msg;

#define THIS_IS_PROD_ID	0x80000000
				/* flag used for indicating that it is the
				   prod_id instead of the prod code. This is 
				   used for the msg_code argument of 
				   GUM_rr_message */
				
typedef struct {		/* struct passing data for processing RPG
				   text messages */
    unsigned short ftmd_line_spec[FTMD_LINE_SPEC_SIZE];
				/* destination line spec */
    unsigned short ftmd_type_spec;	/* destination type spec */
    int data_type;		/* data store for 75 */
    int msgid;			/* msg id of current product 75 */
    char *hd;			/* product headers */
} Send_text_msg_t;

/* function return values */
#define PSV_FAILED -1
#define PSV_PRODUCT_LOST -2

/* public functions */
int PE_initialize (int n_users, User_struct **users);
void PE_process_events (int ev, int user_ind, char *ev_data);
void PE_handle_link_enable_change (User_struct *usr);
void PE_process_link_enable_change ();

int GE_initialize (int n_users, User_struct **users, int p_server_index);
void GE_main_loop ();

int RPI_initialize (int p_server_ind, User_struct **users);
void RPI_update_config_on_disconnect (User_struct *usr);
int RPI_read_user_table (User_struct *usr, int uid);
void RPI_print_user (User_struct *usr);
void RPI_get_empty_up (User_struct *usr);
int RPI_read_pd_info ();

int RRS_initialize (int n_users, User_struct **users);
void RRS_init_read ();
void RRS_set_gsm ();
void RRS_update_rpg_state ();
void RRS_update_rda_status ();
void RRS_update_rpg_status ();
void RRS_update_vol_status (void *avset_ss);
unsigned int RRS_get_volume_number (time_t *clock, time_t *vtime);
int RRS_get_volume_duration ();
char *RRS_gsm_message (User_struct *usr);
void RRS_send_gms ();
void RRS_new_user (User_struct *usr);
int RRS_get_RDA_op_status ();
int RRS_rpg_test_mode ();
int RRS_rpg_inactive ();
void RRS_update_time( time_t t );

int WAN_initialize (int n_users, User_struct **users);
void WAN_init_contact ();
void WAN_connect (User_struct *usr);
void WAN_disconnect (User_struct *usr);
int WAN_read_next_msg (User_struct *usr, char **data);
int WAN_write (User_struct *usr, int priority, char *buf, int length);
void WAN_clear_responses (User_struct *usr);
char *WAN_usr_msg_malloc (int size);
void WAN_free_usr_msg (char *msg);
int WAN_circuit_ready (User_struct *usr, int priority);
int WAN_send_set_params_request (User_struct *usr, 
				Pd_line_entry *line_tbl, int rw_time);
void WAN_change_statistics_report_period (int period);

int PSO_initialize (int n_users, User_struct **users);
void PSO_disconnected (int ev, User_struct *usr, void *ev_data);
void PSO_connect_pending (int ev, User_struct *usr, void *ev_data);
void PSO_disconnecting_etc (int ev, User_struct *usr, void *ev_data);
void PSO_disable (int ev, User_struct *usr, void *ev_data);
void PSO_shutdown (int ev, User_struct *usr, void *ev_data);
void PSO_terminated (int ev, User_struct *usr, void *ev_data);

int PSAI_initialize (int n_users, User_struct **users);
void PSAI_new_user (User_struct *usr);
void PSAI_authentication (int ev, User_struct *usr, void *ev_data);
void PSAI_initial_procedure (int ev, User_struct *usr, void *ev_data);
int PSAI_max_connect_time (User_struct *usr);
void PSAI_process_max_connect_time_disable_request_msg (
					User_struct *usr, char *msg);

int SUS_initialize (int n_users, User_struct **users);
void SUS_new_user (User_struct *usr);
void SUS_link_changed (User_struct *usr, int new_stat);
void SUS_enable_changed (User_struct *usr, int new_stat);
void SUS_loadshed_changed (User_struct *usr, int new_stat, int util);
void SUS_stat_report (User_struct *usr, char *rep, int rep_len);
void SUS_user_id (User_struct *usr, int uid);
void SUS_line_stat_changed (User_struct *usr, int new_stat);
void SUS_process_timer (int ev, User_struct *usr);
void SUS_report_max_nb_util (User_struct *usr);

int HWQ_initialize (int n_users, User_struct **users);
void HWQ_empty_queue (User_struct *usr);
int HWQ_wan_write (User_struct *usr);
int HWQ_put_in_queue (User_struct *usr, int type, char *data);
int HWQ_get_wq_size (User_struct *usr);
void HWQ_comm_load_shed (User_struct *usr);
void HWQ_shed_all (User_struct *usr);
void HWQ_set_vv_mode (int vv_mode);

char *GUM_rr_message (User_struct *usr, int error_bit, int msg_code, 
				int seq_number, int elev_angle);
void GUM_date_time (time_t cr_time, short *date, int *time);

int MT_initialize (int n_users, User_struct **users);
void MT_set_timer (User_struct *usr, int timer, int period);
void MT_cancel_timer (User_struct *usr, int timer);
void MT_cancel_all_timers (User_struct *usr);
int MT_get_timer_event (int *user_ind);
int MT_read_timer (User_struct *usr, int timer);

int PSR_initialize (int n_users, User_struct **users);
void PSR_new_user (User_struct *usr);
void PSR_routine (int ev, User_struct *usr, void *ev_data);
void PSR_session_expiration (User_struct *usr);
void PSR_enter_test_mode (User_struct *usr);
void PSR_enter_inactive_mode (User_struct *usr);
int PSR_status_log_code (User_struct *usr);
void PSR_process_status_supression (User_struct *usr);

int HP_initialize (int n_users, User_struct **users, int p_server_index);
void HP_process_prod_request (User_struct *usr, char *umsg);
void HP_init_prod_table (User_struct *usr);
void HP_clear_prod_table (User_struct *usr);
void HP_handle_routine_products ();
void HP_handle_one_time_products ();
int HP_read_prod_gen_status (Prod_gen_status **stat, 
				Prod_gen_status_header **pgs_hd);
int HP_prod_permission_ok (User_struct *usr, int pid, 
					int check_type, int*wx_modes);
int HP_routine_requested (User_struct *usr, int prod_id);
int HP_send_a_product (User_struct *usr, int prod_id, int lb_id, 
		int msg_id, unsigned int vnum, int seq_number, int prio, 
			int elev, int dest_id, time_t one_time);
void HP_set_to_use_db_read (Prod_header *phd, Indirect_rpg_msg *indirect);
char *HP_print_parameters (short *params, int prod_id);
void HP_prod_status_updated ();

int PWA_initialize (int n_users, User_struct **users, int p_server_index);
void PWA_alert_adapt_update ();
void PWA_process_alert ();
void PWA_process_user_alert_msg ();
char *PWA_alert_adaptation_parameter_msg (User_struct *usr);
void PWA_alert_request (User_struct *usr, char *umsg);
void PWA_line_disconnected (User_struct *usr);

#endif		/* #ifndef PSV_DEF_H */
