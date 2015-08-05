/*   @(#) MPSapi.c 00/01/11 Version 1.52   */

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
1.  10-FEB-96  LMM  Added support for Digital Unix 
2.  11-APR-96  LMM  Check for local device when API built for Local & LAN 
3.  14-MAY-96  mpb  Put pragma's in for 64 to 32 bit conversion (for esca).
4.  23-MAY-96  mpb  Added prototyping of functions if ANSI_C is #ifdef'd.
5.  23-MAY-96  mpb  Change the reference of pointers from "uint" to "ulong"
                    in "SetXstIO()" and "setIOType()" calls.
6.  03-JUL-96  LMM  Added version number string
7.  15-AUG-96  mpb  xstdesc.noblock is a uchar.
8.  15-AUG-96  mpb  xstdesc.streamID, socket is a ulong.
9.  12-NOV-96  LMM  Removed obsolete code with ifdef SOLARIS in setIOType 
10. 30-APR-97  mpb  Make thread save if desired. 
11. 12-MAY-97  mpb  If malloc() fails, return error, and free any previous
                    malloc()'d memory if applicable.
12. 13-OCT-97  mpb  ioctl() system call is different for LOCAL NT.
13. 03-NOV-97  lmm  Display MPSerrno code in MPSperror string
14. 14-NOV-96  mpb  Check for local NT device when API built for Local & LAN.
15. 14-NOV-96  mpb  setIOType needs to know if the sd is LOCAL or not (for NT).
16. 15-DEC-97  mpb  QNX support.
17. 04-MAY-98  mpb  MT-Safe for Windows NT.
18. 05-AUG-98  mpb  MPSioctl()'s second parameter is of type int, not MPSsd.
19. 16-SEP-98  mpb  Support for VxWorks.
20. 16-SEP-98  mpb  See #8 in MPSapi.h
21. 13-OCT-98  mpb  Need typecast from pointer to int.
22. 23-OCT-98  mpb  Support for Linux (LINUX).
23. 21-JAN-99  lmm  Use THREAD_SAFE define, #21 applies to VXWORKS only
24. 09-MAR-99  mpb  Free all memory when descriptor is deleted.
25. 27-MAY-99  mpb  VxWorks requires a value, not a pointer.
26. 18-JUN-99  mpb  Was not calling ReleaseMutex for multi-threaded NT.
                    This could lead to deadlock.
27. 15-SEP-99  mpb  Got to provide for the case in MPSpoll() where the first
                    descriptor is a system type, not MPS.
28. 20-SEP-99  mpb  Do not have the POLLSYS bit set for regular pool() call.
*/

#include "MPSinclude.h"
#if defined(HPUX) || defined(DECUX)        /* #1 */
#ifdef    DECUX_32            /* #3 */
#pragma    pointer_size    save
#pragma    pointer_size    long
#endif /* DECUX_32 */
#include <sys/ioctl.h>
#ifdef    DECUX_32
#pragma    pointer_size    restore
#endif    /* DECUX_32 */
#else

#ifndef DGUX
#ifndef IRIX4
#ifndef VMS
#ifndef WINNT
#ifndef QNX
#ifndef LINUX
#ifdef VXWORKS
#include <ioLib.h>
#else
#include <sys/filio.h>
#endif /* VXWORKS */
#endif /* !QNX */
#endif /* !LINUX */
#endif /* !WINNT */
#endif /* !VMS */
#endif /* !IRIX4 */
#endif /* !DGUX */

#endif

#include "MPSperror.h"
#include "uc_host_version.h"            /* #6 */

struct xstdesc xstDescriptors[MAXCONN];
int MaxDblock = 0;

static int totalconn = 0;

/**
  For the multithreaded case, the routines which update the xstDescriptors[]
  table,  the variable "totalconn", or the variable MaxDblock, should be
  regulated to guarantee the resources will be consistent.

  We will use a mutex to lock out interrupts by other threads while the
  routines TCPopen(), UDPopen() and LocalOpen() are called (in MPSopen())
  which in turn update MaxDblock and call addDesc() which updats the
  xstDescriptors[] table and the totalconn variable.  The same mutex will
  also be used in the routine MPSclose() which calls deleteDesc() which
  updats the xstDescriptors[] table and the totalconn variable.

  The mutex will also be usefull in MPSopen() because the routines 
  gethostbyname() and getservbyname() are not thread safe.  Since the
  mutex is being used, the re-entrant versions of these routines will
  not be used.
**/

/* #17 */
#if defined ( WINNT ) && defined ( _MT )

/**
    MT-Safe Windows NT API.  Need the extra mutex for creating the TLS
    index.
**/
HANDLE  MPSLockMutex, MPSGlobalDataMutex;

#else
/* #10 */
#ifdef THREAD_SAFE

static pthread_mutex_t  totalconn_lock      = PTHREAD_MUTEX_INITIALIZER;

#else

int MPSerrno = 0;

#endif /* THREAD_SAFE */

#endif /* WINNT && _MT */

static char api_version[] = API_VERSION;    /* #6 */

#ifdef VXWORKS
pti_TaskGlobals  *Ptg = NULL;
SEM_ID  pti_semMutex = NULL;
#endif /* VXWORKS */

/*<-------------------------------------------------------------------------
| 
|            MPS Library Routines
V  
-------------------------------------------------------------------------->*/


/*<-------------------------------------------------------------------------
| MPSopen()
|
| Opens a connection to the MPS server requested.  Obtains a TCP connection
| to the server via socket and connect, this routine depends on the server
| name being in the /etc/hosts database and the "mps"service entry in the 
| /etc/services database.  Obtains a connection to an xSTRa protocol STREAM 
| via a COPEN request.  Once the request is made the process is blocked
| until a response to the COPEN request is received from the server or 
| an error occurs.
| Timeout value is in seconds.
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

MPSsd MPSopen ( OpenRequest *p_open )

#else

MPSsd MPSopen(p_open)
OpenRequest *p_open;

#endif    /* ANSI_C */
{
#ifdef    DECUX_32            /* #3 */
#pragma   pointer_size    save
#pragma   pointer_size    long
#endif /* DECUX_32 */
   struct hostent *hp;
   struct servent *sp;
#ifdef    DECUX_32
#pragma   pointer_size    restore
#endif    /* DECUX_32 */
   int    ret_val;

#ifdef VXWORKS
   semTake ( pti_semMutex, WAIT_FOREVER );
#endif /* VXWORKS */

/* #17 */
#if defined ( WINNT ) && defined ( _MT )
   WaitForSingleObject ( MPSLockMutex, INFINITE );
#endif /* WINNT && _MT */

/* #10 */
#ifdef THREAD_SAFE
   pthread_mutex_lock ( &totalconn_lock );
#endif /* THREAD_SAFE */

   if ( totalconn >= MAXCONN )
   {
      MPSerrno = ECONNREFUSED;
      ret_val = ERROR;
   }
   else
   {
      /* Range Check */
      if ( ( p_open->openTimeOut > -1 && p_open->openTimeOut < OPENTIME ) || 
             p_open->openTimeOut < -1 )
      {
         p_open->openTimeOut = OPENTIME;
      }

#ifdef ALLAPI
      if ( ( p_open->serverName[0] == '/' )  ||      /* #2 */
           ( p_open->serverName[0] == '\\' ) )
      {
         ret_val = LocalOpen ( p_open );
      }
      else
      {
#endif /* ALLAPI */

         if ( (hp = gethostbyname(p_open->serverName)) == NULL )
         {
            ret_val = LocalOpen ( p_open );
         }

         /* Changed to this because Solaris2.2 getservbyname does not work
         ** with a NULL as the proto param although it claims to support it.
         */

         else if ( (sp = getservbyname(p_open->serviceName, "tcp")) )
         {
            ret_val = TCPopen ( p_open, hp, sp );
         }

         else if ( (sp = getservbyname(p_open->serviceName, "udp")) )
         {
            ret_val = UDPopen ( p_open, hp, sp );
         }

         else
         {
            MPSerrno = ENXIO;
            ret_val = ERROR;
         }

#ifdef ALLAPI
      }
#endif /* ALLAPI */
   }

/* #10 */
#ifdef THREAD_SAFE
   pthread_mutex_unlock ( &totalconn_lock );
#endif /* THREAD_SAFE */

/* #17 */
#if defined ( WINNT ) && defined ( _MT )
   ReleaseMutex ( MPSLockMutex );
#endif /* WINNT && _MT */

#ifdef VXWORKS
   semGive ( pti_semMutex );
#endif /* VXWORKS */

   return ret_val;
} /* end MPSopen() */

#ifndef    VMS
/*<-------------------------------------------------------------------------
| MPSopenS()
|
| Initiates a non-blocking connection to the MPS server requested. Obtains a
| TCP connection to the server via socket and connect, this routine depends
| on the server name being in the /etc/hosts database and the "mps" service
| entry in the /etc/services database. 
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int MPSopenS ( OpenRequest *p_open )

#else

int MPSopenS(p_open)
OpenRequest *p_open;

#endif    /* ANSI_C */
{
#ifdef    DECUX_32            /* #3 */
#pragma   pointer_size    save
#pragma   pointer_size    long
#endif /* DECUX_32 */
   struct hostent *hp;
   struct servent *sp;
#ifdef    DECUX_32
#pragma   pointer_size    restore
#endif    /* DECUX_32 */

   if ( totalconn >= MAXCONN )
   {
      ReturnError(ECONNREFUSED);
   }

   /* Range Check */
   if ( ( p_open->openTimeOut > -1 && p_open->openTimeOut < OPENTIME ) || 
                                               p_open->openTimeOut < -1 )
      p_open->openTimeOut = OPENTIME;

   if ( (hp = gethostbyname(p_open->serverName)) == NULL )
      ReturnError(ENOSYS);

   /* Changed to this because Solaris2.2 getservbyname does not work
   ** with a NULL as the proto param although it claims to support it.
   */

   if ( (sp = getservbyname(p_open->serviceName, "tcp")) )
      return(TCPopenS(hp, sp));

   if ( (sp = getservbyname(p_open->serviceName, "udp")) )
      ReturnError(ENOSYS);

   ReturnError(ENXIO);
} /* end MPSopenS() */
#endif    /* VMS */

#ifndef    VMS
/*<-------------------------------------------------------------------------
| MPSopenP()
|
| Completes a connection to the MPS server requested. Obtains a connection
| to an xSTRa protocol STREAM via a COPEN request. Once the request is made
| the process is blocked until a response to the COPEN request is received
| from the server or an error occurs. Timeout value is in seconds.
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

MPSsd MPSopenP ( OpenRequest *p_open, int sc )

#else

MPSsd MPSopenP(p_open, sc)
OpenRequest *p_open;
int sc;

#endif    /* ANSI_C */
{
#ifdef    DECUX_32            /* #3 */
#pragma   pointer_size    save
#pragma   pointer_size    long
#endif /* DECUX_32 */
   struct hostent *hp;
   struct servent *sp;
#ifdef    DECUX_32
#pragma   pointer_size    restore
#endif    /* DECUX_32 */

   /* Range Check */
   if ( ( p_open->openTimeOut > -1 && p_open->openTimeOut < OPENTIME ) || 
                                               p_open->openTimeOut < -1 )
      p_open->openTimeOut = OPENTIME;

   if ( (hp = gethostbyname(p_open->serverName)) == NULL )
      ReturnError(ENOSYS);

   /* Changed to this because Solaris2.2 getservbyname does not work
   ** with a NULL as the proto param although it claims to support it.
   */

   if ( (sp = getservbyname(p_open->serviceName, "tcp")) )
#if defined ( __hp9000s800 ) || defined ( WINNT )
      return(TCPopenP(p_open, sc, hp, sp));
#else
      return(TCPopenP(p_open, sc));
#endif /* ! (__hp9000s800 || WINNT) */

   if ( (sp = getservbyname(p_open->serviceName, "udp")) )
      ReturnError(ENOSYS);

   ReturnError(ENXIO);
} /* end MPSopenP() */
#endif    /* VMS */


/*<-------------------------------------------------------------------------
| MPSwrite()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int MPSwrite ( MPSsd d, caddr_t p_data, int len )

#else

int MPSwrite(d, p_data, len)
MPSsd d;
caddr_t p_data;
int len;

#endif    /* ANSI_C */
{
   if ( StreamOpen(d) )
   {
      switch ( StreamType(d) )
      {
         case MPS_TCP:
            return(TCPwrite(d, p_data, len));
   
         case MPS_UDP:
            return(UDPwrite(d, p_data, len));

         case MPS_LOCAL:
#if defined ( WINNT )
            return(LocalWrite(d, p_data, len));
#else
            return(LocalWrite(GetSocket(d), p_data, len));
#endif /* WINNT */

         default:
            return(ERROR);
      }
   }
   else
   {
      MPSerrno = EBADF;
      return(ERROR);
   }
} /* end MPSwrite() */


/*<-------------------------------------------------------------------------
| MPSread()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int MPSread ( MPSsd d, caddr_t p_buf, int len )

#else

int MPSread(d, p_buf, len)
MPSsd d;
caddr_t p_buf;
int len;

#endif    /* ANSI_C */
{
   if ( StreamOpen(d) )
   {
      switch ( StreamType(d) )
      {
         case MPS_TCP:
            return(TCPread(d, p_buf, len));
   
         case MPS_UDP:
            return(UDPread(d, p_buf, len));
   
         case MPS_LOCAL:
#if defined ( NO_STREAMS ) || defined ( WINNT )
            return(LocalRead(d, p_buf, len));
#else
            return(LocalRead(GetSocket(d), p_buf, len));
#endif

         default:
            return(ERROR);
      }
   }
   else
   {
      MPSerrno = EBADF;
      return(ERROR);
   }
} /* end MPSread() */

/*<-------------------------------------------------------------------------
| MPSioctl()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int MPSioctl ( MPSsd d, int command, caddr_t p_arg )  /* #18 */

#else

int MPSioctl(d, command, p_arg)
MPSsd d;
int command;
caddr_t p_arg;

#endif    /* ANSI_C */
{
   int status;

   if ( !StreamOpen(d) )
   {
      MPSerrno = EBADF;
      return(ERROR);
   }

   switch ( command )
   {
      case X_SETIOTYPE:
#ifndef VMS
         status =
            setIOType(GetSocket(d), (ulong)p_arg, StreamType(d)); /* #5, #15 */
#else
         status = MPSvmssetIOtype(d, (uint)p_arg);
#endif
#ifdef    WINNT
         SetXstIO(d, (uchar)p_arg);    /* #5 */
#else
         SetXstIO(d, (ulong)p_arg);    /* #5 */
#endif    /* WINNT */
         return(status);

      case X_GETIOTYPE:
         /* BlockIO returns TRUE if Blocking, but BLOCK = 0 */
         *p_arg = (char)!BlockIO(d);
         return(0);
   }

   switch ( StreamType(d) )
   {
      case MPS_TCP:
         return(TCPioctl(d, command, (caddr_t)p_arg));

      case MPS_UDP:
         return(UDPioctl(d, command, (caddr_t)p_arg));

      case MPS_LOCAL:
#if defined ( NO_STREAMS ) || defined ( WINNT )
         return(LocalIoctl(d, command, (caddr_t)p_arg));
#else
         return(LocalIoctl(GetSocket(d), command, (caddr_t)p_arg));
#endif

      default:
         return(ERROR);
   }
} /* end MPSioctl() */


/*<-------------------------------------------------------------------------
| MPSgetmsg()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C
#ifdef    DECUX_32            /* #3 */
#pragma   pointer_size    save
#pragma   pointer_size    long
#endif /* DECUX_32 */

int MPSgetmsg ( MPSsd d, struct xstrbuf *p_ctl, struct xstrbuf *p_dbuf, 
                int *flags )

#ifdef    DECUX_32
#pragma   pointer_size    restore
#endif    /* DECUX_32 */
#else

int MPSgetmsg(d, p_ctl, p_dbuf, flags)
MPSsd d;
struct xstrbuf *p_ctl, *p_dbuf;
int *flags;

#endif    /* ANSI_C */
{
   if ( StreamOpen(d) )
   {
      switch ( StreamType(d) )
      {
         case MPS_TCP:
            return(TCPgetmsg(d, p_ctl, p_dbuf, flags));
   
         case MPS_UDP:
            return(UDPgetmsg(d, p_ctl, p_dbuf, flags));

         case MPS_LOCAL:
#if defined ( NO_STREAMS ) || defined ( WINNT )
            return(LocalGetmsg(d, p_ctl, p_dbuf, flags));
#else
            return(LocalGetmsg(GetSocket(d), (struct strbuf *)p_ctl,
                               (struct strbuf *)p_dbuf, flags));
#endif
   
         default:
            return(ERROR);
      }
   }
   else
   {
      MPSerrno = EBADF;
      return(ERROR);
   }
} /* end MPSgetmsg () */


/*<-------------------------------------------------------------------------
| MPSputmsg()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C
#ifdef    DECUX_32            /* #3 */
#pragma   pointer_size    save
#pragma   pointer_size    long
#endif /* DECUX_32 */

int MPSputmsg ( MPSsd d, struct xstrbuf *p_ctl, struct xstrbuf *p_data, 
                int flags )

#ifdef    DECUX_32
#pragma    pointer_size    restore
#endif    /* DECUX_32 */
#else

int MPSputmsg(d, p_ctl, p_data, flags)
MPSsd d;
struct xstrbuf *p_ctl, *p_data;
int flags;

#endif    /* ANSI_C */
{
   if ( StreamOpen(d) )
   {
      switch ( StreamType(d) )
      {
         case MPS_TCP:
            return(TCPputmsg(d, p_ctl, p_data, flags));
   
         case MPS_UDP:
            return(UDPputmsg(d, p_ctl, p_data, flags));

         case MPS_LOCAL:
#if defined ( NO_STREAMS ) || defined ( WINNT )
            return(LocalPutmsg(d, p_ctl, p_data, flags));
#else
            return(LocalPutmsg(GetSocket(d), (struct strbuf *)p_ctl,
                               (struct strbuf *)p_data, flags));
#endif
 
         default:
            return(ERROR);
      }
   }
   else
   {
      MPSerrno = EBADF;
      return(ERROR);
   }
} /* end MPSputmsg() */


/*<-------------------------------------------------------------------------
| MPSclose()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int MPSclose ( MPSsd d )

#else

int MPSclose(d)
MPSsd d;

#endif    /* ANSI_C */
{
   int    ret_val;
   mpssd  s;

#ifdef VXWORKS
   semTake ( pti_semMutex, WAIT_FOREVER );
#endif /* VXWORKS */
/* #17 */
#if defined ( WINNT ) && defined ( _MT )
   WaitForSingleObject ( MPSLockMutex, INFINITE );
#endif /* WINNT && _MT */

/* #10 */
#ifdef THREAD_SAFE
   pthread_mutex_lock ( &totalconn_lock );
#endif /* THREAD_SAFE */

   s = GetSocket(d);

   if ( !StreamOpen(d) )
   {
      ret_val = 0;
   }
   else
   {
      switch ( StreamType(d) )
      {
         case MPS_TCP:
         {
            if ( TCPclose(s) == ERROR )
            {
               ret_val = ERROR;
            }
            else
            {
               deleteDesc(d);
               ret_val = 0;
            }

            break;
         }

         case MPS_UDP:
         {
            if ( UDPclose(d) == ERROR )
            {
               ret_val = ERROR;
            }
            else
            {
               deleteDesc(d);
               ret_val = 0;
            }

            break;
         }

         case MPS_LOCAL:
         {
            if ( LocalClose(s) == ERROR )
            {
               ret_val = ERROR;
            }
            else
            {
               deleteDesc(d);
               ret_val = 0;
            }

            break;
         }

         default:
         {
            ret_val = ERROR;
            break;
         }
      }
   }

/* #10 */
#ifdef THREAD_SAFE
   pthread_mutex_unlock ( &totalconn_lock );
#endif /* THREAD_SAFE */

/* #26 */
#if defined ( WINNT ) && defined ( _MT )
   ReleaseMutex ( MPSLockMutex );
#endif /* WINNT && _MT */

#ifdef VXWORKS
   semGive ( pti_semMutex );
#endif /* VXWORKS */

   return ret_val;
} /* end MPSclose() */


/*<-------------------------------------------------------------------------
| MPSpoll()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C
#ifdef    DECUX_32            /* #3 */
#pragma   pointer_size    save
#pragma   pointer_size    long
#endif /* DECUX_32 */

int MPSpoll ( struct xpollfd *pfds, int nfds, int timeval )

#ifdef    DECUX_32
#pragma   pointer_size    restore
#endif    /* DECUX_32 */
#else

int MPSpoll(pfds, nfds, timeval)
struct xpollfd *pfds;
int nfds, timeval;

#endif    /* ANSI_C */
{
   int  i, type;

#if !defined(WINNT) && !defined(VXWORKS)
   int  val;
#endif /* !WINNT && !VXWORKS */

   /* #27 */
   /* We have an array of descriptors; some of them can be MPS sd's, and
      some (for the non WINNT and VXWORKS case) can be actual file 
      descriptors.  We need to figure out which type of poll to call.  If 
      there are no MPS sd's in the array, than call the regular system poll().
      If at least one descriptor is of MPS type, than use its brand of poll.
      We will assume that if there are more than one MPS sd's, they are ALL of 
      the same type. */

   type = 0;
   for(i=0; i<nfds; i++)
   {
      if(pfds[i].events & POLLSYS)
      {
         /* NT and VxWorks do not support non MPS stream 
            descriptors (POLLSYS) */
#if defined(WINNT) || defined(VXWORKS)
         MPSerrno = EIO;
         return MPS_ERROR;
#else
         type = -1;
#endif /* !WINNT && !VXWORKS */ 
      }
      else
      {
         type = xstDescriptors[pfds[i].sd].type;
         break; /* Since we have found one MPS, we bail from here. */
      }
   }

   switch(type)
   {
      case MPS_TCP:
         return(TCPpoll(pfds, nfds, timeval));

      case MPS_UDP:
         return(UDPpoll(pfds, nfds, timeval));

      case MPS_LOCAL:
         return(LocalPoll(pfds, nfds, timeval));

#if !defined(WINNT) && !defined(VXWORKS)
      case -1:
      {
         struct pollfd *tfds;
         int i;

         /* #28 Since we will be calling the system poll() routine, we need
            to zero out the POLLSYS bit (not a system parameter).  We make
            a copy of the structure so we will not change any of the input
            parameters. */
         if(!(tfds = (struct pollfd *)malloc(sizeof(struct pollfd) * nfds)))
         {
            ReturnError(ENOBUFS);
         }

         for(i=0; i<nfds; i++)
         {
            if(pfds[i].events & POLLSYS)
            {
               tfds[i].fd = pfds[i].sd;
               tfds[i].events = pfds[i].events & ~POLLSYS;
            }
         }

         val = poll(tfds, nfds, timeval);
         if(val == -1)
         {
            free(tfds);
            MPSerrno = errno;
            return MPS_ERROR;
         }

         for(i=0; i<nfds; i++)
         {
            pfds[i].revents = tfds[i].revents;
         }

         free(tfds);
         return val;
      }
#endif /* !WINNT && !VXWORKS */

      default:
         return(ERROR);
   }
} /* end MPSpoll() */


/*<-------------------------------------------------------------------------
| MPSperror()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int MPSperror ( char *s )

#else

MPSperror(s)
char *s;

#endif    /* ANSI_C */
{
   uint tmp;

#ifdef    VMS
   if ( MPSerrno == EVMSERR )
   {
      tmp = errno;
      errno = EVMSERR;
      perror((s == NULL ? "" : s));
      errno = tmp;
      return;
   }
#endif    /* VMS */

   if ( s )
      fprintf(stderr, "%s: ", s);

   tmp = (uint)(MPSerrno & 0xff);

#ifndef HPUX
   if ( (tmp & MPSERR) == MPSERR )
   {
      /* An xSTRa environment error code */
      tmp &= ~MPSERR;

      if ( tmp > MPSERRNOMAX )
         fprintf(stderr, "Unknown MPSerrno  <%d>\n", tmp);
      else /* #13 */
         fprintf(stderr, "MPS%03d: %s\n", MPSerrno, MPSerrStrings[tmp]);
   }
   else
#endif
   {
      /* A local system error code */

      tmp = errno;
      errno = MPSerrno;
#ifdef    VMS
      perror("");
#else
      perror(NULL);
#endif
      errno = tmp;
   }

   return 0;
} /* end MPSperror() */


/*<-------------------------------------------------------------------------
| addDesc()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int addDesc ( mpssd s, int type )

#else

int addDesc(s, type)
mpssd s;
int type;

#endif    /* ANSI_C */
{
   int i;

   for ( i = 0; i < MAXCONN; i++ )
   {
      if ( xstDescriptors[i].state == CLOSED )
      {
         xstDescriptors[i].socket = s;
         xstDescriptors[i].state = OPEN;
         xstDescriptors[i].ioc = FALSE;
         xstDescriptors[i].type = type;

         /* High-Priority Message List */
         xstDescriptors[i].p_ilist = (FreeList *)malloc(sizeof(FreeList));

         /* #11 */
         if ( ! xstDescriptors[i].p_ilist )
         {
            return ERROR;
         }

         xstDescriptors[i].p_ilist->head = 
                  xstDescriptors[i].p_ilist->tail = NULL;

         /* Data/Proto List */
         xstDescriptors[i].p_mlist = (FreeList *)malloc(sizeof(FreeList));

         /* #11 */
         if ( ! xstDescriptors[i].p_mlist )
         {
            free ( xstDescriptors[i].p_ilist );
            return ERROR;
         }

         xstDescriptors[i].p_mlist->head = 
                  xstDescriptors[i].p_mlist->tail = NULL;

#ifdef VXWORKS
	 xstDescriptors[i].tid = taskIdSelf();
#endif 

         totalconn++;

         return(i);
      }
   }
   return(ERROR);
} /* end addDesc() */



/*<-------------------------------------------------------------------------
| deleteDesc()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

void deleteDesc ( int s )

#else

void deleteDesc(s)
int s;

#endif    /* ANSI_C */
{
   struct xstdesc *p_xst = &xstDescriptors[s];

   p_xst->socket = 0;
   p_xst->state = CLOSED;
   p_xst->streamID = 0;    /* #8 */

   totalconn--;

   drainList(p_xst->p_ilist);
   drainList(p_xst->p_mlist);

   /* #24 */
   if (p_xst->p_ilist)
   {
      free(p_xst->p_ilist);
   }
   if (p_xst->p_mlist)
   {
      free(p_xst->p_mlist);
   }

#ifdef VXWORKS
   xstDescriptors[s].tid = 0;
#endif

   xstDescriptors[s].sin = NULL;
} /* end deleteDesc() */


/*<-------------------------------------------------------------------------
| setIOType()
|
| Sets the socket s to blocking (noblock = 0) or non-blocking (noblock = 1)
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int setIOType ( mpssd s, ulong noblock, uchar sd_type )  /* #15 */

#else

int setIOType ( s, noblock, sd_type )  /* #15 */
mpssd s;
ulong noblock;
uchar sd_type;

#endif    /* ANSI_C */
{
    /* #9 - removed ifdef SOLARIS here (obsolete, was setting O_NONBLOCK) */

/* #12 */
#ifdef WINNT
    if ( sd_type == MPS_LOCAL )  /* #15 */
    {
       DWORD  dwByteCount;

       if ( ! DeviceIoControl (
          ( HANDLE ) s,
          IOCTL_PCI334_FIONBIO,
          &noblock,
          sizeof ( ulong ),
          NULL,
          0,
          &dwByteCount,
          NULL ) )
       {
           errno = WSA2MPS ( GetLastError ( ) );
           ReturnError ( errno );
       }
    }
    else
    {
       if ( ioctlsocket ( s, FIONBIO, &noblock ) == SOCKET_ERROR ) 
       {
           errno = WSA2MPS ( WSAGetLastError ( ) );
           ReturnError ( errno );
       }
    }
#else /* !WINNT */

#ifdef VXWORKS
    if ( sd_type == MPS_LOCAL )  /* #15 */
    {
       if ( ioctl( s, FIONBIO, (int) noblock ) == ERROR ) /* #21, #25 */
       {
          ReturnError ( errno );
       }
    }
    else
    {
       if ( ioctl( s, FIONBIO, (int) &noblock ) == ERROR ) /* #21, #25 */
       {
          ReturnError ( errno );
       }
    }
#else
    if ( ioctl( s, FIONBIO, &noblock ) == ERROR )       /* #23 */
    {
       ReturnError ( errno );
    }
#endif
#endif /* !WINNT */

    return ( 0 );
} /* end setIOType() */



/*<-------------------------------------------------------------------------
| MPSSysDescriptor()
|
| Slow interface to descriptor mappings.  Useful for event handlers that
| wish to use their own select ( or other event handler ) and need the 
| "real" descriptor.  
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

mpssd    MPSSysDescriptor ( MPSsd sd )

#else

mpssd MPSSysDescriptor(sd)
MPSsd sd;

#endif    /* ANSI_C */
{
   return ( GetSocket ( sd ) );
} /* end MPSSysDescriptor() */


/*<-------------------------------------------------------------------------
| MPSdisabled()
|
V  
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

int MPSdisabled ( int err )

#else

int MPSdisabled(err)
int err;

#endif    /* ANSI_C */
{
   MPSerrno = err;
   return(ERROR);
} /* end MPSdisabled() */
