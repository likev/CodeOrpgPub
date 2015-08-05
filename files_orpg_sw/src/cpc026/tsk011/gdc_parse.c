
/******************************************************************

    gdc is a tool that generates device configuration file for
    specified site(s). This module contains routines that interpret
    template files.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/25 19:04:58 $
 * $Id: gdc_parse.c,v 1.20 2013/06/25 19:04:58 steves Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "gdc_def.h"

#define MAX_RECUR_DEPTH 128
#define MAX_CONDITION_LEVEL 6
#define MAX_LOOP_LEVEL 6
#define MAX_CALL_PARMS 64

extern int VERBOSE;
extern int NO_var_value_print;
extern char SRC_dir[];
extern char INPUT_dir[];

typedef struct {	/* loop struct */
    char *name;
    int cr_ind;
} loop_var_t;

static loop_var_t Lpvars[MAX_LOOP_LEVEL];
static int N_lpvars = 0;

typedef struct {	/* procedure struct */
    char *name;
    char *params;
    char *cont;
    int size;
    int n_params;
} Procedure_t;

#define MAX_N_PROCEDURES 64
static Procedure_t Prcs[MAX_N_PROCEDURES];	/* current procedures */
static int N_prcs = 0;

typedef struct {	/* for context saving */
    char *cr_loc;
    char *cr_file;
    loop_var_t *lpvars;
    int n_lpvars;
    Procedure_t *prcs;
    int n_prcs;
    FILE *fout;
    int done;
    char ccs[sizeof (GDC_CONT_CHARS)];
    int indent_width;
    int keep_blank_line;
    int vst_ind;	/* start index of the local variables */
    int n_vs;		/* number of local variables */
    char field_delimiter;
} Context_t;

#define MAX_CALL_DEPTH 64
static Context_t Contexts[MAX_CALL_DEPTH];	/* contex stack */
static int N_contexts = 0;

static char *Cr_loc = NULL;
static char *Cr_file = NULL;
static FILE *Fout = NULL;
static int Done = 0;
static char Ccs[] = GDC_CONT_CHARS;

static int Ignore_error = 0;	/* for passing param through several calls */
static int Gdc_comment_printed = 0;	/* the gdc comments has been printed */
static int Is_include = 0;	/* the current file is an include file */

enum {PCC_SEC_ILLEGAL, PCC_SEC_EMPTY, PCC_SEC_COMMENT, PCC_SEC_LINE, 
      PCC_SEC_ASSIGN, PCC_SEC_CONDITION, PCC_SEC_ELSE, PCC_SEC_PROCEDURE,
      PCC_SEC_CALL, PCC_SEC_LOOP};	/* return values of Get_section_type */

#define GV_NOT_WILDCARD -1	/* return values of Get_value */
#define GV_NOT_FOUND -2

/* return values of Process_loop */
enum {PL_DONE, PL_NOT_LOOP};

#define GDCP_FAILED -1

/* local functions */
static void Process_section (char *secp, int sec_size);
static int Get_section_type (char *secp, int size, int *st_off, 
						int *slen, int *tlen);
static int Get_sect_contents (int type, char *text, int *off, int *len);
static int Get_condition_str (char *str);
static int Get_assign_str (char *str, char *buf, int buf_size);
static int Get_loop_str (char *str);
static char *Evaluate_variables (char *text);
static int Evaluate_condition (char *text);
static void Process_assign (char *text, char *cont);
static void Print_a_line (char *line);
static int Get_value (char *var, char **value, char *buf);
static int Process_loop (char *cp, char *cont, int slen);
static int Process_conditional_sections (char *cp, int size);
static char *Look_for_ending_brace (char *st);
static void Print_gdc_comments ();
static void Print_variable_values (char *title);
static int Procees_special_instruction (char *ins);
static void Change_control_chars (char *ops, int st_tok);
static void Process_include (char *text, int size, char *fname);
static void Process_procedure (char *cp, char *cont, int slen);
static void Remove_procedures ();
static char *Truncate_text (char *text, int size);
static void Process_call (char *cp, char *cont, int slen);
static void Remove_loop_vars ();
static void Push_context (int new_file);
static void Pop_context (int new_file);
static int Get_procedure_parameters (char *cp, char **parms, 
					int *n_pms, int local);
static void Save_values (char *v_names, int n_vs, char **values);
static void Set_values (char *v_names, int n_vs, char **values);
static int Get_procedure_name (char *cp, char *key, char *name, int size);


/**********************************************************************

    Returns 1 if the current file is an include file, or 0 otherwise.

**********************************************************************/

int GDCP_is_include () {
    return (Is_include);
}

/**********************************************************************

    Returns Ccs.

**********************************************************************/

char *GDCP_get_ccs () {
    return (Ccs);
}

/**********************************************************************

    Sets Done to 1.

**********************************************************************/

void GDCP_set_done () {
    Done = 1;
}

/**********************************************************************

    Processes the content of a configuration template. The content starts
    at "text" and its total size is "size" bytes. "text" has at least
    one extra byte after "size" bytes.

**********************************************************************/

void GDCP_process_template (char *text, int size, char *fname, 
						char ccs[], FILE *fout) {

    if (VERBOSE)
	printf ("Process template file %s\n", fname);
    Gdc_comment_printed = 0;
    Push_context (1);
    strcpy (Ccs, ccs);
    Fout = fout;
    Cr_file = fname;
    Process_section (text, size);
    Pop_context (1);
}

/**********************************************************************

    Processes the content of an include file "fname". Its content is in
    "text" of "size" bytes. "text" has at least one extra byte after
    "size" bytes. An include does not generate any output to the gdc's 
    output. It is used for reading the data files and defining global 
    variables.

**********************************************************************/

static void Process_include (char *text, int size, char *fname) {

    Is_include = 1;
    Push_context (1);
    Fout = NULL;
    if (VERBOSE)
	Fout = stdout;
    Cr_file = fname;
    Process_section (text, size);
    Pop_context (1);
    Is_include = 0;
}

/**********************************************************************

    Reads the variable definition file and sets the global variables 
    accordingly. If the full path of "name" is not specified, we will
    try Src_dir first and then the dir for the input file. If "optional"
    is true, we return if the file does not exist.

**********************************************************************/

void GDCP_read_include_file (char *name, int optional) {
    char fname[MAX_STR_SIZE], *buf;
    int size;
    struct stat st;

    GDCM_strlcpy (fname, name, MAX_STR_SIZE);
    GDCM_add_dir (SRC_dir, fname, MAX_STR_SIZE);
    if (strlen (INPUT_dir) <= 0) {	/* no other dir to try */
	if (optional && (stat (fname, &st) < 0 || !S_ISREG (st.st_mode)))
	    return;
    }
    else if (stat (fname, &st) < 0 || !S_ISREG (st.st_mode)) {
	GDCM_strlcpy (fname, name, MAX_STR_SIZE);
	GDCM_add_dir (INPUT_dir, fname, MAX_STR_SIZE);
	if (optional && (stat (fname, &st) < 0 || !S_ISREG (st.st_mode)))
	    return;
    }
    if (VERBOSE)
	printf ("Process include file %s\n", fname);

    size = GDCF_read_file (fname, &buf);
    Process_include (buf, size, fname);
    free (buf);
}

/**********************************************************************

    Processes gdc control variable "v_name" of value "value".

**********************************************************************/

void GDCP_gdc_control (char *v_name, char *value) {

    if (strcmp (v_name, "_gdc_set_interpreting") == 0)
	Change_control_chars (value, 0);
    else if (strcmp (v_name, "_gdc_include") == 0)
	GDCP_read_include_file (value, 0);
    else if (strcmp (v_name, "_gdc_print_variables") == 0)
	Print_variable_values (value);
}

/**********************************************************************

    Prints error message "message" and terminate the program.

**********************************************************************/

void GDCP_exception_exit (const char *format, ...) {
    int i;
    va_list args;

    va_start (args, format);
    fprintf (stderr, "ERROR: ");
    vfprintf (stderr, format, args);
    va_end (args);

    if (Cr_loc != NULL) {
	if (Cr_file != NULL)
	    fprintf (stderr, "AT (FILE %s): \n%s\n", 
			Cr_file, Truncate_text (Cr_loc, 200));
	else
	    fprintf (stderr, "AT: \n%s\n", Truncate_text (Cr_loc, 200));
    }
    else if (Cr_file != NULL)
	fprintf (stderr, "IN FILE: %s\n", Cr_file);

    for (i = N_contexts - 1; i >= 0; i--) {
	if (Contexts[i].cr_loc != NULL) {
	    if (Contexts[i].cr_file != NULL)
		fprintf (stderr, "CALLED FROM (FILE: %s): \n%s\n", 
		Contexts[i].cr_file, Truncate_text (Contexts[i].cr_loc, 128));
	    else
		fprintf (stderr, "CALLED FROM: \n%s\n", 
				Truncate_text (Contexts[i].cr_loc, 128));
	}
	else if (Contexts[i].cr_file != NULL)
	    fprintf (stderr, "IN FILE: %s\n", Contexts[i].cr_file);
    }
    exit (1);
}

/**********************************************************************

    Truncates text to size and copy it to a buffer. Returns the buffer. 

**********************************************************************/

static char *Truncate_text (char *text, int size) {
    static char *buf = NULL;
    int len, cnt;
    char *p;

    buf = STR_reset (buf, size);
    len = strlen (text);
    if (len > size - 4) {
	strncpy (buf, text, size - 4);
	buf[size - 4] = '\0';
	strcat (buf, "...");
    }
    else {
	strncpy (buf, text, len);
	buf[len] = '\0';
    }
    p = buf;
    cnt = 0;		/* at most 4 lines */
    while (*p != '\0') {
	if (*p == '\n') {
	    cnt++;
	    if (cnt >= 4) {
		p[0] = '\0';
		break;
	    }
	}
	p++;
    }
    return (buf);
}

/**********************************************************************

    Prints gdc comment and variables.

**********************************************************************/

static void Print_gdc_comments () {
    static int stack_cnt = 0;

    if (Is_include || stack_cnt > 0)
	return;
    stack_cnt++;
    if (!Gdc_comment_printed) {
	time_t tm;
        char buf[MAX_STR_SIZE];

	if (Fout == NULL)
	    return;
	sprintf (buf, "%c\n", Ccs[COM_C]);
	Print_a_line (buf);
	tm = time (NULL);
	sprintf (buf, "%c This file is generated by gdc for site %s on %s", 
			Ccs[COM_C], GDCV_get_value ("SITE_NAME"), ctime (&tm));
	Print_a_line (buf);
	sprintf (buf, "%c from file %s\n", Ccs[COM_C], Cr_file);
	Print_a_line (buf);
    }
    Gdc_comment_printed = 1;
    stack_cnt--;
}

/**********************************************************************

    Prints all global variables and their values.

**********************************************************************/

static void Print_variable_values (char *title) {
    Variable_t *vars;
    int i, n_vars;
    char *buf;

    if (Fout == NULL)
	return;
    /* we must call Print_a_line at lease once to let gdc messages print out */
    if (strlen (title) > 0)
	Print_a_line (title);
    buf = STR_reset (NULL, 256);
    buf = STR_copy (buf, "  Variables set to \"YES\" are ");
    buf[0] = Ccs[COM_C];
    n_vars = GDCV_get_variables (&vars);
    for (i = 0; i < n_vars; i++) {
	Variable_t *v = vars + i;
	if (strcmp (v->value, "YES") == 0) {
	    buf = STR_cat (buf, v->name);
	    buf = STR_cat (buf, " ");
	}
    }
    Print_a_line (buf);
    STR_free (buf);

    /* in the following, we directly send to Fout to avoid buffer management */
    fprintf (Fout, 
	"%c Other variables set for this site are\n", Ccs[COM_C]);
    fprintf (Fout, "%c SITE_NAME = %s, site_name = %s\n", Ccs[COM_C], \
		GDCV_get_value ("SITE_NAME"), GDCV_get_value ("site_name"));
    for (i = 0; i < n_vars; i++) {
	Variable_t *v = vars + i;
	if (strcmp (v->value, "YES") == 0 ||
				strcasecmp (v->name, "site_name") == 0)
	    continue;
	fprintf (Fout, "%c %s = %s\n", Ccs[COM_C], v->name, v->value);
    }
    fprintf (Fout, "%c\n", Ccs[COM_C]);
}

/**********************************************************************

    Processes a section of the generic configuration. The section starts
    at "secp" and its total size is "sec_size" bytes.

**********************************************************************/

static void Process_section (char *secp, int sec_size) {
    static int stack_dep = 0;
    int size;
    char *cp;

    if (Done)
	return;

    stack_dep++;
    if (stack_dep > MAX_RECUR_DEPTH)
	GDCP_exception_exit ("Too many levels of recursive calls of Process_section\n");

    cp = secp;
    size = sec_size;
    while (size > 0) {
	int type, st_off, slen, tlen;
	char c, *cont;

	Cr_loc = cp;
	type = Get_section_type (cp, size, &st_off, &slen, &tlen);
	if (tlen == 0)
	    GDCP_exception_exit ("Unexpected section size\n");

	cont = cp + st_off;
	c = cont[slen];
	cont[slen] = '\0';
	switch (type) {
	    char *evcont;

	    case PCC_SEC_EMPTY:
		if (Ccs[KEEP_BLANK_C] == 'y' || 
		    (Ccs[KEEP_BLANK_C] == 'o' && !Is_include && 
							Ccs[INDENT_C] < '0'))
		    Print_a_line ("\n");
		break;

	    case PCC_SEC_COMMENT:
		if ((evcont = Evaluate_variables (cont)) != NULL) {
		    Print_a_line (evcont);
		    free (evcont);
		}
		else
		    Print_a_line (cont);
		break;

	    case PCC_SEC_LINE:
		if ((evcont = Evaluate_variables (cont)) != NULL) {
		    Process_section (evcont, strlen (evcont));
		    free (evcont);
		}
		else
		    Print_a_line (cont);
		break;

	    case PCC_SEC_ASSIGN:
		Process_assign (cp, cont);
		break;

	    case PCC_SEC_CONDITION:
		cont[slen] = c;
		tlen = Process_conditional_sections (cp, size);
		break;

	    case PCC_SEC_LOOP:
		Process_loop (cp, cont, slen);
		break;

	    case PCC_SEC_ELSE:
		GDCP_exception_exit ("Unexpected ELSE statement\n");
		break;

	    case PCC_SEC_PROCEDURE:
		Process_procedure (cp, cont, slen);
		break;

	    case PCC_SEC_CALL:
		cont[slen] = c;
		Process_call (cp, cont, slen);
		break;

	    default:
		break;
	}
	cont[slen] = c;
	cp += tlen;
	size -= tlen;
	if (Done)
	    break;
    }

    stack_dep--;
    return;
}

/**********************************************************************

    Processes conditional sections of the generic configuration started
    at "cp" of "size" bytes.

**********************************************************************/

static int Process_conditional_sections (char *cp, int size) {
    char *sec_st, *s_p;
    int sec_len;

    sec_st = NULL;
    sec_len = 0;
    s_p = cp;
    while (1) {
	int s_off, s_len, s_tlen, type, yes, off, len, is_not, skip, is_elseif;
	char *b, buf[MAX_STR_SIZE], *tp, tk[MAX_STR_SIZE];

	Ignore_error = 1;
	type = Get_section_type (s_p, size - (s_p - cp), 
						&s_off, &s_len, &s_tlen);
	Ignore_error = 0;
	if (type != PCC_SEC_CONDITION && type != PCC_SEC_ELSE)
	    break;
	if (type == PCC_SEC_ELSE) {
	    if (sec_st == NULL) {
		sec_len = s_len;
		sec_st = s_p + s_off;
	    }
	    s_p += s_tlen;
	    break;
	}

	tp = s_p;
	while (tp - s_p < size && *tp != Ccs[CND_C] && *tp != '\n')
	    tp++;
	if (*tp != Ccs[CND_C]) 
	    GDCP_exception_exit ("Bad Condition statement\n");
	len = tp - s_p;
	if (len < MAX_STR_SIZE)
	    b = buf;
	else
	    b = MISC_malloc (len + 1);
	memcpy (b, s_p, len);
	b[len] = '\0';

	skip = 0;
	is_not = 0;
	is_elseif = 0;
	if ((off = GDCM_stoken (b, "", 0, tk, MAX_STR_SIZE)) > 0) {
	    if (strcmp (tk, "IF") == 0)
		skip += off;
	    else if (strcmp (tk, "ELSEIF") == 0) {
		if (s_p == cp)
		    GDCP_exception_exit ("ELSEIF must follow IF statement\n");
		is_elseif = 1;
		skip += off;
	    }
	}
	if ((off = GDCM_stoken (b + skip, "", 0, tk, MAX_STR_SIZE)) > 0 &&
					strcmp (tk, "NOT") == 0) {
	    is_not = 1;
	    skip += off;
	}

	if (s_p != cp && !is_elseif)
	    break;

	GDCV_check_condition_reserved_word (1);
	yes = Evaluate_condition (b + skip);
	GDCV_check_condition_reserved_word (0);
	if (b != buf)
	    free (b);
	if (yes == GDCP_FAILED)
	    GDCP_exception_exit ("Condition cannot be evaluated\n");
	if (is_not) {
	    if (yes)
		yes = 0;
	    else
		yes = 1;
	}
	if (yes && sec_st == NULL) {
	    sec_len = s_len;
	    sec_st = s_p + s_off;
	}
	s_p += s_tlen;
    }
    if (sec_st != NULL)
	Process_section (sec_st, sec_len);

    return (s_p - cp);
}

/**********************************************************************

    Processes a procedure section. "cp" points to the procedure tag.
    "cont" points to the section and "slen" is the size of the section.

**********************************************************************/

static void Process_procedure (char *cp, char *cont, int slen) {
    char name[MAX_STR_SIZE], *parms;
    int off, size, n_pms;

    cp += MISC_char_cnt (cp, "\n");
    if ((off = Get_procedure_name (cp, "procedure", name, MAX_STR_SIZE)) <= 0)
	GDCP_exception_exit ("Procedure name not found\n");
    size = Get_procedure_parameters (cp + off, &parms, &n_pms, 1);
    if (size <= 0)
	GDCP_exception_exit ("Parameter not found - coding error\n");
    if (N_prcs >= MAX_N_PROCEDURES)
	GDCP_exception_exit ("Too many procedures defined\n");
    Prcs[N_prcs].name = MISC_malloc (strlen (name) + 1 + size);
    Prcs[N_prcs].params = Prcs[N_prcs].name + strlen (name) + 1;
    strcpy (Prcs[N_prcs].name, name);
    memcpy (Prcs[N_prcs].params, parms, size);
    Prcs[N_prcs].cont = cont;
    Prcs[N_prcs].size = slen;
    Prcs[N_prcs].n_params = n_pms;
    N_prcs++;
}

/**********************************************************************

    Processes a procedure call. "cp" points to the call line. "cont"
    points to the parameter list and "slen" is the size of cont. The
    parameters and local variables must be put on a stack.

**********************************************************************/

static void Process_call (char *cp, char *cont, int slen) {
    char tk[MAX_STR_SIZE];
    char *pms, *parms, *savedv[MAX_CALL_PARMS];
    int i, off, size, n_ps;

    cp += MISC_char_cnt (cp, "\n");
    tk[0] = '\0';
    off = Get_procedure_name (cp, "call", tk, MAX_STR_SIZE);
    for (i = 0; i < N_prcs; i++) {
	if (strcmp (Prcs[i].name, tk) == 0)
	    break;
    }
    size = 0;			/* for eliminating gdc warning */
    if (i >= N_prcs ||		/* double check - should not needed */
	(size = Get_procedure_parameters (cp + off, &pms, &n_ps, 0)) < 0)
	GDCP_exception_exit ("Procedure %s not found - coding error\n", tk);
    if (n_ps > MAX_CALL_PARMS)
	GDCP_exception_exit ("Too many calling parameters\n");
    if (n_ps != Prcs[i].n_params)
	GDCP_exception_exit (
		"Incorrect number of call paramters (%d) - should be %d\n",
				n_ps, Prcs[i].n_params);
    parms = MISC_malloc (size);
    memcpy (parms, pms, size);

    Save_values (parms, n_ps, savedv);
    Push_context (0);
    Set_values (Prcs[i].params, n_ps, savedv);

    Process_section (Prcs[i].cont, Prcs[i].size);

    Save_values (Prcs[i].params, n_ps, savedv);
    Pop_context (0);
    Set_values (parms, n_ps, savedv);

    free (parms);
}

/**********************************************************************

    Saves the values of "n_vs" variables "v_names" in "values".

**********************************************************************/

static void Save_values (char *v_names, int n_vs, char **values) {
    int i;
    char *vname, *v;
    vname = v_names;
    for (i = 0; i < n_vs; i++) {
	values[i] = NULL;
	v = GDCV_get_value (vname);
	if (v != NULL) {
	    values[i] = MISC_malloc (strlen (v) + 1);
	    strcpy (values[i], v);
	}
	vname += strlen (vname) + 1;
    }
}

/**********************************************************************

    Sets the values of "n_vs" variables "v_names" to "values".

**********************************************************************/

static void Set_values (char *v_names, int n_vs, char **values) {
    int i;
    char *vname = v_names;
    for (i = 0; i < n_vs; i++) {
	if (values[i] != NULL) {
	    GDCV_set_variable (vname, values[i]);
	    free (values[i]);
	    values[i] = NULL;
	}
	vname += strlen (vname) + 1;
    }
}

/**********************************************************************

    Removes all defined procedures.

**********************************************************************/

static void Remove_procedures () {
    int i;
    for (i = 0; i < N_prcs; i++) {
	free (Prcs[i].name);
    }
    N_prcs = 0;
}

/**********************************************************************

    Parses the procedure call paramters started from "cp". The paramters
    are returned with "parms" (null-terminated strings one after another)
    and the number with "n_pms". Returns the number of bytes in "cp"
    consumed by the parameters. The return value is also the buffer size
    of "parms". The caller does not free "parms". Returns -1 on failure.

**********************************************************************/

static int Get_procedure_parameters (char *cp, char **parms, 
						int *n_pms, int local) {
    static char *buf = NULL;
    char *p, *st, tk[MAX_STR_SIZE];
    int size, n, len, off;

    p = cp;
    p += MISC_char_cnt (p, " \t");
    if (*p != '(')
	return (-1);
    p++;
    st = p;
    while (*p != '\0' && *p != ')' && *p != '\n')
	p++;
    if (*p == '\0' || *p == '\n')
	return (-1);
    size = p - cp + 1;
    buf = STR_reset (buf, size);
    memcpy (buf, st, p - st);
    buf[p - st] = '\0';
    n = 0;
    p = buf;
    len = 0;
    while ((off = GDCM_ftoken (p, "S,", 0, tk, MAX_STR_SIZE)) > 0) {
	if (!GDCE_is_identifier (tk, 0))
	    return (-1);
	if (local && !GDCV_is_local_var (tk))
	    GDCP_exception_exit ("Procedure parameter cannot be global variable\n");
	strcpy (buf + len, tk);
	len += strlen (tk) + 1;
	n++;
	p += off;
    }
    if (parms != NULL)
	*parms = buf;
    if (n_pms != NULL)
	*n_pms = n;
    return (size);
}

/**********************************************************************

    Removes all defined loop variables.

**********************************************************************/

static void Remove_loop_vars () {
    int i;
    for (i = 0; i < N_lpvars; i++) {
	free (Lpvars[i].name);
    }
    N_lpvars = 0;
}

/**********************************************************************

    Processes a loop section. "cp" points to the loop tag (e.g. V[*]:).
    "cont" points to the section and "slen" is the size of the section.
    Returns PL_DONE if processed, PL_NOT_LOOP if the section is not a 
    loop or PL_NESTED_LOOP if nested loop is detected.

**********************************************************************/

static int Process_loop (char *cp, char *cont, int slen) {
    char tk[MAX_STR_SIZE];

    if (GDCM_ftoken (cp, "", 0, tk, MAX_STR_SIZE) > 0) {
	char *p, *name, *value;
	int cnt;

	p = tk;
	while (*p != ']')
	    p++;
	p[1] = '\0';
	name = tk;
	if (*name == '$')
	    name++;	
	cnt = Get_value (name, &value, NULL);
	if (cnt >= 0) {
	    int i;
	    if (N_lpvars >= MAX_LOOP_LEVEL)
		GDCP_exception_exit ("Max loop levels exceeded\n");
	    Lpvars[N_lpvars].name = MISC_malloc (strlen (name) + 1);
	    p = Lpvars[N_lpvars].name;
	    strcpy (p, name);
	    while (*p != '\0' && *p != '[')
		p++;
	    p[0] = '\0';
	    N_lpvars++;
	    for (i = 0; i < cnt; i++) {
		Lpvars[N_lpvars - 1].cr_ind = i + 1;
		Process_section (cont, slen);
	    }
	    free (Lpvars[N_lpvars - 1].name);
	    N_lpvars--;
	    return (PL_DONE);
	}
    }
    GDCP_exception_exit ("Unexpected loop statement\n");
    return (0);
}

/**********************************************************************

    Removes unnecessary spaces and end-line chars in "line" and prints
    it to the output file.

**********************************************************************/

static void Print_a_line (char *line) {
    char *p, spaces[16], firstc;
    int cnt, i;

    if (Fout == NULL)
	return;
    firstc = line[MISC_char_cnt (line, " \t")];
    if (firstc == Ccs[IGN_C])
	return;
    if (firstc == Ccs[COM_C] || firstc == Ccs[IGN_C]) {
	p = line + MISC_char_cnt (line, " \t") + 1;
	p += MISC_char_cnt (p, " \t");
	if (*p == '#' &&
	    Procees_special_instruction (p))
	    return;
    }

    if (!Is_include && !Gdc_comment_printed)
	Print_gdc_comments ();

    if (firstc == Ccs[PAS_C]) {
	char t[8];
	sprintf (t, " \t%c", Ccs[PAS_C]);
	fprintf (Fout, "%s", line + MISC_char_cnt (line, t));
	return;
    }

    if (Is_include || Ccs[INDENT_C] < '0') {	/* no indentation */
	if (line[strlen (line) - 1] != '\n')
	    fprintf (Fout, "%s\n", line);
	else
	    fprintf (Fout, "%s", line);
	return;
    }

    /* output indentation */
    p = line;
    cnt = 0;
    while (*p != '\0') {
	if (*p == ' ')
	    cnt++;
	else if (*p == '\t')
	    cnt += 8;
	else
	    break;
	p++;
    }
    cnt = cnt % (Ccs[INDENT_C] - '0' + 1);
    for (i = 0; i < cnt; i++)
	spaces[i] = ' ';
    spaces[cnt] = '\0';
    if (line[strlen (line) - 1] != '\n')
	fprintf (Fout, "%s%s\n", spaces, line + MISC_char_cnt (line, " \t"));
    else
	fprintf (Fout, "%s%s", spaces, line + MISC_char_cnt (line, " \t"));

}

/**********************************************************************

    Processes "#..." special instruction "ins". Returns 1 if processed
    or 0 otherwise.

**********************************************************************/

static int Procees_special_instruction (char *ins) {
    char tok[MAX_STR_SIZE];

    if (GDCM_stoken (ins, "", 0, tok, MAX_STR_SIZE) > 0 &&
	strcmp (tok, "#Installed") == 0) {
	int off, offset, is_link;
	offset = is_link = 0;
	if ((off = GDCM_stoken (ins, "", 1, tok, MAX_STR_SIZE)) > 0 &&
	    strcmp (tok, "in#") == 0)
	    offset = off;
	if ((off = GDCM_stoken (ins, "", 2, tok, MAX_STR_SIZE)) > 0 &&
	    strcmp (tok, "in#") == 0 &&
	    GDCM_stoken (ins, "", 1, tok, MAX_STR_SIZE) > 0 &&
	    strcmp (tok, "link") == 0) {
	    offset = off;
	    is_link = 1;
	}
	if (offset > 0) {
	    if (GDCF_set_install_path (is_link, ins + offset))
		Done = 1;
	    return (1);
	}
    }
    else if (GDCM_stoken (ins, "", 0, tok, MAX_STR_SIZE) > 0 &&
	strcmp (tok, "#-f") == 0 &&
	GDCM_stoken (ins, "", 1, tok, MAX_STR_SIZE) > 0 &&
	strcmp (tok, "options#") == 0) {
	Change_control_chars (ins, 2);
	return (1);
    }
    else if (GDCM_stoken (ins, "", 0, tok, MAX_STR_SIZE) > 0 &&
	strcmp (tok, "#Copy") == 0) {
	int fname_ind = -1;	/* not copy */
	if (GDCM_stoken (ins, "", 1, tok, MAX_STR_SIZE) > 0) {
	    if (strcmp (tok, "from#") == 0)
		fname_ind = 2;
	    else if (strcmp (tok, "binary") == 0 &&
		     GDCM_stoken (ins, "", 2, tok, MAX_STR_SIZE) > 0 &&
		     strcmp (tok, "from#") == 0)
		fname_ind = 3;
	}
	if (fname_ind > 0 &&
	    GDCM_ftoken (ins, "", fname_ind, tok, MAX_STR_SIZE) > 0) {
	    int optional, binary;
	    char b[32];

	    optional = 0;
	    if (GDCM_stoken (ins, "", fname_ind + 1, b, 32) > 0
		&& strcmp (b, "optional") == 0)
		optional = 1;
	    binary = 0;
	    if (fname_ind == 3)
		binary = 1;
	    if (GDCF_copy_from_file (tok, binary, Ccs, optional)) {
		Done = 1;
		return (1);
	    }
	}
    }
    return (0);
}

/**********************************************************************

    Processes control character changing tokens in "ops" starting with
    token "st_tok".

**********************************************************************/

static void Change_control_chars (char *ops, int st_tok) {
    char tok[MAX_STR_SIZE];

    while (GDCM_ftoken (ops, "", st_tok, tok, MAX_STR_SIZE) > 0) {
	if (VERBOSE) {
	    if (Cr_file != NULL)
		printf ("    Change interpreting: set \"%s\" in file %s\n",
						tok, Cr_file);
	    else
		printf ("    Change interpreting\n: set \"%s\"", tok);
	}
	GDCM_process_f_option (tok, Ccs, "interpreting setting instruction");
	st_tok++;
    }
}

/**********************************************************************

    Adds a new variable or sets/resets value for a existing variable.
    The variable name is the first token in "text" and the value is
    in "cont" which is a null terminated string. The leading and trailing
    spaces are removed if the value is not quoted by { and }.

**********************************************************************/

static void Process_assign (char *text, char *cont) {
    char v_name[MAX_STR_SIZE], *buf, *p, *v;
    char outbuf[MAX_STR_SIZE], *out;
    int off, strip;

    off = Get_assign_str (text, v_name, MAX_STR_SIZE);
    p = text + off;
    p += MISC_char_cnt (p, " \t\n");
    if (*p != Ccs[LB_C]) {	/* strip leading and trailing spaces */
	v = cont + MISC_char_cnt (cont, " \t\n");
	strip = 1;
    }
    else {
	v = cont;
	strip = 0;
    }
    buf = MISC_malloc (strlen (v) + 1);
    strcpy (buf, v);
    if (strip) {
	char *cp = buf + strlen (buf) - 1;
	while (cp >= buf && MISC_char_cnt (cp, " \t\n") > 0)
	    *cp-- = '\0';
    }
    if (Is_include) {
	out = GDCE_process_expression (buf, strlen (buf), 0, outbuf);
	GDCV_set_variable (v_name, out);
	if (out != outbuf)
	    free (out);
    }
    else
	GDCV_set_variable (v_name, buf);
    free (buf);
}

/**********************************************************************

    Replace variables in "text" by their values. The new text is stored
    in an allocated buffer. The caller will free the buffer. If there is
    no variable, NULL is returned. Undefined variables are not processed.

**********************************************************************/

char *GDCP_evaluate_variables (char *text) {
    return (Evaluate_variables (text));
}

static char *Evaluate_variables (char *text) {
    char *p, *buf, *t, tk[MAX_STR_SIZE], *value;
    int buf_size, str_size;

    buf = text;
    buf_size = 0;
    str_size = strlen (text);
    p = text;
    while (*p != '\0') {
	int nl, ol, fl;

	ol = 0;				/* to remove gcc error */
	tk[0] = '\0';
	fl = p - buf;
	if (*p == '$' && p[1] != '$') {
	    if (p[1] == Ccs[LB_C]) {		/* ${.} format */
		t = p + 2;
		while (*t != '\0') {
		    if (!GDCE_is_identifier (t, 1))
			break;
		    t++;
		}
		if (*t == Ccs[RB_C]) {
		    int len = t - p - 2;
		    if (len < MAX_STR_SIZE) {
			memcpy (tk, p + 2, len);
			tk[len] = '\0';
			ol = t - p + 1;
		    }
		}
	    }
	    else {		/* $. format */
		char *pp = p + 1;
		while (GDCE_is_identifier (pp, 1) && pp < p + MAX_STR_SIZE - 3)
		    pp++;
		ol = pp - p - 1;
		if (ol >= MAX_STR_SIZE - 4)	/* too long */
		    ol = 0;
		if (ol > 0 && *pp == '[') {	/* allow $var[*] and $var[?] */
		    pp++;
		    if (*pp == '*')
			pp++;
		    else if (*pp >= 48 && *pp <= 57) {
			pp++;
			while (*pp >= 48 && *pp <= 57)
			    pp++;
		    }
		    if (*pp == ']')
			ol = pp - p;
		}
		if (ol > 0) {
		    memcpy (tk, p + 1, ol);
		    tk[ol] = '\0';
		    ol++;
		}
	    }
	    if (strlen (tk) > 0) {
		char tbuf[MAX_STR_SIZE];

		if (Get_value (tk, &value, tbuf) != GV_NOT_FOUND) {
					/* replace var by its value */
		    nl = strlen (value);
		    if (nl - ol + str_size + 1 > buf_size) {
			buf_size = nl - ol + str_size + 128;
			t = MISC_malloc (buf_size);
			if (buf == text) {
			    memcpy (t, text, str_size + 1);
			}
			else {
			    memcpy (t, buf, str_size + 1);
			    free (buf);
			}
			buf = t;
		    }
		    if (buf[fl + ol] == '\n' && nl > 0 && value[nl - 1] == '\n')
			nl--;	/* remove duplicated \n caused by variable */
		    memmove (buf + fl + nl, buf  + fl + ol, 
							str_size - fl - ol);
		    memcpy (buf + fl, value, nl);
		    str_size += nl - ol;
		    buf[str_size] = '\0';
		    p = buf + fl + nl - 1;
		}
		else {
		    if (!Ignore_error && GDCV_is_required (tk))
			GDCP_exception_exit ("Variable %s not found\n", tk);
		}
	    }
	}
	p++;
    }
    if (buf == text)
	return (NULL);
    else
	return (buf);
}

/**********************************************************************

    Evaluate the condition text "text". "text" is null terminated. 
    Returns 1 on "true", 0 on "false" or GDCP_FAILED is cannot be evaluated.

**********************************************************************/

static int Evaluate_condition (char *text) {
    static int stack_dep = 0;
    char tk[MAX_STR_SIZE];
    int ret, cnt;

    stack_dep++;
    if (stack_dep > MAX_CONDITION_LEVEL) {
	stack_dep--;
	return (GDCP_FAILED);
    }

    ret = 0;
    cnt = 0;
    while (GDCM_ftoken (text, "Q\"", cnt, tk + 1, MAX_STR_SIZE - 1) > 0) {
	int equal, tind;
	char c, *p, *v, tmp[MAX_STR_SIZE], *expd, *v_name;

	if (tk[1] == '$')
	    v_name = tk + 1;
	else {
	    tk[0] = '$';
	    v_name = tk;
	}

	equal = 0;
	if ((p = strstr (v_name, "==")) != NULL)
	    equal = 1;
	else if ((p = strstr (v_name, "!=")) != NULL)
	    equal = -1;
	if (equal != 0) {
	    int rc;
	    c = *p;
	    *p = '\0';
	    v = v_name;		/* recursively expand v_name */
	    rc = 0;
	    Ignore_error = 1;
	    while ((expd = Evaluate_variables (v)) != NULL) {
		if (v != v_name)
		    free (v);
		v = expd;
		rc++;
		if (rc > 8)
		    break;
	    }
	    if (v != v_name) {
		char *str;
		if ((expd = Evaluate_variables (p + 2)) != NULL)
		   str = expd;
		else
		   str = p + 2;
/*		if (equal == 1 && strcmp (v, str) == 0) */
		if (equal == 1 && GDCV_match_wild_card (v, str) == 1)
		    ret = 1;
/*		if (equal == -1 && strcmp (v, str) != 0) */
		if (equal == -1 && GDCV_match_wild_card (v, str) != 1)
		    ret = 1;
		free (v);
		if (str != p + 2)
		    free (str);
	    }
	    Ignore_error = 0;
	    *p = c;
	    if (ret)
		break;
	    cnt++;
	    continue;
	}

	Ignore_error = 1;
	expd = Evaluate_variables (v_name);
	Ignore_error = 0;
	/* the next 6 line + else will be removed later to remove TOKEN? */
	if (N_lpvars > 0 &&
	    GDCM_stoken (v_name, "S_", 0, tmp, MAX_STR_SIZE) > 0 &&
	    strcmp (tmp, "$TOKEN") == 0 &&
	    MISC_get_token (v_name, "S_Ci", 1, &tind, 0) > 0 &&
	    tind == Lpvars[N_lpvars - 1].cr_ind)
		ret = 1;
	else if (expd != NULL) {
	    if (strcmp (expd, "YES") == 0)
		ret = 1;
	    else
		ret = Evaluate_condition (expd);
	    free (expd);
	}
	if (ret)
	    break;
	cnt++;
    }
    stack_dep--;
    return (ret);
}

/**********************************************************************

    Returns the type of the section started at "secp" of "size" bytes.
    Returns the type of the section. Other returns are: st_off - start 
    offset of the section contents; slen - length of the section text;
    tlen - total length of the section. The input buffer must be writable.
    Upon return, the input buffer is not modified.

**********************************************************************/

static int Get_section_type (char *secp, int size, int *st_off, 
				int *slen, int *tlen) {
    char end_c, *p, *p1, tk[MAX_STR_SIZE];
    int type, lc;
    int off, len, t;

    if (size <= 0)
	return (PCC_SEC_ILLEGAL);

    end_c = secp[size];		/* save end character */
    secp[size] = '\0';

    p = secp + MISC_char_cnt (secp, " \t\n");
    p1 = NULL;
    type = 0;			/* useless - to satisfy gcc */
    if (*p == Ccs[COM_C] || *p == Ccs[IGN_C] || *p == Ccs[PAS_C]) {}
    else if (strncmp (p, "ELSE", 4) == 0 &&
	p[4 + MISC_char_cnt (p + 4, " \t")] == Ccs[CND_C]) {  /* ELSE found */
	p1 = p + 5 + MISC_char_cnt (p + 4, " \t");
	type = PCC_SEC_ELSE;
    }
    else if ((lc = Get_assign_str (secp, NULL, 0)) > 0) { /* assign line */
	p1 = secp + lc;
	type = PCC_SEC_ASSIGN;
    }
    else if ((lc = Get_loop_str (secp)) > 0) { /* loop line */
	p1 = secp + lc;
	type = PCC_SEC_LOOP;
    }
    else if (Is_include && strncmp (p, "procedure", 9) == 0 &&
	     (off = Get_procedure_name (p, "procedure", 
						tk, MAX_STR_SIZE)) > 0) {
	int i, offp;
	for (i = 0; i < N_prcs; i++) {
	    if (strcmp (Prcs[i].name, tk) == 0)
		GDCP_exception_exit ("Procedure %s has already defined\n", tk);
	}
	offp = Get_procedure_parameters (p + off, NULL, NULL, 1);
	if (offp < 0)
	    GDCP_exception_exit ("Unexpected procedure line\n");
	p1 = p + off + offp;
	type = PCC_SEC_PROCEDURE;
    }
    else if (Is_include && strncmp (p, "call", 4) == 0 &&
	     (off = Get_procedure_name (p, "call", tk, MAX_STR_SIZE)) > 0) {
	int i;
	for (i = 0; i < N_prcs; i++) {
	    if (strcmp (Prcs[i].name, tk) == 0)
		break;
	}
	if (i < N_prcs) {
	    int offp = Get_procedure_parameters (p + off, NULL, NULL, 0);
	    if (offp < 0 ||
			MISC_get_token (p + off + offp, "", 0, NULL, 0) > 0)
		GDCP_exception_exit ("Unexpected procedure call line\n");
	    p1 = p + off + offp;
	    type = PCC_SEC_CALL;
	}
    }
    else if ((lc = Get_condition_str (secp)) > 0) {	/* condition line */
	p1 = secp + lc;
	type = PCC_SEC_CONDITION;
    }

    if (p1 == NULL) {
	p = secp + MISC_char_cnt (secp, " \t");
	if (*p == Ccs[COM_C] || *p == Ccs[IGN_C] || *p == Ccs[PAS_C])
	    type = PCC_SEC_COMMENT;
	else if (*p == '\n')
	    type = PCC_SEC_EMPTY;
/*
	else if (*p == Ccs[LB_C] || *p == Ccs[RB_C]) {
	    if (Ignore_error)
		return (PCC_SEC_ILLEGAL);
	    GDCP_exception_exit ("Unexpected bracket %c\n", *p);
	}
*/
	else
	    type = PCC_SEC_LINE;
	p1 = secp;
    }

    t = Get_sect_contents (type, p1, &off, &len);
    if (t < 0)
	return (PCC_SEC_ILLEGAL);
    if (st_off != NULL)
	*st_off = p1 - secp + off;
    if (slen != NULL)
	*slen = len;
    if (tlen != NULL)
	*tlen = p1 - secp + t;

    secp[size] = end_c;			/* restore input buffer */
    return (type);
}

/**********************************************************************

    Verifies that the content of "cp" is "key name (" where name is 
    a qualified variable name. Returns offset after name on success or
    0 on failure. The name is returned with "name" of "size" bytes.

**********************************************************************/

static int Get_procedure_name (char *cp, char *key, char *name, int size) {
    int off, cnt;
    char *p, tk[MAX_STR_SIZE];

    if ((off = GDCM_stoken (cp, "", 0, tk, MAX_STR_SIZE)) <= 0 ||
	strcmp (tk, key) != 0)
	return (0);
    p = cp + off;
    p += MISC_char_cnt (p, " \t");
    cnt = 0;
    while (GDCE_is_identifier (p, 1)) {
	if (cnt < size)
	    name[cnt] = *p;
	cnt++;
	p++;
    }
    off = p - cp;
    p += MISC_char_cnt (p, " \t");
    if (*p != '(')
	return (0);
    if (cnt >= size)
	GDCP_exception_exit ("procedure name too long\n");
    name[cnt] = '\0';
    return (off);    
}

/**********************************************************************

    Checks if "str" is a conditional statement. If it not, it returns 0.
    Otherwise, the number of chars in the conditional phrase is returned.

**********************************************************************/

static int Get_condition_str (char *str) {
    char *p;

    p = str;
    while (*p != '\0' && *p != '\n' && *p != Ccs[CND_C]) 
	p++;
    if (*p != Ccs[CND_C])
	return (0);
    if (Ccs[FORMAT_C] == 'f') {
	char tk[MAX_STR_SIZE];
	if (GDCM_stoken (str, "", 0, tk, MAX_STR_SIZE) <= 0)
	    return (0);
	if (tk[strlen (tk) - 1] == Ccs[CND_C])
	    tk[strlen (tk) - 1] = '\0';
	if (strcmp (tk, "IF") != 0 && strcmp (tk, "ELSEIF") != 0)
	    return (0);
    }
    return (p - str + 1);
}

/**********************************************************************

    Checks if "str" is a loop statement. If it not, it returns 0.
    Otherwise, the number of chars in the loop phrase is returned.

**********************************************************************/

static int Get_loop_str (char *str) {
    char *p, tk[MAX_STR_SIZE], *value, *st;
    int ret, len;

    p = str;
    while (*p != '\0' && *p != '\n' && *p != Ccs[CND_C]) 
	p++;
    if (*p != Ccs[CND_C])
	return (0);
    *p = '\0';
    ret = 0;
    st = str + MISC_char_cnt (str, " \t");
    if (*st == '$')
	st++;
    if (GDCM_stoken (st, "", 0, tk, MAX_STR_SIZE) > 0 &&
	(len = strlen (tk)) > 3 &&
	tk[len - 3] == '[' && tk[len - 2] == '*' && tk[len - 1] == ']' &&
	GDCE_is_identifier (tk, len - 3) &&
	Get_value (tk, &value, NULL) >= 0 &&
	MISC_get_token (st, "", 1, tk, MAX_STR_SIZE) <= 0)
	ret = p - str + 1;

    *p = Ccs[CND_C];
    return (ret);
}

/**********************************************************************

    Checks if "str" is a assignment statement. If it is not, it returns 0.
    Otherwise, the number of chars in the assignment phrase is returned.
    On success, the variable name is put in "buf" of size "buf_size".

**********************************************************************/

static int Get_assign_str (char *str, char *buf, int buf_size) {
    char *p, *st;
    int len;
    char tk[MAX_STR_SIZE];
    int off;

    p = str + MISC_char_cnt (str, " \t");
    if (p[0] == 'S' && p[1] == 'E' && p[2] == 'T' &&
	(off = GDCM_stoken (p, "", 0, tk, MAX_STR_SIZE)) > 0 &&
				    strcmp (tk, "SET") == 0)
	p += off;
    else if (Ccs[FORMAT_C] == 'f')
	return (0);

    st = p;
    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && 
				*p != Ccs[ASN_C])
	p++;
    len = p - st;
    p += MISC_char_cnt (p, " \t");
    if (*p != Ccs[ASN_C] || p <= st)
	return (0);
    if (p[-1] == '!' || p[1] == '=')	/* conditional statement */
	return (0);
    if (!GDCE_is_identifier (st, len))
	return (0);
    if (len >= MAX_STR_SIZE)		/* variable must not be too long */
	return (0);
    if (buf != NULL) {
	if (len >= buf_size)
	    GDCP_exception_exit ("Buffer too short - coding error\n");
	strncpy (buf, st, len);
	buf[len] = '\0';
    }
    return (p - str + 1);
}

/**********************************************************************

    Returns the starting offset (coff) and number of bytes (clen) of
    contents of a section at pointer "text". The contents of a section
    is the text of the current line or quoted by { and } if "{" is
    the first non-" \t\n" char. If all chars after "{" on the same line
    are " \t\n", These chars are discarded. Returns the number of bytes 
    consumed by the section. The section is ended at the next '\n' after
    '}' or before a non-" \t\n" char. Returns -1 on failure.

**********************************************************************/

static int Get_sect_contents (int type, char *text, int *coff, int *clen) {
    char *p;

    if (type == PCC_SEC_CONDITION) {
	int n;
	p = text;
	while ((n = Get_condition_str (p)) > 0)
	    p += n;
	if (p != text &&
	    p[MISC_char_cnt (p, " \t\n")] == Ccs[LB_C]) {
	    *coff = 0;
	    p = Look_for_ending_brace (p + MISC_char_cnt (p, " \t\n"));
	    if (p == NULL)
		return (-1);
	    p++;
	    *clen = p - text;
	    return (p - text);
	}
    }
    if (text[MISC_char_cnt (text, " \t\n")] == Ccs[LB_C] &&
	(type == PCC_SEC_CONDITION || type == PCC_SEC_ASSIGN || 
	 type == PCC_SEC_ELSE || type == PCC_SEC_PROCEDURE || 
	 type == PCC_SEC_LOOP)) {
	char *end, *tp;

	p = text + MISC_char_cnt (text, " \t\n");
	end = Look_for_ending_brace (p);
	if (end == NULL)
	    return (-1);
	p++;
	if (p[MISC_char_cnt (p, " \t")] == '\n')
	    p += MISC_char_cnt (p, " \t") + 1;
	*coff = p - text;

	p = end;
	tp = p - 1;		/* remove empty chars before } from content */
	while (tp >= text + *coff) {
	    if (*tp != ' ' && *tp != '\t')
		break;
	    tp--;
	}
	if (*tp != '\n')
	    tp = p;
	else
	    tp++;
	*clen = tp - text - *coff;
	/* include empty chars after } in section */
	while (*(p = p + 1) != '\0') {
	    if (*p == '\n') {
		p++;
		break;
	    }
	    if (*p != ' ' && *p != '\t')
		break;
	}
	return (p - text);
    }
    p = text;
    while (*p != '\0' && *p != '\n')
	p++;
    if (*p == '\n')
	p++;
    *coff = 0;
    *clen = p - text;
    return (p - text);
}

/**********************************************************************

    Assuming *st == '{' and returns the pointer to ending "}". If the
    ending "}" is not found, the process is terminated if Ignore_error
    is not set. Otherwise it returns NULL.

**********************************************************************/

static char *Look_for_ending_brace (char *st) {
    int level;
    char *p;

    p = st + 1;
    level = 1;
    while (*p != '\0') {
	if (*p == Ccs[LB_C])
	    level++;
	else if (*p == Ccs[RB_C])
	    level--;
	if (level == 0)
	    break;
	p++;
    }
    if (*p != Ccs[RB_C]) {
	if (Ignore_error)
	    return (NULL);
	GDCP_exception_exit ("No matching %c for %c\n", Ccs[RB_C], Ccs[LB_C]);
    }
    return (p);
}

/**********************************************************************

    Gets value for variable named "var". The value, null-terminated, is
    stored in a read-only buffer. The pointer of the buffer is returned
    with "value". If "var" is an array element, the value is the token
    corresponding to the element. For wild-card element, the current
    index is used for the loop variables and the inner loop index for
    others. If the element does not exist, empty string value is
    assumed. It returns the number of elements if "var" is a wild-card
    element, or GV_NOT_WILDCARD otherwise. It returns GV_NOT_FOUND if
    the variable is not defined. "buf" is caller provided buffer of
    MAX_STR_SIZE bytes. If buf == NULL, this function is used for
    returning number of loop counts.

**********************************************************************/

static int Get_value (char *var, char **value, char *buf) {
    int type, ind, cnt;
    char *st, *end, c, tk[MAX_STR_SIZE], *val, cs[32], vname[MAX_STR_SIZE]; 
    enum {NOT_ARRAY, YES_INDEX, NO_INDEX};

    sprintf (cs, " \t\n.%c$", Ccs[CND_C]);
    *value = "";
    type = NOT_ARRAY;
    st = var;
    while (*st != '\0' && *st != '[')
	st++;
    if (*st == '[') {
	end = st + 1;
	while (*end != '\0' && *end != ']')
	    end++;
	if (*end == ']' && 
	    (end[1] == '\0' || MISC_char_cnt (end, cs) > 0)) {
	    *end = '\0';
	    if (st[1] == '*' && end - st == 2)
		type = NO_INDEX;
	    else if (sscanf (st + 1, "%d%c", &ind, &c) == 1)
		type = YES_INDEX;
	    *end = ']';
	    if (strlen (var) >= MAX_STR_SIZE)
		GDCP_exception_exit ("Variable name too long\n");
	    strcpy (vname, var);
	    vname[st - var] = '\0';
	}
    }
    if (type != NOT_ARRAY)
	st[0] = '\0';
    val = GDCV_get_value (var);
    if (type != NOT_ARRAY)
	st[0] = '[';
    if (val == NULL)
	return (GV_NOT_FOUND);

    cnt = 0;
    if (type == NOT_ARRAY)
	*value = val;
    else {
	if (type == NO_INDEX) {
	    int i;
	    for (i = 0; i < N_lpvars; i++) {
		if (strcmp (Lpvars[i].name, vname) == 0) {
		    ind = Lpvars[i].cr_ind;
		    break;
		}
	    }
	    if (i >= N_lpvars) {
		if (buf != NULL) {
		    if (N_lpvars == 0)
			GDCP_exception_exit ("No loop index available\n");
		    ind = Lpvars[N_lpvars - 1].cr_ind;
		}
		else
		    ind = -1;
	    }
	    else if (buf == NULL)
		GDCP_exception_exit (
				"Duplicated use of loop variable %s\n", var);
	}
	if (type == NO_INDEX) {
	    cnt = MISC_get_token (val, "Q\"", 0, NULL, 0);
	}
	if (GDCM_ftoken (val, "Q\"", ind - 1, tk, MAX_STR_SIZE) > 0) {
	    strcpy (buf, tk);
	    *value = buf;
	}
    }
	
    if (type == NO_INDEX)
	return (cnt);
    else
	return (GV_NOT_WILDCARD);
}

/**********************************************************************

    Pushes the current context onto the context stack and intializes the
    context. If new_file is true, the call is a new file. Otherwise, 
    it is a procedure call in the current file.

**********************************************************************/

static void Push_context (int new_file) {
    Context_t *t;

    if (N_contexts >= MAX_CALL_DEPTH)
	GDCP_exception_exit ("Too many levels of procedure calls\n");
    t = Contexts + N_contexts;
    t->cr_loc= Cr_loc;
    t->cr_file = Cr_file;
    t->lpvars = NULL;
    if (N_lpvars > 0) {
	t->lpvars = MISC_malloc (N_lpvars * sizeof (loop_var_t));
	memcpy (t->lpvars, Lpvars, N_lpvars * sizeof (loop_var_t));
    }
    t->n_lpvars = N_lpvars;
    if (new_file) {
	t->prcs = NULL;
	if (N_prcs > 0) {
	    t->prcs = MISC_malloc (N_prcs * sizeof (Procedure_t));
	    memcpy (t->prcs, Prcs, N_prcs * sizeof (Procedure_t));
	}
	t->n_prcs = N_prcs;
	strcpy (t->ccs, Ccs);
	t->field_delimiter = GDCE_get_field_delimiter ();
    }
    else {		/* there values will not be used */
	t->n_prcs = 0;
	t->prcs = NULL;
	t->ccs[0] = '\0';
	t->field_delimiter = '\0';
    }
    t->fout = Fout;
    t->done = Done;
    GDCV_get_var_range (&t->vst_ind, &t->n_vs);
    N_contexts++;

    if (new_file) {
	Cr_loc = NULL;
	Cr_file = NULL;
	strcpy (Ccs, GDC_CONT_CHARS);
	Fout = NULL;
	N_prcs = 0;
	GDCE_set_field_delimiter ('\0');
    }
    N_lpvars = 0;
    Done = 0;
    GDCV_set_var_range (t->vst_ind + t->n_vs, 0);
}

/**********************************************************************

    Pops the latest context from the context stack and intializes the
    context. If new_file is true, the call is a new file. Otherwise, 
    it is a procedure call in the current file.

**********************************************************************/

static void Pop_context (int new_file) {
    Context_t *t;

    if (N_contexts <= 0)
	return;
    t = Contexts + N_contexts - 1;
    Cr_loc = t->cr_loc;
    Remove_loop_vars ();
    N_lpvars = t->n_lpvars;
    if (N_lpvars > 0) {
	memcpy (Lpvars, t->lpvars, N_lpvars * sizeof (loop_var_t));
	free (t->lpvars);
    }
    if (new_file) {
	Cr_file = t->cr_file;
	Remove_procedures ();
 	N_prcs = t->n_prcs;
	if (N_prcs > 0) {
	    memcpy (Prcs, t->prcs, N_prcs * sizeof (Procedure_t));
	    free (t->prcs);
	}
	Fout = t->fout;
	strcpy (Ccs, t->ccs);
	GDCE_set_field_delimiter (t->field_delimiter);
    }
    Done = t->done;
    GDCV_set_var_range (t->vst_ind, t->n_vs);
    N_contexts--;
}
