/*   @(#) x25restcont.c 00/09/08 Version 1.01   */
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
1.  21-NOV-00  djb  Original code (modified from x25pvcrcv.c).

*/

/************************************************************************
*
* x25restcont.c
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

#define VALIDOPTIONS    "c:o:s:tv:"
#define USAGE "\
Usage: %s [-c subnetid] [-t] [-o controller] [-s server] [-v service]\n"

#define PName	argv[0]

#ifdef WINNT
#define	DEFAULT_SERVER 	"\\\\.\\PTIXpqcc"
#else
#define	DEFAULT_SERVER 	"/dev/ucsclone"
#endif /* WINNT */

typedef struct
{
	uint32 snid;
	char   enabled;
} ioctldata;

ioctldata       data;
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
x25restcont (argc, argv, ptg)
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
	int			send_ioctl;
	int			lci;
	OpenRequest		oreqX25;
	char			*pProto;
	char			*pServer;
	char			*pService;
	char		        pSnid[10];
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

    signal (SIGUSR1, exit_program);
#endif /* VXWORKS */

    /*
    * Set the defaults.
    */
    tracing 	= 0;			/* Don't trace events		*/
    strcpy (pSnid, "CTRL");			/* Subnet id			*/
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
            strcpy (pSnid, optarg);
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
	data.snid = snidtox25 (pSnid);

	while (1)
	{
		printf ("SNID = %s\n\nOptions:-\n---------\n", pSnid);
		printf ("S) Change SNID\n");
		printf ("X) Send Restart/confirm\n");
		printf ("T) Change state to Transient Dataxfer\n");
		printf ("C) Cancel transient state change\n");
		printf ("Q) Quit\n\n\n> ");
		send_ioctl = 0;

		if (gets (inbuf))
		{
			switch (toupper (*inbuf))
			{
				case 'S':
					printf ("SNID = ");
					while (!gets (pSnid));
					data.snid = snidtox25 (pSnid);
				break;
				case 'X':
					strioctl.ic_cmd = N_sendRESTART;
					strioctl.ic_len = 4;
					strioctl.ic_dp = (char *)&data;
					send_ioctl = 1;
				break;
				case 'T':
					strioctl.ic_cmd = N_tempDXFER;
					strioctl.ic_len = 5;
					strioctl.ic_dp = (char *)&data;
					data.enabled = 1;
					send_ioctl = 1;
				break;
				case 'C':
					strioctl.ic_cmd = N_tempDXFER;
					strioctl.ic_len = 5;
					strioctl.ic_dp = (char *)&data;
					data.enabled = 0;
					send_ioctl = 1;
				break;
				case 'Q':
					exit_program(0);
				break;
				default:
					continue;
				break;
			}
					
		}

		strioctl.ic_timout = -1;

		if (send_ioctl)
		{
			if ( MPSioctl ( x25Fd, X_STR, &strioctl ) == MPS_ERROR )
			{
				MPSperror ( "MPSioctl N_DLPInotify X_STR" );
				exit_program ( MPS_ERROR );
			}
			else
			{
				printf ("\n\nIoctl Sent Successfully\n\n");
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
