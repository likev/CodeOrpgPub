
/*************************************************************************

    Initializes the RPG product distribution adaptation/configuration data.

**************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/10 14:02:46 $
 * $Id: mnttsk_prod_dist.c,v 1.24 2014/04/10 14:02:46 steves Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <orpg.h> 
#include <orpgsite.h>
#include <infr.h> 

#include <mnttsk_pd_def.h>

/* Constant Definitions/Macro Definitions/Type Definitions */

/* Static Global Variables */
static int N_links = 0;
static Comms_link_t *Links = NULL;

/* Static Function Prototypes */
static int Get_command_line_options (int argc, char *argv[], 
						char **startup_type);
static void Cs_error_func (char *err_msg);
static int Read_comms_link_conf ();
static void Set_de_value (char *id, int is_str_type, 
					void *values, int n_items);
static int Init_comms_dbs ();
static void Clear_comms_dbs ();
static int Create_comms_data ();
static int Get_de_value (char *de_id, int is_str_type, void *values);
static int Read_dea_comms_data ();
static int Generate_line_struct ();
static int Generate_comms_link_conf ();


/**************************************************************************

    The main function.

**************************************************************************/

int main (int argc, char *argv[]) {

    char *startup_type, *db_name;
    int ret;

    /* Initialize log-error services */
    ORPGMISC_init (argc, argv, 200, 0, -1, 0);
    CS_error (Cs_error_func);

    if ((ret = Get_command_line_options (argc, argv, &startup_type)) < 0)
	exit (1);

    db_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
    if (db_name == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA);
	exit (1);
    }
    DEAU_LB_name (db_name);

    ORPGDA_open (ORPGDAT_PROD_INFO, LB_WRITE);

    if (strcmp (startup_type, "clear") == 0)
	Clear_comms_dbs ();
    else if (strcmp (startup_type, "startup") == 0 ||
	     strcmp (startup_type, "restart") == 0 ||
	     strcmp (startup_type, "init") == 0) {
	if (!ORPGMISC_is_operational () || 
				strcmp (startup_type, "init") == 0) {
	    if (Init_comms_dbs () < 0)
		exit (1);
	}
	if (Create_comms_data () < 0)
	    exit (1);
    }
    else {
	LE_send_msg (GL_ERROR, "Unexpected -t option: %s", startup_type);
	exit (1);
    }
    exit (0);
}

/**************************************************************************

    Creates binary a/c data to support runtime RPG and comms_link.conf 
    to support comms managers. Returns 0 on success or a negative error
    code.

**************************************************************************/

static int Create_comms_data () {
    char buf[4];

    if (Read_dea_comms_data () < 0 ||
	Generate_line_struct () < 0 ||
	Generate_comms_link_conf () < 0 ||
	IUD_init_user_db (Links, N_links) < 0)
	return (-1);

    ORPGDA_write (ORPGDAT_PROD_INFO, buf, 0, PD_USER_INFO_MSG_ID);

    LE_send_msg (GL_INFO, "Product distri adapt/config data init completed\n");
    return (0);
}

/**************************************************************************

    Generates the PD_LINE_INFO_MSG_ID message in data store ORPGDAT_PROD_INFO.

**************************************************************************/

static int Generate_line_struct () {
    Pd_distri_info *pd;
    Pd_line_entry *l_tbl;
    char *cpt;
    int ret, i;

    LE_send_msg (GL_INFO, "Initializing PD_LINE_INFO_MSG_ID message\n");

    pd = (Pd_distri_info *)malloc (ALIGNED_SIZE (sizeof (Pd_distri_info)) +
			N_links * sizeof (Pd_line_entry));
    cpt = (char *)pd;
    cpt += ALIGNED_SIZE (sizeof (Pd_distri_info));
    pd->line_list = cpt - (char *)pd;
    pd->nb_retries = NB_RETRIES;
    pd->nb_timeout = NB_TIMEOUT;
    pd->connect_time_limit = NB_CONNECT_TIME_LIMIT;

    l_tbl = (Pd_line_entry *)cpt;
    for (i = 0; i < N_links; i++) {
	l_tbl[i].line_ind = Links[i].line_ind;
	l_tbl[i].cm_ind = Links[i].cm_ind;
	l_tbl[i].p_server_ind = Links[i].user_ind;
	l_tbl[i].link_state = Links[i].link_state;
	l_tbl[i].baud_rate = Links[i].line_rate;
	l_tbl[i].packet_size = Links[i].packet_size;
	l_tbl[i].n_pvcs = Links[i].n_pvcs;
	l_tbl[i].conn_time_limit = Links[i].time_out;

	if (strcmp (Links[i].line_type, "Dedic") == 0)
	    l_tbl[i].line_type = DEDICATED;
	else if (strcmp (Links[i].line_type, "D-in") == 0)
	    l_tbl[i].line_type = DIAL_IN;
	else if (strcmp (Links[i].line_type, "D-out") == 0)
	    l_tbl[i].line_type = DIAL_OUT;
	else if (strcmp (Links[i].line_type, "WAN") == 0)
	    l_tbl[i].line_type = WAN_LINE;
	else {
	    LE_send_msg (GL_ERROR, 
			"Unexpected line type (%s)", Links[i].line_type);
	    return (-1);
	}
	if (strcmp (Links[i].cm_name, "cm_tcp") == 0)
	    l_tbl[i].protocol = PROTO_TCP;
	else
	    l_tbl[i].protocol = PROTO_X25;
	strncpy (l_tbl[i].port_password, Links[i].access_word, PASSWORD_LEN);
/*
printf (" line_ind %d cm_ind %d p_server_ind %d link_state %d baud_rate %d packet_size %d n_pvcs %d conn_time_limit %d port_password %s protocol %d\n", l_tbl[i].line_ind, l_tbl[i].cm_ind, l_tbl[i].p_server_ind, l_tbl[i].link_state, l_tbl[i].baud_rate, l_tbl[i].packet_size, l_tbl[i].n_pvcs, l_tbl[i].conn_time_limit, l_tbl[i].port_password, l_tbl[i].protocol);
*/
    }

    pd->n_lines = N_links;
    cpt += N_links * sizeof (Pd_line_entry);

    CS_cfg_name ("");
    ret = ORPGDA_write (ORPGDAT_PROD_INFO, (char *)pd, cpt - (char *)pd, 
						PD_LINE_INFO_MSG_ID);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGDA_write ORPGDAT_PROD_INFO failed (%d)\n", ret);
	return (-1);
    }
    return (0);
}

/**************************************************************************

    Read comms a/c from the Comms DEA. Returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_dea_comms_data () {
    double *values, d;
    char *p;
    int ret, i;

    if (Links != NULL) {
	free (Links);
	Links = NULL;
    }
    if ((ret = DEAU_get_values ("comms.n_link", &d, 1)) <= 0) {
	LE_send_msg (GL_ERROR, 
		"DEAU_get_values comms.n_link failed (%d)\n", ret);
	return (-1);
    }
    N_links = (int)d;
    LE_send_msg (GL_INFO, "Number of comms links is %d\n", N_links);
    if (d <= 0)
	return (0);

    Links = (Comms_link_t *)malloc (N_links * sizeof (Comms_link_t));
    values = (double *)malloc (N_links * sizeof (double));
    if (Links == NULL || values == NULL) {
	LE_send_msg (GL_INFO, "malloc failed\n");
	return (-1);
    }
    memset (Links, 0, N_links * sizeof (Comms_link_t));

    for (i = 0; i < N_links; i++)
	Links[i].line_ind = i;
    if (Get_de_value ("comms.user_index", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].user_ind = (int)values[i];
    if (Get_de_value ("comms.cm_index", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].cm_ind = (int)values[i];
    if (Get_de_value ("comms.device_number", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].dev_n = (int)values[i];
    if (Get_de_value ("comms.port_number", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].port_n = (int)values[i];
    if (Get_de_value ("comms.line_rate", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].line_rate = (int)values[i];
    if (Get_de_value ("comms.packet_size", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].packet_size = (int)values[i];
    if (Get_de_value ("comms.n_pvcs", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].n_pvcs = (int)values[i];
    if (Get_de_value ("comms.link_state", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].link_state = (int)values[i];
    if (Get_de_value ("comms.en_enabled", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].en = (int)values[i];
    if (Get_de_value ("comms.user_class", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].user_class = (int)values[i];
    if (Get_de_value ("comms.line_time_out", 0, values) < 0)
	return (-1);
    for (i = 0; i < N_links; i++)
	Links[i].time_out = (int)values[i];

    if (Get_de_value ("comms.line_type", 1, &p) < 0)
	return (-1);
    for (i = 0; i < N_links; i++) {
	strncpy (Links[i].line_type, p, CFG_SHORT_STR_SIZE - 1);
	p += strlen (p) + 1;
    }
    if (Get_de_value ("comms.cm_name", 1, &p) < 0)
	return (-1);
    for (i = 0; i < N_links; i++) {
	strncpy (Links[i].cm_name, p, CFG_SHORT_STR_SIZE - 1);
	p += strlen (p) + 1;
    }
    if (Get_de_value ("comms.access_word", 1, &p) < 0)
	return (-1);
    for (i = 0; i < N_links; i++) {
	strncpy (Links[i].access_word, p, CFG_SHORT_STR_SIZE - 1);
	p += strlen (p) + 1;
    }

    return (0);
}

/**************************************************************************

    Reads N_links values of DE "de_id" and puts them in "values". Returns 0
    on success or -1 on failure.

**************************************************************************/

static int Get_de_value (char *de_id, int is_str_type, void *values) {
    int ret;

    if (is_str_type)
	ret = DEAU_get_string_values (de_id, (char **)values);
    else
	ret = DEAU_get_values (de_id, (double *)values, N_links);
    if (ret != N_links) {
	LE_send_msg (GL_ERROR, 
		"DEAU_get_values %s failed (%d != %d)\n", de_id, ret, N_links);
	return (-1);
    }
    return (0);
}

/**************************************************************************

    Deletes Comms DEA and user DB by reseting the value of DE 
    "comms.init_state". This will force them to be re-initialized. This 
    function does not return.

**************************************************************************/

static void Clear_comms_dbs () {
    int ret;

    ret = DEAU_set_values ("comms.init_state", 1, "not completed", 1, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"DEAU_set_values comms.init_state failed (%d)\n", ret);
	exit (1);
    }
    LE_send_msg (GL_INFO, "Comms DEA and user DB are deleted");
    exit (0);
}

/**************************************************************************

    Initializes the comms info in the DE DB and the product user DB. Returns
    0 on success or a negative error code.

**************************************************************************/

static int Init_comms_dbs () {
    int ret;
    char *p;

    /* check if the initialization has already completed */
    ret = DEAU_get_string_values ("comms.init_state", &p);
    if (ret > 0 && strcmp (p, "init completed") == 0) {
	LE_send_msg (GL_INFO, "Comms info is complete - not initialized\n");
	return (0);
    }
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "DEAU_get_string_values comms.init_state failed - DEA DB not initialized\n");
	return (-1);
    }

    if ((ret = Read_comms_link_conf ()) < 0 ||
	(ret = IUD_init_user_db (Links, N_links)) < 0) {
	return (ret);
    }

    ret = DEAU_set_values ("comms.init_state", 1, "init completed", 1, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"DEAU_set_values comms.init_state failed (%d)\n", ret);
	return (-1);
    }
    LE_send_msg (GL_INFO, "Init comms DE values and user DB completed");
    return (0);
}

/**************************************************************************

    Reads the comms configuration file "comms_link.conf" and sets the
    comms DE values. Returns 0 on success or a negative error code.

**************************************************************************/

static int Read_comms_link_conf () {
    char tmp[CFG_NAME_SIZE], *vs, *buf;
    int cnt, ind, i, not_faa;
    Comms_link_t l;
    double value, *d;

    not_faa = 0;
    if( (DEAU_get_values( ORPGSITE_DEA_REDUNDANT_TYPE, &value, 1 )) > 0 ){
       if( (int) value != ORPGSITE_FAA_REDUNDANT )
          not_faa = 1;
    }

    if (not_faa) 
       LE_send_msg (GL_INFO, "Site is Not an FAA Redundant Site\n" );
    else
       LE_send_msg (GL_INFO, "Site is an FAA Redundant Site\n" );
       
    CS_cfg_name ("comms_link.conf");
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);

    CS_control (CS_KEY_OPTIONAL);
    if (CS_entry ("Run_time_created", 0, CFG_NAME_SIZE, tmp) > 0) {
	LE_send_msg (GL_ERROR, "comms_link.conf created by RPG - not read\n");
	return (-1);
    }
    CS_control (CS_KEY_REQUIRED);
    LE_send_msg (GL_INFO, "Reading %s...\n", CS_cfg_name (NULL));

    if (CS_entry ("number_links", 1 | CS_INT, 0, (void *)&N_links) < 0)
	return (-1);
    if (N_links == 0) {
	LE_send_msg (GL_INFO, "No comms link defined in %s\n", 
						CS_cfg_name (NULL));
	return (-1);
    }
    Links = (Comms_link_t *)malloc (N_links * sizeof (Comms_link_t));
    if (Links == NULL) {
	LE_send_msg (GL_INFO, "malloc failed\n");
	return (-1);
    }

    cnt = 0;
    while (cnt < N_links &&
	   CS_entry ((char *)cnt, CS_INT_KEY | CS_INT, 0,
				(void *)&(l.line_ind)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 1 | CS_INT, 0,
				(void *)&(l.user_ind)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 2 | CS_INT, 0,
				(void *)&(l.cm_ind)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 3 | CS_INT, 0,
				(void *)&(l.dev_n)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 4 | CS_INT, 0,
				(void *)&(l.port_n)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 5, CFG_SHORT_STR_SIZE,
				(void *)l.line_type) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 6 | CS_INT, 0,
				(void *)&(l.line_rate)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 7, CFG_SHORT_STR_SIZE,
				(void *)l.cm_name) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 8 | CS_INT, 0,
				(void *)&(l.packet_size)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 9 | CS_INT, 0,
				(void *)&(l.n_pvcs)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 10 | CS_INT, 0,
				(void *)&(l.link_state)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 11 | CS_INT, 0,
				(void *)&(l.en)) > 0 &&
	   CS_entry ((char *)cnt, CS_INT_KEY | 12 | CS_INT, 0,
				(void *)&(l.user_class)) > 0) {
	if (strcmp (l.line_type, "Dedic") != 0 &&
	    strcmp (l.line_type, "D-out") != 0 &&
	    strcmp (l.line_type, "WAN") != 0 &&
	    strcmp (l.line_type, "D-in") != 0) {
	    CS_report ("unexpected line type");
	    return (-1);
	}
	CS_control (CS_KEY_OPTIONAL);
	if (CS_entry ((char *)cnt, CS_INT_KEY | 13 | CS_INT, 0,
				(void *)&(l.time_out)) <= 0)
	    l.time_out = 0;
	if (CS_entry ((char *)cnt, CS_INT_KEY | 14, CFG_SHORT_STR_SIZE,
				(void *)l.access_word) <= 0)
	    strcpy (l.access_word, "NS__");
	CS_control (CS_KEY_REQUIRED);
	memcpy (Links + cnt, &l, sizeof (Comms_link_t));
	cnt++;
    }
    if (cnt != N_links) {
	CS_report ("Incorrect number of links specified");
	return (-1);
    }

    vs = NULL;
    cnt = 0;
    CS_control (CS_RESET);
    CS_control (CS_NO_ENV_EXP);
    while (CS_entry (CS_NEXT_LINE, 0, CFG_NAME_SIZE, tmp) > 0) {
	if (strcmp (tmp, "number_links") == 0)
	    continue;
	if (sscanf (tmp, "%d", &ind) == 1 && ind >= 0 && ind < N_links)
	    continue;
	CS_entry (CS_THIS_LINE, CS_FULL_LINE, CFG_NAME_SIZE, tmp);
	vs = STR_append (vs, tmp, strlen (tmp) + 1);
	cnt++;
    }
    CS_control (CS_YES_ENV_EXP);

    /* set comms DE values */
    buf = STR_reset (NULL, N_links * sizeof (double));
    d = (double *)buf;
    d[0] = (double)N_links;
    Set_de_value ("comms.n_link", 0, d, 1);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].user_ind;
    Set_de_value ("comms.user_index", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].cm_ind;
    Set_de_value ("comms.cm_index", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].dev_n;
    Set_de_value ("comms.device_number", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].port_n;
    Set_de_value ("comms.port_number", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].line_rate;
    Set_de_value ("comms.line_rate", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].packet_size;
    Set_de_value ("comms.packet_size", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].n_pvcs;
    Set_de_value ("comms.n_pvcs", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].link_state;
    Set_de_value ("comms.link_state", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].en;
    Set_de_value ("comms.en_enabled", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].user_class;
    Set_de_value ("comms.user_class", 0, d, N_links);
    for (i = 0; i < N_links; i++)
	d[i] = (double)Links[i].time_out;
    Set_de_value ("comms.line_time_out", 0, d, N_links);
    buf = STR_reset (buf, 0);
    for (i = 0; i < N_links; i++)
	buf = STR_append (buf, Links[i].line_type, 
				strlen (Links[i].line_type) + 1);
    Set_de_value ("comms.line_type", 1, buf, N_links);
    buf = STR_reset (buf, 0);
    for (i = 0; i < N_links; i++){

        if( (not_faa) && strcmp( Links[i].cm_name, "cm_uconx" ) == 0 ){
	   buf = STR_append (buf, "cm_uconx_", 
				strlen ("cm_uconx_") + 1);
           LE_send_msg (GL_INFO, 
                        "Changing Link index %d cm_uconx to cm_uconx_\n" );
        }
        else
	   buf = STR_append (buf, Links[i].cm_name, 
	  			strlen (Links[i].cm_name) + 1);
    }
    Set_de_value ("comms.cm_name", 1, buf, N_links);
    buf = STR_reset (buf, 0);
    for (i = 0; i < N_links; i++)
	buf = STR_append (buf, Links[i].access_word, 
				strlen (Links[i].access_word) + 1);
    Set_de_value ("comms.access_word", 1, buf, N_links);
    Set_de_value ("comms.misc_spec", 1, vs, cnt);

    STR_free (vs);
    STR_free (buf);
    LE_send_msg (GL_INFO, "Setting comms DE values completed\n");

/*
for (cnt = 0; cnt < N_links; cnt++)
printf ("%d %d %d %d %d   %s %d %s   %d %d %d %d %d %d %s\n", Links[cnt].line_ind, Links[cnt].user_ind, Links[cnt].cm_ind, Links[cnt].dev_n, Links[cnt].port_n, Links[cnt].line_type, Links[cnt].line_rate, Links[cnt].cm_name, Links[cnt].packet_size, Links[cnt].n_pvcs, Links[cnt].link_state, Links[cnt].en, Links[cnt].user_class, Links[cnt].time_out, Links[cnt].access_word);
*/
    return (0);
}

/**************************************************************************

    Sets the value DE of "id" to "n_items" "values".

**************************************************************************/

static void Set_de_value (char *id, int is_str_type, 
					void *values, int n_items) {
    int ret;
    ret = DEAU_set_values (id, is_str_type, values, n_items, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "DEAU_set_values %s failed (%d)", id, ret);
	exit (1);
    }
}

/**************************************************************************

    Generates the comm_link.conf file for supporting the comms managers.

**************************************************************************/

static int Generate_comms_link_conf () {
    FILE *fl;
    char fname[CFG_NAME_SIZE + 80], tmp[CFG_NAME_SIZE], *p;
    int ret, i, n_misc_spec_lines;

    if (!ORPGMISC_is_operational ())
	return (0);

    CS_cfg_name ("comms_link.conf");

    fl = fopen (CS_cfg_name (NULL), "r");
    if (fl != NULL) {
	CS_control (CS_KEY_OPTIONAL);
	CS_control (CS_COMMENT | '#');
	CS_control (CS_RESET);
	if (CS_entry ("Run_time_created", 0, CFG_NAME_SIZE, tmp) == 
						    CS_KEY_NOT_FOUND) {
	    LE_send_msg (GL_INFO, "Comms_link.conf exists - not created\n");
	    fclose (fl);
	    return (0);
	}
	fclose (fl);
	CS_control (CS_KEY_REQUIRED);
    }

    LE_send_msg (GL_INFO, "Creating comms_link.conf...\n");
    ret = MISC_get_cfg_dir (fname, CFG_NAME_SIZE);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, "MISC_get_cfg_dir failed (%d)", ret);
	return (-1);
    }
    strcat (fname, "/comms_link.conf");
    fl = fopen (fname, "w");
    if (fl == NULL) {
 	LE_send_msg (GL_ERROR, "fopen %s failed\n", fname);
	return (-1);
    }

    fprintf (fl, "\n# This is the link configuration file for ORPG product distribution\n\n");
    fprintf (fl, "# LN: link number (0, 1, ...);\n");
    fprintf (fl, "# UN: comm_user number (0, 1, ...);\n");
    fprintf (fl, "# CN: comm_manager number (0, 1, ...);\n");
    fprintf (fl, "# DN: physical device number (0, 1, ...);\n");
    fprintf (fl, "# PN: physical port number (0, 1, ...);\n");
    fprintf (fl, "# LT: link type (Dedic (dedicated), D-in (dial in), D-out (dial in/out));\n");
    fprintf (fl, "# LR: line rate (e.g. 56000); 0 means using external clock;\n");
    fprintf (fl, "# CS: comm_manager name (cm_simpact, cm_uconx, cm_sisco, cm_tcp, cm_atlas)\n");
    fprintf (fl, "# MPS: maximum packet size (number of bytes, >= 32) (If NS > 0 and MPS = 512,\n");
    fprintf (fl, "#      the line is considered as a satellite NB line, T1 will be set to 6).\n");
    fprintf (fl, "# NS: number of PVC stations on the link; 0 for HDLC.\n");
    fprintf (fl, "# LS: link state (0: enabled, 1: disabled; see prod_distri_info.h).\n");
    fprintf (fl, "# DEN: Incoming data event notification. 1 enabled; 0 disabled.\n");
    fprintf (fl, "\n");
    fprintf (fl, "# LN  UN  CN  DN  PN  LT          LR CS          MPS NS  LS DEN\n");
    for (i = 0; i < N_links; i++)
	fprintf (fl, "%4d%4d%4d%4d%4d  %5s  %7d %9s  %4d  %1d   %1d   %1d\n",
		Links[i].line_ind, Links[i].user_ind, Links[i].cm_ind, 
		Links[i].dev_n, Links[i].port_n, Links[i].line_type, 
		Links[i].line_rate, Links[i].cm_name, Links[i].packet_size, 
		Links[i].n_pvcs, Links[i].link_state, Links[i].en);
    fprintf (fl, "\n");
    fprintf (fl, "number_links    %d\n", N_links);

    n_misc_spec_lines = DEAU_get_string_values ("comms.misc_spec", &p);
    for (i = 0; i < n_misc_spec_lines; i++) {
	fprintf (fl, "%s\n", p);
	p += strlen (p) + 1;
    }
    fprintf (fl, "\n");
    fprintf (fl, "Run_time_created\n");
    fclose (fl);
    return (0);
}

/**************************************************************************

    Prints CS error message.

**************************************************************************/

static void Cs_error_func (char *err_msg) {
    LE_send_msg (GL_ERROR, "%s", err_msg);
}

/****************************************************************************

   Description:
      Process command line arguments.

   Inputs:
      argc - number of command line arguments.
      argv - the command line arguments.

   Outputs:
      startup_action - start up action (STARTUP or RESTART)

   Returns:
      exits on error, or returns 0 on success.

*****************************************************************************/

static int Get_command_line_options (int argc, char *argv[], 
						char **startup_action) {
    extern char *optarg;
    extern int optind;
    int c, err;

    /* Initialize startup_action to RESTART and input_file_path to NULL. */
    *startup_action = "startup";

    err = 0;
    while ((c = getopt (argc, argv, "ht:")) != EOF) {

	switch (c) {

	    case 't':
		*startup_action = optarg;
		break;

	    case 'h':
	    case '?':
	    default:
		err = 1;
		break;
	}
    }

    if (err == 1) {              /* Print usage message */
	printf ("Usage: %s [options]\n", MISC_basename (argv[0]));
	printf ("    options:\n");
	printf ("    -t startup[restart, init or clear] (default: startup)\n");
	printf ("       \"clear\" deletes product distribution a/c DBs\n");
	printf ("    -h (print usage msg and exit)\n");
	exit (1);
    }

    return (0);
}
