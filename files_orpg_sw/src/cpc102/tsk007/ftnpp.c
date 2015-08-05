
/******************************************************************

	file: ftnpp.c

	The preprocessor for the CONCURRENT NEXRAD FORTRAN source 
	code files (.ftn).
	
******************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/10/26 21:27:38 $
 * $Id: ftnpp.c,v 1.17 2010/10/26 21:27:38 jing Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "ftnpp_def.h"

#define BUF_SIZE	1024
#define EXTRA_SPACE	128
#define FORTRAN_LINE_LEN 70
#define MAXN_PATH	64
#define MAXN_IFD_STR 32

static char X_replacement = 'C';

static char Prog_name [NAME_LEN];	/* name of this program */

static char *Ifdef_string[MAXN_IFD_STR];
					/* strings for IFDEF test */
static int N_ifdef_strings = 0;		/* number of strings for IFDEF test */

static char *Inc_path [MAXN_PATH];	/* paths for include files */
static char *Src_path [MAXN_PATH];	/* alternative paths for .ftn files */
static int Inc_path_cnt = 0;		/* number of include paths */
static int Src_path_cnt = 0;		/* number of source paths */

static char Out_name [NAME_LEN] = "";	/* current output file name */

static int Level = 0;			/* current number of include levels */


static int Read_options (int argc, char **argv);
static int Process_a_section (char *fname, char *st_pat, char *end_pat, 
	FILE *outfl);
static void Process_a_line (char *buf, int line_num, char *fname);
static void Process_long_line (char *buf, int line_num, char *fname);
static void Process_source_file (char *inp_name);

static void Process_include (char *buf, char *inc_name, FILE *ofl,
			int line_num, char *fname);

/******************************************************************

    Description: The main function of ftnpp.

******************************************************************/

int main (int argc, char **argv) {
    char inp_name [NAME_LEN];
    int i;

    strncpy (Prog_name, argv [0], NAME_LEN);
    Prog_name [NAME_LEN - 1] = 0;

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (-1);

    /* find file names and process each of them */
    for (i = 0; i < argc; i++) {
	char *suffix;

	strncpy (inp_name, argv [i], NAME_LEN);
	inp_name [NAME_LEN - 1] = '\0';
	suffix = inp_name + (strlen (inp_name) - 4);
	Level = 0;
	if (suffix > inp_name && strncmp (suffix, ".ftn", 4) == 0)
	    Process_source_file (inp_name);
    }
    exit (0);
}

/*****************************************************************

    Description: This function is called when a error is encountered.
		it prints out an error message, removes the output 
		file and exits.

    Input:	status - exit status;
		line - the current line number;
		fname - the current file name.

*****************************************************************/

void FTNPP_task_terminate (int status, int line, char *fname)
{

    if (fname != NULL)
	printf ("        - %s failed at line %d of file %s\n", 
			Prog_name, line, fname);
    else
	printf ("        - %s failed\n", Prog_name);

    unlink (Out_name);
    exit (-1);
}

/*****************************************************************

    Description: This function processes input file "inp_name".

    Input:	inp_name - input file name;

*****************************************************************/

static void Process_source_file (char *inp_name)
{
    char full_name [NAME_LEN];
    FILE *ofl, *ifl;
    int i;

    /* the output file name */
    strcpy (Out_name, inp_name);
    Out_name [strlen (Out_name) - 2] = '\0';	/* use .f suffix */

    /* find the input file */
    for (i = 0; i < Src_path_cnt; i++) {
	int slen;

	slen = strlen (Src_path [i]);
	if (slen + strlen (inp_name) + 1 >= NAME_LEN) {
	    printf ("Path string too long\n");
	    FTNPP_task_terminate (-1, 0, NULL);
	}
	strcpy (full_name, Src_path [i]);
	if (slen > 0 && full_name [slen - 1] != '/')
	    strcat (full_name, "/");
	strcat (full_name, inp_name);

	ifl = fopen (full_name, "r");
	if (ifl != NULL) {
	    fclose (ifl);
	    break;
	}
    }

    /* open output file */
    ofl = fopen (Out_name, "w");
    if (ofl == NULL) {
	printf ("Failed in creating output file %s\n", Out_name);
	FTNPP_task_terminate (-1, 0, NULL);
    }

    /* process file full_name */
    if (Process_a_section (full_name, "", "", ofl) < 0) {
	printf ("Failed in opening file %s\n", inp_name);
	FTNPP_task_terminate (-1, 0, NULL);
    }

    fclose (ofl);
    return;
}

/************************************************************************

    Description: This is the main function of processing, line-by-line,
	the specified section of the include file, fname. The output is
	written to file ofl. The section is deliminated by the starting
	pattern, st_pat, and ending pattern, end_pat. We assume both
	patterns starts at the first character on a line. If the line is
	longer than the buffer, this function may not process correctly.
	The function calls itself.

    Inputs:	fname - input file name;
		st_pat - start pattern of the section;
		end_pat - end pattern of the section;
		ofl - output file handle;

    Return:	It returns 0 on success of -1 if the file does not exist.

    Notes:	It terminates the program when a fatal error is encountered.

************************************************************************/

static int Process_a_section (char *fname, char *st_pat, char *end_pat,
	FILE *ofl)
{
    FILE *ifl;
    int copy, found;
    char buf [BUF_SIZE], *pt;
    int st_plen, end_plen, len;
    int ifdef_pos;	/* where we are in terms IFDEF position: 0 - outside
			   the loop; 1 after IFDEF; 2 - after ELSE; */
    int delete = 0;		/* deleted lines after C$DELETE */
    int ifdef_matched;
    char inc_name [NAME_LEN];
    int line_num;

    /* make sure the recursive call does not go too deep */
    if (Level > 64) {
	printf ("Too many recursive include levels\n");
	FTNPP_task_terminate (-1, 0, fname);
    }

    /* remove additional elements after pattern */
    pt = st_pat;
    while (*pt != '\0') {
	if (*pt == '(' || *pt == ' ' || *pt == '\t' || *pt == '\n') {
	    *pt = '\0';
	    break;
	}
	pt++;
    }

    /* check this section to ignore duplicated include */
    if (RDP_check_section (fname, st_pat) == RDP_INCLUDED)
				/* this section has already been included */
	return (0);

    /* open the input file */
    ifl = fopen (fname, "r");
    if (ifl == NULL)
	return (-1);
    Level++;
    line_num = 0;

    /* process line by line */
    strcpy (inc_name, "");
    ifdef_pos = 0;
    delete = 0;
    ifdef_matched = 0;
    copy = 0;
    found = 0;
    st_plen = strlen (st_pat);
    end_plen = strlen (end_pat);
    while (fgets (buf, BUF_SIZE - EXTRA_SPACE, ifl) != NULL) {

	line_num++;

	/* process C$DELETE*/
	if (buf [1] == '$') {
	    if (strncmp (buf, "C$DELETE", 8) == 0) {
		if (delete == 1) {
		    printf ("Unexpected C$DELETE\n");
		    FTNPP_task_terminate (-1, line_num, fname);
		}
		delete = 1;
		continue;
	    }
	    if (strncmp (buf, "C$ENDDELETE", 11) == 0) {
		if (delete == 0) {
		    printf ("Unexpected C$ENDDELETE\n");
		    FTNPP_task_terminate (-1, line_num, fname);
		}
		delete = 0;
		continue;
	    }
	}
	if (delete)
	    continue;

	/* process C$INSERT */
	if (buf[1] == '$' && strncmp (buf, "C$INSERT", 8) == 0) {
	    char tmpbuf [BUF_SIZE];
	    strcpy (tmpbuf, buf + 8);
	    strcpy (buf, tmpbuf);
	}

	/* process IFDEF */
	if (buf [0] == '#') {
	    if (strncmp (buf, "#IFDEF", 6) == 0) {
		int i;
		if (ifdef_pos != 0) {
		    printf ("Unexpected #IFDEF\n");
		    FTNPP_task_terminate (-1, line_num, fname);
		}
		ifdef_pos = 1;
		for (i = 0; i < N_ifdef_strings; i++) {
		    if (strcmp (strtok (buf + 6, "\n\t "),
						Ifdef_string[i]) == 0) {
			ifdef_matched = 1;
			break;
		    }
		}
		continue;
	    }
	    if (strncmp (buf, "#ELSE", 5) == 0) {
		if (ifdef_pos != 1) {
		    printf ("Unexpected #ELSE\n");
		    FTNPP_task_terminate (-1, line_num, fname);
		}
		ifdef_pos = 2;
		continue;
	    }
	    if (strncmp (buf, "#ENDIF", 6) == 0) {
		if (ifdef_pos == 0) {
		    printf ("Unexpected #ENDIF\n");
		    FTNPP_task_terminate (-1, line_num, fname);
		}
		ifdef_pos = 0;
		ifdef_matched = 0;
		continue;
	    }
	}

	if ((ifdef_matched && ifdef_pos == 2) ||
	    (ifdef_matched == 0 && ifdef_pos == 1))
        {
	    continue;		/* discard the line */
	}

	if (found == 0) {
	    if (st_pat [0] == '\0') {
		copy = 1;
		found = 1;
	    }
	    else if (strncmp (st_pat, buf, st_plen) == 0 &&
		 !(REP_check_legal_char (buf[st_plen]))) {
		copy = 1;
		found = 1;
		continue;
	    }
	}

	if (copy == 0)
	    continue;

	if (end_pat[0] != '\0' && copy == 1 &&
		strncmp (end_pat, buf, end_plen) == 0) {
	    copy = 0;
	    break;	/* done */
	}

	if (strncasecmp ("$include", buf, 8) == 0) {
	    Process_include (buf, inc_name, ofl, line_num, fname);
	    continue;
	}

	if (strncasecmp ("$EJECT", buf, 6) == 0) {
	    buf[0] = 'C';
	    continue;
	}

	/* process non-standard syntax */
	Process_a_line (buf, line_num, fname);

	/* write out the line */
	len = strlen (buf);
	if (fwrite (buf, sizeof (char), len, ofl) != len) {
	    printf ("Failed in writing output file\n");
	    FTNPP_task_terminate (-1, 0, NULL);
	}
    }

    /* IFDEF not finished */
    if (ifdef_pos != 0) {
	printf ("#IFDEF is not closed\n");
	FTNPP_task_terminate (-1, line_num, fname);
    }

    /* C$DELETE not finished */
    if (delete != 0) {
	printf ("C$DELETE is not closed\n");
	FTNPP_task_terminate (-1, line_num, fname);
    }

    /* pattern not found */
    if (st_plen > 0 && found == 0) {
	printf ("Section %s is not found\n", st_pat);
	FTNPP_task_terminate (-1, line_num, fname);
    }

    /* include section not ended */
    if (st_plen > 0 && found == 1 && copy == 1) {
	printf ("Section %s is not terminated\n", st_pat);
	FTNPP_task_terminate (-1, line_num, fname);
    }

    fclose (ifl);
    Level--;

    return (0);
}

/**************************************************************************

    Description: This function processes an #INCLUDE line.

    Inputs:	buf - line buffer;
		inc_name - name of the next include file.
		ofl - output file handle;
		line_num - current line number;
		fname - current file name;

    Notes:	This function exits if a fatal error is found.

**************************************************************************/

static void Process_include (char *buf, char *inc_name, FILE *ofl,
			     int line_num, char *fname)
{
    int len;
    char *tok;
    char new_st [NAME_LEN];
    char c;
    int i;

    /* write out this line before process it */
    buf [0] = 'C';
    len = strlen (buf);
    if (fwrite (buf, sizeof (char), len, ofl) != len) {
	printf ("Failed in writing output file\n");
	FTNPP_task_terminate (-1, 0, NULL);
    }

    /* terminate at ";" */
    tok = buf;
    while (*tok != '\0') {
	if (*tok == ';') {
	    *tok = '\0';
	    break;
	}
	tok++;
    }

    if ((tok = strtok (buf + 8, " ,\t\n")) == NULL) {
	printf ("Bad INCLUDE line\n");
	FTNPP_task_terminate (-1, line_num, fname);
    }
    if (strncmp (tok, "**", 2) != 0) {	/* a file name */
	strncpy (inc_name, tok, NAME_LEN);
	inc_name [NAME_LEN - 1] = 0;
	/* get pattern */
	if ((tok = strtok (NULL, " ,\t\n")) != NULL) {
	    strncpy (new_st, tok, NAME_LEN);
	    if (new_st [0] == ';')	/* a comment string */
		new_st [0] = '\0';
	}
	else
	    new_st [0] = '\0';
    }
    else
	strncpy (new_st, tok, NAME_LEN);
    new_st [NAME_LEN - 1] = 0;

    if (strlen (inc_name) < 2) {
	printf ("Bad INCLUDE line (No file name)\n");
	FTNPP_task_terminate (-1, line_num, fname);
    }
    if (new_st [0] != '\0' &&
	(strlen (new_st) < 2 || strncmp (new_st, "**", 2) != 0)) {
	printf ("Bad INCLUDE line (Bad section name: %s)\n", new_st);
	FTNPP_task_terminate (-1, line_num, fname);
    }

    len = 0;		/* remove the group name */
    while ((c = inc_name [len]) != '\0' && c != '/') {
	if (c >= 65 && c <= 90)	/* turn into lower case */
	    inc_name [len] = c + 32;
	len++;
    }
    inc_name [len] = '\0';

    for (i = 0; i < Inc_path_cnt; i++) {
	char full_name [NAME_LEN];
	int slen;
	int ret;

	slen = strlen (Inc_path [i]);
	if (slen + strlen (inc_name) + 1 >= NAME_LEN) {
	    printf ("Path string too long\n");
	    FTNPP_task_terminate (-1, line_num, fname);
	}
	strcpy (full_name, Inc_path [i]);
	if (slen > 0 && full_name [slen - 1] != '/')
	    strcat (full_name, "/");
	strcat (full_name, inc_name);

	/* insert the new file */
	if (new_st[0] == '\0')
	    ret = Process_a_section (full_name, new_st, "", ofl);
	else
	    ret = Process_a_section (full_name, new_st, "/*", ofl);

	if (ret == 0)
	    break;
    }
    if (i >= Inc_path_cnt) {
	printf ("File %s not found\n", inc_name);
	FTNPP_task_terminate (-1, line_num, fname);
    }

    {
	/* write ending mark after process it */
	buf [0] = 'C';
	strcpy (buf + 9, "- DONE\n");
	len = strlen (buf);
	if (fwrite (buf, sizeof (char), len, ofl) != len) {
	    printf ("Failed in writing output file\n");
	    FTNPP_task_terminate (-1, 0, NULL);
	}
    }

    return;
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "D:I:s:r:thl")) != EOF) {
	switch (c) {

	    case 'D':
		if (N_ifdef_strings >= MAXN_IFD_STR) {
		    printf ("Too many -D option specified\n");
		    err = 1;
		}
		else {
		    if ((Ifdef_string[N_ifdef_strings] = 
				malloc (strlen (optarg) + 1)) == NULL) {
			printf ("malloc failed\n");
			err = 1;
		    }
		    else {
			strcpy (Ifdef_string[N_ifdef_strings], optarg);
			N_ifdef_strings++;
		    }
		}
		break;

	    case 'I':
		if (Inc_path_cnt >= MAXN_PATH ||
		    strlen (optarg) == 0)
		    err = 1;
		Inc_path [Inc_path_cnt] = optarg;
		Inc_path_cnt++;
		break;

	    case 's':
		if (Src_path_cnt >= MAXN_PATH ||
		    strlen (optarg) == 0)
		    err = 1;
		Src_path [Src_path_cnt] = optarg;
		Src_path_cnt++;
		break;

	    case 't':
		X_replacement = ' ';
		break;

	    case 'l':
		REP_need_truncate ();
		break;

	    case 'r':
		if (REP_argument (optarg) != 0)
		    err = 1;
		break;

	    case 'h':
	    case '?':
		err = 1;
		break;
	}
    }

    if (err == 1) {              /* Print usage message */
	printf ("Usage: %s (options) fortran_file_name_list\n", Prog_name);
	printf ("       Options:\n");
	printf ("       -t (test mode - replace leading X by space)\n");
	printf ("       -l (characters after __ are truncated in identifiers)\n");
	printf ("       -D #IFDEF_string (no defaults)\n");
	printf ("       -I include_file_path (default: current dir)\n");
	printf ("       -s source_file_path (default: current dir)\n");
	printf ("       -r Old_name=New_name (default: No)\n");
	printf ("\n");
	printf ("       fortran_file_name must have suffix \".ftn\".\n");
	printf ("       \"-r\" option specifies an identifier replacement in the FORTRAN\n");
	printf ("       code. The name is case sensitive.\n");
	printf ("       The -I, -s and -r options may be specified multiply.\n");
	printf ("       If and only if the -I (or -s) option is not specified,\n");
	printf ("       the current directory is used by default.\n");
	return (-1);
    }

    if (Inc_path_cnt == 0) {
	Inc_path [0] = "";
	Inc_path_cnt = 1;
    }
    if (Src_path_cnt == 0) {
	Src_path [0] = "";
	Src_path_cnt = 1;
    }

    return (0);
}

/*************************************************************************

    Description: This function processes a line.

    Inputs:	buf - the line buffer;

*************************************************************************/

static void
Process_a_line (char *buf, int line_num, char *fname)
{
    static int lastx = 0;
    int line_len_changed;
    char *cpt, c;
    static int slash_cnt = 0, quo_cnt = 0;
    int continuing_line;

    if (buf[0] == '$') {
	/* disable $INLINE directive */
	if (strncmp (buf, "$INLINE", 7) == 0) {
	    buf [0] = 'C';
	    return;
	}

	/* disable $INSKIP directive */
	if (strncmp (buf, "$INSKIP", 7) == 0) {
	    buf [0] = 'C';
	    return;
	}

	/* commented $TCOM */
	if (strncmp (buf, "$TCOM", 5) == 0) {
	    buf [0] = 'C';
	    return;
	}
    }

    /* replace the leading X */
    if (buf[0] == 'X' || buf[0] == 'x' ||
	(lastx == 1 && buf[0] == ' ' && buf [5] != ' ')) {
	lastx = 1;
	buf[0] = X_replacement;
    }
    else 
	lastx = 0;

    cpt = buf;
    while (*cpt == ' ' || *cpt == '\t')
	cpt++;

    /* process the END, IMPLICIT NONE and BLOCK DATA statements */
    if (cpt > buf) {
	if (RDP_special_statement (cpt, Level, line_num, fname)) {
	    buf [0] = 'C';
	    return;
	}
    }

    if (cpt - buf == 5)
	continuing_line = 1;
    else
	continuing_line = 0;

    /* To process hexidecimal constant format Z we count slash_cnt and quo_cnt; 
       We also need to process continued lines which is determined by space_cnt 
       and do_space_cnt */
    line_len_changed = 0;
    if (!continuing_line)
	slash_cnt = quo_cnt = 0;

    while ((c = *cpt) != '\0') {
	char tbuf [BUF_SIZE];

	/* counting slashes and quotation marks */
	if (c == '/')
	    slash_cnt++;
	if (c == '\'')
	    quo_cnt++;

	/* process hexidecimal constant format Y */
	if (c == 'Y' && cpt [1] == '\'') {
	    *cpt = 'X';
	}

	/* process hexidecimal constant format Z */
	if (c == 'Z' && (slash_cnt % 2) == 1 && (quo_cnt % 2) == 0) {
	    char *st, *end, cc;
	    int len;

	    st = cpt + 1;
	    while (*st == ' ')
		st++;
	    end = st;
	    while (((cc = *end) <= '9' && cc >= '0') || (cc <= 'F' && cc >= 'A'))
		end++;
	    len = end - st;
	    if (len > 0 &&
		(cpt > buf && ((cc = cpt [-1]) < '0' || 
			(cc > '9' && cc < 'A') || cc > 'Z')) &&
		((cc = *end) < '0' || (cc > '9' && cc < 'A') || cc > 'Z')) {		
						/* hex data found */
		strcpy (tbuf, st);
		cpt[1] = '\'';
		strncpy (cpt + 2, tbuf, len);
		cpt += (len + 2);
		cpt [0] = '\'';
		strcpy (cpt + 1, tbuf + len);

		line_len_changed = 1;
	    }
	}

	if (c == ';') {			/* process in line comments */

	    /* the following is save as long as EXTRA_SPACE is larger than
		the inserted string length */
	    strcpy (tbuf, cpt);
	    *cpt = '\n';
	    strcpy (cpt + 1, "C ");
	    strcpy (cpt + 3, tbuf);
	    return;
	}

	cpt++;
    }

    /* special processing on the FORTRAN code */
    if (REP_processing (buf, line_num, fname) != 0)
	line_len_changed = 1;

    if (line_len_changed)
	Process_long_line (buf, line_num, fname);

    return;
}

/************************************************************************

    Description: This function splits a long line into two lines. 

    Inputs:	buf - the line buffer;
		line_num - current line number;
		fname - current file name;

    Notes:	This function exits if a fatal error is found.

************************************************************************/

static void Process_long_line (char *buf, int line_num, char *fname)
{
    char tbuf [BUF_SIZE];
    int len;

    /* comment line is not processed */
    if (buf[0] != ' ' && buf[0] != '\t')
	return;

    len = strlen (buf);
    if (len > FORTRAN_LINE_LEN) {
	char *pt;

	/* search for the point to break */
	pt = buf + FORTRAN_LINE_LEN;
	while (pt > buf && *pt != ' ')
	    pt--;

	if (*pt != ' ') {
	    printf ("A long line can not be broken down\n");
	    FTNPP_task_terminate (-1, line_num, fname);
	}

	strcpy (tbuf, pt + 1);
	*pt = '\n';
	strcpy (pt + 1, "     *  ");
	strcpy (pt + 9, tbuf);
    }
    return;
}

