/*   @(#) MPSinclude.h 00/06/02 Version 1.37   */

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
1.  13-OCT-97  mpb  Embedded NT enhancements.
2.  30-OCT-97  lmm  Include errno.h for system error codes, and mpserrno.h
                    for UconX error codes
3.  30-OCT-97  mpb  Cleaned up some errno redefinitions in the NT world.
4.  03-NOV-97  mpb  winntdrv.h now gets included in mpsproto.h.
5.  10-NOV-97  lmm  Added system_errno extern
6.  15-DEC-97  mpb  QNX support.
7.  17-DEC-97  mpb  Moved NTpoll(), NTgetmsg() and MTwritev() from MPSutil.c
                    to MPSlocal_winnt.c, so no prototype needed here.
8.  21-APR-98  mpb  The default QNX packing for structures is the byte boundry.
                    The board code wants 4 byte boundry.  The board way can
                    not just be the default, since the QNX OS expects it the
                    other way.
9.  05-AUG-98  mpb  The second parameter for TCPioctl() and UDPioctl() is
                    of type int, not of type MPSsd.  LocalIoctl() got it right.
10. 06-AUG-98  mpb  "struct timeval" is not defined in the regular system
                    headers for the POSIX case.  It is referenced in Solaris
                    2.6.  Also, had to move the POSIX definitions below the
                    include of sys/time.h since time_t is used int timeval.
11. 16-SEP-98  mpb  VxWorks support.
12. 23-OCT-98  mpb  Support for Linux (LINUX).
13. 21-JAN-99  lmm  Added THREAD_SAFE define for use by API routines
                    Don't need struct timeval for DECUX (see change #10)
14. 31-MAY-00  jjw  Local (non-network) Linux STREAMS support.
*/

#define MPSAPI

#if (_POSIX_C_SOURCE - 0 >= 199506L) && defined (MPS_THREADS)
#define THREAD_SAFE
#endif

#ifdef QNX   /* #6 */
#define EBADMSG 77
typedef unsigned char   unchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;
#endif /* QNX */

#ifdef VXWORKS
typedef unsigned int    uint;

/* System structures that VxWorks does not have, but we need.  Define here,
        and add the variables that we will use. */

typedef struct hostent
{
   int  h_addr;      /* Return value from hostGetByName(). */
} HostEnt;

typedef struct servent
{
   int  s_port;      /* Port number of service. */
} ServEnt;

struct hostent *gethostbyname ( const char* );
struct servent *getservbyname ( const char*, const char* );
#endif /* VXWORKS */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>	/* #2 */
#include <sys/types.h>

/* #10 */
/* Standard system includes will not do these typedefs if _POSIX_C_SOURCE is
   defined. */
#ifdef _POSIX_C_SOURCE
#ifndef LINUX

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

typedef unsigned char   unchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;

typedef long    hostid_t;

extern int ioctl(int, int, ...);

#ifndef DECUX	/* #13 - not needed for DECUX */
/* #10 */
struct timeval {
        time_t  tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};
#endif

#endif /* !LINUX */
#endif /* _POSIX_C_SOURCE */

#ifndef	WINNT

#ifdef THREAD_SAFE
#include <pthread.h>
#endif /* THREAD_SAFE */
#else
#include "MPSwinnt.h"
#endif	/* !WINNT */

#ifndef	VMS

#ifndef WINNT
#include <unistd.h>
#ifdef VXWORKS
#include <net/uio.h>
#include <systime.h>
#else
#include <sys/uio.h>
#endif /* VXWORKS */
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef VXWORKS
#include <sys/fcntlcom.h>
#else
#include <netdb.h>
#include <sys/file.h>
#endif /* VXWORKS */

#ifndef NO_STREAMS
#ifdef VXWORKS
#include <streams/stropts.h>
#else
#include <sys/stropts.h>   /* #14 - Remove encapsulating "#ifndef LINUX" */
#endif /* VXWORKS */
#endif
#ifndef QNX
#ifdef VXWORKS
#include <streams/poll.h>
#else
#include <sys/poll.h>
#endif /* VXWORKS */
#endif /* QNX - #6 */
#include <sys/ioctl.h>
#if defined (VXWORKS) || defined (LINUX)
#include <time.h>
#else
#include <sys/time.h>
#endif /* VXWORKS || LINUX */

#endif	/* !WINNT */

#ifdef VXWORKS
#include <stdlib.h>
#else
#include <malloc.h>
#endif /* !VXWORKS */
#include <fcntl.h>

#if defined (  _AIX ) || defined ( QNX )
#include <sys/select.h>
#endif

#define SERVICE_PROTO		NULL

#else

#include <iodef.h>
#include <file.h>
#include <stat.h>
#include <descrip.h>
#include <ssdef.h>
#include "MPSvms.h"
#ifdef  __alpha
#include <signal.h> /* for sleep */
#endif

/* Can't pass a null second argument to TCPware getservbyname() */
#undef	SERVICE_PROTO
#define	SERVICE_PROTO	"TCP"	/* only TCP supported */

#endif	/* VMS */


typedef int mpssd;

#include <sys/stat.h>

/* #8 */
#ifdef QNX
#pragma pack ( push, 4 );  /* Save current alignment, and then set to 4. */
#endif /* QNX */

#include "uclid.h"
#include "xstopts.h"
#include "xstpoll.h"
#include "xconfig.h"
#include "strstat.h"
#include "report.h"
#include "MPSapi.h"
#include "mpserrno.h"		/* #2 */
#include "mpsproto.h"		/* Prototypes of API functions. */

/* #8 */
#ifdef QNX
#pragma pack ( pop );  /* Restore the origonal alignment. */
#endif /* QNX */

#ifdef VXWORKS
extern SEM_ID  pti_semMutex;
#endif /* VXWORKS */

#ifdef IPAPI
/* 
** TCP/IP API only, disable local calls 
*/
#define LocalOpen(p1) 					   MPSdisabled(ENOTLOCAL)
#define LocalClose(p1) 				   	MPSdisabled(ENOTLOCAL)
#define LocalRead(p1, p2, p3) 			MPSdisabled(ENOTLOCAL)
#define LocalWrite(p1, p2, p3) 			MPSdisabled(ENOTLOCAL)
#define LocalPutmsg(p1, p2, p3, p4) 	MPSdisabled(ENOTLOCAL)
#define LocalGetmsg(p1, p2, p3, p4) 	MPSdisabled(ENOTLOCAL)
#define LocalPoll(p1, p2, p3) 			MPSdisabled(ENOTLOCAL)
#define LocalIoctl(p1, p2, p3) 			MPSdisabled(ENOTLOCAL)
#endif

#ifdef LOCALAPI
/* 
** Local API only, disable IP calls 
*/
#define TCPopen(p1, p2, p3)				MPSdisabled(ENOTTCPIP)
#define TCPopenS(p1, p2)				MPSdisabled(ENOTTCPIP)
#if defined ( __hp9000s800 ) || defined ( WINNT )
#define TCPopenP(p1, p2, p3, p4)		MPSdisabled(ENOTTCPIP)
#else
#define TCPopenP(p1, p2)				MPSdisabled(ENOTTCPIP)
#endif /* ! (__hp9000s800 || WINNT) */
#define TCPclose(p1) 					MPSdisabled(ENOTTCPIP)
#define TCPread(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define TCPwrite(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define TCPputmsg(p1, p2, p3, p4) 		MPSdisabled(ENOTTCPIP)
#define TCPgetmsg(p1, p2, p3, p4) 		MPSdisabled(ENOTTCPIP)
#define TCPpoll(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define TCPioctl(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)

#define UDPopen(p1, p2, p3)			MPSdisabled(ENOTTCPIP)
#define UDPclose(p1) 					MPSdisabled(ENOTTCPIP)
#define UDPread(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define UDPwrite(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define UDPputmsg(p1, p2, p3, p4)	MPSdisabled(ENOTTCPIP)
#define UDPgetmsg(p1, p2, p3, p4)	MPSdisabled(ENOTTCPIP)
#define UDPpoll(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define UDPioctl(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)

#define gethostbyname(p1)				NULL
#define getservbyname(p1, p2)			NULL
#endif


/** Prototypes for API utility functions. **/

#ifdef	ANSI_C

#ifdef	DECUX_32			/* #3 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

#ifndef	LocalGetmsg
int	LocalGetmsg ( MPSsd, struct xstrbuf*, struct xstrbuf*, int* );
#endif
#ifndef	LocalPoll
int	LocalPoll   ( struct xpollfd*, int, int );
#endif
#ifndef	LocalPutmsg
int	LocalPutmsg ( MPSsd, struct xstrbuf*, struct xstrbuf*, int );
#endif
#ifndef	TCPgetmsg
int	TCPgetmsg   ( MPSsd, struct xstrbuf *, struct xstrbuf *, int* );
#endif
#ifndef	TCPopen
MPSsd	TCPopen     ( OpenRequest*, struct hostent*, struct servent* );
#endif
#ifndef	VMS
#ifndef	TCPopenS
int	TCPopenS    ( struct hostent*, struct servent* );
#endif
#endif	/* VMS */
#ifndef	TCPpoll
int	TCPpoll     ( struct xpollfd *, int, int );
#endif
#ifndef	TCPputmsg
int	TCPputmsg   ( MPSsd, struct xstrbuf *, struct xstrbuf *, int );
#endif

#ifndef WINNT    /* UDP not supported yet on NT. */
#ifndef	UDPgetmsg
int	UDPgetmsg   ( MPSsd, struct xstrbuf*, struct xstrbuf*, int* );
#endif
#ifndef	UDPopen
int	UDPopen     ( OpenRequest*, struct hostent*, struct servent* );
#endif
#ifndef	UDPpoll
int	UDPpoll     ( struct xpollfd*, int, int );
#endif
#ifndef	UDPputmsg
int	UDPputmsg   ( MPSsd, struct xstrbuf*, struct xstrbuf*, int );
#endif
#else
#define UDPopen(p1, p2, p3)			MPSdisabled(ENOTTCPIP)
#define UDPclose(p1) 					MPSdisabled(ENOTTCPIP)
#define UDPread(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define UDPwrite(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define UDPputmsg(p1, p2, p3, p4)	MPSdisabled(ENOTTCPIP)
#define UDPgetmsg(p1, p2, p3, p4)	MPSdisabled(ENOTTCPIP)
#define UDPpoll(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define UDPioctl(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#endif /* WINNT */

void	addFree     ( FreeList*, FreeBuf* );
void	drainList   ( FreeList* );
FreeBuf	*getFree    ( FreeList* );
void	headFree    ( FreeList*, FreeBuf* );
FreeBuf	*peekFree   ( FreeList* );

int	addDesc     ( mpssd, int );
int	readData    ( mpssd, caddr_t, int );
int	readHdr     ( mpssd, UCSHdr* );

FreeBuf	*readSave   ( mpssd, UCSHdr* );

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */


#ifndef	LocalClose
int	LocalClose  ( mpssd );
#endif
#ifndef	LocalIoctl
int	LocalIoctl  ( MPSsd, int, caddr_t );
#endif
#ifndef	LocalOpen
MPSsd	LocalOpen   ( OpenRequest * );
#endif
#ifndef	LocalRead
int	LocalRead   ( MPSsd, caddr_t, int );
#endif
#ifndef	LocalWrite
int	LocalWrite  ( MPSsd, caddr_t, int);
#endif
#ifndef	TCPclose
int	TCPclose    ( mpssd );
#endif
#ifndef	TCPioctl
int	TCPioctl    ( MPSsd, int, caddr_t );
#endif
#ifndef	VMS
#if defined ( __hp9000s800 ) || defined ( WINNT )
#ifndef	TCPopenP
MPSsd	TCPopenP    (OpenRequest*, int, struct hostent*, struct servent* );
#endif
#else
#ifndef	TCPopenP
MPSsd	TCPopenP    ( OpenRequest*, int );
#endif
#endif	/* ! (__hp9000s800 || WINNT */
#endif	/* VMS */
#ifndef	TCPread
int	TCPread     ( MPSsd, caddr_t, int );
#endif
#ifndef	TCPwrite
int	TCPwrite    ( MPSsd, caddr_t , int );
#endif
#ifndef	UDPclose
int	UDPclose    ( int );
#endif
#ifndef	UDPioctl
int	UDPioctl    ( MPSsd, int, caddr_t );
#endif
#ifndef	UDPread
int	UDPread     ( int, caddr_t, int );
#endif
#ifndef	UDPwrite
int	UDPwrite    ( int, caddr_t, int );
#endif
void	deleteDesc  ( int );
int	setIOType   ( mpssd, ulong, uchar );
MPSsd   sd2MPSsd    ( mpssd s );

#ifdef	 WINNT
int	sendmsg ( int, const struct msghdr*, int );
int WSA2MPS ( int );
#endif	 /* WINNT */

#else

#ifndef	LocalClose
int	LocalClose  ( );
#endif
#ifndef	LocalGetmsg
int	LocalGetmsg ( );
#endif
#ifndef	LocalIoctl
int	LocalIoctl  ( );
#endif
#ifndef	LocalOpen
MPSsd	LocalOpen   ( );
#endif
#ifndef	LocalPoll
int	LocalPoll   ( );
#endif
#ifndef	LocalPutmsg
int	LocalPutmsg ( );
#endif
#ifndef	LocalRead
int	LocalRead   ( );
#endif
#ifndef	LocalWrite
int	LocalWrite  ( );
#endif
#ifndef	TCPclose
int	TCPclose    ( );
#endif
#ifndef	TCPgetmsg
int	TCPgetmsg   ( );
#endif
#ifndef	TCPioctl
int	TCPioctl    ( );
#endif
#ifndef	TCPopen
MPSsd	TCPopen     ( );
#endif
#ifndef	VMS
#ifndef	TCPopenP
MPSsd	TCPopenP    ( );
#endif
#ifndef	TCPopenS
int	TCPopenS    ( );
#endif
#endif	/* VMS */
#ifndef	TCPpoll
int	TCPpoll     ( );
#endif
#ifndef	TCPputmsg
int	TCPputmsg   ( );
#endif
#ifndef	TCPread
int	TCPread     ( );
#endif
#ifndef	TCPwrite
int	TCPwrite    ( );
#endif
#ifndef	UDPclose
int	UDPclose    ( );
#endif
#ifndef	UDPgetmsg
int	UDPgetmsg   ( );
#endif
#ifndef	UDPioctl
int	UDPioctl    ( );
#endif
#ifndef	UDPopen
int	UDPopen     ( );
#endif
#ifndef	UDPpoll
int	UDPpoll     ( );
#endif
#ifndef	UDPputmsg
int	UDPputmsg   ( );
#endif
#ifndef	UDPread
int	UDPread     ( );
#endif
#ifndef	UDPwrite
int	UDPwrite    ( );
#endif
int	addDesc     ( );
void	addFree     ( );
void	deleteDesc  ( );
void	drainList   ( );
FreeBuf	*getFree    ( );
void	headFree    ( );
FreeBuf	*peekFree   ( );
int	readData    ( );
int	readHdr     ( );
FreeBuf	*readSave   ( );
int	setIOType   ( );
MPSsd   sd2MPSsd    ( );

#endif	/* ANSI_C */

extern int system_errno[];	/* #5 */
