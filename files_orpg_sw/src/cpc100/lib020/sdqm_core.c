
/******************************************************************

	file: sdqm_core.c

	This is part of the SDQM module - The simple data base 
	query management. This contains the core functions.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/02/04 20:26:36 $
 * $Id: sdqm_core.c,v 1.36 2008/02/04 20:26:36 jing Exp $
 * $Revision: 1.36 $
 * $State: Exp $
 */  

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <misc.h> 
#include <sdqm.h>
#include <sdqm_def.h>
#include <sdqs_def.h>

static Compare_field_spec *Cr_cfs;	/* current comparison field spec. This 
					   variable is used for passing this 
					   info from SDQM functions to 
					   Local_compare. */
static Sdqm_db_t *Cr_db;		/* current DB. Used for passing this
					   to Local_set_records and 
					   Local_compare. */

static void *Sdb_tid = NULL;		/* table id of SDB tables */
static Sdb_table_t *Sdbs;		/* SDB tables */
static int N_sdbs = 0;			/* number of SDBs */


static int Define_sdb (char *db_name, int db_type, 
		int max_records, int rec_size, 
		int n_qfs, SDQM_query_field_t *qfs, 
		int n_its, SDQM_query_index_t *its,
		void (*record_byte_swap)(),
		int (*compare_func)(), int (*set_rec_func)());
static int Local_compare (int which_it, void *rec1, void *rec2);
static int Local_range_compare (Compare_field_spec *cfs,
				void *rec, void *min, void *max);
static int Local_set_records (SDQM_query_int *psec, 
				char *min_rec, char *max_rec);
static void Init_record_list (Record_list_t *recl);
static void Free_record_list (Record_list_t *recl);
static void Free_query_info (Query_info *qinfo);
static int Recursive_query (int cr_level, Query_info *qinfo);
static int Process_unit_query (Query_info *qinfo);
static void Find_best_indexing (Query_info *qinfo);
static int Indexing_search (Query_info *qinfo);
static int Prepare_for_non_index_field_comparison  (Query_info *qinfo);
static int Process_single_query (char *query, Query_info *qinfo);
static int Check_alignment (int type, int foff);
static void Post_process_rec_list (Query_info *qinfo);

static int Lowend_search (Query_info *qinfo, Sdqm_db_t *db);
static int Highend_search (Query_info *qinfo, Sdqm_db_t *db);
static int Test_and_save_record (Query_info *qinfo, unsigned short ind);
static int Insert_in_record_list (Record_list_t *recl, unsigned short ind);
static int Process_empty_query (Query_info *qinfo);

#ifdef SDQM_TEST
#define INCLUDE_TEST_FUNCS
#include <sdqm_test.c>
#endif


/******************************************************************

    Description: This function intializes the data base struct and
		the RSIS for a DB of type SDQM_SIMPLE_QUERY_FIELDS.

    Inputs/Output:	See Define_sdb.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

int SDQM_define_sdb (char *db_name, int max_records, int rec_size, 
		int n_qfs, SDQM_query_field_t *qfs, 
		int n_its, SDQM_query_index_t *its,
		void (*record_byte_swap)())
{

     return (Define_sdb (db_name, SDQM_SIMPLE_QUERY_FIELDS, 
		max_records, rec_size, n_qfs, qfs, 
		n_its, its, record_byte_swap, NULL, NULL));
}

/******************************************************************

    Returns the DB struct for LB "lb_name". Returns NULL if not found.
    It db_name == NULL, lb_name is the DB name.

******************************************************************/

Sdqm_db_t *SDQM_get_db_info (char *lb_name, char **db_name) {
    int i;

    for (i = 0; i < N_sdbs; i++) {
	Sdqm_db_t *db;

	db = (Sdqm_db_t *)Sdbs[i].id;
	if (db_name == NULL) {
	    if (strcmp (Sdbs[i].name, lb_name) == 0)
		return (db);
	}
	else if (strcmp (db->lb_name, lb_name) == 0) {
	    *db_name = Sdbs[i].name;
	    return (db);
	}
    }
    return (NULL);
}

/******************************************************************

    Sets table meta data. "text" is a set of null terminated
    strings: src_t, qf_names, qf_types, dll_name and any new item.
    "text_len" is the size of the "text" which is not a simple
    null terminated string. The "text" info will be shipped to the
    sdq client without processing.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

int SDQM_set_sdb_meta (char *db_name,
			char *lb_name, int text_len, char *text) {
    Sdqm_db_t *db;
    int size, i;
    char *p;

    for (i = 0; i < N_sdbs; i++) {
	if (strcmp (db_name, Sdbs[i].name) == 0)
	    break;
    }
    if (i >= N_sdbs)
	return (SDQM_DB_NAME_NOT_FOUND);

    db = (Sdqm_db_t *)Sdbs[i].id;
    size = strlen (lb_name) + 4 + text_len;
    p = malloc (size);
    if (p == NULL)
	return (SDQM_MALLOC_FAILED);
    db->lb_name = p;
    strcpy (p, lb_name);
    p += strlen (lb_name) + 1;
    db->text = p;
    memcpy (p, text, text_len);
    p += text_len;
    db->text_len = text_len;

    return (0);
}

/******************************************************************

    Description: This function intializes a data base struct and
		the RSIS.

    Inputs:	db_name - name of the SDB.
		db_type - DB type.
		max_records - maximum number of records.
		rec_size - record size.
		n_qfs - number of query fields.
		qfs - spec of query fields.
		n_its - number of indexting tables.
		its - spec of indexting tables.
		compare_func - comparison function.
		set_rec_func - function sets record value in terms 
		of field values specified in a query message section.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Define_sdb (char *db_name, int db_type, 
		int max_records, int rec_size, 
		int n_qfs, SDQM_query_field_t *qfs,
		int n_its, SDQM_query_index_t *its,
		void (*record_byte_swap)(),
		int (*compare_func)(), int (*set_rec_func)())
{
    Sdqm_db_t *db;
    char *pqfs, *pits;
    int i;

    if (db_name == NULL || db_name[0] == '\0' || 
	strlen (db_name) >= SDQM_DB_NAME_SIZE ||
	max_records <= 0 || rec_size <= 0 ||
	n_qfs <= 0 || n_its < 0)
	return (SDQM_BAD_ARGUMENT);

    if (Sdb_tid == NULL) {
	Sdb_tid = MISC_open_table (
		sizeof (Sdb_table_t), 4, 0, &N_sdbs, (char **)&Sdbs);
	if (Sdb_tid == NULL)
	    return (SDQM_MALLOC_FAILED);
    }

    /* check db_name */
    for (i = 0; i < N_sdbs; i++) {
	if (strcmp (db_name, Sdbs[i].name) == 0)
	    return (SDQM_ALREADY_DEFINED);
    }

    db = (Sdqm_db_t *)malloc (sizeof (Sdqm_db_t));
    pqfs = malloc (n_qfs * sizeof (Sdqm_query_field_t));
    pits = malloc (n_its * sizeof (Sdqm_query_index_t));
    if (db == NULL || pqfs == NULL || pits == NULL)
	return (SDQM_MALLOC_FAILED);

    db->db_type = db_type;
    db->max_records = max_records;
    db->rec_size = rec_size;
    if (db_type == SDQM_CUSTOM_QUERY_FIELDS) {
	if (compare_func == NULL || set_rec_func == NULL)
	    return (SDQM_BAD_ARGUMENT);
	db->compare_func = compare_func;
	db->set_rec_func = set_rec_func;
    }
    else {
	db->compare_func = Local_compare;
	db->set_rec_func = Local_set_records;
    }

    db->lb_name = "";
    db->text_len = 0;

    db->record_byte_swap = record_byte_swap;
    db->qfs = (Sdqm_query_field_t *)pqfs;
    db->its = (Sdqm_query_index_t *)pits; 

    db->n_qfs = n_qfs;
    for (i = 0; i < n_qfs; i++) {	/* set db->qfs */
	int indg, m, n, ret;
	int roff, type;

	type = qfs[i].type;
	if (type < 0 || type > SDQM_QFT_USER_DEF)
	    return (SDQM_BAD_QUERY_FIELD_TYPE);
	db->qfs[i].type = type;

	indg = 0;
	for (m = 0; m < n_its; m++) {
	    for (n = 0; n < its[m].n_itfs; n++) {
		if (its[m].itfs[n] == i) {
		    indg = 1;
		    break;
		}
	    }
	    if (indg)
		break;
	}
	db->qfs[i].indexing = indg;

	roff = qfs[i].foff;
	if (roff < 0 || roff >= db->rec_size)
	    return (SDQM_BAD_QUERY_FIELD_OFFSET);
	db->qfs[i].foff = roff;

	if ((ret = Check_alignment (type, roff)) < 0)
	    return (ret);

	if (type == SDQM_QFT_USER_DEF) {
	    if (qfs[i].compare == NULL || qfs[i].set_fields == NULL)
		return (SDQM_BAD_USER_DEF_TYPE_FUNC);
	    db->qfs[i].compare = qfs[i].compare;
	    db->qfs[i].set_fields = qfs[i].set_fields;
	}
	else {
	    db->qfs[i].compare = NULL;
	    db->qfs[i].set_fields = NULL;
	}
    }

    db->n_its = n_its;
    for (i = 0; i < n_its; i++)	{	/* varify its and set db->its */
	int k, n;
	SDQM_query_index_t *it;
	Sdqm_query_index_t *dbit;

	it = its + i;
	dbit = db->its + i;
	for (k = 0; k < it->n_itfs; k++) {
	    int f;

	    f = it->itfs[k];
	    if (f < 0 || f >= n_qfs)
		return (SDQM_BAD_FIELD_IN_INDEXING_TABLE);
	    for (n = k + 1; n < it->n_itfs; n++) {
		if (it->itfs[n] == f)
		    return (SDQM_DUPLICATED_FIELD_IN_INDEXING_TABLE);
	    }
	    dbit->itfs[k] = f;
	    dbit->cfs.type[k] = db->qfs[f].type;
	    dbit->cfs.field[k] = f;
	    dbit->cfs.foff[k] = db->qfs[f].foff;
	}
	dbit->n_itfs = it->n_itfs;
	dbit->cfs.nf = it->n_itfs;
    }

    /* init RSIS */
    db->rsid = RSIS_init (max_records, n_its, rec_size, 
                                       NULL, NULL, db->compare_func);
    if (db->rsid == NULL)
	return (SDQM_RSIS_INIT_FAILED);

#ifdef SDQM_TEST
Print_db (db);
#endif

    /* register the new SDB */
    {
	Sdb_table_t *new;

	new = (Sdb_table_t *)MISC_table_new_entry (Sdb_tid, NULL);
	if (new == NULL)
	    return (SDQM_MALLOC_FAILED);
	new->id = (void *)db;
	strcpy (new->name, db_name);
    }

    return (0);
}

/******************************************************************

    Description: This function returns the ID of SDB "db_name".

    Input:	db_name - name of the SDB.

    Return:	db ID on success or NULL on failure.

******************************************************************/

void *SDQM_get_dbid (char *db_name)
{
    int i;

    for (i = 0; i < N_sdbs; i++) {
	if (strcmp (db_name, Sdbs[i].name) == 0)
	    return (Sdbs[i].id);
    }
    return (NULL);
}

/******************************************************************

    Description: This function checks if a field offset is aligned.

    Input:	type - the query field type.
		foff - field offset.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Check_alignment (int type, int foff)
{

    switch (type) {

	case SDQM_QFT_INT:
	    if (foff % sizeof (SDQM_int) != 0)
		return (SDQM_QUERY_FIELD_NOT_ALIGNED);
	    break;
	case SDQM_QFT_SHORT:
	    if (foff % sizeof (SDQM_short) != 0)
		return (SDQM_QUERY_FIELD_NOT_ALIGNED);
	    break;
	case SDQM_QFT_FLOAT:
	    if (foff % sizeof (SDQM_float) != 0)
		return (SDQM_QUERY_FIELD_NOT_ALIGNED);
	    break;
	case SDQM_QFT_STRING:
	    if (foff % sizeof (SDQM_string) != 0)
		return (SDQM_QUERY_FIELD_NOT_ALIGNED);
	    break;
    }
    return (0);
}

/******************************************************************

    Description: This function processes a query msg "query" in 
		data base "db_id".

    Input:	query - The query message.
		len - length of the query message.
		result_type - type of returned results.

    Output:	results - the query results in SDQM_query_results
			format.

    Return:	result_type = SDQM_SERIALIZED_RESULT:
		    length of the result message or SDQM_MALLOC_FAILED
		    if malloc failed.
		result_type = SDQM_INDEX_RESULT_ONLY:
		    length of the result message or a negative SDQM 
		    error code.

******************************************************************/

#define MAX_N_QUERIES 128

int SDQM_process_query (char *query, int len, int result_type, void **results)
{
    static int n_recs[MAX_N_QUERIES];	/* records found per query */
    void *db_id;
    Sdqm_db_t *db;
    SDQM_query *q;
    int size, rec_cnt, i, ret;
    Query_info qinfo;

    q = (SDQM_query *)query;
    *results = NULL;
    if (len < (int)sizeof (SDQM_query))
	return (SDQM_query_err_ret (result_type, 
				SDQM_QUERY_MSG_LEN_ERR, q, results));

    if (len != q->msg_size)
	return (SDQM_query_err_ret (result_type, 
				SDQM_QUERY_MSG_LEN_ERR, q, results));

    db_id = NULL;		/* not useful - turn off gcc warning */
    for (i = 0; i < N_sdbs; i++) {
	if (strcmp (q->db_name, Sdbs[i].name) == 0) {
	    db_id = Sdbs[i].id;
	    break;
	}
    }
    if (i >= N_sdbs)
	return (SDQM_query_err_ret (result_type, 
				SDQM_DB_NAME_NOT_FOUND, q, results));

    db = (Sdqm_db_t *)db_id;
    Cr_db = db;
    SDQM_set_cr_db (db);		/* send db to sdqm_result.c */

    if (q->max_list_size < 0 || q->n_queries < 0)
	return (SDQM_query_err_ret (result_type, 
				SDQM_QUERY_HD_ERR, q, results));
    if (q->n_queries > MAX_N_QUERIES)
	return (SDQM_query_err_ret (result_type, 
				SDQM_BUFFER_OVERFLOW, q, results));

    qinfo.cr_n_recs = 0;
    Init_record_list (&(qinfo.recl));
    qinfo.n_units = 0;
    Cr_db->query_mode = q->query_mode;
    qinfo.max_list_size = q->max_list_size;
    qinfo.done = 0;

    rec_cnt = 0;
    size = sizeof (SDQM_query);
    for (i = 0; i < q->n_queries; i++) {
	SDQM_single_query *sq;

	sq = (SDQM_single_query *)(query + size);
	if (size + (int)sizeof (SDQM_single_query) > len ||
	    size + sq->size > len) {
	    Free_query_info (&qinfo);
	    return (SDQM_query_err_ret (result_type, 
					SDQM_QUERY_LEN_ERR, q, results));
	}
	ret = Process_single_query (query + size, &qinfo);
	if (ret < 0) {
	    Free_query_info (&qinfo);
	    return (SDQM_query_err_ret (result_type, ret, q, results));
	}
	size += sq->size;
	n_recs[i] = qinfo.cr_n_recs - rec_cnt;	/* n records for this query */
	rec_cnt = qinfo.cr_n_recs;

	if (qinfo.secs != NULL) {
	    free (qinfo.secs);
	    qinfo.secs = NULL;
	}
	if (qinfo.done)
	    break;
    }

    Post_process_rec_list (&qinfo);

    /* generate output */
    if (result_type == SDQM_SERIALIZED_RESULT)
	ret = SDQM_generate_results (&qinfo, q, results, n_recs);
    else
	ret = SDQM_generate_index_result (&qinfo, q, results);
    if (ret < 0) {
	Free_query_info (&qinfo);
	return (SDQM_query_err_ret (result_type, ret, q, results));
    }

    Free_query_info (&qinfo);

    return (ret);
}

/******************************************************************

    Description: This function returns all records in responding to
		an empty query request.

    Input:	qinfo - the query info struct.

******************************************************************/

static int Process_empty_query (Query_info *qinfo)
{
    int ind, ret;

    qinfo->rec_add = RSIS_get_record_address (Cr_db->rsid);
    qinfo->mcfs->nf = 0;
    ind = 0;
    while ((ind = RSIS_get_next_ind (Cr_db->rsid, ind)) > 0) {
	if ((ret = Test_and_save_record (qinfo, ind)) < 0)
	    return (ret);
	if (qinfo->done)
	    break;
    }
    return (0);
}

/******************************************************************

    Description: This function performs post-processing of the 
		record list found by the query. Currently, we remove
		duplicated records. We may add sorting later.

    Input:	qinfo - the query info struct.

******************************************************************/

static void Post_process_rec_list (Query_info *qinfo)
{
    unsigned short *list;

    list = qinfo->recl.list;

    if (qinfo->n_units > 1) {
	int n, i;

	n = 0;
	for (i = 0; i < qinfo->cr_n_recs; i++) {
	    unsigned short v;
	    int k;

	    v = list[i];
	    for (k = 0; k < n; k++)
		if (list[k] == v)
		    break;
	    if (k >= n) {
		if (n != i)
		    list[n] = v;
		n++;
	    }
	}
        qinfo->cr_n_recs = n;
    }
}

/******************************************************************

    Description: This function processes a single query "query".

    Input:	query - pointer to a single query in a query msg.
		qinfo - information about the query.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Process_single_query (char *query, Query_info *qinfo)
{
    int ret;

    if ((ret = SDQM_preprocess_query_msg (Cr_db, query, qinfo)) != 0)
				/* verify the query and set qinfo */
	return (ret);

    if (((SDQM_single_query *)query)->n_sections == 0)
	return (Process_empty_query (qinfo));	/* process empty query */

    if ((ret = Recursive_query (0, qinfo)) != 0)	/* process query */
	return (ret);

    return (0);
}

/******************************************************************

    Description: This function is recursively called for finding a
		combination of valus/ranges of all specified query
		fields to be processed as a unit query.

    Input:	cr_level - the current level in query.
		qinfo - information about the query.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Recursive_query (int cr_level, Query_info *qinfo)
{
    Query_level_t *ql;
    int i, ret;

    ql = qinfo->qls + cr_level;
    ret = 0;
    for (i = 0; i < ql->n_values; i++) {
	ql->cr_sec = ql->first_sec + i;
	if (cr_level < qinfo->n_qls - 1) 	/* more levels to go */
	    ret = Recursive_query (cr_level + 1, qinfo);
	else					/* bottom level */
	    ret = Process_unit_query (qinfo);
	if (ret < 0 || qinfo->done)
	    break;
    }
    return (ret);
}

/******************************************************************

    Description: This function processes a unit query - a
		combination of valus/ranges of all specified query
		fields.

    Input:	qinfo - information about the query.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Process_unit_query (Query_info *qinfo)
{
    int ret;

    Find_best_indexing (qinfo);

    if (Cr_db->query_mode & SDQM_DISTINCT_FIELD_VALUES) { /* distinct record */
	if (qinfo->best_it < 0 || 
		qinfo->n_qls > 1)
	    return (SDQM_NOT_IMPLEMENTED);
	memcpy (qinfo->ucfs, &(Cr_db->its[qinfo->best_it].cfs), 
					sizeof (Compare_field_spec));
	qinfo->ucfs->nf = qinfo->n_qls;
    }

    if ((ret = Indexing_search (qinfo)) < 0)
	return (ret);
    qinfo->n_units++;
    return (0);
}

/******************************************************************

    Description: This function searches for the best indexing table
		to use for the unit query.

    Input:	qinfo - information about the query.

******************************************************************/

static void Find_best_indexing (Query_info *qinfo)
{
    struct {
	int field;
	int is_value;
	int checked_out;
    } kfs[MAX_FIELDS_IN_INDEX];		/* key fields in query */
    Sdqm_db_t *db;
    int n_kfs, i, max, best_it;

    db = qinfo->db;
    n_kfs = 0;
    for (i = 0; i < qinfo->n_qls; i++) {
	Query_level_t *ql;

	ql = qinfo->qls + i;
	if (db->qfs[ql->field].indexing) {
	    kfs[n_kfs].field = ql->field;
	    kfs[n_kfs].is_value = qinfo->secs[ql->cr_sec].is_value;
	    n_kfs++;
	}
    }

    if (n_kfs == 0) {			/* no indexing field found */
	qinfo->best_it = -1;
	return;
    }

    max = -1;				/* max number of matched qf */
    best_it = -1;			/* the best it index */
    for (i = 0; i < db->n_its; i++) {	/* check each indexing */
	Sdqm_query_index_t *it;
	int k, n, is_value, found;

	for (n = 0; n < n_kfs; n++)
	    kfs[n].checked_out = 0;

	it = db->its + i;
	found = 0;
	is_value = 0;		/* not necessary - turn off gcc warning */
	for (k = 0; k < it->n_itfs; k++) {
	    int f;

	    f = it->itfs[k];
	    for (n = 0; n < n_kfs; n++) {
		if (!kfs[n].checked_out &&
		    kfs[n].field == f) {

		    is_value = kfs[n].is_value;
		    found = 1;
		    kfs[n].checked_out = 1;
		    break;
		}
	    }
	    if (!found || !is_value)
		break;
	}
	if (found) {
	    if (!is_value)
		k++;
	    if (k > max) {
		max = k;
		best_it = i;
	    }
	}
    }
    qinfo->best_it = best_it;

    return;
}

/******************************************************************

    Description: This function performs the indexing search.

    Input:	qinfo - information about the query.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Indexing_search (Query_info *qinfo)
{
    Sdqm_db_t *db;
    int ind, ret;

    db = qinfo->db;
    qinfo->rec_add = RSIS_get_record_address (db->rsid);
    if (qinfo->best_it < 0) {		/* no indexing table found */

	qinfo->icfs->nf = 0;
	ret = Prepare_for_non_index_field_comparison (qinfo);
	if (ret < 0)
	    return (ret);
	ind = 0;
	while ((ind = RSIS_get_next_ind (db->rsid, ind)) > 0) {
	    if ((ret = Test_and_save_record (qinfo, ind)) < 0)
		return (ret);
	    if (qinfo->done)
		break;
	}
    }
    else {
	Sdqm_query_index_t *it;
	int i, k;

	it = db->its + qinfo->best_it;
	for (i = 0; i < it->n_itfs; i++) {	/* go through the fields in the
						   best indexing table */
	    int f;
	    SDQM_query_int *psec;	/* point to the query section */

	    f = it->itfs[i];
	    for (k = 0; k < qinfo->n_qls; k++) 
		if (qinfo->qls[k].field == f)
		    break;
	    if (k < qinfo->n_qls)
		psec = qinfo->secs[qinfo->qls[k].cr_sec].psec;
	    else
		psec = NULL;

	    if (psec != NULL) {
		ret = Local_set_records (psec, 
					qinfo->imin_rec, qinfo->imax_rec);
		if (db->db_type == SDQM_SIMPLE_QUERY_FIELDS) {
		    Sdqm_query_field_t *qf;
		    qf = db->qfs + f;
		    if (i >= MAX_N_COMPARE_FIELDS)
			return (SDQM_QUERY_TOO_MANY_COMPARE_FIELDS);
		    qinfo->icfs->type[i] = qf->type;
		    qinfo->icfs->field[i] = f;
		    qinfo->icfs->foff[i] = qf->foff;
		    if (ret == 0) {
			i++;
			break;		/* stop at range selection field */
		    }
		}
	    }
	    else
		break;

	}
	qinfo->icfs->nf = i;

	ret = Prepare_for_non_index_field_comparison (qinfo);
	if (ret < 0)
	    return (ret);
	if (db->query_mode & SDQM_HIGHEND_SEARCH)
	    ret = Highend_search (qinfo, db);
	else
	    ret = Lowend_search (qinfo, db);
	if (ret < 0)
	    return (ret);
    }

    return (0);
}

/******************************************************************

    Description: This function walks through the indexed records
		from left (low value) end.

    Input:	qinfo - information about the query.
		db - current DB struct.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Lowend_search (Query_info *qinfo, Sdqm_db_t *db)
{
    int ind, ret;
    int tind, min_checked;
    void *rec, *trec;

    ind = RSIS_find (db->rsid, qinfo->best_it, qinfo->imin_rec, &rec);
    if (ind == RSIS_NOT_FOUND)
	return (0);				/* not found */

    tind = ind;
    trec = rec;
    min_checked = 0;
    Cr_cfs = qinfo->icfs;
    while (1) {			/* travel left */
	if (tind == RSIS_NOT_FOUND ||
	    Local_compare (-1, trec, qinfo->imin_rec) == RSIS_LESS)
	    break;
	min_checked = 1;
	ind = tind;
	rec = trec;
	tind = RSIS_traverse (db->rsid, qinfo->best_it, 
						RSIS_LEFT, tind, &trec);
    }
    while (1) {			/* travel right */
	Cr_cfs = qinfo->icfs;
	if (ind == RSIS_NOT_FOUND ||
	    Local_compare (-1, rec, qinfo->imax_rec) == RSIS_GREATER)
	    break;
	if (!min_checked && 
	    Local_compare (-1, rec, qinfo->imin_rec) != RSIS_LESS)
	    min_checked = 1;
	if (min_checked &&
	    (ret = Test_and_save_record (qinfo, ind)) < 0)
	    return (ret);
	if (qinfo->done)
	    break;
	    
	ind = RSIS_traverse (db->rsid, qinfo->best_it, 
						RSIS_RIGHT, ind, &rec);
    }
    return (0);
}

/******************************************************************

    Description: This function walks through the indexed records
		from right (high value) end.

    Input:	qinfo - information about the query.
		db - current DB struct.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Highend_search (Query_info *qinfo, Sdqm_db_t *db)
{
    int ind, ret;
    int tind, max_checked;
    void *rec, *trec;

    ind = RSIS_find (db->rsid, qinfo->best_it, qinfo->imax_rec, &rec);
    if (ind == RSIS_NOT_FOUND)
	return (0);				/* not found */

    tind = ind;			/* tmp index */
    trec = rec;
    max_checked = 0;
    Cr_cfs = qinfo->icfs;
    while (1) {			/* travel right */
	if (tind == RSIS_NOT_FOUND ||
	    Local_compare (-1, trec, qinfo->imax_rec) == RSIS_GREATER)
	    break;
	max_checked = 1;
	ind = tind;
	rec = trec;
	tind = RSIS_traverse (db->rsid, qinfo->best_it, 
						RSIS_RIGHT, tind, &trec);
    }
    while (1) {			/* travel left */
	Cr_cfs = qinfo->icfs;
	if (ind == RSIS_NOT_FOUND ||
	    Local_compare (-1, rec, qinfo->imin_rec) == RSIS_LESS)
	    break;
	if (!max_checked && 
	    Local_compare (-1, rec, qinfo->imax_rec) != RSIS_GREATER)
	    max_checked = 1;
	if (max_checked &&
	    (ret = Test_and_save_record (qinfo, ind)) < 0)
	    return (ret);
    	if (qinfo->done)
	    break;

	ind = RSIS_traverse (db->rsid, qinfo->best_it, 
						RSIS_LEFT, ind, &rec);
    }
    return (0);
}

/******************************************************************

    Description: This function prepares parameters for non-indexed
		field comparison.

    Input:	qinfo - information about the query.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Prepare_for_non_index_field_comparison  (Query_info *qinfo)
{
    Sdqm_db_t *db;
    Compare_field_spec *cfs;
    int i, cnt;

   /* figure out the query fields needed for sequential search */
    db = qinfo->db;
    cfs = qinfo->icfs;
    cnt = 0;
    qinfo->min_is_max = 1;
    for (i = 0; i < qinfo->n_qls; i++) {
	int f, k;
	SDQM_query_int *psec;		/* point to the query section */

	f = qinfo->qls[i].field;
	for (k = 0; k < cfs->nf; k++)
	    if (cfs->field[k] == f)
		break;
	if (k < cfs->nf)
	    continue;

	psec = qinfo->secs[qinfo->qls[i].cr_sec].psec;
	if (Local_set_records (psec, qinfo->mmin_rec, qinfo->mmax_rec) == 0)
	    qinfo->min_is_max = 0;
	if (db->db_type == SDQM_SIMPLE_QUERY_FIELDS) {
	    Sdqm_query_field_t *qf;
	    if (cnt >= MAX_N_COMPARE_FIELDS)
		return (SDQM_QUERY_TOO_MANY_COMPARE_FIELDS);
	    qf = db->qfs + f;
	    qinfo->mcfs->type[cnt] = qf->type;
	    qinfo->mcfs->field[cnt] = f;
	    qinfo->mcfs->foff[cnt] = qf->foff;
	}
	cnt++;
    }
    qinfo->mcfs->nf = cnt;

    return (0);
}

/******************************************************************

    Description: This function frees resources in query info struct
		"qinfo".

    Input/Output:	qinfo - information about the query.

******************************************************************/

static void Free_query_info (Query_info *qinfo)
{

    if (qinfo->secs != NULL) {
	free (qinfo->secs);
	qinfo->secs = NULL;
    }
    Free_record_list (&(qinfo->recl));
    return;
}

/******************************************************************

    Description: This function adds a new record to data base 
		"db_id".

    Input:	db_id - The DB identifier.
		rec - The new record.

    Return:	It returns the record index on success or
		RSIS_TOO_MANY_RECORDS on failure.

******************************************************************/

int SDQM_insert (void *db_id, void *rec)
{

    Cr_db = (Sdqm_db_t *)db_id;
    return (RSIS_insert (Cr_db->rsid, rec));
}

/******************************************************************

    Description: This function removes a record from data base 
		"db_id".

    Input:	db_id - The DB identifier.
		ind - The index of the record to be deleted.

    Return:	It returns 0 on success or RSIS_INVALID_INDEX 
		if "ind" is not valid.

******************************************************************/

int SDQM_delete (void *db_id, int ind)
{

    Cr_db = (Sdqm_db_t *)db_id;
    return (RSIS_delete (Cr_db->rsid, ind));
}

/******************************************************************

    Description: This function closes data base "db_name" and
		frees all resources. This function is to be 
		implemented after RSIS has a close function available.

    Input:	db_name - name of the SDB.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

int SDQM_remove (char *db_name)
{

    return (0);
}

/******************************************************************

    Description: This compares two records "rec1" and "rec2" in
		terms of the indexing table index "which_it" or
		comparison field spec in "Cr_cfs" if "which_it" < 0.
		The is a multiple field comparison function. It is
		used by indexing searching (called from RSIS; 
		"which_it" >= 0) and sequential searching (called 
		from SDQM, "which_it" < 0).

    Input:	which_it - indexing table in use.
		rec1 - the first record.
		rec2 - the second record.

    Return:	RSIS_LESS, RSIS_GREATER or RSIS_EQUAL.

******************************************************************/

static int Local_compare (int which_it, void *rec1, void *rec2)
{
    Compare_field_spec *cfs;
    int mode, i;

    if (which_it >= 0) {
	cfs = &(Cr_db->its[which_it].cfs);
	mode = 0;
    }
    else {
	cfs = Cr_cfs;
	mode = Cr_db->query_mode;
    }

    for (i = 0; i < cfs->nf; i++) {
	int roff;

	roff = cfs->foff[i];
	switch (cfs->type[i]) {
	    int ipt1, ipt2;
	    int s1, s2;
	    float fpt1, fpt2;
	    char *str1, *str2;
	    int ret, f;

	    case SDQM_QFT_INT:
		ipt1 = *((int *)((char *)rec1 + roff));
		ipt2 = *((int *)((char *)rec2 + roff));
		if ((mode & SDQM_ALL_MATCH) &&
		    (ipt1 == SDQM_UNDEF_INT || ipt2 == SDQM_UNDEF_INT))
			break;
		if (ipt1 < ipt2)
		    return (RSIS_LESS);
		else if (ipt1 > ipt2)
		    return (RSIS_GREATER);
		break;

	    case SDQM_QFT_SHORT:
		s1 = *((short *)((char *)rec1 + roff));
		s2 = *((short *)((char *)rec2 + roff));
		if ((mode & SDQM_ALL_MATCH) &&
		    (s1 == SDQM_UNDEF_SHORT || s2 == SDQM_UNDEF_SHORT))
			break;
		if (s1 < s2)
		    return (RSIS_LESS);
		else if (s1 > s2)
		    return (RSIS_GREATER);
		break;

	    case SDQM_QFT_FLOAT:
		fpt1 = *((float *)((char *)rec1 + roff));
		fpt2 = *((float *)((char *)rec2 + roff));
		if ((mode & SDQM_ALL_MATCH) &&
		    (fpt1 == SDQM_UNDEF_FLOAT || fpt2 == SDQM_UNDEF_FLOAT))
			break;
		if (fpt1 < fpt2)
		    return (RSIS_LESS);
		else if (fpt1 > fpt2)
		    return (RSIS_GREATER);
		break;

	    case SDQM_QFT_STRING:
		str1 = *((char **)((char *)rec1 + roff));
		str2 = *((char **)((char *)rec2 + roff));
		if ((mode & SDQM_ALL_MATCH) &&
		    (strcmp (str1, SDQM_UNDEF_STRING) == 0 || 
				strcmp (str1, SDQM_UNDEF_STRING) == 0))
			break;
		ret = strcmp (str1, str2);
		if (ret < 0)
		    return (RSIS_LESS);
		else if (ret > 0)
		    return (RSIS_GREATER);
		break;

	    case SDQM_QFT_USER_DEF:
		f = cfs->field[i];
		ret = Cr_db->qfs[f].compare ((char *)rec1 + roff, 
						(char *)rec2 + roff);
		if (ret != RSIS_EQUAL)
		    return (ret);
	}
    }

    return (RSIS_EQUAL);
}

/******************************************************************

    Checks if record "rec" is less than record "min" and greater 
    than "max in terms of the comparision field specification "cfs".
    This is a multiple field comparison function. Returns RSIS_LESS 
    if "rec" < "min, RSIS_GREATER if "rec" > "max", or RSIS_EQUAL if
    "min" <= "rec" <= "max".

******************************************************************/

static int Local_range_compare (Compare_field_spec *cfs, 
					void *rec, void *min, void *max) {
    int mode, i;

    mode = Cr_db->query_mode;
    for (i = 0; i < cfs->nf; i++) {
	int roff;

	roff = cfs->foff[i];
	switch (cfs->type[i]) {
	    int s, smin, smax;
	    float f, fmin, fmax;
	    char *str, *strmin, *strmax;
	    int field;

	    case SDQM_QFT_INT:
		s = *((int *)((char *)rec + roff));
		smax = *((int *)((char *)max + roff));
		smin = *((int *)((char *)min + roff));
		if ((mode & SDQM_ALL_MATCH) &&
		    (s == SDQM_UNDEF_INT || smax == SDQM_UNDEF_INT || 
						smin == SDQM_UNDEF_INT))
			break;
		if (s < smin)
		    return (RSIS_LESS);
		else if (s > smax)
		    return (RSIS_GREATER);
		break;

	    case SDQM_QFT_SHORT:
		s = *((short *)((char *)rec + roff));
		smin = *((short *)((char *)min + roff));
		smax = *((short *)((char *)max + roff));
		if ((mode & SDQM_ALL_MATCH) &&
		    (s == SDQM_UNDEF_SHORT || smin == SDQM_UNDEF_SHORT || 
						smax == SDQM_UNDEF_SHORT))
			break;
		if (s < smin)
		    return (RSIS_LESS);
		else if (s > smax)
		    return (RSIS_GREATER);
		break;

	    case SDQM_QFT_FLOAT:
		f = *((float *)((char *)rec + roff));
		fmax = *((float *)((char *)max + roff));
		fmin = *((float *)((char *)min + roff));
		if ((mode & SDQM_ALL_MATCH) &&
		    (f == SDQM_UNDEF_FLOAT || fmin == SDQM_UNDEF_FLOAT || 
						fmax == SDQM_UNDEF_FLOAT))
			break;
		if (f < fmin)
		    return (RSIS_LESS);
		else if (f > fmax)
		    return (RSIS_GREATER);
		break;

	    case SDQM_QFT_STRING:
		str = *((char **)((char *)rec + roff));
		strmin = *((char **)((char *)min + roff));
		strmax = *((char **)((char *)max + roff));
		if ((mode & SDQM_ALL_MATCH) &&
		    (strcmp (str, SDQM_UNDEF_STRING) == 0 || 
				strcmp (strmin, SDQM_UNDEF_STRING) == 0 ||
				strcmp (strmax, SDQM_UNDEF_STRING) == 0))
			break;
		if (strcmp (str, strmin) < 0)
		    return (RSIS_LESS);
		else if (strcmp (str, strmax) > 0)
		    return (RSIS_GREATER);
		break;

	    case SDQM_QFT_USER_DEF:
		field = cfs->field[i];
		if (Cr_db->qfs[field].compare ((char *)rec + roff, 
					(char *)min + roff) == RSIS_LESS)
		    return (RSIS_LESS);
		else if (Cr_db->qfs[field].compare ((char *)rec + roff, 
					(char *)max + roff) == RSIS_GREATER)
		    return (RSIS_GREATER);
		break;
	}
    }

    return (RSIS_EQUAL);
}

/******************************************************************

    Description: This function puts min and max query field values
		in records "min_rec" and "max_rec". This is the local
		implementation for the SIMPLE type SDB. It requires
		the DB struct Cr_db.

    Input:	psec - pointer to the query section.

    Outputs:	min_rec, max_rec - records corresponsing to the min 
		and max values.

    Return:	1 if min equals max or 0 otherwise.

******************************************************************/

static int Local_set_records (SDQM_query_int *psec, 
				char *min_rec, char *max_rec)
{
    Sdqm_query_field_t *qf;
    int foff, min_is_max;

    min_is_max = 1;
    qf = Cr_db->qfs + psec->field;
    foff = qf->foff;
    switch (qf->type) {
	SDQM_query_short *qsecs;
	SDQM_query_float *qsecf;
	SDQM_query_string *qsecstr;
	char *s1, *s2;
	int f;

	case SDQM_QFT_INT:
	    *((int *)(min_rec + foff)) = psec->min;
	    *((int *)(max_rec + foff)) = psec->max;
	    if (psec->min != psec->max)
		min_is_max = 0;
	    break;

	case SDQM_QFT_SHORT:
	    qsecs = (SDQM_query_short *)psec;
	    *((short *)(min_rec + foff)) = qsecs->min;
	    *((short *)(max_rec + foff)) = qsecs->max;
	    if (qsecs->min != qsecs->max)
		min_is_max = 0;
	    break;

	case SDQM_QFT_FLOAT:
	    qsecf = (SDQM_query_float *)psec;
	    *((float *)(min_rec + foff)) = qsecf->min;
	    *((float *)(max_rec + foff)) = qsecf->max;
	    if (qsecf->min != qsecf->max)
		min_is_max = 0;
	    break;

	case SDQM_QFT_STRING:
	    qsecstr = (SDQM_query_string *)psec;
	    s1 = (char *)psec + qsecstr->str1_off;
	    s2 = (char *)psec + qsecstr->str2_off;
	    *((char **)(min_rec + foff)) = s1;
	    *((char **)(max_rec + foff)) = s2;
	    if (strcmp (s1, s2) != 0)
		min_is_max = 0;
	    break;

	case SDQM_QFT_USER_DEF:
	    f = psec->field;
	    min_is_max = Cr_db->qfs[f].set_fields (psec,
					min_rec + foff, max_rec + foff);

	    break;
    }
    return (min_is_max);
}

/******************************************************************

    Description: This function initialize a record list struct.

    Input/Output:	recl - the record list struct.

******************************************************************/

static void Init_record_list (Record_list_t *recl)
{

    recl->n_recs = 0;
    recl->array_size = 0;
    recl->list = NULL;
    return;
}

/******************************************************************

    Description: This function frees memory and initialize a record 
		list struct.

    Input/Output:	recl - the record list struct.

******************************************************************/

static void Free_record_list (Record_list_t *recl)
{

    if (recl->list != NULL)
	free (recl->list);
    Init_record_list (recl);
    return;
}

/******************************************************************

    Description: This function inserts a new record index into the
		record list.

    Input:	qinfo - the query info struct.
		ind - new record index.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

static int Test_and_save_record (Query_info *qinfo, unsigned short ind)
{
    Record_list_t *recl;
    int ret;

    recl = &(qinfo->recl);
    if (qinfo->mcfs->nf > 0) {	/* check non-indexed fields */
	void *r;

	r = qinfo->rec_add + ind * Cr_db->rec_size;
	if (qinfo->min_is_max) {
	    Cr_cfs = qinfo->mcfs;
	    ret = Local_compare (-1, r, qinfo->mmin_rec);
	}
	else 
	    ret = Local_range_compare (qinfo->mcfs, 
				r, qinfo->mmin_rec, qinfo->mmax_rec);
	if (ret == RSIS_LESS || ret == RSIS_GREATER) /* does not match */
	    return (0);
    }

    if ((Cr_db->query_mode & SDQM_DISTINCT_FIELD_VALUES) && 
	recl->n_recs > 0) {			/* process distinct record */
	int pind;

	Cr_cfs = qinfo->ucfs;
	pind = recl->list[recl->n_recs - 1]; /* previous record index */
	if (Local_compare (-1, 
		qinfo->rec_add + ind * Cr_db->rec_size, 
		qinfo->rec_add + pind * Cr_db->rec_size) == RSIS_EQUAL)
	    return (0);			/* not a distinct record */
    }

    if (SDQSA_is_exclusive (qinfo->rec_add + ind * Cr_db->rec_size, 
							qinfo->n_units))
	return (0);

    ret = Insert_in_record_list (recl, ind);
    if (ret < 0)
	return (ret);
    qinfo->cr_n_recs++;
    if (!(Cr_db->query_mode & SDQM_FULL_SEARCH) &&
	qinfo->cr_n_recs >= qinfo->max_list_size)
	qinfo->done = 1;

    return (0);
}

/******************************************************************

    Description: This function inserts a new record index into the
		record list "recl".

    Inputs:	recl - the record list struct.
		ind - new record index.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

#define RECORD_LIST_SIZE_INC 	256

static int Insert_in_record_list (Record_list_t *recl, unsigned short ind)
{

    if (recl->n_recs >= recl->array_size) {	/* realloc the buffer */
	int new_size;
	char *pt;

	new_size = recl->array_size + RECORD_LIST_SIZE_INC;
	pt = malloc (new_size * sizeof (unsigned short));
	if (pt == NULL)
	    return (SDQM_MALLOC_FAILED);

	if (recl->n_recs > 0)
	    memcpy (pt, recl->list, recl->n_recs * sizeof (unsigned short));
	if (recl->list != NULL)
	    free (recl->list);

	recl->array_size = new_size;
	recl->list = (unsigned short *)pt;
    }

    recl->list[recl->n_recs] = ind;
    recl->n_recs++;

    return (0);
}








