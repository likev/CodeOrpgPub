/*   @(#) x25pvcsnd.c 99/11/03 Version 1.11   */
/*******************************************************************************
                             Copyright (c) 1996 by
 
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
1.   7-NOV-96  rjp  Original code.
2.  21-JAN-97  hhn  Removed return from main routine to avoid SunPro 
                    compiler warnings
3.  24-JAN-97  hhn  Added support for controller #
4.  24-JAN-97  lmm  Make default compiler non-ANSI
5.  28-jan-97  pmt  make program manly. does multiple PVCs. yow.
6.  23-may-97  lmm  include sys/errno.h and tps_client.h
7.  11-jul-97  hhn  remove include poll.H 
8.  29-oct-97  lmm  remove use of tps_client.h, added WINNT support
9.  03-nov-97  mpb  added WINNT ifdefs
10.  9-jul-98  rjp  mpb's Push swap module if needed (little endian
                    architecture).
11. 20-jan-99  lmm  Added windowing feature for xmit 
12. 08-oct-99  tdg  Added VxWorks support
13. 03-nov-99  lmm  Don't timeout ioctl calls
*/

/************************************************************************
*
* x25pvcsnd.c
*
* Offer a menu to attach to a pvc, reset the lci, send a file and then detach.
*
************************************************************************/

#define MAX_CTL_SIZ   1000
#define CUDFLEN       4
#define MAX_DAT_SIZ   4096
#define DEF_DAT_SIZ   128
#define FALSE         0
#define MAXLIS        512
#define TRUE          1
#define WINDOW_SIZE   256		/* #11 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifndef WINNT
#include <unistd.h>
#endif /* !WINNT */
#include <sys/types.h>
#ifdef VXWORKS
#include <in.h>
#endif /* VXWORKS */

#ifdef  WINNT
typedef char *    caddr_t;
#include <winsock.h>
int    getopt ( int, char *const *, const char * );
#endif /* !WINNT */

#include "xstopts.h" 
#include "xstypes.h"
#include "xstpoll.h"
#include "mpsproto.h"
#include "xstra.h"
#include "x25_proto.h"
#include "x25_control.h"  /* #11 */
#include "xnetdb.h"
#include "streamio.h"     /* #11 */

#define VALIDOPTIONS    "c:l:n:o:p:s:tv:"
#define USAGE "\
Usage: %s [-c subnetid] [-l starting lci] [-n number of PVCs]\n\
                 [-p pktsize] [-t] [-o controller] [-s server] [-v service]\n"

#define PName   argv[0]

#ifdef WINNT
#define   DEFAULT_SERVER    "\\\\.\\PTIXpqcc"
#else
#define   DEFAULT_SERVER    "/dev/ucsclone"
#endif /* WINNT */

int   tracing;
int   snid;          /* #11 */
struct xstrioctl io; /* #11 */

#ifdef ANSI_C
static void  attach (int, char *, int);
static void  detach (int, char *);
static void  exit_program ( int );
static void  fileSend (int);
static int   get_lci_stats(int, int);
static int   get_stats (int);
static int   msgDisplay (struct xstrbuf *, struct xstrbuf *);
static void  resetSend  (int);
static int   zero_stats (int);
#else
static void  attach ();
static void  detach ();
static void  exit_program ( );
static void  fileSend ();
static int   get_lci_stats();
static int   get_stats ();
static int   msgDisplay ();
static void  resetSend  ();
static int   zero_stats ();
#endif

/* 
* Declare external for getopt
*/
extern char *optarg;
extern int  optind;
/*
* End of getopt.
*/
static int numPVCs, starting_lci;
static int x25Fds[256];


/************************************************************************
*
* main
*
* Read command line options, then offer menu.
*
************************************************************************/
int
#ifdef VXWORKS
x25pvcsnd (argc, argv, ptg)
#else
main (argc, argv)
#endif /* VXWORKS */
int  argc;
char *argv [];
#ifdef VXWORKS
pti_TaskGlobals *ptg;
#endif /* VXWORKS */
{
   int       c;
   char      inbuf [256];
   OpenRequest      oreqX25;
   int       packetSize;
   char      *pProto;
   char      *pServer;
   char      *pService;
   char      *pSnid;
   int       lci, i;
   int       forever = TRUE;

#ifdef  WINNT
   LPSTR    lpMsg;
   WSADATA  WSAData;
   int      optionValue = SO_SYNCHRONOUS_NONALERT;
#endif  /* WINNT */
 
#ifdef  WINNT
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
#endif  /* WINNT */

#ifdef VXWORKS
   init_mps_thread ( ptg );
#endif /* VXWORKS */

   /*
    * Set the defaults.
    */
   tracing    = 0;          /* Don't trace events      */
   starting_lci   = 1;      /* Starting Logical channel   */
   numPVCs    = 1;          /* modest test         */
   pSnid      = "B";        /* Subnet id         */
   pProto     = "x25";
   pServer    = DEFAULT_SERVER;   /* Embedded board not lan   */
   pService   = "mps";      /* Ignored in imbedded board case */
   packetSize = DEF_DAT_SIZ;
   /*
    * Init open request data area with defaults.
    */
   memset (&oreqX25, 0, sizeof (oreqX25));
   oreqX25.dev  = 0;
   oreqX25.port = 0;
   oreqX25.ctlrNumber = 0;
   oreqX25.flags = CLONEOPEN;
   strcpy (oreqX25.serverName, pServer);
   strcpy (oreqX25.serviceName, pService);
   strcpy (oreqX25.protoName, pProto);

   /*
    * Get the options from command line
    */

   while ((c = getopt (argc, argv, VALIDOPTIONS)) != EOF)
   {
      switch (c)
      {
         case 'c':
            pSnid = optarg;      /* Set sub net id (link)   */
            break;

         case 'l':
            starting_lci = strtol (optarg, 0, 10); /* Start Logical channel */
            break;

         case 'n':
            numPVCs = strtol (optarg, 0, 10); /* Number of PVCs      */
            break;

         case 'o':
            oreqX25.ctlrNumber = strtol (optarg, 0, 10 ); /* Controller no. */
            break;
 
         case 'p':
            packetSize = strtol (optarg, 0, 10); /* Packet size      */
            if (packetSize > MAX_DAT_SIZ) /* If too big         */
            {
               printf (
                 "Packet size of %d is too big. Please use value <= %d.\n", 
                  packetSize, MAX_DAT_SIZ);
               packetSize = DEF_DAT_SIZ;
            }
            break;

         case 's':
            strcpy (oreqX25.serverName, optarg); /* Server name      */
            break;

         case 't':          
            tracing = 1;      /* Trace activity      */
            break;

         case 'v':
            strcpy (oreqX25.serviceName, optarg); /* Service name. LAN only */
            break;

         case '?':
         default:
            printf (USAGE, PName) ;
            exit_program (1);
      }

   } /* while options remain */

   if (tracing == 1)
      printf ("Open stream(s) for file send.\n");

   /* #11 - Convert subnetwork ID to binary value */ 
   snid = snidtox25 (pSnid);

   /* Open connection for each PVC, and push swap module if necessary */
   for (i=0; i<numPVCs; i++)
   {
      x25Fds[i] = MPSopen (&oreqX25);
      if (x25Fds[i] < 0)
      {
         MPSperror ("MPSopen failed. Unable to connect to server.");
         exit_program ( MPS_ERROR );
      }

      /* #10 */
      if ( ntohl(0x1) != 0x1 )
      {
         if (tracing == 1)
             printf("Little Endian -- pushing swap module.\n" );

         if ( MPSioctl ( x25Fds[i], X_PUSH, "x25swap" ) == MPS_ERROR )
         {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( MPS_ERROR );
         }
      }

      /* #11 - Send clear stats request */
      zero_stats ( x25Fds[i] );

   } /* end of opening connections */

   printf ("\n");
   printf ("The usual sequence is: \n\n");
   printf ("  A - attach to pvc\n");
   printf ("  R - reset circuit\n");
   printf ("  T - send file\n");
   printf ("  D - detach from pvc\n");
   printf ("  Q - close and exit\n\n");
   printf ("In order for this to work, line 4, in both DTE and DCE x25\n");
   printf ("template files must be greater than or equal to 001, and line 5\n");
   printf ("must be greater than or equal to line 4. In addition, the link\n");
   printf ("associated with the subnet id %s (used here) must\n", pSnid);
   printf ("be connected to the link associated with the subnet id used\n");
   printf ("in the x25pvcrcv process.\n");

   /*
    * prompt for what to do next
    */
   while ( forever )
   {
      printf("\n\n\n\n"                                     );
      printf("                   M E N U \n"                );
      printf("A Attach To PVC(s)   R Send Reset Request\n"  );
      printf("T Send File          D Detach From PVC(s)\n"  );
      printf("Q Quit Program\n"                             );
      printf("Option ? "                                    );
 
      if (gets (inbuf))
      {
         switch (toupper (*inbuf))
         {
            case 'A':      /* Attach          */
               printf ("\n");
               lci = starting_lci;
               for (i=0; i<numPVCs; i++)
               {   
                  attach (x25Fds[i], pSnid, lci);
                  lci++;
               }   
               break;

            case 'R':      /* Send reset request      */
               printf ("\n");
               for (i=0; i<numPVCs; i++)
               {   
                  resetSend (x25Fds[i]);
               }   
               break;
      
            case 'T':      /* Send file         */
               printf ("\n");
               fileSend (packetSize);
               break;

            case 'D':      /* Send detach from pvc      */
               printf ("\n");
               lci = starting_lci;
               for (i=0; i<numPVCs; i++)
               {   
                  if (tracing == 1)
                     printf ("Send detach lci %d\n", lci);
                  detach (x25Fds[i], pSnid);
                  lci++;
               }   
               break;
 
            case 'Q':      /* Quit            */
               for (i=0; i<numPVCs; i++)
               {   
                  if (MPSclose (x25Fds[i]) != 0)
                  {
                     MPSperror ("MPSclose failed");
                     exit_program (1);
                  }
               }   
               exit_program (0);

            default:
               printf ("?\n");
         }
      }
      else
        exit_program (0);         /* EOF */

   } /* do forever */

   return(0);
}

/**************************************************************************
* 
* msgDisplay
*
*
***************************************************************************/
int
msgDisplay (pCtlblk, pDatblk)
struct xstrbuf   *pCtlblk;
struct xstrbuf   *pDatblk;
{
   int    i;
   int    nDI;
   struct xdataf *pData;
   char   *pDatbuf;
   S_X25_HDR *pHdr;

   nDI = 0;            /* No disconnect indication   */
   pHdr = (S_X25_HDR *) &pCtlblk->buf [0];

   if (pHdr->xl_type == XL_CTL)
   {
      printf ("control msg\n");
   }

   if (pHdr->xl_type == XL_DAT)
   {
      printf ("data msg\n");
   }

   switch (pHdr->xl_command)
   {
 
      case N_RC:
         printf ("Read N_RC:\n");
         break;
 
      case N_CI:
         printf ("Read N_CI:\n");
         break;
 
      case N_CC:
         printf ("Read N_CC:\n");
         break;
 
      case N_Data:
         printf ("Read N_Data:\n");
         pData = (struct xdataf *) &pCtlblk->buf [0];       
         if (pData->More)
            printf ("M-bit on\n");
         if (pData->setQbit)
            printf ("Q-bit on\n");
         if (pData->setDbit)
            printf ("D-bit set\n");
         if (pDatblk->len > 0)
         {
            pDatbuf = &pDatblk->buf [0];
            for (i=0; i<pDatblk->len; i++)
               printf ("%x", pDatbuf [i]);
            printf ("\n");
         }
         break;
 
      case N_DAck:
         printf ("Read N_DAck:\n");
         break;
 
      case N_EData:
         printf ("Read N_EData:\n");
         break;
 
      case N_EAck:
         printf ("Read N_EAck:\n");
         break;
 
      case N_RI:
         printf ("Read N_RI:\n");
         break;
 
      case N_DI:
         printf ("Read N_DI:\n");
         nDI = 1;
         break;
 
      case N_DC:
         printf ("Read N_DC:\n");
         break;
 
      default:
         printf ("?\n");
   }
   return (nDI);
}

/***********************************************************************
*
* attach
*
* Send an attach to server and wait for response. Server tests validity
* and returns a message indicating result of test.
*
************************************************************************/

void
attach  (x25Fd, pSnid, lci)
int  x25Fd;
char *pSnid;
int  lci;      /* Logical channel       */
{
   int    answer;
   struct xstrbuf ctlblk;
   char   ctlbuf [MAX_CTL_SIZ];
   struct xstrbuf datblk;
   char   datbuf [MAX_DAT_SIZ];
   int    flags;
   struct pvcattf attach;
   struct pvcattf *pAttach;

   /*
    * Init attach for server. 
    */
   memset (&attach, 0, sizeof (attach));
   attach.xl_type       = XL_CTL;
   attach.xl_command    = N_PVC_ATTACH;
   attach.sn_id         = snidtox25 (pSnid);
   attach.lci           = lci;
   attach.reqackservice = 0;
   attach.reqnsdulimit  = 0;
   attach.nsdulimit     = 0;

   ctlblk.len    = sizeof (struct pvcattf);
   ctlblk.buf    = (char *) &attach;

   if (tracing == 1)
      printf ("Send attach snid %s, lci %d\n", pSnid, lci);

   if (MPSputmsg (x25Fd, &ctlblk, 0, 0) != 0)
   {
      MPSperror ("MPSputmsg for attach failed");
      exit_program ( MPS_ERROR );
   }
   /*
    * Read attach outcome. 
    */
   ctlblk.buf    = ctlbuf;
   ctlblk.maxlen = MAX_CTL_SIZ;
   datblk.buf    = datbuf;
   datblk.maxlen = MAX_DAT_SIZ;
   pAttach       = (struct pvcattf *) ctlbuf;
   flags         = 0;
   answer        = MPSgetmsg (x25Fd, &ctlblk, &datblk, &flags);

   if (answer < 0)
   {
      MPSperror ("MPSgetmsg for attach response failed");
      exit_program ( MPS_ERROR );
   }

   if (pAttach->xl_type == XL_CTL   /* If a control message      */
     &&  pAttach->xl_command == N_PVC_ATTACH) /* and attach response   */
   {
      if (pAttach->result_code != PVC_SUCCESS)
      {
         printf ("Attach rejected (%d).\n", pAttach->result_code);
             
         switch (pAttach->result_code)
         {
            case PVC_NOSUCHSUBNET:
               printf ("Subnetwork not configured\n");
               break;
   
            case PVC_CFGERROR:
               printf ("LCI not in range, no PVCs\n");
               break;

            case PVC_RMTERROR:
               printf ("No response from remote\n");
               break;
         }
         MPSperror ("MPSgetmsg for attach indicates invalid parameters");
         exit_program ( MPS_ERROR );
      }
   }
   else
   {
      printf ("MPSgetmsg for Attach returned invalid attach response.");
      msgDisplay (&ctlblk, &datblk);
      exit_program ( MPS_ERROR );
   }
}

/***********************************************************************
*
* detach
*
* Send a detach to server and wait for response. Server tests validity
* and returns a message indicating result of test.
*
************************************************************************/

void
detach  (x25Fd, pSnid)
int     x25Fd;
char    *pSnid;
{
   int    answer;
   struct xstrbuf ctlblk;
   char   ctlbuf [MAX_CTL_SIZ];
   struct xstrbuf datblk;
   char   datbuf [MAX_DAT_SIZ];
   int    flags;
   struct pvcdetf detach;
   struct pvcdetf *pDetach;

   /*
    * Init detach. 
    */
   memset (&detach, 0, sizeof (detach));
   detach.xl_type    = XL_CTL;
   detach.xl_command = N_PVC_DETACH;

   ctlblk.len    = sizeof (struct pvcdetf);
   ctlblk.buf    = (char *) &detach;
   if (MPSputmsg (x25Fd, &ctlblk, 0, 0) != 0)
   {
      MPSperror ("MPSputmsg for detach failed");
      exit_program ( MPS_ERROR );
   }

   /*
    * Read detach outcome. 
   */
   ctlblk.buf    = ctlbuf;
   ctlblk.maxlen = MAX_CTL_SIZ;
   datblk.buf    = datbuf;
   datblk.maxlen = MAX_DAT_SIZ;
   pDetach       = (struct pvcdetf *) ctlbuf;
   flags         = 0;
   answer        = MPSgetmsg (x25Fd, &ctlblk, &datblk, &flags);

   if (answer < 0)
   {
      MPSperror ("MPSgetmsg for detach response failed");
      exit_program ( MPS_ERROR );
   }

   if (pDetach->xl_type == XL_CTL   /* If a control message      */
     && (pDetach->xl_command == N_PVC_DETACH  /* and either detach response */
     ||  pDetach->xl_command == N_RI)) /* or other side detached already */
   {
      if (pDetach->reason_code != PVC_SUCCESS)
      {
         printf ("Detach rejected (%d).\n", pDetach->reason_code);
               MPSperror ("MPSgetmsg for Detach returned invalid parameters");
               exit_program ( MPS_ERROR );
      }
   }
   else
   {
      MPSperror ("MPSgetmsg for Detach returned invalid detach response.");
      msgDisplay (&ctlblk, &datblk);
      exit_program ( MPS_ERROR );
   }
   if (tracing == 1)
       printf ("Server accepted detach.\n");
}


/***********************************************************************
*
* resetSend
*
* Send a reset to server and wait for response. Server tests validity
* and returns a message indicating result of test.
*
************************************************************************/

void
resetSend  (x25Fd)
int         x25Fd;
{
   int    answer;
   struct xstrbuf ctlblk;
   char   ctlbuf [MAX_CTL_SIZ];
   struct xstrbuf datblk;
   char   datbuf [MAX_DAT_SIZ];
   int    flags;
   struct xrstf   reset;
   struct xrscf   *pConfirm;

   /*
    * Init reset. 
    */
   memset (&reset, 0, sizeof (reset));
   reset.xl_type    = XL_CTL;
   reset.xl_command  = N_RI;

   ctlblk.len     = sizeof (reset);
   ctlblk.buf     = (char *) &reset;

   if (MPSputmsg (x25Fd, &ctlblk, 0, 0) != 0)
   {
      MPSperror ("MPSputmsg for reset failed");
      exit_program ( MPS_ERROR );
   }

   /*
    * Read reset outcome. 
    */
   ctlblk.buf    = ctlbuf;
   ctlblk.maxlen = MAX_CTL_SIZ;
   datblk.buf    = datbuf;
   datblk.maxlen = MAX_DAT_SIZ;
   pConfirm      = (struct xrscf*) ctlbuf;
   flags         = 0;
   answer        = MPSgetmsg (x25Fd, &ctlblk, &datblk, &flags);

   if (answer < 0)
   {
      MPSperror ("MPSgetmsg for reset response failed");
      exit_program ( MPS_ERROR );
   }

   if (  (pConfirm->xl_type == XL_CTL)   /* If a control message      */
      && (pConfirm->xl_command == N_RC   /* and reset confirm         */
      ||  pConfirm->xl_command == N_RI)) /* or indication (collision) */
   {
      return;
   }
   else
   {
      MPSperror ("MPSgetmsg for reset returned invalid reset response.");
      msgDisplay (&ctlblk, &datblk);
      exit_program ( MPS_ERROR );
   }
}



/************************************************************************
*
* fileSend
*
*
************************************************************************/
void
fileSend (packetSize)
int packetSize;
{
   struct xstrbuf ctlblk;
   struct xdataf  data;
   struct xstrbuf datblk;
   char   datbuf [MAX_DAT_SIZ];
   char   fileName [256];
   FILE   *outFp;
   char   *pAnswer;
   int    i, msg_sent;
   int    send_count=0;
   int    stat_count;
   int    xmit_window;

   datblk.buf      = datbuf;

   /*
    * Get name of source file and open it.
    */
    
   outFp        = (FILE *) NULL;
   do
   {
      printf ("Name of transmit file: ");
      pAnswer = gets (fileName);
      if (pAnswer == (char *) NULL)   /* If nothing there             */
      {
         perror ("");
         return;
      }
      if (pAnswer == (char *) EOF)    /* If end of file               */
         return;
      if (strlen (fileName) == 0)      /* If nothing there             */
         return;
      outFp = fopen (fileName, "r");
      if (outFp == (FILE *) NULL )
      {
         printf("Unable to open file %s.\n", fileName);
      }
   } while (outFp == (FILE *) NULL);
 
   if (tracing == 1)
      printf ("Opened %s.\n", fileName);
 
   printf("Starting File Transfer.\n");

   memset (&data, 0, sizeof (data));
   data.xl_type    = XL_DAT;
   data.xl_command = N_Data;
   data.More       = FALSE;
   data.setQbit    = FALSE;
   data.setDbit    = FALSE;

   xmit_window = WINDOW_SIZE*2;

   /* Transfer file */
   while (1)
   {
      if ( xmit_window ) /* if xmit window open */
      {
         ctlblk.len = sizeof (data);
         ctlblk.buf = (char *) &data;
         datblk.len = fread (datbuf, sizeof(char), packetSize, outFp);
         if (!datblk.len) 
            break;

         for (i=0; i<numPVCs; i++)
         {
            /* Send data on transmit link */
            msg_sent = 0;
            while (!msg_sent)
            {
               if ( MPSputmsg ( x25Fds[i], &ctlblk, &datblk, 0 ) < 0 )
               {
                  if( (MPSerrno  != EAGAIN) && (MPSerrno != EWOULDBLOCK ))
                  {
                     MPSperror("MPSputmsg");
                     exit_program ( MPS_ERROR );
                  }
                  continue;
               }
               else 
               {
                  /* Activity display, 1 dot = 1 block transferred */
                  printf("."); 
                  fflush(stdout);  
                  msg_sent = 1;

                  /* Update send cont and decrement xmit window */
                  send_count++;
                  if ( xmit_window > 0 )
                     xmit_window--;
               }
            }  /* while not sent */

         } /* end send for all pvcs */

      } /* if xmit not blocked */

      else /* xmit blocked */
      {
         stat_count = get_stats ( x25Fds[0] );
         if ( ( send_count - stat_count) < WINDOW_SIZE )
         {
            xmit_window = WINDOW_SIZE;
         }
      }
   }

   fclose (outFp);
   printf ("\nFile Transfer Complete.\n");

   for (i=0; i<numPVCs; i++)
   {
      if ( MPSioctl ( x25Fds[i], X_SETIOTYPE, BLOCK ) == ERROR )
      {
         printf("Unable to set BLOCK.\n");
         MPSperror("MPSioctl");
         exit_program   ( MPS_ERROR );
      }
   }
}

/************************************************************************
*
* get_stats - get number of messages transmitted for subnetwork
*
************************************************************************/
int get_stats ( fd )
int fd;
{
   struct persnidstats pss;

   /* Set up get SNID stats request */
   pss.snid = snid;
   io.ic_cmd=N_getSNIDstats;
   io.ic_timout=-1;			/* #13 */
   io.ic_len=sizeof(pss);
   io.ic_dp=(char *)&pss;
 
   /* issue get stats request */
   if (S_IOCTL(fd, I_STR,(char *) &io) < 0)
   {
      perror("x25pvcsnd :getSNIDstats IOCTL failed\n");
      exit_program ( MPS_ERROR );
   }
   return ( pss.mon_array [dt_out_s] );
}

/************************************************************************
*
* get_lci_stats - get number of messages transmitted for lci on subnetwork
*
************************************************************************/
int get_lci_stats ( fd, lci )
int fd;
int lci;
{
   struct xstrioctl io;
   struct vcstatusf infoback;
   struct vcinfo    *bufp;

   /* Set up stats request */
   io.ic_cmd=N_getVCstatus;
   io.ic_timout=-1;			/* #13 */
   io.ic_len=sizeof(infoback);
   io.ic_dp=(char *) &infoback;
   infoback.first_ent = 0;
   infoback.num_ent = 1;
 
   memset (&infoback.vc, 0, sizeof (struct vcinfo));
   infoback.vc.xu_ident = snid;
   infoback.vc.lci = lci;

   /* Issue stats request */
   if (S_IOCTL(fd,X_STR, (char *) &io)<0)
   {
      perror("x25pvcsnd:get_VC_data. IOCTL failed\n");
      exit_program( MPS_ERROR );
   }

   /* Return result */
   bufp = &(infoback.vc);
   return ( bufp->perVC_stats[dt_out_v] );
}


/************************************************************************
*
* zero_stats - clear subnetwork statistics
*
************************************************************************/
int zero_stats ( fd )
int  fd;
{
   struct persnidstats pss;

   pss.snid = snid;
 
   /* Set up request */
   io.ic_cmd=N_zeroSNIDstats;
   io.ic_timout=-1;			/* #13 */
   io.ic_len=sizeof(pss);
   io.ic_dp=(char *)&pss;

   /* Issue zero stats request */
   if (S_IOCTL(fd, I_STR, (char *) &io) < 0)
   {
      printf("x25pvcsnd. zero stats IOCTL FAILED!\n");
   }

   return (0);
}

/**********

exit_program() ---

Instead of just exiting, clean up system before exit.

**********/
#ifdef    ANSI_C
static void exit_program ( int error_val )
#else
static void exit_program ( error_val )
int error_val;
#endif  /* ANSI_C */
{
#ifdef  WINNT
   LPSTR lpMsg;
#endif  /* WINNT */
 
#ifdef  WINNT
   if ( WSACleanup ( ) == SOCKET_ERROR )
   {
      FormatMessage (
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL, WSAGetLastError ( ), LANG_USER_DEFAULT,
         ( LPSTR ) &lpMsg, 0, NULL );
 
      fprintf ( stderr, "\n\nWSACleanup() failed:\n%s\n", lpMsg );
 
      LocalFree ( lpMsg );
   }
#endif  /* WINNT */
 
#ifdef VXWORKS
   cleanup_mps_thread ( );
#endif /* VXWORKS */

   exit ( error_val );

} /* end exit_program() */

