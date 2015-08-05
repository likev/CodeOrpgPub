/* 
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/10/06 18:42:33 $ 
 * $Id: prod_status.h,v 1.60 2008/10/06 18:42:33 cmn Exp $ 
 * $Revision: 1.60 $ 
 * $State: Exp $
 */  

/***********************************************************************

    Description: header file defining data structures for the product 
		generation and user status information.

***********************************************************************/




#ifndef PROD_STATUS_H

#define PROD_STATUS_H

#include <lb.h>
#include <prod_user_msg.h>
#include <prod_request.h>

#define MAX_PRODS_IN_STATUS_MAG	4096
				/* maximum number of products in product status 
				   message, used for msg verification only */

#define PROD_STATUS_MSG 1	/* LB message ID for the product generation
				   message */
#define PGS_LIST_LENGTH 8	/* array size of Prod_gen_status.msg_ids */

/* values used for msg_ids fields in Prod_gen_status */
#define PGS_GEN_OK           0xffffffff /* product generated correctly. */
#define PGS_UNKNOWN          0xfffffffe	/* no knowledge about this product for 
                                           this volume scan. */
#define PGS_SCHEDULED        0xfffffffd /* scheduled for generation */
#define PGS_NOT_SCHEDULED    0xfffffffc /* failed because the product is not 
				           scheduled for generation */
#define PGS_VOLUME_ABORTED   0xfffffffb /* failed because of volume scan abort 
                                           */
#define PGS_TASK_NOT_RUNNING 0xfffffffa	/* failed because a task is not runnning
                                           */
#define PGS_TASK_NOT_CONFIG  PGS_TASK_NOT_RUNNING
#define PGS_INAPPR_WX_MODE   0xfffffff9 /* failed because of the weather mode is 
				           inappropriate */
#define PGS_TIMED_OUT        0xfffffff8 /* failed because of timed-out or other 
				           unkown reasons */
#define PGS_REQED_NOT_SCHED  0xfffffff7 /* requested but not scheduled */

#define PGS_DISABLED_MOMENT  0xfffffff6 /* failed because of disabled moment */
				           
#define PGS_MEMORY_LOADSHED  0xfffffff5 /* failed because of unavailability of memory */
#define PGS_TASK_FAILED      0xfffffff4 /* execution time failure */
#define PGS_SLOT_UNAVAILABLE 0xfffffff3 /* unavailability of real-time, replay, or
                                           customized slots */
#define PGS_INVALID_PARAMS   0xfffffff2 /* invalid product parameters */
#define PGS_DATA_SEQ_ERROR   0xfffffff1 /* product not generated because of data
                                           sequence error */
#define PGS_TASK_SELF_TERM   0xfffffff0 /* task self-terminated */

#define PGS_PRODUCT_DISABLED 0xffffffef /* Product disabled. */
#define PGS_PRODUCT_NOT_GEN  0xffffffee /* Product Not Generated. */

/* schedule infomation: Prod_gen_status.schedule */
#define PGS_SCH_NOT_SCHEDULED  0	/* not scheduled for generation */
#define PGS_SCH_SCHEDULED    0x1	/* scheduled for generation */
#define PGS_SCH_BY_REQUEST   0x2	/* scheduled due to user requests */
#define PGS_SCH_BY_DEFAULT   0x4	/* scheduled by default */

/* elevation index information: Prod_gen_status.elev_index */
#define PGS_ELIND_ALL_ELEVATIONS   REQ_ALL_ELEVS 
#define PGS_ELIND_NOT_SCHEDULED    REQ_NOT_SCHEDLD

typedef struct {		/* header of real time product generation 
				   status message. The message contains this
				   header and an array of Prod_gen_status 
				   structures */
    int length;			/* number of Prod_gen_status structures 
				   following this header */
    int vdepth;			/* number of valid elements in array vtime */
    int list;			/* offset of the array of Prod_gen_status */
    unsigned int vnum[PGS_LIST_LENGTH];
				/* volume scan sequence number (monotonically 
				   increasing).  The first entry conresponding 
				   to the latest (the current) volume */
    time_t vtime[PGS_LIST_LENGTH];
				/* volume time table */
    short wx_mode[PGS_LIST_LENGTH];
				/* weather modes corresponding to the volumes 
				   in "vtime" */
    short vcpnum[PGS_LIST_LENGTH];
				/* VCP numbers corresponding to the volumes 
				   in "vtime" */
} Prod_gen_status_header;

typedef struct {		/* real time generation status of a product */
    short prod_id;		/* buffer number of the product */
    short params[NUM_PROD_DEPENDENT_PARAMS];
    short elev_index;           /* elevation index corresponding to elevation
                                   parameter, if defined.  Special flags include
                                   PGS_ELIND_ALL_ELEVATIONS and 
                                   PGS_ELIND_NOT_SCHEDULED */
    short spare;                /* currently provided for structure alignment. */
				/* product dependent parameters. Refer to 
				   section "NOTES on product parameters" in 
				   prod_user_msg.h */
    char gen_pr;		/* generation period in number of volumes;
				   0 indicates that the product is available 
				   for one-time requests */
    char schedule;		/* bit flags: PGS_SCH_SCHEDULED, 
				   PGS_SCH_BY_REQUEST ... */
    LB_id_t msg_ids[PGS_LIST_LENGTH];
				/* list of the latest product message IDs;
				   Each entry corresponds to a volume with 
				   the first entry conresponding to the latest
				   (the current) volume. If a product is not 
				   available, a special value (PGS_UNKNOWN... 
				   as defined above) is used to indicate the 
				   possible cause. */
} Prod_gen_status;

/* The routine product scheduler publishes the ORPG product generation   
   information in real time. The information is presented as a message stored 
   in LB ORPGDAT_PROD_STATUS. The LB is LB_REPLACEABLE and the message ID is 
   PROD_STATUS_MSG. The message contains a Prod_gen_status_header data 
   structure and a list of Prod_gen_status data structures. The list is in 
   non-decreasing prod_id order. */

enum {US_ENABLED = 1, US_DISABLED};
				/* values for Prod_user_status.enable */
enum {US_DISCONNECTED = 1, US_CONNECT_PENDING, US_CONNECTED};
				/* values for Prod_user_status.link -
				    link status */
enum {US_LOAD_NORMAL = 1, US_LOAD_SHED, US_LOAD_SHED_DISABLED};
				/* values for Prod_user_status.loadshed - 
				   loadshed status */
enum {US_LINE_NORMAL = 1, US_LINE_NOISY, 
			US_UNABLE_CONN_DEDIC, US_LINE_FAILED};
				/* values for Prod_user_status.line_stat - 
				   line status. See p_server manpage. */
enum {				/* values for Prod_user_status.discon_reason - 
				   disconnection reason */
    US_SOLICITED = 1, 		/* transaction finished */
    US_SESSION_EXPIRED, 	/* session time limit reached */
    US_IN_TEST_MODE, 		/* RPG enters or is in test mode */
    US_OTHER_SIDE_REQ, 		/* user originated disconnection */
    US_IO_ERROR, 		/* Fatal I/O error (fatal error detected in a 
				   lower comms layer, connection lost, 
				   comm_manager restarts, read/write timed-out,
				   connection trial failed due to an error). */
    US_BAD_ID, 			/* user profile not found */
    US_BAD_PASSWORD, 		/* incorrect port or user password */
    US_INVALID_MSGS, 		/* too many (3) consecutive invalid messages 
				   or invalid sign-on msg */
    US_NOT_RESPONDING, 		/* user is not responding for certain time (e.g 
				   sign-on timed-out) */
    US_SHUTDOWN,		/* The system is commanded to be shutdown */
    US_LINK_DISABLED,		/* The link is commanded to be disabled */
    US_CONNECT_TIMED_OUT	/* Connection can not be made before time-out */
};

typedef struct {		/* product user status structure. A value of 0 
				   indicates the field is not available. If a 
				   user has ID = 0, it will cause confusion. 
				   0 user ID should not be used. */

    char line_ind;		/* line index */
    char enable;		/* line enable status; 0 indicates this data 
				   structure does not contain valid values. */
    char link;			/* link status */
    char loadshed;		/* load shed status */
    char line_stat;		/* line status */
    char discon_reason;		/* disconnetion reason */
    short uid;			/* user id */
    short util;			/* line utilization (in percentage) */
    short uclass;		/* user class number (-1 if not available) */
    int rate;			/* achieved baud rate (Byte per second, -1 
				   indicates N/A) */
    int tnb_sent;		/* total number of bytes sent */
    int tnb_received;		/* total number of bytes received */
    time_t time;		/* reporting time */

} Prod_user_status;

#define US_STATUS_SIZE 8	/* size of the Prod_user_status array in each
				   product user status message */

/* The product user status for each line is stored in ORPGDAT_PROD_USER_STATUS
   as a message with ID = line index. The ORPGDAT_PROD_USER_STATUS must be of
   LB_REPLACEABLE type. When a message in ORPGDAT_PROD_USER_STATUS is updated,
   an event ORPGEVT_PROD_USER_STATUS is posted. The line index is specified in
   the event message (a character). Each product user status message is an array
   of Prod_user_status. The size of the array is US_STATUS_SIZE. The array 
   contains a history of latest product user status for the line. The first 
   element is the latest.
*/

#endif		/* #ifndef PROD_STATUS_H */
