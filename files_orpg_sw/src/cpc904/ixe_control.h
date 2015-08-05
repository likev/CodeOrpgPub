/*   @(#)ixe_control.h	1.1	07 Jul 1998	*/

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
 * ixe_control.h of snet module
 *
 * SpiderX25
 * @(#)$Id: ixe_control.h,v 1.1 2000/02/25 17:14:23 john Exp $
 * 
 * SpiderX25 Release 8
 */

#define IXE_STID	208

/*
 * M_IOCTL message type constants
 *
 */

#define	IXE_MSG		('E' << 8)	/* prim_class for IXE msgs	 */

#define	IXE_SNREG	(IXE_MSG + 1)	/* register/configure subnetwork */
#define IXE_ENT		(IXE_MSG + 2)	/* put entry into table		 */
#define IXE_DEL		(IXE_MSG + 3)	/* delete entry from table	 */
#define IXE_GET		(IXE_MSG + 4)	/* return table entry		 */
#define IXE_MGET	(IXE_MSG + 5)	/* return all table entries	 */
#define IXE_RESET	(IXE_MSG + 8)	/* reset address table		 */
#define	IXE_STATS	(IXE_MSG + 6)	/* get statistics counts	 */
#define	IXE_STATINIT	(IXE_MSG + 7)	/* reset statistics counters	 */
#define IXE_SNUNREG	(IXE_MSG + 9)	/* deregister subnetwork         */

/*
 * Error Messages
 */

#define	EIXREG	6			/* Subnet registration err (?)	 */

/*
 * Exception handling action codes (for rst_action & exp_action)
 */

#define	DO_ACK		0
#define	DO_DISC		1
#define	DO_RESET	2		/* For exp_action only	*/

/* NSAP Flag values
 */

#define	THIS_NSAP	0	/* Use NSAP given			*/
#define	IP_NSAP		1	/* (not yet supported)			*/

/* Constants for defining timeouts
 */

#define	IX_TICK		(HZ)		/* IXE timer goes off once per sec. */
#define	IX_RATE		(HZ / IX_TICK)	/* No. of IXE timer events per sec. */
#define IX_PVC_TICKS	10	/* Number of ticks between */
				/* resends of pvc attaches */


/******************************************************************
 *
 * M_IOCTL message type definitions 
 *
 */

typedef struct ixe_sninfo {
	unsigned int	nsdu_hwm;	/* NSDU High Water Mark		*/
	unsigned int	idle_ticks;	/* Connection idle timeout	*/
	unsigned int	durn_ticks;	/* Min conn. duration		*/
	unsigned int    hold_down;      /* Hold Down Timer              */
	unsigned char	conform84;	/* X.25 mode to use (TRUE = 84)	*/
	unsigned char	addr_ddn;	/* Use DDN, not CCITT if true	*/
	unsigned char	allow_no_addr;	/* Allow temporary ent on CCITT */
	unsigned char	rst_action;	/* Action to take on reset	*/
	unsigned char	exp_action;	/* Action to take on expedited	*/
	struct xaddrf	my_xaddr;	/* Own X.25 address - used as	*/
					/* calling addr, and for net id	*/
	unsigned short  lpc;            /* Lower PVC range              */
	unsigned short  hpc;            /* Higher PVC range             */
} S_IXE_SNINFO;

typedef struct ixe_snreg {
	char		prim_class;	/* Always IXE_MSG		*/
	char		op;		/* Always IXE_SNREG		*/
	uint32		my_inaddr;	/* Own IP address for this net	*/
	struct ixe_sninfo sn_info;	/* Config params for this net	*/
					/* X25 address(es) to listen on	*/
					/* follow in here		*/
} S_IXE_SNREG;

typedef struct ixe_snunreg
{
        char            prim_class;     /* Always IXE_MSG               */
        char            op;             /* Always IXE_SNUNREG           */
        uint32            my_inaddr;      /* IP address to de-register    */
} S_IXE_SNUNREG;
 
/*
 * Address table manipulation primitives
 */

struct ixe_ent {
	char	prim_class;	/* Always IXE_MSG			*/
	char	op;		/* Always IXE_ENT			*/
	uint32	in_addr;	/* Remote node's IP address		*/
	struct xaddrf x_addr;	/* X.25 address (includes network id)	*/
	unsigned char nsap_flag;/* How to interpret X.25 address	*/
	unsigned char pkt_siz;	/* (Optional) X.25 packet and window 	*/
	unsigned char win_siz;	/*  sizes to request for this node 	*/
	unsigned char num_vcs;	/* (No. of Virtual Circuits to open)	*/
	unsigned char cug_type; /* Whether Bilateral CUG or not         */
	unsigned char cug_field[MAX_CUG_LEN]; /* CUG     or not         */
	unsigned char sub_rev_chrg; /* Subscribe to reverse charging    */
	unsigned char bar_rev_chrg;   /* Bar incoming reverse charge calls */
};

struct ixe_del {
	char	prim_class;	/* Always IXE_MSG			*/
	char	op;		/* Always IXE_DEL			*/
	uint32 	in_addr;	/* IP address of entry to delete	*/
};

struct ixe_get {
	char	prim_class;	/* Always IXE_MSG			*/
	char	op;		/* Always IXE_GET			*/
	uint32 	in_addr;	/* IP address of entry to get		*/

/* Following fields filled in in reply: (as for IXE_ENT)	*/

	struct xaddrf x_addr;
	unsigned char nsap_flag;
	unsigned char pkt_siz;
	unsigned char win_siz;
	unsigned char num_vcs;
	unsigned char cug_type; /* Whether Bilateral CUG or not         */
	unsigned char cug_field[MAX_CUG_LEN]; /* CUG     or not         */
	unsigned char sub_rev_chrg; /* Subscribe to reverse charging    */
	unsigned char bar_rev_chrg;   /* Bar incoming reverse charge calls */
};

struct ixe_reset {
	char		prim_class;	/* Always IXE_MSG	*/
	char		op;		/* Always IXE_RESET	*/
};


/*
 *  Table entry structure - used for mget
 */

typedef struct ix_addr {
	uint32 		in_addr;	/* IP address			*/
	struct xaddrf	x_addr;		/* X.25 addr (includes net id)	*/
	unsigned char	nsap_flag;	/* NSAP flag			*/
	unsigned char	pkt_siz;	/* X.25 packet and window	*/
	unsigned char	win_siz;	/*  sizes to request 		*/
	unsigned char	num_vcs;	/* (No. of VCs to open)		*/
	unsigned char   cug_type; /* Whether Bilateral CUG or not         */
	unsigned char   cug_field[MAX_CUG_LEN]; /* CUG     or not         */
	unsigned char   sub_rev_chrg;	/* Subscribe to reverse charging    */
	unsigned char   bar_rev_chrg;   /* Bar incoming reverse charge calls */
	unsigned char	status;		/* temporary entry or permanent */
	unsigned char   delete_pending; /* entry pending deletion       */
	time_t          fail_call_time; /* The time when a call to this */
					/* address last failed          */
	int     	next_free;	/* Next free entry              */
	uint32	        PeerIndex;	/* Index into Peer Table        */
} IX_ADDR;

#define IX_ADDR_SIZE	(sizeof(IX_ADDR) - sizeof(unsigned char) - sizeof(int))

#define IX_TEMPORARY	1	/* temporary address entry status	*/
#define IX_PERMANENT	2	/* permanent address entry status	*/

/*
 *  Defines for the Multiple Get IOCTL
 *
 *  NOTE: This size of structure "ixe_mget" must not
 *	  exceed MAXIOCBSZ byte (as defined in stream.h).
 *	  Thus if other fields are added to "ixe_mget"
 *	  then MGET_BUFSIZE must decrease to cope
 *	  with any changes made.
 */
#ifndef MAXIOCBSZ
#define MAXIOCBSZ       1024
#endif
 
#define MGET_BUFSIZE	(MAXIOCBSZ - 20)	/*
						 * Take into account the
						 * other 5 fields by reducing
						 * the buffer size by 20
						 */
#define MGET_MAXENT	(MGET_BUFSIZE / sizeof(IX_ADDR) ) 


struct ixe_mget {
	char	prim_class;	   /* Always IXE_MSG			*/
	char	op;		   /* Always IXE_MGET			*/
	unsigned int first_ent;	   /* First entry required		*/
	unsigned int last_ent; 	   /* Last entry required		*/
	unsigned int num_ent;	   /* Number of entries required	*/
	char 	buf[MGET_BUFSIZE]; /* Data Buffer			*/
};

typedef union ixe_adrop {
	char	prim_class;		/* Always IXE_MSG		*/
	struct	ixe_ent   ixe_ent;
	struct	ixe_del   ixe_del;
	struct	ixe_get   ixe_get;
	struct	ixe_mget  ixe_mget;
	struct	ixe_reset ixe_reset;
} S_IXE_ADROP;

/*
 *  Statistics gathering interface primitives
 */

/* Message to request/return counters	*/

struct ixe_stats {
	char		prim_class;	/* Always IXE_MSG		*/
	char		op;		/* Always IXE_STATS		*/
	unsigned int	ixcons_active;	/* No of current IXE conns open	*/
	uint32      	dgs_out,	/* Datagrams passed down by IP	*/
			dgs_in,		/* Datagrams passed up to IP	*/
			nsdus_in,	/* NSDUs passed up from X.25	*/
			nsdus_out;	/* NDUSs passed down to X.25	*/
};

/* Message to reset counters inside IXE	*/

struct ixe_statinit {
	char		prim_class;	/* Always IXE_MSG	*/
	char		op;		/* Always IXE_STATINIT	*/
};

typedef struct ix_peer
{
	uint32         *PleIndex;	/* Index into Ple Table           */
	uint32         callparm_index; /* X.25 Call Parm Id Index        */
	uint32         circ_index;	/* Index for X.25 Circuit Table   */
	uint32         circ_channel;	/* Channel for X.25 Circuit Table */
	uint32         connect_secs;	/* Time connected to this peer    */
	unsigned char   InUse;		/* Set if table entry in use      */
} IX_PEER;
