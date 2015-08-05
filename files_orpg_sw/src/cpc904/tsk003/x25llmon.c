/*   @(#) x25llmon.c 99/12/23 Version 1.7   */

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
 *  x25llmon - The UconX.25 link level monitor. Attaches to lapb protocol
 *  to provide limited link level frame analysis. Uses the UconX API
 *  nonblocking open (MPSopenS, MPSopenP) to connect to the server.
 */

/*
Modification history:
 
Chg Date       Init Description
1.  19-NOV-97  mpb  Compile for Windows NT.
2.  11-JUN-98  mpb  Push swap module if needed (little endian architecture).
3.  13-JUL-98  rjp  Fixed typo.
4.  08-OCT-99  tdg  Added VxWorks support

    22-July-2002 Chris Gilbert (Issue 1-941 CCR NA01-32404) - Add Solaris 8 support.
                 The system include files have changed. Socket.h now calls stream.h. 
                 Therefore we moved that include to the the non QNX defines.
*/

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>



#ifdef  WINNT
#define sleep(a)  Sleep((a)*1000)
typedef char *    caddr_t;
#include <winsock.h>
int    getopt ( int, char *const *, const char * );
#endif /* WINNT */

#if defined ( __hp9000s800 ) || defined ( QNX ) || defined ( WINNT )
#include <xstypes.h>
#include <xstra.h>
#else
#ifdef VXWORKS
#include <sys/socket.h>
#include <netinet/in.h>
#include <streams/stream.h>
#else
#include <sys/socket.h>
#include <sys/stream.h>
#endif /* VXWORKS */
#endif

#if  defined ( WINNT ) || defined ( QNX )
typedef unsigned short ushort;
#endif /* WINNT || QNX */
 
#include "xstopts.h"
#include "uint.h"
#include "ll_mon.h"
#include "dlpi.h"
#include "ll_proto.h"
#include "xstpoll.h"

#include "mpsproto.h"


/*
 *  local definitions
 */

/*
 *  undefine MPS_OPEN to use the api nonblocking open
 */
#define	MPS_OPEN

#define		bzero(c, l)	memset(c, 0, l)

#define		FALSE	0
#define		TRUE	1

#define		DTE	0
#define		DCE	1

#define         MAX_DATASIZE    8192

#ifdef WINNT
#define	DEFAULT_SERVER 	"\\\\.\\PTIXpqcc"
#else
#define	DEFAULT_SERVER 	"/dev/ucsclone"
#endif /* WINNT */

static int	sock, sd;


static struct xstrbuf	 control;
static char             cntlbuff   [   1024 ];
static struct xstrbuf	 data;
static char             databuff   [   8192 ];
static int		 gflg;
static char		 inbuf      [    256 ];

#define USAGE   "Usage : x25llmon [-n subnet] [-s server] [-v service] [-c controller] [-d debug]\n"

#define VALIDOPTIONS    "s:v:c:n:d"

extern	char 	*optarg ;	/* Used for parsing command line args. */

static OpenRequest      oreq;
static unsigned long	 snid = 'A';
static int		 gabby;

static int              forever=1;

extern  unsigned long snidtox25();
extern  int x25tosnid();

static int  getline ( );
static void exit_program ( );

#ifdef VXWORKS
int x25llmon (argc, argv, ptg)
#else
int main(argc,argv)
#endif /* VXWORKS */
int  argc;
char **argv;
#ifdef VXWORKS
pti_TaskGlobals *ptg;
#endif /* VXWORKS */

{
    int			c;
    int       	i, r;
    char 		*p_dat;
    dl_monitor_link_layer_t	*p_req;
    dl_monitor_link_layer_t	*p_ach;
    dl_monitor_link_layer_t	*p_test;
#ifndef WINNT
    void                  shutdown2 ( );
#endif /* WINNT */
    CAPTURE_ENTRY	*p_trace;
    uchar                ctl;
    char		str_snid[5];
#ifdef __hp9000s800
   struct fd_set wrfds;
   struct timeval t, *tp;
#endif

#ifdef	WINNT
    LPSTR    lpMsg;
    WSADATA  WSAData;
    int      optionValue = SO_SYNCHRONOUS_NONALERT;
#endif	/* WINNT */

#ifdef	WINNT
    // For NT, need to initialize the Windows Socket DLL
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
#endif	/* WINNT */

#ifdef VXWORKS
   init_mps_thread ( ptg );
#endif /* VXWORKS */

/* Defaults */
#if 0 /* #3 */
   if ( ntohl ( 0x1 ) != 1 )
   {
      fprintf ( stderr,
         "\nx25llmon does not currently run on Little Endian architecture\n" );
      exit_program ( -1 );
   }
#endif

   gabby           = 0;
   strcpy(oreq.serverName, DEFAULT_SERVER);   /* server name */
   strcpy(oreq.serviceName, "mps");     /* Ignored in embedded case */
   oreq.ctlrNumber = 0;                 /* must be zero for server */
   oreq.port       = 0;
   strcpy ( oreq.protoName, "lapb" );
   oreq.dev        = 0;			/* only one lapb device */
   oreq.flags      = O_RDWR;

    /* Extract options etc. from command line */
    /* (can be null) */

    while ((c = getopt(argc,argv,VALIDOPTIONS)) != EOF)
    {
        switch (c)
        {
            /* Subnet identifier */
            case 'n':
               snid = snidtox25(optarg);
               break;

            /* Server Name, default "/dev/ucsclone" (localhost) */
            case 's':
               strcpy(oreq.serverName, optarg);
               break;

            /* Service Name, default "mps" */
            case 'v':
               strcpy(oreq.serviceName, optarg);
               break;

            case 'd':
               gabby = 1;
               break;

            /* Controller Number */
            case 'c':
               oreq.ctlrNumber = strtol ( optarg, 0, 10 );
               break;

            case '?':
            default:
                printf(USAGE) ;
                exit_program(1);
        }
    }
    x25tosnid (snid, str_snid);

   if ( gabby )
      printf ( "Attempting to open lapb.\n" );

#ifdef MPS_OPEN
    if ( ( sd = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
       MPSperror ( "Unable to open connection to server" );
       exit_program ( ( -1 ) );
    }
#else
    if ( ( sd = MPSopenS ( &oreq ) ) == MPS_ERROR )
    {
       MPSperror ( "Unable to open connection to server" );
       exit_program ( ( -1 ) );
    }

    for ( ;; )
    {
#ifdef __hp9000s800
      FD_ZERO(&wrfds);
      FD_SET(sd, &wrfds);
      tp = (struct timeval *)&t;
      tp->tv_usec = 0;
      tp->tv_sec = 5;

      ii = select(sd+1, 0, &wrfds, 0, tp);

      if ( ii == 1 )
	 if ( FD_ISSET(sd, &wrfds) )
	    break;
      else if ( ii == 0 )
      {
         if ( gabby )
	    printf("Waiting for server to come online\n");
	 continue;
      }
      else
      {
	 MPSperror ( "Unable to open connection to server" );
	 exit_program ( ( -1 ) );
      }
#else
      pfd.fd = sd;
      pfd.revents = 0;
      pfd.events = POLLOUT;
      
      ii = poll(&pfd, 1, 5*1000);

      if ( ii == 1 )
      {
	 if ( pfd.revents & POLLERR )
	 {
	    MPSperror ( "Poll error" );
	    exit_program ( ( -1 ) );
	 }

	 if ( (pfd.revents & POLLOUT) )
	    break;
      }
      else if ( ii == 0 )
      {
	 if ( gabby )
	    printf("Waiting for server to come online\n");
	 continue;
      }
      else
      {
	 MPSperror ( "Unable to open connection to server" );
	 exit_program ( ( -1 ) );
      }
#endif
    }

    if ( ( sock = MPSopenP ( &oreq, sd ) ) == MPS_ERROR )
    {
       MPSperror ( "Unable to open connection to protocol" );
       exit_program ( ( -1 ) );
    }
#endif /* MPS_OPEN */

 
    /* #2 */
    if ( ntohl(0x1) != 0x1 )
    {
       printf("Little Endian -- pushing swap module.\n" );
 
       if ( MPSioctl ( sd, X_PUSH, "llswap" ) == MPS_ERROR )
       {
          MPSperror ( "MPSioctl swap X_PUSH" );
          exit_program ( MPS_ERROR );
       }
    }
 
    if ( gabby )
       printf ( "Sending DL_MON_ATTACH for subnet %s\n", str_snid );

    p_ach = ( dl_monitor_link_layer_t * ) cntlbuff;
    p_ach->dl_primitive = DL_MONITOR_LINK_LAYER;
    p_ach->dl_command = DL_MON_ATTACH;
    p_ach->dl_snid = snid;

    control.len            = sizeof ( dl_monitor_link_layer_t );
    control.buf            = ( char * ) p_ach;
    data.len               = 0;
    if ( MPSputmsg ( sock, &control, &data, 0 ) == ( -1 ) )
    {
       MPSperror ( "MPSputmsg DL_MON_ATTACH failed" );
       exit_program ( -1 );
    }

    if ( gabby )
       printf ( "Awaiting reply to DL_MON_ATTACH\n" );

    control.maxlen = sizeof ( cntlbuff );
    control.len    = 0;
    gflg           = 0;
    if ( MPSgetmsg ( sock, &control, &data, &gflg ) < 0 )
    {
        MPSperror ( "MPSgetmsg DL_MON_ATTACH failed" );
        exit_program ( -1 );
    }
    p_req          = ( dl_monitor_link_layer_t * ) cntlbuff;
    if ( gabby )
       printf ( "Response received to DL_MON_ATTACH\n" );
    switch ( p_req->dl_status )
    {
       case LS_SUCCESS:
          if ( gabby )
             printf ( "LS_SUCCESS received: monitor attached\n");
          break;

       case LS_FAILED:
          printf ( "LS_FAILED received: monitor not attached\n");
          exit_program ( -1 );

       default:
          printf ( "Unrecognized response from DL_MON_ATTACH\n" );
          exit_program ( -1 );
    }

/* Re-init the receive pointers */

#ifndef WINNT
#ifdef VXWORKS
    signal(SIGUSR1, shutdown2);
#else
    signal(SIGINT, shutdown2);
#endif /* VXWORKS */
#endif /* WINNT */

    printf ( "Monitoring...\n" );
    printf ( "Control-c to exit\n" );

    while ( forever )
    {
       control.buf    = cntlbuff;
       control.maxlen = sizeof ( cntlbuff  );
       bzero ( control.buf, control.maxlen );
       data.buf       = databuff;
       data.maxlen    = sizeof ( databuff );
       bzero ( data.buf, data.maxlen );
       gflg         = 0;
       if ( ( r = MPSgetmsg ( sock, &control, &data, &gflg ) ) < 0 )
       {
            MPSperror("MPSgetmsg");
            break;
       }

/* Analyse the message received */

       p_test = ( dl_monitor_link_layer_t * ) cntlbuff;
       p_dat  = databuff;

/* If no control part, then this is received data */

       switch ( p_test->dl_command )
       {
          default:
             printf ( "Unrecognized type = %d\n", p_req->dl_command );
             exit_program ( 1 );

          case DL_MON_DATA:
          {
	     int t = time (NULL);
             for ( i = 0; i < ( int ) p_test->dl_entries; ++i )
             {
		printf ( "%.2d:%.2d ", (t / 60) % 60, t % 60);
                p_trace = &p_test->dl_traces [ i ];
                if ( p_trace->direction     == INBOUND )
                   printf ( "Rx%s %d", str_snid, p_trace->address );
                else
                   printf ( "Tx%s %d", str_snid, p_trace->address );

                if ( ! ( p_trace->control & 1 ) )
                {
/* poll bit */
                   if ( p_trace->extended )
                   {
                     if ( p_trace->control2 & 0x1 )
                       printf ( "*I   %d,%d <%d>\n",
				( p_trace->control2 >> 1 ) & 0x7f,
				( p_trace->control >> 1 ) & 0x7f,
				p_trace->frame_plus.iframe_size);
                     else
                       printf ( " I   %d,%d <%d>\n",
				( p_trace->control2 >> 1 ) & 0x7f,
				( p_trace->control >> 1 ) & 0x7f,
				p_trace->frame_plus.iframe_size);
                   }
                   else
                   {
                     if ( p_trace->control & 0x10 )
                       printf ( "*I   %d,%d <%d>\n",
				( p_trace->control >> 5 ) & 0x7,
				( p_trace->control >> 1 ) & 0x7,
				p_trace->frame_plus.iframe_size);
                     else
                       printf ( " I   %d,%d <%d>\n",
				( p_trace->control >> 5 ) & 0x7,
				( p_trace->control >> 1 ) & 0x7,
				p_trace->frame_plus.iframe_size);
                   }
                 }
                 else
                 {
/* poll bit */
                    if ( p_trace->extended )
                    {
                      if(p_trace->control2 & 0x01)
                        printf ( "*" );
                      else
                        printf ( " " );
                    }
		    else
		    {
		       if ( p_trace->control & 0x10 )
			  printf ( "*" );
		       else
			  printf ( " " );
		    }

                    ctl = p_trace->control & ( ~0x10 ); /* strip poll bit */
       
                    if ( ! ( ctl & 2 ) )
                       ctl     &= 0x0f; /* S frames, mask n(r) and poll bit */
       
                    switch ( ctl )
                    {
                       default:
                          printf ( "??" );
                          break;
                       case 0x01:
                          printf ( "RR" );
                          break;
                       case 0x05:
                          printf ( "RNR" );
                          break;
                       case 0x09:
                          printf ( "REJ" );
                          break;
                       case 0x0f:
                          printf ( "DM" );
                          break;
                       case 0x2f:
                          printf ( "SABM" );
                          break;
                       case 0x6f:
                          printf ( "SABME" );
                          break;
                       case 0x83:
                          printf ( "SNRM" );
                          break;
                       case 0x87:
                          printf ( "FRMR" );
                          break;
                       case 0x43:
                          printf ( "DISC" );
                          break;
                       case 0x63:
                          printf ( "UA" );
                          break;
                       case 0x03:
                          printf ( "UI" );
                          break;
                       case 0x0d:
                          printf ( "SREJ" );
                          break;
                    }
/* poll bit */
                    if ( p_trace->extended )
                    {
/* If s frame, print the n(r) */
                      if ( ! ( ctl & 2 ) )
                        printf ( "  %d ", ( p_trace->control2 >> 1 ) & 0x7f );
                    }
		    else
		    {
/* If s frame, print the n(r) */
		       if ( ! ( ctl & 2 ) )
			  printf ( "  %d", ( p_trace->control >> 5 ) & 0x7 );
		    }
/* If frmr, print the 3 diagnostic bytes. */
                    if ( ctl    == 0x87 )
                       printf ( "%x%x%x", p_trace->frame_plus.
                                                        frmr_octets [ 0 ],
                                          p_trace->frame_plus.
                                                        frmr_octets [ 1 ],
                                          p_trace->frame_plus.
                                                        frmr_octets [ 2 ] );
                    printf ( "\n" );
                 }
             }
          }
       }
    }
#ifdef VXWORKS
    exit_program ( 0 );
#else
    return (0);
#endif /* VXWORKS */
}


/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

                getString()


 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

getString(prompt, s)
char *prompt, *s;
{
   printf("%s", prompt);
   getline ( s, FALSE );

   return 0;
}

/* This routine reads a line of input from the terminal. */

getline ( p_buffer, echo )
char    *p_buffer;
int     echo;
{
   int          i;
   char         c;

   printf ( " > " );
   for ( i = 0; i < 80; ++i )
   {
      c = getchar ( );

      switch ( c )
      {

/* Ignore spaces */

         case 0x20:
            --i;
            continue;

/* BS or DEL            */

         case 0x08:
         case 0x7f:
            if ( i )
            {
               if ( echo )
               {
                  putchar ( 0x8  );
                  putchar ( 0x20 );
                  putchar ( 0x8  );
               }
               i -= 2;                  /* Back out the char & BS/DEL */
            }
            else
               --i;                     /* Beginning of line    */
            continue;

/* CR -- This will end the input */

         case '\n':
/******
            putchar ( '\n' );
******/
            break;

/* All other chars */

         default:
            if ( echo )
               putchar ( c );
            *( p_buffer + i ) = c;
            continue;
      }
      break;
   }
   if ( i )
      *( p_buffer + i ) = ( unsigned char ) 0;   /* Null terminate */
   return ( i );
}

#ifndef WINNT
void shutdown2()
{
   dl_monitor_link_layer_t	*p_req;

   p_req = ( dl_monitor_link_layer_t * ) cntlbuff;
   p_req = ( dl_monitor_link_layer_t * ) cntlbuff;
   p_req->dl_primitive = DL_MONITOR_LINK_LAYER;
   p_req->dl_command = DL_MON_DETACH;
   p_req->dl_snid = snid;

   control.len          = sizeof ( dl_monitor_link_layer_t );
   control.buf          = ( char * ) p_req;
   data.len             = 0;
   if ( MPSputmsg ( sock, &control, 0, 0 ) == ( -1 ) )
   {
      
      MPSperror ( "MPSputmsg failed" );
      exit_program ( -1 );
   }

   if ( gabby )
      printf ( "Awaiting reply to DL_MON_DETACH\n" );

   control.maxlen = sizeof ( cntlbuff );
   control.len    = 0;
   gflg           = 0;
   if ( MPSgetmsg ( sock, &control, &data, &gflg ) < 0 )
   {
       MPSperror ( "MPSgetmsg DL_MON_DETACH failed" );
       exit_program ( -1 );
   }

   p_req          = ( dl_monitor_link_layer_t * ) cntlbuff;
   if ( gabby )
      printf ( "Response received to DL_MON_DETACH\n" );

   switch ( p_req->dl_status )
   {
      case LS_SUCCESS:
	 if ( gabby )
	    printf ( " LS_SUCCESS received: monitor detached\n");
	 break;

      case LS_FAILED:
	 printf ( " LS_FAILED received: monitor not detached\n");
	 exit_program ( -1 );

      default:
	 printf ( "Unrecognized response from DL_MON_DETACH\n" );
	 exit_program ( -1 );
   }

   printf ( "End monitor program.\n" );
   exit_program ( 0 );
}
#endif /* WINNT */


/**********

exit_program() ---

Instead of just exiting, clean up system before exit.

**********/
#ifdef    ANSI_C
static void exit_program ( int error_val )
#else
static void exit_program ( error_val )
int error_val;
#endif	/* ANSI_C */
{
#ifdef	WINNT
   LPSTR lpMsg;
#endif	/* WINNT */

#ifdef	WINNT
   if ( WSACleanup ( ) == SOCKET_ERROR )
   {
      FormatMessage ( 
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL, WSAGetLastError ( ), LANG_USER_DEFAULT, 
         ( LPSTR ) &lpMsg, 0, NULL );

      fprintf ( stderr, "\n\nWSACleanup() failed:\n%s\n", lpMsg );

      LocalFree ( lpMsg );
   }
#endif	/* WINNT */

#ifdef VXWORKS
   cleanup_mps_thread ( );
#endif /* VXWORKS */

   exit ( error_val );

} /* end exit_program() */

