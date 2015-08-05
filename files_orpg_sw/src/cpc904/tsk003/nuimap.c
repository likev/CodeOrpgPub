/*   @(#) nuimap.c 99/11/02 Version 1.5   */

/******************************************************************
 *
 *  SpiderX25 - NUI/facilities editing utility
 *
 *  Copyright 1991  Spider Systems Limited
 *
 *  NUIMAP.C
 *
 *  User utility to manipulate NUI/facility associations
 *
 ******************************************************************/

/*
	Modification history:
 
Chg	Date       Init  Description
1.  12-NOV-97  mpb   Need to have NT type file specification.
2.  20-APR-98  mpb   Compile on QNX (TCP/IP).
3.  11-JUN-98  mpb   Fully port to Windows NT.
4.  11-JUN-98  mpb   Push swap module if needed (little endian architecture).
5.  11-JUN-98  mpb   Need to get the system variable %SystemRoot% from
                     getenv().
6.  14-OCT-98  lmm   Cleaned up usage strings, added support for controller # 
7.  08-OCT-99  tdg   Added VxWorks supported
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifdef  WINNT
typedef char *    caddr_t;
#include <winsock.h>
int    getopt ( int, char *const *, const char * );
#endif /* !WINNT */

#if defined ( __hp9000s800 ) || defined ( QNX ) || defined ( WINNT )
#include "xstypes.h"
#include "xstra.h"
#else
#ifdef VXWORKS
#include <in.h>
#include <streams/stream.h>
#else
#include <sys/stream.h>
#endif /* VXWORKS */
#endif

#include "xstopts.h"
#include "mpsproto.h"
#include "uint.h"
#include "ll_proto.h"
#include "x25_proto.h"
#include "x25_control.h"

#ifdef WINNT
#define	DEFAULT_SERVER 	"\\\\.\\PTIXpqcc"
#else
#define	DEFAULT_SERVER 	"/dev/ucsclone"
#endif /* WINNT */

/* From Space.c */
/*
 * Maximum number of entries in address table, and
 * size of hash tables into it.
 * Hash table size MUST be at least twice address
 * table size, and preferably prime.
 */
#define MAX_NUIADDRS    100
#define MAX_NUIHASH     251
NUI_ADDR        nui_factab[MAX_NUIADDRS];       /* Address table        */

/* File Mode for Read or Write */
#define O_RW	2	
#define MAXLINE 80

/* extern struct nuiformat; */
struct	nuiformat	user_nui;

/* from x25tune.c declarations */
#define	ACCREAD	04
extern open_conf(), close_conf();
extern char  	*get_conf_line(); 

FILE	*fopen();

/* Command line handling. */
#define VALIDOPTIONS "?PGDMZf:n:s:v:c:"
#define USAGE1 "usage: %s [-DG] [-n NUI] [-s server] [-v service] [-c controller]\n"
#define USAGE2 "       %s [-MZ] [-s server] [-v service] [-c controller]\n"
#define USAGE3 "       %s -P [-f file] [-s server] [-v service] [-c controller]\n"

#ifndef WINNT
#define DEFFILE "/etc/nuimapconf"
#define DEFDEV "/dev/x25"    /* This is not used (bit rot). */
#else  /* #1 */
#define DEFFILE "\\System32\\uconx\\etc\\nuimapconf"
#endif /* !WINNT */

static struct xstrioctl io;		/* stream ioctl structure */
static struct nui_put spbit;		/* structure passed to put_ioctl */
static struct nui_get infrep;		/* received in get_ioctl */
static struct nui_del infdel;  	/* used to delete entry */
static struct nui_mget infmget;	/* received in multi-get ioctl */
static struct nui_reset infzero;	/* used to reset address table */

extern int errno;
extern char *optarg;
extern int optind;

static int  mget_ioctl ( );
static int  reset_ioctl ( );
static int  comment ( );
static int  get_ent ( );
static int  put_ioctl ( );
static int  get_ioctl ( );
static int  del_ioctl ( );


#ifdef    ANSI_C
static void exit_program ( int );
#else
static void exit_program ( );
static int error_val;
#endif	/* ANSI_C */

#ifdef VXWORKS
nuimap (argc, argv, ptg)
#else
int main(argc,argv)
#endif /* VXWORKS */
int argc;
char *argv[];
#ifdef VXWORKS
pti_TaskGlobals *ptg;
#endif /* VXWORKS */
{
	int eof;
	int dd;			/*file descriptor for device */
	int put_reqd = 0;
	int get_reqd = 0;
	int del_reqd = 0;
	int mget_reqd = 0;
	int reset_reqd = 0;
	int opt_reqd = 0;
	int no_file = 1;
	int no_nui=1;
	int ents_added = 0;
	int c;   /* getopt() returns an int. */
	FILE *fopen(), *fp;
	OpenRequest oreq;       /* MPS open structure */

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
#endif /* VXWORKS */

	/* Set defaults for server, service name. */
    memset ( &oreq, 0, sizeof ( oreq ) );
	strcpy(oreq.serverName, DEFAULT_SERVER );
	strcpy(oreq.serviceName, "mps");

	/*get text file name and open*/
	while ( ( ( c = getopt ( argc, argv, VALIDOPTIONS ) ) ) != EOF )
	{
		switch (c)
		{
		case 'n' :	/* NUI specified */
				/* check it is not null, 
				   then take a copy.	*/
				
				if (strcmp(optarg, "") == 0)
				{
					fprintf(stderr,"Null NUI specified.\n");
					exit_program (0);
				}
				else	/* non-null NUI */
				{
					user_nui.nui_len=strlen(optarg);
					strcpy((char *)user_nui.nui_string,
						(char *)optarg);
				}
				no_nui = 0;
				break;
				
		case 'f' :	/* File specifed */
				if (! reset_reqd)
				{
					if ((fp = fopen(optarg, "r")) == NULL)
					{
						perror("Failed to open specified file");
						exit_program(1);
					}
					no_file = 0;
#ifdef DEBUG
					fprintf(stderr, "Opened file '%s'\n", optarg);
#endif
				}
				break;

		case 's':
				strcpy(oreq.serverName, optarg);
				break;

		case 'v':
				strcpy(oreq.serviceName, optarg);
				break;

		case 'c':
				oreq.ctlrNumber = strtol ( optarg, 0, 10 );
			    break;

		case 'P' :	/* putioctl option */
				put_reqd = 1;
				break;

		case 'G' :	/* getioctl option */
				get_reqd = 1;
				break;

		case 'D' :	/* del_ioctl option */
				del_reqd = 1;
				break;

		case 'M' :	/* multi-get ioctl option */
				mget_reqd = 1;
				break;
				
		case 'Z' :	/* reset table ioctl option */
				reset_reqd = 1;
				break;

		case '?' :	/* ERROR */
                fprintf(stderr, USAGE1, argv[0]);
                fprintf(stderr, USAGE2, argv[0]);
                fprintf(stderr, USAGE3, argv[0]);
			    exit_program (1);

		default:	printf("tr\n");
				break;

			}
	}

	opt_reqd = put_reqd + get_reqd + del_reqd + reset_reqd + mget_reqd;

	if ((optind != argc) || (opt_reqd != 1))
	{
		if (optind != argc)
			fprintf(stderr, "%s: Too many arguments\n", argv [0]);
		else
			if (opt_reqd > 1)
			    fprintf(stderr, "Only one of options P,G,D,M or Z may be specifed\n");
			else
  			    fprintf(stderr, "One of options P,G,D,M or Z must be specifed\n");
		fprintf(stderr, USAGE1, argv[0]);
		fprintf(stderr, USAGE2, argv[0]);
		fprintf(stderr, USAGE3, argv[0]);
		exit_program (1);
	}

	if (no_file && put_reqd)	/* Only put option can take a file */
	{
        char  config [ 256 ];

/* #5 */
#ifndef WINNT
        sprintf ( config, "%s", DEFFILE );
#else
        sprintf ( config, "%s%s", getenv ( "SystemRoot" ), DEFFILE );
#endif /* !WINNT */

		if ((fp = fopen(config, "r")) == NULL)
		{
			perror("Failed to open default file");
			exit_program(1);
		}
#ifdef DEBUG		
		fprintf(stderr, "Opened default file '%s'\n", DEFFILE); 
#endif
	}
	
	oreq.dev        = 0;   /* default x25 device to zero */
	oreq.port       = 0;
	oreq.flags      = CLONEOPEN;

	strcpy(oreq.protoName, "x25" );

	if ((dd = MPSopen(&oreq))<0)
	{
		MPSperror("MPSopen failed ");
		exit_program(1);
	}

    /* #4 */
    if ( ntohl(0x1) != 0x1 )
    {
       printf("Little Endian -- pushing swap module.\n" );

       if ( MPSioctl ( dd, X_PUSH, "x25swap" ) == MPS_ERROR )
       {
          MPSperror ( "MPSioctl swap X_PUSH" );
          exit_program ( MPS_ERROR );
       }
    }
	
	eof=0;		/* not eof */

	if (mget_reqd)
	{
		mget_ioctl(&infmget,dd);	/* multiple get */
	}

	if (reset_reqd)				
	{
		reset_ioctl(&infzero,dd);	/* empty the NUI table */
#ifdef DEBUG
		printf("prim_class =%d\n",infzero.prim_class);
		printf("op =%d\n",infzero.op); 
#endif			
	
	}		

	/* P is the only option that must process tokens in a file */
	if (put_reqd)	
	while(eof!=EOF)
	{
		ungetc((c=fgetc(fp)),fp);	/*get 1st char on line*/
		eof = c;
			/*in case of eof*/
		if (!comment(c,fp))
		{	
			eof=get_ent(&spbit, fp);	  /* get parameters  */

			if (eof > 0)		/* good entry, send it away  */
			{
				if (put_ioctl(&spbit,dd) < 0)
				{

					fprintf(stderr,
	    "nuimap: Put IOCTL failed after adding %d entries\n", ents_added);
					exit_program (1);
				}
				else
					ents_added++;
			}

			/* N.B. for a bad entry, skip and carry on (eof =0)  */
			/* but terminate loop when no more entries (eof=EOF) */			
		}
	}

	/* G and D options must have an NUI supplied */
	if ((get_reqd) || (del_reqd))
	{
		if (no_nui)
		{
			fprintf(stderr,"%s :No NUI supplied\n", argv[0]);
			exit_program(1);
		}

		if (get_reqd)
			get_ioctl(&infrep, &user_nui, dd);

		if (del_reqd) 			/* delete entry */
			del_ioctl(&infdel, &user_nui, dd);
	}

	/* close device */
	MPSclose(dd);

	/* close file (only used for P option) */
	if (put_reqd)
		fclose(fp);

#ifdef VXWORKS
    exit_program ( 0 );
#else
    return (0);
#endif /* VXWORKS */
}

comment(in_c,fp)	/*checks if c is a comment starter and skips that*/
FILE *fp;		/*line if it is */
char in_c;		/*also skips processing if EOF */
{
        /* fgetc() returns int --> EOF comparison fails with just a char. */
        int  c = in_c;

	while (c==10)	/* a LF */
	{
		c=fgetc(fp);	/*get rid of it*/
		ungetc((c=fgetc(fp)),fp);
	}

	if ((c == '#') || (c == '/') || (c == '*') || (c == EOF))
	{
		while(c!='\n' && c!=EOF)
			c=fgetc(fp);
		return(1);
	}
	else
		return(0);
}


get_ent(sb, fp)

/* Return values:	negative = bad token  		*/
/*			EOF = no more entries left	*/
/*			1 = correctly processed entry	*/
struct nui_put *sb;
FILE *fp;
{
	int ef;
	char *s;
	char *nui_part, *fac_part; 
	char line[MAXLINE];
	int nuifaclen, facpos;

#ifdef DEBUG
	printf("****************************************************\n");
	printf("\nActual table entry being processed:");
#endif

	if (fgets(line, MAXLINE, fp) == NULL)
		ef = EOF;	/*get a line */
	else
		ef = 1;		/* i.e. positive value if entry valid. */

	/* remove any newlines left by fgets() */
	if (*(s=line + strlen(line)-1) == '\n')
		{
			*s = '\0';
		}	

	/* Assumes: you may have an NUI with no facilities, 
		    but you may NOT have facilities with no NUI. */

	if ((nui_part = strtok(line," \t\n")) !=NULL)
	{
		if (strcmp(nui_part,"-")==0)
		{
			fprintf(stderr,"blank NUI in internal table\n");
			exit_program(1);
		}
		else
		{			
			strcpy((char *)sb->nuid.nui_string, (char *)nui_part);
			sb->nuid.nui_len = strlen((char *)sb->nuid.nui_string);
		}
	}

	if ((fac_part = strtok(NULL," \t\n")) != NULL)
	{
		/* set all values to zero */
		sb->nuifacility.LOCDEFPKTSIZE = 0;
		sb->nuifacility.REMDEFPKTSIZE = 0;
		sb->nuifacility.LOCDEFWSIZE = 0;
		sb->nuifacility.REMDEFWSIZE = 0;
		sb->nuifacility.locdefthclass = 0;
		sb->nuifacility.remdefthclass = 0;
		sb->nuifacility.SUB_MODES = 0;
		sb->nuifacility.CUG_CONTROL = 0;

		if (strcmp(fac_part,"-")!=0)
		{			
			/* break up short-hand string into fac. fields */
		/* from old x25aux.c:fetch_NUI_fac */
		nuifaclen = strlen(fac_part);
		facpos = 0;
		while (facpos < nuifaclen)
		{
			switch (fac_part[facpos])
			{
				case 'E':
				case 'e':	/* subscribe to extended */


					sb->nuifacility.SUB_MODES |= SUB_EXTENDED;
					/* pt to EOL or next fac */
					if (fac_part[facpos+1]==',') facpos++;
					facpos++;
					break;

				case 'X':
				case 'x':	/* subscribe to extended CUGS */
					
					sb->nuifacility.CUG_CONTROL |= EXTENDED;
					/* pt to EOL or next fac */
					if (fac_part[facpos+1]==',') facpos++;
					facpos++;
					break;

				case 'B':
				case 'b':	/* subscribe to Basic CUGs*/
					sb->nuifacility.CUG_CONTROL |= BASIC;
					/* pt to EOL or next fac */
					if (fac_part[facpos+1]==',') facpos++;
					facpos++;
					break;

						/* local and remote default packet size */
				case 'P':	/* pN/M,  N,M=4-12 */
				case 'p':
				if (fac_part[facpos+2]=='/')
				{
		/* Local */	    /* 1 digit then slash p_X_/xx[,] */ 
				    if (isdigit(fac_part[facpos+1])) 
				    {
					sb->nuifacility.LOCDEFPKTSIZE = fac_part[facpos+1]-'0';
					/* pt to slash */
					facpos+=2;
				    }
				    else
				    {
					
					printf("nuimap:Bad facility: non-digit locdefpktsize");
					return(0);
				    }
				}
				else if (fac_part[facpos+3]=='/')
				{
				    /* 2 digits then slash p_XX_/xx[,] */
				    if (isdigit(fac_part[facpos+1]) && isdigit(fac_part[facpos+2])) 
				    {
					sb->nuifacility.LOCDEFPKTSIZE = 
						10*(fac_part[facpos+1]-'0')
						+(fac_part[facpos+2]-'0');
					/* pt to slash */		
					facpos+=3;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit locdefpktsize");
					return(0);
				    }
				}
				else 
				{
					printf("nuimap:Bad facility: overlong locdefpktsize.");
					return(1);
				}

				/*single digit */ /* pxx/_X_[,]  */
	/* remote */		if ( (fac_part[facpos+2]==',')
					|| (fac_part[facpos+2]=='\0')
					|| isalpha(fac_part[facpos+2]) )
				{
				/* one digit then comma or EOL or letter for next fac */
				    if (isdigit(fac_part[facpos+1]))   
				    {
					sb->nuifacility.REMDEFPKTSIZE = fac_part[facpos+1] - '0';
					/* pt to next letter or EOL */
					if (fac_part[facpos+2]==',') facpos++;
					facpos+=2;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit remdefpktsize");
					return(0);
				    }
				}
				/* double digit pxx/_XX_[,] */
				else if ( fac_part[facpos+3]==',' 
					|| (fac_part[facpos+3]=='\0')
					|| isalpha(fac_part[facpos+3]) )
				{	
				/* 2 digits then comma or EOL or letter for next fac */
				    if (isdigit(fac_part[facpos+1]) && isdigit(fac_part[facpos+2])) 
				    {
					sb->nuifacility.REMDEFPKTSIZE = 
						10*(fac_part[facpos+1]-'0')
						+(fac_part[facpos+2]-'0');
					/* pt to next letter or EOL */		
					if (fac_part[facpos+3]==',') facpos++;
					facpos+=3;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit remdefpktsize");
					return(0);
				    }
				}
				else
				{
					printf("nuimap:Bad facility: overlong remdefpktsize");
					return(0);
				}
				break;

					/* local and remote default window size */
				case 'W':	/* wN/M,  N,M=1-127 */
				case 'w':
				if (fac_part[facpos+2]=='/')
				{
		/* Local */	    /* 1 digit then slash w_X_/xxx[,] */ 
				    if ( isdigit(fac_part[facpos+1]) )
				    {
					sb->nuifacility.LOCDEFWSIZE = fac_part[facpos+1] - '0';
					/* pt to slash */
					facpos+=2;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit locdefwsize");
					return(0);
				    }
				}
				else if (fac_part[facpos+3]=='/')
				{
				    /* 2 digits then slash w_XX_/xxx[,] */
				    if (isdigit(fac_part[facpos+1]) && isdigit(fac_part[facpos+2])) 
				    {
					sb->nuifacility.LOCDEFWSIZE = 
						10*(fac_part[facpos+1] - '0')
						+(fac_part[facpos+2] - '0');
					/* pt to slash */		
					facpos+=3;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit locdefwsize");
					return(0);
				    }
				}
				else if (fac_part[facpos+4]=='/')
				{
				    /* 3 digits then slash w_XXX_/xxx[,] */
				    if (isdigit(fac_part[facpos+1]) && isdigit(fac_part[facpos+2])
					&& isdigit(fac_part[facpos+3]))				    
				    {
					sb->nuifacility.LOCDEFWSIZE = 
						100*(fac_part[facpos+1]-'0')
						+ 10*(fac_part[facpos+2]-'0')
						+ (fac_part[facpos+3]-'0');
					/* pt to slash */		
					facpos+=4;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit locdefwsize");
					return(0);
				    }
				}
				else 
				{
					printf("nuimap:Bad facility: overlong locdefwsize.");
					return(1);
				}

				/*single digit */ /* wxxx/_X_[,]  */
	/* remote */		if ( (fac_part[facpos+2]==',')
					|| (fac_part[facpos+2]=='\0')
					|| isalpha(fac_part[facpos+2]) )
				{
				/* one digit then comma or EOL or letter for next fac */
				    if ( isdigit(fac_part[facpos+1]) )   
				    {
					sb->nuifacility.REMDEFWSIZE = fac_part[facpos+1]-'0';
					/* pt to next letter or EOL */
					if (fac_part[facpos+2]==',') facpos++;
					facpos+=2;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit remdefwsize");
					return(0);
				    }
				}
				/* double digit wxxx/_XX_[,] */
				else if ( fac_part[facpos+3]==','
					|| (fac_part[facpos+3]=='\0')
					|| isalpha(fac_part[facpos+3]) )
				{	
				/* 2 digits then comma or EOL or letter for next fac */
				    if (isdigit(fac_part[facpos+1]) 
&& isdigit(fac_part[facpos+2])) 
				    {
					sb->nuifacility.REMDEFWSIZE = 
						10*(fac_part[facpos+1]-'0')
						+(fac_part[facpos+2]-'0');
					/* pt to next letter or EOL */		
					if (fac_part[facpos+3]==',') facpos++;
					facpos+=3;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit remdefwsize");
					return(0);
				    }
				}
				/* triple digit wxxx/_XXX_[,] */
				else if ( fac_part[facpos+4]==','
					|| (fac_part[facpos+4]=='\0')
					|| isalpha(fac_part[facpos+4]) )
				{	
				/* 3 digits then comma or EOL or letter for next fac */
				    if (isdigit(fac_part[facpos+1]) && isdigit(fac_part[facpos+2])
					&& isdigit(fac_part[facpos+3])) 
				    {
					sb->nuifacility.REMDEFWSIZE = 
						100*(fac_part[facpos+1]-'0')
						+ 10*(fac_part[facpos+2]-'0')
						+ (fac_part[facpos+3]-'0');
					/* pt to next letter or EOL */		
					if (fac_part[facpos+3]==',') facpos++;
					facpos+=4;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit remdefwsize");
					return(0);
				    }
				}
				else
				{
					printf("nuimap:Bad facility: overlong remdefwsize");
					return(0);
				}
			        break;

					/* local and remote default throughput class */
				case 'T':	/* tN/M,  N,M=0-15 */
				case 't':	
				if (fac_part[facpos+2]=='/')
				{
		/* Local */	    /* 1 digit then slash t_X_/xx[,] */ 
				    if ( isdigit(fac_part[facpos+1]) )
				    {
					sb->nuifacility.locdefthclass = fac_part[facpos+1]-'0';
					/* pt to slash */
					facpos+=2;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit locdefthclass");
					return(0);
				    }
				}
				else if (fac_part[facpos+3]=='/')
				{
				    /* 2 digits then slash t_XX_/xx[,] */
				    if (isdigit(fac_part[facpos+1]) && isdigit(fac_part[facpos+2])) 
				    {
					sb->nuifacility.locdefthclass = 
						10*(fac_part[facpos+1]-'0')
						+(fac_part[facpos+2]-'0');
					/* pt to slash */		
					facpos+=3;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit locdefthclass");
					return(0);
				    }
				}
				else 
				{
					printf("nuimap:Bad facility: overlong locdefthclass.");
					return(1);
				}

				/*single digit */ /* txx/_X_[,]  */
	/* remote */		if ( (fac_part[facpos+2]==',')
					|| (fac_part[facpos+2]=='\0')
					|| isalpha(fac_part[facpos+2]) )
				{
				/* one digit then comma or EOL or letter for next fac */
				    if ( isdigit(fac_part[facpos+1]) )   
				    {
					sb->nuifacility.remdefthclass = (fac_part[facpos+1]-'0');
					/* pt to next letter or EOL */
					if (fac_part[facpos+2]==',') facpos++;
					facpos+=2;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit remdefthclass");
					return(0);
				    }
				}
				/* double digit txx/_XX_[,] */
				else if ( fac_part[facpos+3]==','
					|| (fac_part[facpos+3]=='\0')
					|| isalpha(fac_part[facpos+3]) )
				{	
				/* 2 digits then comma or EOL or letter for next fac */
				    if (isdigit(fac_part[facpos+1]) && isdigit(fac_part[facpos+2])) 
				    {
					sb->nuifacility.remdefthclass = 
						10*(fac_part[facpos+1]-'0')
						+(fac_part[facpos+2]-'0');
					/* pt to next letter or EOL */		
					if (fac_part[facpos+3]==',') facpos++;
					facpos+=3;
				    }
				    else
				    {
					printf("nuimap:Bad facility: non-digit remdefthclass");
					return(0);
				    }
				}
				else
				{
					printf("nuimap:Bad facility: overlong remdefthclass");
					return(0);
				}
			        break;

						/* Subscribe to CUGs */
				case 'G':	/* g or G */
				case 'g':
					sb->nuifacility.CUG_CONTROL |= SUB_CUG;
					/* pt to next letter or EOL */		
					if (fac_part[facpos+1]==',') facpos++;
					facpos++;
				break;

						/* Basic CUG with outgoing access */
				case 'O':	/* o or O */
				case 'o':
					sb->nuifacility.CUG_CONTROL |= SUB_CUGOA;
					/* pt to next letter or EOL */		
					if (fac_part[facpos+1]==',') facpos++;
					facpos++;
				break;
				

				default:    printf("unexpected char:%c\n", fac_part[facpos]);
					    return(0);
				
			} 	/* end switch */
		 }		/* End while loop */


		} /* if non-null facility */
	}

#ifdef DEBUG
	printf("NUI=%s, facility=%s\n",nui_part, fac_part);	
#endif
	return(ef);
}

put_ioctl(sb,dd)
struct nui_put *sb;
int dd;
{	

	io.ic_cmd=N_nuiput;
	io.ic_timout = 0;
	io.ic_len=sizeof(struct nui_put);
	io.ic_dp = (char *)(sb); 

	if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) < 0 )
    {
		return -1;
    }

	return 0;
}

print_facilities(facinfo)
/* encode a set of NUI facilities into 
   a short-hand string of characters.  */
struct facformat facinfo;
{	
	/* local and remote default packet sizes */
	if ( (facinfo.LOCDEFPKTSIZE > 0) &&
	     (facinfo.REMDEFPKTSIZE > 0) )
		printf("p%d/%d,", facinfo.LOCDEFPKTSIZE,
			facinfo.REMDEFPKTSIZE);
	/* local and remote default window sizes */
	if ( (facinfo.LOCDEFWSIZE > 0) &&
	     (facinfo.REMDEFWSIZE > 0) )
		printf("w%d/%d,", facinfo.LOCDEFWSIZE,
			facinfo.REMDEFWSIZE);
	/* local and remote default throughput classes */
	if ( (facinfo.locdefthclass > 0) &&
	     (facinfo.remdefthclass > 0) )
		printf("t%d/%d,", facinfo.locdefthclass,
			facinfo.remdefthclass);
	/* SUB_MODES */
	if (facinfo.SUB_MODES > 0)
	{
		if (facinfo.SUB_MODES&SUB_EXTENDED)
			printf("e,");
	}
	/* CUG_CONTROL */
	if (facinfo.CUG_CONTROL > 0)
	{
		if (facinfo.CUG_CONTROL&BASIC)
			printf("b,");
		if (facinfo.CUG_CONTROL&SUB_CUG)
			printf("g,");	
		if (facinfo.CUG_CONTROL&SUB_CUGOA)
			printf("o,");	
		if (facinfo.CUG_CONTROL&EXTENDED)
			printf("x,");
	}
	/* EOL */
	printf("\n");

    return 0;
}

get_ioctl(infoback,trip, dd)
/* 	Using NUI address as key, this function will find
	the corresponding facilities from the NUI table. 
	(internal to X25) 					*/

struct nui_get *infoback;
struct nuiformat *trip;
int	dd;

/* Note, code assumes NUI has valid syntax */
{
	strcpy((char *)infoback->nuid.nui_string, (char *)trip->nui_string);
	infoback->nuid.nui_len=trip->nui_len;
	io.ic_cmd=N_nuiget; 
	io.ic_timout = 0;
	io.ic_len=sizeof(struct nui_get);
	io.ic_dp = (char *)(infoback);

	if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) < 0 )
    {
		fprintf(stderr, "nuimap: Get IOCTL failed\n");
		exit_program(1);	
	}

	printf("\nNUI Address\tAssociated Facilities\n");

	printf("%s\t", infoback->nuid.nui_string);
	if ((int)strlen((char *)infoback->nuid.nui_string) > 15)
		printf("\n\t\t");
	else if ((int)strlen((char *)infoback->nuid.nui_string) < 8)
		printf("\t");
	print_facilities(infoback->nuifacility);

    return 0;
}

del_ioctl(infodel, testaddr, dd)
/* 	Using the NUI as key, this function will delete 
 * 	the corresponding mapping in the NUI internal table.
 */

struct nui_del *infodel;	
struct nuiformat *testaddr;
int	dd;
{
	strcpy((char *)infodel->nuid.nui_string, (char *)testaddr->nui_string);
	infodel->nuid.nui_len=strlen((char *)testaddr->nui_string);
	io.ic_cmd=N_nuidel; 
	io.ic_timout = 0;
	io.ic_len=sizeof(struct nui_del);
	io.ic_dp = (char *)(infodel);

	if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) <0 ) 
	{
		fprintf(stderr, "nuimap: Delete IOCTL failed\n");
		exit_program(1);
	}

    return 0;
}

mget_ioctl(infomback, dd)
/* 	
 * This function fetches and displays all entries in the NUI table.
 */

struct nui_mget *infomback;
int	dd;

{	
	unsigned int num, i;
	unsigned int total_ent = 0;
	unsigned int start = 0;
	NUI_ADDR *bufp;

	infomback->prim_class = ( char ) N_nuimsg;
	infomback->op = ( char ) N_nuimget;	
	
	io.ic_cmd=N_nuimget; 
	io.ic_timout = 0;
	io.ic_len = sizeof(struct nui_mget);
	io.ic_dp = (char *)(infomback);

	/* print table */
	printf("\nNUI Address\tAssociated Facilities\n");

	for (;;)
	{
		bufp = (NUI_ADDR *)infomback->buf;
	
		infomback->first_ent = start;
		infomback->num_ent = MGET_NMAXENT;
		
		if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) < 0 ) 
		{
			fprintf(stderr, "nuimap: MGET IOCTL failed\n");
			exit_program(1);
		}
		num = infomback->num_ent;
		
		for (i = 0; ((i < num) && (i < MGET_NMAXENT)); i++, bufp++)
		{
			printf("%s\t", bufp->nuid.nui_string);
			if ((int)strlen((char *)bufp->nuid.nui_string) > 15)
				printf("\n\t\t");
			else if ((int)strlen((char *)bufp->nuid.nui_string) < 8)
				printf("\t");
			print_facilities(bufp->nuifacility);
		}

		total_ent += num;

		if (num < MGET_NMAXENT)
		{
			if (total_ent == 0)
				printf("\n  <---------------No entries in table----------->\n");
			break;
		}
			
		start = infomback->last_ent;
	}

    return 0;
}
	

reset_ioctl(flushinfo, dd)
/* 	
 * 	This function deletes all entries in the NUI table.
 */

struct nui_reset *flushinfo;
int	dd;

{	
	flushinfo->prim_class = ( char ) N_nuimsg;
	flushinfo->op = ( char ) N_nuireset;	
	
	io.ic_cmd=N_nuireset; 
	io.ic_timout = 0;
	io.ic_len=sizeof(struct nui_reset);
	io.ic_dp = (char *)(flushinfo);

	if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) < 0 ) 
	{
		fprintf(stderr, "nuimap: Reset IOCTL failed\n");
		exit_program(1);
	}

    return 0;
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
#endif /* VXWORKS */

   exit ( error_val );
} /* end exit_program() */
