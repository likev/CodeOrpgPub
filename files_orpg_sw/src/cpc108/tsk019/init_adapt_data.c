
/******************************************************************

    This is the main module for init_adapt_data.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 19:49:40 $
 * $Id: init_adapt_data.c,v 1.21 2012/06/14 19:49:40 jing Exp $
 * $Revision: 1.21 $
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
#include <node_version.h> 
#include <infr.h> 
#include <rpg_port.h>


#define IAD_NAME_SIZE 256

static char *Cfg_dea = NULL;		/* DEA files dir */
static char *Cfg_dir = NULL;		/* System configuration dir */

static int Operational = 0;		/* operatinal environment */
static char *Type = "";

static void Err_func (char *msg);
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int Terminate (int signal, int sigtype);
static void Iad_init ();
static void Initialize ();
static void Set_to_defaults (int force);
static void Init_dea_database ();
static int Read_dea_files (char *suffix, char *dir_name);
static void Verify_de_values (DEAU_attr_t *de);
static void Verify_values ();
static void Init_legacy_adapt ();
static void Set_baseline_values ();
static void Correct_layer_levels ();
static void Clear_adapt ();


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char *argv[]) {
    int ret;

    if (Read_options (argc, argv) != 0)
	exit (1);

    /* Initialize the LE and CS services */
    ret = ORPGMISC_init (argc, argv, 500, LB_SINGLE_WRITER, -1, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGMISC_init failed (ret %d)", ret);
	exit (1);
    }

    ret = ORPGTASK_reg_term_handler (Terminate);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGTASK_reg_term_hdlr failed: %d", ret);
	exit (1);
    }

    Initialize ();

    DEAU_set_error_func (Err_func);

    Iad_init ();
    exit (0);
}

/**************************************************************************

    Reads in DEA files and initializes the database. In the operational
    environment, we copy the the binary a/c file to the operational one
    if the operational one is empty. We then update the database tables.
    We do this to allow dynamically adding/removing DEs (will need a
    startup).

**************************************************************************/

#define BUF_SIZE 512

static void Iad_init () {
    int ret, ver;
    char *db_name, *cmd, cfg_dir[IAD_NAME_SIZE], obuf[BUF_SIZE], *p;

    if (strcmp (Type, "clear") == 0)
	Clear_adapt ();

    LE_send_msg (GL_INFO, "Initialize adaptation data...");

    if (strcmp (Type, "init") != 0)
	Init_legacy_adapt ();

    if (!Operational)
	Init_dea_database ();

    db_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
    if (db_name == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA);
	exit (1);
    }
    DEAU_LB_name (db_name);
    ret = DEAU_get_string_values ("RPG_DEA_DB_init_state", &p);
    if (ret < 0 || strcmp (p, "init completed") != 0) {
	LE_send_msg (GL_INFO, "RPG adaptation data not installed\n");
	exit (1);
    }

    if (MISC_get_cfg_dir (cfg_dir, IAD_NAME_SIZE) <= 0) {
	LE_send_msg (GL_INFO, "MISC_get_cfg_dir failed\n");
	exit (1);
    }
    obuf[0] = '\0';
    cmd = STR_gen (NULL, 
	"bash -l -c \"ls ", cfg_dir, "/adapt/installed/adapt0*.Z\"", NULL);
    if (MISC_system_to_buffer (cmd, obuf, BUF_SIZE, NULL) < 0) {
	LE_send_msg (GL_INFO, "MISC_system_to_buffer (%s) failed\n", cmd);
	exit (1);
    }
    STR_free (cmd);
    if ((p = strstr (obuf, "adapt0")) != NULL &&
	sscanf (p + 6, "%d", &ver) == 1 &&
	(ver / 10) != (ORPGMISC_RPG_adapt_version_number () / 10)) {
	LE_send_msg (GL_INFO, 
		"Bad RPG adapt version number (%d) installed (shoudl be %d)", 
		ver, ORPGMISC_RPG_adapt_version_number ());
	exit (1);
    }

    LE_send_msg (GL_INFO, "init_adapt_data completed - operational");
    exit (0);
}

/**************************************************************************

    Clears the DEA database to it will be reinitialized later.

**************************************************************************/

static void Clear_adapt () {
    int ret;
    ret = DEAU_set_values ("RPG_DEA_DB_init_state", 
					1, "not completed", 1, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"DEAU_set_values RPG_DEA_DB_init_state failed (%d)\n", ret);
	exit (1);
    }
    LE_send_msg (GL_INFO, "init_adapt_data clear completed");
    exit (0);
}

/**************************************************************************

    Reads all DEA files in dir "Cfg_dea" and "Cfg_dir" and creates the DEA 
    database. This function does not return.

**************************************************************************/

static void Init_dea_database () {
    char *db_name;
    int ret, de_cnt;
    struct stat st;
    char *path, *p;

    db_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
    if (db_name == NULL) {
	LE_send_msg (GL_ERROR, 
			"ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA);
	exit (1);
    }
    DEAU_LB_name (db_name);
    ret = DEAU_get_string_values ("RPG_DEA_DB_init_state", &p);
    if (ret > 0 && strcmp (p, "init completed") == 0) {
	LE_send_msg (GL_INFO, "DEA database exits - not initialized\n");
	exit (0);
    }

    path = NULL;
    path = STR_copy (path, Cfg_dir);
    path = STR_cat (path, "/site_info.dea");
    if (stat (path, &st) < 0) {
	LE_send_msg (GL_INFO, 
	    "site_info.dea not found in %s - Use existing data\n", Cfg_dir);
	exit (0);
    }
    STR_free (path);

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

    LE_send_msg (GL_INFO, "Create a/c DB");
    ret = DEAU_create_dea_db ();
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "DEAU_create_dea_db failed (%d)\n", ret);
	exit (1);
    }
    LE_send_msg (GL_INFO, "Set uninitialized value to default");
    Set_to_defaults (0);
    Correct_layer_levels ();
    LE_send_msg (GL_INFO, "Verify current and baseline values");
    Verify_values ();
    LE_send_msg (GL_INFO, "Set uninitialized baseline values");
    Set_baseline_values ();
    ret = DEAU_set_values ("RPG_DEA_DB_init_state", 1, "init completed", 1, 0);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"DEAU_set_values RPG_DEA_DB_init_state failed (%d)\n", ret);
	exit (1);
    }
    LE_send_msg (GL_INFO, "init_adapt_data completed - non-operational");
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
    char *id, *lid;
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
    lid = NULL;
    while (1) {
	if (ret == DEAU_DE_NOT_FOUND)
	    break;
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "DEAU_get_next_dea failed (%d)", ret);
	    exit (1);
	}
	lid = STR_copy (lid, id);
	ret = DEAU_use_default_values (lid, site, force);
	if (ret < 0 && ret != DEAU_DEFAULT_NOT_FOUND && 
					    ret != DEAU_DE_NOT_FOUND) {
	    LE_send_msg (GL_ERROR, 
			    "DEAU_use_default_values failed (%d)", ret);
	    exit (1);
	}
	ret = DEAU_get_next_dea (&id, NULL);
    }
    STR_free (lid);
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
	ret = DEAU_check_data_range (id, type, total, p);
    }
    else {
	if (DEAU_get_values (NULL, dbuf, nd) != nd)
	    goto failed;
	ret = DEAU_check_data_range (id, DEAU_T_DOUBLE, nd, (char *)dbuf);
    }
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "Bad range spec in dea file for %s", id);
	exit (1);
    }

    if (type == DEAU_T_STRING) {
	ret = DEAU_get_baseline_string_values (NULL, &p);
	if (ret == 0)
	    return;
	if (ret != total)
	    goto failed;
	ret = DEAU_check_data_range (id, type, total, p);
    }
    else {
	ret = DEAU_get_baseline_values (NULL, dbuf, nd);
	if (ret == 0)
	    return;
	if (ret != nd)
	    goto failed;
	ret = DEAU_check_data_range (id, DEAU_T_DOUBLE, nd, (char *)dbuf);
    }
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "Bad range spec in dea file for %s", id);
	exit (1);
    }
    return;

failed:
    LE_send_msg (GL_ERROR, "Bad value in dea file for %s", id);
    exit (1);
}

/**************************************************************************

    Corrects layer levels in terms of the radar height location.

**************************************************************************/

static void Correct_layer_levels () {
    double rda_elev, levels[128];
    int ret, ilevel, n_levels, i;
    char *id;

    id = "site_info.rda_elev";
    ret = DEAU_get_values (id, &rda_elev, 1);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "DEAU_get_values %s failed (%d)", id, ret);
	return;
    }

    id = "layer_prod_params_t.first_dbz_level";
    ret = DEAU_get_values (id, levels, 1);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "DEAU_get_values %s failed (%d)", id, ret);
	return;
    }

    ilevel = (int)(levels[0] + .5);
    while ((double)ilevel * 1000. < rda_elev)
	ilevel++;
    levels[0] = (double)ilevel;
    ret = DEAU_set_values (id, 0, levels, 1, 0);
    if (ret < 0) {
	LE_send_msg (GL_INFO, "DEAU_set_values %s failed (%d)", id, ret);
	return;
    }

    id = "vad_rcm_heights_t.vad";
    n_levels = DEAU_get_values (id, levels, 128);
    if (n_levels < 0) {
	LE_send_msg (GL_ERROR, "DEAU_get_values %s failed (%d)", id, n_levels);
	return;
    }
    for (i = 0; i < n_levels; i++) {
	if (levels[i] >= rda_elev)
	    break;
    }
    if (i > 0) {
	ret = DEAU_set_values (id, 0, levels + i, n_levels - i, 0);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "DEAU_set_values %s failed (%d)", id, ret);
	    return;
	}
    }

    id = "vad_rcm_heights_t.rcm";
    n_levels = DEAU_get_values (id, levels, 128);
    if (n_levels < 0) {
	LE_send_msg (GL_ERROR, "DEAU_get_values %s failed (%d)", id, n_levels);
	return;
    }
    for (i = 0; i < n_levels; i++) {
	if (levels[i] >= rda_elev)
	    break;
    }
    if (i > 0) {
	ret = DEAU_set_values (id, 0, levels + i, n_levels - i, 0);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "DEAU_set_values %s failed (%d)", id, ret);
	    return;
	}
    }
}

/**************************************************************************

    Initializes the directory names and other flags.

**************************************************************************/

static void Initialize () {
    char dir[IAD_NAME_SIZE];
    int ret;

    ret = MISC_get_cfg_dir (dir, IAD_NAME_SIZE);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "MISC_get_cfg_dir failed (%d)", ret);
	exit (1);
    }
    Cfg_dea = STR_copy (Cfg_dea, dir);
    Cfg_dea = STR_cat (Cfg_dea, "/dea");
    Cfg_dir = STR_copy (Cfg_dir, dir);

    if (ORPGMISC_is_operational () && strcmp (Type, "init") != 0)
	Operational = 1;
}

/**************************************************************************

    Terminates this process.

**************************************************************************/

static int Terminate (int signal, int sigtype) {

    LE_send_msg (GL_INFO,  "init_adapt_data terminates (signal %d)", signal);
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
    while ((c = getopt (argc, argv, "t:h?")) != EOF) {
	switch (c) {

	    case 't':
		Type = optarg;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\n\
        Initialize RPG adaptation data.\n\
        Options:\n\
            -t init_type (Specifies initialization type. e.g. init, clear)\n\
            -h (Print usage info)\n";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}



