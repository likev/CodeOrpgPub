/*   @(#)wan_control.h	1.2	04 Sep 1998	*/

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
 * wan_control.h of snet module
 *
 * SpiderX25
 * @(#)$Id: wan_control.h,v 1.1 2000/02/25 17:15:16 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification History
Chg Date	Init Descriptio
1.  26-jun-98	rjp  Define the contents of the wan template file to NOT
		     include x.21 stuff, since we don't support it. Also,
		     replace WAN_translate with WAN_auto_enable.
2.  14-aug-98   rjp  Add dial-up support to W_ENABLE.
*/

#define WAN_STID	210
#define WLOOP_STID	211

/*
    Here re all of the valid states into which a DTE can enter
    during call set up.
 
    Note that all disconnected and data transfer states should have the
    same values throughout all call procedures
 
    First two basic states when no call procedures in use.
*/
#define HDLC_IDLE	0
#define HDLC_ESTB	30
#define HDLC_DISABLED	31
#define HDLC_CONN	40
#define HDLC_DISC	41


#define	W_ZEROSTATS	('W'<<8 | 1)
#define W_GETSTATS	('W'<<8 | 2)
#define	W_SETTUNE	('W'<<8 | 3)
#define W_GETTUNE	('W'<<8 | 4)
#define	W_PUTWANMAP	('W'<<8 | 5)
#define	W_GETWANMAP	('W'<<8 | 6)
#define	W_DELWANMAP	('W'<<8 | 7)
#define W_STATUS	('W'<<8 | 8)
#define W_ENABLE	('W'<<8 | 9)
#define W_DISABLE	('W'<<8 | 10)

#define WAN_STATS	0x34        /* Indicates 'struct wan_stioc' */
#define WAN_TUNE	0x35	    /* Indicates 'struct wan_tnioc' */
#define WAN_MAP		0x36	    /* Indicates 'struct wan_mpioc' */
#define WAN_PLAIN       0x37        /* Indicates 'struct hdioc'     */


/*
    WAN address mapping structure
*/
typedef struct wanmapf {
    uint8 remsize;        /* Rem address size (octets)        */
    uint8 remaddr[20];    /* Remote address                   */
    uint8 infsize;        /* Interface address size  (octets) */
    uint8 infaddr[30];    /* Interface address                */
} wanmap_t ;


/*
    WAN Enable/Disable
*/
struct wan_hdioc {
    uint8       w_type;      /* Table type = WAN_PLAIN                	*/
    uint8       w_spare[3];  /*   (for alignment)             		*/
    uint32      w_snid;      /* Subnetwork ID character               	*/
    wanmap_t    w_wanmap;    /* #2 Provide telephone number or key to one */
};

/*
    HDLC statistics (one per channel)
*/
typedef struct hstats
{
    uint32           hc_txgood;     /* Number of good frames transmitted    */
    uint32           hc_txurun;     /* Number of transmit underruns         */
    uint32           hc_rxgood;     /* Number of good frames received       */
    uint32           hc_rxorun;     /* Number of receive overruns           */
    uint32           hc_rxcrc;      /* Number of receive CRC/Framing errors */
    uint32           hc_rxnobuf;    /* Number of Rx frames with no buffer   */
    uint32           hc_rxnflow;    /* Number of Rx frames with no flow ctl */
    uint32           hc_rxoflow;    /* Number of receive buffer overflows   */
    uint32           hc_rxabort;    /* Number of receive aborts		    */
    uint32           hc_intframes;  /* Number of frames failed to be tx'd   */
				    /* due to loss of modem signals.        */
} hdlcstats_t;

/*
    Ioctl block for WAN L_GETSTATS command
*/
struct wan_stioc {
    uint8       w_type;      /* Always WAN_STATS		*/
    uint8	w_state;     /* line status HDLC_IDLE etc.      */
    uint8       w_spare[2];  /*   (for alignment)		*/
    uint32      w_snid;      /* Subnetwork ID character		*/
    hdlcstats_t hdlc_stats;  /* Table of HDLC stats values	*/
};


/*
    Values for WAN_options field
*/
#define TRANSLATE	1	/* Translate to interface address   */

/*
    Values for WAN_procs field

    N.b. these values are used to directly access the procedural interface
    table for all supported calling procedures. So do not change these values
    without changing the access method for the table.
*/
#define	WAN_NONE	0	/* No calling procedures	*/
#define WAN_X21P        1       /* X21 calling procedures       */
#define WAN_V25bis      2       /* V25bis calling procedures    */

/*
    Values for WAN_interface field
*/
#define WAN_X21         0
#define WAN_V28         1
#define WAN_V35         2


/*
	This contains all of the national network specific timeouts.
*/
struct  WAN_x21
{
    uint16 WAN_cptype;	/* Variant type.                        */
    uint16 T1;		/* Timer for call request state.        */
    uint16 T2;		/* Timer for EOS to data transfer.      */
    uint16 T3A;		/* Timer for call progress signals.     */
    uint16 T4B;		/* Timer for DCE provided info.         */
    uint16 T5;		/* Timer for DTE clear request.         */
    uint16 T6;		/* Timer for DTE clear conf. state.     */
};

/*
	This contains all of the national network specific timeouts.
*/
struct  WAN_v25
{
    uint16 WAN_cptype;	/* Variant type.        */
    uint16 callreq;	/* Abort time for call request command  *
			 * if network does not support CFI.     */
} ;


/*
	This is the structure which contains all tuneable information
	which is passed from user space to HDLC.

	#1 rjp Remove the x.21 stuff from the structure. Add WAN_auto_enable.

*/
struct	WAN_hddef
{
    uint16 WAN_maxframe;	/* WAN maximum frame size 	*/
    uint32 WAN_baud;		/* WAN baud rate		*/
    uint16 WAN_auto_enable;     /* WAN #1 Do we bring up lapb or not on REG */
#if 0
    uint16 WAN_interface;       /* WAN physical interface       */
#endif
    union {
	uint16         WAN_cptype;   /* Variant type */
#if 0				     /* #1 Leave it out			*/
	struct WAN_x21 WAN_x21def;
#endif
	struct WAN_v25 WAN_v25def;
        } WAN_cpdef ;                 /* WAN call procedural definition *
                                       * for hardware interface.	*/
};


/*
    WAN tuning structure
*/
typedef struct wantune
{
    uint16           WAN_options;  /* WAN options	*/
    struct WAN_hddef WAN_hd;	   /* HD information.	*/
} wantune_t;

/*
    Ioctl block for WAN W_SETTUNE command
*/
struct wan_tnioc {
    uint8     w_type;	  /* Always = WAN_TUNE	  	            */
    uint8     w_spare[3]; /* (for alignment)			    */
    uint32    w_snid;	  /* subnetwork id character ('*' => 'all') */
    wantune_t wan_tune;	  /* Table of tuning values		    */
};

#ifndef MAXIOCBSZ
#define MAXIOCBSZ     1024
#endif

#define WAN_MAP_SIZE   (MAXIOCBSZ - 8)
#define MAX_WAN_ENTS   (WAN_MAP_SIZE / sizeof(wanmap_t))
#define MAX_WAN_ADDRS   20

/*
    Ioctl block for WAN GET MAP command
*/
typedef struct wangetf {
    uint16   first_ent;             /* Where to start search   */
    uint16   num_ent;               /* Number entries returned */
    wanmap_t entries[MAX_WAN_ENTS]; /* Data buffer             */
} wanget_t;

struct wanmapgf {
    uint8    w_type;                /* Always WAN_MAP          */
    uint8    w_spare[3];            /* For alignment           */
    uint32   w_snid;                /* Subnetwork id character */
    wanget_t wan_ents;              /* Returned structure      */
};

/*
    Ioctl block for WAN PUT MAP command
*/
struct wanmappf {
    uint8    w_type;                /* Always WAN_MAP	       */
    uint8    w_spare[3];            /* For alignment	       */
    uint32   w_snid;                /* Subnetwork id character */
    wanmap_t wan_ent;               /* Mapping entry           */
};

/*
    Ioctl block for WAN DEL MAP command
*/
struct wanmapdf {
    uint8  w_type;		/* Always WAN_MAP          */
    uint8  w_spare[3];		/* For alignment           */
    uint32 w_snid;		/* Subnetwork id character */
};

