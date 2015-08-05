/*   @(#)x25_mib.h	1.1	07 Jul 1998	*/

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
 * x25_mib.h of snet module
 *
 * SpiderX25
 * @(#)$Id: x25_mib.h,v 1.1 2000/02/25 17:15:23 john Exp $
 * 
 * SpiderX25 Release 8
 */

#include "uint.h"

/*
 * Values for the 'ic_cmd' field of I_STR ioctls.
 * These indicate the variables to be affected.
 * These should be ored with the constants in "snmp.h", which
 * indicate the action to be performed (e.g. get, getnext, test, set).
 */

#define SNMP_X25OPERENTRY		2
#define	SNMP_X25STATENTRY		3
#define SNMP_X25CHANNELENTRY		4
#define	SNMP_X25CIRCUITENTRY		5
#define	SNMP_X25			6
#define	SNMP_X25CLEAREDCIRCUITENTRY	7
#define SNMP_X25CALLPARMENTRY		8

#define CPARM_CREATE_REREF		9
#define CPARM_UNREF			10

/* values giving the maximum sizes of arrays */

#define CALLPARM_MAXLEN		32
#define X121ADDR_MAXLEN		17
#define PVSUP_MAXLEN		32
#define FACILITIES_MAXLEN	109

/*
 * the prefix for a reference into the call parm and
 * circuit tables. Add the index.
 */
#define CALLPARM_PREFIX	"1.3.6.1.2.1.10.5.9.1.1"
#define CIRCUIT_PREFIX  "1.3.6.1.2.1.10.5.5.1.1"

/*
 * Enumerated values
 */

#define IFMODE_DTE	1
#define IFMODE_DCE	2
#define IFMODE_DXE	3

#define PKTSEQ_MOD8	1
#define PKTSEQ_MOD128	2

/*
 * Note that the following structure is used to interface with data in
 * a file, and not a STREAMS driver.
 */
typedef struct mib_x25AdmnEntry {
	int32	version;
	uint32	Index;			/* index of table */
	int32	InterfaceMode;
	int32	MaxActiveCircuits;
	int32	PacketSequencing;
	uint32	RestartTimer;
	uint32	CallTimer;
	uint32	ResetTimer;
	uint32	ClearTimer;
	uint32	WindowTimer;
	uint32	DataRxmtTimer;
	uint32	InterruptTimer;
	uint32	RejectTimer;
	uint32	RegistrationRequestTimer;
	uint32	MinimumRecallTimer;
	int32	RestartCount;
	int32	ResetCount;
	int32	ClearCount;
	int32	DataRxmtCount;
	int32	RejectCount;
	int32	RegistrationRequestCount;
	int32	NumberPVCs;
	SID_T	DefCallParamId[CALLPARM_MAXLEN];
	uint32	DefCallParamId_len;
	unchar	LocalAddress[X121ADDR_MAXLEN];
	uint32	LocalAddress_len;
	SID_T	ProtocolVersionSupported[PVSUP_MAXLEN];
	uint32	ProtocolVersionSupported_len;
} x25AdmnEntry_t;

typedef struct mib_x25OperEntry {
	int32	version;
	uint32	Index;			/* index of table */
	int32	InterfaceMode;
	int32	MaxActiveCircuits;
	int32	PacketSequencing;
	uint32	RestartTimer;
	uint32	CallTimer;
	uint32	ResetTimer;
	uint32	ClearTimer;
	uint32	WindowTimer;
	uint32	DataRxmtTimer;
	uint32	InterruptTimer;
	uint32	RejectTimer;
	uint32	RegistrationRequestTimer;
	uint32	MinimumRecallTimer;
	int32	RestartCount;
	int32	ResetCount;
	int32	ClearCount;
	int32	DataRxmtCount;
	int32	RejectCount;
	int32	RegistrationRequestCount;
	int32	NumberPVCs;
	SID_T	DefCallParamId[CALLPARM_MAXLEN];
	uint32	DefCallParamId_len;
	unchar	LocalAddress[X121ADDR_MAXLEN];
	uint32	LocalAddress_len;
#define DLID_MAXLEN	32
	SID_T	DataLinkId[DLID_MAXLEN];
	uint32	DataLinkId_len;
	SID_T	ProtocolVersionSupported[PVSUP_MAXLEN];
	uint32	ProtocolVersionSupported_len;
} x25OperEntry_t;

typedef struct mib_x25StatEntry {
	int32	version;
	uint32	Index;			/* index of table */
	uint32	InCalls;
	uint32	InCallRefusals;
	uint32	InProviderInitiatedClears;
	uint32	InRemotelyInitiatedResets;
	uint32	InProviderInitiatedResets;
	uint32	InRestarts;
	uint32	InDataPackets;
	uint32	InAccusedOfProtocolErrors;
	uint32	InInterrupts;
	uint32	OutCallAttempts;
	uint32	OutCallFailures;
	uint32	OutInterrupts;
	uint32	OutDataPackets;
	uint32	OutgoingCircuits;
	uint32	IncomingCircuits;
	uint32	TwowayCircuits;
	uint32	RestartTimeouts;
	uint32	CallTimeouts;
	uint32	ResetTimeouts;
	uint32	ClearTimeouts;
	uint32	DataRxmtTimeouts;
	uint32	InterruptTimeouts;
	uint32	RetryCountExceededs;
	uint32	ClearCountExceededs;
} x25StatEntry_t;

typedef struct mib_x25ChannelEntry {
	int32	version;
	uint32	Index;			/* index of table */
	int32	LIC;
	int32	HIC;
	int32	LTC;
	int32	HTC;
	int32	LOC;
	int32	HOC;
} x25ChannelEntry_t;

typedef struct mib_x25CircuitEntry {
	int32	version;
	uint32	Index;			/* index of table */
	int32	Channel;
	int32	Status;
#define	CIRC_STATUS_INVALID		1
#define	CIRC_STATUS_CLOSED		2
#define	CIRC_STATUS_CALLING		3
#define	CIRC_STATUS_OPEN		4
#define	CIRC_STATUS_CLEARING		5
#define	CIRC_STATUS_PVC			6
#define	CIRC_STATUS_PVCRESETTING	7
#define	CIRC_STATUS_STARTCLEAR		8
#define	CIRC_STATUS_STARTPVCRESET	9
#define	CIRC_STATUS_OTHER		10
	uint32	EstablishTime;
	int32	Direction;
#define	DIRECTION_INCOMING	1
#define	DIRECTION_OUTGOING	2
#define	DIRECTION_PVC		3
#define	DIRECTION_DEF		DIRECTION_PVC
	uint32	InOctets;
	uint32	InPdus;
	uint32	InRemotelyInitiatedResets;
	uint32	InProviderInitiatedResets;
	uint32	InInterrupts;
	uint32	OutOctets;
	uint32	OutPdus;
	uint32	OutInterrupts;
	uint32	DataRetransmissionTimeouts;
	uint32	ResetTimeouts;
	uint32	InterruptTimeouts;
	SID_T	CallParamId[CALLPARM_MAXLEN];
	uint32	CallParamId_len;
#define	CALLPARM_DEF	"0.0"
	unchar	CalledDteAddress[X121ADDR_MAXLEN];
	uint32	CalledDteAddress_len;
#define	CALLEDDTE_DEF	""
	unchar	CallingDteAddress[X121ADDR_MAXLEN];
	uint32	CallingDteAddress_len;
#define	CALLINGDTE_DEF	""
	unchar	OriginallyCalledAddress[X121ADDR_MAXLEN];
	uint32	OriginallyCalledAddress_len;
#define	ORIGDTE_DEF	""
#define CIRC_DESC_MAXLEN	255
	unchar	Descr[CIRC_DESC_MAXLEN];
	uint32	Descr_len;
#define	CIRC_DESC_DEF	""
} x25CircuitEntry_t;

typedef struct mib_x25 {
	int32	version;
	uint32	ClearedCircuitEntriesRequested;
	uint32	ClearedCircuitEntriesGranted;
} x25_t;

typedef struct mib_x25ClearedCircuitEntry {
	int32	version;
	uint32	Index;			/* index of table */
	uint32	PleIndex;
	uint32	TimeEstablished;
	uint32	TimeCleared;
	int32	Channel;
	int32	ClearingCause;
	int32	DiagnosticCode;
	uint32	InPdus;
	uint32	OutPdus;
	unchar	CalledAddress[X121ADDR_MAXLEN];
	uint32	CalledAddress_len;
	unchar	CallingAddress[X121ADDR_MAXLEN];
	uint32	CallingAddress_len;
	unchar	ClearFacilities[FACILITIES_MAXLEN];
	uint32	ClearFacilities_len;
} x25ClearedCircuitEntry_t;

#define CUG_MAXLEN	4
#define EXT_MAXLEN	40
#define FAC_MAXLEN	108

#define	THRUPUT_TCRESERVED1	1
#define	THRUPUT_TCRESERVED2	2
#define	THRUPUT_TC75		3
#define	THRUPUT_TC150		4
#define	THRUPUT_TC300		5
#define	THRUPUT_TC600		6
#define	THRUPUT_TC1200		7
#define	THRUPUT_TC2400		8
#define	THRUPUT_TC4800		9
#define	THRUPUT_TC9600		10
#define	THRUPUT_TC19200		11
#define	THRUPUT_TC48000		12
#define	THRUPUT_TC64000		13
#define	THRUPUT_TCRESERVED14	14
#define	THRUPUT_TCRESERVED15	15
#define	THRUPUT_TCRESERVED0	16
#define	THRUPUT_TCNONE		17
#define	THRUPUT_TCDEFAULT	18
#define	THRUPUT_DEF		THRUPUT_TCNONE

typedef struct mib_x25CallParmEntry {
	int32	version;
	uint32	Index;			/* index of table */
	uint32	Status;
#define	CP_VALID		1
#define	CP_CREATE_REQUEST	2
#define	CP_UNDER_CREATION	3
#define	CP_INVALID		4
	uint32	RefCount;
	int32	InPacketSize;
#define	INPKTSIZE_DEF		128
	int32	OutPacketSize;
#define	OUTPKTSIZE_DEF		128
	int32	InWindowSize;
#define	INWINSIZE_DEF		2
	int32	OutWindowSize;
#define	OUTWINSIZE_DEF		2
	int32	AcceptReverseCharging;
#define	ACCREV_DEFAULT		1
#define	ACCREV_ACCEPT		2
#define	ACCREV_REFUSE		3
#define	ACCREV_NEVERACCEPT	4
#define	ACCREV_DEF		ACCREV_REFUSE
	int32	ProposeReverseCharging;
#define	PROPREV_DEFAULT		1
#define	PROPREV_REVERSE		2
#define	PROPREV_LOCAL		3
#define	PROPREV_DEF		PROPREV_LOCAL
	int32	FastSelect;
#define	FS_DEFAULT		1
#define	FS_NOTSPECIFIED		2
#define	FS_FASTSELECT		3
#define	FS_RESTRICTEDFR		4
#define	FS_NOFASTSELECT		5
#define	FS_NORESTRICTEDFR	6
#define	FS_DEF			FS_NOFASTSELECT
	int32	InThruPutClasSize;
	int32	OutThruPutClasSize;
	unchar	Cug[CUG_MAXLEN];
	uint32	Cug_len;
#define	CUG_DEF			""
	unchar	Cugoa[CUG_MAXLEN];
	uint32	Cugoa_len;
#define	CUGOA_DEF		""
#define BCUG_MAXLEN		4
	unchar	Bcug[BCUG_MAXLEN];
	uint32	Bcug_len;
#define	BCUG_DEF		""
#define NUI_MAXLEN	108
	unchar	Nui[NUI_MAXLEN];
	uint32	Nui_len;
#define	NUI_DEF			""
	int32	ChargingInfo;
#define	CHARGE_DEFAULT		1
#define	CHARGE_NOFACILITY	2
#define	CHARGE_NOCHARGINGINFO	3
#define	CHARGE_CHARGINGINFO	4
#define	CHARGE_DEF		CHARGE_NOFACILITY
#define RPOA_MAXLEN	108
	unchar	Rpoa[RPOA_MAXLEN];
	uint32	Rpoa_len;
#define	RPOA_DEF		""
	int32	TrnstDly;
#define	TRNSTDLY_DEF		65536
	unchar	CallingExt[EXT_MAXLEN];
	uint32	CallingExt_len;
#define	CALLINGEXT_DEF		""
	unchar	CalledExt[EXT_MAXLEN];
	uint32	CalledExt_len;
#define	CALLEDEXT_DEF		""
	int32	InMinThuPutCls;
#define	INMINTH_DEF		17
	int32	OutMinThuPutCls;
#define	OUTMINTH_DEF		17
#define TDELAY_MAXLEN	6
	unchar	EndTrnsDly[TDELAY_MAXLEN];
	uint32	EndTrnsDly_len;
#define	TDELAY_DEF		""
#define PRIORITY_MAXLEN	6
	unchar	Priority[PRIORITY_MAXLEN];
	uint32	Priority_len;
#define	PRIORITY_DEF		""
#define PROT_MAXLEN	108
	unchar	Protection[PROT_MAXLEN];
	uint32	Protection_len;
#define	PROT_DEF		""
	int32	ExptData;
#define	ED_DEFAULT		1
#define	ED_NOEXPTDATA		2
#define	ED_EXPTDATA		3
#define	ED_DEF			ED_NOEXPTDATA
#define UDATA_MAXLEN		128
	unchar	UserData[UDATA_MAXLEN];
	uint32	UserData_len;
#define	UDATA_DEF		""
	unchar	CallingNetworkFacilities[FAC_MAXLEN];
	uint32	CallingNetworkFacilities_len;
#define CALLINGFAC_DEF		""
	unchar	CalledNetworkFacilities[FAC_MAXLEN];
	uint32	CalledNetworkFacilities_len;
#define CALLEDFAC_DEF		""
} x25CallParmEntry_t;

struct cparm_create_reref
{
	struct mib_x25CallParmEntry entry;
	uint32 oIndex;
};

struct xcparmrereff {
        unsigned char             xl_type;    /* Always XL_INFO */
        unsigned char             xl_command; /* Always M_CParmReRef */ 
        struct cparm_create_reref cparm_ref;
};

struct xcparmunreff {
        unsigned char    xl_type;             /* Always XL_INFO */
        unsigned char    xl_command;          /* Always M_CParmUnRef */ 
        uint32           index; 
};

struct xcircf {
        unsigned char    xl_type;       /* Always XL_INFO */
        unsigned char    xl_command;    /* Always M_CircuitId */
        uint32           index;	
        uint32           channel;
};

/* the prefix for protocol Version Identifiers */
#define	x25ProtocolVersion	"1.3.6.1.2.1.10.5.10"
#define x25protocolCcittV1976	1
#define x25protocolCcittV1980	2
#define x25protocolCcittV1984	3
#define x25protocolCcittV1988	4
#define x25protocolIso8208V1987	5
#define x25protocolIso8208V1989	6

/* protocol states/types */
#define D_ifOperStatusUp	1
#define D_ifOperStatusDown	2
#define D_ifType_ddn_x25	4
#define D_ifType_rfc877_x25	5
