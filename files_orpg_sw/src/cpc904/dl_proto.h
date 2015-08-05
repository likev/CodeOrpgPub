/*   @(#)dl_proto.h	1.1	07 Jul 1998	*/
/*
 *  Spider STREAMS Data Link Interface Primitives
 *
 *  Copyright (c) 1993 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Authors: Ian Heavens, Gavin Shearer, Nick Felisiak
 *
 *  @(#)$Spider: dl_proto.h,v 1.2 1997/05/23 19:51:21 mark Exp $
 */

#include "dl_version.h"

#if DL_VERSION == 0	/* externally defined */
/*
 *  Generic Ethernet Driver interface.  Only the names have changed
 *  to protect the innocent.  This is defined to be Version 0 of the
 *  Spider Data Link protocol.
 */
#endif
#if DL_VERSION == 1
/*
 *  This protocol interface is derived and generalised from the
 *  Version 1 of the protocol.  The main restriction in this
 *  version is that all hardware addresses must be of length 6.
 */
#endif
#if DL_VERSION == 2
/*
 *  This defines Version 2 of Spider's STREAMS Data Link protocol.
 *  Its main feature is its ability to cope with hardware addresses
 *  of length not equal to 6.
 */
#endif

/*
 *  Primitive type values.
 */

/* note: may also need DL_VERSION_0, etc. for parcpp to cope (bds) */

#define DATAL_TYPE	'R'		/* data link registration */ 
#define DATAL_VERSION0	DATAL_TYPE	/* Version 0 response */
#define DATAL_RESPONSE	'D'		/* Version > 0 response */
#define DATAL_PARAMS	'P'		/* data link parameters */
#define DATAL_TX	't'		/* packet for transmission */
#define DATAL_RX	'r'		/* incoming packet */

#if DL_VERSION == 0
/* these make the interface work for users of the old ethernet interface */
#define ETH_TYPE	DATAL_TYPE
#define ETH_PARAMS	DATAL_PARAMS
#define eth_params	datal_params
#define ETH_TX		DATAL_TX
#define eth_tx		datal_tx
#define ETH_RX		DATAL_RX
#define eth_rx		datal_rx
#define aux_type	version
#define eth_type	datal_type
#define eth_src		src
#define eth_dst		dst
#define ethaddr		addr
#define eth_addr	datal_addr
#define eparm		dlparm
#define erx		dlrx
#define etx		dltx

#define eth_proto	datal_proto

#define S_ETH_TYPE S_DATAL_TYPE
#define S_ETH_RX S_DATAL_RX
#define S_ETH_TX S_DATAL_TX

#define dl_type type
#define dl_lwb lwb
#define dl_upb upb
#define dl_ethaddr addr
#define dl_frgsz frgsz
#define DL_TYPE	ETH_TYPE	/* ethernet registration (old style) */
#define S_DL_TYPE S_ETH_TYPE

#endif /*DL_VERSION*/

/*
 *  Data Link registration.
 *
 *  Currently, the DATAL_TYPE message is always the first to be sent to
 *  the Data Link driver.  The Data Link driver responds either with a
 *  DATAL_RESPONSE message, or with a DATAL_VERSION0 message (this is the
 *  case for older drivers).
 *
 *  The upper user must remember the version number of the protocol
 *  returned in the DATAL_RESPONSE response, or assume version 0 in the
 *  case of an DATAL_VERSION0 response, since the version number is not
 *  currently guaranteed set in other returned messages.
 */

typedef struct datal_response {
	uint8 prim_type;		/* i.e. DATAL_RESPONSE */
	uint8 version;			/* protocol version number */
#if DL_VERSION == 0
	uint8 hw_type;			/* hardware type (host byte order) */
#else
	uint32 mac_type;		/* mac type */
#endif
	uint8 addr_len;			/* hardware address length */
	uint16 lwb;			/* lower bound of type range */
	uint16 upb;			/* upper bound of type range */
	uint16 frgsz;			/* max. packet size on net */
#if DL_VERSION >= 2
	uint8 addr[1];			/* hardware address (variable length) */
#else /*DL_VERSION*/
	uint8 addr[6];			/* hardware address */
#endif /*DL_VERSION*/
} S_DATAL_RESPONSE;

typedef struct datal_type {
	uint8 prim_type;		/* i.e. DATAL_TYPE/ETH_TYPE */
	uint8 version;			/* protocol version number */
	uint16 pad;			/* compatibility */
	uint16 lwb;			/* lower bound of type range */
	uint16 upb;			/* upper bound of type range */
#if DL_VERSION >= 2
	uint16 frgsz;			/* max. packet size on net */
	uint8 addr[1];			/* data link address (variable length)*/
#else /*DL_VERSION*/
	uint8 addr[6];			/* data link address */
	uint16 frgsz;			/* max. packet size on net */
#endif
} S_DATAL_TYPE;

/*
 *  Hardware types.
 */

/*
 * these happen to agree with the values used by ARP (see the latest
 * version of the "Assigned Numbers" RFC, currently RFC 1060)
 */

#define HW_ETHERNET	1		/* Ethernet Version 2 */
#define HW_TOKEN_RING	4		/* Token Ring */
#define HW_802		6		/* 802 Networks */

typedef struct datal_params {
	uint8 prim_type;		/* i.e. DATAL_PARAMS */
	uint8 version;			/* protocol version (unreliable) */
#if DL_VERSION == 0
	uint8 hw_type;			/* hardware type (host byte order) */
#else
	uint32 mac_type;		/* mac type */
#endif
	uint8 addr_len;			/* hardware address length */
#if DL_VERSION == 0
	uint8 addr[6];			/* hardware address */
	uint16 frgsz;			/* max. packet size on net */
#else
#if DL_VERSION == 1
	uint16 frgsz;			/* max. packet size on net */
	uint8 addr[6];			/* hardware address */
#else
	uint16 frgsz;			/* max. packet size on net */
	uint8 addr[1];			/* hardware address (variable length) */
#endif /*DL_VERSION == 1*/
#endif /*DL_VERSION == 0*/
} S_DATAL_PARAMS;

typedef struct datal_rx {
	uint8 prim_type;		/* i.e. DATAL_RX */
	uint8 version;			/* protocol version (unreliable) */
	uint16 pad;			/* compatibility */
#if DL_VERSION >= 2
	uint8 route_length;		/* size of optional routing field */
	uint8 addr_length;		/* size of source or dest address */
#endif
	uint16 type;			/* data link type field */
					/* treat type as length in 802 case */
#if DL_VERSION >= 2
	uint8 src[1];			/* source hardware address */
	/* 
	 * source hardware address is followed by the destination and this
	 * followed by optional routing information.
	 */
#else /*DL_VERSION*/
	uint8 src[6];			/* source hardware address */
#endif /*DL_VERSION*/
} S_DATAL_RX;

typedef struct datal_tx {
	uint8 prim_type;		/* i.e. DATAL_TX */
	uint8 version;			/* protocol version (unreliable) */
	uint16 pad;			/* compatibility */
#if DL_VERSION >= 2
	uint8 route_length;		/* size of optional routing field */
	uint8 addr_length;		/* size of destination address */
#endif
	uint16 type;			/* data link type field */
					/* treat type as length in 802 case */
#if DL_VERSION >= 2
	uint8 dst[1];			/* destination hardware address */
	/* 
	 * desination hardware address is followed by optional routing
	 * information.
	 */
#else /*DL_VERSION*/
	uint8 dst[6];			/* destination hardware address */
#endif /*DL_VERSION*/
} S_DATAL_TX;

/*
 *  Generic data link protocol primitive.
 */

typedef union datal_proto
{
	uint8 type;
	struct datal_type dltype;
	struct datal_params dlparm;
	struct datal_rx dlrx;
	struct datal_tx dltx;
	struct datal_response dlresp;
} S_DATAL_PROTO;
