static char sccsid[] = "@(#)x25file.c	1.1	07 Jul 1998";

/******************************************************************
 *
 *  SpiderX25 - Configuration utility
 *
 *  Copyright 1991  Spider Systems Limited
 *
 *  X25_FILE.C
 *
 *  X25 Control Interface: Read Configuration File
 *
 ******************************************************************/

/*
 * /redknee5/projects/tcp/PBRAIN/SCCS/pbrainF/rel/src/clib/x25util/0/s.x25file.c
 *
 *    Last delta created    17:24:50 3/5/92
 *    This file extracted    11:36:56 3/10/92
 *
 */

/*
Modification history:
 
Chg Date	Init  Description
1.  29-OCT-97   mpb   Compile on Windows NT.
2.  12-NOV-97   mpb   Need to have NT directory type specifications.
3.  13-NOV-97   mpb   Have to get the environment variable for the system
                      root before figuring out where the netconf file is
                      (for win nt).
4.  05-MAR-98   mpb   Need to clean TCP/IP dll up at exit (for NT).
5.  19-may-98   rjp   Merge with Spider release 8.1.1.
6.  09-SEP-99   tdg   Added VxWorks support.

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uint.h>
#ifdef LINUX
#include <errno.h>
#include <ctype.h>
#endif

#ifdef WINNT
#include <winsock.h>
#endif /* WINNT */

int   conf_exerr ( );
int   experr ( );
int   exierr ( );
uint8 get_conf_unchar (  );

#ifndef VXWORKS
#ifdef    ANSI_C
void  exit_program ( int );
#else
void  exit_program ( );
#endif /* ANSI_C */
#endif /* !VXWORKS */

extern char *prog_name;

/* location of configuration files */
#ifndef TEMPLATE
#ifndef WINNT
#define TEMPLATE "/usr/lib/snet/template/"
#else /* #2 */
#define TEMPLATE "\\System32\\uconx\\lib\\snet\\template\\"
#endif /* WINNT */
#endif

/* value for access call */
#define    ACCREAD    04


#define BUFSZ   256

static char *cname;
static int cline;
static FILE *cf;
static char inbuf[BUFSZ];

/* Open file for reading */
open_conf(file)
char *file;
{
    if (file)
    {
        if (file[0] == '/')
            cname = file;
        else
        {
            static char fullname [ 256 ];

            /*
             *  Check if given configuration file exists in
             *  template directory then fail.
             */

/* #3 */
#ifdef WINNT
	    strcpy ( fullname, getenv ( "SystemRoot" ) );
            strcat ( fullname, TEMPLATE );
#else
            strcpy ( fullname, TEMPLATE );
#endif /* WINNT */

            strcat ( fullname, file );
            cname = fullname;
        }

        if (!(cf = fopen(cname, "r")))
            experr("file %s failed to open", cname);
    }
    else
        cf = stdin;

    cline = 0;
    return 0;
}
    
/* Close conf file */
close_conf()
{
    if (cname)
    {
        do
        {
            cline++;
            if (!fgets(inbuf, BUFSZ, cf))
            {
                fclose(cf);
                return 0;
            }
        }
        while (inbuf[0] == '#' || inbuf[0] == '\n');
        conf_exerr("too many lines");
    }
    return 0;    			/* Appease compiler. 		*/
}

/* Return next non-empty line */
char *get_conf_line()
{
    do
    {
	char *cp;			/* #5				*/
        cline++;
        if (!fgets(inbuf, BUFSZ, cf))
            conf_exerr("not enough lines");
        if (!strchr(inbuf, '\n'))
            conf_exerr("line too long");
        *strpbrk(inbuf, "#\n") = '\0';
	/*
	* #5 The 'remove trailing spaces' is from Spider release 8.1.1
	*/
        /* remove trailing spaces */
        cp = inbuf + strlen(inbuf) - 1; /* point at last char */
        while (cp >= inbuf && isspace(*cp))
            cp--;
        cp[1] = '\0';
    }
    while (inbuf[0] == '\0');
    return inbuf;
}

/* Return unsigned short value */
uint16
get_conf_ushort()
{
    register uint16 uval;
    long value;
    char *line;

    char *s;

    /* Read value */
    line = get_conf_line();
    uval = (uint16)(value = strtol(line, &s, 10));
    if (*s) conf_exerr("syntax error");
    if ((long)uval != value) conf_exerr("unsigned short out of range");

    return uval;
}

/*
* #5 'get_conf_uint' is from Spider release 8.1.1.
*/
/* Return unsigned int value */
unsigned get_conf_uint(void)
{
        register unsigned short uval;
        long value;
        char *line;
 
        char *s;
 
        /* Read value */
        line = get_conf_line();
        uval = (unsigned) (value = strtol(line, &s, 10));
        if (*s) conf_exerr("syntax error");
        if ((long)uval != value) conf_exerr("unsigned int out of range");
 
        return uval;
}

/*
* #5 'get_conf_ulong' is from Spider release 8.1.1.
*/
/* Return unsigned long value */
unsigned long get_conf_ulong(void)
{
        register unsigned long uval;
        long value;
        char *line;
 
        char *s;
 
        /* Read value */
        line = get_conf_line();
        uval = (unsigned long)(value = strtol(line, &s, 10));
        if (*s) conf_exerr("syntax error");
        if ((long)uval != value) conf_exerr("unsigned long out of range");
 
        return uval;
}

    
int
hex_val(c)
char c;
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;    /* Not hex! */
}


/* Return short value in up to 'n' hex digits */
uint16
get_conf_hex(n)
{
    uint16 uval = 0;
    register char *p = get_conf_line();

    while (*p && n--)
    {
        register int digit;

        if ((digit = hex_val(*p++)) < 0)
            conf_exerr("not a hex digit");
        uval = uval << 4 | digit;
    }

    if (*p) conf_exerr("too many hex digits");

    return uval;
}
    
/* Return unsigned char value */
uint8
get_conf_unchar()
{
    register uint16 uval = get_conf_ushort();
    if (uval > 255) conf_exerr("out of range, exceeds 255");

    return (uint8)uval;
}
    

/* Report errors */
conf_exerr(text)
char *text;
{
    fprintf(stderr, "%s: %s at line %d of ", prog_name, text, cline);
    if (cname)
    {
        fprintf(stderr, "file %s\n", cname);
        fclose(cf);
    }
    else
        fprintf(stderr, "input\n");
    exit_program(2);

   return 0;
}


exerr(format, arg)
char *format, *arg;
{
    fprintf(stderr, "%s: ", prog_name);
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
    exit_program(2);

   return 0;
}

/*
* #5 'exierr' is from Spider release 8.1.1.
*/
int exierr(char *format, int arg)
{
    fprintf(stderr, "%s: ", prog_name);
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
    exit_program(2);

    return 0;  /* appease compiler */
}


experr(format, device)
char *format, *device;
{
    extern int errno;

    fprintf(stderr, "%s: ", prog_name);
    fprintf(stderr, format, device);
    fprintf(stderr, " (%s)\n", strerror(errno));
    exit_program(2);

   return 0;
}

#ifndef LINUX
char *basename(path)
char    *path;
{
    char    *base = path;

    while (*base)
        if (*base++ == '/')
            path = base;
    
    return path;
}
#endif

#ifndef VXWORKS
/* #4 */
/**********

exit_program() ---

Instead of just exiting, clean up system (windows stuff) before exit.

**********/
#ifdef    ANSI_C
void exit_program ( int error_val )
#else
void exit_program ( error_val )
int    error_val;
#endif    /* ANSI_C */
{
#ifdef    WINNT
    LPSTR   lpMsg;
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
#endif /* !VXWORKS */
