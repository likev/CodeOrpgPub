/*   @(#) MPStcp.c 00/04/11 Version 1.46   */

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
1.  10-FEB-96  LMM  Replaced "uint" with "char *" in all pointer arithmetic
                    for DECUX/Alpha compatibility.  Added required calls
                    to "ntohl" where necessary. 
2.  28-MAR-96  LMM  Corrected pkt header length for TCP IOCTL's
3.  14-MAY-96  mpb  Put pragma's in for 64 to 32 bit conversion (for esca).
4.  23-MAY-96  mpb  Added prototyping of functions if ANSI_C is #ifdef'd.
5.  23-MAY-96  mpb  Took out third parameter (UHDRLEN) of calls to readHdr
                    since the routine only accepts the first two.
6.  25-JUN-96  mpb  Added type casting for the pointer arithmatic.
7.  07-JUL-96  mpb  Check for SERROR returned in TCPgetmsg().
8.  16-AUG-96  mpb  'd' is of type MPSsd, so use it when type casting p_arg.
9.  29-AUG-96  mpb  Changed the hard-coded setting of tlen to sizeof(UCSHdr).
10. 03-SEP-96  mpb  p_fbuf is being free'd too soon in some cases.
11. 12-MAY-97  mpb  Make sure all malloc()'d memory is free()'d before
                    returning.
12. 12-MAY-97  mpb  Check to make sure there is memory malloc'd before blindly
                    calling free().
13. 29-MAY-97  lmm  Added update_iovecs() local routine - provides common
                    function to update io vectors 
14. 13-OCT-97  mpb  Type for TCPclose() should be mpssd.
15. 30-OCT-97  lmm  Added int typecasts to avoid Sunpro compiler warnings
16. 10-NOV-97  lmm  Call ReturnError to ensure user gets system err code
17. 14-NOV-97  mpb  setIOType needs to know if the sd is LOCAL or not (for NT).
18. 15-DEC-97  lmm  Modified end of message check in update_iovecs
19. 15-DEC-97  mpb  QNX support.
20. 03-AUG-98  mpb  TCPioctl()'s second paramter is an int; not a MPSsd.
21. 05-AUG-98  mpb  Have TCPioctl return with MPSerrno set if sendmsg()
                    fails.  This is how TCPwrite() and TCPgetmsg() work.
                    Previously, TCPioctl() would spin on a while() until
                    no error.
22. 05-AUG-98  mpb  Do not alter information from the client app (in case it 
                    needs to be re-sent).  Instead put it in a local structure,
                    and modify all we want.
23. 18-AUG-98  mpb  If the IOCTL is sent successfully, then have client's
                    input structure (xstrioctl) reflect what was returned.
                    Change CPROTO to CDATA for TCPputmsg()'s with empty 
                    control part. 
24. 16-SEP-98  mpb  See #8 in MPSapi.h.
22. 23-OCT-98  mpb  Support for Linux (LINUX).
23. 08-JUN-99  mpb  Needed to provide support for VxWorks in TCPpoll().  Since
                    the system poll() call only works for the embedded streams
                    case, we use the same logic as Windows NT, VMS and SRV3:
                    select().
24. 18-JUN-99  mpb  For the VxWorks/WinNT/VMS case of TCPpoll(), we need to
                    clear the revents field before any processing.
25. 18-JUN-99  mpb  For the VxWorks/WinNT/VMS case of TCPpoll(), the value
                    that select() returned was being returned.  This is wrong,
                    because select() returns the number of events that were
                    true, not the number of descriptors that had one or more
                    events set.
26. 15-OCT-99  lmm  TCPgetmsg() should return a value of -1 in control length
                    field when there is no control part (same for data part).
                    This is consistent with the way getmsg() works.
27. 12-APR-00  kls  Fixed a problem where the processor would completely
                    hangup when an X_UNLINK is executed.  We now poll in
                    TCPioctl and TCPgetmsg.
28. 09-MAY-00  mpb  The msghdr structure for Linux (and probably QNX) has
                    another parameter (msg_controllen) that needs to be 
                    initialized before it can be used.
29. 06-JUN-00  mpb  Need to check for EHOSTDOWN as a possible reason for a 
                    connect error, and clean up before returning.  This will 
                    prevent file descriptor leaks.
28. 16-JUN-00  mpb  Comment #27 should be broadened to say that the polling in
                    TCPioctl() and TCPgetmsg().  This does not just solve the
                    problem of hanging the cpu when a X_UNLINK is executed, but
                    other ioctls are sent to the board as well (like X_STR).
29. 07-DEC-00  mpb  We should not be doing PendingData() and PendingPri() on
                    system file descriptors (do them only on MPSsd's).
*/

#include "MPSinclude.h"

#ifdef WINNT

/*define SD_BOTH 0x02 */
#endif /* WINNT */

#ifdef   VMS
#define   MAXCONNTRIES   3
#endif


#ifdef ANSI_C
void update_iovecs ( struct msghdr*, int );
#else
void update_iovecs ( );
#endif /* ANSI_C */

/*<-------------------------------------------------------------------------
| 
|         MPS TCP Library Routines
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

MPSsd TCPopen ( OpenRequest *p_open, struct hostent *hp, struct servent *sp )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

MPSsd TCPopen(p_open, hp, sp)
OpenRequest *p_open;
struct hostent *hp;
struct servent *sp;

#endif   /* ANSI_C */
{
   OpenReq oreq;
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */
   char *p_resp;
   char *tmp_ptr = NULL;
   UCSHdr *p_hdr = (UCSHdr *)&oreq.uhdr;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   OpenData *p_odata;
   struct xiocblk *p_ioc;
   UCSHdr uhdr;
   struct sockaddr_in sa;
   struct linger linger;
   int   ii, dlen, errnum;
#ifdef   WINNT
   SOCKET   sc;
#else
    int      sc;
#endif   /* WINNT */
#if defined ( __hp9000s800 ) || defined ( WINNT ) || defined ( QNX ) || defined ( VXWORKS )
   struct fd_set wrfds;
   struct timeval t, *p_time;   /* If need hp_32 bit, this pointer needs to
                                   be protected. */
#else
   struct pollfd pfd;
#endif /* hp9000 || WINNT || QNX || VXWORKS */
#ifdef   VMS
   long retry = MAXCONNTRIES;
#endif   /* VMS */

#ifdef VXWORKS
   sa.sin_addr.s_addr = hp->h_addr;
#else
   memcpy((char *)&sa.sin_addr.s_addr, (char *)hp->h_addr, hp->h_length);
#endif /* VXWORKS */

   sa.sin_family = AF_INET;
   sa.sin_port = sp->s_port;
      
#ifndef   VMS

#ifdef   WINNT
   if ( ( sc = socket ( AF_INET, SOCK_STREAM, 0 ) ) == INVALID_SOCKET )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( ( sc = socket ( AF_INET, SOCK_STREAM, 0 ) ) == ERROR )
   {
#endif   /* WINNT */
      ReturnError( ESOCKET );
   }
 
   /* Set socket option for close */
   linger.l_onoff = TRUE;
   linger.l_linger = 0;

#ifdef   WINNT
   if ( setsockopt ( sc, SOL_SOCKET, SO_LINGER, ( char * ) &linger, 
      sizeof ( struct linger ) ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( setsockopt ( sc, SOL_SOCKET, SO_LINGER, ( char * ) &linger, 
      sizeof ( struct linger ) ) == ERROR )
   {
#endif   /* WINNT */
      ReturnError( errno );
   }


   /*
   ** There is a known bug in select() which may cause a socket to 
   ** "return TRUE for writing when in fact the write would block",
   ** SunOS 4.1.1.  When selecting for a connect completion the socket
   ** is selected for writing.  The above bug could cause the API to 
   ** believe that a connection has been established when it actually
   ** has not.
   */

   setIOType(sc, NONBLOCK, MPS_TCP);  /* #17 */
#ifdef   WINNT
   if ( connect ( sc, ( struct sockaddr * ) &sa, sizeof ( sa ) ) ==
      SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( connect ( sc, ( struct sockaddr * ) &sa, sizeof ( sa ) ) == ERROR )
   {
#endif
      if ( ( errno == EINPROGRESS ) || ( errno == EALREADY ) ||
            ( errno == EAGAIN ) )      /* for DGUX */
      {
#if defined ( __hp9000s800 ) || defined ( WINNT ) || defined ( QNX ) || \
    defined ( VXWORKS )
         FD_ZERO ( &wrfds );
         FD_SET ( sc, &wrfds );
         p_time = ( struct timeval * ) &t;
         p_time->tv_usec = 0;
         p_time->tv_sec = p_open->openTimeOut;
 
         ii = select ( sc+1, 0, &wrfds, 0, p_time );
#ifdef   WINNT
         errno = WSA2MPS ( WSAGetLastError ( ) );
#endif   /* WINNT */
 
         switch ( ii )
         {
            default:
#ifdef   WINNT
            case SOCKET_ERROR:
#else
            case ERROR:
#endif   /* WINNT */
               ReturnError ( errno );
 
            case 0:
#ifdef   WINNT
               closesocket ( sc );
#else
               close(sc);
#endif   /* WINNT */
               ReturnError ( EOPENTIMEDOUT );
 
            case 1:
            /*
            *  On HP systems there is a problem (bug) that allows
            *  an application to open a server with a service name
            *  that is not supported by the server and yet the socket
            *  is writable.
            *  This logic checks the connection status and returns
            *  an error if the socket is not really connected.
            *
            *  Might as well do the same check for Windows NT.
            */
#ifdef   __hp9000s800
            if ( FD_ISSET ( sc, &wrfds ) )
            {
               if ( connect ( sc, ( struct sockaddr * ) &sa, 
                  sizeof ( sa ) ) == ERROR )
               {
                  if ( errno != EISCONN )
                  {
                     ReturnError ( ECONNECT );
                  }
               }
            
               break;
            }

            ReturnError ( ECONNECT );
#else
            break;
#endif   /* in the __hp9000s800 || WINNT || QNX #ifdef, but not __hp9000s800 */
         }
#else   /* ! (__hp9000s800 || WINNT) */
         pfd.fd = sc;
         pfd.revents = 0;
         pfd.events = POLLOUT;
         
         ii = poll ( &pfd, 1, p_open->openTimeOut * 1000 );

         switch ( ii )
         {
            case 0:
               close ( sc );
               ReturnError ( EOPENTIMEDOUT );

            case 1:
               if ( pfd.revents & POLLERR )
                  ReturnError ( errno) ;

               if ( ( pfd.revents & POLLOUT ) )
                  break;

               MPSerrno = ECONNECT;
               return ( ERROR );

            default:
            case ERROR:
               MPSerrno = errno;
               return ( ERROR );
         }
#endif   /* (__hp9000s800 || WINNT || QNX || VXWORKS) */
      }
      /* #29 */
      else if ( ( errno == ECONNREFUSED ) || ( errno == EHOSTDOWN ) )
      {
#ifdef   WINNT
         closesocket ( sc );
#else
         close ( sc );
#endif   /* WINNT */
         ReturnError ( errno );
      }

      else
      {
         ReturnError( errno );
      }
   }

   setIOType ( sc, BLOCK, MPS_TCP );  /* #17 */

#else   /* VMS */

   while ( retry )
   {
      if ( (sc = socket(AF_INET, SOCK_STREAM, 0)) == ERROR )
      {
         ReturnError(ESOCKET);
      }

      /* Set socket option for close */
      linger.l_onoff = TRUE;
      linger.l_linger = 0;
      if ( setsockopt(sc, SOL_SOCKET, SO_LINGER, (char *)&linger, 8) == ERROR )
      {
         ReturnError(errno);
      }

      if ( (ii = connect(sc, (struct sockaddr *)&sa, sizeof(sa))) < 0 )
      {
         if ( errno == ETIMEDOUT )
         {
            perror("MPSopen (connect)");
            printf("MPSopen: Retrying connect.\n");
            close(sc);
            retry--;
            sleep(1);
            continue;
         }
         else
         {
            ReturnError(errno);
         }
      }
      else
      {
         break;
      }
   }

   if ( !retry )
   {
      MPSerrno = ECONNECT;
      return(ERROR);
   }

#endif   /* VMS */

   p_hdr->mtype = htons ( COPEN );
   p_hdr->flags = 0;
   p_hdr->csize = 0;
   p_hdr->dsize = htons ( sizeof ( OpenData ) );

   p_odata = &oreq.odata;

   p_odata->slotnum = htonl ( p_open->ctlrNumber );
   strcpy ( p_odata->protocol, p_open->protoName );
   p_odata->dev = htonl ( p_open->dev );
   p_odata->flags = htonl ( p_open->flags );

#ifdef   WINNT
/****
   Have to use send() because Window's NT sockets are created as overlapped
   handles, which allow stuff like select() to work properly.  The problem
   though, is that the C-runtime library routines (_write(), _read() )require
   a non-overlapped handle. 
****/
   if ( send ( sc, ( char * ) p_hdr, sizeof ( OpenReq ), 0 ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
      closesocket ( sc );
      ReturnError ( errno );
   }
#else
   if ( write ( sc, ( char * ) p_hdr, sizeof ( OpenReq ) ) == ERROR )
   {
      close ( sc );
      ReturnError ( errno );
   }
#endif   /* WINNT */

   /* A wait read for the response.  Only an SIOCACK or SERROR are
    * valid here.
    */
   while ( readHdr ( sc, &uhdr ) == ERROR )
   {
      if ( MPSerrno != EWOULDBLOCK )
      {
         return ( ERROR );
      }
   }

   /* Calculate size of data */
   dlen = uhdr.csize + uhdr.dsize;

   if ( dlen )
   {
      /* Get a buffer */
      if ( ! ( p_resp = ( char * ) malloc ( dlen ) ) )
      {
         ReturnError ( ENOBUFS );
      }

      tmp_ptr = p_resp;    /* Hold onto beginning for "free()" */
   }

   /* Read Data portion. Don't care about data now, but may need to
    * some day.
    */
   while ( readData ( sc, p_resp, dlen ) == ERROR )
   {
      if ( MPSerrno != EWOULDBLOCK )
      {
         /* #11 */
         if ( tmp_ptr )
         {
            free ( tmp_ptr );
         }

         return ( ERROR );
      }
   }

   switch ( uhdr.mtype )
   {
      case SOPENACK:
         /* This should never fail, but .. */
         if ( (ii = addDesc ( sc, MPS_TCP ) ) < 0 )
         {
            MPSclose ( sc );
            /* #11 */
            if ( tmp_ptr )
            {
               free ( tmp_ptr );
            }

            ReturnError ( ENOBUFS );
         }
         else
         {
            p_odata =
               ( OpenData * ) ( ( char * ) p_resp + uhdr.csize );/* #1 */
            SetStrId ( ii, ntohl ( p_odata->slotnum ) );   /* #1 */

            if ( !MaxDblock )
            {
               MaxDblock = ntohl ( p_odata->dev );
            }

            /* Free temp buffer space */
            /* #12 */
            if ( tmp_ptr )
            {
               free ( tmp_ptr );
            }

            return ( ii );
         }

      case SIOCNAK:
         TCPclose ( sc );
         if ( uhdr.csize )
         {
            p_ioc = ( struct xiocblk * ) p_resp;

            errnum = ntohl ( p_ioc->ioc_error );   /* #16 */

            p_resp += uhdr.csize;
         }
         else
            errnum = EIO;   /* #16 */

         if ( uhdr.dsize )
         {
            memcpy ( p_open, p_resp, sizeof ( OpenRequest ) );
         }

         /* #11 */
         if ( tmp_ptr )
         {
            free ( tmp_ptr );
         }

         ReturnError ( errnum );   /* #16 */

      case SERROR:
      /* Error code is in first byte of data */
         errnum = *p_resp;      /* #16 */

         /* #11 */
         if ( tmp_ptr )
         {
            free ( tmp_ptr );
         }

         ReturnError ( errnum );   /* #16 */

      default:
      /* Free temp buffer space */
         /* #12 */
         if ( tmp_ptr )
         {
            free ( tmp_ptr );
         }

#ifdef   WINNT
         closesocket ( sc );
#else
         close ( sc );
#endif   /* WINNT */
         ReturnError ( EBADMSG );
   }
} /* end TCPopen() */

#ifndef   VMS
/*<-------------------------------------------------------------------------
| TCPopenS()
|
| Perform the first phase of the open server procedure. Attempt to establish
| a TCP connection to the server in non-blocking mode. Returns the socket
| descriptor from successful socket call. The application must poll for
| completion of the TCP connection and then call MPSopenP() with the socket
| descriptor to complete the open to the server.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int TCPopenS ( struct hostent *hp, struct servent *sp )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int TCPopenS(hp, sp)
struct hostent *hp;
struct servent *sp;

#endif   /* ANSI_C */
{
   struct sockaddr_in sa;
   struct linger linger;
   int sc;

#ifdef VXWORKS
   sa.sin_addr.s_addr = hp->h_addr;
#else
   memcpy ( (char * ) &sa.sin_addr.s_addr,
      ( char * ) hp->h_addr, hp->h_length );
#endif /* VXWORKS */

   sa.sin_family = AF_INET;
   sa.sin_port = sp->s_port;

#ifdef   WINNT
   if ( ( sc = socket ( AF_INET, SOCK_STREAM, 0 ) ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( ( sc = socket ( AF_INET, SOCK_STREAM, 0 ) ) == ERROR )
   {
#endif   /* WINNT */
      ReturnError ( ESOCKET );
   }
   
   /* Set socket option for close */
   linger.l_onoff = TRUE;
   linger.l_linger = 0;
#ifdef   WINNT
   if ( setsockopt ( sc, SOL_SOCKET, SO_LINGER, ( char * ) &linger,
      sizeof ( struct linger ) ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( setsockopt ( sc, SOL_SOCKET, SO_LINGER, ( char * ) &linger,
      sizeof ( struct linger ) ) == ERROR )
   {
#endif   /* WINNT */
      ReturnError ( errno );
   }

   /* Do nonblocking connect, check status */
   setIOType ( sc, NONBLOCK, MPS_TCP );   /* #17 */
#ifdef   WINNT
   if ( connect ( sc, ( struct sockaddr * ) &sa, sizeof ( sa ) ) == ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( connect ( sc, ( struct sockaddr * ) &sa, sizeof ( sa ) ) == ERROR )
   {
#endif   /* WINNT */
      if ( errno == EINPROGRESS || errno == EALREADY )
      {
         MPSerrno = EWOULDBLOCK;
         return ( sc );
      }  /* #29 */
      else if ( ( errno == ECONNREFUSED ) || ( errno == EHOSTDOWN ) )
      {
#ifdef   WINNT
         closesocket ( sc );
#else
         close ( sc );
#endif   /* WINNT */
      }

      ReturnError ( errno );
   }

   return ( sc );
} /* end TCPopenS() */
#endif   /* !VMS */

#ifndef   VMS
/*<-------------------------------------------------------------------------
| TCPopenP()
|
| Perform the final phase of the TCPopen procedure to establish the 
| end-to-end connection between the application and a specific protocol
| on the server.
V  
-------------------------------------------------------------------------->*/

#if defined ( __hp9000s800 ) || defined ( WINNT ) || defined ( QNX ) || defined ( VXWORKS )

#ifdef   ANSI_C

MPSsd TCPopenP ( OpenRequest *p_open, int sc, struct hostent *hp, 
                 struct servent *sp )

#else

MPSsd TCPopenP(p_open, sc, hp, sp)
OpenRequest *p_open;
int sc;
/* If need hp_32 bit, these pointers will need to be protected. */
struct hostent *hp;
struct servent *sp;
#endif   /* ANSI_C */

#else

#ifdef   ANSI_C

MPSsd TCPopenP ( OpenRequest *p_open, int sc )

#else

MPSsd TCPopenP(p_open, sc)
OpenRequest *p_open;
int sc;

#endif   /* ANSI_C */

#endif   /* __hp9000s800 || WINNT  || QNX || VXWORKS */

{
   OpenReq oreq;
   OpenData *p_odata;
   UCSHdr uhdr;
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */
   UCSHdr *p_hdr = (UCSHdr *)&oreq.uhdr;
   struct xiocblk *p_ioc;
   char *p_resp;
   char *tmp_ptr = NULL;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   int ii, dlen, errnum;
#if defined ( __hp9000s800 ) || defined ( WINNT ) || defined ( QNX ) || defined ( VXWORKS )
   struct fd_set wrfds;
   struct timeval t, *p_time;
   struct sockaddr_in sa;
#else
   struct pollfd pfd;
#endif /* hp9000 || WINNT  || QNX || VXWORKS */

   /*
   ** There is a known bug in select() which may cause a socket to 
   ** "return TRUE for writing when in fact the write would block",
   ** SunOS 4.1.1.  When selecting for a connect completion the socket
   ** is selected for writing.  The above bug could cause the API to 
   ** believe that a connection has been established when it actually
   ** has not.
   */

   /*
    *  Check for connect completion by
    *  selecting the socket for writing.
    */
   setIOType ( sc, NONBLOCK, MPS_TCP );  /* #17 */

#if defined ( __hp9000s800 ) || defined ( WINNT ) || defined ( QNX ) || defined ( VXWORKS )
   /*
    *  Rebuild the socket structure to accomodate the
    *  HP sysytem connect bug.
    */
#ifdef VXWORKS
   sa.sin_addr.s_addr = hp->h_addr;
#else
   memcpy ( ( char * ) &sa.sin_addr.s_addr, ( char * ) hp->h_addr,
      hp->h_length );
#endif /* VXWORKS */

   sa.sin_family = AF_INET;
   sa.sin_port = sp->s_port;

   FD_ZERO ( &wrfds );
   FD_SET ( ( unsigned int ) sc, &wrfds );
   p_time = ( struct timeval * ) &t;
   p_time->tv_usec = 0;
   p_time->tv_sec = 0;

   ii = select( sc+1, 0, &wrfds, 0, p_time );
#ifdef   WINNT
   errno = WSA2MPS ( WSAGetLastError ( ) );
#endif   /* WINNT */
   switch ( ii )
   {
      default:
#ifdef   WINNT
      case SOCKET_ERROR:
#else
      case ERROR:
#endif   /* WINNT */
         ReturnError ( errno );

      case 1:
      /*
      *  On HP systems there is a problem (bug) that allows
      *  an application to open a server with a service name
      *  that is not supported by the server and yet the socket
      *  is writable.
      *  This logic checks the connection status and returns
      *  an error if the socket is not really connected.
      *
      * Might as well do the check for Windows NT.
      */
         if ( FD_ISSET ( sc, &wrfds ) )
         {
            if ( connect ( sc, ( struct sockaddr * ) &sa, sizeof ( sa ) )
               == ERROR )
            {
               if ( errno != EISCONN )
               {
                  ReturnError ( ECONNECT );
               }
            }
            break;
         }

      case 0:
         ReturnError ( EWOULDBLOCK );
   }
#else
   pfd.fd = sc;
   pfd.revents = 0;
   pfd.events = POLLOUT;
   
   ii = poll(&pfd, 1, p_open->openTimeOut*1000);

   switch ( ii )
   {
      default:
      case ERROR:
         MPSerrno = errno;
         return(ERROR);

      case 1:
         if ( pfd.revents & POLLERR )
            ReturnError(errno);

         if ( (pfd.revents & POLLOUT) )
            break;

      case 0:
         ReturnError(EWOULDBLOCK);
   }
#endif

   setIOType ( sc, BLOCK, MPS_TCP ); /* #17 */

   p_hdr->mtype = htons ( COPEN );
   p_hdr->flags = 0;
   p_hdr->csize = 0;
   p_hdr->dsize = htons ( sizeof ( OpenData ) );

   p_odata = &oreq.odata;

   p_odata->slotnum = htonl ( p_open->ctlrNumber );
   strcpy ( p_odata->protocol, p_open->protoName );
   p_odata->dev = htonl ( p_open->dev );
   p_odata->flags = htonl ( p_open->flags );

#ifdef   WINNT
/****
   Have to use send() because Window's NT sockets are created as overlapped
   handles, which allow stuff like select() to work properly.  The problem
   though, is that the C-runtime library routines (_write(), _read() )require
   a non-overlapped handle. 
****/
   if ( send ( sc, ( char * ) p_hdr, sizeof ( OpenReq ), 0 ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
      closesocket ( sc );
      ReturnError ( errno );
   }
#else
   if ( write ( sc, ( char * ) p_hdr, sizeof ( OpenReq ) ) == ERROR )
   {
      close ( sc );
      ReturnError ( errno );
   }
#endif   /* WINNT */

   /* A wait read for the response.  Only an SIOCACK or SERROR are
    * valid here.
    */
   while ( readHdr ( sc, &uhdr ) == ERROR )
   {
      if ( MPSerrno != EWOULDBLOCK )
      {
         return ( ERROR );
      }
   }

   /* Calculate size of data */
   dlen = uhdr.csize + uhdr.dsize;

   if ( dlen )
   {
      /* Get a buffer */
      if ( ! ( p_resp = ( char * ) malloc ( dlen ) ) )
      {
         ReturnError ( ENOBUFS );
      }

      tmp_ptr = p_resp;    /* Hold onto beginning for "free()" */
   }

   /* Read Data portion. Don't care about data now, but may need to
    * some day.
    */
   while ( readData ( sc, p_resp, dlen ) == ERROR )
   {
      if ( MPSerrno != EWOULDBLOCK )
      {
         /* #11 */
         if ( tmp_ptr )
         {
            free ( tmp_ptr );
         }

         return ( ERROR );
      }
   }

   switch ( uhdr.mtype )
   {
      case SOPENACK:
         /* This should never fail, but .. */
         if ( (ii = addDesc ( sc, MPS_TCP ) ) < 0 )
         {
            MPSclose ( sc );

            /* #11 */
            if ( tmp_ptr )
            {
               free ( tmp_ptr );
            }

            ReturnError ( ENOBUFS );
         }
         else
         {
            p_odata =
               ( OpenData * ) ( ( char * ) p_resp + uhdr.csize );/* #1 */
            SetStrId ( ii, ntohl ( p_odata->slotnum ) );   /* #1 */

            if ( !MaxDblock )
            {
               MaxDblock = ntohl ( p_odata->dev );
            }

            /* Free temp buffer space */
            /* #12 */
            if ( tmp_ptr )
            {
               free ( tmp_ptr );
            }

            return ( ii );
         }

      case SIOCNAK:
         TCPclose ( sc );
         if ( uhdr.csize )
         {
            p_ioc = ( struct xiocblk * ) p_resp;

            errnum = ntohl ( p_ioc->ioc_error );   /* #16 */

            p_resp += uhdr.csize;
         }
         else
         {
            errnum = EIO;      /* #16 */
         }

         if ( uhdr.dsize )
         {
            memcpy ( p_open, p_resp, sizeof ( OpenRequest ) );
         }

         /* #11 */
         if ( tmp_ptr )
         {
            free ( tmp_ptr );
         }

         ReturnError ( errnum );   /* #16 */

      case SERROR:
      /* Error code is in first byte of data */
         errnum = *p_resp;      /* #16 */

         /* #11 */
         if ( tmp_ptr )
         {
            free ( tmp_ptr );
         }

         ReturnError ( errnum );   /* #16 */

      default:
      /* Free temp buffer space */

         /* #12 */
         if ( tmp_ptr )
         {
            free ( tmp_ptr );
         }

#ifdef   WINNT
         closesocket ( sc );
#else
         close ( sc );
#endif   /* WINNT */
         ReturnError ( EBADMSG );
   }
} /* end TCPopenP() */
#endif   /* !VMS */

/*<-------------------------------------------------------------------------
| TCPwrite()
|
| Creates a CDATA header and appends the user's data to it using the
| scatter/gather write facilitie sendmsg().
| 11/14/94 jl.  Code had assumed TCP would take at least the uclid
|               header on the first try, but that's not always true.
|               Modify code to send the rest of the header if necessary
|               on a retry.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C

int TCPwrite ( MPSsd d, caddr_t p_data, int len )

#else

int TCPwrite(d, p_data, len)
MPSsd d;
caddr_t p_data;
int len;

#endif   /* ANSI_C */
{
   struct msghdr msg;
   struct iovec iov[2];
   UCSHdr uhdr;
   int s, nvecs, write_count, tlen;

   s = GetSocket ( d );

   if ( TCPWrCnt ( d ) )
   {
      /* Retry */
      if ( TCPWrCnt ( d ) < UHDRLEN )
      {
         /* Retry of header that was not completely sent last time.
            Reinitialize header. */
         uhdr.mtype = htons ( CDATA );
         uhdr.flags = 0;
         uhdr.csize = 0;
         uhdr.dsize = htons ( ( ushort )len );

         /* Remainder of UCS Header */
         iov [ 0 ].iov_base =
            ( caddr_t ) ( ( char * ) &uhdr + TCPWrCnt ( d ) );
         iov [ 0 ].iov_len = sizeof ( UCSHdr ) - TCPWrCnt ( d );

         /* Input data */
         iov [ 1 ].iov_base = p_data;
         iov [ 1 ].iov_len = len;

         nvecs = 2;
         tlen = len + UHDRLEN - TCPWrCnt ( d );
      }
      else
      {
         /* Retry of data that was not completely sent last time. */

         len += UHDRLEN;
         len -= TCPWrCnt ( d );

         iov [ 0 ].iov_base = 
            ( char * ) ( ( p_data + TCPWrCnt ( d ) ) - UHDRLEN );
         iov [ 0 ].iov_len = len;

         nvecs = 1;
         tlen = len;
      }
   }
   else
   {
      /* First time through. */
      uhdr.mtype = htons ( CDATA );
      uhdr.flags = 0;
      uhdr.csize = 0;
      uhdr.dsize = htons ( ( ushort )len );
   
      /* UCS Header */
      iov [ 0 ].iov_base = ( caddr_t ) &uhdr;
      iov [ 0 ].iov_len = sizeof ( UCSHdr );
   
      /* Input data */
      iov [ 1 ].iov_base = p_data;
      iov [ 1 ].iov_len = len;

      nvecs = 2;

      tlen = len + UHDRLEN;
   }

   /* Init msghdr */
   msg.msg_name = NULL;
   msg.msg_namelen = 0;
   msg.msg_iov = ( struct iovec * ) iov;
   msg.msg_iovlen = nvecs;
#if defined (QNX) || defined (LINUX)
   msg.msg_flags = 0;
   msg.msg_control = 0;
   msg.msg_controllen = 0;   /* #28 */
#else
   msg.msg_accrights = NULL;
   msg.msg_accrightslen = 0;
#endif /* QNX || LINUX */

   /* Do while message not completely written (or error) */
   while ( tlen )
   {
#ifdef   WINNT
      if ( ( write_count = sendmsg ( s, &msg, 0 ) ) == SOCKET_ERROR )
      {
         errno = WSA2MPS ( WSAGetLastError ( ) );
#else
      if ( ( write_count = sendmsg (s, &msg, 0 ) ) == ERROR )
      {
#endif   /* WINNT */
         /* Error */
            ReturnError ( errno );
      }

      /* reduce total length to write by amount already written */
      tlen -= write_count;

      /* update total number of bytes written */
      TCPWrCnt ( d ) += write_count;

      /* if message not completely written, update io vectors */
      if ( tlen )
      {
         /* #13 - update io vectors to reflect amount written */
         update_iovecs (&msg, write_count);
      }
   } /* end while message not completely written */

   /* note no pending data and return */
   TCPWrCnt ( d ) = 0;
   return ( len );

} /* end TCPwrite() */


/*<-------------------------------------------------------------------------
| TCPread()
|
| V1.0 emulates Message Non-Discard mode. 
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int TCPread ( MPSsd d, caddr_t p_buf, int len )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int TCPread(d, p_buf, len)
MPSsd d;
caddr_t p_buf;
int len;

#endif   /* ANSI_C */
{
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */
   UCSHdr *p_hdr;
   FreeBuf *p_free;
   caddr_t p_data;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   UCSHdr uhdr;
   int s,  rval, errnum;

   s = GetSocket ( d );

   if ( PendingPri ( d ) )
   {
     ReturnError ( EBADMSG );
   }

   if ( PendingData ( d ) )
   {
      if ( !( p_free = peekFree ( xstDescriptors [ d ].p_mlist ) ) )
      {
         ReturnError ( EIO );
      }
      p_hdr =
         ( UCSHdr * ) ( ( char * ) p_free + sizeof ( FreeBuf ) );   /* #1 */

      if ( p_hdr->mtype == SDATA )
      {
         if ( ! ( p_free = getFree ( xstDescriptors [ d ].p_mlist ) ) ||
             (p_hdr != ( UCSHdr * )
             ( ( char * )p_free + sizeof ( FreeBuf ) ) ) )/* #1 */
         {
            ReturnError ( EIO );
         }
         p_data = ( caddr_t ) ( ( char * ) p_hdr + UHDRLEN );      /* #1 */
         rval = min ( len, ( int )p_hdr->dsize );         /* #15 */
         memcpy ( p_buf, p_data, rval );
         if ( rval == p_hdr->dsize )
         {
            /* Copied complete message */
            if ( p_free )
            {
               free( p_free );
            }

            return( p_hdr->dsize );
         }
         else
         {
            /* Some data left, save it */
            p_hdr->dsize -= rval;
            /* Move data up */
            memcpy(p_data, ( char * ) ( p_data+rval ), rval );
 
            headFree ( xstDescriptors [ d ].p_mlist, p_free );
            return ( rval );
         }
      }
      else
      {
         ReturnError ( EBADMSG );
      }
   }

   memset( &uhdr, 0, UHDRLEN );

   if ( BlockIO ( d ) )
   {
      while ( readHdr ( s, &uhdr ) == ERROR )
      {
         if ( MPSerrno != EWOULDBLOCK )
         {
            return( ERROR );
         }
      }
   }
   else
   {
      if ( readHdr ( s, &uhdr ) == ERROR )
      {
         if ( (MPSerrno = errno) == EWOULDBLOCK )
         {
            MPSerrno = EAGAIN;
         }
         return(ERROR);
      }
   }

   if ( uhdr.mtype == SDATA )
   {
      rval = min ( len, ( int ) uhdr.dsize );      /* #15 */
  
      /* Recv data directly into user's buffer */
      while ( readData ( s, p_buf, rval ) == ERROR )
      {
         if ( MPSerrno != EWOULDBLOCK )
         {
            return ( ERROR );
         }
      }

      if ( rval != uhdr.dsize )
      {
         uhdr.dsize -= rval;

         /* User's buffer to small, must save remaining data */
         if ( (p_free = ( FreeBuf * ) readSave ( s, &uhdr ) ) == NULL )
         {
            return ( ERROR );
         }
      
         addFree ( xstDescriptors [ d ].p_mlist, p_free );
         return ( rval );
      }
      else
      {
         return ( rval );
      }
   }
   
   /* Non Data message, read rest of message */
   if ( (p_free = ( FreeBuf * ) readSave ( s, &uhdr ) ) == NULL )
   {
      return ( ERROR );
   }

   switch ( uhdr.mtype )
   {
      case SOPENACK:
         /* This is an error */
         break;
          
      case SPROTO:
         if ( uhdr.flags == RS_HIPRI )
         {
            addFree ( xstDescriptors [ d ].p_ilist, p_free );
         }
         else
         {
            addFree ( xstDescriptors [ d ].p_mlist, p_free );
         }
         MPSerrno = EBADMSG;
         return ( ERROR );
         break;

      case SIOCACK:
         /* This is invalid here, trash the message */
         break;
 
      case SERROR:
         /* Not sure here, this is fatal */
         p_data =
            ( caddr_t ) ( ( char * ) p_free +
            sizeof ( FreeBuf ) + UHDRLEN );/* #1 */

         errnum = *p_data;      /* #16 */
         free ( p_free );
         ReturnError ( errnum );   /* #16 */

      default:
         break;
   }

   free ( p_free );
   MPSerrno = EBADMSG;
   return ( ERROR );
} /* end TCPread() */

/*<-------------------------------------------------------------------------
| TCPioctl()
|
| Once request is successfully made the process is blocked until a 
| response to the ioctl request is received from the server.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C

int TCPioctl ( MPSsd d, int command, caddr_t p_arg )

#else

int TCPioctl(d, command, p_arg)
MPSsd d;
int command;
caddr_t p_arg;

#endif   /* ANSI_C */
{
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */
   struct xstrioctl *p_iocb;
   struct xiocblk *p_ioc;
   caddr_t p_data;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   FreeBuf *p_fbuf;
   struct xstrioctl  striocb;
   struct msghdr msg;
   struct iovec iov[2];
   struct xpollfd xpfd;
   UCSHdr uhdr;
   int tlen, write_count, s;   /* #13 */
   char iotype;
   int   ioc_rval, ioc_error, errnum, nvecs;
   ulong  muxid;

   s = GetSocket ( d );

   if ( PendingIoc ( d ) )
   {
     ReturnError ( EINVAL );
   }

   uhdr.mtype = htons ( CIOCTL );
   uhdr.csize = 0;
   uhdr.dsize = 0;

   switch ( command )
   {
      case X_POP:
      {
         /* No argument */

         uhdr.flags = htons ( X_POP );
         p_iocb = ( struct xstrioctl * ) &striocb;
         p_iocb->ic_len = 0;
         p_iocb->ic_dp = NULL;
         break;
      }
    
      case X_PUSH:
      {
         /* p_arg is the name of the module to PUSH */

         p_iocb = ( struct xstrioctl * ) &striocb;
         uhdr.flags = htons ( X_PUSH );
#ifdef   WINNT
         uhdr.dsize = ( ushort ) p_iocb->ic_len = 
            htons ( ( ushort ) ( strlen ( p_arg ) + 1 ) );
#else
         uhdr.dsize = p_iocb->ic_len = 
            htons ( strlen ( p_arg ) + 1 );
#endif   /* WINNT */
         p_iocb->ic_dp = p_arg;
         break;
      }
    
      case X_LINK:
      {
         /* p_arg is the file descriptor to be linked with this stream */

         p_iocb = ( struct xstrioctl * ) &striocb;
         uhdr.flags = htons ( X_LINK );
         uhdr.dsize = p_iocb->ic_len =  htons( 4 ); 

         if ( (MPSsd)p_arg == d )      /* #1 */      /* #8 */
         {
            MPSerrno = EINVAL;
            return ( ERROR );
         }

         muxid = GetStrId ( ( ulong ) p_arg);   /* #1 */
         muxid = htonl ( muxid );      /* #1 */
         p_iocb->ic_dp = ( caddr_t ) &muxid;
         break;
      }

      case X_UNLINK:
      {
         /* p_arg is the muxid to unlink or -1 */

         p_iocb = ( struct xstrioctl * ) &striocb;
         uhdr.flags = htons ( X_UNLINK );
         uhdr.dsize = p_iocb->ic_len = htons ( 4 );

         muxid = ( ulong ) p_arg;         /* #1 */
         muxid = htonl ( muxid );         /* #1 */
         p_iocb->ic_dp = ( caddr_t ) &muxid;
         break;
      }

      case X_STR:
      {
         memcpy ( &striocb, p_arg, sizeof ( struct xstrioctl ) ); /* #22 */
         p_iocb = ( struct xstrioctl * ) &striocb;
         uhdr.flags = htons ( ( ushort ) p_iocb->ic_cmd );
         uhdr.dsize = htons ( ( ushort ) p_iocb->ic_len );
         /* Make network byte order for use below in making tcp packet */
         p_iocb->ic_len = htons ( ( ushort ) p_iocb->ic_len );
         break;
      }

      case X_FLUSH:
      {
         /* p_arg FLUSHR, FLUSHW or FLUSHRW */

         p_iocb = ( struct xstrioctl * ) &striocb;
         uhdr.flags = htons( X_FLUSH );
         uhdr.csize = p_iocb->ic_len = htons ( 1 );

         /* Use iotype as storage */
         iotype = ( uchar ) ( ( ulong )p_arg & 0xff );   /* #1 */
         p_iocb->ic_dp = ( caddr_t ) &iotype;
         break;
      }
   }

   /* #21 - The client app better not have mucked with the sending info. */
   if ( TCPWrCnt ( d ) )
   {
      /* Retry */
      if ( TCPWrCnt ( d ) < UHDRLEN )
      {
         /* Retry of header that was not completely sent last time.
            Reinitialize header. */

         /* Remainder of UCS Header */
         iov [ 0 ].iov_base =
            ( caddr_t ) ( ( char * ) &uhdr + TCPWrCnt ( d ) );
         iov [ 0 ].iov_len = UHDRLEN - TCPWrCnt ( d );

         /* Input data */
         iov [ 1 ].iov_base = p_iocb->ic_dp;
         iov [ 1 ].iov_len = ntohs ( (ushort ) p_iocb->ic_len );

         nvecs = 2;
         tlen = UHDRLEN + iov [ 1 ].iov_len - TCPWrCnt ( d );
      }
      else
      {
         /* Retry of data that was not completely sent last time. */
         iov [ 0 ].iov_base = 
            ( char * ) ( ( p_iocb->ic_dp + TCPWrCnt ( d ) ) - UHDRLEN );
         iov [ 0 ].iov_len = UHDRLEN + ntohs ( (ushort ) p_iocb->ic_len )
            - TCPWrCnt ( d );

         nvecs = 1;
         tlen = iov [ 0 ].iov_len;
      }
   }
   else
   {
      /* First time through */

      /* 
       * Create IO Vectors and send message via IP.
       */

      /* UCS Header */
      iov [ 0 ].iov_base = ( caddr_t ) &uhdr;
      iov [ 0 ].iov_len = UHDRLEN;
      
      /* Input data */
      iov [ 1 ].iov_base = p_iocb->ic_dp;
      iov [ 1 ].iov_len = ntohs ( (ushort ) p_iocb->ic_len );   /* #2 */
      tlen = UHDRLEN + iov [ 1 ].iov_len;
      nvecs = 2;
   }
      
   /* Init msghdr */
   msg.msg_name = NULL;
   msg.msg_namelen = 0;
   msg.msg_iov = ( struct iovec * ) iov;
   msg.msg_iovlen = nvecs;
#if defined (QNX) || defined (LINUX)
   msg.msg_flags = 0;
   msg.msg_control = 0;
   msg.msg_controllen = 0;   /* #28 */
#else
   msg.msg_accrights = NULL;
   msg.msg_accrightslen = 0;
#endif /* QNX || LINUX */
      
   /* Do while message not completely written (or error) */
   while ( tlen )
   {
#ifdef   WINNT
      if ( ( write_count = sendmsg ( s, &msg, 0 ) ) == SOCKET_ERROR )
      {
         errno = WSA2MPS ( WSAGetLastError ( ) );
#else
      if ( ( write_count = sendmsg ( s, &msg, 0 ) ) == ERROR )
      {
#endif   /* WINNT */

         /* ERROR */
         ReturnError ( errno );
      }
   
      /* reduce total length to write by amount already written */
      tlen -= write_count;

      /* update total number of bytes written */
      TCPWrCnt ( d ) += write_count;

      /* if message not completely written, update io vectors */
      if ( tlen )
      {
         /* #13 - update io vectors to reflect amount written */
         update_iovecs ( &msg, write_count);
      }
   } /* end while message not completely written */

   /* Note no pending data. */
   TCPWrCnt ( d ) = 0;
      
   if ( command == X_FLUSH )
   {
      return ( 0 );
   }

   SetPendIoc ( d );
   iotype = !BlockIO ( d );

#ifndef   VMS
   if ( setIOType ( s, 1, MPS_TCP ) == ERROR )   /* #17 */
#else
   if ( MPSvmssetIOtype ( d, 1 ) == ERROR )
#endif
   {
     ReturnError ( EIO );
   }

   /* Get response to IOCTL */
   while ( TRUE )
   {
      while ( readHdr ( s, &uhdr ) == ERROR )
      {
         if ( MPSerrno != EWOULDBLOCK )
         {
            return ( ERROR );
         }

         /* #27 - Added poll to return CPU to scheduler to break
            real-time spin loop                                  */

         xpfd.sd = sd2MPSsd(s);
         xpfd.revents = 0;
         xpfd.events = POLLIN | POLLPRI;

         errnum = MPSpoll(&xpfd, 1, -1);

         if(errnum == MPS_ERROR)
         {
            if((MPSerrno != EAGAIN) &&
               (MPSerrno != EINTR ) &&
               (MPSerrno != EWOULDBLOCK ))
            {
               MPSperror("API Internal problem");
            }
   
            continue;
         }

      }

      if ( ( p_fbuf = ( FreeBuf * ) readSave ( s, &uhdr ) ) == NULL )
      {
         return ( ERROR );
      }

      switch ( uhdr.mtype )
      {
         case SOPENACK:
            /* Not sure here */
            break;
 
         case SDATA:
            addFree ( xstDescriptors [ d ].p_mlist, p_fbuf );
            break;

         case SPROTO:
            if ( uhdr.flags == RS_HIPRI )
            {
               addFree ( xstDescriptors [ d ].p_ilist, p_fbuf );
            }
            else
            {
               addFree ( xstDescriptors [ d ].p_mlist, p_fbuf );
            }
            break;

         case SIOCACK:
             p_iocb = ( struct xstrioctl * ) p_arg; /* #23 */

            /* Trash buffer, return OK */
            ClrPendIoc ( d );

            if ( uhdr.csize )
            {
               p_ioc = ( struct xiocblk * ) ( ( char * ) p_fbuf + 
                          sizeof ( FreeBuf ) + UHDRLEN );      /* #1 */
               p_data =
                  ( caddr_t ) ( ( char * ) p_ioc + uhdr.csize );   /* #1 */
            }
            else
            {                  /* #1 */
               p_data = ( caddr_t ) ( ( char * ) p_fbuf +
                  sizeof ( FreeBuf ) + UHDRLEN );
            }

            /* #23 - Return fields for X_STR */
            if ( command == X_STR )
            {
               if ( uhdr.dsize )
               {
                  memcpy(p_iocb->ic_dp, p_data, uhdr.dsize);
                  p_iocb->ic_len = uhdr.dsize;
               }
               else
               {
                  p_iocb->ic_len = 0;
               }
            }

            /* #10 */
            ioc_rval   = p_ioc->ioc_rval;
            ioc_error   = p_ioc->ioc_error;
            free ( p_fbuf );

            /* Reset I/O type */
#ifndef   VMS
            setIOType ( s, iotype, MPS_TCP );   /* #17 */
#else
            MPSvmssetIOtype ( d, iotype );
#endif

            if ( ntohl ( ioc_error)  )      /* #1 */
            {
               ReturnError ( ntohl ( ioc_error ) );    /* #1 */
            }
            else
            {
               return ( ntohl ( ioc_rval ) );
            }   
       
         case SIOCNAK:
            p_iocb = ( struct xstrioctl * ) p_arg; /* #23 */

            ClrPendIoc ( d );
            if ( uhdr.csize )
            {
               p_ioc  = ( struct xiocblk * ) ( ( char * ) p_fbuf +
                  sizeof ( FreeBuf ) + UHDRLEN );   /* #1 */
               p_data =
                  ( caddr_t ) ( ( char * ) p_ioc + uhdr.csize );      /* #1 */
            }
            else /* #1 */
            {
               p_data = ( caddr_t ) ( ( char * ) p_fbuf + sizeof ( FreeBuf )
                  + UHDRLEN );
            }

            /* #23 - Return fields for X_STR */
            if ( command == X_STR )
            {
               if ( uhdr.dsize )
               {
                  memcpy ( p_iocb->ic_dp, p_data, uhdr.dsize );
                  p_iocb->ic_len = uhdr.dsize;
               }
               else
               {
                  p_iocb->ic_len = 0;
               }
            }

            /* Don't need buffer, free it */    /* #10 */
            ioc_rval   = p_ioc->ioc_rval;
            ioc_error   = p_ioc->ioc_error;
            free ( p_fbuf );

            /* Reset I/O type */
#ifndef   VMS
            setIOType ( s, iotype, MPS_TCP );   /* #17 */
#else
            MPSvmssetIOtype ( d, iotype );
#endif
            if ( ntohl ( ioc_error ) )         /* #1 */
            {
               ReturnError ( ntohl ( ioc_error ) );      /* #1 */
            }
            else
            {
               return ( ntohl ( ioc_rval ) );                  /* #1 */
            }

         case SERROR:
            p_data = ( caddr_t ) ( ( char * ) p_fbuf + 
               sizeof ( FreeBuf ) + UHDRLEN );  /* #1 */

            errnum = *p_data;      /* #16 */
            free ( p_fbuf );
            ReturnError ( errnum );   /* #16 */

         default:
            printf ( "MPSapi: Error unknown message %x received.\n",
               uhdr.mtype );
            break;
      }
   }
} /* end TCPioctl() */


/*<-------------------------------------------------------------------------
| TCPgetmsg()
|
| V1.0 Performs a blocking read for either an SDATA or SPROTO message.
| Uses the flags field to indicate MORECTL or MOREDATA is remaining in a
| message.  Subsequent calls will retreive the remaining information.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int TCPgetmsg ( MPSsd d, struct xstrbuf *p_ctl, struct xstrbuf *p_dbuf, 
                int *flags )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int TCPgetmsg(d, p_ctl, p_dbuf, flags)
MPSsd d;
struct xstrbuf *p_ctl, *p_dbuf;
int *flags;

#endif   /* ANSI_C */
{
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */
   UCSHdr *p_hdr;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   UCSHdr uhdr;
   FreeBuf *p_free;
   int s, more, errnum;
   caddr_t p_ctrl, p_data;
   struct xpollfd xpfd;

   s = GetSocket ( d );

   while ( TRUE )
   {
      more = 0;

      p_free = NULL;

      if ( *flags == RS_HIPRI )
      {
         if ( PendingPri ( d ) )
         {
            if ( ! ( p_free = getFree ( xstDescriptors [ d ].p_ilist ) ) ) 
            {
               ReturnError ( EIO );
            }
         }
      }
      else 
      {
         if ( PendingPri ( d ) )
         {
            if ( ! ( p_free = getFree ( xstDescriptors [ d ].p_ilist ) ) ) 
            {
               ReturnError ( EIO );
            }
         }
         else if ( PendingData ( d ) )
         {
            if ( ! ( p_free = getFree ( xstDescriptors [ d ].p_mlist ) ) ) 
            {
               ReturnError ( EIO );
            }
         }
      }
   
      if ( !p_free )
      {
         if ( BlockIO ( d ) )
         {
            while ( readHdr ( s, &uhdr ) == ERROR )
            {
               if ( MPSerrno != EWOULDBLOCK )
               {
                  return ( ERROR );
               }

               /* #27 - Added poll to return CPU to scheduler to break
                  real-time spin loop                                 */

               xpfd.sd = sd2MPSsd(s);
               xpfd.revents = 0;
               xpfd.events = POLLIN | POLLPRI;

               errnum = MPSpoll(&xpfd, 1, -1);

               if(errnum == MPS_ERROR)
               {
                  if((MPSerrno != EAGAIN) &&
                     (MPSerrno != EINTR ) &&
                     (MPSerrno != EWOULDBLOCK ))
                  {
                     MPSperror("API Internal problem");
                  }

                  continue;
               }


            }
         }
         else
         {
            if ( readHdr ( s, &uhdr ) == ERROR )
            {
               ReturnError ( errno );
            }
         }

         /* Read message into a buffer */
         if ( ( p_free = ( FreeBuf * ) readSave ( s, &uhdr ) ) == NULL )
         {
            return ( ERROR );
         }
      } 
   
      p_hdr = ( UCSHdr * ) ( ( char * ) p_free + sizeof ( FreeBuf ) ); /* #1 */
      p_ctrl = ( caddr_t ) ( ( char * ) p_hdr + UHDRLEN );             /* #1 */
      p_data = ( caddr_t ) ( ( char * ) p_ctrl + p_hdr->csize );       /* #1 */

      if ( *flags == RS_HIPRI && p_hdr->flags != RS_HIPRI )
      {
         /* Must be a new message, add to list */
         addFree ( xstDescriptors [ d ].p_mlist, p_free );
         continue;
      }
   
      switch ( p_hdr->mtype )
      {
         case SPROTO:
            *flags = p_hdr->flags;

            if ( p_hdr->csize && p_ctl )
            {
               /* #15 - added int typecasts in stmt below */
               p_ctl->len = min ( ( int ) p_ctl->maxlen, ( int ) p_hdr->csize );
               memcpy ( p_ctl->buf, p_ctrl, p_ctl->len );
   
               p_hdr->csize -= p_ctl->len;
            }

            if ( p_hdr->csize )
            {
               more |= MORECTL;
            }
          
            /* Fall through */
         case SDATA:
            if ( p_hdr->mtype != SPROTO && p_ctl )
            {
               p_ctl->len = -1;		/* #26 - no control part */
            }

            if ( p_dbuf )
            {
               if ( p_hdr->dsize )
               {
                  /* #15 - added int typecasts in stmt below */
                  p_dbuf->len = min ( (int) p_dbuf->maxlen, (int)p_hdr->dsize );
                  memcpy ( p_dbuf->buf, p_data, p_dbuf->len );
   
                  p_hdr->dsize -= p_dbuf->len;
               }
               else
               {
                  p_dbuf->len = -1;	/* #26 - no data part */
               }
            }

            if ( p_hdr->dsize )
            {
               more |= MOREDATA;
            }

            if ( !more )
            {
               /* Copied complete message */
               if ( p_free )
               {
                  free ( p_free );
               }
         
               return ( 0 );
            }
            else
            {
               /* If control left and some data copied move control part up */
               if ( p_hdr->csize && p_ctl )
                  memcpy ( p_ctrl, ( char * ) ( p_ctrl+p_ctl->len ),
                     p_hdr->csize );

               /* If data left and some data copied move data part up */
               if ( p_hdr->dsize && p_dbuf )
               {
                  memcpy ( ( char * ) ( p_ctrl+p_hdr->csize ), 
                     ( char * ) ( p_data+p_dbuf->len ), p_hdr->dsize );
               }

               headFree ( xstDescriptors [ d ].p_mlist, p_free );
               return ( more );
            }
   
         case SERROR:      /* #7 */
            if ( p_hdr->dsize )
            {   /* Error is first byte of data section. */
               errnum = ( int ) *p_data;
            }
            else
            {
               errnum = MPSERR;   /* Bad data, don't know what error is. */
            }
            free ( p_free );
            ReturnError ( errnum );   /* #16 */

         default:
            ReturnError ( EBADMSG );
      }
   }     
} /* end TCPgetmsg() */


/*<-------------------------------------------------------------------------
| TCPputmsg()
|
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int TCPputmsg ( MPSsd d, struct xstrbuf *p_ctl, struct xstrbuf *p_data, 
                int flags)

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int TCPputmsg(d, p_ctl, p_data, flags)
MPSsd d;
struct xstrbuf *p_ctl, *p_data;
int flags;

#endif   /* ANSI_C */
{
   struct msghdr msg;
   struct iovec iov[3];
   UCSHdr uhdr;
   int nvecs, write_count, s, tlen;

   s = GetSocket ( d );

   /* Ensure that control length doesn't exceed max size */
   if ( p_ctl )
   {
      if ( ( p_ctl->len ) > MaxDblock )
      {
         ReturnError(ERANGE);
      }
   }

   /* Set initial io vector for UCS Header */
   nvecs = 0;
   iov [ nvecs ].iov_base  = ( caddr_t ) &uhdr;
   iov [ nvecs++ ].iov_len = sizeof ( UCSHdr );
  
   /* Set up UCS Header */
   uhdr.mtype = htons ( CPROTO );
   uhdr.csize = 0;
   uhdr.dsize = 0;
   uhdr.flags = htons ( ( ushort ) flags );
 
   /* Init total length to write */
   tlen = sizeof ( UCSHdr );

   /* If control portion specified */ 
   if ( p_ctl )
   {
      if ( ( ushort ) p_ctl->len > 0 )
      {
         uhdr.csize = htons ( ( ushort ) p_ctl->len );

         /* Control part */
         iov [ nvecs ].iov_base  = p_ctl->buf;
         iov [ nvecs++ ].iov_len = p_ctl->len;
   
         /* Update total length to write */
         tlen += p_ctl->len;
      }
   }
   else if ( flags == RS_HIPRI )
   {
      /* if hi-priority, must have control part */
      ReturnError ( EINVAL );
   }
  
   /* If data portion specified */
   if ( p_data )
   {
      /* #23 - If no control part, change type to CDATA  */
      if ( !uhdr.csize )
      {
         uhdr.mtype = htons ( CDATA );
      }

      if ( ( ushort ) p_data->len > 0)
      {
         uhdr.dsize = htons ( ( ushort ) p_data->len );
 
         /* Data part */
         iov [ nvecs ].iov_base  = p_data->buf;
         iov [ nvecs++ ].iov_len = p_data->len;

         /* Update total length to write */
         tlen += p_data->len;
      }
   }

   /* Init msghdr */
   msg.msg_name = NULL;
   msg.msg_namelen = 0;
   msg.msg_iov = ( struct iovec * ) iov;
   msg.msg_iovlen = nvecs;
#if defined (QNX) || defined (LINUX)
   msg.msg_flags = 0;
   msg.msg_control = 0;
   msg.msg_controllen = 0;   /* #28 */
#else
   msg.msg_accrights = NULL;
   msg.msg_accrightslen = 0;
#endif /* QNX || LINUX */
   
   /* See if a portion of the message has already been sent */
   if ( ( write_count = TCPWrCnt ( d ) ) )
   {
      /* reduce total length to write by amount already written */
      tlen -= write_count;

      /* #13 - update io vectors to reflect amount written */
      update_iovecs ( &msg, write_count);
   }

   /* Do while message not completely written (or error) */
   while ( tlen )
   {
#ifdef   WINNT
      if ( ( write_count = sendmsg ( s, &msg, 0 ) ) == SOCKET_ERROR )
      {
         errno = WSA2MPS ( WSAGetLastError ( ) );
#else
      if ( ( write_count = sendmsg ( s, &msg, 0 ) ) == ERROR )
      {
#endif   /* WINNT */

         /* Error */
         ReturnError ( errno );
      }
   
      /* reduce total length to write by amount already written */
      tlen -= write_count;

      /* update total number of bytes written */
      TCPWrCnt ( d ) += write_count;

      /* if message not completely written, update io vectors */
      if ( tlen )
      {
         /* #13 - update io vectors to reflect amount written */
         update_iovecs ( &msg, write_count);
      }
   } /* end while message not completely written */

   /* note no pending data and return */ 
   TCPWrCnt ( d ) = 0;
   return ( 0 );

} /* end TCPputmsg() */


/*<-------------------------------------------------------------------------
| TCPclose()
|
V  
-------------------------------------------------------------------------->*/

/* #14 */
#ifdef   ANSI_C

int TCPclose ( mpssd s )

#else

int TCPclose(s)
mpssd s;

#endif   /* ANSI_C */
{
   UCSHdr uhdr;

   uhdr.mtype = htons ( CCLOSE );
   uhdr.flags = 0;
   uhdr.csize = 0;
   uhdr.dsize = 0;
   
#ifdef   WINNT
/****
   Have to use send() because Window's NT sockets are created as overlapped
   handles, which allow stuff like select() to work properly.  The problem
   though, is that the C-runtime library routines (_write(), _read() )require
   a non-overlapped handle. 
****/
   if ( send ( s, ( char * ) &uhdr, UHDRLEN, 0 ) == SOCKET_ERROR )
#else
   if ( write ( s, ( char * ) &uhdr, UHDRLEN ) == ERROR )
#endif    /* WINNT */
   {
      if ( ( MPSerrno = errno ) == EWOULDBLOCK )
      {
         MPSerrno = EAGAIN;
      }

      return ( ERROR );
   }

#ifdef    WINNT
/***
   send() unlike write() does not wait for the event to actually occur before
   returning.  So, we must ensure the Close got sent all the way through the
   socket before it gets closed.
***/
   if ( shutdown ( s, SD_BOTH ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
      ReturnError( errno );
   }


   if ( closesocket ( s ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( close ( s ) == ERROR )
   {
#endif    /* WINNT */

      if ( ( MPSerrno = errno ) == EWOULDBLOCK )
      {
         MPSerrno = EAGAIN;
      }

      return( ERROR );
   }

   return 0;
} /* end TCPclose() */


/*<-------------------------------------------------------------------------
| TCPpoll()
|
| Emulates the poll() system call used on stream sds.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int TCPpoll ( struct xpollfd *pfds, int nfds, int timeval )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int TCPpoll(pfds, nfds, timeval)
struct xpollfd *pfds;
int nfds, timeval;

#endif   /* ANSI_C */
{
#ifdef SVR4
   struct pollfd tfds[MAXCONN];
   int cnt, i;

   for ( i = 0; i < nfds; i++ )
   {
      if ( pfds[i].events & POLLSYS )
      {
         tfds[i].fd = pfds[i].sd;
         tfds[i].events = pfds[i].events & ~POLLSYS;
      }
      else
      {
         tfds[i].fd = GetSocket(pfds[i].sd);
         tfds[i].events = pfds[i].events;
      }

      tfds[i].revents = 0;
   }

   if ( poll(tfds, nfds, timeval) == ERROR )
   {
      ReturnError(errno);
   }

   for ( i = 0, cnt = 0; i < nfds; i++ )
   {
      pfds[i].revents = 0;

      /* #29 */
      if ( ! ( pfds[i].events & POLLSYS ) )
      {
         if ( PendingData(pfds[i].sd) )
            pfds[i].revents |= POLLIN;

         if ( PendingPri(pfds[i].sd) )
            pfds[i].revents |= POLLPRI;
      }

      pfds[i].revents |= tfds[i].revents;

      if ( pfds[i].revents )
      {
         cnt++;
      }
   }

   return ( cnt );
#endif   /* SVR4 */

#if defined(SVR3) || defined(VMS) || defined(WINNT) || defined(VXWORKS)

#ifdef   VMS
   fd_set         wrset, rdset, exset, *p_wr, *p_rd, *p_ex;
#else
   struct fd_set  wrset, rdset, exset, *p_wr, *p_rd, *p_ex;
#endif

   int wrcnt, rdcnt, excnt, cnt, curfd, maxfd = 0;
   int i, revents;
   int real_cnt;
   struct timeval t, *p_time;

   FD_ZERO ( &wrset );
   FD_ZERO ( &rdset );
   FD_ZERO ( &exset );

   p_wr = p_rd = p_ex = NULL;
   wrcnt = rdcnt = excnt = 0;

   for ( i = 0; i < nfds; i++ )
   {
      /* #24 */
      pfds [ i ].revents = 0;

      if ( !pfds [ i ].events )
      {
         continue;
      }

      if ( pfds [ i ].events & POLLSYS )
      {
         curfd = pfds [ i ].sd;
      }
      else
      {
         curfd = GetSocket ( pfds [ i ].sd);
      }

      if ( curfd > maxfd )
      {
         maxfd = curfd;
      }

      if ( pfds [ i ].events & ( POLLPRI | POLLIN ) )
      {
         FD_SET ( ( unsigned int ) curfd, &rdset ); 
         rdcnt++;
      }

      if ( pfds [ i ].events & POLLOUT )
      {
         FD_SET ( ( unsigned int ) curfd, &wrset ); 
         wrcnt++;
      }
   }

   p_time = ( struct timeval * ) &t;

   switch ( timeval )
   {
      case -1:
         p_time = NULL;
         break;

      case 0:
         p_time->tv_sec = p_time->tv_usec = 0;
         break;

      default:
         if ( timeval < 0 )
         {
            ReturnError ( EINVAL );
         }

         /* Get seconds */
         p_time->tv_sec = timeval / 1000;

         /* Get microseconds from milliseconds */
         p_time->tv_usec = ( timeval % 1000 ) * 1000;
         break;
   }

   if ( rdcnt )
   {
      p_rd = ( fd_set * ) &rdset;
   }

   if ( wrcnt )
   {
      p_wr = ( fd_set * ) &wrset;
   }

#ifdef    WINNT
   if ( ( cnt = select ( maxfd+1, p_rd, p_wr, p_ex, p_time ) ) ==
      SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( ( cnt = select ( maxfd+1, p_rd, p_wr, p_ex, p_time ) ) == ERROR )
   {
#endif    /* WINNT */
      ReturnError ( errno );
   }

   if ( cnt )
   {
      if ( p_rd )
      {
         for ( i = 0; i < nfds; i++ )
         {
            if ( pfds [ i ].events & POLLSYS )
            {
               curfd = pfds [ i ].sd;
            }
            else
            {
               curfd = GetSocket ( pfds [ i ].sd );
            }

            if ( FD_ISSET ( curfd, p_rd ) )
            {
               pfds [ i ].revents |= POLLIN;
            }
         }
      }

      if ( p_wr )
      {
         for ( i = 0; i < nfds; i++ )
         {
            if ( pfds [ i ].events & 0x80 )
            {
               curfd = pfds [ i ].sd;
            }
            else
            {
               curfd = GetSocket ( pfds [ i ].sd );
            }

            if ( FD_ISSET ( curfd, p_wr ) )
            {
               pfds [ i ].revents |= POLLOUT;
            }
         }
      }

      if ( p_ex )
      {
         for ( i = 0; i < nfds; i++ )
         {
            if ( pfds [ i ].events & 0x80 )
            {
               curfd = pfds [ i ].sd;
            }
            else
            {
               curfd = GetSocket ( pfds [ i ].sd );
            }

            if ( FD_ISSET ( curfd, p_wr ) )
            {
               pfds [ i ].revents |= POLLIN;
            }
         }
      }

      /* #25 */
      /* Count up how many descriptors have the event field set, because
         that is what we really have to return (not the total number of
         events set). */
      real_cnt = 0;
      for ( i = 0; i < nfds; i++ )
      {
         if ( pfds [ i ].revents )
         {
            real_cnt++;
         }
      }

      cnt = real_cnt;
   }

   for ( i = 0; i < nfds; i++ )
   {
      revents = 0;

      /* #29 */
      if ( ! ( pfds[i].events & POLLSYS ) )
      {
         if ( PendingData ( pfds [ i ].sd ) )
         {
            revents |= POLLIN;
         }

         if ( PendingPri ( pfds [ i ].sd ) )
         {
            revents |= POLLPRI;
         }
      }

      /* If there is a pending data or apriority message on the sd's holding
         queue, but no new data waiting on the socket, we must increment count
         and set the revents field since it was not done after the select()
         call. */
      if ( !pfds [ i ].revents && revents )
      {
         pfds [ i ].revents = revents;
         cnt++;
      }
   }

   return ( cnt );
#endif   /* SVR3 || VMS || WINNT */
} /* end TCPpoll() */

/*<-------------------------------------------------------------------------
| update_iovecs()
|
|  #13 - added this local routine to provide common function that can
|        be used to update io vectors (used by TCPwrite, TCPioctl, TCPputmsg)
V  
-------------------------------------------------------------------------->*/

#ifdef ANSI_C
void update_iovecs ( struct msghdr *msg, int count )
#else
void update_iovecs ( msg, count )
struct msghdr *msg;      /* address of msg header */
int count;         /* count of bytes written */
#endif /* ANSI_C */
{
   struct iovec *p_iov;
   int nvecs, save_count, total_bytes;

   save_count = count;      /* save original count in case of error */
   p_iov = msg->msg_iov;

   /* update io vectors to reflect number of bytes written */
   for ( nvecs = 0, total_bytes = 0; nvecs < msg->msg_iovlen; nvecs++ )
   {
      total_bytes += p_iov->iov_len;

      /* exit loop if we've accounted for all bytes written */
      if ( !count )
      {
         break;
      }
   
      /* see if we have finished this part of message */
      if ( ( int ) p_iov->iov_len <= count )   /* #18 */
      {
         /* this portion was completed. reduce count remaining
            and advance to next io vector */ 
         count -= p_iov->iov_len;
         p_iov++;
      }
      else
      {
         /* we did not complete this portion of message - exit loop */
         break;
      }
   }
    
   /* if we didn't account for all bytes written, something is wrong */
   if ( nvecs == msg->msg_iovlen )
   {
      printf( "\n\nMPStcp - update_iovecs: Exiting due to fatal error\n");
      printf( "Bytes written = %d exceeded bytes expected=%d\n\n\n", 
               save_count, total_bytes);
      exit (-1);
   }
   else
   {
      /* reduce bytes remaining for this part of message & adjust bufr addr */
      p_iov->iov_len -= count;
      p_iov->iov_base = (char *)p_iov->iov_base + count;   /* #6 */

      /* reset vector address to current vector and reduce #vectors remaining */
      msg->msg_iov = p_iov;
      msg->msg_iovlen -= nvecs;
   }

   return;
} /* end update_iovecs() */

