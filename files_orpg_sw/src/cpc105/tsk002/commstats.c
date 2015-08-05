/*   @(#) commstats.c 99/10/25 Version 1.24   */
 
/*******************************************************************************
                             Copyright (c) 1997 by                             

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
1.  04-mar-97  pmt  Initial version.
2.  31-mar-97  lmm  Added display of total blks for each class
3.  08-APR-97  mpb  Turned all tabs into spaces.
4.  13-OCT-97  mpb  Enhancements for embedded Windows NT driver.
5.  06-APR-98  mpb  mpsio.h is now included in winntdrv.h, so NT apps should not
                    include it on their own.
6.  22-APR-98  mpb  getopt() returns an int, not a char.  This does make a
                    a difference in QNX with the comparison of a char with
                    EOF (-1).
7.  02-JUL-98  lmm  Display max number of mbufs
8.  17-SEP-98  kls  Port to VXWORKS
9.  22-MAR-99  lmm  Added support for stream dump "-d" option 
                    Misc code cleanup
10. 30-APR-99  mpb  Modified the logic of reading in the stream dump info
                    so it works with WIndows NT read (which will get all
                    that is specified if available).
11. 25-AUG-99  mpb  "-i" will get the Device ID's of all PTI controllers
                    embedded in host.
*/

#if defined ( VMS ) || defined ( WINNT )	/* #4 */
typedef unsigned int    u_int;
int   errno;
#endif    /* VMS || WINNT */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fcntl.h>
 
#ifdef    WINNT		/* #4 */
typedef char *    caddr_t;
#include <winsock.h>
int    getopt ( int, char *const *, const char * );
#else
#include <netinet/in.h>
#endif    /* WINNT */
#ifdef VXWORKS
IMPORT int getopt(int argc, char **argv, char *optstring);
#endif /* VXWORKS */
#include "xstopts.h"
#include "xstypes.h"
#include "xstpoll.h"
#include "xconfig.h"
#include "strstat.h"
#include "report.h"
#ifndef WINNT   /* #5 */
#include "mpsio.h"
#endif /* !WINNT */
#include "mpsproto.h"

#define COMM_ALLCTLR            -1
#define MAXCMDLINESIZE          1024
#define MAXTAGLEN               512
#define MAXBOARDS               16
 
#define SUCCESS                 0
 
#ifdef WINNT		/* #4 */
#define	DEFAULT_SERVER 	"\\\\.\\PTIXpqcc"
#else
#define	DEFAULT_SERVER 	"/dev/ucsclone"
#endif /* WINNT */
#define DEFAULT_SERVICE         "mps"

#define VALID_OPTIONS         "dic:s:S:"
#define USAGE                 "\
usage: %s [-s server] [-S service] [-c ctlrnum] [-d] [-i]\n\n"

#define     strioctl        xstrioctl

#define   MAX_DATASIZE   256
char rxbuf[MAX_DATASIZE];

#define NUM_QSTATES 8 
char *qstate_table[] = { "Idle", "Sched", "RUNNING", "Runsched",
                         "Preempt", "Presched", "Sleeping", "Slpsched"};

/*
** Upstream & downstream states (embedded cards)
*/

#define FLOWING       0x00
#define DAMMED        0x01            /* Stream blocked by flow control */
#define FLOODED       (0x02 | 0x04)   /* Stream blocked no desc available */

/*
** Socket states (servers)
*/

#define TCP_FLOWING   0x00
#define TCP_DAMMED    0x01            /* Stream blocked by flow control */
#define TCP_CLOSING   0x02            /* Stream in process of closing */

/*
 * SMDATA sm_state flag definitions
 */

#define STALLOC       0x01            /* stream is allocated */
#define STWOPEN       0x02            /* waiting for 1st open */
#define IOCWAIT       0x04            /* Someone wants to do ioctl */
#define STPLEX        0x08            /* stream is being multiplexed */
#define STWCLOSE      0x10            /* Stream close is pending */
#define STCLOSING     0x20            /* Stream is in process of closing */

extern char *optarg;
extern int  optind, opterr;


#ifdef    ANSI_C
static int    comm_close     ( MPSsd );
static int    comm_open      ( char*, char*, int );
static int    comm_strdump   ( MPSsd, int, char* );
static int    comm_sysrep    ( MPSsd, int, char* );
static char   *getCtlrString ( int );
static void   exit_program   ( int );
#ifdef VXWORKS
int           commstats      ( int, char**, pti_TaskGlobals *);
#else
int           main           ( int, char** );
#endif
#else
static int    comm_close     ( );
static int    comm_open      ( );
static int    comm_strdump   ( );
static int    comm_sysrep    ( );
static char   *getCtlrString ( );
static void   exit_program   ( );
#ifdef VXWORKS
int           commstats      ( );
#else
int           main         ( );
#endif
#endif    /* ANSI_C */

/*<-------------------------------------------------------------------------
| --------------------------------------------------------------------------
||                                                                        ||
||                           commstats                                    ||
||                                                                        ||
V --------------------------------------------------------------------------
-------------------------------------------------------------------------->*/

#ifdef    ANSI_C

#ifdef  DECUX_32                        /* #1 */
#pragma pointer_size    save
#pragma pointer_size    long
#endif /* DECUX_32 */


#ifdef VXWORKS
int commstats ( int argc, char **argv, pti_TaskGlobals *ptg )
#else
int main ( int argc, char **argv )
#endif /* VXWORKS */

#ifdef  DECUX_32
#pragma pointer_size    restore
#endif  /* DECUX_32 */

#else

#ifdef VXWORKS
int commstats ( argc, argv, ptg )
#else
int main ( argc, argv )
#endif /* VXWORKS */
int   argc;
char  **argv;
#ifdef VXWORKS
pti_TaskGlobals *ptg;
#endif /* VXWORKS */

#endif
{
    int      c;   /* #6 */
    char     *service;
    int      bnum, val;
    int      stream_dump;
    MPSsd    sd;
    char     *server;
    int      getCtlrInfo = FALSE;

#ifdef    WINNT		/* #4 */
    LPSTR        lpMsg;
    WSADATA      WSAData;
    int          optionValue = SO_SYNCHRONOUS_NONALERT;
#endif    /* WINNT */

#ifdef VXWORKS
    init_mps_thread ( ptg );
#endif /* VXWORKS */

#ifdef    WINNT		/* #4 */
   /* For NT, need to initialize the Windows Socket DLL. */
    if ( WSAStartup ( 0x0101, &WSAData) )
    {
        FormatMessage ( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, WSAGetLastError ( ), LANG_USER_DEFAULT, 
            ( LPSTR ) &lpMsg, 0, NULL );
        fprintf ( stderr, "\n\nWSAStartup() failed:\n%s\n", lpMsg );

        LocalFree ( lpMsg );
        exit ( BAD_EXIT );
    }
#endif    /* WINNT */

    /* Initialize variables and structures */
    bnum     = -1;
    server   = DEFAULT_SERVER;
    service  = DEFAULT_SERVICE;
    stream_dump = FALSE;

    while ( ( c = getopt ( argc, argv, VALID_OPTIONS ) ) != EOF ) 
    {
        switch ( c ) 
        {
            case 'S':
            {
                service = optarg;
                break;
            }

            case 's':
            {
                server = optarg;
                break;
            }

            case 'c':
            {
                bnum = strtol ( optarg, 0, 0 );
                break;
            }

            case 'd':
            {
                stream_dump = TRUE;
                break;
            }

            case 'i':
            {
                getCtlrInfo = TRUE;
                break;
            }
 
            default:
            case '?':
            {
                printf ( USAGE, argv [ 0 ] );
                exit_program ( 1 );
            }
        }
    }

    /* #11 */
    /* If controller Information is requested, the server must be an
       embedded driver.  Also, since we will open to the Host driver,
       and do no referencing to the card, we will run the request
       here and then terminate. */
    if ( getCtlrInfo )
    {
       OpenRequest  oreq;
       struct xstrioctl  info_ctl;
       mps_ctlr_t  ctlrInfo;
       int  i;
       
       if ( strcmp ( server, DEFAULT_SERVER ) )
       {
          fprintf (
             stderr,
             "\n\n\t\tERROR: Controller Information is only available\n" );
          fprintf(
             stderr,
             "\t\t       for an embedded Host driver\n" );
          exit_program ( -1 );

       }

       memset ( &oreq, 0, sizeof ( OpenRequest ) );
       strcpy ( oreq.serverName, DEFAULT_SERVER );
       strcpy ( oreq.serviceName, "nothing" );
       strcpy ( oreq.protoName, "bif" );  /* Do not open to protocol. */

       if ( ( sd = MPSopen ( &oreq ) ) == MPS_ERROR )
       {
           MPSperror ( "MPSopen()" );
           exit ( -1 );
       }

       memset ( &ctlrInfo, 0, sizeof ( mps_ctlr_t ) );

       info_ctl.ic_cmd    = CTLR_INFO;
       info_ctl.ic_len    = sizeof ( mps_ctlr_t );
       info_ctl.ic_dp     = ( char * ) &ctlrInfo;
       info_ctl.ic_timout = 300;

       if ( MPSioctl ( sd, X_STR, ( caddr_t ) &info_ctl ) == MPS_ERROR ) 
       {
          MPSperror ( "MPSioctl()" );
          MPSclose ( sd );
          exit_program ( -1 );
       }

       fprintf ( stdout, "\n%d Controller%s Found -- \n\n", ctlrInfo.total,
          (ctlrInfo.total > 1) ? "s" : "");

       for ( i = 0; i < ctlrInfo.total; i++ )
       {
          fprintf (
             stdout,
            "Controller %d: %s\n",
             ctlrInfo.info[i].ctlr_num,
             getCtlrString(ctlrInfo.info[i].dev_id));
       }

       MPSclose ( sd );
       exit_program ( 0 );
    }

    /* if device is not /dev/xxx and no controller was specified,
       set board number to "host" (15) */

    /* #4 */
    if ( strncmp ( "/dev/", server, 5 ) &&
         strcmp ( "\\\\.\\PTIXpqcc", server ) &&
         ( bnum == -1 ) )
    {
       bnum = 15;
    }
    else if ( bnum == -1 )  /* if no controller was ever selected */
    {
       bnum = 0; 
    }
          
    if ( ( val = comm_open ( server, service, bnum ) ) == MPS_ERROR )
    {
        exit_program ( MPS_ERROR );
    }

    sd = ( MPSsd ) val;

    /*
    * Send the System Report request to the server or embeddded card.
    */

    if ( comm_sysrep ( sd, bnum, server ) != SUCCESS )
    {
        comm_close ( sd );
        exit_program ( MPS_ERROR );
    }

    if ( stream_dump )
    {
        if ( comm_strdump ( sd, bnum, server ) != SUCCESS )
        {
            comm_close ( sd );
            exit_program ( MPS_ERROR );
        }
    }

    comm_close ( sd );

    exit_program ( SUCCESS );
    return 0;   /* Appease the compiler (warnings). */
} /* end main() */


/*<-------------------------------------------------------------------------
| comm_open()
|
|    Open the specified UconX controller or server device.
| 
|    Return Values:
|    ------------------------------
|    Descriptor value  -- Open successful
|    MPS_ERROR         -- Error occured
V
-------------------------------------------------------------------------->*/
#ifdef  ANSI_C
 
static int comm_open ( char *serverName, char *serviceName, int bnum )
 
#else
 
static int comm_open ( serverName, serviceName, bnum )
char  *serverName;       /* Server name for LAN, device name for Local. */
char  *serviceName;      /* For LAN case. */
int   bnum;
 
#endif  /* ANSI_C */
{
    int             val;
    OpenRequest     oreq;
 
    strncpy ( oreq.serverName, serverName, MAXSERVERLEN );
    strncpy ( oreq.serviceName, serviceName, MAXSERVNAMELEN );
    strncpy ( oreq.protoName, "strhead", MAXPROTOLEN );
    oreq.port                       = 0;
 
    oreq.ctlrNumber     = bnum;
    oreq.dev            = 0;
    oreq.openTimeOut    = 10;   /* 10 seconds before timeout. */
    oreq.flags          = O_RDWR;
 
    if ( ( val = ( int ) MPSopen ( &oreq ) ) == MPS_ERROR )
    {
        MPSperror ( "comm_open() - MPSopen()" );
        return MPS_ERROR;
    }
 
    return val;
} /* end comm_open() */
 
 

/*<-------------------------------------------------------------------------
| comm_close() 
|
|    Close the specified UconX controller or server device.
| 
|    Return Values:
|    ------------------------------
|    SUCCESS   --      All went ok.
|    MPS_ERROR --      Error occured.
V
-------------------------------------------------------------------------->*/
 
#ifdef  ANSI_C
 
static int comm_close ( MPSsd sd )
 
#else
 
static int comm_close ( sd )
MPSsd   sd;
 
#endif  /* ANSI_C */
{
    if ( MPSclose ( sd ) == MPS_ERROR )
    {
        MPSperror ( "comm_close() - MPSclose()" );
        return MPS_ERROR;
    }
 
    sd = 0;
    return SUCCESS;
} /* end comm_close() */


/*<-------------------------------------------------------------------------
| comm_sysrep()
|
|    Display system report
| 
|    Return Values:
|    ------------------------------
|    SUCCESS   --      All went ok.
|    MPS_ERROR --      Error occured.
V
-------------------------------------------------------------------------->*/
 
#ifdef  ANSI_C
 
static int comm_sysrep ( MPSsd sd, int bnum, char *server )
 
#else
 
static int comm_sysrep ( sd, bnum, server )
MPSsd   sd;
int     bnum;
char    *server;
 
#endif
{
    mps_sysrep_t  rdata, *p_data;
    int           nclass[2];
    int           i, totblks;
    struct        strioctl rctl;

    rdata.bnum = htonl ( bnum );
    /* rdata.btype does not have to be defined. */
 
    rctl.ic_cmd     = SYSREP;
    rctl.ic_len     = sizeof ( mps_sysrep_t );
    rctl.ic_dp      = ( char * ) &rdata;
    rctl.ic_timout  = 300;
 
    if ( MPSioctl ( sd, ( int ) X_STR, ( caddr_t ) &rctl ) == MPS_ERROR )
    {
        MPSperror ( "comm_sysrep() - MPSioctl()" );
        fprintf ( stderr, "Unable to obtain report from board #%d\n", bnum );
        return MPS_ERROR;
    }

    p_data = &rdata;

    /* Obtained system report. Print out. */
    if (bnum == 15)
    {
        printf ( "\nSystem Report for Server: %s\n", server );
        printf ( "mbuflist:\tcurrent\t%d,\tmin\t%d,\tmax\t%d\n",
            ntohl ( p_data->mbuflist_cur ), ntohl ( p_data->mbuflist_min ),
            ntohl ( p_data->mbuflist_max ) );	/* #7 */

        printf ( "mcllist:\tcurrent\t%d,\tmin\t%d,\tmax\t%d\n",
            ntohl ( p_data->mcllist_cur ), ntohl ( p_data->mcllist_min ),
            ntohl ( p_data->mcllist_max ) );
    }
    else
    {
        printf ( "\nSystem Report for Server %s, Controller: %d\n\n",
            server, bnum );
    }
 
    /* This is the only way to get the value lined up correctly in VMS. */
    nclass[1] = p_data -> nclass;
    nclass[1] = ntohl ( nclass[1] );

    for ( i = 0, totblks = 0; i < nclass[1]; ++i )
    {
        printf ( "dblk %d (%d):\tcurrent\t%d,\tmax\t%d,\ttot\t%d,\tfail\t%d\n",
            i+1, ntohl ( p_data->dbsize[i] ), ntohl ( p_data->dblk [ i ].use ),
            ntohl ( p_data->dblk [ i ].max ), ntohl ( p_data->dbnum [ i ] ),
            ntohl ( p_data->dblk [ i ].fail ) );

        totblks += ntohl ( p_data->dbnum [ i ] );
    }

    printf ( "total dblks:\tcurrent\t%d,\tmax\t%d,\ttot\t%d,\tfail\t%d\n",
        ntohl ( p_data->dblock.use ), ntohl ( p_data->dblock.max ),
        totblks, ntohl ( p_data->dblock.fail ) );

    printf ( "total mblks:\tcurrent\t%d,\tmax\t%d,\ttot\t%d,\tfail\t%d\n",
        ntohl ( p_data->mblock.use ), ntohl ( p_data->mblock.max ),
        totblks, ntohl ( p_data->mblock.fail ) );
 
    printf ( "memory avail:\tcurrent\t%d,\tmin\t%d\n",
        ntohl ( p_data->memory_cur ), ntohl ( p_data->memory_min ) );

    return ( SUCCESS );
} /* end comm_sysrep() */


/*<-------------------------------------------------------------------------
| comm_strdump()
|
|    Dump streams information
| 
|    Return Values:
|    ------------------------------
|    SUCCESS   --      All went ok.
|    MPS_ERROR --      Error occured.
V
-------------------------------------------------------------------------->*/
 
#ifdef  ANSI_C
 
static int comm_strdump ( MPSsd sd, int bnum, char *server )
 
#else
 
static int comm_strdump ( sd, bnum, server )
MPSsd   sd;
int     bnum;
char    *server;
 
#endif
{
   mps_strdump_t rdata;
   struct        strioctl rctl;
   mps_stream_rec_t *p_stream;
   mps_queue_rec_t  *p_queue;
   bit32 *p_data;
   int eodata, lwords, qstate;
   int len, len2;
   char rq_modname[MAX_MODNAME+1];
   char wq_modname[MAX_MODNAME+1];

   rdata.bnum = htonl ( bnum );
   /* rdata.btype does not have to be defined. */
 
   rctl.ic_cmd     = STRDUMP;
   rctl.ic_len     = sizeof ( mps_strdump_t );
   rctl.ic_dp      = ( char * ) &rdata;
   rctl.ic_timout  = 300;

   if ( MPSioctl ( sd, ( int ) X_STR, ( caddr_t ) &rctl ) == MPS_ERROR )
   {
      MPSperror ( "comm_strdump() - MPSioctl()" );
      return MPS_ERROR;
   }

   /* Read stream/queue records until end of data */
   eodata = FALSE;
   while ( !eodata )
   {
      /* #10 -
         There should either be a mps_stream_rec_t, or a mps_stream_rec_t
         structure.  Since mps_stream_rec_t is smaller in size, read for
         it first.  If it turns out to be the bigger one, then read in 
         the extra bytes. */
      if ( ( len = MPSread (sd, rxbuf, sizeof ( mps_stream_rec_t )))
         == MPS_ERROR )
      {
         MPSperror ( "comm_strdump() - MPSread()" );
         return MPS_ERROR;
      }

      p_stream = ( mps_stream_rec_t * ) &rxbuf;

      switch ( ntohl (p_stream->rectype ) )
      {
         case STREAM_RECORD:
         case TCP_STREAM_RECORD:
         {
            if ( len != sizeof ( mps_stream_rec_t ) )
            { 
               printf ("Invalid stream record message size\n"); 
               printf ("Expected %d bytes - received %d bytes\n",
                          sizeof (mps_stream_rec_t), len );
               return MPS_ERROR;
            } 

            /* If little-endian host, we must swap data */
            if ( ntohl (1) != 1 )
            {
               p_data = ( bit32 * ) p_stream;
               lwords = len/4;
               while ( lwords )
               {
                  *p_data = htonl (*p_data);
                  p_data++;
                  lwords--;
               } 
            }

            if ( p_stream->rectype == TCP_STREAM_RECORD )  
            {
               printf ("\n*** STREAM %d (socket %d) ***\n", 
                  p_stream->sm_sid , p_stream->h_sid);
               printf ("    address:  %8X  shead rq:  %8X  shead wq:  %8X\n",
                  p_stream->p_smd, p_stream->sm_rq, p_stream->sm_wq);
               printf ("    reads:    %8X  writes:    %8X  state:     ",
                  p_stream->reads, p_stream->writes );
               if ( p_stream->sm_dstate == TCP_FLOWING )
                  printf ("Flowing");
               else if ( p_stream->sm_dstate == TCP_DAMMED )
                  printf ("Dammed");
               else if ( p_stream->sm_dstate == TCP_CLOSING )
                  printf ("Closing");
               else
                  printf ("%8X", p_stream->sm_dstate );

               printf ("\n    socktab:  %8X\n",
                  p_stream->sm_ustate);
               printf ("\n    Module name R/W   q_addr     q_ptr  ");
               printf ("hiwat  lowat  bytes   msgs flags state\n"); 
               break;
            }
             
            printf ("\n*** STREAM %d (host stream %d) ***\n", 
               p_stream->sm_sid , p_stream->h_sid);
            printf ("    address:  %8X  shead rq:  %8X  shead wq:  %8X\n",
               p_stream->p_smd, p_stream->sm_rq, p_stream->sm_wq);
            printf ("    reads:    %8X  rdblock:   %8X  rdunblock: %8X\n",
               p_stream->reads, p_stream->rdblock, p_stream->rdunblock);
            printf ("    writes:   %8X  wrblock:   %8X  wrunblock: %8X\n",
               p_stream->writes, p_stream->wrblock, p_stream->wrunblock);
            printf ("                        wrnodesc:  %8X  wrdescenb: %8X\n",
               p_stream->wrnodesc, p_stream->wrdescenable);

            printf ("    up state:  ");
            if ( p_stream->sm_ustate == FLOWING )
               printf ("Flowing");
            else if (p_stream->sm_ustate & DAMMED )
               printf (" Dammed");
            else if (p_stream->sm_ustate & FLOODED )
               printf ("Flooded");
            else
               printf ("%8X", p_stream->sm_ustate);

            printf ("  dn state:   ");
            if ( p_stream->sm_dstate == FLOWING )
               printf ("Flowing");
            else if (p_stream->sm_dstate & DAMMED )
               printf (" Dammed");
            else if (p_stream->sm_dstate & FLOODED )
               printf (" Flooded");
            else
               printf ("%8X", p_stream->sm_dstate);

            if ( p_stream->sm_state & ( IOCWAIT | STWCLOSE | STCLOSING ) )
               printf ("  sm_state:");
            if ( p_stream->sm_state & IOCWAIT )
               printf ("  IOCWait");
            if ( p_stream->sm_state & STWCLOSE )
               printf ("  WClose");
            if ( p_stream->sm_state & STCLOSING )
               printf ("  Closing");

            printf ("\n\n");
            printf ("    Module name R/W   q_addr     q_ptr  ");
            printf ("hiwat  lowat  bytes   msgs flags state\n"); 
            break;
         } /* end case of stream record */

         case QUEUE_RECORD:
         {
            /* #10 - Need more data to complete the sturcture. */
            if ( ( len2 = MPSread (sd, rxbuf + sizeof ( mps_stream_rec_t ),
               sizeof ( mps_queue_rec_t ) - sizeof ( mps_stream_rec_t ) ) )
               == MPS_ERROR )
            {
               MPSperror ( "comm_strdump() - MPSread()" );
               return MPS_ERROR;
            }

            len += len2;

            if ( len != sizeof ( mps_queue_rec_t ) )
            { 
               printf ("Invalid queue record message size\n"); 
               printf ("Expected %d bytes - received %d bytes\n",
                        sizeof (mps_queue_rec_t), len );
               return MPS_ERROR;
            } 
            p_queue = ( mps_queue_rec_t * ) &rxbuf;

            /* If little-endian host, we must swap data */
            if ( ntohl (1) != 1 )
            {
               strcpy ( rq_modname, p_queue->rq_modname );
               strcpy ( wq_modname, p_queue->wq_modname );
               p_data = ( bit32 * ) p_stream;
               lwords = len/4;
               while ( lwords )
               {
                  *p_data = htonl (*p_data);
                  p_data++;
                  lwords--;
               } 
               strcpy ( p_queue->rq_modname, rq_modname );
               strcpy ( p_queue->wq_modname, wq_modname );
            }

            if ( p_queue->lowermux )
               printf ("\n");

            if ( qstate = p_queue->rq_state )
            {
               for (qstate=1; qstate < NUM_QSTATES; qstate++)
               {
                  if ( p_queue->rq_state == (1 << ( qstate -1 ) ) ) 
                    break; 
               }
            }
            if ( qstate >= NUM_QSTATES )
               qstate = 0;

            printf ("    %s  R  %8X  %8X  %5X  %5X  %5X  %5X  %4X %s\n", 
              p_queue->rq_modname, p_queue->rq, p_queue->rq_ptr,
              p_queue->rq_hiwat, p_queue->rq_lowat, p_queue->rq_count,
              p_queue->rq_size, p_queue->rq_flag, qstate_table[qstate] );

            if ( qstate = p_queue->wq_state )
            {
               for (qstate=1; qstate < NUM_QSTATES; qstate++)
               {
                  if ( p_queue->wq_state == (1 << ( qstate -1 ) ) ) 
                    break; 
               }
            }
            if ( qstate >= NUM_QSTATES )
               qstate = 0;
            
            printf ("    %s  W  %8X  %8X  %5X  %5X  %5X  %5X  %4X %s\n", 
              p_queue->wq_modname, p_queue->wq, p_queue->wq_ptr,
              p_queue->wq_hiwat, p_queue->wq_lowat, p_queue->wq_count,
              p_queue->wq_size, p_queue->wq_flag, qstate_table[qstate] );

            break;
         } /* end case of stream record */

         case EOD_RECORD:
         {
            printf ("\n*** End of data ***\n");
            eodata = TRUE;
            break;
         }

         default:
         {
            printf ("Invalid record type rcvd = %x\n", p_stream->rectype); 
            return MPS_ERROR;
         }

      } /* end switch on record type */ 

   }  /* do until end of data */ 

   return 0;

} /* end comm_strdump */

/*<-------------------------------------------------------------------------
| getCtlrString()
|
|    Retrieves an information string given the devID
|    of the controller.
|
|    Return Values:
|    ------------------------------
|    SUCCESS   --      Information string for the controller.
V
-------------------------------------------------------------------------->*/

static char *getCtlrString(int devId)
{
   static char  buff[128]; /* We return this, make sure it is static. */
   char  devstr[16];

   sprintf(devstr, " (0x%04x)", devId);

   switch(devId)
   {
      case 0x0334:
      {
         strcpy(buff, "PT-PCI334");
         break;
      }

      case 0xC334:
      {
         strcpy(buff, "PT-CPC334");
         break;
      }

      case 0xD334:
      {
         strcpy(buff, "PT-PMC334");
         break;
      }

      case 0xC340:
      {
         strcpy(buff, "PT-CPC340");
         break;
      }

      case 0x0370:
      {
         strcpy(buff, "PT-PCI370");
         break;
      }

      case 0x0372:
      {
         strcpy(buff, "PT-PCI372");
         break;
      }

      case 0x3701:
      {
         strcpy(buff, "PT-PCI370PQ");
         break;
      }

      case 0x3721:
      {
         strcpy(buff, "PT-PCI372PQ");
         break;
      }

      case 0x3702:
      {
         strcpy(buff, "PT-CPC370-6U");
         break;
      }

      case 0x3722:
      {
         strcpy(buff, "PT-CPC372-6U");
         break;
      }

      case 0x3703:
      {
         strcpy(buff, "PT-CPC370-3U");
         break;
      }

      case 0x3723:
      {
         strcpy(buff, "PT-CPC372-3U");
         break;
      }

      case 0x3704:
      {
         strcpy(buff, "PT-PMC370");
         break;
      }

      case 0x3724:
      {
         strcpy(buff, "PT-PMC372");
         break;
      }

      case 0x3480:
      {
         strcpy(buff, "PT-CPC348");
         break;
      }

      case 0x3802:
      {
         strcpy(buff, "PT-CPC380");
         break;
      }

      case 0x3822:
      {
         strcpy(buff, "PT-CPC382");
         break;
      }

      case 0x3440:
      {
         strcpy(buff, "PT-PCI344");
         break;
      }

      default:
      {
         strcpy(buff, "Unknown Device");
         break;
      }
   }

   strcat(buff, devstr);

   return buff;

} /* end getCtlrString() */


/**********

exit_program() ---

Instead of just exiting, clean up system (windows stuff) before exit.

**********/
#ifdef    ANSI_C
static void exit_program ( int error_val )
#else
static void exit_program ( error_val )
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
} /* end exit_program() */

