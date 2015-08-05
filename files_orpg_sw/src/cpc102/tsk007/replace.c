

/**************************************************************************

    Description: This module contains the routines that replaces certain 
	FORTRAN identifiers in order to solve some of the compiler 
	incompatibily problems.

**************************************************************************/
/*
 * RCS info
 * $Author: dodson $
 * $Locker:  $
 * $Date: 1998/10/17 17:20:47 $
 * $Id: replace.c,v 1.11 1998/10/17 17:20:47 dodson Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#include <stdio.h>
#include <string.h>            /* strncmp(), strncat(), strncpy()         */
#include <strings.h>           /* strncasecmp()                           */

#include "ftnpp_def.h"

#define MAXN_NAMES	64
#define MAXN_LHEX	64
#define NAME_SIZE 	32

#define MAX_LINE_LEN	4096	/* maximum length of a complete FORTRAN
				   line */


static int N_lhex = 0;		/* number of large hex constants found */
static char Lhex_name [MAXN_LHEX][NAME_SIZE];
				/* store constant names */
static char Lhex_value [MAXN_LHEX][8];
				/* store hexdecimal velue strings */

static int N_names = 0;		/* number of names to be replaced */
static char *Old_name [MAXN_NAMES];
				/* store old names */
static char *New_name [MAXN_NAMES];
				/* store new names */

static int Line_num;		/* current line number */
static char *File_name;		/* current file name */

static int Truncate = 0;	/* long names need to be truncated */

/* static functions */
static int Store_lhex (char *line);
static void Replace_token (char *line, int off, char *otok, char *ntok);
static int Process_large_hex_const (char *line);
static void Process_mem (char *cline);
static void Process_truncate (char *line);


/**************************************************************************

    Description: This function sets variable Truncate.

**************************************************************************/

void REP_need_truncate ()
{

    Truncate = 1;
    return;
}

/**************************************************************************

    Description: This function resets N_lhex when an END statement is
		encountered.

**************************************************************************/

void REP_reset_N_lhex ()
{

    N_lhex = 0;
}

/**************************************************************************

    Description: This function accepts a command line specification of 
		a name replacemant. The format is Old_name=New_name.

    Input:	argv - an argument string specifying a name replacement.

    Return:	It returns 0 on success or -1 if the argument string
		can not be parsed.

**************************************************************************/

int REP_argument (char *argv)
{
    char *pt;

    if (N_names >= MAXN_NAMES) {
	printf ("Too many replacement names\n");
	FTNPP_task_terminate (-2, 0, NULL);
    }

    Old_name[N_names] = argv;
    pt = argv;
    while (*pt != '\0' && *pt != '=')
	pt++;

    if (*pt != '=')
	return (-1);

    *pt = '\0';
    New_name[N_names] = pt + 1;

    if (strlen (Old_name[N_names]) <= 0)
	return (-1);

    N_names++;

    return (0);
}

/**************************************************************************

    Description: This function performs name replacement as set by the 
		ftnpp command line options. It then processes the overflow 
		problem when a large constant is assigned to a short integer.

    Input:	line - a line of FORTRAN code to be processed.

    Return:	It returns 0 if the code is unchanged or 1 if changed.

    Note:	In the processing, the line length may need to be increased.
		We assume that the buffer allocated for "line" is large
		enough to hold the change.

		The line is assumed either terminted at '\0' or '\n' since
		in-line comments may follow.

		This function exits if a fatal error is found.

**************************************************************************/

int REP_processing (char *line, int line_n, char *fname)
{
    static char cline [MAX_LINE_LEN] = "";	/* a complete line */
    static int endoff = 0;			/* offset of the end of cline */
    int changed, len;
    int i;
    char c;

    changed = 0;
    Line_num = line_n;
    File_name = fname;

    /* comment line */
    if ((c = *line) != ' ' && c != '\t' && (c < 48 || c >= 58))
	return (changed);

    /* form complete line and process it */
    if (c == ' ' && (c = line[5]) != ' ' && c != '\t' && 
	strncmp ("     ", line, 5) == 0) {	/* a continuing line */
	strncat (cline, line + 7, MAX_LINE_LEN - endoff);
    }
    else {					/* non-continuing line */
	Process_mem (cline);
	strncpy (cline, line, MAX_LINE_LEN);
	endoff = 0;
    }
    while ((c = cline[endoff]) != '\0') {	/* update endoff */
	if (c == ';' || c == '\n')		/* remove inline comment and */
	    cline[endoff] = '\0';		/* line return symbol */
	else {
	    endoff++;
	    if (endoff >= MAX_LINE_LEN) {	/* check boundary of cline */
		printf ("FROTRAN line too long\n");
		FTNPP_task_terminate (-1, Line_num, File_name);
	    }
	}
    }

    /* make sure the line is allways terminated by '\n' */
    len = strlen (line);
    if (line [len - 1] != '\n') {
	line [len] = '\n';
	line [len + 1] = '\0';
    }

    /* replace names */
    for (i = 0; i < N_names; i++) {
	int off, cnt;

	cnt = 0;
	while ((off = REP_match_tocken (Old_name [i], line)) >= 0) {
	    Replace_token (line, off, Old_name [i], New_name [i]);
	    changed = 1;
	    cnt++;
	    if (cnt > 10) {
		printf ("Too many token matched\n");
		FTNPP_task_terminate (-1, Line_num, File_name);
	    }
	}
    }

    /* process the large constant assign statements */
    if (Process_large_hex_const (line) != 0)
	changed = 1;

    /* process long function names */
    if (Truncate)
	Process_truncate (line);

    return (changed);
}

/**************************************************************************

    Description: This function replaces large value constants' assignment
		by their actual values. This is needed to pass SUN FORTRAN  
		compiler. The following are two examples:

      INTEGER RASFLG1
      PARAMETER(RASFLG1=X'BA07',RASFLG2=X'8000',RASFLG3=X'00C0',
      OUTPUT(NINFO)=RASFLG1	...This will fail
      	(OUTPUT(NINFO)=X'BA07' ...is fine)

      INTEGER RASFLG1,RASFLG2,RASFLG3,RAS_PACK_DES
      PARAMETER(RASFLG1=X'BA07',RASFLG2=X'8000',RASFLG3=X'00C0',
      INTEGER*2    BUFOUT(*),I2WRD(2), BUF_INIT(PACKDESC)

      DATA BUF_INIT( FLAGSOF1   ) /RASFLG1/   ...This will fail
	(DATA BUF_INIT( FLAGSOF1   ) /X'BA07'/ ...is fine)

    Input:	line - a line of FORTRAN code to be processed.

    Return:	It returns 0 if the code is unchanged or 1 if changed.

    Notes:	The above assignments cause overflow problems. A direct
		assignment from X'BA07' is fine because it is a untyped
		constant. The assignment is a bit-by-bit copy. The SUN
		compiler is more strict and catches the problem while
		the HP compiler does not. However, the latter may be
		incorrect in processing a bad assignment such as  
		OUTPUT(NINFO)=RASFLG1.

		We corrently process all parameterized constants. We may
		also need to process DATA constants. We will add later 
		when there is a need.

		This function exits if a fatal error is found.

**************************************************************************/

static int Process_large_hex_const (char *line)
{
    int changed;

    changed = 0;
    if (Store_lhex (line) == 0 && N_lhex > 0) {	/* replace large const */
	int i;

	for (i = 0; i < N_lhex; i++) {
	    int off;
	    int cnt;

	    cnt = 0;
	    while ((off = REP_match_tocken (Lhex_name [i], line)) >= 0) {
		Replace_token (line, off, Lhex_name [i], Lhex_value [i]);
		changed = 1;
		cnt++;
		if (cnt > 10) {
		    printf ("Too many token matched\n");
		    FTNPP_task_terminate (-1, Line_num, File_name);
		}
	    }
	}
    }

    return (changed);
}

/**************************************************************************

    Description: This function searches for large hexidecimal constants
		in a PARAMETER statement and stores them in the tables
		for later large constant replacement.

    Input:	line - a line of FORTRAN code to be processed.

    Return:	returns 1 if the line is a PARAMETER statement, or 0 if
		the line is not.

**************************************************************************/

static int Store_lhex (char *line)
{
    static int in_par = 0;	/* a parameter line started */
    char *pt;

    pt = line;
    while (*pt == ' ' || *pt == '\t')
	pt++;

    if (in_par == 1 &&		/* check whether it is a continuing line */
	pt - line != 5)
	in_par = 0;

    /* check whether it is a parameter statement */
    if (in_par == 0) {
	if (pt [0] != 'P' && pt [0] != 'p')
	    return (0);
	if (strncasecmp ("parameter", pt, 9) != 0)
	    return (0);
	in_par = 1;
    }

    pt--;
    while (pt[1] != '\n') {
	char *ps, *pe, cc;

	pt++;
	if (*pt != 'X')
	    continue;
	if (pt[1] != '\'')
	    continue;
	if (strlen (pt) < 7)
	    continue;
	if (pt[6] != '\'')
	    continue;

	cc = pt[2];		/* check the first char of the hex */
	if (!((cc >= 56 && cc <= 57) ||		/* 8 through 9 */
		(cc >= 65 && cc <= 70) ||	/* A through F */
		(cc >= 97 && cc <= 102)))	/* a through f */
	    continue;

	/* a hex found; search for the constant name */
	ps = pt - 1;	
        while (ps >= line && (*ps == ' ' || *ps == '\t'))
	    ps--;

	if (ps >= line && *ps == '=') {	/* found a = */
	    int len;

	    ps--;
            while (ps >= line && (*ps == ' ' || *ps == '\t'))
		ps--;

	    pe = ps;		/* end of the constatnt name */
            while (ps >= line && REP_check_legal_char (*ps))
		ps--;

	    ps++;
	    len = pe - ps + 1;	/* name length */
	    if (len > 0 && REP_check_legal_char (*ps)) {
					/* the constatnt name is found */
		int ok;

		ok = 1;
		if (N_lhex >= MAXN_LHEX) {
		    printf ("Too many large hex constants found\n");
		    FTNPP_task_terminate (-1, Line_num, File_name);
		}
		if (len >= NAME_SIZE) {
		    printf ("Constant name is too long\n");
		    FTNPP_task_terminate (-1, Line_num, File_name);
		}

		/* store the name and the hex value */
		if (ok) {
		    strncpy (Lhex_name [N_lhex], ps, len);
		    Lhex_name [N_lhex][len] = '\0';
		    strncpy (Lhex_value [N_lhex], pt, 7);
		    Lhex_value [N_lhex][7] = '\0';
		    N_lhex++;
		}
	    }
	}
    }
    return (1);
}

/**************************************************************************

    Description: This function replaces the otok at offset "off" on line
		"line" by the new token "ntok". We assume "line" is large
		enough to hold the new line. The function prints an error
		message if it fails to do the job.

    Inputs:	line - a line of FORTRAN code to be processed.
		off - offset of "otok" in string "line";
		otok - old token to be replaced;
		ntok - the new token;

**************************************************************************/

#define TMP_SIZE	512

static void Replace_token (char *line, int off, char *otok, char *ntok)
{
    char buf [TMP_SIZE];
    char *cpt;
    int llen, olen, nlen;

    llen = strlen (line);
    olen = strlen (otok);
    nlen = strlen (ntok);

    if (llen + nlen - olen >= TMP_SIZE) {
	printf ("Line is too long to process\n");
	FTNPP_task_terminate (-1, Line_num, File_name);
    }

    if (strncmp (line + off, otok, olen) != 0) {
	printf ("Error found in Replace_token\n");
	FTNPP_task_terminate (-1, Line_num, File_name);
    }
    printf ("ftnpp message: line replacement:\n");
    printf ("old: %s", line);

    cpt = line;
    strcpy (buf, line);
    strncpy (cpt, buf, off);
    cpt += off;
    strncpy (cpt, ntok, nlen);
    cpt += nlen;
    strcpy (cpt, buf + off + olen);
    printf ("new: %s", line);

    return;
}

/**************************************************************************

    Description: This function finds a token of "tok" in a line "line".
		We assume that there is at least one char in "tok".

    Input:	tok - the token to be searched for;
		line - a line of FORTRAN code to be processed.

    Return:	It returns the offset of the token on success or -1 if
		the token is not found.

**************************************************************************/

int REP_match_tocken (char *tok, char *line)
{
    char *pt, first, second, c;
    int len;

    pt = line - 1;
    first = *tok;
    second = tok [1];
    if (second == '\0')
	second = '\n';
    len = 0;
    while ((c = pt[1]) != '\n' && c != '\0') {

	pt++;
	if (*pt != first ||
	    pt[1] != second)
	    continue;
	if (len == 0)
	    len = strlen (tok);
	if (strncmp (pt, tok, len) != 0)
	    continue;

	if (pt > line &&
	    REP_check_legal_char (pt [-1]))
		continue;

	if (REP_check_legal_char (pt [len]))
	    continue;

	/* make sure it is not a char string */
	if ((pt > line && pt [-1] == '\'') || pt [len] == '\'')
	    continue;

	/* found */
	return (pt - line);
    }
    return (-1);
}

/***********************************************************************

    Description: Finds out whether a character is a legal FORTRAN name
		character.  We assume "_" and "`" are fine.

    Input:	cc - the character to be tested.

    Return:	If cc is a legal char used for FORTRAN name, this 
		function returns 1. Otherwise it returns 0.

***********************************************************************/

int REP_check_legal_char (char cc)
{

    if ((cc >= 48 && cc <= 57) ||
	(cc >= 65 && cc <= 90) ||
	(cc >= 95 && cc <= 112))
	return (1);
    else
	return (0);
}

/***********************************************************************

    Description: Detects whether the MEM array is directly invoked in
		arithmetic and assinment statement. A warning is given if
		it is used. MEM used in this way may cause a memory
		access error in the run time when the LB is allocated
		in the shared memory and LB_DIRECT is used.

    Input:	cline - A complete FORTRAN line.

***********************************************************************/

static void Process_mem (char *cline)
{
    int off, toff;
    int warning;

    /* process each MEM */
    toff = 0;			/* offset in the line to be processed */
    warning = 0;
    while ((off = REP_match_tocken ("MEM", cline + toff)) >= 0) {
	char before, after, c;
	char *pt;

	toff += (off + 3);

	/* find the charactes before and after "MEM" */
	pt = cline + toff;
	while ((c = *pt) == ' ' || c == '\t')
	    pt++;
	after = *pt;
	pt = cline + toff - 4;
	while (pt >= cline && ((c = *pt) == ' ' || c == '\t'))
	    pt--;
	if (pt >= cline)
	    before = *pt;
	else
	    before = '\0';

	if (before == '=' || before == '+' || before == '*' || before == '-' ||
	    after == '=' || after == '+' || after == '*' || after == '-')
	    warning = 1;
	else if (before == '/' || after == '/') {
				/* we need further check whether this is a
				   common or data statement */
	    pt = cline;
	    while ((c = *pt) == ' ' || c == '\t')
		pt++;
	    if (REP_match_tocken ("COMMON", pt) != 0 &&
		REP_match_tocken ("common", pt) != 0 &&
		REP_match_tocken ("DATA", pt) != 0 &&
		REP_match_tocken ("data", pt) != 0)
		warning = 1;
	}
    }

    if (warning) {
	printf ("ftnpp warning: MEM used in arithmetic or assignment statement:\n");
	printf ("%s\n", cline);
	printf ("  - near line %d, file %s\n", Line_num, File_name);
    }

    return;
}

/***********************************************************************

    Description: Trancates long function names. Identifier characters
		after "__" are replaced by spaces.

    Input:	line - A source code line.

***********************************************************************/

static void Process_truncate (char *line)
{
    char *pt;
    char c;

    pt = line;
    while ((c = *pt) != '\0') {
	if (c == '_' && pt[1] == '_') {
	    pt += 2;
	    while (REP_check_legal_char (*pt)) {
		*pt = ' ';
		pt++;
	    }
	}
	else
	    pt++;
    }
    return;
}
