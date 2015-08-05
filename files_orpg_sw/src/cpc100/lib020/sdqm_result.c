
/******************************************************************

	file: sdqm_result.c

	This is part of the SDQM module - The simple data base 
	query management. This contains the functions processing
	query and its results.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/04/15 19:35:54 $
 * $Id: sdqm_result.c,v 1.11 2005/04/15 19:35:54 jing Exp $
 * $Revision: 1.11 $
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
#include <lb.h> 

static Sdqm_db_t *Cr_db;		/* current DB. Used for passing this
					   to Local_set_records and 
					   Local_compare. */

static int Get_variable_field_info (Query_info *qinfo, 
			Variable_fields_t **vf_info, int *vf_cnt);


/******************************************************************

    Description: Receives the pointer to the current DB.

    Input:	cr_db - pointer to the current DB.

******************************************************************/

void SDQM_set_cr_db (Sdqm_db_t *cr_db)
{
     Cr_db = cr_db;
}

/******************************************************************

    Description: This function goes through a query msg, verifies 
		it and sets the information struct about the query.

    Input:	db - the DB struct.
		query - the query message.

    Output:	qinfo - information about the query.

    Return:	0 on success or a negative SDQM error number.

******************************************************************/

int SDQM_preprocess_query_msg (Sdqm_db_t *db, char *query, 
						Query_info *qinfo)
{
    SDQM_single_query *q;
    SDQM_query_int *qs;		/* query section */
    Query_sec_t *secs;
    Query_level_t *qls;
    int len, size, i, cnt, field, level;

    q = (SDQM_single_query *)query;
    len = q->size;

    qinfo->db = db;
    qinfo->query = query;
    qinfo->secs = (Query_sec_t *)malloc (q->n_sections * sizeof (Query_sec_t) + 
			MAX_QUERY_LEVELS * sizeof (Query_level_t) +
			db->rec_size * 4 + 
			sizeof (Compare_field_spec) * 3);
    if (qinfo->secs == NULL)
	return (SDQM_MALLOC_FAILED);
    qinfo->qls = (Query_level_t *)((char *)qinfo->secs + 
			q->n_sections * sizeof (Query_sec_t));
    qinfo->imin_rec = (char *)((char *)qinfo->qls + 
			MAX_QUERY_LEVELS * sizeof (Query_level_t));
    qinfo->imax_rec = (char *)((char *)qinfo->imin_rec + db->rec_size);
    qinfo->mmin_rec = (char *)((char *)qinfo->imax_rec + db->rec_size);
    qinfo->mmax_rec = (char *)((char *)qinfo->mmin_rec + db->rec_size);
    qinfo->icfs = (Compare_field_spec *)((char *)qinfo->mmax_rec + 
							db->rec_size);
    qinfo->mcfs = (Compare_field_spec *)((char *)qinfo->icfs + 
						sizeof (Compare_field_spec));
    qinfo->ucfs = (Compare_field_spec *)((char *)qinfo->mcfs + 
						sizeof (Compare_field_spec));
    memset (qinfo->imin_rec, 0, 2 * db->rec_size);
	/* some "done-care" fields will not be set but used by RSIS_find.
	   We set all fields to 0 for determinist behavior. */

    qinfo->n_secs = q->n_sections;
    secs = qinfo->secs;
    size = sizeof (SDQM_single_query);
    for (i = 0; i < q->n_sections; i++) {
	int sec_size, field, is_value, k, m;

	if (size + 2 * (int)sizeof (SDQM_int) > len)
	    return (SDQM_QUERY_LEN_ERR);

	qs = (SDQM_query_int *)(query + size);
	field = qs->field;
	if (field < 0 || field >= db->n_qfs)
	    return (SDQM_QUERY_FIELD_ERR);

	if (db->qfs[field].type != qs->type)
	    return (SDQM_QUERY_TYPE_ERR);

	is_value = 0;
	switch (qs->type) {
	    SDQM_query_int *qint;
	    SDQM_query_short *qshort;
	    SDQM_query_float *qf;
	    SDQM_query_string *qstr;

	    case SDQM_QFT_INT:
		if (qs->size != sizeof (SDQM_query_int))
		    return (SDQM_QUERY_FIELD_SIZE_ERR);
		qint = (SDQM_query_int *)qs;
		if (qint->min == qint->max)
		    is_value = 1;
		sec_size = qs->size;
		break;

	    case SDQM_QFT_SHORT:
		if (qs->size != sizeof (SDQM_query_short))
		    return (SDQM_QUERY_FIELD_SIZE_ERR);
		qshort = (SDQM_query_short *)qs;
		if (qshort->min == qshort->max)
		    is_value = 1;
		sec_size = qshort->size;
		break;

	    case SDQM_QFT_FLOAT:
		if (qs->size != sizeof (SDQM_query_float))
		    return (SDQM_QUERY_FIELD_SIZE_ERR);
		qf = (SDQM_query_float *)qs;
		if (qf->min == qf->max)
		    is_value = 1;
		sec_size = qf->size;
		break;

	    case SDQM_QFT_STRING:
		if (qs->size <= (int)sizeof (SDQM_query_string))
		    return (SDQM_QUERY_FIELD_SIZE_ERR);
		qstr = (SDQM_query_string *)qs;
		if (qstr->str1_off == qstr->str2_off ||
		    strcmp ((char *)qs + qstr->str1_off, 
				(char *)qs + qstr->str2_off) == 0)
		    is_value = 1;
		sec_size = qstr->size;
		break;

	    /* for user defined types, we call set_fields to get is_value */
	    case SDQM_QFT_USER_DEF:
		is_value = db->qfs[field].set_fields ( 
					qs, qinfo->imin_rec, qinfo->imax_rec);
		sec_size = qs->size;
		break;

	    default:
		return (SDQM_QUERY_BAD_FIELD_TYPE);
	}
	for (k = 0; k < i; k++)
	    if (secs[k].field > field)
		break;
	for (m = i; m > k; m--)
	    secs[m] = secs[m - 1];
	secs[k].field = field;
	secs[k].psec = (SDQM_query_int *)(query + size);
	secs[k].is_value = is_value;
	size += sec_size;
    }

    /* sets the query levels */
    qls = qinfo->qls;
    level = -1;
    field = -1;
    cnt = 0;
    for (i = 0; i < q->n_sections; i++) {
	int f;

	f = secs[i].field;
	if (field == f) {			/* the same field */
	    qls[level].field = f;
	    if (cnt == 0)
		qls[level].first_sec = i;
	    cnt++;
	    qls[level].n_values = cnt;
	}
	else {					/* a new field */
	    field = f;
	    level++;
	    cnt = 0;
	    i--;
	}
    }
    qinfo->n_qls = level + 1;

    qinfo->best_it = -1;
    qinfo->n_ipfs = 0;
    qinfo->icfs->nf = 0;
    qinfo->mcfs->nf = 0;
    qinfo->ucfs->nf = 0;

#ifdef SDQM_TEST
Print_qinfo (qinfo);
#endif

    return (0);
}

/******************************************************************

    Description: This function generates a result message indicating
		query failure.

    Input:	result_type - type of returned results.
		err - error code.

    Output:	results - the query result message.

    Return:	result_type = SDQM_SERIALIZED_RESULT:
		    length of the result message or SDQM_MALLOC_FAILED
		    if malloc failed.
		result_type = SDQM_INDEX_RESULT_ONLY:
		    The error code "err".

******************************************************************/

int SDQM_query_err_ret (int result_type, 
				int err, SDQM_query *q, void **results)
{
    SDQM_query_results *r;

    if (result_type != SDQM_SERIALIZED_RESULT)
	return (err);

    r = (SDQM_query_results *)malloc (sizeof (SDQM_query_results));
    if (r == NULL)
	return (SDQM_MALLOC_FAILED);
    *results = r;

    r->msg_size = sizeof (SDQM_query_results);
    r->err_code = -err;
    r->type = q->type;
    r->n_vf = 0;
    r->n_recs = 0;
    r->n_recs_found = 0;
    r->index_off = 0;
    r->recs_off = r->vf_spec_off= 0;

    return (sizeof (SDQM_query_results));
}

/******************************************************************

    Description: This function generates the query index result.
		This needs to be extended to support table links.

    Input:	qinfo - the query info struct.
		q - pointer to the beginning of the query message.

    Output:	results - the query result message.

    Return:	length of the result message or a negative SDQM 
		error code.

******************************************************************/

int SDQM_generate_index_result (Query_info *qinfo, SDQM_query *q,
					void **results)
{
    SDQM_index_results *r;
    Sdqm_db_t *db;
    int n_recs, n;
    unsigned short *list;
    int size;

    db = qinfo->db;
    n_recs = qinfo->cr_n_recs;

    /* malloc the space */
    if (n_recs > q->max_list_size)
	n = q->max_list_size;
    else
	n = n_recs;
    size = sizeof (SDQM_index_results) + n * sizeof (unsigned short);
    r = (SDQM_index_results *)malloc (size);
    if (r == NULL)
	return (SDQM_MALLOC_FAILED);
    *results = (void *)r;
    list = (unsigned short *)((char *)r + sizeof (SDQM_index_results));

    r->n_recs_found = n_recs;
    r->n_recs = n;
    r->list = list;
    r->rec_size = db->rec_size;
    memcpy (list, qinfo->recl.list, n * sizeof (short));
    r->rec_add = RSIS_get_record_address (db->rsid);

    return (size);
}

/******************************************************************

    Description: This function generates the query results message.
		This needs to be extended to support table links.

    Input:	qinfo - the query info struct.
		q - pointer to the beginning of the query message.
		n_rpq - number records found per query.

    Output:	results - the query result message.

    Return:	length of the result message or SDQM_MALLOC_FAILED
		if malloc failed.

******************************************************************/

int SDQM_generate_results (Query_info *qinfo, SDQM_query *q,
					void **results, int *n_rpq)
{
    Sdqm_db_t *db;
    int n_recs;
    unsigned short *list;
    SDQM_query_results *r;
    SDQM_int n, size, rec_size, i;
    char *rec_add, *recs, *vfpt, *vf_info_org;
    unsigned short *spt;
    Variable_fields_t *vf_info;
    int vf_size, vf_cnt, off;

    db = qinfo->db;
    n_recs = qinfo->cr_n_recs;
    list = qinfo->recl.list;

    if ((vf_size = Get_variable_field_info (qinfo, &vf_info, &vf_cnt)) < 0)
	return (SDQM_query_err_ret (SDQM_SERIALIZED_RESULT, vf_size, q, results));
    vf_info_org = (char *)vf_info;

    /* malloc the space */
    if (n_recs > q->max_list_size)
	n = q->max_list_size;
    else
	n = n_recs;
    size = sizeof (SDQM_query_results) + 
		n * db->rec_size + vf_size + vf_cnt * sizeof (SDQM_int) +
		db->text_len;
    if (q->need_index)
	size += n * sizeof (unsigned short);
    size = ALIGNED_SIZE (size) + 2 * ALIGNED_LENGTH;
	/* extra space here - both vf_spec_off and recs_off must be aligned */
    r = (SDQM_query_results *)malloc (size);
    if (r == NULL)
	return (SDQM_query_err_ret (SDQM_SERIALIZED_RESULT, 
					SDQM_MALLOC_FAILED, q, results));
    *results = r;
    off = sizeof (SDQM_query_results);

    r->type = SDQM_QUERY;
    r->msg_size = size;
    r->rec_size = db->rec_size;
    r->err_code = 0;
    r->n_recs_found = n_recs;
    r->n_recs = n;
    r->tmp_p = n_rpq;
    r->db_info_off = off;
    off += ALIGNED_SIZE (db->text_len);
    memcpy ((char *)r + r->db_info_off, db->text, db->text_len);
    r->recs_off = off;
    off += n * db->rec_size;
    if (q->need_index) {
	r->index_off = off;
	spt = (unsigned short *)((char *)r + r->index_off);
	off += n * sizeof (unsigned short);
    }
    else {
	r->index_off = 0;
	spt = NULL;
    }
    r->n_vf = vf_cnt;
    off = ALIGNED_SIZE (off);
    r->vf_spec_off = off;
    off += vf_cnt * sizeof (SDQM_int);
    for (i = 0; i < vf_cnt; i++)
	((SDQM_int *)((char *)r + r->vf_spec_off))[i] = vf_info[i].foff;

    vfpt = (char *)r + off;
    rec_add = RSIS_get_record_address (db->rsid);
    recs = (char *)r + r->recs_off;
    rec_size = db->rec_size;
    for (i = 0; i < n; i++) {
	memcpy (recs, rec_add + list[i] * rec_size, rec_size);
	if (vf_size > 0) {
	    char *p;

	    p = recs + vf_info->foff;
	    memcpy (vfpt, *((char **)p), vf_info->len);
	    *((int *)p) = vfpt - (char *)r;
	    vfpt += vf_info->len;
	    vf_info++;
	}
	recs += rec_size;
	if (spt != NULL) {
	    *spt = list[i];
	    spt++;
	}
    }

    if (vf_info_org != NULL)
	free (vf_info_org);

    return (size);
}

/******************************************************************

    Description: This function searches the records and find the 
		info about variable size fields. The caller must 
		free "vf_info".

    Input:	qinfo - the query info struct.

    Output:	vf_info - the variable fields info.
		vf_cnt - number of variable fields in the record.

    Return:	size of total variable fields on success or a negative 
		SDQM error number.

******************************************************************/

static int Get_variable_field_info (Query_info *qinfo, 
			Variable_fields_t **vf_info, int *vf_cnt)
{
    Variable_fields_t *vf;
    Sdqm_query_field_t *qfs;
    unsigned short *list;
    char *rec_add;
    int n_qfs, cnt, i, size;

    list = qinfo->recl.list;
    *vf_info = NULL;

    cnt = 0;			/* how many variable size fields? */
    for (i = 0; i < qinfo->db->n_qfs; i++) {
	if (qinfo->db->qfs[i].type == SDQM_QFT_STRING)
	    cnt++;
    }
    *vf_cnt = cnt;
    if (cnt == 0)
	return (0);

    /* malloc space */
    vf = (Variable_fields_t *)malloc (
			cnt * qinfo->cr_n_recs * sizeof (Variable_fields_t));
    if (vf == NULL)
	return (SDQM_MALLOC_FAILED);
    *vf_info = vf;

    rec_add = RSIS_get_record_address (Cr_db->rsid);

    qfs = Cr_db->qfs;
    n_qfs = Cr_db->n_qfs;
    size = 0;
    for (i = 0; i < qinfo->cr_n_recs; i++) {
	char *rec;
	int f;

	rec = rec_add + list[i] * Cr_db->rec_size;
	for (f = 0; f < n_qfs; f++) {
	    if (qfs[f].type == SDQM_QFT_STRING) {
		vf->foff = qfs[f].foff;
		vf->len = strlen (*((char **)(rec + vf->foff))) + 1;
		size += vf->len;
		vf++;
	    }
	}
    }
    return (size);
}

