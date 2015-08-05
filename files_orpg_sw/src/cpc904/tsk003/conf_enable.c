/*   @(#) conf_enable.c 99/11/02 Version 1.8   */
 
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

/*
Modification history.
Chg Date       Init Description
 1. 15-Jul-98 	rjp Added swap module push.
 2. 08-oct-99   tdg Added VxWorks support
*/

#ifdef DEBUG
static int debug = 1;
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef WINNT
#include <winsock.h>
typedef char * caddr_t;
int    getopt ( int, char *const *, const char * );
/* 
* Declare external for getopt
*/
extern char		*optarg;
extern int		optind;
#else
#include <unistd.h>
#include <netinet/in.h>
#endif /* WINNT */
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#ifdef VXWORKS
extern char		*optarg;
extern int		optind;
#endif /* VXWORKS */

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
#include "wan_control.h"
#include "xnetdb.h"

typedef struct ll_snioc LL_SNIOC;
typedef struct xll_reg XLL_REG;

 
#define VALIDOPTIONS    "ETs:tv:o:"
#define USAGE \
 "Usage: %s [-t] [-E] [-T] [-o controller] [-s server] [-v service]\n"
 
#define PName   argv[0]
 
#ifdef WINNT
#define DEFAULT_SERVER  "\\\\.\\PTIXpqcc"
#else
#define DEFAULT_SERVER  "/dev/ucsclone"
#endif /* WINNT */

static void our_options (int, char *[], char **, char **, char **);
static int wanCommand (NC *, char *, char *, char *, char *, unsigned);

#ifdef ANSI_C
static void exit_program (int);
#else
static void exit_program ();
#endif

static int     tracing;

/* #4 - added these variables to support T1/E1 cards */

static int t1mode = 0, e1mode = 0;
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
int conf_enable (argc, argv, ptg)
#else 
int main (argc, argv)
#endif /* VXWORKS */
    int		argc;
    char	*argv [];
#ifdef VXWORKS
    pti_TaskGlobals *ptg;
#endif /* VXWORKS */
{
    char		*pCtlrNumber;
    NC			*pNc;
    char		*pServer;
    char		*pService;
    int         i;
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
    init_mps_thread (ptg);
#endif /* VXWORKS */
    
    /*
    * Set defaults.
    */
    tracing	= 0;
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

    for ( i=0; i<num_links; i++ )
    {
       /*
       * Enable subnetwork 
       */
       wanCommand (pNc, pServer, pService, pCtlrNumber, pSnids[i], W_ENABLE);	 
    }

    /*
    * Terminate
    */
    nc_terminate (pNc);
    if (tracing != 0)
        printf ("nc_terminated\n");

#ifdef VXWORKS
    exit_program (0);
#else
    return (0);
#endif /* VXWORKS */
}

 
/*************************************************************************
* our_options
*
* Get options from command line.
*************************************************************************/
void
our_options (argc, argv, ppServerName, ppServiceName, ppCtlrNumber)
    int         argc;
    char        *argv [];
    char        **ppServerName;
    char        **ppServiceName;
    char        **ppCtlrNumber;
{
    int         c;

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
            tracing = 1;                /* Trace activity               */
            break;
 
            case 'o':                   /* Controller Number            */
            *ppCtlrNumber = optarg;
            break;

            case 'T':
            t1mode = 1;     		    /* T1 board */
            num_links = 48;
            break;

            case 'E':
            e1mode = 1;                 /* E1 board */
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
* wanCommand
*
*************************************************************************/
int
wanCommand (pNc, pServer, pService, pCtlrNumber, pSnid, command)
    NC          *pNc;
    char        *pServer;
    char        *pService;
    char        *pCtlrNumber;
    char        *pSnid;
    unsigned	command;
{
    int                 answer;
    char                *pDevice0;
    char                *pProtoName;
    char                *pServerLabel;
    char                *pWwwLabel;
    char		wanLabel [100];
    struct wan_hdioc    wan_tn;

    /*
    * Open wan layer
    */
    pProtoName     = "wan";
    pDevice0       = "0";
    pWwwLabel      = "WWW0";

    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber, pProtoName,
            pDevice0, pWwwLabel);
    if (pServerLabel == NULL)
    {
        printf ("nc_open to %s, %s failed.\n", pServer, pDevice0);
        exit_program (1);
    }
    if (tracing == 1)
        printf ("Opened %s, %s. Answer %s\n", pServer, pProtoName,
		pServerLabel);
    strcpy (wanLabel, pServerLabel);
    /*
    * #1. All servers are big endian. If we're little endian, push the
    *     wan swap module on the stream.
    */
    if (ntohl (1) != 1)			/* If not network (big) order	*/
    {
	if (tracing == 1)
    	    printf ("Push swap module on %s for '%s' protocol.\n", pServer,
		    pProtoName);
	if (nc_push (pNc, pWwwLabel, "wanswap") == 0)
	{
            printf ("nc_push to %s, %s failed.\n", pServer, pDevice0);
            exit_program (1);
	}
    }

    /*
    * Build and send command to wan.
    */
    if (tracing == 1)
	printf ("Send W_ENABLE to wan layer.\n");
    memset (&wan_tn, 0, sizeof (wan_tn));
    wan_tn.w_type       = WAN_PLAIN;
    wan_tn.w_snid       = snidtox25 (pSnid);
    answer = nc_strioc (pNc, wanLabel, command, 10,
            (unsigned char *) &wan_tn, sizeof (wan_tn));
    if (answer == -1)                   /* If failed                    */
    {
	if (command == W_DISABLE)
            printf ("Error on nc_strioc W_DISABLE 'wans'\n");
	if (command == W_ENABLE)
            printf ("Error on nc_strioc W_ENABLE 'wans'\n");
        return 0;
    }
    answer = nc_close (pNc, pWwwLabel);
    if (answer == 0)
    {
        printf ("Error closing %s\n", pWwwLabel);
        return 0;
    }
    if (tracing == 1)
        printf ("Closed %s, %s. Answer %s\n", pServer, pProtoName,
                pServerLabel);
    if (command == W_ENABLE)
        printf ("Subnet %s enabled.\n", pSnid);
    if (command == W_DISABLE)
        printf ("Subnet %s disabled.\n", pSnid);
    return 1;
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

