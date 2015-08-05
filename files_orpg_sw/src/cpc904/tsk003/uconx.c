/*   @(#) uconx.c 99/11/02 Version 1.4   */

 
/***************************************************************************
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
****************************************************************************/
 
/***************************************************************************
*
* conf_control.c
*
***************************************************************************/
 
/*
Modification history:
 
Chg Date        Init Description
 1.  1-jul-98   rjp  Originated.
 
*/
#include <stdio.h>
#include <stdlib.h>

#ifdef WINNT
typedef char * caddr_t;
#include <winsock.h>
#endif /* WINNT */

#include "xstopts.h"
#include "mpsproto.h"

#ifndef uchar
#define uchar unsigned char
#endif

OpenRequest 	oreq_x25;		/* MPS open structure 		*/
OpenRequest 	oreq_lapb;		/* MPS open structure 		*/
OpenRequest 	oreq_wan;		/* MPS open structure 		*/
uchar		serverName [MAXSERVERLEN]; /* MPS open server		*/
uchar		serviceName [MAXSERVNAMELEN]; /* MPS open service	*/
int  		controllerNumber; 	/* MPS open controller		*/


/*************************************************************************
* exit_program
*
* Instead of just exiting, clean up system before exit.
* 
*************************************************************************/

#ifndef VXWORKS
#ifdef    ANSI_C
void
exit_program ( int error_val )
#else
void
exit_program ( error_val )
    int		error_val;
#endif  /* ANSI_C */
{
#ifdef  WINNT
    LPSTR	lpMsg;
#endif  /* WINNT */
 
#ifdef  WINNT
    if (WSACleanup () == SOCKET_ERROR)
    {
	FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER
		       | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError (),
		LANG_USER_DEFAULT, (LPSTR) &lpMsg, 0, NULL );
 
      fprintf ( stderr, "\n\nWSACleanup() failed:\n%s\n", lpMsg );
 
      LocalFree ( lpMsg );
   }
#endif  /* WINNT */
 
   exit (error_val );
 
} 				/* end exit_program() 			*/
#endif /* !VXWORKS */
