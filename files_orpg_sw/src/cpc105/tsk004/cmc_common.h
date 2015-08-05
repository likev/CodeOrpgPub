
/***********************************************************************

    Description: comm_manager common function library public header file.

***********************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/02/14 21:43:37 $
 * $Id: cmc_common.h,v 1.21 2007/02/14 21:43:37 jing Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */  

#ifndef CMC_COMMON_H

#define CMC_COMMON_H

#define RSS_LB_REPLACE_ONLY
#include <rss_replace.h>

#ifndef NO_ORPG
#include <orpgerr.h>
#else
#define GL_ERROR 0
#define GL_INFO 0
#endif
#include <infr.h>

#ifdef SIMPACT
    #include <cms_def.h>
    #define CMC_verbose CMS_verbose
#endif
#ifdef UCONX
    #define CMC_verbose CMU_verbose
    #include <cmu_def.h>
#endif
#ifdef CM_TCP
    #define CMC_verbose CMT_verbose
    #include <cmt_def.h>
#endif

#ifdef CM_CISCO
    #define CMCS_verbose
    #include <cmcs_def.h>
#endif

#ifdef CM_MC
    #include <cmm_def.h>
#endif

#define NAME_LEN 128		/* length of name strings */


enum {				/* processing state ("state" in Req_struct or
				   Resp_struct) */
    CM_NEW,			/* new and unprocessed */
    CM_NEW_PACKED,		/* new packed data and unprocessed */
    CM_DONE			/* processing finished and response sent */
};

struct req_struct {		/* internal request message struct */
    int state;			/* CM_NEW, CM_DONE or start time */
    char *data;			/* CM_WRITE and CM_SET_PARAMS; pointer to the 
				   buffer holding the original message 
				   including header CM_req_struct and the data.
				   */
    LB_id_t msg_id;		/* LB msg id of this request */

    /* the following is identical to the CM_req_struct */
    int type;			/* request type; for CM_WRITE type, the data 
				   will following this structure */
    int req_num;		/* request number; unique for the link */
    int link_ind;		/* link index */

    int time_out;		/* time out value in seconds for this request;
				   0 means no time out specified */
    int parm;			/* request parameter; 
				   CM_WRITE: write priority (1 - 3) with
					     3 meaning the lowest priority;
				   CM_CANCEL: request number to cancel; -1 
					      means all previous requests */
    int data_size;		/* CM_WRITE only; data size */
};
typedef struct req_struct Req_struct;


#ifndef DEDICATED_DEFINED_ELSEWHERE
enum {				/* values for link types */
    DEDICATED,			/* dedicated */
    DIAL_IN,			/* dial-in */
    DIAL_IN_OUT			/* dial-in/out */
};
#endif

enum {				/* values for link_state */
    LINK_DISCONNECTED,		/* disconnected */
    LINK_CONNECTED,		/* all PVCs are BOUND and the link is ready */
    LINK_CONNECTING		/* connect pending state */
};

enum {				/* values for conn_activity ,dial activity*/
    NO_ACTIVITY,		/* no connect/disconnect request is being 
				   processed */
    CONNECTING,			/* a connect request is being processed */
    DISCONNECTING,		/* a disconnect request, orginated by local client 
				   is being processed */
};

enum {				/* values for connect_state, enable_state, 
				   bind_state and attach_state */
    ENABLING,			/* in the process of enabling the state */
    ENABLED,			/* enabled state */
    DISABLING,			/* in the process of disabling the state */
    DISABLED,			/* disabled or failed state  */
    STOP_ENABLING		/* this field will be removed */
};

enum {				/* protocol type ("proto") */
    PROTO_PVC,			/* PVC */
    PROTO_HDLC			/* HDLC */
};

enum {				/* values for dial state */
    NORMAL,			/* means that the link will be
				   treated like a dedicated line */
    HDLC_DIAL_CONNECT,		
    WAITING_FOR_MODEM_RESPONSE,
    RECEIVED_MODEM_RESPONSE,
    HDLC_DIAL_DISCONNECT,
    X25_DIAL_CONFIG,		
    X25_DIAL_CONFIG_PENDING,		
    X25_DIAL_CONNECT,	
    PROCESS_EXCEPTION,
    X25_DIAL_CONNECT_PENDING,	
    X25_DIAL_DISCONNECT,
    HDLC_DIAL_CONFIG,	        /* configure the link as hdlc */
    HDLC_DIAL_CONFIG_EXCEPTION, /* configure the link as hdlc due to exception*/
    HDLC_DIAL_CONFIG_PENDING   
};

/* special return values of CMC_read_link_config () */
#define CMC_CONF_READ_FAILED (struct link_struct *)0
#define CMC_CONF_READ_DONE (struct link_struct *)(-1)

/* public function provided by the CMC modules */
struct link_struct *CMC_read_link_config (char *cs_proto_name);
char *CMC_get_link_conf_name ();
int CMC_get_new_instance ();
int CMC_get_comm_manager_index ();
char *CMC_get_proto_name ();
void CMC_start_up (int argc, char **argv, int *verbose);
int CMC_initialize (int n_links, struct link_struct **links);
void CMC_register_termination_signals (void (*term_func)());
void CMC_house_keeping (int n_links, struct link_struct **Links);
int CMC_align_bytes (int n_bytes);
int CMC_reallocate_input_buffer (struct link_struct *link, int pvc, int size);
void CMC_terminate ();
int CMC_get_en_flag ();
void CMC_start_new_instance (int wp);
void CMC_need_delayed_free ();
void CMC_free (char *pt);
time_t CMC_term_start_time ();
int CMC_allow_queued_payload ();

int CMPR_process_requests (int nlinks, struct link_struct **links);
void CMPR_process_cancel (struct link_struct *link, Req_struct *req);
void CMPR_send_response (struct link_struct *link, Req_struct *req, 
							int ret_value);
void CMPR_send_data (struct link_struct *link, int pvc, int len, char *data);
void CMPR_send_event_response (struct link_struct *link, int ret_code, char *buf);
void CMPR_connect_failure (struct link_struct *link);
void CMPR_dialout_failure (struct link_struct *link,int ret_value);
void CMPR_cleanup (struct link_struct *link);
void CMPR_send_compressed_data (struct link_struct *link, int pvc);
int CMPR_comp_hd_size ();

int CMRR_get_requests (int n_links, struct link_struct **links);

void CMRATE_reset (struct link_struct *link);
void CMRATE_start_write (struct link_struct *link, int bytes);
void CMRATE_write_done (struct link_struct *link);
int CMRATE_get_rate (struct link_struct *link);
int CMRATE_init (struct link_struct *link);

int CMCT_initialize (int keepalive_time);
void CMCT_check_connect ();
int CMCT_register_rhost (int addr, void (*callback)());

int CMMON_register_address (int addr);
int CMMON_get_connect_status (int addr);
void CMMON_update (time_t cr_time);
void CMMON_cm_ping_lb_dir (char *dir);
void CMMON_restart_request (char *cmd, int wp);


/* The following functions are called by the CMC functions */
char *CM_additional_flags ();
int CM_parse_additional_args (int c, char *optarg);
void CM_print_additional_usage ();

void HA_write_data (struct link_struct *link, int pvc);
void HA_connection_procedure (struct link_struct *link);

void XP_connection_procedure (struct link_struct *link);
void XP_write_data (struct link_struct *link, int pvc);
void XP_statistics_request (struct link_struct *link);
void XP_statistics_reset (struct link_struct *link);

void SH_house_keeping (struct link_struct *link, time_t cr_time);

#endif		/* #ifndef CMC_COMMON_H */

