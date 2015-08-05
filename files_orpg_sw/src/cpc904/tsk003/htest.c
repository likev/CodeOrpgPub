/*   @(#) htest.c 99/11/02 Version 1.30   */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1993 by
 
               +++    +++                           +++     +++
               +++    +++                           +++     +++
               +++    +++                            +++   +++ 
               +++    +++   +++++     + +    +++   +++++   +++ 
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
               +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
               +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
               +++    ++++++      +++++ ++++++++++ +++ +++++   
               +++    ++++++      +++++ ++++++++++ +++  +++    
               +++    ++++++      ++++   +++++++++ +++  +++    
               +++    ++++++                             +     
               +++    ++++++      ++++   +++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++ +++++   
               +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
               +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
                +++  +++    +++++     + +    +++   ++++++  +++ 
                ++++++++                             +++    +++
                 ++++++         Corporation         ++++    ++++
                  ++++   All the right connections  +++      +++
 

   This software is furnished  under  a  license and may be used and
   copied only  in  accordance  with  the  terms of such license and
   with the inclusion of the above copyright notice.   This software
   or any other copies thereof may not be provided or otherwise made
   available to any other person.   No title to and ownership of the
   program is hereby transferred.
 
   The information  in  this  software  is subject to change without
   notice and should not  be  considered  as  a  commitment by UconX
   Corporation.
           UconX Corporation
         San Diego, California

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/*

Modification history:
 
Chg Date       Init Description
1.  20-AUG-96  mpb  Put in function prototypes for "modern" compiler usage;
                    (ANSI_C).
2.  10-MAR-97  LMM  check for START_FAIL response
3.  22-APR-97  mpb  When we fwrite the data in rxbuf to the received file,
                    should use gdata.len for length, not pdata.len.
4.  13-OCT-97  mpb  Enhancements for embedded Windows NT driver.
5.  28-OCT-97  lmm  Removed use of tps_client.h, revised test for swap
                    module
6.  19-DEC-97  LMM  Misc cleanup of includes
7.  29-JUN-98  mpb  Have logic for when either the xmit or recv links go up
                    or down.
8.  04-AUG-98  lmm  Use actual baud rate instead of index
9.  18-AUG-98  lmm  misc code cleanup 
10. 30-oct-98  lmm  Support for data-mode only & additional encoding modes
11. 02-feb-99  lmm  Added error strings for status codes
12. 05-MAY-99  mpb  Took out globals so VxWorks can run it.
13. 06-MAY-99  mpb  VxWorks hooks.
*/
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#ifdef  VMS
#include <sys/errno.h>
#else
#include <errno.h>
#endif
 
#ifdef   WINNT    /* #4 */
typedef char * caddr_t;
#include <winsock.h>
int   getopt ( int, char *const *, const char * );
#else
#include <netinet/in.h>
#endif   /* WINNT */
#ifdef VXWORKS
IMPORT int getopt(int argc, char **argv, char *optstring);
#endif /* VXWORKS */

#include "xstopts.h"
#include "xstypes.h"
#include "xstpoll.h"
#include "mpsproto.h"
#include "hdlchdr.h"
#include "hdlcutils.h"

#ifdef WINNT      /* #4 */
#define  DEFAULT_SERVER    "\\\\.\\PTIXpqcc"
#else
#define  DEFAULT_SERVER    "/dev/ucsclone"
#endif /* WINNT */

#define  PName       argv[0]

/* #10 - added encoding param for 'n' option & 'h' for header/data mode */

#define  USAGE       \
"Usage:%s [-b baudrate] [-c controller] [-h] [-f framesize] [-m] [-n encoding]\n            [-s server] [-S service] [-v] outlink inlink sndfile rcvfile \n"
 
#define  VALIDOPTIONS   "hmvb:c:f:n:s:S:"

 
extern int  optind;
extern char *optarg;

static void exit_program ( );

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:

      main()
 
  Main loop.  Performs a file transfer between two HDLC links that are
  connected with an external loopback cable.

:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#ifdef VXWORKS
int htest ( int argc, char **argv, pti_TaskGlobals *ptg )
#else
int main ( int argc, char **argv )
#endif /* VXWORKS */
{
   struct xstrbuf pctl;
   struct xstrbuf pdata;
   struct xstrbuf gctl;
   struct xstrbuf gdata;
   char     txbuf [ MAX_DATASIZE ];
   char     rxbuf [ MAX_DATASIZE ];
   HDLC_HDR ctlbuff;
   struct xpollfd      xfd;
   HDLC_HDR            *p_hdr;  
   FILE                *outfp, *infp;
   OpenRequest         oreq;
   LCTRL               in, out;
   int                 option, gflg, count;
   int                 framesize, baud, encoding, mode, modem;
   int                 got_data, link_status;
   char                verbose;
   /* #10 - strings for encoding modes */
   char *encoding_modes[] = { "NRZ", "NRZI", "FM0", "FM1",
                              "Manchester", "Differential Manchester",
                              "NRZ (with clock)", "NRZI (with clock)" };
#ifdef   WINNT
   LPSTR    lpMsg;
   WSADATA  WSAData;
   int      optionValue = SO_SYNCHRONOUS_NONALERT;
#endif   /* WINNT */

#ifdef   WINNT
   /* For NT, need to initialize the Windows Socket DLL */
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
#endif   /* WINNT */

#ifdef VXWORKS
   init_mps_thread(ptg);
#endif /* VXWORKS */

   /* Check for minimum number of arguments */
   if ( argc < 5 )
   {
      printf ( USAGE, PName );
      exit_program ( BAD_EXIT );
   }

   /* Defaults for optional parameters */
 
   baud      = 38400;           /* #8 */
   framesize = 512;             /* frame size */
   encoding  = NRZ_MODE;        /* #10 - NRZ */
   mode      = DATA_ONLY_MODE;  /* enable data only mode */
   modem     = 0;               /* ignore modem sigs */
   verbose   = FALSE;
  
   memset ( ( char * ) &oreq, 0, sizeof ( OpenRequest ) );
   strcpy ( oreq.serverName, DEFAULT_SERVER );  /* embedded board driver */
   strcpy ( oreq.serviceName, "mps" );          /* Ignored in embedded case */
   oreq.ctlrNumber = 0;                         /* embedded board 0 */
   strcpy ( oreq.protoName, "hdlc"  );
 
   /* Extract options etc. from command line */
   /* (can be null) */

   while ( ( option = getopt ( argc, argv, VALIDOPTIONS ) ) != EOF )
   {
      switch ( option )
      {
         /* Baud Rate */
         case 'b':
            baud  = strtol ( optarg, 0, 10 );
            break;
 
         /* Controller Number */
         case 'c':
            oreq.ctlrNumber = strtol ( optarg, 0, 10 );
            break;
 
         /* #10 - Use headers with data */
         case 'h':
            mode &= ~DATA_ONLY_MODE;
            mode |= CTRL_DATA_MODE;  /* #10 - control and data mode */
            break;

         /* Frame Size */
         case 'f':
            framesize = strtol ( optarg, 0, 10 );
            if ( framesize  > MAX_DATASIZE )
            {
               printf (
                  "Transmit data size too large, setting to %d.\n",
                  MAX_DATASIZE );
               framesize = MAX_DATASIZE;
            }
            break;
 
         /* #11 - Encoding option */
         case 'n':
            encoding = strtol ( optarg, 0, 10 );     /* encoding mode */

            if ( ( encoding < 0 ) || ( encoding > MAX_ENCODING_MODE ) )
            {
               printf ("\nInvalid data encoding mode\n\n");
               printf(USAGE, PName);
               exit_program(BAD_EXIT);
            }
            break;

         /* Modem interface */
         case 'm':
            modem = 1;  	/* enable modem sigs */
            break;
 
         /* Server Name, default "/dev/ucsclone" (localhost) */
         case 's':
            strcpy ( oreq.serverName, optarg );
            break;
 
         /* Service Name, LAN only */
         case 'S':
            strcpy ( oreq.serviceName, optarg );
            break;
 
         /* Verbose mode */
         case 'v':
            verbose = 1;
            break;
 
         case '?':
         default:
            printf ( USAGE, PName );
            exit_program ( 1 );
      }
   }

   /* Check for presence of required parameters */
   if ( ( argc - optind ) < 4 ) 
   {
      printf ( USAGE, PName );
      exit_program ( BAD_EXIT );
   }

   /* Get output and input link numbers */
   out.link = atoi ( argv [ optind++ ] );
   in.link  = atoi ( argv [ optind++ ] );

   /* Open source file (file to transmit) */
   if ( ( outfp = fopen ( argv [ optind ], "r" ) ) == NULL )
   {
      printf ( "Unable to open file %s.\n", argv [ optind ] );
      exit_program ( BAD_EXIT );
   }

   /* Open destination file (file to receive) */
   if ( ( infp = fopen ( argv [ ++optind ], "w" ) ) == NULL )
   {
      printf ( "Unable to open file %s.\n", argv [ optind ] );
      exit_program ( BAD_EXIT );
   }
 
   if ( verbose )
   {
      printf ( "  Server       : %s\n", oreq.serverName );
      printf ( "  Controller   : %d\n", oreq.ctlrNumber );
      printf ( "  Protocol     : %s\n", oreq.protoName );
      printf ( "  Transmit Link: %d\n", out.link );
      printf ( "  Receive Link : %d\n", in.link );
      printf ( "  Baud rate    : %d\n", baud);
      printf ( "  Mode         : %s\n", (mode & DATA_ONLY_MODE) ?
                                        "Data Only" : "Control/Data" );
      printf ( "  Modem sigs   : %s\n", modem ? "Enabled" : "Disabled" );
      printf ( "  Encoding     : %s\n", encoding_modes[encoding]);
      printf ( "  Framesize    : %d\n", framesize);
      printf ( "  Transmit file: %s\n", argv[optind-1]);
      printf ( "  Receive  file: %s\n", argv[optind]);
   }
 
   /* Open links */
   if ( hdlc_open ( &oreq, &out, baud, encoding, framesize, mode, modem )
                   == MPS_ERROR )  /* #10 */
   {
      exit_program ( MPS_ERROR );
   }

   if ( hdlc_open ( &oreq, &in, baud, encoding, framesize, mode, modem ) 
                   == MPS_ERROR )  /* #10 */
   {
      exit_program ( MPS_ERROR );
   }

   /* Start links */
   if ( hdlc_cmd ( in.sd,  START_LINK ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   if ( hdlc_cmd ( out.sd, START_LINK ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   /* #2 - Wait for response from links indicating an UP status */
   if ( hdlc_response ( in.sd, LINK_UP, START_FAIL ) == MPS_ERROR )
   {
      printf ("Exiting due to START_FAIL on link %d\n", in.link);
      exit_program (MPS_ERROR);
   }

   if ( verbose )
   {
      printf ( "Link %d up.\n", in.link );
   }

   /* #2 - Wait for response from links indicating an UP status */
   if ( hdlc_response ( out.sd, LINK_UP, START_FAIL ) == MPS_ERROR )
   {
      printf ("Exiting due to START_FAIL on link %d\n", out.link);
      exit_program (MPS_ERROR);
   }

   if ( verbose )
   {
      printf ( "Link %d up.\n", out.link );
   }

   /* Clear Stats */
   if ( hdlc_cmd ( in.sd,  CLEAR_STAT ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   if ( hdlc_cmd ( out.sd, CLEAR_STAT ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   link_status = LINK_UP;   /* Assume up when started. */

   printf ( "Starting File Transfer.\n" );

   /* Transfer file */
   while ( ( count = fread ( txbuf, sizeof ( char ), framesize, outfp ) )
                  != ( int ) NULL )
   {
      /* #7 - If modem signals enabled, make sure link is up before
              we send data */
      if ( modem )
      {
         while ( 1 )
         {
            xfd.sd = out.sd;
            xfd.events = POLLIN | POLLPRI | POLLOUT;

            if ( MPSpoll ( &xfd, 1, -1 ) == MPS_ERROR )
            {
               MPSperror ( "MPSpoll()" );
               exit_program ( MPS_ERROR );
            }

            /* Check for information from the link first. */
            if ( ( xfd.revents & POLLIN ) || ( xfd.revents & POLLPRI ) )
            {
               gctl.maxlen    = sizeof ( HDLC_HDR );
               gctl.buf       = ( char * ) &ctlbuff;
               gdata.maxlen   = sizeof ( rxbuf );
               gdata.buf      = rxbuf;
               gflg           = 0;

               if ( MPSgetmsg ( out.sd, &gctl, &gdata, &gflg ) == MPS_ERROR )
               {
                  MPSperror ( "MPSgetmsg()" );
                  exit_program ( MPS_ERROR );
               }

               p_hdr = &ctlbuff;

               if ( p_hdr->command == LINK_DOWN )
               {
                  printf ("Xmit link Down\n");
                  link_status = LINK_DOWN;
               }
               else if ( p_hdr->command == LINK_UP )
               {
                  printf ("Xmit link Up\n");
                  link_status = LINK_UP;
               }
               else
               {
                  fprintf ( stderr, 
                     "\n\nUnexpected command (0x%x) from link !!!\n\n",
                     p_hdr->command );

                  exit_program ( -1 );
               }
            }
            else   /* We can send data, check to see if the link is up. */
            {
               if ( link_status == LINK_UP )
               {
                  break;
               }
            }
         } /* while(1) */
      } /* if modem sigs enabled */ 

      /* Send data on transmit link */
      pdata.len      = count;
      pdata.buf      = txbuf;
 
      /* #11 - If data-only mode, don't need HDLC header */
      if ( mode & DATA_ONLY_MODE )
      {
         pctl.len = 0;
         pctl.buf = 0;
      }
      else
      {
         p_hdr          = &ctlbuff;
         p_hdr->command = SEND_DATA;
         p_hdr->count   = count;
         pctl.len       = sizeof ( HDLC_HDR );
         pctl.buf       = ( char * ) p_hdr;
      }

      if ( MPSputmsg ( out.sd, &pctl, &pdata, 0 ) == MPS_ERROR )
      {
         MPSperror ( "Send text failed" );
         exit_program ( MPS_ERROR );
      }

      gctl.maxlen    = sizeof ( HDLC_HDR );
      gctl.buf       = ( char * ) &ctlbuff;
      gdata.maxlen   = sizeof ( rxbuf );
      gdata.buf      = rxbuf;
      gflg           = 0;
   
      got_data = 0;
      while ( !got_data )
      {
         if ( MPSgetmsg ( in.sd, &gctl, &gdata, &gflg ) == MPS_ERROR )
         {
            MPSperror ( "Read Error\n" );
            exit_program ( MPS_ERROR );
         }

         /* #11 - If control part received check message type */
         if ( gctl.len > 0 )
         {
            p_hdr = &ctlbuff;

            /* Check response.  We are looking for RCV_DATA if it is the 
               block we just send or LINK_DOWN if the modem signals dropped. */

            switch ( p_hdr->command )
            {
               case RCV_DATA:
               {
                  got_data = 1;
                  break;
               }

               /* #7 */
               case LINK_DOWN:
               {
                  printf ( "Recv link down\n" );
                  continue;
               }

               /* #7 */
               case LINK_UP:
               {
                  printf ( "Recv link up\n" );
                  continue;
               }

               default:
               {
                  printf ( "Non-data message, response is %x - exiting\n",
                            p_hdr->command );
                  /* Close text files */
                  fclose ( outfp );
                  fclose ( infp );

                  /* Halt links */
                  if ( hdlc_stop ( out.sd ) == MPS_ERROR )
                  {
                     exit_program(MPS_ERROR);
                  }

                  if ( hdlc_stop ( in.sd ) == MPS_ERROR )
                  {
                     exit_program(MPS_ERROR);
                  }

                  /* Close links */
                  if ( hdlc_close ( out.sd ) == MPS_ERROR )
                  {
                     exit_program(MPS_ERROR);
                  }

                  if ( hdlc_close ( in.sd ) == MPS_ERROR )
                  {
                     exit_program(MPS_ERROR);
                  }

                  exit_program ( BAD_EXIT );
               }
            } /* end switch on command type */

         } /* if control part received */

         else /* no control part, assume data received */
         {
            got_data = 1;
            break;
         }
         
      } /* while we haven't received data */

      /* Activity display, 1 dot = 1 block transferred */
      printf ( "." );
      fflush ( stdout );

      /* Write data to disk */
      if ( gdata.len > 0 )
      {
         if ( fwrite ( rxbuf, sizeof ( char ), gdata.len, infp ) 
                  == ( int ) NULL )
         {
            perror ( "Write error to receive file\n" );
            exit_program ( BAD_EXIT );
         }
      }
      else
      {
         printf ( "Null data message received\n" );
         exit_program(BAD_EXIT);
      }
   }

   printf ( "\n" );

   /* Close text files */
   fclose ( outfp );
   fclose ( infp );

   printf("\n");
   if ( hdlc_stats ( &in ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   printf("\n");
   if ( hdlc_stats ( &out ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   /* Halt links */
   if ( hdlc_stop ( out.sd ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   if ( hdlc_stop ( in.sd ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   /* Close links */
   if ( hdlc_close ( out.sd ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   if ( hdlc_close ( in.sd ) == MPS_ERROR )
   {
      exit_program(MPS_ERROR);
   }

   printf ( "File Transfer Complete.\n" );

   exit_program ( GOOD_EXIT );

   return 0;

} /* end main() */


/**********

exit_program() ---

Instead of just exiting, clean up system before exit.

**********/
#ifdef   ANSI_C
static void exit_program ( int error_val )
#else
static void exit_program ( error_val )
int   error_val;
#endif   /* ANSI_C */
{
#ifdef   WINNT
   LPSTR    lpMsg;
#endif   /* WINNT */

#ifdef   WINNT
   if ( WSACleanup ( ) == SOCKET_ERROR )
   {
      FormatMessage ( 
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL, WSAGetLastError ( ), LANG_USER_DEFAULT, 
         ( LPSTR ) &lpMsg, 0, NULL );
      fprintf ( stderr, "\n\nWSACleanup() failed:\n%s\n", lpMsg );

      LocalFree ( lpMsg );
   }
#endif   /* WINNT */

#ifdef VXWORKS
   cleanup_mps_thread();
#endif /* VXWORKS */

   exit ( error_val );

} /* end exit_program() */



