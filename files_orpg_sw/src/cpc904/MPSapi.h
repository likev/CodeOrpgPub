/*   @(#) MPSapi.h 00/01/11 Version 1.16   */

/************************************************************************
*                        Copyright (c) 1999 by
*
*
*
*                 ======      =============  =============
*               ===    ===    =============  =============
*              ===      ===        ===            ===
*             ===        ===       ===            ===
*             ===        ===       ===            ===
*             ===        ===       ===            ===
*             ===        ===       ===            ===
*             ===       ===        ===            ===
*             ===      ===         ===            ===
*             === ======           ===            ===
*             ===                  ===            ===
*             ===                  ===            ===
*             ===                  ===            ===
*             ===                  ===            ===
*             ===                  ===            ===
*             ===                  ===       =============
*             ===                  ===       =============
*
*
*             P e r f o r m a n c e   T e c h n o l o g i e s
*                              Incorporated
*
*      This software is furnished under a license and may be used and
*      copied only in accordance with the terms of such license and
*      with the inclusion of the above copyright notice.   This software
*      or any other copies thereof may not be provided or otherwise made
*      available to any other person.   No title to and ownership of the
*      program is hereby transferred.
*
*      The information in this software is subject to change without
*      notice and should not be considered as a commitment by Performance 
*      Technologies Incorporated.
* 
*      Performance Technologies Incorporated
*      San Diego, California
*
*************************************************************************/

/*
Modification history:
 
Chg Date       Init Description
1.  17-FEB-96  LMM  Changed ioc_id and ioc_count fields in xiocblk struct
                    from ulong to uints. 
2.  23-MAY-96  mpb  structure FreeBuf has a component called "this".  "this"
                    is a reserved word in C++.  Changed to "this_one".
3.  24-MAY-96  mpb  Changed reference to pointers as "uint" to "ulong".
4.  13-OCT-97  mpb  Ensure that the 'socket' defined in the structure xstdesc
                    is of proper type (based on what OS and LOCAL or LAN).
5.  10-NOV-97  lmm  Modified ReturnError macro to map server error codes
                    into system error codes 
6.  03-DEC-97  mpb  Need to keep track of the controller number in the 
                    descriptor table (for embedded NT).
7.  03-DEC-97  mpb  Need to keep track of the index of the "stream" in the
                    driver's (ntmps) stream info table in the api's descriptor
                    table (for embedded NT).  Remember, NT does not support
                    streams, so things are done just a little bit differently
                    to achieve the same results.
8.  16-SEP-98  mpb  LOCAL is a keyword in VxWorks, so change to MPS_LOCAL.
                    Change for UDP, TCP and RAWIP to be consistant.
9.  06-APR-98  lmm  Set errno to MPSerrno if ReturnError returns system error
*/

#ifndef	WINNT
#define	uchar	unsigned char
#define	ushort	unsigned short
#define	ulong	unsigned long
#else
#define	uchar	u_char
#define	ushort	u_short
#define	uint	u_int
#define	ulong	u_long
#endif	/* WINNT */

#ifdef ioc_uid
#undef ioc_uid
#endif

#ifdef ioc_gid
#undef ioc_gid
#endif

#ifndef TRUE
#define TRUE 		1
#endif

#ifndef FALSE
#define FALSE		0
#endif

#ifndef ERROR
#define ERROR		-1
#endif

#define CLOSED		0
#define OPEN		1

#ifndef	min
#define min(a,b)        ( a > b ? b : a )
#endif	/* min */

typedef struct freebuf
{
   struct freebuf *next;
   struct freebuf *this_one;
} FreeBuf;

typedef struct freelist
{
   struct freebuf *head;
   struct freebuf *tail;
} FreeList;

struct xstdesc
{
	mpssd socket;         /* #4 */
	ulong streamID;       /* board side stream id */
	ulong ctlrNum;        /* #6 */
	ulong streamIndex;    /* #7 host side stream id */
	uchar ioc;
	uchar state;
	uchar type;
	uchar noblock;

	struct sockaddr_in *sin;

	int wrcnt;

	FreeList *p_mlist;
	FreeList *p_ilist;
#ifdef VXWORKS
	int tid;  	      /* task ID associated with stream */
#endif
};

/* #8 */
#define MPS_TCP         1
#define MPS_UDP         2
#define MPS_RAWIP       3
#define MPS_LOCAL       4

extern struct xstdesc xstDescriptors[];

#define GetSocket(s)      xstDescriptors[s].socket
#define PendingData(s)    (ulong)(xstDescriptors[s].p_mlist->head)  /* #3 */
#define PendingError(s)   0
#define PendingPri(s)     (ulong)(xstDescriptors[s].p_ilist->head)  /* #3 */
#define StreamOpen(s)  	  (xstDescriptors[s].state == OPEN)
#define StreamType(s)     xstDescriptors[s].type
#define PendingIoc(s)     xstDescriptors[s].ioc
#define SetPendIoc(s)     xstDescriptors[s].ioc = 1
#define ClrPendIoc(s)     xstDescriptors[s].ioc = 0
#define SetXstIO(s, a)    xstDescriptors[s].noblock = a
#define BlockIO(s)        !xstDescriptors[s].noblock
#define SetStrId(s,id)    xstDescriptors[s].streamID = id
#define GetStrId(s)       xstDescriptors[s].streamID
#define TCPWrCnt(s)       xstDescriptors[s].wrcnt
#define SetCtlrNum(s, a)  xstDescriptors[s].ctlrNum = a        /* #6 */
#define GetCtlrNum(s)     xstDescriptors[s].ctlrNum            /* #6 */
#define SetStrIndex(s, a) xstDescriptors[s].streamIndex = a    /* #7 */
#define GetStrIndex(s)    xstDescriptors[s].streamIndex        /* #7 */
#ifdef VXWORKS
#define GetTid(s)	  xstDescriptors[s].tid
#endif

/* 
** Streams IOCTL structure 
** This structure is the format of the M_IOCTL message type.  Defined here
** so that there is no dependance on xstra.h
*/
struct xiocblk 
{
   int	  ioc_cmd;			/* ioctl command type */
   unsigned short ioc_uid;		/* effective uid of user */
   unsigned short ioc_gid;		/* effective gid of user */
   uint   ioc_id;			/* ioctl id */		     /* #1 */ 
   uint   ioc_count;			/* bytes in data field */    /* #1 */ 
   int	  ioc_error;			/* error code */
   int	  ioc_rval;			/* return value  */
};

/* #5 - Modified this macro to map server error codes into system error codes */
/* #9 - Set errno to MPSerrrno if we convert to system error */

#define ReturnError(err) \
   { \
      MPSerrno = err; \
      if ( ( MPSerrno > MPSERR ) && ( MPSerrno <= MPSSYSERRMAX ) ) \
      { \
         if ( system_errno [ MPSerrno - MPSERR ] ) \
            errno = MPSerrno = system_errno [MPSerrno - MPSERR]; \
      } \
      return(ERROR); \
   }

#define OPENTIME	10		/* Time to wait for open resp. (10s) */

extern int MaxDblock;

