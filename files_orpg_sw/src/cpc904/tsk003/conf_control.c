/*   @(#) conf_control.c 99/11/02 Version 1.14   */

/***************************************************************************
*
*    ===     ===     ===         ===         ===
*    ===     ===   =======     =======     =======
*    ===     ===  ===   ===   ===   ===   ===   ===
*    ===     === ===     === ===     === ===     ===
*    ===     === ===     === ===     === ===     ===   ===            ===
*    ===     === ===         ===     === ===     ===  =====         ======
*    ===     === ===         ===     === ===     === ==  ===      =======
*    ===     === ===         ===     === ===     ===      ===    ===   =
*    ===     === ===         ===     === ===     ===       ===  ==
*    ===     === ===         ===     === ===     ===        =====
*    ===========================================================
*    ===     === ===         ===     === ===     ===        =====
*    ===     === ===         ===     === ===     ===       ==  ===
*    ===     === ===     === ===     === ===     ===      ==    ===
*    ===     === ===     === ===     === ===     ====   ===      ===
*     ===   ===   ===   ===   ===   ===  ===     =========        ===  ==
*      =======     =======     =======   ===     ========          =====
*        ===         ===         ===     ===     ======             ===
*    
*      U   c   o   n   X      C   o   r   p   o   r   a   t   i   o   n
*    
*      This software is furnished  under  a  license and may be used and
*      copied only  in  accordance  with  the  terms of such license and
*      with the inclusion of the above copyright notice.   This software
*      or any other copies thereof may not be provided or otherwise made
*      available to any other person.   No title to and ownership of the
*      program is hereby transferred.
*
*      The information  in  this  software  is subject to change without
*      notice  and  should  not be considered as a commitment by UconX
*      Corporation.
* 
****************************************************************************/

/***************************************************************************
* 
* conf_control.c
*
***************************************************************************/

/*
Modification history:

Chg Date	Init Description
 1.  1-jul-98	rjp  Originated.
 2. 15-jul-98	rjp  Added support for little endian hosts.
 3. 22-jul-98	rjp  Fixed bug running with second controller.
 4. 04-mar-98   lmm  Added support for T1/E1 cards
 5. 08-oct-99   tdg  Added VxWorks support
*/

#ifdef DEBUG
static int debug = 1;
#endif

#ifdef WINNT
typedef char * caddr_t;
#include <winsock.h>
int getopt (int, char *const *, const char *);
/* 
* Declare external for getopt
*/
extern char		*optarg;
extern int		optind;
#else
#include <unistd.h>
#include <netinet/in.h>
#endif /* WINNT */

#ifdef VXWORKS
extern char		*optarg;
extern int		optind;
#endif /* VXWORKS */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <string.h>
#include <sys/types.h>

#include <debug.h>
#include <nc.h>

#include "xstopts.h"
#include "xstypes.h"
#include "xstpoll.h"
#include "mpsproto.h"
#include "uint.h"
#include "sdlpi.h"
#include "ll_proto.h"
#include "x25_proto.h"
#include "ll_control.h"
#include "x25_control.h"
#include "xnetdb.h"

#define VALIDOPTIONS    "TEs:tv:o:"
#define USAGE \
 "Usage: %s [-t] [-E] [-T] [-o controller] [-s server] [-v service]\n"
 
#define PName	argv[0]

#ifdef WINNT
#define DEFAULT_SERVER  "\\\\.\\PTIXpqcc"
#else
#define DEFAULT_SERVER  "/dev/ucsclone"
#endif /* WINNT */


typedef struct ll_snioc LL_SNIOC;
typedef struct xll_reg XLL_REG;

static void lc_lapb_wan (NC *, char *, char *, char *, char *, char *, char *,
                         char *, char *, int class);

static void lc_x25_lapb ();
static void our_options (int, char *[], char **, char **, char **);

#ifdef ANSI_C
static void exit_program (int);
#else
static void exit_program ();
#endif

static int tracing;

/* #4 - added these variables to support T1/E1 cards */

static int t1mode = 0, e1mode = 0;
static int link_offset = 1;
static int num_links = 4;

static char *pSnids[] =
   {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
   "AA", "AB", "AC", "AD", "AE", "AF", "AG", "AH", "AI", "AJ", "AK", "AL",
   "AM", "AN", "AO", "AP", "AQ", "AR", "AS", "AT", "AU", "AV" };


/*************************************************************************
* main
*
*************************************************************************/

#ifdef VXWORKS
int conf_control (argc, argv, ptg)
#else 
int main (argc, argv)
#endif /* VXWORKS */
    int		argc;
    char	*argv [];
#ifdef VXWORKS
    pti_TaskGlobals *ptg;
#endif /* VXWORKS */
{
    char	*pCtlrNumber;
    char	*pDevice0;
    char	*pLllLabel;
    NC		*pNc;
    char	*pProtoName;
    char	*pServer;
    char	*pService;
    char	*pServerLabel;
    char	*pXxxLabel;
    char    device[4];
    char    label[4];
    int     i, label_val, linknum, snid_num;

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

    /* re-set defaults */

    t1mode = 0;
    e1mode = 0;
    link_offset = 1;
    num_links = 4;

#ifdef VXWORKS
    init_mps_thread (ptg);
#endif /* VXWORKS */
    
    /*
    * Set defaults.
    */
    tracing 	= 0;
    pServer     = DEFAULT_SERVER;
    pService    = "mps";
    pCtlrNumber = "0";
    /*
    * Get command line options.
    */
    our_options (argc, argv, &pServer, &pService, &pCtlrNumber);

    pNc = nc_initialise (NULL);
    if (pNc == NULL)
    {
	printf ("nc_initialise failed to initialize\n");
	exit_program (1);
    }
    /*
    * Open x25 layer
    */
    pProtoName	= "x25";
    pDevice0	= "0";
    pXxxLabel  	= "XXX0";
    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber, pProtoName,
	    pDevice0, pXxxLabel);

    if (pServerLabel == NULL)
    {
	printf ("nc_open to %s failed.\n", pServer);
	exit_program (1);
    }
    if (tracing == 1)
        printf ("Opened %s, %s. Answer %s\n", pServer, pProtoName,
		pServerLabel);
    /*
    * #2. All servers are big endian. If we're little endian, push the
    *     x25 swap module on the stream. 
    */
    if (ntohl (1) != 1)                 /* If not network (big) order   */
    {
        if (nc_push (pNc, pXxxLabel, "x25swap") == 0)
        {
            printf ("nc_push to %s for '%s' failed.\n", pServer, pProtoName);
            exit_program (1);
        }
        if (tracing == 1)
            printf ("Pushed swap module on %s for %s.\n", pServer, pXxxLabel);
    }
    /*
    * Open lapb layer
    */
    pProtoName  = "lapb";
    pDevice0    = "0";
    pLllLabel   = "LLL0";
    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber, pProtoName,
            pDevice0, pLllLabel);
    if (pServerLabel == NULL)
    {
        printf ("nc_open to %s failed.\n", pServer);
        exit_program (1);
    }
    if (tracing == 1)
        printf ("Opened %s, %s. Answer '%s'\n", pServer, pProtoName,
		pServerLabel);
    /*
    * #2. All servers are big endian. If we're little endian, push the
    *     lapb swap module on the stream.
    */
    if (ntohl (1) != 1)                 /* If not network (big) order   */
    {
        if (nc_push (pNc, pLllLabel, "llswap") == 0)
        {
            printf ("nc_push to %s for '%s' failed.\n", pServer, pProtoName);
            exit_program (1);
        }
	if (tracing == 1)
            printf ("Pushed swap module on %s for '%s'.\n", pServer, pLllLabel);
    }

    /* #4 - added support for T1/E1 cards */

    label_val = 120;
    snid_num = 0;
    linknum = 0;
    for ( i=0; i<num_links/2;  i++ )
    {
       /* Configure DTE link */
       sprintf (device, "%d", linknum);
       sprintf (label, "%d", label_val++);

#if 0
       if (tracing == 1 )
#endif
       {
          printf ("Configuring link %s snid=%s label=%s\n", 
             device, pSnids[snid_num], label);
       }

       lc_lapb_wan (pNc, pServer, pService, pCtlrNumber, device, label, "LLL0",
	    "WWW", pSnids[snid_num++], LC_LAPBDTE);

       /* Configure DCE link */
       sprintf (device, "%d", linknum + link_offset );
       sprintf (label, "%d", label_val++);

#if 0
       if (tracing == 1 )
#endif
       {
          printf ("Configuring link %s snid=%s label=%s\n", 
              device, pSnids[snid_num], label);
       }

       lc_lapb_wan (pNc, pServer, pService, pCtlrNumber, device, label, "LLL0",
	    "WWW", pSnids[snid_num++], LC_LAPBDCE);

       /* At this point we've configured a pair of links and have to move on
	  to the next pair of links.  On T1 card we configure links 0-24, 1-25 
	  & on E1 links 0-32, 1-33 - so offset to next pair is 1.  On other 
          cards we configure 0-1, 2-3, etc. so offset to next pair is 2. */ 

       if ( t1mode || e1mode )
	  linknum += 1;
       else
	  linknum += 2;
    }

    /*
    * #2. All servers are big endian. If we're little endian, pop the
    *     lapb swap module off the stream.
    */
    if (ntohl (1) != 1)                 /* If not network (big) order   */
    { 
        if (nc_pop (pNc, pLllLabel) == 0)
        {
            printf ("nc_pop from %s for '%s' failed.\n", pServer, pProtoName);
            exit_program (1);
        }
        if (tracing == 1)
            printf ("Popped swap module from %s for '%s'.\n", pServer,
                    pLllLabel);
    }

    label_val = 130;
    snid_num = 0;
    for ( i=0; i<num_links/2;  i++ )
    {
       /* Configure DTE link */
       sprintf (label, "%d", label_val++);

       lc_x25_lapb (pNc, pServer, pCtlrNumber, "XXX0", "LLL0", label, 
            pSnids[snid_num++], "def.dte84.x25");

       /* Configure DCE link */
       sprintf (label, "%d", label_val++);

       lc_x25_lapb (pNc, pServer, pCtlrNumber, "XXX0", "LLL0", label, 
            pSnids[snid_num++], "def.dce84.x25");
    }

    /*
    * Terminate
    */
    nc_terminate (pNc);
    if (tracing == 1)
        printf ("nc_terminate\n");

#ifdef VXWORKS
    exit_program (0);
#endif 
    return (0);
}

/*************************************************************************
* our_options
*
* Get options from command line.
*************************************************************************/
void
our_options (argc, argv, ppServerName, ppServiceName, ppCtlrNumber)
    int		argc;
    char	*argv [];
    char	**ppServerName;
    char	**ppServiceName;
    char	**ppCtlrNumber;
{
    int		c;
    /*
    * Get the options from command line
    */
 
    while ((c = getopt (argc, argv, VALIDOPTIONS)) != EOF)
    {
        switch (c)
        {
            case 's':                   /* Server name                  */
            *ppServerName = optarg;
            break;

            case 'v':                   /* Service name - LAN only      */
            *ppServiceName = optarg;
            break;
 
            case 't':
            tracing = 1;		/* Trace activity               */
            break;
 
            case 'o':                   /* Controller Number            */
            *ppCtlrNumber = optarg;
            break;

            case 'T':
            t1mode = 1;		/* T1 board */
            link_offset = 24;
            num_links = 48;
            break;
 
            case 'E':
            e1mode = 1;		/* E1 board */
            link_offset = 32;
            num_links = 48;
            break;
 
            case '?':
            default:
            printf (USAGE, PName) ;
            exit_program (1);
        }
    }

    if ( t1mode && e1mode )
    {
        printf ("\nT1 Mode and E1 Mode are mutually exclusive\n");
        printf(USAGE, PName) ;
        exit_program(1);
    }
}


/*************************************************************************
* lc_lapb_wan
*
* Link wan layer under lapb. Configure lapb and wan using the lltune
* and wantune utilities.
*
*************************************************************************/

void
lc_lapb_wan (pNc, pServer, pService, pCtlrNumber, pDevice, pLabel,
	pLapbLabel, pWanLabel, pSnid, class)
    NC		*pNc;
    char	*pServer;
    char	*pService;
    char	*pCtlrNumber;
    char	*pDevice;
    char	*pLabel;
    char	*pLapbLabel;
    char	*pWanLabel;
    char	*pSnid;
    int		class;
{
    int		answer;
    int		lapbMuxid;
    char	command [500];
    char	*pAnswer;
    char	*pProtoName;
    char	*pServerLabel;
    LL_SNIOC    snioc;
    char    cmd_path [ 128 ];
#ifdef WINNT
    char    *s;
#endif /* WINNT */

    /*
    * Open wan layer for port pDevice.
    */
    pProtoName     = "wan";
    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber, pProtoName,
            pDevice, pWanLabel);
    if (pServerLabel == NULL)
    {
        printf ("nc_open to %s, %s failed.\n", pServer, pDevice);
        exit_program (1);
    }
    if (tracing == 1)
        printf ("Opened %s, %s. Answer %s\n", pServer, pProtoName, pWanLabel);
    /*
    * Link wan under lapb.
    */
    if (tracing == 1)
	printf ("Linking %s under %s for %s\n", pWanLabel, pLapbLabel, pLabel);
    pAnswer = nc_link (pNc, pLapbLabel, pWanLabel, pLabel);
    if (pAnswer == NULL)
    {
        printf ("nc_link (pNc, '%s', '%s') failed\n", pLapbLabel, pWanLabel);
        exit_program (1);
    }
    if (tracing == 1)
	printf ("Closing %s\n", pWanLabel);
    /*
    * Don't close wan.
    */
    /*
    * Get code for label pLabel.
    */
    lapbMuxid = nc_muxval (pNc, pLabel);
    if (lapbMuxid == -1)
    {
        printf ("nc_muxval failed on '%s'\n", pLabel);
        exit_program (1);
    }
    if (tracing == 1)
	printf ("Get label value: Label %s has value %d\n", pLabel, lapbMuxid);
    /*
    * Assign the subnet id to the stream. The ioctl goes to the lapb layer.
    * The lapb layer sends a message to the wan layer which allows the wan
    * layer to associate the subnet id with the stream. Later, when
    * wantune executes for this subnetid, the wan layer uses the provided
    * subnet id to locate the stream that the configuration parameters
    * are intended for.
    */
    memset (&snioc, 0, sizeof (snioc));
    snioc.lli_type      = LI_SNID;
    snioc.lli_snid      = snidtox25 (pSnid);
    snioc.lli_index     = lapbMuxid;
    snioc.lli_class     = class;	/* LC_LAPBDTE or LC_LAPBDCE	*/

    answer = nc_strioc (pNc, pLabel, L_SETSNID, 10, (unsigned char *) &snioc,
            sizeof (snioc));
    if (answer == -1)                   /* If failed                    */
    {
        printf ("Error on nc_strioc L_SETSNID '%s'\n", pLabel);
        exit_program (1);
    }
    if (tracing == 1)
 	printf ("Send L_SETSNID to %s.\n", pLabel);
    /*
    * Tune wan and lapb. For example,
    *
    * 	"/etc/wantune -P -d 1 -s B -t /dev/ucsclone -c 0 def.wan";
    *
    * and
    *
    *   "/etc/lltune -P -p lapb -s B -d 0 -t /dev/ucsclone -c 0 def.lapb",
    */
#if defined(WINNT) || defined(VXWORKS)
#ifndef VXWORKS
    if ( s = getenv ( "UCONx" ) )
    {
        sprintf ( cmd_path, "%s\\bin\\", s );
    }

    else
#endif /* !VXWORKS */
    {
        sprintf ( cmd_path, "" );
    }
#else
    sprintf ( cmd_path, "/etc/" );
#endif /* WINNT || VXWORKS */

    sprintf ( command, "%swantune -P -d %s -s %s -t %s -c %s def.wan",
	    cmd_path, pDevice, pSnid, pServer, pCtlrNumber );

    if (tracing == 1)
    {
	    printf ( "Issue %s\n", command );
    }

    answer = nc_shell (pNc, command);
    if (answer == 0)
    {
        printf ("Error on nc_shell wantune\n");
        exit_program (1);
    }

    sprintf (command, "%slltune -P -p lapb -s %s -d 0 -t %s -c %s def.lapb",
	    cmd_path, pSnid, pServer, pCtlrNumber);

    if (tracing == 1)
    {
	   printf ( "Issue %s\n", command );
    }

    answer = nc_shell (pNc, command);
    if (answer == 0)
    {
        printf ("Error on nc_shell lltune\n");
        exit_program (1);
    }
}

/*************************************************************************
* lc_x25_lapb
*
* Link lapb under x25 and configure x25 using x25tune utility. Then
* assign subnet ID. Assigning the subnet may or may not cue the x25
* network software to try to bring up the lapb (link) layer depending
* on the wan configuration. See WANTEMPLATE and WAN_auto_enable.
*
*************************************************************************/

void
lc_x25_lapb (pNc, pServer, pCtlrNumber, pX25Label, pLapbLabel, pMuxLabel,
	pSnid, pFile)
    NC		*pNc;
    char	*pServer;
    char        *pCtlrNumber;
    char	*pX25Label;
    char	*pLapbLabel;
    char	*pMuxLabel;
    char	*pSnid;
    char	*pFile;
{
    int		answer;
    char	command [500];
    int		x25Muxid;
    char	*pAnswer;
    XLL_REG 	xlreg;
    char    cmd_path [ 128 ];
#ifdef WINNT
    char    *s;
#endif /* WINNT */

    /*
    * Link lapb under x25.
    */

    if (tracing == 1)
	printf ("Link %s under %s for %s\n", pLapbLabel, pX25Label, pMuxLabel);

    pAnswer = nc_link (pNc, pX25Label, pLapbLabel, pMuxLabel);
    if (pAnswer == NULL)
    {
        printf ("nc_link (pNc, '%s', '%s') failed\n", pX25Label, pLapbLabel);
        exit_program (1);
    }
    /*
    * Get code for label muxid label.
    */
    x25Muxid = nc_muxval (pNc, pMuxLabel);
    if (x25Muxid == -1)
    {
        printf ("nc_muxval failed on '%s'\n", pMuxLabel);
        exit_program (1);
    }
    if (tracing == 1)
        printf ("Get label value: Label %s has value %d\n", pMuxLabel,
		x25Muxid);
    /*
    * Don't close lapb.
    */
    /*
    * Tune x25 for subnet.
    */
#if defined(WINNT) || defined(VXWORKS)
#ifndef VXWORKS
    if ( s = getenv ( "UCONx" ) )
    {
        sprintf ( cmd_path, "%s\\bin\\", s );
    }
    else
#endif /* !VXWORKS */
    {
        sprintf ( cmd_path, "" );
    }
#else
    sprintf ( cmd_path, "/etc/" );
#endif /* WINNT || VXWORKS */

    sprintf (command, "%sx25tune -P -s %s -d 0 -t %s -c %s %s",
	    cmd_path, pSnid, pServer, pCtlrNumber, pFile);

    if (tracing == 1)
    {
        printf ("Issue %s\n", command );
    }

    answer = nc_shell (pNc, command);
    if (answer == 0)
    {
        printf ("Error on nc_shell x25tune\n");
        exit_program (1);
    }

    /*
    * Assign subnet. Assigning the subnet may or may not cue the x25
    * network software to try to bring up the lapb (link) layer depending
    * on the wan configuration. See WANTEMPLATE and WAN_auto_enable.
    */
    if (tracing == 1)
	printf ("Issue N_snident ioctl to %s.\n", pMuxLabel);
    memset (&xlreg, 0, sizeof (xlreg));
    xlreg.snid          = snidtox25 (pSnid);
    xlreg.dl_sap        = 0;
    xlreg.lmuxid        = x25Muxid;
    xlreg.dl_max_conind = 1;
    answer = nc_strioc (pNc, pMuxLabel, N_snident, 10,
	    (unsigned char *) &xlreg, sizeof (xlreg));
    if (answer == -1)                   /* If failed                    */
    {
        printf ("Error on nc_strioc N_snident snid %s, %d, '%s'\n", 
		pSnid, x25Muxid, pMuxLabel);
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
   cleanup_mps_thread ();
#endif /* VXWORKS */
 
   exit ( error_val );
 
} /* end exit_program() */
