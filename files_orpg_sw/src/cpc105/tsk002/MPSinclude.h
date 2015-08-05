/*   @(#) MPSinclude.h 96/08/13 Version 1.7   */
/*******************************************************************************
                             Copyright (c) 1995 by                             

     ===     ===     ===         ===         ===                               
     ===     ===   =======     =======     =======                              
     ===     ===  ===   ===   ===   ===   ===   ===                             
     ===     === ===     === ===     === ===     ===                            
     ===     === ===     === ===     === ===     ===   ===            ===    
     ===     === ===         ===     === ===     ===  =====         ======    
     ===     === ===         ===     === ===     === ==  ===      =======    
     ===     === ===         ===     === ===     ===      ===    ===   =        
     ===     === ===         ===     === ===     ===       ===  ==             
     ===     === ===         ===     === ===     ===        =====               
     ===========================================================                
     ===     === ===         ===     === ===     ===        =====              
     ===     === ===         ===     === ===     ===       ==  ===             
     ===     === ===     === ===     === ===     ===      ==    ===            
     ===     === ===     === ===     === ===     ====   ===      ===           
      ===   ===   ===   ===   ===   ===  ===     =========        ===  ==     
       =======     =======     =======   ===     ========          ===== 
         ===         ===         ===     ===     ======             ===         
                                                                                
       U   c   o   n   X      C   o   r   p   o   r   a   t   i   o   n         
                                                                                
       This software is furnished  under  a  license and may be used and
       copied only  in  accordance  with  the  terms of such license and
       with the inclusion of the above copyright notice.   This software
       or any other copies thereof may not be provided or otherwise made
       available to any other person.   No title to and ownership of the
       program is hereby transferred.
 
       The information  in  this  software  is subject to change without
       notice  and  should  not be considered as a commitment by UconX
       Corporation.
  
*******************************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2000/12/05 22:52:21 $
 * $Id: MPSinclude.h,v 1.6 2000/12/05 22:52:21 jing Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  

#define MPSAPI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef	WINNT

#include <sys/errno.h>

#else

#include "MPSwinnt.h"
#include <errno.h>

#endif	/* !WINNT */

#ifndef	VMS

#ifndef WINNT
#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/file.h>

#ifndef NO_STREAMS
#include <sys/stropts.h>
#endif
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#else

#define	ENOMEM			12		/* Not enough core. */
#define	ERANGE			34
#define	EINVAL			22
#define	EBADMSG			77
#define	EIO				5
#define	ENOBUFS			132
#define	ECONNREFUSED	146
#define	EISCONN			133
#define	EAGAIN			11
#define	EALREADY		149
#define	EINPROGRESS		150
#define	EWOULDBLOCK		EAGAIN
#define	ENXIO			6
#define	EBADF			9
#define	ECONNRESET		131

#include <io.h>
#include <winsock.h>

#endif	/* !WINNT */

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifdef _AIX
#include <sys/select.h>
#endif

#define SERVICE_PROTO		NULL

#else

#include <iodef.h>
#include <file.h>
#include <stat.h>
#include <descrip.h>
#include <ssdef.h>
#include "errno.h"
#include "MPSvms.h"
#ifdef  __alpha
#include <signal.h> /* for sleep */
#endif

/* Can't pass a null second argument to TCPware getservbyname() */
#undef	SERVICE_PROTO
#define	SERVICE_PROTO	"TCP"	/* only TCP supported */

#endif	/* VMS */

#include <sys/stat.h>
#include "uclid.h"
#include "xstopts.h"
#include "xstpoll.h"
#include "MPSapi.h"

#ifdef	WINNT /* Since same name as system include file, must give path. */
#include "..\include\errno.h"
#else
#include "errno.h"
#endif	/* WINNT */

#include "mpsproto.h"		/* Prototypes of API functions. */


#ifdef IPAPI
/* 
** TCP/IP API only, disable local calls 
*/
#define LocalOpen(p1) 					MPSdisabled(ENOTLOCAL)
#define LocalClose(p1) 					MPSdisabled(ENOTLOCAL)
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

#define UDPopen(p1, p2, p3)				MPSdisabled(ENOTTCPIP)
#define UDPclose(p1) 					MPSdisabled(ENOTTCPIP)
#define UDPread(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define UDPwrite(p1, p2, p3) 			MPSdisabled(ENOTTCPIP)
#define UDPputmsg(p1, p2, p3, p4) 		MPSdisabled(ENOTTCPIP)
#define UDPgetmsg(p1, p2, p3, p4) 		MPSdisabled(ENOTTCPIP)
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
int	LocalGetmsg ( MPSsd, struct strbuf*, struct strbuf*, int* );
#endif
#ifndef	LocalPoll
int	LocalPoll   ( struct xpollfd*, int, int );
#endif
#ifndef	LocalPutmsg
int	LocalPutmsg ( MPSsd, struct strbuf*, struct strbuf*, int );
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
void	addFree     ( FreeList*, FreeBuf* );
void	drainList   ( FreeList* );
FreeBuf	*getFree    ( FreeList* );
void	headFree    ( FreeList*, FreeBuf* );
FreeBuf	*peekFree   ( FreeList* );
int	readHdr     ( int, UCSHdr* );
FreeBuf	*readSave   ( int, UCSHdr* );

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */


#ifndef	LocalClose
int	LocalClose  ( MPSsd );
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
int	TCPclose    ( MPSsd );
#endif
#ifndef	TCPioctl
int	TCPioctl    ( MPSsd, MPSsd, caddr_t );
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
#endif	/* ! (__hp9000s800 || WINNT */ */
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
int	UDPioctl    ( MPSsd, MPSsd, caddr_t );
#endif
#ifndef	UDPread
int	UDPread     ( int, caddr_t, int );
#endif
#ifndef	UDPwrite
int	UDPwrite    ( int, caddr_t, int );
#endif
int	addDesc     ( int, int );
void	deleteDesc  ( int );
int	readData    ( int, caddr_t, int );
#ifdef	WINNT
int	setIOType   ( SOCKET, ulong );
#else
int	setIOType   ( int, int );
#endif	/* WINNT */
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

#endif	/* ANSI_C */
