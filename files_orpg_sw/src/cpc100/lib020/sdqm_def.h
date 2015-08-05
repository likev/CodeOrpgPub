
/***********************************************************************

    Description: private include file for the SDQM module.

***********************************************************************/

/*
* RCS info
* $Author: jing $
* $Locker:  $
* $Date: 2004/03/10 16:29:17 $
* $Id: sdqm_def.h,v 1.8 2004/03/10 16:29:17 jing Exp $
* $Revision: 1.8 $
* $State: Exp $
*/

#ifndef SDQM_DEF_H

#define SDQM_DEF_H

#include <smi.h>
#include <sdqm.h>

enum {SDQM_SIMPLE_QUERY_FIELDS, SDQM_CUSTOM_QUERY_FIELDS};
					/* DB types */

#define MAX_N_COMPARE_FIELDS 32		/* max number of comparison fields;
					   must >= MAX_FIELDS_IN_INDEX */

#define MAX_QUERY_LEVELS	32	/* max number of possible query levels
					   (the query fields in a query
					   message) */

typedef struct {			/* struct specifying comparison fields 
					   */
    int nf;				/* number of comparison fields */
    short type[MAX_N_COMPARE_FIELDS];	/* field type of comparison fields */
    short field[MAX_N_COMPARE_FIELDS];	/* field enums of comparison fields */
    short foff[MAX_N_COMPARE_FIELDS];	/* field offset, in records, of 
					   comparison fields */
} Compare_field_spec;

typedef struct {			/* specification of an indexing table */
    int n_itfs;				/* number of query fields for this 
					   indexing table */
    int itfs[MAX_FIELDS_IN_INDEX];	/* list of the query fields (enums)  
					   for this indexing table */
    Compare_field_spec cfs;		/* compare field spec for this it. used
					   by SDQM_insert */
} Sdqm_query_index_t;

typedef struct {			/* query field attibutes */
    int type;				/* type of the field */
    int indexing;			/* boolean, the field is used in at 
					   least one indexing table */
    int foff;				/* field offset in the record */
    int (*compare)();			/* comparison function; 
					   SDQM_QFT_USER_DEF only */
    int (*set_fields)();		/* set_field function; 
					   SDQM_QFT_USER_DEF only */
} Sdqm_query_field_t;

typedef struct {			/* DB data struct */
    int db_type;			/* DB type */
    int max_records;			/* maximum number of records */
    int rec_size;			/* record size */
    int n_qfs;				/* number of query fields */
    Sdqm_query_field_t *qfs;		/* specs of the query fields */
    int n_its;				/* number indexing tables */
    Sdqm_query_index_t *its;		/* specs for indexing tables */
    char *rsid;				/* RSIS ID */
    int (*compare_func)();		/* comparison function */
    int (*set_rec_func)();		/* records generation func */
    void (*record_byte_swap)();		/* record byte swap func */
    int query_mode;			/* query mode */
    char *text;				/* ASCII text info (not null term) from
					   the server (SDQM_set_sdb_meta) and
					   to be passed to the client:
					   src_t, qf_names, qf_types, dll_name
					   ... */
    int text_len;			/* size of the text info */
    char *lb_name;			/* LB name for this DB */
} Sdqm_db_t;

typedef struct {			/* struct for sorted query sections */
    int field;				/* field enumeration */
    SDQM_query_int *psec;		/* pointer to the section */
    int is_value;			/* boolean, whether this is a value */
} Query_sec_t;

typedef struct {			/* struct for query result */
    int n_recs;				/* number of records in list */
    int array_size;			/* buffer size for list */
    unsigned short *list;		/* the record index list */
} Record_list_t;

typedef struct {			/* struct for query level info */
    int field;				/* query field enumeration */
    int n_values;			/* number of values/ranges for this 
					   level */
    int first_sec;			/* index of the first section in the
					   query message */
    int cr_sec;				/* current section in the query message 
					   */
} Query_level_t;

typedef struct {			/* struct for passing query info */
    Sdqm_db_t *db;			/* the data base */
    char *query;			/* the query message */
    int n_secs;				/* number of sections in the msg */
    Query_sec_t *secs;			/* sorted query sections */
    int n_qls;				/* number of query levels */
    Query_level_t *qls;			/* query level info */
    int best_it;			/* best index of the indexing found */
    char *imin_rec;			/* record with min query value for 
					   subsequent index search */
    char *imax_rec;			/* record with max query value for 
					   subsequent index search */
    char *mmin_rec;			/* record with min query value for 
					   subsequent non-index field match */
    char *mmax_rec;			/* record with max query value for 
					   subsequent non-index field match */
    int n_ipfs;				/* number of query fields that have  
				 	   been processed by indexing search
					   - This field is not used. */
    int cr_n_recs;			/* current number of found records 
					   after both indexing and sequential
					   searches */
    int n_units;			/* number of unit queries processed */
    int max_list_size;			/* max number of records requested */
    int done;				/* boolean: the query is completed */
    Compare_field_spec *icfs;		/* compare field spec for current index
					   search */
    Compare_field_spec *mcfs;		/* compare field spec for current 
					   non-index field match */
    Compare_field_spec *ucfs;		/* compare field spec for testing
					   unique record */
    int min_is_max;			/* min = max, used by current non-index 
					   field match */
    char *rec_add;			/* current record area pointer in DB */
    Record_list_t recl; 		/* query results (record list) */
} Query_info;

typedef struct {			/* temporary struct for moving variable
					   size fields to the query results */
    int foff;				/* field offset */
    int len;				/* field length */
} Variable_fields_t;

typedef struct {			/* struct for registering DB name */
    void *id;				/* SDB ID */
    char name[SDQM_DB_NAME_SIZE];	/* SDB name */
} Sdb_table_t;


void SDQM_set_cr_db (Sdqm_db_t *cr_db);
int SDQM_preprocess_query_msg (Sdqm_db_t *db, char *query, 
						Query_info *qinfo);
int SDQM_generate_results (Query_info *qinfo, SDQM_query *q, 
					void **results, int *n_rpq);
int SDQM_generate_index_result (Query_info *qinfo, SDQM_query *q,
					void **results);
int SDQM_query_err_ret (int result_type, 
			int err, SDQM_query *q, void **results);
Sdqm_db_t *SDQM_get_db_info (char *lb_name, char **db_name);

#endif		/* #ifndef SDQM_DEF_H */


