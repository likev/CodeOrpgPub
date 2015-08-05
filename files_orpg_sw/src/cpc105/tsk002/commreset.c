/*   @(#) commreset.c 99/11/02 Version 1.6   */

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
Modification history:
 
Chg Date       Init Description
1.  29-MAY-98  lmm  Copied from commload.c, and cut out all but reset
                    functionality.  This is intended to be used to reset
                    the host board of a LAN server, so have the open go
                    to strhead instead of UC_bif.
2.  11-AUG-98  lmm  Use HOST_BNUM define in mpsio.h
                    Added support for soft reset
3.   5-MAY-99  tdg  Added VxWorks support
*/

#if defined ( VMS ) || defined ( WINNT )
typedef unsigned char   u_char;
typedef unsigned int    u_int;
#endif    /* VMS || WINNT */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef VXWORKS
#include <netinet/in.h>
#endif /* VXWORKS */
#ifdef    WINNT
#include <windows.h>
typedef char *caddr_t;
int getopt ( int, char *const *, const char * );
#include <winsock.h>
#endif    /* WINNT */
#include "xstopts.h"
#include "xstypes.h"
#include "xstpoll.h"
#ifndef WINNT
#include "mpsio.h"
#endif /* !WINNT */
#include "mpsproto.h"

#define    strioctl    xstrioctl

#define PROTO_NAME      "strhead"
#define DEFAULT_SERVICE "mps"
#define VALID_OPTIONS    "fs:S:v"		/* #2 */
#define USAGE            "\
usage: %s [-f] [-s server] [-S service] [-v]\n\n"

extern char    *optarg;
extern int     optind, opterr;

static MPSsd sd = -1;

#ifdef    ANSI_C
static void    exit_program ( int );
#ifdef VXWORKS
int 	       commreset ( int, char**, pti_TaskGlobals* );
#else
int            main ( int, char** );
#endif
#else
static void    exit_program ( );
#ifdef VXWORKS
int 	       commreset ( );
#else
int            main ( );
#endif
#endif    /* ANSI_C */

/*<-------------------------------------------------------------------------
| --------------------------------------------------------------------------
||                                                                        ||
||                           commreset                                    ||
||                                                                        ||
V --------------------------------------------------------------------------
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

#ifdef  DECUX_32
#pragma pointer_size    save
#pragma pointer_size    long
#endif /* DECUX_32 */

#ifdef VXWORKS
int commreset ( int argc, char **argv, pti_TaskGlobals *ptg )
#else
int main ( int argc, char **argv )
#endif

#ifdef  DECUX_32
#pragma pointer_size    restore
#endif  /* DECUX_32 */

#else

#ifdef VXWORKS
int commreset ( argc, argv, ptg )
#else
int main ( argc, argv )
#endif /* VXWORKS */
int  argc;
char **argv;
#ifdef VXWORKS
pti_TaskGlobals *ptg;
#endif

#endif
{
    char  opt;
    char  *server, *service;
    int   softreset, verbose;
    OpenRequest     oreq;
    struct strioctl rctl;
    mps_reset_t     rdata;

#ifdef    WINNT
    LPSTR   lpMsg;
    WSADATA WSAData;
    int     optionValue = SO_SYNCHRONOUS_NONALERT;
#endif    /* WINNT */

#ifdef    WINNT
/* For NT, need to initialize the Windows Socket DLL */
    if ( WSAStartup ( 0x0101, &WSAData) )
    {
        FormatMessage ( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, WSAGetLastError ( ), LANG_USER_DEFAULT, 
            ( LPSTR ) &lpMsg, 0, NULL );
        fprintf ( stderr, "\n\nWSAStartup() failed:\n%s\n", lpMsg );

        LocalFree ( lpMsg );
        exit ( -1 );
    }
#endif    /* WINNT */

#ifdef VXWORKS
    init_mps_thread ( ptg );
#endif

    /* Initialize variables and structures */
    verbose    = 0;
    service    = DEFAULT_SERVICE;
    server     = NULL;
    softreset  = 0;

    while ( ( opt = getopt ( argc, argv, VALID_OPTIONS ) ) != ERROR ) 
    {
        switch ( opt ) 
        {
            /* #2 - added soft reset feature */
            case 'f':
                softreset = 1;
                break;

            case 'S':
                service = optarg;
                break;
   
            case 's':
                server = optarg;
                break;

            case 'v':
                verbose = 1;
                break;

            default:
            case '?':
                printf ( USAGE, argv [ 0 ] );
                exit_program ( ERROR );
        }
    }

    if ( ( argc - optind ) == 1 )  /* Parameter with no flag, use as server. */
    {
        server = argv [ optind ];
    }

    if ( ! server )
    {
        printf ( USAGE, argv [ 0 ] );
        exit_program ( ERROR );
    }

    /* Setup open request */
    memset  ( &oreq, 0, sizeof ( oreq ) );
    strncpy ( oreq.serverName, server, MAXSERVERLEN );
    strncpy ( oreq.serviceName, service, MAXSERVNAMELEN );
    strncpy ( oreq.protoName, PROTO_NAME, MAXPROTOLEN );
    oreq.ctlrNumber     = HOST_BNUM;	/* #2 */
    oreq.openTimeOut    = 10;    /* 10 seconds before timeout. */

    if ( verbose )
    {
        printf ( "Opening connection to server %s\n", server);
    }

    /* Open connection to server */
    if ( ( sd = MPSopen ( &oreq ) ) == -1 )
    {
        MPSperror ( "comm_open() - MPSopen()" );
        exit_program ( ERROR );
    }

    /*
    * This is reset only, so we simply send the reset to the Server.
    */

    if ( verbose )
    {
        printf ( "Issuing reset to server %s\n", server );
        fflush ( stdout );
    }

    memset ( &rdata, 0, sizeof ( mps_reset_t ) );
    rdata.bnum = htonl ( HOST_BNUM );	/* #2 */

    /* #2 - added support for soft reset */
    if (!softreset )
       rctl.ic_cmd    = RESET;
    else
       rctl.ic_cmd    = SOFTRESET;

    rctl.ic_len    = sizeof ( mps_reset_t );
    rctl.ic_dp     = ( char * ) &rdata;
    rctl.ic_timout = 300;

    /* Issue reset IOCTL request */
    if ( MPSioctl ( sd, X_STR, ( caddr_t ) &rctl ) == ERROR ) 
    {
        MPSperror ( "doReset() - MPSioctl()" );
        exit_program ( ERROR );
    }

    if ( verbose )
    {
        printf ( "Reset to server %s successful\n", server );
        fflush ( stdout );
    }

    exit_program ( GOOD_EXIT ); 

    return 0;

} /* end main() */


/**********

exit_program() ---

Instead of just exiting, clean up system before exit.

**********/
#ifdef    ANSI_C
static void exit_program ( int error_val )
#else
static void exit_program ( error_val )
int    error_val;
#endif    /* ANSI_C */
{
#ifdef    WINNT
    LPSTR        lpMsg;
#endif    /* WINNT */

#ifdef    WINNT
    if ( WSACleanup ( ) == SOCKET_ERROR )
    {
        FormatMessage ( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, WSAGetLastError ( ), LANG_USER_DEFAULT, 
            ( LPSTR ) &lpMsg, 0, NULL );
        fprintf ( stderr, "\n\nWSACleanup() failed:\n%s\n", lpMsg );

        LocalFree ( lpMsg );
    }
#endif    /* WINNT */

    /* Close connection to server */
    if ( sd != -1 )
    {
       MPSclose (sd);
    }

#ifdef VXWORKS
    cleanup_mps_thread ( );
#endif 

    exit ( error_val );
} /* end exit_program() */
