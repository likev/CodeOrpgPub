/*   @(#) MPSlocal.c 00/04/14 Version 1.19   */

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
1.  14-MAY-96  mpb  Put pragma's in for 64 to 32 bit conversion (for esca).
2.  23-MAY-96  mpb  Added prototyping of functions if ANSI_C is #ifdef'd.
3.  23-MAY-96  mpb  Changed uint reference of address value to ulong.
4.  25-OCT-96  lmm  Added byte swapping for little endian hosts
5.  22-JAN-97  lmm  Need to byte swap board number for open request 
6.  12-MAY-97  mpb  Make sure all malloc()'d memory is free()'d before
                    returning.
7.  14-MAY-97  mpb  LocalIoctl should return just muxid, not nothl(muxid)
                    since the driver will do the ntohl for us.
8.  13-MAR-98  lmm  Let driver do swapping of IOCTL's
9.  16-SEP-98  mpb  See #8 in MPSapi.h.
10. 16-SEP-98  mpb  Support for VxWorks.
11. 29-SEP-98  mpb  Do not call addDesc() until after the COPEN is processed
                    properly on the card (the ioctl()'s complete correctly).
12. 11-NOV-98  mpb  If either ioctl() fail, close the stream before returning
                    error.
13. 15-SEP-99  mpb  Took out the special processing for polling of a non MPS
                    sd for VxWorks.  MPSpoll() has been changed to catch it
                    before calling LocalPoll().
14. 15-SEP-99  mpb  Need to check to see if POLLSYS is one of the event flags
                    in LocalPoll().  If so, make sure we treat the descriptor
                    as a regular file descriptor.
15. 18-FEB-00  lmm  In LocalOpen(), don't close descriptor if interrupted
16. 04-APR-00  mpb  #15 creates a descriptor leak.  Instead, we will close
                    the descriptor, but only after we give the system and the
                    board a chance to get things set up.  A variant of sleep()
                    must be used since an interrupt can wake you out too
                    early.
17. 14-APR-00  kls  Instead of using the sleep variant of #16, use sigfillset
                    and sigprocmask to block interrupts while we do ioctl
                    calls in LocalOpen.
18. 31-MAY-00  jjw  Local (non-network) Linux STREAMS support.
*/

#include "MPSinclude.h"

#if defined( VXWORKS )
   #include <streams/stropts.h>
   #include <ioLib.h>
#elif !defined( LINUX )   /* #18 - Change #else to #elif clause. */
   #include <stropts.h>
#endif /* VXWORKS */

#include <signal.h>

/*<-------------------------------------------------------------------------
| 
|			MPS Localhost Library Routines
V  
-------------------------------------------------------------------------->*/


/*<-------------------------------------------------------------------------
| LocalOpen()
|
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C

MPSsd LocalOpen ( OpenRequest *p_open )

#else
MPSsd LocalOpen(p_open)
OpenRequest *p_open;

#endif	/* ANSI_C */
{
   OpenData odata, *p_odata;
   struct strioctl striocb;
   int sc;
   sigset_t old_mask;
   sigset_t block_mask;
   MPSsd newd;

#ifdef VXWORKS
   if ( (sc = open(p_open->serverName, O_RDWR, 0644)) == ERROR )
#else
   if ( (sc = open(p_open->serverName, O_RDWR)) == ERROR )
#endif /* VXWORKS */
   {
      ReturnError(errno);
   }

   memcpy((caddr_t)&odata, (caddr_t)&p_open->ctlrNumber, sizeof(OpenData));

   odata.dev = htonl ( odata.dev);	/* #4 */
   odata.flags = htonl ( odata.flags);	/* #4 */
   odata.slotnum = htonl ( odata.slotnum);	/* #5 */

   striocb.ic_cmd = COPEN;			/* #8 */
   striocb.ic_timout = p_open->openTimeOut;
   striocb.ic_dp = (caddr_t)&odata;
   striocb.ic_len = sizeof(OpenData);

#if !defined(VXWORKS)
   /* #17 grab every signal, and block them out for now. */
   sigfillset (&block_mask);
   sigprocmask (SIG_BLOCK, &block_mask, &old_mask);
#endif /* !VXWORKS */

#ifdef VXWORKS
   if ( ioctl(sc, I_STR, (int)&striocb) == ERROR )
#else
   if ( ioctl(sc, I_STR, &striocb) == ERROR )
#endif /* VXWORKS */
   {
      close ( sc );          /* #12 */
#if !defined(VXWORKS)
      sigprocmask (SIG_SETMASK, &old_mask, NULL); /* restore orignal signal
                                                     mask -#17 */
#endif /* !VXWORKS */
      ReturnError ( errno );
   }

   p_odata = (OpenData *)striocb.ic_dp;

   if ( ioctl(sc, I_SRDOPT, RMSGN) == ERROR )
   {
      close ( sc );          /* #12 */
#if !defined(VXWORKS)
      sigprocmask (SIG_SETMASK, &old_mask, NULL);   /* restore orignal signal
                                                       mask - #17 */
#endif /* !VXWORKS */

      ReturnError ( errno );
   }

#if !defined(VXWORKS)
   sigprocmask (SIG_SETMASK, &old_mask, NULL);      /* restore orignal signal
                                                       mask - #17 */
#endif /* !VXWORKS */

   /* #11 */
   if ( (newd = addDesc(sc, MPS_LOCAL)) < 0 )
   {
      LocalClose(sc);
      MPSerrno = ENOBUFS;
      return(ERROR);
   }

   SetStrId(newd, p_odata->slotnum);

   return(newd);
} /* end LocalOpen() */
   

/*<-------------------------------------------------------------------------
| LocalWrite()
|
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

int LocalWrite ( MPSsd s, caddr_t p_data, int len )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

int LocalWrite(s, p_data, len)
MPSsd s;
caddr_t p_data;
int len;

#endif	/* ANSI_C */
{
   int cnt;

   if ( (cnt = write(s, p_data, len)) == ERROR )
      ReturnError(errno);

   return(cnt);
} /* end LocalWrite() */

/*<-------------------------------------------------------------------------
| LocalRead()
|
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

int LocalRead ( MPSsd s, caddr_t p_data, int len )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

int LocalRead(s, p_data, len)
MPSsd s;
caddr_t p_data;
int len;

#endif	/* ANSI_C */
{
   int cnt;

   cnt = read(s, p_data, len);

   if ( cnt == ERROR )
   {
      if ( errno == EWOULDBLOCK )
	 errno = EAGAIN;

      MPSerrno = errno;
      return(ERROR);
   }

   return(cnt);
} /* end LocalRead() */

/*<-------------------------------------------------------------------------
| LocalIoctl()
|
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

int LocalIoctl ( MPSsd s, int command, caddr_t p_arg )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

int LocalIoctl(s, command, p_arg)
MPSsd s;
int command;
caddr_t p_arg;

#endif	/* ANSI_C */
{
   struct strioctl strioctl;
   struct xstrioctl *p_ioc;	/* #4 */
   ulong  temp, muxid;		/* #3 */

   muxid = 0;
   switch ( command )
   {
      case X_POP:
	 /* No argument */
         strioctl.ic_cmd = X_POP;	/* #8 */
	 strioctl.ic_timout = 0;
	 strioctl.ic_dp = NULL;
	 strioctl.ic_len = 0;
	 p_arg = (caddr_t)&strioctl;
	 command = I_STR;
	 break;
	 
      case X_PUSH:
	 /* p_arg is the name of the module to PUSH */
         strioctl.ic_cmd = X_PUSH;	/* #8 */
	 strioctl.ic_timout = 0;
	 strioctl.ic_dp = (char *)p_arg;
	 strioctl.ic_len = strlen(p_arg) + 1;
	 p_arg = (caddr_t)&strioctl;
	 command = I_STR;
	 break;
	 
      case X_UNLINK:
	 /* p_arg is the muxid to unlink or -1 */
         strioctl.ic_cmd = X_UNLINK;	/* #8 */
	 strioctl.ic_timout = 0;

	 muxid = (ulong)p_arg;			/* #3 */
         muxid = htonl ( muxid );		/* #4 */
	 strioctl.ic_dp = (char *)&muxid;

	 strioctl.ic_len = sizeof(int);
	 p_arg = (caddr_t)&strioctl;
	 command = I_STR;
	 break;

      case X_LINK:
	 /* p_arg is the file descriptor to be linked with this stream */

	 muxid = GetStrId((ulong)p_arg);	/* #3 */
         muxid = htonl ( muxid );		/* #4 */
	 p_arg = (caddr_t)muxid;

         strioctl.ic_cmd = X_LINK;		/* #8 */
	 strioctl.ic_timout = 0;
	 strioctl.ic_dp = (char *)&muxid;
	 strioctl.ic_len = sizeof(int);
	 p_arg = (caddr_t)&strioctl;
	 command = I_STR;
	 break;

      case X_STR:
         p_ioc = ( struct xstrioctl * ) p_arg;	 	/* #4 */ 

         /* #8 - removed htonl of p_ioc->ic_cmd (let driver swap) */
	 command = I_STR;
	 break;

      case X_FLUSH:
	 /* p_arg is FLUSHW, FLUSHR or FLUSHRW */
         strioctl.ic_cmd = X_FLUSH;		/* #8 */
	 strioctl.ic_timout = 0;

	 /* Use muxid as storage */
	 temp = (ulong)p_arg;		/* #3 */
	 strioctl.ic_dp = (char *)&temp;
	 strioctl.ic_len = sizeof(int);
	 p_arg = (caddr_t)&strioctl;
	 command = I_STR;
	 break;
	 
      default:
	 break;
   }

#ifdef VXWORKS
   if ( (muxid = ioctl(s, command, (int)p_arg)) == ERROR )
#else
   if ( (muxid = ioctl(s, command, p_arg)) == ERROR )
#endif /* VXWORKS */
   {
      ReturnError(errno);
   }

   return muxid;				/* #7 */
} /* end LocalIoctl() */

/*<-------------------------------------------------------------------------
| LocalGetmsg()
|
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

int LocalGetmsg ( MPSsd s, struct strbuf *p_ctl, struct strbuf *p_dbuf, 
                  int *flags )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

int LocalGetmsg(s, p_ctl, p_dbuf, flags)
MPSsd s;
struct strbuf *p_ctl, *p_dbuf;
int *flags;

#endif	/* ANSI_C */
{
   int cnt;

   if ( (cnt = getmsg(s, p_ctl, p_dbuf, flags)) == ERROR )
      ReturnError(errno);

   return(cnt);
} /* end LocalGetmsg() */

/*<-------------------------------------------------------------------------
| LocalPutmsg()
|
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

int LocalPutmsg ( MPSsd s, struct strbuf *p_ctl, struct strbuf *p_data, 
                  int flags )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

int LocalPutmsg(s, p_ctl, p_data, flags)
MPSsd s;
struct strbuf *p_ctl, *p_data;
int flags;

#endif	/* ANSI_C */
{
   int cnt;

   if ( (cnt = putmsg(s, p_ctl, p_data, flags)) == ERROR )
      ReturnError(errno);

   return(cnt);
} /* end LocalPutmsg() */

/*<-------------------------------------------------------------------------
| LocalClose()
|
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C

int LocalClose ( MPSsd s )

#else

int LocalClose(s)
MPSsd s;

#endif	/* ANSI_C */
{
   if ( close(s) == ERROR )
      ReturnError(errno);

   return(0);
} /* end LocalClose() */


/*<-------------------------------------------------------------------------
| LocalPoll()
|
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

int LocalPoll ( struct xpollfd *pfds, int nfds, int timeval )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

int LocalPoll(pfds, nfds, timeval)
struct xpollfd *pfds;
int nfds, timeval;

#endif	/* ANSI_C */
{
   struct pollfd *tfds;
   int cnt, i;

   if(!(tfds = (struct pollfd *)malloc(sizeof(struct pollfd) * nfds)))
   {
      ReturnError(ENOBUFS);
   }

   for(i=0; i<nfds; i++)
   {
      /* #14 */
      if(pfds[i].events & POLLSYS)
      {
         tfds[i].fd = pfds[i].sd;
         tfds[i].events = pfds[i].events & ~POLLSYS;
      }
      else
      {
         if(!StreamOpen(pfds[i].sd))
         {
            tfds[i].fd = GetSocket(pfds[i].sd);
            tfds[i].revents = POLLNVAL;
            tfds[i].events = 0;
         }
         else
         {
            tfds[i].fd = GetSocket(pfds[i].sd);
            tfds[i].events = pfds[i].events;
         }
      }
   }

   if((cnt = poll(tfds, nfds, timeval)) == ERROR)
   {
      /* #6 */
      free ( tfds );
      ReturnError(errno);
   }

   for(i=0; i<nfds; i++)
   {
      pfds[i].revents = tfds[i].revents;
   }

   free(tfds);
   return(cnt);
} /* end LocalPoll() */


