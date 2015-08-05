
/***********************************************************************

    Description: include file for the SDQM module.

***********************************************************************/

/*
* RCS info
* $Author: jing $
* $Locker:  $
* $Date: 2012/06/14 18:58:00 $
* $Id: sdqm.h,v 1.34 2012/06/14 18:58:00 jing Exp $
* $Revision: 1.34 $
* $State: Exp $
*/

#ifndef SDQM_H

#define SDQM_H

#include <sdq.h>

#define SDQM_DB_NAME_SIZE 32		/* DB name size */
#define SDQ_DEFAULT_PORT 30245		/* SDQ service default port number */

/* request msg types (for SDQM_query.type) */
enum {SDQM_QUERY};

/* query field type values */
enum {SDQM_QFT_INT, SDQM_QFT_SHORT, SDQM_QFT_STRING, SDQM_QFT_FLOAT, 
	SDQM_QFT_USER_DEF};		/* SDQM_QFT_USER_DEF must be the
					   last one */

#define MAX_FIELDS_IN_INDEX	 16	/* max possble number of fields in
					   an indexing table definition */

typedef struct {			/* query field attibutes */
    int type;				/* type of the field */
    int foff;				/* field offset in the record */
    int (*compare)();			/* comparison function; 
					   SDQM_QFT_USER_DEFINED type only */
    int (*set_fields)();		/* set_field function; 
					   SDQM_QFT_USER_DEFINED type only */
} SDQM_query_field_t;

typedef struct {		/* specification of an indexing table */
    int n_itfs;			/* number of query fields for this table */
    int *itfs;				/* list of the query fields (enums)  
					   for this indexing table */
} SDQM_query_index_t;

typedef struct {			/* query definition for SDQM_QFT_INT */
    SDQM_int size;			/* size of this struct */
    char type;				/* field type enumeration */
    char field;				/* field enumeration */
    short unused;			/* unused */
    SDQM_int min;			/* minimum query value */	
    SDQM_int max;			/* maximum query value */
} SDQM_query_int;

typedef struct {			/* query definition for SDQM_QFT_INT */
    SDQM_int size;			/* size of this struct */
    char type;				/* field type enumeration */
    char field;				/* field enumeration */
    short unused;			/* unused */
    SDQM_short min;			/* minimum query value */	
    SDQM_short max;			/* maximum query value */
} SDQM_query_short;

typedef struct {		/* query definition for SDQM_QFT_FLOAT */
    SDQM_int size;			/* size of this struct */
    char type;				/* field type enumeration */
    char field;				/* field enumeration */
    short unused;			/* unused */
    SDQM_float min;			/* minimum query value */
    SDQM_float max;			/* maximum query value */
} SDQM_query_float;

typedef struct {			/* query definition for 
					   SDQM_QFT_STRING */
    SDQM_int size;		/* size of this struct including the string */
    char type;				/* field type enumeration */
    char field;				/* field enumeration */
    short unused;			/* unused */
    SDQM_short str1_off;	/* offset of NULL terminated query string 1 */
    SDQM_short str2_off;		/* offset of NULL terminated query 
					   string 2. If str1_off == str2_off,
					   the same string. */
} SDQM_query_string;

typedef struct {			/* struct specifying a single query */
    SDQM_int size;			/* size of this simple query, struct 
					   plus following data for sections */
    SDQM_int n_sections;	/* number of query sections specified */
} SDQM_single_query;			/* query section data follow this 
					   struct. Each section is represented 
					   by SDQM_query_int, SDQM_query_string 
					   and so on. */

typedef struct {			/* struct specifying a query */
    SDQM_int msg_size;			/* size of the entire message */
    char type;				/* message type */
    char is_bigendian;			/* no longer used */
    char need_index;			/* index list is also needed */
    char query_mode;			/* query mode bit field */
    char db_name[SDQM_DB_NAME_SIZE];	/* DB name */
    SDQM_int max_list_size;		/* max size of the returned results */
    SDQM_int n_queries;		/* number of simple queries specified */
} SDQM_query;				/* simple queries data follow this 
					   struct. Each section is started with
					   with SDQM_single_query. */

/* values for argument "result_type" of SDQM_process_query () */
enum {SDQM_INDEX_RESULT_ONLY, SDQM_SERIALIZED_RESULT};

int SDQM_define_sdb (char *db_name, int max_records, int rec_size, 
		int n_qfs, SDQM_query_field_t *qfs, 
		int n_its, SDQM_query_index_t *its,
		void (*record_byte_swap)());
void *SDQM_get_dbid (char *db_name);
int SDQM_set_sdb_meta (char *db_name,  
				char *lb_name, int text_len, char *text);
int SDQM_remove (char *db_name);
int SDQM_process_query (char *query, int len, int result_type, void **results);

void SDQM_select_int_value (int field, int value);
void SDQM_select_short_value (int field, short value);
void SDQM_select_short_range (int field, short min, short max);
void SDQM_select_float_value (int field, float value);
void SDQM_select_float_range (int field, float min, float max);
void SDQM_select_string_value (int field, char *str);
void SDQM_select_string_range (int field, char *str1, char *str2);
void SDQM_query_next ();
void SDQM_query_end ();
int SDQM_execute_query_local (int max_list_size, 
			int need_index, SDQM_query_results **results);

void SDQM_append_qm_buf (int size, char *data);
void SDQM_set_query_mode (int mode);


void SDQM_query_begin (char *db_name);
void SDQM_select_int_range (int field, int min, int max);
int SDQM_execute_query_index (int max_list_size, SDQM_index_results **results);
int SDQM_insert (void *db_id, void *rec);
int SDQM_delete (void *db_id, int ind);


#endif		/* #ifndef SDQM_H */

