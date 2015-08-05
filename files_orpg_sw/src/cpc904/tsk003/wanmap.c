/*   @(#) wanmap.c 99/11/02 Version 1.5   */

/******************************************************************
 *
 *  SpiderX25 - WAN spit-edit utility
 *
 *  Copyright 1991  Spider Systems Limited
 *
 *  WANMAP.C
 *
 *  Main program and common services
 *
 ******************************************************************/

/*
	Modification history:
 
Chg	Date       Init Description
1.  11-JUN-98  mpb  Fully port to Windows NT.
2.  11-JUN-98  mpb  Push swap module if needed (little endian architecture).
3.  11-JUN-98  mpb  Need to get the system variable %SystemRoot% from
                    getenv().
4.  11-JUN-98  mpb  Have special logic for explicit Windows NT input file
                    names.
5.  14-OCT-98  lmm  Cleaned up usage strings, added support for controller #
6.  08-OCT-99  tdg  Added VxWorks supported
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
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
#include "wan_proto.h"
#include "wan_control.h" 

#ifdef WINNT
#define	DEFAULT_SERVER 	"\\\\.\\PTIXpqcc"
#else
#define	DEFAULT_SERVER 	"/dev/ucsclone"
#endif /* WINNT */

/* File Mode for Read or Write */
#define O_RW	2	
#define MAXLENNAME 40
#define MAXNUMPARM 30
#define TRUE	1
#define FALSE 	0
#define MAXLINE 80

/* Command line handling. */
#define VALIDOPTIONS "?PGZn:s:v:c:"
#define USAGE1 "usage: %s  -P  -n subnet_id [-s server] [-v service] [-c ctlr] filename\n"
#define USAGE2 "       %s  -Z  -n subnet_id [-s server] [-v service] [-c ctlr]\n"
#define USAGE3 "       %s [-G] -n subnet_id [-s server] [-v service] [-c ctlr]\n"

#define MAXRMASIZE   20       /* Max size for remote address    */
#define MAXIFASIZE   30       /* Max size for interface address */


/* #5 */
#ifndef WINNT
#define MAPDIR "/usr/lib/snet/interfacemap/"
#else
#define MAPDIR "\\System32\\uconx\\lib\\snet\\interfacemap\\"
#endif /* !WINNT */

static struct xstrioctl io;		/* stream ioctl structure */
static struct wanmappf  spbit;		/* structure passed to put_ioctl */
static struct wanmapgf  infoget;	/* received in get ioctl */
static struct wanmapdf  infodel;	/* sent in del ioctl */

static char	*subnetid;

extern FILE *open_map();

extern int errno;
extern char *optarg;
extern int optind;

extern  unsigned long snidtox25();

static int  comment ( );
static int  get_ent ( );
static int  put_ioctl ( );
static int  get_ioctl ( );
static int  del_ioctl ( );
static int  log2 ( );

#ifdef    ANSI_C
static void exit_program ( int );
#else
static void exit_program ( );
static int error_val;
#endif	/* ANSI_C */

/************************************************************************/
#ifdef VXWORKS
int wanmap (argc, argv, ptg)
#else
int main(argc,argv)
#endif /* VXWORKS */
/************************************************************************/

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
	int no_file = 1;
	int ents_added = 0;
	int c;
	FILE *fopen(), *fp;
	OpenRequest oreq;                            /* MPS open structure */

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

	/*
	    Get text file name and open
	*/
	while ((c = getopt (argc, argv, VALIDOPTIONS)) != EOF)
	{
		switch (c)
		{
		
		case 's':

			strcpy(oreq.serverName, optarg);

		break;

		case 'v':

			strcpy(oreq.serviceName, optarg);

		break;

		case 'c':

		    oreq.ctlrNumber = strtol ( optarg, 0, 10 );

		break;

		/* Subnetwork ID  specified */
		case 'n' :
		
			subnetid = optarg;
			
		break;


		/* Putioctl option */	
		case 'P':

			put_reqd = 1;

		break;

		/* Getioctl option */			
		case 'G':

			get_reqd = 1;

		break;

		/* Delioctl option (delete all entries) */			
		case 'Z':

			del_reqd = 1;

		break;

		/* ERROR */	
		case '?' :
		
			fprintf(stderr, USAGE1, argv[0]);
			fprintf(stderr, USAGE2, argv[0]);
			fprintf(stderr, USAGE3, argv[0]);
			exit (1);
		}
	}

	/*
	    If no arguments supplied then default to get request
	*/
	if (put_reqd == 0 && get_reqd == 0 && del_reqd == 0)
		get_reqd = 1;			

	/*
	    Check no multiple options
	*/
	if ( (put_reqd && get_reqd) || (put_reqd && del_reqd) ||
	                               (get_reqd && del_reqd)   )
	{
		fprintf(stderr, USAGE1, argv[0]);
		fprintf(stderr, USAGE2, argv[0]);
		fprintf(stderr, USAGE3, argv[0]);
		fprintf(stderr, "wanmap: only one of -P, -G or -Z may be specified\n");
		exit (1);
	}

	/*
	    Check for subnetwork identifier
	*/
	if (!subnetid)
	{
	    fprintf(stderr,
		    "no subnetwork identifier given\n");
	    exit(1);
	}
	
	/*
	    Put option take a file
	*/	
	if (put_reqd)
	{
		if (argv[optind] == NULL)
		{
			fprintf(stderr,"No mapping file specified\n");
			exit(1);
		}
		else if ((fp = open_map(argv[optind])) == NULL)
		{
			exit(1);
		}
#ifdef DEBUG		
		fprintf(stderr, "Opened file '%s'\n", argv[optind]); 
#endif
	}

	oreq.dev        = 0;			/* default wan device to zero */
	oreq.port       = 0;
	oreq.flags      = CLONEOPEN;

	strcpy(oreq.protoName, "wan" );
	
	if ((dd = MPSopen(&oreq))<0)
	{
		printf("Failed to open server %s service %s",
			oreq.serverName, oreq.serviceName);
		perror("wanmap");
		exit(1);
	}

    /* #2 */
    if ( ntohl(0x1) != 0x1 )
    {
       printf("Little Endian -- pushing swap module.\n" );

       if ( MPSioctl ( dd, X_PUSH, "wanswap" ) == MPS_ERROR )
       {
          MPSperror ( "MPSioctl swap X_PUSH" );
          exit_program ( MPS_ERROR );
       }
    }

	eof=0;		/* not eof */

	/*
	    G option specified.
	*/
	if (get_reqd)
	{
		get_ioctl(&infoget,dd);
	}

	/*
	    Z option specified.
	*/
	if (del_reqd)
	{
		del_ioctl(&infodel,dd);
	}

	/*
	    P option must process tokens in a file
	*/
	if (put_reqd)

	while(eof!=EOF)
	{
		ungetc((c=fgetc(fp)),fp);	/* get 1st char on line     */
		eof = c;                        /* in case of eof           */
			
		if (!comment(c,fp))
		{
			eof=get_ent(&spbit,fp); /* get parameters  */

			if (eof > 0)		/* good entry, send it away */
			{
				if (put_ioctl(&spbit,dd) < 0)
				{
				    fprintf(stderr,	
					   "wanmap: Put IOCTL failed after adding %d entries\n",
					    ents_added);
				    exit (1);
				}
				else
				    ents_added++;
			}
#ifdef DEBUG
			else
				printf("\n");
#endif
			/*
			    N.B. for a bad entry, skip and carry on (eof =0)
			    but terminate loop when no more entries (eof=EOF)
			*/
		}
	}

	/*
	    Close device
	*/
	MPSclose(dd);

	/*
	    Close file (only used for P option)
	*/
	if (put_reqd)
		fclose(fp);

#ifdef VXWORKS
    exit_program ( 0 );
#else
    return (0);
#endif /* VXWORKS */
}

/************************************************************************/
comment(in_c,fp)
/************************************************************************/

/*
    Checks if c is a comment starter and skips that
    line if it is. Also skips processing if EOF
*/

FILE *fp;
char in_c;

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

/************************************************************************/
get_ent(sb,fp)
/************************************************************************/

/*
    Return values:	negative = bad token  		
			EOF = no more entries left
			1 = correctly processed entry
*/

struct wanmappf *sb;
FILE *fp;

{
	static int entry = 0;
	char rl,il;
	char *s,*ra,*ia; 
	char line[MAXLINE];

#ifdef DEBUG
	printf("****************************************************\n");
	printf("\nActual table entry being processed:");
#endif

	/*
	    Get a line or return EOF
	*/
	if (fgets(line, MAXLINE, fp) == NULL)
		return(EOF);

	entry++;

	/*
	    Remove any newlines left by fgets()
	*/
	if (*(s=line + strlen(line)-1) == '\n')
	{
		*s = '\0';
	}	

	/*
	    Read in remote address.  ASCII string length in bytes.
	*/

	if ((ra = strtok(line," \t\n")) != NULL)
	{	
	    rl = sb->wan_ent.remsize = strlen(ra);

	    if ( rl <= MAXRMASIZE )
		memcpy(sb->wan_ent.remaddr,ra,sb->wan_ent.remsize);
	    else
	    {
		fprintf(stderr, "Entry %d: Remote Address Too Long\n",entry);
		return(-2);
	    }
	}
	
	/*
	    Read in interface address. ASCII string length in bytes.
	*/
	if ((ia = strtok(NULL," \t\n")) != NULL)
	{	
	    il = sb->wan_ent.infsize = strlen(ia);

	    if ( il <= MAXIFASIZE )
		memcpy(sb->wan_ent.infaddr,ia,sb->wan_ent.infsize);
	    else
	    {
		fprintf(stderr, "Entry %d: Interface Address Too Long\n",entry);
		return(-2);
	    }
	}
	else
	{
	    fprintf(stderr, "Entry %d: Premature end of input\n",entry);
	    return(-2);
	}

	/*
	    Fill subnetwork that this entry is for
	*/
	sb->w_snid = snidtox25(subnetid);
	

#ifdef DEBUG
	printf("rl=%d, ra=%s, il=%d, ia=%s\n",
					  rl,ra,il,ia);	
#endif

	return(1);	/* Positive value indicates good entry */
}

/************************************************************************/
log2(num)
/************************************************************************/

/*
*/

int num;

{
	int l;
	for(l=0;num>1;num /=2,l++);
	return(l);
}

/************************************************************************/
put_ioctl(sb,dd)
/************************************************************************/

/*
*/

struct wanmappf *sb;
int dd;

{	

	io.ic_cmd=W_PUTWANMAP;
	io.ic_timout = 0;
	io.ic_len=sizeof(struct wanmappf);
	io.ic_dp = (char *)(sb); 

	if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) <0 )
    {
		return -1;
    }

	return (0);
}


/************************************************************************/
get_ioctl(infoback, dd)
/************************************************************************/

/* 	
    This function fetches and displays all address entries in the
    WAN address table.
*/

struct wanmapgf *infoback;
int	dd;

{	
	unsigned int num, i;
	unsigned int total_ent = 0;
	wanmap_t *bufp;

	io.ic_cmd=W_GETWANMAP;
	io.ic_timout = 0;
	io.ic_len = sizeof(struct wanmapgf);
	io.ic_dp = (char *)(infoback);

	/*
	    Start at first entry, This field will be updated
	    by WAN module as it performs the search. Fill in
	    subnetworkid that table is required from.
	*/
	infoback->wan_ents.first_ent = 0;
	infoback->w_snid   = snidtox25(subnetid);
	
	/*
	    Print table
	*/
	printf("\nRem Address       \tInterface Address\n\n");

	for (;;)
	{
		bufp = (wanmap_t *)infoback->wan_ents.entries;
	
		infoback->wan_ents.num_ent = 0;
		
		if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) < 0 ) 
		{
			fprintf(stderr, "wanmap: GET IOCTL failed\n");
			exit(1);
		}
		num = infoback->wan_ents.num_ent;
		
		for (i = 0 ; i < num ; i++, bufp++)
		{
		    /* Print out Remote Address */
		    printf("%-17s",bufp->remaddr);

		    putchar('\t');

		    /* Print out Interface Address */
		    printf("%s\n",bufp->infaddr);
		}

		total_ent += num;

		if (num < MAX_WAN_ENTS)
		{
		    if (total_ent == 0)
			printf("<--------- No entries in table --------->\n");
		    break;
		}
	}

    return 0;
}
	

/************************************************************************/
del_ioctl(delmap,dd)
/************************************************************************/

/* 	
    This function deletes all address entries in the
    WAN address table.
*/

struct wanmapdf *delmap;
int	dd;

{
    io.ic_cmd=W_DELWANMAP;
    io.ic_timout = 0;
    io.ic_len = sizeof(struct wanmapdf);
    io.ic_dp = (char *)(delmap);

    /*
	Fill in subnetworkid of table to be deleted
    */
    delmap->w_snid   = subnetid[0];
		
    if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) <0 )
    {
        fprintf(stderr, "wanmap: DEL IOCTL failed\n");
        exit(1);
    }

    return 0;
}


/************************************************************************/
FILE *open_map(file)
/************************************************************************/

/*
    Open file for reading. The default mapping directory will
    be assumed if no directory given.
*/

char *file;

{
    char *cname;
    FILE *cf;

    cf = NULL;
    
/* #4 */
#ifdef WINNT
    /* For Windows NT, the explicit path name will either be
       "drive letter", ':', back-slash, path  -> like "c:\path",
       or back-slash, path  -> lie "\path".  So if the first char-
       acter is a '\', or the second is a ':', then we have an 
       explicit file name. */
    if ( ( file[0] == '\\') ||
         ( ( strlen ( file ) > 1 ) && file[1] == ':' ) )
    {
        cname = file;
    }
#else
    if (file[0] == '/')
    {
    	/* A directory pathname is included. */
	    cname = file;
    }
#endif /* WINNT */
    else
    {
    	static char fullname[100];

	    /*
	        Assume mapping file to be in the interfacemap
	        directory.
	    */

#ifdef WINNT
        strcpy ( fullname, getenv ( "SystemRoot" ) );
		strcat ( fullname, MAPDIR );
#else
		strcpy ( fullname, MAPDIR );
#endif /* WINNT */

	    strcat(fullname, file);
	    cname = fullname;
    }
    
    if ( (cf = fopen(cname, "r")) == (FILE *)NULL ) 
    {
	    fprintf(stderr, "Failed to open file %s\n",cname);
    }

    return(cf);
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
