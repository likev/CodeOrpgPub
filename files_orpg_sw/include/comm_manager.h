/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/10/06 18:42:32 $
 * $Id: comm_manager.h,v 1.28 2008/10/06 18:42:32 cmn Exp $
 * $Revision: 1.28 $
 * $State: Exp $
 *
 * 12MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Add support for TCP Dial-out.
 *
 */
/***********************************************************************

    Description: Public include file for comm_manager.
***********************************************************************/


#ifndef COMM_MANAGER_H

#define COMM_MANAGER_H


#define MAX_N_LINKS 48		/* maximum number of links this comm_manager
				   can manage */
#define MAX_N_STATIONS 3	/* max number of PVCs per link */
#define MAX_N_REQS (5 + MAX_N_STATIONS)
				/* number of pending requests per link; This 
				   should be sufficient for normal use. Where 
				   5 is the number of request types */
#define CTM_HDRSZE_BYTES 12


enum {				/* request types ("type" in Req_struct or 
				   Resp_struct) */
    CM_CONNECT,			/* make a connection on link "link_ind" */
    CM_DIAL_OUT,		/* dail out and make a connection on link 
				   "link_ind" */
    CM_DISCONNECT,		/* terminate the connection on link 
				   "link_ind" */
    CM_WRITE,			/* write a message of size "data_size" on link 
				   "link_ind" with priority "parm"; The 
				   data is in message of id "data_id"; The 
				   prority level can be between 0 - 
				   MAX_N_STATIONS - 1; 0 indicaets the highest 
				   */
    CM_STATUS,			/* request a status response on link 
				   "link_ind" */
    CM_CANCEL,			/* cancel the previous req of number "parm" */
    CM_DATA,			/* a incoming data message from the user */
    CM_EVENT,			/* a event notification from the comm_manager */
    CM_SET_PARAMS		/* sets/resets link parameters */
};

#define RQ_CONNECT CM_CONNECT
#define RQ_DIAL_OUT CM_DIAL_OUT
#define RQ_DISCONNECT CM_DISCONNECT
#define RQ_WRITE CM_WRITE
#define RQ_STATUS CM_STATUS
#define RQ_CANCEL CM_CANCEL
#define RQ_DATA CM_DATA
#define RQ_LOST_CONN CM_LOST_CONN


typedef struct {		/* request message struct */
    int type;			/* request type; for CM_WRITE type, the data 
				   will following this structure */
    int req_num;		/* request number; unique for the link */
    int link_ind;		/* link index */
    unsigned int time;		/* request time (UNIX time) */

    int parm;			/* request parameter; 
				   CM_WRITE: write priority (1 - n_pvc) with
					     n_pvc meaning the lowest priority;
				   CM_CANCEL: request number to cancel; -1 
					      means all previous requests */
    int data_size;		/* CM_WRITE, CM_DIAL_OUT & CM_SET_PARAMS only; 
				   CM_WRITE: data size.
				   CM_SET_PARAMS: sizeof (Link_params) 
				   CM_DIAL_OUT: sizeof (Dial_params) */
} CM_req_struct;

enum {				/* return codes in the response messages 
				   ("ret_code" in Resp_struct) */
    CM_SUCCESS,			/* requested action completed successfully */
    CM_TIMED_OUT,		/* transaction time-out */
    CM_NOT_CONFIGURED,		/* failed because the link is not configured 
				   for the requested task */
    CM_DISCONNECTED,		/* the connection is not built or lost */
    CM_CONNECTED,		/* the link is connected  */
    CM_BAD_LINK_NUMBER,	/* 5 */	/* the specified link is not configured */
    CM_INVALID_PARAMETER,	/* a parameter is illegal in the request */
    CM_TOO_MANY_REQUESTS,	/* too many pending and unfinished requests */
    CM_IN_PROCESSING,		/* a previous request of the same type is being
				   processed */
    CM_TERMINATED,		/* requested failed because a new conflicting
				   request started; */
    CM_FAILED,		/* 10 *//* requested action failed */
    CM_REJECTED,		/* requested action is rejected by the other 
				   side of the link */
    CM_LOST_CONN,		/* connection lost due to remote action */
    CM_CONN_RESTORED,		/* connection restored due to remote action */
    CM_LINK_ERROR,		/* a link error is detected and the link is 
				   disconnected */
    CM_START,		/* 15 *//* this comm_manager instance is just started */
    CM_TERMINATE,		/* this comm_manager instance is going to 
				   terminates */
    CM_STATISTICS,		/* a statistics reporting event */
    CM_EXCEPTION,		/* line exception (hardware or software errors
				   detected) */
    CM_NORMAL,			/* returned to normal from exception state */
    CM_PORT_IN_USE,	/* 20 *//* Dial port is in use  by another client*/
    CM_DIAL_ABORTED,		/* reset was pressed at the modem front panel during 
				   dilaing or modem did not detect a dial tone*/
    CM_INCOMING_CALL,		/* modem detected an incoming ring after dialing 
				   command was entered*/
    CM_BUSY_TONE,		/* Modem detected a busy tone after dialing*/
    CM_PHONENO_FORBIDDEN,	/* The no. is on the forbidden numbers list */
    CM_PHONENO_NOT_STORED, /*25*//* phone no. not strored in modem memory*/
    CM_NO_DIALTONE,		/* No answer-back tone or ring-back tone was detected 
				   in the remote modem*/
    CM_MODEM_TIMEDOUT,		/* Ringback is detected, but the call is not completed 
				   due to  timeout, i.e modem did not send any 
				   response with in the timeout value*/
    CM_INVALID_COMMAND,		/* Invalid dialout command or a command that the modem
				   cannot execute */
    CM_TRY_LATER,		/* Try the request at a later time*/
    CM_MODEM_PROBLEMS,  /*30*/  /* General catch all modem dial-out problems */
    CM_MODEMRETRY_PROBLEMS,     /* General catch all modem retry-able dial-out problems */
    CM_RTR_PROBLEMS,            /* General catch all router dial-out problems */
    CM_RTRRETRY_PROBLEMS,       /* General catch all router retry-able dial-out problems */
    CM_DIAL_TIMEOUT,            /* After about 25 seconds modem still didn't go offhook  */
    CM_STATUS_MSG,	/* 35 *//* A status message from cm_tcp */
    CM_BUFFER_OVERFLOW,		/* To-be-packed messages expired in the 
				   request buffer. */
    CM_WRITE_PENDING		/* response for CM_STATUS request */
};

#define RET_SUCCESS CM_SUCCESS
#define RET_TIMED_OUT CM_TIMED_OUT
#define RET_NOT_CONFIGURED CM_NOT_CONFIGURED
#define RET_DISCONNECTED CM_DISCONNECTED
#define RET_CONNECTED CM_CONNECTED
#define RET_BAD_LINK_NUMBER CM_BAD_LINK_NUMBER
#define RET_INVALID_PARAMETER CM_INVALID_PARAMETER
#define RET_TOO_MANY_REQUESTS CM_TOO_MANY_REQUESTS
#define RET_IN_PROCESSING CM_IN_PROCESSING
#define RET_TERMINATED CM_TERMINATED
#define RET_FAILED CM_FAILED
#define RET_REJECTED CM_REJECTED

typedef struct {		/* response message struct */
    int type;			/* request type; CM_DATA means an incoming
				   data; CM_LOST_CONN indicates that the 
				   connection is lost; Data will follow this 
				   structure for type CM_DATA and CM_STATUS */
    int req_num;		/* request number; meaningless for CM_DATA */
    int link_ind;		/* link index */
    unsigned int time;		/* response time (UNIX time) */

    int ret_code;		/* a code indicating the request processing 
				   results */

    int data_size;		/* CM_DATA and CM_STATUS only; data size 
				   after this structure */
} CM_resp_struct;


/* data structure for X25 statistics. Each field except "rate" is the 
   accumulated number of packets since last report. */
#define CM_SF_UNAVAILABLE 0xffff	/* indicates that the field is not 
					   available in this report. */
typedef struct {
    unsigned short R_I;			/* good recerved info packets */
    unsigned short X_I;			/* good trasmitted info packets */
    unsigned short R_RR;
    unsigned short X_RR;
    unsigned short R_RNR;
    unsigned short X_RNR;
    unsigned short R_REJ;
    unsigned short X_REJ;
    unsigned short R_SABM;
    unsigned short X_SABM;
    unsigned short R_DISC;
    unsigned short X_DISC;
    unsigned short R_DM;
    unsigned short X_DM;
    unsigned short R_UA;
    unsigned short X_UA;
    unsigned short R_FRMR;
    unsigned short X_FRMR;
    unsigned short R_RESTART;
    unsigned short X_RESTART;
    unsigned short R_RESET;
    unsigned short X_RESET;
    unsigned short R_ERROR;		/* overrun, I frame too long */
    unsigned short X_ERROR;		/* underrun, timed-out */
    unsigned short R_FCS;		/* FCS error */
    unsigned short rate;		/* achieved rate in bytes per second */
} X25_statistics;

typedef struct {	/* data structure for TCP statistics */ 

    int keep_alive_test_period;		/* in seconds */
    int no_input_time;			/* number of seconds after the lattest
					   input data read */
    int rate;				/* achieved rate in bytes per second */
    int tnb_sent;			/* total number of bytes sent */
    int tnb_received;			/* total number of bytes received */
} TCP_statistics;

enum {				/* values for link types */
    CM_DEDICATED,		/* dedicated */
    CM_DIAL_IN,			/* dial-in */
    CM_DIAL_IN_OUT,		/* dial-in/out */
    CM_WAN			/* WAN based connection */
};

typedef struct {		/* link parameters sent through CM_SET_PARAMS
				   request. Negative values indicate not 
				   specified */
    int link_type;		/* link type */
    int line_rate;		/* line baud rate */
    int packet_size;		/* packet size */
    int rw_time;		/* max read/write time */
    int rep_period;		/* statistics reporting period */
} Link_params;

typedef struct {		/*dial link parameters for CM_DIAL_OUT option*/
    char phone_no[32];		/* telephone no. to dial out */
} Dial_params;

#endif		/* #ifndef COMM_MANAGER_H */
