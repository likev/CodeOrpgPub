
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains the functions for replading inrinsic functions, char
    operators and custom functions.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/08 17:13:30 $
 * $Id: ftc_replace.c,v 1.5 2010/11/08 17:13:30 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

typedef struct {			/* Func replacement table */
    char *f_name;
    char *text;
} R_func_t;
static R_func_t *R_funcs = NULL;
static int N_r_funcs = 0;;
static void *Rf_tblid = NULL;	/* table id for replacement funcs table */

typedef struct {		/* Variable replacement table */
    char *v_name;
    char *c_name;
    int formal;		/* This is a formal variable in the current routine */
} R_var_t;
static R_var_t *R_vars = NULL;
static int N_r_vars = 0;;
static void *Rv_tblid = NULL;	/* table id for replacement variable table */

static char *Bad_words = NULL;
static int N_bad_words = 0;

static int To_be_implemented;

static char *Get_intrinsic_name (int ind, char **tks, int nt);
static int Rf_cmp (void *e1, void *e2);
static int Rv_cmp (void *e1, void *e2);
static int Get_n_args (char **tks, int ind, int nt);
static char *Lookup_intri_name (int f_name, char *name);
static int Handle_char_operators (char *ftn, int ind, 
					char **tks, int *tof, int nt);
static int Replace_funcs (char *ftn, int ind, 
					char **tks, int *tof, int nt);
static int Check_tbds (char **tks, int nt, int ind);
static int Replace_var (char *ftn, int ind, char **tks, int *tof, int nt);
static int Handle_power_operator (char *ftn, int ind, 
					char **tks, int *tof, int nt);


/******************************************************************

    Returns true, if there is a feature to be implemented in the 
    current line, or false otherwise.

******************************************************************/

int FR_tb_implemented () {
    return (To_be_implemented);
}

int FR_is_replace_var (char *name) {
    R_var_t rv;
    int t;
    rv.v_name = name;
    if (Rv_tblid != NULL && MISC_table_search (Rv_tblid, &rv, Rv_cmp, &t) == 1)
	return (1);
    return (0);
}

/******************************************************************

    Preprocess a line of FORTRAN code "ftn". Function replacement is
    performed. Character operators are replaced by functins. The 
    modification is made in place on "ftn". The processed line is 
    then tokenized.

******************************************************************/

int FR_preprocess (char *ftn, int n, char **tks, int *tofp) {
    static int *tof = NULL, s_tof = 0;
    int nt, changed, i;

    To_be_implemented = 0;
    if (n > s_tof) {
	if (tof != NULL)
	    free (tof);
	s_tof = n + 128;
	tof = MISC_malloc (s_tof * sizeof (int));
    }

    changed = 0x7fffffff;
    while (1) {
	int prev_changed = changed;
	nt = FP_get_tokens (ftn, n, tks, tof);
	if (nt > 0 && tks[nt - 1][0] == '\n')
	    nt--;
	if (!changed)
	    break;
	changed = 0;
	for (i = nt - 1; i >= 0; i--) {
	    if (tof[i] >= prev_changed)
		continue;
	    int is_var = FP_is_char (tks[i][0]) && !FP_is_digit (tks[i][0]) &&
					    !FR_is_reserved_word (tks[i]);
	    if (is_var && Check_tbds (tks, nt, i))
		return (0);
	    if (is_var) {
		if (Replace_var (ftn, i, tks, tof, nt)) {
		    changed = tof[i];
		    break;
		}
	    }
	    if (is_var) {
		char *cname = Get_intrinsic_name (i, tks, nt);
		if (cname != NULL) {
		    FP_str_replace (ftn, tof[i], strlen (tks[i]), cname);
		    changed = tof[i];
		    break;
		}
	    }
	    if (tks[i][0] == '\'' || is_var) {
		if (Handle_char_operators (ftn, i, tks, tof, nt)) {
		    changed = tof[i];
		    break;
		}
	    }
	    if (is_var) {
		if (Replace_funcs (ftn, i, tks, tof, nt)) {
		    changed = tof[i];
		    break;
		}
	    }
	    if (i + 1 < nt && tks[i][0] == '*' && tks[i + 1][0] == '*') {
		int ret = Handle_power_operator (ftn, i, tks, tof, nt);
		if (ret < 0) {
		    To_be_implemented = 1;
		    return (0);
		}
		changed = nt;
		break;
	    }
	}
    }

    if (tofp != NULL)
	memcpy (tofp, tof, (nt + 1) * sizeof (int));

    return (nt);
}

/******************************************************************

    Processes FTN operator "**". tks[ind] is the first "*". Returns
    -1 if an error is found or 1 on success.

******************************************************************/

static int Handle_power_operator (char *ftn, int ind, 
					char **tks, int *tof, int nt) {
    int st1, e2;

    if (ind - 1 < 0 || ind + 2 >= nt)
	return (-1);
    st1 = ind - 1;
    if (tks[st1][0] == ')') {
	int in_b;

	st1--;
	in_b = 0;
	while (st1 >= 0) {
	    if (tks[st1][0] == '(') {
		if (!in_b)
		    break;
		in_b--;
	    }
	    else if (tks[st1][0] == ')')
		in_b++;
	    st1--;
	}
	if (st1 < 0 || tks[st1][0] != '(')
	    return (-1);
	if (st1 > 0 && 
		FP_is_char (tks[st1 - 1][0]) && !FP_is_digit (tks[st1 - 1][0]))
	    st1--;
    }
    e2 = ind + 2;
    if (e2 + 1 < nt && FP_is_char (tks[e2][0]) && !FP_is_digit (tks[e2][0]) && 
						tks[e2 + 1][0] == '(')
	e2++;
    if (tks[e2][0] == '(')
	e2 = FU_next_token (e2 + 1, nt, tks, ')');
    if (e2 >= nt)
	return (-1);
    FP_str_replace (ftn, tof[e2 + 1], 0, ")");
    FP_str_replace (ftn, tof[ind], tof[ind + 2] - tof[ind], ",");
    FP_str_replace (ftn, tof[st1], 0, "pow (");
    return (1);
}

/******************************************************************

    Checks "tk" and sets To_be_implemented is it is not implemented.

******************************************************************/

static int Check_tbds (char **tks, int nt, int ind) {
    static char *tbd_words[] = {"EXTERNAL", "INQUIRE", "OPEN", "CLOSE", "INQUIRE", "READ", "PRINT", "BACKSPACE", "ENDFILE", "REWIND", "TYPE", NULL};
    int nfs, i;

    nfs = sizeof (tbd_words) / sizeof (char *) - 1;
    for (i = 0; i < nfs; i++) {
	if (strcasecmp (tks[ind], tbd_words[i]) == 0) {
	    if (i == 0) {
		if (ind == 0) {
		    To_be_implemented = 1;
		    return (1);
		}
		return (0);
	    }
	    if (ind == 0 || 
		(ind + 1 < nt && tks[ind + 1][0] == '(')) {
		To_be_implemented = 1;
		return (1);
	    }
	    if (strcasecmp (tks[0], "IF") == 0 ||
		strcasecmp (tks[0], "ELSE") == 0 ||
		strcasecmp (tks[0], "ELSEIF") == 0) {
		if (ind > 0 && tks[ind - 1][0] == ')') {
		    To_be_implemented = 1;
		    return (1);
		}
	    }
	}
    }
    return (0);
}

/******************************************************************

    Replaces variable tks[ind] if necessary.

******************************************************************/

static int Replace_var (char *ftn, int ind, char **tks, int *tof, int nt) {
    R_var_t rv;
    int t, i, type_def, is_func;
    char b[MAX_STR_SIZE];

    if (strcasecmp (tks[0], "END") == 0) {
	for (i = 0; i < N_r_vars; i++)
	    R_vars[i].formal = 0;
	return (0);
    }

    rv.v_name = tks[ind];
    if (Rv_tblid == NULL || MISC_table_search (Rv_tblid, &rv, Rv_cmp, &t) != 1)
	return (0);

    if (strcasecmp (tks[0], "PARAMETER") == 0 ||
	strcasecmp (tks[0], "DATA") == 0)
	return (0);

    type_def = 0;
    if (strcasecmp (tks[0], "INTEGER") == 0 ||
	strcasecmp (tks[0], "LOGICAL") == 0 ||
	strcasecmp (tks[0], "REAL") == 0 ||
	strcasecmp (tks[0], "DOUBLE") == 0 ||
	strcasecmp (tks[0], "CHARACTER") == 0 ||
	strcasecmp (tks[0], "COMMON") == 0)
	type_def = 1;
    is_func = 0;
    for (i = 0; i < (nt >= 3? 3: nt); i++) {
	if ((type_def && strcasecmp (tks[i], "FUNCTION") == 0) ||
	    strcasecmp (tks[i], "SUBROUTINE") == 0)
	    is_func = 1;
    }

    if (is_func) {
	R_vars[t].formal = 1;
	return (0);
    }

    if (type_def) {
	int in_b = 0;
	for (i = 0; i < ind; i++) {
	    if (tks[i][0] == '(')
		in_b++;
	    else if (tks[i][0] == ')')
		in_b--;
	}
	if (!in_b)
	    return (0);		/* no replacement */
    }

    if (R_vars[t].formal)	/* we do not replace formal var */
	return (0);

    sprintf (b, "(%s)", R_vars[t].c_name);
    FP_str_replace (ftn, tof[ind], strlen (tks[ind]), b);
    return (1);
}

/******************************************************************

    Performs custom function replacement. Returns 1 if ftn is processed
    and changed or 0 otherwise.

******************************************************************/

static int Replace_funcs (char *ftn, int ind, 
					char **tks, int *tof, int nt) {
    R_func_t rf, *rfp;
    int has_call, t, e;
    char buf[MAX_STR_SIZE], *p;

    has_call = 0;
    if (ind >= 1 && strcasecmp (tks[ind - 1], "call") == 0)
	has_call = 1;
    if (!( (ind + 2 < nt && tks[ind + 1][0] == '(') || has_call ))
	return (0);
    e = FU_next_token (ind + 2, nt, tks, ')');
    if (e >= nt)
	return (0);

    rf.f_name = tks[ind];
    if (Rf_tblid == NULL || MISC_table_search (Rf_tblid, &rf, Rf_cmp, &t) != 1)
	return (0);

    rfp = R_funcs + t;
    strcpy (buf, rfp->text);
    p = buf + strlen (buf) - 1;
    while (p >= buf) {
	char b[MAX_STR_SIZE];
	int ai, nl, rl, st, end;

	if (*p == '$' && sscanf (p + 1, "%d", &ai) == 1) {

	    if (FU_get_a_field (tks + ind + 1, nt - (ind + 1), ai - 1, 
						&st, &end) <= 0) {
		fprintf (stderr,
		  "Func %s, to be replaced by %s, does not have %d-th arg\n",
					rfp->f_name, rfp->text, ai);
		exit (1);
	    }
	    st += ind + 1;
	    end += ind + 1;
	    nl = tof[end + 1] - tof[st];
	    memcpy (b, ftn + tof[st], nl);
	    b[nl] = '\0';
	    rl = 2;
	    if (ai >= 10)
		rl = 3;
	    FP_str_replace (buf, p - buf, rl, b);
	}
	p--;
    }
    if (!has_call && ind > 0) {
	strcat (buf, ")");
	FP_str_replace (buf, 0, 0, "(");
    }
    FP_str_replace (ftn, tof[ind], tof[e + 1] - tof[ind], buf);
    if (has_call)
	FP_str_replace (ftn, tof[ind - 1], strlen (tks[ind - 1]), "");
    return (1);
}

/******************************************************************

    Handles CHARACTER operators and functions. Returns 1 if ftn is
    processed and changed or 0 otherwise.

******************************************************************/

static int Handle_char_operators (char *ftn, int ind, 
					char **tks, int *tof, int nt) {
    static char *ops[] = {".NE.", ".EQ.", ".GT.", ".LT.", ".GE.", ".LE.",
			"=", NULL};
    static char *op_names[] = {"NE", "EQ", "GT", "LT", "GE", "LE", 
			"ASSIGN", NULL};
    static char *funcs[] = {"LGE", "LGT", "LLE", "LLT", "INDEX", "LEN", NULL};
    int n, nops, nfs, i, nop1, nop2;
    char off1[MAX_STR_SIZE], size1[MAX_STR_SIZE];
    char off2[MAX_STR_SIZE], size2[MAX_STR_SIZE];
    char buf[MAX_STR_SIZE], arg1[MAX_STR_SIZE], arg2[MAX_STR_SIZE];

    /* process char functions */
    nfs = sizeof (funcs) / sizeof (char *) - 1;
    for (i = 0; i < nfs; i++) {
	if (strcasecmp (tks[ind], funcs[i]) == 0)
	    break;
    }
    if (i < nfs) {
	int e, n1, n2;

	if (ind + 2 >= nt || tks[ind + 1][0] != '(')
	    return (0);
	e = FU_next_token (ind + 2, nt, tks, ')');
	if (e >= nt)
	    return (0);
	n1 = FR_parse_char_opr (tks, ind + 2, nt, off1, size1, &nop1);
	if (i == 5) {		/* function LEN */
	    if (n1 <= 0 || ind + 2 + n1 != e)
		return (0);
	    FP_str_replace (ftn, tof[ind], 
				tof[e] - tof[ind] + strlen (tks[e]), size1);
	    return (1);
	}

	if (n1 <= 0 || tks[ind + 2 + n1][0] != ',')
	    return (0);
	n2 = FR_parse_char_opr (tks, ind + n1 + 3, nt, off2, size2, &nop2);
	if (n2 <= 0 || ind + n1 + n2 + 3 != e)
	    return (0);
	strcpy (arg1, FU_get_c_text (tks + ind + 2, nop1));
	if (off1[0] != '\0')
	    sprintf (arg1 + strlen (arg1), " + %s", off1);
	strcpy (arg2, FU_get_c_text (tks + ind + n1 + 3, nop2));
	if (off2[0] != '\0')
	    sprintf (arg2 + strlen (arg2), " + %s", off2);
	sprintf (buf, "str_op (\"%s\", %s, %s, %s, %s)", 
				funcs[i], arg1, size1, arg2, size2);
	FP_str_replace (ftn, tof[ind], 
				tof[e] - tof[ind] + strlen (tks[e]), buf);
	return (1);
    }

    /* process char operators */
    if (ind > 1 && tks[ind - 1][0] == '/' && tks[ind - 2][0] == '/') {
					/* // not processed */
	To_be_implemented = 1;
	return (0);
    }
    n = FR_parse_char_opr (tks, ind, nt, off1, size1, &nop1);
    if (n <= 0)
	return (0);
    if (ind + n + 1 >= nt)
	return (0);
    nops = sizeof (ops) / sizeof (char *) - 1;
    for (i = 0; i < nops; i++) {
	if (strcasecmp (tks[ind + n], ops[i]) == 0)
	    break;
    }
    if (i < nops) {
	int n2, iafter;

	n2 = FR_parse_char_opr (tks, ind + n + 1, nt, off2, size2, &nop2);
	if (n2 <= 0)
	    return (0);
	iafter = ind + n + n2 + 1;
	if (strcmp (ops[i], "=") == 0 && iafter < nt)
	    return (0);
	
	if (iafter + 1 < nt && tks[iafter][0] == '/' &&
			tks[iafter + 1][0] == '/') {	/* // not processed */
	    To_be_implemented = 1;
	    return (0);
	}
	strcpy (arg1, FU_get_c_text (tks + ind, nop1));
	if (off1[0] != '\0')
	    sprintf (arg1 + strlen (arg1), " + %s", off1);
	strcpy (arg2, FU_get_c_text (tks + ind + n + 1, nop2));
	if (off2[0] != '\0')
	    sprintf (arg2 + strlen (arg2), " + %s", off2);
	sprintf (buf, "str_op (\"%s\", %s, %s, %s, %s)", 
				op_names[i], arg1, size1, arg2, size2);
	FP_str_replace (ftn, tof[ind], 
	    tof[ind + n + n2] - tof[ind] + strlen (tks[ind + n + n2]), buf);
	return (1);
    }

    return (0);
}


/******************************************************************

    Returns the number of tokens consumed by the char variable at
    "ind" token. Returns 0 if tks[ind] is not a char operand. If it
    is a char token, nopr returns the number of tokens excluding the
    sub-string spec. "offp" and "sizep" return the offset and size 
    in text.

******************************************************************/

int FR_parse_char_opr (char **tks, int ind, int nt, 
				char *offp, char *sizep, int *nopr) {
    int e, n, size;
    Ident_table_t *id;

    if (ind + 1 >= nt || tks[ind + 1][0] != '(')
	e = ind;		/* e - end of dim spec */
    else {
	int i;
	e = FU_next_token (ind + 2, nt, tks, ')');
	if (e >= nt)
	    return (0);
	i = ind + 1;
	while (i < e && tks[i][0] != ':')
	    i++;
	if (tks[i][0] == ':')
	    e = ind;
    }
    n = e - ind + 1;
    *nopr = n;
    id = FS_var_search (tks[ind]);
    if (id != NULL) {
	if (id->type != T_CHAR)
	    return (0);
	size = id->size;
    }
    else {
	if (tks[ind][0] != '\'')
	    return (0);
	size = strlen (tks[ind]) - 2;
    }

    offp[0] = '\0';
    sizep[0] = '\0';
    if (id != NULL && e + 2 < nt && tks[e + 1][0] == '(') {
					/* get substring def */
	int e1, i, st, end, t;

	e1 = FU_next_token (e + 2, nt, tks, ')');
	if (e1 >= nt)
	    return (0);
	n = e1 - ind + 1;
	i = e + 2;
	while (i < e1 && tks[i][0] != ':')
	    i++;
	if (tks[i][0] != ':')
	    return (0);
	st = -1;
	end = -1;
	if (i == e + 2)
	    st = 1;
	else if (i == e + 3 && sscanf (tks[e + 2], "%d", &t) == 1)
	    st = t;
	if (e1 == i + 1)
	    end = id->size;
	else if (e1 == i + 2 && sscanf (tks[i + 1], "%d", &t) == 1)
	    end = t;
	if (st > 1)
	    sprintf (offp, "%d", st - 1);
	else if (st < 0) {
	    strcpy (offp, FU_get_c_text (tks + e + 2, i - (e + 2)));
	    strcat (offp, " - 1");
	}
	if (end >= st && st > 0)
	    sprintf (sizep, "%d", end - st + 1);
	else {
	    if (end < 0)
		strcpy (sizep, FU_get_c_text (tks + i + 1, e1 - (i + 1)));
	    else
		sprintf (sizep, "%d", end);
	    if (st > 0)
		sprintf (sizep + strlen (sizep), " - %d + 1", st);
	    else {
		char *t = FU_get_c_text (tks + e + 2, i - (e + 2));
		if (strcmp (sizep, t) == 0)	/* special case (X:X) */
		    strcpy (sizep, "1") ;
		else
		    sprintf (sizep + strlen (sizep), " - (%s) + 1", t);
	    }
	}
    }
    else
	sprintf (sizep, "%d", size);

    return (n);
}

/******************************************************************

    Returns 1 if "id" is a FTN reserved word or 0 otherwise.

******************************************************************/

int FR_is_reserved_word (char *id) {
    static char *reserved[] = {"IF", "DO", "CALL", "CONTINUE", "END", "ELSE", "ELSEIF", "THEN", "ENDIF", "RETURN", "GOTO", "WRITE", "TYPE"};
    int i;

    for (i = 0; i < sizeof (reserved) / sizeof (char *); i++) {
	if (strcasecmp (id, reserved[i]) == 0)
	    return (1);
    }
    return (0);
}

/******************************************************************

    Initializes this module. The replacement function table is 
    initialized.

******************************************************************/

void FR_init () {
    char b[MAX_STR_SIZE], *fname;
    FILE *fd;
    enum {SEC_NON, SEC_BNANES, SEC_RFUNC, SEC_RVAR};
    int sec;

    fname = "ftc.conf";
    fd = fopen (fname, "r");
    if (fd == NULL)
	return;

    printf ("    Reading %s\n", fname);
    if (Rf_tblid == NULL) {
	Rf_tblid = MISC_open_table (sizeof (R_func_t), 
				    64, 1, &N_r_funcs, (char **)&R_funcs);
	if (Rf_tblid == NULL) {
	    fprintf (stderr, "MISC_open_table Rf_tblid failed\n");
	    exit (1);
	}
    }
    if (Rv_tblid == NULL) {
	Rv_tblid = MISC_open_table (sizeof (R_var_t), 
				    64, 1, &N_r_vars, (char **)&R_vars);
	if (Rv_tblid == NULL) {
	    fprintf (stderr, "MISC_open_table Rv_tblid failed\n");
	    exit (1);
	}
    }

    sec = SEC_NON;
    while (fgets (b, MAX_STR_SIZE, fd) != NULL) {
	R_func_t rf;
	R_var_t rv;
	char *tks[MAX_TKS], c;
	int tof[MAX_TKS], nt, l, t;

	if ((c = b[MISC_char_cnt (b, " \t\n")]) == '#' || c == '\0')
	    continue;

	if (strstr (b, "WORDS not for variable names:") != NULL) {
	    sec = SEC_BNANES;
	    continue;
	}
	else if (strstr (b, "LIST of variable replacement:") != NULL) {
	    sec = SEC_RVAR;
	    continue;
	}
	else if (strstr (b, "LIST of function replacement:") != NULL) {
	    sec = SEC_RFUNC;
	    continue;
	}

	if (sec == SEC_BNANES) {
	    char tk[MAX_STR_SIZE];
	    int i = 0;
	    while (MISC_get_token (b, "", i, tk, MAX_STR_SIZE) > 0) {
		Bad_words = STR_append (Bad_words, tk, strlen (tk) + 1);
		N_bad_words++;
		i++;
	    }
	    continue;
	}

	nt = FP_get_tokens (b, MAX_TKS, tks, tof);
	if (tks[nt - 1][0] == '\n')
	    nt--;
	if (sec == SEC_RVAR) {
	    if (nt < 4 || tks[1][0] != '-' || tks[2][0] != '>') {
		fprintf (stderr, "Unexpected line in %s: %s\n", fname, b);
		exit (1);
	    }
	    rv.v_name = MISC_malloc (strlen (b) + 4);
	    strcpy (rv.v_name, tks[0]);
	    rv.c_name = rv.v_name + strlen (tks[0]) + 1;
	    l = tof[nt] - tof[3];
	    memcpy (rv.c_name, b + tof[3], l);
	    rv.c_name[l] = '\0';
	    rv.formal = 0;
	    if (MISC_table_search (Rv_tblid, &rv, Rv_cmp, &t) == 1) {
		fprintf (stderr, "Replacement var %s redefined in %s\n", 
						    tks[0], fname);
		exit (1);
	    }
	    if (MISC_table_insert (Rv_tblid, &rv, Rv_cmp) < 0) {
		fprintf (stderr, "MISC_table_insert Rv_tblid failed\n");
		exit (1);
	    }
	}
	else {
	    if (nt < 6 || tks[1][0] != '-' || tks[2][0] != '>') {
		fprintf (stderr, "Unexpected line in %s: %s\n", fname, b);
		exit (1);
	    }
	    rf.f_name = MISC_malloc (strlen (b) + strlen (tks[0]) + 2);
	    strcpy (rf.f_name, tks[0]);
	    rf.text = rf.f_name + strlen (tks[0]) + 1;
	    l = tof[nt] - tof[3];
	    memcpy (rf.text, b + tof[3], l);
	    rf.text[l] = '\0';
	    if (MISC_table_search (Rf_tblid, &rf, Rf_cmp, &t) == 1) {
		fprintf (stderr, "Replacement func %s redefined in %s\n", 
						    tks[0], fname);
		exit (1);
	    }
	    if (MISC_table_insert (Rf_tblid, &rf, Rf_cmp) < 0) {
		fprintf (stderr, "MISC_table_insert Rf_tblid failed\n");
		exit (1);
	    }
	}
    }
    fclose (fd);
    return;
}

/******************************************************************

    Returns true if "name" is allowed to be a global variable name.

******************************************************************/

int FR_is_var_name_allowed (char *name) {
    int i;
    char *p = Bad_words;
    for (i = 0; i < N_bad_words; i++) {
	if (strcmp (p, name) == 0)
	    return (0);
	p += strlen (p) + 1;
    }
    return (1);
}

/******************************************************************

    Comparation function for RF table search.

******************************************************************/

static int Rf_cmp (void *e1, void *e2) {
    R_func_t *id1, *id2;
    id1 = (R_func_t *)e1;
    id2 = (R_func_t *)e2;
    return (strcasecmp (id1->f_name, id2->f_name));
}

/******************************************************************

    Comparation function for RV table search.

******************************************************************/

static int Rv_cmp (void *e1, void *e2) {
    R_var_t *id1, *id2;
    id1 = (R_var_t *)e1;
    id2 = (R_var_t *)e2;
    return (strcasecmp (id1->v_name, id2->v_name));
}

/******************************************************************

    If tks[ind] is an intrinsic function, the name of the replacement
    function is returned. Otherwise NULL is returned except that
    nt == 0 in which case "unknown" is returned if the name is an
    intrinsic but the new name cannot be determined.

******************************************************************/

#define MINMAX_MAX 6	/* max number of args for MIN, MAX implemented */

static char *Get_intrinsic_name (int ind, char **tks, int nt) {
    static char fn[64];
    char *c_name;

    if (ind + 3 >= nt || tks[ind + 1][0] != '(')
	return (NULL);
    c_name = Lookup_intri_name (1, tks[ind]);
    if (c_name == NULL)
	return (NULL);

    if (strcasecmp (c_name, "F_MAX") == 0 || 
					strcasecmp (c_name, "F_MIN") == 0) {
	int n = Get_n_args (tks, ind, nt);
	if (n < 2 || n > MINMAX_MAX)
	    return (NULL);
	if ((tks[ind][0] == 'A' || tks[ind][0] == 'a') && tks[ind][4] == '0')
	    sprintf (fn, "(float)%s%d", c_name, n);
	else if (tks[ind][3] == '1')
	    sprintf (fn, "(int)%s%d", c_name, n);
	else
	    sprintf (fn, "%s%d", c_name, n);
	return (fn);
    }
    return (c_name);
}

/******************************************************************

    Returns 1 if "name" is a c name of a intrinsic function or 0
    otherwise.

******************************************************************/

int FR_is_intrinsic (char *name) {
    int n;

    if ((strncasecmp (name, "F_MAX", 5) == 0 || 
				strncasecmp (name, "F_MIN", 5) == 0) &&
	sscanf (name + 5, "%d", &n) == 1 && n >= 2 && n <= MINMAX_MAX)
	return (1);
    if (Lookup_intri_name (0, name) != NULL)
	return (1);
    return (0);
}

/******************************************************************

    Lookup the intrinsic function name table by, if f_name is true,
    FTN name, or c name otherwise.

******************************************************************/

static char *Lookup_intri_name (int f_name, char *name) {
    static struct intri_name {
	char *f_name;
	char *c_name;
    } intri_names[] = {
	{"INT", "F_INT"},	{"IFIX", "F_INT"},	{"IDINT", "F_INT"},
	{"REAL", "F_REAL"},	{"FLOAT", "F_REAL"},	{"SNGL", "F_REAL"},
	{"DOUBLE", "F_DOUBLE"},
	{"AINT", "F_AINT"},	{"DINT", "F_AINT"},
	{"ANINT", "F_ANINT"},	{"DNINT", "F_ANINT"},
	{"NINT", "F_NINT"},	{"IDNINT", "F_NINT"},
	{"ABS", "F_ABS"},	{"IABS", "F_ABS"},	{"DABS", "F_ABS"},
	{"MOD", "F_MOD"},	{"AMOD", "F_MOD"},	{"DMOD", "F_MOD"},
	{"SIGN", "F_SIGN"},	{"ISIGN", "F_SIGN"},	{"DSIGN", "F_SIGN"},
	{"DIM", "F_DIM"},	{"IDIM", "F_DIM"},	{"DDIM", "F_DIM"},
	{"DPROD", "F_DPROD"},
	{"MAX", "F_MAX"},	{"MAX0", "F_MAX"},
	{"AMAX1", "F_MAX"},	{"DMAX1", "F_MAX"},
	{"AMAX0", "F_MAX"},	{"MAX1", "F_MAX"},
	{"MIN", "F_MIN"},	{"MIN0", "F_MIN"},
	{"AMIN1", "F_MIN"},	{"DMIN1", "F_MIN"},
	{"AMIN0", "F_MIN"},	{"MIN1", "F_MIN"},
	{"SQRT", "sqrt"},	{"DSQRT", "sqrt"},
	{"EXP", "exp"},		{"DEXP", "exp"},
	{"LOG", "log"},		{"ALOG", "log"},	{"DLOG", "log"},
	{"LOG10", "log10"},	{"DLOG10", "log10"},
	{"SIN", "sin"},		{"DSIN", "sin"},
	{"COS", "cos"},		{"DCOS", "cos"},
	{"TAN", "tan"},		{"DTAN", "tan"},
	{"ASIN", "asin"},	{"DASIN", "asin"},
	{"ACOS", "acos"},	{"DACOS", "acos"},
	{"ATAN", "atan"},	{"DATAN", "atan"},
	{"ATAN2", "atan2"},	{"DATAN2", "atan2"},
	{"SINH", "sinh"},	{"DSINH", "sinh"},
	{"COSH", "cosh"},	{"DCOSH", "cosh"},
	{"TANH", "tanh"},	{"DTANH", "tanh"},
	{"CHAR", "not_implemented"},	{"ICHAR", "not_implemented"},
	{"_NON_EXIST_", "pow"},
    };
    int i;

    for (i = 0; i < sizeof (intri_names) / sizeof (struct intri_name); i++) {
	if (f_name) {
	    if (strcasecmp (intri_names[i].f_name, name) == 0) {
		if (strcmp (intri_names[i].c_name, "not_implemented") == 0) {
		    To_be_implemented = 1;
		    return (NULL);
		}
		return (intri_names[i].c_name);
	    }
	}
	else {
	    if (strcasecmp (intri_names[i].c_name, name) == 0)
		return (intri_names[i].f_name);
	}
    }
    return (NULL);
}

/******************************************************************

    Returns the number of arguments of function tks[ind]. -1 is 
    returned on failure.

******************************************************************/

static int Get_n_args (char **tks, int ind, int nt) {
    int i, st, end;

    i = 0;
    while (FU_get_a_field (tks + ind + 1, nt - (ind + 1), i, &st, &end) > 0)
	i++;
    return (i);
}




