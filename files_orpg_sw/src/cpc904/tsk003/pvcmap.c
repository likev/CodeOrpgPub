/*   @(#) pvcmap.c 99/11/02 Version 1.6   */

/******************************************************************
 *
 *  SpiderX25 - PVC spit-edit utility
 *
 *  Copyright 1991  Spider Systems Limited
 *
 *  PVCMAP.C
 *
 *  Main program and common services
 *
 ******************************************************************/

/*
    Modification history:
 
Chg Date       Init Description
1.  12-NOV-97  mpb  Need to have NT type file specification.
2.  20-APR-98  mpb  Compile on QNX (TCP/IP).
3.  11-JUN-98  mpb  Fully port to Windows NT.
4.  11-JUN-98  mpb  Push swap module if needed (little endian architecture).
5.  11-JUN-98  mpb  Need to get the system variable %SystemRoot% from getenv().
6.  14-OCT-98  lmm  Cleaned up usage strings, added support for controller #
7.  08-OCT-99  tdg   Added VxWorks supported
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
#include "ll_proto.h"
#include "x25_proto.h"
#include "x25_control.h"

#ifdef WINNT
#define    DEFAULT_SERVER     "\\\\.\\PTIXpqcc"
#else
#define    DEFAULT_SERVER     "/dev/ucsclone"
#endif /* WINNT */

/* File Mode for Read or Write */
#define O_RW    2    
#define MAXLENNAME 40
#define MAXNUMPARM 30
#define TRUE    1
#define FALSE     0
#define MAXLINE 80

/* Command line handling. */
#define VALIDOPTIONS "?PGf:s:v:c:"

#define USAGE1 "usage: %s [-G] [-s server] [-v service] [-c controller]\n"
#define USAGE2 "       %s -P [-f file] [-s server] [-v service] [-c controller]\n"

#ifndef WINNT
#define DEFFILE "/etc/pvcmapconf"
#define DEFDEV  "/dev/x25"       /* This is not used (bit rot). */
#else  /* #1 */
#define DEFFILE "\\System32\\uconx\\etc\\pvcmapconf"
#endif /* !WINNT */

static struct xstrioctl io;        /* stream ioctl structure */
static struct pvcconff spbit;      /* structure passed to put_ioctl */
static struct pvcmapf  infoget;    /* received in get ioctl */

extern int errno;
extern char *optarg;
extern int optind;

extern  unsigned long snidtox25();
extern  int x25tosnid();

static int  get_ent ( );
static int  get_ioctl ( );
static int  comment ( );
static int  put_ioctl ( );
static int  log2 ( );


#ifdef    ANSI_C
static void exit_program ( int );
#else
static void exit_program ( );
static int error_val;
#endif    /* ANSI_C */

/************************************************************************/
#ifdef VXWORKS
int pvcmap (argc, argv, ptg)
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
    int dd;            /*file descriptor for device */
    int put_reqd = 0;
    int get_reqd = 0;
    int no_file = 1;
    int ents_added = 0;
    int c;
    FILE *fopen(), *fp;
    OpenRequest oreq;                            /* MPS open structure */

#ifdef    WINNT
    LPSTR    lpMsg;
    WSADATA  WSAData;
    int      optionValue = SO_SYNCHRONOUS_NONALERT;
#endif    /* WINNT */

#ifdef    WINNT
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
#endif    /* WINNT */

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
        
        /* File Specified */
        case 'f':

            if ((fp = fopen(optarg, "r")) == NULL)
            {
                perror("Failed to open specified file");
                exit_program(1);
            }
            no_file = 0;
#ifdef DEBUG
            fprintf(stderr, "Opened file '%s'\n", optarg);
#endif
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

        /* Putioctl option */    
        case 'P':

            put_reqd = 1;

        break;

        /* Getioctl option */            
        case 'G':

            get_reqd = 1;

        break;


        /* ERROR */    
        case '?' :
            fprintf(stderr, USAGE1, argv[0]);
            fprintf(stderr, USAGE2, argv[0]);
            exit_program (1);
        }
    }

    /*
        If no arguments supplied then default to get request
    */
    if (put_reqd == 0 && get_reqd == 0)
        get_reqd = 1;            

    /*
        Check no multiple options
    */
    if ( put_reqd && get_reqd )
    {
        fprintf(stderr, "pvcmap: only one of -P or -G may be specified\n");
        fprintf(stderr, USAGE1, argv[0]);
        fprintf(stderr, USAGE2, argv[0]);
        exit_program (1);
    }

    /*
        Put option take a file
    */    
    if (no_file && put_reqd)
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
            fprintf ( stderr, "Failed to open %s\n", DEFFILE );
            perror("");
            exit_program(1);
        }
#ifdef DEBUG        
        fprintf(stderr, "Opened default file '%s'\n", DEFFILE); 
#endif
    }
    
    oreq.dev        = 0;            /* default X.25 device to zero */
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
    

    eof=0;        /* not eof */

    /*
        G option specified.
    */
    if (get_reqd)
    {
        get_ioctl(&infoget,dd);
    }

    /*
        P option must process tokens in a file
    */
    if (put_reqd)
    while(eof!=EOF)
    {
        ungetc((c=fgetc(fp)),fp);    /* get 1st char on line     */
        eof = c;                        /* in case of eof           */
            
        if (!comment(c,fp))
        {
            eof=get_ent(&spbit,fp); /* get parameters  */

            if (eof > 0)        /* good entry, send it away */
            {
                if (put_ioctl(&spbit,dd) < 0)
                {
                    fprintf(stderr,    
                       "pvcmap: Put IOCTL failed after adding %d entries\n",
                        ents_added);
                    exit_program (1);
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

    while (c==10)    /* a LF */
    {
        c=fgetc(fp);    /*get rid of it*/
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
    Return values:    negative = bad token          
            EOF = no more entries left
            1 = correctly processed entry
*/

struct pvcconff *sb;
FILE *fp;

{
    static int entry = 0;
    char *lp,*rp,*lw,*rw;
    char *s;
    char *sn,*lcn; 
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
        Read in subnetwork identifier
    */
    if ((sn = strtok(line," \t\n")) != NULL)
    {    
        if ( !(sb->sn_id = snidtox25(sn)))
        {
            fprintf(stderr, "Entry %d: %s is an invalid subnetwork id\n",entry, sn);
            return(-2);
        }
    }

    /*
        Logical channel identifier
    */
    if ((lcn = strtok(NULL," \t\n")) != NULL)
    {
        sb->lci = atoi(lcn);
    }

    /*
        Local packet size
    */
    if ((lp = strtok(NULL," \t\n")) != NULL)
    {
        sb->locpacket = log2(atoi(lp));
    }

    /*
        Remote packet size
    */
    if ((rp = strtok(NULL," \t\n")) != NULL)
    {
        sb->rempacket = log2(atoi(rp));
    }

    /*
        Local window size
    */
    if ((lw = strtok(NULL," \t\n")) != NULL)
    {
        sb->locwsize = atoi(lw);
    }

    /*
        Remote window size
    */
    if ((rw = strtok(NULL," \t\n")) != NULL)
    {
        sb->remwsize = atoi(rw);
    }
    else
    {
        fprintf(stderr, "Entry %d: Premature end of input\n",entry);
        return(-2);
    }
    

#ifdef DEBUG
    printf("sn=%s, lci=%s, lp=%s, rp=%s, lw=%s, rw=%s\n",
                      sn,lcn,lp,rp,lw,rw);    
#endif

    return(1);    /* Positive value indicates good entry */
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

struct pvcconff *sb;
int dd;

{    

    io.ic_cmd=N_putpvcmap;
    io.ic_timout = 0;
    io.ic_len=sizeof(struct pvcconff);
    io.ic_dp = (char *)(sb); 

    if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) < 0 )
    {
        return -1;
    }

    return 0;
}


/************************************************************************/
get_ioctl(infoback, dd)
/************************************************************************/

/*     
    This function fetches and displays all pvc entries in the
    x25 vc database.
*/

struct pvcmapf *infoback;
int    dd;

{    
    unsigned int num, i, j, dumdec1, dumdec2;
    unsigned int total_ent = 0;
    struct pvcconff *bufp;

    io.ic_cmd=N_getpvcmap; 
    io.ic_timout = 0;
    io.ic_len = sizeof(struct pvcmapf);
    io.ic_dp = (char *)(infoback);

    /*
        Start at first entry, This field will be updated
        by X.25 module as it performs the search.
    */
    infoback->first_ent = 0;    
    
    /*
        Print table
    */
    printf("\nSubnet\tLCI\tLocpkt\tRempkt\tLwin\tRwin\n");

    for (;;)
    {
        bufp = (struct pvcconff *)infoback->entries;
    
        infoback->num_ent = 0;
        
        if ( MPSioctl ( dd, X_STR, ( caddr_t ) &io ) < 0 ) 
        {
            fprintf(stderr, "pvcmap: GET IOCTL failed\n");
            exit_program(1);
        }
        num = infoback->num_ent;
        
        for (i = 0 ; i < num ; i++, bufp++)
        {
            unsigned char sn_id[5];

            x25tosnid (bufp->sn_id, sn_id);
            printf("  %s\t%03x\t",sn_id,bufp->lci);

            /*
                Packet sizes are in log form so convert
            */
             dumdec2=1;
            dumdec1=(bufp->locpacket);
            for (j = 1; j <=dumdec1 ; ++j)
                dumdec2=2*dumdec2;
            printf(" %d\t",dumdec2);

            dumdec2=1;
            dumdec1=(bufp->rempacket);
            for (j = 1; j <=dumdec1 ; ++j)
                dumdec2=2*dumdec2;
            printf("% d\t",dumdec2);
 
            printf(" %d\t",bufp->locwsize);
            printf(" %d\t",bufp->remwsize);

            printf("\n");
        }

        total_ent += num;

        if (num < MAX_PVC_ENTS)
        {
            if (total_ent == 0)
                printf("\n <--------- No entries in table ---------->\n");
            break;
        }
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
#endif    /* ANSI_C */
{
#ifdef    WINNT
   LPSTR lpMsg;
#endif    /* WINNT */

#ifdef    WINNT
   if ( WSACleanup ( ) == SOCKET_ERROR )
   {
      FormatMessage ( 
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
         NULL, WSAGetLastError ( ), LANG_USER_DEFAULT, 
         ( LPSTR ) &lpMsg, 0, NULL );

      fprintf ( stderr, "\n\nWSACleanup() failed:\n%s\n", lpMsg );

      LocalFree ( lpMsg );
   }
#endif    /* WINNT */

#ifdef VXWORKS
   cleanup_mps_thread ( );
#endif /* VXWORKS */

   exit ( error_val );
} /* end exit_program() */

