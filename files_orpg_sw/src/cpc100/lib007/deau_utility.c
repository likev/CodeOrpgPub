
/**************************************************************************

    Data element attribute utility library module - basic functions.

**************************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:49 $
 * $Id: deau_utility.c,v 1.22 2012/06/14 18:57:49 jing Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>
#include <dlfcn.h>

#include <infr.h>
#include "deau_def.h"


typedef struct {			/* custom function pointers */
    char *name;				/* function name */
    void *pt;				/* function pointer */
} function_pts_t;

static void *Func_tbl = NULL;		/* function pointer table id */
static function_pts_t *Funcs;		/* function pointer table */
static int N_funcs = 0;			/* size of function pointer table */
static char *Next_tok = "";		/* the next token */

static int Preprocess_attr_text (char *text, void *buf);
static int Get_next_item (int type, void *buf, char **token);
static char *Get_function_pt (char *name);
static int Parse_values (char *value, double *buffer, int buf_size);
static int Match_site (char *site, char *site_names);
static int Parse_string_values (char *value, char **p);
static int Parse_string_fields (char **p);
static int Look_up_site_group (char *gname, char **value);
static int Parse_enum_values (DEAU_attr_t *attr, 
			int which_value, double *values, int buf_size);
static void uuencode_one (unsigned char *in, unsigned char *out);
static int uudecode (unsigned char *str, unsigned char **outp);


/************************************************************************

    Moves the current value of DE "id" to baseline, if "to_baseline" is 
    true, or vice versa otherwise. Returns 0 on success or a negative 
    error number.

************************************************************************/

int DEAU_move_baseline (char *id, int to_baseline) {
    char *vb;
    DEAU_attr_t *at;
    int ret;

    ret = DEAU_get_attr_by_id (id, &at);
    if (ret < 0)
	return (ret);
    vb = NULL;
    if (to_baseline) {
	vb = STR_copy (vb, at->ats[DEAU_AT_VALUE]);
	ret = DEAU_update_attr (id, DEAU_AT_BASELINE, vb);
    }
    else {
	vb = STR_copy (vb, at->ats[DEAU_AT_BASELINE]);
	ret = DEAU_update_attr (id, DEAU_AT_VALUE, vb);
    }
    STR_free (vb);
    if (ret < 0)
	return (ret);
    return (0);
}

/************************************************************************

    Returns, with "enum", the enum value for string value "str" in terms
    of attributes "attr". If "buf_s" > 0, it returns the string value in 
    terms of the value of "*enum" in "str" of sizeof (buf_s). Returns 0 
    on success or a negative error code.

************************************************************************/

int DEAU_get_enum (DEAU_attr_t *attr, char *str, int *enu, int buf_s) {
    static char *r = NULL, *e = NULL;
    static int nr = 0;
    char *pr, *pe;
    int i;

    if (attr != NULL) {
	char buf[2], *ate;
	int num, ne;
	ate = attr->ats[DEAU_AT_ENUM];
	if (ate[0] == '\0')
	    return (DEAU_BAD_ENUM);
	num = Preprocess_attr_text (attr->ats[DEAU_AT_RANGE], buf);
	if (num <= 0 || buf[0] != '{' || buf[1] != '}')
	    return (DEAU_BAD_RANGE);
	nr = Parse_string_fields (&r);
	num = Preprocess_attr_text (ate, NULL);
	if (num <= 0)
	    return (DEAU_BAD_ENUM);
	ne = Parse_string_fields (&e);
	if (ne != nr)
	    return (DEAU_BAD_ENUM);
    }

    pr = r;
    pe = e;
    for (i = 0; i < nr; i++) {
	int n;
	if (buf_s > 0) {
	    if (sscanf (pe, "%d", &n) == 1 &&
		n == *enu) {
		strncpy (str, pr, buf_s);
		str[buf_s - 1] = '\0';
		return (0);
	    }
	}
	else {
	    if (strcmp (pr, str) == 0) {
		if (sscanf (pe, "%d", &n) != 1)
		    return (DEAU_BAD_ENUM);
		*enu = n;
		return (0);
	    }
	}
	pr += strlen (pr) + 1;
	pe += strlen (pe) + 1;
    }
    return (DEAU_BAD_ENUM);
}

/************************************************************************

    Returns the number of allowable values for range attribute "range"
    if the allowable values are finite. The values, a set of null-terminated
    string, are returned with "p". If the range is not a finite set,
    it returns 0.

************************************************************************/

int DEAU_get_allowable_values (DEAU_attr_t *attr, char **p) {
    static char *vb = NULL;
    char buf[2], *range;
    int num, ret;

    range = attr->ats[DEAU_AT_RANGE];
    num = Preprocess_attr_text (range, buf);
    if (num <= 0 || buf[0] != '{' || buf[1] != '}')
	return (0);
    ret = Parse_string_fields (&vb);
    *p = vb;
    return (ret);
}

/************************************************************************

    Returns the min and max values for range attribute "range"
    if the range attribute is a inteval. The values, two null-terminated
    string, are returned with "p". If the range is not an interval,
    it returns 0. The return value is 1, 3, 5, or 7 if, respectively,
    the range is (), [), (] and [].

************************************************************************/

int DEAU_get_min_max_values (DEAU_attr_t *attr, char **p) {
    static char *vb = NULL;
    char buf[2], *range;
    int ret;

    range = attr->ats[DEAU_AT_RANGE];
    ret = Preprocess_attr_text (range, buf);
    if (ret != 2 || 
	(buf[0] != '[' && buf[0] != '(') ||
	(buf[1] != ']' && buf[1] != ')') ||
	Parse_string_fields (&vb) != 2)
	return (0);
    ret = 1;
    if (buf[0] == '[')
	ret += 2;
    if (buf[1] == ']')
	ret += 4;
    *p = vb;
    return (ret);
}

/************************************************************************

    Retrieves the number of values of data element "id". The function 
    returns the number of values on success. On failure the function 
    returns a negative error code. See DEAU man-page for more spec.

************************************************************************/

int DEAU_get_number_of_values (char *id) {
    DEAU_attr_t *attr;
    int ret;

    ret = DEAU_get_attr_by_id (id, &attr);
    if (ret < 0)
	return (ret);
    return (Parse_values (attr->ats[DEAU_AT_VALUE], NULL, 0));
}

/************************************************************************

    Retrieves the number of baseline values of data element "id". The
    function returns the number of values on success. On failure the
    function returns a negative error code. See DEAU man-page for more spec.

************************************************************************/

int DEAU_get_number_of_baseline_values (char *id) {
    DEAU_attr_t *attr;
    int ret;

    ret = DEAU_get_attr_by_id (id, &attr);
    if (ret < 0)
	return (ret);
    return (Parse_values (attr->ats[DEAU_AT_BASELINE], NULL, 0));
}

/************************************************************************

    Retrieves numerical values of data element "id" and put them in buffer 
    "values" of size "buf_size". The function returns the number of values
    retrieved on success. On failure the function returns a negative error 
    code. See DEAU man-page for more spec.

************************************************************************/

int DEAU_get_values (char *id, double *values, int buf_size) {
    DEAU_attr_t *attr;
    int ret;

    ret = DEAU_get_attr_by_id (id, &attr);
    if (ret < 0)
	return (ret);
    if (attr->ats[DEAU_AT_ENUM][0] != '\0')	/* enumeration type */
	return (Parse_enum_values (attr, DEAU_AT_VALUE, values, buf_size));
    return (Parse_values (attr->ats[DEAU_AT_VALUE], values, buf_size));
}

/************************************************************************

    Retrieves string values of data element "id", puts them in a static 
    buffer and returns the pointer to the buffer with "p". The function 
    returns the number of values retrieved on success. On failure the 
    function returns a negative error code. See DEAU man-page for more 
    spec.

************************************************************************/

int DEAU_get_string_values (char *id, char **p) {
    static char *vb = NULL;
    DEAU_attr_t *attr;
    int ret;

    ret = DEAU_get_attr_by_id (id, &attr);
    if (ret < 0)
	return (ret);
    ret = Parse_string_values (attr->ats[DEAU_AT_VALUE], &vb);
    *p = vb;
    return (ret);
}

/************************************************************************

    Retrieves branch names internal node "node", puts them in a static 
    buffer and returns the pointer to the buffer with "p". The function 
    returns the number of values retrieved on success. On failure the 
    function returns a negative error code. See DEAU man-page for more 
    spec.

************************************************************************/

int DEAU_get_branch_names (char *node, char **p) {
    static char *vb = NULL;
    DEAU_attr_t *attr;
    int ret;

    ret = DEAU_get_attr_by_id (DEAU_get_node_name (node), &attr);
    if (ret < 0)
	return (ret);
    ret = Parse_string_values (attr->ats[DEAU_AT_VALUE], &vb);
    *p = vb;
    return (ret);
}

/************************************************************************

    Returns the full node name for node "node".

************************************************************************/

char *DEAU_get_node_name (char *node) {
    static char *name = NULL;
    name = STR_reset (name, 128);
    name = STR_copy (name, "@@");
    name = STR_cat (name, node);
    return (name);
}

/************************************************************************

    Retrieves baseline string values of data element "id", put them in 
    a static buffer and returns the pointer to the buffer with "p". The 
    function returns the number of values retrieved on success. On failure 
    the function returns a negative error code. See DEAU man-page for more 
    spec.

************************************************************************/

int DEAU_get_baseline_string_values (char *id, char **p) {
    static char *vb = NULL;
    DEAU_attr_t *attr;
    int ret;

    ret = DEAU_get_attr_by_id (id, &attr);
    if (ret < 0)
	return (ret);
    ret = Parse_string_values (attr->ats[DEAU_AT_BASELINE], &vb);
    *p = vb;
    return (ret);
}

/************************************************************************

    Retrieves baseline values of data element "id" and put them in
    buffer "values" of size "buf_size". The function returns the number 
    of values retrieved on success. On failure the function returns a 
    negative error code. See DEAU man-page for more spec.

************************************************************************/

int DEAU_get_baseline_values (char *id, double *values, int buf_size) {
    DEAU_attr_t *attr;
    int ret;

    ret = DEAU_get_attr_by_id (id, &attr);
    if (ret < 0)
	return (ret);
    if (attr->ats[DEAU_AT_ENUM][0] != '\0')	/* enumeration type */
	return (Parse_enum_values (attr, DEAU_AT_BASELINE, values, buf_size));
    return (Parse_values (attr->ats[DEAU_AT_BASELINE], values, buf_size));
}

/************************************************************************

    Sets the current value attribute of data element "id" using the 
    "n_items" data values in array "values". If "str_type" is non-zero,
    the values are treated as strings. They must all be null terminated
    and stored in buffer pointed to by "values" one after another.
    If "str_type" is zero, the values are numerical and stored in a
    double array pointed by "values". If "base_line" is non-zero,
    the base-line attribute, instead of the current value attribute, is set.
    For enumeration type, both string and numerical values are accepted.
    The values are not accepted if range check fails. The function returns 
    0 on success or a negative error code. See DEAU man-page for more spec.

************************************************************************/

int DEAU_set_values (char *id, int str_type, void *values, 
					int n_items, int base_line) {
    DEAU_attr_t *attr, *a;
    char *p;
    int ret, t;

    if (id == NULL)
	return (DEAU_BAD_ARGUMENT);
    ret = DEAU_get_attr_by_id (id, &attr);
    if (ret < 0)
	return (ret);

    if (!str_type && attr->ats[DEAU_AT_ENUM][0] != '\0') {
						/* enumeration type */
	static char *vb = NULL;
	int n, i;
	vb = STR_reset (vb, 128);
	for (i = 0; i < n_items; i++) {
	    char buf[128];
	    n = (int)(((double *)values)[i]);
	    a = attr;
	    if (i > 0)
		a = NULL;			/* for efficiency */
	    if (DEAU_get_enum (a, buf, &n, 128) < 0)
		return (DEAU_BAD_ENUM);
	    vb = STR_append (vb, buf, strlen (buf) + 1);
	}
	str_type = 1;
	values = vb;
    }

    t = DEAU_T_DOUBLE;
    if (str_type)
	t = DEAU_T_STRING;
    if (DEAU_check_data_range (id, t, n_items, (char *)values) < 0)
	return (DEAU_BAD_RANGE);
    p = DEAU_values_to_text (str_type, values, n_items);

    if (base_line)
	return (DEAU_update_attr (id, DEAU_AT_BASELINE, p));
    else
	return (DEAU_update_attr (id, DEAU_AT_VALUE, p));
}

/************************************************************************

    Converts the "n_items" data values in array "values" to the text 
    value attribute form. If "str_type" is non-zero,
    the values are treated as strings. They must all be null terminated
    and stored in buffer pointed to by "values" one after another.
    If "str_type" is zero, the values are numerical and stored in a
    double array pointed by "values". The function returns the pointer
    to the converted string. See DEAU man-page for more spec.

************************************************************************/

char *DEAU_values_to_text (int str_type, void *values, int n_items) {
    static char *vb = NULL;
    int i;

    vb = STR_reset (vb, 128);
    if (str_type) {
	char *p;
	p = (char *)values;
	for (i = 0; i < n_items; i++) {
	    vb = STR_cat (vb, p);
	    if (i < n_items - 1)
		vb = STR_cat (vb, ", ");
	    p += strlen (p) + 1;
	}
    }
    else {
	double *d;
	d = (double *)values;
	for (i = 0; i < n_items; i++) {
	    char buf[80];
	    if (i < n_items - 1)
		sprintf (buf, "%.16g, ", d[i]);
	    else
		sprintf (buf, "%.16g", d[i]);
	    vb = STR_cat (vb, buf);
	}
    }
    return (vb);
}

/************************************************************************

    Checks if "user_id", e.g. "URC", has a write permission according 
    to attribute "at". It returns 1 if permission is granted, 0 if not,
    or a negative error code.

************************************************************************/

int DEAU_check_permission (DEAU_attr_t *at, char *user_id) {
    char *pms;
    char quots[2], *pt;
    int n_tks;

    pms = at->ats[DEAU_AT_PERMISSION];
    if (strlen (pms) == 0)
	return (0);
    n_tks = Preprocess_attr_text (pms, quots);
    if (n_tks <= 0 || quots[0] != '[' || quots[1] != ']')
	return (0);
    while (1) {
	if (Get_next_item (DEAU_T_STRING, &pt, NULL) <= 0)
	    break;
	if (strcmp (pt, user_id) == 0)
	    return (1);
    }
    return (0);
}

/************************************************************************

    Sets the current value attribute of data element "id" using its
    default attribute at site "site". The function returns 0 on success 
    or a negative error code. See DEAU man-page for more spec.

************************************************************************/

int DEAU_use_default_values (char *id, char *site, int force) {
    static char *vb = NULL;
    DEAU_attr_t *attr;
    int cnt, n_tks, i, ret;

    if (id == NULL)
	return (DEAU_BAD_ARGUMENT);
    Look_up_site_group (NULL, NULL);	/* initialize the site group table */

    ret = DEAU_get_attr_by_id (id, &attr);
    if (ret < 0)
	return (ret);
    if (!force && strlen (attr->ats[DEAU_AT_VALUE]) > 0)
	return (0);
    if (strlen (attr->ats[DEAU_AT_DEFAULT]) == 0)
	return (DEAU_DEFAULT_NOT_FOUND);

    n_tks = Preprocess_attr_text (attr->ats[DEAU_AT_DEFAULT], NULL);
    if (n_tks <= 0)
	return (DEAU_DEFAULT_NOT_FOUND);

    vb = STR_reset (vb, 128);
    cnt = -1;
    for (i = 0; i < n_tks; i++) {
	char *pt, *p;
	ret = Get_next_item (DEAU_T_STRING, &pt, NULL);
	if (ret <= 0)
	    break;

	p = pt;
	while (*p != '\0' && *p != ':')
	    p++;
	if (*p == ':') {
	    if (cnt >= 0)
		break;
	    *p = '\0';
	    if (Match_site (site, pt))
		cnt = 0;
	    else
		continue;
	    *p = ':';
	    pt = p + 1;
	    while (*pt == ' ' || *pt == '\t')
		pt++;
	}
	else if (i == 0)
	    cnt = 0;
	if (cnt < 0)
	    continue;
	vb = STR_cat (vb, pt);
	vb = STR_cat (vb, ",");
	cnt++;
    }
    if (cnt <= 0)
	return (DEAU_DEFAULT_NOT_FOUND);
    ret = DEAU_update_attr (id, DEAU_AT_VALUE, vb);
/*
    if (ret >= 0)
	ret = DEAU_update_attr (id, DEAU_AT_BASELINE, vb);
*/
    return (ret);
}

/************************************************************************

    Sets the function pointer to "func" for function named "func_name".
    Returns 0 on success or a negative error code.

************************************************************************/

int DEAU_set_func_pt (char *func_name, void *func) {
    char *cpt;
    function_pts_t *f;

    if (Func_tbl == NULL) {
	Func_tbl = MISC_open_table (sizeof (function_pts_t), 
					16, 0, &N_funcs, (char **)&Funcs);
	if (Func_tbl == NULL)
	    return (DEAU_MALLOC_FAILED);
    }

    cpt = (char *)malloc (strlen (func_name) + 1);
    f = (function_pts_t *)MISC_table_new_entry (Func_tbl, NULL);
    if (cpt == NULL || f == NULL)
	return (DEAU_MALLOC_FAILED);
    strcpy (cpt, func_name);
    f->name = cpt;
    f->pt = func;
    return (0);
}

/******************************************************************

    Parses the conversion description "desc" and sets the conversion 
    struct "c". In case of parsing error, c->conv_type is set to 
    DEAU_C_UNDEFINED. Returns 0 on success or a negative error code
    on failure.
	
******************************************************************/

int DEAU_parse_conversion_desc (char *desc, DEAU_conversion_t *c) {
    char quots[2], *pt;
    int cnt, ret;

    c->conv_type = DEAU_C_UNDEFINED;
    if (desc == NULL)
	return (0);
    cnt = Preprocess_attr_text (desc, quots);
    if (cnt <= 0)
	return (cnt);

    if (quots[0] == '[' && quots[1] == ']') {	/* scale and offset */
	double d1;
	if (cnt < 1 || cnt > 2)
	    return (DEAU_rep_err_ret ("Bad conversion spec: ", desc, 
							DEAU_BAD_ATTR_SPEC));
	ret = Get_next_item (DEAU_T_DOUBLE, &pt, NULL);
	if (ret <= 0)
	    return (DEAU_rep_err_ret ("Bad conversion spec: ", desc, 
							DEAU_BAD_ATTR_SPEC));
	d1 = *((double *)pt);
	ret = Get_next_item (DEAU_T_DOUBLE, &pt, NULL);
	if (ret <= 0)
	    return (DEAU_rep_err_ret ("Bad conversion spec: ", desc, 
							DEAU_BAD_ATTR_SPEC));
	c->scale = d1;
	c->offset = *((double *)pt);
	c->conv_type = DEAU_C_SCALE;
    }
    else if (quots[0] == '<' && quots[1] == '>') {	/* method */
	if (cnt != 1)
	    return (DEAU_rep_err_ret ("Bad conversion spec: ", desc, 
							DEAU_BAD_ATTR_SPEC));
	ret = Get_next_item (DEAU_T_STRING, &pt, NULL);
	if (ret <= 0 || (pt = Get_function_pt (pt)) == NULL)
	    return (DEAU_rep_err_ret ("Function not found: ", desc, 0));
	c->method = (void (*) (void *, void *))pt;
	c->conv_type = DEAU_C_METHOD;
    }
    else
	return (DEAU_rep_err_ret ("Bad conversion spec: ", desc, 
						DEAU_BAD_ATTR_SPEC));
    return (0);
}

/******************************************************************

    Parses the range description "desc" and sets the range 
    struct "r". In case of parsing error, r->range_type is set to 
    DEAU_R_UNDEFINED. Returns 0 on success or an negative error
    code on failure.
	
******************************************************************/

int DEAU_parse_range_desc (char *desc, int type, DEAU_range_t *r) {
    char quots[2], *pt;
    int cnt, ret;

    r->range_type = DEAU_R_UNDEFINED;
    if (desc == NULL)
	return (0);
    cnt = Preprocess_attr_text (desc, quots);
    if (cnt <= 0)
	return (cnt);

    if (quots[0] == '[' || quots[0] == '(') {	/* min and max */
	if (cnt != 2 || (quots[1] != ']' && quots[1] == ')'))
	    return (DEAU_rep_err_ret ("Bad range spec: ", desc, 
						DEAU_BAD_ATTR_SPEC));
	r->b_min = DEAU_R_INCLUSIVE;
	r->b_max = DEAU_R_INCLUSIVE;
	if (quots[0] == '(')
	    r->b_min = DEAU_R_NOT_INCLUSIVE;
	if (quots[1] == '(')
	    r->b_max = DEAU_R_NOT_INCLUSIVE;

	if (type == DEAU_T_STRING || type == DEAU_T_BOOLEAN) {
	    return (DEAU_rep_err_ret ("Spec not implemented: ", 
					desc, DEAU_SPEC_NOT_IMPLEMENTED));
	}
	else {
	    char *tok;

	    ret = Get_next_item (DEAU_T_DOUBLE, &pt, &tok);
	    if (ret < 0 && strcmp (tok, "-") == 0)
		r->b_min = DEAU_R_NOT_DEFINED;
	    else if (ret > 0 && pt != NULL)
		r->min = *((double *)pt);
	    else 		
		return (DEAU_rep_err_ret ("Bad range spec: ", desc, 
						DEAU_BAD_ATTR_SPEC));

	    ret = Get_next_item (DEAU_T_DOUBLE, &pt, &tok);
	    if (ret < 0 && strcmp (tok, "-") == 0)
		r->b_max = DEAU_R_NOT_DEFINED;
	    else if (ret > 0 && pt != NULL)
		r->max = *((double *)pt);
	    else 		
		return (DEAU_rep_err_ret ("Bad range spec: ", desc, 
						DEAU_BAD_ATTR_SPEC));

	    r->range_type = DEAU_R_MINMAX;
	}
    }
    else if (quots[0] == '{' && quots[1] == '}') {	/* selections */
	if (type == DEAU_T_STRING) {
	    char *b;
	    int off, i;

	    b = (char *)malloc (strlen (desc));
	    if (b == NULL)
		return (DEAU_rep_err_ret ("malloc failed", "", 
						DEAU_MALLOC_FAILED));
	    off = 0;
	    for (i =0; i < cnt; i++) {
		ret = Get_next_item (DEAU_T_STRING, &pt, NULL);
		if (ret <= 0)
		    return (DEAU_rep_err_ret ("Bad range spec: ", desc, 
						DEAU_MALLOC_FAILED));
		strcpy (b + off, pt);
		off += strlen (pt) + 1;
	    }
	    r->values = b;
	    r->range_type = DEAU_R_DISCRETE_STRINGS;
	}
	else {
	    double *b;
	    int i;

	    b = (double *)malloc (cnt * sizeof (double));
	    if (b == NULL)
		return (DEAU_rep_err_ret ("malloc failed", "", 
						DEAU_MALLOC_FAILED));
	    for (i =0; i < cnt; i++) {
		ret = Get_next_item (DEAU_T_DOUBLE, &pt, NULL);
		if (ret <= 0)
		    return (DEAU_rep_err_ret ("Bad range spec: ", desc, 
						DEAU_BAD_ATTR_SPEC));
		b[i] = *((double *)pt);
	    }
	    r->values = (char *)b;
	    r->range_type = DEAU_R_DISCRETE_NUMBERS;
	}
	r->n_values = cnt;
    }
    else if (quots[0] == '<' && quots[1] == '>') {	/* method */
	ret = Get_next_item (DEAU_T_STRING, &pt, NULL);
	if (cnt != 1 || ret <= 0)
	    return (DEAU_rep_err_ret ("Bad range spec: ", desc, 
						DEAU_BAD_ATTR_SPEC));
	pt = Get_function_pt (pt);
	if (pt == NULL)
	    return (DEAU_rep_err_ret ("Function not found: ", desc, 0));
	r->method = (int (*) (void *))pt;
	r->range_type = DEAU_R_METHOD;
    }
    else
	return (DEAU_rep_err_ret ("Bad range spec: ", desc, 
						DEAU_BAD_ATTR_SPEC));

    return (0);
}

/******************************************************************

    Checks if "attr" is valid in terms of the syntex. Returns 0 if
    no error is found or a negative error code otherwise.

******************************************************************/

int DEAU_check_attributes (DEAU_attr_t *attr) {
    char quots[2], *id;
    int type, n_sels, err, cnt;

    id = attr->ats[DEAU_AT_ID];
    if (id[0] == '\0')
	return (DEAU_rep_err_ret ("Bad DE ID - not specified ", "", 
						DEAU_BAD_ID));
    type = DEAU_get_data_type (attr);
    if (type < 0)
	return (DEAU_rep_err_ret ("Bad type - ID: ", id, 
						DEAU_BAD_TYPE_NAME));

    n_sels = -1;
    err = 0;
    if (attr->ats[DEAU_AT_RANGE][0] != '\0') {	/* check range */
	cnt = Preprocess_attr_text (attr->ats[DEAU_AT_RANGE], quots);
	if (cnt <= 0)
	    err = DEAU_BAD_RANGE;
	else if (quots[0] == '[' || quots[0] == '(') {	/* min and max */
	    if (cnt != 2 || (quots[1] != ']' && quots[1] == ')'))
		err = DEAU_BAD_RANGE;
	}
	else if (quots[0] == '{' && quots[1] == '}')	/* selections */
	    n_sels = cnt;
	else if (quots[0] == '<' && quots[1] == '>') {	/* method */
	    if (cnt != 1)
		err = DEAU_BAD_RANGE;
	}
	else
	    err = DEAU_BAD_RANGE;
    }
    if (err < 0)
	return (DEAU_rep_err_ret ("Bad range - ID: ", id, err));

    err = 0;
    if (attr->ats[DEAU_AT_CONVERSION][0] != '\0') {	/* check conversion */
	cnt = Preprocess_attr_text (attr->ats[DEAU_AT_CONVERSION], quots);
	if (cnt <= 0)
	    err = DEAU_BAD_CONVERSION;
	else if (quots[0] == '[' && quots[1] == ']') {	/* scale and offset */
	     if (cnt < 1 || cnt > 2)
		err = DEAU_BAD_CONVERSION;
	}
	else if (quots[0] == '<' && quots[1] == '>') {	/* method */
	    if (cnt != 1)
		err = DEAU_BAD_CONVERSION;
	}
	else
	    err = DEAU_BAD_CONVERSION;
    }
    if (err < 0)
	return (DEAU_rep_err_ret ("Bad conversion - ID: ", id, err));

    err = 0;
    if (attr->ats[DEAU_AT_ENUM][0] != '\0') {	/* check enum */
	if (type != DEAU_T_STRING || n_sels < 0)
	    err = DEAU_BAD_ENUM;
	cnt = Preprocess_attr_text (attr->ats[DEAU_AT_ENUM], NULL);
	if (cnt != n_sels)
	    err = DEAU_BAD_ENUM;
	else {
	    cnt = 0;
	    while (1) {
		int t, ret;
		ret = Get_next_item (DEAU_T_INT, &t, NULL);
		if (ret <= 0)
		    break;
		cnt++;
	    }
	    if (cnt != n_sels)
		err = DEAU_BAD_ENUM;
	}
    }
    if (err < 0)
	return (DEAU_rep_err_ret ("Bad enum - ID: ", id, err));

    return (0);
}

/******************************************************************

    Parses attribute text "text" if it is not NULL. If "buf"
    is not NULL, The first and the last characters, considered as the
    quotation marks, are returned in "buf". If there is no quotation 
    marks, this function is called with "buf" = NULL. The number of
    fields separated by "," is returned on success. It returns a
    negative error code on failure.

******************************************************************/

static int Preprocess_attr_text (char *text, void *buf) {
    static char *vb = NULL;
    char *cpt, *c;
    int len, cnt;

    if (text == NULL)
	return (0);

    Next_tok = "";
    cpt = text;
    while (*cpt == ' ' || *cpt == '\t')
	cpt++;
    len = strlen (cpt);
    if (len == 0)
	return (0);
    vb = STR_reset (vb, 128);
    vb = STR_append (vb, cpt, len + 1);
    cpt = vb + len - 1;
    while (cpt >= vb) {
	if (*cpt == ' ' || *cpt == '\t' || *cpt == '\n')
	    *cpt = '\0';
	else
	    break;
	cpt--;
    }
    if (cpt < vb)
	return (0);
    if (buf != NULL) {
	c = (char *)buf;
	c[1] = *cpt;
	*cpt = '\0';
	cpt = vb;
	c[0] = *cpt;
	cpt++;
    }
    else
	cpt = vb;
    Next_tok = cpt;

    c = cpt - 1;
    cnt = 0;
    while (*cpt != '\0') {
	if (*cpt == ',') {
	    cnt++;
	    c = cpt;
	}
	cpt++;
    }
    cpt = c + 1;
    while (*cpt != '\0') {
	if (*cpt != ' ' && *cpt != '\t') {
	    cnt++;
	    break;
	}
	cpt++;
    }

    return (cnt);
}

/******************************************************************

    Returns the next item, in the current attribute text, pointed by
    "Next_tok" with "buf". Conversion is performed according to
    "type". For string type, returned is a pointer that can be used
    immediately after call. Returns 1 on success or 0 if there is no
    more token. If "type" is not DEAU_T_STRING, the function returns 
    -1 when the token is empty or conversion fails. "Next_tok" is 
    reset. The numbers are converted either to a double or to an 
    integer. If "token" is not NULL, the current token is returned 
    with it (even if the return value is -1).
	
******************************************************************/

static int Get_next_item (int type, void *buf, char **token) {
    char *t_pt, *cpt;

    if (token != NULL)
	*token = "";
    t_pt = Next_tok;
    if (*t_pt == '\0')
	return (0);
    while (*t_pt == ' ' || *t_pt == '\t')
	t_pt++;
    cpt = t_pt;
    while (*cpt != ',' && *cpt != '\0')
	cpt++;
    if (*cpt == ',') {
	*cpt = '\0';
	Next_tok = cpt + 1;
	if (*t_pt == '\0' && type != DEAU_T_STRING)
	    return (-1);
    }
    else {
	Next_tok = cpt;
	if (*t_pt == '\0')
	    return (0);
    }
    cpt--;
    while (cpt >= t_pt && (*cpt == ' ' || *cpt == '\t'))
	cpt--;
    cpt[1] = '\0';

    if (token != NULL)
	*token = t_pt;
    if (type == DEAU_T_STRING) {
	*((char **)buf) = t_pt;
    }
    else if (type == DEAU_T_FLOAT || type == DEAU_T_DOUBLE) {
	static double d;
	char c;
	if (sscanf (t_pt, "%lf%c", &d, &c) != 1)
	    return (-1);
	*((double **)buf) = &d;
    }
    else {
	static int i;
	char c;
	if (sscanf (t_pt, "%d%c", &i, &c) != 1)
	    return (-1);
	*((int **)buf) = &i;
    }

    return (1);
}

/******************************************************************

    Returns the data type of attribute "attr". It returns
    the type macro on success or -1 on failure.
	
******************************************************************/

int DEAU_get_data_type (DEAU_attr_t *attr) {
    return (DEAU_get_data_type_by_string (attr->ats[DEAU_AT_TYPE]));
}

/******************************************************************

    Converts type name "type_name" to data type macro. It returns
    the macro on success or -1 on failure.
	
******************************************************************/

int DEAU_get_data_type_by_string (char *type_name) {
    static char *t_names[] = {"int", "short", "byte", "uint", "ushort", 
		"ubyte", "bit", "float", "double", "string", "boolean"};
    int i;

    if (type_name == NULL || type_name[0] == '\0')
	return (DEAU_T_UNDEFINED);
    for (i = 0; i < 11; i++) {
	if (strcasecmp (type_name, t_names[i]) == 0)
	    return (i + 1);
    }
    return (-1);
}

/**************************************************************************

    Computes and returns the hash value of name "name".

**************************************************************************/

unsigned int DEAU_get_hash_value (char *name) {
    unsigned char *cpt;
    unsigned int h, cnt;

    if (name == NULL)
	return (0);
    h = 0;
    cpt = (unsigned char *)name;
    cnt = 0;
    while (*cpt != '\0') {
	unsigned int t;
	t = *cpt;
	h += t << (cnt % 24);
	cnt++;
	cpt++;
    }
    return (h);
}

/******************************************************************

    Returns the function pointer for function named "name". Returns
    NULL on failure.
	
******************************************************************/

static char *Get_function_pt (char *name) {
    char *p;
    int i;

    for (i = 0; i < N_funcs; i++) {
	if (strcmp (name, Funcs[i].name) == 0)
	    return ((char *)Funcs[i].pt);
    }

    p = (char *)dlsym (RTLD_DEFAULT, name);
    if (dlerror() != NULL || p == NULL)
	return (NULL);
    return (p);
}

/************************************************************************

    Parses text attribute "value" and put the numerical values in "buffer"
    of size "buf_size". If "buf_size" is 0. No parsing is performed and
    the number of values is returned. Returns the number of values found 
    or a negative error code.

************************************************************************/

static int Parse_values (char *value, double *buffer, int buf_size) {
    int cnt, ind, ret, buf_too_small;
    char *pt;

    if (value == NULL)
	return (0);
    cnt = Preprocess_attr_text (value, NULL);
    if (cnt <= 0)
	return (0);
    if (buf_size <= 0)
	return (cnt);

    ind = buf_too_small = 0;
    while (1) {
	ret = Get_next_item (DEAU_T_DOUBLE, &pt, NULL);
	if (ret < 0)
	    return (DEAU_rep_err_ret ("Bad value spec: ", 
					    value, DEAU_BAD_NUMERICAL));
	if (ret == 0)
	    break;
	if (ind < buf_size)
	    buffer[ind] = *((double *)pt);
	else {
	    buf_too_small = 1;
	    break;
	}
	ind++;
    }

    if (buf_too_small)
	return (DEAU_BUFFER_TOO_SMALL);
    return (ind);
}

/************************************************************************

    Parses text attribute "value" and put the string values in a STR 
    buffer "p" provided by the caller. Returns the number of values found 
    or a negative error code.

************************************************************************/

static int Parse_string_values (char *value, char **p) {
    int num;

    if (value == NULL)
	return (0);
    num = Preprocess_attr_text (value, NULL);
    if (num <= 0)
	return (0);
    return (Parse_string_fields (p));
}

/************************************************************************

    Parses the fields as string in "field1, field2, ..." and returns
    them as a sequence of null-terminated strings with "p" which is a
    STR pointer supplied by the caller. It is assumed
    the Preprocess_attr_text has already been called. Returns the number
    of fields.

************************************************************************/

static int Parse_string_fields (char **p) {
    char *vb;
    int cnt, ret;
    char *pt;

    vb = *p;
    vb = STR_reset (vb, 256);
    cnt = 0;
    while (1) {
	ret = Get_next_item (DEAU_T_STRING, &pt, NULL);
	if (ret <= 0)
	    break;
	vb = STR_append (vb, pt, strlen (pt) + 1);
	cnt++;
    }
    *p = vb;

    return (cnt);
}

/************************************************************************

    Parses the field of "which_value", which is either DEAU_AT_VALUE or
    DEAU_AT_BASELINE, of attribute "attr" as enumeration value and returns
    them in buffer "values" of size "buf_size". Returns the number
    of values or a negative error code.

************************************************************************/

static int Parse_enum_values (DEAU_attr_t *attr, 
			int which_value, double *values, int buf_size) {
    static char *v = NULL;
    int n_values, i;
    char *p;

    n_values = Parse_string_values (attr->ats[which_value], &v);
    if (n_values < 0)
	return (n_values);

    p = v;
    for (i = 0; i < n_values; i++) {
	int ret, enu;

	if (i >= buf_size)
	    return (DEAU_BUFFER_TOO_SMALL);
	if (i == 0)
	    ret = DEAU_get_enum (attr, p, &enu, 0);
	else
	    ret = DEAU_get_enum (NULL, p, &enu, 0);	/* more efficient */
	if (ret < 0)
	    return (DEAU_BAD_ENUM);
	values[i] = (double)enu;
	p += strlen (p) + 1;
    }

    return (n_values);
}

/************************************************************************

    Searches the site name "site" in site name list "site_names". Returns
    1 if found or 0 otherwise. Site group names are expanded.

************************************************************************/

static int Match_site (char *site, char *site_names) {
    static char *vb = NULL;
    char *p, *st, *v, c;
    int off, ret;

    vb = STR_reset (vb, 256);
    vb = STR_append (vb, site_names, strlen (site_names) + 1);

    off = 0;
    while (1) {		/* expand site group names */
	p = vb + off;
	while (*p == ' ' || *p == '\t')
	    p++;
	st = p;
	if (*st == '\0')
	    break;
	while (*p != ' ' && *p != '\t' && *p != '\0')
	    p++;
	if (p == st)
	    break;
	c = *p;
	*p = '\0';
	if ((ret = Look_up_site_group (st, &v)) < 0)
	    return (ret);
	*p = c;
	off = p - vb;
	if (ret > 0) {
	    vb = STR_replace (vb, st - vb, p - st, v, strlen (v));
	    off += strlen (v) - (p - st);
	}
    }

    p = strtok (vb, " \t");
    while (p != NULL) {
	if (strcasecmp (p, "Other_sites") == 0 ||
	    strcasecmp (p, site) == 0)
	    return (1);
	p = strtok (NULL, " \t");
    }
    return (0);
}

/************************************************************************

    Looks up the site group names and returns the content of the group
    name. If the group name is not found, returns NULL.

************************************************************************/

static int Look_up_site_group (char *gname, char **value) {
    static char *vb = NULL;
    static int n_sgs = -1;
    int ret, i, n_brs;
    char *p;

    if (n_sgs < 0) {		/* intializes the group database */
	char *id, *v;

	n_sgs = 0;
	n_brs = DEAU_get_branch_names ("@site_names", &p);
	if (n_brs <= 0)
	    return (n_brs);
	vb = STR_reset (vb, 256);
	id = NULL;
	for (i = 0; i < n_brs; i++) {
	    id = STR_copy (id, "@site_names.");
	    id = STR_cat (id, p);
	    ret = DEAU_get_string_values (id, &v);
	    if (ret > 0) {
	        vb = STR_append (vb, p, strlen (p) + 1);
	        vb = STR_append (vb, v, strlen (v) + 1);
		n_sgs++;
	    }
	    p += strlen (p) + 1;
	}
	STR_free (id);
    }

    if (gname == NULL)
	return (0);
    p = vb;
    for (i = 0; i < n_sgs; i++) {
	if (strcmp (p, gname) == 0) {
	    *value = p + strlen (p) + 1;
	    return (1);
	}
	p += strlen (p) + 1;
	p += strlen (p) + 1;
    }
    return (0);
}

/************************************************************************

    uuencode routine with modifications of escaping certain chars. The
    input is "barray" of "size" bytes. The pointer to the encoded string
    is returned with "strp". Returns the length of the encoded string on
    success or a negative error code on failure. The caller needs to free 
    the returned pointer if the return value > 0.

************************************************************************/

int DEAU_uuencode (unsigned char *barray, int size, unsigned char **strp) {
    unsigned char *out, *p, *op, b[3];
    int cnt, i;

    out = (unsigned char *)malloc (size * 4 / 3 + 16);
    if (out == NULL)
	return (-1);

    sprintf ((char *)out, "%d:", size);
    cnt = size / 3;
    p = barray;
    op = out + strlen ((char *)out);
    for (i = 0; i < cnt; i++) {
	uuencode_one (p, op);
	p += 3;
	op += 4;
    }
    if (cnt * 3 != size) {
	for (i = 0; i < 3; i++) {
	    if (p < barray + size) {
		b[i] = *p;
		p++;
	    }
	    else
		b[i] = 0;
	}
	uuencode_one (b, op);
	op += 4;
    }
    *op = '\0';
    for (i = 0; i < op - out; i++) {
	int c = out[i];
	if (c == 32 || c == 44 || c == 59)
	    out[i] += 140;
    }
    *strp = out;
    return (op - out);
}

/************************************************************************

    uuencode routine of one section (three chars).

************************************************************************/

static void uuencode_one (unsigned char *in, unsigned char *out) {
    int A, B, C;

    A = in[0];
    B = in[1];
    C = in[2];
    out[0] = 0x20 + (( A >> 2) & 0x3F);
    out[1] = 0x20 + (((A << 4) | ((B >> 4) & 0xF)) & 0x3F);
    out[2] = 0x20 + (((B << 2) | ((C >> 6) & 0x3)) & 0x3F);
    out[3] = 0x20 + ((C) & 0x3F);
}

/************************************************************************

    uudecode routine with modifications of escaping certain chars.

************************************************************************/

static int uudecode (unsigned char *str, unsigned char **outp) {
    unsigned char *out, *p, *op;
    int size, len, i;

    if (sscanf ((char *)str, "%d", &size) != 1)
	return (DEAU_BAD_CONVERSION);
    out = (unsigned char *)malloc (size + 8);
    if (out == NULL)
	return (DEAU_MALLOC_FAILED);
    len = strlen ((char *)str);
    p = str;
    while (*p != '\0' && *p != ':')
	p++;
    if (*p == ':')
	p++;
    for (i = p - str; i < len; i++) {
	if (str[i] >= 140)
	    str[i] -= 140;
    }
    op = out;
    while (1) {
	int w, x, y, z;
	if (op - out >= size)
	    break;
	if (p - str + 4 > len)
	    return (DEAU_BAD_CONVERSION);
	w = *p - 0x20;
	x = p[1] - 0x20;
	y = p[2] - 0x20;
	z = p[3] - 0x20;
	p += 4;
	op[0] = (w << 2) | ((x >> 4) & 0x3);
	op[1] = ((x & 0xf) << 4) | ((y >> 2) & 0xf);
	op[2] = ((y & 0x3) << 6) | (z & 0x3f);
	op += 3;
    }
    *outp = out;
    return (size);
}

/************************************************************************

    Retrieves binary values of data element "id", puts them in a malloced 
    buffer and returns the pointer to the buffer with "p". If "baseline"
    is non-zero, baseline value is retrieved. The function 
    returns the number of bytes retrieved on success. On failure the 
    function returns a negative error code. See DEAU man-page for more 
    spec. The caller must free the pointer if the return value > 0.

************************************************************************/

int DEAU_get_binary_value (char *id, char **p, int baseline) {
    char *ep, *dp;
    int ret;

    if (baseline)
	ret = DEAU_get_baseline_string_values (id, &ep);
    else
	ret = DEAU_get_string_values (id, &ep);
    if (ret <= 0)
	return (ret);
    if (ret > 1)
	return (DEAU_BAD_CONVERSION);
    if ((ret = uudecode ((unsigned char *)ep, (unsigned char **)&dp)) < 0)
	return (ret);
    *p = dp;
    return (ret);
}

/************************************************************************

    Encodes binary array "bytes" of size "n_bytes" and uses the result 
    to set the value of DE "id". If "baseline" is non-zero, the base-line 
    attribute, instead of the current value attribute, is set. The function 
    returns the string length of the DE value on success or a negative 
    error code. See DEAU man-page for more spec.

************************************************************************/

int DEAU_set_binary_value (char *id, void *bytes, int n_bytes, int baseline) {
    unsigned char *p;
    int len, ret;

    if ((len = DEAU_uuencode ((unsigned char *)bytes, n_bytes, &p)) <= 0)
	return (len);
    ret = DEAU_set_values (id, 1, p, 1, baseline);
    free (p);
    if (ret < 0)
	return (ret);
    return (len);
}



