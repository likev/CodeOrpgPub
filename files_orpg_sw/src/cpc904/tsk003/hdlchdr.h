/*   @(#) hdlchdr.h 99/11/02 Version 1.28   */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1991 by
 
               +++    +++                           +++     +++
               +++    +++                           +++     +++
               +++    +++                            +++   +++ 
               +++    +++   +++++     + +    +++   +++++   +++ 
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
               +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
               +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
               +++    ++++++      +++++ ++++++++++ +++ +++++   
               +++    ++++++      +++++ ++++++++++ +++  +++    
               +++    ++++++      ++++   +++++++++ +++  +++    
               +++    ++++++                             +     
               +++    ++++++      ++++   +++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++ +++++   
               +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
               +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
                +++  +++    +++++     + +    +++   ++++++  +++ 
                ++++++++                             +++    +++
                 ++++++         Corporation         ++++    ++++
                  ++++   All the right connections  +++      +++
 
 

       This software is furnished  under  a  license and may be used and
       copied only  in  accordance  with  the  terms of such license and
       with the inclusion of the above copyright notice.   This software
       or any other copies thereof may not be provided or otherwise made
       available to any other person.   No title to and ownership of the
       program is hereby transferred.
 
       The information  in  this  software  is subject to change without
       notice  and  should  not be considered as a commitment by UconX
       Corporation.
   
       UconX Corporation
       San Diego, California
                                                
                                                 
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

 
/*
Modification history:
 
Chg Date       Init Description
1.  22-AUG-96  rjp  Added code to support processing ioctl to get statistics.
		    Got rid of ABORTS ifdef-ing.
2.  28-AUG-96  rjp  Corrected value of STAT_REQ.
3.  10-MAR-97  lmm  Added define for NO_MEMORY reason in START_FAIL response
4.  04-Nov-97  pmt  Added SNTP timestamp field to header.
5.  23-Dec-97  lmm  Added defines for nrz options
6.  30-jan-98  pmt  Single header for all timestamps
7.  06-feb-98  pmt  undo #7.
8.  05-mar-98  lmm  Remove reserve area in hdlc header (added in change #6)
9.  22-jun-98  lmm  All bit16's in CONFIG struct changed to bit32's 
                    Added LINK_UP define
10. 05-aug-98  pmt  Support zeroing of stats via IOCTL.
11. 03-sep-98  rjp  Added DIAL_LINK command and DIAL_LINK_HDR. 
12. 30-oct-98  lmm  Added defines to support new extended encoding schemes 
                    Added defines for mode parameter (normal, data only) 
13. 07-dec-98  dp   Added defines for T1 configuration IOCTL
14. 29-jan-99  lmm  Added defines/structs for T1/E1 stats request
15. 10-feb-99  lmm  Added HDLC stats to T1 stats
16. 19-feb-99  lmm  Added FISU_SUPPRESS mode
17. 15-jun-99  lmm  Added VALID_MODES define
18. 07-jul-99  lmm  Added LOCAL_LOOPBACK mode (n/a for T1/E1)
19. 13-jul-99  lmm  Added "rfisu" stat to track number of FISU's discarded
                    (applies to Quicc/PQuicc HDLC Serial Driver only)
20. 23-sep-99  lmm  Added NRZ and NRZI with clock encoding modes
*/
 

#ifndef	_hdlchdr_h
#define	_hdlchdr_h

#define N_V25_CALLING 32		/* #11				*/

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	HDLC Header -- for all M_DATA messages in the stream
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

typedef struct
{
   int command;
   int status;
   int count;
   int time_stamp;
   struct utimeval sntp_timestamp;		/* pmt #4 */
#ifdef BANCOMM_TS
   bit8 card_timestamp[10];			/* pmt #6 */
#endif
} HDLC_HDR;

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	DIAL_LINK_HDR -- ask hdlc to dial a v.25bis compatible modem.
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/
 
typedef struct				/* #11. V.25 support		*/
{
    int 	command;		/* HDLC command			*/
    unsigned	ph_size;		/* Number of bytes in number	*/
    char	ph_number [N_V25_CALLING];  /* Telephone number		*/
} HDLC_DIAL_LINK;

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Values for command field
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/* Client Requests */

#define SEND_DATA	0
#define RCV_DATA	1
#define OPEN_LINK	2
#define START_LINK	3
#define STOP_LINK	4
#define CLOSE_LINK	5
#define STAT_REQ	6
#define CLEAR_STAT	7
#define DIAL_LINK	8		/* #11 */
#define T1CONFIG_REQ	9		/* #13 */
#define T1STATS_REQ    10		/* #14 */
#define E1CONFIG_REQ   11		/* #13 */
#define E1STATS_REQ    12		/* #14 */
#define RESET_LINK	0x40

/* Server Replies */

#define LINK_OPEN	0x80
#define OPEN_FAIL	0x81
#define SEND_FAIL	0x82
#define STATISTICS	0x83
#define LINK_UP         0x84	/* #9 */
#define LINK_ESTB	LINK_UP	/* #9 */
#define LINK_DOWN	0x85
#define START_FAIL      0x86
#define LINK_CLOSED	0x87
#define LINK_STOPPED	0x89


/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Values for status field
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#define OK		0	/* good completion */
#define NOT_OPEN	1	/* stream not associated with a link */
#define IN_USE		2	/* link already in use */
#define TOO_BIG		3	/* frame larger than max configured */
#define NO_MEMORY	4	/* no memory for DMA buffers - #3 */ 
#define NO_MODEM_CONFIG	5	/* #11 Trying to use v.25 w/o modem sigs */
#define NO_SIGNALS      6       /* no signals detected */
#define NOT_AVAIL       7       /* link is not available */
#define MAX_STATUS_VAL  7	/* Max status value */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Values for ioctl commands (iocblk_t ioc_cmd)
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#define H_STAT_REQ   (('H'<<4) | STAT_REQ)
#define H_CLEAR_STAT (('H'<<4) | CLEAR_STAT)      /* #10 */
#define T_CONFIG_REQ (('H'<<4) | T1CONFIG_REQ)    /* #13 */
#define T_STATS_REQ  (('H'<<4) | T1STATS_REQ)     /* #14 */
#define E_CONFIG_REQ (('H'<<4) | E1CONFIG_REQ)    /* #14 */
#define E_STATS_REQ  (('H'<<4) | E1STATS_REQ)     /* #14 */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Configuration Structure -- in data portion of OPEN_LINK request.
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/


/* #9 - All bit16's in this structure changed to bit32's */
typedef struct
{
   bit32	link;
   bit32	framesize;	/* max frame size */
   bit32	baud;		/* baud rate time constant 
				   ( -1  for external clock) */
   bit32        encoding;       /* #12 - encoding method (see defines below) */
#define nrznrzi encoding	/* #12 - maintain old name */

/* #12 - Added defines to support new encoding options (QUICC platforms) */

#define NRZ_MODE	  0
#define NRZI_MODE	  1
#define FM0_MODE	  2	/* QUICC platforms only */
#define FM1_MODE          3	/* QUICC platforms only */
#define MANCHESTER_MODE   4	/* QUICC platforms only */
#define DMANCHESTER_MODE  5	/* QUICC platforms only */
#define NRZ_CLOCK_MODE    6	/* #20 - QUICC platforms only */
#define NRZI_CLOCK_MODE   7	/* #20 - QUICC platforms only */
#define MAX_ENCODING_MODE 7	/* #20 */

/* #12 - Provide capability to run in a data only mode (no headers) */
 
   bit32	mode;		/* mode parameters (bitmask) */
#define CTRL_DATA_MODE	0	/* bit 0 = 0 => Ctrl & Data (Protos) */
#define DATA_ONLY_MODE	1	/* bit 0 = 1 => Data Only */
#define FISU_SUPPRESS   2  	/* bit 1 - set for supressing FISU's */
#define LOCAL_LOOPBACK  4	/* bit 2 - set for internal loopback */

/* #17 - Define valid modes */
#define VALID_MODES ( DATA_ONLY_MODE | FISU_SUPPRESS | LOCAL_LOOPBACK )

   bit32	modem;		/* 0 = Don't care, 1 = use signals */
} CONFIG;

typedef struct
{
   bit32        xgood;          /* correctly transmitted frames */
   bit32        rgood;          /* correctly received frames */
   bit32        xunder;         /* transmit underrun count (SCC only) */
#define rfisu   xunder          /* received FISU's discarded (Quicc/PQuicc) */
   bit32        rover;          /* receive overrun count */
   bit32        rlength;        /* receive frame length violation count */
#define rtoo	rlength         /* for older apps */
   bit32        rcrc;           /* receive CRC error */
   bit32        rabt;           /* receive aborts */
} LINK_STAT;

 
/*
    Ioctl block for HDLC H_STAT_REQ command
*/
struct hdlc_stioc 
{
    int		link;      	/* Subnetwork ID character           */
    LINK_STAT   hdlc_stats; 	/* Table of HDLC stats values        */
};


typedef struct		/* T1/E1 CONFIG structure */
{
   bit32 span;		/* span (0 or 1) */
   bit32 func;          /* set or get parameters */
#define T1_SETFIG	0
#define T1_GETFIG	1
#define E1_SETFIG	T1_SETFIG	
#define E1_GETFIG	T1_GETFIG	
   bit32 buildout;	/* line buildout (0-7) */
   bit32 framing;	/* framing (T1: Extended Super Frame or SuperFrame) */
#define T1_FRAMING_ESF	0x88
#define T1_FRAMING_SF	0x00
   bit32 encoding;      /* encoding (T1: B8ZS or AMI, E1: HDB3 or NONE) */
#define T1_ENCODE_B8ZS	0x44
#define T1_ENCODE_AMI	0x00
#define E1_ENCODE_HDB3	0x44
#define E1_ENCODE_NONE	0x00
   bit32 crc4;      	/* CRC4 (E1: on/off) */
#define E1_CRC4_ON	0x11
#define E1_CRC4_OFF     0x00
   bit32 signaling;    	/* signaling (E1: CCS or CAS signaling) */
#define E1_SIGNAL_CCS	0x08
#define E1_SIGNAL_CAS   0x00
   bit32 alarms;	/* alarms (on/off) */
   bit32 clocking;	/* clock, internal, external, recovered (from span) */
#define	TXC_IS_TXC	0	/* use onboard clock          */
#define	TXC_IS_RXC	1	/* use incoming receive clock (recovered) */
#define TXC_IS_EXT	2	/* use BITS (external) clock  */
   bit32 maxframe;	/* global max frame size over all channels */
   bit32 timeslot[32];	/* channel assignments on a per timeslot basis */
   bit32 bitrate;	/* 56kbit or 64kbit bit map */
   bit32 bitmask;       /* 56kbit mask of lsbit or msbit/timeslot */
   bit32 ccr1;		/* raw common control register 1   */
   bit32 ccr2;		/* raw common control register 2   */
   bit32 ccr3;		/* raw common control register 3   */
   bit32 rcr1;		/* raw recieve control register 1  */
   bit32 rcr2;		/* raw recieve control register 2  */
   bit32 tcr1;		/* raw transmit control register 1 */
   bit32 tcr2; 		/* raw transmit control register 2 */

} T1CONFIG, E1CONFIG;

typedef struct		/* T1/E1 Stats structure */
{
   bit32 span;          /* span */
   bit32 func;          /* if = 1, clear stats after returning values */
#define DONT_CLEAR_NTWK_STATS 0
#define CLEAR_NTWK_STATS      1
   bit32 ntwk_ses;      /* severely errored seconds */
   bit32 ntwk_ers;      /* errored seconds */
   bit32 ntwk_bpv;      /* line code violations/bipolar violations */
   bit32 ntwk_crc;      /* CRC errors */
   bit32 ntwk_oof;      /* Out Of Frame errors */
#define NTWK_E1 0x1
#define NTWK_T1 0x2
   bit32 ntwk_type;     /* T1 or E1 network */
   bit32 ntwk_status;   /* framer status ( sr1 of framer ) */
   LINK_STAT   hdlc_stats; 	/* #15 - Table of HDLC stats values */
} T1STATS, E1STATS;

#endif	/* _hdlchdr_h */
