
/******************************************************************

    The ORPGADPT module.
 
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $ 
 * $Date: 2005/03/18 22:18:27 $
 * $Id: orpgadpt.c,v 1.9 2005/03/18 22:18:27 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <orpgadpt.h>
#include <orpg.h>
#include <infr.h>
#define NEED_ADPT_OBJ_TABLE
#include "orpgadpt_n_def.h"

enum {ORPGADPT_UNKNOWN, ORPGADPT_INT, ORPGADPT_SHORT, ORPGADPT_CHAR, 
      ORPGADPT_UINT, ORPGADPT_USHORT, ORPGADPT_UCHAR, ORPGADPT_FLOAT, 
      ORPGADPT_DOUBLE, ORPGADPT_STRING};

extern SMI_info_t *ORPG_smi_info (char *name, void *data);
static ORPGADPT_object_t *Get_data_obj (const char *name);
static int Read_data (ORPGADPT_object_t *obj);
static int Write_data (ORPGADPT_object_t *obj, char *data);
static int Get_type (char *type, int n_items);
static void Move_data_to_array (int type, int n_items, 
					char *array, double *values);
static void Move_data_from_array (int type, int n_items, 
					char *array, double *values);
static void On_change_cb (int fd, LB_id_t msgid, int msg_info, void *arg);
static void Any_change_cb (int fd, LB_id_t msgid, int msg_info, void *arg);
static int Process_baseline (int restore_baseline, const char* name);
static int Get_string_values_from_DB (ORPGADPT_object_t *obj, 
				char *fname, char **values);
static int Get_values_from_DB (ORPGADPT_object_t *obj, const char *fname, 
					int n_items, double **values);


/*   @refer_to <orpgadpt.h>.ORPGADPT_error_occurred for comments  **/
int ORPGADPT_error_occurred () {
    return (0);
}

/*   @refer_to <orpgadpt.h>.ORPGADPT_log_last_error for comments  **/
int ORPGADPT_log_last_error (int le_code, int flags) {
    return (0);
}

/*********************************************************************

    Returns the object table with "objp". The return value is the size
    of the table.

**********************************************************************/

int ORPGADPT_get_obj_table (ORPGADPT_object_t **objp) {
    *objp = Obj_entries;
    return (N_obj_entries);
}

/*********************************************************************

    Gets a unique integer id associated with "name". Returns -1 if an 
    integer id is not supported or if the name does not exist, a positive 
    integer id is returned if an id is available for the specified
    message name. We implement a case that used in RPG (cpc019/tsk001).

**********************************************************************/

int ORPGADPT_get_msg_id (const char* name) {
    if (strcmp (name, "Redundant Information") == 0) {
	static char *id = NULL;
	ORPGADPT_object_t *obj = Get_data_obj (name);
	if (obj == NULL)
	    return (-1);
	id = STR_copy (id, obj->de_id);
	id = STR_cat (id, "channel_number");
	return (DEAU_get_msg_id (id));
    }
    return (-1);
}

/*********************************************************************

    See orpgadpt.h for detailed behavior. We lock the internal node for
    the object.

**********************************************************************/

int ORPGADPT_lock (int lock_command, const char* call_name) {
    static char *id = NULL;
    ORPGADPT_object_t *obj = Get_data_obj (call_name);
    if (obj == NULL)
	return (0);
    id = STR_copy (id, obj->de_id);
    id[STR_size (id) - 2] = '\0';	/* remove terminating "." */
    switch (lock_command) {
	int ret;
	case ORPGADPT_EXCLUSIVE_LOCK:
	    ret = DEAU_lock_de (id, LB_EXCLUSIVE_LOCK);
	    if (ret == LB_SUCCESS)
		return (1);
	    break;

	case ORPGADPT_SHARED_LOCK:
	    ret = DEAU_lock_de (id, LB_SHARED_LOCK);
	    if (ret == LB_SUCCESS)
		return (1);
	    break;

	case ORPGADPT_TEST_EXCLUSIVE_LOCK:
	    ret = DEAU_lock_de (id, LB_TEST_EXCLUSIVE_LOCK);
	    if (ret == LB_SUCCESS)
		return (1);
	    break;

	case ORPGADPT_UNLOCK:
	    DEAU_lock_de (id, LB_UNLOCK);
	    return (1);
	    break;
    }
    return (0);
}

/*********************************************************************

    We implement actual update in PROP_set*. So here we only need to 
    check the error. "call_name" seems to be redundent. Returns true
    on success or false on failure.

**********************************************************************/

int ORPGADPT_set_propobj (const char* call_name, void *prop_obj) {
    ORPGADPT_object_t *obj = (ORPGADPT_object_t *)prop_obj;
    static char *id = NULL;
    int ret, i;

    for (i = 0; i < obj->n_fs; i++) {
	ORPGADPT_field_t *f;
	f = obj->fs + i;
	if (!f->changed)
	    continue;
	id = STR_copy (id, obj->de_id);
	id = STR_cat (id, f->f_name);
	ret = DEAU_update_attr (id, DEAU_AT_VALUE, f->value);
	if (ret < 0) {
	    LE_send_msg (GL_INFO, 
		"ORPGADPT: DEAU_update_attr (%s) failed (%d)", id, ret);
	    return (0);
	}
	f->changed = 0;
    }
    return (1);
}

/*********************************************************************

    Registers for change notification of adaptation data block "name". 
    If "name" is NULL, all adapt data is assumed. "change_callback" is 
    the Function that will be callsed when the specified item changes. 
    "user_data" is the user information that will be passed to the 
    callback function whenver it is called. returns 1 if successful, 
    0 otherwise. "name" = NULL is not implemented.

**********************************************************************/

int ORPGADPT_on_change (const char* call_name, 
		orpgadpt_chg_func_ptr_t change_callback, void* user_data) {
    int gid;
    static char *id = NULL;
    ORPGADPT_object_t *obj = Get_data_obj (call_name);
    if (obj == NULL)
	return (0);
    id = STR_copy (id, obj->de_id);
    id[STR_size (id) - 2] = '\0';	/* remove terminating "." */

    gid = DEAU_UN_register (id, On_change_cb);
    if (gid < 0)
	return (0);
/* printf ("ORPGADPT_on_change id %s, gid %d  cb %p\n", id, gid, change_callback); */
    obj->gid = gid;
    obj->cb = (void *)change_callback;
    return (1);
}

/*********************************************************************

    Restores data object "name" from the base line adaptation data files.
    "replace_message" - not used. Returns 1 if successful, 0 otherwise.

**********************************************************************/

int ORPGADPT_restore_baseline (const char* name, int replace_message) {
    return (Process_baseline (1, name));
}

/*********************************************************************

    Updates the base line adaptation data files for data object "name".
    "replace_message" - not used. Returns 1 if successful, 0 otherwise.

**********************************************************************/

int ORPGADPT_update_baseline (const char* name, int replace_message) {
    return (Process_baseline (0, name));
}

/*********************************************************************

    Returns true.

**********************************************************************/

int ORPGADPT_select_data_store (int data_store_id) {
    return (1);
}

/*********************************************************************

    Returns the data object of access name "name" from the data element
    database. It returns NULL on failure. In case of an exceptional 
    condition, the last error text will be set appropriately.

**********************************************************************/

void *ORPGADPT_get_propobj (const char *call_name) {
    ORPGADPT_object_t *obj;
    static char *id = NULL;
    char *p;
    int n_fs, ret, i;

    obj = Get_data_obj (call_name);
    if (obj == NULL)
	return (NULL);

    if (obj->n_fs == 0) {
	id = STR_copy (id, obj->de_id);
	id[strlen (id) - 1] = '\0';
	if ((n_fs = DEAU_get_branch_names (id, &p)) <= 0) {
	    LE_send_msg (GL_INFO, 
		"ORPGADPT: DEAU_get_branch_names (%s) failed (%d)", id, n_fs);
	    return (NULL);
	}
	obj->n_fs = n_fs;
	obj->fs = MISC_malloc (n_fs * sizeof (ORPGADPT_field_t));
	for (i = 0; i < n_fs; i++) {
	    obj->fs[i].changed = 0;
	    obj->fs[i].f_name = MISC_malloc (strlen (p) + 1);
	    strcpy (obj->fs[i].f_name, p);
	    obj->fs[i].value = NULL;
	    p += strlen (p) + 1;
	}
    }

    for (i = 0; i < obj->n_fs; i++) {
	ORPGADPT_field_t *f;
	DEAU_attr_t *at;
	f = obj->fs + i;
	id = STR_copy (id, obj->de_id);
	id = STR_cat (id, f->f_name);
	ret = DEAU_get_attr_by_id (id, &at);
	if (ret < 0) {
	    LE_send_msg (GL_INFO, 
		"ORPGADPT: DEAU_get_attr_by_id (%s) failed (%d)", id, ret);
	    return (NULL);
	}
	if (f->value != NULL)
	    free (f->value);
	p = at->ats[DEAU_AT_VALUE];
	f->value = MISC_malloc (strlen (p) + 1);
	strcpy (f->value, p);
    }

    return (obj);
}

/*********************************************************************

    Gets the size of the data object of access name "name" in the data 
    element database. It returns the size on success or -1 on failure. 
    In case of an exceptional condition, the last error text will be 
    set appropriately.

**********************************************************************/

int ORPGADPT_get_data_length (const char* name) {
    ORPGADPT_object_t *obj;

    obj = Get_data_obj (name);
    if (obj == NULL ||
	obj->struct_name[0] == '\0')
	return (-1);
    return (obj->si->size);
}

/*********************************************************************

    Gets the data object of access name "name" in the data element
    database and copies it to buffer "data" of "data_len" bytes. It
    returns 1 on success or 0 on failure.

**********************************************************************/

int ORPGADPT_get_data (const char* name, void *data, int data_len) {
    ORPGADPT_object_t *obj;

    obj = Get_data_obj (name);
    if (obj == NULL ||
	obj->struct_name[0] == '\0')
	return (0);
    if (data_len < obj->si->size) {
	LE_send_msg (GL_INFO, 
	    "ORPGADPT: ORPGADPT_get_data (%s) error - buffer too small", name);
	return (0);
    }
    if (Read_data (obj) <= 0)
	return (0);
    memcpy (data, obj->data, obj->si->size);
    return (1);
}

/*********************************************************************

    Sets the data object of access name "name" in the data element
    database with "data" of size "data_len". It returns 1 on success 
    or 0 on failure. "class_name" is not used.

**********************************************************************/

int ORPGADPT_set_data (const char *name, void *data, 
			int data_len, const char *class_name) {
    ORPGADPT_object_t *obj;

    obj = Get_data_obj (name);
    if (obj == NULL ||
	obj->struct_name[0] == '\0')
	return (0);
    if (data_len < obj->si->size) {
	LE_send_msg (GL_INFO, 
	    "ORPGADPT: ORPGADPT_set_data (%s) error - bad length", name);
	return (0);
    }
    if (Write_data (obj, data) <= 0)
	return (0);
    return (1);
}

/*********************************************************************

    Reads the data object "obj" from the data element database. It
    returns 1 on success, 0 if not found or a negative error code.

**********************************************************************/

static int Read_data (ORPGADPT_object_t *obj) {
    SMI_info_t *si;
    int i;

    if (!obj->changed)
	return (1);
    si = obj->si;
    if (obj->data == NULL)
	obj->data = MISC_malloc (si->size);

    for (i = 0; i < si->n_fields; i++) {
	SMI_field_t *fld;
	int n_items, ret, type;
	char *data;

	fld = si->fields + i;
	data = obj->data + fld->offset;
	n_items = fld->n_items;
	type = Get_type (fld->type, n_items);
	if (type == ORPGADPT_STRING) {		/* char string */
	    char *pt;
	    ret = Get_string_values_from_DB (obj, fld->name, &pt);
	    if (ret >= 1) {
		strncpy (data, pt, n_items);
		data[n_items - 1] = '\0';
	    }
	    else 
		data[0] = '\0';
	}
	else {						/* numerical */
	    double *d;
	    Get_values_from_DB (obj, fld->name, n_items, &d);
	    Move_data_to_array (type, n_items, data, d);
	}
    }
    obj->changed = 0;
    return (1);
}

/************************************************************************

************************************************************************/

static ORPGADPT_object_t *Get_data_obj (const char *name) {
    static int initialized = 0;
    int i, ret;

    if (!initialized) {
	char *ac_ds_name;
	ac_ds_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
	if (ac_ds_name == NULL) {
	    LE_send_msg (GL_ERROR, 
		    "ORPGADPT: ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA);
	    exit (1);
	}
	DEAU_LB_name (ac_ds_name);
	ret = DEAU_UN_register (NULL, Any_change_cb);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
		    "ORPGADPT: DEAU_UN_register all failed (%d)", ret);
	    exit (1);
	}
	initialized = 1;
    }

    if (name == NULL)
	return (NULL);
    for (i = 0; i < N_obj_entries; i++) {
	if (strcmp (name, Obj_entries[i].call_name) == 0)
	    break;
    }
    if (i >= N_obj_entries)
	return (NULL);

    Obj_entries[i].si = NULL;
    if (Obj_entries[i].struct_name[0] == '\0' ||
        (Obj_entries[i].si = ORPG_smi_info (
			Obj_entries[i].struct_name, NULL)) != NULL)
	return (Obj_entries + i);
    LE_send_msg (GL_INFO, "ORPGADPT: ORPG_smi_info %s failed", 
					Obj_entries[i].struct_name);
    return (NULL);
}

/************************************************************************

    Returns the type macro of type name "type" and array size "n_items".
    Char array is considered as string type.

************************************************************************/

static int Get_type (char *type, int n_items) {
    char c;
    int t;

    t = ORPGADPT_UNKNOWN;
    c = type[0];
    if (c == 'i' && strcmp (type, "int") == 0)
	t = ORPGADPT_INT;
    else if (c == 'l' && strcmp (type, "long") == 0)
	t = ORPGADPT_INT;
    else if (c == 's' && strcmp (type, "short") == 0)
	t = ORPGADPT_SHORT;
    else if (c == 'u') {
	if (strcmp (type + 9, "int") == 0)
	    t = ORPGADPT_UINT;
	else if (strcmp (type + 9, "long") == 0)
	    t = ORPGADPT_UINT;
	else if (strcmp (type + 9, "short") == 0)
	    t = ORPGADPT_USHORT;
	else if (strcmp (type + 9, "char") == 0)
	    t = ORPGADPT_UCHAR;
    }
    else if (c == 'f' && strcmp (type, "float") == 0)
	t = ORPGADPT_FLOAT;
    else if (c == 'd' && strcmp (type, "double") == 0)
	t = ORPGADPT_DOUBLE;
    else if (c == 'c' && strcmp (type, "char") == 0) {
	if (n_items > 0)
	    t = ORPGADPT_STRING;
	else
	    t = ORPGADPT_CHAR;
    }
    if (t == ORPGADPT_UNKNOWN)
	LE_send_msg (GL_INFO, "ORPGADPT: Type %s not supported\n", type);
    return (t);
}

/************************************************************************

    Writes struct "data", as object "obj", to DEA DB. Returns 1 on success 
    or 0 on failure.

************************************************************************/

static int Write_data (ORPGADPT_object_t *obj, char *data) {
    static char *vb = NULL, *id = NULL;
    int i;
    SMI_info_t *si;

    si = obj->si;
    for (i = 0; i < si->n_fields; i++) {
	SMI_field_t *fld;
	int n_items, ret, type;
	char *dp;

	fld = si->fields + i;
	dp = data + fld->offset;
	n_items = fld->n_items;
	type = Get_type (fld->type, n_items);
	id = STR_copy (id, obj->de_id);
	id = STR_cat (id, fld->name);
	if (type == ORPGADPT_STRING) {		/* char string */
	    dp[n_items - 1] = '\0';
	    ret = DEAU_set_values (id, 1, dp, 1, 0);
	    if (ret < 0) {
		LE_send_msg (GL_ERROR, 
		    "ORPGADPT: DEAU_set_values (s) (%s) failed (%d)", id, ret);
		return (0);
	    }
	}
	else {						/* numerical */
	    vb = STR_reset (vb, n_items * sizeof (double));
	    Move_data_from_array (type, n_items, dp, (double *)vb);
	    ret = DEAU_set_values (id, 0, vb, n_items, 0);
	    if (ret < 0) {
		LE_send_msg (GL_ERROR, 
		    "ORPGADPT: DEAU_set_values (%s) failed (%d)", id, ret);
		return (0);
	    }
	}
    }

    return (1);
}

/************************************************************************

    Moved "n_items" data values from "values" to array "array" of type 
    "array".

************************************************************************/

static void Move_data_to_array (int type, int n_items, 
					char *array, double *values) {
    int i;

    for (i = 0; i < n_items; i++) {
	double d;

	d = values[i];
	if (type == ORPGADPT_INT)
	    ((int *)array)[i] = d;
	else if (type == ORPGADPT_SHORT)
	    ((short *)array)[i] = d;
	else if (type == ORPGADPT_UINT)
	    ((unsigned int *)array)[i] = d;
	else if (type == ORPGADPT_USHORT)
	    ((unsigned short *)array)[i] = d;
	else if (type == ORPGADPT_FLOAT)
	    ((float *)array)[i] = d;
	else if (type == ORPGADPT_DOUBLE)
	    ((double *)array)[i] = d;
	else if (type == ORPGADPT_CHAR)
	    ((char *)array)[i] = d;
	else if (type == ORPGADPT_UCHAR)
	    ((unsigned char *)array)[i] = d;
    }
}

/************************************************************************

    Moved "n_items" data values to "values" from array "array" of type 
    "array".

************************************************************************/

static void Move_data_from_array (int type, int n_items, 
					char *array, double *values) {
    int i;

    for (i = 0; i < n_items; i++) {
	double d;

	d = 0.;
	if (type == ORPGADPT_INT)
	    d = ((int *)array)[i];
	else if (type == ORPGADPT_SHORT)
	    d = ((short *)array)[i];
	else if (type == ORPGADPT_UINT)
	    d = ((unsigned int *)array)[i];
	else if (type == ORPGADPT_USHORT)
	    d = ((unsigned short *)array)[i];
	else if (type == ORPGADPT_FLOAT)
	    d = ((float *)array)[i];
	else if (type == ORPGADPT_DOUBLE)
	    d = ((double *)array)[i];
	else if (type == ORPGADPT_CHAR)
	    d = ((char *)array)[i];
	else if (type == ORPGADPT_UCHAR)
	    d = ((unsigned char *)array)[i];
	values[i] = d;
    }
}

/************************************************************************

    Reads values, from DEA DB, of the field "fname" of data object "obj".
    returns the number of values read on success or 0 on failure.

************************************************************************/

static int Get_string_values_from_DB (ORPGADPT_object_t *obj, 
				char *fname, char **values) {
    static char *id = NULL;
    int n;

    id = STR_copy (id, obj->de_id);
    id = STR_cat (id, fname);
    n = DEAU_get_string_values (id, values);
    if (n <= 0) {
	LE_send_msg (GL_INFO, 
		"ORPGADPT: String value not found for DE %s\n", id);
	return (0);
    }
    return (n);
}

/************************************************************************

    Reads values, from DEA DB, of the field "fname" of data object "obj".
    returns the number of values read on success or 0 on failure. If 
    n_items > 0, additional buffer space is allocated and set to 0.

************************************************************************/

static int Get_values_from_DB (ORPGADPT_object_t *obj, const char *fname, 
					int n_items, double **values) {
    static char *id = NULL, *v_buf = NULL;
    int n, m;

    id = STR_copy (id, obj->de_id);
    id = STR_cat (id, fname);
    n = DEAU_get_number_of_values (id);
    if (n <= 0) {
	LE_send_msg (GL_INFO, "ORPGADPT: No value found for DE %s\n", id);
	if (n_items <= 0)
	    return (0);
	n = 0;
    }
    if (n_items > n)
	v_buf = STR_reset (v_buf, n_items * sizeof (double));
    else
	v_buf = STR_reset (v_buf, n * sizeof (double));
    if (n > 0) {
	m = DEAU_get_values (NULL, (double *)v_buf, n);
	if (m <= 0) {
	    LE_send_msg (GL_INFO, 
		"ORPGADPT: Value is not numerical for DE %s (%d)\n", id, m);
	    if (n_items <= 0)
		return (0);
	    n = 0;
	}
    }
    if (n_items > n)
	memset (v_buf + (n * sizeof (double)), 0,
				(n_items - n) * sizeof (double));
    *values = (double *)v_buf;
    return (n);
}

/*********************************************************************

    The ORPGADPT_on_change callback function.

**********************************************************************/

static void Any_change_cb (int fd, LB_id_t msgid, int msg_info, void *arg) {
    int i;
    for (i = 0; i < N_obj_entries; i++)
	Obj_entries[i].changed = 1;
}

/*********************************************************************

    The ORPGADPT_on_change callback function.

**********************************************************************/

static void On_change_cb (int fd, LB_id_t msgid, int msg_info, void *arg) {
    LB_info info;
    int ret, cnt, ind, i;
    orpgadpt_event_t event;
    ORPGADPT_object_t *obj;
    static int user_data[2] = {0, 0};	/* used in orpgadpt.cpp */

    if ((ret = LB_msg_info (fd, msgid, &info)) < 0) {
	LE_send_msg (GL_INFO, 
	    "ORPGADPT: LB_msg_info (In On_change_cb, msgid %d) error (%d)", 
					msgid, ret);
	return;
    }
/*printf ("On_change_cb msgid %d msg_info %d, tag %d\n", 
					msgid, msg_info, info.mark); */
    cnt = ind = 0;
    for (i = 0; i < N_obj_entries; i++) {
	if (Obj_entries[i].gid == info.mark) {
	    ind = i;
	    cnt++;
	}
    }
    if (cnt == 0) {
	LE_send_msg (GL_INFO, 
	    "ORPGADPT: No match (In On_change_cb, msgid %d, tag %d)", 
					msgid, info.mark);
	return;
    }
    if (cnt > 1) {
	LE_send_msg (GL_INFO, 
	    "ORPGADPT: Multiple match (In On_change_cb, msgid %d, tag %d)", 
					msgid, info.mark);
	return;
    }
    obj = Obj_entries + ind;
    event.name = obj->call_name;
    event.data_id = ORPGDAT_MISC_ADAPT;
    event.event = ORPGADPT_CHANGE_EVENT;
    event.user_data = user_data;
/*    printf ("calls %p with %d, %s\n", obj->cb, event.data_id, event.name); */
    ((orpgadpt_chg_func_ptr_t)(obj->cb)) (&event);
}

/*********************************************************************

    Copies to/from the baseline values for data object "name". Returns 
    1 if successful, 0 otherwise.

**********************************************************************/

static int Process_baseline (int restore_baseline, const char* name) {
    static char *id = NULL, *props = NULL;
    ORPGADPT_object_t *obj;
    int n_props, i;
    char *p;

    obj = Get_data_obj (name);
    if (obj == NULL)		/* name not found */
	return (0);
    id = STR_copy (id, obj->de_id);
    id[STR_size (id) - 2] = '\0';	/* remove terminating "." */
    n_props = DEAU_get_branch_names (id, &p);
    if (n_props <= 0)
	LE_send_msg (GL_INFO, 
	    "ORPGADPT: DEAU_get_branch_names (%s) returns %d", id, n_props);
    props = STR_reset (props, 256);
    for (i = 0; i < n_props; i++) {	/* save prop names */
	int len;
	len = strlen (p) + 1;
	props = STR_append (props, p, len);
	p += len;
    }
    p = props;
    for (i = 0; i < n_props; i++) {	/* process each prop */
	int ret;

	id = STR_copy (id, obj->de_id);
	id = STR_cat (id, p);
	p += strlen (p) + 1;

	if (restore_baseline)
	    ret = DEAU_move_baseline (id, 0);
	else
	    ret = DEAU_move_baseline (id, 1);
	if (ret < 0)
	    LE_send_msg (GL_INFO, 
			"ORPGADPT: DEAU_move_baseline (%s) failed", id);
    }
    return (1);
}

void ORPGADPT_process_callback(void* event_in) {return;}
void ORPGADPT_report_legacy_changes() {return;}
int ORPGADPT_update_legacy_color_tables(const char* name, propobj_t prop_obj) {return (1);}
int ORPGADPT_refresh_legacy(int verbose_level) {return (1);}
int ORPGADPT_restore_all_from_baseline() {return (0);}
