

/**************************************************************************

    Global include file for the SMI (Struct Meta Info) module.

**************************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:58:01 $
 * $Id: smi.h,v 1.7 2012/06/14 18:58:01 jing Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef SMIPP_H
#define SMIPP_H

#include <stdio.h>		/* definition of NULL */
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


typedef struct {		/* struct of meta info */
    char *name;			/* field name */
    char *type;			/* field type */
    int n_items;		/* number of items */
    int size;			/* size of each item */
    int offset;			/* offset of the first item */
    void *ci;			/* custom info */
} SMI_field_t;

typedef struct {		/* struct of meta info */
    char *name;			/* name of the struct */
    int size;			/* size of the struct */
    short n_fields;		/* number of fields including VSF */
    short n_vsfs;		/* number of variable size fields */
    SMI_field_t *fields;	/* pointer to the field array */
    void *ci;			/* custom info */
} SMI_info_t;


typedef struct {		/* struct of meta info stored in file */
    int name_off;		/* offset of field name */
    int type_off;		/* offset field type */
    int n_items;		/* number of items */
} SMI_data_field_t;

typedef struct {		/* struct of meta info stored in file */
    int size;			/* size of this variable size struct */
    int name_off;		/* offset of struct name */
    short n_fields;		/* number of fields including VSF */
    short n_vsfs;		/* number of variable size fields */
    SMI_data_field_t fields[1];	/* the field array */
} SMI_data_info_t;

#define SMI_INVALID_INDEX		-479

/* The following is for SMI application library (SMIA) */

#define SMIA_SMI_NOT_FOUND		-480
#define SMIA_DATA_TOO_SHORT		-481
#define SMIA_STRUCT_NOT_SUPPORTED	-482
#define SMIA_MALLOC_FAILED		-483
#define SMIA_SMI_FUNC_UNSET		-485


enum {SMIA_TD_SIGNED, SMIA_TD_UNSIGNED, SMIA_TD_FLOAT};
		/* for type description returned by SMIA_get_type_desc*/

#ifdef __cplusplus
extern "C"
{
#endif

/* The default function for retrieving the SMI */
SMI_info_t *SMI_get_info (char *name, void *data);
char *SMI_get_info_type_by_id (int major, int minor);
int SMI_get_info_get_all_ids (int **majors, int **minors, char ***types);

int SMIA_serialize (char *type, void *c_data, 
				char **serial_data, int c_data_size);
int SMIA_deserialize (char *type, char *serial_data, 
				char **c_data, int serial_data_size);
void *SMIA_set_smi_func (SMI_info_t *(*smi_get_info)(char *, void *));
int SMIA_free_struct (char *type, char *c_data);
int SMIA_bswap_output (char *type, void *data, int data_len);
int SMIA_bswap_input (char *type, void *data, int data_len);
int SMIA_is_pvss_struct (char *type);

int SMIA_go_through_struct (char *type, void *data, int data_len,
	int (*process_a_primitive_field) (SMI_field_t *, void *, int));
int SMIA_get_type_desc (SMI_field_t *fld);
char *SMIA_get_current_type ();

#ifdef __cplusplus
}
#endif

#endif		/* #ifndef SMIPP_H */


