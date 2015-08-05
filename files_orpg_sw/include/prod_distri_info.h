/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/06/13 16:02:11 $
 * $Id: prod_distri_info.h,v 1.110 2007/06/13 16:02:11 steves Exp $
 * $Revision: 1.110 $
 * $State: Exp $
 */  
    
/***********************************************************************

    Description: header file defining data structures for product 
		distribution adaptation data.

***********************************************************************/


#ifndef PROD_DISTRI_INFO_H

#define PROD_DISTRI_INFO_H


#include <infr.h>
#include <prod_user_msg.h>


#define MAX_N_WX_MODES	3	/* max number of weather modes */

#define PD_LINE_INFO_MSG_ID	1
				/* message ID for the prod distri line table */
#define PD_USER_INFO_MSG_ID	2
				/* not used */
#define PD_SPARE_MSG_ID		3
				/* message ID for the prod attribute table */
#define PD_CURRENT_PROD_MSG_ID	4
				/* message ID for the current prod generation 
				   list */
#define PD_DEFAULT_A_PROD_MSG_ID	5
				/* message ID for the default weather mode A
				   product generation list */
#define PD_DEFAULT_B_PROD_MSG_ID	6
				/* message ID for the default weather mode B
				   product generation list */
#define PD_DEFAULT_M_PROD_MSG_ID	7
				/* message ID for the default maintenance mode
				   product generation list */

#define MAX_N_PROD_INFO_MSGS	7
				/* max number of messages in Prod_Distri_Info
				   store */

/* The following values were in "user_profiles". */
#define NB_RETRIES		2
#define NB_TIMEOUT		120
#define NB_CONNECT_TIME_LIMIT	1440

#define LINE_TBL_SIZE	50	/* maximum size of the line table */
#define USER_TBL_SIZE	2000	/* maximum size of the user table */

#define	USER_NAME_LEN		32
				/* max length of the user name string	*/
#define	PASSWORD_LEN		8
				/* max length of the password string	*/


enum {CLASS_I, CLASS_II, CLASS_III, CLASS_IV, CLASS_V_RGDAC, CLASS_V_RFC};
enum {DEDICATED, DIAL_IN, DIAL_OUT, WAN_LINE};
enum {CLASS_ALL=0, CLASS_DEDICATED, CLASS_DIAL};

enum {				/* values for link_state */
    LINK_ENABLED,		/* link enabled */
    LINK_DISABLED		/* link disabled */
};

enum {PROTO_X25, PROTO_TCP};	/* values for Pd_line_entry.protocol */

enum {				/* values for command in Pd_distri_cmd */
    CMD_NO_CMD,			/* no command */
    CMD_SHUTDOWN,		/* shut down product distribution */
    CMD_LINK_STATE_CHANGED,	/* link enable/disable state changed - 
				   as specified in Pd_line_entry.link_state -
				   no longer used and will be removed */
    CMD_REQUEST_APUP_STATUS,	/* (5) request for APUP status for lines */
    CMD_SWITCH_TO_INACTIVE,	/* switch to inactive mode */
    CMD_SWITCH_TO_ACTIVE,	/* switch to active mode */

    CMD_DISCONNECT,		/* disconnect lines */
    CMD_CONNECT,		/* connect lines */
    CMD_VV_ON,			/* turn on V&V */
    CMD_VV_OFF			/* turn off V&V */
};

typedef short prod_id_t;	/* data type used for product id */

typedef struct {		/* prod distri command message */
    int command;		/* command */
    int n_lines;		/* number of lines specified */
    int line_ind[1];		/* SMI_vss_field[this->n_lines] */
				/* line index list */
} Pd_distri_cmd;
		
#define MAX_NUM_RCM_GEN_TIMES	2
				/* max number of RCM generation times */
typedef struct {		/* basic prod distri info */
    int nb_retries;		/* Number of connection retries, 1 - 999 */
    int nb_timeout;		/* max msg transmissions time: 60 -999 
				   seconds */
    int connect_time_limit;	/* Maximum connect time allowed when  
				   disconnect override is invoked; in 
				   minutes; */

    int n_lines;		/* size of the line list (number of 
				   Pd_line_entry structures that follow) */
    int line_list;		/* offset, in number of bytes from the   
				   beginning of this structure, of the line
				   list (an array of Pd_line_entry). */
} Pd_distri_info;

typedef struct {		/* prod distri adaptation: line based */
    char line_ind;		/* line index; < 0 means unused record */
    char cm_ind;		/* comm manager index */
    char p_server_ind;		/* p_server index */
    char line_type;		/* line type: DEDICATED ... */

    char link_state;		/* LINK_ENABLED or LINK_DISABLED */
    char protocol;		/* PROTO_X25, PROTO_TCP */
    char n_pvcs;		/* number of PVCs */
    char not_used;		/* not used */
    int packet_size;		/* packet size */
    int baud_rate;		/* norminal rate: bits per second */
    int conn_time_limit;	/* dial-in user connection time limit in 
				   minutes */
    char port_password[PASSWORD_LEN];
				/* port password: at least 4 characters */
} Pd_line_entry;

/*
 * NOTE: "_PMS_" refers to Product Distribution Permission Table
 */
#define PD_PMS_ROUTINE 0x1	/* bit set for routine distri control used in 
				   Pd_pms_entry.types */
#define PD_PMS_ONETIME 0x2	/* bit set for onetime distri control used in 
				   Pd_pms_entry.types */

#define PD_PMS_NONE -2		/* special value for Pd_pms_entry.prod_id: 
				   non of the products */
#define PD_PMS_ALL -1		/* special value for Pd_pms_entry.prod_id: 
				   all of the products */

/*
 * Product Distribution Permission Table Entry
 */
typedef struct {
    prod_id_t prod_id;		/* product buffer number or PD_PMS_NONE or
				   PD_PMS_ALL */
    char wx_modes;		/* applicable weather modes: setting bit n 
				   (0, 1, 2, ... ; 0 is LSB) indicates that 
				   this entry is applied to weather mode n */ 
    char types;			/* applicable distri type bits: PD_PMS_ROUTINE 
				   and PD_PMS_ONETIME */
} Pd_pms_entry;

typedef struct {
    prod_id_t prod_id;		/* product buffer number */
    char wx_modes;		/* applicable weather modes: setting bit n 
				   (0, 1, 2, ... ; 0 is LSB) indicates that 
				   this entry is applied to weather mode n */ 
    char period;		/* distribution period in number of volumes */
    char number;		/* total number of products distributed. <= 0 
				   means infinity */
    char map_requested;		/* yes/no. maps are distributed with the 
				   product */
    char priority;		/* 1 (high) / 0 (low); distribution priority */
    short params[NUM_PROD_DEPENDENT_PARAMS];
				/* product dependent parameters. Refer to 
				   section "NOTES on product parameters" in 
				   prod_user_msg.h */
} Pd_prod_item;

enum {UP_LINE_USER, UP_DIAL_USER, UP_DEDICATED_USER_OLD, UP_CLASS};
				/* values for Pd_user_entry.up_type */
#define UP_DEDICATED_USER UP_DIAL_USER

/* The following bit mask is for both Pd_user_entry.cntl and 
   Pd_user_entry.defined */
#define UP_CD_OVERRIDE (1<<0)	/* Max connect time override privilege */
#define UP_CD_APUP_STATUS (1<<1)
				/* APUP status can be requested */
#define UP_CD_RPGOP (1<<2)	/* RPGOP previlege granted */
#define UP_CD_ALERTS (1<<3)	/* alert reception; This will also enable 
				   reception of alert adaptation parameter 
				   messages and alert request processing.*/
#define UP_CD_COMM_LOAD_SHED (1<<4)
				/* comm load shed enabled */
#define UP_CD_STATUS (1<<5)	/* general status msg reception enabled */
#define UP_CD_MAPS (1<<6)	/* map reception enabled */
#define UP_CD_PROD_GEN_LIST (1<<7)
				/* product genaration list reception enabled */
#define UP_CD_PROD_DISTRI_LIST (1<<8)
				/* product distribution list reception 
				   enabled */
#define UP_CD_RCM (1<<9)	/* RCM reception enabled */
#define UP_CD_DAC (1<<10)	/* disconnects after distribution completion */
#define UP_CD_MULTI_SRC (1<<11)
				/* allows messages with different source IDs 
				   on a dedicated line. */
#define UP_CD_AAPM (1<<12)	/* The user receives a Alert Adaptation 
				   Parameters message upon connection if the 
				   user does not use alerts */
#define UP_CD_IMM_DISCON (1<<13)
				/* Lines are disconnected immediately instead 
				   of waiting for completion of current 
				   product. */
#define UP_CD_NO_SCHEDULE (1<<14)
				/* the users requests do not cause product
				   generation. The user gets what's 
				   available. */
#define UP_CD_FREE_TEXTS (1<<15)/* free text message sending enabled */
#define UP_CD_RESTRICTED_RCM (1<<16)	/* restricted RCM reception enabled */

/* The following additional bit mask is for Pd_user_entry.defined */
#define UP_DEFINED_DISTRI_METHOD (1<<24)/* distri_method defined */ 
#define UP_DEFINED_WAIT_TIME_FOR_RPS (1<<25)/* wait_time_for_rps defined */ 
#define UP_DEFINED_CLASS (1<<26)/* class defined */ 
#define UP_DEFINED_PMS (1<<27)	/* permission list defined */ 
#define UP_DEFINED_DD (1<<28)	/* default distribution list defined */ 
#define UP_DEFINED_MAP (1<<29)	/* map list defined */ 
#define UP_DEFINED_MAX_CONNECT_TIME (1<<30)
				/* max_connect_time defined */ 
#define UP_DEFINED_N_REQ_PRODS ((unsigned int)1<<31)
				/* n_req_prods defined */ 

typedef struct {		/* the user profile */
    short entry_size;		/* size of this table entry; < 0 means unused 
				   record; SMI_vss_size this->entry_size; */
    short user_id;		/* 1 to 9999 */
    short pms_len;		/* list length of the product distribution 
				   permission list. 0 is assumed if not 
				   defined. */
    short dd_len;		/* length of the default distribution list. 0 
				   is assumed if not defined. */
    short map_len;		/* length of the map list. 0 is assumed if not 
				   defined. */
				/* zero length indicates that a list does not 
				   exist */
    short pms_list;		/* offset of the product distribution 
				   permission list */
    short dd_list;		/* offset of the default distribution list */
    short map_list;		/* offset of the map list */
				/* All above offsets are in number of bytes and 
				   started from the beginning of this entry */

    short max_connect_time;	/* Maximum connect time: 1 - 1440 minutes.
				   1440 is assumed if not defined. */

    short n_req_prods;		/* max number of routine products requested.
				   31 is assumed if not defined. */

    short wait_time_for_rps;	/* rps waiting time before disconnecting the 
				   user. 0 is assumed if not defined. */
    short distri_method;	/* distribution method code that will be sent
				   to the user in the GSM. 0 is assumed if not
				   defined. */

    char up_type;		/* user profile type: UP_DIAL_USER, 
				   UP_DEDICATED_USER, UP_CLASS */
    char line_ind;		/* line index; UP_DEDICATED_USER 
				   class use:  UP_CLASS: this field determines
				   which types of users the class can be
				   associated with (from CLASS_DIAL,
				   CLASS_DEDICATED, CLASS_ALL (default)) */
#ifdef __cplusplus
    char user_class;			/* For UP_CLASS, this specifies its class index;
				   For UP_DIAL_USER and UP_DEDICATED_USER this
				   specifies which UP_CLASS to incorporate. */
#else				   
    char class;			/* For UP_CLASS, this specifies its class index;
				   For UP_DIAL_USER and UP_DEDICATED_USER this
				   specifies which UP_CLASS to incorporate. */
#endif

    unsigned int cntl;		/* product distribution control bits */
    unsigned int defined;	/* Pd_user_entry field definition bits */

    char user_password[PASSWORD_LEN];
				/* user password: at least 6 characters */
    char user_name[USER_NAME_LEN];
				/* User name; 7 characters or less; */
    /* SMI_vss_field Pd_pms_entry pms[this->pms_len] (this->pms_list) */
    /* SMI_vss_field Pd_prod_item dd[this->dd_len] (this->dd_list) */
} Pd_user_entry;

typedef struct {		/* Default product generation table entry */
    prod_id_t prod_id;		/* product buffer number; < 0 means 
				   unused record */
    char wx_modes;		/* bit fields indicating this entry is 
				   applicable for certain weather modes: bit  
				   0 for weather mode 0 and so on. */
    char gen_pr;		/* generation period in number of volumes;
				   0 indicates that the product is available 
				   for one_time requests */
    char arch_pr;		/* archive period in number of volumes */
    char stor_pr;		/* storage period in number of volumes */
    int stor_retention;		/* storage retention time in number of 
				   minutes */
    short req_num;              /* a unique request sequence number. 
				   By default req_num is 0. */
    short params[NUM_PROD_DEPENDENT_PARAMS];
				/* product dependent parameters. Refer to 
				   section "NOTES on product parameters" in 
				   prod_user_msg.h */
} Pd_prod_entry;		/* all above periods can be 0 which means no 
				   generation or no distribution */

/* Product distribution and generation info is stored in data store 
  "Prod_Gd_Info" implemented by a replaceable LB named ORPGDAT_PROD_INFO.

   Basic product distribution info including line configuration is stored with 
   message PD_LINE_INFO_MSG_ID. In each message, the first part is a 
   Pd_distri_info structure. A prod distri line table (an array of 
   Pd_line_entry) then follows. Note that the table must start at a aligned 
   address.

   Default product generation information, a list of Pd_prod_entry, is   
   stored in message PD_DEFAULT_PROD_MSG_ID.

   Genaral product information is stored in message PD_PROD_ATTR_MSG_ID. This  
   message is a list of entries which start with the Pd_attr_entry   
   structure followed by a map (prod_id_t) list and a dependent product, 
   (dep_prods_list, prod_id_t) list. Note that the lists must start at a 
   aligned address.

   Product distribution commands are sent through event ORPGEVT_PD_LINE. The
   event message is Pd_distri_cmd.

   Notes: Event ORPGEVT_PD_LINE was also used when PD_LINE_INFO_MSG_ID message
   is updated. Command CMD_LINK_STATE_CHANGED indicated that the new link_state
   in PD_LINE_INFO_MSG_ID message should used. Thus one can change the
   link_state for multiple links with one command. For a PD_LINE_INFO_MSG_ID
   message update without a command one must set "command" to CMD_NO_CMD.
   This has been changed. In order to change link enable state, one needs only
   to update the PD_LINE_INFO_MSG_ID message. No event is needed for this 
   purpose. Command CMD_LINK_STATE_CHANGED is not used any longer. Event 
   ORPGEVT_PD_LINE is now used solely for sending p_server commands without 
   need of updating message PD_LINE_INFO_MSG_ID. The event macro 
   ORPGEVT_PD_LINE should also be changed to be something like ORPGEVT_PSV_CMD.
   This macro change, however, has not been done yet.

*/


enum {				/* values for 
				   One_time_prod_req_response.error */
    OTR_PRODUCT_READY = 0,	/* a product is ready */
    OTR_CPU_LOAD_SHED,          /* product unavailable due to cpu loadshedding */
    OTR_TASK_NOT_RUNNING,       /* product unavailable because algorithm is not running */
    OTR_NEXT_VOLUME,            /* product not available now, scheduled for next volume */
    OTR_PRODUCT_NOT_AVAILABLE,  /* product not available in the database */
    OTR_MEM_LOADSHED,		/* product not available due to memory loadshed */
    OTR_SLOT_FULL,		/* not enough slots avaiable to generate product */
    OTR_INVALID_PARAMS,		/* unable to generate product owing to invalid product parameters */
    OTR_TASK_NOT_STARTED,       /* task that generates requested product never started */
    OTR_TASK_FAILED,            /* task that generates requested product failed */
    OTR_TASK_SELF_TERM,         /* task that generates requested product self terminated */
    OTR_DISABLED_MOMENT,        /* radar data (reflectivity, velocity, and/or spectrum width) is not available for the product */
    OTR_VOLUME_ABORTED,         /* Product not generated owing to volume scan abort */ 
    OTR_DATA_SEQ_ERROR,         /* Product not generated owing to an input data sequence error */ 
    OTR_PRODUCT_NOT_GEN		/* Product not generated .... possibly owing to data not available. */
};

enum {				/* values for 
				   One_time_prod_req_response.priority */
    OTR_LOW_PRIORITY,           /* request did not have priority bit set */
    OTR_HIGH_PRIORITY,          /* request had priority bit set */
    OTR_ALERT_PRIORITY          /* alert paired product */
};

typedef struct {		/* response message for one-time prod req */
    prod_id_t prod_id;		/* buffer number of the product */
    short seq_number;		/* request sequence number in the req msg */
    short line_ind;		/* user line index in the req msg */
    short elev;			/* elevation angle in .1 degrees */
    short src_id;		/* source id in the msg header block */
    short dest_id;		/* destination id in the msg header block */
    char error;			/* error number */
    char priority;		/* product request priority; 1(high)/0(low) */
    char map;			/* map needed; 1(yes)/0(no) */
    char last;			/* flag indicating that this is the last 
				   product for this request; yes(1)/no(0) */
    time_t req_time;		/* request time in the req msg */
    unsigned int vol_number;	/* the volume number */
    int lb_id;			/* LB name number that stores the product; 
				   valid only if error == OTR_PRODUCT_READY */
    int msg_id;			/* LB msg ID of the product; valid only if 
				   error == OTR_PRODUCT_READY */
} One_time_prod_req_response;


/* the following section is for product distribution V&V */
enum {				/* values for Vv_record_t.type */
    VV_UNUSED,			/* unused record */
    VV_PRODUCT,			/* a product */
    VV_ALERT,			/* an alert message */
    VV_RR			/* a Request Response message */
};

enum {				/* values for Vv_record_t.status */
    VV_SUCCESS,			/* the message has been successfully sent */
    VV_LOST,			/* product lost */
    VV_LOADSHED,		/* the message has been LOADSHED */
    VV_DISCARDED		/* the message has been discarded due to
				   line disconnection. */
};

typedef struct {		/* product v&v data */
    int id;			/* product ID */
    short params[NUM_PROD_DEPENDENT_PARAMS];
				/* product dependent parameters */
} Vv_record_product_t;

typedef struct {		/* alert v&v data */
    short code;			/* message code */
    short area_number;		/* alert area number */
    int category;		/* alert category */
    int status;			/* alert stutas */
    time_t t_det;		/* time of detection */
} Vv_record_alert_t;

typedef struct {		/* RR v&v data */
    int code;			/* message code */
    int prod_code;		/* product code */
    int elev;			/* elevation angle */
    int er_code;		/* error code */
} Vv_record_rr_t;
				
typedef struct {		/* v&v data */
    short type;			/* message type */
    short status;		/* message status */
    short vol_num;		/* volume number */
    short line_ind;		/* NB line index */
    int seq_num;		/* sequence number */
    time_t t_gen;		/* time of generation */
    time_t t_req;		/* time of one time request */   
    time_t t_eq;		/* time of entering the queue */
    time_t t_dq;		/* time of removing from the queue (start 
				   transmission */
    time_t t_sent;		/* time of transmission completed */
    int priority;		/* transmission priority */
    int ls_priority;		/* loadshed priority */
    int size;			/* message size */

    union {			/* type specific part */
	Vv_record_product_t prod;
	Vv_record_alert_t alert;
	Vv_record_rr_t rr;
    } t;
} Vv_record_t;


#endif		/* #ifndef PROD_DISTRI_INFO_H */
