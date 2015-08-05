
/******************************************************************

    gdc is a tool that generates device configuration file for
    specified site(s). This is the main module.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/01/03 19:22:02 $
 * $Id: gdc_main.c,v 1.23 2011/01/03 19:22:02 jing Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gdc_def.h"

int NO_var_value_print = 0;
int VERBOSE = 0;
char SRC_dir[MAX_STR_SIZE] = ".";
char INPUT_dir[MAX_STR_SIZE] = "";
char ARGV0[MAX_STR_SIZE] = "";

static int Chan_number = 1;
static char Site_name[MAX_STR_SIZE] = "";
static char Input_file[MAX_STR_SIZE] = "";
static char Output_file[MAX_STR_SIZE] = "";
static char Dest_dir[MAX_STR_SIZE] = ".";
static char Out_fname[MAX_STR_SIZE] = "";
static char Check_var[MAX_STR_SIZE] = "";
static char Shared_dir[MAX_STR_SIZE] = "";
static char Link_path[MAX_STR_SIZE] = "";
static char Defaut_var_def_file[MAX_STR_SIZE] = "gdc_vars.def";
static char Apply_routine[MAX_STR_SIZE] = "";
static int To_install = 0;

static char *User_defines = NULL;
static int N_user_defines = 0;

/* Control characters: assignment, condition, passthrough, comment */
static char Cdefs[] = GDC_CONT_CHARS;
static char Cchars[sizeof (Cdefs)];

static int Is_red = 0;		/* This site is a redundant site */
static int All_sites = 0;	/* This is processing all sites */
static FILE *Fout;

/* local functions */
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Process_site ();
static void Check_variable ();
static int Init_site_name_vars ();
static int Get_all_site_names (char **sitesp);
static int Get_control_value (char *control_var, char *value, 
					int size, int search_inc);
static void Do_apply_routine ();


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    char buf[MAX_STR_SIZE];

    if (Read_options (argc, argv) != 0)
	exit (1);
    if (strcmp (Site_name, "ALL_SITES") == 0)
	All_sites = 1;

    if (Apply_routine[0] != '\0')
	Do_apply_routine ();

    if (strlen (Check_var) > 0)
	Check_variable ();

    GDCM_strlcpy (INPUT_dir, MISC_dirname (Input_file, buf, MAX_STR_SIZE),
							MAX_STR_SIZE);
    if (All_sites) {
	char *site_names;
	int n_sites, i, off, of;

	n_sites = Get_all_site_names (&site_names);
	VERBOSE = 0;
	off = 0;
	for (i = 0; i < n_sites; i++) {

	    if ((of = GDCM_ftoken (site_names + off, "S,", 0,
					Site_name, MAX_STR_SIZE)) <= 0) {
		fprintf (stderr, "Missing site - coding error\n");
		exit (1);
	    }
	    off += of;
	    Chan_number = 1;
	    Init_site_name_vars ();
	    Process_site ();
	    if (Is_red) {
		Is_red = 0;
		GDCV_discard_variables ();
		Chan_number = 2;
		Init_site_name_vars ();
		Process_site ();
	    }
	    GDCV_discard_variables ();
	}
    }
    else if (Init_site_name_vars ())
	Process_site ();
    exit (0);
}

/******************************************************************

    Initialize basic site variables. 

******************************************************************/

static int Init_site_name_vars () {
    char usname[MAX_STR_SIZE], *p, incs[MAX_STR_SIZE];
    int i, n;

    if (VERBOSE || All_sites) {
	if (Chan_number > 1)
	    printf ("Process site %s - channel %d\n", Site_name, Chan_number);
	else
	    printf ("Process site %s\n", Site_name);
    }

    if (!GDCE_is_identifier (Site_name, 0)) {
	fprintf (stderr, 
		"Site name (%s) contains unexpected character\n", Site_name);
	exit (1);
    }
    GDCM_strlcpy (usname, Site_name, MAX_STR_SIZE);
    MISC_tolower (usname);
    GDCV_set_variable ("site_name", usname);
    MISC_toupper (usname);
    GDCV_set_variable ("SITE_NAME", usname);
    GDCV_set_variable (usname, "YES");
    GDCV_set_variable ("INPUT_FILE", MISC_basename (Input_file));

    p = User_defines;
    for (i = 0; i < N_user_defines; i++) {
	char var[MAX_STR_SIZE], value[MAX_STR_SIZE];
	GDCM_ftoken (p, "S=", 0, var, MAX_STR_SIZE);
	GDCM_ftoken (p, "S=", 1, value, MAX_STR_SIZE);
	if (strncmp (var, "_gdc_", 5) != 0)
	    GDCV_set_variable (var, value);
	p += strlen (p) + 1;
    }

    n = Get_control_value ("_gdc_include", incs, MAX_STR_SIZE, 0);
    p = incs;
    for (i = 0; i < n; i++) {
	GDCP_read_include_file (p, 0);
	p += strlen (p) + 1;
    }

    GDCP_read_include_file (Defaut_var_def_file, 1);
    return (1);
}

/******************************************************************

    Processes gdc control variable "v_name" of "value".

******************************************************************/

void GDCM_gdc_control (char *v_name, char *value) {

    if (strcmp (v_name, "_gdc_site_is") == 0) {
	char b[MAX_STR_SIZE];
	if (strcmp (value, "redundant") != 0)
	    GDCP_exception_exit ("Unexpected value (%s) for _gdc_site_is\n", 
								value);
	Is_red = 1;
	sprintf (b, "%d", Chan_number);
	GDCV_set_variable ("CHAN_NUM", b);
    }
}

/******************************************************************

    Read the names of all sites. The site names are returned with 
    "sitesp". Returns the number of sites found.

******************************************************************/

static int Get_all_site_names (char **sitesp) {
    char tk[MAX_STR_SIZE];
    char *sites;
    int n, n_sites;

    n = Get_control_value ("_gdc_get_all_sites", tk, MAX_STR_SIZE, 1);
    if (n <= 0) {
	fprintf (stderr, "_gdc_get_all_sites undefined - Don't know how to get the list of sites\n");
	exit (1);
    }
    if (n > 1) {
	fprintf (stderr, "_gdc_get_all_sites multiply defined - Don't know how to get the list of sites\n");
	exit (1);
    }

    if (VERBOSE)
	printf ("    Use %s to get the list of all sites\n", tk);
    sites = GDCR_get_data_value (tk);
    if (sites == NULL) {
	char *msg;
	GDCR_get_error (&msg);
	fprintf (stderr, "%s", msg);
	fprintf (stderr, "Nothing found with %s\n", tk);
	exit (1);
    }
    n_sites = MISC_get_token (sites, "S,", 0, NULL, 0);
    *sitesp = MISC_malloc (strlen (sites) + 1);
    strcpy (*sitesp, sites);
    if (VERBOSE)
	printf ("    %d sites found\n", n_sites);
    return (n_sites);
}

/******************************************************************

    Searches gdc control varible "control_var" in command line, the
    default variable definition file (if search_inc is true) and the
    environmantl variables. It returns their values in "value" of size
    bytes. Returns number of values found on success or -1 on failure.

******************************************************************/

static int Get_control_value (char *control_var, char *value, 
				int size, int search_inc) {
    int cnt, i, sz;
    char *buf, *pi, *po, *val;

    cnt = 0;
    val = value;
    val[0] = '\0';
    sz = size;
    pi = User_defines;
    for (i = 0; i < N_user_defines; i++) {
	char var[MAX_STR_SIZE], vl[MAX_STR_SIZE];
	GDCM_ftoken (pi, "S=", 0, var, MAX_STR_SIZE);
	GDCM_ftoken (pi, "S=", 1, vl, MAX_STR_SIZE);
	if (strcmp (var, control_var) == 0) {
	    GDCM_strlcpy (val, vl, sz);
	    val += strlen (vl) + 1;
	    sz -= strlen (vl) + 1;
	    cnt++;
	}
	pi += strlen (pi) + 1;
    }

    if (search_inc) {
	char incs[MAX_STR_SIZE];
	int n;
	GDCM_strlcpy (incs, Defaut_var_def_file, MAX_STR_SIZE);
	n = Get_control_value ("_gdc_include", incs + strlen (incs) + 1, 
		MAX_STR_SIZE - (strlen (incs) + 1), 0) + 1;
	pi = incs;
	for (i = 0; i < n; i++) {
	    char fname[MAX_STR_SIZE], buf[MAX_STR_SIZE];
	    FILE *fd;
	    GDCM_strlcpy (fname, pi, MAX_STR_SIZE);
	    GDCM_add_dir (SRC_dir, fname, MAX_STR_SIZE);
	    fd = fopen (fname, "r");
	    if (fd != NULL) {
		while (fgets (buf, MAX_STR_SIZE, fd) != NULL) {
		    char tk[MAX_STR_SIZE], *p;
		    int off;
		    p = buf;
		    if ((off = GDCM_stoken (p, "", 0, tk, 8)) > 0 &&
						    strcmp (tk, "SET") == 0)
			p += off;
		    if ((off = GDCM_stoken (p, "S=", 0, tk, MAX_STR_SIZE)) > 0
			&& strcmp (tk, control_var) == 0 &&
			GDCM_ftoken (p + off, "", 0, tk, MAX_STR_SIZE) > 0) {
			GDCM_strlcpy (val, tk, sz);
			val += strlen (tk) + 1;
			sz -= strlen (tk) + 1;
			cnt++;
		    }
		}
		fclose (fd);
	    }
	    pi += strlen (pi) + 1;
	}
    }

    cnt += GDCV_get_var_from_env (control_var, val, sz);
    cnt = GDCV_rm_duplicated_strings (value, cnt);
    if (cnt <= 0)
	return (0);

    buf = MISC_malloc (size);
    memcpy (buf, value, size);
    pi = buf;
    po = value;
    sz = size;
    for (i = 0; i < cnt; i++) {
	char *v = GDCP_evaluate_variables (pi);
	if (v != NULL) {
	    GDCM_strlcpy (po, v, sz);
	    free (v);
	}
	else
	    GDCM_strlcpy (po, pi, sz);
	po += strlen (po) + 1;
	sz -= strlen (po) + 1;
	pi += strlen (pi) + 1;
    }
    free (buf);

    return (cnt);
}

/******************************************************************

    Process the current site as defined in "Vars". 

******************************************************************/

static void Process_site () {
    int size;
    char *buf, ifile[MAX_STR_SIZE], *sitename;

    sitename = GDCV_get_value ("SITE_NAME");

    GDCM_strlcpy (ifile, Input_file, MAX_STR_SIZE);
    GDCM_add_dir (SRC_dir, ifile, MAX_STR_SIZE);
				/* add src dir to input file name */

    if (Output_file[0] == '\0' ||
	All_sites) {
	strcpy (Out_fname, Input_file);
	GDCM_strlcat (Out_fname, ".", MAX_STR_SIZE);
	GDCM_strlcat (Out_fname, sitename, MAX_STR_SIZE);
	if (GDCV_get_value ("FAA_CH2") || GDCV_get_value ("NWS_CH2"))
	    GDCM_strlcat (Out_fname, ".2", MAX_STR_SIZE);
    }
    else
	strcpy (Out_fname, Output_file);
    if (!To_install)
	GDCM_add_dir (Dest_dir, Out_fname, MAX_STR_SIZE);
    else
	GDCM_add_dir ("./", Out_fname, MAX_STR_SIZE);

    Fout = fopen (Out_fname, "w");
    if (Fout == NULL) {
	fprintf (stderr, "Could not open/create file %s\n", Out_fname);
	exit (1);
    }
    GDCF_share_vars (Out_fname, Fout, Dest_dir, Link_path, Shared_dir, To_install);

    size = GDCF_read_file (ifile, &buf);
    if (size >= 0) {
	GDCP_process_template (buf, size, ifile, Cchars, Fout);
	free (buf);
    }

    size = ftell (Fout);
    fclose (Fout);
    if (size == 0)
	unlink (Out_fname);

    if (To_install)
	GDCF_install_file ();
}

/******************************************************************

    Checks variable Check_var. It exits with
    0 If it is defined and is YES; 1 if the function failed;
    2 if variable is not defined; 3 Oherwise. It prints the value
    of Check_var if it is defined and in verbose mode.

******************************************************************/

static void Check_variable () {
    char *v;
    int pv;

    pv = 0;
    if (VERBOSE) {
	pv = 1;
	VERBOSE = 0;
    }
    if (Site_name[0] == '\0') {
	fprintf (stderr, "Site name not specified\n");
	exit (1);
    }
    Init_site_name_vars ();
    if ((v = GDCV_get_value (Check_var)) == NULL)
	exit (2);
    if (pv)
	printf ("%s: %s\n", Check_var, v);
    if (strcmp (v, "YES") == 0)
	exit (0);
    exit (3);
}

/******************************************************************

    Processes Apply_routine. 

******************************************************************/

static void Do_apply_routine () {

    if (strcmp (Apply_routine, "align") == 0) {
	char ifile[MAX_STR_SIZE];
	strcpy (ifile, Input_file);
	GDCM_add_dir (SRC_dir, ifile, MAX_STR_SIZE);
				/* add src dir to input file name */
	GDCR_align_columns (ifile);
    }
    else if (strcmp (Apply_routine, "roc_remote_access_routes") == 0) {
	char *site_names;
	int n_sites = Get_all_site_names (&site_names);
	GDCF_process_nmt_data (n_sites, site_names, Input_file);
    }
    else {
	fprintf (stderr, "Routine name (%s) is not expected\n", Apply_routine);
	exit (1);
    }
    exit (0);
}

/******************************************************************

    Add "dir" in front of "name" if "name" is not a full path. 

******************************************************************/

void GDCM_add_dir (char *dir, char *name, int size) {
    char buf[MAX_STR_SIZE], *b;

    if (name[0] == '/' || strncmp (name, "./", 2) == 0)
	return;
    b = buf;
    if (size > MAX_STR_SIZE)
	b = MISC_malloc (size);
    if (MISC_full_path (dir, name, b, size) == NULL ||
	strlen (b) >= size)
	GDCP_exception_exit ("Path string too long or bad (%s %s)\n", 
							dir, name);
    strcpy (name, b);
    if (b != buf)
	free (b);
}

/******************************************************************

    Savely appends "s2" to "s1" of "size" bytes. 

******************************************************************/

void GDCM_strlcat (char *s1, char *s2, int size) {
    if (strlen (s1) + strlen (s2) >= size)
	GDCP_exception_exit ("Strings too long to cat (%s %s)\n", s1, s2);
    strcat (s1, s2);
}

/******************************************************************

    Savely copies "s2" to "s1" of "size" bytes.

******************************************************************/

void GDCM_strlcpy (char *s1, char *s2, int size) {
    if (strlen (s2) >= size)
	GDCP_exception_exit ("String too long to copy (%s)\n", s2);
    strcpy (s1, s2);
}

/******************************************************************

    Calls MISC_get_token and checks if the token has been truncated.
    If yes, -1 is returned (assuming there is no short token).

******************************************************************/

int GDCM_stoken (char *text, char *cntl, int ind, char *buf, int size) {

    int off = MISC_get_token (text, cntl, ind, buf, size);
    if (buf != NULL && off >= size && strlen (buf) >= size - 1)
	return (-1);
    return (off);
}

/******************************************************************

    Calls MISC_get_token and checks if the token has been truncated.
    If yes, terminates gdc (assuming the full token is too long to 
    process).

******************************************************************/

int GDCM_ftoken (char *text, char *cntl, int ind, char *buf, int size) {

    int off = MISC_get_token (text, cntl, ind, buf, size);
    if (buf != NULL && off >= size && strlen (buf) >= size - 1)
	GDCP_exception_exit ("Text too long to process (%s...)\n", buf);
    return (off);
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
    Fout = stdout;
    strcpy (Cchars, Cdefs);
    GDCM_strlcpy (ARGV0, argv[0], MAX_STR_SIZE);
    while ((c = getopt (argc, argv, "c:s:o:D:i:S:f:C:d:L:IpP:vh?")) != EOF) {

	switch (c) {
	    char tok[MAX_STR_SIZE];

            case 'c':
		if (sscanf (optarg, "%d", &Chan_number) != 1 ||
		    Chan_number <= 0 || Chan_number > 2) {
		    fprintf (stderr, "unexpected -c specification\n");
		    exit (1);
		}
                break;

            case 's':
		GDCM_strlcpy (Site_name, optarg, MAX_STR_SIZE);
		MISC_toupper (Site_name);
                break;

            case 'o':
		GDCM_strlcpy (Output_file, optarg, MAX_STR_SIZE);
                break;

            case 'S':
		GDCM_strlcpy (SRC_dir, optarg, MAX_STR_SIZE);
                break;

            case 'D':
		GDCM_strlcpy (Dest_dir, optarg, MAX_STR_SIZE);
                break;

            case 'i':
		GDCM_strlcpy (Defaut_var_def_file, optarg, MAX_STR_SIZE);
                break;

            case 'f':
		GDCM_process_f_option (optarg, Cchars, "-f option");
                break;

            case 'd':
		if (GDCM_stoken (optarg, "S=", 0, tok, MAX_STR_SIZE) <= 0 ||
		    GDCM_stoken (optarg, "S=", 1, tok, MAX_STR_SIZE) <= 0) {
		    fprintf (stderr, "Unexpected -d option (%s)\n", optarg);
		    exit (1);
		}
		User_defines = STR_append (User_defines, 
					optarg, strlen (optarg) + 1);
		N_user_defines++;
                break;

            case 'C':
		GDCM_strlcpy (Check_var, optarg, MAX_STR_SIZE);
                break;

            case 'I':
		To_install = 1;
                break;

            case 'L':
		To_install = 1;
		if (GDCM_stoken (optarg, "S,", 0, tok, MAX_STR_SIZE) > 0)
		    GDCM_strlcpy (Shared_dir, tok, MAX_STR_SIZE);
		if (GDCM_stoken (optarg, "S,", 1, tok, MAX_STR_SIZE) > 0)
		    GDCM_strlcpy (Link_path, tok, MAX_STR_SIZE);
		if (strlen (Shared_dir) <= 0 || strlen (Link_path) <= 0) {
		    fprintf (stderr, "Unexpected -L option (%s)\n", optarg);
		    exit (1);
		}
                break;

            case 'p':
		printf ("This option is no longer needed\n");
                break;

            case 'P':
		GDCM_strlcpy (Apply_routine, optarg, MAX_STR_SIZE);
                break;

            case 'v':
		VERBOSE = 1;
                break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind == argc - 1)		/* get the input file name  */
	GDCM_strlcpy (Input_file, argv[optind], MAX_STR_SIZE);
    else if (optind < argc - 1) {
	fprintf (stderr, "Multiple input files specified\n");
	exit (1);
    }

    if (strlen (Site_name) == 0 && Apply_routine[0] == '\0') {
	fprintf (stderr, "Site name not specified\n");
	exit (1);
    }
    if (strlen (Input_file) == 0 && Check_var[0] == '\0') {
	fprintf (stderr, "Input file name not specified\n");
	exit (1);
    }
    if (strcmp (Site_name, "ALL_SITES") == 0 && 
	(To_install || strlen (Check_var) > 0)) {
	fprintf (stderr, "Unexpected ALL_SITE specified\n");
	exit (1);
    }

    return (err);
}

/**************************************************************************

    Process -f option of argument "optarg". Returns 0 on success ot
    -1 on failure.

**************************************************************************/

int GDCM_process_f_option (char *optarg, char *cchars, char *msg) {
    char cold, dash, cnew, c;
    int found, i, cnt, indent;

    if (strncmp (optarg, "indent=", 7) == 0) {
	cnt = sscanf (optarg + 7, "%d%c", &indent, &c);
	if (strlen (optarg) == 7)
	    cchars[INDENT_C] = '0' - 1;		/* this is -1 */
	else if (cnt == 1 && indent >= 0 && indent <= 32)
	    cchars[INDENT_C] = '0' + indent;
	else 
	    GDCP_exception_exit ("Unexpected %s: %s\n", msg, optarg);
    }
    else if (strncmp (optarg, "keep_blank=", 11) == 0) {
	if (strcmp (optarg + 11, "yes") == 0)
	    cchars[KEEP_BLANK_C] = 'y';
	else if (strcmp (optarg + 11, "no") == 0)
	    cchars[KEEP_BLANK_C] = 'n';
	else if (strcmp (optarg + 11, "") == 0)
	    cchars[KEEP_BLANK_C] = 'o';
	else 
	    GDCP_exception_exit ("Unexpected %s: %s\n", msg, optarg);
    }
    else if (strcmp (optarg, "cs_style") == 0) {
	GDCM_process_f_option ("{-<", cchars, msg);
	GDCM_process_f_option ("}->", cchars, msg);
	GDCM_process_f_option ("!-#", cchars, msg);
	cchars[INDENT_C] = '0' - 1;		/* this is -1 */
	return (0);
    }
    else if (strcmp (optarg, "full") == 0)
	cchars[FORMAT_C] = 'f';
    else if (strcmp (optarg, "short") == 0)
	cchars[FORMAT_C] = 's';
    else if (((cnt = sscanf (optarg, "%c%c%c%c", &cold, &dash, &cnew, &c))
			!= 2 && cnt != 3) || dash != '-')
	GDCP_exception_exit ("Unexpected %s: %s\n", msg, optarg);
    else {

	if (cnt == 2)
	    cnew = 255;
	found = 0;
	i = 0;
	while (Cdefs[i] != '\0') {
	    if (Cdefs[i] == cold) {
		int k;

		cchars[i] = cnew;
		k = 0;
		while (cchars[k] != '\0') {
		    if (k != i && cchars[k] == cchars[i])
			cchars[k] = cold;	/* swap the control char */
		    k++;
		}
		found = 1;
		break;
	    }
	    i++;
	}
	if (!found)
	    GDCP_exception_exit ("%c is not a control char in %s: %s\n", 
						cold, msg, optarg);
    }

    return (0);
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Reads configuration file template \"input_file\" and generates site\n\
        configuration file for site \"site\". If \"site\" is ALL_SITES, site\n\
        files for all sites are generated. The -f and -d options can be used\n\
        multiply. Some options do not need \"site\" and/or \"input_file\".\n\
        Options:\n\
          -c channel_number (1 or 2. Channel number for redundant sites. The\n\
             default is 1)\n\
          -o output_file (Output file name. The default is \"input_file.site\"\n\
             or \"input_file.site.2\")\n\
          -S src_dir (Directory for input files. The default is the current\n\
             directory)\n\
          -D dest_dir (Directory for output files. The default is the current\n\
             directory)\n\
          -i default_vdf (Default variable definition file to include. The\n\
             default is gdc_vars.def)\n\
          -f instruction (Changes template interpreting, See gdc man-page)\n\
          -d var=value (Defines variable \"var\", sets its value to \"value\")\n\
	  -C var (Checks variable \"var\". Exits with 0 if \"var\" is YES, 2 if\n\
	     undefined, 1 on error, or 3 else. Prints value in verbose mode)\n\
	  -I (Installs the site file)\n\
	  -L shared_dir,lpath (Installs link to file in shared_dir with lpath)\n\
          -P align (Column-aligns input file. The output is input_file.a)\n\
          -P routine (Applies \"routine\" to \"input_file\". See gdc man-page)\n\
          -v (Verbose mode)\n\
          -h (Prints usage info)\n\
	Examples:\n\
	  gdc -s ktlx lan_switch\n\
	  gdc -c 2 -s tjua lan_switch\n\
	  gdc -s ALL_SITES lan_switch\n\
";

    printf ("Usage:  %s [options] -s site input_file\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
