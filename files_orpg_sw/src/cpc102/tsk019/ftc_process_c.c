
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains processing functions.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/08 17:13:30 $
 * $Id: ftc_process_c.c,v 1.3 2010/11/08 17:13:30 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

enum {CL_OTHERS, CL_BLANK, CL_COMMENT};	/* c line type */

static char *Get_comment_text (int n_spaces, int comment, char *buf, int is_c);
static int Is_comment (int cfile, int lcnt, char *buf);
static int Process_line (char *buf, int level, int cfile, int line);
static char *Get_new_identifier (char *id);
static void Build_identifier_table (int cfile);
static void Set_new_identifiers ();
static char *Create_new_id (char *oid, char *nid);
static int Id_cmp (void *e1, void *e2);

/******************************************************************

    Processes the input file and generates output.

******************************************************************/

void FP_process_cfile (char *in_file, char *out_file) {
    char buf[MAX_STR_SIZE], *prolog;
    int n_lines, cfile, cnt, in_include, pl_type, level, cr_line;

    if (strncmp (in_file + (strlen (in_file) - 2), ".c", 2) != 0) {
	fprintf (stderr, "Unexpected input c file name (%s)\n", in_file);
	exit (1);
    }

    cfile = FF_open_file (in_file, NULL, &n_lines);

    prolog = FP_get_prologue (cfile, 0, n_lines, 1);

    Build_identifier_table (cfile);

    in_include = 0;		/* the line is from FORTRAN include file */
    pl_type = CL_OTHERS;		/* type of previoud output line */
    level = 0;			/* C block level */
    cnt = -1;			/* line count */
    while (FF_get_line (cfile, cnt + 1, buf, MAX_STR_SIZE) >= 0) {
	int n;

	cnt++;
	FM_tk_free ();
	n = Is_comment (cfile, cnt, buf);
	if (n > 0) {

	    if (cnt == 0) {
		cnt += n - 1;
		continue;	/* discard the f2c prologue */
	    }
	    cnt += n - 1;
	    if (strstr (buf, "/* INCLUDE ") != NULL)
		in_include = 1;
	    if (in_include) {	/* discard comments from include files */
		if (strstr (buf, "/* INCLUDE - DONE ") != NULL)
		    in_include = 0;
		continue;
	    }
	    if (strncmp (buf, "/* .", 4) == 0)	/* discard MODULE PROLOGUE */
		continue;
	    if (strncmp (buf, "/* RCS", 6) == 0 ||
		strncmp (buf, "/* $", 4) == 0)	/* discard RCS lines */
		continue;
	    if (buf[3] == '@' && buf[strlen (buf) - 5] == '#') {
		sscanf (buf + 4, "%d", &cr_line);
		continue;			/* line number lable */
	    }
	    if (FP_process_comment (buf, cnt, 1) == 0)
		continue;
	}
	else {
	    char *s, *p;
	    s = "/* Subroutine */ ";
	    if ((p = strstr (buf, s)) != NULL) {
		if (level == 0) {	/* output prologue */
		    FF_output_text (out_file, prolog, strlen (prolog));
		    FF_output_text (out_file, "\n", 1);
		}
		FP_str_replace (buf, p - buf, strlen (s), "");
	    }
	    n = Process_line (buf, level, cfile, cnt);
	    if (n > 0) {
		cnt += n - 1;
		continue;
	    }
	}

	if (buf[0] == '\n' && (pl_type == CL_BLANK || pl_type == CL_COMMENT))
	    continue;	/* blank line following blank of comment discarded */

	pl_type = CL_OTHERS;		/* set pl_type for future use */
	if (n > 0)
	    pl_type = CL_COMMENT;
	else if (buf[0] == '\n')
	    pl_type = CL_BLANK;

	/* output the current line */
	if (n > 0) {	/* add space to align comment */
	    char *p;
	    int i;
	    for (i = 0; i < level; i++)
		FF_output_text (out_file, "    ", 4);
	    p = buf + MISC_char_cnt (buf, " \t");
	    FF_output_text (out_file, p, strlen (p));
	}
	else
	    FF_output_text (out_file, buf, strlen (buf));

	if (n == 0) {		/* upate level */
	    if (strstr (buf, "{") != NULL)
		level++;
	    if (strstr (buf, "}") != NULL)
		level--;
	}
    }

}

/******************************************************************

    Processes line "buf" in c section "level". Returns 0 if the line
    is to be discarded or 1 otherwise.

******************************************************************/

static int Process_line (char *buf, int level, int cfile, int line) {
    static char *o_type[] = {"integer", "real", "logical", "shortint",
	"address", "doublereal", "shortlogical", "logical1",
	"integer1"};
    static char *n_type[] = {"int", "float", "int", "short",
	"char *", "double", "short", "char",
	"char"};
    static int n_types = sizeof (o_type) / sizeof (char *);
    char *tks[MAX_TKS];
    int tof[MAX_TKS], ind, nt, i;

    /* remove "extern function" lines */
    nt = FP_get_tokens (buf, 5, tks, NULL);
    if (nt >= 5 && strcmp (tks[0], "extern") == 0 && 
			tks[3][0] == '(' && tks[4][0] == ')') {
	int cnt = 1;
	while ((nt = FP_get_tokens (buf, MAX_TKS, tks, NULL)) > 0 && 
		tks[nt - 2][0] != ';' &&
	 	FF_get_line (cfile, line + cnt, buf, MAX_STR_SIZE) >= 0)
	    cnt++;
	return (cnt);
    }

    /* remove static inside the subroutine */
    if (level > 0) {
	nt = FP_get_tokens (buf, 1, tks, tof);
	if (strcmp (tks[0], "static") == 0)
	    FP_str_replace (buf, tof[0], strlen (tks[0]) + 1, "");
    }

    /* process type names */
    nt = FP_get_tokens (buf, 2, tks, tof);
    ind = 0;
    if (strcmp (tks[0], "static") == 0 || strcmp (tks[0], "extern") == 0)
	ind = 1;
    for (i = 0; i < n_types; i++) {
	if (strcmp (o_type[i], tks[ind]) == 0) {
	    FP_str_replace (buf, tof[ind], strlen (tks[ind]), n_type[i]);
	    break;
	}
    }

    /* modify identifiers */
    nt = FP_get_tokens (buf, MAX_TKS, tks, tof);
    for (i = nt - 1; i >= 0; i--) {
	char *nd;
	if (!FP_is_char (tks[i][0]))
	    continue;
	if ((nd = Get_new_identifier (tks[i])) != NULL)
	    FP_str_replace (buf, tof[i], strlen (tks[i]), nd);
    }

    return (0);
}

/******************************************************************

    Processes comment line "buf". Returns 0 if the line is to be 
    discarded or 1 otherwise.

******************************************************************/

int FP_process_comment (char *buf, int cnt, int is_c) {
    char *tks[3], *p;
    int nt, off;

    if (is_c) {
	nt = FP_get_tokens (buf + 2, 3, tks, NULL);
	off = 2;
    }
    else {
	nt = FP_get_tokens (buf + 1, 3, tks, NULL);
	off = 1;
    }
    if (nt >= 2 && strcasecmp (tks[0], "write") == 0 && tks[1][0] == '(')
	return (0);		/* commented WRITE */
    if (nt >= 3 && strcasecmp (tks[1], "format") == 0 && tks[2][0] == '(')
	return (0);		/* commented FORMAT */
    if (strncmp (buf + off, "    ", 4) == 0 && 
			!FP_is_char (buf[off + 4]) && buf[off + 4] != ' ')
	return (0);		/* FORTRAN continue line  */
    if (nt >= 2 && strcasecmp (tks[0], "if") == 0 && tks[1][0] == '(')
	return (0);		/* commented IF */
    if (nt >= 2 && strcasecmp (tks[0], "else") == 0 && 
				(tks[1][0] == '*' || tks[1][0] == '\n'))
	return (0);		/* commented ELSE */
    if (nt >= 2 && strcasecmp (tks[0], "endif") == 0 && 
				(tks[1][0] == '*' || tks[1][0] == '\n'))
	return (0);		/* commented ENDIF */
    if (nt >= 2 && strcmp (tks[0], "TYPE") == 0 && tks[1][0] == '*')
	return (0);		/* commented TYPE */

    /* if all chars are upper case, we change to lower case except the first */
    p = buf + 1;
    while (*p != '\0') {
	if (*p <= 122 && *p >= 97)	/* a lower case char */
	    break;
	p++;
    }
    if (*p == '\0') {
	int c = 0;
	p = buf + 1;
	while (*p != '\0') {
	    if (*p <= 90 && *p >= 65) {
		if (c > 0)
		    *p = *p + 32;
		c++;
	    }
	    p++;
	}
    }

    /* Remove digits beyond 72 chars and trailing spaces */
    if (is_c) {
	if (cnt > 0 && strlen (buf) > 75) {
	    p = buf + strlen (buf) - 4;
	    while ((p >= buf && (*p == ' ' || *p == '\n' || *p == '\t')) || (
				p >= buf + 72 && FP_is_digit (*p)))
		p--;
	    strcpy (p + 2, "*/\n");
	}
    }
    else {
	if (strlen (buf) > 72) {
	    p = buf + strlen (buf) - 1;
	    while ((p >= buf && (*p == ' ' || *p == '\n' || *p == '\t')) || 
				(p >= buf + 71 && FP_is_digit (*p)))
		p--;
	    strcpy (p + 1, "\n");
	}
    }

    return (1);
}

/******************************************************************

    Get the first "mnt" tokens from "text" and returns them with
    "tks". "b" is a STR buffer. If "tof" is not NULL, the offset of
    each token is returned. Returns the number of tokens returned.

******************************************************************/

static char **Keys = NULL;
void FP_put_keys (char **keys) {
    Keys = keys;
}

int FP_get_tokens (char *text, int mnt, char **tks, int *tof) {
    char *st, *end;
    int cnt, in_comment;

    st = text;
    in_comment = 0;
    cnt = 0;
    while (*st != '\0') {
	int len, num_const;
	char str_quot;

	st += MISC_char_cnt (st, " \t");
	if (*st == '\0')
	    break;
	if (in_comment) {
	    if (strncmp (st, "*/", 2) == 0) {
		in_comment = 0;
		st += 2;
	    }
	    else
		st++;
	    continue;
	}
	if (strncmp (st, "/*", 2) == 0) {
	    in_comment = 1;
	    st += 2;
	    continue;
	}

	end = st;
	if (Keys != NULL && *end == '.') {
	    char *p1, *p2, *p3;
	    int k = 0;
	    p1 = p2 = NULL;
	    while (Keys[k] != NULL) {
		char *pk, *p;
		pk = Keys[k];
		p = end + 1;
		while (*p == ' ') {
		    p1 = p;		/* last space between . and, eg, EQ */
		    p++;
		}
		while (*p != '\0' && *pk != '\0') {
		    int c1, c2;
		    c1 = *p;		/* case insensitive comparing */
		    if (c1 > 90)
			c1 -= 32;
		    c2 = *pk;
		    if (c2 > 90)
			c2 -= 32;
		    if (c1 != c2)
			break;
		    pk++;
		    p++;
		}
		p3 = p;
		while (*p3 == ' ') {
		    if (p2 == NULL)
			p2 = p3;	/* first space between, eg EQ and . */
		    p3++;
		}
		if (*pk == '\0' && *p3 == '.') {
		    if (p1 != NULL) {
			*p1 = '.';	/* move . close to EQ */
			*end = ' ';
		    }
		    if (p2 != NULL) {
			*p2 = '.';	/* move . close to EQ */
			*p3 = ' ';
		    }
		    end = p3 + 1;
		    break;
		}
		k++;
	    }
	}

	if (Keys != NULL && end == st) {	/* FTN hex const */
	    if ((*end == 'X' || *end == 'x') && end[1] == '\'') {
		end += 2;
		while (*end != '\0' && *end != '\'')
		    end++;
		if (*end != '\'' || end - st > 6)
		    end = st;	/* not a hex const */
		else
		    end++;
	    }	
	}

	if (end == st) {
	    str_quot = 0;
	    if (*end == '\'' || *end == '"') {
		str_quot = *end;
		end++;
	    }
	    num_const = 0;
	    if (FP_is_digit (*end) || (*end == '.' && FP_is_digit (end[1])))
		num_const = 1;
	    while (*end != '\0') {
		if (str_quot) {
		    if ((*end == str_quot) && end[-1] != '\\') {
			end++;
			break;
		    }
		}
		else if (num_const) {
		    int yes = FP_is_digit (*end) ||
			(*end == '.' && (end == st || FP_is_digit (end[-1]))) ||
			((*end == 'e' || *end == 'E') && 
			 (FP_is_digit (end[1]) || end[1] == '+' || 
							    end[1] == '-')) ||
			((*end == '+' || *end == '-') && 
			 (end[-1] == 'e' || end[-1] == 'E') && 
			 FP_is_digit (end[1]));
		    if (!yes)
			break;
		}
		else if (!FP_is_char (*end))
		    break;
		end++;
	    }
	}

	len = end - st;
	if (len >= MAX_STR_SIZE)
	    len = MAX_STR_SIZE - 1;
	if (len == 0)
	    len = 1;
	tks[cnt] = FM_tk_malloc (len + 1);
	memcpy (tks[cnt], st, len);
	tks[cnt][len] = '\0';
	if (tof != NULL)
	    tof[cnt] = st - text;
	cnt++;
	if (end == st)
	    st = end + 1;
	else
	    st = end;
	if (cnt >= mnt)
	    break;
    }
    if (tof != NULL)
	tof[cnt] = st - text;
    
    return (cnt);
}

/****************************************************************************

    Returns 1 if c is a English char or a number or "_" or 0 otherwise.

****************************************************************************/

int FP_is_char (char c) {
    if ((c <= 57 && c >= 48) || (c <= 90 && c >= 65) || (c <= 122 && c >= 97) || c == '_')
	return (1);
    return (0);
}

/****************************************************************************

    Returns 1 if c is a English char or a number or "_" or 0 otherwise.

****************************************************************************/

int FP_is_digit (char c) {
    if (c <= 57 && c >= 48)
	return (1);
    return (0);
}

/******************************************************************

    Replaces "n_bytes" bytes at offset "off" in "buf" by "str".

******************************************************************/

void FP_str_replace (char *buf, int off, int n_bytes, char *str) {

    memmove (buf + off + strlen (str), buf + off + n_bytes, 
				strlen (buf + off + n_bytes) + 1);
    memcpy (buf + off, str, strlen (str));
}

/******************************************************************

    Returns the number of lines if this starts a comment or 0 otherwise.

******************************************************************/

static int Is_comment (int cfile, int lcnt, char *buf) {

    if (strncmp (buf + MISC_char_cnt (buf, " \t"), "/*", 2) == 0) {
	char *p, b[MAX_STR_SIZE];
	int next = 0;
	p = buf + MISC_char_cnt (buf, " \t") + 2;
	while (1) {
	    char *p1 = strstr (p, "*/");
	    if (p1 != NULL) {
		p1 += 2;
		p1 += MISC_char_cnt (p1, " \t\n");
		if (*p1 == '\0')
		    return (next + 1);
		if (next == 0)
		    return (0);
		else {
		    fprintf (stderr, "Unexpected comment section - text follows multi-line\n");
		    exit (1);
		}
	    }
	    next++;
	    if (FF_get_line (cfile, lcnt + next, b, MAX_STR_SIZE) <= 0) {
		fprintf (stderr, "Comment section not closed\n");
		exit (1);
	    }
	    p = b + MISC_char_cnt (b, " \t");
	}
    }
    else
	return (0);
}

/******************************************************************

    Creats the prologue for the file by reading info from the FORTRAN
    MODULE PROLOGUE. Returns the prologue.

******************************************************************/

char *FP_get_prologue (int file, int st_l, int end_l, int is_c) {
    static char *desc_b = NULL;
    char buf[MAX_STR_SIZE];
    int desc, param, inp, out, cnt;

    desc_b = STR_reset (desc_b, 1024);
    desc_b = STR_copy (desc_b, "\n/*\\//////////////////////////////////////////////////////////////////////\n\n   Description:\n");
    desc = param = inp = out = 0;
    cnt = st_l - 1;
    while (cnt + 1 < end_l &&
		FF_get_line (file, cnt + 1, buf, MAX_STR_SIZE) >= 0) {
	char *p;

	cnt++;
	if ((is_c && strncmp (buf, "/*", 2) != 0) ||
	    (!is_c && buf[0] == ' '))
	    desc = param = inp = out = 0;
	else {
	    if (strstr (buf, "MODULE FUNCTION:") != NULL) {
		desc = 1;	/* description */
		continue;
	    }
	    else if (strstr (buf, "MODULES ") != NULL) {
		desc = 0;
		continue;
	    }
	    else if (strstr (buf, "PARAMETERS:") != NULL) {
		param = 1;	/* input/output */
		desc = 0;
		continue;
	    }
	    else if (strstr (buf, "DATABASE/FILE") != NULL ||
		     strstr (buf, "INTERNAL TABLES") != NULL) {
		desc = param = inp = out = 0;
		break;
	    }
	}

	if (param) {
	    if (strstr (buf, "*   INPUT") != NULL) {
		desc_b = STR_cat (desc_b, "\n   Inputs:\n");
		inp = 1;
		continue;
	    }
	    else if (strstr (buf, "*   OUTPUT") != NULL) {
		desc_b = STR_cat (desc_b, "\n   Outputs:\n");
		inp = 0;
		out = 1;
		continue;
	    }
	    else if (strstr (buf, "*   ACTUAL") != NULL) {
		inp = out = 0;
		continue;
	    }
	}

	if (desc) {
	    p = Get_comment_text (6, 0, buf, is_c);
	    if (strlen (p) > 6) {
		desc_b = STR_cat (desc_b, p);
		desc_b = STR_cat (desc_b, "\n");
	    }
	}
	else if (inp || out) {
	    char tok[MAX_STR_SIZE], var[MAX_STR_SIZE];
	    p = Get_comment_text (0, 0, buf, is_c);
	    if (MISC_get_token (p, "", 0, tok, MAX_STR_SIZE) > 0 &&
		strcmp (tok, "P") == 0 &&
		MISC_get_token (p, "", 1, var, MAX_STR_SIZE) > 0) {
		int i;
		int off = MISC_get_token (p, "", 3, tok, MAX_STR_SIZE);
		MISC_tolower (var);
		desc_b = STR_cat (desc_b, "      ");
		desc_b = STR_cat (desc_b, var);
		desc_b = STR_cat (desc_b, " - ");
		desc_b = STR_cat (desc_b, p + off - strlen (tok) - 1);
		desc_b = STR_cat (desc_b, "\n");

		for (i = 0; i < 10; i++) {
		    char ss[64], buf1[MAX_STR_SIZE];
		    if (FF_get_line (file, cnt + i + 1, buf1, 
						MAX_STR_SIZE) <= 0 ||
			strstr (buf1, 
				"                               ") == NULL)
			break;
		    p = Get_comment_text (0, 0, buf1, is_c);
		    strcpy (ss, "                                        ");
		    ss[6 + strlen (var) + 3] = '\0';
		    desc_b = STR_cat (desc_b, ss);
		    desc_b = STR_cat (desc_b, p);
		    desc_b = STR_cat (desc_b, "\n");
		}
	    }
	}
    }
    desc_b = STR_cat (desc_b, "\n///////////////////////////////////////////////////////////////////////\\*/\n");
    return (desc_b);
}

/******************************************************************

    Gets and returns the text of comment line in "buf". "n_spaces"
    spaces are added in front if comment is false. If comment is true,
    c comment delimiters are added. Leading "." are removed.

******************************************************************/

static char *Get_comment_text (int n_spaces, int comment, char *buf, int is_c) {
    static char *b = NULL;
    char *p, *pe;
    int i;

    b = STR_copy (b, "");
    if (comment)
	b = STR_cat (b, "/* ");
    else {
	for (i = 0; i < n_spaces; i++)
	    b = STR_cat (b, " ");
    }
    if (is_c)
	p = buf + MISC_char_cnt (buf, "/*. \t");
    else
	p = buf + 1 + MISC_char_cnt (buf + 1, "*. \t");
    pe = p + strlen (p) - 1;
    if (is_c && pe >= p + 2 &&
	strncmp (pe - 2, "*/", 2) == 0)
	pe -= 2;
    pe--;
    while (pe >= p) {
	if (*pe == ' ' || *pe == '\t')
	    pe--;
	else
	    break;
    }
    if (pe >= p) {
	char tb[MAX_STR_SIZE];
	memcpy (tb, p, pe - p + 1);
	tb[pe - p + 1] = '\0';
	b = STR_cat (b, tb);
    }
    if (comment)
	b = STR_cat (b, " */");
    return (b);
}

/******************************************************************

    Builds the identifier table and figures out the replacement
    identifiers.

******************************************************************/

typedef struct {
    char *o_id;
    char *n_id;
} Id_table_t;

static Id_table_t *Ids = NULL;	/* identifier table entries */
static int N_ids = 0;			/* # of identifier table entries */
static void *Id_tblid = NULL;		/* table id for identifier table */

static void Build_identifier_table (int cfile) {
    int cnt, i;
    char buf[MAX_STR_SIZE];

    Id_tblid = MISC_open_table (sizeof (Id_table_t), 
				256, 1, &N_ids, (char **)&Ids);
    if (Id_tblid == NULL) {
	fprintf (stderr, "MISC_open_table failed\n");
	exit (1);
    }

    cnt = -1;
    while (FF_get_line (cfile, cnt + 1, buf, MAX_STR_SIZE) >= 0) {
	char *tks[MAX_TKS];
	Id_table_t id;
	int nt, ind, n;

	cnt++;
	n = Is_comment (cfile, cnt, buf);
	if (n > 0) {
	    cnt += n - 1;
	    continue;
	}
	FM_tk_free ();
	nt = FP_get_tokens (buf, MAX_TKS, tks, NULL);
	for (i = 0; i < nt; i++) {
	    if (!FP_is_char (tks[i][0]))
		continue;
	    id.o_id = tks[i];
	    if (MISC_table_search (Id_tblid, &id, Id_cmp, &ind) == 1)
		continue;	/* already in the table */
	    id.o_id = MISC_malloc (2 * (strlen (tks[i]) + 1));
	    id.n_id = id.o_id + strlen (tks[i]) + 1;
	    strcpy (id.o_id, tks[i]);
	    id.n_id[0] = '\0';
	    if (MISC_table_insert (Id_tblid, &id, Id_cmp) < 0) {
		fprintf (stderr, "MISC_table_insert failed\n");
		exit (1);
	    }
	}
    }

    Set_new_identifiers ();
}

/******************************************************************

    Comparation function for identifier table search.

******************************************************************/

static int Id_cmp (void *e1, void *e2) {
    Id_table_t *id1, *id2;
    id1 = (Id_table_t *)e1;
    id2 = (Id_table_t *)e2;
    return (strcmp (id1->o_id, id2->o_id));
}

/******************************************************************

    Returns the new identifier for replacing "id". Returns NULL is
    replacement is not needed.

******************************************************************/

static char *Get_new_identifier (char *id) {
    Id_table_t idn;
    int ind;

    idn.o_id = id;
    if (MISC_table_search (Id_tblid, &idn, Id_cmp, &ind) == 1) {
	if (Ids[ind].n_id != Ids[ind].o_id)
	    return (Ids[ind].n_id);
	return (NULL);
    }
    return (NULL);
}


/******************************************************************

    Sets new identifiers for replacing the old ones.

******************************************************************/

static void Set_new_identifiers () {
    int i;

    for (i = 0; i < N_ids; i++) {
	Id_table_t *id = Ids + i;
	if (Create_new_id (id->o_id, id->n_id) == NULL)
	    id->n_id = id->o_id;
    }

    /* make sure there is no identical ones */
    while (1) {
	int fine = 1;
	for (i = 0; i < N_ids; i++) {
	    int k;
	    for (k = i + 1; k < N_ids; k++) {
		if (strcmp (Ids[k].n_id, Ids[i].n_id) == 0) {
		    if (Ids[i].n_id != Ids[i].o_id)
			Ids[i].n_id = Ids[i].o_id;  /* reverse the change */
		    else
			Ids[k].n_id = Ids[k].o_id;  /* reverse the change */
		    fine = 0;
		    break;
		}
	    }
	    if (!fine)
		break;
	}
	if (fine)
	    break;
    }
}

/****************************************************************************

    Generate the new identifier for "old" and put it in "nid". Returns nid
    if there is a new identifier or NULL otherwise.

****************************************************************************/

static char *Create_new_id (char *oid, char *nid) {
    char buf[MAX_STR_SIZE], *id;
    int len;

    id = oid;
    len = strlen (id);
    if (len > 2 && strcmp (id + len - 2, "__") == 0) {
	strncpy (buf, id, len - 2);
	buf[len - 2] = '\0';
	strcpy (nid, buf);
	id = nid;
    }

    if (strlen (id) > 8 && id[0] == 'a' && 
	FP_is_digit (id[1]) && FP_is_digit (id[2]) && FP_is_digit (id[3]) && 
	FP_is_digit (id[4]) && FP_is_char (id[5]) && 
	id[6] == '_' && id[7] == '_') {
	strcpy (buf, id + 8);
	if (buf[0] <= 122 && buf[0] >= 97)
	    buf[0] -= 32;
	strcpy (nid, buf);
	id = nid;
    }
    if (id == oid)
	return (NULL);
    else
	return (nid);
}

