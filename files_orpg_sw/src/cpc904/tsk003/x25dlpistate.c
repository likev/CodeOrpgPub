/*   @(#) x25dlpistate.c 00/09/08 Version 1.02   */
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
1.  08-NOV-00  djb  Original code (based on x25pvcrcv.c)
2.  21-NOV-00  djb  Added functionality for user restarts
*/

/************************************************************************
*
* x25dlpistate.c
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
#include <signal.h>
#endif /* VXWORKS */

#ifdef  WINNT
typedef char *    caddr_t;
#include <winsock.h>
int    getopt ( int, char *const *, const char * );
#endif /* !WINNT */

#define NOTIFY_STATUS
#define USER_RESTART

#include "xstopts.h" 
#include "xstypes.h"
#include "xstpoll.h"
#include "mpsproto.h"
#include "xstra.h"
#include "ll_mon.h"
#include "dlpi.h"
#include "x25_proto.h"
#include "x25_control.h"
#include "x25_monitor.h"
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
static void  fileReceive (struct xpollfd *);
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
static int			starting_lci, numPVCs;

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
x25dlpistate (argc, argv, ptg)
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
	char			inbuf [256];
	int			lci;
	OpenRequest		oreqX25;
	char			*pProto;
	char			*pServer;
	char			*pService;
	char			*pSnid;
	char			snid_str[10];
	int			i;
	int			x25Fd;
	struct xpollfd		rcv_poll;
	struct xstrioctl	strioctl;
	int			retval, flags;
	struct xstrbuf		ctlblk;
	char			ctlbuf [MAX_CTL_SIZ];
	struct xstrbuf		datblk;
	char			datbuf [MAX_DAT_SIZ];
	struct xdataf		*pData;
	S_X25_HDR		*pHdr;
	struct x25_statusmon	*status;

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

    taskVarAdd (0, &infp);
    taskVarAdd (0, &numPVCs);
    taskVarAdd (0, &starting_lci);

    signal (SIGUSR1, exit_program);
#endif /* VXWORKS */

    /*
    * Set the defaults.
    */
    tracing 	= 0;			/* Don't trace events		*/
    starting_lci = 0;			/* Logical channel 		*/
    numPVCs = 0;
    pSnid	= "LIST";			/* Subnet id			*/
    pProto 	= "x25";
    pServer 	= DEFAULT_SERVER;	/* Embedded board not LAN	*/
    pService	= "mps";		/* Ignored in imbedded board case */

    memset (&rcv_poll, 0, sizeof (struct xpollfd));

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

	if ((x25Fd = MPSopen (&oreqX25)) < 0)
	{
		MPSperror ("MPSopen failed. Unable to connect to server.");
		exit_program (1);
	}

	if ( ntohl(0x1) != 0x1 )
	{
		if (tracing == 1)
		printf("Little Endian -- pushing swap module.\n" );

		if ( MPSioctl ( x25Fd, X_PUSH, "x25swap" ) == MPS_ERROR )
		{
			MPSperror ( "MPSioctl swap X_PUSH" );
			exit_program ( MPS_ERROR );
		}
	}

	/*save for polling on rcv pkts */
	rcv_poll.sd = x25Fd;
	rcv_poll.events = POLLIN | POLLPRI;

	strioctl.ic_cmd = N_DLPInotify;
	strioctl.ic_timout = -1;
	strioctl.ic_len = 0;
	strioctl.ic_dp = NULL;

	if ( MPSioctl ( x25Fd, X_STR, &strioctl ) == MPS_ERROR )
	{
		MPSperror ( "MPSioctl N_DLPInotify X_STR" );
		exit_program ( MPS_ERROR );
	}
	printf ("DLPI state listener is attached.  Commencing reporting.\n");
	printf ("=======================================================\n\n");

	ctlblk.maxlen	= sizeof (ctlbuf);
	datblk.maxlen	= sizeof (datbuf);

	while (TRUE)
	{
		retval = MPSpoll ( &rcv_poll, 1, 10000 );   /* msec */
		if (!retval)
		{
			continue;
		}

		if (retval == -1)
		{
			MPSperror ( "Poll Error\n" );
			continue;
		}

		if (tracing == 1)
		{
			printf ("<Event: %0s%0s>\n",
				(rcv_poll.revents & POLLIN)? "POLLIN ":"",
				(rcv_poll.revents & POLLPRI)? "POLLPRI ":"");
		}

		/*
		 * Init to read data
		 */
		memset (&ctlblk, 0, sizeof (ctlblk));
		memset (ctlbuf, 0, sizeof (ctlbuf));
		memset (datbuf, 0, sizeof (datbuf));
		ctlblk.buf	= ctlbuf;
		datblk.buf	= datbuf;
		status		= (struct x25_statusmon *)datbuf;
 
		flags = 0;
		if ((retval = MPSgetmsg (rcv_poll.sd, &ctlblk, &datblk, &flags)) < 0)
		{
			MPSperror ("MPSgetmsg failed\n");
		}

		pHdr = (S_X25_HDR *) ctlbuf;
		pData = (struct xdataf *) ctlbuf;
 
		if (tracing == 1)
		{
			printf ("Received message: ctl = %d bytes, data = %d bytes\n",
				ctlblk.len, datblk.len);
			if (ctlblk.len>0)
			{
				printf ("CTL:  ");
				for (i=0; i<ctlblk.len; i++)
				{
					printf ("%02X ", (unsigned char)ctlbuf[i]);
					if (i%16 == 15)
						printf ("\n      ");
				}
				if (i%16 != 0)
					printf ("\n");
			}
			if (datblk.len>0)
			{
				printf ("DATA: ");
				for (i=0; i<datblk.len; i++)
				{
					printf ("%02X ", (unsigned char)datbuf[i]);
					if (i%16 == 15)
						printf ("\n      ");
				}
				if (i%16 != 0)
					printf ("\n");
			}
		}

		if ((pHdr->xl_type == XL_DAT) && (pHdr->xl_command == N_Data))
		{
			if (datblk.len != sizeof (struct x25_statusmon))
			{
				printf ("Invalid data block - discarded\n");
				continue;
			}

			if (x25tosnid (status->snid, snid_str) == -1)
			{
				strcpy (snid_str, "<UNKNOWN>");
			}
			else
			{
				snid_str[4] = '\0';
			}

			if (status->state >> 16 == 0xFFFF) /* #2 */
			{
				printf ("Event on subnet '%s': ", snid_str);
				switch (status->state & 0xFFFF)
				{
					case UREV_TRANSIENT_DXFER_ON:  printf ("transient dataxfer enabled\n"); break;
					case UREV_TRANSIENT_DXFER_OFF: printf ("transient dataxfer cancelled\n"); break;
					case UREV_PVC_CAN_ATTACH:      printf ("PVCs available for attachment\n"); break;
					default:
						printf ("unknown event (0x%04X) ", status->state & 0xFFFF);
				}
			}
			else
			{
				printf ("Subnet '%s' has changed from ", snid_str);
				switch (status->state>>16)
				{
					case DL_UNBOUND:            printf ("DL_UNBOUND "); break;
					case DL_BIND_PENDING:       printf ("DL_BIND_PENDING "); break;
					case DL_UNBIND_PENDING:     printf ("DL_UNBIND_PENDING "); break;
					case DL_IDLE:               printf ("DL_IDLE "); break;
					case DL_UNATTACHED:         printf ("DL_UNATTACHED "); break;
					case DL_ATTACH_PENDING:     printf ("DL_ATTACH_PENDING "); break;
					case DL_DETACH_PENDING:     printf ("DL_DETACH_PENDING "); break;
					case DL_UDQOS_PENDING:      printf ("DL_UDQOS_PENDING "); break;
					case DL_OUTCON_PENDING:     printf ("DL_OUTCON_PENDING "); break;
					case DL_INCON_PENDING:      printf ("DL_INCON_PENDING "); break;
					case DL_CONN_RES_PENDING:   printf ("DL_CONN_RES_PENDING "); break;
					case DL_DATAXFER:           printf ("DL_DATAXFER "); break;
					case DL_USER_RESET_PENDING: printf ("DL_USER_RESET_PENDING "); break;
					case DL_PROV_RESET_PENDING: printf ("DL_PROV_RESET_PENDING "); break;
					case DL_RESET_RES_PENDING:  printf ("DL_RESET_RES_PENDING "); break;
					case DL_DISCON8_PENDING:    printf ("DL_DISCON8_PENDING "); break;
					case DL_DISCON9_PENDING:    printf ("DL_DISCON9_PENDING "); break;
					case DL_DISCON11_PENDING:   printf ("DL_DISCON11_PENDING "); break;
					case DL_DISCON12_PENDING:   printf ("DL_DISCON12_PENDING "); break;
					case DL_DISCON13_PENDING:   printf ("DL_DISCON13_PENDING "); break;
					case DL_SUBS_BIND_PND:      printf ("DL_SUBS_BIND_PND "); break;
					case DL_SUBS_UNBIND_PND:    printf ("DL_SUBS_UNBIND_PND "); break;
					default:
						printf ("an unknown state (0x%04X) ", status->state>>16);
				}
				printf ("to ", snid_str);
				switch (status->state & 0xFFFF)
				{
					case DL_UNBOUND:            printf ("DL_UNBOUND\n"); break;
					case DL_BIND_PENDING:       printf ("DL_BIND_PENDING\n"); break;
					case DL_UNBIND_PENDING:     printf ("DL_UNBIND_PENDING\n"); break;
					case DL_IDLE:               printf ("DL_IDLE\n"); break;
					case DL_UNATTACHED:         printf ("DL_UNATTACHED\n"); break;
					case DL_ATTACH_PENDING:     printf ("DL_ATTACH_PENDING\n"); break;
					case DL_DETACH_PENDING:     printf ("DL_DETACH_PENDING\n"); break;
					case DL_UDQOS_PENDING:      printf ("DL_UDQOS_PENDING\n"); break;
					case DL_OUTCON_PENDING:     printf ("DL_OUTCON_PENDING\n"); break;
					case DL_INCON_PENDING:      printf ("DL_INCON_PENDING\n"); break;
					case DL_CONN_RES_PENDING:   printf ("DL_CONN_RES_PENDING\n"); break;
					case DL_DATAXFER:           printf ("DL_DATAXFER\n"); break;
					case DL_USER_RESET_PENDING: printf ("DL_USER_RESET_PENDING\n"); break;
					case DL_PROV_RESET_PENDING: printf ("DL_PROV_RESET_PENDING\n"); break;
					case DL_RESET_RES_PENDING:  printf ("DL_RESET_RES_PENDING\n"); break;
					case DL_DISCON8_PENDING:    printf ("DL_DISCON8_PENDING\n"); break;
					case DL_DISCON9_PENDING:    printf ("DL_DISCON9_PENDING\n"); break;
					case DL_DISCON11_PENDING:   printf ("DL_DISCON11_PENDING\n"); break;
					case DL_DISCON12_PENDING:   printf ("DL_DISCON12_PENDING\n"); break;
					case DL_DISCON13_PENDING:   printf ("DL_DISCON13_PENDING\n"); break;
					case DL_SUBS_BIND_PND:      printf ("DL_SUBS_BIND_PND\n"); break;
					case DL_SUBS_UNBIND_PND:    printf ("DL_SUBS_UNBIND_PND\n"); break;
					default:
						printf ("an unknown state (0x%04X)\n", status->state & 0xFFFF);
				}
			}
		}
		else
			printf ("Invalid message type received - discarding.\n");

		rcv_poll.revents = 0;
	}
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
