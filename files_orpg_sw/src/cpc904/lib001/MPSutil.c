/*   @(#) MPSutil.c 00/04/11 Version 1.38   */
 
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
3.  17-FEB-97  lmm  Moved check for receive length=0 in readData
4.  30-APR-97  mpb  Make thread save if desired. 
5.  12-MAY-97  mpb  When memory is malloc'd, make sure that it is either
                    returned to the calling routine's control, or free()'d.
6.  13-OCT-97  mpb  Embedded WINNT specific system routines and errors.
7.  30-OCT-97  mpb  Cleaned up some errno redefinitions in the NT world.
8.  03-NOV-97  mpb  Consolidated some of the error processing in for WINNT.
9.  02-DEC-97  mpb  Moved embedded NT routines (NTwritev(), NTgetmsg(),
                    NTpoll()) to MPSlocal_winnt.c since serve no purpose here.
10. 04-MAY-98  mpb  MT-Safe for Windows NT.
11. 23-SEP-98  mpb  gethostbyname() & getservbyname for VxWorks.
12. 21-JAN-99  lmm  Use THREAD_SAFE define
13. 17-MAY-99  mpb  When reading data over a TCP link (readData()), and only
                    part of the message is received, we will now poll on the
                    descriptor til more data is available.  Before we were just
                    spinning with non-blocked reads.
14. 06-DEC-99  mpb  sd2MPSsd() - Compiler did not understand logic and gave a
                    warning about not returning a value. 
15. 12-APR-00  kls  Made sd2MPSsd non-static to support calls from MPStcp.c.
*/

#include "MPSinclude.h"


/* #4 */
#ifdef THREAD_SAFE
static pthread_once_t  mps_once = PTHREAD_ONCE_INIT;
static pthread_key_t   mps_key;
#endif /* THREAD_SAFE */

/* #10 */
#if defined ( WINNT ) && defined ( _MT )
static DWORD  mps_index;
#endif /* WINNT && _MT */

/*<-------------------------------------------------------------------------
| 
|         MPS Utility Routines
V  
-------------------------------------------------------------------------->*/

/* #4 */
#ifdef THREAD_SAFE
#ifdef ANSI_C
static void  mps_free_tsd ( void* );
#else
static void  mps_free_tsd ( );
#endif /* ANSI_C */
#endif /* THREAD_SAFE */

#ifdef WINNT
typedef UCHAR bit8;
#endif

/*<-------------------------------------------------------------------------
| readHdr()
|
| Reads an MPS header.
V  
-------------------------------------------------------------------------->*/

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #1 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int readHdr ( mpssd s, UCSHdr *p_uhdr )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int readHdr(s, p_uhdr)
int s;
UCSHdr *p_uhdr;

#endif   /* ANSI_C */
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

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #1 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

int readData ( mpssd s,  caddr_t p_buf, int len )

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

int readData(s, p_buf, len )
int s;
caddr_t p_buf;
int len;

#endif   /* ANSI_C */
{
   int tlen, rlen;
   int cnt;
   struct xpollfd xpfd;

   tlen = len;
   while ( len > 0 )
   {
      /* Check for errors here */
#ifdef NO_STREAMS
      if ( ( rlen = read ( s, p_buf, len ) ) != ERROR )
#else
#ifdef   WINNT
      if ( ( rlen = recv ( s, p_buf, len, 0 ) ) != SOCKET_ERROR )
#else
      if ( ( rlen = recv ( s, p_buf, len, 0 ) ) != ERROR )
#endif   /* WINNT */
#endif /* !NO_STREAMS */
      {
         /* #3 - moved check to here (formerly followed if clause) */
         if ( rlen == 0 )
         {
            /* if recv returns 0 then connection is disconnected */
            ReturnError ( ECONNRESET );
         }

         p_buf += rlen;

         if ( ! ( len -= rlen ) )
         {
            break;
         }
         else
         {
            continue;
         }
      }

#ifdef   WINNT
      if ( rlen == SOCKET_ERROR )
      {
         errno = WSA2MPS ( WSAGetLastError ( ) );
#else
      if ( rlen == ERROR )
      {
#endif /* WINNT */
         if ( errno == EWOULDBLOCK )
         {

            /* If some data read, continue reading else return */
            if ( len < tlen )
            {
               /* #13 */
               xpfd.sd = sd2MPSsd ( s );
               xpfd.revents = 0;
               xpfd.events = POLLIN | POLLPRI;

               /* Let the system tell us when there is data. */
               cnt = MPSpoll ( &xpfd, 1, -1 );

               if ( cnt == MPS_ERROR )
               {
                  if ( ( MPSerrno != EAGAIN )      && 
                       ( MPSerrno != EWOULDBLOCK ) &&
                       ( MPSerrno != EINTR ) )
                  {
                     /* Log error, and try again. */
                     MPSperror ( "API Internal problem" );
                  }

                  continue;
               }

               /* Better only have one item ... */
               if ( cnt != 1 )
               {
                  fprintf ( stderr, "%s-%d: system is unstable\n",
                     __FILE__, __LINE__ );
                  exit ( -1 );
               }

               /* Have data, so do read. */
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

#ifdef   ANSI_C
#ifdef   DECUX_32         /* #1 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */

FreeBuf *readSave ( mpssd s, UCSHdr *p_uhdr )
{   /* need to have "{" here because this function returns a pointer. */

#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */
#else

FreeBuf *readSave(s, p_uhdr)
mpssd s;
UCSHdr *p_uhdr;
{

#endif   /* ANSI_C */

   int dlen;
#ifdef   DECUX_32         /* #1 */
#pragma   pointer_size   save
#pragma   pointer_size   long
#endif /* DECUX_32 */
   FreeBuf *p_free;
   char *p_buf;
#ifdef   DECUX_32
#pragma   pointer_size   restore
#endif   /* DECUX_32 */

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
         /* #5 */
         free ( p_free );
         return(NULL);
      }
   }

   return((FreeBuf *)p_free);
} /* end readSave() */


/* #6 */
#ifdef   WINNT
int sendmsg ( int s, const struct msghdr *msg, int flags )
/************
   senmsg ( )   ---

   Windows NT does not provide the sendmsg() socket system call.  The UconX
   MPS API uses it, and to port it as cleanly over to NT, we need this routine.

   There are two aspects to the system call sendmsg().  The first is that the
   socket does not have to be in a connected state (optional information is
   provided to connect it if it is not already).  The second is the ability to
   write from noncontiguous buffers (scatter write).

   The UconX api only takes advantage of the scatter write functionality of 
   sendmsg(), so that is what will be provided here.

   To ensure that all the data will be sent contiguously, we must put it all
   together in one contiguous space, then send that space.  Just performing
   consecutive writes for each data block will not this requirments.

   Parameters:
      s      - A descriptor identifying a connected socket.
      msg      - Holds information blocks in struct iovec part.
      flags   - Same flags as used in send() and sendto() system calls.

   Return values:
      Unless a memory malloc error occurs, this call will have the same return
      values as the routines send() and sendto().  In the  case of a memory
      alloc error, SOCKET_ERROR will still be returned, and the error code that
      can be retrieved by WSAGetLastError() will be set to ENOMEM since there
      is no equivalent WSAENOMEM value.

************/
{
   int      i, len, val;
   char   *buff_ptr, *ptr;

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
#endif   /* WINNT */

#ifdef    WINNT
int WSA2MPS ( int val )
/************
   WSA2MPS ( )   ---

   Convert an NT error number to an MPS erro number.  The WSA errors come from
   Windows Socket (TCP) conditions, The NTMPS errors come from the embedded
   card's device driver.
************/
{
   int err_num;

   switch ( val )
   {
      case WSAEBADF:
         err_num = EBADF;
         break;
      case WSAEINPROGRESS:
         err_num = EINPROGRESS;
         break;
      case WSAEALREADY:
         err_num = EALREADY;
         break;
      case WSAEISCONN:
         err_num = EISCONN;
         break;
      case WSAEINVAL:
         err_num = EINVAL;
         break;
      case WSAEWOULDBLOCK:
         err_num = EWOULDBLOCK;   
         break;
      case WSAECONNRESET:
         err_num = ECONNRESET;
         break;
      case WSAENOBUFS:
         err_num = ENOBUFS;
         break;
      case WSAEACCES:
         err_num = EACCES;
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

/* All NTMPS errors should be able to be interpreted by clearing
   out the NTMPS_STATUS bits in the error code.  So if it is not
   a WSA (NT system) error, then it must be a NT embedded error. */
#ifdef EXPLICIT_NTMPS_ERRORS
      case NTMPS_EAGAIN:
         err_num = EAGAIN;
         break;
      case NTMPS_EBADF:
         err_num = EBADF;
         break;
      case NTMPS_EBADMSG:
         err_num = EBADMSG;
         break;
      case NTMPS_EINTR:
         err_num = EINTR;
         break;
      case NTMPS_EINVAL:
         err_num = EINVAL;
         break;
      case NTMPS_EIO:
         err_num = EIO;
         break;
      case NTMPS_EMFILE:
         err_num = EMFILE;
         break;
      case NTMPS_ENFILE:
         err_num = ENFILE;
         break;
      case NTMPS_ENOBUFS:
         err_num = ENOBUFS;
         break;
      case NTMPS_ENOSR:
         err_num = ENOSR;
         break;
      case NTMPS_ENXIO:
         err_num = ENXIO;
         break;
      case NTMPS_EOPENTIMEDOUT:
         err_num = EOPENTIMEDOUT;
         break;
      case NTMPS_ECONNRESET:
         err_num = ECONNRESET;
         break;
      case NTMPS_EEXIST:
         err_num = EEXIST;
         break;
      case NTMPS_ETIMEDOUT:
         err_num = ETIMEDOUT;
         break;

      default:
         err_num = val;   /* #8 -  Let the system error take care of it. */
         break;
#else
      default:
         /* Need to get rid of the ntmps (embedded) flag bits so the eror
            can be interpreted as an MPSerrno. */
         err_num = val & ~NTMPS_STATUS;
         break;
#endif /* EXPLICIT_NTMPS_ERRORS */
   }

   return err_num;
} /* end WSA2MPS() */
#endif    /* WINNT */

/* #4 */
#ifdef THREAD_SAFE
/***
    If THREAD_SAFE is defined, then the API is compiled for the
    POSIX system interface for threads.
***/

#ifdef ANSI_C
int init_mps_thread ( void )
#else
int init_mps_thread ( )
#endif /* ANSI_C */
/****************

    In a multi-threaded program, just having MPSerrno defined as a global
    parameter will lead to undeterministic references.  To solve this,
    MPSerrno is set up to reside in the Thread-specific data (TSD) area
    of each thread.  This will allow MPSerrno to be global to only the
    thread it resides in.

    This routine will set up the TSD for MPSerrno of the thread it is
    invoked in.  

    NOTE: This routine MUST be called for each and every thread that uses
    the MPS API.  If it is not called, references to the value of MPSerrno
    will yield unexpected values, and setting MPSerrno will most likely
    cause the program to core.

****************/
{
    int  *mpserrno;
    static int  once = 0;
    static pthread_mutex_t  lock = PTHREAD_MUTEX_INITIALIZER;
 
    /* Ensure the key (mps_key) only gets created once. */
    pthread_mutex_lock ( &lock );

    if ( !once )
    {
        once = 1;

        if ( pthread_key_create ( &mps_key, mps_free_tsd ) != 0 )
        {
           return -1;
        }
    }

    pthread_mutex_unlock ( &lock );
 
    /* Get and assign space for TSD to hold MPSerrno value. */
    mpserrno = malloc ( sizeof ( int ) );
    pthread_setspecific ( mps_key, mpserrno );

    return 0;
} /* end init_mps_thread() */

#ifdef ANSI_C
static void mps_free_tsd ( void *mpserrno )
#else
static void mps_free_tsd ( mpserrno )
void *mpserrno;
#endif /* ANSI_C */
/****************

    The destructor function that is used to deallocate a thread's instance
    of it's thread-specific data item (MPSerrno) once the thread terminates.

****************/
{
    /* TSD storage area for MPSerrno. */
    free ( mpserrno );
} /* end mps_free_tsd() */
 
#ifdef ANSI_C
int *_MPSerrno ( void )
#else
int *_MPSerrno ( )
#endif /* ANSI_C */
/****************

    For the thread case, MPSerrno is defined (in mprproto.h) to be a 
    reference to the memory that the address this routine returns.  The
    address is that of the TSD for the thread-global variable MPSerrno.

    This allows for transparent setting and referencing of MPSerrno.

****************/
{
    return ( int * ) pthread_getspecific ( mps_key );
} /* end _MPSerrno() */

#endif /* THREAD_SAFE */


/* #10 */
#if defined ( WINNT ) && defined ( _MT )
/***
    Windows NT MT-Safe support for our the API.
***/

int init_mps_thread ( void )
/****************
    In a multi-threaded program, just having MPSerrno defined as a global
    parameter will lead to undeterministic references.  To solve this,
    MPSerrno is set up to reside in the Thread Local Storage (TLS).  This
    will allow MPSerrno to be global to only the thread it resides in.

    This routine will set up the API thread specific structures needed for 
    proper running in an MT-Safe environemnt.  The TLS for the MPSerrno
    value will be built.  Also the mutexes used for synchronization will be
    created.

    NOTE: This routine MUST be called for each and every thread that uses
    the MPS API.  If it is not called, references to the value of MPSerrno
    will yield unexpected values, and setting MPSerrno will most likely
    cause the program to core.

    Return Values:
    --------------
    -1  :  Error occurred.  Calling program should exit.
     0  :  All went well.
****************/
{
    int  *mpserrno;
    static int  once = 0;
    int  retval = 0;

    /* Create a mutex which will protect concurrent processing of this routine.
       If the mutex is already created when CreatMutex() is called, the
       existing objects handle is returned. */
    if ( ( MPSLockMutex = CreateMutex ( NULL, FALSE, NULL ) ) == NULL )
    {
        return -1;
    }

    if ( WaitForSingleObject ( MPSLockMutex, INFINITE ) == WAIT_FAILED )
    {
        return -1;
    }

    if ( !once )
    {
        once = 1;

        /* Create the API Global Data Mutex. */
        if ( ( MPSGlobalDataMutex = CreateMutex ( NULL, FALSE, NULL ) ) == NULL )
        {
            return -1;
        }

        /* Create the TLS index. */
        if ( mps_index = TlsAlloc ( ) == -1 )
        {
            return -1;
        }
    }

    if ( ReleaseMutex ( MPSLockMutex ) == 0 )
    {
        return -1;
    }
 
    /* Get and assign space for TLS to hold MPSerrno value. */
    if ( ( mpserrno = malloc ( sizeof ( int ) ) ) == NULL )
    {
        return -1;
    }

    if ( ! TlsSetValue ( mps_index, mpserrno ) )
    {
        return -1;
    }

    return 0;
} /* end init_mps_thread() */

void cleanup_mps_thread ( void )
/****************

    The destructor function that is used to deallocate a thread's instance
    of it's thread-specific data item (MPSerrno).  This routine must be
    called by each thread before it terminates,

****************/
{
    /* TLS storage area for MPSerrno. */
    free ( TlsGetValue ( mps_index ) );
} /* end cleanup_mps_thread() */
 
int *_MPSerrno ( void )
/****************

    For the thread case, MPSerrno is defined (in mprproto.h) to be a 
    reference to the memory that the address this routine returns.  The
    address is that of the TLS for the thread-global variable MPSerrno.

    This allows for transparent setting and referencing of MPSerrno.

****************/
{
    return ( int * ) TlsGetValue ( mps_index );
} /* end _MPSerrno() */

#endif /* WINNT && _MT */

#ifdef VXWORKS
/***
   VxWorks does not provide this function, so we need to do what our API
   expects to be done in it here.  All we care about is the IP address.
   We are rerturning the address of a statically declared structure in this
   routine (to be consistant with the real routine), so do not call this
   again before you you the information returned.  Just like the real routine,
   this one is not Thread-Safe.  Our API takes care of the multi-thread
   problem by mutex's around the code that eventually calls this routine.
***/
struct hostent *gethostbyname ( const char *name )
{
   int  h_addr;
   static struct hostent  ret_hostent;

   if ( ( h_addr = hostGetByName ( name ) ) == ERROR )
   {
      return NULL;
   }

   ret_hostent.h_addr = h_addr;
   return &ret_hostent;
} /* end gethostbyname() */

/***
   VxWorks does not provide this function, so we need to do what our API
   expects to be done in it here.  All we care about is the port number.
   We are rerturning the address of a statically declared structure in this
   routine (to be consistant with the real routine), so do not call this
   again before you you the information returned.  Just like the real routine,
   this one is not Thread-Safe.  Our API takes care of the multi-thread
   problem by mutex's around the code that eventually calls this routine.
***/
struct servent *getservbyname ( const char *name, const char *proto )
{
   int  s_port;
   static struct servent  ret_servent;

   /***
      Currently, the only way I have found in the manual to get the service
      port number is in an example where it is hard-coded.  We will do that
      until a better way is found.  Note, Wind River Technical support 
      says there is no getservbyname() equivalent routine.

      Note: the port number is stored in network byte order.
   ***/

   if ( ! strcmp ( name, "mps" ) )
   {
      if ( ! strcmp ( proto, "tcp" ) )
      {
         s_port = htons ( 48879 );
      }
      else
      {
         return NULL;
      }
   }
   else if ( ! strcmp ( name, "umps" ) )
   {
      if ( ! strcmp ( proto, "udp" ) )
      {
         s_port = htons ( 48878 );
      }
      else
      {
         return NULL;
      }
   }
   else
   {
      return NULL;
   }

   ret_servent.s_port = s_port;
   return &ret_servent;
} /* end getservbyname() */
#endif /* VXWORKS */

#ifdef VXWORKS
#include <signal.h>

extern int optind;
extern int opterr;
extern char *optarg;
extern int MPSclose ();

void init_mps_thread ( pti_TaskGlobals *ptg )
{
   int ix;

   optarg  = 0;
   optind  = 0;
   opterr  = 1;

   taskVarAdd ( 0, &MPSerrno );
   taskVarAdd ( 0, (int *)&Ptg );

   Ptg = ptg;

   /* set this task's std I/O */

   ioTaskStdSet ( 0, STD_IN,  ioTaskStdGet ( Ptg->xTID, STD_IN ) );
   ioTaskStdSet ( 0, STD_OUT, ioTaskStdGet ( Ptg->xTID, STD_OUT ) );
   ioTaskStdSet ( 0, STD_ERR, ioTaskStdGet ( Ptg->xTID, STD_ERR ) );

   signal ( SIGUSR1, MPSexit );

} /* end init_mps_thread() */

void cleanup_mps_thread ( void )
{
    pti_TaskGlobals *ptg = Ptg;
    int sd, tid = taskIdSelf();

    /* close all open streams for this task */

    for ( sd = 0; sd < MAXCONN; sd++ )
    {
      if ( tid == GetTid(sd) )
      {
        if ( StreamOpen(sd) )
          if ( MPSclose (sd) == MPS_ERROR )
            MPSperror ( "MPSclose failed for sd %d\n", sd );
      }
    }

    taskVarDelete ( 0, &MPSerrno );
    taskVarDelete ( 0, (int *)&Ptg );

    if ( ptg->tskSem )
      semGive ( ptg->tskSem ); 
} /* end cleanup_mps_thread() */
#endif /* VXWORKS */


/**********

MPSexit() ---

Instead of just exiting, clean up system (windows stuff) before exit.

**********/
#ifdef    ANSI_C
void MPSexit ( int error_val )
#else
void MPSexit ( error_val )
int    error_val;
#endif    /* ANSI_C */
{
#ifdef    WINNT
    LPSTR   lpMsg;
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

#ifdef VXWORKS
    cleanup_mps_thread ( );
#endif

    exit ( error_val );
} /* end MPSexit() */


#ifdef ANSI_C
MPSsd sd2MPSsd ( mpssd s )
#else
MPSsd sd2MPSsd ( s )
mpssd s;
#endif /* ANSI_C */
{
   /***
      Search the descriptor table for a stream that is in the open state,
      and has the streamIndex specified.  
   ***/
   MPSsd sd;

   for ( sd = 0; sd < MAXCONN; sd++ )
   {
      if ( ( xstDescriptors[sd].state == OPEN ) &&
           ( xstDescriptors[sd].socket == s ) )
      {
         return sd;
      }
   }

   if ( sd == MAXCONN )
   {
      sd = -1;  /* #14 */
   }

   return sd;   /* #14 */
} /* end sd2MPSsd() */

