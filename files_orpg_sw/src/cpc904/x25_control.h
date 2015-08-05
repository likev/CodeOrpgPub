/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 * x25_control.h of snet module
 *
 * SpiderX25
 * @(#)$Id: x25_control.h,v 1.3 2000/11/27 20:51:07 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history.

Chg Date	Init Description
 1.  8-Nov-00   djb  Added ioctl definition for enabling DLPI monitoring
 2.  8-Nov-00   djb  Added ioctl definition for enabling Tx Acking
 3. 21-Nov-00   djb  Added restart enabled field to wlcfg data structure and 
                     ioctl definitions for user restarts.
*/

#define X25_STID	200
#define	NLI1_STID	211
#define	NLI2_STID	213

/******************************************************************
 *
 * M_IOCTL message type constants for I_STR 
 *
 */

#define	N_snident	 (('N'<<8) | 1)
#define	N_snmode	 (('N'<<8) | 2)
#define	N_snconfig	 (('N'<<8) | 3)
#define	N_snread	 (('N'<<8) | 4)
#define	N_getstats	 (('N'<<8) | 5)
#define	N_zerostats	 (('N'<<8) | 6)
#define	N_putpvcmap	 (('N'<<8) | 7)
#define	N_getpvcmap	 (('N'<<8) | 8)
#define	N_getVCstatus	 (('N'<<8) | 9)
#define N_getnliversion  (('N'<<8) | 10)

/* Tracing */
#define N_traceon	 (('N'<<8) | 11)
#define N_traceoff	 (('N'<<8) | 12)

/*  NUI table manipulation */
#define N_nuimsg	 (('N'<<8) | 13) /* prim_class for NUI msgs  */
#define N_nuiput	 (('N'<<8) | 14) /* put entry into table     */
#define N_nuidel	 (('N'<<8) | 15) /* delete entry from table  */
#define N_nuiget	 (('N'<<8) | 16) /* return table entry       */
#define N_nuimget	 (('N'<<8) | 17) /* return all table entries */
#define N_nuireset	 (('N'<<8) | 18) /* reset address table      */

#define N_zeroVCstats    (('N'<<8) | 21)
#define N_putx32map      (('N'<<8) | 22)
#define N_getx32map      (('N'<<8) | 23)
#define N_getSNIDstats	 (('N'<<8) | 24)
#define N_zeroSNIDstats	 (('N'<<8) | 25)

/* QOS Data Priority Manipulation */
#define N_setQOSDATPRI   (('N'<<8) | 26)
#define N_resetQOSDATPRI (('N'<<8) | 27)

#ifdef NOTIFY_STATUS /* #1 */
#define N_DLPInotify     (('N'<<8) | 28) /* DLPI status change notification   */
#endif /* NOTIFY_STATUS */

#ifdef X25_ACKS /* #2 */
#define N_enablelocalACK (('N'<<8) | 29)
#endif /* X25_ACKS */

#ifdef USER_RESTART /* #3 */
#define N_sendRESTART    (('N'<<8) | 30) /* Manual REST/C Transmission        */
#define N_tempDXFER      (('N'<<8) | 31) /* Toggle temporary data-xfer state  */
#endif /* USER_RESTART */

/******************************************************************
 *
 * M_IOCTL message type definitions 
 *
 */

#define NLI_VERSION 3	/* Current NLI version, returned by N_getnliversion */

struct nliformat {
	unsigned char	version;	/* NLI version number */
};

struct xll_reg {			/* For ioctl registration */
	uint32		snid;		/* Subnetwork identifier ( ppa ) */
	uint32		lmuxid;		/* LINK identifier */
	uint16		dl_sap;		/* Link SAP */
	uint16		dl_max_conind;	/* Number of conn inds */
};

struct snoptformat {
	uint32      	U_SN_ID;
	unsigned short	newSUB_MODES;
	unsigned char	rd_wr;
};

struct pvcconff {
	uint32      	  sn_id;        /* Subnetwork       */
	unsigned short    lci;          /* Logical channel  */
	unsigned char     locpacket;    /* Loc packet size  */
	unsigned char     rempacket;    /* Rem packet size  */
	unsigned char     locwsize;     /* Loc window size  */
	unsigned char     remwsize;     /* Rem window size  */
};

#ifndef MAXIOCBSZ
#define MAXIOCBSZ     1024
#endif

#define PVC_MAP_SIZE   (MAXIOCBSZ - 8)
#define MAX_PVC_ENTS   (PVC_MAP_SIZE / sizeof(struct pvcconff))

struct pvcmapf {
	struct pvcconff entries[MAX_PVC_ENTS]; /* Data buffer             */
	unsigned int    first_ent;             /* Where to start search   */
	unsigned int    num_ent;               /* Number entries returned */
};

/************************ 
  NUI Table Manipulation 
 ************************/

#define NUIMAXSIZE	64
#define NUIFACMAXSIZE	32

struct nuiformat {
	unsigned char	nui_len;
	unsigned char	nui_string[NUIMAXSIZE];	   /* Network User Identifier */
};

struct facformat {
	unsigned short SUB_MODES;	/* Mode tuning bits for net */
        unsigned char  LOCDEFPKTSIZE;	/* Local default pkt p */
        unsigned char  REMDEFPKTSIZE;	/* Local default pkt p */
        unsigned char  LOCDEFWSIZE;	/* Local default window size */
        unsigned char  REMDEFWSIZE;	/* Local default window size */
	unsigned char  locdefthclass;   /* Local default value         */
	unsigned char  remdefthclass;   /* Remote default value        */
	unsigned char  CUG_CONTROL;	/* CUG facilities */
};

/*
 * Address table manipulation primitives
 */

struct nui_put {
	char	prim_class;	/* Always NUI_MSG			*/
	char	op;		/* Always NUI_ENT			*/
	struct	nuiformat	nuid;		/* NUI			*/
	struct	facformat	nuifacility;	/* NUI facilities	*/
};

struct nui_del {
	char	prim_class;	/* Always NUI_MSG			*/
	char	op;		/* Always NUI_DEL			*/
	struct	nuiformat	nuid;		/* NUI	to delete	*/
};

struct nui_get {
	char		prim_class;	/* Always NUI_MSG		*/
	char		op;		/* Always NUI_GET		*/
	struct nuiformat	nuid;		/* NUI to get		*/
/* Following field filled in in reply: (as for NUI_PUT)	*/
	struct facformat	nuifacility;	/* NUI facilities	*/
};

struct nui_reset {
	char		prim_class;	/* Always NUI_MSG	*/
	char		op;		/* Always NUI_RESET	*/
};

typedef struct nui_addr {
	struct facformat	nuifacility;	/* NUI facilities	*/
	int     		next_free;	/* Next free entry      */
	struct nuiformat	nuid;		/* NUI			*/
	unsigned char		status;		/* temp/permanent entry */
} NUI_ADDR;

#define NUI_TEMPORARY	1	/* temporary address entry status	*/
#define NUI_PERMANENT	2	/* permanent address entry status	*/

/*
 *  Defines for the Multiple Get IOCTL
 *
 *  NOTE: This size of structure "nui_mget" must not
 *	  exceed MAXIOCBSZ byte (as defined in stream.h).
 *	  Thus if other fields are added to "nui_mget"
 *	  then MGET_NBUFSIZE must decrease to cope
 *	  with any changes made.
 */
 
#define MGET_NBUFSIZE	(MAXIOCBSZ - 20)	/*
						 * Take into account the
						 * other 5 fields by reducing
						 * the buffer size by 20
						 */
#define MGET_NMAXENT	(MGET_NBUFSIZE / sizeof(NUI_ADDR) ) 


struct nui_mget {
	char		prim_class;	   /* Always NUI_MSG		*/
	char		op;		   /* Always NUI_MGET		*/
	unsigned int 	first_ent;	   /* First entry required	*/
	unsigned int 	last_ent; 	   /* Last entry required	*/
	unsigned int 	num_ent;	   /* Number of entries required*/
	char 		buf[MGET_NBUFSIZE]; /* Data Buffer		*/
};

typedef union nui_adrop {
	char	prim_class;		/* Always NUI_MSG		*/
	struct	nui_put   nui_put;
	struct	nui_del   nui_del;
	struct	nui_get   nui_get;
	struct	nui_mget  nui_mget;
	struct	nui_reset nui_reset;
} S_NUI_ADROP;


/* ----------------------------------------------------------------------
 *
 * Definition for the configuration record structure.  Includes all
 *  parameters.
 * ---------------------------------------------------------------------- 
 */

typedef struct wlcfg {
	uint32         U_SN_ID;		/* Upper level id for net (soft) */
	unsigned char  NET_MODE;	/* Prot/net in use e.g. PinkBook */
	unsigned char  X25_VSN;		/* Non-zero : 1984, otherwise 1980 */
	unsigned char  L3PLPMODE;	/* 1 DTE, 0 DCE, 2 DXE */

        /* X25 PLP virtual circuit ranges */

	short         LPC;               /* Lowest  Permanent VC           */
	short         HPC;               /* Highest Permanent VC           */
        short         LIC;               /* Lowest  Incoming channel       */
        short         HIC;               /* Highest Incoming channel       */
        short         LTC;               /* Lowest  Two-way channel        */
        short         HTC;               /* Highest Two-way channel        */
        short         LOC;               /* Lowest  Outgoing channel       */
        short         HOC;               /* Highest Outgoing channel       */
	short         NPCchannels;	 /* Number PVC channels            */
	short         NICchannels;       /* Number IC channels             */
	short         NTCchannels;       /* Number TC channels             */
	short         NOCchannels;       /* Number OC channels             */
        short         Nochnls;           /* Total number of channels       */

	unsigned char  THISGFI;		/* GFI operating on net */
        unsigned char  LOCMAXPKTSIZE;	/* Max value acceptable for pkt p */
        unsigned char  REMMAXPKTSIZE;	/* Max value acceptable for pkt p */
        unsigned char  LOCDEFPKTSIZE;	/* Local default pkt p */
        unsigned char  REMDEFPKTSIZE;	/* Local default pkt p */
	unsigned char  LOCMAXWSIZE;	/* Max value acceptable for wsize */
	unsigned char  REMMAXWSIZE;	/* Max value acceptable for wsize */
        unsigned char  LOCDEFWSIZE;	/* Local default window size */
        unsigned char  REMDEFWSIZE;	/* Local default window size */

        unsigned short MAXNSDULEN;	/* Max data delivery to N-user */

        /* X25 PLP timer and retransmission values */

        short         ACKDELAY;		 /* Ack suppress and buffs low     */
        short         T20value;          /* Restart request                */
        short         T21value;          /* Call request                   */
        short         T22value;          /* Reset request                  */
        short         T23value;          /* Clear request                  */

        short         Tvalue;            /* Ack and busy timer             */
        short         T25value;          /* Window rotation timer          */
        short         T26value;          /* Interrupt response             */
	short         T28value;          /* Registration request           */

        short         idlevalue;         /* Idle timeout value for link    */
        short         connectvalue;      /* Link connect timer             */

        unsigned char R20value;          /* Restart request                */
        unsigned char R22value;          /* Reset request                  */
        unsigned char R23value;          /* Clear request                  */
	unsigned char R28value;          /* Registration request           */

	/* Local values for qos checking */

	unsigned short localdelay;       /* Internal delay locally      */
	unsigned short accessdelay;      /* Line access delay locally   */

	/* Throughput Classes */
	
	unsigned char  locmaxthclass;    /* Local max thruput           */
	unsigned char  remmaxthclass;    /* Remote max thruput          */
	unsigned char  locdefthclass;    /* Local default value         */
	unsigned char  remdefthclass;    /* Remote default value        */
	unsigned char  locminthclass;    /* Local minimum for the PSDN  */
	unsigned char  remminthclass;    /* Remote minimum for the PSDN */

	unsigned char  CUG_CONTROL;	 /* CUG facilities subscribed to */
	unsigned short SUB_MODES;	 /* Mode tuning bits for net */

	/* PSDN localisation record */

	struct {
	unsigned short SNMODES;			/* Mode tuning for PSDN */
	unsigned char intl_addr_recogn;		/* Recognise intnatl  */
	unsigned char intl_prioritised;		/* Prioritise intnatl */
	unsigned char dnic1;			/* 4 BCD digits DNIC  */
	unsigned char dnic2;			/* used when required */
	unsigned char prty_encode_control;	/* Encode priority    */
	unsigned char prty_pkt_forced_value;    /* Force pkt size     */
	unsigned char src_addr_control;		/* Calling addr fixes */
	unsigned char dbit_control;		/* Action on D-bit    */
	unsigned char thclass_neg_to_def;       /* TELENET negn type  */
	unsigned char thclass_type;		/* tclass map handle  */
	unsigned char thclass_wmap[16];		/* tclass -> wsize    */
	unsigned char thclass_pmap[16];		/* tclass -> psize    */
	} psdn_local;

	/* Link level local address or local DTE address */

	struct lsapformat local_address;

#ifdef USER_RESTART /* #3 */
        unsigned char user_restart_enabled;
#endif /* USER_RESTART */
        } x25tune_t;

/* ---- WELL KNOWN SAP's ---- */

#define    X25overLAPD        0x10

/* ---- SUBNET TUNING MODE BITS ---- */

#define    SUB_EXTENDED           1  /* Subscribe extended fac               */
#define    BAR_EXTENDED           2  /* Bar extended incoming                */
#define    SUB_FSELECT            4  /* Subscribe fselect fac                */
#define    SUB_FSRRESP            8  /* Subscribe fs restrict                */
#define    SUB_REVCHARGE         16  /* Subscribe rev charge                 */
#define    SUB_LOC_CHG_PREV      32  /* Subscribe local charging prevention  */
#define    BAR_INCALL            64  /* Bar incoming calls                   */
#define    BAR_OUTCALL          128  /* Bar ougoing calls                    */
#define    SUB_TOA_NPI_FMT      256  /* Subscribe TOA/NPI Address Format     */
#define    BAR_TOA_NPI_FMT      512  /* Bar TOA/NPI Address Format incoming  */
#define    SUB_NUI_OVERRIDE    1024  /* Subscribe NUI Override               */
#define    BAR_CALL_X32_REG    2048  /* Bar calls while X.32 reg. in process */
#define    AND_80                 0  /* BIT RESERVED 84/80                   */

/* ---- PSDN LOCALISATION TUNING MODE BITS ---- */

#define    ACC_NODIAG             1  /* Short clr etc is OK                  */
#define    USE_DIAG               2  /* Use diagnostic packet                */
#define    CCITT_CLEAR_LEN        4  /* Restrict Clear lengths               */
#define    BAR_DIAG               8  /* Bar diagnostic packets               */
#define    DISC_NZ_DIAG          16  /* Discard diag packets on non-zero LCN */
#define    ACC_HEX_ADD           32  /* Allow hex digits in dte addresses    */
#define    BAR_NONPRIV_LISTEN    64  /* Bar non privileged users from   
                                        listening for incoming calls.        */
/* ---- CUG SUBSCRIPTION BITS ------- */

#define		SUB_CUG			1
#define 	SUB_PREF		2
#define		SUB_CUGOA		4
#define		SUB_CUGIA		8
#define		BASIC			16
#define		EXTENDED		32
#define 	BAR_CUG_IN		64

/* ----- D-BIT CONTROL ACTIONS ------- */

#define 	CC_IN_CLR		1
#define 	CC_IN_ZERO		2
#define		CC_OUT_CLR		4
#define 	CC_OUT_ZERO		8
#define 	DT_IN_RST		16
#define  	DT_IN_ZERO		32
#define		DT_OUT_RST		64
#define		DT_OUT_ZERO		128


/* ------ X25 Version Types --------- */
#define	X25_VSN_80	0
#define	X25_VSN_84	1
#define	X25_VSN_88	2


/* ------ NETWORK MODE TYPES --------- */

/* (Only X25_LLC and DATAPAC currently plumbed into code) */

#define		X25_LLC		1	/*  X.25(84)/LLC-2	*/
#define		X25_88		2	/*  X.25(88)		*/
#define		X25_84		3	/*  X.25(84)		*/
#define		X25_80		4	/*  X.25(80)		*/
#define		PSS		5	/*  UK			*/
#define		AUSTPAC		6	/*  Australia		*/
#define		DATAPAC		7	/*  Canada		*/
#define		DDN		8	/*  USA			*/
#define		TELENET		9	/*  USA			*/
#define		TRANSPAC	10	/*  France		*/
#define		TYMNET		11	/*  USA			*/
#define		DATEX_P		12	/*  West Germany	*/
#define		DDX_P		13	/*  Japan		*/
#define		VENUS_P		14	/*  Japan		*/
#define		ACCUNET		15	/*  USA			*/
#define		ITAPAC		16	/*  Italy		*/
#define		DATAPAK		17	/*  Sweden		*/
#define		DATANET		18	/*  Holland		*/
#define		DCS		19	/*  Belgium		*/
#define		TELEPAC		20	/*  Switzerland		*/
#define		F_DATAPAC	21	/*  Finland		*/
#define		FINPAC		22	/*  Finland		*/
#define		PACNET		23	/*  New Zealand		*/
#define		LUXPAC		24	/*  Luxembourgh		*/
#define		X25_CIRCUIT	25	/*  Circuit switched network */

#define		G_8		0X10	/* GFI - not extended sequencing   */
#define		G_128		0X20	/* GFI - extended sequencing       */


/* Statistical Information */

/* GLOBAL */

/* totals for VC stats */

#define		cll_in_g	1	/* Calls rcvd and indicated        */
#define		cll_out_g	2	/* Calls sent                      */
#define		caa_in_g	3	/* Call established for outgoing   */
#define		caa_out_g	4	/* Ditto - in call                 */
#define		ed_in_g		5	/* Interrupts rcvd                 */
#define		ed_out_g	6	/* Interrupts sent                 */
#define		rnr_in_g	7	/* Receiver not ready rcvd         */
#define		rnr_out_g	8	/* Receiver not ready sent         */
#define		rr_in_g		9	/* Receiver ready rvcd             */
#define		rr_out_g	10	/* Receiver ready sent             */
#define		rst_in_g	11	/* Resets rcvd                     */
#define		rst_out_g	12	/* Resets sent                     */
#define		rsc_in_g	13	/* Restart confirms rcvd           */
#define		rsc_out_g	14	/* Restart confirms sent           */
#define		clr_in_g	15	/* Clears rcvd                     */
#define		clr_out_g	16	/* Clears sent                     */
#define		clc_in_g	17	/* Clear confirms rcvd             */
#define		clc_out_g	18	/* Clear confirms sent             */

/* Totals for driver */

#define		cll_coll_g	19	/* Call collision count (not rjc)  */
#define		cll_uabort_g	20	/* Calls aborted by user b4 sent   */
#define		rjc_buflow_g	21	/* Calls rejd no buffs b4 sent     */
#define		rjc_coll_g	22	/* Calls rejd - collision DCE mode */
#define		rjc_failNRS_g	23	/* Calls rejd negative NRS resp    */
#define		rjc_lstate_g	24	/* Calls rejd link disconnecting   */
#define		rjc_nochnl_g	25	/* Calls rejd no lcns left         */
#define		rjc_nouser_g	26	/* In call but no user on NSAP     */
#define		rjc_remote_g	27	/* Call rejd by remote responder   */
#define		rjc_u_g		28	/* Call rejd by NS user            */
#define		dg_in_g		29	/* DIAG packets in                 */
#define		dg_out_g	30	/* DIAG packets out                */
#define		p4_ferr_g	31	/* Format errors in P4             */
#define		rem_perr_g	32	/* Remote protocol errors          */
#define		res_ferr_g	33	/* Restart format errors           */
#define		res_in_g	34	/* Restarts received (inc DTE/DXE) */
#define		res_out_g	35	/* Restarts sent (inc DTE/DXE)     */
#define		vcs_labort_g	36	/* Circuits aborted via link event */
#define		r23exp_g	37	/* Circuits hung by r23 expiry     */
#define		l2conin_g	38	/* Link level connect established  */
#define		l2conok_g	39	/* LLC connections accepted        */
#define		l2conrej_g	40	/* LLC connections rejd            */
#define		l2refusal_g	41	/* LLC connnect requests refused   */
#define		l2lzap_g	42	/* Oper requests to kill link      */
#define		l2r20exp_g	43	/* R20 retransmission expiry       */
#define		l2dxeexp_g	44	/* DXE/connect expiry              */
#define		l2dxebuf_g	45	/* DXE resolv abort - no buffers   */
#define		l2noconfig_g	46	/* No config base - error          */
#define		xiffnerror_g	47	/* Upper i/f bad M_PROTO type	   */
#define		xintdisc_g	48	/* Internal disconnect events	   */
#define		xifaborts_g	49	/* Interface abort_vc called	   */
#define		PVCusergone_g	50	/* Count of non-user interactions  */

#define		max_opens_g	51	/* highest no. simul. opens so far */
#define		vcs_est_g	52	/* VCs established since reset 	   */
#define		bytes_in_g	53	/* Total data bytes received       */
#define		bytes_out_g	54	/* Total data bytes sent	   */
#define		dt_in_g		55	/* Count of data packets sent      */
#define		dt_out_g	56	/* Count of data packets received  */
#define		res_conf_in_g	57	/* Restart Confirms received       */
#define		res_conf_out_g	58	/* Restart Confirms sent           */

#define		reg_in_g	59	/* Registration requests rcvd      */
#define		reg_out_g	60	/* Registration requests sent      */
#define		reg_conf_in_g	61	/* Registration confirms rcvd      */
#define		reg_conf_out_g	62	/* Registration confirms sent      */
#define		l2r28exp_g	63	/* R28 retransmission expiry       */
#define		Cantlzap_g	64
#define		L2badcc_g	65
#define		L2baddcnf_g	66
#define		L3T25timeouts_g	67
#define		L3badAE_g	68
#define		L3badT20_g	69
#define		L3badT24_g	70
#define		L3badT25_g	71
#define		L3badT28_g	72
#define		L3badevent_g	73
#define		L3badgfi_g	74
#define		L3badlstate_g	75
#define		L3badltock2_g	76
#define		L3badrandom_g	77
#define		L3badxtock0_g	78
#define		L3clrbadstate_g	79
#define		L3conlt0_g	80
#define		L3deqfailed_g	81

#define		L3indnodata_g	82
#define		L3matrixcall_g	83
#define		L3nodb_g	84
#define		L3qoscheck_g	85
#define		L3outbad_g	86
#define		L3shortframe_g	87
#define		L3tabfault_g	88
#define		L3usererror_g	89
#define		L3usergone_g	90
#define		LNeednotneeded_g 91
#define		NSUbadref_g	92
#define		NSUdtnull_g	93
#define		NSUednull_g	94
#define		NSUrefrange_g	95
#define		NeednotNeeded_g	96
#define		NoNRSrequest_g	97
#define		UDRbad_g	98
#define		Ubadint_g	99
#define		Unoint_g	100
#define		L3baddiag_g	101

#define		glob_mon_size	102	/* 1 over last, for length         */


/* SUBNETWORK statistics */

#define		cll_in_s	 1	/* Calls rcvd                      */
#define		cll_out_s	 2	/* Calls sent                      */
#define		caa_in_s	 3	/* Call established for outgoing   */
#define		caa_out_s	 4	/* Ditto - in call                 */
#define		dt_in_s		 5	/* Data packets rcvd               */
#define		dt_out_s	 6	/* Data packets sent               */
#define		ed_in_s		 7	/* Interrupts rcvd                 */
#define		ed_out_s	 8	/* Interrupts sent                 */
#define		rnr_in_s	 9	/* Receiver not ready rcvd         */
#define		rnr_out_s	10	/* Receiver not ready sent         */
#define		rr_in_s		11	/* Receiver ready rvcd             */
#define		rr_out_s	12	/* Receiver ready sent             */
#define		prov_rst_in_s	13	/* Provider initiated Resets rcvd  */
#define		rem_rst_in_s	14	/* Remote initiated Resets rcvd    */
#define		rsc_in_s	15	/* Reset confirms rcvd             */
#define		rsc_out_s	16	/* Reset confirms sent             */
#define		prov_clr_in_s	17	/* Provider initiated Clears rcvd  */
#define		clc_in_s	18	/* Clear confirms rcvd             */
#define		clc_out_s	19	/* Clear confirms sent             */
#define		perr_in_s	20	/* Pkts with Protocol Errors rcvd  */
#define		out_vcs_s	21	/* Outgoing Circuits               */
#define		in_vcs_s	22	/* Incoming Circuits               */
#define		twoway_vcs_s	23	/* Two-way  Circuits               */
#define		res_in_s	24	/* Restarts in                     */
#define		res_out_s	25	/* Restarts out                    */
#define		res_timeouts_s	26	/* Restart timeouts                */
#define		cll_timeouts_s	27	/* Call timeouts                   */
#define		rst_timeouts_s	28	/* Reset timeouts                  */
#define		clr_timeouts_s	29	/* Clear timeouts                  */
#define		ed_timeouts_s	30	/* Interrupt timeouts              */
#define		retry_exceed_s	31	/* Retry Count Exceededs           */
#define		clear_exceed_s	32	/* Clear Count Exceededs           */
#define		octets_in_s	33	/* Octets rcvd		           */
#define		octets_out_s	34	/* Octets sent		           */
#define		rec_in_s	35	/* Restart Confirms rcvd           */
#define		rec_out_s	36	/* Restart Confirms sent           */
#define		rst_in_s	37	/* Reset Confirms Sent		   */
#define		rst_out_s	38	/* Reset Confirms rcvd		   */
#define		dg_in_s		39	/* DIAG packets in                 */
#define		dg_out_s	40	/* DIAG packets out                */
#define		res_in_conn_s	41	/* Restarts in connected state     */
#define		clr_in_s	42	/* Clears rcvd            	   */
#define		clr_out_s	43	/* Clears sent             	   */
#define		pkts_in_s	44	/* Packets rcvd            	   */
#define		pkts_out_s	45	/* Packets sent             	   */
#define		vcs_est_s	46	/* SVCs established		   */
#define		max_svcs_s	47	/* Max. no. of SVC opened	   */
#define		svcs_s		48	/* SVCs currently open		   */
#define		pvcs_s		49	/* PVCs currently attached	   */
#define		max_pvcs_s	50	/* Max no PVCs ever attached	   */
#define         rjc_coll_s      51
#define         rjc_failNRS_s   52
#define         rjc_nouser_s    53
#define         rjc_buflow_s    54
#define		reg_in_s	55	/* Registration requests rcvd      */
#define		reg_out_s	56	/* Registration requests sent      */
#define		reg_conf_in_s	57	/* Registration confirms rcvd      */
#define		reg_conf_out_s	58	/* Registration confirms sent      */

#define         snid_mon_size   59


/* Per SNID statistics retrieval record */
struct persnidstats {
	uint32       snid;			/* Subnet id		   */
	int 	     network_state;		/* Network State	   */
	uint32       mon_array[snid_mon_size];	/* L3persnidmonarray	   */
};


/* VIRTUAL CIRCUIT statistics */

#define		cll_in_v	 1	/* Calls rcvd and indicated        */
#define		cll_out_v	 2	/* Calls sent                      */
#define		caa_in_v	 3	/* Call established for outgoing   */
#define		caa_out_v	 4	/* Ditto - in call                 */
#define		dt_in_v		 5	/* Data packets rcvd               */
#define		dt_out_v	 6	/* Data packets sent               */
#define		ed_in_v		 7	/* Interrupts rcvd                 */
#define		ed_out_v	 8	/* Interrupts sent                 */
#define		rnr_in_v	 9	/* Receiver not ready rcvd         */
#define		rnr_out_v	10	/* Receiver not ready sent         */
#define		rr_in_v		11	/* Receiver ready rvcd             */
#define		rr_out_v	12	/* Receiver ready sent             */
#define		rst_in_v	13	/* Resets rcvd                     */
#define		rst_out_v	14	/* Resets sent                     */
#define		rsc_in_v	15	/* Restart confirms rcvd           */
#define		rsc_out_v	16	/* Restart confirms sent           */
#define		clr_in_v	17	/* Clears rcvd                     */
#define		clr_out_v	18	/* Clears sent                     */
#define		clc_in_v	19	/* Clear confirms rcvd             */
#define		clc_out_v	20	/* Clear confirms sent             */
#define		octets_in_v	21	/* Bytes rxed			   */
#define		octets_out_v	22	/* Bytes txed			   */
#define		rst_timeouts_v	23	/* Reset timeouts		   */
#define		ed_timeouts_v	24	/* Interrupt timeouts		   */
#define		prov_rst_in_v	25	/* Provider initiated reset	   */
#define		rem_rst_in_v	26	/* Remote initiated reset	   */

#define         VC_mon_size	27


struct vcinfo {
	struct xaddrf     rem_addr;	/* = called for outward calls */
					/* = caller for inward calls  */
	struct xaddrf     loc_addr;	/* = caller for outward calls */
					/* = called for inward calls  */
	uint32      	  xu_ident;     /* subnetwork id              */
	uint32      	  process_id;	/* effective user id	      */
	unsigned short	  lci;		/* Logical Channel Identifier */
	unsigned char     xstate;       /* VC state                   */
	unsigned char     xtag;         /* VC check record            */
	unsigned char	  ampvc;	/* =1 if a PVC	              */
	unsigned char	  call_direction;
					/* in=0, out=1		      */
	unsigned char	  vctype;
	int		  perVC_stats[VC_mon_size];
					/* Per-VC statistics array    */
};

#define VC_STATES_SIZE   (MAXIOCBSZ - 8)
#define MAX_VC_ENTS      (VC_STATES_SIZE / sizeof(struct vcinfo))

struct vcstatusf {
	unsigned int    first_ent;             /* Where to start search   */
	unsigned int    num_ent;               /* Number entries returned */
	struct vcinfo	vc;		/* Data buffer, extendable by	*/
					/* malloc overlay		*/
};

/* QOS Data Priotity setting and resetting */
struct qosdatpri {
	int		band;
	unsigned int    tx_window;
};
