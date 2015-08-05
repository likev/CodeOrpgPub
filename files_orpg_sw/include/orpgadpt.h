/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/01/15 15:13:35 $
 * $Id: orpgadpt.h,v 1.38 2008/01/15 15:13:35 jing Exp $
 * $Revision: 1.38 $
 * $State: Exp $
 */
/**************************************************************************

      Module: orpgadpt.h

 Description: ORPG adaptation data API

       Notes: This is included by orpg.h.

 **************************************************************************/

#ifndef ORPGADPT_H
#define ORPGADPT_H

#include <orpg.h>
#include <orpgdat.h>
#include <le.h>
#include <orpgerr.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum { ORPGADPT_CHANGE_EVENT, ORPGADPT_REMOVE_EVENT} orpgadpt_event_type_t;
typedef int OC_BOOL;
typedef void* propobj_t;
typedef int LONG_T;
typedef char CHAR_T;
typedef char BYTE_T;
typedef short SHORT_T;
typedef unsigned short USHORT_T;
typedef unsigned int ULONG_T;
typedef float FLOAT_T;
typedef int oc_data_type_t;
typedef unsigned int oc_access_t;
#define TRUE 1
#define FALSE 0
#define PROP_CLEAR_ERROR 0x0002
#define PROP_REPORT_DETAILS 0x0001

typedef struct {
    int data_id;	/*  ORPG data store id */
    orpgadpt_event_type_t event;
			/*  Type of event, change event or remove event */
    char* name;	/*  Name of the message/object that has changed */
    void* user_data;	/*  User Data passed in on registration */
} orpgadpt_event_t;

typedef void (*orpgadpt_chg_func_ptr_t)(orpgadpt_event_t* event);

/**  Causes ORPGADPT_log_last_error and ORPGADPT_last_error_text to report any low level details associated with the error */
#define ORPGADPT_REPORT_DETAILS		0x0001

/**  Causes PROP_log_last_error	and PROP_last_error_text to clear the last error message after their work is done */
#define ORPGADPT_CLEAR_ERROR		0x0002

/**  Constants for the lock_command parameter of the ORPGADPT_lock function */
#define ORPGADPT_EXCLUSIVE_LOCK		1
#define ORPGADPT_SHARED_LOCK		2
#define ORPGADPT_TEST_EXCLUSIVE_LOCK	3
#define	ORPGADPT_TEST_SHARED_LOCK	4
#define ORPGADPT_UNLOCK			5

/*  Adaptation data version, this version number should be changed any time the binary structure
	of any adaptation message changes */
#define ORPGADPT_DATA_VERSION		RPG_BUILD_VERSION

#define ADPTU_NAME_SIZE 128

#ifdef SUNOS
#define ND_CFG_DIR "/tftpboot"
#else
#define ND_CFG_DIR "/tftpboot"
#endif

char *ORPGADPTU_find_archive_name (char *src_dir, char *date, char *time_str, 
	char *site_name, char *node_name, int ver, int *n_files, char **files);

int ORPGADPTU_install_adapt (char *src_dir, char *s_date, 
					char *s_time, int need_file_move);
char *ORPGADPTU_save_adapt ();
int ORPGADPTU_restore_1 (char *sitep, char *cfgp, int buf_size);
int ORPGADPTU_restore_2 (char *fname);
char *ORPGADPTU_create_archive_name (char *node, 
				char *site, int ver, time_t tm);
char *ORPGADPTU_install_dir (char *node, char *site, int chan, int ver);


int ORPGADPT_get_data(const char* name, void* data, int data_len);

int ORPGADPT_set_data(const char* name, void* data, int data_len, const char* class_name);

int ORPGADPT_get_data_length(const char* name);

propobj_t ORPGADPT_get_propobj(const char* name);

int ORPGADPT_get_msg_id(const char* name);

int ORPGADPT_lock(int lock_command, const char* msg_name);

OC_BOOL ORPGADPT_set_propobj(const char* name, propobj_t prop_obj);

OC_BOOL ORPGADPT_on_change(const char* name, orpgadpt_chg_func_ptr_t change_callback, void* user_data);

OC_BOOL ORPGADPT_select_data_store(int data_store_id);

OC_BOOL ORPGADPT_restore_baseline(const char* name, OC_BOOL replace_message);

char* ORPGADPT_get_archive_name(const char* dir, const char* date, const char* time, const char* site_name, const char* host_name);

OC_BOOL ORPGADPT_save(const char* destination_dir, const char* site_name, char* orpg_host_name, char* alternate_host_name, char** save_archive_name);

int ORPGADPT_restore(const char* dir, const char* date, const char* time, const char* site_name, char* orpg_host_name, char* alt_host_name, char** restore_archive_name);

int ORPGADPT_install(const char* dir, const char* date, const char* time, const char* site_name, char* orpg_host_name, char* alt_host_name, char** install_archive_name);

OC_BOOL ORPGADPT_update_baseline(const char* name, OC_BOOL replace_message);

OC_BOOL ORPGADPT_update_legacy_color_tables(const char* name, propobj_t prop_obj);

OC_BOOL ORPGADPT_refresh_legacy(int verbose_level);

/**  This function is obsolete  */
/**  Select the current adaptation data source, default is ORPGDAT_ADAPT  */
OC_BOOL ORPGADPT_select_data_store(int data_store_id);

OC_BOOL ORPGADPT_log_last_error(int le_code, int flogs);

OC_BOOL ORPGADPT_last_error_text(char* error_buf, int error_buf_len, int flags);
/**  Return TRUE if an error has occurred, and has not been cleared */
OC_BOOL ORPGADPT_error_occurred();

OC_BOOL ORPGADPT_clear_error();

long PROP_get_long (void *prop_obj, const char *prop_name, long array_index);
short PROP_get_short (void *prop_obj, 
				const char *prop_name, long array_index);
int PROP_get_string (void *prop_obj, const char *prop_name, 
				long array_index, char *buf, long str_len);
int PROP_get_string_i (void *prop_obj, long prop_index, 
			long array_index, char *buf, long str_len);
void *PROP_meta_props_i (void *prop_obj, long prop_index);
int PROP_no_of_elements (void *prop_obj, const char* prop_name);
int PROP_count (void *prop_obj);
const char *PROP_name (void *prop_obj, long prop_index);
const char *PROP_long_name (void *prop_obj, long prop_index);
int PROP_set_string (void *prop_obj, const char *prop_name, 
			long array_index, const char *str_value);
int PROP_set_string_i (void *prop_obj, long prop_index, 
			long array_index, const char *str_value);
int PROP_last_error_text (char *error_buf, int error_buf_len, 
					int flags, int wrap_length);
int PROP_log_last_error (int le_code, int flags);
int PROP_print (void *obj, FILE *file);
int PROP_error_occurred ();
void PROP_dec_ref (void *prop_obj);

LONG_T PROP_find(propobj_t prop_obj, const char* prop_name);
int PROP_init(propobj_t obj);
oc_data_type_t PROP_get_data_type(propobj_t obj, const char* prop_name);
oc_data_type_t PROP_get_data_type_i(propobj_t obj, LONG_T prop_index);

void PROP_clear_error();
const char* PROP_description(propobj_t obj, LONG_T prop_index);
int PROP_no_of_elements_i(propobj_t obj, LONG_T prop_index);
int PROP_access_is_i(propobj_t obj, LONG_T prop_id, oc_access_t access_bits);
int PROP_access_is(propobj_t obj, const char* prop_name, oc_access_t access_bits);
CHAR_T PROP_get_char(propobj_t obj, const char* prop_name, LONG_T array_index);
CHAR_T PROP_get_char_i(propobj_t obj, LONG_T prop_index, LONG_T array_index);
BYTE_T PROP_get_byte(propobj_t obj, const char* prop_name, LONG_T array_index);
BYTE_T PROP_get_byte_i(propobj_t obj, LONG_T prop_index, LONG_T array_index);
SHORT_T PROP_get_short_i(propobj_t obj, LONG_T prop_index, LONG_T array_index);
USHORT_T PROP_get_ushort(propobj_t obj, const char* prop_name, LONG_T array_index);
USHORT_T PROP_get_ushort_i(propobj_t obj, LONG_T prop_index, LONG_T array_index);
LONG_T PROP_get_long_i(propobj_t obj, LONG_T prop_index, LONG_T array_index);
ULONG_T PROP_get_ulong(propobj_t obj, const char* prop_name, LONG_T array_index);
ULONG_T PROP_get_ulong_i(propobj_t obj, LONG_T prop_index, LONG_T array_index);
FLOAT_T PROP_get_float(propobj_t obj, const char* prop_name, LONG_T array_index);
FLOAT_T PROP_get_float_i(propobj_t obj, LONG_T prop_index, LONG_T array_index);
int PROP_set_char(propobj_t obj, const char* prop_name, LONG_T array_index, CHAR_T value);
int PROP_set_char_i(propobj_t obj, LONG_T prop_index, LONG_T array_index, CHAR_T value);
int PROP_set_byte(propobj_t obj, const char* prop_name, LONG_T array_index, BYTE_T value);
int PROP_set_byte_i(propobj_t obj, LONG_T prop_index, LONG_T array_index, BYTE_T value);
int PROP_set_short(propobj_t obj, const char* prop_name, LONG_T array_index, SHORT_T value);
int PROP_set_short_i(propobj_t obj, LONG_T prop_index, LONG_T array_index, SHORT_T value);
int PROP_set_ushort(propobj_t obj, const char* prop_name, LONG_T array_index, USHORT_T value);
int PROP_set_ushort_i(propobj_t obj, LONG_T prop_index, LONG_T array_index, USHORT_T value);
int PROP_set_long(propobj_t obj, const char* prop_name, LONG_T array_index, LONG_T value);
int PROP_set_long_i(propobj_t obj, LONG_T prop_index, LONG_T array_index, LONG_T value);
int PROP_set_ulong(propobj_t obj, const char* prop_name, LONG_T array_index, ULONG_T value);
int PROP_set_ulong_i(propobj_t obj, LONG_T prop_index, LONG_T array_index, ULONG_T value);
int PROP_set_float(propobj_t obj, const char* prop_name, LONG_T array_index, FLOAT_T value);
int PROP_set_float_i(propobj_t obj, LONG_T prop_index, LONG_T array_index, FLOAT_T value);
propobj_t PROP_meta_props(propobj_t obj, const char* prop_name);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef ORPGADPT_H DO NOT REMOVE! */
