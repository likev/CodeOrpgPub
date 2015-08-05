/****************************************************************
		
    Internal include file for the SMI (Struct Meta Info) module.

*****************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2003/11/14 16:09:13 $
 * $Id: smipp_def.h,v 1.3 2003/11/14 16:09:13 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef SMIPP_DEF_H
#define SMIPP_DEF_H


#define MIPP_NAME_SIZE 128

struct Smi_struct {		/* struct or type to be processes */
    char *name;			/* name of the struct or type */
    char *major;		/* major ID of the struct or type */
    char *minor;		/* minor ID of the struct or type */
    char *real_name;		/* original name after typedef resolution.
				   Need free. */
    struct Smi_struct *next_id;	/* links to aliased IDs */
    struct Smi_struct *next;	/* the next entry of the linked list */
};

typedef struct Smi_struct Smi_struct_t;

struct Smi_vss_info {		/* SMI VSS comments */
    int token;			/* token index */
    char *size;			/* field array size */
    char *offset;		/* field offset */
    int need_data;		/* size or offset contains this-> */
    struct Smi_vss_info *next;	/* the next entry of the linked list */
};

typedef struct Smi_vss_info Smi_vss_info_t;

struct Smi_vss_size {		/* SMI VSS size comments */
    int token;			/* token index */
    char *size;			/* VSS struct size */
    int need_data;		/* size contains this-> */
    struct Smi_vss_size *next;	/* the next entry of the linked list */
};

typedef struct Smi_vss_size Smi_vss_size_t;

typedef struct {		/* struct of internal meta info */
    char *name;			/* field name */
    char *type;			/* field type */
    int n_items;		/* number of items */
    int size;			/* size of each item */
    int offset;			/* offset of the first item */
    char *ni_exp;		/* n_items expression */
    char *offset_exp;		/* offset expression */
} Smi_field_t;

typedef struct {		/* struct of meta info */
    char *name;			/* name of the struct */
    int size;			/* size of the struct */
    short n_fs;			/* number of fields including VSF */
    short n_vsfs;		/* number of variable size fields */
    int need_data;		/* ni_exp or offset_exp contains this-> */
    int src_line;		/* source line number of the definition */
    char *src_name;		/* source file name of the definition */
    char *vsize;		/* size definition */
    Smi_field_t *fields;	/* pointer to the field array */
} Smi_info_t;

char *SMIM_malloc (int size);

void SMIT_init (char *buf, int *tks, int tkcnt);
void SMIT_free ();
int SMIT_find_type (int st_tkind, int n_tks, char *ret_buf, int buf_size);
int SMIT_find_typedef (char *type, char *ret_buf, int buf_size);
Smi_info_t *SMIT_get_smi (char *name);

int SMIP_tokenize_text (char *inbuf, int size, 
			char **outbuf_p, int **tks_p, int *tkcnt_p);
int SMIP_is_alpha (char c);
int SMIP_is_separator (char c);
void SMIP_add_new_smi_struct (Smi_struct_t *smi);
void SMIP_free ();
Smi_struct_t *SMIP_get_smis ();
void SMIP_error (int tk, const char *format, ...);
void SMIP_warning (int tk, const char *format, ...);
Smi_vss_info_t *SMIP_get_vss (int tk);
int SMIP_get_source_name (int tk, char **name);
char *SMIP_get_function_name ();
Smi_vss_size_t *SMIP_get_vsize (int st_tk, int end_tk);
int SMIP_get_next_tokens (int max_n, char *st, char **tk, int *tk_len);


#endif		/* #ifndef SMIPP_DEF_H */
