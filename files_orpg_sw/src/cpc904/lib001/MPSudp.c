/*   @(#) MPSudp.c 00/01/11 Version 1.26   */
 
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
                    for DECUX/Alpha compatibility.   Added required calls 
                    to "ntohl" where necessary.
2.  14-MAY-96  mpb  Put pragma's in for 64 to 32 bit conversion (for esca).
3.  23-MAY-96  mpb  Added prototyping of functions if ANSI_C is #ifdef'd.
4.  24-MAY-96  mpb  Two cases in UDPioctl() where it is just returning with
                    no value.  Changed the one in the case "X_SETIOTYPE" to
                    the return value of SetXstIO() which it calls, and the
                    one in the case "X_GETIOTYPE" to 0.
5.  23-MAY-96  mpb  Change the reference of pointers from "uint" to "ulong".
6.  07-JUL-96  mpb  Check for SERROR returned in UDPgetmsg().
7.  12-MAY-97  mpb  Make sure all malloc()'d memory is free()'d before
                    returning.
8.  30-OCT-97  lmm  Added int typecasts to avoid Sunpro compiler warnings
9.  10-NOV-97  lmm  Call ReturnError to ensure user gets system err code
10. 14-NOV-96  mpb  setIOType needs to know if the sd is LOCAL or not (for NT).
11. 15-DEC-97  mpb  QNX support.
12. 05-AUG-98  mpb  UDPioctl()'s second parameter is of type int, not MPSsd.
13. 16-SEP-98  mpb  See #8 in MPSapi.h.
14. 04-JAN-00  lmm  If no control part in putmsg, change type to CDATA.
*/

#include "MPSinclude.h"

#define RecvFrom(p1,p2,p3,p4,p5,p6) \
   if ( recvfrom(p1,p2,p3,p4,(struct sockaddr *)p5,p6) == ERROR ) \
      ReturnError(errno);


/** Local routines prototypes ... **/
#ifdef   ANSI_C

#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

static int   rcvMesg ( int, struct msghdr*, int );
static int   sndMesg ( int, struct msghdr*, int );

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */

#else

static int   rcvMesg ( );
static int   sndMesg ( );

#endif   /* ANSI_C */

/*<-------------------------------------------------------------------------
| 
|         UDP MPS Library Routines
V  
-------------------------------------------------------------------------->*/

/*<-------------------------------------------------------------------------
| UDPopen()
|
| Opens a UDP connection to an MPS server.  Currently a bind is done, but
| no connect, so we need to save the sin struct for future writes.  A goto
| is here for broadcasting, since many broadcast messages could be received
| before the OPENACK is returned (an error in TCP).
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int UDPopen ( OpenRequest *p_open, struct hostent *hp, struct servent *sp )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int UDPopen(p_open, hp, sp)
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
   OpenData *p_odata;
   UCSHdr *p_hdr;
   struct sockaddr_in *sa;
   struct xiocblk *p_ioc;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   struct sockaddr_in isa;
   struct xpollfd pfd;
   char data[2048];
   int sc, ii, size, errnum;
#ifdef UDP_SHAREPORT
   int reuseval = 1;
#endif

   if ( !(sa = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in))) )
   {
      ReturnError(ENOBUFS);
   }

   memset(sa, 0, sizeof(struct sockaddr_in));

#ifdef   WINNT
   if ( (sc = socket(AF_INET, SOCK_DGRAM, 0)) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( (sc = socket(AF_INET, SOCK_DGRAM, 0)) == ERROR )
   {
#endif   /* WINNT */

      /* #7 */
      free ( sa );

      ReturnError(ESOCKET);
   }

#ifdef   UDP_SHAREPORT

#ifdef   WINNT
   if ( setsockopt(sc, SOL_SOCKET, SO_REUSEPORT, (char *)&reuseval,
      sizeof (reuseval)) == SOCKET_ERROR )
#else
   if ( setsockopt(sc, SOL_SOCKET, SO_REUSEPORT, (char *)&reuseval,
      sizeof (reuseval)) == ERROR )
#endif
   {
      /* #7 */
      free ( sa );

      ReturnError(ESOCKET);
   }

#endif

   sa->sin_family = AF_INET;
   sa->sin_addr.s_addr = INADDR_ANY;
   sa->sin_port = p_open->port;

#ifdef   WINNT
   if ( bind ( sc, ( struct sockaddr * ) sa,
      sizeof ( struct sockaddr_in ) ) == SOCKET_ERROR )
#else
   if ( bind ( sc, ( struct sockaddr * ) sa,
      sizeof ( struct sockaddr_in ) ) == ERROR )
#endif   /* WINNT */
   {
      /* #7 */
      free ( sa );

      ReturnError( ESOCKET );
   }

#ifdef VXWORKS
   sa->sin_addr.s_addr = hp->h_addr;
#else
   memcpy((char *)&sa->sin_addr.s_addr, (char *)hp->h_addr, hp->h_length);
#endif /* VXWORKS */
   
   sa->sin_family = AF_INET;
   sa->sin_port = sp->s_port;

   p_hdr = (UCSHdr *)&oreq.uhdr;

   p_hdr->mtype = htons(COPEN);
   p_hdr->flags = 0;
   p_hdr->csize = 0;
   p_hdr->dsize = htons(sizeof(OpenData));

   p_odata = &oreq.odata;

   p_odata->slotnum = htonl(p_open->ctlrNumber);
   strcpy(p_odata->protocol, p_open->protoName);
   p_odata->dev = htonl(p_open->dev);
   p_odata->flags = htonl(p_open->flags);

   /* Send Streams message */
#ifdef   WINNT
   if ( sendto ( sc, ( char * ) p_hdr, sizeof ( OpenReq ), 0, 
      ( struct sockaddr * ) sa, sizeof ( struct sockaddr_in ) ) == ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( sendto ( sc, ( char * ) p_hdr, sizeof ( OpenReq ), 0, 
      ( struct sockaddr * ) sa, sizeof ( struct sockaddr_in ) ) == ERROR )
   {
#endif   /* WINNT */

      /* #7 */
      free ( sa );

      ReturnError ( errno );
   }

top:

   memset(&isa, 0, sizeof(struct sockaddr_in));

   size = sizeof(isa);

   pfd.events = POLLIN | POLLSYS;
   pfd.sd = sc;
   pfd.revents = 0;

   if ( UDPpoll(&pfd, 1, p_open->openTimeOut*1000) )
   {
      if ( pfd.revents & POLLIN )
      {
         /* Save the iotype and set to Non blocking */
         setIOType(sc, (uint)NONBLOCK, MPS_UDP);   /* #10 */

#ifdef   WINNT
         if ( recvfrom ( sc, data, 2048, 0, 
            ( struct sockaddr * ) &isa, &size ) == ERROR )
         {
            errno = WSA2MPS ( WSAGetLastError ( ) );
            closesocket ( sc );
#else
         if ( recvfrom ( sc, data, 2048, 0, 
            ( struct sockaddr * ) &isa, &size ) == ERROR )
         {
            close( sc );
#endif   /* WINNT */

            /* #7 */
            free ( sa );

            ReturnError ( errno );
         }

         /* Restore iotype */
         setIOType(sc, (uint)BLOCK, MPS_UDP);   /* #10 */
      }
      else
      {
#ifdef   WINNT
         closesocket ( sc );
#else
         close ( sc );
#endif   /* WINNT */

         /* #7 */
         free ( sa );

         ReturnError ( EOPENTIMEDOUT );
      }
   }
   else
   {
#ifdef   WINNT
      closesocket ( sc );
#else
      close(sc);
#endif   /* WINNT */

      /* #7 */
      free ( sa );

      ReturnError ( EOPENTIMEDOUT );
   }

   p_hdr = (UCSHdr *)data;

   switch ( ntohs(p_hdr->mtype) )
   {
      case SOPENACK:
         /* This should never fail, but .. */
         if ( (ii = addDesc(sc, MPS_UDP)) < 0 )
         {
            MPSclose(sc);

            /* #7 */
               free ( sa );

            ReturnError ( ENOBUFS );
         }
         else
         {
            p_odata = ( OpenData * )( data + ( UHDRLEN + p_hdr->csize ) ); 
            SetStrId(ii, ntohl(p_odata->slotnum));

            if ( !MaxDblock )
            {
               MaxDblock = ntohl(p_odata->dev);
            }

            xstDescriptors[ii].sin = sa;
            return(ii);
         }
     
      case SIOCNAK:
         if ( ntohs(p_hdr->csize) )
         {
            p_ioc = (struct xiocblk *)&data[UHDRLEN];
            errnum = ntohl(p_ioc->ioc_error);   /* #9 */
         }
         else
            errnum = EIO;         /* #9 */

         if ( p_hdr->dsize )
            memcpy(p_open, &data[UHDRLEN + p_hdr->csize], sizeof(OpenRequest));

         /* Send a CCLOSE to server */

         p_hdr->mtype = htons(CCLOSE);
         p_hdr->flags = p_hdr->csize = p_hdr->dsize = 0;

         /* If error can't do anything about it */
         sendto(sc, (char *)p_hdr, UHDRLEN, 0, 
           (struct sockaddr *)sa, sizeof(struct sockaddr_in));

#ifdef   WINNT
         closesocket ( sc );
#else
         close(sc);
#endif   /* WINNT */

         /* #7 */
         free ( sa );

         ReturnError(errnum);   /* #9 */

      case SERROR:
         /* Error code is in first byte of data */

         /* #7 */
         free ( sa );

         ReturnError ( data [ UHDRLEN ] );

      default:
         goto top;
   }
} /* end UDPopen() */


/*<-------------------------------------------------------------------------
| UDPwrite()
|
| Writes an MPS message to an MPS server over a UDP connection.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int UDPwrite ( int d, caddr_t p_data, int len )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int UDPwrite(d, p_data, len)
int d;
caddr_t p_data;
int len;

#endif   /* ANSI_C */
{
   struct msghdr msg;
   struct iovec iov[2];
   UCSHdr uhdr;
   int s, ii;

   s = GetSocket(d);

   uhdr.mtype = htons(CDATA);
   uhdr.flags = 0;
   uhdr.csize = 0;
   uhdr.dsize = htons ( ( ushort ) len );

   /* UCS Header */
   iov[0].iov_base = (caddr_t)&uhdr;
   iov[0].iov_len = sizeof(UCSHdr);

   /* Input data */
   iov[1].iov_base = p_data;
   iov[1].iov_len = len;

   /* Init msghdr */
#ifdef DGUX
   msg.msg_name = (struct sockaddr *)xstDescriptors[d].sin;
#else
   msg.msg_name = (caddr_t)xstDescriptors[d].sin;
#endif
   msg.msg_namelen = sizeof(struct sockaddr_in);
   msg.msg_iov = (struct iovec *)iov;
   msg.msg_iovlen = 2;
#if defined (QNX) || defined (LINUX)
   msg.msg_flags = 0;
#else
   msg.msg_accrights = NULL;
   msg.msg_accrightslen = 0;
#endif /* QNX || LINUX */

   if ( (ii = sndMesg(s, &msg, 0)) == ERROR )
   {
      ReturnError(errno);
   }

   return(ii - UHDRLEN);
} /* end UDPwrite() */




/*<-------------------------------------------------------------------------
| UDPgetmsg()
|
| UDP is message-discard mode.  Must read all data in message so if user's
| buffers are not big enough for either the control or data part, the 
| leftover data is lost.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int UDPgetmsg ( MPSsd d, struct xstrbuf *p_ctl, struct xstrbuf *p_dbuf, 
                int *flags )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int UDPgetmsg(d, p_ctl, p_dbuf, flags)
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
   FreeBuf *p_free;
   caddr_t p_buf = NULL;
   caddr_t p_ctrl, p_data;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   int s, more, errnum;
   struct msghdr msg;
   struct iovec iov[2];

   s = GetSocket(d);

   while ( TRUE )
   {
      more = 0;

      p_free = NULL;

      if ( *flags == RS_HIPRI )
      {
         if ( PendingPri(d) )
         {
            if ( !(p_free = getFree(xstDescriptors[d].p_ilist)) ) 
               ReturnError(EIO);
         }
      }
      else 
      {
         if ( PendingPri(d) )
         {
            if ( !(p_free = getFree(xstDescriptors[d].p_ilist)) ) 
               ReturnError(EIO);
         }
    else if ( PendingData(d) )
         {
            if ( !(p_free = getFree(xstDescriptors[d].p_mlist)) ) 
               ReturnError(EIO);
         }
      }
   
      if ( !p_free )
      {
         p_buf = (caddr_t) malloc(sizeof(FreeBuf)+UHDRLEN+   /* #1 */
            p_ctl->maxlen + p_dbuf->maxlen);

         /* Users buffer */
         iov[0].iov_base = (caddr_t)(p_buf + sizeof(FreeBuf));
         iov[0].iov_len = UHDRLEN+p_ctl->maxlen+p_dbuf->maxlen;
               
         /* Init msghdr */
#ifdef DGUX
         msg.msg_name = (struct sockaddr *)xstDescriptors[d].sin;
#else
         msg.msg_name = (caddr_t)xstDescriptors[d].sin;
#endif
         msg.msg_namelen = sizeof(struct sockaddr_in);
         msg.msg_iov = (struct iovec *)iov;
         msg.msg_iovlen = 1;
#if defined (QNX) || defined (LINUX)
         msg.msg_flags = 0;
#else
         msg.msg_accrights = NULL;
         msg.msg_accrightslen = 0;
#endif /* QNX || LINUX */
         
         if ( rcvMesg(s, &msg, 0) == ERROR )
         {
            /* #7 */
            if ( p_buf )
            {
               free ( p_buf );
            }

            ReturnError(errno);
         }

         p_free = (FreeBuf *)p_buf;
      } 
   
      p_hdr = (UCSHdr *)((char *)p_free + sizeof(FreeBuf));   /* #1 */
      p_ctrl = (caddr_t)((char *)p_hdr + UHDRLEN);      /* #1 */
      p_data = (caddr_t)((char *)p_ctrl + p_hdr->csize);   /* #1 */

      if ( *flags == RS_HIPRI && p_hdr->flags != RS_HIPRI )
      {
         /* Must be a new message, add to list */
         addFree(xstDescriptors[d].p_mlist, p_free);
         continue;
      }
   
      switch ( p_hdr->mtype )
      {
         case SPROTO:
            *flags = p_hdr->flags;

            p_ctl->len = min( (int)p_ctl->maxlen, (int)p_hdr->csize); /* #8 */
            memcpy(p_ctl->buf, p_ctrl, p_ctl->len);
   
            if ( (p_hdr->csize -= p_ctl->len) )
               more = MORECTL;
   
            /* Fall through */
         case SDATA:
            if ( p_hdr->mtype != SPROTO )
               p_ctl->len = 0;

            p_dbuf->len = min( (int)p_dbuf->maxlen, (int)p_hdr->dsize); /* #8 */
            memcpy(p_dbuf->buf, p_data, p_dbuf->len);
   
            if ( (p_hdr->dsize -= p_dbuf->len) )
               more |= MOREDATA;
             
            if ( !more )
            {
               /* Copied complete message */
               if ( p_free )
               {
                  free(p_free);
               }

               return(0);
            }
            else
            {
               /* Move control part up */
               if ( p_hdr->csize)
                  memcpy(p_ctrl, (char *)(p_ctrl+p_ctl->len), p_hdr->csize);
          
               /* Move data part up */
               if ( p_hdr->dsize )
               {
                  memcpy((char *)(p_ctrl+p_hdr->csize),
                    (char *)(p_data+p_dbuf->len), p_hdr->dsize);
               }
   
               headFree(xstDescriptors[d].p_mlist, p_free);
               return(more);
            }

         case SERROR:      /* #6 */
            if ( p_hdr->dsize )
            {   /* Error is first byte of data section. */
                errnum = ( int ) *p_data;   /* #9 */
            }
            else
            {
               errnum = MPSERR;   /* #9 - Bad data, unknown error  */
            }

            if ( p_free )
            {
               free(p_free);
            }
            ReturnError(errnum);   /* #9 */
   
         default:
            /* #7 */
            if ( p_free )
            {
               free(p_free);
            }
            ReturnError ( EBADMSG );
      }
   }     
} /* end UDPgetmsg() */


/*<-------------------------------------------------------------------------
| UDPputmsg()
|
| Sends a PROTO message downstream.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int UDPputmsg ( MPSsd d, struct xstrbuf *p_ctl, struct xstrbuf *p_data, 
                int flags )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int UDPputmsg(d, p_ctl, p_data, flags)
MPSsd d;
struct xstrbuf *p_ctl, *p_data;
int flags;

#endif   /* ANSI_C */
{
   struct msghdr msg;
   struct iovec iov[3];
   UCSHdr uhdr;
   int i, s, tlen;

   s = GetSocket(d);

   /* Get total length for range check */
   if ( p_ctl )
   {
      if ( (tlen = p_ctl->len) > MaxDblock )
      {
         ReturnError(ERANGE);
      }
   }

   if ( p_data )
      tlen += p_data->len;

   /* UCS Header */
   iov[0].iov_base = (caddr_t)&uhdr;
   iov[0].iov_len = sizeof(UCSHdr);

   uhdr.mtype = CPROTO;
   uhdr.csize = 0;
   uhdr.dsize = 0;
   uhdr.flags = flags;

   i = 1;
   if ( p_ctl )
   {
      if ( p_ctl->len )
      {
         uhdr.csize = p_ctl->len;

         /* Control part */
         iov[i].iov_base = p_ctl->buf;
         iov[i++].iov_len = p_ctl->len;
      }
   }
   else if ( flags == RS_HIPRI)
      ReturnError(EINVAL);

   if ( p_data )
   {
      /* #14 - If no control part, change type to CDATA  */
      if ( !uhdr.csize )
      {
         uhdr.mtype = htons ( CDATA );
      }

      if ( p_data->len )
      {
         uhdr.dsize = p_data->len;

         /* Control part */
         iov[i].iov_base = p_data->buf;
         iov[i++].iov_len = p_data->len;
      }
   }

   /* Init msghdr */
#ifdef DGUX
   msg.msg_name = (struct sockaddr *)xstDescriptors[d].sin;
#else
   msg.msg_name = (caddr_t)xstDescriptors[d].sin;
#endif
   msg.msg_namelen = sizeof(struct sockaddr_in);
   msg.msg_iov = (struct iovec *)iov;
   msg.msg_iovlen = i;
#if defined (QNX) || defined (LINUX)
   msg.msg_flags = 0;
#else
   msg.msg_accrights = NULL;
   msg.msg_accrightslen = 0;
#endif /* QNX || LINUX */

   if ( sndMesg(s, &msg, 0) == ERROR )
      ReturnError(errno);

   return(0);
} /* end UDPputmsg() */


/*<-------------------------------------------------------------------------
| UDPclose()
|
| Sends an CLOSE, then does a system close.  Applications should catch 
| control-c and issue this call.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C

int UDPclose ( int d )

#else

int UDPclose(d)
int d;

#endif   /* ANSI_C */
{
   UCSHdr uhdr;
   int i, s;

   s = GetSocket(d);

   uhdr.mtype = htons(CCLOSE);
   uhdr.flags = uhdr.csize = uhdr.dsize = 0;

   /* Get ACK/NAK */
#ifdef   WINNT
   if ( (i = sendto(s, (char *)&uhdr, UHDRLEN, 0, 
      (struct sockaddr *)xstDescriptors[d].sin, 
      sizeof(struct sockaddr_in))) == ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( (i = sendto(s, (char *)&uhdr, UHDRLEN, 0, 
      (struct sockaddr *)xstDescriptors[d].sin, 
      sizeof(struct sockaddr_in))) == ERROR )
   {
#endif   /* WINNT */

      ReturnError ( errno );
   }

#ifdef   WINNT
   closesocket ( s );
#else
   close ( s );
#endif   /* WINNT */

   return ( 0 );
} /* end UDPclose() */


/*<-------------------------------------------------------------------------
| UDPioctl()
|
| A blocking ioctl.
|
| ACK Data is currently received into a static buffer, should be received
| into the user's buffer on input.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int UDPioctl ( MPSsd d, int command, caddr_t p_arg )  /* #12 */

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int UDPioctl(d, command, p_arg)
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
   ulong *p_muxid;         /* #1 */
   UCSHdr *p_hdr;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   UCSHdr uhdr;
   struct xstrioctl striocb;
   struct sockaddr_in isa;
   struct msghdr msg;
   struct iovec iov[2];
   char buf[1024];
   int ret, s, size, errnum;
   ulong muxid;            /* #1 */
   uchar flush;

   s = GetSocket(d);

   if ( PendingIoc(d) )
      ReturnError(EINVAL);

   uhdr.mtype = CIOCTL;
   uhdr.csize = 0;
   uhdr.dsize = 0;

   switch ( command )
   {
      case X_POP:
    /* No argument */

         uhdr.flags = X_POP;
         p_iocb = (struct xstrioctl *)&striocb;
         p_iocb->ic_len = 0;
         p_iocb->ic_dp = NULL;
         break;
    
      case X_PUSH:
    /* p_arg is the name of the module to PUSH */

         p_iocb = (struct xstrioctl *)&striocb;
         uhdr.flags = X_PUSH;
         uhdr.dsize = p_iocb->ic_len = strlen(p_arg) + 1;
         p_iocb->ic_dp = p_arg;
         break;
    
      case X_LINK:
         /* p_arg is the file descriptor to be linked with this stream */

         p_iocb = (struct xstrioctl *)&striocb;
         uhdr.flags = X_LINK;
         uhdr.dsize = p_iocb->ic_len = 4;

         muxid = GetStrId((ulong)p_arg);   /* #1 */
         muxid = htonl ( muxid );       /* #1 */
         p_iocb->ic_dp = (caddr_t)&muxid;
         break;

      case X_UNLINK:
         /* p_arg is the muxid to unlink or -1 */

         p_iocb = (struct xstrioctl *)&striocb;
         uhdr.flags = X_UNLINK;
         uhdr.dsize = p_iocb->ic_len = 4;

         muxid = (ulong)p_arg;         /* #1 */
         muxid = htonl ( muxid );       /* #1 */
         p_iocb->ic_dp = (caddr_t)&muxid;
         break;

      case X_STR:
         p_iocb = (struct xstrioctl *)p_arg;
         uhdr.mtype = CIOCTL;
         uhdr.flags = p_iocb->ic_cmd;
         uhdr.csize = 0;
         uhdr.dsize = p_iocb->ic_len;
         break;

      case X_SETIOTYPE:
#ifdef   WINNT
         SetXstIO(d, (uchar)p_arg);   /* #5 */
#else
         SetXstIO(d, (ulong)p_arg);   /* #5 */
#endif   /* WINNT */
         /* #4, #5, #10 */
         return ( setIOType ( s, ( ulong ) p_arg ), MPS_UDP );

      case X_GETIOTYPE:
         /* BlockIO returns TRUE if Blocking, but BLOCK = 0 */
         *p_arg = (char)!BlockIO(d);
         return 0;   /* #4 */

      case X_FLUSH:
         /* p_arg FLUSHR, FLUSHW or FLUSHRW */

         p_iocb = (struct xstrioctl *)&striocb;
         uhdr.flags = X_FLUSH;
         uhdr.csize = p_iocb->ic_len = 1;

         flush = (uchar)((ulong)p_arg & 0xff);   /* #5 */
         p_iocb->ic_dp = (caddr_t)&flush;
         break;
   }
      
   /* 
    * Create IO Vectors and send message via IP.
    */

   /* UCS Header */
   iov[0].iov_base = (caddr_t)&uhdr;
   iov[0].iov_len = UHDRLEN;
      
   /* Input data */
   iov[1].iov_base = p_iocb->ic_dp;
   iov[1].iov_len = p_iocb->ic_len;
      
   /* Init msghdr */
#ifdef DGUX
   msg.msg_name = (struct sockaddr *)xstDescriptors[d].sin;
#else
   msg.msg_name = (caddr_t)xstDescriptors[d].sin;
#endif
   msg.msg_namelen = sizeof(struct sockaddr_in);
   msg.msg_iov = (struct iovec *)iov;
   msg.msg_iovlen = 2;
#if defined (QNX) || defined (LINUX)
   msg.msg_flags = 0;
#else
   msg.msg_accrights = NULL;
   msg.msg_accrightslen = 0;
#endif /* QNX || LINUX */
      
   if ( (ret = sndMesg(s, &msg, 0)) == ERROR )
      ReturnError(errno);
      
   if ( command == X_FLUSH )
      return(0);

   SetPendIoc(d);

   while ( TRUE )
   {
      size = sizeof(isa);

      /* Get ACK/NAK */
      RecvFrom(s, buf, 1024, 0, &isa, &size);

      p_hdr = (UCSHdr *)buf;

      switch ( p_hdr->mtype )
      {
         case SIOCACK:
            if ( command == X_LINK )
            {
               p_muxid = (ulong *)&buf[UHDRLEN];      /* #1 */
               muxid = *p_muxid;
            }
            else
               muxid = 0;

            ClrPendIoc(d);
            return(muxid);
       
         case SIOCNAK:
            ClrPendIoc(d);
            if ( p_hdr->csize )
            {
               p_ioc = (struct xiocblk *)&buf[UHDRLEN];
               errnum = ntohl ( p_ioc->ioc_error );      /* #1, #9 */
            }
            else
               errnum = EIO;      /* #9 */

            if ( p_hdr->dsize )
               memcpy(p_iocb->ic_dp, &buf[UHDRLEN+p_hdr->csize],
                  p_iocb->ic_len);

            ReturnError(errnum);   /* #9 */

         case SERROR:
            /* Error code is in first byte of data */
            ReturnError ( buf [ UHDRLEN ] );

         default:
            break;
      }
   }
} /* end UDPioctl() */


/*<-------------------------------------------------------------------------
| UDPpoll()
|
| Both an SVR4 and SVR3 model, SVR4 allows a poll() of a non-STREAMS 
| descriptor, SVR3 does not so a select is used instead.  
| Timeval is milliseconds.
|
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int UDPpoll ( struct xpollfd *pfds, int nfds, int timeval )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int UDPpoll(pfds, nfds, timeval)
struct xpollfd *pfds;
int nfds, timeval;

#endif   /* ANSI_C */
{
#ifdef SVR4
   struct pollfd tfds[10];
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
   }

   if ( (cnt = poll(tfds, nfds, timeval)) == ERROR )
      ReturnError(errno);

   for ( i = 0; i < nfds; i++ )
      pfds[i].revents = tfds[i].revents;

   return(cnt);
#endif

#if defined ( SVR3 ) || defined ( WINNT )
#ifdef   DECUX_32
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */
   fd_set *p_wr, *p_rd, *p_ex;
   struct timeval t, *p_time;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
   fd_set wrset, rdset, exset;
   int wrcnt, rdcnt, excnt, cnt, curfd, maxfd = 0;
   int i;

   FD_ZERO(&wrset);
   FD_ZERO(&rdset);
   FD_ZERO(&exset);

   p_wr = p_rd = p_ex = NULL;
   wrcnt = rdcnt = excnt = 0;

   for ( i = 0; i < nfds; i++ )
   {
      if ( !pfds[i].events )
         continue;

      if ( pfds[i].events & POLLSYS )
         curfd = pfds[i].sd;
      else
         curfd = GetSocket(pfds[i].sd);

      if ( curfd > maxfd )
         maxfd = curfd;

      if ( pfds[i].events & (POLLPRI|POLLIN) )
      {
         FD_SET( ( unsigned int ) curfd, &rdset); 
         rdcnt++;
      }

      if ( pfds[i].events & POLLOUT )
      {
         FD_SET( ( unsigned int ) curfd, &wrset); 
         wrcnt++;
      }
   }

   p_time = (struct timeval *)&t;

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
            ReturnError(EINVAL);

         /* Get seconds */
         p_time->tv_sec = timeval/1000;

         /* Get microseconds from milliseconds */
         p_time->tv_usec = (timeval % 1000) * 1000;
   }

   if ( rdcnt )
      p_rd = (fd_set *)&rdset;

   if ( wrcnt )
      p_wr = (fd_set *)&wrset;

#ifdef   WINNT
   if ( ( cnt = select ( maxfd + 1, p_rd, p_wr, p_ex, p_time ) ) ==
      SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( ( cnt = select ( maxfd + 1, p_rd, p_wr, p_ex, p_time ) ) == ERROR )
   {
#endif   /* WINNT */

      ReturnError ( errno );
   }

   if ( cnt )
   {
      if ( p_rd )
      {
         for ( i = 0; i < nfds; i++ )
         {
            if ( pfds[i].events & POLLSYS )
               curfd = pfds[i].sd;
            else
               curfd = GetSocket(pfds[i].sd);

            if ( FD_ISSET(curfd, p_rd) )
               pfds[i].revents |= POLLIN;
         }
      }

      if ( p_wr )
      {
         for ( i = 0; i < nfds; i++ )
         {
            if ( pfds[i].events & 0x80 )
               curfd = pfds[i].sd;
            else
               curfd = GetSocket(pfds[i].sd);

            if ( FD_ISSET(curfd, p_wr) )
               pfds[i].revents |= POLLOUT;
         }
      }

      if ( p_ex )
      {
         for ( i = 0; i < nfds; i++ )
         {
            if ( pfds[i].events & 0x80 )
               curfd = pfds[i].sd;
            else
               curfd = GetSocket(pfds[i].sd);

            if ( FD_ISSET(curfd, p_wr) )
               pfds[i].revents |= POLLIN;
         }
      }
   }

   return(nfds);
#endif /* defined ( SVR3 ) || defined ( WINNT ) */
} /* end UDPpoll() */


/*<-------------------------------------------------------------------------
| UDPread()
|
| UDP is read discard mode.  Once any part of a dgram is read the entire 
| thing is trashed.  If a PROTO message is read, only the amount of 
| control and data information that can fit into the user's buffer is read,
| any remaining data is lost.  This is also true if the user's buffer is to
| small to receive the incoming DATA message.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int UDPread ( int d, caddr_t p_buf, int len )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int UDPread(d, p_buf, len)
int d;
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
   int s, rval, dlen, errnum;
   struct msghdr msg;
   struct iovec iov[2];

   s = GetSocket(d);

   if ( PendingPri(d) )
      ReturnError(EBADMSG);

   if ( PendingData(d) )
   {
      if ( !(p_free = peekFree(xstDescriptors[d].p_mlist)) )
         ReturnError(EIO);

      p_hdr = (UCSHdr *)((char *)p_free + sizeof(FreeBuf));   /* #1 */

      if ( p_hdr->mtype == SDATA )
      {
         if ( !(p_free = getFree(xstDescriptors[d].p_mlist)) ||
               (p_hdr != (UCSHdr *)((char *)p_free +
               sizeof(FreeBuf))) )/* #1 */
            ReturnError(EIO);

         p_data = (caddr_t)((char *)p_hdr + UHDRLEN);      /* #1 */

         rval = min(len, ( int ) p_hdr->dsize);         /* #8 */

         memcpy(p_buf, p_data, rval);
          
         if ( rval == p_hdr->dsize )
         {
            /* Copied complete message */
            if ( p_free )
               free(p_free);

            return(p_hdr->dsize);
         }
         else
         {
            /* Some data left, save it */
            p_hdr->dsize -= rval;
            /* Move data up */
            memcpy(p_data, (char *)(p_data+rval), rval);
 
            headFree(xstDescriptors[d].p_mlist, p_free);
            return(rval);
         }
      }
      else
         ReturnError(EBADMSG);
   }

   memset(&uhdr, 0, UHDRLEN);

   iov[0].iov_base = (caddr_t)&uhdr;
   iov[0].iov_len = UHDRLEN;

   /* Users buffer */
   iov[1].iov_base = p_buf;
   iov[1].iov_len = len;
         
   /* Init msghdr */
#ifdef DGUX
   msg.msg_name = (struct sockaddr *)xstDescriptors[d].sin;
#else
   msg.msg_name = (caddr_t)xstDescriptors[d].sin;
#endif
   msg.msg_namelen = sizeof(struct sockaddr_in);
   msg.msg_iov = (struct iovec *)iov;
   msg.msg_iovlen = 2;
#if defined (QNX) || defined (LINUX)
   msg.msg_flags = 0;
#else
   msg.msg_accrights = NULL;
   msg.msg_accrightslen = 0;
#endif /* QNX || LINUX */
         
   if ( (dlen = rcvMesg(s, &msg, 0)) == ERROR )
      ReturnError(errno);

   if ( uhdr.mtype == SDATA )
      return(min(min(len, (int) uhdr.dsize), dlen));   /* #8 */
   
   /* Non Data message, save what we got */
   dlen = min( (int) uhdr.csize + (int) uhdr.dsize, len);   /* #8 */
   if ( (p_data = (caddr_t)malloc(sizeof(FreeBuf) + UHDRLEN + dlen)) == NULL )
      ReturnError(ENOBUFS);
   
   memcpy(p_data+sizeof(FreeBuf), &uhdr, UHDRLEN);

   p_free = (FreeBuf *)p_data;

   switch ( uhdr.mtype )
   {
      case SPROTO:
         p_free->this_one = p_free;

         if ( uhdr.flags == RS_HIPRI )
            addFree(xstDescriptors[d].p_ilist, p_free);
         else
            addFree(xstDescriptors[d].p_mlist, p_free);

         MPSerrno = EBADMSG;
         return(ERROR);
         break;

      case SERROR:
         /* Not sure here, this is fatal */
         p_data = (caddr_t)((char *)p_free + sizeof(FreeBuf) +
            UHDRLEN);/* #1 */
         errnum = *p_data;      /* #9 */
         free(p_free);
         ReturnError(errnum);      /* #9 */

      case SOPENACK:
      case SIOCACK:
      default:
         /* This is invalid here, trash the message */
         break;
   }

   free(p_free);
   ReturnError ( EBADMSG );
} /* end UDPread() */


/*<-------------------------------------------------------------------------
| rcvMesg()
|
| This routine facilitates byte swapping.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

static int rcvMesg ( int s, struct msghdr *p_msg, int flag )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

static int rcvMesg(s, p_msg, flag)
int s;
struct msghdr *p_msg;
int flag;

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
   int   dlen;

#ifdef   WINNT
   if ( ( dlen = recv ( s, ( char * ) p_msg,
      sizeof ( struct msghdr ), 0 ) ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( ( dlen = recvmsg ( s, p_msg, 0 ) ) == ERROR )
   {
#endif   /* WINNT */

      return(ERROR);
   }

   p_hdr = (UCSHdr *)p_msg->msg_iov[0].iov_base;

   p_hdr->mtype = ntohs(p_hdr->mtype);
   p_hdr->flags = ntohs(p_hdr->flags);
   p_hdr->csize = ntohs(p_hdr->csize);
   p_hdr->dsize = ntohs(p_hdr->dsize);

   return ( dlen );
} /* end rcvMesg() */


/*<-------------------------------------------------------------------------
| sndMesg()
|
| This routine facilitates byte swapping.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #3 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

static int sndMesg ( int s, struct msghdr *p_msg, int flag )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

static int sndMesg(s, p_msg, flag)
int s;
struct msghdr *p_msg;
int flag;

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
   int   dlen;

   p_hdr = (UCSHdr *)p_msg->msg_iov[0].iov_base;

   p_hdr->mtype = htons(p_hdr->mtype);
   p_hdr->flags = htons(p_hdr->flags);
   p_hdr->csize = htons(p_hdr->csize);
   p_hdr->dsize = htons(p_hdr->dsize);

#ifdef   WINNT
   if ( ( dlen = sendmsg ( s, p_msg, 0 ) ) == SOCKET_ERROR )
   {
      errno = WSA2MPS ( WSAGetLastError ( ) );
#else
   if ( ( dlen = sendmsg ( s, p_msg, 0 ) ) == ERROR )
   {
#endif   /* WINNT */

      return ( ERROR );
   }
   else
   {
      return ( dlen );
   }
} /* end sndMesg() */
