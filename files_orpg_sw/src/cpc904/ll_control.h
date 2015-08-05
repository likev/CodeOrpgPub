/*   @(#)ll_control.h	1.2	08/18/98	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * ll_control.h of snet module
 *
 * SpiderX25
 * @(#)$Id: ll_control.h,v 1.3 2000/11/22 16:36:19 jing Exp $
 * 
 * SpiderX25 Release 8
 */

/* Modification history.
Chg Date        Init Modification
 1. 18-aug-98   rjp  Moved state literals from lapb.h to here. The state is
                     returned with stats request, so provide a definition.
 2. 24-AUG-98   mpb  Two of the #defines here (REGISTERING and RESET) are 
                     also #defined in windows NT system #include files. 
                     Since we do not use that windo's functionality here,
                     let Spider over ride.
 3.  1-oct-98   rjp  Add new state, ADM_CONN_REQ.
 4. 19-May-00	jsp  NWS - Add ioctl for LINK ENABLE/DISABLE
*/

#ifdef WINNT
/* #2 */
#ifdef REGISTERING
#undef REGISTERING
#endif /* REGISTERING */
#ifdef RESET
#undef RESET
#endif /* RESET */
#endif /* WINNT */


#define	LAPB_STID	201
#define LLC_STID	202

/*
* #1 Move state literals from lapb.h to here.
*/
/* States */
#define OFF              0
#define START            1
#define D_CONN           2
#define ADM              3
#define ARM              4
#define POLLING          5
#define PRESETUP         6
#define REGISTERING      7
#define SETUP            8
#define RESET            9
#define LL_ERROR         10
#define NORMAL           11
#define ADM_CONN_REQ	 12		/* #3				*/
 

/* IOCTL commands */
#define L_SETSNID    ('L'<<8 | 1) /* Set subnetwork identifier (use ll_snioc) */
#define L_GETSNID    ('L'<<8 | 2) /* Get subnetwork identifier (use ll_snioc) */
#define L_SETTUNE    ('L'<<8 | 3) /* Set tuning parameters (use ll_tnioc)     */
#define L_GETTUNE    ('L'<<8 | 4) /* Get tuning parameters (use ll_tnioc)     */
#define L_GETSTATS   ('L'<<8 | 5) /* Get statistics counts (use ll_stioc)     */
#define L_ZEROSTATS  ('L'<<8 | 6) /* Zero statistics       (use ll_hdioc)     */
#define L_TRACEON    ('L'<<8 | 7) /* Set message tracing on.                  */
#define L_TRACEOFF   ('L'<<8 | 8) /* Set message tracing off.                 */
#define L_GETGSTATS  ('L'<<8 | 10) /* Get global stats counts (use ll_gstioc) */
#define L_ZEROGSTATS ('L'<<8 | 11) /* Zero global statistics		      */

#ifdef NWS  /* #4 */
#define L_LINKDISABLE  ('L'<<8 | 12) /* Disable a link   (use ll_sntioc)  2:A */
#define L_LINKENABLE   ('L'<<8 | 13) /* Enable a link    (use ll_sntioc)  2:A */
#else
#define L_LINKDISABLE  ('L'<<8 | 12) /* Disable a link    (use ll_lnkst)  2:A */
#define L_LINKENABLE   ('L'<<8 | 13) /* Enable a link     (use ll_lnkst)  2:A */
#endif /* NWS */

#define L_PUTX32MAP  ('L'<<8 | 15) /* Put X.32 Table Entries                  */
#define L_GETX32MAP  ('L'<<8 | 16) /* Get X.32 Table Entries                  */

/* Values for 'lli_type' (with names of corresponding structures) */
#define LI_PLAIN	0x01        /* Indicates 'struct ll_hdioc'  */
#define LI_SNID		0x02        /* Indicates 'struct ll_snioc'  */
#define LI_STATS	0x04        /* Indicates 'struct ll_stioc'  */
#define LI_GSTATS	0x16        /* Indicates 'struct ll_gstioc' */

#define LI_LAPBTUNE	0x13        /* Indicates 'struct lapb_tnioc'*/
#define LI_LLC2TUNE	0x23        /* Indicates 'struct llc2_tnioc'*/

/* LAPB tuning structure */
typedef struct lapbtune {
    uint16  N2;		    /* Maximum number of retries		*/
    uint16  T1;		    /* Acknowledgement time	(unit 0.1 sec)  */
    uint16  Tpf;            /* P/F cycle retry time	(unit 0.1 sec)  */
    uint16  Trej;           /* Reject retry time	(unit 0.1 sec)  */
    uint16  Tbusy;          /* Remote busy check time   (unit 0.1 sec)  */
    uint16  Tidle;          /* Idle P/F cycle time	(unit 0.1 sec)  */
    uint16  ack_delay;      /* RR delay time		(unit 0.1 sec)  */
    uint16  notack_max;     /* Maximum number of unack'ed Rx I-frames   */
    uint16  tx_window;      /* Transmit window size			*/
    uint16  tx_probe;       /* P-bit position before end of Tx window   */
    uint16  max_I_len;      /* Maximum I-frame length			*/
    uint16  llconform;      /* LAPB conformance				*/
    uint16  sabm_in_x32;    /* Action when SABM received in X.32 setup  */

} lapbtune_t;

/* Values for llconform field in lapbtune_t  */

#define	IGN_UA_ERROR       1	/* UA frames ignored in ERROR state	*/
#define	FRMR_FRMR_ERROR    2	/* FRMR is retransmitted in ERROR state	*/
#define	FRMR_INVRSP_ERROR  4	/* Invalid response frames are FRMR'ed	*/
#define	SFRAME_PBIT        8	/* S frame commands must have P-bit set	*/
#define	NO_DM_ADM         16	/* Dont send DM on entry to ADM state   */
#define	IGN_DM_ERROR      32	/* DM frames ignored in ERROR state	*/

/* LLC2 tuning structure */
typedef struct llc2tune {
    uint16  N2;		    /* Maximum number of retries		*/
    uint16  T1;		    /* Acknowledgement time	(unit 0.1 sec)  */
    uint16  Tpf;            /* P/F cycle retry time	(unit 0.1 sec)  */
    uint16  Trej;           /* Reject retry time	(unit 0.1 sec)  */
    uint16  Tbusy;          /* Remote busy check time   (unit 0.1 sec)  */
    uint16  Tidle;          /* Idle P/F cycle time	(unit 0.1 sec)  */
    uint16  ack_delay;      /* RR delay time		(unit 0.1 sec)  */
    uint16  notack_max;     /* Maximum number of unack'ed Rx I-frames   */
    uint16  tx_window;      /* Transmit window (if no XID received)	*/
    uint16  tx_probe;       /* P-bit position before end of Tx window   */
    uint16  max_I_len;      /* Maximum I-frame length			*/
    uint16  xid_window;     /* XID window size (receive window)		*/
    uint16  xid_Ndup;       /* Duplicate MAC XID count  (0 => no test)  */
    uint16  xid_Tdup;       /* Duplicate MAC XID time   (unit 0.1 sec)  */
} llc2tune_t;

/* Header alone. Supports: L_ZEROSTATS, L_LINKENABLE & L_LINKDISABLE */
struct ll_hdioc {
    uint8           lli_type;      /* Table type = LI_PLAIN	*/
    uint8           lli_spare[3];  /*   (for alignment)		*/
    uint32          lli_snid;      /* Subnetwork ID character	*/
};

#ifdef NWS  /* #4 */
/* Ioctl for LINK ENABLE/DISABLE */
struct ll_sntioc {
/*  uint8           lli_type;      Table type = LI_STATUS        */
    uint8           lli_status; /* Table of tuning values        */
    uint8           lli_spare[3];/*   (for alignment)            */
    uint32          lli_snid;   /* Subnetwork ID character       */
};
#endif /* NWS */

/* Ioctl block for L_SETSNID and L_GETSNID commands */
struct ll_snioc {
    uint8           lli_type;      /* Table type = LI_SNID		*/
    uint8           lli_class;	   /* DTE/DCE/extended                  */
    uint8           lli_spare[2];  /*   (for alignment)			*/
    uint32          lli_snid;      /* Subnetwork ID character		*/
    uint32          lli_index;     /* Link index			*/
    uint32	    lli_slp_snid;  /* Subnetwork identifier for SLP	*/
    uint16	    lli_slp_pri;   /* SLP priority			*/
};


/* Ioctl block for LAPB L_SETTUNE and L_GETTUNE commands */
struct lapb_tnioc {
    uint8           lli_type;      /* Table type = LI_LAPBTUNE		  */
    uint8           lli_spare[3];  /*   (for alignment)			  */
    uint32          lli_snid;      /* Subnetwork ID character ('*' => 'all') */

    lapbtune_t      lapb_tune;     /* Table of tuning values		  */
};

/* Ioctl block for LLC2 L_SETTUNE and L_GETTUNE commands */
struct llc2_tnioc {
    uint8           lli_type;      /* Table type = LI_LLC2TUNE		  */
    uint8           lli_spare[3];  /*   (for alignment)			  */
    uint32          lli_snid;      /* Subnetwork ID character ('*' => 'all') */

    llc2tune_t      llc2_tune;     /* Table of tuning values		  */
};

/* Statistics table definitions 	*/
/* Both LAPB and LLC2 			*/
#define tx_ign		    0	/* no. ignored + not sent	*/
#define rx_badlen	    1	/* bad length frames received	*/
#define rx_unknown 	    2	/* unknown frames received	*/
#define t1_exp		    3	/* no. of T1 timeouts		*/
#define t4_exp		    4	/* no. of T4 timeouts		*/
#define t4_n2_exp	    5	/* T4 timouts after N2 times	*/

#define RR_rx_cmd           6	/* RR = Receive Ready		*/
#define RR_rx_rsp           7	/* tx = transmitted		*/
#define RR_tx_cmd           8	/* rx = received		*/
#define RR_tx_rsp           9	/* cmd/rsp = command/response	*/
#define RR_tx_cmd_p        10	/* p = p-bit set      		*/

#define RNR_rx_cmd         11	/* RNR = Receive Not Ready	*/
#define RNR_rx_rsp         12
#define RNR_tx_cmd         13
#define RNR_tx_rsp         14
#define RNR_tx_cmd_p       15

#define REJ_rx_cmd         16	/* REJ = Reject			*/
#define REJ_rx_rsp         17
#define REJ_tx_cmd         18
#define REJ_tx_rsp         19 
#define REJ_tx_cmd_p       20

#define SABME_rx_cmd       21	/* SABME = Set Asynchronous 	*/
#define SABME_tx_cmd       22	/*	 Balanced Mode Extended	*/

#define DISC_rx_cmd        23	/* DISC = Disconnect		*/
#define DISC_tx_cmd        24

#define UA_rx_rsp          25	/* UA = Unnumbered 		*/
#define UA_tx_rsp          26	/*	Acknowledement		*/

#define DM_rx_rsp          27	/*				*/
#define DM_tx_rsp          28

#define I_rx_cmd           29	/* I = Information		*/
#define I_tx_cmd           30

#define FRMR_rx_rsp        31	/* FRMR = Frame Reject		*/
#define FRMR_tx_rsp        32 

#define tx_rtr		   33	/* no. of retransmitted frames 	*/
#define rx_bad		   34	/* erroneous frames received   	*/
#define rx_dud		   35	/* received and discarded     	*/
#define rx_ign		   36	/* received and ignored        	*/

#define XID_rx_cmd         37
#define XID_rx_rsp         38
#define XID_tx_cmd         39
#define XID_tx_rsp         40

#define TEST_rx_cmd        41
#define TEST_rx_rsp        42
#define TEST_tx_cmd        43
#define TEST_tx_rsp        44

/* LAPB only */
#define SABM_rx_cmd        45   /* SABM = Set Asynchronous	*/
#define SABM_tx_cmd        46 	/*	  Balanced Mode		*/
#define SARM_rx_cmd        47 	/* SARM = Set Asynchronous	*/
#define SARM_tx_cmd        48	/*	  Response Mode		*/

#define lapbstatmax        49

/* LLC2 only */
/* values mutually exclusive with LAPB */

#define I_rx_rsp	   45	
#define I_tx_rsp           46

#define UI_rx_cmd          47	
#define UI_tx_cmd          48

#define llc2statmax        49

/* Global L2 statistics */
#define frames_tx	    0	/* frames transmitted		*/
#define frames_rx	    1	/* frames received		*/
#define sabm_tx		    2	/* for LAPB  			*/	
#define sabm_rx             3   /* for LAPB                     */
#define sabme_tx	    2   /* for LLC-2 			*/
#define sabme_rx            3   /* for LLC-2                    */
#define bytes_tx	    4	/* data bytes transmitted	*/
#define bytes_rx	    5	/* data bytes received		*/
#define globstatmax	    6	/* size of global stats array	*/

typedef struct llc2_stats {
	uint32	llc2monarray[llc2statmax]; /* array of LLC2 stats */
} llc2stats_t;

typedef struct lapb_stats {
	uint32	lapbmonarray[lapbstatmax]; /* array of LAPB stats */
} lapbstats_t;

/* Ioctl block for L_GETSTATS command (per-link) */
struct llc2_stioc {
    uint8           lli_type;	   /* Table type = LI_STATS		*/
    uint8           lli_spare[3];  /*   (for alignment)			*/
    uint32          lli_snid;      /* Subnetwork ID character		*/
    llc2stats_t     lli_stats;     /* Table of stats values		*/
};

struct lapb_stioc {
    uint8           lli_type;   /* Table type = LI_STATS		*/
    uint8	    state;      /* connection state			*/
    uint16          lli_spare;  /*   (for alignment)			*/
    uint32          lli_snid;   /* Subnetwork ID character		*/

    lapbstats_t     lli_stats;  /* Table of stats values		*/
};

/* Ioctl block for L_GETGSTATS command */
struct llc2_gstioc {
    uint8           lli_type;	   /* Table type = LI_GSTATS		*/
    uint8           lli_spare[3];  /*   (for alignment)			*/
    uint32	    llc2gstats[globstatmax];
				   /* global statistics table           */
};

struct lapb_gstioc {
    uint8           lli_type;      /* Table type = LI_GSTATS		*/
    uint8           lli_spare[3];  /*   (for alignment)			*/
    uint32	    lapbgstats[globstatmax];
				   /* global statistics table           */
};

/* Union of ioctl blocks */
typedef union lli_union {
    struct ll_hdioc	ll_hd;      /* Parameter-less command       	*/
    struct ll_snioc	ll_sn;      /* Set/get subnetwork identifier    */
    struct lapb_tnioc   lapb_tn;    /* Set/get LAPB tuning          	*/
    struct llc2_tnioc   llc2_tn;    /* Set/get LLC2 tuning          	*/
    struct llc2_stioc   llc2_st;    /* Get llc2 per-link statistics  	*/
    struct lapb_stioc   lapb_st;    /* Get lapb per-link statistics   	*/
    struct llc2_gstioc  llc2_gst;   /* Get llc2 global statistics      	*/
    struct lapb_gstioc  lapb_gst;   /* Get lapb global statistics     	*/
} lliun_t;
