
/******************************************************************

    gdc is a tool that generates device configuration file for
    specified site(s). This module processes gdc variables.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/01/03 19:23:58 $
 * $Id: gdc_variable.c,v 1.1 2011/01/03 19:23:58 jing Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gdc_def.h"

static Variable_t *Vars = NULL;		/* global var array */
static int N_vars = 0;

static char *Required_vars = NULL;
static int N_required_vars = 0;

static Variable_t *Lvars = NULL;	/* local variable array */
static int Lv_st = 0;			/* start index of current context */
static int Lv_n = 0;			/* number of vars of current context */
static int N_lvars = 0;			/* size of Lvars */

static int Check_condition_reserved_word = 0;

static void Set_variable (char *v_name, char *value, int type);
static int Process_gdc_control_vars (char *v_name, char *value);
static void Add_required_var (char *var);


/**********************************************************************

    Sets/resets Check_condition_reserved_word.

**********************************************************************/

void GDCV_check_condition_reserved_word (int yes) {
    Check_condition_reserved_word = yes;
}

/**********************************************************************

    Adds/resets the value of variable "v_name" to "value".

**********************************************************************/

void GDCV_set_variable (char *v_name, char *value) {

    if (strncmp (v_name, "_gdc_", 5) == 0 &&
	Process_gdc_control_vars (v_name, value))
	return;
    if (GDCV_is_local_var (v_name)) {
	Set_variable (v_name, value, VAR_LOCAL);
	return;
    }
    Set_variable (v_name, value, VAR_GLOBAL);
}

/**********************************************************************

    Returns "Lv_st" and "Lv_n".

**********************************************************************/

void GDCV_get_var_range (int *st_ind, int *n_vs) {
    *st_ind = Lv_st;
    *n_vs = Lv_n;
}

/**********************************************************************

    Sets the local variable range with stating index "st_ind" and "n_vs"
    variables. Unused local variables are freed.

**********************************************************************/

void GDCV_set_var_range (int st_ind, int n_vs) {
    int i;

    if (st_ind + n_vs > N_lvars)
	GDCP_exception_exit ("Unexpected local var range - coding error\n");
    for (i = st_ind + n_vs; i < N_lvars; i++) {
	if (Lvars[i].name == NULL)
	    break;
	free (Lvars[i].name);
	Lvars[i].name = NULL;
    }
    Lv_st = st_ind;
    Lv_n = n_vs;
}

/**********************************************************************

    Returns true if variable name "v_name" is local (All lower case 
    characters), or false otherwise

**********************************************************************/

int GDCV_is_local_var (char *v_name) {
    int len, cnt, i;
    len = strlen (v_name);
    cnt = 0;
    for (i = 0; i < len; i++) {
	if (v_name[i] != tolower (v_name[i]))
	    cnt++;
    }
    if (cnt == 0 && strcmp (v_name, "site_name") != 0)
	return (1);
    return (0);
}

/**********************************************************************

    Processes gdc control variable of name "v_name" and value "value".
    Returns 1 on success or 0 if v_name is not a gdc control variable.

**********************************************************************/

static int Process_gdc_control_vars (char *v_name, char *value) {

    if (strcmp (v_name, "_gdc_set_field_delimiter") == 0) {
	char tk[MAX_STR_SIZE];
	if (GDCM_stoken (value, "Q\"", 0, tk, MAX_STR_SIZE) <= 0 ||
	    strlen (tk) != 1)
	    GDCP_exception_exit (
		"Unexpected value (%s) for _gdc_set_field_delimiter\n", value);
	GDCE_set_field_delimiter (tk[0]);
    }
    else if (strcmp (v_name, "_gdc_set_data_delimiter") == 0) {
	char tk[MAX_STR_SIZE], name[MAX_STR_SIZE];
	if (GDCM_stoken (value, "Q\"", 1, tk, MAX_STR_SIZE) <= 0 ||
	    strlen (tk) != 1)
	    GDCP_exception_exit (
		"Unexpected value (%s) for _gdc_set_data_delimiter\n", value);
	GDCM_ftoken (value, "Q\"", 0, name, MAX_STR_SIZE);
	GDCR_set_delimiter (name, tk[0]);
    }
    else if (strcmp (v_name, "_gdc_add_required_variable") == 0) {
	if (strlen (value) < 1)
	    GDCP_exception_exit (
	      "Unexpected value (%s) for _gdc_add_required_variable\n", value);
	Add_required_var (value);
    }
    else if (strcmp (v_name, "_gdc_set_interpreting") == 0 ||
	     strcmp (v_name, "_gdc_include") == 0 ||
	     strcmp (v_name, "_gdc_print_variables") == 0) {
	GDCP_gdc_control (v_name, value);
    }
    else if (strcmp (v_name, "_gdc_read_file") == 0) {
	char tk[MAX_STR_SIZE];
	if (GDCM_ftoken (value, "Q\"", 0, tk, MAX_STR_SIZE) > 0 &&
	    strcmp (tk, "fr_site.dat") == 0 &&
	    GDCF_read_fr_site (value) < 0) {
	    exit (1);
	}
    }
    else if (strcmp (v_name, "_gdc_site_is") == 0) {
	GDCM_gdc_control (v_name, value);
    }
    else if (strcmp (v_name, "_gdc_install_file") == 0 ||
	     strcmp (v_name, "_gdc_import_from") == 0 ||
	     strcmp (v_name, "_gdc_execute") == 0) {
	GDCF_gdc_control (v_name, value);
    }
    else if (strcmp (v_name, "_gdc_error") == 0) {
	GDCP_exception_exit ("%s\n", value);
    }
    else if (strcmp (v_name, "_gdc_get_all_sites") != 0)
	return (0);
    return (1);
}

/**********************************************************************

    Adds/resets the value of variable "v_name" to "value".

**********************************************************************/

static void Set_variable (char *v_name, char *value, int type) {
    static int buf_size = 0;
    int i;

    if (type == VAR_LOCAL) {
	Variable_t vbuf, *v;
	for (i = Lv_st; i < Lv_st + Lv_n; i++) {
	    if (strcmp (Lvars[i].name, v_name) == 0)
		break;
	}
	if (i >= N_lvars)
	    v = &vbuf;
	else {
	    v = Lvars + i;
	    if (v->name != NULL)
		free (v->name);
	}
	v->name = malloc (strlen (v_name) + strlen (value) + 2);
	strcpy (v->name, v_name);
	v->value = v->name + strlen (v_name) + 1;
	strcpy (v->value, value);
	v->type = type;
	if (i >= N_lvars) {
	    Lvars = (Variable_t *)STR_append ((char *)Lvars, 
					(char *)v, sizeof (Variable_t));
	    N_lvars++;
	}
	if (i >= Lv_st + Lv_n)
	    Lv_n++;
	return;
    }

    for (i = 0; i < N_vars; i++) {
	if (strcmp (Vars[i].name, v_name) == 0)
	    break;
    }
    if (i >= N_vars) {
	if (N_vars >= buf_size) {
	    char *p;
	    buf_size = N_vars * 2 + 128;
	    p = MISC_malloc (buf_size * sizeof (Variable_t));
	    if (Vars != NULL) {
		memcpy (p, Vars, N_vars * sizeof (Variable_t));
		free (Vars);
	    }
	    Vars = (Variable_t *)p;
	}
	N_vars++;
    }
    else
	free (Vars[i].name);
    Vars[i].name = malloc (strlen (v_name) + strlen (value) + 2);
    strcpy (Vars[i].name, v_name);
    Vars[i].value = Vars[i].name + strlen (v_name) + 1;
    strcpy (Vars[i].value, value);
    Vars[i].type = type;
}

/**************************************************************************

    Returns the value for variable "var". It returns NULL if "var" is
    not defined.

**************************************************************************/

char *GDCV_get_value (char *var) {
    static char *env_value = NULL;
    char buf[MAX_STR_SIZE];
    int i, cnt;

    if (Check_condition_reserved_word && (
	 strcmp (var, "IF") == 0 ||
	 strcmp (var, "CASE") == 0 ||
	 strcmp (var, "ELSE") == 0 ||
	 strcmp (var, "NOT") == 0))
	GDCP_exception_exit ("Unexpected variable name: %s\n", var);

    for (i = Lv_st; i < Lv_st + Lv_n; i++) {
	if (strcmp (Lvars[i].name, var) == 0)
	    return (Lvars[i].value);
    }
    for (i = 0; i < N_vars; i++) {
	if (strcmp (Vars[i].name, var) == 0)
	    return (Vars[i].value);
    }
    cnt = GDCV_get_var_from_env (var, buf, MAX_STR_SIZE);
    if (cnt > 0) {
	if (cnt > 1)
	    GDCP_exception_exit (
	    "Variable %s multiply defined in environmental variables\n", var);
	env_value = STR_copy (env_value, buf);
	return (env_value);
    }
    return (NULL);
}

/**************************************************************************

    Returns the values (null terminated strings) of the variable "var"
    in "buf" of "size" bytes found in the environmental variables. Returns
    the number of values found.

**************************************************************************/

int GDCV_get_var_from_env (char *var, char *buf, int size) {
    char *env, *b;
    int cnt;

    cnt = 0;
    b = buf;
    b[0] = '\0';
    env = getenv (var);
    if (env != NULL) {
	GDCM_strlcpy (b, env, size);
	b += strlen (env) + 1;
	size -= strlen (env) + 1;
	cnt++;
    }
	
    env = getenv ("GDC_VARIABLES");
    if (env != NULL) {
	char tk[MAX_STR_SIZE], name[MAX_STR_SIZE], v[MAX_STR_SIZE];
	int ind = 0;
	while (GDCM_ftoken (env, "S;", ind, tk, MAX_STR_SIZE) > 0) {
	    if (GDCM_ftoken (tk, "S=", 0, name, MAX_STR_SIZE) > 0 &&
		strcmp (name, var) == 0 &&
		GDCM_ftoken (tk, "S=", 1, v, MAX_STR_SIZE) > 0) {
		GDCM_strlcpy (b, v, size);
		b += strlen (v) + 1;
		size -= strlen (v) + 1;
		cnt++;
	    }
	    ind++;
	}
    }

    return (GDCV_rm_duplicated_strings (buf, cnt));
}

/**************************************************************************

    Removes duplicated strings in "cnt" null-terminated strings "strs".
    Returns the number of distinct strings.

**************************************************************************/

int GDCV_rm_duplicated_strings (char *strs, int cnt) {
    int i, c;
    char *pi, *po;

    pi = strs;
    po = strs;
    c = 0;
    for (i = 0; i < cnt; i++) {
	int k;
	char *p = strs;
	for (k = 0; k < i - 1; k++) {
	    if (strcmp (p, pi) == 0)
		break;
	    p += strlen (p) + 1;
	}
	if (k >= i - 1) {
	    memmove (po, pi, strlen (pi) + 1);
	    po += strlen (po) + 1;
	    c++;
	}
	pi += strlen (pi) + 1;
    }
    return (c);
}

/**************************************************************************

    Returns 1 if "var" is required or 0 otherwise.

**************************************************************************/

int GDCV_is_required (char *var) {
    int i;
    char *p;

    if (Required_vars == NULL)
	return (0);
    p = Required_vars;
    for (i = 0; i < N_required_vars; i++) {
	if (GDCV_match_wild_card (var, p))
	    return (1);
	p += strlen (p) + 1;
    }
    return (0);
}

/**************************************************************************

    Returns 1 if "str" matches pattern "pat" (a string contains wild cards)
    or 0 otherwise. The currently supported wild card is only "*". Buffer
    "pat" must be writable.Lv_n

**************************************************************************/

int GDCV_match_wild_card (char *str, char *pat) {
    char *p, *sp, *pp;
    int cnt;

    sp = str;
    pp = pat;
    p = pp;
    cnt = 0;
    while (*p != '\0') {
	if (*p == '*') {
	    int nc = p - pp;
	    if (nc > 0) {
		if (cnt == 0) {
		    if (strncmp (sp, pp, nc) != 0)
			return (0);
		    sp += nc;
		}
		else {
		    char *t;
		    *p = '\0';
		    t = strstr (sp, pp);
		    *p = '*';
		    if (t == NULL)
			return (0);
		    sp = t + nc;
		}
	    }
	    p++;
	    pp = p;
	    cnt++;
	}
	else
	    p++;
    }
    if (cnt == 0) {
	if (strcmp (sp, pp) == 0)
	    return (1);
    }
    else {
	if (strlen (pp) == 0)
	    return (1);
	if (strlen (sp) >= strlen (pp) && 
		strcmp (sp + strlen (sp) - strlen (pp), pp) == 0)
	    return (1);
    }
    return (0);
}

/**************************************************************************

    Returns the value for variable "var". It returns NULL if "var" is
    not defined.

**************************************************************************/

int GDCV_get_variables (Variable_t **vars) {

    *vars = Vars;
    return (N_vars);
}

/**************************************************************************

    Discards all defined variables.

**************************************************************************/

void GDCV_discard_variables () {
    int i;

    for (i = 0; i < N_vars; i++)
	free (Vars[i].name);
    for (i = 0; i < Lv_st + Lv_n; i++) {
	free (Lvars[i].name);
	Lvars[i].name = NULL;
    }
    Lv_st = Lv_n = 0;
    N_vars = 0;
    STR_free (Required_vars);
    Required_vars = NULL;
    N_required_vars = 0;
}

static void Add_required_var (char *var) {
    Required_vars = STR_append (Required_vars, var, strlen (var) + 1);
    N_required_vars++;
}

/**************************************************************************

    Removes all variables of "type".

**************************************************************************/

void GDCV_remove_variables (int type) {
    int i, cnt;

    cnt = 0;
    for (i = 0; i < N_vars; i++) {
	if (Vars[i].type == type)
	    free (Vars[i].name);
	else {
	    Vars[cnt] = Vars[i];
	    cnt++;
	}
    }
    N_vars = cnt;
}

