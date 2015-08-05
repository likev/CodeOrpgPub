
/******************************************************************

	file: sdqm_test.c

	Test program for the SDQM module - The simple data base 
	query management.
	
******************************************************************/

/* 
 * RCS info
 * $Author: eddief $
 * $Locker:  $
 * $Date: 2002/05/14 18:53:27 $
 * $Id: sdqm_test.c,v 1.14 2002/05/14 18:53:27 eddief Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 
#include <rpgdbm.h>
#include <sdqm_def.h>

typedef struct {
    unsigned int f1;
    int f2;
    int f3;
    char *f4;
} Record_t;

enum {FIELD_0, FIELD_1, FIELD_2, FIELD_3};

typedef struct {			/* query section for user defined 
					   type: unsigned int */
    SDQM_int size;			/* size of this struct */
    char type;				/* field type enumeration */
    char field;				/* field enumeration */
    short unused;			/* unused */
    unsigned int min;			/* minimum query value */			
    unsigned int max;			/* maximum query value */
} uint_query_sec_t;


#ifndef INCLUDE_TEST_FUNCS

static char Db_name[] = "My_db";
static char *Prod_lb;
static char *Up_lb;

static void Init_db ();
static void Insert_recs ();
static void Query_db ();
static int Print_query_results (SDQM_query_results *results);
int Print_rec (void *p, char *msg);
int Print_table (void *dbpt, int which_it);
static void Rec_byte_swap (char *rec);
static void print_info (sdb_table_info_t *info);
/* static void test_sdb_info (); */

/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv)
{
    SDQM_query_results *results;
    int n_recs, ret;
    char *record, *message;

    Prod_lb = "/export/home/jing/data/pdist/product_data_base.lb";
    Up_lb = "/export/home/jing/data/pdist/user_profile_data_base.lb";

/*    test_sdb_info (); */

    n_recs = SDQ_select (Up_lb, "user_name <= z and msg_id <= 5", (void **)&results);
    if (n_recs < 0) {
	printf ("SDQM_select failed (ret %d)\n", n_recs);
	exit (1);
    }
    printf ("SDQM_select returns %d n_recs_found %d\n", 
					n_recs, results->n_recs_found);
    ret = SDQ_get_record ((void *)results, 0, &record);
    printf ("SDQ_get_record 0 returns %d\n", ret);
    ret = SDQ_get_record ((void *)results, n_recs - 1, &record);
    printf ("SDQ_get_record %d returns %d\n", n_recs - 1, ret);

    ret = SDQ_get_message ((void *)results, 0, &message);
    printf ("SDQ_get_message 0 returns %d\n", ret);
    ret = SDQ_get_message ((void *)results, n_recs - 1, &message);
    printf ("SDQ_get_message %d returns %d\n", n_recs - 1, ret);

/*  ret = SDQ_update (Up_lb, "user_name <= z and msg_id = 555", message, ret);
    printf ("SDQ_update returns %d\n", ret); */

/*  ret = SDQ_insert (Up_lb, "user_name <= z and msg_id < 555", message, ret);
    printf ("SDQ_insert returns %d\n", ret); */

    exit (0);

    Init_db ();
    Insert_recs ();
    Query_db ();

    exit (0);
}

static void print_info (sdb_table_info_t *info) {
    char *name, *type;
    int i;

    printf ("name %s, n_qfs %d, lb_name %s, msg_id_off %d\n", 
		info->name, info->n_qfs, info->lb_name, info->msg_id_off);
    name = info->qf_names;
    type = info->qf_types;
    for (i = 0; i < info->n_qfs; i++) {
	printf ("    -%s-%s-\n", name, type);
	name += strlen (name) + 1;
	type += strlen (type) + 1;
    }
}

/*
static void test_sdb_info () {
    sdb_table_info_t *info;
    int ret;

    SDQ_set_server_address (NULL);

    ret = SDQM_get_sdb_info (Prod_lb, &info);
    if (ret < 0)
	printf ("SDQM_get_sdb_info returns %d\n", ret);
    else
	print_info (info);

    ret = SDQM_get_sdb_info (Up_lb, &info);
    if (ret < 0)
	printf ("SDQM_get_sdb_info returns %d\n", ret);
    else
	print_info (info);

    ret = SDQM_get_sdb_info (Prod_lb, &info);
    if (ret < 0)
	printf ("SDQM_get_sdb_info returns %d\n", ret);
    else
	print_info (info);

    ret = SDQM_get_sdb_info (Up_lb, &info);
    if (ret < 0)
	printf ("SDQM_get_sdb_info returns %d\n", ret);
    else
	print_info (info);

    ret = SDQM_get_sdb_info ("rpg_ufile_d", &info);
    if (ret < 0)
	printf ("SDQM_get_sdb_info returns %d\n", ret);
    else
	print_info (info);
}
*/

static int Compare (void *f1, void *f2)
{
    unsigned int *ip1, *ip2;

    ip1 = (int *)f1;
    ip2 = (int *)f2;
    if (*ip1 < *ip2)
	return (RSIS_LESS);
    else if (*ip1 > *ip2)
	return (RSIS_GREATER);
    else
	return (RSIS_EQUAL);
}

static int Set_fields (void *psec, char *min_f, char *max_f)
{
    uint_query_sec_t *q;

    if (min_f == NULL) {	/* perform byte swapping on psec */
	return (0);
    }

    q = (uint_query_sec_t *)psec;
    *((int *)min_f) = q->min;
    *((int *)max_f) = q->max;
    if (q->min != q->max)
	return (0);
    else
	return (1);
}


/******************************************************************

    Description: Selects an unsigned integer value.

    Input:	field - field enum.
		value - value to select.

******************************************************************/

void Select_uint_value (int field, int value)
{
    uint_query_sec_t qi;

    qi.size = sizeof (uint_query_sec_t);
    qi.field = field;
    qi.type = SDQM_QFT_USER_DEF;
    qi.min = value;
    qi.max = value;
    SDQM_append_qm_buf (qi.size, (char *)&qi);
    return;
}


static void Init_db ()
{
    SDQM_query_field_t qfs[] = { {SDQM_QFT_USER_DEF, 0, Compare, Set_fields},
				 {SDQM_QFT_INT, 4, NULL ,NULL},
				 {SDQM_QFT_INT, 8, NULL ,NULL},
				 {SDQM_QFT_STRING, 12, NULL ,NULL} };
    int it0[] = {FIELD_0, FIELD_1};
    int it1[] = {FIELD_1, FIELD_0, FIELD_3};
    SDQM_query_index_t its[4];
    int ret;

    its[0].n_itfs = sizeof (it0)/sizeof (int);
    its[0].itfs = it0;
    its[1].n_itfs = sizeof (it1)/sizeof (int);
    its[1].itfs = it1;
    ret = SDQM_define_sdb (Db_name, 128, 
		sizeof (Record_t), 4, qfs, 2, its, Rec_byte_swap);
    if (ret < 0) {
	printf ("SDQM_define_sdb faied, ret %d\n", ret);
	exit (1);
    }

    return;
}

static void Insert_recs ()
{
    static Record_t r[] = {{3, 5, 3, "abc"},
			   {3, 1, 3, "bds"},
			   {3, 3, 7, "hggh"},
			   {7, 1, 7, "cggg"},
			   {3, 5, 7, "dj"},
			   {6, 4, 3, "crew"},
			   {3, 4, 4, "dbvc"},
			   {3, 7, 1, "aaa"},
			   {2, 4, 7, "blklklk"}  };
    void *dbpt;
    int i, ret;

    dbpt = SDQM_get_dbid (Db_name);
    if (dbpt == NULL) {
	printf ("SDQM_get_dbid failed\n");
	exit (1);
    }

    for (i = 0; i < 9; i++) {
	ret = SDQM_insert (dbpt, &(r[i]));
	if (ret < 0) {
	    printf ("SDQM_insert failed, i %d, ret %d\n", i, ret);
	    exit (1);
	}
    }
/*
    Print_table (dbpt, 0);
    Print_table (dbpt, 1);
*/
}

static void Query_db ()
{
    SDQM_query_results *results;
    int ret;

    SDQM_query_begin (Db_name, 11);

/*    SDQM_select_int_value (0, 6); */

/*   Select_uint_value (0, 6); */

/*    SDQM_select_string_range (3, "a", "az"); */

/*    SDQM_select_string_range (3, "b", "dz"); */

    SDQM_select_int_range (1, 0, 60); 


    SDQM_set_query_mode (/* SDQM_HIGHEND_SEARCH */ 0 |  SDQM_DISTINCT_FIELD_VALUES);

/*    SDQM_select_int_value (0, 3); */
 /*   Select_uint_value (0, 3);  */

    ret = SDQM_execute_query_local (16, 1, &results);
    if (ret < 0) {
	printf ("SDQM_execute_query_local failed, ret %d\n", ret);
	exit (1);
    }

    Print_query_results (results);
}

static int Print_query_results (SDQM_query_results *results)
{
    int i;

    printf ("Query result header:\n");
    printf ("    size %d is_bigendian %d err_code %d query_number %d\n", 
	results->msg_size, results->is_bigendian, results->err_code, 
	results->query_number);

    printf ("    n_recs_found %d n_recs %d recs_off %d index_off %d\n", 
	results->n_recs_found, results->n_recs, 
	results->recs_off, results->index_off);

    printf ("  n_records_found %d n_records_returned %d query_number %d query_error %d\n", 
		SDQM_get_n_records_found (results), 
		SDQM_get_n_records_returned (results), 
		SDQM_get_query_number (results), 
		SDQM_get_query_error (results));

    printf ("    Records:\n");
    for (i = 0; i < SDQM_get_n_records_returned (results); i++) {
	Record_t *rec;

	if (SDQM_get_query_record (results, i, (void **)&rec))
	    printf ("        %d %d %d %s\n", rec->f1, rec->f2, rec->f3, rec->f4);
    }

    printf ("    Indices: ");
    for (i = 0; i < SDQM_get_n_records_returned (results); i++) {
	int index;

	if (SDQM_get_query_index (results, i, &index))
	    printf ("%d ", index);
    }
    printf ("\n");

    return (0);
}

int Print_irec (void *p)
{
    Record_t *rec;

    rec = (Record_t *)p;
    printf ("	             rec: %d %d %d %s\n", 
			rec->f1, rec->f2, rec->f3, rec->f4);
    return (0);
}

static void Rec_byte_swap (char *rec)
{

}



#else		/* #ifndef INCLUDE_TEST_FUNCS */

/* The following function must be included in sdqm_core.c */

int Print_table (void *dbpt, int which_it)
{
    Sdqm_db_t *db;
    Record_t min, *rec;
    int ind;

    db = (Sdqm_db_t *)dbpt;

    printf ("Print table %d\n", which_it);
    min.f1 = 0;
    min.f2 = 0;
    min.f3 = 0;
    min.f4 = 0;
    ind = RSIS_find (db->rsid, which_it, &min, &rec);
    if (ind > 0)
	printf ("     %d  %d  %d  %s - ind %d\n", 
			rec->f1, rec->f2, rec->f3, rec->f4, ind);

    while ((ind = RSIS_traverse (db->rsid, which_it, 
				RSIS_RIGHT, ind, &rec)) > 0)
	printf ("     %d  %d  %d  %s - ind %d\n", 
			rec->f1, rec->f2, rec->f3, rec->f4, ind);

    return (0);
}

int Print_qinfo (Query_info *qinfo)
{
    int i;

    printf ("Query_info: n_secs %d n_qls %d\n", qinfo->n_secs, qinfo->n_qls);

    printf ("        Sections: \n");
    for (i = 0; i < qinfo->n_secs; i++) {
	Query_sec_t *sec;
	sec = qinfo->secs + i;
	printf ("            field %d psec %p (size %d f %d type %d) is_value %d\n", 
		sec->field, sec->psec, sec->psec->size, sec->psec->field, 
		sec->psec->type, sec->is_value);
    }

    printf ("        Levels: \n");
    for (i = 0; i < qinfo->n_qls; i++) {
	Query_level_t *ql;
	ql = qinfo->qls + i;
	printf ("            field %d n_values %d first_sec %d cr_sec %d\n", 
			ql->field, ql->n_values, ql->first_sec, ql->cr_sec);
    }

     return (0);
}

int Print_cfs (Compare_field_spec *cfs)
{
    int i;

    printf ("              type: ");
    for (i = 0; i < cfs->nf; i++) {
	printf ("%d ", cfs->type[i]);
    }
    printf ("\n");
    printf ("              foff: ");
    for (i = 0; i < cfs->nf; i++) {
	printf ("%d ", cfs->foff[i]);
    }
    printf ("\n");
    return (0);
}

int Print_db (Sdqm_db_t *db)
{
    int i, k;

    printf (" db_type %d max_records %d rec_size %d\n", 
		db->db_type, db->max_records, db->rec_size);

    printf ("    gfs: %d\n", db->n_qfs);
    for (i = 0; i < db->n_qfs; i++) {
	printf ("         i %d   type %d indexing %d foff %d\n", 
		i, db->qfs[i].type, db->qfs[i].indexing, db->qfs[i].foff);
    }

    printf ("    its: %d\n", db->n_its);
    for (i = 0; i < db->n_its; i++) {
	Sdqm_query_index_t *it;

	it = db->its + i;
	printf ("         i %d   n %d  -  ", i, it->n_itfs);
	for (k = 0; k < it->n_itfs; k++)
	    printf ("%d ", it->itfs[k]);
	printf ("\n");
	Print_cfs (&(it->cfs));
    }
    printf (" rsid %p, compare_func %p, set_rec_func %p\n", 
		db->rsid, db->compare_func, db->set_rec_func);
    return (0);
}
#endif		/* #ifndef INCLUDE_TEST_FUNCS */


 
