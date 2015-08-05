
/**************************************************************************

    SMI application library module.

**************************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:58:01 $
 * $Id: smi_app.c,v 1.11 2012/06/14 18:58:01 jing Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <misc.h>
#include <smi.h>

typedef struct {		/* struct for storing type info */
    char *type;			/* type name */
    SMI_info_t *smi;		/* SMI for the type */
    int stype;			/* struct type for the type */
} type_table_t;

typedef struct {		/* local SMI per-field info */
    char pvsst;			/* pvssf type */
    char is_sizeof;		/* is a sizeof_* field */
    char type_desc;
    char is_primitive;		/* size of the type if the type is primitive,
				   or 0 if not a primitive */
    char *otype;		/* original type */
    int osize;			/* original size */
} Local_finfo_t;

static int Bytes_used = 0;	/* number of bytes used in deserialization */
static char *Serial_data;	/* data source for deserialization */
static int Serial_data_size;	/* size of deserialization data source */

static SMI_info_t *(*Smi_get_info)(char *, void *) = NULL;
				/* function for getting SMI */

enum {NOT_A_PVSSF = 0, EXPLICIT_SIZE_PVSSF, NULL_TERM_PVSSF};
				/* values for Local_finfo_t.pvsst and return 
				   values of Get_pvssf_type */
enum {POINTERLESS_STRUCT = 0, PVSS_STRUCT, NON_POINTER_VSS};
				/* return values of Get_struct_type */

static int (*Process_a_primitive_field) (SMI_field_t *, void *, int);
static char *Current_type = "";	/* current user-defined type */


/* local functions */
static int Get_struct_type (SMI_info_t *smi);
static int Is_primitive (char *type, int *t_desc);
static int Return_data (char **data);
static int Append_data (char *data, int data_len);
static int Serialize (SMI_info_t *smi, void *c_data);
static int Deserial_bytes (char *to_p, int size);
static int Deserialize (SMI_info_t *smi, char *c_data);
static int Free_struct (SMI_info_t *smi, void *c_data);
static int Get_smi_and_stype (char *type, void *data, 
				SMI_info_t **smip, int *stypep);
static int Cmp_type (void *e1, void *e2);
static int Get_pvssf_type (SMI_info_t *smi, int f_ind);
static int Get_pvss_n_items (SMI_info_t *smi, int f_ind, char *data);
static int Bswap_primitive_field (SMI_field_t *fld, void *data, int data_len);
static int Process_fields (SMI_info_t *smi, void *data, int data_len,
		int (*process_primitive_field) (SMI_field_t *, void *, int),
		int (*process_user_defined_field) (char *, void *, int));
static int Go_through_struct (char *type, void *data, int data_len);

/************************************************************************

    Sets the function for getting the SMI info. This must be called
    before calling any of the SMIA functions. It returns the previous
    version of the function.

************************************************************************/

void *SMIA_set_smi_func (SMI_info_t *(*smi_get_info)(char *, void *)) {
    void *prev = Smi_get_info;
    if (smi_get_info != NULL)
	Smi_get_info = smi_get_info;
    return (prev);
}

/************************************************************************

    Returns TRUE (non-zero) if type "type" is a PVSS struct or FALSE (zero) 
    otherwise. It returns an negative error code on failure. A PVSS 
    structure needs serialization.

************************************************************************/

int SMIA_is_pvss_struct (char *type) {
    SMI_info_t *smi;
    int stype, ret;

    ret = Get_smi_and_stype (type, NULL, &smi, &stype);
    if (ret < 0)
	return (ret);

    if (stype != PVSS_STRUCT)	/* normal C struct or with non-pointer VSS */
	return (0);
    else
	return (1);
}

/************************************************************************

    Frees PVSS struct "c_data" created by SMIA_deserialize.

************************************************************************/

int SMIA_free_struct (char *type, char *c_data) {
    SMI_info_t *smi;
    int stype, ret;

    ret = Get_smi_and_stype (type, NULL, &smi, &stype);
    if (ret < 0)
	return (ret);

    if (stype != PVSS_STRUCT) {	/* normal C struct or with non-pointer VSS */
	free (c_data);
	return (0);
    }

    ret = Free_struct (smi, c_data);
    if (ret < 0)
	return (ret);
    free (c_data);

    return (0);
}

/************************************************************************

    Recursively frees PVSS pointers.

************************************************************************/

static int Free_struct (SMI_info_t *smi, void *c_data) {
    int ret, i;

    for (i = 0; i < smi->n_fields; i++) {
	SMI_field_t *fld;
	char *type, *fbuf;
	SMI_info_t *fsmi;
	int n_items, stype;
	Local_finfo_t *fi;

	fld = smi->fields + i;
	fi = (Local_finfo_t *)(fld->ci);
	if (fi->pvsst == NOT_A_PVSSF && fi->is_primitive)
	    continue;

	if (fi->pvsst != NOT_A_PVSSF) {
	    type = fi->otype;
	    fbuf = *((char **)((char *)c_data + fld->offset));
	    if (fi->is_primitive) {
		free (fbuf);
		continue;
	    }
	    n_items = Get_pvss_n_items (smi, i, (char *)c_data);
	}
	else {
	    type = fld->type;
	    n_items = fld->n_items;
	    fbuf = (char *)c_data + fld->offset;
	}

	ret = Get_smi_and_stype (type, NULL, &fsmi, &stype);
	if (ret < 0)
	    return (ret);

	if (stype == PVSS_STRUCT) {
	    int f;

	    for (f = 0; f < n_items; f++) {
		ret = Free_struct (fsmi, fbuf + f * fsmi->size);
		if (ret < 0)
		    return (ret);
	    }
	}
	if (fbuf != (char *)c_data + fld->offset)
	    free (fbuf);
    }
    return (0);
}

/************************************************************************

    Serializes VSS C struct "c_data" of type "type". The serialized
    data is returned in "serial_data". The caller should free the
    returned pointer if it is not NULL. SMIA_serialize returns the
    size of the serialized data on success or a negative error code.
    If "type" is a non-pvss struct, it returns "c_data" itself as the
    serialized data. Thus, if *serial_data = c_data, we know the
    struct is non-pvss and the returned pointer should not be freed.

************************************************************************/

int SMIA_serialize (char *type, void *c_data, 
				char **serial_data, int c_data_size) {
    SMI_info_t *smi;
    int stype, ret;

    ret = Get_smi_and_stype (type, NULL, &smi, &stype);
    if (ret < 0)
	return (ret);

    if (stype != NON_POINTER_VSS)
	c_data_size = 0;

    if (stype != PVSS_STRUCT) {/* normal C struct or with non-pointer VSS */
	*serial_data = (char *)c_data;
	if (stype == NON_POINTER_VSS)
	    return (c_data_size);
	return (smi->size);
    }

    ret = Serialize (smi, c_data);
    if (ret < 0) {
	Return_data (NULL);
	return (ret);
    }

    return (Return_data (serial_data));
}

/************************************************************************

    Recursively serializes VSS C struct "c_data" of type "smi". The 
    serialized data is appended to the buffer. Note that for non-pvss
    C struct fields, we simply store the binary image of the struct.
    Returns 0 on success or a negative error code.

************************************************************************/

static int Serialize (SMI_info_t *smi, void *c_data) {
    int ret, i;

    for (i = 0; i < smi->n_fields; i++) {
	SMI_field_t *fld;
	char *type, *fbuf;
	SMI_info_t *fsmi;
	int n_items, s, stype;
	Local_finfo_t *fi;

	fld = smi->fields + i;
	fi = (Local_finfo_t *)(fld->ci);
	if (fi->pvsst == NOT_A_PVSSF && fi->is_primitive) {
	    if (fi->is_sizeof) {
		int t = *((int *)((char *)c_data + fld->offset));
#ifdef LITTLE_ENDIAN_MACHINE
		t = INT_BSWAP (t);
#endif
		ret = Append_data ((char *)&t, sizeof (int));
	    }
	    else
		ret = Append_data ((char *)c_data + fld->offset, 
					fld->size * fld->n_items);
	    if (ret < 0)
		return (ret);
	    continue;
	}

	if (fi->pvsst != NOT_A_PVSSF) {
	    n_items = Get_pvss_n_items (smi, i, (char *)c_data);
	    type = fi->otype;
	    fbuf = *((char **)((char *)c_data + fld->offset));
	    if ((s = fi->is_primitive)) {
		ret = Append_data (fbuf, s * n_items);
		if (ret < 0)
		    return (ret);
		continue;
	    }
	}
	else {
	    type = fld->type;
	    n_items = fld->n_items;
	    fbuf = (char *)c_data + fld->offset;
	}

	ret = Get_smi_and_stype (type, NULL, &fsmi, &stype);
	if (ret < 0)
	    return (ret);

	if (stype == PVSS_STRUCT) {
	    int f;

	    for (f = 0; f < n_items; f++) {
		ret = Serialize (fsmi, fbuf + f * fsmi->size);
		if (ret < 0)
		    return (ret);
	    }
	}
	else {
	    ret = Append_data (fbuf, fsmi->size * n_items);
	    if (ret < 0)
		return (ret);
	}
    }
    return (0);
}

/************************************************************************

    Deserializes "serial_data" of length "serial_data_size" to create
    VSS C struct of "type". The created struct is returned with
    "c_data". The caller should free the returned pointer (and any
    pointer inside the struct) if it is not NULL. SMIA_deserialize
    returns the number of bytes used in "serial_data" on success or a
    negative error code. If "type" is a non-pvss struct, it returns
    "serial_data" itself as the C sturct. Thus, if *c_data =
    serial_data, we know the struct is non-pvss and the returned
    pointer should not be freed.

************************************************************************/

int SMIA_deserialize (char *type, char *serial_data, 
				char **c_data, int serial_data_size) {
    SMI_info_t *smi;
    int stype, ret, swap_size;
    char *buf;

    ret = Get_smi_and_stype (type, NULL, &smi, &stype);
    if (ret < 0)
	return (ret);

    if (stype != PVSS_STRUCT) {	/* normal C struct or non-pointer VSS */
	if (serial_data_size < smi->size)
	    return (SMIA_DATA_TOO_SHORT);
	*c_data = serial_data;
	swap_size = serial_data_size;
	if (stype == NON_POINTER_VSS)
	    Bytes_used = serial_data_size;
	else
	    Bytes_used = smi->size;
    }
    else {		/* PVSS struct */
	Bytes_used = 0;
	Serial_data = serial_data;
	Serial_data_size = serial_data_size;
	buf = (char *)malloc (smi->size);
	if (buf == NULL) 
	    return (SMIA_MALLOC_FAILED);
	ret = Deserialize (smi, buf);
	if (ret < 0)
	    return (ret);
	*c_data = buf;
	swap_size = 0;		/* we don't need check and cannot check */
    }

    return (Bytes_used);
}

/************************************************************************

    Recursively deserializes data at Serial_data + Bytes_used of type
    "smi" and generates C structure in "c_data". The available
    serialized data are Serial_data_size - Bytes_used bytes. Note that
    for non-pvss C struct fields, we simply store the binary image of
    the struct. Returns 0 on success or a negative error code on failure.

************************************************************************/

static int Deserialize (SMI_info_t *smi, char *c_data) {
    int ret, i;

    for (i = 0; i < smi->n_fields; i++) {
	SMI_field_t *fld;
	char *type, *fbuf;
	SMI_info_t *fsmi;
	int n_items, s, stype;
	Local_finfo_t *fi;

	fld = smi->fields + i;
	fi = (Local_finfo_t *)(fld->ci);
	if (fi->pvsst == NOT_A_PVSSF && fi->is_primitive) {
	    ret = Deserial_bytes (c_data + fld->offset, 
					fld->size * fld->n_items);
	    if (ret < 0)
		return (ret);
#ifdef LITTLE_ENDIAN_MACHINE
	    if (fi->is_sizeof) {
		int *ip = (int *)(c_data + fld->offset);
		*ip = INT_BSWAP (*ip);
	    }
#endif
	    continue;
	}

	n_items = 0;		/* not necessary - turn off gcc warning */
	if (fi->pvsst == EXPLICIT_SIZE_PVSSF) {
	    n_items = Get_pvss_n_items (smi, i, c_data);
	}
	else if (fi->pvsst == NULL_TERM_PVSSF) {
	    char *cpt, *end;
	    cpt = Serial_data + Bytes_used;
	    end = Serial_data + Serial_data_size;
	    while (cpt < end && *cpt != '\0')
		cpt++;
	    if (cpt >= end)
		return (SMIA_DATA_TOO_SHORT);
	    n_items = cpt - (Serial_data + Bytes_used) + 1;
	}
	if (fi->pvsst != NOT_A_PVSSF) {
	    type = fi->otype;
	    fbuf = NULL;
	    if ((s = fi->is_primitive)) {
		fbuf = (char *)malloc (s * n_items);
		if (fbuf == NULL)
		    return (SMIA_MALLOC_FAILED);
		ret = Deserial_bytes (fbuf, s * n_items);
		if (ret < 0)
		    return (ret);
		*((char **)(c_data + fld->offset)) = fbuf;
		continue;
	    }
	}
	else {
	    type = fld->type;
	    n_items = fld->n_items;
	    fbuf = (char *)c_data + fld->offset;
	}

	ret = Get_smi_and_stype (type, NULL, &fsmi, &stype);
	if (ret < 0)
	    return (ret);

	if (fbuf == NULL) {
	    fbuf = (char *)malloc (fsmi->size * n_items);
	    if (fbuf == NULL)
		return (SMIA_MALLOC_FAILED);
	    *((char **)(c_data + fld->offset)) = fbuf;
	}
	if (stype == PVSS_STRUCT) {
	    int f;

	    for (f = 0; f < n_items; f++) {
		ret = Deserialize (fsmi, fbuf + f * fsmi->size);
		if (ret < 0)
		    return (ret);
	    }
	}
	else {
	    ret = Deserial_bytes (fbuf, fsmi->size * n_items);
	    if (ret < 0)
		return (ret);
	}
    }
    return (0);
}

/************************************************************************

    Copies "size" bytes to pointer "to_p" from the data to be deserialized.

************************************************************************/

static int Deserial_bytes (char *to_p, int size) {

    if (Bytes_used + size > Serial_data_size)
	return (SMIA_DATA_TOO_SHORT);
    memcpy (to_p, Serial_data + Bytes_used, size);
    Bytes_used += size;
    return (0);
}

/************************************************************************

    Gets the SMI and struct type of "type" with data "data". The results
    are returned if the corresponding pointer is not NULL. Returns 0
    on success or a negative error code. Note that SMI is data dependent
    if stype = NON_POINTER_VSS. If the struct is not a non-pointer VSS,
    we check all the fields and add local per field info (Local_finfo_t)
    in SMI. If a non-PVSS pointer field is found, it returns an error.
    For non-pointer VSS, we do this only if data is not NULL. The SMI
    is stored for efficient access later. This function first searches
    the stored SMI. This makes all functions in this module very
    effcient.

************************************************************************/

static int Get_smi_and_stype (char *type, void *data, 
				SMI_info_t **smip, int *stypep) {
    static void *type_tblid;
    static type_table_t *types;
    static int n_types = 0;
    type_table_t item;
    SMI_info_t *smi;
    int stype, ind;

    if (type_tblid == NULL &&
	(type_tblid = MISC_open_table (sizeof (type_table_t), 
			32, 1, &n_types, (char **)&types)) == NULL)
	return (SMIA_MALLOC_FAILED);

    item.type = type;		/* search for stored type */
    if (MISC_table_search (type_tblid, &item, Cmp_type, &ind)) {
	if (data == NULL || types[ind].stype != NON_POINTER_VSS) {
	    if (smip != NULL)
		*smip = types[ind].smi;
	    if (stypep != NULL)
		*stypep = types[ind].stype;
	    return (0);
	}
    }
    else
	ind = -1;		/* not found in stored types */

    if (Smi_get_info == NULL)
	return (SMIA_SMI_FUNC_UNSET);
    smi = Smi_get_info (type, data);
    if (smi == NULL) {
	MISC_log ("Smi_get_info %s failed\n", type);
	return (SMIA_SMI_NOT_FOUND);
    }

    stype = -1;
    if ((smi->n_vsfs == 0 || data != NULL) && ind < 0) {
	int found_pvssf, i;
	if ((item.type = (char *)malloc (strlen (type) + 1)) == NULL)
	    return (SMIA_MALLOC_FAILED);
	strcpy (item.type, type);
	item.smi = smi;
	found_pvssf = 0;
	for (i = 0 ; i < smi->n_fields; i++) {
	    Local_finfo_t *fi;
	    int pvssf_type, len, type_desc;

	    pvssf_type = Get_pvssf_type (smi, i);
	    if (pvssf_type < 0)
		return (pvssf_type);
	    if (pvssf_type > 0)
		found_pvssf = 1;
	    len = strlen (smi->fields[i].type);
	    fi = (Local_finfo_t *)malloc (sizeof (Local_finfo_t) + len + 1);
	    if (fi == NULL)
		return (SMIA_MALLOC_FAILED);
	    fi->pvsst = pvssf_type;
	    fi->otype = (char *)fi + sizeof (Local_finfo_t);
	    strcpy (fi->otype, smi->fields[i].type);
	    if (pvssf_type != NOT_A_PVSSF)
		fi->otype[len - 2] = '\0';
	    fi->is_primitive = Is_primitive (fi->otype, &type_desc);
	    fi->type_desc = type_desc;
	    fi->is_sizeof = 0;
	    fi->osize = smi->fields[i].size;
	    smi->fields[i].ci = fi;
	    if (pvssf_type == EXPLICIT_SIZE_PVSSF)
		((Local_finfo_t *)smi->fields[i - 1].ci)->is_sizeof = 1;
	}

	stype = Get_struct_type (smi);
	if (stype < 0)
	    return (stype);
	if (found_pvssf && stype == NON_POINTER_VSS)
	    return (SMIA_STRUCT_NOT_SUPPORTED);
	item.stype = stype;
	if (MISC_table_insert (type_tblid, (void *)&item, Cmp_type) < 0)
	    return (SMIA_MALLOC_FAILED);
    }

    if (stype < 0 &&
	(stype = Get_struct_type (smi)) < 0)
	return (stype);
    if (smip != NULL)
	*smip = smi;
    if (stypep != NULL)
	*stypep = stype;
    return (0);
}

/************************************************************************

    Type comparison function.

************************************************************************/

static int Cmp_type (void *e1, void *e2) {
    type_table_t *f1, *f2;
    f1 = (type_table_t *)e1;
    f2 = (type_table_t *)e2;
    return (strcmp (f1->type, f2->type));
}

/************************************************************************

    Returns respectively PVSS_STRUCT, NON_POINTER_VSS or
    POINTERLESS_STRUCT if "smi" is a PVSS (Pointer Variable Size
    Struct), a non-pointer VSS or a normal C struct that does not
    contain any pointer field. It returns a negative error code
    otherwise.

************************************************************************/

static int Get_struct_type (SMI_info_t *smi) {
    int struct_type, i, ret;

    if (smi == NULL)
	return (SMIA_SMI_NOT_FOUND);
    if (smi->n_vsfs > 0)
	return (NON_POINTER_VSS);

    struct_type = POINTERLESS_STRUCT;
    for (i = 0; i < smi->n_fields; i++) {
	Local_finfo_t *fi;

	fi = (Local_finfo_t *)(smi->fields[i].ci);
	if (fi->pvsst != NOT_A_PVSSF)
	    struct_type = PVSS_STRUCT;
	if (!fi->is_primitive) {
	    int stype;

	    ret = Get_smi_and_stype (fi->otype, NULL, NULL, &stype);
	    if (ret < 0)
		return (ret);

	    if (stype == PVSS_STRUCT)
		struct_type = PVSS_STRUCT;
	}
    }
    return (struct_type);
}

/**********************************************************************

    Checks if the "f_ind"-th field in struct "smi" is a PVSS pointer 
    field. It returns NOT_A_PVSSF if it is not a pointer. It returns
    EXPLICIT_SIZE_PVSSF if it is an explicitly sized psvv or
    NULL_TERM_PVSSF if it is a psvv of NULL terminated char *. It
    returns a negative error code if the field is other types of
    pointer.

**********************************************************************/

static int Get_pvssf_type (SMI_info_t *smi, int f_ind) {
    SMI_field_t *fld;
    char *cpt;

    fld = smi->fields + f_ind;
    cpt = fld->type;
    while (*cpt != '*' && *cpt != '\0')
	cpt++;
    if (*cpt == '\0')		/* no * found */
	return (NOT_A_PVSSF);
    if (cpt[1] != '\0' || fld->n_items > 1)
	return (SMIA_STRUCT_NOT_SUPPORTED);
    if (f_ind > 0 &&
	strncmp (smi->fields[f_ind - 1].name, "sizeof_", 7) == 0 &&
	strcmp (smi->fields[f_ind - 1].name + 7, fld->name) == 0 &&
	strcmp (smi->fields[f_ind - 1].type, "int") == 0) {
	return (EXPLICIT_SIZE_PVSSF);
    }
    else if (strcmp (fld->type, "char *") == 0) {
	return (NULL_TERM_PVSSF);
    }
    else
	return (SMIA_STRUCT_NOT_SUPPORTED);
}

/**********************************************************************

    Returns the number of items of a PVSS field. It returns 0 if it is
    not a PVSS field.

**********************************************************************/

static int Get_pvss_n_items (SMI_info_t *smi, int f_ind, char *data) {
    Local_finfo_t *fi;

    fi = (Local_finfo_t *)(smi->fields[f_ind].ci);
    if (fi->pvsst == EXPLICIT_SIZE_PVSSF)
	return (*((int *)(data + smi->fields[f_ind - 1].offset)));
    else if (fi->pvsst == NULL_TERM_PVSSF)
	return (strlen (*((char **)(data + smi->fields[f_ind].offset))) + 1);
    else
	return (0);
}

/************************************************************************

    Returns the type description that stored in "fld".

************************************************************************/

int SMIA_get_type_desc (SMI_field_t *fld) {
    Local_finfo_t *fi;
    fi = (Local_finfo_t *)fld->ci;
    return (fi->type_desc);
}

/************************************************************************

    Returns the "Current_type" variable so the caller of 
    SMIA_go_through_struct can access the current type info.

************************************************************************/

char *SMIA_get_current_type () { 
    return (Current_type);
}

/************************************************************************

    This function goes through all fields of "data" of "type" and calls 
    "process_a_primitive_field" if a field is a primitive field. 
    "data_len", if non-zero, is the size of "data". This returns 0 on 
    success or a negative error code.

************************************************************************/

int SMIA_go_through_struct (char *type, void *data, int data_len,
	int (*process_a_primitive_field) (SMI_field_t *, void *, int)) {

    Process_a_primitive_field = process_a_primitive_field;
    return (Go_through_struct (type, data, data_len));
}

/************************************************************************

    This function goes through all fields of "data" of "type" and calls 
    "Process_a_primitive_field" if a field is a primitive field. 
    "data_len", if non-zero, is the size of "data". This returns 0 on 
    success or a negative error code.

************************************************************************/

static int Go_through_struct (char *type, void *data, int data_len) {
    static int call_level = 0;
    char *prev_type;
    SMI_info_t *smi;
    int ret;

    if (call_level == 0)
	Current_type = "";
    prev_type = Current_type;
    Current_type = type;
    call_level++;

    ret = Get_smi_and_stype (type, data, &smi, NULL);
    if (ret >= 0)
	ret = Process_fields (smi, data, data_len, 
		Process_a_primitive_field, Go_through_struct);
    Current_type = prev_type;
    call_level--;
    if (ret < 0)
	return (ret);
    return (0);
}

/************************************************************************

    Performs byte swap on "data" of type "type". "data_len", if non-zero,
    is the size of "data". The input data is in local (right) byte orfer. 
    It returns the size of "type" on success or a negative error code.

************************************************************************/

int SMIA_bswap_output (char *type, void *data, int data_len) {
    SMI_info_t *smi;
    int ret;

    ret = Get_smi_and_stype (type, data, &smi, NULL);
    if (ret < 0)
	return (ret);

    if ((ret = Process_fields (smi, data, data_len, 
			Bswap_primitive_field, SMIA_bswap_output)) < 0)
	return (ret);

    return (smi->size);
}

/************************************************************************

    Performs byte swap on "data" of type "type". "data_len", if non-zero,
    is the size of "data". The input data is in external (wrong) byte 
    order. It returns the size of "type" on success or a negative error 
    code.

************************************************************************/

int SMIA_bswap_input (char *type, void *data, int data_len) {
    int ret;

    if (Smi_get_info == NULL)
	return (SMIA_SMI_FUNC_UNSET);

    Smi_get_info (NULL, (void *)1);	/* set data byte swap */
    ret = SMIA_bswap_output (type, data, data_len);
    Smi_get_info (NULL, (void *)0);	/* reset data byte swap */

    return (ret);
}

/************************************************************************

    Performs generic processing of all fields on "data" of type "smi" 
    and length "data_len". It calls "process_primitive_field" to do the 
    actual job on a primitive field. It calls "process_user_defined_field" 
    to process a field of a user-defined type. This is ofter a recursive 
    call. Both "process_user_defined_field" and "process_primitive_field" 
    return a negative number on faulure. This function returns 0 on 
    success or a negative error code. Note that it is save here regardless 
    the fact that "smi" is a static buffer because struct can not contains 
    itself recursively. 

************************************************************************/

static int Process_fields (SMI_info_t *smi, void *data, int data_len,
		int (*process_primitive_field) (SMI_field_t *, void *, int),
		int (*process_user_defined_field) (char *, void *, int)) {
    int ret, i;

    for (i = 0; i < smi->n_fields; i++) {
	SMI_field_t *fld;
	char *type, *fbuf;
	int n_items, s, d_len, f, off, size;
	Local_finfo_t *fi;

	fld = smi->fields + i;
	fi = (Local_finfo_t *)(fld->ci);
	if (fi->is_sizeof)		/* sizeof_ field is not swapped */
	    continue;
	if (fi->pvsst == NOT_A_PVSSF && fi->is_primitive) {
						/* a primitive type */
	    if ((ret = process_primitive_field (fld, data, data_len)) < 0)
		return (ret);
	    continue;
	}

	if (fi->pvsst != NOT_A_PVSSF) {		/* pointer types */
	    n_items = Get_pvss_n_items (smi, i, (char *)data);
	    type = fi->otype;
	    fbuf = *((char **)((char *)data + fld->offset));
	    if ((s = fi->is_primitive)) {
		SMI_field_t t;

		t.name = fld->name;
		t.type = type;
		t.offset = 0;
		t.size = s;
		t.n_items = n_items;
		t.ci = fi;
		ret = process_primitive_field (&t, fbuf, 0);
		if (ret < 0)
		    return (ret);
		continue;
	    }
	    size = fi->osize;
	    d_len = 0;
	}
	else {				/* other types */
	    type = fld->type;
	    n_items = fld->n_items;
	    fbuf = (char *)data + fld->offset;
	    d_len = 0;
	    if (data_len > 0) {
		d_len = data_len - fld->offset;
		if (d_len <= 0)
		    return (SMIA_DATA_TOO_SHORT);
	    }
	    size = fld->size;
	}

	off = 0;
	for (f = 0; f < n_items; f++) {
	    int len;

	    if (d_len > 0) {
		if (d_len - off <= 0)
		    return (SMIA_DATA_TOO_SHORT);
		len = d_len - off;
	    }
	    else
		len = 0;
	    if ((ret = process_user_defined_field 
				(type, (char *)fbuf + off, len)) < 0)
		return (ret);
	    off += size;
	}
    }
    return (0);
}

/************************************************************************

    Performs array byte swap on "data" of primitive field "fld".
    "data_len", if non-zero, is the size of "data". Returns 1 if the 
    field is of a primitive type and the byte swap is done, 0 if it is 
    not of primitive type or a negative error code.

************************************************************************/

static int Bswap_primitive_field (SMI_field_t *fld, void *data, int data_len) {
    char c, *cpt;
    int offset, size, n_items, f;

    offset = fld->offset;
    size = fld->size;
    n_items = fld->n_items;
    if (data_len > 0 && offset + n_items * size > data_len)
	return (SMIA_DATA_TOO_SHORT);

    if (size == 1)
	return (1);

    cpt = (char *)data + offset;
    if (size == 4) {
	for (f = 0; f < n_items; f++) {
	    c = cpt[0];
	    cpt[0] = cpt[3];
	    cpt[3] = c;
	    c = cpt[1];
	    cpt[1] = cpt[2];
	    cpt[2] = c;
	    cpt += size;
	}
	return (1);
    }

    if (size == 2) {
	for (f = 0; f < n_items; f++) {
	    c = cpt[0];
	    cpt[0] = cpt[1];
	    cpt[1] = c;
	    cpt += size;
	}
	return (1);
    }

    if (size == 8) {
	for (f = 0; f < n_items; f++) {
	    c = cpt[0];
	    cpt[0] = cpt[7];
	    cpt[7] = c;
	    c = cpt[1];
	    cpt[1] = cpt[6];
	    cpt[6] = c;
	    c = cpt[2];
	    cpt[2] = cpt[5];
	    cpt[5] = c;
	    c = cpt[3];
	    cpt[3] = cpt[4];
	    cpt[4] = c;
	    cpt += size;
	}
	return (1);
    }

    return (0);
}

/************************************************************************

    Checks if "type" is a primitive type. Returns the size of it if yes 
    or 0 if not. The type description is returned with "t_desc".

************************************************************************/

static int Is_primitive (char *type, int *t_desc) {
    char *t, c;

    *t_desc = SMIA_TD_SIGNED;
    t = type;
    if (*t == 'u' && strncmp (t, "unsigned ", 9) == 0) {
	t += 9;
	*t_desc = SMIA_TD_UNSIGNED;
    }
    c = *t;

    switch (c) {

	case 'c':
	if (strcmp (t, "char") == 0)
	    return (sizeof (char));
	break;

	case 'i':
	if (strcmp (t, "int") == 0)
	    return (sizeof (int));
	break;

	case 's':
	if (strcmp (t, "short") == 0)
	    return (sizeof (short));
	if (strcmp (t, "short int") == 0)
	    return (sizeof (short int));
	break;

	case 'l':
	if (strcmp (t, "long") == 0)
	    return (sizeof (long));
	if (strcmp (t, "long int") == 0)
	    return (sizeof (long int));
	break;

	case 'f':
	if (strcmp (t, "float") == 0) {
	    *t_desc = SMIA_TD_FLOAT;
	    return (sizeof (float));
	}
	break;

	case 'd':
	if (strcmp (t, "double") == 0) {
	    *t_desc = SMIA_TD_FLOAT;
	    return (sizeof (double));
	}
	break;
    }
    return (0);
}

/************************************************************************

    The following are routines for appending data in a buffer. 

************************************************************************/

#define EXTRA_BUF_SIZE 1024

struct buffer_struct {
    int buf_size;
    int length;
    char *buf;
    struct buffer_struct *next;
};

typedef struct buffer_struct buffer_t;

static buffer_t *First_b = NULL, *Cr_b = NULL;

/************************************************************************

    Appending "data" of "data_len" bytes into the buffer. Returns 0 on
    success or a negative error code.

************************************************************************/

static int Append_data (char *data, int data_len) {
    int room, len;

    if (Cr_b == NULL)
	room = 0;
    else
	room = Cr_b->buf_size - Cr_b->length;

    len = data_len;
    if (len > room)
	len = room;
    if (len > 0) {
	memcpy (Cr_b->buf + Cr_b->length, data, len);
	Cr_b->length += len;
    }

    if (data_len > len) {
	buffer_t *t;
	int s = sizeof (buffer_t) + data_len - len + EXTRA_BUF_SIZE;
	t = (buffer_t *)malloc (s);
	if (t == NULL)
	    return (SMIA_MALLOC_FAILED);
	t->buf_size = s - sizeof (buffer_t);
	t->buf = (char *)t + sizeof (buffer_t);
	t->next = NULL;
	memcpy (t->buf, data + len, data_len - len);
	t->length = data_len - len;
	if (Cr_b != NULL)
	    Cr_b->next = t;
	Cr_b = t;
	if (First_b == NULL)
	    First_b = Cr_b;
    }
    return (0);
}

/************************************************************************

    Retrieves and returns appended data in "data". Returns the size of
    the data on success or a negative error code. If data == NULL, all 
    data are discarded and buffers are freed.

************************************************************************/

static int Return_data (char **data) {
    int size;
    buffer_t *t;

    if (data != NULL)
	*data = NULL;
    if (First_b == NULL)
	return (0);

    size = 0;
    t = First_b;
    while (t != NULL) {
	size += t->length;
	t = t->next;
    }
    if (data != NULL) {
	*data = (char *)malloc (size);
	if (*data == NULL)
	    return (SMIA_MALLOC_FAILED);
    }
    size = 0;
    t = First_b;
    while (t != NULL) {
	char *p;

	if (data != NULL)
	    memcpy (*data + size, t->buf, t->length);
	size += t->length;
	p = (char *)t;
	t = t->next;
	free (p);
    }
    First_b = Cr_b = NULL;
    return (size);
}
