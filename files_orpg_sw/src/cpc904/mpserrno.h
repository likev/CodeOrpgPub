/*   @(#) mpserrno.h 99/12/23 Version 1.2   */
/*<-------------------------------------------------------------------------
| 
|                           Copyright (c) 1997 by
|
|              +++    +++                           +++     +++
|              +++    +++                            +++   +++ 
|              +++    +++   +++++     + +    +++   +++++   +++ 
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|              +++    +++ +++   ++ ++++ ++++ ++++  +++ ++ +++  
|              +++    ++++++      ++++   +++++++++ +++  +++    
|              +++    ++++++                             +     
|              +++    ++++++      ++++   +++++++ +++++  +++    
|              +++    +++ +++   ++ ++++ ++++ +++  ++++ ++++++  
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
|               +++  +++    +++++     + +    +++   ++++++  +++ 
|                ++++++         Corporation          +++    +++
|                 ++++   All the right connections  ++++    ++++
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
|
v                                                
-------------------------------------------------------------------------->*/

/*
Modification history:
 
Chg Date       Init Description
1.  10-NOV-97  LMM  New file to obsolete old errno.h and sys_errno.h files 
*/

#ifndef _mpserrno_h
#define _mpserrno_h

#define MPSERR	0xc0

#ifndef MPSAPI

/* Include the error codes used within the xSTRa environment only */

#define ENO_ERROR       0               /* No error */
#define	EPERM		1		/* Not owner */
#define	ENOENT		2		/* No such file or directory */
#define	ESRCH		3		/* No such process */
#define	EINTR		4		/* Interrupted system call */
#define	E2BIG		7		/* Arg list too long */
#define	ENOEXEC		8		/* Exec format error */
#define	EBADF		9		/* Bad file number */
#define	ECHILD		10		/* No children */
#define	EFAULT		14		/* Bad address */
#define	ENOTBLK		15		/* Block device required */
#define	EXDEV		18		/* Cross-device link */
#define	ENOTDIR		20		/* Not a directory*/
#define	EISDIR		21		/* Is a directory */
#define	ENFILE		23		/* File table overflow */
#define	EMFILE		24		/* Too many open files */
#define	ENOTTY		25		/* Not a typewriter */
#define	ETXTBSY		26		/* Text file busy */
#define	EFBIG		27		/* File too large */
#define	ENOSPC		28		/* No space left on device */
#define	ESPIPE		29		/* Illegal seek */
#define	EROFS		30		/* Read-only file system */
#define	EMLINK		31		/* Too many links */

/* math software */
#define	EDOM		33		/* Argument too large */

/* non-blocking and interrupt i/o */
#define	EINPROGRESS	36		/* Operation now in progress */
#define	EALREADY	37		/* Operation already in progress */
/* ipc/network software */

	/* argument errors */
#define	ENOTSOCK	38		/* Socket operation on non-socket */
#define	EDESTADDRREQ	39		/* Destination address required */
#define	EPROTOTYPE	41		/* Protocol wrong type for socket */
#define	ENOPROTOOPT	42		/* Protocol not available */
#define	ESOCKTNOSUPPORT	44		/* Socket type not supported */
#define	EPFNOSUPPORT	46		/* Protocol family not supported */
#define	EAFNOSUPPORT	47		/* Address family not supported by protocol family */

	/* operational errors */
#define	ENETDOWN	50		/* Network is down */
#define	ENETRESET	52		/* Network dropped connection on reset */
#define	EISCONN		56		/* Socket is already connected */
#define	ENOTCONN	57		/* Socket is not connected */
#define	ESHUTDOWN	58		/* Can't send after socket shutdown */
#define	ETOOMANYREFS	59		/* Too many references: can't splice */

	/* */
#define	ELOOP		62		/* Too many levels of symbolic links */
#define	ENAMETOOLONG	63		/* File name too long */

/* should be rearranged */
#define	EHOSTDOWN	64		/* Host is down */
#define	EHOSTUNREACH	65		/* No route to host */
#define	ENOTEMPTY	66		/* Directory not empty */

/* quotas & mush */
#define	EPROCLIM	67		/* Too many processes */
#define	EUSERS		68		/* Too many users */
#define	EDQUOT		69		/* Disc quota exceeded */

/* Network File System */
#define	ESTALE		70		/* Stale NFS file handle */
#define	EREMOTE		71		/* Too many levels of remote in path */

/* streams */
#define	ETIME		73		/* Timer expired */
#define	ENOMSG		75		/* No message of desired type */

/* SystemV IPC */
#define EIDRM		77		/* Identifier removed */

/* SystemV Record Locking */
#define EDEADLK		78		/* Deadlock condition. */
#define ENOLCK		79		/* No record locks available. */

/* RFS */
#define ENONET		80		/* Machine is not on the network */
#define ERREMOTE	81		/* Object is remote */
#define EADV		83		/* advertise error */
#define ESRMNT		84		/* srmount error */
#define EMULTIHOP	87		/* multihop attempted */
#define EDOTDOT		88		/* Cross mount point (not an error) */
#define EREMCHG		89		/* Remote address changed */

/* POSIX */
#define ENOSYS		90		/* function not implemented */

/* Reno tim */
#define ERESTART	91		/* For reno syscalls */

/* 
** UconX error codes
**
** These codes should only be used by software running in the xSTRa 
** environment or by software that interacts directly with software running
** in the xSTRa environment (a host driver or the UconX API).  Host level
** applications should use MPSperror() to get a detailed explanation of
** an error code returned by the UconX API.
**
** UconX error codes cannot go higher than 63 (MPSERR | 63).
** This is due to the one-byte error code used by an M_ERROR message and
** an attempt to differentiate the UconX error codes from the Unix system
** error codes.  All UconX errors are greater than 0xc0 (192).
*/

#define	EIO		(MPSERR | 1)	/* I/O error */
#define	ENXIO		(MPSERR | 2)	/* No such device or address */
#define	ERANGE		(MPSERR | 3)	/* Argument too large */
#define	EAGAIN		(MPSERR | 4) 	/* Operation would block */
#define	ENOMEM		(MPSERR | 5) 	/* Not enough memory */
#define	EACCES		(MPSERR | 6)	/* Permission denied */
#define	EBUSY		(MPSERR | 7)	/* Device busy */
#define	EEXIST		(MPSERR | 8)	/* Stream already exists */
#define	ENODEV		(MPSERR | 9)	/* No such device */
#define	EINVAL		(MPSERR | 10)	/* Invalid argument */
#define EPIPE		(MPSERR | 11)   /* Broken stream */
#define	EWOULDBLOCK	EAGAIN
#define	EMSGSIZE	(MPSERR | 12)	/* Message too long */
#define	EPROTONOSUPPORT	(MPSERR | 13)	/* Protocol not supported */
#define	EOPNOTSUPP	(MPSERR | 14)	/* Operation not supported on socket */
#define	EADDRINUSE	(MPSERR | 15)	/* Address already in use */
#define	EADDRNOTAVAIL	(MPSERR | 16)	/* Can't assign requested address */
#define	ENETUNREACH	(MPSERR | 17)	/* Network is unreachable */
#define	ECONNABORTED	(MPSERR | 18)	/* Software caused connection abort */
#define	ECONNRESET	(MPSERR | 19)	/* Connection reset by peer */
#define	ENOBUFS		(MPSERR | 20)	/* No buffer space available */
#define	ETIMEDOUT	(MPSERR | 21)	/* Connection timed out */
#define	ECONNREFUSED	(MPSERR | 22)	/* Connection refused */
#define	ENOSTR		(MPSERR | 23)	/* Device is not a stream */
#define	ENOSR		(MPSERR | 24)	/* Out of streams resources */
#define	EBADMSG		(MPSERR | 25)	/* Trying to read unreadable message */
#define ENOLINK		(MPSERR | 26)	/* the link has been severed */
#define ECOMM		(MPSERR | 27)	/* Communication error on send */
#define EPROTO		(MPSERR | 28)	/* Protocol error */
#define EBADE		(MPSERR | 29)	/* Exchange failure */

#endif /* if MPSAPI is not defined */

#define MPSSYSERRMAX    (MPSERR | 29)   /* Max system error code */

#ifdef MPSAPI

/*
** Error codes returned by the API only. 
*/

#define ESOCKET         (MPSERR | 58)	/* No socket for conn */
#define ECONNECT        (MPSERR | 59)	/* No TCP conn to server */
#define EOPENINVAL      (MPSERR | 60)	/* Invalid param for MPSopen */
#define EOPENTIMEDOUT   (MPSERR | 61)	/* No response to MPSopen */
#define ENOTLOCAL   	(MPSERR | 62)	/* API, network only */
#define ENOTTCPIP   	(MPSERR | 63)	/* API, local only */

#endif

#define MPSERRNOMAX	63

#endif /*!_mpserrno_h*/
