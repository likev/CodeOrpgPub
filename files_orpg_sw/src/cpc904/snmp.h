/*   @(#)snmp.h	1.2	08 Oct 1999	*/
/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * Authors: Gavin Shearer, Peter Woodhouse
 *
 * @(#)$Id: snmp.h,v 1.1 2000/02/25 17:14:57 john Exp $
 * 
 */

#include "uint.h"

#ifndef SID_T
#define SID_T	uint32 /* type of subidentifiers */
#endif /* ~SID_T */

/*
 * Values for the 'ic_cmd' field of I_STR ioctls.
 * These indicate the request to be performed.
 * These should be ORed with the constants supplied below and in
 * the *_mib.h files, which specify the variables on which the
 * request should be performed.
 */

#define SNMPIOC			('M' << 8)

#define SNMP_GET_REQ		(SNMPIOC | (0 << 5))
#define SNMP_GETNEXT_REQ	(SNMPIOC | (1 << 5))
#define SNMP_SET_REQ		(SNMPIOC | (3 << 5))	/* old style set */
#define	SNMP_ITEST_REQ		(SNMPIOC | (4 << 5))
#define	SNMP_ISET_REQ		(SNMPIOC | (5 << 5))
#define	SNMP_MGET_REQ		(SNMPIOC | (6 << 5))

#define SNMP_REQ_MASK		(SNMPIOC | (7 << 5))

/*
 * Values for the 'ic_cmd' field of I_STR ioctls.
 * These indicate the variables to be affected.
 * These should be ORed with the constants above, which specify
 * the type of request.
 */

#define	SNMP_IFENTRY		1

/*
 * The following value is a mask which can be used to extract the
 * variable type of an 'ic_cmd' type.
 */

#define SNMP_VAR_MASK		31

/*
 * Values for the 'ic_cmd' field of I_STR ioctls.
 * This indicates that an SNMP control message 
 * is being sent.
 */

#define SNMP_CONTROL		(SNMPIOC | (7 << 5) | 0)

/*
 * init structure for SNMP
 */

struct snmp_init
{
	uint8 prim_type;
	uint8 pad[3];
	uint32 since;		/* in 100ths of a second */
};

#define SNMP_INIT		1

/*
 * interface registration/unregistration structure
 */

#define ID_MAXLEN		32

struct snmp_reg
{
	uint8 prim_type;	/* SNMP_REG or SNMP_UNREG */
	uint8 pad[3];
	uint32 ident;		/* identifier of interface */
	uint32 IfIndex;		/* interface number */
	SID_T ObjId[ID_MAXLEN];	/* lower driver's OID */
	uint32 ObjId_len;	/* length of lower driver's OID */
};

#define SNMP_REG		2
#define SNMP_UNREG		3	/* future: for dynamic reconfig */

/*
 * SNMP_ITEST_REQ and SNMP_ISET_REQ requests contain the following
 * "var_info" structure (below) which gives the offset from the start
 * of the group structure of the variable to be "test"ed of "set".
 * The structure should be followed by the group structure, with
 * the "index" fields and the value to set/test filled in.
 */

struct var_info		/* used in SNMP_ITEST_REQ and SNMP_ISET_REQ */
{
	uint32 version;		/* version number */
	uint32 offset;		/* offset relative to full structure */
};

/*
 * The following structure is used when doing an SNMP_MGET, which
 * retrieves a complete table.
 */

struct snmp_mget
{
	uint32 entry_count;	/* num of entries returned */
	uint32 error;		/* if set, indicates error -
				   couldn't retrieve entire table */
};

/*
 * The following macros calculates the offset of a field in a structure.
 *
 * The first is for arrays, and the second is for other field types.
 */
#define A_OFFSET(structure, field) ((uint32)(((struct structure *)0)->field))
#ifndef VXWORKS
#define OFFSET(structure, field) ((uint32)(&((struct structure *)0)->field))
#endif /* VXWORKS */

/*
 * The following macro calculates the size of a field in a structure
 */
#define SIZE(structure, field) (sizeof(((struct structure *)0)->field))

/*
 * Following is the structure and defines used to extract an interface entry
 */
#define		OBJIDLEN	32
#define		IFDESCRLEN	256
#define		IFPHYSADDRLEN	64

typedef struct mib_ifEntry {
	int32	version;		/* version number of the MIB	     */
	int32	ifIndex;		/* index of this interface           */
	char	ifDescr[IFDESCRLEN];	/* English description of interface  */
	int32	ifType;			/* network type of device            */
	int32	ifMtu;			/* size of largest packet in bytes   */
	uint32	ifSpeed;		/* bandwidth in bits/sec             */
	unchar	ifPhysAddress[IFPHYSADDRLEN];	/* interface's address       */
	unchar	PhysAddrLen;		/* length of physAddr                */
	int32	ifAdminStatus;		/* desired state of interface        */
	int32	ifOperStatus;		/* current operational status        */
	uint32	ifLastChange;		/* sysUpTime when curr state entered */
	uint32	ifInOctets;		/* # octets received on interface    */
	uint32	ifInUcastPkts;		/* # unicast packets delivered       */
	uint32	ifInNUcastPkts; 	/* # broadcasts or multicasts        */
	uint32	ifInDiscards;		/* # packets discarded with no error */
	uint32	ifInErrors;		/* # packets containing errors       */
	uint32	ifInUnknownProtos;	/* # packets with unknown protocol   */
	uint32	ifOutOctets;		/* # octets transmittedwn protocol   */
	uint32	ifOutUcastPkts; 	/* # unicast packets sent protocol   */
	uint32	ifOutNUcastPkts;	/* # broadcast or multicast pkts     */
	uint32	ifOutDiscards;		/* # packets discarded with no error */
	uint32	ifOutErrors;		/* # pkts discarded with an error    */
	uint32	ifOutQLen;		/* # packets in output queue         */
	SID_T	ifSpecific[OBJIDLEN];	/* object ID of media specific info  */
	uint32  ifSpecificLen;		/* length of object ID               */
} ifEntry_t;

#define MIB_IFTYPE_OTHER		1
#define MIB_IFTYPE_REGULAR1822		2
#define MIB_IFTYPE_HDH1822		3
#define MIB_IFTYPE_DDNX25		4
#define MIB_IFTYPE_RFC877X25		5
#define MIB_IFTYPE_ETHERNETCSMACD	6
#define MIB_IFTYPE_ISO88023CSMACD	7
#define MIB_IFTYPE_ISO88024TOKENBUS	8
#define MIB_IFTYPE_ISO88025TOKENRING	9
#define MIB_IFTYPE_ISO88026MAN		10
#define MIB_IFTYPE_STARLAN		11
#define MIB_IFTYPE_PROTEON10MBIT	12
#define MIB_IFTYPE_PROTEON80MBIT	13
#define MIB_IFTYPE_HYPERCHANNEL		14
#define MIB_IFTYPE_FDDI			15
#define MIB_IFTYPE_LAPB			16
#define MIB_IFTYPE_SDLC			17
#define MIB_IFTYPE_T1CARRIER		18
#define MIB_IFTYPE_CEPT			19
#define MIB_IFTYPE_BASICISDN		20
#define MIB_IFTYPE_PRIMARYISDN		21
#define MIB_IFTYPE_PROPPNTTOPNTSERIAL	22
#define MIB_IFTYPE_PPP			23
#define MIB_IFTYPE_SOFTWARE_LOOPBACK	24
#define MIB_IFTYPE_EON			25
#define MIB_IFTYPE_ETHERNET3MBIT	26
#define MIB_IFTYPE_NSIP			27
#define MIB_IFTYPE_SLIP			28
#define MIB_IFTYPE_ULTRA		29
#define MIB_IFTYPE_DS3			30
#define MIB_IFTYPE_SIP			31
#define MIB_IFTYPE_FRAMERELAY		32

#define MIB_IFSTATUS_UP			1
#define MIB_IFSTATUS_DOWN		2
#define MIB_IFSTATUS_TESTING		3
