/*   @(#) x25stat.c 98/08/13 Version 1.9   */


/************************************************************************
*									*
*		    Please set tabs at eight spaces.			*
*									*
*									*
************************************************************************/

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
 * x25stat.c of x25stat module
 *
 * SpiderX25
 * @(#)$Id: x25stat.c,v 1.3 2002/05/14 19:09:10 eddief Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history.
Chg Date       Init Description
 1. 14-Jul-98  rjp  Add push for little endian host. Define ROOTRESET so
                    don't have to be root to reset stats.
 2. 27-Jul-98  rjp  Display stats on controller's other than 0.
 3. 13-AUG-98  mpb  Took out #define for RESET and REGISTERING because
                    they are defined in the Windows System include
                    files, AND they are not used in this program.
 4. 04-SEP-98  mpb  NT defines RESET differently than the way we do.  No
                    harm done since we do not use NT's RESET logic.
 5. 12-OCT-98  mpb  Need NT path to x25conf file.
 6. 04-FEB-99  kls  Added support for wan statistics.
 7. 08-OCT-99  tdg  Added VxWorks support
*/


/*
 * Options :
 *    -a    abbreviated VC statistics
 *    -c     MPS controller
 *    -l     show statistics for virtual circuit with given chnl
 *    -L     as above, but for all channels
 *    -n     show subnetwork statistics
 *    -N     show all subnetwork statistics
 *    -p     show glob statistics for protocol current protos are X25 and LAPB.
 *    -s     MPS server name
 *    -v     MPS service. Only applies to servers, not embedded boards.
 *    -w     show wan statistics (not available yet)
 *    -z     reset statistics
 *
 */


/*  Valid protos are currently:
 *
 *    "x25"    "lapb"
 */


/*
 *  includes
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifndef LINUX
#ifndef GCC
typedef unsigned int    bit32;
#endif
#endif
#ifdef WINNT
typedef char * caddr_t;
#include <winsock.h>
int getopt (int, char *const *, const char *);
#ifdef RESET   /* #4 */
#undef RESET
#endif /* RESET */
#endif /* !WINNT */ 
#if defined ( WINNT ) || defined ( QNX )
#include "xstypes.h"
#include "xstra.h"
#else
#ifdef VXWORKS
#include <signal.h>
#include <in.h>
#include <streams/stream.h>
#else
#include <sys/stream.h>
#endif /* VXWORKS */
#endif /* __hp9000s800 || WINNT || QNX */ 

#include "xstopts.h"
#include "mpsproto.h"

#include <sys/snet/uint.h> 
#include <sys/snet/ll_proto.h> 
#include <sys/snet/ll_control.h> 
#include <sys/snet/mlp_control.h> 
#include <sys/snet/x25_proto.h> 
#include <sys/snet/x25_control.h>
#include <sys/snet/x25states.h> 
#include <sys/snet/snmp.h>
#include <sys/snet/x25_mib.h>
#include <sys/snet/sx25.h>

#include <sys/snet/ixe_control.h>

#include <xnetdb.h> 
#include <sys/snet/wan_control.h> 
#include <sys/snet/dl_control.h> 
#include <sys/snet/timer.h> 
#include <fcntl.h> 
#include <errno.h> 
#include <ctype.h> 
#include <streamio.h> 
#ifndef SUNOS5
#include <stdlib.h> 
#endif


#include "uconx.h"

#ifndef uchar
#define uchar unsigned char
#endif

/*
 * config file 
 */

#ifndef LX25FILE
#ifdef WINNT  /* #5 */
#define LX25FILE    "c:\\winnt\\system32\\uconx\\etc\\x25conf"
/* #else
#define LX25FILE    "/etc/x25conf"  */
#endif /* WINNT */
#endif
#ifndef LMLPFILE
#define LMLPFILE    "/etc/mlpconf" 
#endif

static char LX25FILE[128];

#define MAXFIELD    30 	/* max. length of a field in board file  */ 
#define MAXENTRY    50 	/* max. no of valid entries              */ 
#define MAXDESCRIP  70 	/* max. no of chars in alias description */ 
#define X25TAG       1  /* defines VC in vc database             */ 
#define VCDATTRANS    data_transfer 
#define REAL_SNID    1  /* real subnetwork identifier specified  */ 
#define ALIAS_SNID   2  /* alias for subnetwork pecified        */ 

/*
 * used in reading a line of data
 */ 
#define BLANK_LINE      1 
#define LINE_HAS_DATA   2 
#define S_OK         	2 

/*
 * LAPB states (copied from lapb.h) 
 */
#define OFF        	0 
#define START        	1 
#define D_CONN        	2 
#define ADM        	3 
#define ARM        	4 
#define POLLING        	5 
#define PRESETUP    	6 
#define SETUP        	8 
#define LL_ERROR        10 
#define NORMAL        	11  

#ifndef TRUE
#define TRUE         	1
#endif

#ifndef FALSE
#define FALSE         	0
#endif

#ifndef MAXLINE
#define MAXLINE     120
#endif

#define MAX_CHAR     	40
#define MAX_GLOBAL_PROTOS 20

/*
 * Used in resetting stats
 */
#define GLOBAL_RESET         	1
#define PER_PROTOCOL_RESET    	2
#define PER_VC_RESET		3
#define PER_SNID_RESET        	4

/*
 * Used in getting wan stats
 */
#define H_STAT_REQ   (('H'<<4) | 6)  /* #6 */
#define H_CLEAR_STAT (('H'<<4) | 7)    
/*
 * protos used 
 */


static struct ixe_stats    statblk, *statptr;
static struct ixe_statinit rststatblk;
static struct xstrioctl     ixe_ioc, wan_ioc;

#define IXE    ((unsigned) 0x0020)
#define IXE_DEVICE    "/dev/ixe"


#define X25    ((unsigned) 0x0001) 
#define LAPB   ((unsigned) 0x0002) 
#define LLC2   ((unsigned) 0x0004) 
#define WAN    ((unsigned) 0x0010) 
#define ETHR   ((unsigned) 0x0040) 
#define MLP    ((unsigned) 0x0100) 

#define ALL_PROTOS (X25|LAPB|LLC2|WAN|ETHR|MLP|IXE)

#define USAGE1 "[-zawLN][-p proto][-s server][-v service][-n sub-network]\n[-c controller][-l lcn][ ...[delay interval]" 
/*
 * valid command line opts 
 */ 
#if 1 /* uconx */
#define VALIDOPTIONS    "s:v:p:c:n:l:LNazwf:" 
#else
#define VALIDOPTIONS    "s:SVv:aewzp:b:" 
#endif

#define streq(s1, s2)    (strcmp(s1, s2)==0) 

#define ETHRPFX        "/dev/eth"
#define WANPFX        "/dev/wans"

#define X25_DEVICE     "/dev/x25" 
#define LAPBPFX        "/dev/lapb" 
#define LLCPFX         "/dev/llc2" 
#define WLOOPPFX       "/dev/wloops" 
#define MLPPFX         "/dev/mlp"
#define ROOTRESET			/* #1				*/

static char    *lapb_states[]={ 
    "OFF",           
    "START",         
    "DISCONNECTED",  
    "ADM",          
    "ARM",           
    "POLLING",       
    "PRESETUP",      
    "REGISTERING",      
    "SETUP",         
    "RESET",         
    "ERROR",         
    "NORMAL"         
};

static char    *mlp_states[]={ 
    "",
    "ASSIGNED",       
    "WAIT",          
    "DISCONNECTED",  
    "NETCONNECT",    
    "CONNECTING",    
    "BLOCKED",       
    "FLOW",          
    "DATA",          
    "RESETTING",     
    "RESETTING",     
    "RESETTING",     
    "RESETTING",    
    "RESETTING"      
};

struct brdfile {
    char	device[MAXFIELD];        
    uint32    	real_subnet_ID;
    int        	proto;                  
    char        x25class[MAXFIELD];      
};

static struct brdfile  brdent[MAXENTRY]; 

typedef struct                  /*  #6  */
{
   bit32        xgood;          /* correctly transmitted frames */
   bit32        rgood;          /* correctly received frames */
   bit32        xunder;         /* transmit underrun count */
   bit32        rover;          /* receive overrun count */
   bit32        rlength;         /* receive frame length violation count */
#define rtoo    rlength         /* for older apps */
   bit32        rcrc;           /* receive CRC error */
   bit32        rabt;           /* receive aborts */
} LINK_STAT;

/*
 * holds valid (filtered) entries from board config file
 */ 
static int    entry_no=0; 

/*
 * glob variables 
 */
extern int	errno; 
extern int    	optind; 
extern char    	*optarg; 

extern OpenRequest oreq_x25;
extern OpenRequest oreq_lapb;
extern OpenRequest oreq_wan;
extern int	controllerNumber;
extern char	serverName [];
extern char	serviceName [];

#ifndef VXWORKS
time_t        	time();
#ifndef WINNT
unsigned    	sleep();
#else
#define  sleep(a)  Sleep((a)*1000)
#endif /* WINNT */
void        	exit();
void        	perror();
#endif /* !VXWORKS */

/* local prototypes */
static int get_valid_opts(int argc, char *argv[]);
static int specify_recognised_proto(void);
static int get_all_subnets(char *new_subnet);
static int add_if_not_used_already(char *new_subnet_id);
static int specify_subnetwk(char *argument);
static int single_letter_subnetwk(char *argument);
static int multi_letter_subnetwk(char *argument);
static int set_defaults_if_no_proto(void);
static int read_board_config_file(int *subnet_id, uint32 *snid);
static int read_garbage(char *line_of_chars);
static int read_col(char *line, char *variable, int col_no, int line_no);
static int is_line_based_proto(int subnet_id, uint32 snid, uint32 ascii_snid, char *file_stringsnid, char *file_prefix, char *file_class, int board_no);
static int mlp_with_LAPB(uint32 snid);
static int add_proto(int board_no, uint32 ascii_snid, char *file_class, char *device_prefix, int proto);
static int what_is_onboard(int *onboard);
static int correct_class(char *file_class, unsigned int proto);
static int access_proto_drivers(void);
static void get_per_snid_data(uint32 snid);
static int get_all_info(char *alias_snid, int *subnet_id, uint32 snid, struct subnetent **subnet_ident);
static int gthr_global_board_stats(unsigned int proto, char *proto_name, char *device_name, int fd);
static int gthr_global_stats(unsigned int proto, char *proto_name, char *device_name, int board_no, int fd);
static int get_VC_data(char *vc_snid, int *VCPackets_in, int *VCPackets_out, char *VCState, int *open_connections, int *circuits_established, int *rejected_connections, uint32 snid);
static int get_open_conns(void);
static int total_packets_in_or_out(int *in_packets, int *out_packets);
static int decode_net_state(int state, char *string);
static int disp_SNID_stats(uint32 snid, uint32 *data, int net_state);
static int disp_VC_stats(char *vc_snid, int VCPackets_out, int VCPackets_in, char *VCState);
static int gthr_X25_stats(int *circuits_established, int *rejected_connections);
static int disp_X25_glob_stats(int open_connections, int circuits_established, int rejected_connections);
static int disp_running_totals(int open_connections, int circuits_established, int rejected_connections);
static int disp_IXE_stats(void);
static int get_board_proto_stats(char *alias_snid, char *stringsnid, int *subnet_id, struct subnetent **subnet_ident, uint32 snid);
static int gthr_proto_stats(char *alias_snid, char *stringsnid, int *subnet_id, struct subnetent **subnet_ident, int vline, int fd, int current_proto);
static int header(int *show_header, char *proto_name);
static int disp_mlp_stats(int vline);
static int show(char *message, uint32 A, uint32 B);
static int extract_octet_data(void);
static int show_as_hex_char(unsigned char decimal);
static int get_subnet_description(char *alias_snid, char *stringsnid, int *subnet_id, struct subnetent **subnet_ident, int vline);
static int disp_lapb_stats(int vline);
static int disp_llc2_stats(void);
static void disp_wan_stats(void);
static void disp_ethr_stats(void);
static int gthr_stats(unsigned int gthr_stats_proto, char *proto_name, char *driver_name, int vline, int infd);
static int disp_mlp_glob_stats(void);
static int disp_lapb_glob_stats(void);
static int disp_llc2_glob_stats(void);
static int reset_various_stats(char *stringsnid, uint32 snid);
static int reset(int proto, char *proto_name, int infd, int vline, int which_reset, uint32 snid);
static void close_all_while_asleep(void);
static int newer(char *file, time_t age);
static char *basename(char *path);
static int numeric(char *arg);
static int usage(char *program, char *arguments);
static int error(char *message, char *arg);
static int unrecognised_proto(char *program, char *protos);
static int non_numeric_interval(char *argument);
static int alllower(char *s);
static int validsnid(uint32 sn_id);
static int get_VCState(char *VCState, int xstate);
static int FPRINT(char *s1, char *s2, int i1, int i2);
static int help_message(void);
#ifdef VXWORKS
static void terminate(void);
#endif

/*
 * Set to TRUE if the user selected 'x' opt 
 */
static int    opt_ethernet		=FALSE; 
static int    opt_wan                	=FALSE; 
static int    opt_specify_proto        =FALSE; 
static int    opt_reset            	=FALSE; 
static int    opt_specify_all_subnetwks =FALSE; 
static int    opt_specify_subnetwk     =FALSE; 
static int    opt_specify_board_no     =FALSE; 
static int    opt_specify_chnl         =FALSE; 
static int    opt_specify_all_chnls    =FALSE; 
static int    opt_none            	=FALSE;

static int    all_snets            	=FALSE;
static int    single_snet            	=FALSE;

static int    opt_reset_sub_stats      =FALSE; 
static int    opt_reset_glob_stats     =FALSE; 
static int    opt_reset_all            =FALSE; 
static int    opt_abbrev_vcs           =FALSE; 

static int    got_a_line_no            =FALSE; 
static int    selected_board_no        =(int)-1;
static int    current_board            =(int)-1;

static int    selected_chnl            =(int)-1; 

static int    selected_protos          = (int)NULL;

static int    no_done_devices          = 0;

static char    done_device[MAX_GLOBAL_PROTOS][MAX_CHAR];


/*
 * Set to true if the current subnet is the last one 
 * to have statistics disped on
 */
static int    is_it_last_subnet=FALSE;

static int    reset_stats=0; 

static int    read_mlp_file=FALSE;

static int    circuits_present=FALSE; /* No virtual circuits are known to exist yet */

static int    reset_a_statistic=FALSE;  

/*
 * protos to disp
 */ 
static unsigned    disp_protos=0;

static int interval    =(int)-1; 
static int x25     	=(int)-1;
static int lapb	=(int)-1;
static int ixe		=(int)-1;
static int wan         =(int)-1;

static char stats_per_VC=0; /* don't want statistics per VC */ 

static int line_based_proto=(MLP|LAPB|LLC2|WAN|ETHR); 

static int show_X25_header   =1; 
static int show_MLP_header   =1;
static int show_LAPB_header  =1; 
static int show_LLC_header   =1; 
static int show_WAN_header   =1; 
static int show_ETHR_header  =1; 

static unsigned which_level2_proto=0; 

static int wan_line_no       =0;        /* wan line number */

static char *program_name; 

/*
 * statistics structures. 
 */
static struct datal_stat    	eth_stats; 
static struct llc2_stioc    	llc2_s;    
static struct lapb_stioc    	lapb_s;   
static struct mlp_stioc	mlp_s;   

struct hdlc_stioc               /*  #6  */
{
    int         link;           /* Subnetwork ID character           */
    LINK_STAT   hdlc_stats;     /* Table of HDLC stats values        */
};

static struct hdlc_stioc    	wan_stats;      /*  #6  */

static int            		x25_stats[glob_mon_size];        
static struct lapb_gstioc    	lapb_g;  
static struct llc2_gstioc    	llc2_g; 
static struct mlp_gstioc    	mlp_g; 

static struct    vcstatusf    infoback; 
static struct    vcstatusf    *max_infoback;
static struct    vcinfo       *bufp; 

/*
 * streams ioctl structures. 
 */
static struct xstrioctl    mlp_ioc, x25_ioc, llc2_ioc, lapb_ioc, wan_ioc, eth_ioc, io; 

static char               *basename(); 

FILE               *fopen(); 
FILE               *popen(); 
unsigned short     getuid();      /* for protection */ 


#define MAX_SUBNETWORKS 20

static int current_subnet=0;

static int forever=1;

static char         subnet_list   [MAX_SUBNETWORKS][MAX_CHAR];
static char         g_alias_snid  [MAX_SUBNETWORKS][MAX_CHAR];
static int          g_subnet_id   [MAX_SUBNETWORKS];      
static uint32       g_snid    	   [MAX_SUBNETWORKS];       
static struct subnetent *g_subnet_ident[MAX_SUBNETWORKS]; 

/*
* #define TRACE 
*/

/*************************************************************************/
#ifdef VXWORKS
int x25stat (int argc, char *argv[], pti_TaskGlobals *ptg)
#else
int main(int argc, char *argv[]) 
#endif /* VXWORKS */
/*************************************************************************/

/*
    Loop forever, sleeping for "interval" seconds between cycles. 
    If (interval < 0), return after one cycle. 

    Note that all devices are closed while sleeping.
*/

{
    int k;

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

/*
 * Set to TRUE if the user selected 'x' opt
 */

    strcpy (LX25FILE, "/etc/x25conf");
    entry_no=0;
    opt_ethernet		=FALSE; 
    opt_wan                	=FALSE; 
    opt_specify_proto        	=FALSE; 
    opt_reset            	=FALSE; 
    opt_specify_all_subnetwks 	=FALSE; 
    opt_specify_subnetwk     	=FALSE; 
    opt_specify_board_no     	=FALSE; 
    opt_specify_chnl         	=FALSE; 
    opt_specify_all_chnls    	=FALSE; 
    opt_none            	=FALSE;

    all_snets            	=FALSE;
    single_snet            	=FALSE;

    opt_reset_sub_stats      	=FALSE; 
    opt_reset_glob_stats     	=FALSE; 
    opt_reset_all            	=FALSE; 
    opt_abbrev_vcs           	=FALSE; 

    got_a_line_no            	=FALSE; 
    selected_board_no        	=(int)-1;
    current_board            	=(int)-1;

    selected_chnl            	=(int)-1; 

    selected_protos          	= (int)NULL;
    no_done_devices          	= 0;
    is_it_last_subnet		=FALSE;

    reset_stats			=0; 

    read_mlp_file		=FALSE;

    circuits_present		=FALSE; /* No virtual circuits are known to exist yet */

    reset_a_statistic		=FALSE;  

/*
 * protos to disp
 */ 
    disp_protos=0;

    interval    =(int)-1; 
    x25     	=(int)-1;
    lapb	=(int)-1;
    ixe		=(int)-1;
    wan         =(int)-1;

    stats_per_VC=0; /* don't want statistics per VC */ 

    show_X25_header   =1; 
    show_MLP_header   =1;
    show_LAPB_header  =1; 
    show_LLC_header   =1; 
    show_WAN_header   =1; 
    show_ETHR_header  =1;

    which_level2_proto=0; 

    wan_line_no       =0;        /* wan line number */

#ifdef VXWORKS
    init_mps_thread (ptg);
    signal (SIGUSR1, (void*)&terminate);
#endif /* VXWORKS */

    get_valid_opts(argc, argv); 
    set_defaults_if_no_proto(); 

    while (forever) 
    { 
        access_proto_drivers();
        no_done_devices = 0;

        is_it_last_subnet =FALSE;
        read_mlp_file     =FALSE;
        circuits_present  =FALSE; 

        entry_no         =0;

        if (current_subnet==0)
	    ++current_subnet;

        for (k=0; k<current_subnet; ++k)
        {
	    if (k==current_subnet-1)
		is_it_last_subnet=TRUE;
            read_board_config_file (&(g_subnet_id[k]), &(g_snid[k])); 

            if ( all_snets && (is_it_last_subnet == FALSE)
	    &&  !(opt_specify_all_chnls || opt_specify_chnl
	    ||    opt_abbrev_vcs) )
            {
                /* called with -S and not 
                 * (-V or -a 0r -v flags) all data returned 
                 * in one (last) call to get_all_info
                 */
                continue;
            }
            get_all_info(g_alias_snid[k], &(g_subnet_id[k]),
                    g_snid[k], &(g_subnet_ident[k])); 
            if (all_snets
	    && (opt_specify_all_chnls || opt_specify_chnl || opt_abbrev_vcs))
                break;
            /* called with -S -V flags. all data returned     */
            /* in a single call to get_all_info.        */
        }

        close_all_while_asleep(); 
        if (interval >= 0)
            sleep((unsigned)interval); 
        else
            S_EXIT(0);
        printf("\n"); 

    } 
#ifdef VXWORKS
    exit_program (0);
#endif /* VXWORKS */
    return (0);
}


/*************************************************************************/
int get_valid_opts(int argc, char *argv[]) 
/*************************************************************************/

/*
    reads in command line opts. process arguments to 
    determine which statistics to gthr. optal parameter 
    is interval between statistics updates. default is to 
    disp statistics once only. 
*/

{
    int c; 
    char temp[40];

#ifdef TRACE
    printf ("get_valid_opts\n");
#endif

    strcpy (serverName, DEFAULT_SERVER);
    strcpy (serviceName, DEFAULT_SERVICE);

    program_name=basename(argv[0]); /* get program name */ 

    while ((c=getopt (argc, argv, VALIDOPTIONS)) !=EOF) 
    { 
        switch (c) 
        { 
        case 'w':
            opt_wan=TRUE;
            opt_specify_proto=TRUE;
            disp_protos |=WAN; 
            selected_protos |=WAN; 
            break; 

        case 'p': 
            opt_specify_proto=TRUE;
            specify_recognised_proto(); 
            break; 

        case 'z':
            opt_reset=TRUE;
            reset_stats=1; 
            break; 
 
        case 'c':			/* UconX Controller		*/
            sscanf(optarg, "%d", &controllerNumber);
            if (controllerNumber < 0)
            {
                fprintf(stderr,
                        "%s: Enter a positive integer for controller number.\n",
                        program_name );
                S_EXIT (1);
            }
            break;
  
        case 'f':			/* LX25FILE */
	    strcpy (LX25FILE, optarg);
            break;

        case 's':			/* UconX Server			*/
            strcpy(serverName, optarg);
            break;
 
        case 'v':			/* UconX Service		*/
            strcpy(serviceName, optarg);
            break;		

        case 'n':			/* Statistics on this subnet	*/
            opt_specify_subnetwk=TRUE;
            single_snet = TRUE;
            strcpy(temp,optarg);
            get_all_subnets(temp);
            add_if_not_used_already(temp);
            break; 

        case 'N': 			/* Display on per-subnetwork basis */
            all_snets = TRUE;
            opt_specify_subnetwk=TRUE;
            get_all_subnets(NULL);
            break;
#if 0
        case 'b':
            if (opt_specify_board_no)
            {
                fprintf(stderr,
			"%s: You may only specify one board number.\n",
			program_name ); 
                S_EXIT (1); 
            }

            opt_specify_board_no=TRUE;
            opt_specify_subnetwk=TRUE;

            selected_board_no=atoi(optarg);

            if (!numeric(optarg) || selected_board_no < 0)
            {
                fprintf(stderr,
			"%s: Enter a positive integer for board number.\n",
			program_name ); 
                S_EXIT (1); 
            }
            break;
#endif

        case 'l': 			/* Statistics on this virtual	*/
            if (opt_specify_chnl)	/* circuit (lcn).		*/
            {
                fprintf(stderr,
			"%s: You may only specify one channel.\n",
			program_name ); 
                S_EXIT (1); 
            }
            opt_specify_chnl=TRUE;
            opt_specify_proto=TRUE;
            disp_protos |= X25;
            sscanf(optarg, "%x", &selected_chnl);
            if (selected_chnl < 0)
            {
                fprintf(stderr,
			"%s: Enter a positive integer for channel number.\n",
			program_name ); 
                S_EXIT (1); 
            }
            break;

        case 'L':			/* Statistics on all active 	*/
            opt_specify_all_chnls=TRUE;	/* virtual circuits (lcns).	*/
            opt_specify_proto=TRUE;
            disp_protos |= X25;
            break;

        case 'a':
            opt_abbrev_vcs=TRUE;
            opt_specify_all_chnls=TRUE;
            opt_specify_proto=TRUE;
            disp_protos |= X25;
            break;

        case '?': 
             fprintf(stderr, "Usage: %s %s\n", program_name, USAGE1); 
            help_message();
            S_EXIT (1); 
        } 
    } 
    opt_none=(!opt_wan           
            && !opt_specify_proto
            && !opt_reset
            && !opt_specify_subnetwk
            && !opt_specify_board_no
            && !opt_specify_chnl
            && !opt_specify_all_chnls);            

    if (!opt_specify_proto)
        disp_protos=ALL_PROTOS;

    if (opt_specify_board_no && !opt_specify_subnetwk) 
    {
        get_all_subnets(NULL);
    }
    if (opt_reset && opt_specify_subnetwk)
    {
        opt_reset_sub_stats=TRUE;
    }
    else if (opt_reset)
    {
        opt_reset_sub_stats=TRUE;
        opt_reset_glob_stats=TRUE;
    }

    if (opt_reset && disp_protos==0 && !opt_specify_subnetwk)
    {
        opt_reset_all=TRUE;
        opt_specify_subnetwk=TRUE;
        get_all_subnets(NULL);
        disp_protos=ALL_PROTOS;
    }

    if (argc > optind + 1) 
    { 
        usage (program_name, "Incorrect number of arguments"); 
        S_EXIT (1); 
    } 

    if ( ! opt_specify_subnetwk && opt_specify_chnl)
        {
                fprintf(stderr,
		    "%s: Subnetwork must be specified with logical channel\n",
			program_name );
                S_EXIT (1);
        }

    if (argc==optind + 1) 
    { 
                char * delay_ptr;
        delay_ptr=argv[optind]; 
        if (!numeric(delay_ptr)) 
        { 
            int err=non_numeric_interval(delay_ptr); 
            fprintf(stderr, "Usage: %s %s\n", program_name, USAGE1); 
            help_message();
            S_EXIT (err); 
        } 
        interval=atoi(delay_ptr); 
    } 

#ifndef ROOTRESET 
    if (reset_stats) 
    { 
        /*
             you must be root to zero statistics 
            check REAL UID is 0 
        */ 
        if (getuid() !=0) 
        { 
            fprintf(stderr,
			"%s: must be the super user to reset statistics.\n",
			program_name); 
            S_EXIT(1); 
        } 
    } 

#endif 

    return((int) 0);
}
    

/**********************************************************************/
int specify_recognised_proto(void) 
/**********************************************************************/

/*
    reads in the specified protocol and checks if it 
    is recognised one. 
*/

{

#ifdef TRACE
    printf ("specify_recognised_proto\n");
#endif


    alllower (optarg); 
    if (streq(optarg, "x25"))              disp_protos |=X25; 
    else if (streq(optarg, "lapb"))        disp_protos |=LAPB; 
    else if (streq(optarg, "wan"))         disp_protos |=WAN; /* #6 */
    else S_EXIT(unrecognised_proto(program_name, optarg)); 
    selected_protos |= disp_protos; 
    return((int) 0);
}



/**********************************************************************/
int get_all_subnets(char *new_subnet) 
/**********************************************************************/

/*
    Reads the /etc/x25conf file for subnet id's and
    uses them as "-s xxxx" opts for the program.
    A check is made if the subnet has already been specified, in
    which case it is ignored.
*/

{

    char    	class [MAX_CHAR], 
    		device [MAX_CHAR],  
    		string_subnet_id[MAX_CHAR];  
    int    got_one=FALSE;
    struct subnetent     *subnet_info;
    struct confsubnet    *x25conf_info;
    struct confinterface *interface_info;
    int                  confdone;
    uint32               ascii_snid;
    char                 interface_snid_string[MAX_CHAR];

#ifdef TRACE
    printf ("get_all_subnets\n");
#endif

    if (new_subnet!=NULL)
    { 
        if ((subnet_info = getsubnetbyname(new_subnet)) != NULL)
        {
            x25tosnid( subnet_info->xaddr.sn_id, (unsigned char*)new_subnet );
        }
    }

    confdone = FALSE;   /* don't close or rewind x25conf file */
    setconfent(LX25FILE, 0);

    while (!confdone)
    {
        x25conf_info = getconfsubent(LX25FILE);
        if (x25conf_info == NULL)
        {
            confdone = TRUE;
            break;
        }

        x25tosnid(x25conf_info->snid,
              (unsigned char *)string_subnet_id);

        if (new_subnet==NULL)
            add_if_not_used_already(string_subnet_id);
        else
            got_one=got_one | (strcmp(new_subnet, 
                            string_subnet_id)==0);

        ascii_snid=x25conf_info->snid;
        if ( x25conf_info->dev_type != (char *)NULL);
            strcpy((char *)device, x25conf_info->dev_type);
        while ((interface_info = getnextintbysnid(LX25FILE,
                            ascii_snid)) != NULL)
        {
            if ( x25conf_info->x25reg != (char *)NULL);
            strcpy((char *)class, x25conf_info->x25reg);
            x25tosnid(interface_info->snid,
                (unsigned char *)interface_snid_string);
            x25tosnid(interface_info->llsnid,
                (unsigned char *)string_subnet_id);

            if ( (  strcmp( device, "mlp" ) == 0 || 
            strcmp( device, "wans" ) == 0 ||
                strcmp( device, "wloop" ) == 0 ) && 
               (strcmp( string_subnet_id, "?") != 0 ) &&
                correct_class( class, MLP ) ) 
            {

            if (new_subnet==NULL)
                add_if_not_used_already(string_subnet_id);
            else
                got_one=got_one | (strcmp(new_subnet, 
                        string_subnet_id)==0);
            }
        }
        
    }


    setconfent(LX25FILE, 0);
    endconfent(LX25FILE); /* close x25configuration file */

    if (!got_one && new_subnet !=NULL)
    {
        fprintf (stderr, "%s: \"%s\": subnetwork identifier not found\n",
		program_name, new_subnet); 
        S_EXIT(1);
    }
    return((int) 0);
}


/**********************************************************************/
int add_if_not_used_already(char *new_subnet_id) 
/**********************************************************************/

/*
    Will check if the given subnet identifier exists 
    in the subnet list, and if not will add the subnet id to 
    the existing list and call specify_subnetwk().
*/

{

    int i;
    int already_exists=FALSE;

#ifdef TRACE
    printf ("add_if_not_used_already\n");
#endif


    if (strcmp(new_subnet_id, "?") !=0)
    {
        for(i=0; subnet_list[i][0] != (char)NULL && !already_exists &&
            i<MAX_SUBNETWORKS; ++i)
        {
            already_exists = (strcmp(subnet_list[i],
                        new_subnet_id)==0);
        }

        if (!already_exists)
        {
            strcpy(subnet_list[i],new_subnet_id);
            specify_subnetwk(new_subnet_id);
        }
    }
    return((int) 0);
}


/**********************************************************************/
int specify_subnetwk(char *argument) 
/**********************************************************************/

/*
    determines if a single or multi letter network has 
    been specified. 
*/

{

    if (strlen (argument) !=1) 
        multi_letter_subnetwk(argument); 
    else 
        single_letter_subnetwk(argument); 
    current_subnet++;
    return((int) 0);
}


/**********************************************************************/
int single_letter_subnetwk(char *argument) 
/**********************************************************************/

/*
    Set flags indicating that a single letter subnetwk
              e.g 'A' has been specified
*/

{
    if (g_subnet_id[current_subnet] !=ALIAS_SNID)
    {
        /* If we get here, then either the
           length of the argument=1, or
           an alias of length <=4 was 
           checked but not found, so a snid
           is assumed.
        */
        g_snid[current_subnet]=snidtox25((unsigned char*)argument);

        if (! validsnid (g_snid[current_subnet]))
        {
            fprintf(stderr, "%s: \"%s\": is an invalid subnetwork id.\n",
		    program_name, argument);
            S_EXIT (1);
        }

        /* 
        * Set up a temporary flag to show a subnetwork id was specified
        */
        g_subnet_id[current_subnet]=REAL_SNID;
    }
    return((int) 0);
}


/**********************************************************************/
int multi_letter_subnetwk(char *argument) 
/**********************************************************************/

{
    if (strlen (argument) !=1)
    {
        /*
             Argument can either be a subnet
             alias, or a multi-character snid.
             check for the alias first.
        */
        strncpy (g_alias_snid[current_subnet], argument, MAX_CHAR);

        if ((g_subnet_ident[current_subnet]=
            getsubnetbyname (g_alias_snid[current_subnet]))==NULL)
        {
            /*
                Couldn't find the alias, so
                check length of the argument.
                If this is > 4, then it can't
                be a snid, so exit.
            */
            if ((int)strlen(argument) > SN_ID_LEN)
            {
                fprintf(stderr, "%s: \"%s\" subnetwork alias must be less than %d chars.\n", program_name, argument, SN_ID_LEN+1);
                S_EXIT (1);
            }

            g_snid[current_subnet] =
                    snidtox25((unsigned char*)argument); 
            g_subnet_id[current_subnet]=REAL_SNID; 
        }
        else
        {
            /*
                  An alias was found.  
            */
            g_snid[current_subnet]=
                            (g_subnet_ident[current_subnet])->xaddr.sn_id;
            g_subnet_id[current_subnet]=ALIAS_SNID;
        }
    }
    return((int) 0);
}


/**********************************************************************/
int set_defaults_if_no_proto(void) 
/**********************************************************************/

/*
    checks if a protocol has been specified, if not then 
    sets default parameters. 
*/

{

#ifdef TRACE
    printf ("set_defaults_if_no_proto\n");
#endif

    /*
         if no protos specified, set the defaults. 
    */ 
    if (opt_none) disp_protos=ALL_PROTOS;

    if (disp_protos & X25) which_level2_proto |=X25;
    if (disp_protos & MLP) which_level2_proto |=MLP;
    if (disp_protos & LAPB) which_level2_proto |=LAPB;
    if (disp_protos & LLC2) which_level2_proto |=LLC2;
    if (disp_protos & WAN) which_level2_proto |=WAN;  /* #6 */

    return((int) 0);
}



/**********************************************************************/
int read_board_config_file(int *subnet_id, uint32 *snid) 
/**********************************************************************/

/*
    open the devices of all the appropriate streams modules 
    to start with. 
    first we must check if a subnetwork alias name was specified. 
    if this is so then we need to know which is the 
    associated protocol and real subnetwork id. 
*/

{

    int      		line_no=1; 
    int      		board_no; 
    char    		file_stringsnid [MAX_CHAR],
        		filereg         [MAX_CHAR],  
        		file_prefix     [MAX_CHAR], 
        		file_dev        [MAX_CHAR];

    uint32     		ascii_snid; 
    int    		stayopen;
    struct confsubnet 	*x25conf_info;
    struct confinterface *interface_info;

#ifdef TRACE
    printf ("read_board_config_file\n");
#endif

    board_no = -1; 			/* uconx 			*/


    if (*subnet_id || (disp_protos & line_based_proto)) 
    { 
        /*
            used to count lines in the file which aren't comments 
            lines or empty 
        */ 

        stayopen = 2; /* don't close or rewind x25conf file */
        setconfent(LX25FILE, stayopen);

        read_mlp_file=FALSE;

        while (((x25conf_info = getconfsubent(LX25FILE)) != NULL) &&
              (entry_no < MAXENTRY))
        {
            ascii_snid=x25conf_info->snid;
            strcpy((char *)file_prefix, x25conf_info->dev_type);
            strcpy((char *)filereg, x25conf_info->x25reg);
            x25tosnid(x25conf_info->snid,
                  (unsigned char *)file_stringsnid);

            if (((opt_specify_proto && disp_protos & X25) ||
                 !opt_specify_proto)
	    &&  (opt_specify_subnetwk && *snid==ascii_snid)
	    &&  !opt_specify_chnl && !opt_specify_all_chnls)
            {
            add_proto(board_no, ascii_snid, filereg,
                  X25_DEVICE, X25);
            disp_protos |=X25;
            }

            if (( (opt_specify_proto && (disp_protos & MLP)) ||
            !opt_specify_proto ) &&
            (opt_specify_subnetwk && *snid==ascii_snid) )
            {
            if ((strcmp(file_prefix, "wans")==0) &&
                correct_class(filereg,MLP))
            {
                add_proto(board_no, ascii_snid, filereg,
                      MLPPFX, MLP);
                disp_protos |=MLP;
            }
            }

            if (opt_specify_proto && (disp_protos & WAN))  /* #6 */
            {
            if (strcmp(file_prefix, "wans")==0)
            {
                add_proto(board_no, ascii_snid, filereg,
                      WANPFX, WAN);
                disp_protos |=WAN;
            }
            }

            /* Now get the interface info        */
            while ((interface_info = getnextintbysnid(LX25FILE,
                        x25conf_info->snid)) != NULL)
            {
            if (interface_info->dev_name != (char *)NULL)
                strcpy((char *)file_dev,
                    interface_info->dev_name);
            if (interface_info->lreg != (char *)NULL)
                strcpy((char *)filereg, interface_info->lreg);
            wan_line_no = interface_info->line_number;
            board_no = interface_info->board_number ;
            ascii_snid = interface_info->llsnid;

            line_no++; 


            /*
                 we now have a valid line 
            */ 
#ifdef     DEBUG 
printf("File_prefix is %s\n", file_prefix); 
#endif     
            if (is_line_based_proto(*subnet_id, *snid, ascii_snid, 
                file_stringsnid, file_prefix, filereg, 
                board_no)==BLANK_LINE) 
            {
                continue; 
            }

            /* We need to set to display lapb here if snid    */
            /* matches, since we're not calling         */
            /* mlp_with_lapb to do it.            */
            if (opt_specify_subnetwk && *snid==ascii_snid &&
                !opt_specify_proto)
                disp_protos |=LAPB;


            if ( (opt_specify_subnetwk && *snid==ascii_snid) || 
               ( !opt_specify_subnetwk && !opt_specify_proto )  ||
               ( opt_specify_proto && opt_reset && 
                 !opt_specify_subnetwk) )
            {

                if((strcmp(file_prefix, "wans")==0) &&
                    (disp_protos & MLP) && 
                    correct_class(filereg,MLP) ) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, MLPPFX, MLP);
                }

                if((strcmp(file_prefix, "wans")==0) &&  /* #6 */
                    (disp_protos & WAN)) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, WANPFX, WAN);
                }

                else if((strcmp(file_prefix, "wans")==0) &&
                    (disp_protos & LLC2) &&
                     correct_class(filereg,LLC2) ) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, LLCPFX, LLC2);
                }

                else if((strcmp(file_prefix, "wans")==0) &&
                    (disp_protos & LAPB) && 
                    correct_class(filereg,LAPB) ) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, LAPBPFX, LAPB);
                }

                else if((strcmp(file_prefix, "wloop")==0) &&
                    (disp_protos & MLP) && 
                    correct_class(filereg,MLP) ) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, MLPPFX, MLP);
                }

                else if((strcmp(file_prefix, "wloop")==0) &&
                    (disp_protos & LLC2) &&
                     correct_class(filereg,LLC2) ) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, LLCPFX, LLC2);
                }

                else if((strcmp(file_prefix, "wloop")==0) &&
                    (disp_protos & LAPB) && 
                    correct_class(filereg,LAPB) ) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, LAPBPFX, LAPB);
                }

                else if((strcmp(file_prefix, "ethr")==0) &&
                    (disp_protos & LLC2) &&
                     correct_class(filereg,LLC2) ) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, LLCPFX, LLC2);
                }


                else if((strcmp(file_prefix, "mlp")==0) &&
                    (disp_protos & MLP) &&
                     correct_class(filereg,MLP) ) 
                {
                    add_proto(board_no, ascii_snid,
                            filereg, MLPPFX, MLP);
                }

            }


            else if ((opt_specify_subnetwk && *snid==ascii_snid) ||
                !opt_specify_subnetwk)
            {
                if ((disp_protos & WAN || opt_reset_all ) &&
                    (strcmp(file_prefix, "wans")==0))
                {
                    add_proto(board_no, ascii_snid,
                            filereg, WANPFX, WAN);
                }
                if ((disp_protos & WAN || opt_reset_all) &&
                    (strcmp(file_prefix, "wloop")==0))
                {
                    add_proto(board_no, ascii_snid,
                            filereg, WLOOPPFX, WAN);
                }
                if ((disp_protos & ETHR || opt_reset_all) &&
                    (strcmp(file_prefix, "ethr")==0))
                {
                    add_proto(board_no, ascii_snid,
                            filereg, ETHRPFX, ETHR);
                }
            }
        }    /* end while get interface */
        }
        setconfent(LX25FILE, 0);
        endconfent(LX25FILE); /* close x25 config file */

        if ((*subnet_id)&&(entry_no == 0)&& 
         !opt_specify_all_subnetwks &&
         !opt_specify_chnl && !opt_specify_all_chnls)
        {
        fprintf(stderr, 
               "%s: Subnetwork identifier does not match statistics.\n",
            program_name);
        }
    } 
    return((int) 0);
}




/***********************************************************************/
int read_garbage(char *line_of_chars) 
/**********************************************************************/

/*
    examines a line of characters and determines if 
    any useful data lies in that line. if the line is 
    blank or it is a comment then the code BLANK_LINE 
    will be returned. 
*/

{
    char *    s; 

    /*
          remove any newlines left by fgets() 
        */ 
    if (*(s=line_of_chars + strlen(line_of_chars)-1)=='\n') 
        *s='\0'; 

    /*
         ignore completely blank lines 
    */ 
    if (strlen(line_of_chars)==0) 
        return(BLANK_LINE); 

    /*
         strip comments
    */ 
    if ((s=strchr(line_of_chars,'#')) !=NULL) 
    { 
        *s='\0'; 
        return(BLANK_LINE); 
    } 

    return(LINE_HAS_DATA); 
}

/***********************************************************************/
int read_col(char *line, char *variable, int col_no, int line_no)
/**********************************************************************/

/*
    Reads in the next piece of date terminated by space, 
    tab or newline from a pre-specified file. 
    if line is NULL then the procedure reads from the 
    last previously read character. 
*/

{
    char *     a;
 
    if (!(a=strtok(line, " \t\n"))) 
    { 
        fprintf(stderr, "%s: error in format of `%s' file - line %d - col %d\n", 
            program_name, LX25FILE, line_no, col_no); 
        S_EXIT(1); 
    } 
    strcpy(variable,a);
    return((int) 0);
}


/***********************************************************************/
int is_line_based_proto(int subnet_id, uint32 snid, 
            uint32 ascii_snid, char *file_stringsnid, 
            char *file_prefix, char *file_class, int board_no) 
/***********************************************************************/

/*
    checks to see if protocol is a line based one, if no 
    match is found then the line is ignored and BLANK_LINE 
    is returned, otherwise S_OK. 

    Also restricts protos to those with the correct board no
    if specified via '-b' opt.
*/

{

    if (opt_specify_board_no && !(selected_board_no==board_no))
        return(BLANK_LINE);

    /*
        If the subnet no given by the user matches any in
        the /etc/x25conf file then statistics for that line will be
        included
        this option is ignored if the user has already specified
        a protocol
    */

    if (((opt_specify_subnetwk && snid==ascii_snid) ||
          all_snets) && !opt_specify_proto )
    {
        if (strcmp (file_prefix, "wans")==0)  
        {
            disp_protos |=WAN;
        }
        if (strcmp (file_prefix, "mlp")==0) 
        {
            disp_protos |=MLP;
        }
        if (strcmp (file_prefix, "ethr")==0) 
        {
            disp_protos |=ETHR;
        }
        if (strcmp (file_prefix, "wloop")==0)
        {
            disp_protos |=WAN;
        }

        /*
             Get the underlying protocol for which statistics 
             will be required for, as the subnet id's match
        */
        if (strcmp (file_class,  "LC_LLC1")==0)    
        {
            disp_protos |=LLC2;
        }
        if (strcmp (file_class,  "LC_LLC2")==0) 
        {
            disp_protos |=LLC2;
        }
        if (strcmp (file_class,  "LC_LAPBDTE")==0)     
        {
            disp_protos |=LAPB;
        }
        if (strcmp (file_class,  "LC_LAPBDCE")==0)     
        {
            disp_protos |=LAPB;
        }
        if (strcmp (file_class,  "LC_LAPBXDTE")==0)    
        {
            disp_protos |=LAPB;
        }
        if (strcmp (file_class,  "LC_LAPBXDCE")==0)    
        {
            disp_protos |=LAPB;
        }
        if (strcmp (file_class,  "LC_LAPDTE")==0)     
        {
            disp_protos |=LAPB;
        }
        if (strcmp (file_class,  "LC_LAPDCE")==0)     
        {
            disp_protos |=LAPB;
        }
        if (strcmp (file_class,  "LC_MLP")==0)     
        {
            disp_protos |=MLP;
        }
    }

    /* 
        If mlp and LAPB are specified then bypass the subnet work
        check for now. It will be checked later in mlp_with_LAPB()
    */

    if ((disp_protos & WAN) && (strcmp(file_class,"LC_MLP")==0)) 
        return(S_OK);

    if ((disp_protos & LAPB) && (strcmp(file_class,"LC_MLP")==0)) 
        return(S_OK);

    if (opt_specify_subnetwk && (strcmp(file_class,"LC_MLP")==0)) 
        return(S_OK);

    /*
        if subnetwork identifiers don't match 
         ignore the line 
    */ 
    if (((opt_specify_subnetwk && 
        (((subnet_id && (snid !=ascii_snid)) && !all_snets))) ||
        *file_stringsnid=='?'))
        return(BLANK_LINE); 

    /*
        Check for strange line combinations, and ignore if
        not a valid one.
    */ 
    if (disp_protos & line_based_proto) 
    { 
        if (strcmp (file_prefix, "wans")==0) 
        { 
            if (!(disp_protos & MLP) &&
                !(disp_protos & LAPB) && 
                !(disp_protos & WAN)) 
                return(BLANK_LINE); 
        } 
        else if (strcmp (file_prefix, "mlp")==0) 
        { 
            if (!(disp_protos & MLP) &&
                !(disp_protos & LAPB) &&
                !(disp_protos & WAN)) 
                return(BLANK_LINE); 
        }
        else if (strcmp (file_prefix, "wloop")==0) 
        { 
            if (!(disp_protos & MLP) &&
                !(disp_protos & LAPB) &&
                !(disp_protos & WAN)) 
                return(BLANK_LINE); 
        }
        else if (strcmp (file_prefix, "ethr")==0) 
        { 
            if (!(disp_protos & LLC2) && 
                            !(disp_protos & ETHR)) 
                return(BLANK_LINE); 
        } 
        else 
        { 
            perror ("Invalid device prefix"); 
            S_EXIT (1); 
        } 
    } 

    if (strcmp(file_stringsnid,"?")==0)
        return(BLANK_LINE);

    return(S_OK); 
}


/***********************************************************************/
int mlp_with_LAPB(uint32 snid)
/***********************************************************************/

{

    FILE 	*fd;
    int        	line_no=0;
    char        line_of_chars[MAXLINE];

    uint32      mlp_ascii_snid;
    char        mlp_subnet_id      [MAX_CHAR];
    char        mlp_device_prefix  [MAX_CHAR];
    char        mlp_board_no       [MAX_CHAR];
    char        mlp_line_no        [MAX_CHAR];
    char        mlp_link_layer_device_prefix    [MAX_CHAR];
    char        mlp_link_layer_subnet_id    [MAX_CHAR];
    char        mlp_link_class     [MAX_CHAR];
    char        mlp_stack_file     [MAX_CHAR];
    char        mlp_priority       [MAX_CHAR];
    int         bn;

    fd=fopen(LMLPFILE, "r");
    if (fd==NULL) 
    {
        fprintf(stderr,"%s: Unable to obtain/read %s\n", program_name, LMLPFILE); 
        S_EXIT(1); 
    }


    while ((fgets(line_of_chars, MAXLINE, fd) !=NULL) && 
        (entry_no < MAXENTRY) && !read_mlp_file) 
    {
        /*
            Set so as the file is read only once per parsing
            of the x25conf file per subnet reference.
        */

        if(read_garbage(line_of_chars)==BLANK_LINE) 
            continue;

        read_col(line_of_chars, mlp_subnet_id,      1,    line_no);
        read_col((char *)NULL,  mlp_device_prefix,  2,    line_no);
        read_col((char *)NULL,  mlp_board_no,       3,    line_no);
        read_col((char *)NULL,  mlp_line_no,        4,    line_no);
        read_col((char *)NULL,  mlp_link_layer_device_prefix, 5, line_no);
        read_col((char *)NULL,  mlp_link_layer_subnet_id,    6,
                                line_no);
        read_col((char *)NULL,  mlp_link_class,     7,    line_no);
        read_col((char *)NULL,  mlp_stack_file,     8,    line_no);
        read_col((char *)NULL,  mlp_priority,       9,    line_no);  

        mlp_ascii_snid =
            snidtox25((unsigned char*)mlp_link_layer_subnet_id); 

        if (!numeric(mlp_board_no) && strcmp(mlp_board_no,"?")!=0)
        {
            fprintf(stderr,"%s: Bad board number in %s\n", program_name, LMLPFILE);
            S_EXIT(1);
        }

        if ((opt_specify_board_no && 
            selected_board_no!=atoi(mlp_board_no))
	||  strcmp(mlp_board_no,"?")==0 )
        {
            continue;
        }

        if (opt_specify_proto && 
            !( disp_protos & LAPB || disp_protos & WAN )
             && !opt_reset )
        {
            continue;
        }

        if ((snid != mlp_ascii_snid && opt_specify_subnetwk)
	||  (( !opt_specify_subnetwk && !opt_reset ) &&
            !( opt_specify_proto && disp_protos & WAN )) ) 
        {
            continue;
        }
        disp_protos |= LAPB;

                bn=atoi(mlp_board_no);
        if (bn < 0 || bn > 99)
        {
            fprintf(stderr,"%s: Bad board number in \"%s\".\n", program_name, LMLPFILE);
            S_EXIT(1);
        }
    
        if ( ( strcmp( mlp_device_prefix,"wans" )==0 && 
            disp_protos & WAN ) ||
            ( strcmp( mlp_device_prefix,"wans" )==0 &&
            opt_specify_subnetwk && mlp_ascii_snid == snid ) )
        {
            add_proto(bn, mlp_ascii_snid, mlp_link_class, WANPFX,
                    WAN);
            disp_protos |= WAN;
            line_no++; 
        }

        if ( ( strcmp( mlp_device_prefix,"wloop" )==0 && 
            disp_protos & WAN ) ||
            ( strcmp( mlp_device_prefix,"wloop" )==0 &&
            opt_specify_subnetwk && mlp_ascii_snid == snid ) )
        {
            add_proto(bn, mlp_ascii_snid, mlp_link_class, WLOOPPFX,
                    WAN);
            disp_protos |= WAN;
            line_no++; 
        }

        add_proto(bn, mlp_ascii_snid, mlp_link_class, LAPBPFX, LAPB);
        line_no++; 
    }

    if (fclose(fd)==-1)
    {
        fprintf(stderr, "%s: Cannot close mlp configuration file.\n", program_name);
    }
    read_mlp_file=TRUE;
    return((int) 0);
}


/***********************************************************************/
int add_proto(int board_no, uint32 ascii_snid, char *file_class, 
          char *device_prefix, int proto)
/***********************************************************************/

{
        int     on_board=0;

#ifdef TRACE
    printf ("add_proto\n");
#endif


    if ( ( selected_protos != (int)NULL && selected_protos & proto ) ||
        selected_protos == (int)NULL )
    {
        what_is_onboard(&on_board);
        brdent[entry_no] . real_subnet_ID=ascii_snid; 
        
        if (on_board & proto)
            sprintf(brdent[entry_no].device, "%s%d", device_prefix, board_no);
        else
        {
            strcpy(brdent[entry_no].device,  device_prefix);
        }

        brdent[entry_no] . proto=proto; 
        strcpy(brdent[entry_no].x25class, file_class); 
        entry_no++; 
    }
    return((int) 0);
}



/***********************************************************************/
int what_is_onboard(int *onboard)
/***********************************************************************/

{
    *onboard=0;

/*
    Currently these are not used, but are left in
    for the future
*/

    return((int) 0);
}



/***********************************************************************/
int correct_class(char *file_class, unsigned int proto)
/***********************************************************************/

/*
    Checks if a class is one for a given protocol
*/

{
    if (proto==MLP && 
        strcmp (file_class,  "LC_MLP") !=0) 
            return(FALSE);
    if (proto==LLC2 && 
        (strcmp (file_class,  "LC_LLC1") !=0) &&
        (strcmp (file_class,  "LC_LLC2") !=0))
            return(FALSE);
    if (proto==LAPB &&
        (strcmp (file_class,  "LC_MLAPBDTE") !=0) &&     
        (strcmp (file_class,  "LC_MLAPBDCE") !=0) &&
        (strcmp (file_class,  "LC_LAPBDTE") !=0) &&     
        (strcmp (file_class,  "LC_LAPBDCE") !=0)  &&
        (strcmp (file_class,  "LC_LAPBXDTE") !=0) &&
        (strcmp (file_class,  "LC_LAPBXDCE") !=0) &&
        (strcmp (file_class,  "LC_LAPDTE") !=0) &&
        (strcmp (file_class,  "LC_LAPDCE") !=0))
            return(FALSE);

    if (proto==IXE) return(FALSE); 
    if (proto==X25) return(FALSE); 
    if (proto==ETHR) return(FALSE); 
    if (proto==WAN) return(FALSE); 
    return(TRUE);
}



/***********************************************************************/
int access_proto_drivers(void) 
/***********************************************************************/

{

#ifdef TRACE
    printf ("access_proto_drivers\n");
#endif

#if 1
    memset ((void *) &oreq_x25, 0, sizeof (oreq_x25));
    strcpy (oreq_x25.serverName, serverName);
    strcpy (oreq_x25.serviceName, serviceName);
    oreq_x25.port       = 0;
    oreq_x25.ctlrNumber = controllerNumber; /* #2			*/
    strcpy (oreq_x25.protoName, "x25");
    oreq_x25.dev        = 0;   /* default x25 device to zero */
    oreq_x25.flags      = CLONEOPEN;
    oreq_x25.openTimeOut = 10;
 
    if (( x25 = MPSopen ( &oreq_x25 ) ) == -1 )
    {
    	printf("Open request to server '%s', service '%s' failed.\n",
    	    	    oreq_x25.serverName, oreq_x25.serviceName);
    	MPSperror("MPSopen");
    	exit_program ( -1 );
    }
    /* #2 */
    if ( ntohl(0x1) != 0x1 )
    {
#ifdef TRACE
        printf("Little Endian -- pushing x25swap module.\n" );
#endif
        if ( MPSioctl (x25, X_PUSH, (uchar *) "x25swap" ) == -1)
        {
            MPSperror ( "MPSioctl swap X_PUSH" );
            exit_program ( -1 );
        }
    }

    /*
    * Initialize the open request structure for lapb, but don't open it
    * now.
    */
    memset ((void *) &oreq_lapb, 0, sizeof (oreq_lapb));
    strcpy (oreq_lapb.serverName, serverName);
    strcpy (oreq_lapb.serviceName, serviceName);
    oreq_lapb.port       = 0;
    oreq_lapb.ctlrNumber = controllerNumber; /* #2			*/
    strcpy (oreq_lapb.protoName, "lapb");
    oreq_lapb.dev        = 0;   /* Default device to zero */
    oreq_lapb.flags      = CLONEOPEN;
    oreq_lapb.openTimeOut = 10;

    /*
    * Initialize the open request structure for WAN, but don't open it
    * now.
    */
    memset ((void *) &oreq_wan, 0, sizeof (oreq_wan));
    strcpy (oreq_wan.serverName, serverName);
    strcpy (oreq_wan.serviceName, serviceName);
    oreq_wan.port       = 0;
    oreq_wan.ctlrNumber = controllerNumber; /* #2			*/
    strcpy (oreq_wan.protoName, "wan");
    oreq_wan.dev        = 0;   /* Default device to zero */
    oreq_wan.flags      = CLONEOPEN;
    oreq_wan.openTimeOut = 10;

#else

    if ((x25=S_OPEN(X25_DEVICE, O_RDONLY))==-1) 
    {
        if (errno==ENOENT) 
            perror("Statistics for X25 are not available"); 
        else 
            perror("Failed to access X25 driver"); 
        printf("\n"); 
        disp_protos &=~X25; 
    }
#endif

    return((int) 0);
}

/***********************************************************************/
void get_per_snid_data(uint32 snid) 
/***********************************************************************/

{
    struct persnidstats	pss;
    uint32        	used_snid;
    int            	vline; 
    int            	matching_ids=FALSE; 
    int            	packet_l_proto=FALSE; 

#ifdef TRACE
    printf ("get_per_snid_data\n");
#endif

    /* 
        only show statistics for X25
        proto if the user asked for them, or 
        requested ALL
    */ 
    for (vline=0; vline < entry_no; vline++) 
    { 
        used_snid = brdent[vline].real_subnet_ID;
        packet_l_proto = (brdent[vline].proto == X25); 
        matching_ids = (used_snid == snid || all_snets);

        if ( (opt_specify_subnetwk && !matching_ids) || !packet_l_proto)
         {
            continue;
        }


        pss.snid = used_snid;
        io.ic_cmd=N_getSNIDstats; 
        io.ic_timout=0; 
        io.ic_len=sizeof(pss);
        io.ic_dp=(char *)&pss; 

        if (S_IOCTL(x25,I_STR,(uchar *) &io)<0) 
        { 
            perror("x25stat:getSNIDstats IOCTL failed\n");
        } 
        else
            disp_SNID_stats(used_snid, pss.mon_array, pss.network_state);
    }
}

/***********************************************************************/
int get_all_info(char *alias_snid, int *subnet_id, uint32 snid, 
         struct subnetent **subnet_ident) 
/***********************************************************************/

/*
    the protocol statistics are returned in the ioctl message, 
    along with indications of the no of connections 
    currently extant for TCP and UDP. the connection 
    info is sent up in separate protocol messages. 
    NB: it would be terribly nice if we could make this a 
    a single ioctl call, and that this ioctl got all 
    its info as of the same "instant". 
    instead, we'll just have to hope that all our ioctl`s 
    are serviced reasonably close together. 
*/

{

    char        vc_snid[5]; 
    char        VCState[16]; 
    int         VCPackets_in = 0,
            	VCPackets_out = 0; 
    int         open_connections = 0,
            	circuits_established = 0, 
            	rejected_connections = 0; 

    char	stringsnid[SN_PRINT_LEN];

#ifdef TRACE
    printf ("get_all_info\n");
#endif

    /*
     * Do we want PLP statistics on a per subnetwork basis?
     */

    if (opt_specify_subnetwk && (disp_protos & X25) &&
        !(opt_specify_chnl || opt_specify_all_chnls))
    {
        get_per_snid_data(snid);
    }
        if ((((disp_protos & X25 && opt_specify_proto)
              || (disp_protos==ALL_PROTOS))
          && (opt_specify_chnl || opt_specify_all_chnls || opt_abbrev_vcs))
	|| (disp_protos & X25 && opt_specify_proto && ! opt_specify_subnetwk)
	|| (opt_none && (disp_protos & X25)) )
    {
        get_VC_data(vc_snid, &VCPackets_in, &VCPackets_out, 
		VCState, 
            	&open_connections, 
            	&circuits_established,
            	&rejected_connections, snid); 
    }

    if ( !opt_none )
    {
        get_board_proto_stats(alias_snid,  stringsnid, subnet_id, 
        subnet_ident, snid); 
    }
    if (opt_none || opt_specify_proto || opt_reset_all
        || opt_specify_board_no || (opt_reset && opt_specify_proto ) )
    {

        if (ixe >=0 && (disp_protos & IXE)) 
            gthr_global_board_stats(IXE, "IXE", IXE_DEVICE, ixe); 
        if (x25 >=0 && (disp_protos & X25)) 
            gthr_global_board_stats(X25, "X25", X25_DEVICE, x25); 
        disp_X25_glob_stats(open_connections, circuits_established, 
            rejected_connections);
        /*
             report glob statistics (if any) for LAPB and LLC2 
        */ 
        if (which_level2_proto) 
        { 
            gthr_global_board_stats(MLP, "MLP", MLPPFX, -1); /* uconx */
            gthr_global_board_stats(LLC2, "LLC2", LLCPFX, -1); /* uconx */
            gthr_global_board_stats(LAPB, "LAPB", LAPBPFX, -1); /* uconx */
        }
    }

    if (is_it_last_subnet)
        reset_various_stats(stringsnid, snid); 
    return((int) 0);
}



/***********************************************************************/
int gthr_global_board_stats(unsigned int proto, char *proto_name, 
                char *device_name, int fd) 
/***********************************************************************/

{

    int    onboard;
    int    bn;

    char   subnet        [MAX_CHAR];
    char   board_number  [MAX_CHAR];
    char   class         [MAX_CHAR];
    struct confinterface *interface_info;
    struct confsubnet    *x25conf_info;
    int    stayopen;

#ifdef TRACE
    printf ("gthr_global_board_stats: proto %d name %s dev %s fd %d\n",
	    proto, proto_name, device_name, fd);
#endif

    what_is_onboard(&onboard);
    
    /*
        Current IXE and X25 are only represented as always
        in the kernel
    */

    if( !(onboard&X25) && proto == X25 && disp_protos & X25 )
    {
        gthr_global_stats(proto, proto_name, device_name, -1, fd); 
    }

#if 1 /* uconx */
    if( !(onboard&LAPB) && proto == LAPB && disp_protos & LAPB )
    {
        gthr_global_stats(proto, proto_name, device_name, -1, fd); 
    }
#endif

    if( !(onboard&IXE) && proto == IXE && disp_protos & IXE &&
        !opt_specify_board_no )
    {
        gthr_global_stats(proto, proto_name, device_name, -1, fd); 
    }

    stayopen = 2;       /* don't close or rewind x25conf file */
    setconfent(LX25FILE, stayopen);

    while (((x25conf_info = getconfsubent(LX25FILE)) != NULL) &&
            (entry_no < MAXENTRY))
    {
        while ((interface_info = getnextintbysnid(LX25FILE,
                        x25conf_info->snid)) != NULL)
        {
            bn = interface_info->board_number;
            sprintf((char *)&board_number[0], "%d", bn);

            if (!numeric(board_number) && strcmp(board_number,"?")!=0)
            {
                fprintf(stderr,"%s: Bad board number in \"%s\"\n",
			program_name, LX25FILE);
                S_EXIT(1);
            }

            if (bn < 0 || bn > 99)
            {
                fprintf(stderr,"%s: Bad board number in \"%s\"\n",
			program_name, LX25FILE);
                S_EXIT(1);
            }

            strcpy((char *)class, interface_info->lreg);

            if (correct_class(class, MLP) && proto == LAPB
	    &&  (strcmp( subnet, "?" ) != 0 ) )
            {
                if ((opt_specify_board_no && (bn!= selected_board_no))
		||  (strcmp(board_number,"?")==0))
                {
                continue;
                }

                if(correct_class(class, proto)) 
                {
                    if (onboard&proto)
                        gthr_global_stats(proto, proto_name, device_name,
                          bn, fd); 
                    else
                        gthr_global_stats(proto, proto_name, device_name,
                          -1, fd); 
                }
            } 
            else 
            {
                if (opt_specify_board_no && bn != selected_board_no)
                {
                    continue;
                }
                if(correct_class(class,proto))
                {
                    if (onboard&proto)
                        gthr_global_stats(proto, proto_name, device_name,
                          bn, fd); 
                    else
                        gthr_global_stats(proto, proto_name, device_name,
                          -1, fd); 
                }
                if(correct_class(x25conf_info->x25reg,proto))
                {
                    if (onboard&proto)
                        gthr_global_stats(proto, proto_name, 
                          device_name, bn, fd); 
                    else
                        gthr_global_stats(proto, proto_name, 
                          device_name, -1, fd); 
                }
            }
        }
    }

    setconfent(LX25FILE, 0);
    endconfent(LX25FILE);  /* close x25configuration file */

    return((int) 0);
}

/***********************************************************************/
int gthr_global_stats(unsigned int proto, char *proto_name, char *device_name,
              int board_no, int fd) 
/***********************************************************************/

{
    char    device[MAX_CHAR];
    int    ignore = FALSE;
    int    i;

#ifdef TRACE
    printf ("gthr_global_stats: proto %d, name %s  dev %s brd %d, fd %d\n",
	    proto, proto_name, device_name, board_no, fd);
#endif

    current_board=board_no;

    if (board_no!=-1)
        sprintf(device, "%s%d", device_name, board_no);
    else
        sprintf(device, "%s", device_name);

    for(i=0;i<no_done_devices;++i)
    {
        ignore |= (strcmp(device,done_device[i])==0);
    }
    if (!ignore)
    {
        gthr_stats(proto, proto_name, device, 0, fd); 
        strcpy(done_device[i],device);
        no_done_devices++;
    }
    current_board=(int)-1;

    return((int) 0);
}

/***********************************************************************/
int get_VC_data(char *vc_snid, int *VCPackets_in, int *VCPackets_out,
        char *VCState, int *open_connections,
        int *circuits_established, int *rejected_connections,
        uint32 snid)
/***********************************************************************/

/*
* Sets gobal stats to zero, gets number of open connections and
* builds an IOCTL to signal the kind of info required. Two types of
* structure are passed in the io.ic_dp; infoback for a single entry
* or max_infoback for stats on multiple VCs. Both are of type vcstatuf
* but max_infoback has room for MAX_VC_ENTS-1 entries of type vcinfo
* allocated continguously, accessed by a ptr walk beyond the 'end'
* of vcstausf. For multiple entries that cannot be returned in a
* single ioctl, first_ent is set by the kernel to the next db entry
* to be returned. If num_ent == MAX_VC_ENTS there is more data to
* obtain from the kernel.
* The various types of query are flagged by;
* num open conns: infoback->first_ent=0, num_ent=0, vcinfo=zeroes
* specific VC   : infoback->first_ent=0, num_ent=1, vcinfo=snid set
*                            and lci set
* all VCs, one snet: max_infoback->first_ent=0 initially, num_ent =1
*                            vcinfo=snid set
* specific VC, all snets: (strange query but supported)
*    max_infoback->first_ent=0 initially, num_ent=MAX_VC_ENTS
*                            vcinfo=lci set
* all VCs: max_infoback->first_ent=0 initly, num_ent=0, vcinfo=zeroes
*
* Each vcinfo struct contains an array of statistics for the relevant
* VC which is then displayed.
*
*************************************************************************/

{
    int         no = 0, i; 

#ifdef TRACE
    printf ("get_VC_data\n");
#endif

    *open_connections=0;
    *open_connections=get_open_conns(); 

    *VCPackets_in=0; 
    *VCPackets_out=0; 

    io.ic_cmd=N_getVCstatus;
    io.ic_timout=0;

    if ((disp_protos & X25 && opt_specify_proto) || disp_protos==ALL_PROTOS)
    {
	if (opt_specify_chnl || opt_specify_all_chnls || opt_abbrev_vcs)
      	{
    	    if (opt_specify_chnl && single_snet)
    	    {        
            /* Want VC stats for a specific VC on a specific subnet    */

                io.ic_len=sizeof(infoback); 
		io.ic_dp=(char *) &infoback; 
        	infoback.first_ent = 0; 
        	infoback.num_ent = 1; 
        	bufp = &(infoback.vc); 

        	memset (&infoback.vc, 0, sizeof (struct vcinfo));
        	infoback.vc.xu_ident = snid;
        	infoback.vc.lci = selected_chnl;

        	if (S_IOCTL(x25,I_STR, (uchar *) &io)<0) 
        	{ 
		    perror("x25stat:get_VC_data. IOCTL failed\n"); 
              	    S_EXIT(1); 
        	} 

            	got_a_line_no=TRUE;
            	circuits_present=TRUE;
            	x25tosnid((uint32)infoback.vc.xu_ident,
			(unsigned char*)vc_snid); 

            	if ((opt_specify_chnl
	    	||  opt_specify_all_chnls
	    	||  opt_abbrev_vcs) && infoback.num_ent != 0)
            	{
                    get_VCState(VCState, (int)(infoback.vc.xstate)); 

                    total_packets_in_or_out(VCPackets_in, VCPackets_out); 
                    disp_VC_stats(vc_snid, *VCPackets_out, *VCPackets_in,
                            VCState); 
                }

                gthr_X25_stats(circuits_established, rejected_connections); 
            }
    	    else
    	    {
        	/* Want stats for multiple VCs, so allocate contiguous    */
        	/* space for first_ent, num_ents, followed by MAX_VC_ENT*/
        	/* structures of size vcinfo each. The whole lot can be    */
        	/* passed as a single lump of memory although it would    */
        	/* appear that there is only one vcinfo entry, according*/
        	/* to the structure definition.                */

        	int space_for_MAX_VC_ENT_entries;

        	space_for_MAX_VC_ENT_entries = sizeof(struct vcstatusf) +
                    (MAX_VC_ENTS-1)*sizeof(struct vcinfo); 
        	io.ic_len = space_for_MAX_VC_ENT_entries;

        	if ((max_infoback = (struct vcstatusf *)malloc
            		((unsigned int)space_for_MAX_VC_ENT_entries))==NULL)
        	{
            	    perror("x25stat:get_VC_data. malloc failed\n"); 
            	    S_EXIT(1); 
         	}
        	io.ic_dp = (char *) max_infoback; 
        	max_infoback->first_ent = 0;

        	for (;;)
        	{
            	    bufp = &max_infoback->vc; 

            	    memset (&max_infoback->vc, 0,
                    	    MAX_VC_ENTS * sizeof (struct vcinfo));

            	    max_infoback->num_ent = MAX_VC_ENTS;
            	    /* for all VCs, all snets    */
            	    if (single_snet)
            	    {
                	max_infoback->vc.xu_ident = snid;
                	max_infoback->num_ent = 1;
            	    }
            	    if (opt_specify_chnl)
            	    {
                	max_infoback->vc.lci = selected_chnl;
                	max_infoback->num_ent = 0;
            	    }

            	    if (S_IOCTL(x25,I_STR, (uchar *) &io)<0) 
            	    { 
                	perror("x25stat:get_VC_data IOCTL failed\n"); 
                	S_EXIT(1); 
            	    } 

            	    if (no == 0 ||        /* first time round */
                        max_infoback->num_ent)    /* entries returned */
            	    header(&show_X25_header, "X25");

            	    no=max_infoback->num_ent; 
            	    circuits_present=(no>0);

            	    for (i=0; i<no; i++, bufp++) 
               	    {
                	x25tosnid((uint32)bufp->xu_ident, 
                    		(unsigned char*)vc_snid); 

                	get_VCState(VCState, (int)(bufp->xstate)); 

                	total_packets_in_or_out(VCPackets_in,
                            	VCPackets_out); 

                	if (opt_specify_all_chnls
			||  opt_specify_chnl
			||  opt_abbrev_vcs
			||  (opt_specify_subnetwk && (bufp->xu_ident == snid)))
                	{
                    	    got_a_line_no=TRUE;
                    	    circuits_present=TRUE;
                    	    disp_VC_stats(vc_snid, *VCPackets_out,
                        	    *VCPackets_in, VCState); 
                	}
                    }

            	    if (no < MAX_VC_ENTS) 
                	break; 
        	} 
        	if (max_infoback != NULL)
        	{
            	    free (max_infoback);
            	    max_infoback = NULL;
        	}
    	    }
	}
    	else
    	{
            io.ic_len=sizeof(infoback); 
            io.ic_dp=(char *) &infoback; 
            infoback.first_ent = 0; 
            infoback.num_ent = 0; 
            bufp = &(infoback.vc); 

            memset (&infoback.vc, 0, sizeof (struct vcinfo));
            if (S_IOCTL(x25,I_STR, (uchar *) &io)<0) 
            { 
                perror("x25stat:get_VC_data. IOCTL failed\n"); 
                S_EXIT(1); 
            } 
	}
    }
    gthr_X25_stats(circuits_established, rejected_connections); 
    return((int) 0);
}

/***********************************************************************/
int get_open_conns(void)
/***********************************************************************/

/* To get the number of open connections with X25TAG and in DataTransfer
* mode. Set the entire data block to zero. Returned value contained in
* first_ent (16 bits) as num_ent is only 8 bits.
* Still a little wasteful as an irrelevant vcinfo block has to be passed.
*/
{

#ifdef TRACE
    printf ("get_open_conns\n");
#endif

    io.ic_cmd=N_getVCstatus;
    io.ic_timout=0;
    io.ic_len=sizeof(infoback);
    io.ic_dp=(char *) &infoback;
    infoback.first_ent = 0, infoback.num_ent = 0;
    
    memset (&infoback.vc, 0, sizeof (struct vcinfo));
    if (S_IOCTL(x25,I_STR, (uchar *) &io)<0)
    {
        perror("x25stat:get_open_conns. IOCTL failed\n");
        S_EXIT(1);
    }
    if (infoback.num_ent != 0)
        {
                printf ("x25stat:get_open_conns. Unexpected value in num_ent on return: %i\n", infoback.num_ent);
                S_EXIT (1);
        }
    return(infoback.first_ent);
}


/***********************************************************************/
int total_packets_in_or_out(int *in_packets, int *out_packets) 
/***********************************************************************/

{
    *in_packets=
        bufp->perVC_stats[cll_in_v] + 
        bufp->perVC_stats[caa_in_v] + 
        bufp->perVC_stats[dt_in_v]  + 
        bufp->perVC_stats[ed_in_v]  + 
        bufp->perVC_stats[rnr_in_v] + 
        bufp->perVC_stats[rr_in_v]  + 
        bufp->perVC_stats[rst_in_v] + 
        bufp->perVC_stats[rsc_in_v] + 
        bufp->perVC_stats[clr_in_v] + 
        bufp->perVC_stats[clc_in_v]; 
    *out_packets=
        bufp->perVC_stats[cll_out_v] + 
        bufp->perVC_stats[caa_out_v] + 
        bufp->perVC_stats[dt_out_v]  + 
        bufp->perVC_stats[ed_out_v]  + 
        bufp->perVC_stats[rnr_out_v] + 
        bufp->perVC_stats[rr_out_v]  + 
        bufp->perVC_stats[rst_out_v] + 
        bufp->perVC_stats[rsc_out_v] + 
        bufp->perVC_stats[clr_out_v] + 
        bufp->perVC_stats[clc_out_v]; 
        return((int) 0);
}

/***********************************************************************/
int decode_net_state(int state, char *string)
/***********************************************************************/
{
    switch (state)
    {
        case L_connecting:
            strcpy (string, "Connecting to DXE");
            break;
        case WtgRES:
            strcpy (string, "Connected resolving DXE");
            break;
        case WtgDXE:
            strcpy (string, "Random wait started");
            break;
        case L_connected:
            strcpy (string, "Connected and resolved DXE");
            break;
        case L3restarting:
            strcpy (string, "DTE RESTART REQUEST");
            break;
        case L_disconnecting:
            strcpy (string, "Waiting link disc reply");
            break;
        case WtgRpending:
            strcpy (string, "Buffer to enter WtgRES");
            break;
        case L3rpending:
            strcpy (string, "Buffer to enter L3restarting");
            break;
        case L_dpending:
            strcpy (string, "Buffer to enter L_disconnect");
            break;
        case L_registering:
            strcpy (string, "Registration request");
            break;
        default:
            strcpy (string, "Unknown");
            break;
    }
    return((int) 0);
}

/***********************************************************************/
int disp_SNID_stats(uint32 snid, uint32 *data, int net_state) 
/***********************************************************************/

{
    char string[MAX_CHAR];
    char snidstr[SN_PRINT_LEN];

    (void)x25tosnid(snid, (unsigned char *)&snidstr[0]);

    if (! stats_per_VC && !opt_reset_all) 
    { 
        printf("\nSUBNETWORK STATISTICS FOR X25\n");
        printf("-----------------------------\n");
        printf("\nSubnetwork     : %s\n", snidstr); 

	decode_net_state(net_state, string);
    	printf("State          : %s\n", string);

        printf("\n-------------------------------------\n"); 
        printf("Packet type              TX        RX\n"); 
        printf("-------------------------------------\n\n"); 

        show("Call",        (int)data[cll_out_s],    (int)data[cll_in_s]);
        show("Call accept",    (int)data[caa_out_s],    (int)data[caa_in_s]);
        show("Data",        (int)data[dt_out_s],    (int)data[dt_in_s]);
        show("Restart",        (int)data[res_out_s],    (int)data[res_in_s]);
        show("Restart confirm", (int)data[rec_out_s],    (int)data[rec_in_s]);
        show("RNR",        (int)data[rnr_out_s],    (int)data[rnr_in_s]);
        show("RR",        (int)data[rr_out_s],    (int)data[rr_in_s]);
        show("Resets",        (int)data[rst_out_s],    (int)data[rst_in_s]);
        show("Reset confirms",    (int)data[rsc_out_s],    (int)data[rsc_in_s]);
        show("Clear",        (int)data[clr_out_s],    (int)data[clr_in_s]);
        show("Clear Confirms",    (int)data[clc_out_s],    (int)data[clc_in_s]);
        show("Diagnostic",    (int)data[dg_out_s],    (int)data[dg_in_s]);
        show("Interrupts",    (int)data[ed_out_s],    (int)data[ed_in_s]);
        show("Registration",    (int)data[reg_out_s],    (int)data[reg_in_s]);
        show("Reg confirm",    (int)data[reg_conf_out_s],
                           (int)data[reg_conf_in_s]);
        show("Packets(total)", (int)data[pkts_out_s],    (int)data[pkts_in_s]);
        show("Bytes(total)",    (int)data[octets_out_s],
                            (int)data[octets_in_s]);
        printf("-------------------------------------\n\n"); 
	printf("Running totals\n");
        printf("-------------------------------------\n\n"); 
	printf("Tot no of VCs established  %10u\n", (uint32)data[vcs_est_s]);
	printf("SVCS currently open        %10u\n", (uint32)data[svcs_s]);
	printf("Max SVCS ever open         %10u\n", (uint32)data[max_svcs_s]);
	printf("PVCS currently attached    %10u\n", (uint32)data[pvcs_s]);
	printf("Max PVCS ever attached     %10u\n\n", (uint32)data[max_pvcs_s]);
    }     
    return((int) 0);
}

/***********************************************************************/
int disp_VC_stats(char *vc_snid, int VCPackets_out, int VCPackets_in,
          char *VCState) 
/***********************************************************************/

{
    if (! stats_per_VC && !opt_reset_all && !opt_abbrev_vcs) 
    { 
        printf("\nSubnetwork     : %s\n", vc_snid); 
        printf("LCN            : %03x (hexadecimal)\n", bufp->lci); 
        printf("User ID        : %d\n", bufp->process_id); 
        printf("Call direction : "); 

        if (bufp->call_direction == DIRECTION_INCOMING)
            printf("inward\n"); 
        else  if (bufp->call_direction == DIRECTION_OUTGOING)
            printf("outward\n"); 
        else
            printf("PVC\n");

#ifdef DEBUG 
        /*
             prints whole local address 
        */ 
        x25tos(bufp->rem_addr, buf, 0); 
        printf("%s\n",buf); 
#endif 

        if (bufp->rem_addr.DTE_MAC.lsap_len !=0) 
            extract_octet_data(); 

        printf("VC state       : %d - %s\n\n", bufp->xstate, VCState); 
        printf("-------------------------------------\n"); 
        printf("Packet type              TX        RX\n"); 
        printf("-------------------------------------\n"); 

        show("Call", bufp->perVC_stats[cll_out_v],
            bufp->perVC_stats[cll_in_v]); 
        show("Call confirm", bufp->perVC_stats[caa_out_v],
            bufp->perVC_stats[caa_in_v]); 
        show("Data", bufp->perVC_stats[dt_out_v],
            bufp->perVC_stats[dt_in_v]); 
        show("Interrupt", bufp->perVC_stats[ed_out_v],
            bufp->perVC_stats[ed_in_v]); 
        show("RNR", bufp->perVC_stats[rnr_out_v],
            bufp->perVC_stats[rnr_in_v]); 
        show("RR", bufp->perVC_stats[rr_out_v],
            bufp->perVC_stats[rr_in_v]); 
        show("Reset", bufp->perVC_stats[rst_out_v],
            bufp->perVC_stats[rst_in_v]); 
        show("Reset confirm", bufp->perVC_stats[rsc_out_v],
            bufp->perVC_stats[rsc_in_v]); 
        show("Clear", bufp->perVC_stats[clr_out_v],
            bufp->perVC_stats[clr_in_v]); 
        show("Clear confirm", bufp->perVC_stats[clc_out_v],
            bufp->perVC_stats[clc_in_v]); 
        printf("-------------------------------------\n"); 
        show("Total", VCPackets_out, VCPackets_in); 
        printf("-------------------------------------\n"); 
    }     

    if (opt_abbrev_vcs)
    {
        /* Print LCN */
        printf("%03x\t", bufp->lci);

        /* Print VC Type */
        if (bufp->ampvc)
            printf("PVC       ");
        else
        {
            if (bufp->vctype == X_UTWARD)
                printf("SVC Out   ");
            else if (bufp->vctype == X_INWARD)
                printf("SVC In    ");
            else
                printf("SVC-2way  ");
        }

        /* Print VC State */
        printf("%s", VCState);
        if (strlen (VCState) < 6) printf ("\t");

        /* Print Subnet ID */
        printf("\t%s\t\t", vc_snid);

        /* Print Local Address */
        if (bufp->loc_addr.DTE_MAC.lsap_len !=0) 
        {
            unsigned char     temp, hex; 
            int         counter; 

            for (counter=0;
                counter < (int)bufp->loc_addr.DTE_MAC.lsap_len;
                counter++) 
            { 
                temp =
                bufp->loc_addr.DTE_MAC.lsap_add[counter++ >> 1]; 
                hex=((temp>>4)&15); 
                show_as_hex_char(hex); 

                if (counter >=
                    (int)bufp->loc_addr.DTE_MAC.lsap_len) 
                    break; 

                hex=(temp&15); 
                show_as_hex_char(hex); 
            } 

            while (counter++ < 15)
                printf(" ");

            printf("\t");
        }
        else
            printf("     -      \t");

        /* Print Remote Address */
        if (bufp->rem_addr.DTE_MAC.lsap_len !=0) 
        {
            unsigned char     temp, hex; 
            int         counter; 
        
            for (counter=0;
                counter < (int)bufp->rem_addr.DTE_MAC.lsap_len;
                counter++) 
            { 
                temp =
                bufp->rem_addr.DTE_MAC.lsap_add[counter++ >> 1]; 
                hex=((temp>>4)&15); 
                show_as_hex_char(hex); 
        
                if (counter >=
                    (int)bufp->rem_addr.DTE_MAC.lsap_len) 
                    break; 
        
                hex=(temp&15); 
                show_as_hex_char(hex); 
            } 

            while (counter++ < 15)
                printf(" ");

            printf("\t");
        }
        else
            printf("      -      ");

        printf("\n");
    }
    return((int) 0);
}

/***********************************************************************/
int gthr_X25_stats(int *circuits_established, int *rejected_connections) 
/***********************************************************************/

{

#ifdef TRACE
    printf ("gthr_X25_stats\n");
#endif

    if (x25 >=0 && (disp_protos & X25)) 
    { 
        gthr_global_board_stats(X25, "X25", X25_DEVICE, x25);

        *circuits_established=x25_stats[caa_in_g] +
             x25_stats[caa_out_g]; 

        *rejected_connections=x25_stats[rjc_buflow_g] + 
            x25_stats[rjc_coll_g] + 
            x25_stats[rjc_failNRS_g] +
            x25_stats[rjc_lstate_g] + 
            x25_stats[rjc_nochnl_g]; 
    }
    return((int) 0);
}



/***********************************************************************/
int disp_X25_glob_stats(int open_connections, int circuits_established, 
            int rejected_connections) 
/***********************************************************************/

{
    if (x25 >=0 && (disp_protos & X25) &&
        (!opt_specify_subnetwk || opt_specify_board_no ) &&
        !(opt_specify_chnl || opt_specify_all_chnls) && !opt_reset_all) 
    { 
        printf("\n\nGLOBAL STATISTICS FOR X25\n"); 
        printf("-------------------------\n\n"); 
        if (current_board > -1) 
        {
            printf("Board : %d\n\n", current_board); 
        }     
        printf("-------------------------------------\n"); 
        printf("Packet type              TX        RX\n"); 
        printf("-------------------------------------\n"); 
        show("Call", x25_stats[cll_out_g], x25_stats[cll_in_g]); 
        show("Call accept", x25_stats[caa_out_g], x25_stats[caa_in_g]); 
        show("Restart", x25_stats[res_out_g], x25_stats[res_in_g]); 
        show("Restart confirm", x25_stats[res_conf_out_g],
            x25_stats[res_conf_in_g]); 
        show("RNR", x25_stats[rnr_out_g], x25_stats[rnr_in_g]); 
        show("RR", x25_stats[rr_out_g], x25_stats[rr_in_g]); 
        show("Resets", x25_stats[rst_out_g], x25_stats[rst_in_g]); 
        show("Reset confirms", x25_stats[rsc_out_g],
            x25_stats[rsc_in_g]); 
        show("Diagnostic", x25_stats[dg_out_g], x25_stats[dg_in_g]); 
        show("Interrupts", x25_stats[ed_out_g], x25_stats[ed_in_g]); 
        show("Registration", x25_stats[reg_out_g],
            x25_stats[reg_in_g]); 
        show("Reg confirm", x25_stats[reg_conf_out_g],
            x25_stats[reg_conf_in_g]); 
        show("Packets(total)", x25_stats[dt_out_g],
            x25_stats[dt_in_g]); 
        show("Bytes(total)", x25_stats[bytes_out_g],
            x25_stats[bytes_in_g]); 
        disp_running_totals(open_connections, circuits_established, 
            rejected_connections); 
    }
    return((int) 0);
}



/***********************************************************************/
int disp_running_totals(int open_connections, int circuits_established, 
            int rejected_connections) 
/***********************************************************************/

{
    if (x25 >=0 && (disp_protos & X25) && !opt_reset_all) 
    { 
        printf("-------------------------------------\n"); 
        printf("Running totals\n"); 
        printf("-------------------------------------\n"); 

        /*
             circuits_established will increase/zero/be-the-same 
        */ 
        printf("Tot no of VCs established  %10u\n",
            (uint32)circuits_established); 

        /*
             rejected_connections will increase/zero/be-the-same 
        */ 
        printf("Connections refused        %10u\n",
            (uint32)rejected_connections); 

        /*
             open_connections can go up/DOWN/zero/be-the-same 
        */ 
        printf("Connections currently open %10u\n",
            (uint32)open_connections);

        /*
             max_opens will increase/zero/be-the-same 
        */ 
        printf("Max connections open       %10u\n",
            (uint32)x25_stats[max_opens_g]); 
    }
    return((int) 0);
}



/*****************************************************************/ 
int disp_IXE_stats(void) 
/*****************************************************************/ 

{
    if (ixe >=0 && (disp_protos & IXE) && !opt_reset_all) 
    { 
        statptr=(struct ixe_stats *) ixe_ioc.ic_dp;
        printf("\nIXE:\n\n");
        if (current_board > -1) 
        {
            printf("Board : %d\n\n", current_board); 
        }     
        printf("\t%8u  connections\n", statptr->ixcons_active);
        printf("\t%8u  datagrams in\n", statptr->dgs_in);
        printf("\t%8u  datagrams out\n", statptr->dgs_out);
        printf("\t%8u  NSDUs in\n", statptr->nsdus_in);
        printf("\t%8u  NSDUs out\n", statptr->nsdus_out);
    }
    return((int) 0);
}




/*****************************************************************/ 
int get_board_proto_stats(char *alias_snid, char *stringsnid, int *subnet_id,
              struct subnetent **subnet_ident, uint32 snid) 
/*****************************************************************/ 

/*
    reads the board entry array, and performs ioctl calls 
    for each protocol specified in a manner that is fast 
    enough for the results to be imagined as if they were 
    gathered simultaneously. 
*/

{
    OpenRequest	oreq;			/* uconx open structure		*/
    int     vline; 
    int     current_proto; 
    int     fd=(int)-1; 
    int     matching_ids=FALSE; 

#ifdef TRACE
    printf ("get_board_proto_stats\n");
#endif

    /* 
        only show statistics for this line based 
        proto if the user asked for them, or 
        requested the defaults 
    */ 
    for (vline=0; vline < entry_no; vline++) 
    { 
        current_proto = brdent[vline].proto; 
        matching_ids = (brdent[vline].real_subnet_ID == snid ||
                all_snets);

        if ( (opt_specify_subnetwk && !matching_ids)
	||  !(disp_protos & current_proto))
        {
            continue;
        }
	/*
	* uconx Develop open request structure.
	*/

#if 1 /* uconx */
        switch (current_proto)
	{
	    case X25:
            memset ((void *) &oreq, 0, sizeof (oreq));
            strcpy (oreq.serverName, serverName);
            strcpy (oreq.serviceName, serviceName);
            oreq.port       = 0;
            oreq.ctlrNumber = controllerNumber; /* #2			*/
            strcpy (oreq.protoName, "x25");
            oreq.dev        = 0;   	/* default x25 device to zero */
            oreq.flags      = CLONEOPEN;
            oreq.openTimeOut = 10;
 
            if ( ( fd = MPSopen (&oreq)) == -1 )
            {
                printf("Open request to server '%s', service '%s' failed.\n",
                    oreq.serverName, oreq.serviceName);
                MPSperror("MPSopen");
                exit_program ( -1 );
            }
            /* #2 */
            if ( ntohl(0x1) != 0x1 )
            {
#ifdef TRACE
                printf("Little Endian -- pushing x25swap module.\n" );
#endif
                if ( MPSioctl (fd, X_PUSH, (uchar *) "x25swap" ) == -1)
                {
                    MPSperror ( "MPSioctl swap X_PUSH" );
                    exit_program ( -1 );
                }
            }
	    break;

	    case LAPB:
            memset ((void *) &oreq, 0, sizeof (oreq));
            strcpy (oreq.serverName, serverName);
            strcpy (oreq.serviceName, serviceName);
            oreq.port       = 0;
            oreq.ctlrNumber = controllerNumber; /* #2			*/
            strcpy (oreq.protoName, "lapb");
            oreq.dev        = 0;        /* default lapb device to zero */
            oreq.flags      = CLONEOPEN;
            oreq.openTimeOut = 10;

            if ((fd = MPSopen (&oreq)) == -1)
            {
                printf("Open request to server '%s', service '%s' failed.\n",
                    oreq.serverName, oreq.serviceName);
                MPSperror("MPSopen");
                exit_program ( -1 );
            }
    	    /* #2 */
    	    if ( ntohl(0x1) != 0x1 )
    	    {
#ifdef TRACE
       	        printf("Little Endian -- pushing llswap module.\n" );
#endif
       	        if ( MPSioctl (fd, X_PUSH, (uchar *) "llswap" ) == -1)
       	        {
          	    MPSperror ( "MPSioctl swap X_PUSH" );
          	    exit_program ( -1 );
       	        }
    	    }

	    case WAN:                                 /* #6 */
            memset ((void *) &oreq, 0, sizeof (oreq));
            strcpy (oreq.serverName, serverName);
            strcpy (oreq.serviceName, serviceName);
            oreq.port       = 0;
            oreq.ctlrNumber = controllerNumber; /* #2			*/
            strcpy (oreq.protoName, "wan");
            oreq.dev        = 0;        /* default wan device to zero */
            oreq.flags      = CLONEOPEN;
            oreq.openTimeOut = 10;

            if ((fd = MPSopen (&oreq)) == -1)
            {
                printf("Open request to server '%s', service '%s' failed.\n",
                    oreq.serverName, oreq.serviceName);
                MPSperror("MPSopen");
                exit_program ( -1 );
            }
    	    /* #2 */
    	    if ( ntohl(0x1) != 0x1 )
    	    {
#ifdef TRACE
       	        printf("Little Endian -- pushing wanswap module.\n" );
#endif
       	        if ( MPSioctl (fd, X_PUSH, (uchar *) "wanswap" ) == -1)
       	        {
          	    MPSperror ( "MPSioctl swap X_PUSH" );
          	    exit_program ( -1 );
       	        }
    	    }
	    break;

	    default:
	    printf ("get_board_proto_stats: invalid protocol request\n");
            exit_program (-1);
	}
#else
        if ((fd=S_OPEN(brdent[vline].device, O_RDONLY))==-1) 
        { 
            if (errno==ENOENT) 
                fprintf (stderr,
                    "Statistics are not available on : %s",
                    brdent[vline].device); 
            else 
                fprintf (stderr, "Failed to access driver : %s",
                    brdent[vline].device); 
            printf("\n"); 
            continue; 
        } 
#endif

        x25tosnid(brdent[vline].real_subnet_ID, 
            (unsigned char*)stringsnid); 
        /*
             register the current protocol for glob statistics 
        */ 
        which_level2_proto |=current_proto; 

        llc2_ioc.ic_cmd=L_GETSTATS; 
        lapb_ioc.ic_cmd=L_GETSTATS; 
        mlp_ioc.ic_cmd=L_GETSTATS; 
        gthr_proto_stats(alias_snid, stringsnid, subnet_id, 
            subnet_ident, vline, fd, current_proto); 

        if (S_CLOSE(fd)==-1)
        {
            fprintf(stderr, "Cannot close driver \n");
            S_EXIT (1); 
        }
    } 
    return((int) 0);
}



/*****************************************************************/ 
int gthr_proto_stats(char *alias_snid, char *stringsnid, int *subnet_id,
             struct subnetent **subnet_ident, int vline, int fd,
             int current_proto) 
/*****************************************************************/ 

{

#ifdef TRACE
    printf ("gthr_proto_stats\n");
#endif

    if (!opt_specify_board_no)
    {

        switch (current_proto)
        { 
        case MLP:  
            gthr_stats(MLP, "MLP", NULL, vline, fd); 
            header(&show_MLP_header, "MLP");
            get_subnet_description(alias_snid, stringsnid,
                    subnet_id, subnet_ident, vline); 
            disp_mlp_stats(vline);
            break; 

        case LAPB: 
            gthr_stats(LAPB, "LAPB", NULL, vline, fd); 
            header(&show_LAPB_header, "LAPB");
            get_subnet_description(alias_snid, stringsnid,
                    subnet_id, subnet_ident, vline); 
            disp_lapb_stats(vline);
            break; 

        case LLC2: 
            gthr_stats(LLC2, "LLC2", NULL, vline, fd); 
            header(&show_LLC_header, "LLC2");
            get_subnet_description(alias_snid, stringsnid,
                    subnet_id, subnet_ident, vline); 
            disp_llc2_stats();
            break; 

        case WAN: 
            gthr_stats(WAN, "WAN", NULL, vline, fd); 
            header(&show_WAN_header, "WAN");
            get_subnet_description(alias_snid, stringsnid,
                    subnet_id, subnet_ident, vline); 
            disp_wan_stats();
            break; 

        case ETHR:
            gthr_stats(ETHR, "ETHR", NULL, vline, fd); 
            header(&show_ETHR_header, "ETHERNET");
            get_subnet_description(alias_snid, stringsnid,
                    subnet_id, subnet_ident, vline); 
            disp_ethr_stats();
            break; 

        case X25:
            /* DO Nothing here, handled by disp_SNID_stats() */
            break;

        default: 
            fprintf(stderr, "Bad protocol (%d) assigned \n",
                    current_proto);
            S_EXIT(1); 
            break; 
	}
    } 
    return((int) 0);
}



/*****************************************************************/ 
int header(int *show_header, char *proto_name)
/*****************************************************************/ 

{
    int i;

        if( ( strcmp( proto_name, "X25" ) == 0 ) &&
        !(disp_protos & X25) )
        return((int) 0);

    if (! stats_per_VC && !opt_reset_all) 
    { 
        if (*show_header) 
        { 
            char show_proto = 0;

                        if (strcmp(proto_name,"WAN")==0) 
            {
                printf("\n\nSTATISTICS FOR %s\n", proto_name); 
                printf("---------------"); 
                show_proto = 1;
            }
            else if (strcmp(proto_name,"ETHERNET")==0) 
            {
                printf("\n\nSTATISTICS FOR %s\n", proto_name); 
                printf("---------------"); 
                show_proto = 1;
            }
            else
            {
                if ( ! (opt_specify_chnl ||
                                        opt_specify_all_chnls ||
                                        opt_abbrev_vcs))
                {
                    printf("\n\nPER-SUBNETWORK STATISTICS FOR %s\n", proto_name); 
                    printf("------------------------------"); 
                    show_proto = 1;
                }
            }

            if ( show_proto )
                for (i=0;i<(int)strlen(proto_name);++i)
                    printf("-");
            printf("\n");

            if (opt_abbrev_vcs)
            {
                printf("\nLCI(0x) Type      VC State     Subnetwork\tLocal Address\tRemote Address\n");
                printf("------- ----      --------     ----------\t-------------\t--------------\n");
            }

            if ((strcmp(proto_name, "X25")==0)
                   && ( disp_protos & X25 )
                   && !max_infoback->num_ent && !stats_per_VC) 
            {
                if (opt_specify_chnl)
                    printf("Selected VC not active\n");
                else
                    printf("No VCs active for X25\n"); 

                circuits_present=FALSE;
            }
        } 
    } 
    return((int) 0);
}



/*****************************************************************/ 
int disp_mlp_stats(int vline) 
/*****************************************************************/ 

{
    if (!opt_reset_all) 
    {
        printf("Link mode   : %-25s\n",brdent[vline].x25class); 
        printf("Link state  : %s\n\n", mlp_states[mlp_s.state]); 
        printf("                         TX          RX\n");
        printf("-----------------------------------------\n"); 
        printf(" Frames          %10u  %10u\n",   
            mlp_s.lli_stats.mlpmonarray[MLP_frames_tx], 
            mlp_s.lli_stats.mlpmonarray[MLP_frames_rx]);
        printf(" Reset           %10u  %10u\n",   
            mlp_s.lli_stats.mlpmonarray[MLP_reset_tx], 
            mlp_s.lli_stats.mlpmonarray[MLP_reset_rx]);
        printf(" Confirmations   %10u  %10u\n\n",   
            mlp_s.lli_stats.mlpmonarray[MLP_confs_tx], 
            mlp_s.lli_stats.mlpmonarray[MLP_confs_rx]);
        printf(" No of SLPS's    %10u\n\n", 
            mlp_s.lli_stats.mlpmonarray[MLP_slps]);  
        printf(" MT1             %10u\n", 
            mlp_s.lli_stats.mlpmonarray[MLP_mt1_exp]);  
        printf(" MT2             %10u\n", 
            mlp_s.lli_stats.mlpmonarray[MLP_mt2_exp]);  
        printf(" MT3             %10u\n", 
            mlp_s.lli_stats.mlpmonarray[MLP_mt3_exp]);  
        printf(" Retry expired   %10u\n", 
            mlp_s.lli_stats.mlpmonarray[MLP_mn1_exp]);  
    }
    return((int) 0);
}


/*****************************************************************/ 
int show(char *message, uint32 A, uint32 B) 
/*****************************************************************/ 

/*
    displays a formatted line of data consisting of 
    <message> <amount A> <amount B> 
*/

{
      printf("%17s%10u%10u\n", message, A, B); 
      return((int) 0);
}


/*****************************************************************/ 
int extract_octet_data(void) 
/*****************************************************************/ 

/*
    extract and print the 2 hex digits per octet. 
*/

{

    unsigned char     temp, hex; 
    int         counter; 

    if (bufp->call_direction < X_UTWARD)
        printf("From DTE       : "); 
    else  
        printf("To DTE         : "); 

    for (counter=0; counter < (int)bufp->rem_addr.DTE_MAC.lsap_len;
        counter++) 
    { 
        temp=bufp->rem_addr.DTE_MAC.lsap_add[counter++ >> 1]; 
        hex=((temp>>4)&15); 
        show_as_hex_char(hex); 

        if (counter >=(int)bufp->rem_addr.DTE_MAC.lsap_len) 
            break; 

        hex=(temp&15); 
        show_as_hex_char(hex); 
    } 
    printf("\n"); 
    return((int) 0);
}



/*****************************************************************/ 
int show_as_hex_char(unsigned char decimal) 
/*****************************************************************/ 

/*
    displays a decimal input in range 0..15 as a hexadecimal 
    character. 
*/

{
    if (decimal <=9)
        printf("%c", (decimal + '0')); 
    else
        printf("%c", (decimal + 'A'-10)); 
    return((int) 0);
}



/*****************************************************************/ 
int get_subnet_description(char *alias_snid, char *stringsnid, int *subnet_id,
               struct subnetent **subnet_ident, int vline) 
/*****************************************************************/ 

{
    char description[MAXDESCRIP]; 

    *description='\0';
    if (!opt_reset_all) 
    {
        printf("\nSubnetwork  : "); 

        if (*subnet_id==ALIAS_SNID) 
            printf("%s\n", alias_snid); 
        else 
            printf("%s\n", stringsnid) ; 
    }

    if ((*subnet_ident=getsubnetbyid(brdent[vline].real_subnet_ID)) !=NULL) 
        if ((*subnet_ident)->descrip !=NULL) 
        { 
            strncpy (description, (char*)
                ((*subnet_ident)->descrip), MAXDESCRIP); 
            if (strlen ((char*)((*subnet_ident)->descrip)) >=
                (size_t)MAXDESCRIP) 
            {
                description[MAXDESCRIP - 1]='\0'; 
            }
        } 
    else
        *description='\0'; 

    if (*description !='\0' && !opt_reset_all)
         printf("Description : %s\n", description); 
    return((int) 0);
}





/*****************************************************************/ 
int disp_lapb_stats(int vline) 
/*****************************************************************/ 
{

    if (opt_reset_all) return((int) 0);

    printf("Link mode   : %-25s\n",brdent[vline].x25class); 
    printf("Link state  : %s\n\n", lapb_states[lapb_s.state]); 
    printf("----------------------------------------------------------\n"); 
    printf("  FRAMES      TX_CMD      TX_RSP      RX_CMD      RX_RSP\n"); 
    printf("----------------------------------------------------------\n"); 
    
    printf("Supervisory:\n"); 
    printf("  RR      %10u  %10u  %10u  %10u\n", 
        lapb_s.lli_stats.lapbmonarray[RR_tx_cmd], 
        lapb_s.lli_stats.lapbmonarray[RR_tx_rsp], 
        lapb_s.lli_stats.lapbmonarray[RR_rx_cmd], 
        lapb_s.lli_stats.lapbmonarray[RR_rx_rsp]); 
    printf("  RNR     %10u  %10u  %10u  %10u\n", 
        lapb_s.lli_stats.lapbmonarray[RNR_tx_cmd], 
        lapb_s.lli_stats.lapbmonarray[RNR_tx_rsp], 
        lapb_s.lli_stats.lapbmonarray[RNR_rx_cmd], 
        lapb_s.lli_stats.lapbmonarray[RNR_rx_rsp]); 
    printf("  REJ     %10u  %10u  %10u  %10u\n", 
        lapb_s.lli_stats.lapbmonarray[REJ_tx_cmd], 
        lapb_s.lli_stats.lapbmonarray[REJ_tx_rsp], 
        lapb_s.lli_stats.lapbmonarray[REJ_rx_cmd], 
        lapb_s.lli_stats.lapbmonarray[REJ_rx_rsp]); 

    printf("\nUnumbered:\n"); 

    if (streq(brdent[vline].x25class, "LC_LAPBDTE") || 
        streq(brdent[vline].x25class, "LC_LAPBDCE")) 
        printf("  SABM    %10u              %10u\n", 
            lapb_s.lli_stats.lapbmonarray[SABM_tx_cmd], 
             lapb_s.lli_stats.lapbmonarray[SABM_rx_cmd]); 


    if (streq(brdent[vline].x25class, "LC_LAPBXDTE") || 
        streq(brdent[vline].x25class, "LC_LAPBXDCE")) 
        printf("  SABME   %10u              %10u\n", 
            lapb_s.lli_stats.lapbmonarray[SABME_tx_cmd], 
            lapb_s.lli_stats.lapbmonarray[SABME_rx_cmd]); 


    printf("  DISC    %10u              %10u\n", 
        lapb_s.lli_stats.lapbmonarray[DISC_tx_cmd], 
        lapb_s.lli_stats.lapbmonarray[DISC_rx_cmd]); 


    if (streq(brdent[vline].x25class, "LC_LAPDTE") || 
        streq(brdent[vline].x25class, "LC_LAPDCE")) 
        printf("  SARM                %10u              %10u\n", 
            lapb_s.lli_stats.lapbmonarray[SARM_tx_cmd], 
            lapb_s.lli_stats.lapbmonarray[SARM_rx_cmd]); 
    else    /* no DM packets in LAP */ 
        printf("  DM                  %10u              %10u\n", 
            lapb_s.lli_stats.lapbmonarray[DM_tx_rsp], 
            lapb_s.lli_stats.lapbmonarray[DM_rx_rsp]); 


    printf("  UA                  %10u              %10u\n", 
        lapb_s.lli_stats.lapbmonarray[UA_tx_rsp], 
        lapb_s.lli_stats.lapbmonarray[UA_rx_rsp]); 


    if (streq(brdent[vline].x25class, "LC_LAPDTE") || 
        streq(brdent[vline].x25class, "LC_LAPDCE")) 
        printf("  CMDR                %10u              %10u\n", 
            lapb_s.lli_stats.lapbmonarray[FRMR_tx_rsp], 
            lapb_s.lli_stats.lapbmonarray[FRMR_rx_rsp]); 
    else    /* ie FRMR for LAPB, CMDR for LAP */ 
        printf("  FRMR                %10u              %10u\n", 
            lapb_s.lli_stats.lapbmonarray[FRMR_tx_rsp], 
            lapb_s.lli_stats.lapbmonarray[FRMR_rx_rsp]); 
    printf("\nInformation:\n"); 
    printf("  I       %10u              %10u\n", 
        lapb_s.lli_stats.lapbmonarray[I_tx_cmd], 
        lapb_s.lli_stats.lapbmonarray[I_rx_cmd]); 


    printf("-----------------------------------------------------------\n"); 
    printf("                  TX                              RX\n"); 
    printf("-----------------------------------------------------------\n"); 
    printf("Other:\n"); 
    printf(" Bad length                               %10u\n", 
        lapb_s.lli_stats.lapbmonarray[rx_badlen]); 
    printf(" Unknown                                  %10u\n", 
        lapb_s.lli_stats.lapbmonarray[rx_unknown]); 
    printf(" Erroneous                                %10u\n", 
        lapb_s.lli_stats.lapbmonarray[rx_bad]); 
    printf(" Discarded                                %10u\n", 
        lapb_s.lli_stats.lapbmonarray[rx_dud]); 
    printf(" Ignored    %8d                      %10u\n", 
        lapb_s.lli_stats.lapbmonarray[tx_ign], 
        lapb_s.lli_stats.lapbmonarray[rx_ign]); 
    printf(" Retransmitted %5u\n", 
        lapb_s.lli_stats.lapbmonarray[tx_rtr]); 
    printf("-----------------------------------------------------------\n"); 
    printf("Timers:\n"); 
    printf(" T1          %6d\n", 
        lapb_s.lli_stats.lapbmonarray[t1_exp]); 
    printf(" T4          %6d\n", 
        lapb_s.lli_stats.lapbmonarray[t4_exp]); 
    printf(" T4 (N2 times)%5d\n", 
        lapb_s.lli_stats.lapbmonarray[t4_n2_exp]); 
    return((int) 0);
}


/*****************************************************************/ 
int disp_llc2_stats(void) 
/*****************************************************************/ 


{
    if (opt_reset_all) return((int) 0);

    printf("\n------------------------------------------------------------------------\n"); 
    printf("FRAMES            TX_CMD         TX_RSP          RX_CMD          RX_RSP\n"); 
    printf("------------------------------------------------------------------------\n"); 
    printf("Supervisory:\n"); 
    printf("  RR      %10u      %10u      %10u      %10u\n", 
        llc2_s.lli_stats.llc2monarray[RR_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[RR_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[RR_rx_cmd], 
        llc2_s.lli_stats.llc2monarray[RR_rx_rsp]); 
    printf("  RNR     %10u      %10u      %10u      %10u\n", 
        llc2_s.lli_stats.llc2monarray[RNR_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[RNR_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[RNR_rx_cmd], 
        llc2_s.lli_stats.llc2monarray[RNR_rx_rsp]); 
    printf("  REJ     %10u      %10u      %10u      %10u\n", 
        llc2_s.lli_stats.llc2monarray[REJ_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[REJ_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[REJ_rx_cmd], 
        llc2_s.lli_stats.llc2monarray[REJ_rx_rsp]); 

    printf("\nUnnumbered:\n"); 
    printf("  SABME   %10u                      %10u\n", 
        llc2_s.lli_stats.llc2monarray[SABME_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[SABME_rx_cmd]); 
    printf("  DISC    %10u                      %10u\n", 
        llc2_s.lli_stats.llc2monarray[DISC_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[DISC_rx_cmd]); 
    printf("  UI      %10u                      %10u\n", 
        llc2_s.lli_stats.llc2monarray[UI_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[UI_rx_cmd]); 
    printf("  XID     %10u      %10u      %10u      %10u\n", 
        llc2_s.lli_stats.llc2monarray[XID_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[XID_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[XID_rx_cmd], 
        llc2_s.lli_stats.llc2monarray[XID_rx_rsp]); 
    printf("  TEST    %10u      %10u      %10u      %10u\n", 
        llc2_s.lli_stats.llc2monarray[TEST_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[TEST_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[TEST_rx_cmd], 
        llc2_s.lli_stats.llc2monarray[TEST_rx_rsp]); 
    printf("  UA                      %10u                      %10u\n", 
        llc2_s.lli_stats.llc2monarray[UA_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[UA_rx_rsp]); 
    printf("  DM                      %10u                      %10u\n", 
        llc2_s.lli_stats.llc2monarray[DM_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[DM_rx_rsp]); 
    printf("  FRMR                    %10u                      %10u\n", 
        llc2_s.lli_stats.llc2monarray[FRMR_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[FRMR_rx_rsp]); 

    printf("\nInformation:\n"); 
    printf("  I       %10u      %10u      %10u      %10u\n", 
        llc2_s.lli_stats.llc2monarray[I_tx_cmd], 
        llc2_s.lli_stats.llc2monarray[I_tx_rsp], 
        llc2_s.lli_stats.llc2monarray[I_rx_cmd], 
        llc2_s.lli_stats.llc2monarray[I_rx_rsp]); 


    printf("----------------------------------------------------------\n"); 
    printf("                  TX                              RX\n"); 
    printf("----------------------------------------------------------\n"); 
    printf("Other:\n"); 
    printf(" Bad length                               %10u\n",
        llc2_s.lli_stats.llc2monarray[rx_badlen]); 
    printf(" Unknown                                  %10u\n",
        llc2_s.lli_stats.llc2monarray[rx_unknown]); 
    printf(" Erroneous                                %10u\n", 
        llc2_s.lli_stats.llc2monarray[rx_bad]); 
    printf(" Discarded                                %10u\n", 
        llc2_s.lli_stats.llc2monarray[rx_dud]); 
    printf(" Ignored   %9d                      %10u\n", 
        llc2_s.lli_stats.llc2monarray[tx_ign], 
        llc2_s.lli_stats.llc2monarray[rx_ign]); 
    printf(" Retransmitted %5d\n", 
        llc2_s.lli_stats.llc2monarray[tx_rtr]); 
    printf("----------------------------------------------------------\n"); 
    printf("Timers:\n"); 
    printf(" T1        %9d\n", 
        llc2_s.lli_stats.llc2monarray[t1_exp]); 
    printf(" T4        %9d\n", 
        llc2_s.lli_stats.llc2monarray[t4_exp]); 
    printf(" T4 (after N2)%6d\n", 
        llc2_s.lli_stats.llc2monarray[t4_n2_exp]); 
    return((int) 0);
}



/*****************************************************************/ 
void disp_wan_stats(void) 
/*****************************************************************/ 

{
    if (opt_reset_all) return;
#if 0
    printf("Link State :");
    switch (wan_stats.w_state)
    {
        case HDLC_IDLE:
            printf("HDLC_IDLE\n");
            break;
        case HDLC_ESTB:
            printf("HDLC_ESTB\n");
            break;
        case HDLC_DISABLED:
            printf("HDLC_DISABLED\n");
            break;
        case HDLC_CONN:
            printf("HDLC_CONN\n");
            break;
        case HDLC_DISC:
            printf("HDLC_DISC\n");
            break;
        default:
            printf("Unknown %d  \n", wan_stats.w_state);
            break;
    }
#endif
        printf("         WAN:\n");
    printf("\t%8d  good frames transmitted\n",
        wan_stats.hdlc_stats.xgood);
    printf("\t%8d  good frames received\n",
        wan_stats.hdlc_stats.rgood);
    printf("\t%8d  transmit underruns\n", wan_stats.hdlc_stats.xunder);
    printf("\t%8d  receive overruns\n", wan_stats.hdlc_stats.rover);
    printf("\t%8d  received frames too long\n",
        wan_stats.hdlc_stats.rtoo);
    printf("\t%8d  CRC/frame errors received\n",
        wan_stats.hdlc_stats.rcrc);
    printf("\t%8d  received aborts\n",
        wan_stats.hdlc_stats.rabt);
    return;
}

/*****************************************************************/ 
void disp_ethr_stats(void) 
/*****************************************************************/ 
{
    if (opt_reset_all) return;
        printf("Ethernet    :\n");
        printf("\t%8ld Packets successfully transmitted\n", eth_stats.dl_tx);
        printf("\t%8ld Packets successfully received\n", eth_stats.dl_rx);
        printf("\t%8ld Packets lost\n", eth_stats.dl_lost);
        printf("\t%8ld Receive errors\n", eth_stats.dl_rxerr);
        printf("\t%8ld Transmission errors\n", eth_stats.dl_txerr);
        printf("\t%8ld Collisions detected\n", eth_stats.dl_coll);
    return;
}



/*****************************************************************/ 
int gthr_stats (unsigned int gthr_stats_proto,
		char *proto_name,
           	char *driver_name,
		int vline,
		int infd) 
/*****************************************************************/ 

/*
    Gathers statistics for either global or per protocol.
    Per protocol statistics do not require driver_name, but do need infd
    Only LLC2, LAPB and MLP are currently supported for global stats.
*/

{

    int     fd; 
    int      got_stats=FALSE;

#ifdef TRACE
    if (driver_name == NULL)
        printf ("gthr_stats: proto %d, name %s, drvr 0, vl %d fd %d\n", 
	    gthr_stats_proto, proto_name, vline, infd);
    else
        printf ("gthr_stats: proto %d, name %s drvr %s, vl %d fd %d\n", 
	    gthr_stats_proto, proto_name, driver_name, vline, infd);
#endif

    fd = -1;

    /*
        Collect per protocol statistics 
    */
#if 1
    if (infd != -1) 			/* uconx 			*/
#else
    if (infd != 0) 
#endif
    { 
        switch(gthr_stats_proto)
        {
        case IXE: 
            ixe_ioc.ic_cmd=IXE_STATS;
            ixe_ioc.ic_timout=0;
            ixe_ioc.ic_len=sizeof(struct ixe_stats);
            ixe_ioc.ic_dp=(char *)&statblk;
            got_stats=(S_IOCTL(infd, I_STR,  (uchar *) &ixe_ioc) >=0);
            disp_IXE_stats();
            break;

        case MLP:
            mlp_ioc.ic_cmd=M_GETSTATS; 
            mlp_ioc.ic_len=sizeof(mlp_s); 
            mlp_ioc.ic_dp =(char *) &mlp_s; 
            mlp_s.lli_snid=brdent[vline].real_subnet_ID; 
            mlp_s.lli_type=LI_STATS; 
            got_stats=(S_IOCTL(infd, I_STR,  (uchar *) &mlp_ioc) >=0);
            break;

        case X25:
            x25_ioc.ic_cmd=N_getstats; 
            x25_ioc.ic_timout=0; 
            x25_ioc.ic_len=sizeof(x25_stats); 
            x25_ioc.ic_dp=(char *) x25_stats; 
            got_stats=(S_IOCTL(infd, I_STR,  (uchar *) &x25_ioc) >=0);
            break;
 
        case LLC2:
            llc2_ioc.ic_cmd=L_GETSTATS; 
            llc2_ioc.ic_len=sizeof(llc2_s); 
            llc2_ioc.ic_dp=(char *) &llc2_s; 
            llc2_s.lli_snid=brdent[vline].real_subnet_ID; 
            llc2_s.lli_type=LI_STATS; 
            got_stats=(S_IOCTL(infd, I_STR,  (uchar *) &llc2_ioc) >=0);
            break;

        case LAPB:
            lapb_ioc.ic_cmd=L_GETSTATS; 
            lapb_ioc.ic_len=sizeof(lapb_s); 
            lapb_ioc.ic_dp =(char *) &lapb_s; 
            lapb_s.lli_snid=brdent[vline].real_subnet_ID; 
            lapb_s.lli_type=LI_STATS; 
            got_stats=(S_IOCTL(infd, I_STR,  (uchar *) &lapb_ioc) >=0);
            break;

        case WAN:
            wan_ioc.ic_cmd=H_STAT_REQ;
            wan_ioc.ic_timout=0;
            wan_ioc.ic_len=sizeof(wan_stats);
            wan_ioc.ic_dp=(char *) &wan_stats;
            wan_stats.link=brdent[vline].real_subnet_ID;
            got_stats=(MPSioctl(infd, X_STR, (char *) &wan_ioc) >= 0); /* #6 */
            break;

        case ETHR:
            eth_ioc.ic_cmd=DATAL_STAT; 
            eth_ioc.ic_len=sizeof(eth_stats); 
            eth_ioc.ic_dp=(char *) &eth_stats; 
            got_stats=(S_IOCTL(infd, I_STR,  (uchar *) &eth_ioc) >=0);
            break;

        }
        if (!got_stats)
        { 
            if (strcmp(proto_name,"WAN")!=0) 
            {
                fprintf(stderr,
		    "%s: No statistics for \"%s\".\n",
		    program_name, proto_name); 
                S_EXIT (1); 
            }
        } 
    }

    /*
        Collect global protocol statistics 
    */

    else
    {
	if (gthr_stats_proto & which_level2_proto) 
        { 
            switch(gthr_stats_proto)
	    {
		case X25:
#if 1 /* uconx */
                if ((fd = MPSopen ( &oreq_x25 ) ) == -1 )
                {
                    printf("Open request to server '%s' service '%s' failed.\n",
                        oreq_x25.serverName, oreq_x25.serviceName);
                    MPSperror("MPSopen");
                    exit_program ( -1 );
                }
#else
                if ((fd=S_OPEN(driver_name, O_RDONLY))==-1) 
                { 
                    if (errno==ENOENT) 
                        fprintf(stderr, "No global statistics for %s\n",
                            proto_name); 
                    else 
                        fprintf(stderr, "Failed to access driver: %s\n",
                            driver_name); 
                } 
#endif
                /* #2 */
                if ( ntohl(0x1) != 0x1 )
                {
#ifdef TRACE
                    printf("Little Endian -- pushing x25swap module.\n" );
#endif
                    if ( MPSioctl (fd, X_PUSH, (uchar *) "x25swap" ) == -1 )
                    {
                        MPSperror ( "MPSioctl swap X_PUSH" );
                        exit_program ( -1 );
                    }
                }
		break;

                case LAPB:
#if 1 /* uconx */
                if ((fd = MPSopen ( &oreq_lapb ) ) == -1 )
                {
                    printf("Open request to server '%s' service '%s' failed.\n",
                        oreq_lapb.serverName, oreq_lapb.serviceName);
                    MPSperror("MPSopen");
                    exit_program ( -1 );
                }
                /* #2 */
                if ( ntohl(0x1) != 0x1 )
                {
#ifdef TRACE
                    printf("Little Endian -- pushing llswap module.\n" );
#endif 
                    if ( MPSioctl (fd, X_PUSH, (uchar *) "llswap" ) == -1 )
                    {
                        MPSperror ( "MPSioctl swap X_PUSH" );
                        exit_program ( -1 );
                    }
                }
#endif
                break;
 
	        default:
		printf ("Invalid proto (%d)\n", gthr_stats_proto);
		exit_program ( -1 );
	    }
	    /*
	    * uconx. We have an fd. Check out the stats.
	    */
            { 
                int do_stats;

                if (opt_specify_subnetwk)
                {
                    do_stats=opt_specify_board_no;
                
                }
                else
		    do_stats=TRUE;

                switch(gthr_stats_proto)
                {
                    case LLC2:
                        llc2_ioc.ic_cmd=L_GETGSTATS; 
                        llc2_ioc.ic_len=sizeof(llc2_g); 
                        llc2_ioc.ic_dp=(char *)&llc2_g; 
                        llc2_g.lli_type=LI_GSTATS; 
                        got_stats=(S_IOCTL(fd, I_STR, (uchar *) &llc2_ioc) >=0);
                        if (got_stats && do_stats)
                            disp_llc2_glob_stats(); 
                        break;
                    case LAPB:
                        lapb_ioc.ic_cmd=L_GETGSTATS; 
                        lapb_ioc.ic_len=sizeof(lapb_g); 
                        lapb_ioc.ic_dp=(char *)&lapb_g; 
                        lapb_ioc.ic_timout=0; 
                        lapb_g.lli_type=LI_GSTATS; 
                        got_stats=(S_IOCTL(fd, I_STR, (uchar *) &lapb_ioc) >=0); 
                        if (got_stats && do_stats)
                            disp_lapb_glob_stats(); 
                        break;
                    case MLP:
                        mlp_ioc.ic_cmd=M_GETGSTATS; 
                        mlp_ioc.ic_len=sizeof(mlp_g); 
                        mlp_ioc.ic_dp=(char *)&mlp_g; 
                        mlp_g.lli_type=LI_GSTATS; 
    
                        got_stats=(S_IOCTL(fd, I_STR, (uchar *) &mlp_ioc) >=0);
                        if (got_stats && do_stats)
                            disp_mlp_glob_stats(); 
                        break;
		}

                if (!got_stats)
                { 
                    fprintf(stderr,
			    "%s: Failed to gather global \"%s\" stats\n",
			    program_name, proto_name); 
                    S_EXIT(1); 
                } 
                if (S_CLOSE(fd)==-1)
                { 
                    fprintf(stderr, "%s: Failed to close \"%s\".\n",
                            program_name, driver_name); 
                    S_EXIT(1); 
		} 
	    }
        } 
    }
    return((int) 0);
}



/*****************************************************************/ 
int disp_mlp_glob_stats(void) 
/*****************************************************************/ 

{
    if (opt_reset_all) return((int) 0);

    printf("\n\nGLOBAL STATISTICS FOR MLP\n"); 
    printf("--------------------------\n\n"); 
    if (current_board > -1) 
    {
        printf("Board : %d\n\n", current_board); 
    }     
    printf("----------------------------------------\n"); 
    printf("Statistic               TX           RX\n"); 
    printf("----------------------------------------\n"); 
    printf(" Frames          %10u   %10u\n", 
        mlp_g.mlpgstats[MLP_frames_tx], 
        mlp_g.mlpgstats[MLP_frames_rx]); 
    printf(" Bytes           %10u   %10u\n", 
        mlp_g.mlpgstats[MLP_bytes_tx], 
        mlp_g.mlpgstats[MLP_bytes_rx]); 
    printf(" Resets          %10u   %10u\n", 
        mlp_g.mlpgstats[MLP_reset_tx], 
        mlp_g.mlpgstats[MLP_reset_rx]); 
    printf(" Reset Confirms  %10u   %10u\n", 
        mlp_g.mlpgstats[MLP_confs_tx], 
        mlp_g.mlpgstats[MLP_confs_rx]); 
    return((int) 0);
}




/*****************************************************************/ 
int disp_lapb_glob_stats(void) 
/*****************************************************************/ 

{
    if (opt_reset_all) return((int) 0);
    printf("\n\nGLOBAL STATISTICS FOR LAPB\n"); 
    printf("--------------------------\n\n"); 
    if (current_board > -1) 
    {
        printf("Board : %d\n\n", current_board); 
    }     
    printf("---------------------------------------\n"); 
    printf("Statistic              TX            RX\n"); 
    printf("---------------------------------------\n"); 
    printf(" Frames        %10u    %10u\n", 
        lapb_g.lapbgstats[frames_tx], 
        lapb_g.lapbgstats[frames_rx]); 
    printf(" Bytes         %10u    %10u\n",
        lapb_g.lapbgstats[bytes_tx], 
        lapb_g.lapbgstats[bytes_rx]); 
    printf(" SABMs         %10u    %10u\n",
        lapb_g.lapbgstats[sabm_tx], 
        lapb_g.lapbgstats[sabm_rx]); 
    return((int) 0);
}



/*****************************************************************/ 
int disp_llc2_glob_stats(void) 
/*****************************************************************/ 

{
    if (opt_reset_all) return((int) 0);

    printf("\n\nGLOBAL STATISTICS FOR LLC2\n"); 
    printf("--------------------------\n\n"); 

    if (current_board > -1) 
    {
        printf("Board : %d\n\n", current_board); 
    }     
    printf("----------------------------------------\n"); 
    printf("Statistic               TX           RX\n"); 
    printf("----------------------------------------\n"); 
    printf(" Frames          %10u   %10u\n", 
        llc2_g.llc2gstats[frames_tx], 
        llc2_g.llc2gstats[frames_rx]); 
    printf(" Bytes           %10u   %10u\n", 
        llc2_g.llc2gstats[bytes_tx], 
        llc2_g.llc2gstats[bytes_rx]); 
    printf(" SABMEs          %10u   %10u\n",
        llc2_g.llc2gstats[sabme_tx], 
        llc2_g.llc2gstats[sabm_rx]); 
    return((int) 0);
}




/*****************************************************************/ 
int reset_various_stats(char *stringsnid, uint32 snid) 
/*****************************************************************/ 

{
    int vline; 
    int fd; 

    fd = -1;				/* uconx			*/
    if ((reset_stats) && (disp_protos !=0)) 
    { 
        /*
             resetting of various statistics 
             only reset the global stats once
        */ 
        printf("\nStatistics reset:\n\n"); 

        if (single_snet && disp_protos&X25)
            reset(X25,  "X25",  0, 0, PER_SNID_RESET, 0); 
        else
        {
            if (disp_protos&X25)
            {
            if (opt_specify_all_chnls)
                reset(X25, "X25", 0, 0, PER_VC_RESET, 0); 

            reset(X25,  "X25",  0, 0, GLOBAL_RESET, 0); 
            }
            if (disp_protos&IXE)
            reset(IXE, "IXE", 0, 0, GLOBAL_RESET, 0); 

            if (disp_protos&LAPB)
            reset(LAPB, "LAPB", 0, 0, GLOBAL_RESET, 0); 

            if (disp_protos&LLC2)
            reset(LLC2, "LLC2", 0, 0, GLOBAL_RESET, 0); 

            if (disp_protos&MLP)
            reset(MLP,  "MLP",  0, 0, GLOBAL_RESET, 0); 
        }

        /* 
            reset LAPB/LLC2 statistics 
        */ 
        if (!opt_specify_board_no && (entry_no !=0) &&
            (disp_protos & line_based_proto)) 
        { 
            for (vline=0; vline < entry_no; vline++) 
            { 
#if 1
                if ((fd = MPSopen ( &oreq_lapb ) ) == -1 )
                {
                    printf("Open request to server '%s' service '%s' failed.\n",
                        oreq_lapb.serverName, oreq_lapb.serviceName);
                    MPSperror("MPSopen");
                    exit_program ( -1 );
                }

#else
                if ((fd=S_OPEN(brdent[vline].device, 
                     O_RDONLY))==-1)
                { 
                    fprintf (stderr,
                        "Failed to access driver : %s",
                        brdent[vline].device); 
                } 
#endif
            	/* #2 */
            	if ( ntohl(0x1) != 0x1 )
            	{
#ifdef TRACE
                    printf("Little Endian -- pushing llswap module.\n" );
#endif 
                    if ( MPSioctl ( fd, X_PUSH, (uchar *) "llswap" ) == -1 )
                    {
                        MPSperror ( "MPSioctl swap X_PUSH" );
                        exit_program ( -1 );
                    }
                }
                x25tosnid((uint32)
                    brdent[vline].real_subnet_ID, 
                    (unsigned char*)stringsnid); 
                printf("\tSubnetwork %s:\n",stringsnid);

                if (brdent[vline].proto & ETHR) 
                {
                    reset(ETHR, "ETHR", fd, vline,
                        PER_PROTOCOL_RESET, (uint32)NULL); 
                }
                else if (brdent[vline].proto & LLC2)  
                {
                    reset(LLC2, "LLC2", fd, vline,
                        PER_PROTOCOL_RESET, (uint32)NULL); 
                }
                else if (brdent[vline].proto & LAPB) 
                {
                    reset(LAPB, "LAPB", fd, vline,
                        PER_PROTOCOL_RESET, (uint32)NULL); 
                }
                else if (brdent[vline].proto & MLP)
                {
                    reset(MLP, "MLP", fd, vline,
                        PER_PROTOCOL_RESET, (uint32)NULL); 
                }
                else if (brdent[vline].proto & WAN)            /* #6 */
                {
                    if ((fd = MPSopen ( &oreq_wan ) ) == -1 )
                    {
                        printf("Open request to server '%s' service '%s' failed.\n",
                            oreq_wan.serverName, oreq_wan.serviceName);
                        MPSperror("MPSopen");
                        exit_program ( -1 );
                    }
                    if ( ntohl(0x1) != 0x1 )
                    {
#ifdef TRACE
                    printf("Little Endian -- pushing wanswap module.\n" );
#endif
                        if ( MPSioctl ( fd, X_PUSH, (uchar *) "wanswap" ) == -1 )
                        {
                        MPSperror ( "MPSioctl swap X_PUSH" );
                        exit_program ( -1 );
                        }
                    }


                    reset(WAN, "WAN", fd, vline,
                        PER_PROTOCOL_RESET, (uint32)NULL); 
                }

                if (S_CLOSE(fd)==-1)
                {
                    fprintf(stderr,
                        "%s: Cannot close driver.\n",
                        program_name );
                    S_EXIT(1);
                }
            } 
        }    
        if (!reset_a_statistic)
            printf("\t\tNone\n\n");
    } 
    return((int) 0);
}

/*****************************************************************/ 
int reset(int proto, char *proto_name, int infd, int vline, int which_reset,
      uint32 snid) 
/*****************************************************************/ 

{

    int    	done_reset=FALSE;
    int    	fd;
    int    	on_board;
    OpenRequest	*pOreq;

#ifdef TRACE
    printf ("reset proto %d, name %s, fd %d\n", proto, proto_name, infd);
#endif

    fd = -1;				/* uconx			*/

    what_is_onboard(&on_board);

    if ((which_reset==PER_SNID_RESET) && opt_reset_sub_stats &&
        (proto == X25))
    {
        int fd;
        struct persnidstats pss;

        fd = -1;			/* uconx			*/
#if 1
        if ((fd = MPSopen ( &oreq_x25 ) ) == -1 )
        {
            printf("Open request to server '%s' service '%s' failed.\n",
                        oreq_x25.serverName, oreq_x25.serviceName);
            MPSperror("MPSopen");
            exit_program ( -1 );
        }
#else
        if ((fd=S_OPEN(X25_DEVICE, O_RDWR))==-1) 
        { 
            fprintf(stderr, "Failed to access driver: %s\n", 
                    X25_DEVICE); 
            return((int) 0);
        } 
#endif
            /* #2 */
        if ( ntohl(0x1) != 0x1 )
        {
#ifdef TRACE
            printf("Little Endian -- pushing x25swap module.\n" );
#endif
            if ( MPSioctl (fd, X_PUSH, (uchar *) "x25swap" ) == -1)
            {
                MPSperror ( "MPSioctl swap X_PUSH" );
                exit_program ( -1 );
            }
        }
        pss.snid = snidtox25((unsigned char *)&subnet_list[0]);

        x25_ioc.ic_cmd=N_zeroSNIDstats; 
        x25_ioc.ic_timout=0; 
        x25_ioc.ic_len=sizeof(pss); 
        x25_ioc.ic_dp=(char *)&pss; 

        if (S_IOCTL(x25, I_STR, (uchar *) &x25_ioc) < 0) 
        {
            printf("IOCTL FAILED!\n");
        }
        printf("\t\tReset per-SNID statistics for %s.\n\n", proto_name);
                reset_a_statistic=TRUE;
                (void)S_CLOSE(fd);
                return((int)0);
    }

    if ((((which_reset==PER_PROTOCOL_RESET ||
           which_reset==PER_VC_RESET ) && opt_reset_sub_stats &&
               infd > 0))
        || ((which_reset==PER_VC_RESET) && (proto==X25)))
    {
        switch(proto)
        {
        case X25:
            x25_ioc.ic_cmd=N_zeroVCstats; 
            x25_ioc.ic_timout=0; 
            done_reset=(circuits_present &&
                    S_IOCTL(x25, I_STR, (uchar *) &x25_ioc) >= 0); 
            break;


        case ETHR:
            eth_ioc.ic_cmd=DATAL_ZERO; 
            eth_ioc.ic_timout=0; 
            eth_ioc.ic_len=sizeof(eth_stats); 
            eth_ioc.ic_dp=(char *) &eth_stats; 
            done_reset=(S_IOCTL(infd, I_STR, (uchar *) &eth_ioc) >= 0); 
            break;

        case MLP:
            mlp_ioc.ic_len=sizeof(mlp_s); 
            mlp_ioc.ic_cmd=M_ZEROSTATS; 
            mlp_ioc.ic_dp=(char *) &mlp_s; 
            mlp_ioc.ic_timout=0; 

            mlp_s.lli_snid=brdent[vline].real_subnet_ID; 
            mlp_s.lli_type=LI_STATS; 
            done_reset=(S_IOCTL(infd, I_STR, (uchar *) &mlp_ioc) >= 0); 
            break;

        case LLC2:
            llc2_ioc.ic_len=sizeof(llc2_s); 
            llc2_ioc.ic_cmd=L_ZEROSTATS; 
            llc2_ioc.ic_dp=(char *) &llc2_s; 
            llc2_ioc.ic_timout=0; 

            llc2_s.lli_snid=brdent[vline].real_subnet_ID; 
            llc2_s.lli_type=LI_STATS; 
            done_reset=(S_IOCTL(infd, I_STR, (uchar *) &llc2_ioc) >= 0); 
            break;

        case LAPB:
            lapb_ioc.ic_len=sizeof(lapb_s); 
            lapb_ioc.ic_cmd=L_ZEROSTATS; 
            lapb_ioc.ic_dp=(char *) &lapb_s; 
            lapb_ioc.ic_timout=0; 

            lapb_s.lli_snid=brdent[vline].real_subnet_ID; 
            lapb_s.lli_type=LI_STATS; 
            done_reset=(S_IOCTL(infd, I_STR, (uchar *) &lapb_ioc) >= 0); 
            break;

        case WAN:
            wan_ioc.ic_cmd=H_CLEAR_STAT;
            wan_ioc.ic_timout=0;
            wan_ioc.ic_len=sizeof(wan_stats);
            wan_ioc.ic_dp=(char *) &wan_stats;
            wan_stats.link=brdent[vline].real_subnet_ID;
            done_reset=(MPSioctl(infd, X_STR, (char *) &wan_ioc) >= 0);
            break;
        }
        if (done_reset < 0)
        {
            if (proto==ETHR)
                fprintf(stderr, "\t\tFailed to reset statistics for ethernet.\n\n");
            else if (proto==WAN)
                fprintf(stderr, "\t\tFailed to reset statistics for wan.\n\n");
            else
                fprintf(stderr, "\t\tFailed to reset per-protocol statistics for %s.\n\n", proto_name);

            reset_a_statistic=TRUE;
        }
        else
        {
            if (proto==ETHR)
                printf("\t\tReset statistics for the ethernet.\n\n");
            else if (proto==WAN)
                printf("\t\tReset statistics for wan.\n\n");
            else
                printf("\t\tReset per-protocol statistics for %s.\n\n", proto_name);
            reset_a_statistic=TRUE;
        }
    }

    if ( ( which_reset==GLOBAL_RESET && opt_reset_glob_stats && 
        (disp_protos & proto || opt_reset_all) )
      || ( which_reset==GLOBAL_RESET && opt_specify_board_no &&
         disp_protos & proto && opt_reset ) )
    {
        char      driver_name[MAX_CHAR];
        char    * name;
        char    * board;
        int      i;

        for(i=0;i<no_done_devices;++i)
        {
            name = done_device[i];

            *driver_name = (char)NULL;

            if (proto==LAPB && strncmp( name, LAPBPFX,
                        strlen(LAPBPFX)-1  ) == 0 ) 
            {
                strcpy(driver_name, name );
                *(done_device[i]) = (int)NULL;
            }

            if (proto==LLC2 && strncmp( name, LLCPFX,
                        strlen(LLCPFX)-1 ) == 0 ) 
            {
                strcpy(driver_name, name);
                *(done_device[i]) = (int)NULL;
            }

            if (proto==IXE && strncmp( name, IXE_DEVICE,
                        strlen(IXE_DEVICE)-1 ) == 0 )  
            {
                strcpy(driver_name, name);
                *(done_device[i]) = (int)NULL;
            }

            if (proto==X25 && strncmp( name, X25_DEVICE,
                        strlen(X25_DEVICE)-1 ) == 0 )  
            {
                strcpy(driver_name, name);
                *(done_device[i]) = (int)NULL;
            }

            if (proto==MLP && strncmp( name, MLPPFX,
                        strlen(MLPPFX)-1 ) == 0 )  
            {
                strcpy(driver_name, name);
                *(done_device[i]) = (int)NULL;
            }
/* #6 */
            if (proto==WAN && strncmp( name, WANPFX,
                        strlen(WANPFX)-1 ) == 0 )  
            {
                strcpy(driver_name, name);
                *(done_device[i]) = (int)NULL;
            }

            if (*driver_name==(char)NULL)
            {
                continue;
            }

            board = (char*)( driver_name + strlen(driver_name) -1 );
            if (on_board & proto && opt_specify_board_no
                && numeric(board)
                && selected_board_no != atoi(board) )
            {
                continue;
            }
#if 1
	    pOreq = 0;			/* Make -Wall happy		*/
	
	    switch (proto)
 	    {
	    case X25:
	    pOreq = &oreq_x25;
	    break;

	    case LAPB:
	    pOreq = &oreq_lapb;
	    break;

	    case WAN:
	    pOreq = &oreq_wan;
	    break;

   	    }
	    if ((pOreq != 0)
            && ((fd = MPSopen ( pOreq) ) == -1 ))
            {
                printf("Open request to server '%s' service '%s' failed.\n",
                        pOreq->serverName, pOreq->serviceName);
                MPSperror("MPSopen");
                exit_program ( -1 );
            }
#else
            if ((fd=S_OPEN(driver_name, O_RDONLY))==-1)
            { 
                if (errno==ENOENT) 
                    fprintf(stderr, "No global statistics for %s\n",
			    proto_name); 
                else 
                    fprintf(stderr, "Failed to access driver: %s\n",
			    driver_name); 
            } 
#endif
            /* #2 */
            if ( ntohl(0x1) != 0x1 )
            {
#ifdef TRACE
                printf("Little Endian -- pushing llswap module.\n" );
#endif 
                if ( MPSioctl ( fd, X_PUSH, (uchar *) "llswap" ) == -1 )
                {
                    MPSperror ( "MPSioctl swap X_PUSH" );
                    exit_program ( -1 );
                }
            }

            switch(proto)
            {
            case X25:
                x25_ioc.ic_cmd=N_zerostats; 
                x25_ioc.ic_timout=0; 
                done_reset=(S_IOCTL(fd, I_STR, (uchar *) &x25_ioc) >= 0); 
                break;
#if 0
            case IXE:
                ixe_ioc.ic_cmd=IXE_STATINIT;
                ixe_ioc.ic_timout=0;
                ixe_ioc.ic_len=sizeof(struct ixe_statinit);
                ixe_ioc.ic_dp=(char *)&rststatblk;
                done_reset=(S_IOCTL(fd, I_STR, (uchar *) &ixe_ioc) >= 0); 
                break;
#endif
            case LAPB:
                lapb_ioc.ic_cmd=L_ZEROGSTATS; 
                lapb_ioc.ic_len=sizeof(lapb_g); 
                lapb_ioc.ic_dp =(char *) &lapb_g; 
                lapb_g.lli_type=LI_GSTATS; 
                done_reset=(S_IOCTL(fd, I_STR, (uchar *) &lapb_ioc) >= 0);
                break;
#if 0
            case MLP:
                mlp_ioc.ic_cmd=M_ZEROSTATS; 
                mlp_ioc.ic_len=sizeof(mlp_g); 
                mlp_ioc.ic_dp=(char *)&mlp_g; 
                mlp_g.lli_type=LI_GSTATS; 
                done_reset=(S_IOCTL(fd, I_STR, (uchar *) &mlp_ioc) >= 0);
                break;
#endif
#if 0
            case LLC2:
                llc2_ioc.ic_cmd=L_ZEROGSTATS; 
                llc2_ioc.ic_len=sizeof(llc2_g); 
                llc2_ioc.ic_dp=(char *)&llc2_g; 
                llc2_g.lli_type=LI_GSTATS; 
                done_reset=(S_IOCTL(fd, I_STR, (uchar *) &llc2_ioc) >= 0);
                break;
#endif
            }

            if (!done_reset)
            {
                if (numeric(board) && on_board & proto)
                    fprintf(stderr, "\t\tFailed to reset global statistics for %s : board %s. \n\n", proto_name, board);
                else
                    fprintf(stderr, "\t\tFailed to reset global statistics for %s - in kernel.\n\n", proto_name );
            }
            else
            {
                if (numeric(board) && on_board & proto)
                    printf("\t\tReset global statistics for %s : board %s.\n\n", proto_name, board);
                else
                    printf("\t\tReset global statistics for %s - in kernel.\n\n", proto_name );
                reset_a_statistic=TRUE;
            }
            if (S_CLOSE(fd)==-1)
            {
                printf("\t\tFailed to close %s\n\n",
                    driver_name);
            }
        }
    }
    return((int) 0);
}



/*****************************************************************/ 
void close_all_while_asleep(void) 
/*****************************************************************/ 

{
#if 1

    if ( MPSclose (x25) == -1 )
    {
        printf("Close request to server %s service %s failed.\n",
                oreq_x25.serverName, oreq_x25.serviceName);
        MPSperror("MPSclose");
        exit_program ( -1 );
    }
 
    if ( MPSclose (lapb) == -1 )
    {
        printf("Close request to server '%s' service '%s' failed.\n",
                oreq_lapb.serverName, oreq_lapb.serviceName);
        MPSperror("MPSclose");
        exit_program ( -1 );
    }

/* #6 */
    if ( MPSclose (wan) == -1 )
    {
        printf("Close request to server '%s' service '%s' failed.\n",
                oreq_wan.serverName, oreq_wan.serviceName);
        MPSperror("MPSclose");
        exit_program ( -1 );
    }

#else
    if (x25 >=0)
    {
        if (S_CLOSE(x25)==-1)
        {
            fprintf(stderr, "Cannot close driver \n");
        }
    }
    if (ixe >=0)
    {
        if (S_CLOSE(ixe)==-1)
        {
            fprintf(stderr, "Cannot close driver \n");
        }
    }
#endif
}

/*****************************************************************/ 
char *basename(char *path) 
/*****************************************************************/ 

/*
    return the filename part of a pathname by stripping 
    off all of the path except the last '/'. 
*/
{
     char *base=path; 
    while (*base) 
        if (*base++=='/') path=base; 
    return(path); 
}



/*****************************************************************/ 
int numeric(char *arg) 
/*****************************************************************/ 

/*
    this includes only unsigned positive integers. this algorithm 
    handles "null" strings (but expects a valid char *). 
*/

{
     register char *s=arg; 
    int num=0; 

    if (strcmp(arg,"?")==0) return(0);

    while (*s && (num=isdigit(*s++))) ; 
    return(num); 
}

/*****************************************************************/ 
int usage(char *program, char *arguments) 
/*****************************************************************/ 
{
   fprintf(stderr, "%s: %s\n", program, arguments); 
   fprintf(stderr, "Usage: %s %s\n", program, USAGE1); 
   help_message();
   return((int) 0); 
}



/*****************************************************************/ 
int error(char *message, char *arg) 
/*****************************************************************/ 

/*
    this takes a message and an optal arg, adds a program 
    identifier and newline, and prints the resulting message 
    on the error stream. 
*/

{
     char string[100]; 

    sprintf(string, message, arg); 
    fprintf(stderr, "%s: %s\n", program_name, string); 
    return((int) 0);
}



/*****************************************************************/ 
int unrecognised_proto(char *program, char *protos) 
/*****************************************************************/ 

{
     error("unrecognised protocol \"%s\"", protos); 
    fprintf(stderr, "Usage: %s %s\n", program, USAGE1); 
        help_message();
    return(2); 
}



/*****************************************************************/ 
int non_numeric_interval(char *argument) 
/*****************************************************************/ 

{
     error("non-numeric interval: %s", argument); 
    return(2); 
}


/*****************************************************************/ 
int alllower(char *s) 
/*****************************************************************/ 
{
     int c; 
    while ((c=tolower (*s))) *s++=c; 
    return((int) 0);
}



/*****************************************************************/ 
int validsnid (uint32 sn_id) 
/*****************************************************************/ 

{
    char str_snid[SN_PRINT_LEN];
    int i=0;

    x25tosnid(sn_id, (unsigned char*)str_snid);

    while (str_snid[i])
        if (! isalnum(str_snid[i++]))
            return(0);

    return(1);
}



/*****************************************************************/ 
int get_VCState(char *VCState, int xstate) 
/*****************************************************************/ 

/*
    converts the value of the circuits state into a 
    string format. 
*/

{
     switch (xstate) 
    { 
        case Idle: 
        strcpy(VCState, "Idle"); 
        break; 

        case AskingNRS: 
        strcpy(VCState, "AskingNRS"); 
        break; 

        case P1: 
        strcpy(VCState, "P1"); 
        break; 

        case P2: 
        strcpy(VCState, "P2"); 
        break; 

        case P3: 
        strcpy(VCState, "P3"); 
        break; 

        case P5: 
        strcpy(VCState, "P5"); 
        break; 

        case DataTransfer: 
        strcpy(VCState, "Datatransfer"); 
        break; 

        case DXEbusy: 
        strcpy(VCState, "DXEbusy"); 
        break; 

        case D2: 
        strcpy(VCState, "D2"); 
        break; 

        case D2pending: 
        strcpy(VCState, "D2pending"); 
        break; 

        case WtgRCU: 
        strcpy(VCState, "WtgRCU"); 
        break; 

        case WtgRCN: 
        strcpy(VCState, "WtgRCN"); 
        break; 

        case WtgRCNpending: 
        strcpy(VCState, "WtgRCNpending"); 
        break; 

        case P4pending: 
        strcpy(VCState, "P4pending"); 
        break; 

        case pRESUonly: 
        strcpy(VCState, "pRESUonly"); 
        break; 

        case RESUonly: 
        strcpy(VCState, "RESUonly"); 
        break; 

        case pDTransfer: 
        strcpy(VCState, "pDTransfer"); 
        break; 

        case WRCUpending: 
        strcpy(VCState, "WRCUpending"); 
        break; 

        case DXErpending: 
        strcpy(VCState, "DXErpending"); 
        break; 

        case DXEresetting: 
        strcpy(VCState, "DXEresetting"); 
        break; 

        case P6: 
        strcpy(VCState, "P6"); 
        break; 

        case P6pending: 
        strcpy(VCState, "P6pending"); 
        break; 

        case WUcpending: 
        strcpy(VCState, "WUcpending"); 
        break; 

        case WUNcpending: 
        strcpy(VCState, "WUNcpending"); 
        break; 

        case DXEcpending: 
        strcpy(VCState, "DXEcpending"); 
        break; 

        case DXEcfpending: 
        strcpy(VCState, "DXEcfpending"); 
        break; 

        case DXEclearing: 
        strcpy(VCState, "DXEclearing"); 
        break; 

        default: 
        fprintf(stderr, 
            "Unknown VCstate %d\n", 
            bufp->xstate); 
        S_EXIT(1); 
    } 
    return((int) 0);
}


#ifdef OLDFORMAT    
/*****************************************************************/ 
int FPRINT(char *s1, char *s2, int i1, int i2) 
/*****************************************************************/ 

/*
    used to print x25 statistics out in a two col 
    format provided the arguments allow this 
*/

{
     int val1, val2; 

    val1=x25_stats[i1]; 
    val2=x25_stats[i2]; 
    printf(" %s\t %d", s1, val1); 

    if (val1 > 999999) /* check if line is a 
                * long one 
                */ 
        printf("\n %s\t %d\n", s2, val2); 
    else 
    { 
        if (val2 > 99999999) 
            printf("\n %s\t %d\n", s2, val2); 
        else 
            printf("\t %s\t%d\n", s2, val2); 

    } 
    return((int) 0);
}
#endif 


/*****************************************************************/ 
int help_message(void)
/*****************************************************************/ 
{
   fprintf(stderr, "\nx25stat symbolically displays the contents of ");
   fprintf(stderr, "various per-protocol stats,\nwhich are held ");
   fprintf(stderr, "by various modules in the X.25 network.\n\n");
   fprintf(stderr, "Options :\n");
   fprintf(stderr, "  -a .. show abbreviated statistics for active VCs\n");
   fprintf(stderr, "  -c .. controller number\n");
   fprintf(stderr,
   "  -l .. show statistics for virtual circuit with given channel (hex)\n");
   fprintf(stderr, "  -L .. as above, but for all channels\n");
   fprintf(stderr, "  -n .. show subnetwork statistics\n");
   fprintf(stderr, "  -N .. show all subnetwork statistics\n");
   fprintf(stderr, "  -p .. show global statistics for protocol\n");
   fprintf(stderr, "        current protocols are X25 LAPB\n");
   fprintf(stderr, "  -s .. server name\n");
   fprintf(stderr, "  -v .. service name\n");
   fprintf(stderr, "  -w .. show wan statistics\n");
   fprintf(stderr, "  -z .. reset statistics\n");
   fprintf(stderr, "  -f .. alternative x25conf file (default: /etc/x25conf)\n");
   fprintf(stderr, "  number .. delay between repeating statistics\n");
   fprintf(stderr, "\n");
   return((int) 0);
}

#ifdef VXWORKS
void terminate (void)
{
   interval = (int)-1;
}
#endif /* VXWORKS */
