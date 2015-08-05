/*   @(#)lapb_mib.h	1.1	07 Jul 1998	*/

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
 * lapb_mib.h of snet module
 *
 * SpiderX25
 * @(#)$Id: lapb_mib.h,v 1.1 2000/02/25 17:14:26 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
 * Values for the 'ic_cmd' field of I_STR ioctls.
 * These indicate the variables to be affected.
 * These should be ored with the constants in "snmp.h", which
 * indicate the action to be performed (e.g. get, getnext, test, set).
 */

#define SNMP_LAPBOPERENTRY	3
#define SNMP_LAPBFLOWENTRY	4
#ifdef LAPB_XID_MIB
#define SNMP_LAPBXIDENTRY	5
#endif /* LAPB_XID_MIB */

/*
 * Enumerated values
 */
#define STATTYPE_DTE	1
#define STATTYPE_DCE	2
#define STATTYPE_DXE	3

#define CONTFIELD_MOD8	1
#define CONTFIELD_MOD128 2
/*
 * Note that the following structure is used to interface with data in
 * a file, and not a STREAMS driver.
 */
typedef struct mib_lapbAdmnEntry {
	int32	version;
	uint32	Index;			/* index of table */
	int32	StationType;
#define STATTYPE_DEF	STATTYPE_DTE
	int32	ControlField;
#define	CONTFIELD_DEF	CONTFIELD_MOD8
	int32	TransmitN1FrameSize;
#define	TRANSN1_DEF	36000
	int32	ReceiveN1FrameSize;
#define	RECVN1_DEF	36000
	int32	TransmitKWindowSize;
#define	TRANSK_DEF	7
	int32	ReceiveKWindowSize;
#define	RECVK_DEF	7
	int32	N2RxmitCount;
#define N2_DEF		20
	int32	T1AckTimer;
#define T1_DEF		3000
	int32	T2AckDelayTimer;
#define T2_DEF		0
	int32	T3DisconnectTimer;
#define T3_DEF		60000
	int32	T4IdleTimer;
#define T4_DEF		2147483647
	int32	ActionInitiate;
#define	ACTI_SENDSABM	1	
#define	ACTI_SENDDISC	2
#define	ACTI_SENDDM	3
#define	ACTI_NONE	4
#define	ACTI_OTHER	5
#define	ACTI_DEF	ACT_SENDSABM
	int32	ActionRecvDM;
#define	ACTR_SENDSABM	1	
#define	ACTR_SENDDISC	2
#define	ACTR_OTHER	3
#define	ACTR_DEF	ACTR_SENDSABM
} lapbAdmnEntry_t;

typedef struct mib_lapbOperEntry {
	int32	version;
	uint32	Index;			/* index of table */
	int32	StationType;
	int32	ControlField;
	int32	TransmitN1FrameSize;
	int32	ReceiveN1FrameSize;
	int32	TransmitKWindowSize;
	int32	ReceiveKWindowSize;
	int32	N2RxmitCount;
	int32	T1AckTimer;
	int32	T2AckDelayTimer;
	int32	T3DisconnectTimer;
	int32	T4IdleTimer;
#define PORTID_MAXLEN	32
	SID_T	PortId[PORTID_MAXLEN];
	uint32	PortId_len;
#define PVID_MAXLEN	32
	SID_T	ProtocolVersionId[PVID_MAXLEN];
	uint32	ProtocolVersionId_len;
} lapbOperEntry_t;

#define FRMR_IF_MAXLEN	7

typedef struct mib_lapbFlowEntry {
	int32	version;
	uint32	IfIndex;		/* index of table */
	uint32	StateChanges;
	int32	ChangeReason;
#define	REASON_NOTSTARTED	1
#define	REASON_ABMENTERED	2
#define	REASON_ABMEENTERED	3
#define	REASON_ABMRESET		4
#define	REASON_ABMERESET	5
#define	REASON_DMRECEIVED	6
#define	REASON_DMSENT		7
#define	REASON_DISCRECEIVED	8
#define	REASON_DISCSENT		9
#define	REASON_FRMRRECEIVED	10
#define	REASON_FRMRSENT		11
#define	REASON_N2TIMEOUT	12
#define	REASON_OTHER		13
	int32	CurrentMode;
#define	MODE_DISCONNECTED	1
#define	MODE_LINKSETUP		2
#define	MODE_FRAMEREJECT	3
#define	MODE_DISCONNECTREQUEST	4
#define	MODE_INFOTRANSFER	5
#define	MODE_REJFRAMESENT	6
#define	MODE_WAITINGACK		7
#define	MODE_STATIONBUSY	8
#define	MODE_REMSTATIONBUSY	9
#define	MODE_BOTHSTATIONBUSY	10
#define	MODE_WACKSTATIONBUSY	11
#define	MODE_WACKREMBUSY	12
#define	MODE_WACKABOTHBUSY	13
#define	MODE_REJFRSENTREMBUSY	14
#define	MODE_XIDFRAMESENT	15
#define	MODE_ERROR		16
#define	MODE_OTHER		17
	uint32	BusyDefers;
	uint32	RejOutPkts;
	uint32	RejInPkts;
	uint32	T1Timeouts;
	uint8	FrmrSent[FRMR_IF_MAXLEN];
	uint8	pad;
	uint32	FrmrSent_len;
	uint8	FrmrReceived[FRMR_IF_MAXLEN];
	uint8	pad2;
	uint32	FrmrReceived_len;
	uint32	XidReceived_len;
#define	XIDRECV_MAXLEN	8206
	/*
	 * Due to the large potential size of XidReceived, only
	 * the actual size is returned (no padding to max size)
	 */
	uint8	XidReceived[1];
	uint8	pad3[3];
} lapbFlowEntry_t;

#ifdef LAPB_XID_MIB
#define XID_MAXLEN	255

typedef struct mib_lapbXidEntry {
	int32	version;
	uint32	Index;			/* index of table */
	uint8	AdRIdentifier[XID_MAXLEN];
	uint32	AdRIdentifier_len;
#define ADRID_DEF	""
	uint8	AdRAddress[XID_MAXLEN];
	uint32	AdRAddress_len;
#define ADRAD_DEF	""
	uint8	ParameterUniqueIdentifier[XID_MAXLEN];
	uint32	ParameterUniqueIdentifier_len;
#define PARAMUID_DEF	""
	uint8	GroupAddress[XID_MAXLEN];
	uint32	GroupAddress_len;
#define GROUPAD_DEF	""
	uint8	PortNumber[XID_MAXLEN];
	uint32	PortNumber_len;
#define PORTNO_DEF	""
	/* note: UserDataSubfield is not sent up in this message */
#define	UDATASUB_MAXLEN	8206
	/*
	 * Due to the large potential size of UserDataSubfield, only
	 * the actual size is returned (no padding to max size)
	 */
#define UDATASUB_DEF	""
	uint32	UserDataSubfield_len;
	uint8	UserDataSubfield[1];
} lapbXidEntry_t;

#endif /* LAPB_XID_MIB */

/* the prefix for protocol Version Identifiers */
#define lapbProtocolVersion		"1.3.6.1.2.1.10.16.5"
#define lapbProtocolIso7776v1986	1
#define lapbProtocolCcittV1980		2
#define lapbProtocolCcittV1984		3
