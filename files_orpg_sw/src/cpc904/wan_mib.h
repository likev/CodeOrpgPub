/*   @(#)wan_mib.h	1.1	07 Jul 1998	*/
/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Smilie and, Ian Lartey
 *
 * wan_mib.h of snet module
 *
 * SpiderX25
 * @(#)$Id: wan_mib.h,v 1.1 2000/02/25 17:15:18 john Exp $
 * 
 * SpiderX25 Release 8
 */


/******************************************************************
 *  SpiderX25 - Wan Board Embedded SNMP Code
 ******************************************************************/

/*
 * The following data was obtained from rfc1317.txt
 */

/*
 * Values for the 'ic_cmd' field of I_STR ioctls.
 * These indicate the variables to be affected.
 * These should be ored with the constants in "snmp.h", which
 * indicate the action to be performed (e.g. get, getnext, test, set).
 */
#define SNMP_RS232_NUMBER		2
#define SNMP_RS232_PORT_ENTRY		3
#define SNMP_RS232_SYNC_PORT_ENTRY	4
#define SNMP_RS232_INSIG_ENTRY		5
#define SNMP_RS232_OUTSIG_ENTRY		6

/*
 * Enumerated Values.
 */

/*
 * Port Types (Used in 'Type' field of the General Port Table Entry).
 */
#define OTHER_PORT	1	
#define RS232_PORT	2
#define RS422_PORT	3
#define RS423_PORT	4
#define V35_PORT	5
#define X21_PORT	6

/*
 * A ports type of flow control.
 * (Used in the 'InFlowType' & 'OutFlowType' fields of the General Port Table 
 * Entry).
 */
#define NO_CONTROL	1
#define CTS_RTS_CONTROL	2
#define DSR_DTR_CONTROL	3

/*
 * Clock Sources (Used in 'ClockSource' field of the Synchronous Port Group
 * Table Entry).
 */
#define INTERNAL_CLOCK	1
#define EXTERNAL_CLOCK	2
#define SPLIT_CLOCK	3

/*
 * The role of the device using a port. (Used in 'Role' field of the
 * Synchronous Port Group Table Entry).
 */
#define DTE		1
#define DCE		2
#define DEF_ROLE	DCE

/*
 * The bit stream encoding technique employed on a port.
 * (Used by the 'Encoding' field of the Synchronous Port Group Table Entry).
 */
#define NRZ		1
#define NRZI		2
#define DEF_ENCODING	NRZ

/*
 * Used to control the RTS signal.
 * (Used by the 'RTSControl' field of the Synchronous Port Group Table Entry).
 */
#define CONTROLLED	1
#define CONSTANT	2
#define DEF_CONTROL	CONSTANT

/*
 * Default delay in milliseconds that the DCE must wait after it sees
 * RTS asserted before asserting CTS.
 * Used in 'RTSCTSDelay' field of Synchronous Port Group Table Entry).
 */
#define DEF_RTSCTSDELAY	0

/*
 * The mode and operation of a port with respect to the direction
 * and the simultaniety of data transfer.
 * (Used by the 'Mode' field of the Synchronous Port Group Table Entry).
 */
#define FULL_DUPLEX		1
#define HALF_DUPLEX		2
#define SIMPLEX_RECEIVE		3
#define SIMPLEX_TRANSMIT	4
#define DEF_MODE		FULL_DUPLEX

/*
 * The pattern used to indicate an idle line.
 * (Used by the 'IdlePattern' field of the Synchronous Port Group Table Entry).
 */
#define MARK_IDLE	1
#define SPACE_IDLE	2
#define DEF_IDLE	SPACE_IDLE

/*
 * The minimum number of flag patterns required after the EOF before the
 * start of the next frame will be recognized.
 * (Used in 'MinFlags' field of Synchronous Port Group Table Entry).
 */
#define ONE_FLAG	1
#define TWO_FLAGS	2
#define DEF_FLAGS	TWO_FLAGS
/*
 * Signal Types (Used in the 'Name' field of the Input/Output Control Signal
 * Status Table Entries).
 */
#define RTS_SIGNAL	1
#define CTS_SIGNAL	2
#define DSR_SIGNAL	3
#define DTR_SIGNAL	4
#define RI_SIGNAL	5
#define DCD_SIGNAL	6
#define SQ_SIGNAL	7
#define SRS_SIGNAL	8
#define SRTS_SIGNAL	9
#define SCTS_SIGNAL	10
#define SDCD_SIGNAL	11

/*
 * Signal States (Used in the 'State' field of the Input/Output Control Signal
 * Status Table Entries).
 */
#define SIG_STATE_NONE	1
#define SIG_STATE_ON	2
#define SIG_STATE_OFF	3

/*
 * MIB structures
 */

/*
 * Number of ports in the General Port Table
 */
typedef struct mib_rs232 {
	int32	version;
	int32	Number;
} mib_rs232_t;

/*
 * General Port Table Entry
 */
typedef struct mib_rs232PortEntry {
	int32	version;
	int32	Index;
	int32	Type;
	int32	InSigNumber;
	int32	OutSigNumber;
	int32	InSpeed;
	int32	OutSpeed;
	int32	InFlowType;
	int32	OutFlowType;
} mib_rs232PortEntry_t;

#if 0
/*
 * The Asynchronous Port Group is *** NOT IMPLEMENTED ***
 */
typedef struct mib_rs232AsyncPortEntry {
	int32	version;
	int32	Index;
	int32	Bits;
	int32	StopBits;
	int32	Parity;
	int32	Autobaud;
	uint32	ParityErrs;
	uint32	FramingErrs;
	uint32	OverrunErrs;
} mib_rs232AsyncPortEntry_t;
#endif /* 0 */

/*
 * The Synchronous Port Group Table Entry
 */
typedef struct mib_rs232SyncPortEntry {
	int32	version;
	int32	Index;
	int32	ClockSource;
	uint32	FrameCheckErrs;
	uint32	TransmitUnderrunErrs;
	uint32	ReceiveOverrunErrs;
	uint32	InterruptedFrames;
	uint32	AbortedFrames;
	int32	Role;
	int32	Encoding;
	int32	RTSControl;
	int32	RTSCTSDelay;
	int32	Mode;
	int32	IdlePattern;
	int32	MinFlags;
} mib_rs232SyncPortEntry_t;

/*
 * Input Control Signal Status for a Hardware Port Table Entry
 */
typedef struct mib_rs232InSigEntry {
	int32	version;
	int32	PortIndex;
	int32	Name;
	int32	State;
	uint32	Changes;
} mib_rs232InSigEntry_t;


/*
 * Output Control Signal Status for a Hardware Port Table Entry
 */
typedef struct mib_rs232OutSigEntry {
	int32	version;
	int32	PortIndex;
	int32	Name;
	int32	State;
	uint32	Changes;
} mib_rs232OutSigEntry_t;
