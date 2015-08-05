/*   @(#) lltune.c 00/01/05 Version 1.8   */

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
 * lltune.c of x25util module
 *
 * SpiderX25
 * @(#)$Id: lltune.c,v 1.5 2006/05/11 18:19:57 jing Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history:
 
Chg Date    	Init Description
1.  29-OCT-97   mpb Compile on Windows NT.
2.  04-MAR-98   mpb Compile on Windows NT (TCP/IP).
3.  20-APR-98   mpb Compile on QNX (TCP/IP).
4.  18-may-98 	rjp Merge with Spider 8.1.1 release.
5.   9-jul-98   rjp mpb's Push swap module if needed (little endian
                    architecture).
6.  04-SEP-98   mpb NT defines RESET differently than the way we do.  No
                    harm done since we do not use NT's RESET logic.
7.  14-OCT-98   lmm Updated usage strings
8.  02-SEP-99   tdg Added VxWorks support.
*/

#include        <stdio.h>
#ifdef LINUX
#include        <string.h>
#include        <unistd.h>
#include	<stdlib.h>
#include	<netinet/in.h>
#include        <sys/socket.h>
#endif
#include        <fcntl.h>
#ifndef WINNT
#include        <errno.h>
#else
typedef char *  caddr_t;
#include <windows.h>
#ifdef RESET   /* #6 */
#undef RESET
#endif /* RESET */
#endif /* WINNT */
#include        <sys/types.h>

#if defined ( __hp9000s800 ) || defined ( WINNT ) || defined ( QNX )
#include        "xstypes.h"
#include        "xstra.h"
#else
#ifdef VXWORKS
#include        <sys/socket.h>
#include        <in.h>
#include        <streams/stream.h>
#else
#ifndef LINUX
#include        <sys/socket.h>
#include        <sys/stream.h>
#endif
#endif /* VXWORKS */
#endif

#include        "xstopts.h"
#include        "mpsproto.h"
#include        "uint.h"
#include        "ll_control.h"
#include        "ll_proto.h"
#include        "x25_proto.h"

#ifdef  WINNT
#ifdef ERROR
#undef ERROR   /* Gets defined in Windows system include file. */
#endif /* ERROR */
#ifdef max
#undef max  /* Gets defined in Windows system include file */
#endif /* max */
#ifdef min
#undef min  /* Gets defined in Windows system include file */
#endif /* min */
#include <winsock.h>
#include <errno.h>
int    getopt ( int, char *const *, const char * );
#endif /* !WINNT */



#define  MPS_ERROR  -1

#define    LAPB_DEV    0
#define    LLC2_DEV "/dev/llc2"

#define    ACCREAD    04

extern open_conf(), close_conf();
extern ushort     get_conf_ushort();

extern int  exerr ( );
extern int  exierr ( );
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
    { "lapb", LAPB_DEV,
          sizeof(struct lapb_tnioc),
          (sizeof(lapbtune_t)/sizeof(uint16)),
          LI_LAPBTUNE },
    { "llc2", LLC2_DEV,
          sizeof(struct llc2_tnioc),
          sizeof(llc2tune_t)/sizeof(uint16),
          LI_LLC2TUNE },
    { 0 } };


/* Normally lltune is run by netd which sets SNID */
/******
#define  NO_NETD
******/

#ifdef  NO_NETD
struct xstrioctl ctl;
struct ll_snioc snioc;
#endif  /* NO_NETD */

#define TIMEOUT     60
#define OPTIONS  "PGd:s:p:t:c:v:"    /* Valid command line options     */
#define USAGE1 "usage: %s -s subnet_id -p protocol -P [-d device] \n             [-t target] [-v service] [-c controller] [filename]\n"
#define USAGE2 "usage: %s -s subnet_id -p protocol [-G] [-d device] \n             [-t target] [-v service] [-c controller] [filename]\n"
                     
extern ushort	get_conf_ushort();
extern char *   get_conf_line();

extern char     *optarg;
extern int      errno, optind;

extern char     *basename();

char     *prog_name;
static char     *subnetid, *device, *protocol;

#ifdef VXWORKS
lltune (argc, argv, ptg)
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
    lliun_t            	tnioc;
    struct xstrioctl  	strio;
    uint16            	*tunep;
    int           	c, count;
    int           	dd;    /* Device descriptor */
    int           	lp;
    OpenRequest         oreq;
#ifdef  NO_NETD
    int                 sock, lnk_mux;
#endif  /* NO_NETD */
    /* Booleans to show whether devices, snids etc are present */
    int get_reqd = 0, put_reqd = 0;

#ifdef    WINNT         /* #2 */
    LPSTR        lpMsg;
    WSADATA      WSAData;
    int          optionValue = SO_SYNCHRONOUS_NONALERT;
#endif    /* WINNT */

#ifdef    WINNT         /* #2 */
/* For NT, need to initialize the Windows Socket DLL */
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

    oreq.dev = 0;   /* default: device to zero */

    prog_name = basename(argv[0]);
#ifdef VXWORKS
    taskVarAdd(0, &prog_name);
#endif /* VXWORKS */ 

    strcpy ( oreq.serviceName, "mps" ); /* if -v not used */

    /* Get any command line options */
    while ((c = getopt (argc, argv, OPTIONS)) != EOF)
    {
        switch (c)
        {
        case 'd' :    /* device */
	device = optarg;
        oreq.dev = strtol ( device, 0, 10 );
        break;

        case 's' :    /* subnetwork ID */
        if ((int) strlen(optarg) > SN_ID_LEN)
        {
            fprintf(stderr, USAGE1, argv[0]);
            fprintf(stderr, USAGE2, argv[0]);
            exierr("subnet_id may be no more than %d characters long",
		    (int)SN_ID_LEN);
        }
        subnetid = optarg;
        break;

        case 'p' :    /* protocol */
        protocol = optarg;
        break;
                
        case 'P' :    /* put ioctl option */
        put_reqd = 1;
        break;

        case 'G' :    /* get ioctl option */
        get_reqd = 1;
        break;

        case 't' :      /* get server name */
        strcpy ( oreq.serverName, optarg );
        break;

        case 'v' :      /* get service name */
        strcpy ( oreq.serviceName, optarg );
        break;

        case 'c' :      /* get controller number */
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

    if (put_reqd == 0 && get_reqd == 0)
        get_reqd = 1;            /*
                         *  Set default to put if
                         *  no option specifed
                         */
                         
    if (put_reqd == get_reqd)      /* Put and get have same values */
    {
        fprintf(stderr, USAGE1, argv[0]);
        fprintf(stderr, USAGE2, argv[0]);
        exerr("only one of -P or -G may be specifed");
    }

    if (!subnetid)
    {
        fprintf(stderr, USAGE1, argv[0]);
        fprintf(stderr, USAGE2, argv[0]);
        exerr("no subnetwork identifier given");
    }

    if (!protocol)
    {
        fprintf(stderr, USAGE1, argv[0]);
        fprintf(stderr, USAGE2, argv[0]);
        exerr("no protocol specified");
    }
        if ( ! strlen ( oreq.serverName ) )
           exerr("-t option is required.");

        oreq.port  = 0;
        oreq.flags = O_RDWR;

    /* Find protocol and check */
    for (prp = protos; prp->name; prp++)
        if (strcmp(protocol, prp->name) == 0)
	    break;
    if (!prp->name)
	exerr("%s is an invalid protocol", protocol);

    strcpy ( oreq.protoName,   "lapb"  );
    if ( ( dd = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
            printf ( "Unable to open connection to server: %d %d\n",
                     MPSerrno, errno );
            exit_program ( ( -1 ) );
    }
 
    /* #5 */
    if ( ntohl(0x1) != 0x1 )
    {
        if ( MPSioctl ( dd, X_PUSH, "llswap" ) == MPS_ERROR )
        {
	    MPSperror ( "MPSioctl swap X_PUSH" );
             exit_program ( MPS_ERROR );
        }
    }


#ifdef  NO_NETD
    strcpy ( oreq.protoName,   "wan"  );
    printf ( "Sending Open Server Request.\n" );
    if ( ( sock = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
           printf ( "Unable to open connection to server: %d %d\n",
                     MPSerrno, errno );
           exit_program ( ( -1 ) );
    }
    printf ( "Open Server Successful %d\n", sock );

    /* #5 */
    if ( ntohl(0x1) != 0x1 )
    {
        if ( MPSioctl ( sock, X_PUSH, "wanswap" ) == MPS_ERROR )
        {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( MPS_ERROR );
        }
    }

    if ( ( lnk_mux = MPSioctl ( dd,
               X_LINK, sock ) ) == ( -1 ) )
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

    if ( ( lnk_mux = MPSioctl ( dd,
               X_STR, &ctl ) ) == ( -1 ) )
    {
            printf ( "X_STR return value: %x %d %d\n",
                      lnk_mux, MPSerrno, errno );
            exit_program(1);
    }
    printf ( "X_STR return value: %x\n", lnk_mux );
#endif  /* NO_NETD */

    if (put_reqd)
    {
        /* Read values from configuration file */
        open_conf(argv[optind]);
        tunep = (uint16 *)&tnioc.lapb_tn.lapb_tune;
        count = prp->count; 
/*
 * Since the last 5 mask fields have been added to def.lapb we can not
 * use the old method of reading in the data. For the last five fields
 * we have to read in 5 lines and set up the relevant bit mask.
 * Hence if we decrement the count ( i.e. don't read in last field )
 * and read in five lines, we have the complete file.
 */
        if (prp->type == LI_LAPBTUNE)
            count -= 2;			/* #4				*/
        while (count--)
            *tunep++ = get_conf_ushort();
	/*
	* #4 There are six y/n parameters at end of file.
	*/
        for ( lp = 1 ; prp->type == LI_LAPBTUNE && lp < (1<<6); lp<<=1 )
        {
            char *line = get_conf_line();

            if (!strchr("YyNn", line[0]) || line[1] != '\0')
                conf_exerr("Invalid field, not 'Y|y' or 'N|n'");
            if (strchr("Yy", line[0]))
                *tunep |= lp;
        }
        close_conf();

        tnioc.ll_hd.lli_type = prp->type;
        tnioc.ll_hd.lli_snid = snidtox25(subnetid);
        tnioc.ll_hd.lli_spare[0] = 0;
        tnioc.ll_hd.lli_spare[1] = 0;
        tnioc.ll_hd.lli_spare[2] = 0;

        strio.ic_cmd    = L_SETTUNE;
        strio.ic_timout = TIMEOUT;
        strio.ic_len    = prp->blocklen;
        strio.ic_dp     = (char *)&tnioc;    

                if ( MPSioctl ( dd, X_STR, ( char * ) &strio ) == ( -1 ) )
                        experr("L_SETTUNE ioctl failed on %d", MPSerrno);

#ifndef  NO_NETD
#ifdef VXWORKS
	exit_program ( 0 );
#else
        return 0;
#endif /* VXWORKS */
#endif  /* NO_NETD */
    }
#ifdef  NO_NETD
    if (put_reqd)
#else
    else            /* Get */
#endif  /* NO_NETD */
    {
        tnioc.ll_hd.lli_type = prp->type;
        tnioc.ll_hd.lli_snid = snidtox25(subnetid);
        tnioc.ll_hd.lli_spare[0] = 0;
        tnioc.ll_hd.lli_spare[1] = 0;
        tnioc.ll_hd.lli_spare[2] = 0;

        strio.ic_cmd    = L_GETTUNE;
        strio.ic_timout = TIMEOUT;
        strio.ic_len    = prp->blocklen;
        strio.ic_dp     = (char *)&tnioc;    

                if ( MPSioctl ( dd, X_STR, ( char * ) &strio ) == ( -1 ) )
                        experr("L_GETTUNE ioctl failed on %d", MPSerrno);

        tunep = (uint16 *)&tnioc.lapb_tn.lapb_tune;
        count = prp->count; 

  	/*
  	 * Same applies as above.
   	*/
        if (prp->type == LI_LAPBTUNE)
            count--;
        while (count--)
            printf("%d\n", *tunep++);
        for ( lp = 1 ; prp->type == LI_LAPBTUNE && lp < (1<<5); lp<<=1 )
            printf("%c\n",*tunep & lp ? 'Y' : 'N');

#ifdef VXWORKS
        exit_program ( 0 );
#else
        return 0;
#endif /* VXWORKS */
    }
}


