/*   @(#) xstrmgr.h 99/12/23 Version 1.5   */
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
1.  14-OCT-97  LMM  Merged stream mgr extened info for PKPLUS hif into SMDATA 
2.  03-JUN-98  mpb  Convert the pad_o_plenty parameter of SMDATA structure
                    to strhead.  This will keep track of how COPEN was called.
3.  15-JUN-98  lmm  Removed PKPLUS define
4.  22-MAR-99  lmm  Track read/writes for TCP streams, cleaned up stream
                    state defines
*/

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: xstrmgr.h                                                            ::
 * ::                                                                      ::
 * ::        XSTRA Master/Controller Driver Stream Manager Includes        ::
 * ::                                                                      ::
 * :: This module provides the include file definitions used for the code  ::
 * :: which must provide Stream Management control for the Master or the   ::
 * :: Controller or the joined combination of both in the XSTRA environment::
 * ::                                                                      ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */

#ifndef _xstrmgr_h
#define	_xstrmgr_h

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: XSTRA Stream Manager definitions                                     ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */

/* ........................................................................
 * .                                                                      .
 * . smdata structure definition      (typedef SMDATA)                    .
 * .                                                                      .
 * . the smdata structure is used to store general stream information for .
 * . each stream head.  This structure may be used by XSTRA or the stream .
 * . manager and controls the interface between that stream and the user  .
 * . (in this case, the Master stream via the XCVD module or the LAN      .
 * . client via TCP/IP).                                                  .
 * .                                                                      .
 * ........................................................................
 */

typedef struct smdata       	/* Stream Manager Structure for each Stream */
{
    struct streamtab *sm_strtab;	/* => streamtab for stream's driver */
    queue_t          *sm_wq;		/* stream head write queue pointer */
    queue_t          *sm_rq;		/* stream head read queue pointer  */
    int               sm_state;		/* stream state/flags */
    int               sm_iocid;		/* stream ioctl id */
    int               sm_sid;		/* stream id (index) */
    struct msgb      *sm_iocblk;	/* ioctl ACK/NAK */
    int               sm_pushcnt;	/* number of pushes done on stream */
    unsigned short    sm_wroff;         /* Write Offset */
    unsigned short    strhead;          /* #2 - Was COPEN just to strhead? */
    int               writes;		/* #4 - number of writes on stream */
    int               reads;		/* #4 - number of reads on stream  */
#ifdef TCP				/* required for TCP only           */
    unsigned char    *sm_ptr;		/* ptr to stream mgr specific data */
					/*    used by TCP/IP LAN */
#else
    u_short           sm_ustate;        /* Upstream state                  */
    u_short           sm_dstate;        /* Downstream state                */
    int               h_sid;	        /* host stream id                  */
    int               wrblock;		/* times host blocked card         */
    int               wrunblock;	/* times host unblocked card       */
    int               wrnodesc;		/* times no data descriptors       */
    int               wrdescenable;	/* times reenabled waiting for ctl */
    int               rdblock;		/* times card blocked host         */
    int               rdunblock;	/* times card unblocked host       */
#endif
} SMDATA;

#define IOC_ID_ERROR    -2              /* ioc_id mismatch */

/*
 * SMDATA sm_state flag definitions
 */

#define STFREE           0              /* This stream is unused */
#define STALLOC       0x01              /* stream is allocated */
#define STWOPEN       0x02              /* waiting for 1st open */
#define IOCWAIT       0x04              /* Someone wants to do ioctl */
#define STPLEX        0x08              /* stream is being multiplexed */
#define	STWCLOSE      0x10		/* Stream close is pending */
#define	STCLOSING     0x20		/* Stream is in process of closing */

#ifdef NOTSUPPORTED
#define RMSGDIS       0x40              /* read msg discard */
#define RMSGNODIS     0x80              /* read msg no discard */
#endif

#endif  /* _xstrmgr_h */
