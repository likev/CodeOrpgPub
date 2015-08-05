/*   @(#)wan_proto.h	1.1	07 Jul 1998	*/

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
 * wan_proto.h of snet module
 *
 * SpiderX25
 * @(#)$Id: wan_proto.h,v 1.1 2000/02/25 17:15:19 john Exp $
 * 
 * SpiderX25 Release 8
 */


/*
    WAN primitive type values
*/
#define WAN_SID           1
#define WAN_REG		  2
#define WAN_CTL           3
#define WAN_DAT           4

/*
    WAN_CTL type command values/events pairings
*/
#define WC_CONNECT        1
#define WC_CONCNF         2
#define WC_DISC           3
#define WC_DISCCNF        4

#define wconnect          0x01
#define wconcnf           0x02
#define wdisc             0x04
#define wdisccnf          0x08

/*
    WAN_DAT type command values
*/
#define WC_TX		  1
#define WC_RX		  2

/*
    Values for wan_status field
*/
#define WAN_FAIL          0
#define WAN_SUCCESS       1

/*
    Values for wan_remtype field
*/
#define WAN_TYPE_ASC    0          /* Ascii length in octets       */
#define WAN_TYPE_BCD    1          /* BCD   length in semi octets  */


/*
    SETSNID message for WAN
*/
struct wan_sid {
    uint8  wan_type;           /* WAN_SID                  */
    uint8  wan_spare[3];       /* for alignment		  */
    uint32 wan_snid;           /* Subnetwork ID for stream */
};

/*
    Registration message for WAN
*/
struct wan_reg {
    uint8  wan_type;           /* WAN_REG                        */
    uint8  wan_spare[3];       /* for alignment		  */
    uint32 wan_snid;           /* Subnetwork ID for the WAN line */
};

/*
    WAN Connection Control
*/
struct wan_ctl {
    uint8 wan_type;           /* WAN_CTL                        */
    uint8 wan_command;        /* WC_CONNECT/CONCNF/DISC/DISCCNF */
    uint8 wan_remtype;        /* Octets or semi octets          */
    uint8 wan_remsize;        /* Remote address length          */
    uint8 wan_remaddr[20];    /* Remote address                 */
    uint8 wan_status;         /* Result status                  */
    uint8 wan_diag;	      /* Extra Diagnostic		*/
};

/*
    WAN Data messages - Transmit and Receive
*/
struct wan_msg {
    uint8 wan_type;           /* WAN_DAT     */
    uint8 wan_command;        /* WC_TX/WC_RX */
};


/*
    Generic WAN protocol primitive
*/
union WAN_primitives {
    uint8 wan_type;	      /* Variant type             */
    struct wan_reg   wreg;    /* WAN Registration         */
    struct wan_sid   wsid;    /* WAN Subnetwork ID        */
    struct wan_ctl   wctl;    /* WAN Connection Control   */
    struct wan_msg   wmsg;    /* WAN Data Message         */
};

