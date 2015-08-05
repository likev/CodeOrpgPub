
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains the FORTRAN code translating functions.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/05 19:01:37 $
 * $Id: ftc_convert.c,v 1.5 2010/11/05 19:01:37 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

#define MAX_CALL_DEPTH 64	/* max level of calling Process_expression */
#define MAX_DEPTH 32		/* max value of c block levels */

enum {L_INIT, L_PUSH, L_POP, L_READ};	/* values for L_stack */

static int Ffile = -1;
static char *Prev_line = NULL;	/* previoud non-comment line */

static char *L_type;
static int Cr_line = 0;
static char Cr_func[MAX_STR_SIZE] = "";
static char *Out_file = "";	/* Output file name */
static char Prev_out_file[MAX_STR_SIZE] = "";
static int Type_definition = 0;
static int Dim_definition = 0;
static int Is_main = 0;		/* The routine is main */
static int Is_main_file = 0;	/* This file contains main routine */
enum {PL_CONTENT, PL_BLANK, PL_COMMENT};	/* for pl_type */
static int pl_type = PL_CONTENT;	/* type of previoud output line */
static int Code_added = 0;
static int Last_type_line = 0;

typedef struct {	/* for storing a variable replacement */
    int level;		/* effective c block level */
    char *var;		/* variabel to replace */
    char **r_tks;	/* Null-terminated token array */
} Rv_t;

#define MAX_N_RVS 128
static Rv_t Rvs[MAX_N_RVS];	/* Current array of replacement vars */
static int N_rvs;		/* size of Rvs */

enum {VR_RM, VR_ADD1};	/* values for arg "func" of Update_replace_var */

static void Process_a_line (char *buf, int line, char *c);
static char *Get_c_name (char *var);
static char *Get_c_type (char *var);
static void Process_expression (char **tks, int n_tks, char **otb);
static void Process_function (char **tks, int n_tks, char **otb);
static void Process_array (char **tks, int n_tks, char **otb);
static void Process_label (int label, char *c);
static void Process_do (char **tks, int n_tks, char **otb);
static void Process_if (char **tks, int n_tks, char **otb);
static int L_stack (int act, int value, char *var);
static int Prev_short_if ();
static char **Replace_vars (char *var);
static void Update_replace_var (int func, int level, char *var);
static void Init_this_module ();
static char *Get_c_dim (Ident_table_t *id);
static void Process_a_routine (int st_l, int end_l);
static char *Get_c_id (Ident_table_t *id);
static void Add_ftc_code ();
static void Output_a_blank_line ();
static int Is_var_altered_in_block (int label, char *var);
static int Is_var_modified (char *var, char **tks);
static void Output_as_tbd (int line);
static void Output_lines (char *buf, int is_comment);
static int Is_exp (char **tks, int st, int e);


/******************************************************************

    Returns Cr_line + 1.

******************************************************************/

int FC_line () {
    return (Cr_line + 1);
}

/******************************************************************

    Returns Is_main_file.

******************************************************************/

int FC_is_main () {
    return (Is_main_file);
}

/******************************************************************

    Processes a fortran file "in_file" and generates the c "out_file".

******************************************************************/

void FC_process_ffile (char *in_file, char *out_file) {
    static char *inc_files[] = {"rpgcs.h", "a309.h", "coldat.h", "itc.h", "basedata.h", "packet_af1f.h"};
    int n_lines, st_l, end_l;

    Out_file = out_file;
    if (strncmp (in_file + (strlen (in_file) - 2), ".f", 2) != 0) {
	fprintf (stderr, "Unexpected input FORTRAN file name (%s)\n", in_file);
	exit (1);
    }

    printf ("Converting file %s\n", in_file);
    Ffile = FF_open_file (in_file, NULL, &n_lines);

    if (strcmp (out_file, Prev_out_file) != 0) {
	char buf[MAX_STR_SIZE];
	int i;
	sprintf (buf, "    This module is for task %s.\n", FM_get_task_name ());
	FF_outout_file_header (out_file, buf);
	for (i = 0; i < sizeof (inc_files) / sizeof (char *); i++) {
	    sprintf (buf, "#include <%s>\n", inc_files[i]);
	    FF_output_text (out_file, buf, strlen (buf));
	}
	strcpy (buf, "\n/* #define FTC_MAIN */\n");
	FF_output_text (out_file, buf, strlen (buf));
	sprintf (buf, "#include \"%s.h\"\n", FM_get_task_name ());
	FF_output_text (out_file, buf, strlen (buf));
    }
    strcpy (Prev_out_file, out_file);

    st_l = 0;
    while (1) {
	if (st_l >= n_lines)	/* no more functions */
	    break;
	end_l = st_l;
	FE_init ();
	L_type = FS_first_scan (Ffile, &end_l, n_lines);
	Last_type_line = FS_get_value ("Last_type_line");
	Process_a_routine (st_l, end_l);
	FG_check_used_funcs ();
	FI_save_globals ();
	st_l = end_l;
    }
}

/******************************************************************

    Processes a fortran routine "in_file" and generates the c "Out_file".

******************************************************************/

static void Process_a_routine (int st_l, int end_l) {
    char buf[LARGE_STR_SIZE], *prolog, cbuf[LARGE_STR_SIZE], *clines;
    int in_include, cnt, n;

    Init_this_module ();
    Is_main = 0;
    FE_process_eq ();

    prolog = FP_get_prologue (Ffile, st_l, end_l, 0);

    /* FTN to c conversion */
    in_include = 0;		/* the line is from FORTRAN include file */
    pl_type = PL_CONTENT;	/* type of previoud output line */
    Output_lines (NULL, 0);	/* reset C block level */
    Code_added = 0;
    cnt = st_l;			/* line count */
    while (cnt < end_l && 
		(n = FS_get_a_line (cnt, buf, LARGE_STR_SIZE)) > 0) {
	int is_comment, cr_line, line;
/* if (strstr (buf, "exit") != NULL)
exit (0); */

	line = cnt;
	cnt += n;
	clines = buf;

	if (!Code_added && line > Last_type_line)
	    Add_ftc_code ();

	is_comment = 0;
	if (buf[MISC_char_cnt (buf, " \t")] == '\n' ||	/* blank line */
	    (buf[0] != ' ' && 
	    buf[1 + MISC_char_cnt (buf + 1, " \t")] == '\n')) {
	    strcpy (buf, "\n");
	}
	else if (buf[0] != ' ' && !FP_is_digit (buf[0])) {  /* comment line */

	    is_comment = 1;
	    if (strncmp (buf, "CINCLUDE ", 9) == 0)
		in_include = 1;
	    if (in_include) {	/* discard comments from include files */
		if (strncmp (buf, "CINCLUDE - DONE", 15) == 0)
		    in_include = 0;
		continue;
	    }
	    if (strncmp (buf, "*.", 2) == 0)	/* discard MODULE PROLOGUE */
		continue;
	    if (strncmp (buf, "C RCS", 5) == 0 ||
		strncmp (buf, "C $", 3) == 0)	/* discard RCS lines */
		continue;
	    if (buf[2] == '@' && buf[strlen (buf) - 2] == '#') {
		sscanf (buf + 3, "%d", &cr_line);
		continue;			/* line number lable */
	    }
	    if (FP_process_comment (buf, line, 0) == 0)
		continue;
	    FP_str_replace (buf, 0, 1 + MISC_char_cnt (buf + 1, " \t"), "/* ");
	    strcpy (buf + strlen (buf) - 1, " */\n");
	}
	else {

	    if (in_include && L_type[line] != L_OTHERS && 
						L_type[line] != L_DATA)


	    if (in_include && (L_type[line] == L_PARAM || 
		L_type[line] == L_ROUTINE || L_type[line] == L_COMMON || 
		L_type[line] == L_FORMAT))
		continue;

	    if (L_type[line] == L_ROUTINE) {	/* output prologue */
		FF_output_text (Out_file, prolog, strlen (prolog));
		FF_output_text (Out_file, "\n", 1);
	    }
	    Process_a_line (buf, line, cbuf);
	    if (cbuf[0] == '\0')
		continue;
	    Prev_line = STR_copy (Prev_line, buf);
	    clines = cbuf;
	}

	/* output lines in buf */
	Output_lines (clines, is_comment);
    }
}

/******************************************************************

    Outputs each line in "buf". Indentation is performed here.

******************************************************************/

static void Output_lines (char *buf, int is_comment) {
    static int level = 0;	/* C block level */
    char *lp, *next, c;

    if (buf == NULL) {
	level = 0;
	return;
    }

    buf += MISC_char_cnt (buf, " \t");
    lp = buf;
    next = NULL;
    c = 0;
    while (1) {		/* output each line in buf */
	int i;

	if (next != NULL) {
	    lp = next;
	    *lp = c;
	}
	next = lp;
	while (*next != '\0' && *next != '\n')
	    next++;
	if (*next == '\n') {
	    next++;
	    c = *next;
	    *next = '\0';
	}
	if (strlen (lp) == 0)
	    break;

	if (lp[0] == '\n' && (pl_type == PL_BLANK || pl_type == PL_COMMENT))
	    continue;	/* blank line following blank of comment discarded */

	pl_type = PL_CONTENT;		/* set pl_type for future use */
	if (is_comment)
	    pl_type = PL_COMMENT;
	else if (lp[0] == '\n')
	    pl_type = PL_BLANK;

	if (!is_comment && strstr (lp, "}") != NULL)
		level--;		/* upate level */

	/* output the current line */
	if (lp[0] != '\0' && lp[0] != '\n') {
	    for (i = 0; i < level; i++)	/* add space to align comment */
		FF_output_text (Out_file, "    ", 4);
	}
	FF_output_text (Out_file, lp, strlen (lp));

	if (!is_comment && strstr (lp, "{") != NULL)
		level++;		/* upate level */
    }
    return;
}

/******************************************************************

    Processes line "ftn", at line number "line", and returns the c
    code in "c". "ftn" is not modified. If no c code is generated, an
    empty string is returned. The c code can be muitiple lines.

******************************************************************/

static void Process_a_line (char *ftn, int line, char *c) {
    char *tks[MAX_TKS];
    int nt, ind, len, label;

    FM_tk_free ();
    Cr_line = line;
    Type_definition = 0;

    nt = FS_get_ftn_tokens (ftn, MAX_TKS, tks, NULL);
    label = FS_get_cr_lable ();
    if (FR_tb_implemented ()) {
	Output_as_tbd (line);
	c[0] = '\0';
	return;
    }

    if (nt > 0 && Prev_line == NULL && L_type[line] != L_ROUTINE && 
				L_type[line] != L_BLOCK_DATA) {
	strcpy (Cr_func, "program");
	strcpy (c, "int main (int argc, char **argv) {\n");
	Is_main = Is_main_file = 1;
	Output_lines (c, 0);	/* implicit program line */
    }

    if (nt <= 0) {	/* blank line */
	c[0] = '\0';
    }

    else if (strcasecmp (tks[0], "STOP") == 0) {
	c[0] = '\0';
    }

    else if (strcasecmp (tks[0], "IMPLICIT") == 0 &&
	     strcasecmp (tks[1], "NONE") == 0) {
	c[0] = '\0';
    }

    else if (L_type[line] == L_PARAM || L_type[line] == L_COMMON || 
					L_type[line] == L_EQUIV) {
	c[0] = '\0';
    }

    else if (L_type[line] == L_FORMAT) {
	c[0] = '\0';
	label = L_NOT_FOUND;
    }

    else if (L_type[line] == L_DATA || L_type[line] == L_BLOCK_DATA) {
	Ident_table_t *id;
	int i, used;
	used = 0;
	for (i = 0; i < nt; i++) {
	    if ((id = FS_var_search (tks[i])) != NULL) {
		FU_update_tk (tks + i, Get_c_id (id));
		if (id->prop & P_USED)
		    used = 1;
	    }
	}
	c[0] = '\0';
	if (used || L_type[line] == L_BLOCK_DATA) {
	    tks[nt] = NULL;
	    strcpy (c, "/* TBD: ");
	    strcat (c, FU_get_c_text (tks, 0));
	    strcat (c, " */");
	}
    }

    else if (strcasecmp (tks[0], "END") == 0) {
	if (Is_main)
	    strcpy (c, "exit (0);\n}");
	else
	    strcpy (c, "}");
    }

    else if (L_type[line] == L_ROUTINE) {

	Type_definition = 1;
	if (Cr_func[0] != '\0')
	    FS_exception_exit ("Second routine found in the file\n");

	if (strcasecmp (tks[0], "PROGRAM") == 0) {
	    strcpy (Cr_func, "program");
	    strcpy (c, "int main (int argc, char **argv) {");
	    Is_main = Is_main_file = 1;
	}
	else {
	    char *name, *type;
	    int cnt;

	    if (strcasecmp (tks[0], "SUBROUTINE") == 0)
		ind = 1;
	    else {
		ind = 1;
		while (ind < nt && strcasecmp (tks[0], "FUNCTION") != 0)
		    ind++;
	    }
	    name = NULL;
	    if (ind >= nt || (name = Get_c_name (tks[ind])) == NULL)
		FS_exception_exit ("Bad SUBROUTINE/FUNCTION definition line\n");
	    strcpy (Cr_func, tks[ind]);
	    type = Get_c_type (tks[ind]);
	    sprintf (c, "%s %s (", type, name);
	    cnt = 0;
	    for (ind = ind + 2; ind < nt - 1; ind++) {
		if (tks[ind][0] == ',')
		    strcat (c, ", ");
		else {
		    Ident_table_t *id = FS_var_search (tks[ind]);
		    if (id == NULL)
			FS_exception_exit ("Arg %s not defined\n", tks[ind]);
		    strcat (c, FU_get_arg_c_type (id, Cr_func, cnt, NULL));
		    if (id->type != T_CHAR && id->ndim == 0 && 
				FG_is_pb_reference (Cr_func, cnt, NULL) == 1)
			id->prop |= P_BY_REF;
		    cnt++;
		}
	    }
	    strcat (c, ") {");
	}
    }

    else if (L_type[line] == L_TYPE) {

	Type_definition = 1;
	c[0] = '\0';
	ind = FS_get_size_dim (tks, nt, 1, NULL, NULL, NULL, NULL) + 1;
	if (ind < nt) {
	    int cnt = 0;
	    while (ind < nt) {
		int end;
		Ident_table_t *id;
		if ((id = FS_var_search (tks[ind])) == NULL)
		    FS_exception_exit ("TYPE: %s not defined\n", tks[ind]);
		end = FU_next_token (ind, nt, tks, ',');
		if (!((id->prop & P_PARAM) && !(id->prop & P_PBR_PARM)) &&
		    id->arg_i < 0 &&
		    !(id->prop & (P_FUNC_DEF | P_FUNC_CALL)) && 
		    (id->prop & P_USED) && 
		    !(id->prop & P_COMMON)) {

		    if (cnt == 0)
			sprintf (c, "%s ", FU_get_c_type (id));
		    else 
			strcat (c, ", ");
/*
		    Process_expression (tks + ind, end - ind, tb);
		    strcat (c, FU_get_c_text (tb, 0));
*/
		    strcat (c, Get_c_dim (id));
		    cnt++;
		}
		ind = end + 1;
	    }
	    if (cnt > 0)
		strcat (c, ";");
	}
    }

    else if (strcasecmp (tks[0], "DO") == 0) {
	char *tb[MAX_TKS];
	Process_do (tks, nt, tb);
	strcpy (c, FU_get_c_text (tb, 0));
    }

    else if (strcasecmp (tks[0], "WRITE") == 0) {
	char *tb[MAX_TKS];
	if (FW_process_write (tks, nt, tb) == 0)
	    strcpy (c, FU_get_c_text (tb, 0));
	else {
	    char *err = FW_get_err_msg ();
	    sprintf (c, "/* TBD: %s", ftn);
	    if (c[strlen (c) - 1] == '\n')
		c[strlen (c) - 1] = '\0';
	    if (err[0] != '\0')
		sprintf (c + strlen (c), " (%s)", err);
	    strcat (c, " */\n");
	}
    }

    else if (strcasecmp (tks[0], "IF") == 0 ||
	     strcasecmp (tks[0], "ELSEIF") == 0 ||
	     strcasecmp (tks[0], "ELSE") == 0 ||
	     strcasecmp (tks[0], "ENDIF") == 0) {
	char *tb[MAX_TKS];
	Process_if (tks, nt, tb);
	strcpy (c, FU_get_c_text (tb, 0));
    }

    else if (strcasecmp (tks[0], "CONTINUE") == 0)
	c[0] = '\0';

    else {
	char *tb[MAX_TKS], *tk;
	Process_expression (tks, nt, tb);
	tk = ";";
	FU_replace_tks (tb, -1, 0, &tk, 1);
	strcpy (c, FU_get_c_text (tb, 0));
    }

    if (label != L_NOT_FOUND)
	Process_label (label, c);

    len = strlen (c);
    if (c[len - 1] != '\n') {
	c[len] = '\n';
	c[len + 1] = '\0';
    }
}

/******************************************************************

    Outputs a blank line if the previous line is not blank.

******************************************************************/

static void Output_a_blank_line () {
    if (pl_type != PL_BLANK) {
	char b[8];
	pl_type = PL_CONTENT;	/* force a blank line */
	strcpy (b, "\n");
	Output_lines (b, 0);
    }
}

/******************************************************************

    Outputs FTC generated c code to the output file.

******************************************************************/

static void Add_ftc_code () {
    char b[128], *code;
    int n;
    Ident_table_t *id;

    Code_added = 1;
    Output_a_blank_line ();

    code = NULL;
    n = FE_get_eq_code (&code);
    if (n > 0) {
	strcpy (b, "/* ftc-code for EQUIVALENCEs */\n");
	Output_lines (b, 0);
	Output_lines (code, 0);
    }
    STR_free (code);
    Output_a_blank_line ();

    /* Initializing P_PBR_PARM variables */
    code = NULL;
    FS_var_search (FVS_RESET);
    n = 0;
    code = STR_copy (code, "");
    while ((id = FS_var_search (FVS_NEXT)) != NULL) {
	if (!(id->prop & P_PBR_PARM))
	    continue;
	if (id->type == T_CHAR)		/* should call str_op - do it later */
	    sprintf (b, "/* TBD: %s = %s", id->c_id, id->c_id);
	else
	    sprintf (b, "%s = %s", id->c_id, id->c_id);
	b[strlen (b) - 1] = '\0';	/* remove _ */
	if (id->type == T_CHAR)
	    strcat (b, " */");
	strcat (b, ";\n");
	code = STR_cat (code, b);
	n++;
    }
    if (n > 0) {
	strcpy (b, "/* ftc-code for init pass-by-reference MACROs */\n");
	Output_lines (b, 0);
	Output_lines (code, 0);
    }
    STR_free (code);
    Output_a_blank_line ();
}

/******************************************************************

    Global interface for Process_expression.

******************************************************************/

void FC_process_expr (char **tks, int n_tks, char **otb, int def) {

    int sd = Type_definition;	/* save */
    Type_definition = def;
    Process_expression (tks, n_tks, otb);
    Type_definition = sd;	/* restore */
}

/******************************************************************

    Processes FTN expression of "tks" of "n_tks" tokens. The output is
    put in "otb" and is NULL-terminated token array. "tks" is not
    modified. The processing includes: Identifier replacement,
    function paramter procecing, and array index processing (position
    and starting index).

******************************************************************/

static void Process_expression (char **tks, int n_tks, char **otb) {
    static int l_call = 0;
    char *tb[MAX_TKS];
    int i;

    l_call++;
    if (l_call > MAX_CALL_DEPTH)
	FS_exception_exit ("Too many levels of calling Process_expression\n");

    if (n_tks <= 0) {
	n_tks = 0;
	while (tks[n_tks] != NULL)
	    n_tks++;
    }
    for (i = 0; i < n_tks; i++)
	otb[i] = tks[i];
    otb[n_tks] = NULL;

    for (i = n_tks - 1; i >= 0; i--) {
	int st;
	Ident_table_t *id;

	st = i;
	if (tks[i][0] == ')') {
	    int c = 0;
	    st = i - 1;
	    while (st >= 0) {
		if (c == 0 && tks[st][0] == '(')
		    break;
		if (tks[st][0] == ')')
		    c++;
		else if (tks[st][0] == '(')
		    c--;
		st--;
	    }
	    st--;
	    if (st < 0)
		st = i;
	}

	if (FP_is_char (tks[st][0]) && 
	    (id = FS_var_search (tks[st])) != NULL) {
	    if (st != i) {
		if (id->prop & P_FUNC_CALL) {
		    Process_function (tks + st, i - st + 1, tb);
		    FU_replace_tks (otb, st, i - st + 1, tb, 0);
		    i = st;
		    continue;
		}
		else if (id->ndim > 0) {
		    Process_array (tks + st, i - st + 1, tb);
		    FU_replace_tks (otb, st, i - st + 1, tb, 0);
		    i = st;
		    continue;
		}
	    }
	    else {		/* a variable */
		char **vars;
		FU_update_tk (otb + st, Get_c_id (id));
		if ((vars = Replace_vars (otb[st])) != NULL)
		    FU_replace_tks (otb, st, 1, vars, 0);
	    }
	}

	if (i > 0 && strcasecmp (tks[i - 1], "GOTO") == 0) {
	    char buf[64];
	    strcpy (buf, "L");
	    strncat (buf, tks[i], 62);
	    FU_update_tk (otb + i, buf);
	    FU_update_tk (otb + i - 1, "goto");
	}

	if (strcasecmp (tks[i], "CALL") == 0) {
	    FU_replace_tks (otb, i, 1, NULL, 0);
	    if (otb[i] != NULL && 
			(otb[i + 1] == NULL || otb[i + 1][0] != '(')) {
		FU_ins_tks (otb, i + 1, "(", ")", NULL);
	    }
	}
    }
    if (l_call == 1)
	FU_post_process_c_tks (otb);
    l_call--;
}

/******************************************************************

    Returns the replacement token array for variable "var" or NULL
    if "var" does not need to be replaced. We may need to replace
    variables for meeting c indexing scheme and for supporting 
    equivalence.

******************************************************************/

static char **Replace_vars (char *var) {
    int i;

    for (i = 0; i < N_rvs; i++) {
	if (strcmp (Rvs[i].var, var) == 0)
	    return (Rvs[i].r_tks);
    }

    return (NULL);
}

/******************************************************************

    Processes FTN DO line of "tks" of "n_tks" tokens. The output
    is put in "otb" and is NULL-terminated. "tks" is not modified.

******************************************************************/

static void Process_do (char **tks, int n_tks, char **otb) {
    char *tb[MAX_TKS], *tk, varb[MAX_STR_SIZE], *var;
    int lbl, st, st1, end1, st2, end2, st3, end3, change_var;
    Ident_table_t *id;

    otb[0] = NULL;

    st = 2;
    if (n_tks >= 6 &&
	strcasecmp (tks[0], "DO") == 0 &&
	tks[2][0] == '=') {
	lbl = L_ENDDO;
	st = 1;
    }
    else if (n_tks < 7 ||
	strcasecmp (tks[0], "DO") != 0 ||
	sscanf (tks[1], "%d", &lbl) != 1 ||
	tks[3][0] != '=')
	FS_exception_exit ("Unexpected DO line\n");

    if ((id = FS_var_search (tks[st])) == NULL)
	FS_exception_exit ("Bad DO line - not a variable\n");
    change_var = 0;
    if (id->type == T_INT && id->ndim == 0)
	change_var = 1;
    if (Is_var_altered_in_block (lbl, tks[2]) != 0)
	change_var = 0;
    strcpy (varb, Get_c_id (id));
    var = varb;

    if (change_var)
	L_stack (L_PUSH, lbl, var);
    else
	L_stack (L_PUSH, lbl, NULL);

    FU_ins_tks (otb, -1, "for", "(", var, "=", NULL);

    st1 = st + 2;
    end1 = FU_next_token (st1, n_tks, tks, ',');
    st2 = end1 + 1;
    end2 = FU_next_token (st2, n_tks, tks, ',');
    st3 = end2 + 1;
    end3 = FU_next_token (st3, n_tks, tks, ',');
    if (st1 >= n_tks || st2 >= n_tks)
	FS_exception_exit ("Unexpected DO line - less tokens found\n");
    Process_expression (tks + st1, end1 - st1, tb);
    if (change_var)
	FU_ins_tks (tb, -1, "-", "1",NULL);

    FU_post_process_c_tks (tb);
    FU_replace_tks (otb, -1, 0, tb, 0);
    FU_ins_tks (otb, -1, ";", var, NULL);
    if (change_var)
	tk = "<";
    else
	tk = "<=";
    FU_replace_tks (otb, -1, 0, &tk, 1);
    Process_expression (tks + st2, end2 - st2, tb);
    FU_replace_tks (otb, -1, 0, tb, 0);
    FU_ins_tks (otb, -1, ";", NULL);

    FU_replace_tks (otb, -1, 0, &var, 1);
    if (st3 < n_tks) {
	FU_ins_tks (otb, -1, "+=", NULL);
	Process_expression (tks + st3, end3 - st3, tb);
	FU_replace_tks (otb, -1, 0, tb, 0);
    }
    else
	FU_ins_tks (otb, -1, "++", NULL);
    FU_ins_tks (otb, -1, ")", "{", NULL);
}

/******************************************************************

    Processes FTN IF and ENDIF line of "tks" of "n_tks" tokens. The
    output is put in "otb" and is NULL-terminated. "tks" is not modified.

******************************************************************/

static void Process_if (char **tks, int n_tks, char **otb) {
    char *tb[MAX_TKS];

    otb[0] = NULL;
    if (strcasecmp (tks[0], "ENDIF") == 0) {
	FU_ins_tks (otb, -1, "}", NULL);
	L_stack (L_POP, L_ENDIF, NULL);
    }
    else if (strcasecmp (tks[0], "ELSE") == 0) {
	if (!Prev_short_if ()) {
	    FU_ins_tks (otb, -1, "}", "\n", NULL);
	    L_stack (L_POP, L_ENDIF, NULL);
	}
	FU_ins_tks (otb, -1, "else", NULL);
	if (n_tks > 1) {
	    FU_ins_tks (otb, -1, "\n    ", NULL);
	    Process_expression (tks + 1, n_tks - 1, tb);
	    FU_replace_tks (otb, -1, 0, tb, 0);
	    FU_ins_tks (otb, -1, ";", NULL);
	}
	else {
	    FU_ins_tks (otb, -1, "{", NULL);
	    L_stack (L_PUSH, L_ENDIF, NULL);
	}
    }
    else {
	int ind, st, end;
	ind = 0;
	if (strcasecmp (tks[0], "ELSEIF") == 0) {
	    /* we do not need if (!Prev_short_if ()) { because elseif can 
	       only follow if ... then */
	    FU_ins_tks (otb, -1, "}", "\n", NULL);
	    L_stack (L_POP, L_ENDIF, NULL);
	    ind = 2;

	    FU_ins_tks (otb, -1, "else", "if", "(", NULL);
	    ind++;
	}
	else {
	    FU_ins_tks (otb, -1, "if", "(", NULL);
	    ind++;
	}
	st = 2;
	end = FU_next_token (st, n_tks, tks, ')');
	Process_expression (tks + st, end - st, tb);
	if (end >= n_tks - 1 ||
	   (end + 1 < n_tks && strcasecmp (tks[end + 1], "THEN") == 0)) {
	    FU_replace_tks (otb, -1, 0, tb, 0);
	    FU_ins_tks (otb, -1, ")", NULL);
	    FU_ins_tks (otb, -1, "{", NULL);
	    L_stack (L_PUSH, L_ENDIF, NULL);
	}
	else {
	    FU_replace_tks (otb, -1, 0, tb, 0);
	    FU_ins_tks (otb, -1, ")", NULL);
	    FU_ins_tks (otb, -1, "\n    ", NULL);
	    Process_expression (tks + end + 1, n_tks - end - 1, tb);
	    FU_replace_tks (otb, -1, 0, tb, 0);
	    FU_ins_tks (otb, -1, ";", NULL);
	}
    }
}

/******************************************************************

    Processes FTN LABEL of value "label". c Is the processed c line.
    c is modified for output.

******************************************************************/

static void Process_label (int label, char *c) {

    while (L_stack (L_READ, label, NULL) == label) {
	int len = strlen (c);
	if (len > 0 && c[len - 1] == '\n')
	    c[len - 1] = '\0';
	if (strlen (c) > 0)
	    strcat (c, "\n}");
	else
	    strcat (c, "}");
	L_stack (L_POP, label, NULL);
    }
    if (FS_is_goto_label (label)) {
	char buf[64];
	sprintf (buf, "L%d:\n", label);
	FP_str_replace (c, 0, 0, buf);
    }
}

/******************************************************************

    Returns true if the tks from st to e - 1 is an expresion (not
    a array element).

******************************************************************/

static int Is_exp (char **tks, int st, int e) {
    int s, e1;

    while (tks[st][0] == '(' && tks[e - 1][0] == ')') {
	st++;
	e--;
    }
    if (e - st <= 2)
	return (0);
    if (tks[st][0] == '(')
	s = st;
    else if (tks[st + 1][0] == '(')
	s = st + 1;
    else
	return (1);
    e1 = FU_next_token (s + 1, e, tks, ')');
    if (e1 < e - 1)
	return (1);
    return (0);
}

/******************************************************************

    Processes FTN function of "tks" of "n_tks" tokens. The output
    is put in "otb" and is NULL-terminated. "tks" is not modified.

******************************************************************/

static void Process_function (char **tks, int n_tks, char **otb) {
    Ident_table_t *id;
    char *tb[MAX_TKS];
    int st, end, cnt;

    id = FS_var_search (tks[0]);
    if (id == NULL || !(id->prop & P_FUNC_CALL))
	FS_exception_exit ("Function name (%s) not defined\n", tks[0]);

    otb[0] = NULL;
    Process_expression (tks, 1, tb);
    FU_replace_tks (otb, -1, 0, tb, 0);
    st = 1;
    if (st < n_tks && tks[st][0] != '(')
	return;
    FU_ins_tks (otb, -1, "(", NULL);

    st++;
    end = FU_next_token (st, n_tks, tks, ')');
    if (end >= n_tks)
	FS_exception_exit ("Unclosed '(' for func (%s)\n", tks[0]);

    cnt = 0;
    while (st < end) {
	int e, pb_ref, is_ref, is_array_name, is_exp;
	char type[MAX_STR_SIZE];

	e = FU_next_token (st, end, tks, ',');
	if (e > end)
	    e = end;
	if (e <= st)
	    break;

	if (cnt > 0)
	    FU_ins_tks (otb, -1, ",", NULL);

	pb_ref = FG_is_pb_reference (tks[0], cnt, type);
	if (pb_ref < 0)		/* not found - We assume passby ref */
	    pb_ref = 1;

	if (e - st >= 3)
	    is_exp = Is_exp (tks, st, e);

	is_ref = -1;	/* constants, expression and so on */
	id = NULL;
	if (e == st + 1 || !is_exp)
	    id = FS_var_search (tks[st]);
	is_array_name = 0;
	if (id != NULL) {
	    is_ref = 0;
	    if (id->arg_i >= 0 && Cr_func[0] != '\0' && e == st + 1) {
		is_ref = FG_is_pb_reference (Cr_func, id->arg_i, NULL);
		if (is_ref < 0)	/* not found - We assume passby ref */
		    is_ref = 1;
	    }
	    if ((id->ndim > 0 || id->type == T_CHAR) && e == st + 1) {
					/* pass by array name */
		is_ref = 1;
		is_array_name = 1;
	    }
	    if ((id->prop & P_CVT_POINT) && id->type != T_CHAR && 
				id->ndim == 0)
		is_ref = 1;

	    {	/* add casting to the argument */
		char *ltype, *tb[MAX_TKS];
		if (id->prop & P_CVT_POINT) {
		    id->prop &= ~P_CVT_POINT;
		    ltype = FC_get_ctype_dec (id);
		    id->prop |= P_CVT_POINT;
		}
		else
		    ltype = FC_get_ctype_dec (id);
		tb[0] = NULL;
		if (type[0] != '\0')
		    FU_get_call_cast (type, ltype, is_array_name, tb);
		if (tb[0] != NULL)
		    FU_replace_tks (otb, -1, 0, tb, 0);
	    }
	}

	if (!is_array_name && !is_exp) {
	    if (is_ref == 1 && !pb_ref)
		FU_ins_tks (otb, -1, "*", NULL);
	    else if (is_ref == 0 && pb_ref)
		FU_ins_tks (otb, -1, "&", NULL);
	}

	Process_expression (tks + st, e - st, tb);
	FU_replace_tks (otb, -1, 0, tb, 0);

	if (tks[e][0] != ',')
	    break;
	cnt++;
	st = e + 1;
    }
    FU_ins_tks (otb, -1, ")", NULL);
}

/******************************************************************

    Processes FTN array of "tks" of "n_tks" tokens. The output
    is put in "otb" and is NULL-terminated. "tks" is not modified.

******************************************************************/

static void Process_array (char **tks, int n_tks, char **otb) {
    Ident_table_t *id;
    char *tb[MAX_TKS], *btks[MAX_TKS];
    int st, bst, bnt, i;

    id = FS_var_search (tks[0]);
    if (id == NULL)
	FS_exception_exit ("Array name (%s) not defined\n", tks[0]);

    otb[0] = NULL;
    Process_expression (tks, 1, tb);
    FU_replace_tks (otb, -1, 0, tb, 0);
    st = 1;
    bst = bnt = 0;
    if (id->ndim > 0) {
	if (n_tks < 3 || tks[1][0] != '(')
	    FS_exception_exit ("Missing ( after array name\n");
	st++;

	if (id->prop & P_LOW_BOUND) {
	    bnt = FP_get_tokens (id->dimp, MAX_TKS, btks, NULL);
	    bst = 1;
	}
    }
    for (i = 0; i < id->ndim; i++) {
	int end, bend;
	if (i < id->ndim - 1)
	    end = FU_next_token (st, n_tks, tks, ',');
	else
	    end = FU_next_token (st, n_tks, tks, ')');
	if (end == st || end >= n_tks)
	    FS_exception_exit ("Missing %d-th index expression\n", i + 1);
	Dim_definition = 1;
	Process_expression (tks + st, end - st, tb);
	Dim_definition = 0;

	FU_ins_tks (otb, 1, "]", NULL);
	bend = -1;
	if (!Type_definition) {	/* array element in statment */
	    char *btb[MAX_TKS];
	    int sep;

	    sep = -1;	/* low-up bounds separator */
	    if (id->prop & P_LOW_BOUND) {	/* find low dim bound */
		int k;
		if (i < id->ndim - 1)
		    bend = FU_next_token (bst, bnt, btks, ',');
		else
		    bend = FU_next_token (bst, bnt, btks, ')');
		if (bend == bst || bend >= bnt)
		    FS_exception_exit ("Missing %d-th dim expression\n", i + 1);
		for (k = bst; k < bend; k++) {
		    if (btks[k][0] == ':') {
			sep = k;
			if (bend - bst - 1 <= 0)
			    FS_exception_exit ("Missing upper bound\n");
			Dim_definition = 1;
			Process_expression (btks + bst, sep - bst, btb);
			Dim_definition = 0;
			break;
		    }
		}
	    }

	    if (sep < 0 || sep == bst)	/* low bound not specified */
		FU_ins_tks (otb, 1, "-", "1", NULL);
	    else {
		FU_ins_tks (otb, 1, ")", NULL);
		FU_replace_tks (otb, 1, 0, btb, 0);
		FU_ins_tks (otb, 1, "-", "(", NULL);
	    }
	    FU_replace_tks (otb, 1, 0, tb, 0);
	}
	else {		/* array in definition */
	    int sep, k;
	    sep = -1;	/* low-up bounds separator */
	    for (k = st; k < end; k++) {
		if (tks[k][0] == ':') {
		    sep = k;
		    if (end - sep - 1 <= 0)
			FS_exception_exit ("Missing upper bound\n");
		    break;
		}
	    }
	    if (sep < 0 || sep == st)	/* default (1) low bound */
		FU_replace_tks (otb, 1, 0, tb, 0);
	    else {
		FU_ins_tks (otb, 1, ")", "+", "1", NULL);
		FU_replace_tks (otb, 1, 0, tks + st, sep - st);
		FU_ins_tks (otb, 1, ")", "-", "(", NULL);
		FU_replace_tks (otb, 1, 0, tks + sep + 1, end - sep - 1);
		FU_ins_tks (otb, 1, "(", NULL);
	    }
	}
	FU_ins_tks (otb, 1, "[", NULL);
	st = end + 1;
	bst = bend + 1;
    }
    if (st != n_tks)
	FS_exception_exit ("Trailing token in array\n");
}

/******************************************************************

    Returns the ascii c type name of variable "var". Returns NULL
    if the variable is not defined.

******************************************************************/

static char *Get_c_type (char *var) {
    Ident_table_t *id;

    if ((id = FS_var_search (var)) == NULL)
	return (NULL);
    return (FU_get_c_type (id));
}

/******************************************************************

    Returns c type declarison for variable "id".

******************************************************************/

char *FC_get_ctype_dec (Ident_table_t *id) {
    static char buf[MAX_STR_SIZE];
    int prev = Type_definition;

    Type_definition = 1;
    sprintf (buf, "%s %s", FU_get_c_type (id), Get_c_dim (id));
    Type_definition = prev;
    return (buf);
}

/******************************************************************

    Returns the c code for variable "id" and its array discription.

******************************************************************/

static char *Get_c_dim (Ident_table_t *id) {
    static char buf[MAX_STR_SIZE];
    char *c_id;

    c_id = id->c_id;
    if (c_id == NULL)
	c_id = id->id;
    if (id->size == -1 || (id->dimp != NULL && strcmp (id->dimp, "(*)") == 0))
	sprintf (buf, "*%s", c_id);
    else if (id->ndim == 0) {
	if (id->type == T_CHAR) {
	    if (id->size == -1) 
		sprintf (buf, "%s[*]", c_id);
	    else
		sprintf (buf, "%s[%d]", c_id, id->size);
	}
	else
	    strcpy (buf, c_id);
    }
    else {
	char *tb[MAX_TKS];
	char *tks[MAX_TKS];
	int nt;

	if (id->dimp == NULL)
	    FS_exception_exit ("Missing dim descrip (%s)\n", id->id);
	tks[0] = id->id;
	nt = FP_get_tokens (id->dimp, MAX_TKS - 1, tks + 1, NULL) + 1;
	Process_expression (tks, nt, tb);
	if (id->type == T_CHAR) {
	    char as[32];
	    FU_ins_tks (tb, -1, "[", NULL);
	    if (id->size >= 0)
		sprintf (as, "%d", id->size);
	    else
		strcpy (as, "*");
	    FU_ins_tks (tb, -1, as, "]", NULL);
	}
	strcpy (buf, FU_get_c_text (tb, 0));
    }
    if (FE_get_pad_size (id) > 0) {
	char *p = buf;
	while (*p != '\0' && *p != ']')
	    p++;
	if (*p == ']') {
	    char  b[64];
	    sprintf (b, " + %d", FE_get_pad_size (id));
	    FP_str_replace (buf, p - buf, 0, b);
	}
    }
    if (id->prop & P_CVT_POINT)
	FE_change_var_def (buf);
   return (buf);
}

/******************************************************************

    Returns the ascii c name of variable "var". Returns NULL
    if the variable is not defined.

******************************************************************/

static char *Get_c_name (char *var) {
    Ident_table_t *id;

    if ((id = FS_var_search (var)) == NULL)
	return (NULL);
    return (Get_c_id (id));
}

/******************************************************************

    Returns the c name of variable "var" used in the code.

******************************************************************/

static char *Get_c_id (Ident_table_t *id) {
    static char buf[128];

    if (id->c_id != NULL) {
	strcpy (buf, id->c_id);
	if ((id->prop & P_PARAM) && Dim_definition && Type_definition && (id->prop & P_PBR_PARM))
	    buf[strlen (buf) - 1] = '\0';	/* remove "_" */
    }
    else
	strcpy (buf, id->id);
    if (!Type_definition && (id->prop & P_COMMON)) {
	FP_str_replace (buf, 0, 0, ".");
	FP_str_replace (buf, 0, 0, FU_common_struct_name (id->common));
    }
    if ((id->prop & P_BY_REF) ||
	((id->prop & P_CVT_POINT) && id->type != T_CHAR && id->ndim == 0))
	FP_str_replace (buf, 0, 0, "*");
    return (buf);
}

/******************************************************************

    Returns the true if the previous FORTRAN line is a short IF..

******************************************************************/

static int Prev_short_if () {
    char *tks[MAX_TKS];
    int nt;

    nt = FS_get_ftn_tokens (Prev_line + 6, MAX_TKS, tks, NULL);
    if (nt > 1 && 
	(strcasecmp (tks[0], "IF") == 0 || 
			strcasecmp (tks[0], "ELSEIF") == 0) && 
	strcasecmp (tks[nt - 1], "THEN") != 0 && 
	strcasecmp (tks[nt - 1], ")") != 0)
	return (1);
    if (nt == 1 && 
	strcasecmp (tks[0], "ELSE") == 0)
	return (1);
    return (0);
}

/******************************************************************

    Processes the label stack.

******************************************************************/

static int L_stack (int act, int value, char *var) {
    static int stack[MAX_DEPTH], ind = 0;

    if (act == L_INIT) {
	ind = 0;
    }
    else if (act == L_PUSH) {
	if (ind >= MAX_DEPTH) {
	    FS_exception_exit ("Label stack overflow\n");
	    exit (1);
	}
	stack[ind] = value;
	ind++;
	if (var != NULL)
	    Update_replace_var (VR_ADD1, ind, var);
    }
    else if (act == L_POP) {
	if (ind <= 0 || stack[ind - 1] != value) {
	    if (ind <= 0)
		fprintf (stderr, "Label stack underflow\n");
	    else
		fprintf (stderr, "Label stack POP - bad value (%d %d)\n",
					stack[ind - 1], value);
	    exit (1);
	}
	Update_replace_var (VR_RM, ind, NULL);
	ind--;
    }
    else if (act == L_READ) {
	if (ind <= 0)
	    return (L_NOT_FOUND);
	return (stack[ind - 1]);
    }
    return (0);
}

/******************************************************************

    Updates the replacement variable array "Rvs".

******************************************************************/

static void Update_replace_var (int func, int level, char *var) {

    if (func == VR_RM) {
	int oi, ii;
	oi = ii = 0;
	while (ii < N_rvs) {
	    if (Rvs[ii].level >= level) {
		free (Rvs[ii].r_tks);
		Rvs[ii].r_tks = NULL;
	    }
	    else {
		Rvs[oi] = Rvs[ii];
		oi++;
	    }
	    ii++;
	}
	N_rvs = oi;
    }
    else if (VR_ADD1) { /* replacing var with var + 1 */
	int len;
	char *p;

	if (N_rvs >= MAX_N_RVS) {
	    fprintf (stderr, "Too many replacement variables\n");
	    exit (1);
	}
	len = strlen (var);
	p = MISC_malloc (4 * sizeof (char *) + 2 * len + 16);
	Rvs[N_rvs].r_tks = (char **)p;
	p += 4 * sizeof (char *);
	Rvs[N_rvs].var = (char *)p;
	strcpy (p, var);
	Rvs[N_rvs].r_tks[0] = p;
	p += len + 1;
	Rvs[N_rvs].r_tks[1] = p;
	strcpy (p, "+");
	p += 2;
	Rvs[N_rvs].r_tks[2] = p;
	strcpy (p, "1");
	Rvs[N_rvs].r_tks[3] = NULL;
	Rvs[N_rvs].level = level;
	N_rvs++;
    }
}

/******************************************************************

    Searches to determine if "var" is modified in the code line of
    "tks". Modifying array element does not considered as modifying
    the variable.

******************************************************************/

static int Is_var_modified (char *var, char **tks) {

    int ind = 0;
    while (tks[ind] != NULL) {
	Ident_table_t *id;

/*	if (ind > 0 && tks[ind][0] == '=') {
	    int st, c;
	    st = ind - 1;
	    c = 0;
	    while (st >= 0) {
		if (tks[st][0] == ')')
		    c++;
		else if (tks[st][0] == '(')
		    c--;
		else if (c == 0 && strcasecmp (tks[st], var) == 0)
		    return (1);
		st--;
	    }
	} */
	if (ind > 0 && tks[ind][0] == '=' &&
				strcasecmp (tks[ind - 1], var) == 0)
	    return (1);
	if (ind > 0 && tks[ind][0] == '(' &&
	    (id = FS_var_search (tks[ind - 1])) != NULL &&
	    (id->prop & P_FUNC_CALL)) {
	    int n_tks, end, st;

	     n_tks = 0;
	     while (tks[n_tks] != NULL)
		n_tks++;
	     st = ind + 1;
	     end = FU_next_token (ind + 1, n_tks, tks, ')');
	     while (st < end) {
		int e = FU_next_token (st, end, tks, ',');
		if (strcasecmp (tks[st], var) == 0)
		    return (1);
		st = e + 1;
	     }
	}
	ind++;
    }
    return (0);
}

/******************************************************************

    Starting with the next line until the line of label "label",
    searches to determine if "var" is modified or never used in the
    block of code. Returns 1 if altered, -1 if never used, 0 otherwise.

******************************************************************/

static int Is_var_altered_in_block (int label, char *var) {
    char buf[MAX_STR_SIZE];
    int used, cnt, n;

    used = 0;
    cnt = FC_line ();
    while ((n = FS_get_a_line (cnt, buf, MAX_STR_SIZE)) > 0) {
	int nt, lbl, ind;
	char *tks[MAX_TKS];

	cnt += n;
	if (buf[0] != ' ')	/* a comment line */
	    continue;

	nt = FS_get_ftn_tokens (buf, MAX_TKS, tks, NULL);
	lbl = FS_get_cr_lable ();
	tks[nt] = NULL;

	if (Is_var_modified (var, tks))
	    return (1);
	ind = 0;
	while (tks[ind] != NULL) {
	    if (strcasecmp (tks[ind], var) == 0)
		used = 1;
	    ind++;
	}
	if (lbl == label)
	    break;
    }
    if (!used)
	return (-1);
    return (0);
}

/******************************************************************

    Output the current FTN line (of line number "line") as a 
    to-be-done line. We reread the line to make sure the original 
    FTN line is presented to the output. 

******************************************************************/

static void Output_as_tbd (int line) {
    char buf[LARGE_STR_SIZE], *p;

    FS_get_a_line (line, buf, LARGE_STR_SIZE - 12);
    p = buf + strlen (buf) - 1;
    while (p >= buf && (*p == '\n' || *p == ' ')) {
	*p = '\0';
	p--;
    }
    p = buf + MISC_char_cnt (buf, " ");
    FP_str_replace (p, 0, 0, "/* TBD: ");
    strcat (p, " */\n");
    Output_lines (buf, 1);
}

/******************************************************************

    Initializes this module and frees all previously allocated memory.

******************************************************************/

static void Init_this_module () {
    int i;

    for (i = 0; i < N_rvs; i++) {
	if (Rvs[i].r_tks != NULL)
	    free (Rvs[i].r_tks);
    }
    N_rvs = 0;

    if (Prev_line != NULL)
	STR_free (Prev_line);
    Prev_line = NULL;

    Cr_func[0] = '\0';

    L_stack (L_INIT, 0, NULL);
}





