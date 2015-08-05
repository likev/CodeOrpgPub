
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains the functions creating the task include file.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/01 21:52:56 $
 * $Id: ftc_include.c,v 1.3 2010/11/01 21:52:56 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

typedef struct {
    char *name;		/* name of the block */
    char *value;	/* variable list - null-terminated strings */
} Define_t;

static Define_t *Udefs = NULL;		/* array of used constants */
static int N_udefs = 0;
static void *Udef_tblid = NULL;		/* table id for Udefs table */

static Common_block_t *Ucbs = NULL;	/* array of used common blocks */
static int N_ucbs = 0;
static void *Ucb_tblid = NULL;		/* table id for Ucbs table */

static int Ucb_cmp (void *e1, void *e2);
static void Open_tables ();
static int Udef_cmp (void *e1, void *e2);
static void Read_include ();
static void Create_ftc_funcs (char *tname);
static void Create_ftc_macros (char *tname);
static void Set_used_param (char *ftn);


/******************************************************************

    Writes the global info to the task include file.

******************************************************************/

void FI_update_include () {
    static char *inc_files[] = {"stdio.h", "unistd.h", "stdlib.h", "string.h", "rpgc.h", "math.h"};
    char fname[MAX_STR_SIZE], buf[MAX_STR_SIZE], *text, *templ, tname[MAX_STR_SIZE];
    int i;

    Read_include ();

    sprintf (fname, "%s.h.tmp", FM_get_task_name ());
    sprintf (buf, "    This is the task include file for %s.\n", 
						FM_get_task_name ());
    FF_outout_file_header (fname, buf);

    sprintf (tname, "%s_H", FM_get_task_name ());
    MISC_toupper (tname);
    sprintf (buf, "#ifndef %s\n#define %s\n\n", tname, tname);
    FF_output_text (fname, buf, strlen (buf));

    for (i = 0; i < sizeof (inc_files) / sizeof (char *); i++) {
	sprintf (buf, "#include <%s>\n", inc_files[i]);
	FF_output_text (fname, buf, strlen (buf));
    }

    text = "\n/* defining constants */\n";
    FF_output_text (fname, text, strlen (text));
    for (i = 0; i < N_udefs; i++) {
	if (FR_is_replace_var (Udefs[i].name))
	    continue;
	sprintf (buf, "#define %s %s\n", Udefs[i].name, Udefs[i].value);
	FF_output_text (fname, buf, strlen (buf));
    }

    text = "\n/* defining global variables */\n";
    FF_output_text (fname, text, strlen (text));
    for (i = 0; i < N_ucbs; i++) {
	int k;
	char *var;

	sprintf (buf, "struct {\n");
	FF_output_text (fname, buf, strlen (buf));
	var = Ucbs[i].vars;
	for (k = 0; k < Ucbs[i].n_vars; k++) {
	    sprintf (buf, "    %s;\n", var);
	    FF_output_text (fname, buf, strlen (buf));
	    var += strlen (var) + 1;
	}

	/* add padding variable */
	if (Ucbs[i].pad > 0) {
	    sprintf (buf, "    char %s[%d];\n", 
					"ftc_padding_bytes", Ucbs[i].pad);
	    FF_output_text (fname, buf, strlen (buf));
	}

	sprintf (buf, "} %s;\n\n", Ucbs[i].name);
	FF_output_text (fname, buf, strlen (buf));
    }

    text = "\n/* defining task functions */\n";
    FF_output_text (fname, text, strlen (text));
    FG_get_next_used_func_templ (0);
    while ((templ = FG_get_next_used_func_templ (1)) != NULL) {
	sprintf (buf, "%s\n", templ);
	FF_output_text (fname, buf, strlen (buf));
    }
    strcpy (buf, "\n");
    FF_output_text (fname, buf, strlen (buf));

    Create_ftc_macros (fname);
    Create_ftc_funcs (fname);

    sprintf (buf, "\n#endif    /* #ifndef %s */\n", tname);
    FF_output_text (fname, buf, strlen (buf));

    sprintf (tname, "%s.h", FM_get_task_name ());
    if (rename (fname, tname) != 0) {
	fprintf (stderr, "rename (%s %s) failed\n", fname, tname);
	exit (1);
    }
}

/******************************************************************

    Reads the task include file and puts the contents in the tables.

******************************************************************/

static void Read_include () {
    char fname[MAX_STR_SIZE], buf[MAX_STR_SIZE], *cont, *c_types;
    int ret, off, n, struct_ind, struct_done, pad;

    sprintf (fname, "%s.h", FM_get_task_name ());
    ret = FG_read_c_content (fname, &cont);
    if (ret < 0)
	return;

    Open_tables ();

    off = 0;
    struct_ind = -1;
    struct_done = 0;
    c_types = NULL;
    pad = 0;
    while ((n = FG_get_c_line (cont, off, buf, MAX_STR_SIZE)) >= 0) {
	char tk[MAX_STR_SIZE], c;
	int len;

	off += n;

	if (strstr (buf, "#define F_INT(a) ((int)(a))") != NULL)
	    break;
	len = strlen (buf);
	while (len > 0 && ((c = buf[len - 1]) == ';' || c == '\n' || 
					c == ' ' || c == '\t')) {
	    buf[len - 1] = '\0';
	    len--;
	}

	if (buf[0] == '\0')
	    continue;

	if (struct_ind < 0 &&
	    MISC_get_token (buf, "", 0, tk, MAX_STR_SIZE) > 0) {

	    if (strcmp (tk, "#define") == 0 && 
		MISC_get_token (buf, "", 1, tk, MAX_STR_SIZE) > 0) {
		char *p = buf + strlen (buf) - 1;
		while (p > buf && *p != ')') {
		    *p = '\0';
		    p--;
		}
		p = buf;
		while (*p != '\0' && *p != '(')
		    p++;
		if (*p == '(') {
		    int t;
		    Define_t def;
    
		    def.name = tk;
		    if (MISC_table_search (Udef_tblid, &def, Udef_cmp, &t) == 1)
			continue;		/* already in the table */
		    def.name = MISC_malloc (strlen (tk) + strlen (p) + 2);
		    strcpy (def.name, tk);
		    def.value = def.name + strlen (tk) + 1;
		    strcpy (def.value, p);
		    if (MISC_table_insert (Udef_tblid, &def, Udef_cmp) < 0) {
			fprintf (stderr, "MISC_table_insert Udef_tblid failed\n");
			exit (1);
		    }
		}
	    }
	    else if (strcmp (tk, "struct") == 0 && 
		     MISC_get_token (buf, "", 1, tk, MAX_STR_SIZE) > 0 &&
		     strcmp (tk, "{") == 0) {
		struct_ind = 0;		/* struct started */
		continue;
	    }
	    else if (tk[0] != '#' && buf[len - 1] == ')') {
		strcat (buf, ";");
		FG_add_inc_func (buf);
	    }
	}

	if (struct_ind >= 0 &&
	    MISC_get_token (buf, "", 0, tk, MAX_STR_SIZE) > 0 &&
	    strcmp (tk, "}") == 0) {
	    struct_done = 1;
	    continue;
	}
	if (struct_done && struct_ind >= 0 &&
	    MISC_get_token (buf, "", 0, tk, MAX_STR_SIZE) > 0) {
	    Common_block_t cb;
	    int t;

	    cb.name = tk;
	    if (MISC_table_search (Ucb_tblid, &cb, Ucb_cmp, &t) != 1) {
		cb.name = MISC_malloc (strlen (tk) + 1);
		strcpy (cb.name, tk);
		cb.n_vars = struct_ind;
		cb.vars = c_types;
		cb.pad = pad;
		cb.new = 0;
		if (MISC_table_insert (Ucb_tblid, &cb, Ucb_cmp) < 0) {
		    fprintf (stderr, "MISC_table_insert Ucb_tblid failed\n");
		    exit (1);
		}
	    }
	    else {
		if (Ucbs[t].pad < pad)
		    Ucbs[t].pad = pad;
	    }
	    struct_ind = -1;
	    struct_done = 0;
	    c_types = NULL;
	    pad = 0;
	    continue;
	}

	if (struct_ind >= 0) {
	    int np;
	    char *pad_var = "char ftc_padding_bytes[";
	    if (strncmp (buf + MISC_char_cnt (buf, " "), pad_var, 
						strlen (pad_var)) == 0 &&
		sscanf (buf + MISC_char_cnt (buf, " ") + strlen (pad_var),
							"%d", &np) == 1)
		pad = np;
	    else {		/* a normal field */
		c_types = STR_append (c_types, buf, strlen (buf) + 1);
		struct_ind++;
	    }
	}
    }
    STR_free (cont);
}

/******************************************************************

    Finds all current global items and, if not already in the table,
    adds them to the tables.

******************************************************************/

void FI_save_globals () {
    Common_block_t *cbs;
    int n_cbs, i;

    Open_tables ();

    n_cbs = FS_get_cbs (&cbs);
    for (i = 0; i < n_cbs; i++) {
	Common_block_t *cb, ucb;
	int used, k, t;
	char *var, *c_types, *sname;

	cb = cbs + i;
	ucb.name = FU_common_struct_name (cb->name);
	if (MISC_table_search (Ucb_tblid, &ucb, Ucb_cmp, &t) == 1)
	    continue;		/* already in the table */

	used = 0;
	var = cb->vars;
	c_types = NULL;
	for (k = 0; k < cb->n_vars; k++) {
	    char *ct;
	    Ident_table_t *vid = FS_var_search (var);
	    if (vid == NULL) {
		fprintf (stderr, "common variable (%s in %s) not defined\n",
						var, cb->name);
		exit (1);
	    }
	    if (vid->prop & P_USED)
		used = 1;
	    ct = FC_get_ctype_dec (vid);
	    c_types = STR_append (c_types, ct, strlen (ct) + 1);
	    var += strlen (var) + 1;
	}
	if (!used) {		/* common block not used */
	    STR_free (c_types);
	    continue;
	}

	var = cb->vars;
	for (k = 0; k < cb->n_vars; k++) {
	    Ident_table_t *vid = FS_var_search (var);
	    if (vid != NULL && vid->dimp != NULL)
		Set_used_param (vid->dimp);
	    var += strlen (var) + 1;
	}

	sname = FU_common_struct_name (cb->name);
	ucb.name = MISC_malloc (strlen (sname) + 1);
	strcpy (ucb.name, sname);
	ucb.n_vars = cb->n_vars;
	ucb.vars = c_types;
	ucb.pad = cb->pad;
	ucb.new = 1;
	if (MISC_table_insert (Ucb_tblid, &ucb, Ucb_cmp) < 0) {
	    fprintf (stderr, "MISC_table_insert Ucb_tblid failed\n");
	    exit (1);
	}
    }

    {		/* Adds used constants to the Udef_tblid */
	Ident_table_t *id;

	/* expand used to dependents */
	FS_var_search (FVS_RESET);
	while ((id = FS_var_search (FVS_NEXT)) != NULL) {
	    if ((id->prop & P_PARAM) && (id->prop & P_USED))
		Set_used_param (id->param);
	    if (id->dimp != NULL)
		Set_used_param (id->dimp);
	}

	FS_var_search (FVS_RESET);
	while ((id = FS_var_search (FVS_NEXT)) != NULL) {

	    if ((id->prop & P_USED) && (id->prop & P_PARAM)) {
		Define_t def;
		int t, nt, i;
		char v[MAX_STR_SIZE], *cid, *tks[MAX_TKS], *tb[MAX_TKS], *ct;

		def.name = id->id;
		if (MISC_table_search (Udef_tblid, &def, Udef_cmp, &t) == 1)
		    continue;		/* already in the table */

		sprintf (v, "      (%s)", id->param);
		/* convert to c code */
		nt = FS_get_ftn_tokens (v, MAX_TKS, tks, NULL);
		tb[0] = NULL;
		FC_process_expr (tks, nt, tb, 0);
		i = 0;
		while (tb[i] != NULL) {	/* remove _ after macros */
		    Ident_table_t *idt;
		    char b[MAX_STR_SIZE];
		    if (FP_is_char (tb[i][0]) && 
			tb[i][strlen (tb[i]) - 1] == '_' &&
			strcpy (b, tb[i]) != NULL &&
			(b[strlen (b) - 1] = '\0') == '\0' &&
			(idt = FS_var_search (b)) != NULL &&
			(idt->prop & P_PBR_PARM))
			tb[i][strlen (tb[i]) - 1] = '\0';
		    i++;
		}
		ct = FU_get_c_text (tb, 0);
		cid = id->c_id;
		if (cid == NULL)
		    cid = id->id;
		def.name = MISC_malloc (strlen (cid) + strlen (ct) + 2);
		strcpy (def.name, cid);
		if ((id->prop & P_PBR_PARM) && cid[strlen (cid) - 1] == '_')
		    def.name[strlen (cid) - 1] = '\0';
					/* remove _ after macros name */
		def.value = def.name + strlen (cid) + 1;
		strcpy (def.value, ct);
		if (MISC_table_insert (Udef_tblid, &def, Udef_cmp) < 0) {
		    fprintf (stderr, "MISC_table_insert Udef_tblid failed\n");
		    exit (1);
		}
	    }
	}
    }
}

/******************************************************************

    Sets all parameters in FTN string "ftn" to used.

******************************************************************/

static void Set_used_param (char *ftn) {
    static int depth = 0;
    char *tks[MAX_TKS];
    int nt, i;

    depth++;
    if (depth > 64) {
	fprintf (stderr, "Recursive definition of macros\n");
	exit (1);
    }
    nt = FP_get_tokens (ftn, MAX_TKS, tks, NULL);
    for (i = 0; i < nt; i++) {
	Ident_table_t *tid;
	if (FP_is_char (tks[i][0]) && 
	    (tid = FS_var_search (tks[i])) != NULL && 
	    (tid->prop & P_PARAM)) {
	    tid->prop |= P_USED;
	    Set_used_param (tid->param);
	}
    }
    depth--;
    return;
}

/******************************************************************

    Comparation function for Ucb_tblid table search.

******************************************************************/

static void Open_tables () {

    if (Ucb_tblid == NULL) {
	Ucb_tblid = MISC_open_table (sizeof (Common_block_t), 
				    128, 1, &N_ucbs, (char **)&Ucbs);
	if (Ucb_tblid == NULL) {
	    fprintf (stderr, "MISC_open_table failed\n");
	    exit (1);
	}
    }

    if (Udef_tblid == NULL) {
	Udef_tblid = MISC_open_table (sizeof (Define_t), 
				    128, 1, &N_udefs, (char **)&Udefs);
	if (Udef_tblid == NULL) {
	    fprintf (stderr, "MISC_open_table failed\n");
	    exit (1);
	}
    }
}

/******************************************************************

    Comparation function for Ucb_tblid table search.

******************************************************************/

static int Ucb_cmp (void *e1, void *e2) {
    Common_block_t *cb1, *cb2;
    cb1 = (Common_block_t *)e1;
    cb2 = (Common_block_t *)e2;
    return (strcmp (cb1->name, cb2->name));
}

/******************************************************************

    Comparation function for Ucb_tblid table search.

******************************************************************/

static int Udef_cmp (void *e1, void *e2) {
    Define_t *def1, *def2;
    def1 = (Define_t *)e1;
    def2 = (Define_t *)e2;
    return (strcmp (def1->name, def2->name));
}


/******************************************************************

    Generates code for FTC functions.

******************************************************************/

static void Create_ftc_macros (char *tname) {
    char *text = "\
/* Macros for FORTRAN intrinsit functions */\n\
#define F_INT(a) ((int)(a))\n\
#define F_REAL(a) ((float)(a))\n\
#define F_DOUBLE(a) ((double)(a))\n\
#define F_AINT(a) ((a)-(a) + (int)(a))\n\
#define F_ANINT(a) ((a)>=0? ((a)-(a)+(int)((a)+.5)):((a)-(a)+(int)((a)-.5)))\n\
#define F_NINT(a) ((a) >= 0? ((int)((a)+.5)) : ((int)((a)-.5)))\n\
\n\
#define F_ABS(a) ((a) >= 0? (a) : (-(a)))\n\
#define F_MOD(a1,a2) ((a1)-(int)((a1)/(a2))*(a2))\n\
#define F_SIGN(a1,a2) ((a2) >= 0? F_ABS(a1) : -F_ABS(a1))\n\
#define F_DIM(a1,a2) ((a1) > (a2)? ((a1) - (a2)) : ((a1) - (a1)))\n\
#define F_DPROD(a1,a2) ((a1) * (a2))\n\
\n\
#define F_MAX2(a1,a2) ((a1) >= (a2)? (a1) : (a2))\n\
#define F_MAX3(a1,a2,a3) (F_MAX2 (F_MAX2 (a1, a2), (a3)))\n\
#define F_MAX4(a1,a2,a3,a4) (F_MAX2 (F_MAX2 (a1, a2), F_MAX2 (a3, a4)))\n\
#define F_MAX5(a1,a2,a3,a4,a5) (F_MAX2 (F_MAX4 (a1,a2,a3,a4), (a5)))\n\
#define F_MAX6(a1,a2,a3,a4,a5,a6) (F_MAX2 (F_MAX4 (a1,a2,a3,a4,a5), (a6)))\n\
\n\
#define F_MIN2(a1,a2) ((a1) <= (a2)? (a1) : (a2))\n\
#define F_MIN3(a1,a2,a3) (F_MIN2 (F_MIN2 (a1, a2), (a3)))\n\
#define F_MIN4(a1,a2,a3,a4) (F_MIN2 (F_MIN2 (a1, a2), F_MIN2 (a3, a4)))\n\
#define F_MIN5(a1,a2,a3,a4,a5) (F_MIN2 (F_MIN4 (a1,a2,a3,a4), (a5)))\n\
#define F_MIN6(a1,a2,a3,a4,a5,a6) (F_MIN2 (F_MIN4 (a1,a2,a3,a4,a5), (a6)))\n\
\n\
#define F_ICHAR(a) ((int)(*a))\n\
#define F_CHAR(a) ((int)(*a))\n\
\n\
";

    FF_output_text (tname, text, strlen (text));
}

/******************************************************************

    Generates code for FTC functions.

******************************************************************/

static void Create_ftc_funcs (char *tname) {
    char *tp;
    char *text = "\
\n\
#ifdef FTC_MAIN\n\
\n\
static int iprintf (char *buf, int bs, char *format, va_list args) {\n\
    int off;\n\
    char *st, *end;\n\
\n\
    off = 0;\n\
    st = format;\n\
    end = format;\n\
    while (1) {\n\
	char fmt[128];\n\
	int i1, i2;\n\
\n\
	i1 = i2 = -1;\n\
	while (*st != 0 && *st != '%')\n\
	    st++;\n\
	if (st - end > 0) {\n\
	    if (off + (st - end) > bs)\n\
		return (-1);\n\
	    memcpy (buf + off, end, st - end);\n\
	    off += st - end;\n\
	}\n\
	if (*st == 0)\n\
	    break;\n\
	end = st + 1;\n\
	if (sscanf (end, \"%d\", &i1) == 1) {\n\
	    while (*end <= 57 && *end >= 48)\n\
		end++;\n\
	}\n\
	if (*end == '.') {\n\
	    end++;\n\
	    if (sscanf (end, \"%d\", &i2) == 1) {\n\
		while (*end <= 57 && *end >= 48)\n\
		    end++;\n\
	    }\n\
	}\n\
	memcpy (fmt, st, end - st + 1);\n\
	fmt [end - st + 1] = 0;\n\
	if (*end == 'd') {\n\
	    int i = va_arg (args, int);\n\
	    if (off + 16 >= bs)\n\
		return (-1);\n\
	    sprintf (buf + off, fmt, i);\n\
	    off += strlen (buf + off);\n\
	}\n\
	else if (*end == 'f' || *end == 'g' || *end == 'e' || *end == 'E') {\n\
	    double d = va_arg (args, double);\n\
	    if (off + 32 >= bs)\n\
		return (-1);\n\
	    sprintf (buf + off, fmt, d);\n\
	    off += strlen (buf + off);\n\
	}\n\
	else if (*end == 's') {\n\
	    char *c = va_arg (args, char *);\n\
	    if (i1 < 0)\n\
		return (-2);\n\
	    if (off + i1 >= bs)\n\
		return (-1);\n\
	    memcpy (buf + off, c, i1);\n\
	    off += i1;\n\
	}\n\
	else\n\
	    return (-2);\n\
	end++;\n\
	st = end;\n\
    }\n\
    return (off);\n\
}\n\
\n\
int lprintf (char *buf, int b_s, char *format, ...) {\n\
    va_list args;\n\
    char lb[256], *p;\n\
    int size, nbytes;\n\
\n\
    size = 256;\n\
    p = lb;\n\
    while (1) {\n\
	va_start (args, format);\n\
	nbytes = iprintf (p, size - 1, format, args);\n\
	va_end (args);\n\
	if (nbytes != -1)\n\
	    break;\n\
	if (p != lb)\n\
	    free (p);\n\
	size *= 2;\n\
	p = malloc (size);\n\
	if (p == NULL) {\n\
	    fprintf (stderr, \"malloc failed\\n\");\n\
	    exit (1);\n\
	}\n\
    }\n\
    if (nbytes > 0) {\n\
	if ((unsigned int)buf < 256) {\n\
	    p[nbytes] = 0;\n\
	    printf (\"%s\", p);\n\
	}\n\
	else {\n\
	    if (b_s > 0 && nbytes > b_s)\n\
		nbytes = b_s;\n\
	    memcpy (buf, p, nbytes);\n\
	    if (b_s > 0 && nbytes < b_s) {\n\
		int i;\n\
		for (i = nbytes; i < b_s; i++)\n\
		    buf[i] = ' ';\n\
	    }\n\
	}\n\
    }\n\
    else {\n\
	fprintf (stderr, \"lprintf: bad format: %s\\n\", format);\n\
	exit (1);\n\
    }\n\
    if (p != lb)\n\
	free (p);\n\
    return (nbytes);\n\
}\n\
\n\
int str_op (char *func, char *arg1, int size1, char *arg2, int size2) {\n\
    static char *ops[] = {\"NE\", \"EQ\", \"GT\", \"LT\", \"GE\", \"LE\", \n\
					\"LGT\", \"LLT\", \"LGE\", \"LLE\"};\n\
    int n, i, k;\n\
\n\
    if (strcmp (func, \"ASSIGN\") == 0) {\n\
	if (size1 <= size2)\n\
	    memcpy (arg1, arg2, size1);\n\
	else {\n\
	    memcpy (arg1, arg2, size2);\n\
	    for (k = size2; k < size1; k++)\n\
		arg1[k] = ' ';\n\
	}\n\
	return (0);\n\
    }\n\
\n\
    if (strcmp (func, \"INDEX\") == 0) {\n\
	for (k = 0; k <= size1 - size2; k++) {\n\
	    if (strncasecmp (arg1 + k, arg2, size2) == 0)\n\
		return (k + 1);\n\
	}\n\
	return (0);\n\
    }\n\
\n\
    n = sizeof (ops) / sizeof (char *);\n\
    for (i = 0; i < n; i++) {\n\
	if (strcmp (func, ops[i]) == 0)\n\
	    break;\n\
    }\n\
    if (i < n) {\n\
	int cmp, ret;\n\
	int (*cmpfunc) (const char *, const char *, size_t);\n\
\n\
	if (i < 6)\n\
	    cmpfunc = strncasecmp;\n\
	else\n\
	    cmpfunc = strncmp;\n\
	if (size1 == size2)\n\
	    cmp = cmpfunc (arg1, arg2, size1);\n\
	else if (size1 > size2) {\n\
	    cmp = cmpfunc (arg1, arg2, size2);\n\
	    if (cmp == 0) {\n\
		for (k = size2; k < size1; k++) {\n\
		    if (arg1[k] > ' ') {\n\
			cmp = 1;\n\
			break;\n\
		    }\n\
		    else if (arg1[k] < ' ') {\n\
			cmp = -1;\n\
			break;\n\
		    }\n\
		}\n\
	    }\n\
	}\n\
	else {\n\
	    cmp = cmpfunc (arg1, arg2, size1);\n\
	    if (cmp == 0) {\n\
		for (k = size1; k < size2; k++) {\n\
		    if (' ' > arg2[k]) {\n\
			cmp = 1;\n\
			break;\n\
		    }\n\
		    else if (' ' < arg2[k]) {\n\
			cmp = -1;\n\
			break;\n\
		    }\n\
		}\n\
	    }\n\
	}\n\
\n\
	ret = 0;\n\
	switch (i) {\n\
	    case 0:\n\
	    if (cmp != 0)\n\
		ret = 1;\n\
	    break;\n\
	    case 1:\n\
	    if (cmp == 0)\n\
		ret = 1;\n\
	    break;\n\
	    case 2:\n\
	    case 6:\n\
	    if (cmp > 0)\n\
		ret = 1;\n\
	    break;\n\
	    case 3:\n\
	    case 7:\n\
	    if (cmp < 0)\n\
		ret = 1;\n\
	    break;\n\
	    case 4:\n\
	    case 8:\n\
	    if (cmp >= 0)\n\
		ret = 1;\n\
	    break;\n\
	    case 5:\n\
	    case 9:\n\
	    if (cmp <= 0)\n\
		ret = 1;\n\
	    break;\n\
	}\n\
	return (ret);\n\
    }\n\
\n\
    return (0);\n\
}\n\
\n\
#endif\n\
";

    tp = "\n/* ftc function templates */\n";
    FF_output_text (tname, tp, strlen (tp));
    tp = "int lprintf (char *buf, int b_s, char *format, ...);\n";
    FF_output_text (tname, tp, strlen (tp));

    tp = "\n/* ftc function implementation */\n";
    FF_output_text (tname, tp, strlen (tp));
    FF_output_text (tname, text, strlen (text));
}


