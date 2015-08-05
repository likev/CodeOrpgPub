/*   @(#)miox_mib.h	1.1	07 Jul 1998	*/

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
 * miox_mib.h of snet module
 *
 * SpiderX25
 * @(#)$Id: miox_mib.h,v 1.1 2000/02/25 17:14:31 john Exp $
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

#define SNMP_MIOXPLEENTRY	2
#define SNMP_MIOXPEERENTRY	3
#define SNMP_MIOXPEERENCENTRY	4

/* values giving the maximum sizes of arrays */

#define MIOX_ID_MAXLEN	32
#define ENCADDR_MAXLEN	128

typedef struct mib_mioxPleEntry {
	int32	version;
	uint32	IfIndex;		/* index of table - not in MIB */
	int32	MaxCircuits;
	uint32	RefusedConnections;
	uint32	EnAddrToX121LkupFlrs;
	unchar	LastFailedEnAddr[ENCADDR_MAXLEN];
	uint32	LastFailedEnAddr_len;
	uint32	EnAddrToX121LkupFlrTime;
	uint32	X121ToEnAddrLkupFlrs;
	unchar	LastFailedX121Address[X121ADDR_MAXLEN];
	uint32	LastFailedX121Address_len;
	uint32	X121ToEnAddrLkupFlrTime;
	uint32	QbitFailures;
	unchar	QbitFailureRemoteAddress[X121ADDR_MAXLEN];
	uint32	QbitFailureRemoteAddress_len;
	uint32	QbitFailureTime;
	uint32	MinimumOpenTimer;
#define MINOPENTIMER_DEF	0
	uint32	InactivityTimer;
#define INACTIVITYTIMER_DEF	10000
	uint32	HoldDownTimer;
#define HOLDDOWNTIMER_DEF	0
	uint32	CollisionRetryTimer;
#define COLRETRYTIMER_DEF	0
	SID_T	DefaultPeerId[MIOX_ID_MAXLEN];
	uint32	DefaultPeerId_len;
} mioxPleEntry_t;

typedef struct mib_mioxPeerEntry {
	int32	version;
	uint32	Index;			/* index of table */
	int32	Status;
#define PEER_STATUS_VALID		1
#define PEER_STATUS_CREATEREQUEST	2
#define PEER_STATUS_UNDERCREATION	3
#define PEER_STATUS_INVALID		4
#define PEER_STATUS_CLEARCALL		5
#define PEER_STATUS_MAKECALL		6
	uint32	MaxCircuits;
#define	MAXPEERCIRC_DEF		1
	uint32	IfIndex;
#define	IFINDEX_DEF		1
	uint32	ConnectSeconds;
	SID_T	X25CallParamId[MIOX_ID_MAXLEN];
	uint32	X25CallParamId_len;
#define	CALLPARAM_DEF		"0.0"
	unchar	EnAddr[ENCADDR_MAXLEN];
	uint32	EnAddr_len;
#define ENCADDR_DEF		""
	unchar	X121Address[X121ADDR_MAXLEN];
	uint32	X121Address_len;
#define X121ADDR_DEF		""
	SID_T	X25CircuitId[MIOX_ID_MAXLEN];
	uint32	X25CircuitId_len;
#define	CIRCID_DEF		"0.0"
#define PEER_DESC_MAXLEN	255
	unchar	Descr[PEER_DESC_MAXLEN];
	uint32	Descr_len;
#define	PEER_DESC_DEF		""
} mioxPeerEntry_t;

typedef struct mib_mioxPeerEncEntry {
	int32	version;
	uint32	PeerIndex;		/* index of table */
	uint32	Index;			/* index of table - not in MIB */
	int32	Type;
} mioxPeerEncEntry_t;
