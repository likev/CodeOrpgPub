
/***********************************************************************

    ftc is a tool that helps porting FORTRAN programs. This is the 
    include file.

***********************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/11/02 21:30:13 $
 * $Id: ftc_def.h,v 1.4 2010/11/02 21:30:13 jing Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  

#ifndef FTC_DEF_H
#define FTC_DEF_H

#define MAX_STR_SIZE 512
#define LARGE_STR_SIZE (MAX_STR_SIZE * 8)
#define MAX_TKS 512

enum {T_OTHERS, T_INT, T_REAL, T_DOUBLE, T_CHAR, T_VOID, T_UNKNOWN};
				/* LOGICAL, INTEGER*4 are INT */
#define P_PARAM (0x1)
#define P_COMMON (0x2)
#define P_FUNC_DEF (0x4)
#define P_FUNC_CALL (0x8)
#define P_EQUIV (0x10)
#define P_GLOB (0x20)
#define P_USED (0x40)
#define P_MODIFIED (0x80)
#define P_CVT_POINT (0x100)
#define P_DONE (0x200)
#define P_PBR_PARM (0x400)	/* PARAM to pass to func by reference */
#define P_BY_REF (0x800)
#define P_LOW_BOUND (0x1000)	/* lower dimentsion bound specified */

typedef struct {
    char *id;		/* identifier name */
    char type;		/* type - T_OTHERS ... */
    char ndim;		/* number of dimensions */
    char size;		/* size of the type (INT, CHAR) */
    char arg_i;		/* index as an func arg; -1 is not an arg. */
    short prop;		/* property bit flags */
    char n_pass_to;	/* size of pass_to */
    char *param;	/* PARAMETER value */
    char *dimp;		/* dimension description */
    char *common;	/* common block name */
    char *equiv;	/* EQUIVALENCE discription */
    char *c_id;		/* identifier name for c */
    char *pass_to;	/* list of null-term strings of "func_name, arg_ind" */
    void *meq;		/* more fields used by equiv processing */
} Ident_table_t;

typedef struct {
    char *name;		/* name of the block */
    int n_vars;		/* number of variales */
    int pad;		/* size of the padding variable */
    char *vars;		/* variable list - null-terminated strings */
    int new;
} Common_block_t;

enum {L_TYPE, L_PARAM, L_ROUTINE, L_COMMON, L_EQUIV, L_DATA, L_FORMAT, L_BLOCK_DATA, L_OTHERS};

#define FVS_RESET ((void *)0)
#define FVS_NEXT ((void *)1)

/* special values for line lable */
#define L_NOT_FOUND -1
#define L_ENDIF -2
#define L_ENDDO -3

char *FM_tk_malloc (int size);
void FM_tk_free ();
char *FM_get_task_name ();
char *FM_process_expr (char *inp, int size, char *outbuf);

int FF_open_file (char *name, int *sizep, int *nlinesp);
int FF_get_line (int fd, int line, char *buf, int b_size);
void FF_output_text (char *name, char *text, int len);
void FF_outout_file_header (char *out_file, char *text);
char *FF_file_name (int file);
void FF_set_main (char *name);

void FP_process_cfile (char *in_file, char *out_file);
char *FP_get_prologue (int cfile, int st_l, int end_l, int is_c);
int FP_process_comment (char *buf, int cnt, int is_c);
void FP_str_replace (char *buf, int off, int n_bytes, char *str);
int FP_get_tokens (char *text, int mnt, char **tks, int *tof);
int FP_is_char (char c);
int FP_is_digit (char c);
void FP_put_keys (char **keys);

void FC_process_ffile (char *in_file, char *out_file);
char *FC_get_ctype_dec (Ident_table_t *id);
int FC_line ();
int FC_is_main ();
void FC_process_expr (char **tks, int n_tks, char **otb, int def);

Ident_table_t *FS_var_search (char *var);
char *FS_first_scan (int ffile, int *ln, int n_lines);
int FS_get_a_line (int lcnt, char *buf, int bsizse);
int FS_get_ftn_tokens (char *ftn, int n, char **tks, int *tof);
int FS_is_goto_label (int label);
int FS_get_cr_lable ();
int FS_get_size_dim (char **tks, int nt, int st, int *size, 
					int *dim_st, int *dim_l, int *ndim);
int FS_get_cbs (Common_block_t **cbsp);
void FS_conver_to_c_id (char *buf, Ident_table_t *id);
void FS_exception_exit (const char *format, ...);
char *FS_get_format (int label);
int FS_get_value (char *var);
void FS_update_common_padding (Ident_table_t *id, int pad);

void FG_search_templates (char *templ_f, int n_inp_files, char *inp_files);
int FG_is_pb_reference (char *func, int aind, char *type);
void FG_read_templates (char *task_name);
int FG_is_task_func (char *func);
char *FG_get_next_used_func_templ (int next);
int FG_read_c_content (char *fname, char **buf);
int FG_get_c_line (char *cont, int off, char *buf, int b_size);
char *FG_get_c_text (char **tks);
void FG_add_inc_func (char *line);
void FG_check_used_funcs ();

void FI_save_globals ();
void FI_update_include ();

void FE_change_var_def (char *c_text);
void FE_save_eq (char *buf, char *var1, char *var2);
void FE_init ();
void FE_process_eq ();
int FE_get_eq_code (char **code);
int FE_get_pad_size (Ident_table_t *id);
int FE_evaluate_int_expr (char *expr, int err);

int FR_is_reserved_word (char *id);
int FR_preprocess (char *ftn, int n, char **tks, int *tofp);
int FR_is_intrinsic (char *name);
void FR_init ();
int FR_parse_char_opr (char **tks, int ind, int nt, 
				char *offp, char *sizep, int *nopr);
int FR_tb_implemented ();
int FR_is_var_name_allowed (char *name);
int FR_is_replace_var (char *name);

int FW_process_write (char **tks, int n_tks, char **otb);
char *FW_get_err_msg ();

int Print_tks (char *lbl, char **tks, int n);
int FU_next_token (int st, int nt, char **tks, char target);
void FU_replace_tks (char **tks, int st, int no, char **ntks, int nn);
void FU_ins_tks (char **tks, int ind, ...);
void FU_update_tk (char **tk, char *str);
int FU_get_a_field (char **tks, int nt, int ind, int *st, int *end);
char *FU_get_c_text (char **tks, int nt);
char *FU_get_c_type (Ident_table_t *id);
void FU_post_process_c_tks (char **tks);
char *FU_get_arg_c_type (Ident_table_t *id, char *func, int n_arg,
						char *c_dimp);
void FU_get_call_cast (char *type, char *ltype, int is_array_name, char **tb);
char *FU_common_struct_name (char *common);

#endif		/* #ifndef FTC_DEF_H */
