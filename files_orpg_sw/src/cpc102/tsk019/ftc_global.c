
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains functions collecting global info (function templates).

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/01 21:52:55 $
 * $Id: ftc_global.c,v 1.2 2010/11/01 21:52:55 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

typedef struct {
    char *name;
    char *templ;
    int is_task;
    int from_inc;
    int used;
    char *ctmpl;
} Func_template_t;

static Func_template_t *Fts = NULL;
static int N_fts = 0;
static void *Ft_tblid = NULL;		/* table id for all func templ table */

enum {PREF_UNKNOWN, PREF_YES, PREF_NO};	/* for Ags_t.is_ref */
typedef struct {
    char *id;
    char type;
    char ndim;
    char size;
    char is_ref;
    int n_pass_to;
    char *pass_to;
    char *c_dimp;
} Ags_t;

typedef struct {
    char *id;
    char n_args;
    char type;
    char ndim;
    char size;
    Ags_t *ags;
} Ftn_ft_t;

static Ftn_ft_t *Ftn_funcs = NULL;
static int N_ftn_funcs = 0;
static void *Ftn_func_tblid = NULL;	/* table id for FTN function table */

static char Task_name[MAX_STR_SIZE] = "";

static void Read_h_file (char *fname, int required);
static void Read_c_file (char *fname);
static void Read_f_file (char *fname);
static int Get_type_tks (char **tks);
static void Add_ft (char *func, char **tks);
static int Process_c_ft (char **tks, int nt);
static int Ft_cmp (void *e1, void *e2);
static int Ftnt_cmp (void *e1, void *e2);
static void Add_ftn_func_templates ();
static int Is_reference_temp (char *ftmp, int aind, char *type);
static int Read_c_task_tag (char *fname, char *task);


/******************************************************************

    Reads the function template file for task "task".

******************************************************************/

void FG_read_templates (char *task_name) {
    char name[MAX_STR_SIZE];

    Ft_tblid = MISC_open_table (sizeof (Func_template_t), 
				256, 1, &N_fts, (char **)&Fts);
    if (Ft_tblid == NULL) {
	fprintf (stderr, "MISC_open_table Ft_tblid failed\n");
	exit (1);
    }

    strcpy (Task_name, task_name);
    sprintf (name, "ftc_%s.h", task_name);

    Read_h_file (name, 1);

    FG_add_inc_func ("int str_op (char *func, char *arg1, int size1, char *arg2, int size2);");
}

/******************************************************************

    Adds a function template line "line" to the Fts table as a from_inc
    function.

******************************************************************/

void FG_add_inc_func (char *line) {
    char *tks[MAX_TKS];
    int nt, ind, t;
    Func_template_t ft;

    nt = FP_get_tokens (line, MAX_TKS, tks, NULL);
    tks[nt] = NULL;
    if (nt <= 3 || tks[nt - 1][0] != ';' || tks[nt - 2][0] != ')')
	return;
    ind = Process_c_ft (tks, nt);
    if (ind < 0)
	return;
    ft.name = tks[ind];
    if (MISC_table_search (Ft_tblid, &ft, Ft_cmp, &t) == 1)
	Fts[t].from_inc = 1;
}

/******************************************************************

    Reads the function template file "templ_f" and "n_inp_files"
    "inp_files" and builds table "Fts". All function templates are 
    saved in file "templ_f".

******************************************************************/

void FG_search_templates (char *templ_f, int n_inp_files, char *inp_files) {
    char *name, *p;
    int i;

    Ft_tblid = MISC_open_table (sizeof (Func_template_t), 
				256, 1, &N_fts, (char **)&Fts);
    if (Ft_tblid == NULL) {
	fprintf (stderr, "MISC_open_table Ft_tblid failed\n");
	exit (1);
    }

    Ftn_func_tblid = MISC_open_table (sizeof (Ftn_ft_t), 
				256, 1, &N_ftn_funcs, (char **)&Ftn_funcs);
    if (Ftn_func_tblid == NULL) {
	fprintf (stderr, "MISC_open_table Ftn_func_tblid failed\n");
	exit (1);
    }

    p = templ_f;
    while (*p != '\0' && *p != '.')
	p++;
    if (*p == '\0') {		/* a task name */
	char b[MAX_STR_SIZE];
	strcpy (Task_name, templ_f);
	sprintf (b, "ftc_%s.h", templ_f);
	strcpy (templ_f, b);
    }

    Read_h_file (templ_f, 0);
    name = inp_files;
    for (i = 0; i < n_inp_files; i++) {
	int len = strlen (name);
	printf ("process %s\n", name);
	if (len > 3 && strcasecmp (name + len - 2, ".c") == 0)
	    Read_c_file (name);
	else if (len > 3 && strcasecmp (name + len - 2, ".h") == 0)
	    Read_h_file (name, 1);
	else if (len > 3 && strcasecmp (name + len - 2, ".f") == 0)
	    Read_f_file (name);
	else if (len > 5 && strcasecmp (name + len - 4, ".ftn") == 0)
	    Read_f_file (name);
	else {
	    fprintf (stderr, "Unexpected file name (%s)\n", name);
	    exit (1);
	}
	name += strlen (name) + 1;
    }

    if (N_ftn_funcs > 0)
	Add_ftn_func_templates ();

    if (n_inp_files > 0) {
	char tname[MAX_STR_SIZE], comment[MAX_STR_SIZE];
	int i;

	strcpy (tname, templ_f);
	strcat (tname, ".tmp");
	if (Task_name[0] != '\0')
	    sprintf (comment, "  /* %s */", Task_name);
	for (i = 0; i < N_fts; i++) {
	    FF_output_text (tname, Fts[i].templ, strlen (Fts[i].templ));
	    if (Task_name[0] != '\0' && Fts[i].is_task)
		FF_output_text (tname, comment, strlen (comment));
	    FF_output_text (tname, "\n", 1);
	}
	if (rename (tname, templ_f) != 0) {
	    fprintf (stderr, "rename (%s %s) failed\n", tname, templ_f);
	    exit (1);
	}
    }
}

/******************************************************************

    Returns true if function "func" is a task function or false 
    otherwise. Returns -1 if the functin is not found.

******************************************************************/

int FG_is_task_func (char *func) {
    Func_template_t ft;
    int t;

    ft.name = func;
    if (MISC_table_search (Ft_tblid, &ft, Ft_cmp, &t) == 1)
	return (Fts[t].is_task);
    return (-1);
}

/******************************************************************

    Sets used flag in Fts for function used in current routine.

******************************************************************/

void FG_check_used_funcs () {
    int i;

    for (i = 0; i < N_fts; i++) {
	Ident_table_t *id;

	if (Fts[i].from_inc || Fts[i].used)
	    continue;
	id = FS_var_search (Fts[i].name);
	if (id != NULL && (id->prop & P_USED)) {
	    int nt, k, in_b;
	    char *tks[MAX_TKS], *ctmpl;

	    if (id->c_id != NULL) {
		Func_template_t ft;
		int t;
		ft.name = id->c_id;
		if (MISC_table_search (Ft_tblid, &ft, Ft_cmp, &t) == 1 &&
			Fts[t].from_inc)
		    continue;	/* already read from previous include */
	    }

	    Fts[i].used = 1;

	    /* convert identifiers to c */
	    nt = FP_get_tokens (Fts[i].templ, MAX_TKS, tks, NULL);
	    tks[nt] = NULL;
	    in_b = 0;		/* in [] - we do not convert vars in [] */
	    for (k = 0; k < nt; k++) {
		if (tks[k][0] == '[')
		    in_b++;
		else if (tks[k][0] == ']')
		    in_b--;
		if (!in_b && FP_is_char (tks[k][0])) {
		    if (strcmp (tks[k], Fts[i].name) == 0) {
			if (id->c_id != NULL)
			    FU_update_tk (tks + k, id->c_id);
			else
			    FU_update_tk (tks + k, id->id);
		    }
		    else {
			char b[MAX_STR_SIZE];
			strcpy (b, tks[k]);
			FS_conver_to_c_id (b, NULL);
			FU_update_tk (tks + k, b);
		    }
		}
/*
		if (in_b && FP_is_char (tks[k][0])) {
		    Ident_table_t *tid;
		    tid = FS_var_search (tks[k]);
		    if (tid != NULL && (tid->prop & P_PARAM))
			tid->prop |= P_USED;
		}
*/
	    }
	    if (Fts[i].ctmpl != NULL)
		free (Fts[i].ctmpl);
	    ctmpl = FG_get_c_text (tks);
	    Fts[i].ctmpl = MISC_malloc (strlen (ctmpl) + 1);
	    strcpy (Fts[i].ctmpl, ctmpl);
	}
    }
}

/******************************************************************

    Returns the next task function template if "next" is true or,
    otherwise, resets to the first template.

******************************************************************/

char *FG_get_next_used_func_templ (int next) {
    static int ind = 0;
    int i;

    if (!next) {
	ind = 0;
	return (NULL);
    }

    for (i = ind; i < N_fts; i++) {

	if (Fts[i].from_inc || Fts[i].used) {
	    ind = i + 1;
	    if (Fts[i].used)
		return (Fts[i].ctmpl);
	    return (Fts[i].templ);
	}
    }
    return (NULL);
}

/******************************************************************

    Returns true if the aind-th argument of function "func" is passed
    by reference or false otherwise. Returns -1 if the functin or the
    arg is not found.

******************************************************************/

int FG_is_pb_reference (char *func, int aind, char *type) {
    Func_template_t ft;
    Ftn_ft_t fft;
    int t;

    if (type != NULL)
	type[0] = 0;

    if (FR_is_intrinsic (func))
	return (0);

    ft.name = func;
    if (MISC_table_search (Ft_tblid, &ft, Ft_cmp, &t) == 1)
	return (Is_reference_temp (Fts[t].templ, aind, type));

    if (N_ftn_funcs <= 0)
	return (-1);
    fft.id = func;
    if (MISC_table_search (Ftn_func_tblid, &fft, Ftnt_cmp, &t) == 1) {
	Ags_t *arg;
	int i;
	char *p;

	if (aind >= Ftn_funcs[t].n_args) {
	    printf ("Ask for missing %d-th arg in FTN %s\n", aind, func);
	    return (-1);
	}
	arg = Ftn_funcs[t].ags + aind;
	if (arg->is_ref == PREF_YES)
	    return (1);
	else if (arg->is_ref == PREF_NO)
	    return (0);
	p = arg->pass_to;
	for (i = 0; i < arg->n_pass_to; i++) {
	    char f[MAX_STR_SIZE];
	    int a, ispr;
	    if (sscanf (p, "%s %d", f, &a) != 2) {
		fprintf (stderr, "Unespected error in is_pb_reference\n");
		exit (1);
	    }
	    ispr = FG_is_pb_reference (f, a, NULL);
	    if (ispr != 0)
		return (ispr);
	    p += strlen (p) + 1;
	}
	return (0);
    }
    return (-1);
}

/******************************************************************

    Returns true if the aind-th argument of function template "ftmp"
    is passed by reference or false otherwise. Returns -1 if the arg
    is not found.

******************************************************************/

static int Is_reference_temp (char *ftmp, int aind, char *type) {
    char *tks[MAX_TKS];
    int nt, st, end, cnt;

    nt = FP_get_tokens (ftmp, MAX_TKS, tks, NULL);

    if (nt > 0 && tks[1][0] == '(' && FP_is_char (tks[0][0]))
	st = 2;	/* default int function */
    else {
	int n = Get_type_tks (tks);
	if (n <= 0 || n + 1 >= nt ||
		tks[n + 1][0] != '(' || !FP_is_char (tks[n][0])) {
	    printf ("Bad FT: %s\n", ftmp);
	    return (-1);
	}
	st = n + 2;	/* first token of the first arg */
    }
    end = FU_next_token (st, nt, tks, ')');	/* end of arg list */
    if (end >= nt) {
	printf ("Bad (unclosed parenthesis) FT: %s\n", ftmp);
	return (-1);
    }

    cnt = 0;
    while (st < end) {

	int e = FU_next_token (st, end, tks, ',');
	if (cnt == aind) {
	    int i;
	    if (type != NULL) {
		int e1 = e - 1;
		if (e1 >= end)
		    e1 = end - 1;
		strcpy (type, FU_get_c_text (tks + st, e1 - st + 1));
	    }
	    for (i = st; i < e; i++) {
		if (tks[i][0] == '*' || tks[i][0] == '[') {
		    return (1);
		}
	    }
	    return (0);
	}
	st = e + 1;
	cnt++;
    }
    printf ("Ask for missing %d-th arg in FT: %s\n", aind, ftmp);
    return (-1);
}

/******************************************************************

    Adds function template info in Ftn_funcs to Fts.

******************************************************************/

static void Add_ftn_func_templates () {
    int i;

    for (i = 0; i < N_ftn_funcs; i++) {
	Ftn_ft_t *fft;
	Ident_table_t id;
	char buf[MAX_STR_SIZE];
	Func_template_t ft;
	int t, k;

	fft = Ftn_funcs + i;

	id.type = fft->type;
	id.ndim = fft->ndim;
	id.size = fft->size;
	sprintf (buf, "%s %s (", FU_get_c_type (&id), fft->id);
	for (k = 0; k < fft->n_args; k++) {
	    Ags_t *arg = fft->ags + k;

	    if (k > 0)
		strcat (buf, ", ");
	    id.type = arg->type;
	    id.ndim = arg->ndim;
	    id.size = arg->size;
	    id.c_id = NULL;
	    id.id = arg->id;
	    strcat (buf, FU_get_arg_c_type (&id, fft->id, k, arg->c_dimp));
	}
	strcat (buf, ");");

	ft.name = fft->id;
	if (MISC_table_search (Ft_tblid, &ft, Ft_cmp, &t) == 1) {
	    if (strcmp (Fts[t].templ, buf) != 0) {
		printf ("FTN %s already defined but different - Updated\n",
								fft->id);
		printf ("    OLD: %s\n", Fts[t].templ);
		printf ("    NEW: %s\n", buf);
		MISC_table_free_entry (Ft_tblid, t);
	    }
	    else
		continue;    /* already defined - Future consistency check */
	}

	memset (&ft, 0, sizeof (Func_template_t));
	ft.name = MISC_malloc (strlen (fft->id) + strlen (buf) + 2);
	strcpy (ft.name, fft->id);
	ft.templ = ft.name + strlen (fft->id) + 1;
	strcpy (ft.templ, buf);
	ft.is_task = 0;
	ft.from_inc = 0;
	ft.used = 0;
	ft.ctmpl = 0;
	if (Task_name[0] != '\0')
	    ft.is_task = 1;
	if (MISC_table_insert (Ft_tblid, &ft, Ft_cmp) < 0) {
	    fprintf (stderr, "MISC_table_insert Ft_tblid failed\n");
	    exit (1);
	}
    }
}

/******************************************************************

    Reads function templates from .h file "fname" and puts them in 
    "Fts".

******************************************************************/

static void Read_h_file (char *fname, int required) {
    char buf[MAX_STR_SIZE], *cont;
    int ret, off, n;

    ret = FG_read_c_content (fname, &cont);
    if (ret < 0) {
	if (required) {
	    fprintf (stderr, "open (%s) failed\n", fname);
	    exit (1);
	}
	return;
    }

    off = 0;
    while ((n = FG_get_c_line (cont, off, buf, MAX_STR_SIZE)) >= 0) {
	char *tks[MAX_TKS];
	int nt;

	off += n;
	FM_tk_free ();
	nt = FP_get_tokens (buf, MAX_TKS, tks, NULL);
	tks[nt] = NULL;
	if (nt < 2 || tks[nt - 1][0] != ';' || tks[nt - 2][0] != ')')
	    continue;
	if (nt > 3)
	    Process_c_ft (tks, nt);
    }
    STR_free (cont);

    if (Task_name[0] != '\0')
	Read_c_task_tag (fname, Task_name);
}

/******************************************************************

    Adds a new function template to Fts.

******************************************************************/

static void Add_ft (char *func, char **tks) {
    Func_template_t ft;
    int t;
    char *temp;

    temp = FG_get_c_text (tks);
    ft.name = func;
    if (MISC_table_search (Ft_tblid, &ft, Ft_cmp, &t) == 1) {
	if (strcmp (temp, Fts[t].templ) != 0)
	    MISC_table_free_entry (Ft_tblid, t);
	else
	    return;	/* already defined - Future consistency check */
    }

    memset (&ft, 0, sizeof (Func_template_t));
    ft.name = MISC_malloc (strlen (func) + strlen (temp) + 2);
    strcpy (ft.name, func);
    ft.templ = ft.name + strlen (func) + 1;
    strcpy (ft.templ, temp);
    ft.is_task = 0;
    ft.from_inc = 0;
    ft.used = 0;
    ft.ctmpl = NULL;
    if (MISC_table_insert (Ft_tblid, &ft, Ft_cmp) < 0) {
	fprintf (stderr, "MISC_table_insert failed\n");
	exit (1);
    }
}

/******************************************************************

    Comparation function for FT table search.

******************************************************************/

static int Ft_cmp (void *e1, void *e2) {
    Func_template_t *ft1, *ft2;
    ft1 = (Func_template_t *)e1;
    ft2 = (Func_template_t *)e2;
    return (strcmp (ft1->name, ft2->name));
}

/******************************************************************

    Comparation function for FT table search.

******************************************************************/

static int Ftnt_cmp (void *e1, void *e2) {
    Ftn_ft_t *ft1, *ft2;
    ft1 = (Ftn_ft_t *)e1;
    ft2 = (Ftn_ft_t *)e2;
    return (strcasecmp (ft1->id, ft2->id));
}

/******************************************************************

    Processes a c line to determine if it is a function template.
    If it is, Add_ft is called to add it to Fts. We do not resolve
    defined types. We assume the defined type is a single word that
    is not a c type word. Returns the tk ind to the function name or
    -1 if it is not a valid template line.

******************************************************************/

static int Process_c_ft (char **tks, int nt) {
    int ind, n, i, level, fn_ind;

    fn_ind = 0;
    if (tks[1][0] == '(' && FP_is_char (tks[0][0]))
	ind = 1;	/* default int function */
    else {
	int n = Get_type_tks (tks);
	if (n <= 0 || n + 1 >= nt ||
		tks[n + 1][0] != '(' || !FP_is_char (tks[n][0]))
	    return (-1);
	fn_ind = n;
	ind = n + 1;
    }
    n = Get_type_tks (tks + ind + 1);
    i = ind + n + 2;
    level = 0;
    while (i < nt) {		/* go though array spec */
	if (tks[i][0] == '[')
	    level++;
	else if (tks[i][0] == ']')
	    level--;
	else if (level == 0)
	    break;
	i++;
    }
    if ((ind + 1 < nt && tks[ind + 1][0] == ')') ||	/* no params */
	(ind + 2 < nt && strcmp (tks[ind + 1], "void") == 0 && 
					tks[ind + 2][0] == ')') ||
	(n > 0 && i < nt &&
	 (tks[i][0] == ')' || tks[i][0] == ',') &&
	 FP_is_char (tks[ind + n + 1][0]))) {
	Add_ft (tks[ind - 1], tks);
	return (fn_ind);
    }
    return (-1);
}

/******************************************************************

    Returns the number of tokens composing a type string. This is 
    a incomplete version - e.g. Array and function pointers are not
    processed. Defined types are not processed.

******************************************************************/

static int Get_type_tks (char **tks) {
    static char *typs[] = {"int", "short", "char", "float", "double", 
					"long", "void", "const"};
    int tcnt, ind, n_types;

    tcnt = 0;
    ind = 0;
    n_types = sizeof (typs) / sizeof (char *);
    while (tks[ind] != NULL) {
	int i;
	for (i = 0; i < n_types; i++) {
	    if (strcmp (typs[i], tks[ind]) == 0)
		break;
	}
	if (i < n_types)
	    tcnt++;
	else
	    break;
	ind++;
    }

    if (tcnt == 0) {
	if (FP_is_char (tks[ind][0]))	/* other types */
	    ind++;
	else
	    return (0);
    }
    while (tks[ind][0] == '*')
		ind++;
    return (ind);
}

/******************************************************************

    Reads function templates from .c file "fname" and puts them in 
    "Fts".

******************************************************************/

static void Read_c_file (char *fname) {
    char buf[MAX_STR_SIZE], *cont;
    int ret, off, n, level;

    ret = FG_read_c_content (fname, &cont);
    if (ret < 0) {
	fprintf (stderr, "open (%s) failed\n", fname);
	exit (1);
    }

    off = level = 0;
    while ((n = FG_get_c_line (cont, off, buf, MAX_STR_SIZE)) >= 0) {
	char *tks[MAX_TKS];
	int nt;

	off += n;
	FM_tk_free ();
	nt = FP_get_tokens (buf, MAX_TKS, tks, NULL);
	tks[nt] = NULL;
	if (level == 0) {
	    if (nt < 2 || tks[nt - 1][0] != '{' || tks[nt - 2][0] != ')')
		continue;
	    tks[nt - 1][0] = ';';
	    if (nt > 3)
		Process_c_ft (tks, nt);
	}
	if (tks[nt - 1][0] == '{')
	    level++;
	else if (tks[nt - 1][0] == '}')
	    level--;
    }
    STR_free (cont);
}

/******************************************************************

    Reads function templates from .f file "fname" and builds the
    FTN function template table.

******************************************************************/

#define MAX_N_ARGS 64

static void Read_f_file (char *fname) {
    int file, ln, n_lines;

    file = FF_open_file (fname, NULL, &n_lines);

    ln = 0;
    while (1) {		/* for each function in the file */
	int n_args, i;
	Ident_table_t *id;
	Ags_t args[MAX_N_ARGS];
	Ftn_ft_t fft;

	if (ln >= n_lines)	/* no more functions */
	    break;
	FS_first_scan (file, &ln, n_lines);
    
	for (i = 0; i < MAX_N_ARGS; i++)
	    args[i].id = NULL;
	fft.id = NULL;
    
	FS_var_search (FVS_RESET);
	n_args = 0;
	while ((id = FS_var_search (FVS_NEXT)) != NULL) {
	    Ags_t *arg;
    
	    if (id->prop & P_FUNC_DEF) {
		fft.id = MISC_malloc (strlen (id->id) + 1);
		strcpy (fft.id, id->id);
		fft.type = id->type;
		fft.ndim = id->ndim;
		fft.size = id->size;
		fft.ags = NULL;
		fft.n_args = 0;
	    }
	    else if (id->arg_i >= 0) {
		char *c_dimp;
		if (id->arg_i >= MAX_N_ARGS) {
		    fprintf (stderr, "Too many args for %s\n", id->id);
		    exit (1);
		}
		arg = args + id->arg_i;
		if (arg->id != NULL) {
		    fprintf (stderr, "Two arg %d for the same func %s\n", 
						    id->arg_i, id->id);
		    exit (1);
		}
		c_dimp = FC_get_ctype_dec (id);

		if (c_dimp == NULL) {
		    arg->id = MISC_malloc (strlen (id->id) + 1);
		    arg->c_dimp = NULL;
		}
		else {

		    if (Task_name[0] == '\0') {
				/* replace macros in dim by numbers */
			int st, end;
			char *p = c_dimp + strlen (c_dimp) - 1;
			st = end = -1;
			while (p >= c_dimp) {
			    char b[128];
			    int v;
			    if (end < 0 && *p == ']')
				end = p - c_dimp;
			    if (end >= 0 && *p == '[') {
				st = p - c_dimp;
				memcpy (b, c_dimp + st + 1, end - st - 1);
				b[end - st - 1] = '\0';
				v = FE_evaluate_int_expr (b, -1);
				if (v != -1) {
				    sprintf (b, "%d", v);
				    FP_str_replace (c_dimp, st + 1, 
							end - st - 1, b);
				}
				end = -1;
			    }
			    p--;
			}
		    }

		    arg->id = MISC_malloc (strlen (id->id) + 
						strlen (c_dimp) + 2);
		    arg->c_dimp = arg->id + strlen (id->id) + 1;
		    strcpy (arg->c_dimp, c_dimp);
		}
		strcpy (arg->id, id->id);
		arg->type = id->type;
		arg->ndim = id->ndim;
		arg->size = id->size;
		arg->pass_to = NULL;
		arg->n_pass_to = 0;
		if ((id->prop & P_MODIFIED) || id->ndim > 0 || 
					(id->type == T_CHAR && id->size != 1))
		    arg->is_ref = PREF_YES;
		else if (id->n_pass_to == 0)
		    arg->is_ref = PREF_NO;
		else {
		    char *p;
		    arg->is_ref = PREF_UNKNOWN;
		    p = id->pass_to;
		    for (i = 0; i < id->n_pass_to; i++) {
			arg->pass_to = STR_append (arg->pass_to, p, 
						    strlen (p) + 1);
			arg->n_pass_to++;
		    }
		}
		if (id->arg_i + 1 > n_args)
		    n_args = id->arg_i + 1;
	    }
	}
	if (fft.id == NULL) {
/*	    printf ("No function defined in %s\n", fname); */
	    continue;
	}
	for (i = 0; i < n_args; i++) {
	    if (args[i].id == NULL) {
		fprintf (stderr, "Missing arg %d for func %s\n", i, fft.id);
		exit (1);
	    }
	    fft.ags = (Ags_t *)STR_append ((char *)fft.ags, 
				    (char *)(args + i), sizeof (Ags_t));
	}
	fft.n_args = n_args;
	if (MISC_table_insert (Ftn_func_tblid, &fft, Ftnt_cmp) < 0) {
	    fprintf (stderr, "MISC_table_insert Ftn_func_tblid failed\n");
	    exit (1);
	}
    }
}

/******************************************************************

    Reads the c line starting att "off" in c file "cont". The line 
    is copied to buf of size b_size with leading spaces removed.
    Returns the length of the line.

******************************************************************/

int FG_get_c_line (char *cont, int off, char *buf, int b_size) {
    char *p;
    int len, l, s, num_sign;

    p = cont + off;
    if (*p == '\0')
	return (-1);
    num_sign = 0;
    if (p[MISC_char_cnt (p, " \t")] == '#')
	num_sign = 1;
    while (*p != '\0') {
	if (*p == ';' || *p == '{' || *p == '}' || (num_sign && *p == '\n')) {
	    p++;
	    break;
	}
	p++;
    }
    len = p - (cont + off);
    l = len;
    if (l >= b_size)
	l = b_size - 1;
    s = MISC_char_cnt (cont + off, " \t");
    memcpy (buf, cont + off + s, l - s);
    buf[l - s] = '\0';
    return (len);
}

/******************************************************************

    Reads the entire c file of "fname" and puts the content in "buf".
    Comments are removed. Returns 0 on success of -1 if the file does
    not exist.

******************************************************************/

int FG_read_c_content (char *fname, char **buf) {
    char b[MAX_STR_SIZE];
    int in_com;
    FILE *fd;

    fd = fopen (fname, "r");
    if (fd == NULL)
	return (-1);

    *buf = NULL;
    in_com = 0;
    while (fgets (b, MAX_STR_SIZE, fd) != NULL) {
	char *p, *st;
	int num_sign;

	p = b + MISC_char_cnt (b, " \t");
	if (p[0] == '/' && p[1] == '/')
	    continue;
	num_sign = 0;
	if (p[0] == '#')
	    num_sign = 1;

	st = p;
	while (*p != '\0') {
	    if (*p == '\n') {
		p[0] = ' ';
		p[1] = '\0';
		break;
	    }
	    if (p[0] == '/' && p[1] == '*') {
		p[0] = '\0';
		*buf = STR_cat (*buf, st);
		if (num_sign)
		    *buf = STR_cat (*buf, "\n");
		p[0] = '/';
		in_com = 1;
		p++;
	    }
	    if (p[0] == '*' && p[1] == '/') {
		if (in_com) {
		    st = p + 2;
		    in_com = 0;
		}
	    }
	    p++;
	}

	if (!in_com) {
	    *buf = STR_cat (*buf, st);
	    if (num_sign)
		*buf = STR_cat (*buf, "\n");
	}
    }
    fclose (fd);
    return (0);
}

/******************************************************************

    Converts "tks", NULL-terminated, to c text and returns the text.

******************************************************************/

char *FG_get_c_text (char **tks) {
    static char buf[MAX_STR_SIZE];
    enum {TT_OTHER, TT_COMA, TT_ID};
    int pre_type, i, add_space;

    buf[0] = '\0';
    add_space = 0;
    pre_type = TT_OTHER;
    i = 0;
    while (tks[i] != NULL) {
	char *tk;
	int type = TT_OTHER;
	if (tks[i][0] == ',')
	    type = TT_COMA;
	if (FP_is_char (tks[i][0]))
	    type = TT_ID;
	tk = tks[i];
	add_space = 0;
	if (i > 0) {
	    if (pre_type == TT_ID || pre_type == TT_COMA)
		add_space = 1;
	    if (tk[0] == ')' || type == TT_COMA || tk[0] == ']' || tk[0] == '[')
		add_space = 0;
	}
	if (add_space)
	    strcat (buf, " ");
	strcat (buf, tks[i]);
	pre_type = type;
	i++;
    }
    return (buf);
}

/******************************************************************

    Reads the entire c file of "fname" and retrieves the task tag in
    the comment and put it in Fts. Returns 0 on success of -1 if
    the file does not exist.

******************************************************************/

static int Read_c_task_tag (char *fname, char *task) {
    char b[MAX_STR_SIZE];
    FILE *fd;

    fd = fopen (fname, "r");
    if (fd == NULL)
	return (-1);

    while (fgets (b, MAX_STR_SIZE, fd) != NULL) {
	char *p;

	p = b;
	while (*p != '\0') {
	    char n[MAX_STR_SIZE];
	    if (*p == '/' && p[1] == '*' &&
		sscanf (p + 2, "%s", n) == 1 &&
		strcmp (n, task) == 0)
		break;
	    p++;
	}
	if (*p != '\0') {
	    Func_template_t ft;
	    int nt, ind, t;
	    char *tks[MAX_TKS];

	    FM_tk_free ();
	    nt = FP_get_tokens (b, MAX_TKS, tks, NULL);
	    ind = 0;
	    while (ind < nt && tks[ind][0] != '(')
		ind++;
	    ind--;
	    
	    ft.name = tks[ind];
	    if (MISC_table_search (Ft_tblid, &ft, Ft_cmp, &t) == 1)
		Fts[t].is_task = 1;
	}
    }
    fclose (fd);
    return (0);
}






