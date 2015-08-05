
/***********************************************************************

    Description: Internal include file for sdqs - Simple Data Base
    Query Manager.

***********************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/02/04 20:26:39 $
 * $Id: sdqs_def.h,v 1.9 2008/02/04 20:26:39 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */


#ifndef SDQS_DEF_H

#define SDQS_DEF_H

#include <smi.h> 
#include <sdqm_def.h> 

#define GL_STATUS	(0x10000000 | LE_CRITICAL_BIT)
#define GL_ERROR	(0x20000000 | LE_CRITICAL_BIT)
#define GL_INFO		0

#define MAXN_CONNS	32	/* Maximum number of connections to server */

#define GET_MACRO_FUNC_NAME "SDQSD_get_macro"

typedef struct {		/* for a data_input line */
    char *field_name;		/* name of sdb field */
    char *size;			/* size specification */
    char *expression;		/* expression for calculating the sdb field */
} data_input_t;

typedef struct {		/* for specifying an index tree */
    int n_fields;		/* number of sdb record fields */
    char *fields;		/* sdb field names for the tree */
} index_tree_t;

typedef struct {		/* runtime structure for each sdb table */
    int (*Get_record) ();	/* get_record function pointer */
    void (*Byte_swap) ();	/* byte_swap function pointer */
    int (*Get_misc) ();		/* get_misc function pointer */
    int (*Free_record) ();	/* free_record function pointer */
    int (*House_keep) ();	/* house keep function pointer */
    int lb_fd;			/* LB fd */
    LB_id_t msgid;		/* current message id */
    int src_msg_size;		/* size of the part of msg to be read */
    int rec_size;		/* size of the sdb record */
    int maxn_recs;		/* maximum number of records */
    void *db_id;		/* sdqm id */
    int n_recs;			/* current number of records */
    int msgid_field_ind;	/* query field index of "msg_id" */
    int has_saved_recs;		/* msg 1 is used for saved records */
} runtime_t;

/* for table_t.data_endian */
enum {SDQS_SERIAL, SDQS_BIG_ENDIAN, SDQS_LITTLE_ENDIAN, SDQS_LOCAL_ENDIAN};

struct schme_struct {		/* SDB table info */
    char *name;			/* table name */
    char *src_t;		/* source (LB message) struct name */
    char *sdb_t;		/* sdb record struct name */
    char *src_lb;		/* source LB path */
    int n_dis;			/* number of data_input entries */
    data_input_t *dis;		/* list of data_input entries */
    char *data_input_func;	/* custom data input function */
    char *house_keep_func;	/* custom house keep function */
    int n_qfs;			/* number of query fields (include the added
				   msg_id field) */
    char *qfs;			/* query field names ("" for added msg_id) */
    char *types;		/* query field types ("" for added msg_id) */
    int n_its;			/* number of index trees (not include the added
				   IT for msg_id) */
    index_tree_t *its;		/* list of index trees */
    SMI_data_info_t *smi;	/* SMI for sdb_t */
    short no_msg_exp_proc;	/* msg expiration is not processed */
    char data_endian;		/* LB message data format */
    char lb_is_bigendian;	/* the LB is big endian (on big endian host) */
    runtime_t rt;
};
typedef struct schme_struct table_t;


/* for argument "switch" of SDQSC_get_func_name */
enum {GET_RECORD_FUNC, BYTE_SWAP_FUNC, GET_MISC_FUNC, FREE_RECORD_FUNC};

char *SDQSC_get_func_name (int which, table_t *table);
unsigned int SDQSC_check_sum ();
void SDQSC_read_conf_file (char *conf_name);
char *SDQSM_malloc (int size);
void SDQSC_generate_code ();
void SDQSO_init (int no_compilation);
int SDQSC_get_sdb_tables (table_t **tables);
int SDQSO_get_port ();
char *SDQSM_get_port ();
int SDQSO_house_keep (int in_termination_phase);
void SDQSC_convert_chars (int cnt, char *str, char c1, char c2);
char *SDQSM_get_environ_name ();
char *SDQSM_get_shared_lib_name ();
char *SDQSM_get_smi_func_name ();
char *SDQSM_get_shared_lib_basename ();
void SDQSM_set_port_number (char *port);
int SDQSO_get_css_port ();
int SDQS_select (char *lb_name, int mode, int max_n, int is_big_endian,
					char *where, void **result);
void SDQS_save_records ();
int SDQS_select_i (char *db_name, int mode, int max_n, int index_query,
					char *where, void **result);
int SDQSA_is_exclusive (char *rec, int sec);

#endif		/* #ifndef SDQS_DEF_H */
