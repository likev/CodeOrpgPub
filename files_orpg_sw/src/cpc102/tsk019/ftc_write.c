
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains the functions for processing WRITE.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/01 21:52:59 $
 * $Id: ftc_write.c,v 1.2 2010/11/01 21:52:59 jing Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

static char Err_msg[MAX_STR_SIZE];

static char *Get_c_format (char *fmt, char **tks, int nt, int *vs);


/******************************************************************

    Converts the FTN write format "fmt" to c format string and returns
    it. "tks" are the variable list of the write. Substring specs in 
    "tks" are processed and removed. This is not a complete 
    implementatin. It returns NULL if it does not recognize the format.

******************************************************************/

#define MAX_W_VARS 32

char *Get_c_format (char *fmt, char **tks, int nt, int *vs) {
    static char out[MAX_STR_SIZE];
    char *p, tk[MAX_STR_SIZE], buf[MAX_STR_SIZE], type[MAX_W_VARS];
    int size[MAX_W_VARS], free_f, ind, cnt, type_ind;

    free_f = 0;
    if (strcmp (fmt, "*") == 0)		/* free format */
	free_f = 1;

    *vs = 0;
    ind = 0;
    cnt = 0;
    strcpy (out, "\"");
    while (ind < nt) {
	char b[MAX_STR_SIZE];
	Ident_table_t *id;

	if (tks[ind][0] == '(')	{	/* looping write */
	    sprintf (Err_msg, "Looping not implemented");
	    return (NULL);
	}
	if (cnt >= MAX_W_VARS) {
	    sprintf (Err_msg, "Too many variables");
	    return (NULL);
	}
	id = FS_var_search (tks[ind]);
	if (id == NULL) {
	    int l = strlen (tks[ind]);
	    if (tks[ind][0] != '"' || tks[ind][l - 1] != '"') {
		sprintf (Err_msg, "Variable %s not defined", tks[ind]);
		return (NULL);
	    }
	    type[cnt] = T_CHAR;
	    size[cnt] = l - 2;
	}
	else {
	    type[cnt] = id->type;
	    size[cnt] = id->size;
	    if (id->type == T_CHAR) {
		int n, nopr;
		char b[MAX_STR_SIZE], *ts[MAX_TKS], offp[MAX_STR_SIZE],
						sizep[MAX_STR_SIZE];
		*vs = 1;
		n = FR_parse_char_opr (tks, ind, nt, offp, sizep, &nopr);
		if (n > nopr) {		/* remove substring spec */
		    FU_replace_tks (tks, ind + nopr, n - nopr, NULL, 0);
		    nt -= n - nopr;
		}
    
		if (offp[0] != '\0') {	/* add offset */
		    sprintf (b, " + %s", offp);
		    n = FP_get_tokens (b, MAX_TKS, ts, NULL);
		    FU_replace_tks (tks, ind + nopr, 0, ts, n);
		    nt += n;
		}
		size[cnt] = FE_evaluate_int_expr (sizep, 0);
	    }
	}

	if (free_f) {
	    if (type[cnt] == T_CHAR && size[cnt] > 0)
		sprintf (b, "%c%ds", '%', size[cnt]);
	    else if (type[cnt] == T_INT)
		strcpy (b, "%d");
	    else if (type[cnt] == T_REAL || type[cnt] == T_DOUBLE)
		strcpy (b, "%g");
	    else {
		sprintf (Err_msg, "Variable type (%d) not supported", type[cnt]);
		return (NULL);
	    }
	    if (cnt > 0)
		strcat (out, " ");
	    strcat (out, b);
	}

	cnt++;
	ind = FU_next_token (ind, nt, tks, ',') + 1;
    }
    if (free_f) {
	strcat (out, "\"");
	return (out);
    }

    strcpy (buf, fmt);
    p = buf + strlen (buf) - 1;
    while (p >= buf) {
	if (*p == '"' || *p == ' ' || *p == ')')
	    *p = '\0';
	else
	    break;
	p--;
    }
    p = buf + MISC_char_cnt (buf, " \"'(");

    strcpy (out, "\"");
    ind = 0;
    type_ind = 0;
    while (MISC_get_token (p, "S,", ind, tk, MAX_STR_SIZE) > 0) {
	char b[MAX_STR_SIZE], *tp;
	int rc, f, i1, i2;

	if (*tk == '\'') {
	    tp = tk + strlen (tk) - 1;
	    if (*tp != '\'') {
		sprintf (Err_msg, "Unclosed string constant");
		return (NULL);
	    }
	    memcpy (b, tk + 1, tp - tk - 1);
	    b[tp - tk - 1] = '\0';
	    strcat (out, b);
	    ind++;
	    continue;
	}

	tp = tk;
	i1 = i2 = -1;
	if (sscanf (tp, "%d", &rc) == 1) {
	    while (FP_is_digit (*tp))
		tp++;
	}
	else
	    rc = 1;
	f = tp[0];
	if (f == '\0') {
	    sprintf (Err_msg, "Format not implemented");
	    return (NULL);
	}
	tp++;
	if (sscanf (tp, "%d", &i1) == 1) {
	    while (FP_is_digit (*tp))
		tp++;
	}
	if (tp[0] == '.') {
	    tp++;
	    if (sscanf (tp, "%d", &i2) == 1) {
		while (FP_is_digit (*tp))
		    tp++;
	    }
	}
	if (tp[0] != '\0')
	    goto err;

	if (f == 'i' || f == 'I' || f == 'l' || f == 'L' ) {
	    if (i1 < 0 || i2 >= 0)
		goto err;
	    if (type_ind >= cnt)
		goto err1;
	    if (type[type_ind] != T_INT) {
		sprintf (Err_msg, "Var type does not match int");
		return (NULL);
	    }
	    sprintf (b, "%c%dd", '%', i1);
	    type_ind++;
	}
	else if (f == 'f' || f == 'F') {
	    if (i2 < 0)
		goto err;
	    if (type_ind >= cnt)
		goto err1;
	    if (type[type_ind] != T_REAL) {
		sprintf (Err_msg, "Var type does not match float");
		return (NULL);
	    }
	    sprintf (b, "%c%d.%df", '%', i1, i2);
	    type_ind++;
	}
	else if (f == 'e' || f == 'E') {
	    if (i2 < 0)
		goto err;
	    if (type_ind >= cnt)
		goto err1;
	    if (type[type_ind] != T_REAL) {
		sprintf (Err_msg, "Var type does not match float");
		return (NULL);
	    }
	    sprintf (b, "%c%d.%dE", '%', i1, i2);
	    type_ind++;
	}
	else if (f == 'd' || f == 'D') {
	    if (i2 < 0)
		goto err;
	    if (type_ind >= cnt)
		goto err1;
	    if (type[type_ind] != T_DOUBLE) {
		sprintf (Err_msg, "Var type does not match double");
		return (NULL);
	    }
	    sprintf (b, "%c%d.%dE", '%', i1, i2);
	    type_ind++;
	}
	else if (f == 'a' || f == 'A') {
	    if (i1 < 0 || i2 >= 0)
		goto err;
	    if (type_ind >= cnt)
		goto err1;
	    if (type[type_ind] != T_CHAR) {
		sprintf (Err_msg, "Var type does not match string");
		return (NULL);
	    }
	    if (i1 != size[type_ind]) {
		sprintf (Err_msg, "Format str size not equal var size");
		return (NULL);
	    }
	    sprintf (b, "%c%ds", '%', i1);
	    type_ind++;
	}
	else if (f == 'x' || f == 'X') {
	    int k;
	    if (i1 >= 0)
		goto err;
	    b[0] = '\0';
	    for (k = 0; k < rc; k++)
		strcat (b, " ");
	}
	else if (f == '/') {
	    int k;
	    if (i1 >= 0)
		goto err;
	    b[0] = '\0';
	    for (k = 0; k < rc; k++)
		strcat (b, "\n");
	}
	else
	    goto err;;
	strcat (out, b);
	ind++;
    }
    if (type_ind < cnt) {
	sprintf (Err_msg, "Too many vars in term of format");
	return (NULL);
    }
    strcat (out, "\"");

    return (out);

err:
    sprintf (Err_msg, "Format not implemented");
    return (NULL);
err1:
    sprintf (Err_msg, "Too few vars in term of format");
    return (NULL);
}

/******************************************************************

    Returns the current error message.

******************************************************************/

char *FW_get_err_msg () {
    return (Err_msg);
}

/******************************************************************

    Processes FTN WRITE line of "tks" of "n_tks" tokens. The output
    is put in "otb" and is NULL-terminated. "tks" is not modified.

******************************************************************/

int FW_process_write (char **tks, int n_tks, char **otb) {
    char *tb[MAX_TKS], *fmt, *ts[MAX_TKS];
    int st, end, e1, dev, label, line_ret, vs, i;
    Ident_table_t *id;

    Err_msg[0] = '\0';
    otb[0] = NULL;

    end = 0;
    if (n_tks < 3 ||
	strcasecmp (tks[0], "WRITE") != 0 ||
	tks[1][0] != '(' ||
	(end = FU_next_token (2, n_tks, tks, ')')) >= n_tks) {
	sprintf (Err_msg, "Format to be supported");
	return (-1);
    }

    st = 2;
    e1 = FU_next_token (st, end, tks, ',');
    if (e1 >= end) {
	sprintf (Err_msg, "Unexpected WRITE line - missing ','");
	return (-1);
    }

    st = e1 + 1;
    if (end != st + 1) {		/* format to be supported */
	sprintf (Err_msg, "Format to be supported");
	return (-1);
    }

    fmt = tks[st];
    if (sscanf (fmt, "%d", &label) == 1 &&
	FS_get_format (label) != NULL)
	fmt = FS_get_format (label);

    for (i = end + 1; i < n_tks; i++)
	ts[i] = tks[i];
    ts[n_tks] = NULL;
    fmt = Get_c_format (fmt, ts + end + 1, n_tks - (end + 1), &vs);
    if (fmt == NULL)
	return (-1);

    st = 2;
    line_ret = 1;
    if (e1 == st + 1 &&
	((sscanf (tks[st], "%d", &dev) == 1 && dev == 6) ||
	 strcmp (tks[st], "*") == 0)) {
	if (!vs)
	    FU_ins_tks (otb, -1, "printf", "(", NULL);
	else
	    FU_ins_tks (otb, -1, "lprintf", "(", "(", "char", "*", ")", "6", ",", "0", ",", NULL);
    }
    else if ((id = FS_var_search (tks[st])) != NULL &&
	     id->type == T_CHAR) {
	int nopr, n;
	char offp[MAX_STR_SIZE], sizep[MAX_STR_SIZE],
					b[MAX_STR_SIZE], *tts[MAX_TKS];

	line_ret = 0;
	FU_ins_tks (otb, -1, "lprintf", "(", NULL);

	FR_parse_char_opr (tks, st, n_tks, offp, sizep, &nopr);
	FC_process_expr (tks + st, nopr, tb, 0);

	if (offp[0] != '\0') {
	    sprintf (b, " + %s", offp);
	    n = FP_get_tokens (b, MAX_TKS, tts, NULL);
	    FU_replace_tks (tb, -1, 0, tts, n);
	}
	sprintf (b, ",%s,", sizep);
	n = FP_get_tokens (b, MAX_TKS, tts, NULL);
	FU_replace_tks (tb, -1, 0, tts, n);

	FU_replace_tks (otb, -1, 0, tb, 0);
	FU_post_process_c_tks (otb);
    }
    else {
	FU_ins_tks (otb, -1, "lprintf", "(", "(", "char", "*", ")", NULL);
	FC_process_expr (tks + st, e1 - st + 1, tb, 0);
	FU_replace_tks (otb, -1, 0, tb, 0);
	FU_ins_tks (otb, -1, "0", ",", NULL);
    }

    if (line_ret)
	FP_str_replace (fmt, strlen (fmt) - 1, 0, "\\n");
    FU_ins_tks (otb, -1, fmt, ",", NULL);

    FC_process_expr (ts + end + 1, 0, tb, 0);
    FU_replace_tks (otb, -1, 0, tb, 0);
    FU_ins_tks (otb, -1, ")", ";", NULL);
    return (0);
}
