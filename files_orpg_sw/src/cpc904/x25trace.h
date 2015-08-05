/*   @(#)x25trace.h	1.1	07 Jul 1998	*/

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
 * x25trace.h of snet module
 *
 * SpiderX25
 * @(#)$Id: x25trace.h,v 1.1 2000/02/25 17:15:30 john Exp $
 * 
 * SpiderX25 Release 8
 */


/*
    IOCTL commands
*/
#define T_TRACEON   ('T'<<8 | 1)
#define T_TRACEOFF  ('T'<<8 | 2)

#define	EOS	'\0'
#ifndef NULL
#define	NULL	0
#endif /* NULL */
#define O_RW	2	/* File Mode for Read or Write */
#ifndef FALSE
#define	FALSE	0
#endif
#ifndef TRUE
#define	TRUE	1
#endif

/*
    M_IOCTL message type definitions 
*/

#define MAX_SUBNETS   10

struct trc_regioc {
    uint8     all_snworks;		/* Trace on all subnetworks	*/
    uint8     spare[3];			/* for alignment		*/
    uint32    snid;			/* Subnetwork			*/
    uint8     level;			/* Level of tracing required	*/
    uint8     spare2[3];		/* for alignment		*/
    uint32    active[MAX_SUBNETS+1];	/* tracing actively on 		*/
};

/*
    Levels of tracing available
*/
#define T_DATA    1              /* Trace all data tx and rx */


/*
 * Possible device names for all tracing options.
 */
#define	X25_DEV		"/dev/x25"

#define	MLP_DEV		"/dev/mlp"
#define	LAPB_DEV	"/dev/lapb"
#define	LLC2_DEV	"/dev/llc2"

#define	MAX_DEV_NM_LEN	16	/* maximum length of device name */
#define	DEV_TBL_SZ	6	/* Number of devices supported   */


/*
 * These structures are used to check devices and subnetworks.
 */
struct	dev_table
{
	char	name[MAX_DEV_NM_LEN] ;
	int	fd ;
	struct trc_regioc trace_reg;
} ;
struct	snid_table
{
	uint32	snid ;
	int	dev_ind ;	/* index into device table. */
} ; 


/*
 * Standard paths into the structures, shortened to save a poor typist.
 */
#define	TR_SNID(i)	x25_devs[i].trace_reg.snid

/*
    Types of tracing message
*/
#define TR_CTL       100            /* Basic                   */
#define TR_LLC2_DAT  101            /* Basic + LLC2 parameters */
#define TR_LAPB_DAT  TR_CTL         /* Basic for now           */
#define TR_MLP_DAT   TR_CTL         /* Basic for now           */
#define TR_X25_DAT   TR_CTL         /* Basic for now           */

/*
    Format for control part of trace messages
*/
struct trc_ctl {
    uint8           trc_prim;	 /* Trace msg identifier     */
    uint8           trc_mid;     /* Id of protocol module    */
    uint16          trc_spare;   /* for alignment	     */
    uint32          trc_snid;    /* Subnetwork Id            */
    uint8           trc_rcv;     /* Message tx or rx         */
    uint8           trc_spare2[3]; /* for alignment	     */
    uint32          trc_time;    /* Time stamp               */
    uint16          trc_seq;     /* Message seq number       */
};

struct trc_llc2_dat {
    uint8           trc_prim;	 /* Trace msg identifier     */
    uint8           trc_mid;     /* Id of protocol module    */
    uint16          trc_spare;   /* for alignment	     */
    uint32          trc_snid;    /* Subnetwork Id            */
    uint8           trc_rcv;     /* Message tx or rx         */
    uint8           trc_spare2[3];/* for alignment           */
    uint32          trc_time;    /* Time stamp               */
    uint16          trc_seq;     /* Message seq number       */
    uint8           trc_rmt[6];  /* Remote address           */
};

/*
    Generic trace protocol primitive
*/
typedef union trc_union {
    uint8                   trc_prim;	      /* Trace msg identifier        */
    struct trc_ctl	    trc_hdr;          /* Basic trace data            */
    struct trc_llc2_dat	    trc_llc2dat;      /* LLC2  trace data            */
} trc_types;
