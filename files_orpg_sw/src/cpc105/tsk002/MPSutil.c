
/************************************************************************

    Description: This was an UconX library module. A change was made in 
	this module to improve its performance. This file should be 
	include in our cm_uconx build to override the module in libMPS.

************************************************************************/

/* 
 * RCS info
 * $Author: hoyt $
 * $Locker:  $
 * $Date: 1998/06/24 15:06:01 $
 * $Id: MPSutil.c,v 1.6 1998/06/24 15:06:01 hoyt Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  


static char sccsid[] = "@(#) MPSutil.c 96/08/13 Version 1.4";
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
-------------------------------------------------------------------------->*/
  

/*<-------------------------------------------------------------------------
|
|  This software is furnished  under  a  license and may be used and
|  copied only  in  accordance  with  the  terms of such license and
|  with the inclusion of the above copyright notice.   This software
|  or any other copies thereof may not be provided or otherwise made
|  available to any other person.   No title to and ownership of the
|  program is hereby transferred.
|
|  The information  in  this  software  is subject to change without
|  notice and should not  be  considered  as  a  commitment by UconX
|  Corporation.
|			  UconX Corporation
|			San Diego, California
v                                                
-------------------------------------------------------------------------->*/

/*
Modification history:
 
Chg Date       Init Description
1.  14-MAY-96  mpb  Put pragma's in for 64 to 32 bit conversion (for esca).
2.  23-MAY-96  mpb  Added prototyping of functions if ANSI_C is #ifdef'd.
*/

#include "MPSinclude.h"
#include <string.h>
#include <stdlib.h>


/*<-------------------------------------------------------------------------
| 
|			MPS Utility Routines
V  
-------------------------------------------------------------------------->*/


/*<-------------------------------------------------------------------------
| readHdr()
|
| Reads an MPS header.
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

int readHdr ( int s, UCSHdr *p_uhdr )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

int readHdr(s, p_uhdr)
int s;
UCSHdr *p_uhdr;

#endif	/* ANSI_C */
{
   if ( readData(s, (caddr_t)p_uhdr, UHDRLEN) == ERROR )
      return(ERROR);

   p_uhdr->mtype = ntohs(p_uhdr->mtype);
   p_uhdr->flags = ntohs(p_uhdr->flags);
   p_uhdr->csize = ntohs(p_uhdr->csize);
   p_uhdr->dsize = ntohs(p_uhdr->dsize);

   return(0);
} /* end readHdr() */


/*<-------------------------------------------------------------------------
| readData()
|
| Reads data to an input buffer.
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

int readData ( int s,  caddr_t p_buf, int len )

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

int readData(s, p_buf, len )
int s;
caddr_t p_buf;
int len;

#endif	/* ANSI_C */
{
   int tlen, rlen;

   tlen = len;
   while ( len > 0 )
   {
      /* Check for errors here */
#ifdef NO_STREAMS
      if ( ( rlen = read ( s, p_buf, len ) ) != ERROR )
#else
#ifdef	WINNT
      if ( ( rlen = recv ( s, p_buf, len, 0 ) ) != SOCKET_ERROR )
#else
      if ( ( rlen = recv ( s, p_buf, len, 0 ) ) != ERROR )
#endif	/* WINNT */
#endif
      {
         p_buf += rlen;

         if ( ! ( len -= rlen ) )
		 {
			break;
		 }
         else
		 {
		         struct pollfd pfd;
		         pfd.fd = s;
		         pfd.revents = 0;
		         pfd.events = POLLIN;
       			 if (poll ( &pfd, 1, 1000 ) == 0)
			      continue;
		 }
      }

      if ( rlen == 0 )
      {
         /* if recv returns 0 then connection is disconnected */
		 ReturnError ( ECONNRESET );
      }

#ifdef	WINNT
	  if ( rlen == SOCKET_ERROR )
	  {
		 errno = WSA2MPS ( WSAGetLastError ( ) );
#else
	  if ( rlen == ERROR )
	  {
#endif	/* WINNT */
		 if ( errno == EWOULDBLOCK )
		 {

            /* If some data read, continue reading else return */
				if ( len < tlen )
				{
					continue;
				}
				else
				{
					ReturnError ( EWOULDBLOCK );
	            }
			}
			else
			{
				/* A non-EWOULDBLOCK error, this is probably fatal */
				ReturnError ( errno );
         }
      }
   }
   
   return(0);
} /* end readData() */



/*<-------------------------------------------------------------------------
| readSave()
|
| Given MPS header, reads the expected data and saves it in a malloced 
| buffer.
V  
-------------------------------------------------------------------------->*/

#ifdef	ANSI_C
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */

FreeBuf *readSave ( int s, UCSHdr *p_uhdr )
{	/* need to have "{" here because this function returns a pointer. */

#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */
#else

FreeBuf *readSave(s, p_uhdr)
int s;
UCSHdr *p_uhdr;
{

#endif	/* ANSI_C */

   int dlen;
#ifdef	DECUX_32			/* #1 */
#pragma	pointer_size	save
#pragma	pointer_size	long
#endif /* DECUX_32 */
   FreeBuf *p_free;
   char *p_buf;
#ifdef	DECUX_32
#pragma	pointer_size	restore
#endif	/* DECUX_32 */

   dlen = sizeof(FreeBuf) + UHDRLEN + p_uhdr->csize + p_uhdr->dsize;
   if ( (p_buf = (char *)malloc(dlen)) == NULL )
   {
      MPSerrno = ENOBUFS;
      return(NULL);
   }
   
   p_free = (FreeBuf *)p_buf;
   p_free->this_one = p_free;
   
   /* Leave room for list pointers */
   p_buf += sizeof(FreeBuf);
    
   memcpy(p_buf, (char *)p_uhdr, UHDRLEN);
   p_buf += UHDRLEN;
   dlen = p_uhdr->dsize + p_uhdr->csize;

   while ( readData(s, p_buf, dlen) == ERROR )
   {
      if ( MPSerrno != EWOULDBLOCK )
	  {
		return(NULL);
	  }
   }

   return((FreeBuf *)p_free);
} /* end readSave() */


#ifdef	WINNT
int sendmsg ( int s, const struct msghdr *msg, int flags )
/************
	senmsg ( )   ---

	Windows NT does not provide the sendmsg() socket system call.  The UconX
	MPS API uses it, and to port it as cleanly over to NT, we need this routine.

	There are two aspects to the system call sendmsg().  The first is that the socket
	does not have to be in a connected state (optional information is provided
	to connect it if it is not already).  The second is the ability to write from
	noncontiguous buffers (scatter write).

	The UconX api only takes advantage of the scatter write functionality of 
	sendmsg(), so that is what will be provided here.

	To ensure that all the data will be sent contiguously, we must put it all together
	in one contiguous space, then send that space.  Just performing consecutive
	writes for each data block will not this requirments.

	Parameters:
		s		- A descriptor identifying a connected socket.
		msg		- Holds information blocks in struct iovec part.
		flags	- Same flags as used in send() and sendto() system calls.

	Return values:
		Unlees a memory malloc error occurs, this call will have the same return
		values as the routines send() and sendto().  In the  case of a memory
		alloc error, SOCKET_ERROR will still be returned, and the error code that
		can be retrieved by WSAGetLastError() will be set to ENOMEM since there
		is no equivalent WSAENOMEM value.

************/
{
	int		i, len, val;
	char	*buff_ptr, *ptr;

	len = 0;
	for ( i = 0; i < msg->msg_iovlen; i++ )
	{
		len += msg->msg_iov [ i ].iov_len;
	}

	buff_ptr = malloc ( len );
	if ( !buff_ptr )
	{
		WSASetLastError ( ENOMEM );
		return SOCKET_ERROR;
	}

	ptr = buff_ptr;
	for ( i = 0; i < msg->msg_iovlen; i++ )
	{
		memcpy ( ptr, ( char * ) msg->msg_iov [ i ]. iov_base,
				msg->msg_iov [ i ].iov_len );
		ptr += msg->msg_iov [ i ].iov_len;
	}

	val = send ( s, buff_ptr, len, flags );
	free ( buff_ptr );

	return val;
} /* end sendmsg() */
#endif	/* WINNT */



#ifdef	 WINNT
int WSA2MPS ( int err_num )
/************
   WSA2MPS ( )   ---

   Convert an WSA error number to an MPS erro number.  Windows NT has different
   error numbers for socket calls, but we must must use the regular BSD style
   ones in our API (both for usage and return values).
************/
{

   /* The following comment is from winsock.h: */
   
   /*
   * All Windows Sockets error constants are biased by WSABASEERR from
   * the "normal"
   */

   /* With this in mind, we will just return the error number minus WSABASEERR.  If
	  it turns out that thins are not as "normal" as hoped for (not properly biased),
	  then one big massive switch will have to be performed. */

	switch ( err_num )
	{
		case WSAEBADF:
			err_num = EBADF;
			break;
		case WSAEINVAL:
			err_num = EINVAL;
			break;
		case WSAEWOULDBLOCK:
			err_num = EWOULDBLOCK;	
			break;
		case WSAEINPROGRESS:
			err_num = EINPROGRESS;
			break;
		case WSAEALREADY:
			err_num = EALREADY;
			break;
		case WSAECONNRESET:
			err_num = ECONNRESET;
			break;
		case WSAENOBUFS:
			err_num = ENOBUFS;
			break;
		case WSAEISCONN:
			err_num = EISCONN;
			break;
		case WSAECONNREFUSED:
			err_num = ECONNREFUSED;
			break;
		case WSAEACCES:
			err_num = EACCES;
		case ENOMEM:
			err_num = ENOMEM;	/* No WSAENOMEM equivalent. */
			break;
		case WSAEINTR:
			err_num = EINTR;
			break; 
		case WSAENAMETOOLONG:
			err_num = ENAMETOOLONG;
			break;
		case WSAENOTEMPTY:
			err_num = ENOTEMPTY;
			break;
		case WSAEMFILE:
			err_num = EMFILE;
			break;
		case WSAEFAULT:
			err_num = EFAULT;
			break;
		case WSAEOPNOTSUPP:
		case WSAEPFNOSUPPORT:
		case WSAEAFNOSUPPORT:
		case WSAEADDRINUSE:
		case WSAEADDRNOTAVAIL:
		case WSAENETDOWN:
		case WSAENETUNREACH:
		case WSAENETRESET:
		case WSAECONNABORTED:
		case WSAENOTCONN:
		case WSAESHUTDOWN:
		case WSAETOOMANYREFS:
		case WSAETIMEDOUT:
		case WSAELOOP:
		case WSAEHOSTDOWN:
		case WSAEHOSTUNREACH:
		case WSAEPROCLIM:
		case WSAEUSERS:
		case WSAEDQUOT:
		case WSAESTALE:
		case WSAEREMOTE:
		case WSAEDISCON:
		case WSASYSNOTREADY:
		case WSAVERNOTSUPPORTED:
		case WSANOTINITIALISED:
		case WSAENOTSOCK:
		case WSAEDESTADDRREQ:
		case WSAEMSGSIZE:
		case WSAEPROTOTYPE:
		case WSAENOPROTOOPT:
		case WSAEPROTONOSUPPORT:
		case WSAESOCKTNOSUPPORT:
		default:
// printf("MYK: WSA2MPS default hit with err_num = %d\n", err_num);
			err_num = ERROR;
			break;
	}   

	return err_num;
} /* end WSA2MPS() */
#endif	 /* WINNT */
