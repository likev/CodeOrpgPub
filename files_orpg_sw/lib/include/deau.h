/*******************************************************************

    Public header file for the data element attribute utility library 
    module.

*******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/25 14:01:50 $
 * $Id: deau.h,v 1.21 2014/03/25 14:01:50 steves Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */


#ifndef DEAU_H
#define DEAU_H

/* attribute types */
enum {DEAU_AT_ID = 0, DEAU_AT_NAME, DEAU_AT_TYPE, DEAU_AT_RANGE, 
      DEAU_AT_VALUE, DEAU_AT_UNIT, DEAU_AT_DEFAULT, DEAU_AT_ENUM, 
      DEAU_AT_DESCRIPTION, DEAU_AT_CONVERSION, DEAU_AT_EXCEPTION, 
      DEAU_AT_ACCURACY, DEAU_AT_PERMISSION, DEAU_AT_BASELINE, 
      DEAU_AT_MISC, DEAU_AT_N_ATS};

typedef struct {		/* for text format attributes */
    char *ats[DEAU_AT_N_ATS];	/* never be NULL; empty string if undefined */
} DEAU_attr_t;

#define DEAU_NAME_SIZE	128

/* !!! Do not change this unless you have checked all dependent code */
enum {				/* data attribute types */
    DEAU_T_UNDEFINED, DEAU_T_INT, DEAU_T_SHORT, DEAU_T_BYTE, DEAU_T_UINT, 
	DEAU_T_USHORT, DEAU_T_UBYTE, DEAU_T_BIT, DEAU_T_FLOAT, DEAU_T_DOUBLE, 
	DEAU_T_STRING, DEAU_T_BOOLEAN};

#define DEAU_MALLOC_FAILED		-560
#define DEAU_BAD_ATTR_TOKEN		-561
#define DEAU_BAD_ATTR_SPEC		-562
#define DEAU_BAD_TYPE_NAME		-563
#define DEAU_TYPE_TOO_LONG		-564
#define DEAU_DATA_TOO_SHORT		-565
#define DEAU_BAD_ATTR_NAME		-566
#define DEAU_NAME_TOO_LONG		-567
#define DEAU_BAD_TABLE_IDS		-568
#define DEAU_LB_UNDEFINED		-569
#define DEAU_BAD_DEA_MSG		-570
#define DEAU_HASH_TABLE_NOT_FOUND	-571
#define DEAU_BAD_HASH_TABLE		-572
#define DEAU_ID_TABLE_NOT_FOUND		-573
#define DEAU_BAD_ID_TABLE		-574
#define DEAU_DE_NOT_FOUND		-575
#define DEAU_BAD_NUMERICAL		-576
#define DEAU_BUFFER_TOO_SMALL		-577
#define DEAU_BAD_ARGUMENT		-578
#define DEAU_DEFAULT_NOT_FOUND		-579
#define DEAU_ATTR_RESPECIFIED		-580
#define DEAU_BAD_VALUE_IN_DEA_FILE	-581
#define DEAU_SPEC_NOT_IMPLEMENTED	-582
#define DEAU_BAD_EXTERNAL_REF		-583
#define DEAU_BAD_ENUM			-584
#define DEAU_BAD_ID			-585
#define DEAU_BAD_RANGE			-586
#define DEAU_BAD_CONVERSION		-587
#define DEAU_SYNTAX_ERROR		-588

#ifdef __cplusplus
extern "C"
{
#endif

void DEAU_set_error_func (void (*err_func) (char *));
int DEAU_use_attribute_file (char *fname, int override);
int DEAU_check_data_range (char *name, int data_type, 
					int array_size, char *data);
int DEAU_set_func_pt (char *func_name, void *func);

int DEAU_check_struct_range (char *type, void *data, int data_len);
int DEAU_get_number_of_checked_fields ();
void DEAU_LB_name (char *name);
int DEAU_create_dea_db ();

int DEAU_get_attr_by_id (char *id, DEAU_attr_t **at);
int DEAU_get_next_dea (char **id, DEAU_attr_t **at);
void DEAU_free_tables ();
int DEAU_update_hash_tables ();
int DEAU_special_suffix (char *suff);
int DEAU_update_attr (char *id, int which_attr, char *text_attr);
int DEAU_use_default_values (char *id, char *site, int force);
int DEAU_check_permission (DEAU_attr_t *at, char *user_id);
int DEAU_get_number_of_values (char *id);
int DEAU_get_number_of_baseline_values (char *id);
int DEAU_get_values (char *id, double *values, int buf_size);
int DEAU_get_string_values (char *id, char **p);
int DEAU_get_branch_names (char *node, char **p);
int DEAU_get_baseline_values (char *id, double *values, int buf_size);
int DEAU_get_baseline_string_values (char *id, char **p);
int DEAU_move_baseline (char *id, int to_baseline);
int DEAU_set_values (char *id, int str_type, void *values, 
					int n_items, int base_line);
int DEAU_get_data_type (DEAU_attr_t *at);
int DEAU_get_allowable_values (DEAU_attr_t *at, char **p);
int DEAU_check_attributes (DEAU_attr_t *attr);
int DEAU_UN_register (char *de_group, 
		void (*notify_func)(int, EN_id_t, int, void *));
int DEAU_read_listed_attrs (char *id_list);
int DEAU_write_listed_attrs (char *id_list, int *attr_type, char *attrs);
void DEAU_cache_need_update ();
void DEAU_set_read_attr_type (int attr, int yes);
int DEAU_write_listed_remote (char *lb_name, char *id_list, 
					int *which_attr, char *attrs);
int DEAU_read_listed_remote (char *lb_name, char *id_list, 
					int attr_mask, char **result);
char *DEAU_values_to_text (int str_type, void *values, int n_items);
void DEAU_delete_cache ();
int DEAU_lock_de (char *id, int lock_cmd);
int DEAU_get_msg_id (char *id);
int DEAU_remote_lock_de (char *id, int lock_cmd);
int DEAU_get_editable_alg_names (int all, char **names);
int DEAU_get_editable_alg_names_remote (char *lb_name, int all, char **names);
int DEAU_get_min_max_values (DEAU_attr_t *attr, char **p);
int DEAU_get_enum (DEAU_attr_t *attr, char *str, int *enu, int buf_s);
int DEAU_get_dea_by_msgid (LB_id_t msgid, DEAU_attr_t *de);
int DEAU_update_dea_db (char *remote_host, char *lb_name, LB_id_t msg_id);
int DEAU_set_binary_value (char *id, void *bytes, int n_bytes, int baseline);
int DEAU_get_binary_value (char *id, char **p, int baseline);
int DEAU_uuencode (unsigned char *barray, int size, unsigned char **strp);
int DEAU_rm_add_des (int n_files, char *files, int n_rm, char *ids);

#ifdef __cplusplus
}
#endif

#endif			/* #ifndef DEAU_H */
