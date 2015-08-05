/*   @(#) dlpiabm.h 99/12/23 Version 1.9   */
/* Data Link Provider Interface ( DLPI ) Primitives and Definitions */

/*
Modification history:
 
Chg Date       Init Description
1.  09-SEP-98  lmm  Added DL_NO_ANSWER disconnect reason code
2.  06-NOV-98  lmm  Removed DL_SP_LOOPBACK define (obsolete)
3.  11-NOV-98  rjp  Added support for of end-to-end acks.
4.  18-NOV-98  rjp  Changed spellings.
5.  23-NOV-98  lmm  Added data link encoding to bind request
6.  29-SEP-99  lmm  Added NRZ/NRZI (with clock) encoding modes
*/

#define	DL_BIND_ACK			1
#define	DL_BIND_REQ			2
#define	DL_CONNECT_CON			3
#define	DL_CONNECT_REQ			4
#define	DL_DISCONNECT_IND		5
#define	DL_DISCONNECT_REQ		6
#define	DL_ERROR_ACK			7
#define	DL_GET_STATISTICS_ACK		8
#define	DL_GET_STATISTICS_REQ		9
#define DL_OK_ACK                       10
#define DL_RESET_IND			11
#define	DL_RESET_REQ			12
#define DL_RESET_CON                    13
#define	DL_UNBIND_REQ			14
#define DL_TEST_IND			15
#define DL_DATA_REQ                     16
#define DL_DATA_IND                     17
#define DL_SIM_REQ			18
#define DL_SIM_CON			19
#define DL_SEND_ACK_REQ		 	20	/* #3 */
#define DL_SEND_ACK_REJECT	 	21	/* #3 */
#define DL_ACK_RECD_REQ			22	/* #3 */
#define DL_ACK_RECD_CON			23	/* #3 */


#define	DL_ERROR_BASE			0xe0
#define	DL_DLSAP_IN_USE			0xe0
#define	DL_LL2_REJECT			0xe1
#define DL_NOTINIT		        0xe2
#define DL_TIMEDOUT			0xe3
#define DL_NOBUFFERS			0xe4
#define	DL_LINK_DOWN			0xe5
#define	DL_BADDATA                      0xe7
#define	DL_REJ_AND_SREJ                 0xe8
#define DL_INVALID_ADDRESS		0xe9


#define		MAX_ADDR_LEN	4

typedef         struct
{
   bit32        dl_add_xid;
   bit32        dl_add_rej;
   bit32        dl_add_srej;
   bit32        dl_add_ui;
   bit32        dl_add_sim_rim;
   bit32        dl_add_up;
   bit32        dl_add_ext_addr;
   bit32        dl_del_resp_i;
   bit32        dl_del_cmd_i;
   bit32        dl_add_mod128;
   bit32        dl_add_test;
   bit32        dl_add_rd;
   bit32        dl_addr0_local;
   bit32        dl_ext_addr1_local;
   bit32        dl_ext_addr2_local;
   bit32        dl_ext_addr3_local;
   bit32        dl_addr0_remote;
   bit32        dl_ext_addr1_remote;
   bit32        dl_ext_addr2_remote;
   bit32        dl_ext_addr3_remote;
} dl_options;


typedef		struct
{
   bit32	dl_primitive;
} dlpi_primitive;

typedef		struct
{
   bit32	dl_primitive;
   bit32        dl_sap;			/* Link number			   */
   bit32        dl_service;             /* Data link or monitor link       */
#define         DL_DATALINK     0
#define         DL_MONITOR      1       /* Monitor mode only               */
#define         DL_END_TO_END   2	/* #3 Support end to end acknowledge */

   bit32	dl_addr_type;
#define		DL_DTE		0
#define		DL_DCE		1

/* Max frame size EXCLUDING 2-byte hdlc header 				   */
   bit32        dl_max_frame;		
   bit32        dl_baud_rate;           /* line speed			   */

/* Timer T1, at the end of which retransmission of a frame is initiated    */
   bit32	dl_t1;	

/* Timer T3, is the timeout interval for completion of link initialization */
   bit32        dl_t3;

/* N2, the maximum number of trans/retransmissions of a given frame        */
   bit32        dl_n2;

/* K, the maximum number of information frames outstanding at any time     */
   bit32        dl_k;

/* The time to delay before transmitting an RR in response to one or more
   I-frames (a value of zero => an RR response to each I-frame 
   individually.                                                           */
   bit32	dl_rr_delay;

/* The maximum number of unacknowledged I-frames that should be
   received before sending an RR response. */
   bit32	dl_unack_max;

/* "Keep alive" function.  An RR will be sent from the primary every
    n tenths of a second if no I-frames are received from the remote
    to solicit a response to insure that the link is still alive.          */
   bit32	dl_rualive;

/* Special feature to support REUTERS IDN.  They provide no modem
   signals to determine whether the link is physically connected.
   However, they guarantee that if it is physically connected, data
   will always be present.  They simply use a timer.  If nothing
   is received in 5 seconds, they consider the link to be detached.
   ZERO disables this function.  Positive values are interpreted
   as tenths of seconds. */
   bit32	dl_Reuters;

   bit32        dl_modem_sigs;          /* Boolean: wait for CTS & DCD     */
   dl_options	dl_options_list;	/* Select optional adccp functions */
   bit32        dl_encoding;            /* #5 - link encoding mode         */
 
#ifndef NRZ_MODE
#define NRZ_MODE          0
#define NRZI_MODE         1
#define FM0_MODE          2     /* QUICC platforms only */
#define FM1_MODE          3     /* QUICC platforms only */
#define MANCHESTER_MODE   4     /* QUICC platforms only */
#define DMANCHESTER_MODE  5     /* QUICC platforms only */
#define NRZ_CLOCK_MODE    6     /* #6 - QUICC platforms only */
#define NRZI_CLOCK_MODE   7     /* #6 - QUICC platforms only */
#define MAX_ENCODING_MODE 7     /* #6 */
#endif

} dl_bind_req_t;

typedef         struct
{   
   bit32	dl_primitive;
   bit32	dl_error_primitive;
   bit32	dl_errno;
   bit32	dl_unix_errno;
} dl_error_ack_t;

/* This structure is located in the M_DATA linked to the dlpi_primitive
   M_PROTO. */

typedef         struct
{
   bit32        dl_Xgood;
   bit32        dl_Rgood;
   bit32        dl_Xunder;
   bit32        dl_Rover;
   bit32        dl_Rtoo;
   bit32        dl_Rcrc;
   bit32	dl_Rabt;

   bit32	dl_link_state;
#define		DL_UNDEFINED		0
#define         DL_PRESETUP		1
#define         DL_UP    		2
#define         DL_SETUP		4
#define		DL_RESET		5
#define		DL_DISCONNECT		6
   bit32        dl_TxIFrames;
   bit32        dl_TxRRs;
   bit32        dl_TxRNRs;
   bit32        dl_TxREJs;
   bit32        dl_TxUSFrames;
   bit32        dl_TxDISCs;
   bit32        dl_TxSABMs;
   bit32        dl_TxSABMEs;
   bit32        dl_TxSARMs;
   bit32        dl_TxUAs;
   bit32        dl_TxFRMRs;
   bit32        dl_TxUUFrames;
   bit32        dl_TxpollA;
   bit32        dl_TxpollB;
   bit32        dl_TxpollU;
   bit32        dl_Txwindow;
   bit32        dl_RxIFrames;
   bit32        dl_RxRRs;
   bit32        dl_RxRNRs;
   bit32        dl_RxREJs;
   bit32        dl_RxUSFrames;
   bit32        dl_RxDISCs;
   bit32        dl_RxSABMs;
   bit32        dl_RxSABMEs;
   bit32        dl_RxSARMs;
   bit32        dl_RxUAs;
   bit32        dl_RxFRMRs;
   bit32        dl_RxUUFrames;
   bit32        dl_RxpollA;
   bit32        dl_RxpollB;
   bit32        dl_RxpollU;
   bit32        dl_Rxwindow;
} dl_get_statistics_ack_t;


typedef         struct
{
   bit32	dl_primitive;
   bit32	dl_sap;
} dl_bind_ack_t;

typedef		struct			/* #3 */
{
   bit32	dl_primitive;
   bit32	dl_n_ack;		/* #4 Ack this many messages	*/
} dl_send_ack_req_t;

typedef 	struct			/* #3 Support end-to-end ack 	*/
{
   bit32	dl_primitive;
   bit32	dl_ack_recd;		/* #4				*/
} dl_ack_recd_con_t;

typedef         struct
{
   bit32        dl_primitive;
   bit32	dl_correct_primitive;
} dl_ok_ack_t;

typedef         struct
{
   bit8         direction;
#define         INBOUND     0
#define         OUTBOUND    1
   bit8		addr_len;
   bit8         address[8];
   bit8		mod128;
   bit8         control;
   bit8		ctl2;
   bit8      frmr_octets [ 3 ];		/* got rid of union; pmt */
   bit16     iframe_size;
} dl_trace_t;

typedef         struct
{
   bit32        dl_primitive;
   bit32	dl_entries;
   dl_trace_t   dl_traces  [ 16 ];
} dl_test_ind_t;


typedef         struct
{
   bit32        dl_primitive;
   bit32	dl_originator;
#define		DL_REMOTE	0
#define		DL_LOCAL	1
   bit32	dl_reason;
#define		DL_START_FAIL	0
#define         DL_CONNECT_FAIL	1
#define		DL_DISC_RCVD	2
#define		DL_NO_SIG	3
#define		DL_NO_ANSWER	4		/* #1 */
} dl_disconnect_ind_t;

typedef         struct
{
   bit32        dl_primitive;
   bit32        dl_data_type;
#define         DL_I_FORMAT     0
#define         DL_UI_FORMAT    1
} dl_data_t;

