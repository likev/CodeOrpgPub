
/************************************************************************

    This is the main module fot the application adaptation HCI.

************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/10/09 14:27:09 $
 * $Id: hci_apps_adapt.c,v 1.34 2012/10/09 14:27:09 jing Exp $
 * $Revision: 1.34 $
 * $State: Exp $
 */

/*	Local include definition files */

#include <hci.h>
#include <hci_apps_adapt_def.h>

static int Operational = 0;	/* The RPG is operational */

char *Sb = NULL;		/* string buffer */
Apps_list_t *Apps;		/* application array */
int N_apps = 0;			/* size of application table */

static char *Des = NULL;	/* array for DEs */
static char *Apb = NULL;	/* buffer for application array */

static int De_cnt = 0;		/* current DE count */
static int Psize_sb;		/* number fo byte in Sb that are permanent */


static void Err_func (char *msg);
static int Read_de_values (char *id, int type, int baseline, void **values);
static char *Generate_desc_string (DEAU_attr_t *at);
static int Look_for_spec (char *desc, int baseline, int *where);
static char *Get_full_dea_file_path (char *id);
static void Post_process_des (Apps_list_t *app);
static char *Get_intern_node_name (char *id);
static int Search_string (char *text, char *str);
static int option_cb (char, char *);
static int Compare_apps_name (const void *a1p, const void *a2p);
static void print_usage ();
static void Updated_values_desc (Dea_t *de, int baseline, char **vbuf, 
						int voff, int rep_len);


/************************************************************************

    The main function for the RPG application adaptation data task.		
************************************************************************/

int main (int argc, char *argv[]) {

    if (ORPGMISC_is_operational ())
	Operational = 1;
    else
	Operational = 0;

    /*  Initialize custom options */
    HCI_set_custom_args ("I:T", option_cb, print_usage );
    /*  Initialize HCI */
    HCI_init( argc, argv, HCI_APPS_ADAPT_TASK );

    HAA_gui_main (Operational);

    return 0;
}

/**************************************************************************

    Callback from HCI_read_custom_options.

**************************************************************************/

static int option_cb (char opt, char *optarg) {

    if (opt == 'T')
	Operational = 0;
    else if (opt == 'I')
	HAA_set_init_app_name (optarg);
    else
        return (-1);
    return 0;
}

/**************************************************************************

    .

**************************************************************************/

static void print_usage () {

    printf ("-I app_name (Sets initial selection of the app name)\n");
}

/**************************************************************************

    Reads names of all applications. This function is called once initially.

**************************************************************************/

void HAA_read_app_names () {
    char *ac_ds_name, *p, *tp;
    int n_apps, i, all_apps;

    DEAU_set_error_func (Err_func);
    ac_ds_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
    if (ac_ds_name == NULL) {
	HCI_LE_error("ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA);
	HCI_task_exit (HCI_EXIT_FAIL);
    }
    DEAU_LB_name (ac_ds_name);
    HCI_LE_log("DEA DB: %s", ac_ds_name);

    Sb = STR_reset (Sb, 50000);					/* strings */
    Apb = STR_reset (Apb, 100 * sizeof (Apps_list_t));		/* apps */
    Des = STR_reset (Des, 1000 * sizeof (Dea_t));		/* DEs */

    all_apps = 1;
    if (Operational)
	all_apps = 0;
    n_apps = DEAU_get_editable_alg_names (all_apps, &p);
    if (n_apps == RMT_CANCELLED) {
	HCI_LE_log("HAA_read_app_names cancelled");
	HCI_task_exit (HCI_EXIT_SUCCESS);
    }
    if (n_apps < 0) {
	HCI_LE_error("DEAU_get_editable_alg_names (DB %s) failed (%d)", 
		ac_ds_name, n_apps);
	HCI_task_exit (HCI_EXIT_SUCCESS);
    }
    if (n_apps == 0) {
	HCI_LE_log("No application found in DEA DB (%s)", ac_ds_name);
	HCI_task_exit (HCI_EXIT_SUCCESS);
    }
    tp = p;
    while (*tp != '\0') {
	if (*tp == ' ')
	    *tp = '\0';
	tp++;
    }

    for (i = 0; i < n_apps; i++) {
	Apps_list_t app;

	app.app_name_o = STR_size (Sb);
	Sb = STR_append (Sb, p, strlen (p) + 1);
	p += strlen (p) + 1;
	app.display_name_o = STR_size (Sb);
	Sb = STR_append (Sb, p, strlen (p) + 1);
	p += strlen (p) + 1;
	app.n_des = -1;
	Apb = STR_append (Apb, (char *)&app, sizeof (Apps_list_t));
    }
    qsort (Apb,  n_apps,  sizeof (Apps_list_t), Compare_apps_name);

    Psize_sb = STR_size (Sb);
    De_cnt = 0;
    Apps = (Apps_list_t *)Apb;
    N_apps = n_apps;
}

/**************************************************************************

    Comparison function used for sorting the display names.

**************************************************************************/

static int Compare_apps_name (const void *a1p, const void *a2p) {
    Apps_list_t *a1, *a2;

    a1 = (Apps_list_t *)a1p;
    a2 = (Apps_list_t *)a2p;
    return (strcmp (Sb + a1->display_name_o, Sb + a2->display_name_o));
}

/**************************************************************************

    Deletes all DEA store in locally in "Des" and free the resource for
    reloading.

**************************************************************************/

void HAA_delete_des () {
    int i;

    Des = STR_reset (Des, 0);		/* free buffer */
    Sb = STR_replace (Sb, Psize_sb, STR_size (Sb) - Psize_sb, NULL, 0);
    De_cnt = 0;

    for (i = 0; i < N_apps; i++)
	Apps[i].n_des = -1;

    DEAU_delete_cache ();
}

/**************************************************************************

    Reads data elements for application of index "app_ind". Returns 0 on 
    success of a negative error code on failure.

**************************************************************************/

int HAA_read_data_elements (int app_ind) {
    static char *app_name = NULL, *id = NULL;
    static char *buf = NULL, *tmp_buf = NULL;
    Apps_list_t *app;
    int n_des, i, cnt, ret;
    char *param_list;

    if (app_ind < 0 || app_ind >= N_apps) {
	HCI_LE_error("Error: invalid application index");
	return (-1);
    }
    app = Apps + app_ind;
    if (app->n_des >= 0)
	return (0);

    app->n_des = 0;
    app_name = STR_copy (app_name, "alg.");
    app_name = STR_cat (app_name, Sb + app->app_name_o);
    ret = DEAU_read_listed_attrs (Get_intern_node_name (app_name));
    if (ret == RMT_CANCELLED) {
	HCI_LE_log("HAA_read_data_elements cancelled");
	HCI_task_exit (HCI_EXIT_SUCCESS);
    }
    n_des = DEAU_get_branch_names (app_name, &param_list);
    if (n_des <= 0) {
	HCI_LE_log("No data element (%d) found for (%s)", 
					n_des, Sb + app->app_name_o);
	return (-1);
    }
    buf = STR_reset (buf, 256);
    for (i = 0; i < n_des; i++) {	/* save the parameter names */
	buf = STR_append (buf, param_list, strlen (param_list) + 1);
	param_list += strlen (param_list) + 1;
    }
    param_list = buf;
    app->de_ind = De_cnt;

    /* This section is for increasing low bandwidth efficiency */
    tmp_buf = STR_reset (tmp_buf, 512);
    for (i = 0; i < n_des; i++) {
	id = STR_copy (id, app_name);
	id = STR_cat (id, ".");
	id = STR_cat (id, param_list);
	tmp_buf = STR_cat (tmp_buf, id);
	tmp_buf = STR_cat (tmp_buf, " ");
	param_list += strlen (param_list) + 1;
    }
    ret = DEAU_read_listed_attrs (tmp_buf);
    if (ret == RMT_CANCELLED) {
	HCI_LE_log("HAA_read_data_elements cancelled");
	HCI_task_exit (HCI_EXIT_SUCCESS);
    }

    param_list = buf;
    cnt = 0;
    for (i = 0; i < n_des; i++) {
	DEAU_attr_t *at;
	Dea_t de;
	char *desc, *p;
	int n_baseline, n_sels, ret, n;

	if (strcmp (param_list, "alg_name") == 0) {	/* reserved name */
	    param_list += strlen (param_list) + 1;
	    continue;
	}
	id = STR_copy (id, app_name);
	id = STR_cat (id, ".");
	id = STR_cat (id, param_list);
	ret = DEAU_get_attr_by_id (id, &at);
	if (ret < 0) {
	    HCI_LE_log("DE (%s) not found (%d)", id, ret);
	    return (-1);
	}

	if (Operational && (at->ats[DEAU_AT_PERMISSION][0] == '\0' ||
	    strstr (at->ats[DEAU_AT_MISC], "@-Not_for_alg_edit-@") != NULL)) {
	    param_list += strlen (param_list) + 1;
	    continue;
	}

	de.id_o = STR_size (Sb);
	Sb = STR_append (Sb, id, strlen (id) + 1);
	de.perm_o = STR_size (Sb);
	Sb = STR_append (Sb, at->ats[DEAU_AT_PERMISSION], 
				strlen (at->ats[DEAU_AT_PERMISSION]) + 1);
	de.name_o = STR_size (Sb);
	Sb = STR_append (Sb, at->ats[DEAU_AT_NAME], 
				strlen (at->ats[DEAU_AT_NAME]) + 1);
	de.description_o = STR_size (Sb);
	Sb = STR_append (Sb, at->ats[DEAU_AT_DESCRIPTION], 
				strlen (at->ats[DEAU_AT_DESCRIPTION]) + 1);
	de.de_num = -1;
	if ((p = strstr (at->ats[DEAU_AT_MISC], "SRC@-")) != NULL) {
	    char *t = MISC_malloc (strlen (p) + 1);
	    strcpy (t, p);
	    p = t;
	    while (*p != '\0' && *p != ' ')
		p++;
	    if (*p == ' ')
		*p = '\0';
	    if ((p = strstr (t, "::")) != NULL &&
		sscanf (p + 2, "%d", &n) == 1)
		de.de_num = n;
	    free (t);
	}
	de.desc_o = STR_size (Sb);
	desc = Generate_desc_string (at);
	Sb = STR_append (Sb, desc, strlen (desc) + 1);
	de.type = DEAU_get_data_type (at);

	de.n_sel_values = 0;
	n_sels = DEAU_get_allowable_values (at, &p);
	if (n_sels > 0) {
	    int k;
	    de.sel_values_o = STR_size (Sb);
	    for (k = 0; k < n_sels; k++) {
		Sb = STR_append (Sb, p, strlen (p) + 1);
		p += strlen (p) + 1;
	    }
	    de.n_sel_values = n_sels;
	}

	de.values = de.back_values = NULL;
	de.n_values = Read_de_values (id, de.type, 0, &(de.values));
	n_baseline = Read_de_values (id, de.type, 1, &(de.baseline));
	if (n_baseline == 0)
	    de.baseline = NULL;
	else if (n_baseline != de.n_values) {
	    free (de.baseline);
	    de.baseline = NULL;
	}
	if (de.baseline == NULL)
	    HCI_LE_log("Bad baseline (%s) - not used", id);

	p = at->ats[DEAU_AT_ACCURACY];
	if (strlen (p) > 0) {
	    if (*p != '[' ||
		sscanf (p + 1, "%lf", &de.accuracy) != 1 ||
		de.accuracy <= 0.0) {
		HCI_LE_log("Unexpected accuracy (%s) - not used", p);
		de.accuracy = 0.0;
	    }
	}
	else
	    de.accuracy = 0.0;

	de.value_upd = de.baseline_upd = 0;
	Des = STR_append (Des, (char *)&de, sizeof (Dea_t));
	De_cnt++;
	cnt++;

	param_list += strlen (param_list) + 1;
    }
    app->n_des = cnt;
    app->des = (Dea_t *)(Des + app->de_ind * sizeof (Dea_t));
    Apps = (Apps_list_t *)Apb;

    Post_process_des (app);
    return (0);
}

/**************************************************************************

    Post-processes DEs for application "app".

**************************************************************************/

static void Post_process_des (Apps_list_t *app) {
    Dea_t *de, *des;
    int max_name, max_desc, n, n_des, done, has_selection;

    des = app->des;
    n_des = app->n_des;

    do {	/* sort based on the DE number in the orginal DEA file */
	done = 1;
	for (n = 1; n < n_des; n++) {
	    if (des[n].de_num < des[n - 1].de_num) {
		Dea_t d;
		d = des[n];
		des[n] = des[n - 1];
		des[n - 1] = d;
		done = 0;
	    }
	}
    } while (!done);

    max_name = max_desc = has_selection = 0;
    de = des;
    for (n = 0; n < n_des; n++) {
	int len;

	len = strlen (Sb + de->name_o);
	if (len > max_name)
	    max_name = len;
	len = strlen (Sb + de->desc_o);
	if (len > max_desc)
	    max_desc = len;
	if (de->n_sel_values > 0)
	    has_selection = 1;
	de++;
    }
    app->name_field_width = max_name + 8;
    if (app->name_field_width < 20)
	app->name_field_width = 20;
    app->desc_field_width = max_desc + 8;
    if (app->desc_field_width < 40)
	app->desc_field_width = 40;
    app->has_selection = has_selection;
}

/**************************************************************************

    Reads values of DE "id" of "type" and returns the values with "values".
    If "baseline" is non-zero, the baseline values are read instead of the
    current values. The values are returned with a malloced buffer, which
    the caller needs to free after use. The function returns the number of
    values. If the base line values are less than that of the current values,
    we do not use the baseline values (returning 0). If it is too many, we
    use the first part.

**************************************************************************/

static int Read_de_values (char *id, int type, int baseline, void **values) {
    int n_vs;
    char *p, *t;
    int size, n;

    n_vs = DEAU_get_number_of_values (id);
    if (n_vs <= 0)
	return (0);

    if (type == DEAU_T_STRING) {
	if (baseline)
	    n = DEAU_get_baseline_string_values (id, &p);
	else
	    n = DEAU_get_string_values (id, &p);
	if (n < n_vs)
	    return (0);
    }
    else
	p = NULL;

    size = HAA_get_size_of_value_field (type, n_vs, p);
    t = MISC_malloc (size);
    if (type == DEAU_T_STRING)
	memcpy (t, p, size);
    else {
	if (baseline)
	    n = DEAU_get_baseline_values (id, (double *)t, n_vs);
	else
	    n = DEAU_get_values (id, (double *)t, n_vs);
	if (n < n_vs) {
	    free (t);
	    return (0);
	}
    }
    *values = (void *)t;
    return (n_vs);
}

/**************************************************************************

    Generate the string that will be displayed at the "range" column in
    the data element editing area. 

**************************************************************************/

static char *Generate_desc_string (DEAU_attr_t *at) {
    static char buf[256], *p;
    int ret;

    if (strlen (at->ats[DEAU_AT_RANGE]) + 
				strlen (at->ats[DEAU_AT_UNIT]) + 4 > 256)
	return ("");

    buf[0] = '\0';
    ret = DEAU_get_min_max_values (at, &p);
    if (ret > 0) {
	char *p1;
	p1 = p + strlen (p) + 1;
	switch (ret) {
	    case 1:
		sprintf (buf, " %s < x < %s", p, p1);
		break;
	    case 3:
		sprintf (buf, " %s <= x < %s", p, p1);
		break;
	    case 5:
		sprintf (buf, " %s < x <= %s", p, p1);
		break;
	    case 7:
		sprintf (buf, " %s <= x <= %s", p, p1);
		break;
	    default:
		break;
	}
    }
    else {
	ret = DEAU_get_allowable_values (at, &p);
	if (ret > 0) {
	    int i;
	    for (i = 0; i < ret; i++) {
		if (i == 0)
		    sprintf (buf + strlen (buf), " %s", p);
		else
		    sprintf (buf + strlen (buf), ", %s", p);
		p += strlen (p) + 1;
	    }
	}
    }
    if (strlen (at->ats[DEAU_AT_UNIT]) > 0) {
	if (buf[0] == '\0')
	    sprintf (buf, " %s", at->ats[DEAU_AT_UNIT]);
	else
	    sprintf (buf + strlen (buf), ", %s", at->ats[DEAU_AT_UNIT]);
    }
    return (buf);
}

/************************************************************************

    Returns, in text form, the "ind"-th value of "de". Returns an empty
    string if the value is not found.
	
************************************************************************/

char *HAA_get_text_from_value (Dea_t *de, int ind) {
    static char buf[128], *p;

    if (de->n_values > 0 && ind >= 0 && ind < de->n_values) {
	if (de->type == DEAU_T_STRING) {
	    int i;
	    p = (char *)de->values;
	    for (i = 0; i < ind; i++)
		p += strlen (p) + 1;
	    strncpy (buf, p, 128);
	    buf[127] = '\0';
	}
	else
	    HAA_form_num_string (((double *)de->values)[ind], de->type, buf);
    }
    else
	buf[0] = '\0';
    return (buf);
}

/**************************************************************************

    Directs DEAU messages to LE.

**************************************************************************/

static void Err_func (char *msg) {
    HCI_LE_log("%s", msg);
}

/************************************************************************

    Returns the number of bytes in the one of the value fields of Dea_t.

************************************************************************/

int HAA_get_size_of_value_field (int type, int n_values, void *values) {
    int size, i;

    if (type == DEAU_T_STRING) {
	char *p;
	p = (char *)values;
	size = 0;
	for (i = 0; i < n_values; i++) {
	    size += strlen (p) + 1;
	    p += strlen (p) + 1;
	}
    }
    else
	size = n_values * sizeof (double);
    return (size);
}

/************************************************************************

    Updates the values and baseline values for an application. "n_ids" 
    DEs of identifers "ids" of the application are updated with new values 
    of "values" and "baseline". All DEs must belong to the same DEA file 
    (i.e. the same application). "ids", "values" and "baseline" are char 
    pointer arrays. A NULL value of "values" or "baseline" element 
    indicates the item is not to be updated.

************************************************************************/

void HAA_update_apps_dea_file (int app_ind) {
    static char *vbuf = NULL;
    FILE *fl;
    char buf[4096], *fname, *deid, id[256];
    int n_des, ind, n_bytes;
    Dea_t *des;

    des = Apps[app_ind].des;
    n_des = Apps[app_ind].n_des;

    if (n_des <= 0)
	return;
    deid = Sb + des[0].id_o;
    fname = Get_full_dea_file_path (deid);
    if (fname == NULL) {
	HCI_LE_log("Cannot find DEA file name for %s", deid);
	return;
    }

    fl = fopen (fname, "r+");
    if (fl == NULL) {
	HCI_LE_log("fopen file (%s) to update failed", fname);
	return;
    }

    vbuf = STR_reset (vbuf, 256);
    ind = -1;
    id[0] = '\0';	/* current id */
    n_bytes = 0;
    while (fgets (buf + n_bytes, 4096 - n_bytes, fl) != NULL) {
	int len, rep_len, off, where, i, cnt, cont_line;
	char *p_read;

	p_read = buf + n_bytes;
	len = strlen (p_read);
	vbuf = STR_append (vbuf, p_read, len);

	cont_line = 0;
	if (len >= 1 && p_read[len - 1] == '\n') {
	    if (len >= 2 && p_read[len - 2] == '\\')
		cont_line = 1;
	    else {
		char *p = p_read + len - 2;
		while (p > p_read && (*p == ' ' || *p == '\t'))
		    p--;
		if (*p == ',')
		    cont_line = 1;
	    }
	}
	if (cont_line) {
	    n_bytes += len;
	    continue;
	}
	n_bytes = 0;

	off = 0;
	{
	    int t0, t1;
	    char tk0[128], tk1[128];
	    t0 = MISC_get_token (buf, "", 0, tk0, 128);
	    t1 = MISC_get_token (buf, "", 1, tk1, 128);
	    if (t0 <= 0 || tk0[0] == '#')
		continue;
	    if (t1 <= 0 ||
		(tk0[strlen (tk0) - 1] != '=' && tk1[0] != '=')) { /* new id */
		strncpy (id, tk0, 256);
		id[255] = '\0';
		ind = -1;
		for (i = 0; i < n_des; i++) { /* id is the last part of DE id */
		    Dea_t *de;
		    char *pp;
		    de = des + i;
		    if (!de->value_upd && !de->baseline_upd)
			continue;
		    pp = Sb + de->id_o;
		    cnt = 0;
		    while (*pp != '\0' && cnt < 2) {
			if (*pp == '.')
			    cnt++;
			pp++;
		    }
		    if (cnt < 2)
			continue;
		    if (strcmp (pp, id) == 0) {
			ind = i;
			off = strstr (buf, id) + strlen (id) - buf;
			break;
		    }
		}
	    }
	}
	if (ind < 0)
	    continue;
	for (i = 0; i < 2; i++) {	/* 0 - value; 1 - baseline */
	    rep_len = Look_for_spec (buf + off, i, &where);
	    if (rep_len >= 0) {
		Updated_values_desc (des + ind, i, 
			&vbuf, STR_size (vbuf) - strlen (buf) + off + where,
			rep_len);
	    }
	}
    }

    fseek (fl , 0, SEEK_SET);
    if (fwrite (vbuf, 1, STR_size (vbuf), fl) != STR_size (vbuf)) {
	HCI_LE_error("fwrite %s failed", fname);
	fclose (fl);
	return;
    }
    ftruncate (fileno (fl), STR_size (vbuf));
    fclose (fl);
}

/************************************************************************

    Looks for the specification segment in "text" of the value, or baseline
    if "baseline" is non-zero, attribute. Returns the length of the
    segment and the offset in "text" with "where". Returns -1 if not found.

************************************************************************/

static int Look_for_spec (char *text, int baseline, int *where) {
    char *p, *attr_name;
    int off, an_len;

    if (baseline)
	attr_name = "baseline";
    else
	attr_name = "value";
    an_len = strlen (attr_name);
    p = text;
    while (1) {
	char c1, c2, *pe, *t, last_non_space;

	off = Search_string (p, attr_name);
	if (off < 0)
	    return (-1);
	p += off;

	t = p - 1;
	while (t >= text && (*t == ' ' || *t == '\t'))
	    t--;
	if (t < text)
	    c1 = '\0';		/* non-spacing char before attr_name */
	else
	    c1 = *t;
	t = p + an_len;
	while (*t == ' ' || *t == '\t')
	    t++;
	c2 = *t;		/* non-spacing char after attr_name */

	if ((c1 != '\0' && c1 != ';') || c2 != '=') {
	    p += an_len;		/* not qualify for a attr name */
	    continue;
	}
	p += an_len;
	while (*p != '=')
	    p++;
	p++;
	while (*p == ' ' || *p == '\t')
	    p++;
	pe = p;
	last_non_space = *pe;
	while (*pe != ';' && *pe != '\0') {
	    if (*pe == '\n' && last_non_space != ',' && last_non_space != '\\')
		break;
	    if (*pe != ' ' && *pe != '\t')
		last_non_space = *pe;
	    pe++;
	}
/*	if ((*pe == '\n' || *pe == '\0') && pe[-1] == '\\')
	    return (-1);	 we do not do multi-line attribute spec */
	*where = p - text;
	return (pe - p);
    }
}

/************************************************************************

    Searches case-insensitively string "str" in string "text". "str"
    is assumed to be all lower case. Returns offset in text or
    -1 if not found.

************************************************************************/

static int Search_string (char *text, char *str) {
    static char *buf = NULL;
    char *p;

    buf = STR_copy (buf, text);
    p = buf;
    while (*p != '\0') {
	if (*p <= 'Z' && *p >= 'A')
	    *p += 32;
	p++;
    }
    p = strstr (buf, str);
    if (p == NULL)
	return (-1);
    return (p - buf);
}

/************************************************************************

    Returns the full path of dea file corresponding to DE of identifier 
    "id".

************************************************************************/

static char *Get_full_dea_file_path (char *id) {
    static char buf[MAX_NAME_SIZE * 2] = "";
    static int dir_len = 0;
    int len;
    char *p, *e;

    if (dir_len == 0) {
	buf[MAX_NAME_SIZE * 2 - 1] = '\0';
	if (MISC_get_cfg_dir (buf, MAX_NAME_SIZE - 1) < 0)
	    buf[0] = '\0';
	else
	    strcat (buf, "/dea/");
	dir_len =  strlen (buf);
    }
    p = id;
    if (strncmp (p, "alg.", 4) != 0)
	return (NULL);
    p += 4;
    e = p;
    while (*e != '\0' && *e != '.')
	e++;
    if (*e != '.')
	return (NULL);
    len = dir_len;
    memcpy (buf + len, p, e - p);
    len += e - p;
    strcpy (buf + len, ".alg");
    return (buf);
}

/************************************************************************

    Updates the value attributes for DE "de" in DEA file "vbuf". If
    "baseline" is not zero, the baseline value is updated instead. voff
    is the location of the existing attributes and rep_len is the length
    of the existing attributes text.

************************************************************************/

static void Updated_values_desc (Dea_t *de, int baseline, char **vbuf, 
						int voff, int rep_len) {
    int upd;

    upd = de->value_upd;
    if (baseline)
	upd = de->baseline_upd;
    if (upd) {
	int k;
	for (k = de->n_values - 1; k >= 0; k--) {
	    char *p, *tp, *op, *end;
	    int ccnt;

	    tp = NULL;
	    if (baseline) {
		tp = de->values;
		de->values = de->baseline;
	    }
	    p = HAA_get_text_from_value (de, k);

	    op = *vbuf + voff;
	    end = op + rep_len;
	    ccnt = 0;		/* comma count */
	    while (op < end) {
		if (*op == ',')
		    ccnt++;
		else if (ccnt == k) {
		    int len;
		    op += MISC_char_cnt (op, " \t\\\n");
		    len = MISC_char_cnt (op, "\0,; \t\\\n");
		    if (len < 128) {
			char b[128];
			double v1, v2;
			memcpy (b, op, len);
			b[len] = '\0';
			if (de->type == DEAU_T_STRING && strcmp (b, p) == 0)
			    break;
			if (!(de->type == DEAU_T_STRING) &&
				sscanf (b, "%lf", &v1) == 1 &&
				sscanf (p, "%lf", &v2) == 1 && v1 == v2)
			    break;
		    }
		    *vbuf = STR_replace (*vbuf, op - *vbuf, len, p, strlen (p));
		    break;
		}
		op++;
	    }
	    if (baseline)
		de->values = tp;
	}
    }
}

/************************************************************************

    Returns the internal node DE ID for "app_name".

************************************************************************/

static char *Get_intern_node_name (char *id) {
    static char *buf = NULL;

    buf = STR_copy (buf, id);
    buf = STR_replace (buf, 0, 0, "@@", 2);
    return (buf);
}

/**************************************************************************

    Sets values or baseline values (if "is_base_line" is non-zero) for DE
    "id" with "n_items" values in "values" of type "is_str_type". The 
    attrubute update is delayed until this function is called with "id"
    = NULL in which case the update is performed in batch mode. Returns
    the number of DEAs updated or a negative error code. This function is
    a replacement for DEAU_set_values for supporting low-bandwidth/long-delay 
    connnection to the DEA DB.

**************************************************************************/

int HAA_set_values (char *id, int is_str_type, void *values, 
					int n_items, int is_base_line) {
    static int cnt = 0;
    static char *id_list = NULL, *which_attr = NULL, *attrs = NULL;
    char *p;
    int att, ret;

    if (id == NULL) {
	if (cnt > 0) {
	    ret = DEAU_write_listed_attrs (id_list, (int *)which_attr, attrs);
	    if (ret < 0)
		HCI_LE_log("DEAU_write_listed_attrs failed (%d)", ret);
	    else if (ret != cnt)
		HCI_LE_log("DEAU_write_listed_attrs - %d DEAs not updated", 
			cnt - ret);
	    cnt = 0;
	    return (ret);
	}
	return (0);
    }

    if (cnt == 0) {
	id_list = STR_reset (id_list, 1024);
	which_attr = STR_reset (which_attr, 128);
	attrs = STR_reset (attrs, 1024);
    }

    p = DEAU_values_to_text (is_str_type, values, n_items);
    id_list = STR_cat (id_list, id);
    id_list = STR_cat (id_list, " ");
    if (is_base_line)
	att = DEAU_AT_BASELINE;
    else
	att = DEAU_AT_VALUE;
    which_attr = STR_append (which_attr, (char *)&att, sizeof (int));
    attrs = STR_append (attrs, p, strlen (p) + 1);
    cnt++;
    return (0);
}

/**************************************************************************

    Converts numerical value "v" of "type" to string form for display 
    purpose. A real number must have a decimal pointer. "buf", which is
    assumed to be sufficiently large, returns the string.

**************************************************************************/

void HAA_form_num_string (double v, int type, char *buf) {
    char *p;

    sprintf (buf, "%.12g", v);
    p = buf;
    while (*p != '\0') {
	if (*p == '.')
	    break;
	p++;
    }
    if ((*p == '\0' || p[1] == '\0') && 
		(type == DEAU_T_DOUBLE || type == DEAU_T_FLOAT))
	strcat (buf, ".0");
    return;
}


 


