/*   @(#)dlpi.h	1.3	14 Jul 1998	*/
/*
 * Copyright (c) 1991 UNIX International, Inc.
 * 
 * Permission to use, copy, modify, and distribute this documentation for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appears in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation,
 * and that the name UNIX international not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  UNIX International makes no representations
 * about the suitablity of this documentation for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * UNIX INTERNATIONAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * DOCUMENTATION, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL UNIX INTERNATIONAL BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS DOCUMENTATION.
 *
 * Taken from the Data Link Provider Interface Specification by Unix
 * International.  Edited for Spider by Gavin Shearer.
 *
 * @(#)$Spider: dlpi.h,v 1.1 1997/05/26 13:59:37 mark Exp $
 */

/*
Modification history:

Chg Date	Init Description
1.  15-Jun-98	rjp  Added support for x25llmon.
2.   7-jul-98   rjp  Added definition for uint, ulong and unchar.
3.  13-jul-98   rjp  Added definition for DL_MONITOR_LINK_LAYER_SIZE.

*/

#ifndef _SYS_DLPI_H
#define _SYS_DLPI_H

/*
 * dlpi.h header for Data Link Provider Interface
 */

/*
 * This header file has encoded the values so an existing driver
 * or user which was written with the Logical Link Interface (LLI)
 * can migrate to the DLPI interface in a binary compatible manner.
 * Any fields which require a specific format or value are flagged
 * with a comment containing the message "LLI compat.".
 */

/*
 *	DLPI revision definition history
 */
#define DL_CURRENT_VERSION	0x02	/* current version of DLPI */
#define DL_VERSION_2		0x02	/* version of DLPI March 12, 1991 */

#ifndef ulong				/* #2.				*/
#define ulong unsigned long
#endif
#ifndef uint				/* #2.				*/
#define uint unsigned 
#endif
#ifndef unchar				/* #2.				*/
#define unchar unsigned char
#endif

/*
 * Primitives for Local Management Services
 */
#define DL_INFO_REQ		0x00	/* information request, LLI compat. */
#define DL_INFO_ACK		0x03	/* information ack., LLI compat. */
#define DL_ATTACH_REQ		0x0b	/* attach a PPA */
#define DL_DETACH_REQ		0x0c	/* detach a PPA */
#define DL_BIND_REQ		0x01	/* bind DLSAP address, LLI compat. */
#define DL_BIND_ACK		0x04	/* DLSAP address bound, LLI compat. */
#define DL_UNBIND_REQ		0x02	/* unbind DLSAP address, LLI compat. */
#define DL_OK_ACK		0x06	/* success ack., LLI compat. */
#define DL_ERROR_ACK		0x05	/* error acknowledgment, LLI compat. */
#define DL_SUBS_BIND_REQ	0x1b	/* bind Subsequent DLSAP address */
#define DL_SUBS_BIND_ACK	0x1c	/* subsequent DLSAP address bound */
#define DL_SUBS_UNBIND_REQ	0x15	/* subsequent unbind */
#define DL_ENABMULTI_REQ	0x1d	/* enable multicast addresses */
#define DL_DISABMULTI_REQ	0x1e	/* disable multicast addresses */
#define DL_PROMISCON_REQ	0x1f	/* turn on promiscuous mode */
#define DL_PROMISCOFF_REQ	0x20	/* turn off promiscuous mode */

/*
 * Primitives used for Connectionless Service
 */
#define DL_UNITDATA_REQ		0x07	/* datagram send request, LLI compat. */
#define DL_UNITDATA_IND		0x08	/* datagram receive ind., LLI compat. */
#define DL_UDERROR_IND		0x09	/* datagram error ind. , LLI compat. */
#define DL_UDQOS_REQ		0x0a	/* set QOS for datagram transmissions */

/*
 * Primitives used for Connection-Oriented Service
 */
#define DL_CONNECT_REQ		0x0d	/* connect request */
#define DL_CONNECT_IND		0x0e	/* incoming connect indication */
#define DL_CONNECT_RES		0x0f	/* accept previous connect indication */
#define DL_CONNECT_CON		0x10	/* connection established */
#define DL_TOKEN_REQ		0x11	/* passoff token request */
#define DL_TOKEN_ACK		0x12	/* passoff token ack. */
#define DL_DISCONNECT_REQ	0x13	/* disconnect request */
#define DL_DISCONNECT_IND	0x14	/* disconnect indication */
#define DL_RESET_REQ		0x17	/* reset service request */
#define DL_RESET_IND		0x18	/* incoming reset indication */
#define DL_RESET_RES		0x19	/* complete reset processing */
#define DL_RESET_CON		0x1a	/* reset processing complete */

/*
 * Primitives used for Acknowledged Connectionless Service
 */
#define DL_DATA_ACK_REQ		0x21	/* data unit transmission request */
#define DL_DATA_ACK_IND		0x22	/* arrival of a command PDU */
#define DL_DATA_ACK_STATUS_IND	0x23	/* status indication of DATA_ACK_REQ */
#define DL_REPLY_REQ		0x24	/* request a DLSDU from the remote */
#define DL_REPLY_IND		0x25	/* arrival of a command PDU */
#define DL_REPLY_STATUS_IND	0x26	/* status indication of REPLY_REQ */
#define DL_REPLY_UPDATE_REQ	0x27	/* hold a DLSDU for transmission */
#define DL_REPLY_UPDATE_STATUS_IND 0x28	/* status of REPLY_UPDATE request */

/*
 * Primitives used for XID and TEST operations
 */
#define DL_XID_REQ		0x29	/* send an XID command request */
#define DL_XID_IND		0x2a	/* arrival of XID command */
#define DL_XID_RES		0x2b	/* send an XID response */
#define DL_XID_CON		0x2c	/* arrival of an XID response */
#define DL_TEST_REQ		0x2d	/* send a TEST command request */
#define DL_TEST_IND		0x2e	/* arrival of TEST command */
#define DL_TEST_RES		0x2f	/* send a TEST response */
#define DL_TEST_CON		0x30	/* arrival of a TEST response */

/*
 * Primitives to get and set the physical address, and to get statistics
 */
#define DL_PHYS_ADDR_REQ	0x31	/* request to get physical address */
#define DL_PHYS_ADDR_ACK	0x32	/* return physical address */
#define DL_SET_PHYS_ADDR_REQ	0x33	/* request to set physical address */
#define DL_GET_STATISTICS_REQ	0x34	/* request to get statistics */
#define DL_GET_STATISTICS_ACK	0x35	/* return statistics */
/*
 * Primitive for support of link layer monitoring (x25llmon).
 */
#define DL_MONITOR_LINK_LAYER	0x36	/* #1. request monitor of link layer */

/*
 * DLPI interface states
 */
#define DL_UNATTACHED		0x04	/* PPA not attached */
#define DL_ATTACH_PENDING	0x05	/* waiting ack. of DL_ATTACH_REQ */
#define DL_DETACH_PENDING	0x06	/* waiting ack. of DL_DETACH_REQ */
#define DL_UNBOUND		0x00	/* PPA attached, LLI compat. */
#define DL_BIND_PENDING		0x01	/* waiting ack. of DL_BIND_REQ,
					   LLI compat. */
#define DL_UNBIND_PENDING	0x02	/* waiting ack. of DL_UNBIND_REQ,
					   LLI compat. */
#define DL_IDLE			0x03	/* DLSAP bound, awaiting use,
					   LLI compat. */
#define DL_UDQOS_PENDING	0x07	/* waiting ack of DL_UDQOS_REQ */
#define DL_OUTCON_PENDING	0x08	/* outgoing connection,
					   awaiting DL_CONN_CON */
#define DL_INCON_PENDING	0x09	/* incoming connection,
					   awaiting DL_CONN_RES */
#define DL_CONN_RES_PENDING	0x0a	/* waiting ack. of DL_CONNECT_RES */
#define DL_DATAXFER		0x0b	/* connection-oriented data transfer */
#define DL_USER_RESET_PENDING	0x0c	/* user initiated reset,
					   awaiting DL_RESET_CON */
#define DL_PROV_RESET_PENDING	0x0d	/* provider initiated reset,
					   awaiting DL_RESET_RES */
#define DL_RESET_RES_PENDING	0x0e	/* waiting ack. of DL_RESET_RES */
#define DL_DISCON8_PENDING	0x0f	/* waiting ack. of DL_DISC_REQ when
					   in DL_OUTCON_PENDING */
#define DL_DISCON9_PENDING	0x10	/* waiting ack. of DL_DISC_REQ when
					   in DL_INCON_PENDING */
#define DL_DISCON11_PENDING	0x11	/* waiting ack. of DL_DISC_REQ when
					   in DL_DATAXFER */
#define DL_DISCON12_PENDING	0x12	/* waiting ack. of DL_DISC_REQ when
					   in DL_USER_RESET_PENDING */
#define DL_DISCON13_PENDING	0x13	/* waiting ack. of DL_DISC_REQ when
					   in DL_DL_PROV_RESET_PENDING */
#define DL_SUBS_BIND_PND	0x14	/* waiting ack. of DL_SUBS_BIND_REQ */
#define DL_SUBS_UNBIND_PND	0x15	/* waiting ack. of DL_SUBS_UNBIND_REQ */

/*
 * DL_ERROR_ACK error return values
 */
#define DL_ACCESS		0x02	/* improper permissions for request,
					   LLI compat. */
#define DL_BADADDR		0x01	/* DLSAP address in improper format
					   or invalid */
#define DL_BADCORR		0x05	/* sequence number not from
					   outstanding DL_CONN_IND */
#define DL_BADDATA		0x06	/* user data exceeded provider limit */
#define DL_BADPPA		0x08	/* specified PPA was invalid */
#define DL_BADPRIM		0x09	/* primitive received is not known
					   by DLS provider */
#define DL_BADQOSPARAM		0x0a	/* QOS parameters contained invalid
					   values */
#define DL_BADQOSTYPE		0x0b	/* QOS structure type is unknown or
					   unsupported */
#define DL_BADSAP		0x00	/* bad LSAP selector, LLI compat. */
#define DL_BADTOKEN		0x0c	/* token used not associated with
					   an active stream */
#define DL_BOUND		0x0d	/* attempted second bind with
					   dl_max_conind or dl_conn_mgmt > 0
					   on same DLSAP or PPA */
#define DL_INITFAILED		0x0e	/* physical link init. failed */
#define DL_NOADDR		0x0f	/* provider could not allocate
					   alternate address */
#define DL_NOTINIT		0x10	/* physical link not initialised */
#define DL_OUTSTATE		0x03	/* primitive issued in improper state,
					   LLI compat. */
#define DL_SYSERR		0x04	/* UNIX system error occurred,
					   LLI compat. */
#define DL_UNSUPPORTED		0x07	/* requested service not supplied
					   by provider */
#define DL_UNDELIVERABLE	0x11	/* previous data unit could not
					   be delivered */
#define DL_NOTSUPPORTED		0x12	/* primitive is known but not
					   supported by DLS provider */
#define DL_TOOMANY		0x13	/* limit exceeded */
#define DL_NOTENAB		0x14	/* promiscuous mode not enabled */
#define DL_BUSY			0x15	/* other streams for a particular PPA
					   are in the post-attached state */
#define DL_NOAUTO		0x16	/* automatic handling of XID and TEST
					   responses not supported */
#define DL_NOXIDAUTO		0x17	/* automatic handling of XID
					   not supported */
#define DL_NOTESTAUTO		0x18	/* automatic handling of TEST
					   not supported */
#define DL_XIDAUTO		0x19	/* automatic handling of XID response */
#define DL_TESTAUTO		0x1a	/* automatic handling of TEST resp. */
#define DL_PENDING		0x1b	/* pending outstanding connect ind. */

/*
 * NOTE: The range of error codes, 0x80-0xff is reserved for
 *	implementation specific error codes. This reserved range of error
 *	codes will be defined bythe DLS Provider.
 */

/*
 * DLPI media types supported
 */
#define DL_CSMACD		0x00	/* IEEE 802.3 CSMA/CD network,
					   LLI compat. */
#define DL_TPB			0x01	/* IEEE 802.4 Token Passing Bus,
					   LLI compat. */
#define DL_TPR			0x02	/* IEEE 802.5 Token Passing Ring,
					   LLI compat. */
#define DL_METRO		0x03	/* IEEE 802.6 MetroNet,
					   LLI compat. */
#define DL_ETHER		0x04	/* Ethernet Bus, LLI compat. */
#define DL_HDLC			0x05	/* ISO HDLC protocol support,
					   bit synchronous */
#define DL_CHAR			0x06	/* character synchronous protocol
					   support, e.g. BISYNC */
#define DL_CTCA			0x07	/* IBM Channel-to-Channel Adapter */
#define DL_FDDI			0x08	/* Fiber Distributed Data Interface */
#define DL_OTHER		0x09	/* any other medium not listed above */

/*
 * DLPI provider service supported
 *
 * These must be allowed to be bitwise-OR for dl_service_mode in
 * DL_INFO_ACK.
 */
#define DL_CODLS		0x01	/* connection-oriented service */
#define DL_CLDLS		0x02	/* connectionless datalink service */
#define DL_ACLDLS		0x04	/* acknowledged connectionless
					   service */

/*
 * DLPI provider style
 *
 * The DLPI provider style which determines whether a provider
 * requires a DL_ATTACH_REQ to inform the provider which PPA
 * user messages should be sent and received on.
 */
#define DL_STYLE1		0x0500	/* PPA is implicitly bound by open(2) */
#define DL_STYLE2		0x0501	/* PPA must be explicitly bound
					   via DL_ATTACH_REQ */

/*
 * DLPI originator for disconnects and resets
 */
#define DL_PROVIDER		0x0700	/* originator is the DLS provider */
#define DL_USER			0x0701  /* originator is the DLS used */

/*
 * DLPI disconnect reasons
 */
#define DL_CONREJ_DEST_UNKNOWN			0x0800
#define DL_CONREJ_DEST_UNREACH_PERMANENT	0x0801
#define DL_CONREJ_DEST_UNREACH_TRANSIENT	0x0802
#define DL_CONREJ_QOS_UNAVAIL_PERMANENT		0x0803
#define DL_CONREJ_QOS_UNAVAIL_TRANSIENT		0x0804
#define DL_CONREJ_PERMANENT_COND		0x0805
#define DL_CONREJ_TRANSIENT_COND		0x0806
#define DL_DISC_ABNORMAL_CONDITION		0x0807
#define DL_DISC_NORMAL_CONDITION		0x0808
#define DL_DISC_PERMANENT_CONDITION		0x0809
#define DL_DISC_TRANSIENT_CONDITION		0x080a
#define DL_DISC_UNSPECIFIED			0x080b

/*
 * DLPI reset reasons
 */
#define DL_RESET_FLOW_CONTROL	0x0900
#define DL_RESET_LINK_ERROR	0x0901
#define DL_RESET_RESYNCH	0x0902

/*
 * DLPI status values for acknowledged connectionless data transfer
 */
#define DL_CMD_MASK		0x0f	/* mask for command part of status */
#define DL_CMD_OK		0x00	/* command accepted */
#define DL_CMD_RS		0x01	/* unimplemented or inactivated
					   service */
#define DL_CMD_UE		0x05	/* datalink user interface error */
#define DL_CMD_PE		0x06	/* protocol error */
#define DL_CMD_IP		0x07	/* permanent implementation dependent
					   error */
#define DL_CMD_UN		0x09	/* resources temporarily unavailable */
#define DL_CMD_IT		0x0f	/* temporary implementation dependent
					   error */
#define DL_RSP_MASK		0xf0	/* mask for response part of status */
#define DL_RSP_OK		0x00	/* response DLSDU present */
#define DL_RSP_RS		0x10	/* unimplemented or inactivated
					   service */
#define DL_RSP_NE		0x30	/* response DLSDU never submitted */
#define DL_RSP_NR		0x40	/* response DLSDU not requested */
#define DL_RSP_UE		0x50	/* datalink user interface error */
#define DL_RSP_IP		0x70	/* permanent implementation dependent
					   error */
#define DL_RSP_UN		0x90	/* resources temporarily unavailable */
#define DL_RSP_IT		0xf0	/* temporary implementation dependent
					   error */

/*
 * Service class values for acknowledged connectionless data transfer
 */
#define DL_RQST_RSP		0x01	/* use acknowledge capability in
					   MAC sublayer */
#define DL_RQST_NORSP		0x02	/* no acknowledgement service
					  requested */

/*
 * DLPI address type definition
 */
#define DL_FACT_PHYS_ADDR	0x01	/* factory physical address */
#define DL_CURR_PHYS_ADDR	0x02	/* current physical address */

/*
 * DLPI flag definitions
 */
#define DL_POLL_FINAL		0x01	/* if set, indicates poll/final
					   bit set */

/*
 * XID and TEST responses supported by the provider
 */
#define DL_AUTO_XID		0x01	/* provider will respond to XID */
#define DL_AUTO_TEST		0x02	/* provider will respond to TEST */

/*
 * Subsequent bind type
 */
#define DL_PEER_BIND		0x01	/* subsequent bind on a peer address */
#define DL_HIERARCHICAL_BIND	0x02	/* subsequent bind on a hierarchical
					   address */

/*
 * DLPI promiscuous mode definitions
 */
#define DL_PROMISC_PHYS		0x01	/* promiscuous mode at physical level */
#define DL_PROMISC_SAP		0x02	/* promiscous mode at SAP level */
#define DL_PROMISC_MULTI	0x03	/* promiscuous mode for multicast */

/*
 * DLPI quality of service definition for use in QOS structure definitions
 *
 * The QOS structures are used in connection establishment, DL_INFO_ACK,
 * and setting connectionless QOS values.
 */

/*
 * Throughput
 *
 * This parameter is specified for both directions.
 */
typedef struct {
	long	dl_target_value;	/* desired bits/second */
	long	dl_accept_value;	/* min. acceptable bits/second */
} dl_through_t;

/*
 * Transit delay specification
 *
 * This parameter is specified for both directions.
 * It is expressed in milliseconds, assuming a DLSDU size of 128 octets.
 * The scaling of the value to the current DLSDU size is provider dependent.
 */
typedef struct {
	long	dl_target_value;	/* desired transit delay */
	long	dl_accept_value;	/* min. acceptable transit delay */
} dl_transdelay_t;

/*
 * Priority specification
 *
 * The priority range is 0-100, with 0 being the highest value.
 */
typedef struct {
	long	dl_min;			/* minimum priority */
	long	dl_max;			/* maximum priority */
} dl_priority_t;

/*
 * Protection specification
 */
#define DL_NONE			0x0B01	/* no protection supplied */
#define DL_MONITOR		0x0B02	/* protection against passive
					   monitoring */
#define DL_MAXIMUM		0x0B03	/* protection against modification,
					   replay, addition, or deletion */
typedef struct {
	long	dl_min;			/* minimum protection */
	long	dl_max;			/* maximum protection*/
} dl_protect_t;

/*
 * Resilience specification
 *
 * The probabilities are scaled by a factor of 10,000 with a time interval
 * of 10,000 seconds.
 */
typedef struct {
	long	dl_disc_prob;		/* prob. of provider initiated DISC */
	long	dl_reset_prob;		/* prob. of provider initiated RESET */
} dl_resilience_t;

/*
 * QOS type definition to be used for negotiation with the
 * remote end of a connection, or a connectionless unitdata request.
 *
 * There are two type definitions to handle the negotiation
 * process at connection establishment. The typedef "dl_qos_range_t"
 * is used to present a range for parameters. This is used
 * in the DL_CONNECT_REQ and DL_CONNECT_IND messages. The typedef
 * "dl_qos_sel_t" is used to select a specific value for the QOS
 * parameters. This is used in the DL_CONNECT_RES, DL_CONNECT_CON,
 * and DL_INFO_ACK messages to define the selected QOS parameters
 * for a connection.
 *
 * NOTES:
 *	A datalink provider which has unknown values for any of the fields
 *	will use a value of DL_UNKNOWN for all values in the fields.
 *
 *	A QOS parameter value of DL_QOS_DONT_CARE informs the DLS
 *	provider that the user requesting this value does not care
 *	what the QOS parameter is set to. This value becomes the
 *	least possible value in the range of QOS parameters.
 *	The order of the QOS parameter range is then:
 *
 *		DL_QOS_DONT_CARE < 0 < MAXIMUM QOS VALUE
 */
#define DL_UNKNOWN		-1
#define DL_QOS_DONT_CARE	-2

/*
 * Every QOS structure has the first 4 bytes containing a type
 * field, denoting the definition of the rest of the structure.
 * This is used in the same manner as the "dl_primitive" variable
 * is in messages.
 *
 * The following list is the defined QOS structure type values and structures.
 */
#define DL_QOS_CO_RANGE1	0x0101 /* QOS range structure for
					  connection-oriented mode service */
#define DL_QOS_CO_SEL1		0x0102 /* QOS selection structure for
					  connection-oriented mode service */
#define DL_QOS_CL_RANGE1	0x0103 /* QOS range structure for
					  connectionless mode service */
#define DL_QOS_CL_SEL1		0x0104 /* QOS selection structure for
					  connectionless mode service */
typedef struct {
	ulong		dl_qos_type;		/* DL_QOS_CO_RANGE1 */
	dl_through_t	dl_rcv_throughput;	/* desired and acceptable */
	dl_transdelay_t	dl_rcv_trans_delay;	/* desired and acceptable */
	dl_through_t	dl_xmt_throughput;
	dl_transdelay_t	dl_xmt_trans_delay;
	dl_priority_t	dl_priority;		/* min and max values */
	dl_protect_t	dl_protection;		/* min and max values */
	long		dl_residual_error;
	dl_resilience_t	dl_resilience;
} dl_qos_co_range1_t;

typedef struct {
	ulong		dl_qos_type;		/* DL_QOS_CO_SEL1 */
	long		dl_rcv_throughput;
	long		dl_rcv_trans_delay;
	long		dl_xmt_throughput;
	long		dl_xmt_trans_delay;
	long		dl_priority;
	long		dl_protection;
	long		dl_residual_error;
	dl_resilience_t	dl_resilience;
} dl_qos_co_sel1_t;

typedef struct {
	ulong		dl_qos_type;		/* DL_QOS_CL_RANGE1 */
	dl_transdelay_t dl_trans_delay;
	dl_priority_t	dl_priority;
	dl_protect_t	dl_protection;
	long		dl_residual_error;
} dl_qos_cl_range1_t;

typedef struct {
	ulong		dl_qos_type;		/* DL_QOS_CL_SEL1 */
	long		dl_trans_delay;
	long		dl_priority;
	long		dl_protection;
	long		dl_residual_error;
} dl_qos_cl_sel1_t;

/*
 * DLPI interface primitive definitions.
 *
 * Each primitive is sent as a STREAMS message. It is possible that
 * the messages may be viewed as a sequence of bytes that have the
 * following form without any padding. The structure definition
 * of the following messages may have to change, depending on the
 * underlying hardware architecture and crossing of a hardware
 * boundary with a different hardware architecture.
 *
 * Fields in the primitives having a name of the form
 * "dl_reserved" cannot be used and have the value of
 * binary zero (i.e. no bits turned on).
 *
 * Each message has the name defined followed by the
 * STREAMS message typei (one of M_PROTO, M_PCPROTO and M_DATA).
 */

/*
 *	LOCAL MANAGEMENT SERVICE PRIMITIVES
 */

/*
 * DL_INFO_REQ, M_PCPROTO type
 */
typedef struct {
	ulong		dl_primitive;		/* DL_INFO_REQ */
} dl_info_req_t;

/*
 * DL_INFO_ACK, M_PCPROTO type
 */
typedef struct {
	ulong		dl_primitive;		/* DL_INFO_ACK */
	ulong		dl_max_sdu;		/* max. bytes in a DLSDU */
	ulong		dl_min_sdu;		/* min bytes in a DLSDU */
	ulong		dl_addr_length;		/* length of DLSAP address */
	ulong		dl_mac_type;		/* type of medium supported */
	ulong		dl_reserved;		/* value set to zero */
	ulong		dl_current_state;	/* state of DLPI interface */
	long		dl_sap_length;		/* current length of SAP
						   part of DLSAP address */
	ulong		dl_service_mode;	/* CO, CL or ACL */
	ulong		dl_qos_length;		/* length of QOS values */
	ulong		dl_qos_offset;		/* offset from start of block */
	ulong		dl_qos_range_length;	/* available range of QOS */
	ulong		dl_qos_range_offset;	/* offset from start of block */
	ulong		dl_provider_style;	/* style 1 or style 2 */
	ulong		dl_addr_offset;		/* offset of DLSAP address */
	ulong		dl_version;		/* DLPI version number */
	ulong		dl_brdcst_addr_length;  /* length of broadcast addr. */
	ulong		dl_brdcst_addr_offset;  /* offset from start of block */
	ulong		dl_growth;		/* set to zero */
} dl_info_ack_t;

/*
 * DL_ATTACH_REQ, M_PROTO type
 */
typedef struct {
	ulong		dl_primitive;		/* DL_ATTACH_REQ */
	ulong		dl_ppa;			/* ID of the PPA */
} dl_attach_req_t;

/*
 * DL_DETACH_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_DETACH_REQ */
} dl_detach_req_t;

/*
 * DL_BIND_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* set to DL_BIND_REQ */
	ulong	dl_sap;				/* information to identify
						   the DLSAP address */
	ulong	dl_max_conind;			/* max. number of outstanding
						   connection indications */
	ushort dl_service_mode;			/* CO, CL or ACL */
	ushort dl_conn_mgmt;			/* if non-zero this is a
						   connnection mgmt. stream */
	ulong	dl_xidtest_flg;			/* indicates automatic sending
						   of TEST and XID frames */
} dl_bind_req_t;

/*
 * DL_BIND_ACK, M_PCPROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_BIND_ACK */
	ulong	dl_sap;				/* DLSAP address information */
	ulong	dl_addr_length;			/* length of whole DLSAP addr.*/
	ulong	dl_addr_offset;			/* offset from start of block */
	ulong	dl_max_conind;			/* allowed maxinum number
						   of connection indications */
	ulong	dl_xidtest_flg;			/* responses supported by
						   provider */
} dl_bind_ack_t;

/*
 * DL_SUBS_BIND_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_SUBS_BIND_REQ */
	ulong	dl_subs_sap_offset;		/* offset of subseq. SAP */
	ulong	dl_subs_sap_length;		/* length of subseq. SAP */
	ulong	dl_subs_bind_class;		/* peer or hierarchical */
} dl_subs_bind_req_t;

/*
 * DL_SUBS_BIND_ACK, M_PCPROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_SUBS_BIND_ACK */
	ulong	dl_subs_sap_offset;		/* offset of subseq. SAP */
	ulong	dl_subs_sap_length;		/* length of subseq. SAP */
} dl_subs_bind_ack_t;

/*
 * DL_UNBIND_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_UNBIND_REQ */
} dl_unbind_req_t;

/*
 * DL_SUBS_UNBIND_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_SUBS_UNBIND_REQ */
	ulong	dl_subs_sap_offset;		/* offset of subseq. SAP */
	ulong	dl_subs_sap_length;		/* length of subseq. SAP */
} dl_subs_unbind_req_t;

/*
 * DL_OK_ACK, M_PCPROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_OK_ACK */
	ulong	dl_correct_primitive;		/* prim. being acknowledged */
} dl_ok_ack_t;

/*
 * DL_ERROR_ACK, M_PCPROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_ERROR_ACK */
	ulong	dl_error_primitive;		/* primitive in error */
	ulong	dl_errno;			/* DLPI error code */
	ulong	dl_unix_errno;			/* UNIX system error code */
} dl_error_ack_t;

/*
 * DL_ENABMULTI_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_ENABMULTI_REQ */
	ulong	dl_addr_length;			/* length of multicast addr. */
	ulong	dl_addr_offset;			/* offset from start of block */
} dl_enabmulti_req_t;

/*
 * DL_DISABMULTI_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_DISABMULTI_REQ */
	ulong	dl_addr_length;			/* length of multicast addr. */
	ulong	dl_addr_offset;			/* offset from start of block */
} dl_disabmulti_req_t;

/*
 * DL_PROMISCON_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_PROMISCON_REQ */
	ulong	dl_level;			/* physical, SAP level or
						   all multicast */
} dl_promiscon_req_t;

/*
 * DL_PROMISCOFF_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_PROMISCOFF_REQ */
	ulong	dl_level;			/* physical, SAP level or
						   all multicast */
} dl_promiscoff_req_t;

/*
 * Primitives to get and set the physical address
 */

/*
 * DL_PHYS_ADDR_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_PHYS_ADDR_REQ */
	ulong	dl_addr_type;			/* factory or current
						   physical address */
} dl_phys_addr_req_t;

/*
 * DL_PHYS_ADDR_ACK, M_PCPROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_PHYS_ADDR_ACK */
	ulong	dl_addr_length;			/* length of physical address */
	ulong	dl_addr_offset;			/* offset from start of block */
} dl_phys_addr_ack_t;

/*
 * DL_SET_PHYS_ADDR_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_SET_PHYS_ADDR_REQ */
	ulong	dl_addr_length;			/* length of physical address */
	ulong	dl_addr_offset;			/* offset from start of block */
} dl_set_phys_addr_req_t;

/*
 * Primitives to get statistics
 */

/*
 * DL_GET_STATISTICS_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_GET_STATISTICS_REQ */
} dl_get_statistics_req_t;

/*
 * DL_GET_STATISTICS_ACK, M_PCPROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_GET_STATISTICS_ACK */
	ulong	dl_stat_length;			/* length of stats. structure */
	ulong	dl_stat_offset;			/* offset from start of block */
} dl_get_statistics_ack_t;


/*
 *	CONNECTION-ORIENTED SERVICE PRIMITIVES
 */

/*
 * DL_CONNECT_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_CONNECT_REQ */
	ulong	dl_dest_addr_length;		/* length of DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of DLSAP address */
	ulong	dl_qos_length;			/* length of QOS structure */
	ulong	dl_qos_offset;			/* offset of QOS structure */
	ulong	dl_growth;			/* set to zero */
} dl_connect_req_t;

/*
 * DL_CONNECT_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_CONNECT_IND */
	ulong	dl_correlation;			/* provider's correlation
						   token */
	ulong	dl_called_addr_length;		/* length of called address */
	ulong	dl_called_addr_offset;		/* offset of called address */
	ulong	dl_calling_addr_length;		/* length of calling address */
	ulong	dl_calling_addr_offset;		/* offset of calling address */
	ulong	dl_qos_length;			/* length of QOS structure */
	ulong	dl_qos_offset;			/* offset of QOS structure */
	ulong	dl_growth;			/* set to zero */
} dl_connect_ind_t;

/*
 * DL_CONNECT_RES, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_CONNECT_RES */
	ulong	dl_correlation;			/* provider's correlation
						   token */
	ulong	dl_resp_token;			/* token associated with
						   responding stream */
	ulong	dl_qos_length;			/* length of QOS structure */
	ulong	dl_qos_offset;			/* offset of QOS structure */
	ulong	dl_growth;			/* set to zero */
} dl_connect_res_t;

/*
 * DL_CONNECT_CON, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_CONNECT_CON */
	ulong	dl_resp_addr_length;		/* length of responder's addr.*/
	ulong	dl_resp_addr_offset;		/* offset of responder's addr.*/
	ulong	dl_qos_length;			/* length of QOS structure */
	ulong	dl_qos_offset;			/* offset of QOS structure */
	ulong	dl_growth;			/* set to zero */
} dl_connect_con_t;

/*
 * DL_TOKEN_REQ, M_PCPROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_TOKEN_REQ */
} dl_token_req_t;

/*
 * DL_TOKEN_ACK, M_PCPROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_TOKEN_ACK */
	ulong	dl_token;			/* connection response token
						   associated with the stream */
} dl_token_ack_t;

/*
 * DL_DISCONNECT_REQ, M_PROTO type
 */
typedef struct {
	ulong		dl_primitive;		/* DL_DISCONNECT_REQ */
	ulong		dl_reason;		/* normal, abnormal, permanent
						   or transient */
	ulong		dl_correlation;		/* association with connection
						   indication */
} dl_disconnect_req_t;

/*
 * DL_DISCONNECT_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_DISCONNECT_IND */
	ulong	dl_originator;			/* user or provider */
	ulong	dl_reason;			/* permanent or transient */
	ulong	dl_correlation;			/* association with connection
						   indication */
} dl_disconnect_ind_t;

/*
 * DL_RESET_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_RESET_REQ */
} dl_reset_req_t;

/*
 * DL_RESET_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_RESET_IND */
	ulong	dl_originator;			/* user or provider */
	ulong	dl_reason;			/* flow control, link error or
						   resynch. */
} dl_reset_ind_t;

/*
 * DL_RESET_RES, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_RESET_RES */
} dl_reset_res_t;

/*
 * DL_RESET_CON, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_RESET_CON */
} dl_reset_con_t;


/*
 *	CONNECTIONLESS SERVICE PRIMITIVES
 */

/*
 * DL_UNITDATA_REQ, M_PROTO type, with M_DATA block(s)
 */
typedef struct {
	ulong	dl_primitive;			/* DL_UNITDATA_REQ */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of DLSAP address */
	dl_priority_t	dl_priority;		/* priority range */
} dl_unitdata_req_t;

/*
 * DL_UNITDATA_IND, M_PROTO type, with M_DATA block(s)
 */
typedef struct {
	ulong	dl_primitive;			/* DL_UNITDATA_IND */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of destination
						   DLSAP address */
	ulong	dl_src_addr_length;		/* length of source
						   DLSAP address */
	ulong	dl_src_addr_offset;		/* offset of source
						   DLSAP address */
	ulong	dl_group_address;		/* set to 1 if multicast or
						   broadcast message */
} dl_unitdata_ind_t;

/*
 * DL_UDERROR_IND, M_PROTO type
 *	(or M_PCPROTO type if LLI-based provider)
 */
typedef struct {
	ulong	dl_primitive;			/* DL_UDERROR_IND */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of DLSAP address */
	ulong	dl_unix_errno;			/* UNIX system error code */
	ulong	dl_errno;			/* DLPI error code */
} dl_uderror_ind_t;

/*
 * DL_UDQOS_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_UDQOS_REQ */
	ulong	dl_qos_length;			/* length (in bytes) of
						   requested QOS */
	ulong	dl_qos_offset;			/* offset of QOS structure */
} dl_udqos_req_t;


/*
 *	Primitives to handle XID and TEST operations
 */

/*
 * DL_TEST_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_TEST_REQ */
	ulong	dl_flag;			/* poll/final bit */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of DLSAP address */
} dl_test_req_t;

/*
 * DL_TEST_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_TEST_IND */
	ulong	dl_flag;			/* poll/final bit */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of destination
						   DLSAP address */
	ulong	dl_src_addr_length;		/* length of source
						   DLSAP address */
	ulong	dl_src_addr_offset;		/* offset of source
						   DLSAP address */
} dl_test_ind_t;

/*
 *	DL_TEST_RES, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_TEST_RES */
	ulong	dl_flag;			/* poll/final bit */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of DLSAP address */
} dl_test_res_t;

/*
 *	DL_TEST_CON, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_TEST_CON */
	ulong	dl_flag;			/* poll/final bit */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of destination
						   DLSAP address */
	ulong	dl_src_addr_length;		/* length of source
						   DLSAP address */
	ulong	dl_src_addr_offset;		/* offset of source
						   DLSAP address */
} dl_test_con_t;

/*
 * DL_XID_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_XID_REQ */
	ulong	dl_flag;			/* poll/final bit */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of DLSAP address */
} dl_xid_req_t;

/*
 * DL_XID_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_XID_IND */
	ulong	dl_flag;			/* poll/final bit */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of destination
						   DLSAP address */
	ulong	dl_src_addr_length;		/* length of source
						   DLSAP address */
	ulong	dl_src_addr_offset;		/* offset of source
						   DLSAP address */
} dl_xid_ind_t;

/*
 * DL_XID_RES, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_XID_RES */
	ulong	dl_flag;			/* poll/final bit */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of DLSAP address */
} dl_xid_res_t;

/*
 *	DL_XID_CON, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_XID_CON */
	ulong	dl_flag;			/* poll/final bit */
	ulong	dl_dest_addr_length;		/* length of destination
						   DLSAP address */
	ulong	dl_dest_addr_offset;		/* offset of destination
						   DLSAP address */
	ulong	dl_src_addr_length;		/* length of source
						   DLSAP address */
	ulong	dl_src_addr_offset;		/* offset of source
						   DLSAP address */
} dl_xid_con_t;


/*
 *	ACKNOWLEDGED CONNECTIONLESS SERVICE PRIMITIVES
 */

/*
 * DL_DATA_ACK_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_DATA_ACK_REQ */
	ulong	dl_correlation;			/* user's correlation token */
	ulong	dl_dest_addr_length;		/* length of destination addr.*/
	ulong	dl_dest_addr_offset;		/* offset of destination addr.*/
	ulong	dl_src_addr_length;		/* length of source address */
	ulong	dl_src_addr_offset;		/* offset of source address */
	ulong	dl_priority;			/* priority value */
	ulong	dl_service_class;		/* DL_RQST_RSP or
						   DL_RQST_NORSP */
} dl_data_ack_req_t;

/*
 * DL_DATA_ACK_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_DATA_ACK_IND */
	ulong	dl_dest_addr_length;		/* length of destination addr.*/
	ulong	dl_dest_addr_offset;		/* offset of destination addr.*/
	ulong	dl_src_addr_length;		/* length of source address */
	ulong	dl_src_addr_offset;		/* offset of source address */
	ulong	dl_priority;			/* priority for data unit
						   transmission */
	ulong	dl_service_class;		/* DL_RQST_RSP or
						   DL_RQST_NORSP */
} dl_data_ack_ind_t;

/*
 * DL_DATA_ACK_STATUS_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_DATA_ACK_STATUS_IND */
	ulong	dl_correlation;			/* user's correlation token */
	ulong	dl_status;			/* success or failure of
						  previous request */
} dl_data_ack_status_ind_t;

/*
 * DL_REPLY_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_REPLY_REQ */
	ulong	dl_correlation;			/* user's correlation token */
	ulong	dl_dest_addr_length;		/* length of destination addr.*/
	ulong	dl_dest_addr_offset;		/* offset of destination addr.*/
	ulong	dl_src_addr_length;		/* length of source address */
	ulong	dl_src_addr_offset;		/* offset of source address */
	ulong	dl_priority;			/* priority for data unit
						   transmission */
	ulong	dl_service_class;		/* DL_RQST_RSP or
						   DL_RQST_NORSP */
} dl_reply_req_t;

/*
 * DL_REPLY_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_REPLY_IND */
	ulong	dl_dest_addr_length;		/* length of destination addr.*/
	ulong	dl_dest_addr_offset;		/* offset of destination addr.*/
	ulong	dl_src_addr_length;		/* length of source address */
	ulong	dl_src_addr_offset;		/* offset of source address */
	ulong	dl_priority;			/* priority for data unit
						   transmission */
	ulong	dl_service_class;		/* DL_RQST_RSP or
						   DL_RQST_NORSP */
} dl_reply_ind_t;

/*
 * DL_REPLY_STATUS_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_REPLY_STATUS_IND */
	ulong	dl_correlation;			/* user's correlation token */
	ulong	dl_status;			/* success or failure of
						   previous request */
} dl_reply_status_ind_t;

/*
 * DL_REPLY_UPDATE_REQ, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_REPLY_UPDATE_REQ */
	ulong	dl_correlation;			/* user's correlation token */
	ulong	dl_src_addr_length;		/* length of source address */
	ulong	dl_src_addr_offset;		/* offset of source address */
} dl_reply_update_req_t;

/*
 * DL_REPLY_UPDATE_STATUS_IND, M_PROTO type
 */
typedef struct {
	ulong	dl_primitive;			/* DL_REPLY_UPDATE_STATUS_IND */
	ulong	dl_correlation;			/* user's correlation token */
	ulong	dl_status;			/* success or failure of
						   previous request */
} dl_reply_update_status_ind_t;

/*
 * A union of all the primitives
 */
union DL_primitives {
	ulong				dl_primitive;
	dl_info_req_t			info_req;
	dl_info_ack_t			info_ack;
	dl_attach_req_t			attach_req;
	dl_detach_req_t			detach_req;
	dl_bind_req_t			bind_req;
	dl_bind_ack_t			bind_ack;
	dl_unbind_req_t			unbind_req;
	dl_subs_bind_req_t		subs_bind_req;
	dl_subs_bind_ack_t		subs_bind_ack;
	dl_subs_unbind_req_t		subs_unbind_req;
	dl_ok_ack_t			ok_ack;
	dl_error_ack_t			error_ack;
	dl_connect_req_t		connect_req;
	dl_connect_ind_t		connect_ind;
	dl_connect_res_t		connect_res;
	dl_connect_con_t		connect_con;
	dl_token_req_t			token_req;
	dl_token_ack_t			token_ack;
	dl_disconnect_req_t		disconnect_req;
	dl_disconnect_ind_t		disconnect_ind;
	dl_reset_req_t			reset_req;
	dl_reset_ind_t			reset_ind;
	dl_reset_res_t			reset_res;
	dl_reset_con_t			reset_con;
	dl_unitdata_req_t		unitdata_req;
	dl_unitdata_ind_t		unitdata_ind;
	dl_uderror_ind_t		uderror_ind;
	dl_udqos_req_t			udqos_req;
	dl_enabmulti_req_t		enabmulti_req;
	dl_disabmulti_req_t		disabmulti_req;
	dl_promiscon_req_t		promiscon_req;
	dl_promiscoff_req_t		promiscoff_req;
	dl_phys_addr_req_t		physaddr_req;
	dl_phys_addr_ack_t		physaddr_ack;
	dl_set_phys_addr_req_t		set_physaddr_req;
	dl_get_statistics_req_t		get_statistics_req;
	dl_get_statistics_ack_t		get_statistics_ack;
	dl_test_req_t			test_req;
	dl_test_ind_t			test_ind;
	dl_test_res_t			test_res;
	dl_test_con_t			test_con;
	dl_xid_req_t			xid_req;
	dl_xid_ind_t			xid_ind;
	dl_xid_res_t			xid_res;
	dl_xid_con_t			xid_con;
	dl_data_ack_req_t		data_ack_req;
	dl_data_ack_ind_t		data_ack_ind;
	dl_data_ack_status_ind_t	data_ack_status_ind;
	dl_reply_req_t			reply_req;
	dl_reply_ind_t			reply_ind;
	dl_reply_status_ind_t		reply_status_ind;
	dl_reply_update_req_t		reply_update_req;
	dl_reply_update_status_ind_t	reply_update_status_ind;
	dl_monitor_link_layer_t		monitor_link_layer; /* #1 LLMON */
};

#define DL_INFO_REQ_SIZE		sizeof(dl_info_req_t)
#define DL_INFO_ACK_SIZE		sizeof(dl_info_ack_t)
#define DL_ATTACH_REQ_SIZE		sizeof(dl_attach_req_t)
#define DL_DETACH_REQ_SIZE		sizeof(dl_detach_req_t)
#define DL_BIND_REQ_SIZE		sizeof(dl_bind_req_t)
#define DL_BIND_ACK_SIZE		sizeof(dl_bind_ack_t)
#define DL_UNBIND_REQ_SIZE		sizeof(dl_unbind_req_t)
#define DL_SUBS_BIND_REQ_SIZE		sizeof(dl_subs_bind_req_t)
#define DL_SUBS_BIND_ACK_SIZE		sizeof(dl_subs_bind_ack_t)
#define DL_SUBS_UNBIND_REQ_SIZE		sizeof(dl_subs_unbind_req_t)
#define DL_OK_ACK_SIZE			sizeof(dl_ok_ack_t)
#define DL_ERROR_ACK_SIZE		sizeof(dl_error_ack_t)
#define DL_CONNECT_REQ_SIZE		sizeof(dl_connect_req_t)
#define DL_CONNECT_IND_SIZE		sizeof(dl_connect_ind_t)
#define DL_CONNECT_RES_SIZE		sizeof(dl_connect_res_t)
#define DL_CONNECT_CON_SIZE		sizeof(dl_connect_con_t)
#define DL_TOKEN_REQ_SIZE		sizeof(dl_token_req_t)
#define DL_TOKEN_ACK_SIZE		sizeof(dl_token_ack_t)
#define DL_DISCONNECT_REQ_SIZE		sizeof(dl_disconnect_req_t)
#define DL_DISCONNECT_IND_SIZE		sizeof(dl_disconnect_ind_t)
#define DL_RESET_REQ_SIZE		sizeof(dl_reset_req_t)
#define DL_RESET_IND_SIZE		sizeof(dl_reset_ind_t)
#define DL_RESET_RES_SIZE		sizeof(dl_reset_res_t)
#define DL_RESET_CON_SIZE		sizeof(dl_reset_con_t)
#define DL_UNITDATA_REQ_SIZE		sizeof(dl_unitdata_req_t)
#define DL_UNITDATA_IND_SIZE		sizeof(dl_unitdata_ind_t)
#define DL_UDERROR_IND_SIZE		sizeof(dl_uderror_ind_t)
#define DL_UDQOS_REQ_SIZE		sizeof(dl_udqos_req_t)
#define DL_ENABMULTI_REQ_SIZE		sizeof(dl_enabmulti_req_t)
#define DL_DISABMULTI_REQ_SIZE		sizeof(dl_disabmulti_req_t)
#define DL_PROMISCON_REQ_SIZE		sizeof(dl_promiscon_req_t)
#define DL_PROMISCOFF_REQ_SIZE		sizeof(dl_promiscoff_req_t)
#define DL_PHYS_ADDR_REQ_SIZE		sizeof(dl_phys_addr_req_t)
#define DL_PHYS_ADDR_ACK_SIZE		sizeof(dl_phys_addr_ack_t)
#define DL_SET_PHYS_ADDR_REQ_SIZE	sizeof(dl_set_phys_addr_req_t)
#define DL_GET_STATISTICS_REQ_SIZE	sizeof(dl_get_statistics_req_t)
#define DL_GET_STATISTICS_ACK_SIZE	sizeof(dl_get_statistics_ack_t)
#define DL_XID_REQ_SIZE			sizeof(dl_xid_req_t)
#define DL_XID_IND_SIZE			sizeof(dl_xid_ind_t)
#define DL_XID_RES_SIZE			sizeof(dl_xid_res_t)
#define DL_XID_CON_SIZE			sizeof(dl_xid_con_t)
#define DL_TEST_REQ_SIZE		sizeof(dl_test_req_t)
#define DL_TEST_IND_SIZE		sizeof(dl_test_ind_t)
#define DL_TEST_RES_SIZE		sizeof(dl_test_res_t)
#define DL_TEST_CON_SIZE		sizeof(dl_test_con_t)
#define DL_DATA_ACK_REQ_SIZE		sizeof(dl_data_ack_req_t)
#define DL_DATA_ACK_IND_SIZE		sizeof(dl_data_ack_ind_t)
#define DL_DATA_ACK_STATUS_IND_SIZE	sizeof(dl_data_ack_status_ind_t)
#define DL_REPLY_REQ_SIZE		sizeof(dl_reply_req_t)
#define DL_REPLY_IND_SIZE		sizeof(dl_reply_ind_t)
#define DL_REPLY_STATUS_IND_SIZE	sizeof(dl_reply_status_ind_t)
#define DL_REPLY_UPDATE_REQ_SIZE	sizeof(dl_reply_update_req_t)
#define DL_REPLY_UPDATE_STATUS_IND_SIZE	sizeof(dl_reply_update_status_ind_t)
#define DL_MONITOR_LINK_LAYER_SIZE	sizeof(dl_monitor_link_layer_t) /* #3 */
#endif /* _SYS_DLPI_H */

