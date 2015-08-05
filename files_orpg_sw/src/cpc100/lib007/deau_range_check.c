

/**************************************************************************

    The ORPG data field attribute library module - ragne checking and
    data conversion functions.

**************************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/05/19 19:35:08 $
 * $Id: deau_range_check.c,v 1.22 2011/05/19 19:35:08 jing Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <infr.h>
#include <deau_def.h>


typedef struct {		/* struct stores attribute of local interest */
    char *name;			/* data identifier */
    char no_attr;		/* no attribute found */
    char type;			/* data type */
    DEAU_range_t r;		/* binary range spec */
    char *range_desc;		/* text range spec */
    DEAU_conversion_t c;	/* binary conversion spec */
    char *conv_desc;		/* text conversion spec */
} Local_attribute_t;

typedef struct {		/* hash table entry for local data elements */
    unsigned int hash;		/* hash value */
    Local_attribute_t *ld;	/* data element */
} Local_data_t;

static int N_fields_OK = 0;	/* number of fields checked OK */
static int N_fields_failed = 0;	/* number of fields checked failed */


static int Range_check_primitive_field 
			(SMI_field_t *fld, void *data, int data_len);
static double Get_numerical_value (int data_type, char *pt);
static int Local_comp (void *a1, void *a2);
static int Get_attr_by_name (char *name, Local_attribute_t **latt);
static int Get_range_and_conversion (char *name, 
				Local_attribute_t **dap, int data_type);

static int GetExpoBase2 (double d) {
    int i;
    i = 0;
    ((short *)(&i))[0] = (((short *)(&d))[3] & (short)32752);
				/* _123456789ab____ & 0111111111110000 */
    return (i >> 4) - 1023;
}

static int Equals (double d1, double d2) {
    int e1, e2, e3;
    if (d1 == d2)
	return 1;
    e1 = GetExpoBase2 (d1);
    e2 = GetExpoBase2 (d2);
    e3 = GetExpoBase2 (d1 - d2);
    if ((e3 - e2 < -48) && (e3 - e1 < -48))
	return 1;
    return 0;
}

static int Compare (double d1, double d2) {
    if (Equals(d1, d2) == 1)
	return 0;
    if (d1 > d2)
	return 1;
    return -1;
}

/************************************************************************

    Returns the number of fields that are range-checked and OK in the 
    previous call of DEAU_check_struct_range.

************************************************************************/

int DEAU_get_number_of_checked_fields () {
    return (N_fields_OK);
}

/************************************************************************

    Performs range check of all fields on "data" of type "type". 
    "data_len", if non-zero, is the size of "data". This returns 
    the number of range-check-failed fields on success. In other error 
    conditions, this function returns an negative error code.

************************************************************************/

int DEAU_check_struct_range (char *type, void *data, int data_len) {
    int ret;

    N_fields_OK = N_fields_failed = 0;
    ret = SMIA_go_through_struct (type, data, data_len,
					Range_check_primitive_field);
    if (ret < 0)
	return (ret);
    return (N_fields_failed);
}

/************************************************************************

    Performs range check on "data" of primitive field "fld".
    "data_len", if non-zero, is the size of "data". Returns 0 if the 
    field is within range, -1 if it is not or a negative error code.

************************************************************************/

#define MAX_STRING_SIZE 128

static int Range_check_primitive_field 
			(SMI_field_t *fld, void *data, int data_len) {
    int type_desc, ret, l1, l2, deau_t;
    char name[MAX_STRING_SIZE], *cr_type;

    if (data_len > 0 && fld->offset + fld->n_items * fld->size > data_len)
	return (DEAU_DATA_TOO_SHORT);

    /* prepare DEAU data name */
    cr_type = SMIA_get_current_type (); 
    l1 = strlen (cr_type);
    l2 = strlen (fld->name);
    if (l1 + l2 + 2 > MAX_STRING_SIZE)
	return (DEAU_TYPE_TOO_LONG);
    strcpy (name, cr_type);
    name[l1] = '.';
    strcpy (name + l1 + 1, fld->name);
    if (name[6] == ' ')		/* space after "struct" */
	name[6] = '_';

    /* prepare DEAU data type */
    type_desc = SMIA_get_type_desc (fld);
    if (type_desc == SMIA_TD_SIGNED) {
	if (fld->size == 4)
	    deau_t = DEAU_T_INT;
	else if (fld->size == 2)
	    deau_t = DEAU_T_SHORT;
	else 
	    deau_t = DEAU_T_BYTE;
    }
    else if (type_desc == SMIA_TD_UNSIGNED) {
	if (fld->size == 4)
	    deau_t = DEAU_T_UINT;
	else if (fld->size == 2)
	    deau_t = DEAU_T_USHORT;
	else 
	    deau_t = DEAU_T_UBYTE;
    }
    else {			/* type_desc == SMIA_TD_FLOAT */
	if (fld->size == 4)
	    deau_t = DEAU_T_FLOAT;
	else 
	    deau_t = DEAU_T_DOUBLE;
    }

    ret = DEAU_check_data_range (name, deau_t, 
				fld->n_items, (char *)data + fld->offset);

    if (ret > 0) {
	N_fields_OK++;
	ret = 0;
    }
    else if (ret == -1) {
	N_fields_failed++;
	ret = 0;
    }
    return (ret);
}

/************************************************************************

    Performs data range check on an array of size "array_size" at address
    "data". The data is a primitive type "data_type". The data name is 
    "name". The function returns 1 if range check is OK, 0 if no range 
    is specified, -1 if any array element is out of range, or other 
    negative codes.

************************************************************************/

int DEAU_check_data_range (char *name, int data_type, 
					int array_size, char *data) {
    static const char d_size[] = {1, 4, 2, 1, 4, 2, 1, 1, 4, 8, 1, 1};
		/* size of primitive types - depending on enum DEAU_T_* */
    Local_attribute_t *da;
    char *pt, per_ele_name[128];
    int ind, dsize, ret, per_ele_range_check, err;
    DEAU_range_t *r;

    per_ele_range_check = 0;
    ret = Get_range_and_conversion (name, &da, data_type);
    if (ret < 0)
	return (ret);
    if (ret <= 0 && array_size > 0) {	/* try per array element range check */
	if (name == NULL)
	    return (ret);
	strncpy (per_ele_name, name, 121);
	per_ele_name[120] = '\0';
	strcat (per_ele_name, "[]");
	ret = Get_range_and_conversion (per_ele_name, &da, data_type);
	per_ele_range_check = 1;
    }
    if (ret <= 0)
	return (ret);

    r = &(da->r);
    if (r->range_type == DEAU_R_UNDEFINED && !per_ele_range_check)
	return (0);

    if (!per_ele_range_check && data_type == DEAU_T_BYTE && 
		array_size > 1 && da->type == DEAU_T_STRING) {
	data_type = DEAU_T_STRING;
	data[array_size - 1] = '\0';
	dsize = array_size;
	array_size = 1;
	
    }
    else
        dsize = d_size[data_type];
    pt = data;

    err = 1;

    for (ind = 0; ind < array_size; ind++) {
	double d;
	int i, off;
	char buf[128];

        if ( ind > 0 ) {
	    if (data_type == DEAU_T_STRING)
	        pt += strlen (pt) + 1;
	    else
	        pt += dsize;
        }

	if (per_ele_range_check) {
	    strncpy (per_ele_name, name, 121);
	    per_ele_name[120] = '\0';

	    sprintf (per_ele_name + strlen (per_ele_name), "[%d]", ind);
	    ret = Get_range_and_conversion (per_ele_name, &da, data_type);
	    if (ret <= 0)
		continue;
	    r = &(da->r);
	    if (r->range_type == DEAU_R_UNDEFINED)
		continue;
	}

	d = Get_numerical_value (data_type, pt);
	if (da->c.conv_type != DEAU_C_UNDEFINED) {
	    if (da->c.conv_type == DEAU_C_SCALE) 
		d = d * da->c.scale + da->c.offset;
	    else if (da->c.conv_type == DEAU_C_METHOD) {
		if (data_type != da->type)
		    return (DEAU_rep_err_ret
			("Range check not done - bad type: ", da->name, 0));
		da->c.method (pt, &d);
	    }
	    else {
		return (DEAU_rep_err_ret ("Range check not implemented: ", 
							da->name, 0));
	    }
	}

	if (r->range_type == DEAU_R_MINMAX) {
	    if (data_type == DEAU_T_STRING)
		return (DEAU_rep_err_ret ("Range check failed - bad type: ", 
							da->name, -1));
	    if ((r->b_min == DEAU_R_INCLUSIVE && Compare (d, r->min) < 0) ||
		(r->b_min == DEAU_R_NOT_INCLUSIVE && d <= r->min) ||
		(r->b_max == DEAU_R_INCLUSIVE && Compare (d, r->max) > 0) ||
		(r->b_max == DEAU_R_NOT_INCLUSIVE && d >= r->max)) {
		sprintf (buf, 
		    "Range check failed (value %f; %s): ", d, da->range_desc);
                DEAU_rep_err_ret (buf, da->name, -1);
                err = -1;
                continue;
	    }
	}
	else if (r->range_type == DEAU_R_DISCRETE_NUMBERS) {
	    if (data_type == DEAU_T_STRING)
		return (DEAU_rep_err_ret ("Range check failed - bad type: ", 
							da->name, -1));
	    for (i = 0; i < r->n_values; i++) {
		if (Equals (d, ((double *)(r->values))[i]))
		    break;
	    }
	    if (i >= r->n_values) {
		sprintf (buf, "Range check failed (value %f): ", d);
                DEAU_rep_err_ret (buf, da->name, -1);
                err = -1;
                continue;
	    }
	}
	else if (r->range_type == DEAU_R_DISCRETE_STRINGS) {
	    if (data_type != DEAU_T_STRING)
		return (DEAU_rep_err_ret ("Range check failed - bad type: ", 
							da->name, -1));
	    off = 0;
	    for (i = 0; i < r->n_values; i++) {
		if (strcmp (pt, r->values + off) == 0)
		    break;
		off += strlen (r->values + off) + 1;
	    }
	    if (i >= r->n_values) {
		sprintf (buf, "Range check failed (value %s): ", pt);
                DEAU_rep_err_ret (buf, da->name, -1);
                err = -1;
                continue;
	    }
	}
	else if (r->range_type == DEAU_R_METHOD) {
	    if (r->method (pt) == 0) {

               DEAU_rep_err_ret ("Range check failed (method): ", da->name, -1);
               err = -1;
               continue;
            }
	}
	else {
	    return (DEAU_rep_err_ret ("Range check not implemented: ", 
							da->name, 0));
	}
    }

    return (err);
}

/************************************************************************

    Retrieves the data attributes for "name" and, if succeeded, sets up
    the convertion and range structures. Returns 1 on success, 0 if "name" 
    is not found or a negative error code on failure.

************************************************************************/

static int Get_range_and_conversion (char *name, 
				Local_attribute_t **dap, int data_type) {
    int ret;
    Local_attribute_t *da;

    ret = Get_attr_by_name (name, dap);
    if (ret <= 0) {
	if (ret == DEAU_MALLOC_FAILED)
	    DEAU_rep_err_ret ("malloc failed", "", 0);
	else if (ret < 0) {
	    char b[128];
	    if (name != NULL)
		sprintf (b, "Attr of %s not found (%d)", name, ret);
	    else
		sprintf (b, "Current attr not found (%d)", ret);
	    DEAU_rep_err_ret (b, "", 0);
	}
	return (ret);
    }
    da = *dap;
    if (da->type == DEAU_T_UNDEFINED)
	da->type = data_type;

    if (da->c.conv_type == DEAU_C_NOT_EVALUATED) {
	if ((ret = DEAU_parse_conversion_desc (da->conv_desc, &(da->c))) < 0)
	    return (ret);
    }

    if (da->r.range_type == DEAU_R_NOT_EVALUATED) {
	if ((ret = DEAU_parse_range_desc (da->range_desc, 
						da->type, &(da->r))) < 0) {
	    DEAU_rep_err_ret ("DBG: Range spec not found ", da->name, 0);
	    return (ret);
	}
    }
    return (1);
}

/******************************************************************

    Returns the value, in double, of data pointed to by "pt" of 
    type "data_type".
	
******************************************************************/

static double Get_numerical_value (int data_type, char *pt) {
    double d;

    switch (data_type) {
	case DEAU_T_INT:
	    d = (double)(*((int *)pt));
	    break;
	case DEAU_T_SHORT:
	    d = (double)(*((short *)pt));
	    break;
	case DEAU_T_BYTE:
	    d = (double)(*((signed char *)pt));
	    break;
	case DEAU_T_UINT:
	    d = (double)(*((unsigned int *)pt));
	    break;
	case DEAU_T_USHORT:
	    d = (double)(*((unsigned short *)pt));
	    break;
	case DEAU_T_UBYTE:
	    d = (double)(*((unsigned char *)pt));
	    break;
	case DEAU_T_FLOAT:
	    d = (double)(*((float *)pt));
	    break;
	case DEAU_T_DOUBLE:
	    d = (double)(*((double *)pt));
	    break;
	default:
	    d = 0.;
	    break;
    }
    return (d);
}

/******************************************************************

    Searches data entry of name "name". Returns the data entry with
    "latt" if it is found. Returns 1 on success, 0 not found or a 
    negative error code. If the data element is not in the DB, we 
    still save it so we don't have to search the DB later. 
	
******************************************************************/

static int Get_attr_by_name (char *name, Local_attribute_t **latt) {
    static void *ldis_tbl = NULL;	/* local data item table id */
    static Local_data_t *ldis;		/* local data item table */
    static int n_ldis = 0;		/* size of local data item table */
    Local_data_t ld;
    Local_attribute_t *ldp;
    DEAU_attr_t *da;
    char *buf, *range_desc, *conv_desc;
    int ind, s, no_attr, type, ret;

    if (ldis_tbl == NULL) {
	if ((ldis_tbl = MISC_open_table (sizeof (Local_data_t), 
				64, 1, &n_ldis, (char **)&ldis)) == NULL)
	    return (DEAU_MALLOC_FAILED);
    }

    da = NULL;
    if (name == NULL) {
	ret = DEAU_get_attr_by_id (name, &da);
	if (ret < 0)
	    return (ret);
	name = da->ats[DEAU_AT_ID];
    }

    ld.hash = DEAU_get_hash_value (name);
    if (MISC_table_search (ldis_tbl, &ld, Local_comp, &ind) > 0) {
	Local_data_t *dfirst, *last;

	dfirst = ldis + ind;
	last = ldis + (n_ldis - 1);
	while (1) {
	    if (dfirst->hash != ld.hash)
		break;
	    if (strcmp (dfirst->ld->name, name) == 0) {
		if (dfirst->ld->no_attr)
		    return (0);
		*latt = dfirst->ld;
		return (1);
	    }
	    if (dfirst == last)
		break;
	    dfirst++;
	}
    }

    ret = 0;
    if (da == NULL)
	ret = DEAU_get_attr_by_id (name, &da); /* get data element from DB */
    if (ret < 0) {
	range_desc = NULL;
	conv_desc = NULL;
	type = DEAU_T_UNDEFINED;
	no_attr = 1;
	DEAU_rep_err_ret ("DBG: DEA not found ", name, 0);
    }
    else {
	range_desc = da->ats[DEAU_AT_RANGE];
	conv_desc = da->ats[DEAU_AT_CONVERSION];
	type = DEAU_get_data_type (da);
	if (type < 0)
	    type = DEAU_T_UNDEFINED;
	no_attr = 0;
    }

    if (name == NULL)
	name = "";
    s = sizeof (Local_attribute_t) + strlen (name) + 1;
    if (range_desc != NULL)
	s += strlen (range_desc) + 1;
    if (conv_desc != NULL)
	s += strlen (conv_desc) + 1;
    buf = (char *)malloc (s);
    if (buf == NULL)
	return (DEAU_MALLOC_FAILED);

    ldp = (Local_attribute_t *)buf;
    buf += sizeof (Local_attribute_t);
    ldp->name = buf;
    strcpy (ldp->name, name);
    buf += strlen (name) + 1;
    ldp->type = type;
    ldp->r.range_type = DEAU_R_NOT_EVALUATED;
    ldp->c.conv_type = DEAU_C_NOT_EVALUATED;
    if (range_desc != NULL) {
	ldp->range_desc = buf;
	strcpy (ldp->range_desc, range_desc);
	buf += strlen (range_desc) + 1;
    }
    else 
	ldp->range_desc = NULL;
    if (conv_desc != NULL) {
	ldp->conv_desc = buf;
	strcpy (ldp->conv_desc, conv_desc);
    }
    else
	ldp->conv_desc = NULL;
    ldp->no_attr = no_attr;
    ld.ld = ldp;

    if (name[0] != '\0' && MISC_table_insert (ldis_tbl, &ld, Local_comp) < 0)
	return (DEAU_MALLOC_FAILED);
    *latt = ldp;
    if (no_attr)
	return (0);
    return (1);
}

/******************************************************************

    Comparison function for accessing the local data item table.
	
******************************************************************/

static int Local_comp (void *a1, void *a2) {
    Local_data_t *d1, *d2;

    d1 = (Local_data_t *)a1;
    d2 = (Local_data_t *)a2;
    if (d1->hash < d2->hash)
	return (-1);
    else if (d1->hash > d2->hash)
	return (1);
    else
	return (0);
}
