/*   @(#) hdlcutils.c 99/11/02 Version 1.3   */

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
 
Chg Date    Init  Description
1.  05-MAY  mpb   Create.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#ifdef WINNT
typedef char * caddr_t;
#include <winsock.h>
#else
#include <signal.h>
#include <netinet/in.h>
#endif   /* WINNT */

#include "xstopts.h"
#include "xstypes.h"
#include "xstpoll.h"
#include "mpsproto.h"
#include "hdlchdr.h"
#include "hdlcutils.h"

/* #13 - strings for status codes */
char *hdlc_status[] =
{    "OK", 
     "Link not open", 
     "Link in use", 
     "Framesize too big",
     "No bufs available for configured framesize",
     "No modem signals for V.25 modem",
     "No signals detected",
     "Link is unavailable",
};


/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
        
      hdlc_close()

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#ifdef ANSI_C
int hdlc_close(MPSsd link)
#else
int hdlc_close(link)
MPSsd link;
#endif /* ANSI_C */
{
   struct xstrbuf gctl;
   struct xstrbuf gdata;
   char   rxbuf [ MAX_DATASIZE ];
   HDLC_HDR ctlbuff;
   HDLC_HDR *p_hdr;
   int      gflg;

   if ( hdlc_cmd(link, CLOSE_LINK) == MPS_ERROR )
   {
      return MPS_ERROR;
   }

   gctl.maxlen         = sizeof(HDLC_HDR);
   gctl.buf            = ( char * ) &ctlbuff;
   gdata.maxlen        = sizeof ( rxbuf );
   gdata.buf           = rxbuf;
   p_hdr               = &ctlbuff;
   do
   {
      gflg = 0;
      if ( MPSgetmsg ( link, &gctl, &gdata, &gflg ) == MPS_ERROR )
      {
         MPSperror("Read Error");
         return MPS_ERROR;
      }
   } while ( p_hdr->command != LINK_CLOSED );

   if ( MPSclose(link) == MPS_ERROR )
   {
      printf ( "Close failed link %d\n", link );
      MPSperror ( "MPSclose failed" );
      return MPS_ERROR;
   }

   return 0;
} /* end hdlc_close() */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
      hdlc_response()

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#ifdef ANSI_C
int hdlc_response(MPSsd d, int response, int fail)
#else
int hdlc_response(d, response, fail)
MPSsd d;
int response, fail;
#endif /* ANSI_C */
{
   struct xstrbuf gctl;
   struct xstrbuf gdata;
   char   rxbuf [ MAX_DATASIZE ];
   HDLC_HDR ctlbuff;
   HDLC_HDR *p_hdr;
   int   gflg;

   do
   {
      p_hdr        = &ctlbuff;
      gctl.maxlen  = sizeof(HDLC_HDR);
      gctl.buf     = ( char * ) &ctlbuff;
      gdata.maxlen = sizeof ( rxbuf );
      gdata.buf    = rxbuf;
      gflg         = 0;

      if ( MPSgetmsg ( d, &gctl, &gdata, &gflg ) == MPS_ERROR )
      {
         printf("Read Error\n");
         return MPS_ERROR;
      }
      
      if ( fail && p_hdr->command == fail )
      {
         /* #3 - indicate status received */
         /* #13 - output error code using status strings */
         if ( p_hdr->status <= MAX_STATUS_VAL )
         {
            printf("Error, command %x received. status=%s\n", 
                    response, hdlc_status [ p_hdr->status ] );
         }
         else
         {
            printf("Error, command %x received. status=%x\n", 
                    response, p_hdr->status );
         }

         return(MPS_ERROR);
      }
   } while ( p_hdr->command != response );

   return(0);
} /* end hdlc_response() */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
      hdlc_stats()

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#ifdef ANSI_C
int hdlc_stats(LCTRL *ln)
#else
int hdlc_stats(ln)
LCTRL *ln;
#endif /* ANSI_C */
{
   struct xstrbuf pctl;
   struct xstrbuf pdata;
   struct xstrbuf gctl;
   struct xstrbuf gdata;
   char   txbuf [ MAX_DATASIZE ];
   char   rxbuf [ MAX_DATASIZE ];
   HDLC_HDR ctlbuff;
   HDLC_HDR *p_hdr;
   LINK_STAT lstat;
   MPSsd fd, gflg;

   fd = ln->sd;

   p_hdr          = &ctlbuff;
   p_hdr->command = STAT_REQ;
   p_hdr->status = 0;
   p_hdr->count = sizeof(LINK_STAT);

   pctl.len       = sizeof(HDLC_HDR);
   pctl.buf       = ( char * )p_hdr;
   pdata.len      = sizeof(LINK_STAT);
   pdata.buf      = txbuf;

   if ( MPSputmsg ( fd, &pctl, &pdata, 0 ) == MPS_ERROR )
   {
      MPSperror("MPSputmsg failed");
      return MPS_ERROR;
   }

   /* Receive statistics from card */
   do
   {
      p_hdr        = &ctlbuff;

      gctl.maxlen  = sizeof(HDLC_HDR);
      gctl.buf     = ( char * ) &ctlbuff;

      gdata.maxlen = MAX_DATASIZE;
      gdata.buf    = rxbuf;

      gflg         = 0;

      if ( MPSgetmsg ( fd, &gctl, &gdata, &gflg ) == MPS_ERROR )
      {
         MPSperror("MPSgetmsg failed");
         return MPS_ERROR;
      }
      
   } while ( p_hdr->command != STATISTICS);

   memcpy(&lstat, rxbuf, sizeof(LINK_STAT));

   printf("                         Link %d\n", ln->link);
   printf("Good Transmits :       %5d\n", lstat.xgood);
   printf("Good Receives  :       %5d\n", lstat.rgood);
   printf("\nErrors\n");
   printf("  Xmt Underrun :       %5d\n", lstat.xunder);
   printf("  Rcv Overrun  :       %5d\n", lstat.rover);
   printf("  Count Too Big:       %5d\n", lstat.rlength);
   printf("  Bad Rcv Crc  :       %5d\n", lstat.rcrc);
   printf("  Rcv Aborts   :       %5d\n", lstat.rabt);   /* #8 */

   return 0;
} /* end hdlc_stats() */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
        
      hdlc_stop()

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#ifdef ANSI_C
int hdlc_stop(MPSsd link)
#else
int hdlc_stop(link)
MPSsd link;
#endif /* ANSI_C */
{
   struct xstrbuf gctl;
   struct xstrbuf gdata;
   char   rxbuf [ MAX_DATASIZE ];
   HDLC_HDR ctlbuff;
   HDLC_HDR *p_hdr;
   int      gflg;

   if(hdlc_cmd(link, STOP_LINK) == MPS_ERROR)
   {
      return MPS_ERROR;
   }

   gctl.maxlen         = sizeof(HDLC_HDR);
   gctl.buf            = ( char * ) &ctlbuff;
   gdata.maxlen        = sizeof ( rxbuf );
   gdata.buf           = rxbuf;
   p_hdr               = &ctlbuff;
   do
   {
      gflg = 0;
      if ( MPSgetmsg ( link, &gctl, &gdata, &gflg ) == MPS_ERROR )
      {
         MPSperror("Read Error");
         return MPS_ERROR;
      }
   } while ( p_hdr->command != LINK_STOPPED );

   return 0;
} /* end hdlc_stop() */


/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
        
      hdlc_open()

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

/* #12 - added mode parameter to this call */

#ifdef ANSI_C
int hdlc_open(OpenRequest *p_oreq, LCTRL *p_lctl,
             int baud, int encoding, int maxframe, int mode, int modem)
#else
int hdlc_open(p_oreq, p_lctl, baud, encoding, maxframe, mode, modem)
OpenRequest *p_oreq;
LCTRL *p_lctl;
int baud, encoding, maxframe, mode, modem;
#endif /* ANSI_C */
{
   struct xstrbuf pctl;
   struct xstrbuf pdata;
   struct xstrbuf gctl;
   struct xstrbuf gdata;
   char   txbuf [ MAX_DATASIZE ];
   char   rxbuf [ MAX_DATASIZE ];
   HDLC_HDR ctlbuff;
   HDLC_HDR *p_hdr;
   CONFIG *p_config;
   int gflg;

   if ( (p_lctl->sd = MPSopen(p_oreq)) == MPS_ERROR )
   {
      printf("Open link failed: 0x%x\n", MPSerrno);
      MPSperror("MPSopen failed");

      return MPS_ERROR;
   }
   
/***
    By default, the UconX Server platforms use big endian byte-ordering.
    For local hosts running with little endian byte-ordering, a swap module
    must be pushed onto the top of the server's protocol stream.  This swap
    module will provide the necessary byte-swapping for fields of the protocol
    messages exchanged between the host and the server.
***/
 
   if ( ntohl(1) != 1 )
   {
      /* push swap module on the stream */
      if ( MPSioctl( p_lctl->sd, X_PUSH, "hdlcswap" ) == MPS_ERROR )
      {
         MPSperror("MPSioctl swap X_PUSH");
         return MPS_ERROR;
      }
   }

   p_hdr               = &ctlbuff;
   p_hdr->command      = OPEN_LINK;
   p_hdr->count        = sizeof(CONFIG);
 
   p_config            = (CONFIG *)txbuf;
   p_config->link      = p_lctl->link;
   p_config->framesize = maxframe;
   p_config->baud      = baud;
   p_config->encoding  = encoding;
   p_config->mode      = mode;		/* #12 */
   p_config->modem     = modem;

   pctl.len            = sizeof(HDLC_HDR);
   pctl.buf            = ( char * )p_hdr;
   pdata.len           = sizeof(CONFIG);
   pdata.buf           = txbuf;
    
   if ( MPSputmsg ( p_lctl->sd, &pctl, &pdata, 0 ) == MPS_ERROR )
   {
      MPSperror("Open Link failed");
      return MPS_ERROR;
   }
 
   gctl.maxlen         = sizeof(HDLC_HDR);
   gctl.buf            = ( char * ) &ctlbuff;
   gdata.maxlen        = sizeof ( rxbuf );
   gdata.buf           = rxbuf;
   p_hdr               = &ctlbuff;

   do
   {
      gflg             = 0;
      if ( MPSgetmsg ( p_lctl->sd, &gctl, &gdata, &gflg ) == MPS_ERROR )
      {
         MPSperror("Read Error");
         return MPS_ERROR;
      }
      
      if ( p_hdr->command == OPEN_FAIL )
      {
         /* #13 - output error code using status strings */
         if ( p_hdr->status <= MAX_STATUS_VAL )
         {
            printf("OPEN_LINK %d failed - %s\n", 
                 p_lctl->link, hdlc_status [ p_hdr->status] );
         }
         else
         {
            printf("OPEN_LINK %d failed - %s status=%x\n", 
                 p_lctl->link, p_hdr->status );
         }

         return(MPS_ERROR);
      }
   } while ( p_hdr->command != LINK_OPEN );

   printf("Link %d open.\n", p_lctl->link);
   return 0;
} /* end hdlc_open() */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
        
      hdlc_cmd()

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/

#ifdef ANSI_C
int hdlc_cmd(MPSsd link, int command)
#else
int hdlc_cmd(link, command)
MPSsd link;
int command;
#endif /* ANSI_C */
{
   struct xstrbuf pctl;
   struct xstrbuf pdata;
   HDLC_HDR ctlbuff;
   HDLC_HDR *p_hdr;

   p_hdr          = &ctlbuff;
   p_hdr->command = command;
   p_hdr->count   = 0;
   pctl.len       = sizeof(HDLC_HDR);
   pctl.buf       = ( char * )p_hdr;
   pdata.len      = 0;
   pdata.buf      = 0;

   if ( MPSputmsg ( link, &pctl, &pdata, 0 ) == MPS_ERROR )
   {
      MPSperror("MPSputmsg failed");
      return MPS_ERROR;
   }

   return 0;
} /* end hdlc_cmd() */
