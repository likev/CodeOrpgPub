/*   @(#) MPSapi.h 96/05/13 Version 1.2   */
/*<-------------------------------------------------------------------------
| 
|                           Copyright (c) 1991 by
|
|              +++    +++                           +++     +++
|              +++    +++                           +++     +++
|              +++    +++                            +++   +++ 
|              +++    +++   +++++     + +    +++   +++++   +++ 
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|              +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
|              +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
|              +++    ++++++      +++++ ++++++++++ +++ +++++   
|              +++    ++++++      +++++ ++++++++++ +++  +++    
|              +++    ++++++      ++++   +++++++++ +++  +++    
|              +++    ++++++                             +     
|              +++    ++++++      ++++   +++++++ +++++  +++    
|              +++    ++++++      +++++ ++++++++ +++++  +++    
|              +++    ++++++      +++++ ++++++++ +++++ +++++   
|              +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
|              +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|               +++  +++    +++++     + +    +++   ++++++  +++ 
|               ++++++++                             +++    +++
|                ++++++         Corporation         ++++    ++++
|                 ++++   All the right connections  +++      +++
|
|
|      This software is furnished  under  a  license and may be used and
|      copied only  in  accordance  with  the  terms of such license and
|      with the inclusion of the above copyright notice.   This software
|      or any other copies thereof may not be provided or otherwise made
|      available to any other person.   No title to and ownership of the
|      program is hereby transferred.
|
|      The information  in  this  software  is subject to change without
|      notice  and  should  not be considered as a commitment by UconX
|      Corporation.
|  
|      UconX Corporation
|      San Diego, California
|                                               
v                                                
-------------------------------------------------------------------------->*/

/*
Modification history:
 
Chg Date       Init Description
1.  17-FEB-96  LMM  Changed ioc_id and ioc_count fields in xiocblk struct
                    from ulong to uints. 
2.  23-MAY-96  mpb  structure FreeBuf has a component called "this".  "this"
                    is a reserved word in C++.  Changed to "this_one".
3.  24-MAY-96  mpb  Changed reference to pointers as "uint" to "ulong".
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
#ifdef	WINNT
	SOCKET socket;
#else
	ulong socket;
#endif	/* WINNT */
	ulong streamID;

	uchar ioc;
	uchar state;
	uchar type;
	uchar noblock;

	struct sockaddr_in *sin;

	int wrcnt;

	FreeList *p_mlist;
	FreeList *p_ilist;
};

#define TCP		1
#define UDP 		2
#define RAWIP   	3
#define LOCAL		4

extern struct xstdesc xstDescriptors[];

#define GetSocket(s)	xstDescriptors[s].socket
#define PendingData(s)	(ulong)(xstDescriptors[s].p_mlist->head)  /* #3 */
#define PendingError(s) 0
#define PendingPri(s)	(ulong)(xstDescriptors[s].p_ilist->head)  /* #3 */
#define StreamOpen(s)   (xstDescriptors[s].state == OPEN)
#define StreamType(s)	xstDescriptors[s].type
#define PendingIoc(s)	xstDescriptors[s].ioc
#define SetPendIoc(s)	xstDescriptors[s].ioc = 1
#define ClrPendIoc(s)	xstDescriptors[s].ioc = 0
#define SetXstIO(s, a)	xstDescriptors[s].noblock = a
#define BlockIO(s)	!xstDescriptors[s].noblock
#define SetStrId(s,id)	xstDescriptors[s].streamID = id
#define GetStrId(s)	xstDescriptors[s].streamID
#define TCPWrCnt(s)	xstDescriptors[s].wrcnt

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

#define ReturnError(err) \
   { \
      MPSerrno = err; \
      return(ERROR); \
   }


#define OPENTIME	10		/* Time to wait for open resp. (10s) */

extern int MaxDblock;

