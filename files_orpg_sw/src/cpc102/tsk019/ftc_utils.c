
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains the additional utility functions for conversion.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/01 21:52:58 $
 * $Id: ftc_utils.c,v 1.2 2010/11/01 21:52:58 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

static int Is_int (char *str, int *i);
static int Is_oparator (char **tks, int i);
static int Is_c_type_name (char *name);
static int Remove_brakets (char **tks, int end);


int Print_tks (char *lbl, char **tks, int n) {
    int i;

    printf ("%s: ", lbl);
    if (n == 0) {
	while (tks[n] != NULL)
	    n++;
    }
    for (i = 0; i < n; i++)
	printf ("%s ", tks[i]);
    printf ("\n");
    return (0);
}

/******************************************************************

    Returns the index of the token that equals to "target" in "tks"
    of "nt" tokens starting with index "st". If not found, returns
    nt. Tokens inside () are ignored.

******************************************************************/

int FU_next_token (int st, int nt, char **tks, char target) {
    int end, l;

    l = 0;
    end = st;
    while (end < nt) {
	if (l == 0 && tks[end][0] == target && tks[end][1] == '\0')
	    break;
	if (tks[end][0] == '(')
	    l++;
	else if (tks[end][0] == ')')
	    l--;
	end++;
    }
    return (end);
}

/******************************************************************

    Replaces "no" tokens in NULL-terminated token array "tks",
    starting at index "st", with array "ntks" of "nn" tokens. If nn
    = 0, nn is the number of tokens of "ntks". If "st" < 0, ntks is
    appended. If ntks is NULL, deletion is performed.

******************************************************************/

void FU_replace_tks (char **tks, int st, int no, char **ntks, int nn) {
    int i, nt;

    if (ntks == NULL)
	nn = 0;
    else if (nn == 0) {
	while (ntks[nn] != NULL)
	    nn++;
    }

    nt = 0;
    while (tks[nt] != NULL)
	nt++;

    if (st < 0)
	st = nt;

    if (no < 0 || nn < 0 || st < 0 || st + no > nt) {
	fprintf (stderr, "Bad FU_replace_tks calling parameters (st %d no %d nn %d nt %d)\n",
					st, no, nn, nt);
	exit (1);
    }

    if (nn > no) {
	for (i = nt; i >= st + no; i--)
	    tks[i + nn - no] = tks[i];
	nt += nn - no;
    }
    else if (nn < no) {
	i = st + no;
	while (1) {
	    tks[i + nn - no] = tks[i];
	    if (tks[i] == NULL)
		break;
	    i++;
	}
    }

    for (i = st; i < st + nn; i++) {
	if (i >= nt)
	    tks[i + 1] = NULL;
	FU_update_tk (tks + i, ntks[i - st]);
    }
}

/******************************************************************

    Inserts a list of tokens at the ind-th token of "tks".

******************************************************************/

void FU_ins_tks (char **tks, int ind, ...) {
    va_list args;
    char *p;

    if (ind < 0) {
	ind = 0;
	while (tks[ind] != NULL)
	    ind++;
    }
    va_start (args, ind);
    while (1) {
	p = va_arg (args, char *);
	if (p == NULL)
	    break;
	FU_replace_tks (tks, ind, 0, &p, 1);
	ind++;
    }
    va_end (args);
}

/******************************************************************

    Replaces the contents of token "tk" with str.

******************************************************************/

void FU_update_tk (char **tk, char *str) {

    *tk = FM_tk_malloc (strlen (str) + 1);
    strcpy (*tk, str);
}

/******************************************************************

    Returns the ind-th field of FTN dimension desp or function args 
    "tks" which starts at the first (. The starting and ending indeces
    are returned with "st" and "end". Returns the Number of tokens 
    of the field on success of 0 on failure.

******************************************************************/

int FU_get_a_field (char **tks, int nt, int ind, int *st, int *end) {
    int s, cnt;

    s = 1;		/* remove starting ( */
    nt--;		/* remove ending ) */
    cnt = 0;
    while (s < nt) {
	int e = FU_next_token (s, nt, tks, ',');
	if (cnt == ind) {
	    *st = s;
	    if (e >= nt)
		e = nt - 1;
	    else
		e--;
	    *end = e;
	    return (e - s + 1);
	}
	cnt++;
	s = e + 1;
    }
    return (0);
}

/******************************************************************

    Converts "tks", NULL-terminated, to c text and returns the text.

******************************************************************/

enum {TT_OTHER, TT_OPR, TT_ID, TT_UOPR};

char *FU_get_c_text (char **tks, int nt) {
    static char buf[LARGE_STR_SIZE];
    int pre_type, i, add_space;

    buf[0] = '\0';
    add_space = 0;
    pre_type = TT_OTHER;
    i = 0;
    while (tks[i] != NULL) {
	char *tk;
	int type;

	if (nt > 0 && i >= nt)
	    break;
	type = Is_oparator (tks, i);
	tk = tks[i];
	add_space = 0;
	if (i > 0) {
	    if (type == TT_OPR || pre_type == TT_OPR || 
		(tk[0] == '(' && pre_type == TT_ID) ||
		(type == TT_ID && pre_type == TT_ID) ||
		tk[0] == '{' ||
		tks[i - 1][0] == ';' || tks[i - 1][0] == ',' ||
		(type == TT_UOPR && pre_type == TT_ID))
		add_space = 1;
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

    Returns the token type of i-th token in "tks".

******************************************************************/

static int Is_oparator (char **tks, int i) {
    static char *opr2[] = {"=", "/", "|", "&&", "||",
		"<=", ">=", "==", "!=", ">", "<", "+=", NULL};
    static char *opr1[] = {"!", NULL};
    static char *opr[] = {"+", "-", "*", "&", NULL};
    char *tk;

    int ind = 0;
    tk = tks[i];
    if (FP_is_char (tk[0]))
	return (TT_ID);
    while (opr2[ind] != NULL) {
	if (strcmp (opr2[ind], tk) == 0)
	    return (TT_OPR);
	ind++;
    }
    ind = 0;
    while (opr1[ind] != NULL) {
	if (strcmp (opr1[ind], tk) == 0)
	    return (TT_UOPR);
	ind++;
    }
    ind = 0;
    while (opr[ind] != NULL) {
	if (strcmp (opr[ind], tk) == 0) {
	    int pr;
	    if (i > 0 && tks[i - 1][0] == ']')
		return (TT_OPR);
	    if (i > 0 && (tks[i - 1][0] == ',' || tks[i - 1][0] == '('))
		return (TT_UOPR);
	    pr = i - 1;
	    while (pr >= 0 && tks[pr][0] == ')')
		pr--;
	    if (pr >= 0 && (FP_is_digit (tks[pr][0]) ||
				tks[pr][0] == '.' || tks[pr][0] == '"'))
		return (TT_OPR);
	    while (pr >= 0 && !FP_is_char (tks[pr][0]))
		pr--;
	    if (pr >= 0) {
		if (Is_c_type_name (tks[pr]))
		    return (TT_UOPR);
		return (TT_OPR);
	    }
	    return (TT_UOPR);
	}
	ind++;
    }
    
    return (TT_OTHER);
}

static int Is_c_type_name (char *name) {
    static char *tnames[] = {"char", "int", "float", "double", "short", 
						"void", NULL};
    int k = 0;
    while (tnames[k] != NULL) {
	if (strcmp (tnames[k], name) == 0)
	    return (1);
	k++;
    }
    return (0);
}

/******************************************************************

    Compares the variable type "ltype" to the desired template type
    "type" and, if different, returns the cast to "type" in "tb. The
    types are in c variable definition form. If not cast is needed,
    tb[0] is set to NULL. "is_array_name" is true if the var to cast
    from is an array name (not array element).

******************************************************************/

void FU_get_call_cast (char *type, char *ltype, int is_array_name, char **tb) {
    char *ty[MAX_TKS], *lty[MAX_TKS];
    int nt, nlt, ind, st1, st2;

    tb[0] = NULL;
    nt = FP_get_tokens (type, MAX_TKS, ty, NULL);
    ty[nt] = NULL;
    nlt = FP_get_tokens (ltype, MAX_TKS, lty, NULL);
    lty[nlt] = NULL;

    if (!is_array_name)	{	/* non-array or array element */
	ind = 0;
	while (ty[ind] != NULL && ty[ind][0] != '[')
	    ind++;
	if (ty[ind] == NULL && ty[0] != NULL && lty[0] != NULL && 
				strcmp (ty[0], lty[0]) == 0)
	    return;		/* The same type */
    }

    ind = 0;
    while (1) {		/* compare the two types */
	if (ty[ind] == NULL && lty[ind] == NULL)
	    return;	/* ty = lty */
	if (ty[ind] == NULL || lty[ind] == NULL)
	    break;
	if (ind != 1 && strcmp (ty[ind], lty[ind]) != 0)
	    break;	/* different - we do not compare var name */
	ind++;
	if (ind == 1 && ty[ind] != NULL && ty[ind][0] == '*') {
				/* type name match and target is pointer */
	     int k, b_cnt;
	     b_cnt = 0;
	     for (k = ind; k < nlt; k++) {
		if (lty[k][0] == '[')
		    b_cnt++;
	     }
	     if (b_cnt == 1)	/* a[.] to *a - casting is not needed */
		return;
	}
    }

    /* generate the cast */
    ind = 0;
    st1 = st2 = -1;	/* the first and second [ */
    while (ty[ind] != NULL) {
	if (ty[ind][0] == '[') {
	    if (st1 < 0)
		st1 = ind;
	    else if (st2 < 0)
		st2 = ind;
	    else
		break;
	}
	ind++;
    }
    if (st2 > 0) {
	FU_ins_tks (tb, -1, "(", "*", ")", NULL);
	FU_replace_tks (ty, 1, st2 - 1, tb, 0);
	tb[0] = NULL;
	FU_ins_tks (tb, -1, "(", ")", NULL);
	FU_replace_tks (tb, 1, 0, ty, 0);
    }
    else if (st1 > 0)
	FU_ins_tks (tb, -1, "(", ty[0], "*", ")", NULL);
    else if (nt > 1 && ty[1][0] == '*')
	FU_ins_tks (tb, -1, "(", ty[0], "*", ")", NULL);
    else
	FU_ins_tks (tb, -1, "(", ty[0], ")", NULL);
}

/******************************************************************

    Returns the ascii c string for fuction "func"'s "n_arg"-th
    argument. id is the argument. id may not exist in the var table if
    c_dimp is not null. fields used in fd are id, c_id, type, ndim and
    size. c_dimp, if not null, is the c text of the type.    

******************************************************************/

char *FU_get_arg_c_type (Ident_table_t *id, char *func, int n_arg,
						char *c_dimp) {
    static char buf[MAX_STR_SIZE];
    char *var;

    var = id->id;
    if (id->c_id != NULL)
	var = id->c_id;
    if (id->ndim >= 1 || id->type == T_CHAR) {

	if (c_dimp == NULL)
	    c_dimp = FC_get_ctype_dec (id);
/* Code for convert the arg to array pointer. This is only for ndim >= 2. We 
   do not use it (See notes).
	char *tb[MAX_TKS];
	int nt, s1, e1, ind;
	nt = FP_get_tokens (c_dimp, MAX_TKS, tb, NULL);
	tb[nt] = NULL;
	s1 = e1 = -1;
	ind = 0;
	while (ind < nt) {
	    if (s1 < 0 && tb[ind][0] == '[')
		s1 = ind;
	    else if (tb[ind][0] == ']') {
		e1 = ind;
		break;
	    }
	    ind++;
	}
	if (s1 <= 0 || e1 <= s1) {
	    fprintf (stderr, "Unexpected error in FU_get_arg_c_type\n");
	    exit (1);
	}
	FU_replace_tks (tb, s1, e1 - s1 + 1, NULL, 0);
	FU_ins_tks (tb, s1, ")", NULL);
	FU_ins_tks (tb, s1 - 1, "(", "*", NULL);
	strcpy (buf, FU_get_c_text (tb, 0));
*/
	strcpy (buf, c_dimp);
    }
    else if (FG_is_pb_reference (func, n_arg, NULL) == 0)
	sprintf (buf, "%s %s", FU_get_c_type (id), var);
    else
	sprintf (buf, "%s *%s", FU_get_c_type (id), var);
    return (buf);
}

/******************************************************************

    Returns the struct name of common block "id->common".

******************************************************************/

char *FU_common_struct_name (char *common) {
    static char buf[128];
    char *p;
    int is_c;

    strcpy (buf, common);
    MISC_tolower (buf);
    if (buf[0] > 90)
	buf[0] -= 32;
    p = buf;
    is_c = 0;
    while (*p != '\0') {
	if ( p > buf && (*p < '0' || *p > '9')) {
	    is_c = 1;
	    break;
	}
	p++;
    }
    if (!is_c)		/* not char in name except the first char */
	strcat (buf, "_s");
    while (!FR_is_var_name_allowed (buf))
	strcat (buf, "_s");
    return (buf);
}

/******************************************************************

    Returns the ascii c type name of variable "id".

******************************************************************/

char *FU_get_c_type (Ident_table_t *id) {
    static char buf[32];

    if (id->type == T_INT) {
	if (id->size == 1)
	    strcpy (buf, "char");
	else if (id->size == 2)
	    strcpy (buf, "short");
	else
	    strcpy (buf, "int");
    }
    else if (id->type == T_REAL)
	strcpy (buf, "float");
    else if (id->type == T_DOUBLE)
	strcpy (buf, "double");
    else if (id->type == T_CHAR)
	strcpy (buf, "char");
    else
	strcpy (buf, "void");
    return (buf);
}

/******************************************************************

    Returns 1 if "str" is an integer or 0 otherwise. The integer value
    is returned with "i".

******************************************************************/

static int Is_int (char *str, int *i) {
    char c;
    if (sscanf (str, "%d%c", i, &c) == 1)
	return (1);
    return (0);
}

/******************************************************************

    Simplifies c expression "tks".

******************************************************************/

void FU_post_process_c_tks (char **tks) {
    int ind, done;

    done = 0;
    while (!done) {
	done = 1;
	ind = 0;
	while (tks[ind] != NULL) {	/* combine a + b */
	    int i1, i2;
	    char buf[128], *tk;
	    if (ind >= 2 && Is_int (tks[ind], &i1) && 
		(tks[ind - 1][0] == '+' || tks[ind - 1][0] == '-') && 
						tks[ind - 1][1] == '\0' &&
		Is_int (tks[ind - 2], &i2)) {
		if (ind == 2 || tks[ind - 3][0] == '(' || 
						tks[ind - 3][0] == '[') {
		    if (tks[ind - 1][0] == '+')
			i1 = i2 + i1;
		    else
			i1 = i2 - i1;
		    sprintf (buf, "%d", i1);
		    tk = buf;
		    FU_replace_tks (tks, ind - 2, 3, &tk, 1);
		    done = 0;
		    break;
		}
		else if (tks[ind - 3][0] == '-' || tks[ind - 3][0] == '+') {
		    if (tks[ind - 3][0] == '-')
			i2 = -i2;
		    if (tks[ind - 1][0] == '+')
			i1 = i2 + i1;
		    else
			i1 = i2 - i1;
		    if (i1 >= 0)
			sprintf (buf, "%d", i1);
		    else
			sprintf (buf, "%d", -i1);
		    tk = buf;
		    FU_replace_tks (tks, ind - 3, 4, &tk, 1);
		    if (i1 >= 0)
			FU_ins_tks (tks, ind - 3, "+", NULL);
		    else
			FU_ins_tks (tks, ind - 3, "-", NULL);
		    done = 0;
		    break;
		}
	    }
	    ind++;
	}
	if (!done)
	    continue;

 	ind = 0;
	while (tks[ind] != NULL) {	/* removes + 0, - 0, 0 + */
	    if (tks[ind][0] == '0' && tks[ind][1] == '\0') {
		int di = -1;
		if (ind > 0 && (tks[ind - 1][0] == '+' || 
			tks[ind - 1][0] == '-') && tks[ind - 1][1] == '\0') 
		    di = ind - 1;
		else if (tks[ind + 1] != NULL && tks[ind + 1][0] == '+' &&
					tks[ind + 1][1] == '\0')
		    di = ind;
		if (di >= 0) {
		    FU_replace_tks (tks, di, 2, NULL, 0);
		    done = 0;
		    break;
		}
	    }
	    ind++;
	}
	if (!done)
	    continue;

 	ind = 0;
	while (tks[ind] != NULL) {
/*
	    if (tks[ind][0] == ')' && ind >= 2 && tks[ind - 2][0] == '(') {
		int rm;
		if (tks[ind - 1][0] == '*' || Is_c_type_name (tks[ind - 1]))
		    rm = 0;
		else if (ind >= 4 && strcmp (tks[ind - 4], "#define") == 0)
		    rm = 1;
		else if (ind >= 3 && FP_is_char (tks[ind - 3][0]))
		    rm = 0;
		else
		    rm = 1;
		if (rm) {
		    FU_replace_tks (tks, ind, 1, NULL, 0);
		    FU_replace_tks (tks, ind - 2, 1, NULL, 0);
		    done = 0;
		}
		break;
	    }
*/
	    if (tks[ind][0] == ')') {	/* removes ( ) */
		if (Remove_brakets (tks, ind)) {
		    done = 0;
		    break;
		}
	    }
	    if (ind >= 1 &&		/* remove *& or &* */
		((tks[ind][0] == '&' && tks[ind - 1][0] == '*') ||
			(tks[ind][0] == '*' && tks[ind - 1][0] == '&')) &&
		tks[ind - 1][1] == '\0') {
		if (tks[ind][1] == '\0')
		    FU_replace_tks (tks, ind - 1, 2, NULL, 0);
		else {
		    memmove (tks[ind], tks[ind] + 1, strlen (tks[ind]));
		    FU_replace_tks (tks, ind - 1, 1, NULL, 0);
		}
	    }
	    ind++;
	}
   }

    ind = 0;	/* removes & and [0] in &...[0] */
    while (tks[ind] != NULL) {
	if (tks[ind][0] == '&' && tks[ind + 1] != NULL && 
			tks[ind + 2] != NULL && tks[ind + 2][0] == '[') {
	    int i, inb;

	    i = ind + 3;
	    inb = 1;
	    while (tks[i] != NULL) {
		if (tks[i][0] == '[')
		    inb++;
		else if (tks[i][0] == ']') {
		    inb--;
		    if (inb == 0 && 
				(tks[i + 1] == NULL || tks[i + 1][0] != '['))
			break;
		}
		i++;
	    }
	    if (tks[i][0] == ']' && tks[i - 1][0] == '0' && 
						tks[i - 2][0] == '[') {
		FU_replace_tks (tks, i - 2, 3, NULL, 0);
		FU_replace_tks (tks, ind, 1, NULL, 0);
	    }
	}

	ind++;
    }
}

/******************************************************************

    Removes brackets ( and ) if it can be removed. "end" points to
    ")". Returns true if removed.

******************************************************************/

static int Remove_brakets (char **tks, int end) {
    int st, i;

    st = end - 1;
    while (st >= 0) {
	if (tks[st][0] == ')')
	    return (0);
	if (tks[st][0] == '(')
	    break;
	st--;
    }
    if (st < 0)
	return (0);
    if (end == st + 1 && st > 0 && FP_is_char (tks[st - 1][0]))
	return (0);
    if (end == st + 2) {	/* single token in ( ) */
	if (tks[st + 1][0] == '*' || Is_c_type_name (tks[st + 1]))
	    return (0);
	else if (st >= 1 && FP_is_char (tks[st - 1][0]) &&
			!(st >= 2 && strcmp (tks[st - 2], "#define") == 0))
	    return (0);		/* function or array */
    }
    else {
	char c;
	int b_ok;

	if (st >= 1 && FP_is_char (tks[st - 1][0]))
	    return (0);		/* function or array */
	for (i = st + 1; i < end; i++) {
	    if (Is_c_type_name (tks[i]))
		return (0);
	}
	b_ok = 0;	/* check left side */
	if (st == 0)
	    b_ok = 1;
	else if ((c = tks[st - 1][0]) == '[' || c == '(' || c == '+' || c == ',') 
 	    b_ok = 1;
	if (!b_ok)
	    return (0);
	b_ok = 0;	/* check right side */
	if (tks[end + 1] == NULL)
	    b_ok = 1;
	else if ((c = tks[end + 1][0]) == ']' || c == ')' || c == '+' || c == '-'|| c == ',') 
 	    b_ok = 1;
	if (!b_ok)
	    return (0);
    }
    FU_replace_tks (tks, end, 1, NULL, 0);
    FU_replace_tks (tks, st, 1, NULL, 0);
    return (1);
}


