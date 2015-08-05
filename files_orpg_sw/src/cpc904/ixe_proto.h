/*   @(#)ixe_proto.h	1.1	07 Jul 1998	*/

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
 * ixe_proto.h of snet module
 *
 * SpiderX25
 * @(#)$Id: ixe_proto.h,v 1.1 2000/02/25 17:14:24 john Exp $
 * 
 * SpiderX25 Release 8
 */

/******************************************************************
 *
 * M_PROTO message type definitions 
 *
 */

typedef union ixe_proto {
	int type;
	S_NET_TX	net_tx;		/* Defined in DL_PROTO.H	*/
	S_IP_RX		ip_rx;		/* Defined in DL_PROTO.H	*/
	S_SN_FRGSZ	sn_frgsz;	/* Defined in DL_PROTO.H	*/
	IP_DL_REG	ip_reg;
} S_IXE_PROTO;
