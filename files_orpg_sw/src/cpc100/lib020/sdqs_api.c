
/******************************************************************

	This implements the SDQ interface.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/02/05 15:47:14 $
 * $Id: sdqs_api.c,v 1.11 2008/02/05 15:47:14 jing Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */  

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <dlfcn.h>

#include <infr.h> 
#include <sdqm_def.h>
#include <sdqs_def.h>

#define LOCAL_NAME_SIZE 128

#define MAX_SDQS_SERVERS 32

typedef struct {		/* table meta info (per LB) */
    char *name;			/* table name */
    int n_qfs;			/* number of query fields */
    char *qf_names;		/* query field names (null terminated) list */
    char *qf_types;		/* query field types (null terminated) list */
    char *qf_foff;		/* query field offsets (null terminated) */
    char *lb_name;		/* LB name for this table */
    int rec_size;		/* record size */
    void (*record_byte_swap)();	/* record byte swap func */
} sdb_table_info_t ;		/* per LB */

typedef struct {		/* for select expression */
    char type;			/* [a-z] for operand; O for OR; A for AND; 
				   N for NOT; '\0' for array termination; '(';
				   ')'; */
    char not;			/* NOT operator on this field is needed */
    char operation;		/* OPE_* (OPE_EQUAL ...) */
    char processed;		/* this operand has been processed */
    char *field;		/* field name */
    char *value;		/* ASCII value */
} expr_item_t;

enum {OPE_EQUAL, OPE_NOT_EQUAL, OPE_LESS_EQUAL, OPE_GREAT_EQUAL, 
	OPE_LESS_THAN, OPE_GREAT_THAN};	/* used by expr_item_t.operation */

typedef struct {		/* table of values to be excluded */
    short foff;			/* field offset */
    char type;			/* type of field */
    char section;		/* in which section */
    union {
	short svalue;		/* short values to be excluded */
	int ivalue;		/* int value to be excluded */
	float fvalue;		/* float value to be excluded */
	char *value;		/* string value to be excluded */
    } v;
} exclusive_value_t;

enum {EXCV_INT, EXCV_SHORT, EXCV_FLOAT, EXCV_STRING};
				/* for exclusive_value_t.type */

#define MAX_N_EXCVS	128	/* buffer size for Excvs */
static exclusive_value_t *Excvs;/* array of values to be excluded */
static int N_excvs;		/* size of Excvs array */
static int Cr_excv_foff;	/* field offset of the current excl value */
static int Cr_excv_sind;	/* section index of the current excl value */
static int Cr_excv_type;	/* section type of the current excl value */

static sdb_table_info_t *Lbns = NULL;	/* LB list */
static int N_lbns = 0;		/* number of items in Lbns */
static void *Lbns_tblid = NULL;	/* Lbns table id */
static int Query_index = 0;

static void Convert_chars (int cnt, char *str, char c1, char c2);
static char *Get_unsigned_type (char *type, int *uns_p);
static int Get_next_tokens (int max_n, char *st, char **tk, int *tk_len);
static int Is_alpha (char c);
static int Is_separator (char c);
static int Cmp_name (void *e1, void *e2);
static int Get_sdb_info (char *call_name, sdb_table_info_t **info_p);
static int Process_expression (expr_item_t *items, int buf_size);
static int Parse_where_spec (expr_item_t *exprs, int expr_buf_size, 
			int n_tks, char **tks, int *tk_len, char *tk_buffer);
static int Process_section (sdb_table_info_t *info, expr_item_t *exprs, 
				int sec_st, int sec_end, int total);
static int Get_int_min_max (expr_item_t *exprs, int st, int end, 
					int *min, int *max);
static int Get_float_min_max (expr_item_t *exprs, int st, int end, 
					float *min, float *max);
static int Get_string_min_max (expr_item_t *exprs, int st, int end, 
				char *min, char *max, int buf_size);
static int Save_exclusive_value (char *value);
static int Set_up_low (int operation, 
			char *up, char *low, char *v, int size);
static int Int_bswap (int x);
static void Byte_swap_result_msg (SDQM_query_results *r, 
					sdb_table_info_t *info);


/******************************************************************

    The internal version of SDQS_select. It accepts the DB name and 
    returns the index result. When index_query is true, 
    SDQM_index_results is returned (more efficient).

******************************************************************/

int SDQS_select_i (char *db_name, int mode, int max_n, int index_query,
					char *where, void **result) {
    int ret;
    Sdqm_db_t *db;

    db = SDQM_get_db_info (db_name, NULL);
    if (db == NULL)
	return (SDQM_DB_NAME_NOT_FOUND);
    Query_index = index_query;
    ret = SDQS_select (db->lb_name, mode, max_n, MISC_i_am_bigendian (),
					where, result);
    Query_index = 0;
    return (ret);
}

/******************************************************************

    Executes data query on LB "lb_name". The select expression is
    in "where". It returns number of records found on success or
    a negative error code.

******************************************************************/

#define MAX_N_TKS 128			/* max number of tokens in "where" */
#define EXPR_SIZE 256			/* max size for select expressin */

int SDQS_select (char *lb_name, int mode, int max_n, int is_big_endian,
					char *where, void **result) {
    int tk_len[MAX_N_TKS];		/* token lenth */
    char *tks[MAX_N_TKS];		/* pointer to tokens */
    int n_tks;				/* number of tokens */
    exclusive_value_t excv_buffer[MAX_N_EXCVS];
    int lbns_ind, ret, i;
    static char *tk_buffer = 0;
    sdb_table_info_t *info;
    expr_item_t exprs[EXPR_SIZE];
    int n_exprs;
    int sec_st, sec_end, total;

    if ((lbns_ind = Get_sdb_info (lb_name, &info)) < 0)
	return (lbns_ind);

    if ((n_tks = Get_next_tokens (128, where, tks, tk_len)) < 0)
	return (n_tks);
    if (n_tks == 128)
	return (SDQM_TEXT_TOO_LONG);

    if (tk_buffer != NULL)
	free (tk_buffer);
    tk_buffer = malloc (strlen (where) + n_tks);
    if (tk_buffer == NULL)
	return (SDQM_MALLOC_FAILED);

    n_exprs = Parse_where_spec (exprs, EXPR_SIZE, 
				n_tks, tks, tk_len, tk_buffer);
    if (n_exprs < 0)
	return (n_exprs);

    if ((n_exprs = Process_expression (exprs, EXPR_SIZE)) < 0)
	return (SDQM_SYNTAX_ERROR);		/* could be buffer overflow */

    for (i = 0; i < n_exprs; i++) {		/* process NOT */
	expr_item_t *e;
	e = exprs + i;
	e->processed = 0;
	if (e->not) {
	    if (e->operation == OPE_EQUAL)
		e->operation = OPE_NOT_EQUAL;
	    else if (e->operation == OPE_NOT_EQUAL)
		e->operation = OPE_EQUAL;
	    else if (e->operation == OPE_LESS_EQUAL)
		e->operation = OPE_GREAT_THAN;
	    else if (e->operation == OPE_GREAT_EQUAL)
		e->operation = OPE_LESS_THAN;
	    else if (e->operation == OPE_LESS_THAN)
		e->operation = OPE_GREAT_EQUAL;
	    else if (e->operation == OPE_GREAT_THAN)
		e->operation = OPE_LESS_EQUAL;
	    e->not = 0;
	}
    }

    SDQM_query_begin (info->name);
    Excvs = excv_buffer;	/* the two variables are used for */
    N_excvs = 0;		/* passing the table to sub-functions */

    total = 0;
    sec_st = -1;
    sec_end = 0;
    Cr_excv_sind = 0;
    while (1) {				/* for each query section */

	for (i = sec_end; i <= n_exprs; i++) {
	    if (exprs[i].type == '\0' || exprs[i].type ==  'O') {
		sec_st = sec_end;
		sec_end = i;
		break;
	    }
	}
	if (sec_st < 0 || sec_st == sec_end)	/* no more sections */
	    break;

	ret = Process_section (info, exprs, sec_st, sec_end, total);
	if (ret < 0)
	    return (ret);
	if (ret > 0)
	    Cr_excv_sind++;

	total += ret;
	if (exprs[sec_end].type ==  'O')
	    sec_end++;			/* skip the OR */
    }

    if (total == 0 && n_tks > 0)	/* nothing to be done */
	return (0);

    if (mode != 0)
	SDQM_set_query_mode (mode);
    if (max_n <= 0)
	max_n = 0x7fffffff;
    if (Query_index)
	ret = SDQM_execute_query_index (max_n + N_excvs,
				(SDQM_index_results **)result);
    else {
	ret = SDQM_execute_query_local (max_n + N_excvs, 0, 
				(SDQM_query_results **)result);
	if (ret >= 0 && is_big_endian != MISC_i_am_bigendian ())
	    Byte_swap_result_msg (*result, info);
    }
    return (ret);
}

/**************************************************************************

    Checks if "rec" of section "sec" is an exclusive record. Returns 1
    if it is or 0 otherwise.

**************************************************************************/

int SDQSA_is_exclusive (char *rec, int sec) {
    static int st_ind = 0;
    int k;

    if (N_excvs == 0)
	return (0);
    if (st_ind >= N_excvs || sec < Excvs[st_ind].section)
	st_ind = 0;

    for (k = st_ind; k < N_excvs; k++) {
	char *field;
	exclusive_value_t *e;

	e = Excvs + k;
	if (e->section > sec)
	    break;
	if (e->section < sec) {
	    st_ind = k + 1;
	    continue;
	}

	field = rec + e->foff;
	switch (e->type) {
	    case EXCV_INT:
	    if (*((int *)field) == e->v.ivalue)
		return (1);
	    break;
	    case EXCV_SHORT:
	    if (*((short *)field) == e->v.svalue)
		return (1);
	    break;
	    case EXCV_FLOAT:
	    if (*((float *)field) == e->v.fvalue)
		return (1);
	    break;
	    case EXCV_STRING:
	    if (strcmp (*((char **)field), e->v.value) == 0)
		return (1);
	    break;
	}
    }
    return (0);
}

/******************************************************************

    Description: This function performs byte swap of a query result 
		message. It accepts the message in local indian 
		format and converts it to the different byte order 
		format. We don't check message error because the 
		message is assumed to be generated internally and 
		thus correct.

    Input:	r - pointer to the result message.

******************************************************************/

static void Byte_swap_result_msg (SDQM_query_results *r, 
					sdb_table_info_t *info) {
    char *rec;
    int *vf;
    int i;

    /* process records */
    rec = (char *)((char *)r + r->recs_off);
    for (i = 0; i < r->n_recs; i++) {
	if (info->record_byte_swap != NULL)
	    (info->record_byte_swap) (rec + sizeof (LB_id_t));
	vf = (int *)rec;		/* swap the added msg_id */
	*vf = Int_bswap (*vf);
	rec += info->rec_size;
    }

    /* process variable field spec */
    vf = (int *)((char *)r + r->vf_spec_off);
    for (i = 0; i < r->n_vf; i++) {
	*vf = Int_bswap (*vf);
	vf++;
    }

    r->msg_size = Int_bswap (r->msg_size);
    r->rec_size = Int_bswap (r->rec_size);
    r->n_recs_found = Int_bswap (r->n_recs_found);
    r->n_recs = Int_bswap (r->n_recs);
    r->recs_off = Int_bswap (r->recs_off);
    r->vf_spec_off = Int_bswap (r->vf_spec_off);
    r->db_info_off = Int_bswap (r->db_info_off);
}

/******************************************************************

    Description: Integer byte swapping function.

    Input:	x - the integer to be byte swapped.

    Return:	byte swapped x.

******************************************************************/

static int Int_bswap (int x) {
    return (INT_BSWAP (x));
}

/**************************************************************************

    Processes a query section stored in "exprs" started with "sec_st" and
    ended with "sec_end - 1". "info" provides the sdb meta info.
    Returns the number of query fields in the section or a negative 
    error code. "total" is the total SDQM_select_*_range calls so far.
    If "total" is non-zero, we need to call SDQM_query_next before calling 
    the first SDQM_select_*_range.

**************************************************************************/

#define MAX_STRING_SIZE 256

static int Process_section (sdb_table_info_t *info, expr_item_t *exprs, 
				int sec_st, int sec_end, int total) {
    int cnt, i, ret;

    cnt = 0;
    for (i = sec_st; i < sec_end; i++) {	/* for each item in section */
	int uns, find;
	char *fname, *n, *t, *f, *t1;
	expr_item_t *e;

	e = exprs + i;
	if (e->type == 'A')
	    continue;
	if (e->type < 'a' || e->type > 'z')
	    return (SDQM_RELATION_NOT_SUPPORTED);
	fname = e->field;
	if (e->processed)		/* already processed */
	    continue;
	n = info->qf_names;
	t = info->qf_types;
	f = info->qf_foff;
	for (find = 0; find < info->n_qfs; find++) {	/* field index */
	    if (strcmp (n, fname) == 0)
		break;
	    n += strlen (n) + 1;
	    t += strlen (t) + 1;
	    f += strlen (f) + 1;
	}
	if (find >= info->n_qfs)
	    return (SDQM_FIELD_NOT_FOUND);
	t1 = Get_unsigned_type (t, &uns);	/* with unsigned removed */

	sscanf (f, "%d", &Cr_excv_foff);
	if (strcmp (t1, "int") == 0 ||
	    strcmp (t1, "long") == 0) {
	    int min, max;
	    Cr_excv_type = EXCV_INT;
	    ret = Get_int_min_max (exprs, i, sec_end, &min, &max);
	    if (ret < 0)
		return (ret);
	    if (max < min)
		return (0);
	    if (cnt == 0 && total)
		SDQM_query_next ();
	    SDQM_select_int_range (find, min, max);
	}

	else if (strcmp (t1, "short") == 0) {
	    int min, max;
	    short smin, smax;
	    Cr_excv_type = EXCV_SHORT;
	    ret = Get_int_min_max (exprs, i, sec_end, &min, &max);
	    if (ret < 0)
		return (ret);
	    if (max < min)
		return (0);
	    smin = 0x8000;
	    smax = 0x7fff;
	    if (min > (int)smin)
		smin = min;
	    if (max < (int)smax)
		smax = max;
	    if (cnt == 0 && total)
		SDQM_query_next ();
	    SDQM_select_short_range (find, smin, smax);
	}

	else if (strcmp (t1, "float") == 0) {
	    float min, max;
	    Cr_excv_type = EXCV_FLOAT;
	    ret = Get_float_min_max (exprs, i, sec_end, &min, &max);
	    if (ret < 0)
		return (ret);
	    if (max < min)
		return (0);
	    if (cnt == 0 && total)
		SDQM_query_next ();
	    SDQM_select_float_range (find, min, max);
	}

	else if (strcmp (t1, "char *") == 0) {
	    char min[MAX_STRING_SIZE], max[MAX_STRING_SIZE];
	    Cr_excv_type = EXCV_STRING;
	    ret = Get_string_min_max (exprs, i, sec_end, 
					min, max, MAX_STRING_SIZE);
	    if (ret < 0)
		return (ret);
	    if (strcmp (max, min) < 0)
		return (0);
	    if (cnt == 0 && total)
		SDQM_query_next ();
	    SDQM_select_string_range (find, min, max);
	}
	else
	    return (SDQM_BAD_FIELD_TYPE);
	cnt++;
    }
    return (cnt);
}

/**************************************************************************

    Calculates the min and max values for field exprs[st]. It checks
    all items that involves in the same field in the section. It also
    compiles the list of exclusive values. Returns 0 on success or a 
    negative error code on failure.

**************************************************************************/

#define TMP_MAX_EXCVS 128	/* array size of tmp exclusive values */

static int Get_int_min_max (expr_item_t *exprs, int st, int end, 
					int *min, int *max) {
    char *fname, *texcp[TMP_MAX_EXCVS];
    int k, v, absmin, absmax, up, low, texcv[TMP_MAX_EXCVS], texccnt;
    char c;

    fname = exprs[st].field;
    absmin = 0x80000000;
    absmax = 0x7fffffff;
    *max = absmax;
    *min = absmin;
    texccnt = 0;
    for (k = st; k < end; k++) {
	expr_item_t *e;

	e = exprs + k;
	if (e->type < 'a' || e->type > 'z')
	    continue;
	if (k > st && strcmp (fname, e->field) != 0)
	    continue;

	if (sscanf (e->value, "%d%c", &v, &c) != 1)
	    return (SDQM_BAD_FIELD_VALUE);
	texcp[texccnt] = e->value;
	up = absmax;
	low = absmin;
	if (Set_up_low (e->operation, 
		(char *)&up, (char *)&low, (char *)&v, sizeof (int)) > 0)
	    texcv[texccnt++] = v;
	if (up < *max)
	    *max = up;
	if (low > *min)
	    *min = low;

	if (k > st)	/* mark it as processed */
	    e->processed = 1;
    }
    for (k = 0; k < texccnt; k++) {
	if (texcv[k] >= *min && texcv[k] <= *max &&
	    Save_exclusive_value (texcp[k]) < 0)
	    return (SDQM_BUFFER_OVERFLOW);
    }
    return (0);
}

/**************************************************************************

    Saves an exclusive value "value". Returns 0 on success 
    or -1 on failure.

**************************************************************************/

static int Save_exclusive_value (char *value) {

    if (N_excvs >= MAX_N_EXCVS)
	return (-1);
    Excvs[N_excvs].foff = Cr_excv_foff;
    Excvs[N_excvs].section = Cr_excv_sind;
    Excvs[N_excvs].type = Cr_excv_type;
    switch (Cr_excv_type) {
	case EXCV_INT:
	sscanf (value, "%d", &(Excvs[N_excvs].v.ivalue));
	break;
	case EXCV_SHORT:
	sscanf (value, "%hd", &(Excvs[N_excvs].v.svalue));
	break;
	case EXCV_FLOAT:
	sscanf (value, "%f", &(Excvs[N_excvs].v.fvalue));
	break;
	case EXCV_STRING:
	Excvs[N_excvs].v.value = value;
	break;
    }
    N_excvs++;
    return (0);
}

/**************************************************************************

    Sets the upper and lower bounds of a value "v" with "operator". The
    bounds are returned with "up" and "low". "size" is the size of the
    objects pointed by "up", "low" and "v". Returns 1 if the value is
    to be excluded or 0 otherwise.

**************************************************************************/

static int Set_up_low (int operation, 
			char *up, char *low, char *v, int size) {

    if (operation == OPE_EQUAL) {
	memcpy (up, v, size);
	memcpy (low, v, size);
    }
    else if (operation == OPE_NOT_EQUAL)
	return (1);
    else if (operation == OPE_LESS_EQUAL)
	memcpy (up, v, size);
    else if (operation == OPE_GREAT_EQUAL)
	memcpy (low, v, size);
    else if (operation == OPE_LESS_THAN) {
	memcpy (up, v, size);
	return (1);
    }
    else if (operation == OPE_GREAT_THAN) {
	memcpy (low, v, size);
	return (1);
    }
    return (0);
}

/**************************************************************************

    The float version of Get_int_min_max.

**************************************************************************/

static int Get_float_min_max (expr_item_t *exprs, int st, int end, 
					float *min, float *max) {
    char *fname, *texcp[TMP_MAX_EXCVS];
    int k, texccnt;
    float v, absmin, absmax, up, low, texcv[TMP_MAX_EXCVS];
    char c;

    fname = exprs[st].field;
    absmin = 1.175494351E-38F;
    absmax = 3.402823466E+38F;
    *max = absmax;
    *min = absmin;
    texccnt = 0;
    for (k = st; k < end; k++) {
	expr_item_t *e;

	e = exprs + k;
	if (e->type < 'a' || e->type > 'z')
	    continue;
	if (k > st && strcmp (fname, e->field) != 0)
	    continue;

	if (sscanf (e->value, "%f%c", &v, &c) != 1)
	    return (SDQM_BAD_FIELD_VALUE);
	texcp[texccnt] = e->value;
	up = absmax;
	low = absmin;
	if (Set_up_low (e->operation, 
		(char *)&up, (char *)&low, (char *)&v, sizeof (float)) > 0)
	    texcv[texccnt++] = v;
	if (up < *max)
	    *max = up;
	if (low > *min)
	    *min = low;

	if (k > st)	/* mark it as processed */
	    e->processed = 1;
    }
    for (k = 0; k < texccnt; k++) {
	if (texcv[k] >= *min && texcv[k] <= *max &&
	    Save_exclusive_value (texcp[k]) < 0)
	    return (SDQM_BUFFER_OVERFLOW);
    }
    return (0);
}

/**************************************************************************

    The string version of Get_int_min_max.

**************************************************************************/

static int Get_string_min_max (expr_item_t *exprs, int st, int end, 
				char *min, char *max, int buf_size) {
    char *fname;
    int k, texccnt;
    char *v, *absmin, *absmax, *up, *low;
    char *texcv[TMP_MAX_EXCVS];

    fname = exprs[st].field;
    absmin = "\1";
    absmax = "\377";
    strcpy (max, absmax);
    strcpy (min, absmin);
    texccnt = 0;
    for (k = st; k < end; k++) {
	expr_item_t *e;

	e = exprs + k;
	if (e->type < 'a' || e->type > 'z')
	    continue;
	if (k > st && strcmp (fname, e->field) != 0)
	    continue;

	v = e->value;
	if (strlen (v) + 1 >= buf_size)
	    return (SDQM_BUFFER_OVERFLOW);
	up = absmax;
	low = absmin;
	if (Set_up_low (e->operation, 
		(char *)&up, (char *)&low, (char *)&v, sizeof (char *)) > 0)
	    texcv[texccnt++] = v;
	if (strcmp (up, max) < 0)
	    strcpy (max, up);
	if (strcmp (low, min) > 0)
	    strcpy (min, low);

	if (k > st)	/* mark it as processed */
	    e->processed = 1;
    }
    for (k = 0; k < texccnt; k++) {
	if (strcmp (texcv[k], min) >= 0 && strcmp (texcv[k], max) <= 0 &&
	    Save_exclusive_value (texcv[k]) < 0)
	    return (SDQM_BUFFER_OVERFLOW);
    }
    return (0);
}

/******************************************************************

    Parses the tokens of the "where" statement and creates the 
    expression array and returns it with "exprs". The buffer size
    of "exprs" is "expr_buf_size". The array is terminated by
    ->type = '\0' for ready to call Process_expression. field ->not
    is initialized. "tk_buffer" is used for strings in "exprs".
    The size of "exprs" is guaranteed. Returns the size of "exprs"
    on success or a negative error code.

******************************************************************/

static int Parse_where_spec (expr_item_t *exprs, int expr_buf_size, 
			int n_tks, char **tks, int *tk_len, char *tk_buffer) {
    int cnt, op_part, i;
    char *tkp;

    tkp = tk_buffer;
    cnt = 0;
    op_part = 0;
    for (i = 0; i < n_tks; i++) {
	int st_cnt;

	st_cnt = cnt;			/* used later for checking */
	if (cnt >= expr_buf_size)
	    return (SDQM_TEXT_TOO_LONG);	/* buffer overflow */
	exprs[cnt].not = 0;
	strncpy (tkp, tks[i], tk_len[i]);
	tkp[tk_len[i]] = '\0';
	if (strcmp (tkp, "and") == 0)
	    exprs[cnt++].type = 'A';
	else if (strcmp (tkp, "or") == 0)
	    exprs[cnt++].type = 'O';
	else if (strcmp (tkp, "not") == 0)
	    exprs[cnt++].type = 'N';
	else if (strcmp (tkp, "(") == 0)
	    exprs[cnt++].type = '(';
	else if (strcmp (tkp, ")") == 0)
	    exprs[cnt++].type = ')';
	else {				/* part of the current operand */
	    exprs[cnt].type = 'a';	/* anything a - z */
	    if (op_part == 0)
		exprs[cnt].field = tkp;
	    else if (op_part == 1) {
		char next_char;

		if (i + 1 < n_tks && tk_len[i + 1] == 1)
		    next_char = tks[i + 1][0];
		else
		    next_char = '\0';
		    
		if (strcmp (tkp, "=") == 0)
		    exprs[cnt].operation = OPE_EQUAL;
		else if (strcmp (tkp, "<") == 0) {
		    if (next_char == '>') {
			exprs[cnt].operation = OPE_NOT_EQUAL;
			i++;
		    }
		    else if (next_char == '=') {
			exprs[cnt].operation = OPE_LESS_EQUAL;
			i++;
		    }
		    else
			exprs[cnt].operation = OPE_LESS_THAN;
		}
		else if (strcmp (tkp, ">") == 0) {
		    if (next_char == '=') {
			exprs[cnt].operation = OPE_GREAT_EQUAL;
			i++;
		    }
		    else
			exprs[cnt].operation = OPE_GREAT_THAN;
		}
		else if (strcmp (tkp, "!") == 0) {
		    if (next_char == '=') {
			exprs[cnt].operation = OPE_NOT_EQUAL;
			i++;
		    }
		    else
			return (SDQM_PARSE_ERROR);
		}
		else 
		    return (SDQM_PARSE_ERROR);		/* bad operator */
	    }
	    else if (op_part == 2)
		exprs[cnt].value = tkp;
	    op_part++;
	    if (op_part >= 3) {		/* operand completed */
		op_part = 0;
		cnt++;
	    }
	}
	if (op_part > 0 && cnt != st_cnt)
	    return (SDQM_PARSE_ERROR);		/* incomplete operand spec */
	tkp += strlen (tkp) + 1;
    }
    if (op_part > 0)
	return (SDQM_PARSE_ERROR);		/* incomplete operand spec */

    if (cnt >= expr_buf_size)
	return (SDQM_TEXT_TOO_LONG);		/* buffer overflow */
    exprs[cnt].type = '\0';		/* terminate array exprs */

    return (cnt);
}

/******************************************************************

    Returns the SDB info in "info_p" for LB "call_name". 
    If it does not exist, it executes a query to get the table 
    info and stores it. This function assumes the query result is
    correct. Otherwise the function may crash.

    Returns the LB call_name table index of "info_p" on success 
    or a negative error number.

******************************************************************/

static int Get_sdb_info (char *lb_name, sdb_table_info_t **info_p) {
    sdb_table_info_t ent;
    sdb_table_info_t *sdb;
    int off, ind, i, lbns_ind;
    char *names, *types, *sdb_name, *src_t, *tp;
    char *dll_name, *data_endian;
    Sdqm_db_t *db_info;

    if (Lbns_tblid == NULL) {		/* initialize the SDB info table */
	if ((Lbns_tblid = MISC_open_table (sizeof (sdb_table_info_t), 
				8, 1, &N_lbns, (char **)&Lbns)) == NULL)
	    return (SDQM_MALLOC_FAILED);
    }

    ind = -1;
    ent.lb_name = lb_name;		/* search existing call names */
    if (MISC_table_search (Lbns_tblid, &ent, Cmp_name, &i)) {
	*info_p = Lbns + i;
	return (i);
    }

    /* A new lb - get the SDB info */
    db_info = SDQM_get_db_info (lb_name, &sdb_name);
    if (db_info == NULL)
	return (SDQM_DB_NAME_NOT_FOUND);

    names = types = src_t = tp = dll_name = NULL;
    src_t = db_info->text;	/* See Init_sdb_table () in sdqs_operate.c */
    names = src_t + strlen (src_t) + 1;
    types = names + strlen (names) + 1;
    dll_name = types + strlen (types) + 1;
    data_endian = dll_name + strlen (dll_name) + 1;

    tp = NULL;
    tp = malloc (strlen (sdb_name) + strlen (names) +
		strlen (types) + db_info->n_qfs * 8 + strlen (lb_name) + 
		strlen (src_t) + strlen (dll_name) + 7);
    if (tp == NULL)
	return (SDQM_MALLOC_FAILED);
    lbns_ind = 0;
    {
	sdb_table_info_t tsdb;
	tsdb.lb_name = tp;
	strcpy (tp, lb_name);
	if ((lbns_ind = MISC_table_insert (Lbns_tblid, &tsdb, Cmp_name)) < 0) {
	    if (tp != NULL)
		free (tp);
	    return (SDQM_MALLOC_FAILED);
	}
    }

    sdb = Lbns + lbns_ind;
    off = strlen (lb_name) + 1;
    sdb->name = tp + off;
    strcpy (sdb->name, sdb_name);
    off += strlen (sdb_name) + 1;
    sdb->n_qfs = db_info->n_qfs;
    sdb->qf_names = tp + off;
    strcpy (sdb->qf_names, names);
    off += strlen (names) + 1;
    sdb->qf_types = tp + off;
    strcpy (sdb->qf_types, types);
    off += strlen (types) + 1;
    sdb->qf_foff = tp + off;
    sdb->rec_size = db_info->rec_size;
    sdb->record_byte_swap = db_info->record_byte_swap;

    tp = sdb->qf_foff;
    for (i = 0; i < db_info->n_qfs; i++) {
	sprintf (tp, "%d:", db_info->qfs[i].foff);
	tp += strlen (tp);
    }
    tp[-1] = '\0';
    Convert_chars (sdb->n_qfs - 1, sdb->qf_names, ':', '\0');
    Convert_chars (sdb->n_qfs - 1, sdb->qf_types, ':', '\0');
    Convert_chars (sdb->n_qfs - 1, sdb->qf_foff, ':', '\0');

    *info_p = sdb;
    return (lbns_ind);
}

/************************************************************************

    Name comparison function for LB call name table insertion.

************************************************************************/

static int Cmp_name (void *e1, void *e2) {
    sdb_table_info_t *f1, *f2;
    f1 = (sdb_table_info_t *)e1;
    f2 = (sdb_table_info_t *)e2;
    return (strcmp (f1->lb_name, f2->lb_name));
}

/******************************************************************

    Converts the first "cnt" characters of "c1" in string "str" to 
    "c2".

******************************************************************/

static void Convert_chars (int cnt, char *str, char c1, char c2) {
    int i;
    char *p;

    p = str;
    i = 0;
    while (i < cnt) {
	if (*p == c1) {
	    *p = c2;
	    i++;
	}
	p++;
    }
}

/**************************************************************************

    Find outs "max_n" tokens starting at "st" which is a NULL terminated
    char string. The tokens are returned in "tk" and their sizes in 
    "tk_len". Characters within quotation (') are considered as a single
    token. The function returns the number of tokens found or a
    negative error code.

**************************************************************************/

static int Get_next_tokens (int max_n, char *st, char **tk, int *tk_len) {
    char *ip;
    int cnt, intkn, inquot;

    ip = st;
    cnt = 0;
    intkn = inquot = 0;
    while (1) {
	char c;

	c = *ip;
	if (c == '\'') {
	    if (inquot)
		inquot = 0;
	    else
		inquot = 1;
	}
	if (Is_alpha (c) || (inquot && c != '\0')) {
	    if (!intkn && c != '\'') {
		intkn = 1;
		tk[cnt] = ip;
	    }
	    ip++;
	    continue;
	}

	if (intkn) {
	    intkn = 0;
	    tk_len[cnt] = ip - tk[cnt];
	    cnt++;
	    if (cnt >= max_n)
		return (cnt);
	}

	if (Is_separator (c)) {
	    ip++;
	    continue;
	}

	/* other single char symbals */
	if (c == '\0')
	    break;
	if (c != '\'') {
	    tk[cnt] = ip;
	    tk_len[cnt] = 1;
	    cnt++;
	    if (cnt >= max_n)
		return (cnt);
	}
	ip++;
    }
    if (inquot)
	return (SDQM_OPEN_QUOTATION);
    return (cnt);
}

/**************************************************************************

    Returns non-zero if "c" is an alpha numerical character. Zero otherwise.

**************************************************************************/

static int Is_alpha (char c) {
    if ((c >= '#' && c <= '~') && c != '(' && c != ')' &&
	c != '<' && c != '=' && c != '>' && c != '\'')
	return (1);
    return (0);
}

/**************************************************************************

    Returns non-zero if "c" is a separation character. Zero otherwise.

**************************************************************************/

static int Is_separator (char c) {
    if (c == ' ' || c == '\n' || c == '\t')
	return (1);
    return (0);
}

/******************************************************************

    Removes the "unsigned" key word from "type" and returns the 
    pointer to the type word after "unsigned". "uns_p" is set to
    1 if the type is unsigned. It is set to 0 otherwise.

******************************************************************/

static char *Get_unsigned_type (char *type, int *uns_p) {

    if (strncmp (type, "unsigned ", 9) == 0) {
	*uns_p = 1;
	return (type + 9);
    }
    else {
	*uns_p = 0;
	return (type);
    }
}


/***************************************************************************

    The following is the implementation of Process_expression.

***************************************************************************/

#define TMP_BUF_SIZE 256	/* size of the tmp expression buffer */

static int Get_term_parenthesis (expr_item_t *items);
static int Get_next_operand (expr_item_t *items, int *first);
static int Get_next_term (expr_item_t *items, int *first);
static int Eliminate_trivial_parentheses (expr_item_t *items);
static int Replace_items (expr_item_t *items, int n_items, 
				int n_new_items, expr_item_t *new_items);
static int Process_not (expr_item_t *items, int ind);
static int Remove_parentheses (expr_item_t *items, int ind);

/***************************************************************************

    Processes expression "items". The expression is terminated by ->type =
    '\0'. It is processed in place. The buffer size
    is "buf_size". All NOT is eliminated and all parentheses are removed.
    Syntax errors are checked. It returns the size of the processed
    expression on success or -1 in case of syntax error or buffer overflow.

***************************************************************************/

static int Process_expression (expr_item_t *items, int buf_size) {
    int first, st, ret, size, ind;
    expr_item_t *it;

    it = items;		/* terminate the buffer with mark 'T' */
    while (it->type != '\0')
	it++;
    while (it < items + buf_size) {
	it->type = '\0';
	it++;
    }
    items[buf_size - 1].type = 'T';

    /* check syntax: parentheses are closed and no trailing items left over */
    st = 0;
    while (1) {
	ret = Get_next_term (items + st, &first);
	if (ret < 0)		/* syntax error */
	    return (-1);
	if (ret == 0)
	    break;
	st += ret + first;
    }
    if (items[st].type != '\0') /* syntax error */
	return (-1);

    while (1) {			/* eliminating all NOT */
	if (Eliminate_trivial_parentheses (items) < 0)
	    return (-1);

	ind = 0;
	while (items[ind].type != '\0' && items[ind].type != 'N')
	    ind++;
	if (items[ind].type == '\0')
	    break;		/* done */
	if (Process_not (items, ind) < 0)
	    return (-1);
    }

    while (1) {			/* removing all parentheses */
	if (Eliminate_trivial_parentheses (items) < 0)
	    return (-1);

	ind = 0;
	while (items[ind].type != '\0' && items[ind].type != '(')
	    ind++;
	if (items[ind].type == '\0')
	    break;		/* done */
	if (Remove_parentheses (items, ind) < 0)
	    return (-1);
    }

    size = 0;			/* find final size */
    while (items[size].type != '\0')
	size++;

    return (size);
}

/***************************************************************************

    Removes a pair of parentheses at position "ind". It is assumed to be
    the first in the expression. Returns 0 on success or -1 on syntax 
    error or buffer overflow.

***************************************************************************/

static int Remove_parentheses (expr_item_t *items, int ind) {
    int eind, st1, size1, st2, size2;
    expr_item_t buf[TMP_BUF_SIZE], *it;
    int tp, level, cnt;

    eind = Get_term_parenthesis (items + ind) - 1 + ind; /* where ')' is */

    /* find the start points and sizes of the parts in front and after */
    st1 = ind;
    while (st1 > 0 && items[st1 - 1].type != 'O')
	st1--;			/* no parentheses in front */
    size1 = ind - st1;
    level = 0;
    size2 = 0;
    it = items + eind + 1;
    while (it->type != '\0') {
	if (it->type == '(')
	   level++;
	else if (it->type == ')')
	   level--;
	else if (level == 0 && it->type == 'O')
	    break;
	size2++;
	it++;
    }
    st2 = eind + 1;

    cnt = 0;			/* where to add */
    tp = ind + 1;
    while (1) {
	int ts, tf, i;

	ts = Get_next_term (items + tp, &tf);
	if (ts < 0)
	    return (-1);
	if (ts == 0)
	    break;
	if (tp > ind + 1)
	    buf[cnt++].type = 'O';

	if (size1 + ts + size2 + 1 > TMP_BUF_SIZE)
	    return (-1);
	for (i = 0; i < size1; i++)
	    buf[cnt++] = items[st1 + i];
	for (i = 0; i < ts; i++)
	    buf[cnt++] = items[tp + tf + i];
	for (i = 0; i < size2; i++)
	    buf[cnt++] = items[st2 + i];

	tp += ts + tf;
    }
    if (Replace_items (items + ind - size1, 
	    eind + size2 - (ind - size1) + 1, cnt, buf) < 0)
	return (-1);

    return (0);
}

/***************************************************************************

    Processes a NOT at position "ind" of expression array "items". Returns
    0 on success or -1 on syntax error or buffer overflow.

***************************************************************************/

static int Process_not (expr_item_t *items, int ind) {
    expr_item_t buf[TMP_BUF_SIZE];
    int next_type;

    next_type = items[ind + 1].type;
    if (next_type == 'N') {	/* cancel the two */
	if (Replace_items (items + ind, 2, 0, NULL) < 0)
	    return (-1);
	return (0);
    }
    if (next_type >= 'a' && next_type <= 'z') {	/* an operand */
	if (items[ind + 1].not)
	    items[ind + 1].not = 0;
	else
	    items[ind + 1].not = 1;
	if (Replace_items (items + ind, 1, 0, NULL) < 0)
	    return (-1);
	return (0);
    }
    if (next_type == '(') {
	int tp, cnt;

	tp = ind + 2;			/* term pointer */
	cnt = 0;			/* where to add */
	buf[cnt++].type = '(';
	while (1) {
	    int ts, os, op, tf, of, i;

	    ts = Get_next_term (items + tp, &tf);
	    if (ts < 0)
		return (-1);
	    if (ts == 0)
		break;
	    if (tp > ind + 2)
		buf[cnt++].type = 'A';
	    buf[cnt++].type = '(';
	    op = tf + tp;			/* operand pointer */
	    while (1) {
		os = Get_next_operand (items + op, &of);
		if (os < 0)
		    return (-1);
		if (os == 0)
		    break;
		if (os + 8 > TMP_BUF_SIZE)
		    return (-1);
		if (op != tf + tp)		/* not the first one */
		    buf[cnt++].type = 'O';
		buf[cnt++].type = 'N';
		for (i = 0; i < os; i++)
		    buf[cnt++] = items[op + of + i];
		op += os + of;
	    }
	    buf[cnt++].type = ')';
	    tp += ts + tf;
	}
	buf[cnt++].type = ')';
	if (Replace_items (items + ind, 
		Get_term_parenthesis (items + ind + 1) + 1, cnt, buf) < 0)
	    return (-1);
    }
    return (0);
}

/***************************************************************************

    Finds the next term starting with "items" within the espression. items[0] 
    must be an operand, OR or NOT. NOT is treated as part of the following 
    operand. multiple NOT can be in front of an operand. The position of the
    first item of the next item is returned with "first". Returns the
    size (number of items) of the next item on success, 0 if no more term
    found or -1 on failure. The array of "items" is terminated by type = "\0".

***************************************************************************/

static int Get_next_term (expr_item_t *items, int *first) {
    int cnt, f, st;
    char type;

    type = items[0].type;
    if (type == 'O')
	f = 1;
    else if ((type >= 'a' && type <= 'z') || type == 'N' || type == '(' )
	f = 0;
    else if (type == '\0' || type == ')')
	return (0);
    else 
	return (-1);
    *first = f;

    st = f;
    cnt = 0;
    while (1) {
	int len, off;

	len = Get_next_operand (items + st, &off);
	if (len < 0)
	    return (-1);
	if (len == 0)
	    return (cnt);
	st += len + off;
	cnt += len + off;
    }

    return (-1);
}

/***************************************************************************

    Finds the next operand starting with "items" within the term. items[0] 
    must be an operand, AND or NOT. NOT is treated as part of the following 
    operand. multiple NOT can be in front of an operand. The position of the
    first item of the next operand is returned with "first". Returns the
    size (number of items) of the next operand on success, 0 if no more
    operand within the term or -1 on failure. The array of "items" is 
    terminated by type = "\0".

***************************************************************************/

static int Get_next_operand (expr_item_t *items, int *first) {
    int cnt, f;
    char type;

    type = items[0].type;
    if (type == 'A')
	f = 1;
    else if ((type >= 'a' && type <= 'z') || type == 'N' || type == '(')
	f = 0;
    else if (type == '\0' || type == 'O' || type == ')')
	return (0);
    else 
	return (-1);

    cnt = 0;
    while (items[f + cnt].type == 'N')
	cnt++;

    *first = f;
    type = items[f + cnt].type;
    if (type >= 'a' && type <= 'z')
	return (cnt + 1);
    if (type == '(') {
	int size;
	size = Get_term_parenthesis (items + f + cnt);
	if (size > 0)
	    return (cnt + size);
	return (-1);
    }

    return (-1);
}

/***************************************************************************

    Finds the position of the ending parenthesis that matches the first
    "(". The first item must be "(". Returns the size (number of items)
    in "(...)" including "(" and ")" or -1 on failure. The array of 
    "items" is terminated by type = "\0".

***************************************************************************/

static int Get_term_parenthesis (expr_item_t *items) {
    int level, i;

    level = 0;
    i = 0;
    while (items[i].type != 0) {
	if (items[i].type == '(')
	    level++;
	else if (items[i].type == ')') {
	    level--;
	    if (level == 0)
		return (i + 1);
	}
	i++;
    }
    return (-1);
}

/***************************************************************************

    Eliminates parentheses that 1) with one item; 2) all items in it; 3)
    double parentheses; 4) with no item. It also checks: 1) If there is any
    consecutive prerators or operands; 2) The item after '(' and before ')'
    can not be 'A' or 'O'; 3) The first or the last item is an operator. It
    returns -1 on any of these conditions. Otherwise 0 is retuned.

***************************************************************************/

static int Eliminate_trivial_parentheses (expr_item_t *items) {
    int done, i;

    do {
	char type, nexttype, c2;

	done = 1;
	i = 0;
	while (items[i + 1].type != '\0') {

	    type = items[i].type;
	    nexttype = items[i + 1].type;
	    if ((type >= 'a' && type == 'z' && 
				nexttype >= 'a' && nexttype <= 'z') ||
		((type == 'O' || type == 'A') &&
		 (nexttype == 'O' || nexttype == 'A')))
		return (-1);
	    
	    if (type == '(') {
		int t, t1;

		t = i + Get_term_parenthesis (items + i) - 1;
		c2 = items[t - 1].type;
		if (nexttype == 'A' || nexttype == 'O' ||
		    c2 == 'A' || c2 == 'O')
		    return (-1);

		if (t == i + 1 ||		/* no item */
		    t == i + 2 ||		/* single item */
		    (i == 0 && items[t + 1].type == '\0')) { /* all items */
		    if (Replace_items (items + t, 1, 0, NULL) < 0 ||
			Replace_items (items + i, 1, 0, NULL) < 0)
			return (-1);
		    done = 0;
		}
		else if (nexttype == '(') {	/* double */
		    t1 = i + 1 + Get_term_parenthesis (items + i + 1) - 1;
		    if (t == t1 + 1) {		/* double parentheses */
			if (Replace_items (items + t, 1, 0, NULL) < 0 ||
			    Replace_items (items + i, 1, 0, NULL) < 0)
			    return (-1);
			done = 0;
		    }
		}
	    }
	    i++;
	}
	type = items[0].type;
	c2 = items[i].type;
	if (type == 'A' || type == 'O' ||
	    c2 == 'A' || c2 == 'O')
	    return (-1);
	
    } while (!done);

    return (0);
}

/***************************************************************************

    Replaces "n_items" items started at "items" by "n_new_items" items 
    stored in "new_items". Peplacing is performed in place. The buffer 
    is assumed to be terminated by 'T'. Returns 0 on success or -1 in 
    case of buffer overflow.

***************************************************************************/

static int Replace_items (expr_item_t *items, int n_items, 
			int n_new_items, expr_item_t *new_items) {
    int i;

    if (n_new_items > n_items) {		/* verify buffer size */
	expr_item_t *it;
	it = items;
	while (it->type != '\0')
	    it++;
	it++;
	i = 0;
	while (i < (n_new_items - n_items)) {
	    if (it->type == 'T')
		return (-1);
	    i++;
	    it++;
	}
    }

    if (n_items > 0) {
	i = 0;
	while (items[n_items + i - 1].type != '\0') {
	    items[i] = items[n_items + i];
	    i++;
	}
    }
    if (n_new_items > 0) {
	int cnt = 0;
	while (items[cnt].type != '\0')
	    cnt++;
	for (i = cnt; i >= 0; i--)
	    items[i + n_new_items] = items[i];
	for (i = 0; i < n_new_items; i++)
	     items[i] = new_items[i];
    }
    return (0);
}


