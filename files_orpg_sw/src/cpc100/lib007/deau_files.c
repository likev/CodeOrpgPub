
/**************************************************************************

    Data element attribute utility library module - file access.

**************************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/08/09 15:45:42 $
 * $Id: deau_files.c,v 1.30 2012/08/09 15:45:42 jing Exp $
 * $Revision: 1.30 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <infr.h>
#include "deau_def.h"

#define ID_TOKEN_SIZE	256
#define TOKEN_SIZE	4096


typedef struct {		/* hash table for the In-memory data base */
    unsigned int hash;		/* hash value */
    DEAU_attr_t *de;		/* data element attributes */
} Db_hash_t;

static void *De_tbl = NULL;	/* In-memory data element table id */
static Db_hash_t *Des;		/* In-memory data element table table */
static int N_des = 0;		/* size of In-memory data element table */
static int De_sorted = 0;	/* In-memory data element table is sorted */

static void (*Error_func) (char *) = NULL;
				/* error reporting function */

static char Lb_name[DEAU_NAME_SIZE] = "";
				/* name of the DET data LB */
static int Lb_fd = -1;		/* fd of the DET data LB */

static DEAU_hash_tbl_t *Hash_tbl = NULL;
				/* The LB DE hash table */

static DEAU_attr_t At_buf = {{NULL}};
				/* tmp buffer for returning the DEA */
static char *Str_buf;		/* for passing buffer to Identifier_comp and 
				   Id_comp */

static int N_suffs = 0;		/* number of special DEA file suffixes. Special
				   file suffixes are used as the first part of
				   the DE identifier. */
static char *Suffs = NULL;	/* Special DEA file suffixes */
static char *Cr_suff = NULL;	/* the current special suffix */

static int Next_dea_index = 0;	/* next index used by DEAU_get_next_dea */
static int Tag = 0;		/* LB message tag value */
static char *De_un_nodes = NULL;
static int N_de_un_nodes = 0;

static char *Attr_names[DEAU_AT_N_ATS] = {
    "id", "name", "type", "range", "value", "unit", "default", 
    "enum", "description", "conversion", "exception", "accuracy",
    "permission", "baseline", "misc"
};

static DEAU_attr_t *(*Get_cache_attr_by_id) (char *id, DEAU_attr_t *at_buf);

static int Add_new_data_entry (char *data_name, DEAU_attr_t **data_entry);
static int Sort_comp (void *a1, void *a2);
static void Cs_err_func (char *msg);
static int Add_new_attribute (DEAU_attr_t *da, char *desc, char *fname,
						int line_num, int override);
static int Open_lb ();
static int Identifier_comp (void *a1, void *a2);
static int Hash_comp (void *a1, void *a2);
static DEAU_attr_t *Get_in_memory_attr_by_id (char *id, unsigned int hash);
static int Get_shared_attr_by_id (char *id, unsigned int hash, 
				DEAU_attr_t **at_p, LB_id_t *msgid);
static int Read_hash_tbl ();
static int Set_current_special_suffix (char *fname, char **core_name);
static unsigned int Get_tag_value (char *name, int is_full_name);
static int Save_int_nodes (int st, int *n_css, char **cs, char **hash_b);
static int Get_sections (char *id, char **s);
static int Generate_internal_nodes (int decnt, 
			char **hash_b, DEAU_id_e_t *des, char *idbuf);
static char *Remove_spaces (char *text);
static int Test_type_name (char *st, char *end);
static int Get_next_attr_type (int first, char *text, char **desc);
static int Get_next_attr (char *text, char **spec_p);
static int Is_an_id_line (char *line, char *id, 
					int id_size, int *short_format);
static int Get_token_len (char *str);
static char *Skip_space (char *str);
static int Is_an_attr_line (char *line);
static int Is_comma_term (char *str, int strsize);
static char *Get_name_excluding_cfg_dir (char *fname);
static void Free_in_memory_DEA ();
static int Save_UN_node (char *line);
static int Get_write_tag (DEAU_attr_t *de);
static char *Append_misc_attr (char *misc, char *src);


/************************************************************************

    Sets an error reporting function.

************************************************************************/

void DEAU_set_get_cache_attr_by_id (DEAU_attr_t *(*func) (char *, DEAU_attr_t *)) {
    Get_cache_attr_by_id = func;
}

/************************************************************************

    Sets an error reporting function.

************************************************************************/

void DEAU_set_error_func (void (*err_func) (char *)) {
    Error_func = err_func;
}

/************************************************************************

    Adds a new special suffix "suff". Returns 0 on success or a negative
    error code.

************************************************************************/

int DEAU_special_suffix (char *suff) {
    char *cpt;
    int i;

    if (Suffs == NULL)
	Suffs = STR_reset (Suffs, 64);
    cpt = Suffs;
    for (i = 0; i < N_suffs; i++) {
	if (strcmp (suff, cpt) == 0)	/* already registered */
	    return (0);
	cpt += strlen (cpt) + 1;
    }
    Suffs = STR_append (Suffs, suff, strlen (suff) + 1);
    N_suffs++;
    return (0);
}

/************************************************************************

    Sets an LB name for the DEA data base.

************************************************************************/

void DEAU_LB_name (char *name) {
    if (strcmp (name, Lb_name) == 0)
	return;
    if (strlen (Lb_name) != 0) {
	DEAU_free_tables ();
	if (Lb_fd >= 0)
	    LB_close (Lb_fd);
	Lb_fd = -1;
    }
    strncpy (Lb_name, name, DEAU_NAME_SIZE);
    Lb_name[DEAU_NAME_SIZE - 1] = '\0';
}

/************************************************************************

    Sets an LB name for the DEA data base.

************************************************************************/

char *DEAU_get_LB_name () {
    return (Lb_name);
}

/************************************************************************

    Frees the hash and identifier tables so resource is freed or to
    force a reread of those tables.

************************************************************************/

void DEAU_free_tables () {
    if (Hash_tbl != NULL)
	free (Hash_tbl);
    Hash_tbl = NULL;
}

/************************************************************************

    Writes the DEA in memory to the LB. Returns 0 on success or a
    negative error number.

************************************************************************/

int DEAU_create_dea_db () {
    LB_id_t msgid;
    int i, size, ret;

    if (Lb_fd < 0 && (Lb_fd = Open_lb ()) < 0)
	return (Lb_fd);

    Tag = 0;
    LB_clear (Lb_fd, LB_ALL);
    if ((ret = LB_write (Lb_fd, "", 0, LB_ANY)) < 0)
	return (ret);
    msgid = LB_previous_msgid (Lb_fd);
    if (msgid != DEAU_HASH_TABLE)
	return (DEAU_BAD_TABLE_IDS);

    for (i = 0; i < N_des; i++) {
	DEAU_attr_t *de;
	char *buf;

	de = Des[i].de;
	ret = DEAU_check_attributes (de);
	if (ret < 0)
	    return (ret);
	size = DEAU_serialize_dea (de, &buf);
	Tag = Get_write_tag (de);
	if ((ret = LB_write (Lb_fd, buf, size, LB_ANY)) < 0)
	    return (ret);
    }

    return (DEAU_update_hash_tables ());
}

/************************************************************************

    Updates the hash and identifier tables in the dea LB. Returns 0 on 
    success or a negative error code.

************************************************************************/

int DEAU_update_hash_tables () {
    DEAU_hash_e_t ht;
    DEAU_id_e_t it;
    char *hash_b, *id_b, *str_b;	/* buffers */
    int str_off, msgcnt, ret;
    DEAU_hash_tbl_t htbl;

    if (Lb_fd < 0 && (Lb_fd = Open_lb ()) < 0)
	return (Lb_fd);

    hash_b = STR_reset (NULL, 4096);
    id_b = STR_reset (NULL, 4096);
    str_b = STR_reset (NULL, 4096);

    htbl.is_big_endian = MISC_i_am_bigendian ();
    msgcnt = 0;
    hash_b = STR_append (hash_b, (char *)&htbl, sizeof (DEAU_hash_tbl_t));

    LB_seek (Lb_fd, 0, LB_FIRST, NULL);
    str_off = 0;
    msgcnt = 0;
    while (1) {
	DEAU_attr_t de;
	int msgid, len;
	char *p;

	msgid = DEAU_get_dea_by_msgid (LB_NEXT, &de);
	if (msgid == LB_TO_COME)
	    break;
	if (msgid < 0)
	    return (msgid);
	if (msgid == 0)
	    continue;
	p = de.ats[DEAU_AT_ID];
	if (p[0] == '@' && p[1] == '@') {	/* internal node */
	    LB_delete (Lb_fd, msgid);
	    continue;
	}
	ht.hash = DEAU_get_hash_value (p);
	ht.msgid = msgid;
	it.offset = str_off;
	it.msgid = msgid;
	hash_b = STR_append (hash_b, (char *)&ht, sizeof (DEAU_hash_e_t));
	id_b = STR_append (id_b, (char *)&it, sizeof (DEAU_id_e_t));
	len = strlen (de.ats[DEAU_AT_ID]) + 1;
	str_b = STR_append (str_b, de.ats[DEAU_AT_ID], len);
	str_off += len;
	msgcnt++;
    }

    Str_buf = str_b;
    qsort ((void *)id_b, (size_t)msgcnt, 
			sizeof (DEAU_id_e_t), 
			(int (*)(const void *, const void *))Identifier_comp);

    ret = Generate_internal_nodes (msgcnt, &hash_b,
				(DEAU_id_e_t *)id_b, str_b);
    if (ret < 0)
	return (ret);
    msgcnt += ret;

    qsort ((void *)(hash_b + sizeof (DEAU_hash_tbl_t)), (size_t)msgcnt, 
			sizeof (DEAU_hash_e_t), 
			(int (*)(const void *, const void *))Hash_comp);

    htbl.sizeof_des = msgcnt;
    hash_b = STR_replace (hash_b, 0, sizeof (DEAU_hash_tbl_t), 
				(char *)&htbl, sizeof (DEAU_hash_tbl_t));

    if ((ret = LB_write (Lb_fd, hash_b, STR_size (hash_b), DEAU_HASH_TABLE)) < 0)
	return (ret);

    STR_free (hash_b);
    STR_free (id_b);
    STR_free (str_b);
    return (0);
}

/************************************************************************

    Reads a new data attribute file and appends it to the data attribute
    table. Returns the number of new DEs on success or a negative error 
    code.

************************************************************************/

int DEAU_use_attribute_file (char *fname, int override) {
    static char *sec_buf = NULL;
    char *core_name, *line, *name_no_dir;
    int ret, cnt, core_len, id_cnt;

    if (fname == NULL) {
	Free_in_memory_DEA ();
	return (0);
    }

    core_name = NULL;	/* to disable gcc warning */
    core_len = Set_current_special_suffix (fname, &core_name);
    name_no_dir = Get_name_excluding_cfg_dir (fname);
    CS_cfg_name (fname);
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);
    CS_control (CS_KEY_OPTIONAL);
    CS_error (Cs_err_func);
    CS_set_single_level (1);

    ret = 0;
    cnt = 0;
    id_cnt = 0;
    line = CS_THIS_LINE;
    while (1) {
	char id[ID_TOKEN_SIZE], line_buf[TOKEN_SIZE];
	int line_num, short_format, comma_term;
	DEAU_attr_t *da;

        sec_buf = STR_reset (sec_buf, 512);
	ret = CS_entry (line, CS_FULL_LINE, TOKEN_SIZE, line_buf);
	if (ret == CS_END_OF_TEXT || ret == CS_KEY_NOT_FOUND) {
	    ret = 0;
	    break;
	}
	if (ret < 0)
	    break;
	if (ret == 0 ||
	    Save_UN_node (line_buf)) {
	    line = CS_NEXT_LINE;
	    continue;
	}
	ret = Is_an_id_line (line_buf, id, ID_TOKEN_SIZE, &short_format);
	if (ret <= 0) {
	    ret = DEAU_SYNTAX_ERROR;
	    CS_report ("Syntax error (at or before)");
	    break;
	}
	sec_buf = STR_append (sec_buf, line_buf + ret, 
					strlen (line_buf + ret));
	comma_term = Is_comma_term (sec_buf, STR_size (sec_buf));
	line = CS_NEXT_LINE;
	while (1) {		/* read continuing lines */
	    if (short_format)
		break;
	    ret = CS_entry (line, CS_FULL_LINE, TOKEN_SIZE, line_buf);
	    if (ret == 0)
		continue;
	    if (ret < 0)
		break;
	    if (!comma_term && !Is_an_attr_line (line_buf)) {
		line = CS_THIS_LINE;
		break;
	    }
	    sec_buf = STR_append (sec_buf, line_buf, strlen (line_buf));
	    comma_term = Is_comma_term (sec_buf, STR_size (sec_buf));
	}
	sec_buf = STR_append (sec_buf, "", 1);	/* terminate the section */

	if (Cr_suff != NULL && id[0] != '@') {
					/* insert suffix in identifier */
	    int l1, l2;

	    l1 = strlen (Cr_suff);
	    l2 = strlen (id);
	    if (l1 + l2 + 2 >= ID_TOKEN_SIZE) {
		CS_report ("Identifier too long (at or before)");
		ret = DEAU_NAME_TOO_LONG;
		break;
	    }
	    memmove (id + l1 + core_len + 2, id, l2 + 1);
	    strcpy (id, Cr_suff);
	    id[l1] = '.';
	    memcpy (id + l1 + 1, core_name, core_len);
	    id[l1 + core_len + 1] = '.';
	}
	line_num = cnt;
	ret = Add_new_data_entry (id, &da);
	if (ret < 0)
	    break;
	if (ret > 0)
	    id_cnt++;
	ret = Add_new_attribute (da, sec_buf, name_no_dir, line_num, override);
	if (ret < 0)
	    break;
	cnt++;
    }
    CS_set_single_level (0);
    CS_cfg_name ("");
    if (ret < 0)	
	return (DEAU_rep_err_ret ("Failed in reading file ", fname, ret));
    return (id_cnt);
}

/******************************************************************

    Discards the in-memory DEA read from the DEA files and frees all
    resources allocated for it.
	
******************************************************************/

static void Free_in_memory_DEA () {
    int i, k;
    for (i = 0; i < N_des; i++) {
	DEAU_attr_t *de;
	de = Des[i].de;
	for (k = 1; k < DEAU_AT_N_ATS; k++) {	/* ID is not malloced */
	    if (de->ats[k] != NULL && de->ats[k][0] != '\0')
		free (de->ats[k]);
	}
	free (de);
    }
    if (De_tbl != NULL)
	MISC_free_table (De_tbl);
    De_tbl = NULL;
    N_des = 0;
}

/******************************************************************

    Steps through the LB DEA data base, in the order of sorted
    hash values, to retrieve DE identifiers, if "id" is not
    NULL, and, if "at" is not NULL, DE attributes. If both "id"
    and "at" are NULL, the pointer is reset to the first DE. This
    function returns 0 on success or a negative error code.
	
******************************************************************/

int DEAU_get_next_dea (char **id, DEAU_attr_t **at) {
    int ret;
    LB_id_t msgid;

    if (id == NULL && at == NULL) {
	Next_dea_index = 0;
	return (0);
    }

    if ((ret = Read_hash_tbl ()) < 0)
	return (ret);
    if (Hash_tbl->sizeof_des <= 0 || Next_dea_index >= Hash_tbl->sizeof_des)
	return (DEAU_DE_NOT_FOUND);

    msgid = Hash_tbl->des[Next_dea_index].msgid;
    if ((ret = DEAU_get_dea_by_msgid (msgid, &At_buf)) < 0)
	return (ret);
    if (at != NULL)
	*at = &At_buf;
    if (id != NULL)
	*id = At_buf.ats[0];
    Next_dea_index++;
    return (0);
}

/******************************************************************

    Sets the current index for DEAU_get_next_dea to "ind" if "ind"
    >= 0. Returns the current index.
	
******************************************************************/

int DEAU_set_next_dea (int ind) {
    if (ind >= 0)
	Next_dea_index = ind;
    return (Next_dea_index);
}

/******************************************************************

    Searches data element of identifier "id". Returns the data element
    with "at_p" if it is found. Returns 0 on success, or a negative 
    error code.
	
******************************************************************/

int DEAU_get_attr_by_id (char *id, DEAU_attr_t **at_p) {
    unsigned int hash;
    int ret;

    if (at_p == NULL)
	return (DEAU_BAD_ARGUMENT);
    if (id == NULL) {
	*at_p = &At_buf;
	return (0);
    }
    if (Get_cache_attr_by_id != NULL &&
	(*at_p = Get_cache_attr_by_id (id, &At_buf)) != NULL)
	return (0);
    hash = DEAU_get_hash_value (id);
    if ((*at_p = Get_in_memory_attr_by_id (id, hash)) != NULL)
	return (0);
    ret = Get_shared_attr_by_id (id, hash, at_p, NULL);
    return (ret);
}

/******************************************************************

    Locks data element "id" which can be either an data element or
    an internal node. "lock_cmd" is one of the lock command for 
    LB_lock. It returns one of the LB_lock return values or a DEAU
    error code.
	
******************************************************************/

int DEAU_lock_de (char *id, int lock_cmd) {
    int ret;
    LB_id_t msgid;
    DEAU_attr_t *at_p;

    if (id == NULL)
	return (DEAU_BAD_ARGUMENT);

    ret = Get_shared_attr_by_id (id, DEAU_get_hash_value (id), &at_p, &msgid);
    if (ret == DEAU_DE_NOT_FOUND) {
	id = DEAU_get_node_name (id);
	ret = Get_shared_attr_by_id (id, 
				DEAU_get_hash_value (id), &at_p, &msgid);
    }
    if (ret < 0)
	return (ret);
    return (LB_lock (Lb_fd, lock_cmd, msgid));
}

/******************************************************************

    Returns the message ID in the DEA DB for DE "id". Returns 
    0xffffffff if not found.
	
******************************************************************/

int DEAU_get_msg_id (char *id) {
    int ret;
    LB_id_t msgid;
    DEAU_attr_t *at_p;

    if (id == NULL)
	return (DEAU_BAD_ARGUMENT);
    ret = Get_shared_attr_by_id (id, DEAU_get_hash_value (id), &at_p, &msgid);
    if (ret < 0)
	return (0xffffffff);
    return (msgid);
}

/******************************************************************

    Searches data element of identifier "id", with hash value "hash",
    in the shared DEA LB. Returns the data element with "at_p" if it 
    is found. Returns 0 on success or a negative error code.
	
******************************************************************/

static int Get_shared_attr_by_id (char *id, unsigned int hash, 
				DEAU_attr_t **at_p, LB_id_t *msgid) {
    DEAU_hash_e_t he, *first, *last;
    int ind, ret;

    /* go ahead to search the LB */
    if ((ret = Read_hash_tbl ()) < 0)
	return (ret);

    he.hash = hash;
    if (MISC_bsearch (&he, Hash_tbl->des, Hash_tbl->sizeof_des, 
			sizeof (DEAU_hash_e_t), Hash_comp, &ind) <= 0)
	return (DEAU_DE_NOT_FOUND);
    first = Hash_tbl->des + ind;
    last = Hash_tbl->des + (Hash_tbl->sizeof_des - 1);
    while (first <= last) {
	int ret;

	if (first->hash != hash)
	    break;
	if ((ret = DEAU_get_dea_by_msgid (first->msgid, &At_buf)) < 0)
	    return (ret);
	if (strcmp (At_buf.ats[DEAU_AT_ID], id) == 0) {
	    if (msgid != NULL)
		*msgid = first->msgid;
	    *at_p = &At_buf;
	    return (0);
	}
	first++;
    }
    return (DEAU_DE_NOT_FOUND);
}

/******************************************************************

    Sets the attribute "which_attr" of data element "id" to 
    "text_attr" which is the text form attribute reprentation. 
    "which_attr" must be one of "DEAU_AT_*" defined in deau.h. It 
    returns 0 on success or a negative error code. Refer to "man
    DEAU_update_attr".
	
******************************************************************/

int DEAU_update_attr (char *id, int which_attr, char *text_attr) {
    int size, ret;
    char *buf, *old_at;
    DEAU_attr_t *attr;
    LB_id_t msgid;

    if (id == NULL || text_attr == NULL)
	return (DEAU_BAD_ARGUMENT);
    ret = Get_shared_attr_by_id (id, DEAU_get_hash_value (id), &attr, &msgid);
    if (ret < 0)
	 return (ret);

    if (strcmp (attr->ats[which_attr], text_attr) == 0)
	return (0);

    old_at = attr->ats[which_attr];
    attr->ats[which_attr] = text_attr;
    ret = DEAU_check_attributes (attr);
    if (ret < 0) {
	attr->ats[which_attr] = old_at;
	return (ret);
    }
    size = DEAU_serialize_dea (attr, &buf);
    attr->ats[which_attr] = old_at;
    if ((ret = LB_write (Lb_fd, buf, size, msgid)) < 0)
	return (ret);
    if (attr == &At_buf) {	/* we do not assume text_attr is static */
	if (strlen (text_attr) > strlen (attr->ats[which_attr]))
	    attr->ats[0] = NULL;	/* At_buf will be reread (content not reused) */
	else
	    strcpy (attr->ats[which_attr], text_attr);
    }
    else {	/* text_attr must be static - This however should never happen */
	MISC_log ("DEAU: Unexpected - make sure text_attr static\n");
	attr->ats[which_attr] = text_attr;
    }

    return (0);
}

/******************************************************************

    Registers an DEA update notification callback function "callback"
    for DE group "de_group". A DE group is a collection of DEs whose
    IDs are different from each other only in the last ID segment.
	
******************************************************************/

int DEAU_UN_register (char *de_group, 
		void (*notify_func)(int, EN_id_t, int, void *)) {

    if (Lb_fd < 0 && (Lb_fd = Open_lb ()) < 0)
	return (Lb_fd);
    if (de_group == NULL || de_group[0] == '\0')
	return (LB_UN_register (Lb_fd, LB_ANY, notify_func));
    else {
	int ret, tag_v;
	tag_v = Get_tag_value (de_group, 0);
	ret = LB_UN_register (Lb_fd, LB_UN_MSGID_GROUP (tag_v), notify_func);
	if (ret < 0)
	    return (ret);
	return (tag_v);
    }
}

/**************************************************************************

    Computes and returns the LB message tag value of name "name". The tag
    value is used for DEA update notification. If "is_full_name" is non-zero,
    The last part of the name is removed.

**************************************************************************/

static unsigned int Get_tag_value (char *name, int is_full_name) {
    char *p, *end, *n;
    unsigned int h;

    n = name;
    end = NULL;
    if (is_full_name) {
	p = name;
	while (*p != '\0') {
	    if (*p == '.')
		end = p;
	    p++;
	}
	if (end != NULL)
	    *end = '\0';
	else
	    n = "";
    }
    h = DEAU_get_hash_value (n);
    if (end != NULL)
	*end = '.';
    return ((h + (h > 16)) & 0xffff);
}

/******************************************************************

    Reports an error message consisting of "msg" and "desc" and 
    returns "ret_value".
	
******************************************************************/

int DEAU_rep_err_ret (char *msg, char *desc, int ret_value) {
    static char *vb = NULL;

    if (Error_func != NULL) {
	vb = STR_reset (vb, 512);
	vb = STR_copy (vb, "DEAU: ");
	vb = STR_cat (vb, msg);
	vb = STR_cat (vb, desc);
	vb = STR_cat (vb, "\n");
	Error_func (vb);
    }
    return (ret_value);
}

/******************************************************************

    Searches data entry of name "name". Returns the data entry if 
    it is found. Returns NULL otherwise.
	
******************************************************************/

static DEAU_attr_t *Get_in_memory_attr_by_id (char *id, unsigned int hash) {
    Db_hash_t da, *first, *last;
    int ind;

    if (N_des <= 0)
	return (NULL);
    if (!De_sorted) {		/* sort the data item table */
	qsort ((void *)Des, (size_t)N_des, sizeof (Db_hash_t), 
			(int (*)(const void *, const void *))Sort_comp);
	De_sorted = 1;
    }

    da.hash = hash;
    if (MISC_bsearch (&da, Des, N_des, sizeof (Db_hash_t), 
						Sort_comp, &ind) <= 0)
	return (NULL);

    first = Des + ind;
    last = Des + (N_des - 1);
    while (first <= last) {
	if (first->hash != da.hash)
	    break;
	if (strcmp (first->de->ats[DEAU_AT_ID], id) == 0) {
	    memcpy (&At_buf, first->de, sizeof (DEAU_attr_t));
	    return (&At_buf);
	}
	first++;
    }
    return (NULL);
}

/******************************************************************

    Comparison function for sorting the data item table.
	
******************************************************************/

static int Sort_comp (void *a1, void *a2) {
    Db_hash_t *d1, *d2;

    d1 = (Db_hash_t *)a1;
    d2 = (Db_hash_t *)a2;
    if (d1->hash < d2->hash)
	return (-1);
    else if (d1->hash > d2->hash)
	return (1);
    else
	return (0);
}

/**************************************************************************

    The CS error reporting function.

**************************************************************************/

static void Cs_err_func (char *msg) {
    char *p = msg + strlen (msg) - 1;
    if (p >= msg && *p == '\n')
	*p = '\0';
    DEAU_rep_err_ret (msg, "", 0);
}

/******************************************************************

    Adds a new data attribute file line in "desc" to data element
    "da". Returns 0 on success or a negative error code on failure.
	
******************************************************************/

static int Add_new_attribute (DEAU_attr_t *da, char *desc, char *fname,
					int line_num, int override) {
    char *spec, *buf, *text;
    int att;

    if (desc[0] == '\0')
	return (0);
    text = desc;
    spec = NULL;	/* to disable gcc warning */
    while (1) {
	att = Get_next_attr (text, &spec);
	if (att < 0)
	    return (att);
	if (att == 0)
	    break;
	text = NULL;

	if (att == DEAU_AT_TYPE &&
		DEAU_get_data_type_by_string (spec) < 0) {
	    CS_report ("Bad type name (at or before)");
	    return (DEAU_BAD_TYPE_NAME);
	}

	if (att == DEAU_AT_VALUE &&
	    (da->ats[DEAU_AT_MISC][0] == '\0' ||
	     strstr (da->ats[DEAU_AT_MISC], "SRC@-") == NULL)) {
	    int s = strlen (fname) + 1;
	    if (line_num >= 0)
		s += 16;
	    buf = (char *)malloc (s);
	    if (buf == NULL)
		return (DEAU_MALLOC_FAILED);
	    if (line_num >= 0)
		sprintf (buf, "SRC@-%s::%d", fname, line_num);
	    else
		sprintf (buf, "SRC@-%s", fname);
	    da->ats[DEAU_AT_MISC] = Append_misc_attr 
					(da->ats[DEAU_AT_MISC], buf);
	    if (da->ats[DEAU_AT_MISC] == NULL)
		return (DEAU_MALLOC_FAILED);
	}
	buf = (char *)malloc (strlen (spec) + 1);
	if (buf == NULL)
	    return (DEAU_MALLOC_FAILED);
	strcpy (buf, spec);

/* printf ("    Add attr %d, %s, to id %s\n", att, buf, da->ats[0]); */
    
	if (att == DEAU_AT_MISC) {
	    da->ats[att] = Append_misc_attr (da->ats[att], buf);
	    if (da->ats[att] == NULL)
		return (DEAU_MALLOC_FAILED);
	}
	else if (da->ats[att][0] != '\0' && 
			    strcmp (da->ats[att], buf) != 0 && !override) {
	    free (buf);
	    CS_report ("Attribute respecified (at or before)");
	    return (DEAU_ATTR_RESPECIFIED);
	}
	else {
	    if (da->ats[att][0] != '\0')
		free (da->ats[att]);
	    da->ats[att] = buf;
	}
    }

    return (0);
}

/******************************************************************

    Parses DEA text "text" and returns the type of the first attribute.
    The attribute description is returned with "spec_p". If "text" is
    NULL, the next attribute is parsed. If no more attribute is found,
    0 is returned. In case of error, a negative error code is returned.
    Note that "spec_p" returned may be an empty string.
	
******************************************************************/

static int Get_next_attr (char *text, char **spec_p) {
    static char *st;
    static int next_type = 0;
    int type, ntype;
    char *spec;

    if (text != NULL) {
	st = text;
	type = Get_next_attr_type (1, text, &spec);
	if (type == 0) {
	    st = Remove_spaces (text);
	    if (*st != ':') {
		CS_report ("Bad attr spec (at or before)");
		return (DEAU_BAD_ATTR_SPEC);
	    }
	    st++;
	    *spec_p = Remove_spaces (st);
	    next_type = 0;
	    return (DEAU_AT_VALUE);
	}
	st = spec;
	next_type = type;
    }
    else if (next_type == 0)
	return (0);

    type = next_type;
    ntype = Get_next_attr_type (0, st, &spec);
    *spec_p = Remove_spaces (st);
    if (ntype == 0)
	next_type = 0;
    else
	st = spec;
    next_type = ntype;
    st = spec;
    return (type);
}

/***********************************************************************

    Determine if the first token in "line" from DEA file is an DE ID.
    If it is an ID, the id is copied to "id" of size "id_size". Returns 
    0 if the line does not have an ID. Otherwise, returns the offset to 
    the DEA spec after the ID and sets "short_format" to non-zero if the 
    line is in short format ("ID: value").

***********************************************************************/

static int Is_an_id_line (char *line, char *id, 
					int id_size, int *short_format) {
    char *p, *desc, *st;
    int len, id_len;

    p = line;
    p = Skip_space (p);
    if (Is_an_attr_line (line))			/* an attribute spec */
	return (0);
    st = p;
    len = Get_token_len (p);
    p += len;
    p = Skip_space (p);				/* points to second token */
    id_len = len;
    if (len > 1 && st[len - 1] == ':') {
	id_len = len - 1;
	p = st + len - 1;
    }
    *short_format = 0;
    if (*p == ':') {
	char *p1 = p + 1;
	p1 = Skip_space (p1);
	if (p1[0] == '\0' ||
	    Get_next_attr_type (1, p1, &desc) > 0)
	    return (0);
	*short_format = 1;
    }
    else if (*p != '\0' && Get_next_attr_type (1, p, &desc) <= 0)
	return (0);
    if (id_len >= id_size)
	return (0);
    memcpy (id, st, id_len);
    id[id_len] = '\0';
    return (p - line);
}

/***********************************************************************

    Determines if "line" is a attribute spec line. Returns non-zero if 
    it is true or 0 otherwise.

***********************************************************************/

static int Is_an_attr_line (char *line) {
    char *p, *desc;

    p = line;
    p = Skip_space (p);
    if (Get_next_attr_type (1, p, &desc) > 0)	/* an attribute spec */
	return (1);
    return (0);
}

/***********************************************************************

    Returns non-zero if the last non-space char of "str" of length "strsize"
    is ",". Returns 0 otherwise.

***********************************************************************/

static int Is_comma_term (char *str, int strsize) {
    char *p;
    p = str + strsize - 1;
    while (p >= str && (*p == ' ' || *p == '\t'))
	p--;
    if (*p == ',')
	return (1);
    return (0);
}

/***********************************************************************

    Returns the token length started with "str".

***********************************************************************/

static int Get_token_len (char *str) {
    char *p, c;
    p = str;
    while ((c = *p) != '\0' && c != ' ' && c != '\t')
	p++;
    return (p - str);
}

/***********************************************************************

    Returns the pointer to the next non-space char in "str".

***********************************************************************/

static char *Skip_space (char *str) {
    char *p, c;
    p = str;
    while ((c = *p) == ' ' || c == '\t' || c == '\n')
	p++;
    return (p);
}

/******************************************************************

    Searches string "text" to find the first attr type name. It terminates
    the string before the found attr type name, returns the type and
    the pointer to the found attr description. It returns 0 if no
    attribute type is found. If "first" is non-zero, the type name
    must be the first token in "text".
	
******************************************************************/

static int Get_next_attr_type (int first, char *text, char **desc) {
    char *eq;

    eq = text;
    while (1) {
	char *p, *ts, *te;
	int type;

	while (*eq != '\0' && *eq != '=')
	    eq++;
	if (*eq != '=')
	    return (0);
	p = eq - 1;
	while (p >= text && (*p == ' ' || *p == '\t' || *p == '\n'))
	    p--;
	te = p;
	while (p >= text && *p != ';')
	    p--;
	if (first && p >= text)
	    return (0);
	ts = p + 1;
	ts = Skip_space (ts);
	type = Test_type_name (ts, te);
	if (type > 0) {
	    if (p >= text)
		*p = '\0';
	    *desc = eq + 1;
	    return (type);
	}
	eq++;
    }
}

/******************************************************************

    Tests the token started with "st" and ended with "end" to determine
    if it is a attribute type name. Returns the type macro on success
    or 0 on failure.
	
******************************************************************/

static int Test_type_name (char *st, char *end) {
    char c;
    int type;

    if (st > end)
	return (0);
    c = end[1];
    end[1] = '\0';

    for (type = 1; type < DEAU_AT_N_ATS; type++)
	if (strcasecmp (st, Attr_names[type]) == 0)
	    break;
    if (type >= DEAU_AT_N_ATS)
	type = 0;
    end[1] = c;
    return (type);
}

/******************************************************************

    Returns pointer to the first non-spece character.
	
******************************************************************/

static char *Remove_spaces (char *text) {
    char *p;
    p = text + strlen (text) - 1;
    while (p >= text && (*p == ' ' || *p == '\t' || *p == '\n' || *p == ';')) {
	*p = '\0';
	p--;
    }
    p = text;
    while (*p == ' ' || *p == '\t' || *p == '\n')
	p++;
    return (p);
}

/******************************************************************

    Comparison function for sorting the LB hash table.
	
******************************************************************/

static int Identifier_comp (void *a1, void *a2) {
    DEAU_id_e_t *d1, *d2;

    d1 = (DEAU_id_e_t *)a1;
    d2 = (DEAU_id_e_t *)a2;
    return (strcmp (Str_buf + d1->offset, Str_buf + d2->offset));
}

/******************************************************************

    Comparison function for sorting the LB hash table.
	
******************************************************************/

static int Hash_comp (void *a1, void *a2) {
    DEAU_hash_e_t *d1, *d2;

    d1 = (DEAU_hash_e_t *)a1;
    d2 = (DEAU_hash_e_t *)a2;
    if (d1->hash < d2->hash)
	return (-1);
    else if (d1->hash > d2->hash)
	return (1);
    else
	return (0);
}

/******************************************************************

    Updates local LB DE hash table, Hash_tbl. Returns 0 on success
    or a negative error code.
	
******************************************************************/

static int Read_hash_tbl () {
    char *buf;
    int s, off, ts;
    DEAU_hash_tbl_t *h;

    if (Hash_tbl != NULL)	/* no automatic dynamic update */
	return (0);

    if (Lb_fd < 0 && (Lb_fd = Open_lb ()) < 0)
	return (Lb_fd);

    s = LB_read (Lb_fd, &buf, LB_ALLOC_BUF, DEAU_HASH_TABLE);
    if (s <= 0)
	return (DEAU_HASH_TABLE_NOT_FOUND);
    h = (DEAU_hash_tbl_t *)buf;
    off = sizeof (DEAU_hash_tbl_t);
    if (s >= off && 
	s == (ts = INT_BSWAP (h->sizeof_des)) * (int)sizeof (DEAU_hash_e_t) + off) {
	int i;
	char *p = buf + off;
	for (i = 0; i < ts; i++) {
	    DEAU_hash_e_t *ent;
	    ent = (DEAU_hash_e_t *)p;
	    p += sizeof (DEAU_hash_e_t);
	    ent->hash = INT_BSWAP (ent->hash);
	    ent->msgid = INT_BSWAP (ent->msgid);
	}
	h->is_big_endian = INT_BSWAP (h->is_big_endian);
	h->sizeof_des = INT_BSWAP (h->sizeof_des);
    }

    if (s < off || s != h->sizeof_des * (int)sizeof (DEAU_hash_e_t) + off) {
	free (buf);
	return (DEAU_BAD_HASH_TABLE);
    }
    h->des = (DEAU_hash_e_t *)(buf + off);
    Hash_tbl = h;
    return (0);
}

/************************************************************************

    Serializes the DEA struct "de" and returns it with "buf". Returns the
    number of bytes of the serialized data on success or a negative 
    error number.

************************************************************************/

int DEAU_serialize_dea (DEAU_attr_t *de, char **buf) {
    static char *vb = NULL;
    int i;

    vb = STR_reset (vb, 512);
    for (i = 0; i < DEAU_AT_N_ATS; i++) {
	if (de->ats[i] == NULL || strlen (de->ats[i]) == 0)
	    vb = STR_append (vb, "", 1);
	else
	    vb = STR_append (vb, de->ats[i], strlen (de->ats[i]) + 1);
    }
    *buf = vb;
    return (STR_size (vb));
}

/************************************************************************

    Reads the dea struct with message id "msgid" from the LB. Returns 
    the message id if the message is a DEA, 0 if not or a negative error 
    code.

************************************************************************/

int DEAU_get_dea_by_msgid (LB_id_t msgid, DEAU_attr_t *de) {
    static char *buf = NULL;
    int s, i, cnt;

    de->ats[0] = NULL;
    if (Lb_fd < 0 && (Lb_fd = Open_lb ()) < 0)
	return (Lb_fd);

    if (buf != NULL) {
	free (buf);
	buf = NULL;
    }
    s = LB_read (Lb_fd, &buf, LB_ALLOC_BUF, msgid);
    if (s < 0)
	return (s);
    msgid = LB_previous_msgid (Lb_fd);
    if (msgid == DEAU_HASH_TABLE) {
	free (buf);
	buf = NULL;
	return (0);
    }

    buf[s - 1] = '\0';
    cnt = 0;
    for (i = 0; i < DEAU_AT_N_ATS; i++) {
	if (cnt >= s)
	    break;
	de->ats[i] = buf + cnt;
	cnt += strlen (buf + cnt) + 1;
    }
    if (i < DEAU_AT_N_ATS) {
	de->ats[0] = NULL;
	free (buf);
	buf = NULL;
	return (DEAU_BAD_DEA_MSG);
    }

    return (msgid);
}

/************************************************************************

    Opens the DEA data base LB. Returns 0 on success or a
    negative error number.

************************************************************************/

static int Open_lb () {
    int fd;

    if (Lb_name[0] == '\0')
	return (DEAU_LB_UNDEFINED);
    fd = LB_open (Lb_name, LB_WRITE, NULL);
    if (fd < 0)
	return (DEAU_rep_err_ret ("Could not open LB: ", Lb_name, fd));
    LB_register (fd, LB_TAG_ADDRESS, &Tag);
    return (fd);
}

/******************************************************************

    Adds a new entry to the data item table. The new entry is 
    returned with "data_entry". If the entry exists,
    the old entry is returned

    Returns 1 if a new DE is added, 0 if it is an existing DE, or a 
    negative error code on failure.
	
******************************************************************/

static int Add_new_data_entry (char *data_id, 
					DEAU_attr_t **data_entry) {
    unsigned int hash;
    Db_hash_t *new_entry;
    DEAU_attr_t *new_de;
    char *id;
    int i;

    if (De_tbl == NULL) {
	De_tbl = MISC_open_table (sizeof (Db_hash_t), 
					64, 0, &N_des, (char **)&Des);
	if (De_tbl == NULL)
	    return (DEAU_MALLOC_FAILED);
	De_sorted = 0;
    }

    hash = DEAU_get_hash_value (data_id);
    for (i = 0; i < N_des; i++) {
	if (hash == Des[i].hash &&
	    strcmp (data_id, Des[i].de->ats[DEAU_AT_ID]) == 0) {
	    *data_entry = Des[i].de;
	    return (0);
	}
    }
    
    new_entry = (Db_hash_t *)MISC_table_new_entry (De_tbl, NULL);
    id = (char *)malloc (sizeof (DEAU_attr_t) + strlen (data_id) + 1);
    if (new_entry == NULL || id == NULL)
	return (DEAU_MALLOC_FAILED);
    new_de = (DEAU_attr_t *)id;
    id += sizeof (DEAU_attr_t);
    De_sorted = 0;
    strcpy (id, data_id);
    new_entry->hash = hash;
    new_entry->de = new_de;
    new_de->ats[DEAU_AT_ID] = id;
    for (i = 1; i < DEAU_AT_N_ATS; i++)
	new_de->ats[i] = "";
    *data_entry = new_de;
    return (1);
}

/************************************************************************

    Sets the current special suffix "Cr_suff" according to the current file
    name "fname". If the file has a special suffix, the function returns
    the name core (file name excluding the dir and the suffix) with 
    "core_name". The return value is the length of the core name.

************************************************************************/

static int Set_current_special_suffix (char *fname, char **core_name) {
    char *cpt, *suf, *name;
    int i;

    Cr_suff = NULL;
    if (N_suffs == 0)
	return (0);
    suf = NULL;
    cpt = fname;
    name = fname;
    while (*cpt != '\0') {
	if (*cpt == '.')
	    suf = cpt + 1;
	if (*cpt == '/')
	    name = cpt + 1;
	cpt++;
    }
    if (suf == NULL || suf < name)
	return (0);
    cpt = Suffs;
    for (i = 0; i < N_suffs; i++) {
	if (strcmp (cpt, suf) == 0) {
	    Cr_suff = cpt;
	    *core_name = name;
	    return (suf - name - 1);
	}
	cpt += strlen (cpt) + 1;
    }
    return (0);
}
    
/************************************************************************

    Generates internal nodes of the DE identifier tree. The identifiers
    are stored in buffer "idbuf" with offsets in "des". "decnt" is the 
    total number of identifiers. The identifier list "des" is sorted.
    The new internal node messages are appended to "hash_b". The function
    returns the number of internal nodes generated on success or a 
    negative error code.

************************************************************************/

#define MAX_N_SECTIONS 16

static int Generate_internal_nodes (int decnt, 
			char **hash_b, DEAU_id_e_t *des, char *idbuf) {
    int cnt, n_css[MAX_N_SECTIONS], i, node_cnt;
    char *cs[MAX_N_SECTIONS];

    for (i = 0; i < MAX_N_SECTIONS; i++) {
	n_css[i] = 0;
	cs[i] = STR_reset (NULL, 512);
    }
    node_cnt = 0;
    cnt = 0;
    while (1) {
	char *id, *s[MAX_N_SECTIONS], *ls[MAX_N_SECTIONS];
	int n_s, ret;

	if (cnt < decnt)
	    id = idbuf + (des[cnt].offset);
	else
	    id = NULL;

	n_s = Get_sections (id, s);
	for (i = 0; i <= n_s; i++) {
	    if (i == n_s) {
		if ((ret = Save_int_nodes (i, n_css, cs, hash_b)) < 0)
		    return (ret);
		node_cnt += ret;
		break;
	    }
	    else if (n_css[i] == 0 || strcmp (s[i], ls[i]) != 0) {
		if ((ret = Save_int_nodes (i + 1, n_css, cs, hash_b)) < 0)
		    return (ret);
		node_cnt += ret;
		n_css[i]++;
		cs[i] = STR_append (cs[i], (char *)&(s[i]), sizeof (char *));
		ls[i] = s[i];
	    }
	}
	cnt++;
	if (cnt > decnt)
	    break;
    }
    for (i = 0; i < MAX_N_SECTIONS; i++)
	STR_free (cs[i]);
    return (node_cnt);
}

/************************************************************************

    Devides "id" into sections and returns the pointers to the sections
    with "s". Returns the number of sections found.

************************************************************************/

static int Get_sections (char *id, char **s) {
    char *p;
    int cnt;

    if (id == NULL)
	return (0);
    p = id;
    cnt = 1;
    s[0] = p;
    while (*p != '\0') {
	if (*p == '.') {
	    *p = '\0';
	    if (cnt >= MAX_N_SECTIONS) 
		break;
	    s[cnt] = p + 1;
	    cnt++;
	}
	p++;
    }
    return (cnt);
}

/************************************************************************

    Generates and saves internal nodes using info in "cs" and "n_css" 
    started with elment index of "st". Return the number node generated
    on success or a negative error code.

************************************************************************/

static int Save_int_nodes (int st, int *n_css, 
				char **cs, char **hash_b) {
    static char *id = NULL, *value = NULL;
    int i, cnt;

    id = STR_reset (id, 512);
    cnt = 0;
    for (i = MAX_N_SECTIONS - 1; i >= st; i--) {
	int k, size, ret;
	char *p, *buf;
	DEAU_hash_e_t ht;
	DEAU_attr_t de;

	if (n_css[i] <= 0)
	    continue;
	id = STR_copy (id, "@@");
	for (k = 0; k < i; k++) {
	    p = ((char **)(cs[k]))[n_css[k] - 1];
	    id = STR_cat (id, p);
	    if (k < i - 1)
		id = STR_cat (id, ".");
	}
	value = STR_reset (value, 1024);
	for (k = 0; k < n_css[i]; k++) {
	    p = ((char **)(cs[i]))[k];
	    if (k > 0)
		value = STR_cat (value, ", ");
	    value = STR_cat (value, p);
	}

	for (k = 0; k < DEAU_AT_N_ATS; k++)
	    de.ats[k] = "";
	de.ats[DEAU_AT_ID] = id;
	de.ats[DEAU_AT_VALUE] = value;
	size = DEAU_serialize_dea (&de, &buf);
	Tag = 0;
	if ((ret = LB_write (Lb_fd, buf, size, LB_ANY)) < 0)
	    return (ret);
	ht.hash = DEAU_get_hash_value (id);
	ht.msgid = LB_previous_msgid (Lb_fd);
	*hash_b = STR_append (*hash_b, (char *)&ht, sizeof (DEAU_hash_e_t));

	STR_copy (cs[i], NULL);
	n_css[i] = 0;
	cnt++;
    }
    return (cnt);
}

/************************************************************************

    Returns the point to the part of "fname" that has CFG dir removed.

************************************************************************/

static char *Get_name_excluding_cfg_dir (char *fname) {
    char b[256];
    int n;

    if ((n = MISC_get_cfg_dir (b, 254)) < 0)
	return (fname);
    b[n] = '/';
    b[n + 1] = '\0';
    if (strncmp (fname, b, n) == 0)
	return (fname + n + 1);
    return (fname);
}

/***********************************************************************

    Determines if "line" is a DE_UN_node specification. If it is, the
    spec is extracted and stored in De_un_nodes. Returns 1 if the line is
    or 0 if the line is not. Assuming there is no leading space in line".

***********************************************************************/

static int Save_UN_node (char *line) {
    char *p;

    if (strncmp (line, "DE_UN_node", 10) != 0)
	return (0);
    p = line;
    p[Get_token_len (p)] = '\0';
    if (strcmp (p, "DE_UN_node") != 0)
	return (0);
    p += strlen (p) + 1;
    p = Skip_space (p);
    p[Get_token_len (p)] = '\0';
    if (strlen (p) > 0) {
	De_un_nodes = STR_append (De_un_nodes, p, strlen (p) + 1);
	N_de_un_nodes++;
	return (1);
    }
    return (0);
}

/************************************************************************

    Get the tag value for DE "de" when creating the DE message.

************************************************************************/

static int Get_write_tag (DEAU_attr_t *de) {
    char *p, *node, *id;
    int k, len;

    p = De_un_nodes;
    node = NULL;
    for (k = 0; k < N_de_un_nodes; k++) {
	len = strlen (p);
	id = de->ats[DEAU_AT_ID];
	if (strncmp (p, id, len) == 0 &&
	    id[len] == '.') {
	    return (Get_tag_value (p, 0));
	}
	p += len + 1;
    }
    return (Get_tag_value (de->ats[DEAU_AT_ID], 1));
}

/******************************************************************

    Appends string "src" to the existing attribute "misc". Returns
    the appended string on success or NULL on failure.
	
******************************************************************/

static char *Append_misc_attr (char *misc, char *src) {
    char *b;

    if (misc[0] == '\0')
	return (src);
    b = (char *)malloc (strlen (misc) + strlen (src) + 2);
    if (b == NULL)
	return (NULL);
    sprintf (b, "%s %s", misc, src);
    if (misc[0] != '\0')
	free (misc);
    free (src);
    return (b);
}

/******************************************************************

    Removes "n_rm" data elements and adds new DE entris in "n_files"
    files to the DEA database "db_name". "ids" passes the ids to be
    removed and "files" are the file names containing new DEAs. They are
    both consecutive null-terminated strings. This function recreates
    the database. Thus it can only be used off line.
	
******************************************************************/

int DEAU_rm_add_des (int n_files, char *files, int n_rm, char *ids) {
    char *id, *addf;
    DEAU_attr_t *at, *da;
    int ret, len, i;

    Free_in_memory_DEA ();
    DEAU_get_next_dea (NULL, NULL);
    while (DEAU_get_next_dea (&id, &at) == 0) {
	char *rmid = ids;
	for (i = 0; i < n_rm; i++) {
	    if (strcmp (id, rmid) == 0)
		break;
	    rmid += strlen (rmid) + 1;
	}
	if (i < n_rm)
	    continue;
	if ((ret = Add_new_data_entry (id, &da)) < 0)
	    return (ret);
	for (i = 0; i < DEAU_AT_N_ATS; i++) {
	    if (i == DEAU_AT_ID)
		continue;
	    len = strlen (at->ats[i]);
	    if (len > 0) {
		da->ats[i] = malloc (len + 1);
		if (da->ats[i] == NULL)
		    return (DEAU_MALLOC_FAILED);
		strcpy (da->ats[i], at->ats[i]);
	    }
	}
    }
    addf = files;
    for (i = 0; i < n_files; i++) {
	if ((ret = DEAU_use_attribute_file (addf, 1)) < 0)
	    return (ret);
	addf += strlen (addf) + 1;
    }
    ret = DEAU_create_dea_db ();
    if (ret < 0)
	return (ret);
    return (0);
}

