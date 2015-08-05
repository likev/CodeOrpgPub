/*   @(#) xstra.h 99/12/23 Version 1.16   */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1992 by
 
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
1.  19-MAY-97  mpb  Have one common PANIC() define for all modules.  In
                    theory all modules should be calling xstra.h, so put it
                    here.
2.  04-MAR-98  lmm  Added external dec for alloc_proto
3.  24-MAR-98  rjp  For powerquicc, optimize the queue_t and TASL for speed
		    by defining the elements to be words, not bytes and shorts.
4.  15-JUN-98  lmm  Made queue states bit vals, eliminate QCLOSING, QNEVENB
                    Eliminate TASL struct and misc unused defs
		    Added define for CLASS_RANGE, BCMAX, S_PUTQ macros uppercase
5.  15-DEC-98  lmm  Added spl4 for PQ; removed refs to POWERQUICC (obsolete)
6.  13-JAN-98  lmm  Added splhi define for PQ
7.  18-MAR-99  lmm  Changed all bit16's in queue_t struct to bit32's 
                    (struct is now same for 68K and Pquicc platforms)
8.  05-APR-99  lmm  Changed shorts to ints, and bit16's to bit32's in 
                    module_info struct
9.  11-MAY-99  lmm  Interrupt masks for misc PQ platforms
*/
 

#ifndef _xstra_h
#define _xstra_h

#ifdef LINUX
#ifndef BITS_DEFINED
#define BITS_DEFINED
typedef unsigned char bit8;
typedef unsigned short bit16;
typedef unsigned long bit32;
#endif
#endif

/* #1 */
#define PANIC(a) { printf("\n%s\n", a); panic(); }

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Queue Structure
 
NOTE:   There is assembler language code that accesses the queue structure
        of the "current" queue. So, if the queue structure is CHANGED the
        assembler code in the /usr/UconX/xstra directory must be revised.

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/* #7 - Changed all bit16's in queue_t struct to bit32's 
        (struct is now same for 68K and Pquicc platforms) */

typedef struct	queue 
{
	struct	qinit	*q_qinfo;	/* queue config information */
	struct	msgb	*q_first;	/* -> message at head of queue */
	struct	msgb	*q_last;	/* -> message at tail of queue */
	struct	queue	*q_next;	/* -> next queue in stream */
	struct	queue	*q_link;	/* -> next queue on schedule or sleep 
					      list */
	char    	*q_ptr;		/* -> private data structure */
        bit32           q_count;        /* number of bytes in all messages */
        bit32           q_flag;         /* queue state */
        int             q_minpsz;       /* min packet size accepted */
        int             q_maxpsz;       /* max packet size accepted */
        bit32           q_hiwat;        /* high water mark */
        bit32           q_lowat;        /* low water mark */
        bit32           q_state;        /* state flag for scheduling */
        bit32           q_pri;          /* priority */
        int             q_slice;        /* ticks remaining in time slice */
	struct ustack	*q_stack;	/* address of start of stack */
	bit32		*q_sp;		/* stack pointer saved on preemption */
	bit32		q_sleep;	/* sleep parameter */
} queue_t;

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Queue flags (q_flag)
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#define	QENAB		1		/* Queue is already enabled to run */
#define	QWANTR		2		/* Someone wants to read Q */
#define	QWANTW		4		/* Someone wants to write Q */
#define	QFULL		8		/* Q is considered full */
#define	QREADR		0x10		/* This is the reader (first) Q */
#define QUSE		0x20		/* Set while allocated */
#define	QNOENB		0x40		/* Enable Q on putq only for high
					   priority messages */
#define QLOCK		0x80		/* Queue wants preemption lock restored
					   after sleep */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Queue States (q_state)

  #4 - Made these defines bit values to faciliate checks by xstra

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#define	IDLE		0	/* idle */
#define	SCHEDULED	1	/* on schedule list */
#define	RUNNING		2	/* current running queue */
#define	RUNSCHED	4	/* current running queue, on list to 
				   run again */
#define	PREEMPTED	8	/* was running, has been preempted */
#define	PRESCHED	0x10	/* preempted while in RUNSCHED state, or
				   scheduled while preempted */
#define SLEEPING	0x20	/* sleeping, on sleep list */
#define SLPSCHED	0x40	/* scheduled while sleeping or went to
				   sleep while scheduled */

#define ANYSCHED	(SCHEDULED | RUNSCHED | PRESCHED | SLPSCHED)  /* #4 */
/*

Q  = Add to schedule list	
DQ = Remove from schedule list
SQ = Add to sleep list
SD = Remove from sleep list

State		Schedule		Dispatch		Preempt		
-----		--------		--------		-------		
IDLE		Q, SCHEDULED		    X			   X
SCHEDULED	   --			DQ, RUNNING		   X		
RUNNING		Q, RUNSCHED		    X			Q, PREEMPTED
PREEMPTED	PRESCHED		DQ, RUNNING		   X	
RUNSCHED	   --			    X			PRESCHED
PRESCHED	   --			DQ & Q,	RUNSCHED	   X	
SLEEPING	SLPSCHED		    X			   X
SLPSCHED	   --			    X			   X

		 Sleep			Wakeup			Complete
		------ 			------			--------
IDLE		   X			   X			   X
SHCEDULED	   X			   X			   X
RUNNING		SQ, SLEEPING		   X			  IDLE
PREEMPTED	   X			   X			   X
RUNSCHED	DQ & SQ, SLPSCHED	   X			SCHEDULED
PRESCHED	   X			   X			   X
SLEEPING	   X			SD & Q, PREEMPTED	   X
SLPSCHED	   X			SD & Q, PRESCHED	   X

*/

/*
 *	Dispatch Queue Structure
 */

typedef struct
{
	queue_t *sch_head;
	queue_t *sch_tail;
	int	sch_count;
} SCHED_ELEM;

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Format of structure used to maintain list of free user stacks
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

typedef struct ustack
{
   struct ustack 	*stk_next;	/* next stack on free list */
   bit32		*stk_sp;	/* initial stack pointer (end
					   of stack + 1) */
} USER_STACK;

/*
 * module information structure
 */
struct module_info 
{
	bit16	mi_idnum;		/* module id number */
	char	*mi_idname;		/* module name */
	int     mi_minpsz;		/* min packet size accepted - #8 */
	int     mi_maxpsz;		/* max packet size accepted - #8 */
	bit32	mi_hiwat;		/* hi-water mark - #8 */
	bit32	mi_lowat;		/* lo-water mark - #8 */
};

/*
 * queue information structure
 */
struct	qinit 
{
	int	(*qi_putp)();		/* put procedure */
	int	(*qi_srvp)();		/* service procedure */
	int	(*qi_qopen)();		/* called on startup */
	int	(*qi_qclose)();		/* called on finish */
	int	(*qi_qadmin)();		/* not used (reserved) */
	struct module_info *qi_minfo;	/* module information structure */
	struct module_stat *qi_mstat;	/* not used (reserved) */
};


/*
 * Streamtab (central structure used to locate module or driver)
 */

struct streamtab 
{
	struct qinit *st_rdinit;
	struct qinit *st_wrinit;
	struct qinit *st_muxrinit;
	struct qinit *st_muxwinit;
};

/*
 * structure for tracking multiplexed streams
 */
struct linkblk 
{
	queue_t *l_qtop;	/* lowest level write queue of upper stream */
	queue_t *l_qbot;	/* highest level write queue of lower stream */
	int      l_index;	/* link ID of lower stream */
	int      l_sid;		/* stream ID of lower stream 
				   (for system use only) */
};

/*
 *  Data block descriptor
 */

typedef struct datab 
{
	struct datab	*db_freep;
	bit8		*db_base;
	bit8		*db_lim;
	bit16		db_ref;
	bit8		db_type;
	bit8		db_class;
} dblk_t;

/*
 * Message block descriptor
 */

typedef struct msgb 
{
	struct	msgb	*b_next;
	struct  msgb	*b_prev;
	struct	msgb	*b_cont;
	bit8		*b_rptr;
	bit8		*b_wptr;
	struct datab	*b_datap;
} mblk_t;

/*
 * Data block allocation information.  Defines cutoffs for allocation
 * priorities; bufcall lists.
 */

struct dbalcst 
{
	int dba_cnt;		/* number of blocks currently allocated */
	int dba_lo;		/* cutoff for low priority requests */
	int dba_med;		/* cutoff for medium priority requests */
	struct bc_msg *dba_lop;	/* bufcalls waiting at low priority */
	struct bc_msg *dba_medp;/* bufcalls waiting at medium priority */
	struct bc_msg *dba_hip;	/* bufcalls waiting at high priority */
};


/************************************************************************/
/*			Streams message types				*/
/************************************************************************/


/*
 * Data and protocol messages (regular priority)
 */
#define	M_DATA		00		/* regular data */
#define M_PROTO		01		/* protocol control */

/*
 * Control messages (regular priority)
 */
#define	M_BREAK		0x08		/* line break */
#define M_PASSFP	0x09		/* pass file pointer */
#define	M_SIG		0x0b		/* generate process signal */
#define	M_DELAY		0x0c		/* real-time xmit delay (1 param) */
#define M_CTL		0x0d		/* device-specific control message */
#define	M_IOCTL		0x0e		/* ioctl; set/get params */
#define M_SETOPTS	0x10		/* set various stream head options */
#define M_RETYPE	0x3f		/* retype input			*/


/*
 * Control messages (high priority; go to head of queue)
 */
#define	M_IOCACK	0x81		/* acknowledge ioctl */
#define	M_IOCNAK	0x82		/* negative ioctl acknowledge */
#define M_PCPROTO	0x83		/* priority proto message */
#define	M_PCSIG		0x84		/* generate process signal */
#define	M_FLUSH		0x86		/* flush your queues */
#define	M_STOP		0x87		/* stop transmission immediately */
#define	M_START		0x88		/* restart transmission after stop */
#define	M_HANGUP	0x89		/* line disconnect */
#define M_ERROR		0x8a		/* fatal error used to set u.u_error */
#define M_CLRERROR	0x8b		/* clear error condition */


/*
 * Queue message class definitions.
 */
#define QNORM    0			/* normal messages */
#define QPCTL 0x80			/* priority cntrl messages */

/*
 *  IOCTL structure - this structure is the format of the M_IOCTL message type.
 */
struct iocblk 
{
	int	ioc_cmd;		/* ioctl command type */
	bit16	ioc_uid;		/* effective uid of user */
	bit16	ioc_gid;		/* effective gid of user */
	bit32	ioc_id;			/* ioctl id */
	bit32	ioc_count;		/* count of bytes in data field */
	int	ioc_error;		/* error code */
	int	ioc_rval;		/* return value  */
};

/*
 * Options structure for M_SETOPTS message.  This is sent upstream
 * by driver to set stream head options.
 *
 * M_SETOPTS is not currently supported.
 */
struct stroptions 
{
	short so_flags;			/* options to set */
	short so_readopt;		/* read option */
	bit16 so_wroff;			/* write offset */
	short so_minpsz;		/* minimum read packet size */
	short so_maxpsz;		/* maximum read packet size */
	bit16 so_hiwat;			/* read queue high water mark */
	bit16 so_lowat;			/* read queue low water mark */

	short	so_vtime;		/* polling read parameter */
};

/* 
 * flags for stream options-set message
 */

#define SO_READOPT	   1		/* set read option */
#define SO_WROFF	   2		/* set write offset */
#define SO_MINPSZ	   4		/* set min packet size */
#define SO_MAXPSZ	   8		/* set max packet size */
#define SO_HIWAT	0x10		/* set high water mark */
#define SO_LOWAT	0x20		/* set low water mark */


/************************************************************************/
/*		Miscellaneous parameters and flags			*/
/************************************************************************/

/*
 * Stream head default high/low water marks
 */
#define STRHIGH 	512
#define STRLOW		128

/*
 * Values for stream flag in open to indicate module open, clone open;
 * return value for failure.
 */
#define MODOPEN		  1	/* open as a module */
#define CLONEOPEN	  2	/* open for clone, pick own minor device */
#define BCASTOPEN  0x10000000	/* open for broadcast */
#define OPENFAIL	 -1	/* returned for open failure */

/*
 * Priority definitions for block allocation.
 */
#define BPRI_LO		1
#define BPRI_MED	2
#define BPRI_HI		3

/*
 * CLASS_RANGE defines the number of buffer size classes that should be used
 * to attempt to satisfy the allocation request.  A CLASS_RANGE of one means
 * that only the class which is the closest larger match to the requested
 * buffer size may be used, whereas a CLASS_RANGE of 3 indicates that the
 * closest class plus the next 2 larger classes may be used.
 */
 		
#define CLASS_RANGE     2	/* #4 */

/*
 * Sleep flag values for gettoken system call.
 */
#define TK_NOSLEEP	0	/* return error if can't get token */
#define	TK_SLEEP	1	/* sleep if can't get token */


/*
 * Value for packet size that denotes infinity
 */
#define INFPSZ		-1

/*
 * Flags for flushq()
 */
#define FLUSHALL	1	/* flush all messages */
#define FLUSHDATA	0	/* don't flush control messages */

/*
 * Max IOCTL data block size
 */
#define MAXIOCBSZ	1024


/************************************************************************/
/*	Definitions of Streams macros and function interfaces.		*/
/************************************************************************/

/*
 * determine block allocation cutoff for given class and priority.
 */
#define BCMAX(class, pri) ( pri == BPRI_LO ? dballoc[class].dba_lo : \
			   ( pri == BPRI_HI ? DBLK[class].num : \
			    dballoc[class].dba_med))

/*
 * canenable - check if queue can be enabled by putq().
 *
 */

#define canenable(q)	(!((q)->q_flag & QNOENB))

/*
 * Finding related queues
 */
#define	OTHERQ(q)	((q)->q_flag&QREADR? (q)+1: (q)-1)
#define	WR(q)		((q)->q_flag&QREADR? (q)+1: (q))
#define	RD(q)		((q)->q_flag&QREADR? (q)  : (q)-1)

/*
 * put a message of the next queue of the given queue
 */
#define putnext(q, mp)	((*(q)->q_next->q_qinfo->qi_putp)((q)->q_next, mp))

/*
 * Test if data block type is one of the data messages (i.e. not a control
 * message).
 */
#define datamsg(type) (type == M_DATA || type == M_PROTO || type == M_PCPROTO)

/*
 * extract queue class of message block
 */
#define queclass(bp) (bp->b_datap->db_type & QPCTL)

/*
 * Align address on next lower word boundary
 */
#define straln(a)	(caddr_t)((long)(a) & ~(sizeof(int)-1))

/*
 * Copy data from one data buffer to another.
 * The addresses must be word aligned - if not, use bcopy!
 */
#define	strbcpy(s, d, c)	bcopy(s, d, c)

/* 
 * Simple putq procedure: doesn't call qenable or modify q_count, just places
 * buffer onto the queue using q_first and q_last.
 */

#define S_PUTQ( q, b ) {                                \
     if( (q)->q_first == NULL )                         \
         (q)->q_first = (q)->q_last = (mblk_t *)(b);    \
     else {                                             \
         (q)->q_last->b_next = (mblk_t *)(b);           \
         (q)->q_last = (mblk_t *)(b);                   \
     }                                                  \
}

/*
 * Set interrupt mask levels
 */
#ifdef UCONXPPC /* #5 */
#define splstr()        splx(0)
#define splhi()		splstr()	/* #6 */

/* #9 */
#if defined(PQ34X) || defined(PQ344) || defined (PQ348) || defined(MPS800)   
#define PQ_LEVEL0       0x0003FFFF  /* Level 7 ints off */
#define PQ_LEVEL4       0x0243FFFF  /* Level 4 (Pquicc) + IRQ 3 (Slave Quicc) */
#else
#define PQ_LEVEL0       0x0000FFFF
#define PQ_LEVEL4       0x0040FFFF
#endif

#define PQ_LEVEL0_MASK  (~(PQ_LEVEL0))
#define PQ_LEVEL4_MASK  (~(PQ_LEVEL4))

#define spl0()          splx(PQ_LEVEL0_MASK) /* lvl 0 mask */
#define spl4()          splx(PQ_LEVEL4_MASK) /* lvl 4 mask */

#else /* not PPC */
#define spl0()		splx(0)
#define spl1()		splx(1)
#define spl2()		splx(2)
#define spl3()		splx(3)
#define spl4()		splx(4)
#define spl5()		splx(5)
#define spl6()		splx(6)
#define spl7()		splx(7)
#define splhi()		splx(7)
#define splstr()	splx(7)
#endif

/*
 * Declarations for system calls and internal routines
 */
extern mblk_t	*rmvb();
extern mblk_t	*dupmsg();
extern mblk_t	*copymsg();
extern mblk_t	*allocb();
extern mblk_t	*xallocb();
extern mblk_t	*unlinkb();
extern mblk_t	*dupb();
extern mblk_t	*copyb();
extern mblk_t	*getq();
extern mblk_t	*cvd_getq();
extern int	putq();
extern queue_t	*backq();
extern queue_t	*allocq();
extern int	qenable();
extern mblk_t	*unlinkb();
extern mblk_t	*unlinkmsg();
extern int	pullupmsg();
extern int	adjmsg();
extern queue_t	*getendq();
extern mblk_t   *alloc_proto();         /* #2 */

/*
 * shared or externally configured data structures
 */

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#define ERROR	-1
#endif

#ifndef NULL
#define NULL	0
#endif

/*
 * min & max macros
 */
#ifndef min
#define min(a,b)	( a > b ? b : a )
#endif

#ifndef max
#define max(a,b)	( a < b ? b : a )
#endif


/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Bufcall Message Structure 
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

typedef struct bc_msg
{
	struct bc_msg	*bc_next;	/* Next bufcall message on queue */
	int		(*bc_call)();   /* Routine to call when buffer avail */
	bit32		bc_param;	/* Parameter to pass on call */
	bit32		bc_id;		/* unique identifier */
} BC_MSG;

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Timer Message Structure 
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

typedef struct ti_msg
{
	struct ti_msg	*ti_next;	/* Next timer message on queue */
	struct ti_msg	*ti_prev;	/* Previous timer msg on queue */
	bit8		ti_unique;	/* Incremented each time msg is
					   allocated (high byte of msg ID) */
	bit8		ti_state;	/* State */
	short		ti_ticks;	/* Tick count */
	int		(*ti_call)();	/* Routine to call on expiration */
	bit32		ti_param;	/* Parameter to pass on call */
} TI_MSG;


/* states */

#define TI_FREE		0	/* Processing expiration or on free list */
#define TI_PENDING	1	/* On timer module's input queue */
#define TI_RUNNING	2	/* On timer module's run queue */

/* Timer Internal Queue Structure */

typedef struct
{
	TI_MSG  *tr_head;
	TI_MSG  *tr_tail;
	int	tr_count;
} TRUN_Q;

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Format of Token Table entries
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

typedef struct
{
	char	tk_name [ 16 ];		/* Token name (null-terminated) */
	int	tk_inout;		/* 0 = not available, 1 = available */
} TOKEN;

#define TK_IN	1		/* Token is available (unlocked) */
#define TK_OUT	0		/* Token is currently allocated (locked) */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
	Generic queue and message structure templates for 
	queue manipulation routines.  Can be used either for
	double- or single-linked lists.  In the latter case,
	the "prev" field of gen_msg is not used.
 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

typedef struct gen_msg
{
	struct gen_msg	*next;		/* next message on queue */
	struct gen_msg	*prev;		/* previous message on queue */
} GEN_MSG;

typedef struct
{
	GEN_MSG		*head;		/* head pointer */
	GEN_MSG		*tail;		/* tail pointer */
	bit32		count;		/* number of messages on queue */
} GEN_QUEUE;

#endif /* _xstra_h */
