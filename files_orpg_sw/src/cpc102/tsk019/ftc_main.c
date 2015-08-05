
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This is the 
    main module.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/02 21:30:13 $
 * $Id: ftc_main.c,v 1.2 2010/11/02 21:30:13 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"


static char Template_file[MAX_STR_SIZE] = "";
					/* name of function template file */
static char Out_file[MAX_STR_SIZE] = "";

static int C_post_process = 0;		/* Run as post processor of f2c */
static char Task_name[MAX_STR_SIZE] = "task";	/* Task name */
static int Search_task_ft = 0;

static char *Input_files = NULL;
static int N_input_files = 0;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static char *Get_next_value (char *inp, int *v);
static char *Get_next_symbol (char *inp, char *s);
static int Arithmetic_cal (char *inp, int *out);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {

    if (Read_options (argc, argv) != 0)
	exit (1);

    FR_init ();
    if (Template_file[0] != '\0') {
	FG_search_templates (Template_file, N_input_files, Input_files);
	exit (0);
    }

    if (C_post_process)
	sprintf (Out_file, "%s.p", Input_files);
    else
	sprintf (Out_file, "%s.c", Input_files);

    if (C_post_process)
	FP_process_cfile (Input_files, Out_file);
    else {
	int i;
	char *file;
	FG_read_templates (Task_name);
	file = Input_files;
	for (i = 0; i < N_input_files; i++) {
	    FC_process_ffile (file, Out_file);
	    file += strlen (file) + 1;
	}
	if (FC_is_main ())
	    FF_set_main (Out_file);
	FI_update_include ();
    }

    exit (0);
}

/******************************************************************

    Returns Task_name.

******************************************************************/

char *FM_get_task_name () {
    return (Task_name);
}

/******************************************************************

    FM_tk_malloc allocates "size" bytes for a token. FM_tk_free 
    frees all memory allocated by FM_tk_malloc.

******************************************************************/

static char **Tkbufs = NULL;
static int N_tkbufs = 0;
static int Cr_tboff = 0, Cr_tbsize = 0;

char *FM_tk_malloc (int size) {
    char *p;

    if (size > Cr_tbsize || N_tkbufs == 0) {
	int s = size;
	if (s < 2048)
	    s = 2048;
	p = MISC_malloc (s);
	Cr_tbsize = s;
	Cr_tboff = 0;
	Tkbufs = (char **)STR_append ((char *)Tkbufs, (char *)&p, sizeof (char *));
	N_tkbufs++;
    }

    p = Tkbufs[N_tkbufs - 1] + Cr_tboff;
    Cr_tboff += size;
    Cr_tbsize -= size;
    return (p);
}

void FM_tk_free () {
    int i;
    char **p = (char **)Tkbufs;
    for (i = 0; i < N_tkbufs; i++)
	free (p[i]);
    N_tkbufs = 0;
    Tkbufs = (char **)STR_reset ((char *)Tkbufs, 0);
}

/**********************************************************************

    Processes integer expression pointed to by "inp" of "size" bytes and
    puts the result in "outbuf" of size MAX_STR_SIZE. If the output is too
    large, a buffer for output is malloced and the caller must free it.
    Returns the pointer to the output. No static variables can be used
    here so recursive calls are allowed. This is a simplified version 
    from gdc.

**********************************************************************/

char *FM_process_expr (char *inp, int size, char *outbuf) {
    char *buf, *out, *in, *r_buf;

    in = inp;
    buf = NULL;
    while (1) {			/* process all brackets on the same level */
	char *p, *pe, *bst, *bend, *name, *rpst, tbuf[MAX_STR_SIZE];
	int cnt, rplen, name_len;

	p = in;
	pe = in + size;
	bst = bend = name = NULL;
	cnt = name_len = 0;
	while (p < pe) {		/* find bst, bend, and name */
	    if (*p == '(') {
		if (bst == NULL)	/* starting bracket */
		    bst = p;
		else
		    cnt++;
	    }
	    else if (*p == ')') {
		if (cnt == 0) {
		    bend = p;		/* matched ending bracket */
		    break;
		}
		cnt--;
	    }
	    p++;
	}
    
	out = rpst = NULL;
	rplen = 0;
	if (out == NULL && bst != NULL) {
	    rpst = bst;
	    rplen = bend - bst + 1;
	    out = FM_process_expr (bst + 1, bend - bst - 1, tbuf);
	}
    
	if (out != NULL) {	/* bracketed section processed */
	    int ll, os, s;

	    if (buf != inp && buf != outbuf)
		free (buf);
	    ll = strlen (out);
	    os = (size - rplen) + ll;
	    if (os < MAX_STR_SIZE)
		buf = outbuf;
	    else
		buf = MISC_malloc (os + 1);
	    memmove (buf, in, rpst - in);
	    s = rpst - in;
	    memmove (buf + s, out, ll);
	    s += ll;
	    memmove (buf + s, rpst + rplen, os - s);
	    buf[os] = '\0';
	    if (out != tbuf)
		free (out);
	}
	else
	    break;

	in = buf;
	size = strlen (buf);
    }

    if (buf == NULL) {		/* no bracket found */
	if (size < MAX_STR_SIZE)
	    buf = outbuf;
	else
	    buf = MISC_malloc (size + 1);
	memcpy (buf, inp, size);
	buf[size] = '\0';
    }

    r_buf = buf;
    {
	int result = 0;
	if (Arithmetic_cal (r_buf, &result) < 0) {
	    if (r_buf != outbuf)
		free (out);
	    return (NULL);
	}
	if (r_buf != outbuf)
	    free (out);
	sprintf (outbuf, "%d", result);
	return (outbuf);
    }
}

/****************************************************************************

    Gets the next value in text "inp" and returns it with "v". Returns the 
    pointer to after the used chars on success or NULL on failure.

****************************************************************************/

static char *Get_next_value (char *inp, int *v) {
    char b[64], *p;

    if (sscanf (inp, "%d", v) != 1)
	return (NULL);
    sprintf (b, "%d", *v);
    p = strstr (inp, b);
    if (p == NULL)
	return (NULL);
    return (inp + (p - inp) + strlen (b));
}

/****************************************************************************

    Gets the next symbal in text "inp" and returns it with "s". Returns the 
    pointer to after the used chars on success or NULL on failure.

****************************************************************************/

static char *Get_next_symbol (char *inp, char *s) {

    char *p = inp + MISC_char_cnt (inp, " \t");
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '\0') {
	*s = *p;
	return (p + 1);
    }
    return (NULL);
}

/****************************************************************************

    Calculates integer arithmetic expression "inp" and returns the result in
    "out". Returns 0 on success or -1 on failure.

****************************************************************************/

static int Arithmetic_cal (char *inp, int *out) {
    int accum, cr_term, v;
    char s, *p;

    accum = 0;
    cr_term = 1;
    p = inp;
/*    p = Get_next_symbol (p, &s);
    if (p != NULL) {
	if (s != '-')
	    return (-1);
	cr_term = -1;
    }
*/
    p = Get_next_value (p, &v);
    if (p == NULL)
	return (-1);
    cr_term *= v;
    while (1) {

	p = Get_next_symbol (p, &s);
	if (p == NULL)		/* error */
	    return (-1);
	if (s == '\0')		/* end of inp */
	    break;
	p = Get_next_value (p, &v);
	if (p == NULL)
	    return (-1);
	if (s == '+') {
	    accum += cr_term;
	    cr_term = v;
	}
	else if (s == '-') {
	    accum += cr_term;
	    cr_term = -v;
	}
	else if (s == '*')
	    cr_term *= v;
	else if (s == '/') {
	    if (v == 0) {
		fprintf (stderr, "Devided by 0\n");
		return (-1);
	    }
	    cr_term /= v;
	}
	else
	    return (-1);
    }
    accum += cr_term;
    *out = accum;
    return (0);
}

/**************************************************************************

    Reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    while ((c = getopt (argc, argv, "s:pt:h?")) != EOF) {

	switch (c) {

	    case 'p':
		C_post_process = 1;
		break;

            case 's':
		strcpy (Template_file, optarg);
                break;

	    case 't':
		Search_task_ft = 1;
		strcpy (Task_name, optarg);
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }
    while (optind <= argc - 1) {	/* get the input file name  */
	Input_files = STR_append (Input_files, argv[optind], 
						strlen (argv[optind]) + 1);
	N_input_files++;
	optind++;
    }

    if (N_input_files == 0) {
	fprintf (stderr, "Input file not specified\n");
	exit (1);
    }
/*    if (Template_file[0] == '\0' && N_input_files > 1) {
	fprintf (stderr, "Multiple input files specified\n");
	exit (1);
    }
*/

    return (err);
}


/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Converts FORTRAN files \"input_files\" to a c file.\n\
        Options:\n\
          -p (Post processes the c file generated by f2c)\n\
          -s file (Searches function templates from \"input_file\"(s) and\n\
             adds them to \"file\". The input files can be of any of types\n\
             .h, .c, and .f. If \"file\" does not exist, it is created.\n\
             If file does not have \".\" in it, it is condisered as a task\n\
             name and file created is ftc_taskname.h)\n\
          -t task_name (Specifies the task name. Default: task)\n\
          -h (Prints usage info)\n\
	Examples:\n\
";

    printf ("Usage:  %s [options] input_file(s)\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
