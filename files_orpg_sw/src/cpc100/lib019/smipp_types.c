/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2006/09/15 15:04:12 $
 * $Id: smipp_types.c,v 1.7 2006/09/15 15:04:12 jing Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <smi.h>
#include "smipp_def.h"

typedef struct {
    int left;			/* index in Tks for the first token */
    int right;			/* index in Tks for the last token */
} Typedef_ind;

static Typedef_ind *Tds = NULL;	/* typedef table */
static int N_tds = 0;		/* size of typedef table */
static int Td_bufsize = 0;	/* buffer size of typedef table */

static char *Text;		/* tokenized text */
static int *Tks;		/* token table */
static int Tkcnt;		/* size of token table */

extern char Prog_name[];
extern int SMIM_debug;

struct stored_p {		/* struct for keeping track of malloced 
				   pointers so later can be freed */	
    struct stored_p *next;
};
typedef struct stored_p Stored_p_t;
typedef struct {
    Stored_p_t *sp;		/* list of malloced pointers */
    Stored_p_t *last_sp;	/* last entry of sp */
} Stored_t;


static int Find_type (int st_tkind, int n_tks, char *ret_buf, int buf_size);
static int Find_typedef (char *tdef, char *ret_buf, int buf_size);
static void Replace_chars (char *str, int len, char *in_str, int size);
static int Search_for_struct_def (char *name, int *st_ind, int *end_ind);
static int Calculate_array_size (int st, int last, char **size_code);
static char *Get_field_name_and_type (int st, int last, char **type);
static char *Local_malloc (int size, Stored_t *sp);
static void Local_free (Stored_t *sp);
static int Process_a_field (int st, Smi_field_t *field, 
					Stored_t *sp, int *need_data);
static void Append_str (char **str, char *str1);


/**************************************************************************

    Initialize this module. 

**************************************************************************/

void SMIT_init (char *buf, int *tks, int tkcnt) {
    int level, i, k;

    Text = buf;
    Tks = tks;
    Tkcnt = tkcnt;

    for (i = 0; i < tkcnt; i++) {
	if (strcmp (buf + tks[i], "typedef") != 0)
	    continue;

	if (N_tds >= Td_bufsize) {
	    char *b;
	    Td_bufsize = N_tds + 512;
	    b = SMIM_malloc (Td_bufsize * sizeof (Typedef_ind));
	    memcpy (b, Tds, N_tds * sizeof (Typedef_ind));
	    Tds = (Typedef_ind *)b;
	}

	Tds[N_tds].left = i + 1;
	level = 0;
	for (k = i + 2; k < tkcnt; k++) {
	    if (*(buf + tks[k]) == '{')
		level++;
	    else if (*(buf + tks[k]) == '}')
		level--;
	    else if (level == 0) {
		if (*(buf + tks[k]) == ';') {	/* other typedef */
		    Tds[N_tds].right = k - 1;
		    N_tds++;
		    break;
		}
		if (*(buf + tks[k]) == '(') {	/* function pointer */
		    if (k + 2 < tkcnt &&
			*(buf + tks[k + 1]) == '*' && 
			SMIP_is_alpha (*(buf + tks[k + 2]))) {
			Tds[N_tds].right = k + 2;
			N_tds++;
			break;
		    }
		    else {
			if (SMIM_debug)
			    SMIP_warning (k, "unexpected typedef\n");
			break;
		    }
		}
	    }
	}
    }
}

/**************************************************************************

    Returns meta info of struct or type "name". Returns NULL on failure.
    Nothing to be freed by the caller.

**************************************************************************/

Smi_info_t *SMIT_get_smi (char *name) {
    static Stored_t sp = {NULL, NULL};
    static Smi_info_t smi;
    Smi_field_t *flds;
    Smi_vss_size_t *vsize;
    int st_ind, end_ind, n_fields, next_st, n_vsfs, i, k;
    int *saved_tks;

    if ((n_fields = Search_for_struct_def (name, &st_ind, &end_ind)) == 0)
	return (NULL);

    Local_free (&sp);
    flds = (Smi_field_t *)Local_malloc (n_fields * sizeof (Smi_field_t), &sp);

    saved_tks = NULL;
    next_st = st_ind;
    n_vsfs = 0;
    Process_a_field (-1, 0, NULL, NULL);
    for (i = 0; i < n_fields; i++) {
	int need_data, next_tk, ind, level;

 	next_tk = Process_a_field (next_st, flds + i, &sp, &need_data);
	if (next_tk < 0)
	    return (NULL);
	if (*(Text + Tks[next_tk - 1]) == ',') {
					/* more fields on the same line */
	    if (saved_tks == NULL) {	/* save the token indexes */
		saved_tks = (int *)SMIM_malloc (
				(end_ind - st_ind + 1) * sizeof (int));
		for (k = 0; k <= end_ind - st_ind; k++)
		    saved_tks[k] = Tks[st_ind + k];
	    }
	    ind = next_tk - 2;		/* the first token to remove */
	    level = 0;
	    while (ind >= next_st) {
		char c;
		c = *(Text + Tks[ind]);
		if (c == ']')
		    level++;
		else if (c == '[')
		    level--;
		else if (level == 0 && (Text + Tks[ind])[-1] == ' ')
		    break;
		ind--;
	    }
	    /* move type tokens to the front of the next field name */
	    for (k = ind - 1; k >= next_st; k-- )
		Tks[next_tk + k - ind] = Tks[k];	
	    next_st = next_tk + next_st - ind;
	}
	else
	    next_st = next_tk;
	if (need_data) 
	    n_vsfs++;
    }
    if (saved_tks != NULL) {	/* restore save the token indexes */
	for (k = 0; k <= end_ind - st_ind; k++)
	    Tks[st_ind + k] = saved_tks[k];
	free (saved_tks);
    }

    if (SMIM_debug)
	printf ("Struct: name %s, n_fields %d\n", name, n_fields);

    smi.vsize = NULL;
    if ((vsize = SMIP_get_vsize (st_ind, end_ind)) != NULL) {
	smi.vsize = vsize->size;
	if (vsize->need_data)
	    n_vsfs++;
    }
    smi.name = name;
    smi.n_fs = n_fields;
    smi.n_vsfs = n_vsfs;
    smi.fields = flds;
    smi.src_line = SMIP_get_source_name (st_ind - 1, &(smi.src_name));
    if (n_vsfs > 0)
	smi.need_data = 1;
    else
	smi.need_data = 0;
    return (&smi);
}

/**************************************************************************

    Finds meta info of a field started with token index "st". The info 
    is put in "field". Returns -1 on failure or the token index of
    next field.

**************************************************************************/

static int Process_a_field (int st, Smi_field_t *field, 
					Stored_t *sp, int *need_data) {
    static int union_name_ind = 0;
    int last, i, level;
    char c, *f_type, *cpt;
    int need_type_resolution;
    Smi_vss_info_t *vss;

    if (st < 0) {
	union_name_ind = 0;		/* not in a union */
	return (0);
    }

    if (union_name_ind && *(Text + Tks[st]) == '}') {
	union_name_ind = 0;
	while (st < Tkcnt) {
	    if (*(Text + Tks[st]) == ';') {
		st++;
		break;
	    }
	    st++;
	}
    }
    if (!union_name_ind && st + 1 < Tkcnt && *(Text + Tks[st + 1]) == '{' &&
	strcmp (Text + Tks[st], "union") == 0) {
	union_name_ind = 1;
	st += 2;
	for (i = st; i < Tkcnt; i++) {
	    if (*(Text + Tks[i]) == '}')
		break;
	}
	if (i >= Tkcnt)
	    return (-1);
	union_name_ind = i + 1;
	if (*(Text + Tks[union_name_ind]) == ';')
	    union_name_ind = -1;		/* union without name */
    }

    level = 0;
    for (i = st; i < Tkcnt; i++) {
	c = *(Text + Tks[i]);
	if (c == '{')
	    level++;
	else if (c == '}')
	    level--;
	else if (level == 0 && (c == ';' || c == ',')) {
	    last = i;
	    break;
	}
    }
    if (i >= Tkcnt)		/* could not find ';' or ',' */
	return (-1);

    vss = SMIP_get_vss (i);
    if (vss != NULL) {
	field->ni_exp = vss->size;
	field->offset_exp = vss->offset;
	field->n_items = 0;
	*need_data = vss->need_data;
    }
    else {
	field->ni_exp = NULL;
	field->offset_exp = NULL;
	field->n_items = Calculate_array_size (st, last - 1, NULL);
	if (field->n_items < 0)
	    field->n_items = Calculate_array_size (st, last - 1, 
							&(field->ni_exp));
	*need_data = 0;
    }

    field->name = Get_field_name_and_type (st, last - 1, &f_type);
    if (union_name_ind > 0) {
	char *b = SMIM_malloc (strlen (field->name) + 
				strlen (Text + Tks[union_name_ind]) + 2);
	sprintf (b, "%s.%s", Text + Tks[union_name_ind], field->name);
	field->name = b;
    }
    field->type = Local_malloc (strlen (f_type) + 1, sp);
    strcpy (field->type, f_type);

/* printf ("  Field: name %s, type %s, n_items %d\n", 
			field->name, field->type, field->n_items); */

    /* process any recursive typedef resolution */
    cpt = f_type;
    need_type_resolution = 0;
    if (strncmp (cpt, "struct ", 7) == 0) {
	cpt += 7;
	need_type_resolution = 1;
    }
    while (*cpt != ' ' && *cpt != '\t' && *cpt != '\n' && *cpt != '\0')
	cpt++;
    *cpt = '\0';		/* take the first part of the type */
    for (i = 0; i < N_tds; i++) {
	if (strcmp (Text + Tks[Tds[i].right], f_type) == 0) {	/* found */
	    need_type_resolution = 1;
	    break;
	}
    }
    if (need_type_resolution) {
	Smi_struct_t *smi;
	smi = (Smi_struct_t *)SMIM_malloc (sizeof (Smi_struct_t) + 
						strlen (f_type) + 1);
	smi->name = (char *)smi + sizeof (Smi_struct_t);
	strcpy (smi->name, f_type);
	smi->major = smi->minor = NULL;
	smi->next_id = NULL;
	SMIP_add_new_smi_struct (smi);
    }
    return (last + 1);
}

/**************************************************************************

    Gets the field name and type of a struct field started at token "st" and
    ended at token "last". The name and type does not need to be freed.

**************************************************************************/

static char *Get_field_name_and_type (int st, int last, char **type) {
    int level, i;
    static char buf[512];

    *type = buf;
    level = 0;
    for (i = st; i <= last; i++) {
	char c = *(Text + Tks[i]);
	if (c == '{')
	    level++;
	else if (c == '}')
	    level--;
	else if (level != 0)
	    continue;
	if (c == '(') {			/* a function pointer */
	    if (i + 2 <= last && *(Text + Tks[i + 1]) == '*') {
		if (SMIT_find_type (st, last - st + 1, buf, 512) == 0)
		    strcpy (buf, "UNKNOWN TYPE");
		return (Text + Tks[i + 2]);
	    }
	    break;
	}
	if (c == '[') {
	    if (i - 1 >= st) {
		if (SMIT_find_type (st, i - st - 1, buf, 512) == 0)
		    strcpy (buf, "UNKNOWN TYPE");
		return (Text + Tks[i - 1]);
	    }
	    break;
	}
	if (i == last) {
	    if (i >= st) {
		if (SMIT_find_type (st, i - st, buf, 512) == 0)
		    strcpy (buf, "UNKNOWN TYPE");
		return (Text + Tks[i]);
	    }
	    break;
	}
    }
    SMIP_error (i, "field name not found\n");
    exit (1);
}

/**************************************************************************

    Calculates of array size of a struct field started at token "st" and
    ended at token "last". Returns -1 if the size is not explicit. If
    *size_code is not NULL, returns the size expression.

**************************************************************************/

static int Calculate_array_size (int st, int last, char **size_code) {
    int size, i, in, level, cnt, k;
    char c, *code;

    c = *(Text + Tks[last]);
    if (c != ']' && c != ')')
	return (1);			/* not an array */

    in = level = 0;
    size = 1;
    code = NULL;
    for (i = st; i <= last; i++) {
	char c = *(Text + Tks[i]);
	if (c == '{')
	    level++;
	else if (c == '}')
	    level--;
	else if (level != 0)
	    continue;
	if (c == '[') {
	    if (in > 0)
		break;			/* syntex error - nested [] */
	    cnt = 0;
	    in++;
	}
	else if (c == ']') {
	    if (size_code == NULL) {
		int t;
		if (cnt == 0)
		    t = 1;
		else if (cnt != 1 ||
		    sscanf (Text + Tks[i - 1], "%d", &t) != 1)
		    return (-1);
		size *= t;
	    }
	    else {
		if (in <= 0)
		    SMIP_error (i, "empty array size\n");
		if (code != NULL)
		    Append_str (&code, " * ");
		Append_str (&code, "(");
		for (k = 0; k < cnt; k++)
		    Append_str (&code, Text + Tks[k + i - cnt]);
		Append_str (&code, ")");
	    }
	    in--;
	}
	else if (in)			/* in [] */
	    cnt++;
    }
    if (in != 0)
	SMIP_error (i, "syntex error\n");
    if (size_code != NULL) {
	*size_code = code;
	size = 0;
    }
    return (size);
}

/**************************************************************************

    Appends "str1" to "str". Every time a new buffer is allocated.

**************************************************************************/

static void Append_str (char **str, char *str1) {
    int len;
    char *cpt;

    len = strlen (str1);
    if (*str != NULL)
	len += strlen (*str);
    cpt = SMIM_malloc (len + 1);
    if (*str != NULL) {
	strcpy (cpt, *str);
	free (*str);
    }
    else
	cpt[0] = '\0';
    strcat (cpt, str1);
    *str = cpt;
}

/**************************************************************************

    Searches for the section that defines struct or type "name". The 
    indexes to the first and last tokens are returned in "st_ind" and 
    "end_ind". The function returns the number of fields on success or 
    0 on failure.

**************************************************************************/

static int Search_for_struct_def (char *name, int *st_ind, int *end_ind) {
    int st, i, level, union_level, cnt, end;
    char buf[512], *resolved_name;

    if (strncmp (name, "struct ", 7) != 0 &&	/* resolve short typedef */
	SMIT_find_typedef (name, buf, 512) > 0 &&
	strncmp (buf, "struct ", 7) == 0)
	resolved_name = buf;
    else
	resolved_name = name;

    st = 0;		/* search start point */
    if (strncmp (resolved_name, "struct ", 7) == 0) {	/* a struct */
	for (i = 0; i < Tkcnt; i++) {
	    if (strcmp (Text + Tks[i], resolved_name) == 0 &&
			i + 1 < Tkcnt && *(Text + Tks[i + 1]) == '{') {
		st = i;
		end = Tkcnt;
		break;
	    }
	}
	if (i >= Tkcnt)
	    return (0);
    }
    else {					/* a typedef with definition */
	for (i = 0; i < N_tds; i++) {
	    if (strcmp (Text + Tks[Tds[i].right], resolved_name) == 0) {
		st = Tds[i].left;
		end = Tds[i].right + 1;		/* search must stop here */
		break;
	    }
	}
	if (i >= N_tds)
	    return (0);
    }

    cnt = 0;
    level = 0;
    union_level = -1;
    *st_ind = -1;
    for (i = st; i < Tkcnt; i++) {
	char c;

	c = *(Text + Tks[i]);
	if (c == '{') {
	    level++;
	    if (i > st && strcmp (Text + Tks[i - 1], "union") == 0)
		union_level = level;
	}
	else if (c == '}') {
	    if (union_level == level) {
		union_level = -1;
		cnt--;
	    }
	    level--;
	}
	else if (level == 1 || union_level == level) {
	    if (*st_ind < 0)
		*st_ind = i;
	    *end_ind = i;
	    if (c == ';' || c == ',' )
		cnt++;
	}
	if (level == 0 && *st_ind > 0)
	    return (cnt);
	if (i >= end)
	    break;
    }
    return (0);
}

/**************************************************************************

    Find the full type definition of typedef "type" and returns in 
    "ret_buf". If not found, returns "type". Returns 1 if found,
    or zero if not-found.

**************************************************************************/

int SMIT_find_typedef (char *type, char *ret_buf, int buf_size) {
    int i;

    for (i = 0; i < N_tds; i++) {
	if (strcmp (Text + Tks[Tds[i].right], type) == 0) {	/* found */
	    if (SMIT_find_type (Tds[i].left, 
			Tds[i].right - Tds[i].left, ret_buf, buf_size) &&
			strcmp (ret_buf, "struct") != 0)
		return (1);
	}
    }
    if (strlen (type) + 1 > buf_size)
	SMIP_error (-1, "buffer overflow\n");
    strcpy (ret_buf, type);
    return (0);
}

/**************************************************************************

    Find recursively the full type definition of text started with token 
    index "st_tkind" and of length of "n_tks" tokens. Return 0 if not a 
    valid type 1 otherwise.

**************************************************************************/

int SMIT_find_type (int st_tkind, int n_tks, char *ret_buf, int buf_size) {
    int changed;

    if (Find_type (st_tkind, n_tks, ret_buf, buf_size) == 0)
	return (0);

    changed = 1;
    while (changed > 0) {
	char *cpt, *tk, *pre_tk;

	cpt = ret_buf;
	tk = pre_tk = NULL;
	changed = 0;
	while (1) {
	    char c;
	    char buf[512];

	    if ((c = *cpt) == ' ' || c == '\0') {
		*cpt = '\0';
		if (tk != NULL && 
		    (pre_tk == NULL || strncmp (pre_tk, "struct ", 7) != 0)) {
		    if (Find_typedef (tk, buf, 512)) {
			int l = strlen (tk);
			*cpt = c;
			Replace_chars (tk, l, buf, buf_size - (tk - ret_buf));
			changed = 1;
			break;
		    }
		}
		*cpt = c;
		pre_tk = tk;
		tk = NULL;
	    }
	    else if (tk == NULL)
		tk = cpt;
	    if (c == '\0')
		break;
	    cpt++;
	}
    }
    return (1);
}

/**************************************************************************

    Replaces first "len" chars in string "str" by string "in_str". The
    buffer size of "str" is "size".

**************************************************************************/

static void Replace_chars (char *str, int len, char *in_str, int size) {
    int in_l, l;

    in_l = strlen (in_str);
    l = strlen (str + len);
    if (in_l + l >= size)
	SMIP_error (-1, "string buffer overflow\n");
    memmove (str + in_l, str + len, l + 1);
    memcpy (str, in_str, in_l);
}

/**************************************************************************

    Find the type definition of typedef "tdef" and returns in 
    "ret_buf". Returns 1 if found or zero if not-found.

**************************************************************************/

static int Find_typedef (char *tdef, char *ret_buf, int buf_size) {
    int i;

    for (i = 0; i < N_tds; i++) {
	if (strcmp (Text + Tks[Tds[i].right], tdef) == 0) {	/* found */
	    if (Find_type (Tds[i].left, 
		Tds[i].right - Tds[i].left, ret_buf, buf_size) &&
		strcmp (ret_buf, "struct") != 0)
		return (1);
	}
    }
    return (0);
}

/**************************************************************************

    Find the type definition of text started with token index "st_tkind" 
    and of length of "n_tks" tokens. Return 0 if not a valid type 1 
    otherwise.

**************************************************************************/

static int Find_type (int st_tkind, int n_tks, char *ret_buf, int buf_size) {
    char *cpt;
    int stind, endind, level, start_pt, len;

    /* check buffer size */
    stind = st_tkind;
    endind = st_tkind + n_tks;
    level = 0;
    len = 0;
    while (stind < endind) {
	cpt = Text + Tks[stind];
	if (*cpt == '{')
	    level++;
	else if (*cpt == '}')
	    level--;
	else if (level == 0)
	    len += strlen (cpt) + 1;
	stind++;
    }
    if (len + 4 >= buf_size)
	SMIP_error (st_tkind, "Type string too long\n");

    stind = st_tkind;
    endind = st_tkind + n_tks;
    start_pt = 0;
    len = 0;
    /* first token must be an identifier */
    cpt = Text + Tks[stind];
    if (!SMIP_is_alpha (*cpt))
	return (0);
    sprintf (ret_buf + len, "%s ", cpt);
    len += strlen (ret_buf + len);
    stind++;

    /* if the first token is struct, the second must be an identifier */
/*
    if (strcmp (cpt, "struct") == 0) {
	cpt = Text + Tks[stind];
	if (n_tks < 2 || !SMIP_is_alpha (*cpt))
	    return (0);
	sprintf (ret_buf + len, "%s ", cpt);
	len += strlen (ret_buf + len);
	stind++;
	start_pt = 1;
    }
*/

    /* append other tokens */
    level = 0;
    while (stind < endind) {
	cpt = Text + Tks[stind];
	if (*cpt == '{')
	    level++;
	else if (*cpt == '}')
	    level--;
	else if (level == 0) {
	    if (*cpt == '(') {		/* function pointer */
		if (stind + 1 >= endind || *(Text + Tks[stind + 1]) != '*')
		    return (0);
		strcpy (ret_buf + len, "( * ) ");
		len += strlen (ret_buf + len);
					/* we don't do more check - left for */
		break;			/* the compiler and do not take args */
	    }
	    if (SMIP_is_alpha (*cpt)) {
		if (start_pt)
		    return (0);
		sprintf (ret_buf + len, "%s ", cpt);
		len += strlen (ret_buf + len);
	    }
	    if (*cpt == '*') {
		if (!start_pt)
		    start_pt = 1;
		strcpy (ret_buf + len, "* ");
		len += strlen (ret_buf + len);
	    }
	}
	stind++;
    }

    if (len > 0 && ret_buf[len - 1] == ' ')
	ret_buf[len - 1] = '\0';

    return (1);
}

/**************************************************************************

    Malloc a segment of size "size" and keep the pointer to be freed later. 

**************************************************************************/

static char *Local_malloc (int size, Stored_t *sp) {
    Stored_p_t *p;

    p = (Stored_p_t *)SMIM_malloc (sizeof (Stored_p_t) + size);
    p->next = NULL;
    if (sp->sp == NULL) {
	sp->sp = p;
	sp->last_sp = p;
    }
    else {
	sp->last_sp->next = p;
	sp->last_sp = p;
    }
    return ((char *)p + sizeof (Stored_p_t));
}

/**************************************************************************

    Malloc a segment of size "size" and keep the pointer to be freed later. 

**************************************************************************/

static void Local_free (Stored_t *sp) {
    Stored_p_t *p;

    p = sp->sp;
    while (p != NULL) {
	Stored_p_t *next = p->next;
	free (p);
	p = next;
    }
    sp->sp = NULL;
}

/**************************************************************************

    Frees tables and resources used by this module. 

**************************************************************************/

void SMIT_free () {

    if (Tds != NULL)
	free (Tds);
    Tds = NULL;
    Td_bufsize = N_tds = 0;
}

