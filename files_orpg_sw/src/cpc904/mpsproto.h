/*   @(#) mpsproto.h 99/12/23 Version 1.30   */

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
|      UconX Corporation
|      San Diego, California
|                                               
v                                                
-------------------------------------------------------------------------->*/

/*
Modification history:
 
Chg Date       Init Description
1.  30-APR-97  mpb  Make thread save if desired.
2.  10-JUL-97  mpb  Added MPS_THREADS to "defined _POSIX_C_SOURCE"
                    statement to fix DECUX compiler error.
3.  13-OCT-97  mpb  MPSSysDescriptor() needs to return type SOCKET for NT.
4.  30-OCT-97  mpb  Cleaned up some errno redefinitions in the NT world.
5.  31-OCT-97  mpb  Need winntdrv.h to get driver defined error values which 
                    winnt does not know about.
6.  19-DEC-97  lmm  Added define for MPS_ERROR
7.  26-MAR-98  mpb  Windows NT needs mpsio.h so it can get kernel information
                    statistics from a client application.
8.  04-MAY-98  mpb  MT-Safe for Windows NT.
9.  05-AUG-98  mpb  MPSioctl()'s second parameter is of type int, not MPSsd.
10. 28-JAN-99  mpb  For the multi-threaded prototypes, need the extern "C"
                    gaurding when doing C++ code.
11. 29-sep-99  rjp  Add api multicasting function prototypes.
12. 12-NOV-99  mpb  Moved the extern "C" gaurding for c++ code to encompass
                    the whole file.
*/

#ifdef    __cplusplus
extern "C"
{
#endif    /* __cplusplus */
#define MPS_ERROR -1

#ifdef WINNT
#include "mpsio.h"
#include "winntdrv.h"
#endif /* WINNT */

/***
    Function prototypes for the API functions.  This should be included 
    by the client applications, and the API files.
***/

#ifdef VXWORKS
#include <semLib.h>
#define sleep(a)  taskDelay(sysClkRateGet()*(a))
typedef struct
{
   int  xTID;
   SEM_ID  tskSem;
} pti_TaskGlobals;

IMPORT pti_TaskGlobals  *Ptg;
IMPORT int  MPSerrno;

extern void cleanup_mps_thread ( void );
extern void init_mps_thread    ( pti_TaskGlobals* );

#else
/* #8 */
#if defined ( WINNT ) && defined ( _MT )
extern void cleanup_mps_thread ( void );
extern int  init_mps_thread ( void );
extern int  *_MPSerrno ( void );
#define MPSerrno ( * ( _MPSerrno ( ) ) )
#else
/* #1 */
#if defined(MPS_THREADS) && ((_POSIX_C_SOURCE - 0 >= 199506L) || defined(_REENTRANT))
extern int init_mps_thread ( );
extern int *_MPSerrno ( );
#define MPSerrno ( * ( _MPSerrno ( ) ) )
#else
extern int MPSerrno;
#endif /* (_POSIX_C_SOURCE - 0 >= 199506L) */
#endif /* WINNT && _MT */
#endif /* VXWORKS */


#ifdef VMS
/* Map VMS system error values to what UCONx client
   applications will expect to receive. */
#define   EBADMSG        77
#endif /* VMS */

#ifdef    ANSI_C

#ifdef    DECUX_32            /* #3 */
#pragma   pointer_size    save
#pragma   pointer_size    long
#endif /* DECUX_32 */
int       MPSgetmsg        ( MPSsd, struct xstrbuf*, struct xstrbuf*, int* );
int       MPSpoll          ( struct xpollfd*, int, int );
int       MPSputmsg        ( MPSsd, struct xstrbuf*, struct xstrbuf*, int );
#ifdef    DECUX_32
#pragma   pointer_size    restore
#endif    /* DECUX_32 */

int      MPSSysDescriptor ( MPSsd );

int       MPSclose         ( MPSsd );
int       MPSdisabled      ( int );
void      MPSexit          ( int );
int       MPSioctl         ( MPSsd, int, caddr_t );  /* #9 */
MPSsd     MPSopen          ( OpenRequest* );
MPSsd     MPSMopen         ( OpenRequest*,struct sockaddr_in *, int); /* #11 */
#ifndef VMS
MPSsd     MPSopenP         ( OpenRequest*, int );
int       MPSopenS         ( OpenRequest* );
#endif  /* VMS */
int       MPSperror        ( char* );
int       MPSread          ( MPSsd, caddr_t, int );
int       MPSwrite         ( MPSsd, caddr_t, int );

#else

int       MPSSysDescriptor ( );
int       MPSclose         ( );
int       MPSdisabled      ( );
void      MPSexit          ( );
int       MPSgetmsg        ( );
int       MPSioctl         ( );
MPSsd     MPSopen          ( );
#ifndef VMS
MPSsd     MPSopenP         ( );
int       MPSopenS         ( );
#endif  /* VMS */
int       MPSperror        ( );
int       MPSpoll          ( );
int       MPSputmsg        ( );
int       MPSread          ( );
int       MPSwrite         ( );

#endif    /* ANSI_C */

#ifdef    __cplusplus
}    /* extern "C" */
#endif    /* __cplusplus */
