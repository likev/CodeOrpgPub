/*   @(#) MPSperror.h 00/01/11 Version 1.4   */

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
1.  17-FEB-96  LMM  Added system_errno table to map UconX server codes
                    into system error codes
*/

/*
** MPSperror strings for errors from the xSTRa environment. 
** Since errnos start at 1 the 0 element is unused.
*/

char *MPSerrStrings[] = 
{
   "",
   "I/O error",						/* EIO             */  
   "No such device or address",				/* ENXIO           */  
   "Argument too large",				/* ERANGE          */  
   "Operation would block",				/* EAGAIN          */  
   "Not enough memory",					/* ENOMEM          */  
   "Permission denied",					/* EACCES          */  
   "Device busy",					/* EBUSY           */  
   "Stream already exists",				/* EEXIST          */  
   "No such device",					/* ENODEV          */  
   "Invalid argument",					/* EINVAL          */ 
   "Broken stream",					/* EPIPE           */ 
   "Message too long",					/* EMSGSIZE        */ 
   "Protocol not supported",				/* EPROTONOSUPPORT */ 
   "Operation not supported on socket",			/* EOPNOTSUPP      */ 
   "Address already in use",				/* EADDRINUSE      */ 
   "Can't assign requested address",			/* EADDRNOTAVAIL   */ 
   "Network is unreachable",				/* ENETUNREACH     */ 
   "Software caused connection abort",			/* ECONNABORTED    */ 
   "Connection reset by peer",				/* ECONNRESET      */ 
   "No buffer space available",				/* ENOBUFS         */ 
   "Connection timed out",				/* ETIMEDOUT       */ 
   "Connection refused",				/* ECONNREFUSED    */ 
   "Device is not a stream",				/* ENOSTR          */ 
   "Out of streams resources",				/* ENOSR           */ 
   "Trying to read unreadable message",			/* EBADMSG         */ 
   "the link has been severed",				/* ENOLINK         */ 
   "Communication error on send",			/* ECOMM           */ 
   "Protocol error",					/* EPROTO          */ 
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",  					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unknown error",					/* NOTUSED	   */
   "Unable to get a socket for conn",			/* ESOCKET         */ 
   "Unable to get a TCP conn to server",		/* ECONNECT        */ 
   "Invalid param in MPSopen request",			/* EOPENINVAL      */ 
   "No response to MPSopen request",			/* EOPENTIMEDOUT   */ 
   "API installed is for LAN only",			/* ENOTLOCAL	   */
   "API installed is for local only",			/* ENOTTCPIP	   */
};

/* #1 - Table to map UconX server error codes into system error codes */

int system_errno [ ] =
{
   0,		/* Initial entry is null */

#ifdef EIO
   EIO,
#else
   0,
#endif

#ifdef ENXIO
   ENXIO,
#else
   0,
#endif

#ifdef ERANGE 
   ERANGE,
#else
   0,
#endif

#ifdef EAGAIN 
   EAGAIN,
#else
   0,
#endif

#ifdef ENOMEM 
   ENOMEM,
#else
   0,
#endif

#ifdef EACCES 
   EACCES,
#else
   0,
#endif

#ifdef EBUSY 
   EBUSY,
#else
   0,
#endif

#ifdef EEXIST 
   EEXIST,
#else
   0,
#endif

#ifdef ENODEV 
   ENODEV,
#else
   0,
#endif

#ifdef EINVAL 
   EINVAL,
#else
   0,
#endif

#ifdef EPIPE 
   EPIPE,
#else
   0,
#endif

#ifdef EMSGSIZE 
   EMSGSIZE,
#else
   0,
#endif

#ifdef EPROTONOSUPPORT 
   EPROTONOSUPPORT,
#else
   0,
#endif

#ifdef EOPNOTSUPP 
   EOPNOTSUPP,
#else
   0,
#endif

#ifdef EADDRINUSE 
   EADDRINUSE,
#else
   0,
#endif

#ifdef EADDRNOTAVAIL 
   EADDRNOTAVAIL,
#else
   0,
#endif

#ifdef ENETUNREACH 
   ENETUNREACH,
#else
   0,
#endif

#ifdef ECONNABORTED 
   ECONNABORTED,
#else
   0,
#endif

#ifdef ECONNRESET 
   ECONNRESET,
#else
   0,
#endif

#ifdef ENOBUFS 
   ENOBUFS,
#else
   0,
#endif

#ifdef ETIMEDOUT 
   ETIMEDOUT,
#else
   0,
#endif

#ifdef ECONNREFUSED 
   ECONNREFUSED,
#else
   0,
#endif

#ifdef ENOSTR 
   ENOSTR,
#else
   0,
#endif

#ifdef ENOSR 
   ENOSR,
#else
   0,
#endif

#ifdef EBADMSG 
   EBADMSG,
#else
   0,
#endif

#ifdef ENOLINK 
   ENOLINK,
#else
   0,
#endif

#ifdef ECOMM 
   ECOMM,
#else
   0,
#endif

#ifdef EPROTO
   EPROTO,
#else
   0,
#endif

#ifdef EBADE
   EBADE,
#else
   0,
#endif

};
