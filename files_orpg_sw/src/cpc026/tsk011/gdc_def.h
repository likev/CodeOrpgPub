
/***********************************************************************

    Description: Internal include file for gdc.

***********************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/05/03 16:27:34 $
 * $Id: gdc_def.h,v 1.9 2011/05/03 16:27:34 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#ifndef GDC_DEF_H
#define GDC_DEF_H

#include <stdio.h>
#include <infr.h>

#define MAX_STR_SIZE 256	/* This must be at least 128 bytes */

typedef struct {
    char *name;
    char *value;
    int type;
} Variable_t;
enum {VAR_GLOBAL, VAR_LOCAL, VAR_DATA};	/* values for Variable_t.type */

enum {ASN_C, CND_C, PAS_C, COM_C, IGN_C, LB_C, RB_C, FORMAT_C, INDENT_C, KEEP_BLANK_C};
#define GDC_CONT_CHARS "=:^!#{}f1o\0"
enum {GDCR_NO_ERROR, GDCR_NOT_FOUND, GDCR_FILE_MISSING, GDCR_SYNTAX_ERROR};

int GDCM_process_f_option (char *optarg, char *cchars, char *msg);
void GDCM_gdc_control (char *v_name, char *value);
void GDCM_strlcat (char *s1, char *s2, int size);
void GDCM_add_dir (char *dir, char *name, int size);
void GDCM_strlcpy (char *s1, char *s2, int size);
int GDCM_stoken (char *text, char *cntl, int ind, char *buf, int size);
int GDCM_ftoken (char *text, char *cntl, int ind, char *buf, int size);

void GDCR_align_columns (char *fname);
char *GDCR_get_data_value (char *full_path);
int GDCR_get_error (char **msg);
void GDCR_set_delimiter (char *fname, char delimiter);

char *GDCE_process_expression (char *inp, int size, int arith, char *outbuf);
void GDCE_set_field_delimiter (char delimiter);
char GDCE_get_field_delimiter ();
int GDCE_is_identifier (char *str, int n_bytes);

void GDCP_exception_exit (const char *format, ...);
int GDCP_is_include ();
void GDCP_process_template (char *text, int size, char *fname, 
					char ccs[], FILE *fout);
char *GDCP_evaluate_variables (char *text);
void GDCP_gdc_control (char *v_name, char *value);
void GDCP_read_include_file (char *name, int optional);
char *GDCP_get_ccs ();
void GDCP_set_done ();

void GDCF_share_vars (char *out_fname, FILE *fout, char *dest_dir, 
		char *link_path, char *shared_dir, int to_install);
int GDCF_set_install_path (int is_link, char *inst);
int GDCF_install_file ();
int GDCF_read_file (char *name, char **buf_p);
int GDCF_copy_from_file (char *fname, int binary, char *ccs, int optional);
void GDCF_gdc_control (char *v_name, char *value);
int GDCF_read_fr_site (char *params);
void GDCF_process_nmt_data (int n_sites, char *site_names, char *input_file);

void GDCV_set_variable (char *v_name, char *value);
char *GDCV_get_value (char *var);
int GDCV_is_required (char *var);
int GDCV_match_wild_card (char *str, char *pat);
int GDCV_get_variables (Variable_t **vars);
void GDCV_discard_variables ();
int GDCV_is_local_var (char *v_name);
void GDCV_get_var_range (int *st_ind, int *n_vs);
void GDCV_set_var_range (int st_ind, int n_vs);
void GDCV_check_condition_reserved_word (int yes);
int GDCV_rm_duplicated_strings (char *strs, int cnt);
int GDCV_get_var_from_env (char *var, char *buf, int size);


#endif		/* #ifndef GDC_DEF_H */

