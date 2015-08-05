
/**************************************************************************

    Data element attribute utility library module - remote access. This
    module is for supporting low-bandwidth/long-delay remote access to
    the DEA database.

**************************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 15:40:34 $
 * $Id: deau_remote.c,v 1.9 2005/09/14 15:40:34 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <infr.h>
#include "deau_def.h"


static char *Db_host = NULL;		/* host name of DEA db */
static char *Lb_name = NULL;		/* LB name of DEA db */

static char *Cdes = NULL;		/* cached DEs read from remote host */
static int N_cdes = 0;			/* number of cached DEs */

static unsigned int Attr_mask = 0xffffffff;
					/* attribute bit mask */
static char *Rpc_name = NULL;		/* RPC name buffer */


static char *Get_lb_name ();
static int Process_id_list (char **b, int need_expand);
static void Recover_null_in_string (char *str);
static void Hide_null_in_string (char *str, int len);
static DEAU_attr_t *Get_cache_attr_by_id (char *id, DEAU_attr_t *at_buf);
static void Replace_char (char *str, char from, char to);


/**************************************************************************

    Sets the attribute type used by DEAU_read_listed_attrs.

**************************************************************************/

void DEAU_set_read_attr_type (int attr, int yes) {
    if (attr == DEAU_AT_ID)
	return;
    if (yes)
	Attr_mask |= (1 << attr);
    else
	Attr_mask &= ~(1 << attr);
}

/**************************************************************************

    Reads a list of DEAs through RPC for reducing number of RPCs and number
    of bytes to read remotely. See "man deau" for further details.

**************************************************************************/

int DEAU_read_listed_attrs (char *id_list) {
    DEAU_attr_t de;
    char *result, *p;
    int n_des, i, ret;

    if (Lb_name == NULL && Get_lb_name () == NULL)
	return (DEAU_LB_UNDEFINED);

    if (Db_host == NULL)
	return (0);

    DEAU_set_get_cache_attr_by_id (Get_cache_attr_by_id);

    if (N_cdes == 0)
	Cdes = STR_reset (Cdes, sizeof (DEAU_attr_t) * 100);

    /* This is RPC */
    Rpc_name = STR_copy (Rpc_name, Db_host);
    Rpc_name = STR_cat (Rpc_name, ":");
    Rpc_name = STR_cat (Rpc_name, "DEAU_read_listed_remote");
    ret = RSS_rpc (Rpc_name, "i-r s-i s-i i-i s-o", &n_des, 
				Lb_name, id_list, Attr_mask, &result);

    if (ret < 0)
	return (ret);
    if (n_des <= 0)
	return (n_des);

    Recover_null_in_string (result);

    /* save the result in cache */
    p = result;
    for (i = 0; i < n_des; i++) {
	int k, n;
	for (k = 0; k < N_cdes; k++) {
	    if (strcmp (((DEAU_attr_t *)Cdes + k)->ats[DEAU_AT_ID], p) == 0)
		break;
	}
	if (k < N_cdes)	{	/* The DE already in cache */
	    DEAU_attr_t *dep = (DEAU_attr_t *)Cdes + k;
	    for (n = 0; n < DEAU_AT_N_ATS; n++) {
		if (dep->ats[n][0] != '\0')
		    free (dep->ats[n]);
	    }
	}
	for (n = 0; n < DEAU_AT_N_ATS; n++) {
	    int l = strlen (p);
	    if (l == 0)
		de.ats[n] = "";
	    else {
		de.ats[n] = (char *)MISC_malloc (l + 1);
		strcpy (de.ats[n], p);
	    }
	    p += l + 1;
	}
	if (k < N_cdes)
	    Cdes = STR_replace (Cdes, k * sizeof (DEAU_attr_t), 
		sizeof (DEAU_attr_t), (char *)&de, sizeof (DEAU_attr_t));
	else {
	    Cdes = STR_append (Cdes, (char *)&de, sizeof (DEAU_attr_t));
	    N_cdes++;
	}
    }

    return (0);
}

/**************************************************************************

    Updates a list of DEAs through a single RPC for reducing number of RPCs.
    See "man deau" for further details.

**************************************************************************/

int DEAU_write_listed_attrs (char *id_list, int *which_attr, char *attrs) {
    static char *ids = NULL;
    int ret, n_ids, i, rpcret;
    char *p;

    if (Lb_name == NULL && Get_lb_name () == NULL)
	return (DEAU_LB_UNDEFINED);

    ids = STR_copy (ids, id_list);
    n_ids = Process_id_list (&ids, 0);
    if (n_ids <= 0)
	return (n_ids);
    p = attrs;
    for (i = 0; i < n_ids; i++)
	p += strlen (p) + 1;
    Hide_null_in_string (attrs, (p - attrs) - 1);

    if (Db_host == NULL) {
	ret = DEAU_write_listed_remote (Lb_name, id_list, which_attr, attrs);
    }
    else {			/* RPC */
	char format[128];
	Rpc_name = STR_copy (Rpc_name, Db_host);
	Rpc_name = STR_cat (Rpc_name, ":");
	Rpc_name = STR_cat (Rpc_name, "DEAU_write_listed_remote");
	sprintf (format, "i-r s-i s-i ia-%d-i s-i", n_ids);
	rpcret = RSS_rpc (Rpc_name, format, &ret, 
				    Lb_name, id_list, which_attr, attrs);
	if (rpcret < 0)
	    return (rpcret);
    }

    return (ret);
}

/**************************************************************************

    Deletes all DEAs in the cache.

**************************************************************************/

void DEAU_delete_cache () {
    DEAU_attr_t *des;
    int i, n;

    des = (DEAU_attr_t *)Cdes;
    for (i = 0; i < N_cdes; i++) {
	for (n = 0; n < DEAU_AT_N_ATS; n++) {
	    if (*(des->ats[n]) != '\0')
		free (des->ats[n]);
	}
	des++;
    }
    N_cdes = 0;
}

/**************************************************************************

    The RPC version of DEAU_lock_de.

**************************************************************************/

int DEAU_remote_lock_de (char *id, int lock_cmd) {
    int ret, rpcret;

    if (Db_host == NULL)
	ret = DEAU_lock_de (id, lock_cmd);
    else {
	Rpc_name = STR_copy (Rpc_name, Db_host);
	Rpc_name = STR_cat (Rpc_name, ":");
	Rpc_name = STR_cat (Rpc_name, "DEAU_lock_de");
	rpcret = RSS_rpc (Rpc_name, "i-r s-i i-i", &ret, id, lock_cmd);
	if (rpcret < 0)
	    return (rpcret);
    }
    return (ret);
}

/**************************************************************************

    Searches in the DEA cache to find DEA of "id". If found the DEA is
    copied to "at_buf" and the pointer to the DEA is returned. Returns
    NULL is not found.

**************************************************************************/

static DEAU_attr_t *Get_cache_attr_by_id (char *id, DEAU_attr_t *at_buf) {
    DEAU_attr_t *des;
    int i;

    des = (DEAU_attr_t *)Cdes;
    for (i = 0; i < N_cdes; i++) {
	if (strcmp (des->ats[DEAU_AT_ID], id) == 0) {
	    memcpy (at_buf, des, sizeof (DEAU_attr_t));
	    return (at_buf);
	}
	des++;
    }
    return (NULL);
}

/**************************************************************************

    This is the batch reading function that runs on a remote host. It returns
    the number of DEs read on success or a negative error code. Refer to
    DEAU_read_listed_attrs. 

**************************************************************************/

int DEAU_read_listed_remote (char *lb_name, char *id_list, 
					int attr_mask, char **result) {
    static char *ids = NULL, *att_b = NULL;
    char *id;
    int n_ids, cnt, i;

    DEAU_LB_name (lb_name);
    *result = NULL;

    ids = STR_copy (ids, id_list);
    n_ids = Process_id_list (&ids, 1);
    if (n_ids <= 0)
	return (n_ids);

    att_b = STR_reset (att_b, 2000);
    cnt = 0;
    id = ids;
    for (i = 0; i < n_ids; i++) {
	DEAU_attr_t *at;
	char *buf;
	int ret, k, size;

	ret = DEAU_get_attr_by_id (id, &at);
	if (ret >= 0) {
	    for (k = 0; k < DEAU_AT_N_ATS; k++) {
		if (!(attr_mask & (1 << k)))
		    at->ats[k] = "";	/* attribute not requested */
	    }
	    size = DEAU_serialize_dea (at, &buf);
	    att_b = STR_append (att_b, buf, size);
	    cnt++;
	}
	id += strlen (id) + 1;
    }

    Hide_null_in_string (att_b, STR_size (att_b) - 1);
    *result = att_b;
    return (cnt);
}

/**************************************************************************

    This is the batch DEA updating function that runs on a remote host. 
    It returns the number of successful DEA updates on success or a 
    negative error code. Refer to DEAU_read_listed_attrs. 

**************************************************************************/

int DEAU_write_listed_remote (char *lb_name, char *id_list, 
					int *which_attr, char *attrs) {
    static char *ids = NULL;
    int n_ids, cnt, i;
    char *at, *id;

    DEAU_LB_name (lb_name);

    ids = STR_copy (ids, id_list);
    n_ids = Process_id_list (&ids, 0);
    if (n_ids <= 0)
	return (n_ids);
    Recover_null_in_string (attrs);

    cnt = 0;
    id = ids;
    at = attrs;
    for (i = 0; i < n_ids; i++) {
	int ret;
	ret = DEAU_update_attr (id, which_attr[i], at);
	if (ret >= 0)
	    cnt++;
	id += strlen (id) + 1;
	at += strlen (at) + 1;
    }
    return (cnt);
}

/**************************************************************************

    Expands the wild-card ids in the id list in "ids". Returns the number of 
    ids in the expanded list on success or a negative error code on failure.

**************************************************************************/

static int Process_id_list (char **ids, int need_expand) {
    int off, total, cnt;
    char *p;

    p = *ids;
    if (p[0] == ' ')
	return (DEAU_BAD_ARGUMENT);
    if (p[0] == '\0')
	return (0);
    total = 1;
    while (*p != '\0') {
	if (*p == ' ') {
	    *p = '\0';
	    if (p[1] == ' ')
		return (DEAU_BAD_ARGUMENT);
	    if (p[1] == '\0')
		break;
	    total++;
	}
	p++;
    }

    off = 0;
    cnt = 0;
    while (1) {
	int len;
	p = *ids + off;
	len = strlen (p);
	if (len == 0)
	    break;
	if (len >= 2 && p[len - 1] == '*' && p[len - 2] == '.') {
	    static char *buf = NULL;
	    int n_bns, ins_off, i, l;
	    char *bnames;

	    if (!need_expand)
		return (DEAU_BAD_ARGUMENT);
	    buf = STR_copy (buf, p);
	    buf[len - 2] = '\0';
	    n_bns = DEAU_get_branch_names (buf, &bnames);
	    if (n_bns < 0)
		return (n_bns);
	    buf[len - 2] = '.';
	    buf[len - 1] = '\0';
	    *ids = STR_replace (*ids, off, len + 1, NULL, 0);
	    total--;
	    ins_off = off;
	    for (i = 0; i < n_bns; i++) {
		buf = STR_cat (buf, bnames);
		l = strlen (buf);
		*ids = STR_replace (*ids, ins_off, 0, buf, l + 1);
		ins_off += l + 1;
		total++;
		buf[len - 1] = '\0';
		bnames += strlen (bnames) + 1;
	    }
	    p = *ids + off;
	    len = strlen (p);
	}
	off += len + 1;
	cnt++;
	if (cnt >= total)
	    break;
    }
    return (cnt);
}

/**************************************************************************

    Updates the value of DE of message "msg_id" in the DEA database on host 
    "remote_host" using the value of the local DEA database. "lb_name" is 
    the DEA database file's full path on "remote_host". If "lb_name" is NULL,
    the local DEA database file's full path is used. If "msg_id" is LB_ALL, 
    all messages that has "URC" or "AGENCY" permission are updated. Returns 
    the number of successful DEA updates on success or a negative error code.
    Identical value update does not cause database updated and is considered 
    successful.

**************************************************************************/

int DEAU_update_dea_db (char *remote_host, char *lb_name, LB_id_t msg_id) {
    static char *ids = NULL;
    static char *values = NULL;
    static char *which_attrs = NULL;
    int ret, n_ids, rpcret, value_attr;
    char format[128];

    if (Lb_name == NULL && Get_lb_name () == NULL)
	return (DEAU_LB_UNDEFINED);

    value_attr = DEAU_AT_VALUE;
    if (msg_id == LB_ALL) {
	DEAU_attr_t *at;
	char *id;

	ids = STR_reset (ids, 1000);
	values = STR_reset (values, 1000);
	which_attrs = STR_reset (which_attrs, 200);

	n_ids = 0;
	DEAU_get_next_dea (NULL, NULL);
	while (1) {
	    if ((ret = DEAU_get_next_dea (&id, &at)) == DEAU_DE_NOT_FOUND)
		break;
	    if (ret < 0)
		return (ret);
	    if (at->ats[DEAU_AT_ID][0] != '@' && 
		(DEAU_check_permission (at, "URC") > 0 ||
			    DEAU_check_permission (at, "AGENCY") > 0 ||
			    DEAU_check_permission (at, "ROC") > 0)) {
		ids = STR_append (ids, (char *)at->ats[DEAU_AT_ID],
					strlen (at->ats[DEAU_AT_ID]));
		ids = STR_append (ids, " ", 1);
		values = STR_append (values, (char *)at->ats[DEAU_AT_VALUE], 
					strlen (at->ats[DEAU_AT_VALUE]) + 1);
		which_attrs = STR_append (which_attrs, 
					(char *)&value_attr, sizeof (int));
		n_ids++;
	    }
	}
	ids = STR_append (ids, "", 1);
    }
    else {
	DEAU_attr_t at;
	n_ids = 1;
	ret = DEAU_get_dea_by_msgid (msg_id, &at);
	if (ret != (int)msg_id)
	    return (DEAU_DE_NOT_FOUND);
	ids = STR_copy (ids, (char *)at.ats[DEAU_AT_ID]);
	values = STR_copy (values, (char *)at.ats[DEAU_AT_VALUE]);
	which_attrs = STR_append (which_attrs, 
					(char *)&value_attr, sizeof (int));
    }

    Hide_null_in_string (values, STR_size (values) - 1);
    Rpc_name = STR_copy (Rpc_name, remote_host);
    Rpc_name = STR_cat (Rpc_name, ":");
    Rpc_name = STR_cat (Rpc_name, "DEAU_write_listed_remote");
    sprintf (format, "i-r s-i s-i ia-%d-i s-i", n_ids);
    if (lb_name == NULL)
	lb_name = Lb_name;
    rpcret = RSS_rpc (Rpc_name, format, &ret, 
				lb_name, ids, which_attrs, values);
    if (rpcret < 0)
	return (rpcret);
    return (ret);
}

/**************************************************************************

    Gets the DEA DB LB name and sets Db_host and Lb_name. It returns lb_name
    on success of NULL on failure.

**************************************************************************/

static char *Get_lb_name () {
    char *n, *p;

    n = DEAU_get_LB_name ();
    if (n[0] == '\0')
	return (NULL);
    p = n;
    while (*p != '\0') {
	if (*p == ':')
	    break;
	p++;
    }
    if (*p != ':' || p == n) {
	Lb_name = STR_copy (Lb_name, n);
	return (Lb_name);
    }
    Lb_name = STR_copy (Lb_name, p + 1);
    Db_host = STR_copy (Db_host, n);
    Db_host[p - n] = '\0';
    if (NET_get_ip_by_name (Db_host) == NET_get_ip_by_name (NULL))
	Db_host = NULL;		/* local host */
    return (Lb_name);
}

/**************************************************************************

    Converts all '\0' in "str" of "len" bytes to '\255'. 

**************************************************************************/

static void Hide_null_in_string (char *str, int len) {
    char *p, *end;
    p = str;
    end = p + len;
    while (p < end) {
	if (*p == '\0')
	    *p = '\255';
	p++;
    }
}

/**************************************************************************

    Converts all '\255' in null-terminated "str" to '\0'. 

**************************************************************************/

static void Recover_null_in_string (char *str) {
    char *p = str;
    while (*p != '\0') {
	if (*p == '\255')
	    *p = '\0';
	p++;
    }
}

/**************************************************************************

    Returns the names, separated by space, with "names", of all algorithm
    applications that has at least one editable parameter. This calls its 
    remote implementation to do the job if necessary. If all is non-zero,
    all application names are returned. Returns the number of applications 
    found or a negative error code. This function is specific to the 
    hci_apps_adapt. I put it here in DEAU because of the convenience for
    RPC implementation.

**************************************************************************/

int DEAU_get_editable_alg_names (int all, char **names) {
    int ret, rpcret;

    if (Lb_name == NULL && Get_lb_name () == NULL)
	return (DEAU_LB_UNDEFINED);

    if (Db_host == NULL) {
	ret = DEAU_get_editable_alg_names_remote (Lb_name, all, names);
    }
    else {			/* RPC */
	Rpc_name = STR_copy (Rpc_name, Db_host);
	Rpc_name = STR_cat (Rpc_name, ":");
	Rpc_name = STR_cat (Rpc_name, "DEAU_get_editable_alg_names_remote");
	rpcret = RSS_rpc (Rpc_name, "i-r s-i i-i s-o", 
				&ret, Lb_name, all, names);
	if (rpcret < 0)
	    return (rpcret);
    }
    return (ret);
}

/**************************************************************************

    The implementation of DEAU_get_editable_alg_names.

**************************************************************************/

int DEAU_get_editable_alg_names_remote (char *lb_name, int all, char **names) {
    static char *name_buf = NULL, *output = NULL;
    char *p, *app_names;
    int n_apps, cnt, i;

    DEAU_LB_name (lb_name);
    n_apps = DEAU_get_branch_names ("alg", &p);
    if (n_apps <= 0)
	return (n_apps);

    name_buf = STR_reset (name_buf, 256);
    output = STR_reset (output, 512);
    for (i = 0; i < n_apps; i++) {
	int len = strlen (p) + 1;
	name_buf = STR_append (name_buf, p, len);
	p += len;
    }
    output = STR_copy (output, "");
    cnt = 0;
    app_names = name_buf;		/* skip the reserved spaces */
    for (i = 0; i < n_apps; i++) {
	static char *buf = NULL;
	DEAU_attr_t *at;
	int n_ps, len, k, ret;
	char *p_names;

	buf = STR_copy (buf, "alg.");
	buf = STR_cat (buf, app_names);
	n_ps = DEAU_get_branch_names (buf, &p_names);
	len = strlen (buf);

	if (all)
	    k = 0;
	else {
	    for (k = 0; k < n_ps; k++) {
		buf[len] = '\0';
		buf = STR_cat (buf, ".");
		buf = STR_cat (buf, p_names);
		ret = DEAU_get_attr_by_id (buf, &at);
		if (ret == 0 &&
		    at->ats[DEAU_AT_PERMISSION][0] != '\0' &&
		    strstr (at->ats[DEAU_AT_MISC], 
					"@-Not_for_alg_edit-@") == NULL)
		    break;
		p_names += strlen (p_names) + 1;
	    }
	}
	if (k < n_ps) {
	    output = STR_cat (output, app_names);	/* application name */
	    output = STR_cat (output, " ");

	    buf = STR_copy (buf, "alg.");	/* the display name */
	    buf = STR_cat (buf, app_names);
	    buf = STR_cat (buf, ".alg_name");
	    ret = DEAU_get_attr_by_id (buf, &at);
	    if (ret == 0 && at->ats[DEAU_AT_VALUE][0] != '\0') {
		len = STR_size (output) - 1;
		output = STR_cat (output, at->ats[DEAU_AT_VALUE]);
		Replace_char (output + len, ' ', '_');
	    }
	    else
		output = STR_cat (output, app_names);
	    output = STR_cat (output, " ");

	    cnt++;
	}
	app_names += strlen (app_names) + 1;
    }
    *names = output;
    return (cnt);
}

/***************************************************************************

    Replace all chars "from" in string "str" to char "to".

***************************************************************************/

static void Replace_char (char *str, char from, char to) {
    char *p;

    p = str;
    while (*p != '\0') {
	if (*p == from)
	    *p = to;
	p++;
    }
}





