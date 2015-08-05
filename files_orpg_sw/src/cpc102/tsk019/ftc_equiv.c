
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains the functions for EQUIVALENCE processing.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/01 21:52:55 $
 * $Id: ftc_equiv.c,v 1.2 2010/11/01 21:52:55 jing Exp $
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
    int group;
    char *var1;
    char *var2;
    Ident_table_t *id1;
    Ident_table_t *id2;
    char *eq;
    char *cast;
    int level;
} Eq_t;

typedef struct {		/* local fields for Ident_table_t.loc */
    int up;
    int low;
    int pad;
} Id_more_t;
#define OFF_UNDEF -100000000		/* undefined value for offset */

static Eq_t *Eqs = NULL;		/* equivalence table entries */
static int N_eqs = 0;			/* # of Eqs entries */
static void *Eq_tblid = NULL;		/* table id for Eqs table */

static void Set_eq_cast (Ident_table_t *id1, Ident_table_t *d2, Eq_t *eq);
static int Get_var_size (Ident_table_t *id, int common_size);
static int Get_var_offset (Ident_table_t *id, char *dimp, char **otb);
static int Get_eq_offset (char *eq, int position, Ident_table_t *id);
static int Get_padding_size (Ident_table_t *id, int nbytes);
static int Get_num_offset (Ident_table_t *id, char *dimp);
static int Get_common_size (Ident_table_t *id, int full_size);


/******************************************************************

    Processes EQUIVALENCE statements.

******************************************************************/

void FE_process_eq () {
    int i, grp, g;

    if (N_eqs <= 0)
	return;

    /* initialize id1, id2, and their loc field */
    for (i = 0; i < N_eqs; i++) {
	Eq_t *eq = Eqs + i;
	if ((eq->id1 = FS_var_search (eq->var1)) == NULL) {
	    fprintf (stderr, "Code error - eq var %s not found\n", eq->var1);
	    exit (1);
	}
	if ((eq->id2 = FS_var_search (eq->var2)) == NULL) {
	    fprintf (stderr, "Code error - eq var %s not found\n", eq->var2);
	    exit (1);
	}
	if (eq->id1->meq != NULL)
	    free (eq->id1->meq);
	eq->id1->meq = NULL;
	if (eq->id2->meq != NULL)
	    free (eq->id2->meq);
	eq->id2->meq = NULL;
    }
    for (i = 0; i < N_eqs; i++) {
	Id_more_t *im;
	int k;
	Eq_t *eq = Eqs + i;

	for (k = 0; k < 2; k++) {
	    Ident_table_t *id;
	    if (k == 0)
		id = eq->id1;
	    else
		id = eq->id2;
	    if (id->meq != NULL)
		continue;
	    id->meq = MISC_malloc (sizeof (Id_more_t));
	    im = (Id_more_t *)id->meq;
	    im->up = im->low = OFF_UNDEF;
	    im->pad = 0;
	}
    }

    /* grouping Eq variables */
    grp = 0;
    Eqs[0].group = grp;
    grp++;
    for (i = 0; i < N_eqs; i++) {
	int g1, g2, k;

	Eq_t *eq = Eqs + i;
	if (eq->group >= 0)
	    continue;
	g1 = g2 = -1;
	for (k = 0; k < N_eqs; k++) {
	    if (Eqs[k].group < 0)
		continue;
	    if (strcasecmp (eq->id1->id, Eqs[k].id1->id) == 0 ||
		strcasecmp (eq->id1->id, Eqs[k].id2->id) == 0)
		g1 = Eqs[k].group;
	    if (strcasecmp (eq->id2->id, Eqs[k].id1->id) == 0 ||
		strcasecmp (eq->id2->id, Eqs[k].id2->id) == 0)
		g2 = Eqs[k].group;
	}
	if (g1 < 0 && g2 < 0) {
	    eq->group = grp;
	    grp++;
	    continue;
	}
	if (g1 >= 0 && g2 < 0) {
	    eq->group = g1;
	    continue;
	}
	if (g1 < 0 && g2 >= 0) {
	    eq->group = g2;
	    continue;
	}
	eq->group = g1;
	for (k = 0; k < N_eqs; k++) {	/* merge g1 and g2 */
	    if (Eqs[k].group == g2)
		Eqs[k].group = g1;
	}	
    }

    /* find the tree root (the real variable) for each group */
    for (g = 0; g < grp; g++) {
	int low, up, init, done, i;
	Ident_table_t *rootid;
	Id_more_t *im;

	/* sets up and low for all eq vars in the group */
	low = up = OFF_UNDEF;
	init = 0;
	done = 0;
	while (!done) {
	    done = 1;
	    for (i = 0; i < N_eqs; i++) {
		Ident_table_t *rid, *nid;
		int nu, nl, s, roff, noff;
		Id_more_t *im1, *rim, *nim;

		Eq_t *eq = Eqs + i;
		if (eq->group != g)
		    continue;
		if (!init) {
		    rim = (Id_more_t *)eq->id1->meq;
		    low = 0;
		    rim->low = low;
		    up = Get_var_size (eq->id1, 1);
		    if (up <= 0) {
			printf ("EQ %s not processed - size of %s missing\n",
						eq->eq, eq->id1->id);
			rootid = NULL;
			goto done;
		    }
		    rim->up = up;
		    init = 1;
		}
		im1 = (Id_more_t *)eq->id1->meq;
		if (im1->up == OFF_UNDEF && 
				((Id_more_t *)eq->id2->meq)->up == OFF_UNDEF) {
		    done = 0;
		    continue;
		}
		if (im1->up != OFF_UNDEF) {	/* id1 is reference */
		    rid = eq->id1;
		    nid = eq->id2;
		    roff = Get_eq_offset (eq->eq, 0, rid);
		    noff = Get_eq_offset (eq->eq, 1, nid);
		}
		else {				/* id2 is reference */
		    rid = eq->id2;
		    nid = eq->id1;
		    roff = Get_eq_offset (eq->eq, 1, rid);
		    noff = Get_eq_offset (eq->eq, 0, nid);
		}

		s = Get_var_size (nid, 1);
		if (roff == OFF_UNDEF || noff == OFF_UNDEF || s <= 0) {
		    printf ("EQ %s not processed\n", eq->eq);
		    rootid = NULL;
		    goto done;
		}
		rim = (Id_more_t *)rid->meq;
		nl = rim->low + roff - noff;
		nim = (Id_more_t *)nid->meq;
		nu = nl + s;
		if (nim->up != OFF_UNDEF) {
		    if (nl < nim->low && nu > nim->up)
			printf ("Warning conflicting eq (%s)\n", eq->eq);
		    if (nim->up < nu)
			nim->up = nu;
		    if (nim->low > nl)
			nim->low = nl;
		}
		else {
		    nim->up = nu;
		    nim->low = nl;
		}
		if (nl < low)
		    low = nl;
		if (nu > up)
		    up = nu;
	    }
	}

	/* find the root vars in the group */
	rootid = NULL;
	for (i = 0; i < N_eqs; i++) {
	    int k;
	    Eq_t *eq = Eqs + i;
	    if (eq->group != g)
		continue;
	    for (k = 0; k < 2; k++) {
		Ident_table_t *id;
		if (k == 0)
		    id = eq->id1;
		else
		    id = eq->id2;
		if (id == rootid)
		    continue;
		if (rootid == NULL) {
		    rootid = id;
		    continue;
		}
		if (id->prop & P_COMMON) {
		    if (rootid->prop & P_COMMON) {
			printf ("Warning: EQUIV of two common vars (%s)\n",
						    eq->eq);
			rootid = NULL;
			goto done;
		    }
		    rootid = id;
		}
		else if (!(rootid->prop & P_COMMON)) {
		    Id_more_t *rootim;
		    rootim = (Id_more_t *)rootid->meq;
		    im = (Id_more_t *)id->meq;
		    if (im->low < rootim->low)
			rootid = id;
		    else if (im->low == rootim->low && im->up > rootim->up)
			rootid = id;
		}
	    }
	}

done:
	if (rootid == NULL)	/* empty group or root not found */
	    continue;

	im = (Id_more_t *)rootid->meq;
	if (im->low > low) {
	    if (rootid->prop & P_COMMON)
		printf ("Warning: Common %s (having %s) needs neg padding\n", 
						rootid->common, rootid->id);
	    else {
		fprintf (stderr, "Coding error: low not reached\n");
		exit (1);
	    }
	}
	if (im->up < up) {
	    if (rootid->prop & P_COMMON)
		FS_update_common_padding (rootid, up - im->up);
	    else {
		im->pad = Get_padding_size (rootid, up - im->up);
		if (im->pad < 0) {	/* failed for the group */
		    im->pad = 0;
		    printf ("EQ not processed\n");
		    continue;
		}
		if (im->pad > 0 && rootid->ndim == 0) {	/* never happen */
		    printf ("Padding scalar var not implemented (%s)\n",
						rootid->id);
		    printf ("EQ not processed\n");
		    im->pad = 0;
		    continue;
		}
	    }
	}
	rootid->prop |= P_DONE;
    }

    /* for each group, set TP for each eq var and casting for each branch */
    for (g = 0; g < grp; g++) {
	int done, level;

	done = level = 0;
	while (!done) {
	    done = 1;
	    for (i = 0; i < N_eqs; i++) {
		Eq_t *eq = Eqs + i;
		if (eq->group != g)
		    continue;

		if (!(eq->id1->prop & P_DONE) && (eq->id2->prop & P_DONE)) {
		    Set_eq_cast (eq->id1, eq->id2, eq);
		    eq->id1->prop |= P_DONE;
		    eq->id1->prop |= P_CVT_POINT;
		    eq->level = level;
		    done = 0;
		}
		if ((eq->id1->prop & P_DONE) && !(eq->id2->prop & P_DONE)) {
		    Ident_table_t *ti;
		    Set_eq_cast (eq->id2, eq->id1, eq);
		    eq->id2->prop |= P_DONE;
		    eq->id2->prop |= P_CVT_POINT;
		    eq->level = level;
		    ti = eq->id2;
		    eq->id2 = eq->id1;
		    eq->id1 = ti;
		    done = 0;
		}
	    }
	    level++;
	}

	/* set vars used by "used" var to "used" */
	done = 0;
	while (!done) {
	    done = 1;
	    for (i = 0; i < N_eqs; i++) {
		Eq_t *eq = Eqs + i;
		if (eq->group != g)
		    continue;
		if ((eq->id1->prop & P_USED) && !(eq->id2->prop & P_USED)) {
		    eq->id2->prop |= P_USED;
		    done = 0;
		}
	    }
	}
    }

}

/******************************************************************

    Outputs the casting code for equivalences.

******************************************************************/

int FE_get_eq_code (char **code) {
    int level, max, cnt;

    *code = STR_copy (*code, "");
    cnt = 0;
    level = 0;
    max = 0;
    while (level <= max) {
	int i;

	for (i = 0; i < N_eqs; i++) {
	    Eq_t *eq = Eqs + i;
	    if (eq->level > max)
		max = eq->level;

	    if (eq->level == level && (eq->id1->prop & P_USED)) {
		char b[256];
		if (eq->cast != NULL) {
		    sprintf (b, "%s  /* %s */\n", eq->cast, eq->eq);
		    *code = STR_cat (*code, b);
		    cnt++;
		}
		else {
		    sprintf (b, "/* TBD: equivalence %s */\n", eq->eq);
		    *code = STR_cat (*code, b);
		    cnt++;
		}
	    }
	}
	level++;
    }
    return (cnt);
}

/******************************************************************

    Retunrs the passing size (in # of c first dim) of variable "id" 
    due to eq.

******************************************************************/

int FE_get_pad_size (Ident_table_t *id) {

    if (id->meq == NULL)
	return (0);
    return (((Id_more_t *)id->meq)->pad);
}

/******************************************************************

    Returns the eq element offset in bytes of "eq". The first var is
    processed if position is 0, otherwise the second is processed.

******************************************************************/

static int Get_eq_offset (char *eq, int position, Ident_table_t *id) {
    char *tb[MAX_TKS], buf[MAX_STR_SIZE];
    int nt, nf, s, st, end;

    if (eq[0] != '(' || eq[strlen (eq) - 1] != ')') {
	printf ("Get_eq_offset: Missing bracket %s\n", eq);
	return (OFF_UNDEF);
    }
    nt = FP_get_tokens (eq, MAX_TKS, tb, NULL);
    nf = FU_get_a_field (tb, nt, position, &st, &end);
    if (nf <= 0) {
	printf ("Get_eq_offset: Missing field (%d) %s\n", position, eq);
	return (OFF_UNDEF);
    }
    tb[end + 1] = NULL;
    if (strcasecmp (tb[st], id->id) != 0) {
	fprintf (stderr, "Get_eq_offset: Coding error - Wrong id (%s %s)\n",
				tb[st], id->id);
	exit (1);
    }
    if (end == st)	/* variable only */
	s = 0;
    else {
	sprintf (buf, "%s", FU_get_c_text (tb + st + 1, 0));
	s = Get_num_offset (id, buf);
    }
    if (s == OFF_UNDEF) {
	printf ("Get_eq_offset: Evalu. %s offset of %s failed\n", id->id, buf);
	return (OFF_UNDEF);
    }
    if ((id->prop & P_COMMON) && id->common != NULL)
	s += Get_common_size (id, 0);
    return (s);
}

/******************************************************************

    Returns the number of padding element in unit of the right most
    dim of var "id" for padding "nbytes" bytes.

******************************************************************/

static int Get_padding_size (Ident_table_t *id, int nbytes) {
    int s, n;

    s = id->size;
    if (id->ndim > 0) {
	char b[256];
	int i;
	strcpy (b, "(");
	for (i = 0; i < id->ndim - 1; i++)
	    strcat (b, "1, ");
	strcat (b, "2)");
	s = Get_num_offset (id, b);
    }
    if (s <= 0 || s == OFF_UNDEF) {
	printf ("Padding not possible for %s, size %d, ndim %d (%s)\n",
				id->id, id->size, id->ndim, id->dimp);
	return (-1);
    }
    n = nbytes / s;
    if (nbytes > n * s)
	n++;
    return (n);
}

/******************************************************************

    Returns size of the variable "id". Returns -1 on failure. If
    common_size is true, the size of the entire common block is 
    returned.

******************************************************************/

static int Get_var_size (Ident_table_t *id, int common_size) {
    char buf[512], *p;
    int len, cnt, s;

    if (common_size && (id->prop & P_COMMON) && id->common != NULL)
	return (Get_common_size (id, 1));

    if (id->size < 0) {
	printf ("Get_var_size: Undefined size (%s)\n", id->id);
	return (0);
    }
    if (id->ndim == 0)
	return (id->size);

    len = strlen (id->dimp);
    if (id->dimp[0] != '(' || id->dimp[len - 1] != ')') {
	printf ("Get_var_size: Bad dim desc (%s)\n", id->dimp);
	return (-1);
    }
    strcpy (buf, id->dimp);

    p = buf;
    cnt = 0;
    while (1) {
	if (*p == ',') {
	    cnt++;
	    FP_str_replace (buf, p - buf, 1, ") * (");
	}
	if (*p == 0) {
	    cnt++;
	    break;
	}
	p++;
    }
    if (cnt != id->ndim) {
	printf ("Get_var_size: Bad dim desc (%s)\n", id->dimp);
	return (-1);
    }
    sprintf (buf + strlen (buf), " * %d", id->size);
    s = FE_evaluate_int_expr (buf, 0);

    return (s);
}

/******************************************************************

    Returns the size of the common block that contains variable "id".
    Returns 0 if id is not a common variable. If full_size is false,
    The size of the common block before var is returned.

******************************************************************/

static int Get_common_size (Ident_table_t *id, int full_size) {
    int n, cnt, i, k;
    Common_block_t *cbs;
    char *var;

    if (!(id->prop & P_COMMON) || id->common == NULL)
	return (0);

    n = FS_get_cbs (&cbs);
    for (i = 0; i < n; i++) {
	if (strcmp (cbs[i].name, id->common) == 0)
	    break;
    }
    if (i >= n) {
	fprintf (stderr, "Common block %s (for %s) not found\n",
			    id->common, id->id);
	exit (1);
    }
    var = cbs[i].vars;
    cnt = 0;
    for (k = 0; k < cbs[i].n_vars; k++) {
	int s;
	Ident_table_t *cid;

	if (!full_size && strcmp (var, id->id) == 0)
	    break;
	cid = FS_var_search (var);
	if (cid == NULL) {	/* should never happen */
	    fprintf (stderr, "Variable %s not in common block %s\n", 
					    var, id->common);
	    exit (1);
	}
	s = Get_var_size (cid, 0);
	if (s <= 0) {	/* should never happen */
	    fprintf (stderr, "Size of var %s (in common %s) not found\n", 
						    var, id->common);
	    exit (1);
	}
	cnt += s;
	var += strlen (var) + 1;
    }
    return (cnt);
}

/******************************************************************

    Evaluates and returns the integer numerical expression "expr" 
    which may contain parameters. "expr" is modified here.

******************************************************************/

int FE_evaluate_int_expr (char *expr, int err) {
    char *p, ob[MAX_STR_SIZE];
    int done, s;

    /* Replace parameters by values */
    done = 0;
    while (!done) {
	char *tks[MAX_TKS];
	int tofs[MAX_TKS], i, nt;

	done = 1;
	nt = FP_get_tokens (expr, MAX_TKS, tks, tofs);
	for (i = nt - 1; i >= 0; i--) {
	    Ident_table_t *id;
	    if (FP_is_char (tks[i][0]) && !FP_is_digit (tks[i][0])) {
		if ((id = FS_var_search (tks[i])) != NULL &&
		    id->param != NULL) {
		    FP_str_replace (expr, tofs[i] + strlen (tks[i]), 0, ")");
		    FP_str_replace (expr, tofs[i], strlen (tks[i]), id->param);
		    FP_str_replace (expr, tofs[i], 0, "(");
		    done = 0;
		}
		else {
		    if (err == 0) {
			fprintf (stderr, 
			    "Paramter %s in expression not defined\n", tks[i]);
			exit (1);
		    }
		    return (err);
		}
	    }
	}
    }

    p = FM_process_expr (expr, strlen (expr), ob);
    if (p == NULL) {
	if (err == 0) {
	    fprintf (stderr, "Expression (%s) cannot be evaluated\n", expr);
	    exit (1);
	}
	return (err);
    }
    sscanf (ob, "%d", &s);
    if (p != ob)
	free (p);
    return (s);
}

/******************************************************************

    Returns numerical offset of the variable "id" with FTN 
    dimension spec "dimp". Returns OFF_UNDEF on failure.

******************************************************************/

static int Get_num_offset (Ident_table_t *id, char *dimp) {
    char *tb[MAX_TKS], buf[MAX_STR_SIZE];
    int s;

    if (id->size < 0) {
	printf ("Get_num_offset: Undefined size (%s)\n", id->id);
	return (OFF_UNDEF);
    }
    if (Get_var_offset (id, dimp, tb) < 0)
	return (OFF_UNDEF);
    strcpy (buf, FU_get_c_text (tb, 0));
    s = FE_evaluate_int_expr (buf, 0);
    s *= id->size;
    return (s);
}

/******************************************************************

    Returns c text offset, in "tks", of the variable "id" with FTN 
    dimension spec "dimp". Returns 0 on success or -1 on failure.

******************************************************************/

static int Get_var_offset (Ident_table_t *id, char *dimp, char **otb) {
    char *ots[MAX_TKS], *dts[MAX_TKS];
    int nots, ndts, ind, no, ost, oend;

    if (dimp[0] != '(' || dimp[strlen (dimp) - 1] != ')') {
	printf ("Get_var_offset: Missing bracket %s\n", dimp);
	return (-1);
    }
    if (id->ndim <= 0 || id->dimp == NULL) {
	printf ("Get_var_offset: Variable %s has no dim\n", id->id);
	return (-1);
    }

    nots = FP_get_tokens (dimp, MAX_TKS, ots, NULL);
    ndts = FP_get_tokens (id->dimp, MAX_TKS, dts, NULL);

    otb[0] = NULL;
    ind = 0;
    while ((no = FU_get_a_field (ots, nots, ind, &ost, &oend)) > 0) {
	int nd, i, st, end;

	if (ind > 0)
	    FU_ins_tks (otb, -1, "+", NULL);
	FU_ins_tks (otb, -1, "(", NULL);
	FU_replace_tks (otb, -1, 0, ots + ost, oend - ost + 1);
	FU_ins_tks (otb, -1, "-", "1", NULL);
	FU_ins_tks (otb, -1, ")", NULL);

	for (i = 0; i < ind; i++) {
	    if ((nd = FU_get_a_field (dts, ndts, i, &st, &end)) <= 0) {
		printf ("Get_var_offset: Field %d missing in %s\n", 
						i, id->dimp);
		return (-1);
	    }
	    FU_ins_tks (otb, -1, "*", NULL);
	    if (nd > 1)
		FU_ins_tks (otb, -1, "(", NULL);
	    FU_replace_tks (otb, -1, 0, dts + st, end - st + 1);
	    if (nd > 1)
		FU_ins_tks (otb, -1, ")", NULL);
	}
	ind++;
    }
    if (ind != id->ndim) {
	printf ("Get_var_offset: Offset \"%s\" has %d fields != ndim (%d)\n",
						dimp, ind, id->ndim);
	return (-1);
    }

    return (0);
}

/******************************************************************

    Sets the cast statement for eqviv "eq". In case of failure, eq->eq
    is NULL.

******************************************************************/

static void Set_eq_cast (Ident_table_t *id1, Ident_table_t *id2, Eq_t *eq) {
    char *tks[MAX_TKS];
    int nt, st, lst, lcnt, bst, bcnt;
    char *tb[MAX_TKS], *otb[MAX_TKS], *cast, *ct;
    int s1, e1, s2, e2, ntct;

    nt = FP_get_tokens (eq->eq, MAX_TKS, tks, NULL);
    lst = lcnt = bst = bcnt = -1;
    st = 0;
    if (tks[st][0] != '(')
	goto err1;
    s1 = st + 1;
    e1 = FU_next_token (s1, nt, tks, ',');
    if (tks[e1][0] != ',')
	goto err1;
    s2 = e1 + 1;
    e2 = FU_next_token (s2, nt, tks, ')');
    if (tks[e2][0] != ')')
	goto err1;
/*printf ("eq %s, %d %d   %d %d   %s %s\n", eq->eq, s1, e1, s2, e2, id1->id, id2->id); */
    if (strcasecmp (tks[s1], id1->id) == 0) {
	lst = s1;
	lcnt = e1 - s1;
	bst = s2;
	bcnt = e2 - s2;
    }
    else {
	lst = s2;
	lcnt = e2 - s2;
	bst = s1;
	bcnt = e1 - s1;
    }

    otb[0] = NULL;
    FC_process_expr (tks + lst, 1, tb, 0);
    FU_replace_tks (otb, 0, 0, tb, 0);

    /* add the cast */
    ct = FC_get_ctype_dec (id1);
    FE_change_var_def (ct);
    ntct = FP_get_tokens (ct, MAX_TKS, tb, NULL);
    s1 = e1 = -1;
    st = 0;
    while (st < ntct) {
	if (s1 < 0 && tb[st][0] == '[')
	    s1 = st;
	else if (tb[st][0] == ']')
	    e1 = st;
	st++;
    }
    if (s1 >= 0 && e1 >= 0) {
	FU_ins_tks (otb, -1, "=", "(", FU_get_c_type (id1), "(", "*", ")", 
								NULL);
	FU_replace_tks (otb, -1, 0, tb + s1, e1 - s1 + 1);
	FU_ins_tks (otb, -1, ")", NULL);
    }
    else
	FU_ins_tks (otb, -1, "=", "(", FU_get_c_type (id1), "*", ")", NULL);

    /* add the right-hand variable */
    FU_ins_tks (otb, -1, "&", NULL);
    FC_process_expr (tks + bst, bcnt, tb, 0);
    FU_replace_tks (otb, -1, 0, tb, 0);
    st = 0;
    while (tb[st] != NULL && tb[st][0] != '[')
	st++;
    if (tb[st] == NULL) {
	int nd, i;

	nd = id2->ndim;
	if (id2->type == T_CHAR)
	    nd++;
	for (i = 0; i < nd; i++)
	    FU_ins_tks (otb, -1, "[", "0", "]", NULL);
    }
    else if (id2->type == T_CHAR)
	FU_ins_tks (otb, -1, "[", "0", "]", NULL);

    /* add offset of left hand variable */
    if (lcnt > 1) {
	if (Get_var_offset (id1, 
			FU_get_c_text (tks + lst + 1, lcnt - 1), tb) < 0)
	    goto err1;
	FU_ins_tks (otb, -1, "-", NULL);
	if (tb[1] != NULL)
	    FU_ins_tks (otb, -1, "(", NULL);
	FU_replace_tks (otb, -1, 0, tb, 0);
	if (tb[1] != NULL)
	    FU_ins_tks (otb, -1, ")", NULL);
    }

    FU_ins_tks (otb, -1, ";", NULL);
    FU_post_process_c_tks (otb);
    cast = FU_get_c_text (otb, 0);
    eq->cast = MISC_malloc (strlen (cast) + 1);
    strcpy (eq->cast, cast);

    return;
err1:
    printf ("Equivalence %s not processed\n", eq->eq);
    return;
}

/******************************************************************

    Saves an EQUIVALENCE in table. We may check dupllicated and 
    inconsistant EQs here in the future.

******************************************************************/

void FE_save_eq (char *buf, char *var1, char *var2) {
    Eq_t *eq;

    if (Eq_tblid == NULL) {
	Eq_tblid = MISC_open_table (sizeof (Eq_t), 
				    64, 0, &N_eqs, (char **)&Eqs);
	if (Eq_tblid == NULL) {
	    fprintf (stderr, "MISC_open_table Eq_tblid failed\n");
	    exit (1);
	}
    }

    eq = (Eq_t *)MISC_table_new_entry (Eq_tblid, NULL);
    eq->group = -1;
    eq->var1 = MISC_malloc (strlen (var1) + 1);
    strcpy (eq->var1, var1);
    eq->var2 = MISC_malloc (strlen (var2) + 1);
    strcpy (eq->var2, var2);
    eq->id1 = eq->id2 = NULL;
    eq->eq = MISC_malloc (strlen (buf) + 1);
    strcpy (eq->eq, buf);
    eq->cast = NULL;
    eq->level = 0;
}

/******************************************************************

    Modifies turned-to-point c variable definition string "c_text".

******************************************************************/

void FE_change_var_def (char *c_text) {
    char *ps;
    int cnt;

    ps = c_text;
    cnt = 0;
    while (*ps != 0) {
	if (*ps == '[')
	    cnt++;
	ps++;
    }

    ps = c_text;
    while (*ps != 0 && *ps != '[')
	ps++;
    if (*ps == '[') {
	char *pe = ps + 1;
	while (*pe != 0 && *pe != ']')
	    pe++;
	if (*pe != ']')
	    FS_exception_exit ("Unclosed [ in c code: %s\n", c_text);
	if (cnt == 1) {
	    FP_str_replace (c_text, ps - c_text, pe - ps + 1, "");
	    FP_str_replace (c_text, 0, 0, "*");
	}
	else {
	    FP_str_replace (c_text, ps - c_text, pe - ps + 1, ")");
	    FP_str_replace (c_text, 0, 0, "(*");
	}
    }
    else {
	FP_str_replace (c_text, 0, 0, "*");
    }
}

/******************************************************************

    Initialize this module and frees all previously allocated memory.

******************************************************************/

void FE_init () {
    int i;

    for (i = 0; i < N_eqs; i++) {
	if (Eqs[i].var1 != NULL)
	    free (Eqs[i].var1);
	if (Eqs[i].var2 != NULL)
	    free (Eqs[i].var2);
	if (Eqs[i].eq != NULL)
	    free (Eqs[i].eq);
	if (Eqs[i].cast != NULL)
	    free (Eqs[i].cast);
    }
    if (Eq_tblid != NULL)
	MISC_free_table (Eq_tblid);
    N_eqs = 0;
    Eqs = NULL;
    Eq_tblid = NULL;
}
