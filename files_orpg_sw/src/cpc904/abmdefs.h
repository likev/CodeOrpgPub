/*   @(#) abmdefs.h 99/12/23 Version 1.5   */
/* Events associated with primary and secondary states */

#define	LINK_SET	1
#define	LINK_RCVA	2
#define	LINK_RCVB	3
#define	LINK_T1_TO	4
#define LINK_SETUP	5
#define LINK_REQUEST	6
#define LINK_XDISC   	7
#define	LINK_T3_TO	8
#define	LINK_UPDATE	9

#define TRUE		1
#define FALSE		0
#define MOD8_MASK	7   /* mask to count variables and states cyclicly */
#define MOD128_MASK	127   /* mask to count variables and states cyclicly */

/* Low and high water marks for RNR condition */

#define	RNR_LOW		20
#define	RNR_HIGH	40

#define MAX_S_C_FRAME 6	/* 4 bytes for address, 2 bytes for ctl; modified from
			   10 bytes. pmt  */

/* Control field - these fields are redundant.  */

#define CFRAME	0x01	/* This bit is set for control frames */
#define UFRAME	0x02	/* This bit is set for unnumbered frames */

#define SFMASK	0x0F	/* Projects to supervisory frame type */
#define UFMASK	0xEF	/* Projects to unnumbered frame type */

/* Supervisory commands (LAP) */

#define RR	1
#define RNR	5
#define REJ	9
#define SREJ	13

/* Unnumbered Commands */

#define DISC 0x43
#define DM   0x0F
#define SABM 0x2F
#define SABME 0x6F
#define SARM 0x0F
#define UA   0x63
#define FRMR 0x87
#define CMDR 0x87
#define SIM  0x07


/* FRMR/CMDR FRAME FORMAT */

#define W_BIT   1 /* set  the W bit in frmr_cause */
#define Y_BIT   4 /* likewise for Y */
#define Z_BIT   8 /* ditto Z (Invalid N'(R) in LAPB) */
#define X_and_W 3 /* set X and W - no X without W */


/* bit to set if FRMR in reply to a response; reply to command is not set */
#define RESP_IND 0x10

/* masks for bits in the frames */
/* mask for the P/F bit (bit 4) */

#define POLLBIT  0x10

/* mask to get command out of the control field */

#define CMD_MASK 0xEF /* mask out poll/final bit */
#define NIBB      0xF /* get the supervisory command */

/* masks to get N'(R) and N'(S) from the control field */

#define NR_MASK 0xe0
#define NS_MASK 0x0e

/* bits for different categories of frames */

#define C_FRAME_BIT 1 /* 0 for I-frame, 1 for S- or U-frame */
#define U_FRAME_BIT 2 /* 0 on an S-frame, 1 for U-frame */
#define U_and_C     3 /* U bit and C bit */

/* GLOBAL LINK STATUS */

#define	LINK_UNDEF   0
#define LINK_DEAD    1
#define LINK_isUP    2		/* Formerly was LINK_UP */
#define LINK_isDOWN  3
#define	PRI_RESET    4
#define	LINK_DISC    5

/********************************************************/

/* THE FOLLOWING CONSTANTS ARE NOT TO BE CHANGED
 * THEY REPRESENT POSITIONS IN THE BRANCH TABLES
 *
 ********************************************************/

/* branch table in the DTE primary
 * the initiator responds to a command according to the
 * substate it is in and the command it receives
 */

/*
* LAPB values
* These values are used to dimension the ibranch table, ie, the code reads
*
*     ibranch [PRI_STATE] [PRI_CMD]
*
* The 'substate' values below range from NORMAL (0) through WAIT_SETM (3),
* and are used as the first subscript in the ibranch array.
*
* The 'command' values below range from DCE_RR (0) through IL2DOWN (7). The
* literal, IL2DOWN is beyond the table, ie, it is not a valid second
* subscript to ibranch.
*
* The code tests for the IL2DOWN value before using it as a subscript. For
* the IL2DOWN case, it skips the switch based on ibranch.
*/
#define PRI_STATE   4
#define PRI_CMD     7

/* values for substate */

#define NORMAL      0 
#define NRBUSY      1 /* DTE secondary busy */
#define REC_TIMEOUT 2 /* in timeout recovery */
#define WAIT_SETM   3 /* frmr sent; waiting for SABM */

/* values for command - icmd */

#define DCE_RR     0
#define DCE_RNR    1
#define DCE_REJ    2
#define I_IGNORE   3
#define IREJ       4 /* bad input to DTE primary */
#define IRESET     5 /* reset link (LAPB) */
#define	DCE_SREJ   6

#define	IL2DOWN	   7 /* I changed value from 6 to 7 when I added DCE_SREJ */
		     /*	This does not appear to be in the state table ibranch */
		     /* Since DCE_SREJ is in state table, I need next value */	

/* values for case labels */

#define RRNORMAL    1
#define RNRCASE     2
#define REJ_NORMAL  3
#define TIMEOUT     5
#define RRBUSY      8
#define IGNORE      9
#define I_REJECT   10
#define REJ_BUSY   11
#define I_RESET    12
#define RNR_RNR    13
#define SREJ_NORMAL 14
#define SREJ_BUSY  15

/* branch table for the DTE secondary.
 */
#define SEC_STATE 7 /* number of substates in DTE secondary */
#define SEC_CMD   6 /* number of commands */

/* values for substates */

#define DTE_DISC 0 /* DTE initiated disconnect */
#define SEC_DOWN 1 /* link properly disc'd being set up */
#define SEC_RR   2 /* link is up and ready */
#define SEC_RNR  3 /* link not ready (pktin is full) */
#define SEC_REJ  4 /* link up and ready, bad N'(S) received */
#define SEC_ERR  5 /* link error. reset or disconnect */
#define SEC_SREJ 6 /* link up and ready, bad N'(S) rec'd, srej state */

/* values for commands */

#define DTE_RR   0 /* RR reply to I-frame from the node */
#define DTE_RNR  1 /* RNR reply, cos pktin is full */
#define DTE_REJ  2 /* REJ reply N'(S) out of sequence */
#define DCE_SETM 3 /* mode set (SABM/SARM) from DCE */
#define DCE_DISC 4 /* DISC from DCE */
#define DTE_CMDR 5 /* incorrect frame format */

/* case labels */

#define RR_RR     1
#define RNR_RR    2
#define RR_RNR    3
/* RNR_RNR = 13 (defined for DTE primary) */
#define RR_REJ    4
#define RNR_REJ   5
#define REJ_REJ   6
#define REJ_RR    7
#define SEC_RESET 8
/* IGNORE = 9 (see DTE primary) */
#define SEC_DISC  10
#define SEC_CMDR  11 /* secondary rejection state */
#define SEC_SETUP 12 /* setup delay ready, till primary's done */
#define SEND_DM  14 /* link is disconnected LAPB sends DM */

/* END OF THE STATE MACHINE TABLES */

/* set mode - different commands in LAPB and LAP */

#define SETM SABM

/* states for the slots in the primary */

#define S1 0 /* slot is empty and available */
#define S2 1 /* slot is in use, frame is back in the primary, wait ack */
