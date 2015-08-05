/*   @(#) xstopts.h 97/04/08 Version 1.5   */
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
|
v                                                
-------------------------------------------------------------------------->*/

/*
Modification history:
 
Chg Date       Init Description
1.  09-AUG-96  mpb  32 bit pointer for DECUX.
2.  16-AUG-96  mpb  Be sure ERROR gets defined as -1.  Windows NT has it 
                    defined in the system include file (wingdi.h) as 0.
                    This might lead to problems in a windows app, we should
                    look out for it.
3.  28-OCT-97  lmm  Added defines for GOOD_EXIT and BAD_EXIT
*/

#ifndef	_xstopts_h
#define	_xstopts_h

#include "xstopen.h"

/* 
** Include the following definitions ONLY if not defined by host system.
*/

#ifndef RS_HIPRI

/* Read options */
#define RMSGN   2                       /* read msg no discard */

/* Flush options */

#define FLUSHR 1			/* flush read queue */
#define FLUSHW 2			/* flush write queue */
#define FLUSHRW 3			/* flush both queues */

/* Flags for MPSgetmsg()/MPSputmsg() */
#define RS_HIPRI	1		/* send/recv high priority message */

/* Flags returned as value of MPSgetmsg() */
#define MORECTL		1		/* more ctl info is left in message */
#define MOREDATA	2		/* more data is left in message */

/* Value for timeouts (ioctl, select) that denotes infinity */
#ifndef __hp9000s800
#define INFTIM		-1
#endif

#endif

/* STREAM Ioctls for the xSTRa environment. */ 
#define	XSTR		('X'<<8)
#define X_NREAD		(XSTR|0x01)
#define X_PUSH		(XSTR|0x02)
#define X_POP		(XSTR|0x83)
#define X_LOOK		(XSTR|0x04)
#define X_FLUSH		(XSTR|0x05)
#define X_SRDOPT	(XSTR|0x06)
#define X_GRDOPT	(XSTR|0x07)
#define X_STR		(XSTR|0x08)
#define X_SETSIG	(XSTR|0x09)
#define X_GETSIG	(XSTR|0x0a)
#define X_FIND		(XSTR|0x0b)
#define X_LINK		(XSTR|0x0c)
#define X_UNLINK	(XSTR|0x0d)
#define X_PEEK		(XSTR|0x0e)
#define X_FDINSERT	(XSTR|0x0f)
#define X_SENDFD	(XSTR|0x10)
#define X_RECVFD	(XSTR|0x11)
#define X_SETIOTYPE	(XSTR|0x12)
#define X_GETIOTYPE	(XSTR|0x13)

/*
** The following structures are identical to the strioctl and strbuf 
** structures defined by System V Streams.  They are provided for 
** host systems which do not have a native STREAMS environment and 
** for compatiblity across different STREAMS environments.
*/

/* Ioctl argument structure */
struct xstrioctl 
{
   int 	 ic_cmd;			/* command */
   int	 ic_timout;			/* timeout value */
   int	 ic_len;			/* length of data */
#ifdef  DECUX_32                        /* #1 */
#pragma pointer_size    save
#pragma pointer_size    long
#endif /* DECUX_32 */
   char	*ic_dp;				/* pointer to data */
#ifdef  DECUX_32
#pragma pointer_size    restore
#endif  /* DECUX_32 */
};

/* Stream buffer structure for getmsg and putmsg */
struct xstrbuf 
{
   int	 maxlen;			/* Max number of bytes in buffer */
   int	 len;				/* Number of bytes sent/returned */
   char	*buf;				/* pointer to data */
};

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifdef  VMS
#define GOOD_EXIT       1       /* SS$_NORMAL */
#define BAD_EXIT        44      /* SS$_ABORT */
#else
#define GOOD_EXIT       0
#define BAD_EXIT        1
#endif

/* #2 */
#ifdef WINNT
#undef  ERROR
#endif /* WINNT */

#ifndef ERROR
#define ERROR	-1
#endif

#define BLOCK		0
#define NONBLOCK	1

/* Maximum open connections per process */
#define MAXCONN		256

#endif	/* _xstopts_h */
