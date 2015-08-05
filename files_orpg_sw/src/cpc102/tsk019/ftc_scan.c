
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains the routines that perform the first scan of the FORTRAN.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/02 21:30:14 $
 * $Id: ftc_scan.c,v 1.4 2010/11/02 21:30:14 jing Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

static Ident_table_t *Ids = NULL;	/* identifier table entries */
static int N_ids = 0;			/* # of identifier table entries */
static void *Id_tblid = NULL;		/* table id for identifier table */

static Common_block_t *Cbs = NULL;	/* array of common blocks */
static int N_cbs = 0;

static char *L_type = NULL;
static int N_lines = 0;

static int *Goto_labels = NULL;
static int N_goto_labels = 0;

static char *Args = NULL;
static int N_args = 0;

typedef struct {			/* FORMAT table */
    int label;
    char *format;
} Format_t;
static Format_t *Formats = NULL;
static int N_formats = 0;;

static int Ffile = -1;
static int Cr_line = 0;
static int Last_type_line = 0;

static int Build_identifier_table (char *line, int lcnt, int glob);
static void Add_a_routine_id (char *id, int type, int size, int prop);
static int Id_cmp (void *e1, void *e2);
static void Init_this_module ();
static void Set_passing_args (char **tks, int nt);


/******************************************************************

    Searches variable "var" in the variable table and returns the
    table entry for it on success or NULL on failure.

******************************************************************/

Ident_table_t *FS_var_search (char *var) {
    static int ind = 0;
    Ident_table_t id;
    int t;

    if (var == FVS_RESET) {
	ind = 0;
	return (NULL);
    }
    else if (var == FVS_NEXT) {
	if (ind >= N_ids)
	    return (NULL);
	ind++;
	return (Ids + ind - 1);
    }

    id.id = var;
    if (MISC_table_search (Id_tblid, &id, Id_cmp, &t) != 1)
	return (NULL);
    return (Ids + t);
}

/******************************************************************

    Returns the array of the common clocks.

******************************************************************/

int FS_get_cbs (Common_block_t **cbsp) {

    *cbsp = Cbs;
    return (N_cbs);
}

/******************************************************************

    Increases the common block pading size to "pad" for the common
    that contains variable "id".

******************************************************************/

void FS_update_common_padding (Ident_table_t *id, int pad) {
    int i;

    for (i = 0; i < N_cbs; i++) {
	if (strcmp (Cbs[i].name, id->common) == 0) {
	    if (Cbs[i].pad < pad)
		Cbs[i].pad = pad;
	    break;
	}
    }
}

/******************************************************************

    Returns the integer variable "var"'s value.

******************************************************************/

int FS_get_value (char *var) {

    if (strcmp (var, "Last_type_line") == 0)
	return (Last_type_line);
    return (0);
}

/******************************************************************

    Returns true if "lable" is a goto label or false otherwise.

******************************************************************/

int FS_is_goto_label (int label) {
    int i;

    for (i = 0; i < N_goto_labels; i++) {
	if (Goto_labels[i] == label)
	    return (1);
    }
    return (0);
}

/******************************************************************

    Returns the format string for "lable".

******************************************************************/

char *FS_get_format (int label) {
    int i;

    for (i = 0; i < N_formats; i++) {
	if (Formats[i].label == label)
	    return (Formats[i].format);
    }
    return (NULL);
}

/**********************************************************************

    Prints error message "message" and terminate the program.

**********************************************************************/

void FS_exception_exit (const char *format, ...) {
    va_list args;
    char b[MAX_STR_SIZE];

    va_start (args, format);
    fprintf (stderr, "ERROR: ");
    vfprintf (stderr, format, args);
    va_end (args);

    if (Cr_line >= 0 && FF_get_line (Ffile, Cr_line, b, MAX_STR_SIZE) >= 0)
	fprintf (stderr, "AT: \n%s\n", b);
    if (FF_file_name (Ffile) != NULL)
	fprintf (stderr, "In FILE: \n%s\n", FF_file_name (Ffile));
    exit (1);
}

/******************************************************************

    Scans the FORTRAN file of "n_lines" lines and builds various
    tables. "ln" is the starting line number and returns the line
    number after the "END" line.

******************************************************************/

char *FS_first_scan (int ffile, int *ln, int n_lines) {
    char buf[LARGE_STR_SIZE];
    int cnt, n, glob, i;

    Init_this_module ();

    N_lines = n_lines;
    Ffile = ffile;
    L_type = MISC_malloc (N_lines * sizeof (char));

    Id_tblid = MISC_open_table (sizeof (Ident_table_t), 
				256, 1, &N_ids, (char **)&Ids);
    if (Id_tblid == NULL) {
	fprintf (stderr, "MISC_open_table failed\n");
	exit (1);
    }

    glob = 0;
    cnt = *ln;			/* line count */
    while ((n = FS_get_a_line (cnt, buf, LARGE_STR_SIZE)) > 0) {
	int line = cnt;
	cnt += n;
	*ln = cnt + 1;
	if (buf[0] != ' ' && !FP_is_digit (buf[0])) {	/* a comment line */
	    if (strncmp (buf, "CINCLUDE - DONE", 15) == 0)
		glob--;
	    else if (strncmp (buf, "CINCLUDE ", 9) == 0)
		glob++;
	}
	else {
/*printf ("line %s\n", buf);*/
	    if (Build_identifier_table (buf, line, glob) == 1)
		break;
	}
    }

    /* generate c identifiers */
    for (i = 0; i < N_ids; i++) {
	char buf[MAX_STR_SIZE];
	buf[0] = '\0';
	if ((Ids[i].prop & P_PARAM)) {
	    strcpy (buf, Ids[i].id);
	    MISC_toupper (buf);
	    if (Ids[i].prop & P_PBR_PARM)
		strcat (buf, "_");
	}
	else if ((Ids[i].prop & (P_FUNC_CALL | P_FUNC_DEF)) && 
				FG_is_task_func (Ids[i].id) != 1)
	    continue;
	else {
	    strcpy (buf, Ids[i].id);
	    FS_conver_to_c_id (buf, Ids + i);
	}

	Ids[i].c_id = MISC_malloc (strlen (buf) + 1);
	strcpy (Ids[i].c_id, buf);
    }

    return (L_type);
}

/******************************************************************

    Converts, in-place, a FTN identifier name "buf" to its c name.

******************************************************************/

void FS_conver_to_c_id (char *buf, Ident_table_t *id) {

    MISC_tolower (buf);
    if (strlen (buf) > 8 && buf[0] == 'a' && 
	FP_is_digit (buf[1]) && FP_is_digit (buf[2]) && 
	FP_is_digit (buf[3]) && FP_is_digit (buf[4]) && 
	FP_is_char (buf[5]) && 
	buf[6] == '_' && buf[7] == '_') {
	int len = strlen (buf + 8);
	memmove (buf, buf + 8, len);
	buf[len] = '\0';
	if (buf[0] <= 122 && buf[0] >= 97)
	    buf[0] -= 32;
	if (FP_is_digit (buf[0]))
	    FP_str_replace (buf, 0, 0, "N");
    }
    if (id != NULL && (id->prop & (P_FUNC_DEF | P_FUNC_CALL))) {
	if (buf[0] <= 122 && buf[0] >= 97)
	    buf[0] -= 32;
    }
}

/******************************************************************

    Gets a FORTRAN line at line "lcnt" of the file. The line is put 
    in "buf" of "bsize" bytes. Chars beyond 72 are discarded and 
    continuing lines are processed. Returns the line consumed.

******************************************************************/

int FS_get_a_line (int lcnt, char *buf, int bsize) {
    char b[MAX_STR_SIZE];
    int cnt, ct_line, is_cm;

    Cr_line = lcnt;
    buf[0] = '\n';
    buf[1] = '\0';
    cnt = is_cm = 0;
    ct_line = -1;
    while (FF_get_line (Ffile, lcnt + cnt, b, MAX_STR_SIZE) >= 0) {
	char *p;
	p = b;
	while (*p != '\0') {
	    if (*p == '\t') {
		FP_str_replace (p, 0, 1, "        ");
		p += 7;
	    }
	    p++;
	}
	if (b[0] != ' ' && !FP_is_digit (b[0])) {	/* comment line */
	    if (cnt == 0)
		is_cm = 1;
	    else if (!is_cm) {
		cnt++;
		continue;
	    }
	}
	if (cnt > 0 && (strncmp (b, "     ", 5) != 0 ||
	   		MISC_char_cnt (b + 5, " \t") != 0))
	    break;		/* not a continuing line */
	if (cnt > 0)
	    ct_line = cnt;
	if (strlen (b) > 72) {
	    b[72] = '\0';
	    p = b + 71;
	    while (p > b + 6 && (*p == ' ' || *p == '\n'))
		p--;
	    p[1] = '\n';
	    p[2] = '\0';
	}
	p = b;
	if (cnt > 0) {
	    p = b + 6;
	    p += MISC_char_cnt (p, " \t");
	    if (p > b + 6)		/* keep one space */
		p--;
	}
	if (strlen (buf) - 1 + strlen (p) >= bsize)
	    FS_exception_exit ("Buffer too small for continuing line\n");
	strcpy (buf + strlen (buf) - 1, p);
	cnt++;
    }
    if (ct_line >= 0)
	return (ct_line + 1);
    else if (cnt > 0)
	return (1);
    return (0);
}

/******************************************************************

    Parses FORTRAN tokens on line "ftn".

******************************************************************/

static int Cr_label = L_NOT_FOUND;

int FS_get_cr_lable () {
    return (Cr_label);
}

int FS_get_ftn_tokens (char *ftn, int n, char **tks, int *tof) {
    static char *ops[] = {"NE", "EQ", "GT", "LT", "GE", "LE", "OR", "AND", "NOT", "FALSE", "TRUE", NULL};
    static char *nops[] = {"!=", "==", ">", "<", ">=", "<=", "||", "&&", "!", "0", "1", NULL};
    int cnt, label, nt, ii, oi, off;
    char *p;

    cnt = MISC_char_cnt (ftn, " ");
    if (cnt > 4 || !FP_is_digit (ftn[cnt]) || 
				sscanf (ftn + cnt, "%d", &label) != 1)
	label = L_NOT_FOUND;

    p = ftn;
    while (*p != '\n' && *p != '\0' && p < ftn + 6)
	p++;
    off = p - ftn;

    FP_put_keys (ops);
    nt = FR_preprocess (p, n, tks, tof);
    FP_put_keys (NULL);
    if (nt > 0 && strcasecmp (tks[0], "ENDDO") == 0) {
	int i;
	label = L_ENDDO;
	for (i = 0; i < nt - 1; i++)
	    tks[i] = tks[i + 1];
	nt--;
    }
    Cr_label = label;

    ii = oi = 0;
    while (ii < nt) {
	int len;

	if (tof != NULL)
	    tof[oi] = tof[ii] + off;
	if (tks[ii][0] == '.' && (len = strlen (tks[ii])) > 2 &&
		 tks[ii][len - 1] == '.') {
	    int i, n;
	    n = sizeof (ops) / sizeof (char *) - 1;
	    for (i = 0; i < n; i++) {
		if (strncasecmp (tks[ii] + 1, ops[i], len - 2) == 0)
		    break;
	    }
	    if (i >= n)
		FS_exception_exit ("Unexpected operator: %s\n", tks[ii]);
	    FU_update_tk (tks + oi, nops[i]);
	    ii++;
	    oi++;
	    continue;
	}

	if (strcasecmp (tks[ii], "end") == 0 && 
		ii + 1 < nt && strcasecmp (tks[ii + 1], "if") == 0) {
	    FU_update_tk (tks + oi, "ENDIF");
	    ii++;
	}
	else if (strcasecmp (tks[ii], "go") == 0 && 
		ii + 1 < nt && strcasecmp (tks[ii + 1], "to") == 0) {
	    FU_update_tk (tks + oi, "GOTO");
	    ii++;
	}
	else if (strcasecmp (tks[ii], "else") == 0 && 
		ii + 1 < nt && strcasecmp (tks[ii + 1], "if") == 0) {
	    FU_update_tk (tks + oi, "ELSEIF");
	    ii++;
	}
	else if (strcasecmp (tks[ii], "return") == 0) {
	    FU_update_tk (tks + oi, "return");
	}
	else if (tks[ii][0] == '\'' && tks[ii][strlen (tks[ii]) - 1] == '\'') {
	    tks[ii][0] = '"';
	    tks[ii][strlen (tks[ii]) - 1] = '"';
	    tks[oi] = tks[ii];
	}
	else if (strcasecmp (tks[ii], "/") == 0 && 
		ii + 1 < nt && strcasecmp (tks[ii + 1], "/") == 0) {
	    FU_update_tk (tks + oi, "//");
	    ii++;
	}
	else if (tks[ii][0] == 'X' && tks[ii][1] == '\'' &&
		 (len = strlen (tks[ii])) > 3 && tks[ii][len - 1] == '\'') {
	    tks[ii][0] = '0';
	    tks[ii][1] = 'x';
	    tks[ii][len - 1] = '\0';
	}
	else if (oi != ii)
	    tks[oi] = tks[ii];
/*	    FU_update_tk (tks + oi, tks[ii]); */
	ii++;
	oi++;
    }
    if (tof != NULL)
	tof[oi] = tof[ii] + off;
    if (oi > 0 && tks[oi - 1][0] == '\n')	/* discard line return */
	oi--;
    return (oi);
}

/******************************************************************

    Builds the identifier table and figures out the replacement
    identifiers. Returns 1 if "END" is reached or 0 otherwise.

******************************************************************/

static int Build_identifier_table (char *line, int lcnt, int glob) {
    char *tks[MAX_TKS], *p;
    int tof[MAX_TKS], nt, type, tsize, t_n, is_func, dim_st, dim_l, ndim;

/* printf ("line - %s", line); */
    FM_tk_free ();
    p = line + strlen (line) - 1;
    while (p >= line && (*p == '\n' || *p == ' ')) {
	*p = '\0';
	p--;
    }
    nt = FS_get_ftn_tokens (line, MAX_TKS, tks, tof);
    if (nt <= 0)
	return (0);

    type = T_OTHERS;
    tsize = -2;
    dim_st = -1;
    dim_l = 0;
    ndim = 0;
    t_n = 1;		/* # tokens used for the type */
    if (strcasecmp (tks[0], "INTEGER") == 0 ||
	strcasecmp (tks[0], "LOGICAL") == 0) {
	type = T_INT;
	t_n += FS_get_size_dim (tks, nt, 1, &tsize, &dim_st, &dim_l, &ndim);
    }
    else if (strcasecmp (tks[0], "REAL") == 0) {
	type = T_REAL;
	t_n += FS_get_size_dim (tks, nt, 1, &tsize, &dim_st, &dim_l, &ndim);
    }
    else if (strcasecmp (tks[0], "DOUBLE") == 0) {
	type = T_DOUBLE;
	t_n += FS_get_size_dim (tks, nt, 1, &tsize, &dim_st, &dim_l, &ndim);
    }
    else if (strcasecmp (tks[0], "CHARACTER") == 0) {
	type = T_CHAR;
	t_n += FS_get_size_dim (tks, nt, 1, &tsize, &dim_st, &dim_l, &ndim);
    }

    is_func = 0;
    if (type != T_OTHERS && strcasecmp (tks[t_n], "FUNCTION") == 0)
	is_func = 1;

    /* parse a type line */
    if (type != T_OTHERS && !is_func) {
	int ind, next_ind, t;
	int i;

	L_type[lcnt] = L_TYPE;
	Last_type_line = lcnt;
	ind = t_n;
	while (ind < nt) {
	    Ident_table_t id;
	    char *p;
	    int ts, d_st, d_l, nd;

	    if (!FP_is_char (tks[ind][0]) || FP_is_digit (tks[ind][0]))
		FS_exception_exit ("Unexpected variable name: %s\n", tks[ind]);

	    next_ind = FS_get_size_dim (tks, nt, ind + 1, &ts, 
				&d_st, &d_l, &nd) + ind + 1;

	    if (ts == -2)
		ts = tsize;
	    else if (tsize != -2)
		printf ("type size redefined for %s\n", tks[ind]);
	    if (ts == -2) {		/* undefined */
		if (type == T_CHAR)
		    ts = 1;
		else if (type == T_DOUBLE)
		    ts = 8;
		else if (type == T_VOID)
		    ts = 0;
		else
		    ts = 4;
	    }

	    if (d_l == 0) {
		d_l = dim_l;
		d_st = dim_st;
		nd = ndim;
	    }
	    else if (dim_l > 0)
		printf ("dim descrip redefined for %s\n", tks[ind]);

	    id.id = tks[ind];
	    if (MISC_table_search (Id_tblid, &id, Id_cmp, &t) == 1)
		FS_exception_exit ("Variable %s redefined\n", tks[ind]);
	    id.id = MISC_malloc (strlen (tks[ind]) + 1);
	    strcpy (id.id, tks[ind]);
	    id.type = type;
	    id.prop = 0;
	    if (glob)
		id.prop |= P_GLOB;
	    id.param = id.common = id.c_id = id.meq = NULL;
	    id.equiv = STR_copy (NULL, "");
	    id.size = ts;
	    if (d_l > 0) {
		char *p;
		int dimlen = tof[d_st + d_l] - tof[d_st];
		id.dimp = MISC_malloc (dimlen + 1);
		memcpy (id.dimp, line + tof[d_st], dimlen);
		id.dimp[dimlen] = '\0';
		p = id.dimp;
		while (*p != '\0') {
		    if (*p == ':') {
			id.prop |= P_LOW_BOUND;
			break;
		    }
		    p++;
		}
	    }
	    else
		id.dimp = NULL;
	    id.ndim = nd;

	    id.arg_i = -1;
	    p = Args;
	    for (i = 0; i < N_args; i++) {
		if (strcasecmp (p, tks[ind]) == 0) {
		    id.arg_i = i;
		    break;
		}
		p += strlen (p) + 1;
	    }
	    id.pass_to = NULL;
	    id.n_pass_to = 0;

	    if (MISC_table_insert (Id_tblid, &id, Id_cmp) < 0)
		FS_exception_exit ("MISC_table_insert failed\n");

	    if (next_ind < nt && tks[next_ind][0] == ',')
		ind = next_ind + 1;
	    else {
		if (next_ind < nt)
		    FS_exception_exit (
			"Unused trailing token %s in type def\n", 
					tks[next_ind]);
		break;
	    }
	}
    }

    else if (strcasecmp (tks[0], "END") == 0)
	return (1);

    /* parse a PARAMETER line */
    else if (strcasecmp (tks[0], "PARAMETER") == 0) {
	int ind, next;

	L_type[lcnt] = L_PARAM;
	nt -= 1;	/* ignore the closing ")" */
	ind = 2;
	while (ind < nt) {
	    Ident_table_t id;
	    int i;

	    if (ind + 1 >= nt || tks[ind + 1][0] != '=')
		FS_exception_exit ("Expected '=' - But it is not (%s)\n", tks[ind + 1]);
	    if (ind + 2 >= nt)
		FS_exception_exit ("No value for PARAMETER (%s)\n", tks[ind]);
	    next = ind + 3;
	    while (next < nt && tks[next][0] != ',' && next <= nt)
		next++;
	    id.id = tks[ind];
	    if (MISC_table_search (Id_tblid, &id, Id_cmp, &i) == 1) {
		int len = tof[next] - tof[ind + 2];
		if (Ids[i].param != NULL)
		    FS_exception_exit ("Duplicated PARAMETER definition (%s)\n", tks[ind]);
		Ids[i].param = MISC_malloc (len + 1);
		memcpy (Ids[i].param, line + tof[ind + 2], len);
		Ids[i].param[len] = '\0';
		Ids[i].prop |= P_PARAM;
	    }
	    else
		FS_exception_exit ("PARAMETER (%s) not defined\n", tks[ind]);
	    ind = next + 1;
	}
    }

    /* parse function definition or calls */
    else if (strcasecmp (tks[0], "SUBROUTINE") == 0 || is_func) {
	int ind, tp, sz;

	if (is_func) {
	    ind = t_n + 1;
	    tp = type;
	    sz = tsize;
	}
	else {
	    ind = 1;
	    tp = T_VOID;
	    sz = 0;
	}
	L_type[lcnt] = L_ROUTINE;
	Add_a_routine_id (tks[ind], tp, sz, P_FUNC_DEF);
	Args = STR_reset (Args, 256);
	N_args = 0;
	for (ind = ind + 2; ind < nt - 1; ind++) {
	    if (tks[ind][0] != ',') {
		Args = STR_append (Args, tks[ind], strlen (tks[ind]) + 1);
		N_args++;	/* save parameters for later use */
	    }
	}
    }

    /* parse function definition or calls */
    else if (strcasecmp (tks[0], "PROGRAM") == 0) {
	L_type[lcnt] = L_ROUTINE;
    }

    /* parse a common line */
    else if (strcasecmp (tks[0], "COMMON") == 0) {
	int ind, i;
	char *cname;

	L_type[lcnt] = L_COMMON;
	if (nt < 2)
	    return (0);
	cname = tks[1];
	ind = 2;
	if (tks[1][0] == '/') {
	    if (nt < 5 || tks[3][0] != '/')
		FS_exception_exit ("unexpected COMMON line: %s %s%s%s...\n", 
				tks[0], tks[1], tks[2], tks[3]);
	    cname = tks[2];
	    ind = 4;
	}
	for (i = 0; i < N_cbs; i++) {
	    if (strcmp (Cbs[i].name, cname) == 0)
		break;
	}
	if (i >= N_cbs) {
	    Common_block_t cb;
	    cb.name = MISC_malloc (strlen (cname) + 1);
	    strcpy (cb.name, cname);
	    cb.n_vars = 0;
	    cb.pad = 0;
	    cb.vars = NULL;
	    cb.new = 0;
	    Cbs = (Common_block_t *)STR_append ((char *)Cbs, (char *)&cb, 
						sizeof (Common_block_t));
	    N_cbs++;
	}

	while (ind < nt) {
	    Ident_table_t id;
	    int t;

	    if (tks[ind][0] == ',') {
		ind++;
		continue;
	    }

	    id.id = tks[ind];
	    if (MISC_table_search (Id_tblid, &id, Id_cmp, &t) != 1)
		FS_exception_exit ("COMMON variable %s undefined\n", tks[ind]);
	    Ids[t].prop |= P_COMMON;
	    if (Ids[t].common != NULL && strcmp (Ids[t].common, cname) != 0)
		FS_exception_exit (
			"Variable %s belongs to two commons (%s %s)\n", 
					tks[ind], Ids[t].common, cname);
	    Ids[t].common = MISC_malloc (strlen (cname) + 1);
	    strcpy (Ids[t].common, cname);

	    Cbs[i].vars = STR_append (Cbs[i].vars, 
					tks[ind], strlen (tks[ind]) + 1);
	    Cbs[i].n_vars++;

	    ind++;
	}
    }

    /* parse an equivalence line */
    else if (strcasecmp (tks[0], "EQUIVALENCE") == 0) {
	int ind;

	L_type[lcnt] = L_EQUIV;
	ind = 1;
	while (ind < nt) {
	    Ident_table_t id;
	    char buf[MAX_STR_SIZE], *var1, *var2;
	    int end, cnt, len, n2, i;

	    end = ind + 1;
	    cnt = 0;
	    n2 = nt;		/* not yet available */
	    while (end < nt) {
		if (tks[end][0] == '(')
		    cnt++;
		else if (tks[end][0] == ')') {
		    if (cnt == 0)
			break;
		    cnt--;
		}
		if (cnt == 0 && tks[end][0] == ',')
		    n2 = end + 1;
		end++;
	    }
	    if (end >= nt || tks[ind][0] != '(' || tks[end][0] != ')')
		FS_exception_exit (
			"Bad (parenthesis) EQUIVALENCE line: %s %s%s...\n",
					tks[0], tks[1], tks[2]);
	    if (n2 >= nt || !FP_is_char (tks[n2][0]))
		FS_exception_exit (
			"Bad (second var) EQUIVALENCE line: %s %s%s...\n",
					tks[0], tks[1], tks[2]);

	    len = tof[end] - tof[ind] + 1;
	    memcpy (buf, line + tof[ind], len);
	    buf[len] = '\0';
	    var1 = var2 = NULL;
	    for (i = 0; i < 2; i++) {
		int t;
		if (i == 0) {
		    id.id = tks[ind + 1];
		    var1 = id.id;
		}
		else {
		    id.id = tks[n2];
		    var2 = id.id;
		}
		if (MISC_table_search (Id_tblid, &id, Id_cmp, &t) == 1) {
		    Ids[t].prop |= P_EQUIV;
		    if (Ids[t].equiv[0] != '\0')
			Ids[t].equiv = STR_cat (Ids[t].equiv, ", ");
		    Ids[t].equiv = STR_cat (Ids[t].equiv, buf);
		}
		else
		    FS_exception_exit (
			"EQUIVALENCE variable %s not defined\n", id.id);
	    }
	    FE_save_eq (buf, var1, var2);	/* we cannot pass id at this */
	    ind = end + 1;	/* time - they will change as vars added */
	    if (ind < nt) {
		if (tks[ind][0] != ',')
		    FS_exception_exit ("Bad (comma) in EQUIVALENCE line\n");
		ind++;
	    }
	}
    }

    /* parse a DATA line */
    else if (strcasecmp (tks[0], "DATA") == 0) {
	L_type[lcnt] = L_DATA;
    }

    else if (nt >= 1 && strcasecmp (tks[0], "BLOCK") == 0 && 
			strcasecmp (tks[1], "DATA") == 0) {
	L_type[lcnt] = L_BLOCK_DATA;
    }

    /* parse a FORMAT line */
    else if (strcasecmp (tks[0], "FORMAT") == 0) {
	Format_t fmt;
	char *st, *p;

	L_type[lcnt] = L_FORMAT;
	if (FS_get_cr_lable () == L_NOT_FOUND)
	     FS_exception_exit ("FORMAT line without label\n");
	st = line + tof[1];
	st += MISC_char_cnt (st, " \t");
	p = st + strlen (st) - 1;
	while (p >= st && (*p == '\n'|| *p == ' '|| *p == '\t')) {
	    *p = '\0';
	    p--;
	}
	fmt.format = MISC_malloc (strlen (st) + 1);
	strcpy (fmt.format, st);
	fmt.label = FS_get_cr_lable ();
	Formats = (Format_t *)STR_append ((char *)Formats, (char *)&fmt, 
						sizeof (Format_t));
	N_formats++;
    }

    else {				/* other lines */
	int i;

	L_type[lcnt] = L_OTHERS;
	for (i = 0; i < nt; i++) {
	    int lbl;

	    if (FP_is_char (tks[i][0]) && !FP_is_digit (tks[i][0]) &&
					!FR_is_reserved_word (tks[i])) {
		Ident_table_t *id;

		if ((i > 0 && strcasecmp (tks[i - 1], "CALL") == 0) ||
		    (i < nt - 1 && tks[i + 1][0] == '('))
		    Add_a_routine_id (tks[i], T_UNKNOWN, 0, P_FUNC_CALL);

		id = FS_var_search (tks[i]);
		if (id != NULL) {
		    id->prop |= P_USED;
		    if (id->prop & P_FUNC_CALL)
			Set_passing_args (tks + i, nt - i);
		    if (i + 1 < nt && tks[i + 1][0] == '=' && tks[i + 1][1] == '\0')
			id->prop |= P_MODIFIED;
		}
	    }

	    if (strcasecmp (tks[i], "GOTO") == 0 && i + 1 < nt &&
		sscanf (tks[i + 1], "%d", &lbl) == 1) {
		Goto_labels = (int *)STR_append ((char *)Goto_labels, 
					(char *)&lbl, sizeof (int));
		N_goto_labels++;
	    }
	}
    }
    return (0);
}

/******************************************************************

    Looks for type size and dimension description starting at st-th
    token in "tks". If dim not specified, *dim_l is set to 0. If 
    size not defined, size is set to -2 (-1 for any size). Returns 
    the numbr of tokens used.

******************************************************************/

int FS_get_size_dim (char **tks, int nt, int st, int *size, 
					int *dim_st, int *dim_l, int *ndim) {
    int ind, sz, d_ind, d_tks, nd;
    char *var;

    var = tks[st - 1];
    sz = -2;
    d_ind = -1;
    d_tks = 0;
    nd = 0;
    ind = st;
    while (ind < nt) {
	if (tks[ind][0] == '*') {
	    if (sz >= -1)
		FS_exception_exit ("size dup defined in type def (%s)\n", var);
	    if (ind + 1 < nt && FP_is_digit (tks[ind + 1][0]) &&
		sscanf (tks[ind + 1], "%d", &sz) == 1) {
		ind += 2;
	    }
	    else if (ind + 3 < nt && tks[ind + 1][0] == '(' && 
						tks[ind + 3][0] == ')') {
		if (sscanf (tks[ind + 2], "%d", &sz) != 1) {
		    if (strcmp (tks[ind + 2], "*") == 0)
			sz = -1;
		    else
			sz = FE_evaluate_int_expr (tks[ind + 2], 0);
		}
		ind += 4;
	    }
	    else {
		sz = -1;
		ind += 1;
	    }
	}
	else if (ind + 1 < nt && tks[ind][0] == '(') {
	    int end, i;
	    end = FU_next_token (ind + 1, nt, tks, ')');
	    if (end >= nt)
		FS_exception_exit ("Unclosed '(' in type def (%s)\n", var);
	    if (d_ind >= 0)
		FS_exception_exit ("Dim dup defined in type def (%s)\n", var);
	    d_ind = ind;
	    d_tks = end - ind + 1;

	    i = ind + 1;
	    while (i < end) {
		i = FU_next_token (i, end, tks, ',') + 1;
		nd++;
	    }
	    ind += d_tks;
	}
	else
	    break;
    }

    if (ind < nt && tks[ind][0] == '/') {	/* skip the value assign */
	ind++;
	if (tks[ind][0] == '/')
	    FS_exception_exit ("'//' seen in type def (%s)\n", var);
	while (ind < nt && tks[ind][0] != '/')
	    ind++;
	if (tks[ind][0] == '/')
	    ind++;
    }

    if (dim_st != NULL)
	*dim_st = d_ind;
    if (dim_l != NULL)
	*dim_l = d_tks;
    if (size != NULL)
	*size = sz;
    if (ndim != NULL)
	*ndim = nd;
    return (ind - st);
}

/******************************************************************

    Sets pass_to for arguments passed to a function call. tks[0] is
    the function name.

******************************************************************/

static void Set_passing_args (char **tks, int nt) {
    int end, cnt, st;

    if (nt < 3 ||
	tks[1][0] != '(' ||
	(end = FU_next_token (2, nt, tks, ')')) >= nt ||
	tks[end][0] != ')' ||
	end < 3)
	return;		/* no argument */

    cnt = 0;
    st = 2;
    while (1) {
	Ident_table_t *id;
	int e, i;
	char buf[MAX_STR_SIZE], *p;

	e = FU_next_token (st + 1, nt, tks, ',');
	if (e > end)
	    e = end;
	if (e <= st)	/* no more args */
	    break;
    
	id = FS_var_search (tks[st]);
/*	if (id == NULL) {    the arg can also be a macro - how do we know?
	    if (!FP_is_digit (tks[st][0]) && tks[st][0] != '\'')
		FS_exception_exit ("Arg variable %s not defined\n", tks[st]);
	}
*/

	if (id != NULL) {
	    sprintf (buf, "%s %d\n", tks[0], cnt);
	    p = id->pass_to;
	    for (i = 0; i < id->n_pass_to; i++) {
		if (strcmp (p, buf) == 0)  /* the record is already there */
		    break;
		p += strlen (p) + 1;
	    }
	    if (i >= id->n_pass_to) {
		id->pass_to = STR_append (id->pass_to, buf, strlen (buf) + 1);
		id->n_pass_to++;
	    }
	    if (id->prop & P_PARAM) {
		int pbr = FG_is_pb_reference (tks[0], cnt, NULL);
		if (pbr)	/* 1 or -1 */
		    id->prop |= P_PBR_PARM;
	    }
	}

	if (tks[e][0] != ',')
	    break;
	st = e + 1;
	cnt++;
    }
}

/******************************************************************

    Add a new function/subroutine identifier to the table.

******************************************************************/

static void Add_a_routine_id (char *fid, int type, int size, int prop) {
    Ident_table_t id;
    int i;

    id.id = fid;
    if (MISC_table_search (Id_tblid, &id, Id_cmp, &i) == 1) {
	if (!(Ids[i].prop & prop) && Ids[i].ndim == 0)
	    Ids[i].prop |= prop | P_GLOB | P_USED;
    }
    else {
	memset (&id, 0, sizeof (Ident_table_t));
	id.id = MISC_malloc (strlen (fid) + 1);
	strcpy (id.id, fid);
	id.type = type;
	id.size = size;
	id.prop = prop | P_GLOB | P_USED;
	id.arg_i = -1;
	if (MISC_table_insert (Id_tblid, &id, Id_cmp) < 0) {
	    fprintf (stderr, "MISC_table_insert failed\n");
	    exit (1);
	}
    }
}

/******************************************************************

    Comparation function for identifier table search.

******************************************************************/

static int Id_cmp (void *e1, void *e2) {
    Ident_table_t *id1, *id2;
    id1 = (Ident_table_t *)e1;
    id2 = (Ident_table_t *)e2;
    return (strcasecmp (id1->id, id2->id));
}

/******************************************************************

    Initialize this module and frees all previously allocated memory.

******************************************************************/

static void Init_this_module () {
    int i;

    if (L_type != NULL)
	free (L_type);
    L_type = NULL;
    N_lines = 0;

    if (Cbs != NULL) {
	for (i = 0; i < N_cbs; i++) {
	    free (Cbs[i].name);
	    if (Cbs[i].vars != NULL)
		STR_free (Cbs[i].vars);
	}
	STR_free ((char *)Cbs);
    }
    Cbs = NULL;
    N_cbs = 0;

    if (Goto_labels != NULL)
	STR_free ((char *)Goto_labels);
    Goto_labels = NULL;
    N_goto_labels = 0;

    for (i = 0; i < N_ids; i++) {
	free (Ids[i].id);
	if (Ids[i].dimp != NULL)
	    free (Ids[i].dimp);
	if (Ids[i].param != NULL)
	    free (Ids[i].param);
	if (Ids[i].common != NULL)
	    free (Ids[i].common);
	if (Ids[i].c_id != NULL)
	    free (Ids[i].c_id);
	if (Ids[i].equiv != NULL)
	    STR_free (Ids[i].equiv);
	if (Ids[i].pass_to != NULL)
	    STR_free (Ids[i].pass_to);
	if (Ids[i].meq != NULL)
	    free (Ids[i].meq);
    }
    if (Id_tblid != NULL)
	MISC_free_table (Id_tblid);
    N_ids = 0;
    Id_tblid = NULL;
    Ids = NULL;

    if (Args != NULL)
	STR_free (Args);
    Args = NULL;
    N_args = 0;

    for (i = 0; i < N_formats; i++)
	free (Formats[i].format);
    if (Formats != NULL)
	STR_free ((char *)Formats);
    Formats = NULL;
    N_formats = 0;

    Last_type_line = 0;
}

