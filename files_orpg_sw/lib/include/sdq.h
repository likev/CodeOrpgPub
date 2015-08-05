
/***********************************************************************

    Description: public include file for the SDQ module.

***********************************************************************/

/*
* RCS info
* $Author: jing $
* $Locker:  $
* $Date: 2005/09/14 14:37:55 $
* $Id: sdq.h,v 1.5 2005/09/14 14:37:55 jing Exp $
* $Revision: 1.5 $
* $State: Exp $
*/

#ifndef SDQ_H

#define SDQ_H


#include <infr.h>
#include <float.h>			/* for FLT_MIN */

#define SDQM_UNDEF_INT (int)0x80000000	/* undefined int value */
#define SDQM_UNDEF_SHORT (short)0x8000	/* undefined short value */
#define SDQM_UNDEF_FLOAT FLT_MIN	/* undefined float value */
#define SDQM_UNDEF_STRING "~"		/* undefined string value. We use a
					   valid string, instead of NULL, for 
					   convenient implementation in SDQM. 
					   This is the maximum of any valid 
					   string. We return this in query 
					   results for undefined fields. */

typedef int SDQM_int;			/* integer type used by SDQM */
typedef short SDQM_short;		/* short integer type used by SDQM */
typedef float SDQM_float;		/* float type used by SDQM */
typedef char * SDQM_string;		/* string type used by SDQM */

typedef struct {		/* query result: type SDQM_QUERY, result_type 
						SDQM_SERIALIZED_RESULT */
    SDQM_int msg_size;		/* total size of result message */
    char is_bigendian;		/* no longer used */
    unsigned char err_code;	/* error code (minus SDQM error number) */
    char n_vf;			/* number of varible size fields */
    char type;			/* SDQM_QUERY */
    SDQM_int db_info_off;	/* offset where db info starts */
    SDQM_int rec_size;		/* record size */
    SDQM_int n_recs_found;	/* number of records found */
    SDQM_int n_recs;		/* number of records in this msg */
    SDQM_int recs_off;		/* offset where records start */
    SDQM_int index_off;		/* offset where index list starts; If 0 no 
						index list. */
    SDQM_int vf_spec_off;	/* offset where the variable field spec
					(field offset in record) starts */
    void *tmp_p;		/* temporary pointer. */
} SDQM_query_results;		/* result data follow this struct */


/* bit field used for SDQM_query.query_mode */
#define SDQM_ALL_MATCH		0x1	/* Field match mode */
#define SDQM_EXACT_MATCH	0	/* Field match mode */
#define SDQM_FULL_SEARCH	0x2	/* Partial search mode */
#define SDQM_PARTIAL_SEARCH	0	/* Partial search mode */
#define SDQM_HIGHEND_SEARCH	0x4	/* Search direction */
#define SDQM_LOWEND_SEARCH	0	/* Search direction */
#define SDQM_DISTINCT_FIELD_VALUES 0x8	/* distinct query field values */
#define SDQM_ALL_FIELD_VALUES	0	/* all query field values */

/* SDQM error code */
#define SDQM_MALLOC_FAILED	-140
#define SDQM_QUERY_MSG_LEN_ERR	-141
#define SDQM_QUERY_LEN_ERR	-142
#define SDQM_QUERY_FIELD_ERR	-143
#define SDQM_QUERY_FIELD_SIZE_ERR	-144
#define SDQM_QUERY_BAD_FIELD_TYPE	-145
#define SDQM_QUERY_TOO_MANY_COMPARE_FIELDS	-146
#define SDQM_BAD_FIELD_IN_INDEXING_TABLE	-147
#define SDQM_DUPLICATED_FIELD_IN_INDEXING_TABLE	-148
#define SDQM_RSIS_INIT_FAILED	-149

#define SDQM_BAD_ARGUMENT	-150
#define SDQM_BAD_QUERY_FIELD_TYPE	-151
#define SDQM_NOT_IMPLEMENTED	-152
#define SDQM_QUERY_TYPE_ERR	-153
#define SDQM_BAD_QUERY_FIELD_OFFSET	-154
#define SDQM_QUERY_FIELD_NOT_ALIGNED	-155
#define SDQM_BAD_USER_DEF_TYPE_FUNC	-156
#define SDQM_ALREADY_DEFINED		-157
#define SDQM_DB_NAME_NOT_FOUND		-158
#define SDQM_QUERY_HD_ERR		-159

#define SDQM_PARSE_ERROR		-139
#define SDQM_BAD_FIELD_VALUE		-138
#define SDQM_BAD_FIELD_TYPE		-137
#define SDQM_TEXT_TOO_LONG		-136
#define SDQM_FIELD_NOT_FOUND		-135
#define SDQM_SYNTAX_ERROR		-134
#define SDQM_RELATION_NOT_SUPPORTED	-133
#define SDQM_BUFFER_OVERFLOW		-132
#define SDQM_OPEN_QUOTATION		-131

#define SDQM_BAD_HOST_NAME		-130
#define SDQM_RECORD_NOT_FOUND		-129
#define SDQM_BAD_RESULT			-128
#define SDQM_MSG_ID_NOT_FOUND		-127
#define SDQM_MSG_AMBIGUOUS		-126
#define SDQM_DLOPEN_FAILED		-125
#define SDQM_DLSYM_FAILED		-124
#define SDQM_TOO_MANY_SERVERS		-123

#define SDQ_REQ_SYNT_ERROR		-122

#ifdef __cplusplus
extern "C"
{
#endif

char *SDQM_get_full_lb_name (char *name, unsigned int *ipp);

int SDQ_select (char *lb_name, char *where, void **result);
int SDQ_update (char *lb_name, char *where, void *message, int msg_len);
int SDQ_insert (char *lb_name, void *message, int msg_len);
int SDQ_delete (char *lb_name, char *where);
int SDQ_lock (char *lb_name, char *where, int action);
int SDQ_get_record (void *result, int n, char **record);
int SDQ_get_message (void *result, int n, char **message);
void SDQ_set_query_mode (int mode);
void SDQ_set_maximum_records (int maxn_records);

int SDQ_get_n_records_found (void *r);
int SDQ_get_query_error (void *r);
int SDQ_get_n_records_returned (void *r);
int SDQ_get_query_record (void *r, int n, void **rec);
int SDQ_get_query_index (void *r, int n, int *ind);
int SDQ_get_msg_id (void *result, int ind, LB_id_t *msg_id);

#ifdef __cplusplus
}
#endif

/* The following is used by writing RPG custom server modules - Otherwise they
   do not need to be public. */

/* values for argument "state" of the sdqs user house keeping function */
enum {SDQM_HK_INIT, SDQM_HK_BUILD, SDQM_HK_NEW_RECORD, 
				SDQM_HK_ROUTINE, SDQM_HK_TERM};

typedef struct {		/* query result: type SDQM_QUERY, result_type
						SDQM_INDEX_RESULT_ONLY */
    int query_number;		/* the query number (as in SDQM_query) */
    int n_recs_found;		/* number of records found */
    int n_recs;			/* number of records in this msg */
    int rec_size;		/* size of records */
    unsigned short *list;	/* index list */
    void *rec_add;		/* record base address */
} SDQM_index_results;		/* result sections follow this struct */


#endif		/* #ifndef SDQ_H */

