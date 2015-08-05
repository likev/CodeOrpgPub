/*   @(#) x25pvcrcv.c 99/11/02 Version 1.9   */
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
5.  28-jan-97  pmt  corrected printf; detach should read reset
6.  29-jan-97  pmt  expanded to manly proportions: do multiple pvcs. this
		    not only exercises X.25, but entire streams build for
		    X.25 since each pvc is a separate stream.
7.  23-may-97  lmm  include sys/errno.h and tps_client.h 
8.  11-jul-97  hhn  remove include poll.h 
9.  03-nov-97  mpb  added WINNT ifdefs
10.  9-jul-98  rjp  mpb's Push swap module if needed (little endian
		    architecture).
11. 08-oct-99  tdg  Added VxWorks support

*/

/************************************************************************
*
* x25pvcrcv.c
*
*
************************************************************************/

#define MAX_CTL_SIZ	1000
#define MAX_DAT_SIZ 	4096
#define FALSE		0
#define TRUE		1


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#ifndef WINNT
#include <unistd.h>
#endif /* !WINNT */
#include <string.h>
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
#include "xnetdb.h"

#define VALIDOPTIONS    "c:n:l:o:s:tv:"
#define USAGE "\
Usage: %s [-c subnetid] [-l starting_lci] [-n number of PVCs] [-t]\n\
                 [-o controller]] [-s server] [-v service]\n"

#define PName	argv[0]

#ifdef WINNT
#define	DEFAULT_SERVER 	"\\\\.\\PTIXpqcc"
#else
#define	DEFAULT_SERVER 	"/dev/ucsclone"
#endif /* WINNT */

static int 	tracing;

#ifdef ANSI_C
static void  attach (int, char *, int);
static void  detach (int, char *);
static void exit_program ( int );
static void  fileReceive ();
static int   msgDisplay (struct xstrbuf *, struct xstrbuf *);
static void  resetConfirmSend  (int);
static void  resetSend  (int);
#else
static void  attach ();
static void  detach ();
static void exit_program ( );
static void  fileReceive ();
static int   msgDisplay ();
static void  resetConfirmSend  ();
static void  resetSend  ();
#endif

/* 
* Declare external for getopt
*/
extern char		*optarg;
extern int		optind;
/*
* End of getopt.
*/
static int			x25Fds[256];
static int			starting_lci, numPVCs;
static struct xpollfd          rcv_poll [ 256 ];

static FILE                *infp;

/************************************************************************
*
* main
*
* Read command line options, then offer menu.
*
************************************************************************/
int
#ifdef VXWORKS
x25pvcrcv (argc, argv, ptg)
#else
main (argc, argv)
#endif /* VXWORKS */
    int			argc;
    char		*argv [];
#ifdef VXWORKS
    pti_TaskGlobals     *ptg;
#endif /* VXWORKS */
{
    int			c;
    char		inbuf [256];
    int			lci;
    OpenRequest		oreqX25;
    char		*pProto;
    char		*pServer;
    char		*pService;
    char		*pSnid;
    int                 i;
    int                 forever = TRUE;

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

    taskVarAdd(0,&infp);
#endif /* VXWORKS */

    /*
    * Set the defaults.
    */
    tracing 	= 0;			/* Don't trace events		*/
    starting_lci = 1;			/* Logical channel 		*/
    numPVCs = 1;
    pSnid	= "A";			/* Subnet id			*/
    pProto 	= "x25";
    pServer 	= DEFAULT_SERVER;	/* Embedded board not LAN	*/
    pService	= "mps";		/* Ignored in imbedded board case */

    memset ( rcv_poll, 0, sizeof ( struct xpollfd ) * 256 );

    /*
    * Init open request data area
    */
    memset (&oreqX25, 0, sizeof (oreqX25));
    oreqX25.dev 	= 0;
    oreqX25.port 	= 0;
    oreqX25.ctlrNumber 	= 0;
    oreqX25.flags	= CLONEOPEN;
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
            case 'c':                   /* Sub net id                   */
            pSnid = optarg;
            break;
 
            case 'l':
            starting_lci = strtol (optarg, 0, 10); /* Start Logical channel  */
            break;
 
            case 'n':
            numPVCs = strtol (optarg, 0, 10); /* Number of PVCs         */
            break;
 
            case 'o': 			/* Controller Number 		*/
            oreqX25.ctlrNumber = strtol (optarg, 0, 10 );
            break;

	    case 's':			/* Server name			*/
	    strcpy (oreqX25.serverName, optarg);
	    break;

 	    case 't':
	    tracing = 1;		/* Trace activity		*/
	    break;

	    case 'v':			/* Service name - LAN only	*/
	    strcpy (oreqX25.serviceName, optarg);
	    break;
 
            case '?':
            default:
            printf( USAGE, PName) ;
            exit_program(1);
        }
    }
    if (tracing == 1)
	printf ("Open stream for read file.\n");

    for (i=0; i<numPVCs; i++)
    {
       x25Fds[i] = MPSopen (&oreqX25);
       if (x25Fds[i] < 0)
       {
           MPSperror ("MPSopen failed. Unable to connect to server.");
           exit_program (1);
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

       /*save for polling on rcv pkts */
       rcv_poll [ i ].sd = x25Fds [ i ] ;
       rcv_poll [ i ].events = POLLIN | POLLPRI;
    }

    printf ("\nThe usual sequence is:\n\n");
    printf ("    A - attach to pvc\n");
    printf ("    R - send reset\n");
    printf ("    T - receive file\n");
    printf ("    D - detach from pvc\n\n");
    printf ("In order for this to work, line 4, in both DTE and DCE x25\n");
    printf ("template files, must be greater than or equal to 001, and line 5\n"
);
    printf ("must be greater than or equal to line 4. In addition, the link\n");
    printf ("associated with the subnet id %s (used here) must\n",pSnid);
    printf ("be connected to the link associated with the subnet id used\n");
    printf ("in the x25pvcsnd process.\n");

    /*
    * prompt for what to do next
    */
    while ( forever )
    {
        printf("\n\n\n\n"                                               );
        printf("                   M E N U \n"       			);
        printf("A Attach To PVC(s)	R Send Reset Request\n"		);
        printf("T Receive File		D Detach From PVC(s)\n"		);
        printf("Q Quit Program\n"					);
        printf("Option ? "                                              );
 
        if (gets (inbuf))
        {
	    switch (toupper (*inbuf))
            {
                case 'A':               /* Attach                       */
                printf ("\n");
                lci = starting_lci;
                for (i=0; i<numPVCs; i++)
                {
                   attach (x25Fds[i], pSnid, lci);
                   lci++;
                }
                break;
 
                case 'R':               /* Send reset request           */
                printf ("\n");
                for (i=0; i<numPVCs; i++)
                {
                   resetSend (x25Fds[i]);
                }
                break;
 
                case 'D':               /* Send detach from pvc         */
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

		case 'T':		/* Receive file			*/
		printf ("\n");
		fileReceive ();
		break;

               	case 'Q':		/* Quit				*/
                for (i=0; i<numPVCs; i++)
                {
                   if (MPSclose(x25Fds[i]) != 0) 
                   {
                       MPSperror ("MPSclose failed.");
                       exit_program (1);
                   }
                }
                exit_program (0);

		default:
		printf ("?\n");
	    }
	}
	else
	    exit_program (0);			/* EOF */

    }

    return (0);
}

/**************************************************************************
* 
* msgDisplay
*
*
***************************************************************************/
int
msgDisplay (pCtlblk, pDatblk)
    struct xstrbuf	*pCtlblk;
    struct xstrbuf	*pDatblk;
{
    int			i;
    int			nDI;
    struct xdataf       *pData;
    char		*pDatbuf;
    S_X25_HDR 		*pHdr;


    nDI = 0;				/* No disconnect indication	*/
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
    int			x25Fd;
    char		*pSnid;
    int			lci;		/* Logical channel 		*/
{
    int			answer;
    struct xstrbuf	ctlblk;
    char		ctlbuf [MAX_CTL_SIZ];
    struct xstrbuf	datblk;
    char		datbuf [MAX_DAT_SIZ];
    int			flags;
    struct pvcattf	attach;
    struct pvcattf	*pAttach;

    /*
    * Init attach for server. 
    */
    memset (&attach, 0, sizeof (attach));
    attach.xl_type  	 = XL_CTL;
    attach.xl_command    = N_PVC_ATTACH;
    attach.sn_id   	 = snidtox25 (pSnid);
    attach.lci	   	 = lci;
    attach.reqackservice = 0;
    attach.reqnsdulimit  = 0;
    attach.nsdulimit     = 0;

    ctlblk.maxlen 	= MAX_CTL_SIZ;
    ctlblk.len 		= sizeof (struct pvcattf);
    ctlblk.buf  	= (char *) &attach;
    if (tracing == 1)
        printf ("Send attach snid %s, lci %d\n", pSnid, lci);
    if (MPSputmsg (x25Fd, &ctlblk, 0, 0) != 0)
    {
    	MPSperror ("MPSputmsg for attach failed");
    	exit_program (1);
    }
    /*
    * Read attach outcome. 
    */
    ctlblk.buf    = ctlbuf;
    datblk.maxlen = MAX_DAT_SIZ;
    datblk.buf    = datbuf;
    pAttach 	  = (struct pvcattf *) ctlbuf;
    flags 	  = 0;
    answer 	  = MPSgetmsg (x25Fd, &ctlblk, &datblk, &flags);
    if (answer < 0)
    {
        MPSperror ("MPSgetmsg for attach response failed");
        exit_program (1);
    }
    if (pAttach->xl_type == XL_CTL	/* If a control message		*/
    &&  pAttach->xl_command == N_PVC_ATTACH) /* and attach response	*/
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
            exit_program (1);
        }
    }
    else
    {
        MPSperror ("MPSgetmsg for Attach returned invalid attach response.");
	msgDisplay (&ctlblk, &datblk);
	exit_program (1);
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
    int			x25Fd;
    char		*pSnid;
{
    int			answer;
    struct xstrbuf	ctlblk;
    char		ctlbuf [MAX_CTL_SIZ];
    struct xstrbuf	datblk;
    char		datbuf [MAX_DAT_SIZ];
    int			flags;
    struct pvcdetf	detach;
    struct pvcdetf	*pDetach;

    /*
    * Init detach. 
    */
    memset (&detach, 0, sizeof (detach));
    detach.xl_type  	 = XL_CTL;
    detach.xl_command    = N_PVC_DETACH;

    ctlblk.maxlen 	= MAX_CTL_SIZ;
    ctlblk.len 		= sizeof (struct pvcdetf);
    ctlblk.buf  	= (char *) &detach;

    if (MPSputmsg (x25Fd, &ctlblk, 0, 0) != 0)
    {
    	MPSperror ("MPSputmsg for detach failed");
    	exit_program (1);
    }
    /*
    * Read detach outcome. 
    */
    ctlblk.buf    = ctlbuf;
    datblk.maxlen = MAX_DAT_SIZ;
    datblk.buf    = datbuf;
    pDetach 	  = (struct pvcdetf *) ctlbuf;
    flags 	  = 0;
    answer 	  = MPSgetmsg (x25Fd, &ctlblk, &datblk, &flags);
    if (answer < 0)
    {
        MPSperror ("MPSgetmsg for detach response failed");
        exit_program (1);
    }
    if (pDetach->xl_type == XL_CTL	/* If a control message		*/
    &&   (pDetach->xl_command == N_PVC_DETACH  /* and either detach response */
       || pDetach->xl_command == N_RI)) /* or other side detached already */
    {
        if (pDetach->reason_code != PVC_SUCCESS)
        {
	    printf ("Detach rejected (%d).\n", pDetach->reason_code);
            MPSperror ("MPSgetmsg for detach indicates invalid parameters");
            exit_program (1);
        }
    }
    else
    {
        MPSperror ("MPSgetmsg for detach returned invalid detach response.");
	msgDisplay (&ctlblk, &datblk);
	exit_program (1);
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
    int			x25Fd;
{
    int			answer;
    struct xstrbuf	ctlblk;
    char		ctlbuf [MAX_CTL_SIZ];
    struct xstrbuf	datblk;
    char		datbuf [MAX_DAT_SIZ];
    int			flags;
    struct xrstf	reset;
    struct xrscf	*pConfirm;

    /*
    * Init reset. 
    */
    memset (&reset, 0, sizeof (reset));
    reset.xl_type  	= XL_CTL;
    reset.xl_command	= N_RI;

    ctlblk.maxlen 	= MAX_CTL_SIZ;
    ctlblk.len 		= sizeof (reset);
    ctlblk.buf  	= (char *) &reset;
    if (MPSputmsg (x25Fd, &ctlblk, 0, 0) != 0)
    {
    	MPSperror ("MPSputmsg for reset failed");
    	exit_program (1);
    }
    /*
    * Read reset outcome. 
    */
    ctlblk.buf    = ctlbuf;
    datblk.maxlen = MAX_DAT_SIZ;
    datblk.buf    = datbuf;
    pConfirm 	  = (struct xrscf*) ctlbuf;
    flags 	  = 0;
    answer 	  = MPSgetmsg (x25Fd, &ctlblk, &datblk, &flags);
    if (answer < 0)
    {
        MPSperror ("MPSgetmsg for reset response failed");
        exit_program (1);
    }
    if ((pConfirm->xl_type == XL_CTL)	/* If a control message		*/
    &&   (pConfirm->xl_command == N_RC 	/* and reset confirm		*/
      ||  pConfirm->xl_command == N_RI)) /* or indication (collision)	*/
    {
	return;
    }
    else
    {
        MPSperror ("MPSgetmsg for reset returned invalid reset response.");
	msgDisplay (&ctlblk, &datblk);
	exit_program (1);
    }
}



/***********************************************************************
*
* resetConfirmSend
*
* Send a reset confirm to server. 
*
************************************************************************/

void
resetConfirmSend  (x25Fd)
    int			x25Fd;
{
    struct xstrbuf	ctlblk;
    struct xrscf	resetConfirm;

    /*
    * Init reset confirm. 
    */
    memset (&resetConfirm, 0, sizeof (resetConfirm));
    resetConfirm.xl_type  	= XL_CTL;
    resetConfirm.xl_command	= N_RC;

    ctlblk.maxlen 	= MAX_CTL_SIZ;
    ctlblk.len 		= sizeof (resetConfirm);
    ctlblk.buf  	= (char *) &resetConfirm;
    if (MPSputmsg (x25Fd, &ctlblk, 0, 0) != 0)
    {
    	MPSperror ("MPSputmsg for reset confirm failed");
    	exit_program (1);
    }
}



/************************************************************************
*
* fileReceive
*
*
************************************************************************/
void
fileReceive ()
{
    struct xstrbuf	ctlblk;
    char		ctlbuf [MAX_CTL_SIZ];
    struct xstrbuf	datblk;
    char		datbuf [MAX_DAT_SIZ];
    char		*pAnswer;
    struct xdataf       *pData;
    S_X25_HDR 		*pHdr;
    int flags, i, timedout, retval;
    char		file_in[256];
    int			answer;


 
    ctlblk.maxlen  	= sizeof (ctlbuf);
    datblk.maxlen  	= sizeof (datbuf);
    datblk.buf		= datbuf;
 
    /*
    * Get name of destination file and open it.
    */
    infp        = (FILE *) NULL;
 
    do
    {
        printf ("Name of receive file: ");
        pAnswer = gets (file_in);
        if (pAnswer == (char *) NULL)   
            return;
        if (pAnswer == (char *) EOF)    
            return;
        if (strlen (file_in) == 0)
	    return;
        infp = fopen (file_in, "w");
        if (infp == (FILE *) NULL )
        {
            printf("Unable to open file %s.\n", file_in);
        }
    } while (infp == (FILE *) NULL);
    if (tracing == 1)
        printf ("Opened %s. Read data\n", file_in);  


   timedout = FALSE;
   while ( !timedout )
   {
      retval = MPSpoll ( rcv_poll, numPVCs, 10000 );   /* msec */
      switch ( retval )
      {
         case -1:
         {
            MPSperror ( "\nPoll Error\n" );
            timedout = TRUE;   /* fake a timeout to get out */
            break;
         }
 
         case 0:
         {
            printf( "\n\nPoll timeout; File Transfer Complete\n");
 
            /* bail out; test is over or bad things occurred */
            timedout = TRUE;
            break;
         }
 
         default:
         {
 
            /* search for the pvcs with data to read */
            for ( i = 0; i < numPVCs; i++ )
            {
               /* is this a link ready to be serviced */
               if ( ( rcv_poll[ i ].revents & POLLIN) ||
                     (rcv_poll[ i ].revents & POLLPRI))
               {

                  /*
                  * Init to read data
                  */
                  memset (&ctlblk, 0, sizeof (ctlblk));
                  memset (ctlbuf, 0, sizeof (ctlbuf));
                  memset (datbuf, 0, sizeof (datbuf));
                  ctlblk.maxlen       = sizeof (ctlbuf);
                  ctlblk.buf          = ctlbuf;
                  datblk.maxlen       = MAX_DAT_SIZ;
                  datblk.buf          = datbuf;
 
		  flags = 0;
                  if ( (retval = MPSgetmsg ( rcv_poll [ i ].sd, 
					&ctlblk, &datblk, &flags ) ) < 0 )
                  {
                      MPSperror ( "MPSgetmsg failed\n" );
                      exit_program (1);
                  }
 
                   pHdr = (S_X25_HDR *) ctlbuf;
 
                   if (pHdr->xl_type == XL_CTL)
                   {
 
                       switch (pHdr->xl_command)
                       {
 
                           case N_RC:
                           if (tracing == 1)
                               printf ("Read N_RC.\n");
                           break;
 
                           case N_CI:
                           if (tracing == 1)
                               printf ("Read N_CI.\n");
                           break;
 
                           case N_CC:
                           if (tracing == 1)
                               printf ("Read N_CC.\n");
                           break;
 
                           case N_RI:
		           resetConfirmSend (rcv_poll[i].sd);
                           break;
 
                           case N_DI:
                           if (tracing == 1)
                               printf ("Read N_DI.\n");
                           fclose ( infp );
                           return;
 
                           case N_DC:
                           if (tracing == 1)
                               printf ("Read N_DC.\n");
                           break;
 
                           default:
                           printf ("?\n");
                       }

                   } /* End if control message */

                   if (pHdr->xl_type == XL_DAT) 
                   {
 
                       switch (pHdr->xl_command)
                       {
                           case N_Data:
                           pData = (struct xdataf *) ctlbuf;
                           if (pData->More &&  tracing == 1)
                              printf ("M-bit on\n");
                          if (pData->setQbit &&  tracing == 1)
                              printf ("Q-bit on\n");
                           if (pData->setDbit &&  tracing == 1)
                               printf ("D-bit set\n");
                           /*
                           * Write message to user's file
                           */
			   if (i == 0)
			   {
                               answer = fwrite (datbuf, sizeof (char), 
                                                datblk.len, infp);
                               if (answer == (int) NULL)
                               {
                                   perror ("Write error to receive file\n");
                                   exit_program (ERROR);
                               }
                		fflush (infp);
			   }

                          printf (".");
                          fflush (stdout);
                          break;
 
                          case N_DAck:
                          if (tracing == 1)
                              printf ("Read N_DAck.\n");
                          break;
 
                          case N_EData:
                          if (tracing == 1)
                              printf ("Read N_EData.\n");
                          break;
 
                          case N_EAck:
                          if (tracing == 1)
                              printf ("Read N_EAck.\n");
                          break;
 
                          default:
                          printf ("?\n");
                      }

                  } /* End if data message */

               }
               rcv_poll[i].revents = 0;
	    }
            break;
         }
      }
   }
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

   fclose (infp);
#endif /* VXWORKS */

   exit ( error_val );

} /* end exit_program() */
