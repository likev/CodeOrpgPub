
/******************************************************************

    This is the main module for process_adapt_data.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:41:56 $
 * $Id: pad_main.c,v 1.20 2005/12/27 16:41:56 steves Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <orpg.h> 
#include <infr.h> 


#define PAD_NAME_SIZE 256

#define BIN_AC_NAME "ac.lb"

static char Pad_cmd[PAD_NAME_SIZE];	/* process_adapt_data command */
static char Cfg_dea[PAD_NAME_SIZE];	/* DEA files dir */
static char Cfg_dir[PAD_NAME_SIZE];	/* System configuration dir */

static char Merge_src[PAD_NAME_SIZE];	/* source DB file (LB) for merge */
static char Merge_dest[PAD_NAME_SIZE];	/* destination DB file for merge */

static int Operational = 0;		/* operatinal environment */


static void Err_func (char *msg);
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int Terminate (int signal, int sigtype);
static void Pad_init ();
static void Pad_merge ();
static void Pad_backup ();
static void Pad_restore ();
static void Initialize ();
static void Set_to_defaults (int force);
static void Init_dea_database ();
static int Read_dea_files (char *suffix, char *dir_name);
static void Verify_de_values (DEAU_attr_t *de);
static void Verify_values ();
static void Init_legacy_adapt ();
static void Set_baseline_values ();


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char *argv[]) {
    int ret;

/*
PADRL_read_legacy_data ("/export/home/jing/objs/adapt.lb");
exit (0);
*/

    if (Read_options (argc, argv) != 0)
	exit (1);

    /* Initialize the LE and CS services */
    ret = ORPGMISC_init (argc, argv, 500, LB_SINGLE_WRITER, -1, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMISC_le_init failed (ret %d)", ret);
	exit (1);
    }

    ret = ORPGTASK_reg_term_handler (Terminate);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGTASK_reg_term_hdlr failed: %d", ret);
	exit (1);
    }

    Initialize ();

    DEAU_set_error_func (Err_func);

    if (strcmp (Pad_cmd, "init") == 0)
	Pad_init ();
    else if (strcmp (Pad_cmd, "merge") == 0)
	Pad_merge ();
    else if (strcmp (Pad_cmd, "backup") == 0)
	Pad_backup ();
    else if (strcmp (Pad_cmd, "restore") == 0)
	Pad_restore ();
    else {
	LE_send_msg (GL_ERROR, "Unexpected process_adapt_data command: %s", Pad_cmd);
	exit (1);
    }

    exit (0);
}

/**************************************************************************

    Reads in DEA files and initializes the database. In the operational
    environment, we copy the the binary a/c file to the operational one
    if the operational one is empty. We then update the database tables.
    We do this to allow dynamically adding/removing DEs (will need a
    startup).

**************************************************************************/

static void Pad_init () {
    LB_status st;
    LB_attr attr;
    int ret;

    LE_send_msg (GL_INFO, "process_adapt_data init...");

    Init_legacy_adapt ();

    if (!Operational)
	Init_dea_database ();

    ret = ORPGDA_open (ORPGDAT_ADAPT_DATA, LB_WRITE);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGDA_open (%d) failed (%d)", ORPGDAT_ADAPT_DATA, ret);
	LE_send_msg (GL_ERROR, "RPG adaptation data not installed");
	exit (1);
    }

    st.attr = &attr;
    st.n_check = 0;
    ret = ORPGDA_stat (ORPGDAT_ADAPT_DATA, &st);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGDA_stat (%d) failed (%d)", ORPGDAT_ADAPT_DATA, ret);
	exit (1);
    }

    if (st.n_msgs == 0) {
	LE_send_msg (GL_ERROR, "RPG adaptation data empty");
	exit (1);
    }

    LE_send_msg (GL_INFO, "process_adapt_data init completed - operational");
    exit (0);
}

/**************************************************************************

    Merges the data values in "Merge_src" into "Merge_dest". This function
    does not return.

**************************************************************************/

static void Pad_merge () {
    static char *ivb = NULL, *buf = NULL, *dbuf = NULL;
    DEAU_attr_t *at;
    char *id, *p;
    int ret, cnt, i;
    typedef struct {
	int type;
	int id_off;
	int n_values;
	int value_off;
    } id_value_t;
    id_value_t *ivs;

    LE_send_msg (GL_INFO, "process_adapt_data merge...");
    if (strlen (Merge_src) == 0 || strlen (Merge_dest) == 0) {
	LE_send_msg (GL_ERROR, 
	    "Not both source and destination files are specified for merge");
	exit (1);
    }

    LE_send_msg (GL_INFO, "Reading merge source (%s)", Merge_src);
    DEAU_LB_name (Merge_src);
    ivb = STR_reset (ivb, 10000);
    buf = STR_reset (buf, 100000);
    dbuf = STR_reset (dbuf, 100000);
    cnt = 0;
    DEAU_get_next_dea (NULL, NULL);
    ret = DEAU_get_next_dea (&id, &at);
    while (1) {
	if (ret == DEAU_DE_NOT_FOUND)
	    break;
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
			"DEAU_get_next_dea (%s) failed (%d)", Merge_src, ret);
	    exit (1);
	}
	ret = DEAU_check_permission (at, "URC");
	if (ret > 0) {
	    id_value_t iv;
	    int nv;

	    iv.id_off = STR_size (buf);
	    buf = STR_append (buf, (char *)at->ats[DEAU_AT_ID], 
				    strlen (at->ats[DEAU_AT_ID]) + 1);
	    iv.type = DEAU_get_data_type (at);
	    nv = DEAU_get_number_of_values (NULL);
	    if (nv > 0) {
		iv.n_values = nv;
		if (iv.type == DEAU_T_STRING) {
		    iv.value_off = STR_size (buf);
		    if ((ret = DEAU_get_string_values (NULL, &p)) != nv) {
			LE_send_msg (GL_ERROR, 
				"DEAU_get_string_values failed (%d)", ret);
			exit (1);
		    }
		    for (i = 0; i < nv; i++) {
			buf = STR_append (buf, p, strlen (p) + 1);
			p += strlen (p) + 1;
		    }
		}
		else {
		    static char *tbf = NULL;
		    iv.value_off = STR_size (dbuf);
		    tbf = STR_reset (tbf, nv * sizeof (double));
		    DEAU_get_values (NULL, (double *)tbf, nv);
		    dbuf = STR_append (dbuf, tbf, nv * sizeof (double));
		}
		ivb = STR_append (ivb, (char *)&iv, sizeof (id_value_t));
		cnt++;
	    }
	}
	ret = DEAU_get_next_dea (&id, &at);
    }
    if (cnt == 0)		/* nothing to merge */
	exit (0);

    LE_send_msg (GL_INFO, "Staring merge to (%s)", Merge_dest);
    DEAU_LB_name (Merge_dest);
    ivs = (id_value_t *)ivb;
    for (i = 0; i < cnt; i++) {
	char *values;
	int need_merge, n;

	if (ivs->type == DEAU_T_STRING)
	    values = buf + ivs->value_off;
	else
	    values = dbuf + ivs->value_off;
	id = buf + ivs->id_off;
	ret = DEAU_get_attr_by_id (id, &at);
	need_merge = 1;
	if (ret < 0)		/* DE deleted */
	    need_merge = 0;
	else {
	    if (DEAU_check_permission (at, "URC") <= 0) {
		LE_send_msg (GL_INFO, 
		    "Data (id %s) premission changed - not merged", id);
		need_merge = 0;
	    }
	    if (ivs->type != DEAU_get_data_type (at)) {
		LE_send_msg (GL_INFO, 
		    "Data (id %s) type does not match - not merged", id);
		need_merge = 0;
	    }
	    p = values;
	    for (n = 0; n < ivs->n_values; n++) {
		if (ivs->type == DEAU_T_STRING)
		    ret = DEAU_check_data_range (NULL, ivs->type, 1, p);
		else
		    ret = DEAU_check_data_range (NULL, DEAU_T_DOUBLE, 1, p);
		if (ret == -1) {
		    LE_send_msg (GL_INFO, 
		        "Data (id %s) out of range - not merged", id);
		    need_merge = 0;
		}
		else if (ret < 0) {
		    LE_send_msg (GL_INFO, 
		        "Data (id %s) range check falied (%d) - not merged", 
							id, ret);
		    need_merge = 0;
		}
		if (ivs->type == DEAU_T_STRING)
		    p += strlen (p) + 1;
		else
		    p += sizeof (double);
	    }

	    if (need_merge) {
		if (ivs->type == DEAU_T_STRING)
 		    ret = DEAU_set_values (id, 1, values, ivs->n_values, 0);
		else
 		    ret = DEAU_set_values (id, 0, values, ivs->n_values, 0);
		if (ret < 0)
		    LE_send_msg (GL_INFO, 
			"DEAU_set_values value (id %s) failed (%d)", id, ret);
		else
		    LE_send_msg (GL_INFO, "Merge (id %s) done", id);
		if (ivs->type == DEAU_T_STRING)
 		    ret = DEAU_set_values (id, 1, values, ivs->n_values, 1);
		else
 		    ret = DEAU_set_values (id, 0, values, ivs->n_values, 1);
		if (ret < 0)
		    LE_send_msg (GL_INFO, 
			"DEAU_set_values baseline value (id %s) failed (%d)", 
								id, ret);
	    }
	}
	ivs++;
    }

    LE_send_msg (GL_INFO, "Merge completed");
    exit (0);
}

/**************************************************************************

    Saves the current adaptation/configuration for backup purpose.

**************************************************************************/

static void Pad_backup () {
}

/**************************************************************************

    Restores the adaptation/configuration backup to the current database.

**************************************************************************/

static void Pad_restore () {
}

/**************************************************************************

    Reads all DEA files in dir "Cfg_dea" and "Cfg_dir" and creates the DEA 
    database. This function does not return.

**************************************************************************/

static void Init_dea_database () {
    char *db_name;
    int ret, de_cnt;

    DEAU_special_suffix ("alg");

    de_cnt = Read_dea_files (".dea", Cfg_dir);
    de_cnt += Read_dea_files (NULL, Cfg_dea);

    if (de_cnt == 0) {
	LE_send_msg (GL_INFO, "No data element found in %s and %s\n", 
						Cfg_dir, Cfg_dea);
	exit (0);
    }
    else
	LE_send_msg (GL_INFO, "%d data elements found in %s and %s\n", 
						de_cnt, Cfg_dir, Cfg_dea);

    db_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
    if (db_name == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA);
	exit (1);
    }
    DEAU_LB_name (db_name);
    LE_send_msg (GL_INFO, "Create a/c DB");
    ret = DEAU_create_dea_db ();
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "DEAU_create_dea_db failed (%d)\n", ret);
	exit (1);
    }
    LE_send_msg (GL_INFO, "Set uninitialized value to default");
    Set_to_defaults (0);
    LE_send_msg (GL_INFO, "Verify current and baseline values");
    Verify_values ();
    LE_send_msg (GL_INFO, "Set uninitialized baseline values");
    Set_baseline_values ();
    LE_send_msg (GL_INFO, "Process_adapt_data init completed - non-operational");
    exit (0);
}

/**************************************************************************

    Processes one pass of all files. In "pass" == 1, we read all files 
    except "*.upd" files. In "pass" == 1, we read all "*.upd" files to
    override the existing attributes in the database. Returns the number
    of newly found DEs.

**************************************************************************/

static int Read_dea_files (char *suffix, char *dir_name) {
    DIR *dir;
    char *vb;
    struct dirent *dp;
    int de_cnt;

    dir = opendir (dir_name);
    if (dir == NULL) {
	if (errno == ENOENT) {
	    LE_send_msg (GL_INFO, "dir %s not found\n", dir_name);
	    return (0);
	}
	LE_send_msg (GL_ERROR, 
		"opendir (%s) failed, errno %d\n", dir_name, errno);
	return (0);
    }
    vb = NULL;
    vb = STR_reset (vb, 128);
    de_cnt = 0;
    while ((dp = readdir (dir)) != NULL) {
	int ret;
	struct stat st;

	if (strcmp (dp->d_name, ".") == 0 || strcmp (dp->d_name, "..") == 0 ||
	    dp->d_name[strlen (dp->d_name) - 1] == '%')
	    continue;

	vb = STR_copy (vb, dir_name);
	vb = STR_cat (vb, "/");
	vb = STR_cat (vb, dp->d_name);
	ret = stat (vb, &st);
	if (ret < 0) 
	    continue;

	if (S_ISREG (st.st_mode)) {
	    int ln, ls;

	    if (suffix != NULL && 
		((ln = strlen (dp->d_name)) < (ls = strlen (suffix)) ||
		 strcmp (dp->d_name + (ln - ls), suffix) != 0))
		continue;
	    LE_send_msg (GL_INFO, "read DEA file %s\n", vb);
	    ret = DEAU_use_attribute_file (vb, 0);
	    if (ret < 0) {
		LE_send_msg (GL_ERROR, 
		    "DEAU_use_attribute_file (%s) failed (%d)\n", vb, ret);
		exit (1);
	    }
	    else
		de_cnt += ret;
	}
    }
    STR_free (vb);
    closedir (dir);
    return (de_cnt);
}

/**************************************************************************

    Sets undefined baseline value to the current value for all DEs.

**************************************************************************/

static void Set_baseline_values () {
    char *id;
    int ret;
    DEAU_attr_t *at;

    DEAU_get_next_dea (NULL, NULL);
    ret = DEAU_get_next_dea (&id, &at);
    while (1) {
	if (ret == DEAU_DE_NOT_FOUND)
	    break;
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "DEAU_get_next_dea failed (%d)", ret);
	    exit (1);
	}

	if (at->ats[DEAU_AT_BASELINE][0] == '\0' && 
					at->ats[DEAU_AT_ID][0] != '@') {
	    static char *vb = NULL, *idbuf = NULL;

	    idbuf = STR_copy (idbuf, id);
	    vb = STR_copy (vb, at->ats[DEAU_AT_VALUE]);
	    ret = DEAU_update_attr (idbuf, DEAU_AT_BASELINE, vb);
	    if (ret < 0) {
		LE_send_msg (GL_ERROR, 
		    "DEAU_update_attr (baseline) (%s) failed (%d)", id, ret);
		exit (1);
	    }
	}
	ret = DEAU_get_next_dea (&id, &at);
    }
    return;
}

/**************************************************************************

    Sets site the value attributes for all DEs according to the local site
    name and the default attributes. The function terminates the process
    on failure.

**************************************************************************/

static void Set_to_defaults (int force) {
    char site[128], *p;
    char *id;
    int ret;

    if ((p = ORPGMISC_get_site_name ("site")) == NULL) {
	LE_send_msg (GL_INFO, 
		"Site name not available - default values not used");
	return;
    }
    strncpy (site, p, 128);
    site [127] = '\0';

    DEAU_get_next_dea (NULL, NULL);
    ret = DEAU_get_next_dea (&id, NULL);
    while (1) {
	if (ret == DEAU_DE_NOT_FOUND)
	    break;
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "DEAU_get_next_dea failed (%d)", ret);
	    exit (1);
	}
	ret = DEAU_use_default_values (id, site, force);
	if (ret < 0 && ret != DEAU_DEFAULT_NOT_FOUND && 
					    ret != DEAU_DE_NOT_FOUND) {
	    LE_send_msg (GL_ERROR, 
			    "DEAU_use_default_values failed (%d)", ret);
	    exit (1);
	}
	ret = DEAU_get_next_dea (&id, NULL);
    }
    return;
}

/**************************************************************************

    Verifies, for all DEs in the database, if any value or baseline value
    is within the specified range.

**************************************************************************/

static void Verify_values () {
    char *id;
    int ret;
    DEAU_attr_t *at;

    DEAU_get_next_dea (NULL, NULL);
    ret = DEAU_get_next_dea (&id, &at);
    while (1) {
	if (ret == DEAU_DE_NOT_FOUND)
	    break;
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "DEAU_get_next_dea failed (%d)", ret);
	    exit (1);
	}
	Verify_de_values (at);
	ret = DEAU_get_next_dea (&id, &at);
    }
    return;
}

/**************************************************************************

    Verifies if any value or baseline value of data element "de", is within
    the specified range.

**************************************************************************/

static void Verify_de_values (DEAU_attr_t *de) {
    char *id, *p;
    int type, total, ret, nd;
    double dbuf[100];

    id = de->ats[DEAU_AT_ID];
    type = DEAU_get_data_type (de);
    if (type == DEAU_T_UNDEFINED) {
	if (strlen (de->ats[DEAU_AT_RANGE]) == 0)
	    return;
	type = DEAU_T_INT;
    }
    total = DEAU_get_number_of_values (NULL);
    if (total < 0)
	goto failed;
    if (total == 0)
	return;
    nd = total;
    if (nd > 100)
	nd = 100;
    if (type == DEAU_T_STRING) {
	if (DEAU_get_string_values (NULL, &p) != total)
	    goto failed;
	ret = DEAU_check_data_range (NULL, type, total, p);
    }
    else {
	if (DEAU_get_values (NULL, dbuf, nd) != nd)
	    goto failed;
	ret = DEAU_check_data_range (NULL, DEAU_T_DOUBLE, nd, (char *)dbuf);
    }
    if (ret < 0)
	goto failed;

    if (type == DEAU_T_STRING) {
	ret = DEAU_get_baseline_string_values (NULL, &p);
	if (ret == 0)
	    return;
	if (ret != total)
	    goto failed;
	ret = DEAU_check_data_range (NULL, type, total, p);
    }
    else {
	ret = DEAU_get_baseline_values (NULL, dbuf, nd);
	if (ret == 0)
	    return;
	if (ret != nd)
	    goto failed;
	ret = DEAU_check_data_range (NULL, DEAU_T_DOUBLE, nd, (char *)dbuf);
    }
    if (ret < 0)
	goto failed;
    return;

failed:
    LE_send_msg (GL_ERROR, "Bad value in dea file for %s", id);
    exit (1);
}

/**************************************************************************

    Initializes the directory names and other flags.

**************************************************************************/

static void Initialize () {
    char dir[PAD_NAME_SIZE];
    int ret;

    ret = MISC_get_cfg_dir (dir, PAD_NAME_SIZE);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "MISC_get_cfg_dir failed (%d)", ret);
	exit (1);
    }
    if (strlen (dir) + 16 + strlen (BIN_AC_NAME) >= PAD_NAME_SIZE) {
	LE_send_msg (GL_ERROR, "CFG dir name too long");
	exit (1);
    }
    sprintf (Cfg_dea, "%s/%s", dir, "dea");
    strcpy (Cfg_dir, dir);

    if (ORPGMISC_is_operational ())
	Operational = 1;
}

/**************************************************************************

    Terminates this process.

**************************************************************************/

static int Terminate (int signal, int sigtype) {

    LE_send_msg (GL_INFO,  "process_adapt_data terminates (signal %d)", signal);
    return (0);
}

/**************************************************************************

    Directs DEAU messages to LE.

**************************************************************************/

static void Err_func (char *msg) {
    LE_send_msg (GL_INFO, "%s", msg);
}

/**************************************************************************

    Initializes the legacy adaptation data.

**************************************************************************/

static void Init_legacy_adapt () {
    int ret;
    char *buf;

    ORPGDA_open (ORPGDAT_ADAPTATION, LB_WRITE);
    ret = ORPGDA_read (ORPGDAT_ADAPTATION, &buf, LB_ALLOC_BUF, RDACNT);
    if (ret <= 0) {

	LE_send_msg (GL_INFO, "Initialize ORPGDAT_ADAPTATION (RDACNT)...");
        ret = MISC_system_to_buffer ("adapt_data", NULL, 0, NULL);
	if (ret != 0) {
	    LE_send_msg (GL_ERROR, 
			"MISC_system_to_buffer adapt_data failed (%d)", ret);
	    exit (1);
	}
    }
    else
	free (buf);

    ORPGDA_open (ORPGDAT_BASELINE_ADAPTATION, LB_WRITE);
    ret = ORPGDA_read (ORPGDAT_BASELINE_ADAPTATION, &buf, 
						LB_ALLOC_BUF, RDACNT);
    if (ret <= 0) {		/* copy to baseline LB */
	int len;

	LE_send_msg (GL_INFO, 
		"Copy ORPGDAT_ADAPTATION (RDACNT) to baseline...");
	ret = 0;
	len = ORPGDA_read (ORPGDAT_ADAPTATION, &buf, LB_ALLOC_BUF, RDACNT);
	if (len > 0)
	    ret = ORPGDA_write (ORPGDAT_BASELINE_ADAPTATION, buf, len, RDACNT);
	if (len < 0 || ret < 0) {
	    LE_send_msg (GL_ERROR, 
			"Copying failed (read %d, write %d)", len, ret);
	    exit (1);
	}
	free (buf);
    }
    else
	free (buf);

    ret = ORPGDA_read (ORPGDAT_ADAPTATION, &buf, LB_ALLOC_BUF, COLRTBL);
    if (ret <= 0) {

	LE_send_msg (GL_INFO, "Initialize ORPGDAT_ADAPTATION (COLRTBL)...");
        ret = MISC_system_to_buffer ("adapt_data_gen", NULL, 0, NULL);
	if (ret != 0) {
	    LE_send_msg (GL_ERROR, 
		"MISC_system_to_buffer adapt_data_gen failed (%d)", ret);
	    exit (1);
	}
    }
    else
	free (buf);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    err = 0;
    Pad_cmd[0] = '\0';
    Merge_src[0] = '\0';
    Merge_dest[0] = '\0';
    while ((c = getopt (argc, argv, "s:d:h?")) != EOF) {
	switch (c) {

	    case 's':
		strncpy (Merge_src, optarg, PAD_NAME_SIZE);
		Merge_src[PAD_NAME_SIZE - 1] = '\0';
		break;

	    case 'd':
		strncpy (Merge_dest, optarg, PAD_NAME_SIZE);
		Merge_dest[PAD_NAME_SIZE - 1] = '\0';
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind == argc - 1) {       /* get the comm manager index  */
	strncpy (Pad_cmd, argv[optind], PAD_NAME_SIZE);
	Pad_cmd[PAD_NAME_SIZE - 1] = '\0';
    }

    if (err == 0 && strlen (Pad_cmd) == 0) {
	fprintf (stderr, "process_adapt_data command not specified\n");
	err = -1;
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Process adaptation data. Available commands are
        \"init\": Initializes the shared a/c database
        \"merge\": Performs adaptation merging during installation
        \"backup\": Saves current adaptation data
        \"restore\": Restores the saved adaptation backup
        Options:
            -s merge_source_name (The source file (LB) name for merge)
            -d merge_dest_name (The destination file (LB) name for merge)
";

    printf ("Usage:  %s [options] command\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}



