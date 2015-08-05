/*   @(#) wantune.c 00/01/05 Version 1.7   */

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 * wantune.c of x25util module
 *
 * SpiderX25
 * @(#)$Id: wantune.c,v 1.5 2006/05/11 18:19:58 jing Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history:
 
Chg Date       	Init Description
1.  29-OCT-97   mpb  Compile on Windows NT.
2.  19-NOV-97   mpb  'mask' needs to be initialized before used.  I am assuming
3.  04-MAR-98   mpb  Compile on Windows NT (TCP/IP).
                     it was intended to be zero, but that is not a sure thing
                     as a default when declared.
4.  20-APR-98   mpb  Compile on QNX (TCP/IP).
5.  18-May-98   rjp  Merge Spider 8.1.1 release changes.
6.  26-jun-98   rjp  Add WAN_auto_enable, remove x.21.
7.   9-jul-98   rjp  mpb's Push swap module if needed (little endian
		               architecture).
8.  14-OCT-98   lmm  Updated usage strings
9.  01-SEP-99   tdg  Added VxWorks support
*/

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <fcntl.h>
#ifdef LINUX
#include        <unistd.h>
#include <netinet/in.h>
#include        <sys/socket.h>
#endif
#ifndef WINNT
#include        <errno.h>
#else
#include <windows.h>
#endif /* WINNT */
#include        <sys/types.h>

#if defined ( __hp9000s800 ) || defined ( WINNT ) || defined ( QNX )
#include        "xstypes.h"
#include        "xstra.h"
#else
#ifdef VXWORKS
#include        <sys/socket.h>
#include   	<in.h>
#include        <streams/stream.h>
#else
#ifndef LINUX
#include        <sys/socket.h>
#include        <sys/stream.h>
#endif
#endif /* VXWORKS */
#endif

#include        "xstopts.h"
#include        "uint.h"
#include        "wan_control.h"
#ifdef NO_NETD 
#include        "ll_control.h"
#endif

#ifdef  WINNT
#ifdef ERROR
#undef ERROR   // Gets defined in Windows system include file.
#endif /* ERROR */
#include <winsock.h>
#include <errno.h>
int    getopt ( int, char *const *, const char * );
#endif /* !WINNT */

#include        "mpsproto.h"
#include        <x25_proto.h>

#define  MPS_ERROR  -1


#define WANS_DEV    0

#define    ACCREAD    04

extern open_conf(), close_conf();
extern ushort     get_conf_ushort();
extern unsigned long get_conf_ulong();
extern char      *get_conf_line(); 
extern int  exierr ( );
extern int  exerr ( );
extern int  conf_exerr ( );
extern int  snidtox25 ( );
extern int  experr ( );

#ifdef    ANSI_C
extern void  exit_program ( int );
#else
extern void  exit_program ( );
#endif /* ANSI_C */

extern char     *optarg;
extern int     errno, optind;

static struct protos {
    char   *name;
    char   *device;
    int    blocklen;
    int    count;
    uint8    type;
} protos[] = {
    { "wan", WANS_DEV,
          sizeof(struct wan_tnioc),
          sizeof(wantune_t)/sizeof(uint16),
          WAN_TUNE },
    { 0 } };

static struct    wan_tnioc    wan_tn;
 

#define TIMEOUT     60
#define OPTIONS  "PGd:s:t:c:v:"    /* Valid command line options     */
#define USAGE1   "usage: %s  -P  [-d device] [-s subnet_id] [-t target] [-v service]\n               [-c controller] [filename]\n"
#define USAGE2   "       %s [-G] [-d device] [-s subnet_id] [-t target] [-v service] \n               [-c controller]\n"
                     

extern char     *optarg;
extern int      errno, optind;

extern char     *basename();

char     *prog_name;
static char     *subnetid, *device;

/* Normally wantune is run by netd which sets SNID */
/******
#define     NO_NETD    
******/

#ifdef    NO_NETD
static struct xstrioctl ctl;
static struct ll_snioc snioc;
#endif    /* NO_NETD */

/*************************************************************************
* main
*
*************************************************************************/

#ifdef VXWORKS
wantune (argc, argv, ptg)
#else
main (argc, argv)
#endif /* VXWORKS */
int    argc;
char    **argv;
#ifdef VXWORKS
pti_TaskGlobals *ptg;
#endif /* VXWORKS */
{
    struct protos	*prp;
    struct xstrioctl  	wan_ioc;
    wantune_t         	*tunep;
    int           	c;
    int           	dd;    /* Device descriptor */
    OpenRequest        	oreq;
#ifdef    NO_NETD
    int               	sock, lnk_mux;
#endif    /* NO_NETD */

    /* Booleans to show whether devices, snids etc are present */
    int get_reqd = 0, put_reqd = 0; 

#ifdef    WINNT         /* #3 */
    LPSTR        lpMsg;
    WSADATA      WSAData;
    int          optionValue = SO_SYNCHRONOUS_NONALERT;
#endif    /* WINNT */

#ifdef    WINNT         /* #3 */
    /*
    * For NT, need to initialize the Windows Socket DLL
    */
    if ( WSAStartup ( 0x0101, &WSAData) )
    {
        FormatMessage (
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, WSAGetLastError ( ), LANG_USER_DEFAULT,
            ( LPSTR ) &lpMsg, 0, NULL );
        fprintf ( stderr, "\n\nWSAStartup() failed:\n%s\n", lpMsg );

        LocalFree ( lpMsg );
        exit_program ( -1 );
    }
#endif    /* WINNT */

#ifdef VXWORKS
  init_mps_thread ( ptg );
#endif /* VXWORKS */

    memset ( &oreq, 0, sizeof ( oreq ) );
    oreq.dev = 0;    			/* default: device to zero 	*/
    strcpy ( oreq.serviceName, "mps" ); /* if -v not used 		*/
    prog_name = basename(argv[0]);
#ifdef VXWORKS
    taskVarAdd(0, &prog_name);
#endif /* VXWORKS */ 

    /* Get any command line options */
    while ((c = getopt (argc, argv, OPTIONS)) != EOF)
    {
        switch (c)
        {
        case 'd' :    /* device */
        device   = optarg;
        oreq.dev = strtol ( device, 0, 10 );
        break;

        case 's' :    /* subnetwork ID */
        if (strlen(optarg) > SN_ID_LEN)
        {
            fprintf(stderr, USAGE1, argv[0]);
            fprintf(stderr, USAGE2, argv[0]);
            exierr("subnet_id may be no more than %d characters long",
		    (int)SN_ID_LEN);
 	}
        subnetid = optarg;
        break;

        case 'P' :    /* put ioctl option */
        put_reqd = 1;
        break;

        case 'G' :    /* get ioctl option */
        get_reqd = 1;
        break;

        case 't' :    /* get server name */
        strcpy ( oreq.serverName, optarg );
        break;

        case 'v' :    /* get server name */
        strcpy ( oreq.serviceName, optarg );
        break;

        case 'c' :    /* get controller number */
        if ( ! strlen ( optarg ) )
            exerr("-c option is required.");
        oreq.ctlrNumber = strtol ( optarg, 0, 10 );
        break;

        case '?' :    /* ERROR */
        fprintf(stderr, USAGE1, prog_name);
        fprintf(stderr, USAGE2, prog_name);
        exit_program(1);
        }
    }

    if ( ! strlen ( oreq.serverName ) )
    	exerr("-t option is required.");

    if (put_reqd == 0 && get_reqd == 0)   
        get_reqd = 1;            	/*
                         		*  Set default to get if
                         		*  no option specifed
                         		*/
                         
    if (put_reqd == get_reqd)      	/* Put and get have same values */
        exerr("only one of -P or -G may be specifed");

    if (!subnetid)
	exerr("no subnetwork identifier given");

    /* shouldn't use this method. */
    for (prp = protos; prp->name; prp++)
        if (strcmp("wan", prp->name) == 0) break;

#ifdef DEBUG
    printf("wantune.c: contents of prp...\n");
    printf("prp->name=%s\n",prp->name);
    printf("prp->device=%s\n",prp->device);
    printf("prp->blocklen=%d\n",prp->blocklen);
    printf("prp->count=%d\n",prp->count);
    printf("prp->type=%d\n",prp->type);
#endif

    oreq.port  = 0;
    oreq.flags = O_RDWR;
#ifdef    NO_NETD
    strcpy ( oreq.protoName,   "lapb"  );
    printf ( "Sending Open Server Request.\n" );
    if ( ( sock = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
        printf ( "Unable to open connection to server: %d %d\n",
                     MPSerrno, errno );
        exit_program ( ( -1 ) );
    }
    printf ( "Open Server Successful %d\n", sock );
    /* #7 */
    if ( ntohl(0x1) != 0x1 )
    {
        if ( MPSioctl ( sock, X_PUSH, "llswap" ) == -1 )
        {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( -1 );
        }
    }

#endif    /* NO_NETD */

    strcpy ( oreq.protoName,   "wan"  );
    if ( ( dd = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
    	printf ( "Unable to open connection to server: %d %d\n",
                     MPSerrno, errno );
        exit_program ( ( -1 ) );
    }
    /* #7 */
    if ( ntohl(0x1) != 0x1 )
    {
        if ( MPSioctl ( dd, X_PUSH, "wanswap" ) == -1 )
        {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( -1 );
        }
    }

#ifdef    NO_NETD
    if ( ( lnk_mux = MPSioctl ( sock,
               X_LINK, dd ) ) == ( -1 ) )
    {
           printf ( "linkwan-Unable to send LINK: %d %d\n",
                     MPSerrno, errno );
           exit_program   ( -1 );
    }
    printf ( "X_LINK return value: %x\n", lnk_mux );
    ctl.ic_cmd = L_SETSNID;
    ctl.ic_timout = 10;
    ctl.ic_len = sizeof(struct ll_snioc);
    ctl.ic_dp = (char *)&snioc;

    snioc.lli_type = LI_SNID;
    snioc.lli_snid = 'A';
    snioc.lli_index = lnk_mux;

    if ( ( lnk_mux = MPSioctl ( sock,
               X_STR, &ctl ) ) == ( -1 ) )
    {
            printf ( "X_STR return value: %x %d %d\n",
                      lnk_mux, MPSerrno, errno );
            exit_program(1);
    }
    printf ( "X_STR return value: %x\n", lnk_mux );

/*
* Close the firstly opened wan stream as it is now useless because
* of the link to lapb.  Must open a DIRECT stream to WAN to send tuning
* parms - the other stream goes through lapb.
*/

    printf ( "Sending Close Server Request.\n" );

    if ( ( MPSclose ( dd ) ) == MPS_ERROR )
    {
	printf ( "Unable to Close connection to server: %d %d\n",
                     MPSerrno, errno );
        exit_program ( ( -1 ) );
    }
    printf ( "Close Server Successful %d\n", dd );
    printf ( "Sending Open Server Request.\n" );
    if ( ( dd = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
        printf ( "Unable to open connection to server: %d %d\n",
                     MPSerrno, errno );
        exit_program ( ( -1 ) );
    }
    printf ( "Open Server Successful %d\n", dd );
    /* #7 */
    if ( ntohl(0x1) != 0x1 )
    {
        if ( MPSioctl ( dd, X_PUSH, "wanswap" ) == -1 )
        {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( -1 );
        }
    }

#endif    /* NO_NETD */

    if (put_reqd)
    {  
        /* Read values from configuration file */
        open_conf(argv[optind]); 
        tunep = (wantune_t *)&wan_tn.wan_tune;

	memset ((char *) tunep, 0, sizeof (*tunep)); /* Clear it out	*/
        /*
            Get WAN_maxframe
        */
        tunep->WAN_hd.WAN_maxframe = get_conf_ushort();

        /*
            Get WAN_baud
        */
        tunep->WAN_hd.WAN_baud = get_conf_ulong();
        /*
            Get WAN_auto_enable
        */
        tunep->WAN_hd.WAN_auto_enable = 1; /* Default to YES		*/
        tunep->WAN_options = 0;
 
        {
            char *line = get_conf_line();
 
            if (!strchr("YyNn", line[0]) || line[1] != '\0')
                conf_exerr("WAN_auto_enable not 'Y|y' or 'N|n'");
            if (strchr("Nn", line[0]))
        	tunep->WAN_hd.WAN_auto_enable = 0; /* Don't enable lapb	*/
        }

        /*
            Get WAN_connect_proc (Call procedures. None or v.25bis.
        */
        tunep->WAN_hd.WAN_cpdef.WAN_cptype = get_conf_ushort();
	/*
	    Get WAN_v25_callreq (v.25 timeout)
	*/
        tunep->WAN_hd.WAN_cpdef.WAN_v25def.callreq = get_conf_ushort();

        close_conf(); 


        wan_tn.w_type = prp->type;
        wan_tn.w_snid = snidtox25(subnetid);
        wan_tn.w_spare[0] = 0;        
        wan_tn.w_spare[1] = 0;        
        wan_tn.w_spare[2] = 0;        

        wan_ioc.ic_cmd    = W_SETTUNE;
        wan_ioc.ic_timout = TIMEOUT;
        wan_ioc.ic_len    = prp->blocklen;
        wan_ioc.ic_dp     = (char *)&wan_tn;    

#ifdef DEBUG
        {
            uint16 *tuned;

            tuned = (uint16 *)&wan_tn.wan_tune;
            count = prp->count;
        
            while (count--)
            printf("%d\n", *tuned++);
        }
#endif
        if ( MPSioctl ( dd, X_STR, ( char * ) &wan_ioc ) == ( -1 ) )
            experr("W_SETTUNE ioctl failed on %d", MPSerrno);
#ifndef    NO_NETD
#ifdef VXWORKS
	exit_program ( 0 );
#else
        return 0;
#endif /* VXWORKS */
#endif    /* NO_NETD */
    }
#ifdef    NO_NETD
    if (put_reqd)
#else
    else            /* Get */
#endif    /* NO_NETD */
    {
        
        wan_tn.w_type = prp->type;
        wan_tn.w_snid = snidtox25(subnetid);
        wan_tn.w_spare[0] = 0;
        wan_tn.w_spare[1] = 0;
        wan_tn.w_spare[2] = 0;

        wan_ioc.ic_cmd    = W_GETTUNE;
        wan_ioc.ic_timout = TIMEOUT;
        wan_ioc.ic_len    = prp->blocklen;
        wan_ioc.ic_dp     = (char *)&wan_tn;

	if ( MPSioctl ( dd, X_STR, ( char * ) &wan_ioc ) == ( -1 ) )
            experr("W_GETTUNE ioctl failed on %d", MPSerrno);

        tunep = (wantune_t *)&wan_tn.wan_tune;

        /*
            Print WAN_maxframe
        */
        printf("%u\n", tunep->WAN_hd.WAN_maxframe);

        /*
            Print WAN_baud
        */
        printf("%u\n", (unsigned int)tunep->WAN_hd.WAN_baud);

	/*
	    Printf WAN_auto_enable
	*/
	if (tunep->WAN_hd.WAN_auto_enable == 1)
	    printf ("Y\n");
	else
	    printf ("N\n");
            
        /*
            Print WAN_cptype
        */
        printf("%d\n", tunep->WAN_hd.WAN_cpdef.WAN_cptype);
        
        /*
            Print WAN_v25def
        */
        printf("%d\n", tunep->WAN_hd.WAN_cpdef.WAN_v25def.callreq);

#ifdef VXWORKS
	exit_program ( 0 );
#else
        return 0;
#endif /* VXWORKS */
    }
}
