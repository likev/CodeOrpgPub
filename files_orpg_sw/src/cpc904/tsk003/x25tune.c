/*   @(#) x25tune.c 00/09/08 Version 1.8   */

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
 * x25tune.c of x25util module
 *
 * SpiderX25
 * @(#)$Id: x25tune.c,v 1.7 2006/05/11 18:20:00 jing Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history:
 
Chg Date    	Init Description
1.  29-OCT-97   mpb  Compile on Windows NT.
2.  04-MAR-98   mpb  Compile on Windows NT (TCP/IP).
3.  20-APR-98   mpb  Compile on QNX (TCP/IP).
4.  18-may-98   rjp  Merge with Spider 8.1.1.
5.   9-jul-98   rjp  mpb's Push swap module if needed (little endian
		             architecture).
6.  04-SEP-98   mpb  NT defines RESET differently than the way we do.  No
                     harm done since we do not use NT's RESET logic.
7.  14-OCT-98   lmm  Updated usage strings
8.  02-SEP-99   tdg  Added VxWorks support.
9.  21-NOV-00   djb  Added User-initiated restarts flag.
*/

#define NOTIFY_STATUS
#define USER_RESTART

#include        <stdio.h>
#ifdef LINUX
#include        <string.h>
#include        <unistd.h>
#include	<stdlib.h>
#include	<netinet/in.h>
#include	<ctype.h>
#include        <sys/socket.h>
#endif
#include        <fcntl.h>
#ifndef WINNT
#include        <errno.h>
#else
typedef char *  caddr_t;
#include        <windows.h>
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
#include 	<in.h>
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
#include        "x25_control.h"


#ifdef  WINNT
#ifdef ERROR
#undef ERROR   // Gets defined in Windows system include file.
#endif /* ERROR */
#ifdef max
#undef max  // Gets defined in Windows system include file
#endif /* max */
#ifdef min
#undef min  // Gets defined in Windows system include file
#endif /* min */
#include <winsock.h>
#include <errno.h>
int    getopt ( int, char *const *, const char * );
#endif /* !WINNT */

#define bit8 unsigned char


#define  MPS_ERROR  -1


#define    ACCREAD    04

extern open_conf(), close_conf();
extern char      *get_conf_line(); 
extern bit8     get_conf_unchar(); 
extern ushort     get_conf_ushort(), get_conf_hex();

extern char     *optarg;
extern int     errno, optind;

static         int read_x25_config();

/* Normally wantune is run by netd which sets SNID */
/******
#define  NO_NETD
******/

#ifdef  NO_NETD
struct xstrioctl ctl;
struct ll_snioc snioc;
#endif  /* NO_NETD */

#define chcnt(x,y)  ((x)==0 ? 0 : ((y)-(x)+1) )

#define X25_DEV        0    /* Default x25 device          */
#define TIMEOUT     60
#define VALIDOPTIONS    "PGd:s:a:c:t:v:"    /* Valid command line options     */
#define    USAGE1 "usage: %s  -s subnet_id  -P  [-d device] [-a local address]\n                [-t target] [-v service] [-c controller] [filename]\n"
#define    USAGE2 "usage: %s  -s subnet_id [-G] [-d device]\n                [-t target] [-v service] [-c controller] [filename]\n"
                     
static struct  wlcfg     config;
static struct  xstrioctl  strio;

char *prog_name;
static char *subnetid, *device = X25_DEV, *locaddr = NULL;

extern char *basename();

extern int  exerr ( );
extern int  conf_exerr ( );
extern int  snidtox25 ( );
extern int  experr ( );
extern int  hex_val ( );
extern int  print_ioctl ( );

#ifdef    ANSI_C
extern void  exit_program ( int );
#else
extern void  exit_program ( );
#endif /* ANSI_C */

#ifdef VXWORKS
x25tune (argc, argv, ptg)
#else
main (argc, argv)
#endif /* VXWORKS */
int    argc;
char  **argv;
#ifdef VXWORKS
pti_TaskGlobals *ptg;
#endif /* VXWORKS */
{
    int    c;
    int     put_reqd = 0, get_reqd = 0;
    int     xsock, result;            /* Device descriptor */
#ifdef  NO_NETD
    int                lsock, wsock, llnk_mux, xlnk_mux;
#endif  /* NO_NETD */
    OpenRequest        oreq;

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

    strcpy ( oreq.serviceName, "mps" );  /* if -v not used */

    /* Get any command line options */
    while ((c = getopt (argc, argv, VALIDOPTIONS)) != EOF)
    {
        switch (c)
        {
        case 'd' :    /* Device specified */
        device = optarg;    
        oreq.dev = strtol ( device, 0, 10 );
        break;

        case 'a' :    /* Local address */
        if (strlen (optarg) > DTEMAXSIZE)
                    exerr("address too long");
        locaddr = optarg;
        break;
                
        case 's' :    /* subnet_id specified */
        subnetid = optarg;
        break;

        case 'P' :    /* putioctl option */
        put_reqd = 1;
        break;

        case 'G' :    /* getioctl option */
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
        fprintf(stderr, USAGE1, argv [0]);
        fprintf(stderr, USAGE2, argv [0]);
        exit_program (1);
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

    if ( ! strlen ( oreq.serverName ) )
           exerr("-t option is required.");
    oreq.port  = 0;
    oreq.flags = O_RDWR;

#ifdef  NO_NETD
/* Open lapb stream */

    strcpy ( oreq.protoName,   "lapb"  );
    printf ( "Sending Open Server Request.\n" );
    if ( ( lsock = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
           printf ( "Unable to open connection to server: %d %d\n",
                     MPSerrno, errno );
           exit_program ( ( -1 ) );
    }
    printf ( "Open Server Successful %d\n", lsock );
    /* #5 */
    if ( ntohl(0x1) != 0x1 )
    {
        if ( MPSioctl ( lsock, X_PUSH, "llswap" ) == -1 )
        {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( -1 );
	}
    }


/* Open wan stream */

    strcpy ( oreq.protoName,   "wan"  );
    printf ( "Sending Open Server Request.\n" );
    if ( ( wsock = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
           printf ( "Unable to open connection to server: %d %d\n",
                     MPSerrno, errno );
           exit_program ( ( -1 ) );
    }
    printf ( "Open Server Successful %d\n", wsock );
    /* #5 */
    if ( ntohl(0x1) != 0x1 )
    {
        if ( MPSioctl ( wsock, X_PUSH, "wanswap" ) == -1 )
        {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( -1 );
        }
    }

#endif  /* NO_NETD */

/* Open x25 stream */

    strcpy ( oreq.protoName,   "x25"  );
    if ( ( xsock = MPSopen ( &oreq ) ) == MPS_ERROR )
    {
           printf ( "Unable to open connection to server: %d %d\n",
                     MPSerrno, errno );
           exit_program ( ( -1 ) );
    }
    /* #5 */
    if ( ntohl(0x1) != 0x1 )
    {
        if ( MPSioctl ( xsock, X_PUSH, "x25swap" ) == -1 )
        {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( -1 );
        }
    }


#ifdef  NO_NETD

/* Link lapb to wan */

    if ( ( llnk_mux = MPSioctl ( lsock,
               X_LINK, wsock ) ) == ( -1 ) )
    {
           printf ( "linkwan-Unable to send LINK: %d %d\n",
                     MPSerrno, errno );
           exit_program   ( -1 );
    }

/* Link lapb to wan */

    if ( ( xlnk_mux = MPSioctl ( xsock,
               X_LINK, lsock ) ) == ( -1 ) )
    {
           printf ( "linkwan-Unable to send LINK: %d %d\n",
                     MPSerrno, errno );
           exit_program   ( -1 );
    }

/* Close unuseable streams to lapb and wan */

    printf ( "Sending Close Server Request.\n" );

    if ( ( MPSclose ( wsock ) ) == MPS_ERROR )
    {
           printf ( "Unable to Close connection to server: %d %d\n",
                     MPSerrno, errno );
           exit_program ( ( -1 ) );
    }
    printf ( "Close Server Successful %d\n", wsock );

    printf ( "Sending Close Server Request.\n" );

    if ( ( MPSclose ( lsock ) ) == MPS_ERROR )
    {
           printf ( "Unable to Close connection to server: %d %d\n",
                     MPSerrno, errno );
           exit_program ( ( -1 ) );
    }
    printf ( "Close Server Successful %d\n", lsock );

/* Send the subnet id to lapb */

    ctl.ic_cmd = L_SETSNID;
    ctl.ic_timout = 10;
    ctl.ic_len = sizeof(struct ll_snioc);
    ctl.ic_dp = (char *)&snioc;

    snioc.lli_type = LI_SNID;
    snioc.lli_snid = 'A';
    snioc.lli_index = llnk_mux;

    if ( ( llnk_mux = MPSioctl ( lsock,
               X_STR, &ctl ) ) == ( -1 ) )
    {
            printf ( "X_STR return value: %x %d %d\n",
                      llnk_mux, MPSerrno, errno );
            exit_program(1);
    }
        printf ( "X_STR return value: %x\n", llnk_mux );
#endif  /* NO_NETD */
    if (put_reqd)
    {
        /* Fill in the SNID  */
        config.U_SN_ID = snidtox25(subnetid);

        /* Fill in local_address (if present)  */
        if (locaddr)
        {
            int str_len, byte;
            bit8 *ucp;

            if ((str_len = strlen(locaddr)) > LSAPMAXSIZE*2)
                exerr("local address too long");
             config.local_address.lsap_len = str_len;
             ucp = &config.local_address.lsap_add[0];
    
            for (byte = 0; byte < str_len; byte++)
            {
                int digit;
    
                if ((digit = hex_val(locaddr[byte])) < 0)
                    exerr("local address not hex");
                if ((byte & 1) == 0)
                    *ucp = digit<<4;
                else
                    *ucp++ |= digit;
            }
        }
        else
        {
             /* set locaddr to null address */
            memset((char * )&config.local_address, 0, 
                sizeof(config.local_address));
         }

        /* Read config file into the other fields of 'config'*/
        open_conf(argv[optind]);
        read_x25_config(&config);
        close_conf();


        /* Set up the control block and issue the IOCTL */

        strio.ic_cmd    = N_snconfig;
        strio.ic_timout = TIMEOUT;
        strio.ic_len    = sizeof(config);
        strio.ic_dp     = (char *)&config;    

                if ( ( result = MPSioctl ( xsock,
                       X_STR, ( char * ) &strio ) ) == ( -1 ) )
                {
                    MPSperror ( "MPSioctl - X_STR" );
                    printf ( "X_STR N_snconfig return value: %x %d %d\n",
                              result, MPSerrno, errno );
                    exit_program(1);
                }
    }
    else
    {
        config.U_SN_ID  = snidtox25(subnetid);
        strio.ic_cmd    = N_snread;
        strio.ic_timout = TIMEOUT;
        strio.ic_len    = sizeof( config );
        strio.ic_dp     = (char *) &config;
        if ( ( result = MPSioctl ( xsock,
                       X_STR, ( char * ) &strio ) ) == ( -1 ) )
        {
                    printf ( "X_STR N_snread return value: %x %d %d\n",
                              result, MPSerrno, errno );
                    exit_program(1);
        }
        print_ioctl();
    }

#ifdef VXWORKS 
    exit_program ( 0 );
#else
    return 0;
#endif /* VXWORKS */
}


static int read_x25_config(cfgp)
register struct wlcfg *cfgp;
{
    unsigned char *ucp;
    short *sp;
    unsigned int lp;

    cfgp->NET_MODE = get_conf_unchar();

    switch (get_conf_unchar())
    {
    case 84: cfgp->X25_VSN = 1;
         break;

    case 88: cfgp->X25_VSN = 2;
         break;
        
    default:
    case 80: cfgp->X25_VSN = 0;
         break;
    }

    cfgp->L3PLPMODE = get_conf_unchar();
/*  Get Channel Ranges  */
    cfgp->LPC = get_conf_hex(4);
    cfgp->HPC = get_conf_hex(4);
    cfgp->NPCchannels = (short) chcnt(cfgp->LPC, cfgp->HPC);
    cfgp->LIC = get_conf_hex(4);
    cfgp->HIC = get_conf_hex(4);
    cfgp->LTC = get_conf_hex(4);
    cfgp->HTC = get_conf_hex(4);
    cfgp->LOC = get_conf_hex(4);
    cfgp->HOC = get_conf_hex(4);

/*  Set number of channels  */
    cfgp->NICchannels = (short) chcnt(cfgp->LIC, cfgp->HIC);
    cfgp->NTCchannels = (short) chcnt(cfgp->LTC, cfgp->HTC);
    cfgp->NOCchannels = (short) chcnt(cfgp->LOC, cfgp->HOC);
    cfgp->Nochnls = (short) (cfgp->NPCchannels +
	    cfgp->NICchannels +
            cfgp->NTCchannels +
            cfgp->NOCchannels);

    /*  Get THISGFI  */
    {
        int num = get_conf_unchar();

        if (num == 8)
            cfgp->THISGFI = (bit8) G_8;
        else if (num == 128)
            cfgp->THISGFI = (bit8) G_128;
        else
            conf_exerr("invalid value for THISGFI");
    }

/*  Get Window and Packet sizes */
    cfgp->LOCMAXPKTSIZE =  get_conf_unchar();
    cfgp->REMMAXPKTSIZE =  get_conf_unchar();
    cfgp->LOCDEFPKTSIZE =  get_conf_unchar();
    cfgp->REMDEFPKTSIZE =  get_conf_unchar();
    cfgp->LOCMAXWSIZE   =  get_conf_unchar();
    cfgp->REMMAXWSIZE   =  get_conf_unchar();
    cfgp->LOCDEFWSIZE   =  get_conf_unchar();
    cfgp->REMDEFWSIZE   =  get_conf_unchar();

    cfgp->MAXNSDULEN = get_conf_ushort(); 
    
    cfgp->ACKDELAY = get_conf_ushort();

/*  Get Timer Values  */

    sp = &cfgp->T20value;
        
    for (lp = 0; lp < 10; lp++)		/* #4 T28 added			*/
        *sp++ = get_conf_ushort();
    
/*  Get Retransmission Counts  */
    ucp = &cfgp->R20value;
        
    for (lp = 0; lp < 4; lp++)		/* #4 R28 added			*/
        *ucp++ = get_conf_unchar();


/*  Local values for qos checking  */
    cfgp->localdelay     = get_conf_ushort();
    cfgp->accessdelay    = get_conf_ushort();
    cfgp->locmaxthclass  = get_conf_unchar();
    cfgp->remmaxthclass  = get_conf_unchar();
    cfgp->locdefthclass  = get_conf_unchar();
    cfgp->remdefthclass  = get_conf_unchar();
    cfgp->locminthclass  = get_conf_unchar();
    cfgp->remminthclass  = get_conf_unchar();

    /*  Get Closed User Group fields  */    
    cfgp->CUG_CONTROL = 0;
    for (lp = 1; lp < (1<<4); lp<<=1)
    {
        char *line = get_conf_line();

        if (!strchr("YyNn", line[0]) || line[1] != '\0')
            conf_exerr("Closed User Group not 'Y|y' or 'N|n'");
        if (strchr("Yy", line[0]))
            cfgp->CUG_CONTROL |= lp;
    }
    /*
     * Get CUG_FORMAT
     *    0 - Basic Format, that is, DTE belongs to <= 100 CUG's
     *    1 - Extended Format, that is, DTE belongs to
     *            > 100  &  <= 10000 CUG's
     */
    {
        int num = get_conf_unchar();

        if (num == 0)
            cfgp->CUG_CONTROL |= (1<<4);
        else if (num == 1)
            cfgp->CUG_CONTROL |= (1<<5);
        else
            conf_exerr("invalid value for CUG_FORMAT");
    }

    {
        char *line = get_conf_line();
        
        if (!strchr("YyNn", line[0]) || line[1] != '\0')
            conf_exerr("Closed User Group not 'Y|y' or 'N|n'");
        if (strchr("Yy", line[0]))
            cfgp->CUG_CONTROL |= (1<<6);
    }

    /*  Get SUB_MODES  */    
    cfgp->SUB_MODES = 0;

    for (lp = 1; lp < (1<<12); lp<<=1)	/* #4				*/
    {
        char *line = get_conf_line();

        if (!strchr("YyNn", line[0]) || line[1] != '\0')
            conf_exerr("Sub_mode not 'Y|y' or 'N|n'");
        if (strchr("Yy", line[0]))
            cfgp->SUB_MODES |= lp;
    }

/*  Start of PSDN localisation record  */

    for (lp = 1; lp < (1<<7); lp<<=1)
    {
        char *line = get_conf_line();

        if (!strchr("YyNn", line[0]) || line[1] != '\0')
            conf_exerr("SNMODES not 'Y|y' or 'N|n'");
        if (strchr("Yy", line[0]))
            cfgp->psdn_local.SNMODES |= lp;
    }

/*  Get the 2 international call related values  */

    cfgp->psdn_local.intl_addr_recogn = get_conf_unchar();

    {
        char *line = get_conf_line();
        
        if (strchr("Yy", line[0]))
            cfgp->psdn_local.intl_prioritised = 1;
        else
            cfgp->psdn_local.intl_prioritised = 0;
    
        line = get_conf_line();

        for (lp = 0; lp < 4; lp++)
            if (!isdigit(line[lp]))
                conf_exerr("'dnic' not BCD");
        cfgp->psdn_local.dnic1 =
            (bit8) ((line[0] - '0') * 16 + line[1] - '0');
        cfgp->psdn_local.dnic2 =
            (bit8) ((line[2] - '0') * 16 + line[3] - '0');
    }

    cfgp->psdn_local.prty_encode_control   = get_conf_unchar();
    cfgp->psdn_local.prty_pkt_forced_value = get_conf_unchar();
    cfgp->psdn_local.src_addr_control      = get_conf_unchar();

/*  Set the D-bit fields  */
    cfgp->psdn_local.dbit_control = 0;
    for (lp = 1; lp < (1<<8); lp<<=2)
    {
        int num = get_conf_unchar();

        if (num < 0 || num > 2)
            conf_exerr("D-bit field not in 0, 1 or 2");
        if (num == 1)
            cfgp->psdn_local.dbit_control |= (lp<<1);
        if (num == 2)
            cfgp->psdn_local.dbit_control |= lp;
    }

    {
        char *line = get_conf_line();
        
        if (strchr("Yy", line[0]))
            cfgp->psdn_local.thclass_neg_to_def = 1;
        else
            cfgp->psdn_local.thclass_neg_to_def = 0;
    }


    cfgp->psdn_local.thclass_type = get_conf_unchar();

/*  Get packet and window map  */
    for (lp = 0; lp < 2; lp++)
    {
        char *line = get_conf_line();
        int str_len = strlen(line);
        int byte;
        int n;

        /* Set ptr to window map */
        ucp = (lp == 0 ? cfgp->psdn_local.thclass_wmap
                   : cfgp->psdn_local.thclass_pmap);

        for (byte = 0, n = 0; byte < str_len; byte++)
        {
            int tot = 0;

            while ((byte < str_len) &&
                (!(line[byte] == '.' || line[byte] == ',')))
            {
                if (!isdigit(line[byte])) conf_exerr("syntax error");
                tot *= 10;
                tot += (line[byte++] - '0');
            }
            if (n == 16) conf_exerr("too many values");
            ucp[n++] = (bit8) tot;
        }
    }

#ifdef USER_RESTART /* #9 */
    {
        char *line = get_conf_line();
        
        if (strchr("Yy", line[0]))
            cfgp->user_restart_enabled = 1;
        else
            cfgp->user_restart_enabled = 0;
    }
#endif /* USER_RESTART */

    return 0;
}

print_ioctl()
{
    unsigned char *ucp;
    unsigned short us, mask;
    short *sp;
    int num, byte;
    unsigned int lp;

/*  Print NET_MODE  */    
    printf("%d\n", config.NET_MODE);

/*  Print X25_VSN  */
    switch (config.X25_VSN)
    {
    case 1: printf("84\n");
        break;

    case 2: printf("88\n");
        break;
        
    default:
    case 0: printf("80\n");
        break;
    }
    
/*  Print L3PLPMODE  */
    printf("%d\n", config.L3PLPMODE);

/*  Print Channel Ranges  */
    sp = &config.LPC;
    for (lp = 0; lp < 8; lp++)
    {
        printf("%X\n", *sp);
        sp++;
    }

/*  Print THISGFI  */
    num = config.THISGFI;
    if (num == G_8)
        printf("8\n");
    else if (num == G_128)
        printf("128\n");
    else
        exerr("Invalid Value for THISGFI");

/*  Print Window and Packet sizes */
    printf("%d\n", config.LOCMAXPKTSIZE);
    printf("%d\n", config.REMMAXPKTSIZE);
    printf("%d\n", config.LOCDEFPKTSIZE);
    printf("%d\n", config.REMDEFPKTSIZE);
    printf("%d\n", config.LOCMAXWSIZE);
    printf("%d\n", config.REMMAXWSIZE);
    printf("%d\n", config.LOCDEFWSIZE);
    printf("%d\n", config.REMDEFWSIZE);

/*  Print MAXNSDULEN  */
    printf("%d\n", config.MAXNSDULEN);
        
/*  Print ACKDELAY  */    
    printf("%d\n", config.ACKDELAY);

/*  Print Timer Values  */

    sp = &config.T20value;
        
    for (lp = 0; lp < 9; lp++)
    {
        printf("%d\n", *sp);
        sp++;
    }
    
/*  Print Retransmission Counts  */
    ucp = &config.R20value;
        
    for (lp = 0; lp < 3; lp++)
    {
        printf("%d\n", *ucp);
        ucp++;
    }

/*  Local values for qos checking  */
    
    printf("%d\n", config.localdelay);
    printf("%d\n", config.accessdelay);

    ucp = (unsigned char *) &config.locmaxthclass;

    for (lp = 0; lp < 6; lp++)
    {
        printf("%d\n", *ucp);
        ucp++;
    }

/*  Print Closed User Group */
    mask = 1;
    us = (unsigned short) config.CUG_CONTROL;
    
    for (lp = 0 ; lp < 4; lp++)
    {
        if (us & mask)
            printf("Y\n");
        else
            printf("N\n");
        mask <<= 1;
    }
    if (us & mask)
        printf("0\n");
    else
        printf("1\n");
        
    mask <<= 1;
    if (us & mask)
        printf("Y\n");
    else
        printf("N\n");

/*  Print SUB_MODES  */
    mask = 1;
    us = (unsigned short) config.SUB_MODES;
    for (lp = 0 ; lp < 11; lp++)
    {
        if (us & mask)
            printf("Y\n");
        else
            printf("N\n");
        mask <<= 1;
    }

/*  Start of PSDN localisation record  */
    mask = 1;
    us = (unsigned short) config.psdn_local.SNMODES;
    
    for (lp = 0 ; lp < 7; lp++)
    {
        if (us & mask)
            printf("Y\n");
        else
            printf("N\n");
        mask <<= 1;
    }
    

/*  Print the 2 international call related values  */

    ucp = (unsigned char *) &config.psdn_local.intl_addr_recogn;
    printf("%d\n", *ucp);
    ucp++;
    if (*ucp)    /* INTL_PRIORITISED */
        printf("Y\n");
    else
        printf("N\n");    
    ucp++;

/*  Print the DNIC bytes  */
    if (*ucp < 10)
        printf("0");
    printf("%X", *ucp);
    ucp++;
    if (*ucp < 10)
        printf("0");
    printf("%X\n", *ucp);

    ucp++;
    for (lp = 0; lp < 3; lp++)
    {
        printf("%d\n", *ucp);
        ucp++;
    }

/* Print D-bit values */
    num = config.psdn_local.dbit_control;
    
#ifdef DEBUG
    fprintf(stderr, "dbit_control = #%x\n", num);
#endif
    for (lp = 1; lp < (1<<8); lp<<=2)
        {
        int bit1 = num & lp;
        int bit2 = num & (lp<<1);
#ifdef DEBUG
        fprintf(stderr, "bit1 = %d\tbit2 = %d\n", bit1, bit2);
#endif
        if (bit1)
            printf("2\n");
        else if (bit2)
            printf("1\n");
        else
            printf("0\n");
    }
    

    ucp = (unsigned char *) &config.psdn_local.thclass_neg_to_def;
    if (*ucp)    /* TCNEG_TO_DEFAULT */
        printf("Y\n");
    else
        printf("N\n");    

/* Print thclass type */
    ucp = (unsigned char *) &config.psdn_local.thclass_type;
    printf("%d\n", *ucp);

/*  Print packet and window map  */
    for (lp = 0; lp < 2; lp++)
    {
        if (lp == 0)        /* Set ptr to window map */
            ucp = (unsigned char *) config.psdn_local.thclass_wmap;
        else
            ucp = (unsigned char *) config.psdn_local.thclass_pmap;

        for (byte = 1; byte <= 16; byte++)
        {
            printf("%d", *ucp);
            if (byte < 16)
                printf(".");
            ucp++;
        }
        printf("\n");
    }

/*  Print local address  */
#ifdef DEBUG
    fprintf(stderr, "Local address: ");

    if (config.local_address.lsap_len != 0)
    {
        byte = ((config.local_address.lsap_len - 1) / 2) + 1;

        for (lp = 0; lp < byte; lp++)
        {
            if (config.local_address.lsap_add [lp] < 0x10)
                fprintf(stderr, "0");
            fprintf (stderr, "%X", config.local_address.lsap_add [lp]);
        }
        fprintf (stderr, "\n");
    }
    else
        fprintf (stderr, "Not Set\n");
#endif

   return 0;
}
