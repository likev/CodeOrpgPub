/*   @(#) streamio.h 99/12/23 Version 1.2   */
 
/*******************************************************************************
*                            Copyright (c) 1998 by
*
*    ===     ===     ===         ===         ===
*    ===     ===   =======     =======     =======
*    ===     ===  ===   ===   ===   ===   ===   ===
*    ===     === ===     === ===     === ===     ===
*    ===     === ===     === ===     === ===     ===   ===            ===
*    ===     === ===         ===     === ===     ===  =====         ======
*    ===     === ===         ===     === ===     === ==  ===      =======
*    ===     === ===         ===     === ===     ===      ===    ===   =
*    ===     === ===         ===     === ===     ===       ===  ==
*    ===     === ===         ===     === ===     ===        =====
*    ===========================================================
*    ===     === ===         ===     === ===     ===        =====
*    ===     === ===         ===     === ===     ===       ==  ===
*    ===     === ===     === ===     === ===     ===      ==    ===
*    ===     === ===     === ===     === ===     ====   ===      ===
*     ===   ===   ===   ===   ===   ===  ===     =========        ===  ==
*      =======     =======     =======   ===     ========          =====
*        ===         ===         ===     ===     ======             ===
*
*      U   c   o   n   X      C   o   r   p   o   r   a   t   i   o   n
*
*      This software is furnished  under  a  license and may be used and
*      copied only  in  accordance  with  the  terms of such license and
*      with the inclusion of the above copyright notice.   This software
*      or any other copies thereof may not be provided or otherwise made
*      available to any other person.   No title to and ownership of the
*      program is hereby transferred.
*
*      The information  in  this  software  is subject to change without
*      notice  and  should  not be considered as a commitment by UconX
*      Corporation.
*
*******************************************************************************/
 
/*
Modification history:
 
Chg Date       Init Description
1.   7-May-98  rjp  Redefine to MPS... Also, redefine I_... to X_...
2.  11-AUG-98  mpb  Call exit_program() instead of exit() for S_EXIT when
                    woring in the UCONx environment.
*/
 
 

/*
 *  STREAMS system calls macros for portability
 *
 *  Copyright (c) 1990-1994 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine.
 *
 *  Made in Scotland.
 *
 *  These macros are intended for use by all STREAMS applications,
 *  so that STREAMS system calls can be trapped in emulation libraries,
 *  etc.
 *
 *  @(#)$Spider: streamio.h,v 1.1 1997/05/26 13:51:36 mark Exp $
 */

#ifndef STREMUL

/* Native STREAMS */
/*
* xSTRa streams. #1
*/
#define S_OPEN		MPSopen
#define S_CLOSE		MPSclose
#define S_READ		MPSread
#define S_WRITE		MPSwrite
#define S_IOCTL		MPSioctl
#define S_FCNTL		fcntl
#define S_GETMSG	MPSgetmsg
#define S_PUTMSG	MPSputmsg
#define S_POLL		MPSpoll
#define S_FORK		fork
#define S_DUP		dup
#define S_DUP2		dup2
#define	S_EXIT		exit_program    /* #2 */

#define I_STR		X_STR
#define I_LINK		X_LINK
#define I_POP		X_POP
#define I_PUSH		X_PUSH
#define I_UNLINK	X_UNLINK
#define I_UNLINKALL	X_UNLINKALL
/*
 * Define to indicate that all multiplexors beneath a stream should
 * be unlinked.
 */
#define MUXID_ALL       (-1)


#else /* STREMUL */

/* SpiderSTREAMS in User Space */

#define S_OPEN		s_open
#define S_CLOSE		s_close
#define S_READ		s_read
#define S_WRITE		s_write
#define S_IOCTL		s_ioctl
#define S_FCNTL		s_fcntl
#define S_GETMSG	s_getmsg
#define S_PUTMSG	s_putmsg
#define S_POLL		s_poll
#define S_FORK		s_fork
#define S_DUP		s_dup
#define S_DUP2		s_dup2
#define	S_EXIT		s_exit

#endif /* STREMUL */

#ifdef STREMUL
/* user space library calls */
extern int s_open();
extern int s_close();
extern int s_read();
extern int s_write();
extern int s_ioctl();
extern int s_fcntl();
extern int s_getmsg();
extern int s_putmsg();
extern int s_poll();
extern int s_fork();
extern int s_dup();
extern void s_exit();
#endif /* STREMUL */
