
/******************************************************************

    The ORPGADPT module.
 
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:13:55 $
 * $Id: orpgadptsv.c,v 1.13 2005/12/27 16:13:55 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

#include <orpgadpt.h>
#include <orpgsite.h>
#include <orpgred.h>
#include <orpg.h>
#include <infr.h>

#include "orpgadpt_def.h"

#define ORPGADPT_LOCAL_NAME_SIZE 256

/* Structure for each adaptation configuration file that is saved/restored with
   save_adapt, restore_adapt functionality */
typedef struct {
    char description[256];	/* description of the configuration file */
    char file_name[256];	/* Name of the configuration file  */
} cfg_file_t;

/* List of configuration files to save/restore */
cfg_file_t Cfg_files[] = {
	{"System Configuration File", "sys_cfg" },
	{"Alert Table", "alert_table" },
/*	{"Algorithms", "algorithms" },			*/
	{"Communications", "comms_link.conf" },
	{"Load Shed Table", "load_shed_table" },
	{"site info", "site_info.dea" },
/*	{"Product Parameters", "prod_params" },		*/
	{"Product Generation Tables", "product_generation_tables" },
	{"User Profiles", "user_profiles" }
};

/* Number of configuration files to save/restore */
int No_of_cfg_files = sizeof (Cfg_files) / sizeof (cfg_file_t);

/* Named object store or normal linear buffer ? */
typedef enum { ADAPT_NAME_STORE, LB_ADAPT_FILE } adapt_file_type_t;

typedef struct {
    char description[256];		/* Adaptation category description */
    adapt_file_type_t file_type;	/* named object store or normal LB? */
    int src_data_id;			/* Source data id (baseline) */
    int src_msg_id;			/* -1 means all messages (baseline) */
    int dest_data_id;			/* Destination data id (current); -1
					   means not available */
    int dest_msg_id;			/* -1 means all messages (current) */
} adapt_file_t;

/* List of linear buffer files/messages to restore/save when restoring or 
   saving adaptation data. */
static adapt_file_t Restore_files[] = {
/*    { "Algorithms/Miscelleneous", ADAPT_NAME_STORE, ORPGDAT_BASELINE_ADAPT, 
	-1, ORPGDAT_ADAPT, -1 },		*/
    { "Product Distribution Information", LB_ADAPT_FILE, ORPGDAT_BASELINE_PROD_INFO,
	 -1, ORPGDAT_PROD_INFO, -1 },
    { "User Profiles", LB_ADAPT_FILE, ORPGDAT_BASELINE_USER_PROFILES, 
	-1, ORPGDAT_USER_PROFILES, -1 },
    { "Load Shed Thresholds", LB_ADAPT_FILE, ORPGDAT_LOAD_SHED_CAT, 
	LOAD_SHED_THRESHOLD_BASELINE_MSG_ID, ORPGDAT_LOAD_SHED_CAT, 
	LOAD_SHED_THRESHOLD_MSG_ID },
    { "Legacy Common Block", LB_ADAPT_FILE, ORPGDAT_ADAPTATION, 
	-1, ORPGDAT_BASELINE_ADAPTATION, -1},
    { "RDA Clutter Information", LB_ADAPT_FILE, ORPGDAT_CLUTTERMAP, 
	-1, -1, -1 },
    { "HCI configuration", LB_ADAPT_FILE, ORPGDAT_HCI_DATA, 
	-1, -1, -1 },
    { "Adaptation Version", LB_ADAPT_FILE, ORPGDAT_NODE_VERSION, 
	-1, -1, -1 },
    { "Adaptation Version", LB_ADAPT_FILE, ORPGDAT_ADAPT_DATA, 
	-1, -1, -1 }
};

/* Number of entries in the Restore_files table */
static int No_of_restore_files = sizeof (Restore_files)/ sizeof(adapt_file_t);

/*  Extension attached to saved adaptation data files */
static char Adapt_filename_extension[] = ".adapt##";

/*  prefix on the front of the adaptation archive name  */
static char Adapt_filename[] = "adapt";

static char Last_emsg[ORPGADPT_LOCAL_NAME_SIZE] = "";


int ORPGADPTSV_install (const char* dir, const char* date, 
	const char* time, const char* site, const char* alt_host_name, 
	char** error_buf);
void ORPGADPTSV_init_log ();
void ORPGADPTSV_init ();

static void set_error_buf (char** error_buf);
static void create_log_directory ();
static char *get_config_dir ();
static void Create_string (char **buf, ...);
static int Call_rss_copy (char *src_file, char *dest_file, int *stp);
static char *get_time_tag ();
static int clean_directory (const char *dir_path);
static int *build_data_id_list ();
static int execute_tar_command (const char *tar_file, 
	const char *temp_path, const char *src_file, int first_file);
static char *Get_alternate_file_name (char *fname);
static char *Parm_to_str (const char *str);
static char *Get_unique_string ();
static int Run_checksum (char *file, unsigned int *cksum);


/***************************************************************************

    Create local directory "dir_path". "error_buf" returns error message
    on failure. Returns 1 if successful, 0 otherwise.

***************************************************************************/

int ORPGADPTSV_make_directory (const char *dir_path, char **error_buf) {
    char *mkdir_command = NULL;
    char output_buf[250] = "";
    int ret_bytes, status;

    Last_emsg[0] = '\0';
    Create_string (&mkdir_command, "mkdir -p ", dir_path, NULL);

    /*  Create the desination directory  */
    status = MISC_system_to_buffer (mkdir_command, 
			output_buf, sizeof (output_buf), &ret_bytes);

    if ((status < 0) || (ret_bytes != 0))
	sprintf (Last_emsg, 
		"ORPGADPTSV: MISC_system_to_buffer (%s) failed (%d %d)",
		mkdir_command, status, ret_bytes);
    set_error_buf (error_buf);
    return ((status >= 0) && (ret_bytes == 0));
}

/***************************************************************************

    Tests if local directoy "dir_path" exists. Returns 1 if the directory 
    exists, 0 otherwise.

***************************************************************************/

int ORPGADPTSV_directory_exists (const char *dir_path) {
    DIR *dir = opendir (dir_path);
    if (dir != NULL)
        closedir (dir);
    return (dir != NULL);
}

/***************************************************************************

    Server side readdir function. This function works with ORPGADPT_opendir to
    support the system calls opendir and readdir remotely. "dir_out (out)" 
    is the output directory string associated with "dp" and "dp (in)" is
    the DIR handle returned by ORPGADPTSV_opendir or ORPGADPTSV_readdir.
    Returns 1 if successful, 0 otherwise.

***************************************************************************/

int ORPGADPTSV_readdir (char **dir_out, DIR *dp) {
    static struct dirent *dir;

    dir = readdir (dp);
    if (dir == NULL)
	*dir_out = "";
    else
	*dir_out = dir->d_name;
    return (dir != NULL);
}

/***************************************************************************

    Saves adaptation data to an adaptation data archive. 
    "(in) destination_path" - directory where the adaptation archive will be 
    written (can be remote). "(in) site_name" - alternate name to use for 
    the site name.  If NULL, the site in misc_adapt will be used.
    "(in) alternate_host_name" - alternate host name to use for the archive 
    host name. If NULL, the host name in misc_adapt will be used.
    "error_buf (out)" - on success, the name of the archive saved is 
    returned in this value, otherwise the error text is returned
    Returns 1 if successful, 0 otherwise.

***************************************************************************/

int ORPGADPTSV_save (const char *destination_path, const char *site_name, 
			const char *alternate_host_name, char **error_buf) {
    char *orpgdir, output_buf[250], *unique_string;
    static char *temp_path = NULL, *temp_file_name = NULL,
	*tar_file = NULL, *dest_file_name = NULL, *installed_path = NULL, 
	*dest_file = NULL;
    int success, ret_bytes, status, i;
    char adapt_data_version[10], *tmpp, *host_name, *site;

    success = 1;
    status = 0;
    ORPGADPTSV_init_log ();
    LE_send_msg (GL_INFO, 
	"ORPGADPTSV_save: dest_path %s, site_name %s, alt_host_name %s", 
		Parm_to_str (destination_path), Parm_to_str (site_name), 
		Parm_to_str (alternate_host_name));
    orpgdir = getenv ("ORPGDIR");
    output_buf[0] = '0';
    set_error_buf (error_buf);
    Last_emsg[0] = '\0';

    if (ORPGMISC_get_site_name ("type") == NULL) {
	sprintf (Last_emsg, "ORPGADPTSV: Site name not found\n");
	return (0);
    }
    sprintf (adapt_data_version, "%05d", ORPGADPT_DATA_VERSION);
    if (alternate_host_name == NULL || strlen (alternate_host_name) == 0)
	host_name = ORPGMISC_get_site_name ("type");	/* node type */
    else
	host_name = (char *)alternate_host_name;	/* e.g. rpg1, mscf */

    if (host_name != NULL && strcmp (host_name, "mscf") == 0) {
	sprintf (Last_emsg, 
		"ORPGADPTSV: MSCF does not have adapt data to save\n");
	return (success);
    }

    if (site_name == NULL || strlen (site_name) == 0)
	site = ORPGMISC_get_site_name ("site");		/* e.g. KTLX */
    else
	site = (char *)site_name;
    if (orpgdir == NULL) {
	sprintf (Last_emsg, "ORPGADPTSV: ORPGDIR not specified\n");
	return (0);
    }
    if (get_config_dir () == NULL) {
	sprintf (Last_emsg, "ORPGADPTSV: CFGDIR not specified\n");
	return (0);
    }
    if (destination_path == NULL) {
	sprintf (Last_emsg, "ORPGADPTSV: destination path not specified\n");
	return (0);
    }

    /* Generate a unique string for tmp file name */
    unique_string = Get_unique_string ();

    if (strcmp ((const char *)host_name, "rpg") == 0)
	tmpp = "rpg1";
    else
	tmpp = host_name;
    Create_string (&dest_file_name, Adapt_filename, adapt_data_version, 
		".", site, ".", tmpp, ".", get_time_tag (), ".Z", NULL);

    Create_string (&dest_file, destination_path, "/", dest_file_name, NULL);
    Create_string (&temp_path, orpgdir, "/adapt/archive", unique_string, NULL);

    if (!clean_directory ((const char*)temp_path)) {
	sprintf (Last_emsg, 
		"ORPGADPTSV: clean_directory (%s) failed\n", temp_path);
	return (0);
    }

    /* Create the necessary directories */
    if (!ORPGADPT_make_directory ((const char*)temp_path)) {
	sprintf (Last_emsg, 
		"ORPGADPTSV: make_directory (%s) failed", temp_path);
	return (0);
    }
    if (!ORPGADPT_make_directory ((const char*)destination_path)) {
	sprintf (Last_emsg, "ORPGADPTSV: make_directory (%s) failed", 
						destination_path);
	return (0);
    }

    Create_string (&installed_path, 
			get_config_dir (), "adapt/installed", NULL);

    /* Append the appropriate destination path to the tar file */
    Create_string (&tar_file, temp_path, "/", Adapt_filename, 
		adapt_data_version, Adapt_filename_extension, NULL);

    /* mscf does not use any LB adaptation data */
    i = 0;
    if ((status >= 0) && strcmp (host_name, "mscf") != 0) {
	int *data_ids = build_data_id_list ();
	if (data_ids != NULL) {
	    for (i = 0; (data_ids[i] != -1); i++) {
		char *data_path_base;

		data_path_base = MISC_string_basename 
			((char*)ORPGCFG_dataid_to_path (data_ids[i], NULL));
		Create_string (&temp_file_name, temp_path, "/", 
			data_path_base, Adapt_filename_extension, NULL);

		status = ORPGDA_copy_to_lb ((const char*)temp_file_name, 
				data_ids[i], ORPGDA_CREATE_DESTINATION);
		if (status < 0) {
		    sprintf (Last_emsg, 
			"ORPGADPTSV: ORPGDA_copy_to_lb (%s, %d) failed\n", 
				temp_file_name, data_ids[i]);
		    success = 0;
                    break;
		}
	        LE_send_msg (GL_INFO, "Saving adaptation file %s", 
				ORPGCFG_dataid_to_path (data_ids[i], NULL));
		execute_tar_command ((const char *)tar_file, 
			(const char*)temp_path,
			data_path_base, (i == 0));
	    }
	    free (data_ids);
	}
    }

    if (status >= 0) {
	static char *temp_src_file = NULL;
	int j;
	for (j = 0; j < No_of_cfg_files; j++) {

	    Create_string (&temp_file_name, temp_path, "/", 
		Cfg_files[j].file_name, Adapt_filename_extension, NULL);

	    Create_string (&temp_src_file, get_config_dir (), 
					Cfg_files[j].file_name, NULL);

	    if ((strcmp (host_name, "mscf") != 0 &&
		 strcmp ("site_info.dea", Cfg_files[j].file_name) != 0) ||
		(strcmp (host_name, "mscf") == 0 &&
		    (strcmp ("sys_cfg", Cfg_files[j].file_name) == 0 ||
		     strcmp ("site_info.dea", Cfg_files[j].file_name) == 0 ||
		     strcmp ("algorithms", Cfg_files[j].file_name) == 0 ||
		     strcmp ("prod_params", Cfg_files[j].file_name) == 0))) {

		success = Call_rss_copy (temp_src_file, temp_file_name, 
								&status);
		if (!success)
		    break;
		LE_send_msg (GL_INFO, "Saving configuration file %s\n", 
						temp_src_file);
    		execute_tar_command ((const char *)tar_file, 
			(const char *)temp_path, Cfg_files[j].file_name, 
			((i == 0) && (j == 0)));
	    }
	}
    }

    /* Compress */
    if (status >= 0) {
	static char *compress_command = NULL;

	Create_string (&compress_command, "compress ", tar_file, NULL);
	status = MISC_system_to_buffer (compress_command, output_buf, 
					sizeof (output_buf), &ret_bytes);
	if (status < 0) {
	    sprintf (Last_emsg, 
	    "ORPGADPTSV: MISC_system_to_buffer (compress %s) failed (%d)\n",
							tar_file, status);
	    success = 0;
	}
    }

    if (status >= 0) {
	static char *src_file = NULL;

	Create_string (&src_file, tar_file, ".Z", NULL);

	success = Call_rss_copy (src_file, dest_file, &status);
	if (success) {		/* use cksum to verify if copy is OK */
	    unsigned int cksums, cksumd;
	    if (Run_checksum (src_file, &cksums) < 0 ||
		Run_checksum (dest_file, &cksumd) < 0 ||
		cksums != cksumd) {
		sprintf (Last_emsg, 
			"ORPGADPTSV: Copying adaptation data to %s failed\n", 
			dest_file);
		success = 0;
	    }
	}
	if (success) {

	    if (strcmp (host_name, "mscf") != 0) {
		/* Also replace the archive in the installed directory with 
		   a new archive. Remove the contents of the 
		   $CFG_DIR/adapt/installed directory first */

		clean_directory ((const char *)installed_path);
		/* re-create the installed directory */
	        if (!ORPGADPT_make_directory ((const char*)installed_path)) {
		    sprintf (Last_emsg, 
			    "ORPGADPTSV: make_directory (%s) failed\n", 
						installed_path);
		    success = 0;
		}
		else {
		    installed_path = STR_cat (installed_path, "/");
		    installed_path = STR_cat (installed_path, dest_file_name);

		    /* Even if the cleanup does not work, copy the saved 
		       archive to the installed directory */
		    success = Call_rss_copy (src_file, installed_path, 
								&status);
		}
	    }
	}
    }

    /* Clean up the temporary directory */
    clean_directory ((const char*)temp_path);

    if (success) {
	*error_buf = dest_file;
	LE_send_msg (GL_INFO, 
			"Adaptation Archive written to %s\n", *error_buf);
    }
    return (success);
}

/***********************************************************************

    Restores an adaptation archive by copying it into the adaptation 
    installation directory for installation on the next startup. 
    archive_path(in) - Path where one or more adaptation archives exist;
    date(in) - date to restore if NULL, today's date will be used.
    time(in) - time to restore if NULL, the 23:59:59 will be used to 
	extract the latest time for the specified date.
    site(in) - name of the site to restore if NULL and the current site 
	can be determined, then the latest archive for the current site 
	will be selected, otherwise any archive site will be selected
	for restoring.
    alt_host_name(in) - alternate host name to use when searching for 
	archives, if NULL, archives with host names equal to the local 
	host name and its aliases will be used.
    error_buf(out) - on success, the name of the archive used for 
	restoring is returned in this value, otherwise the error is returned.
    Returns TRUE if successful, and FALSE otherwise.

***********************************************************************/

int ORPGADPTSV_restore (const char *dir, const char *date, 
		const char *time, const char *site, 
		char *alt_host_name, char **error_buf) {
    int success, status;
    char *restore_archive_name, *cfgdir;
    static char *cfg_path = NULL, *tar_file = NULL;
    char *host_name, *site_name;

    success = 1;
    restore_archive_name = NULL;
    cfgdir = getenv ("CFG_DIR");
    set_error_buf (error_buf);
    Last_emsg[0] = '\0';

    ORPGADPTSV_init_log ();
    LE_send_msg (GL_INFO, 
"ORPGADPTSV_restore: dir %s, date %s, time %s, site %s, alt_host_name %s", 
		Parm_to_str (dir), Parm_to_str (date), Parm_to_str (time), 
		Parm_to_str (site), Parm_to_str (alt_host_name));
    RMT_time_out (600);	/* Set time out high - low bandwidth connection */

    status = 0;
    if (alt_host_name == NULL || strcmp (alt_host_name, "_localhost") == 0)
	host_name = ORPGMISC_get_site_name ("type");
    else
	host_name = alt_host_name;

    if (host_name != NULL && strcmp (host_name, "mscf") == 0) {
	sprintf (Last_emsg, 
		"ORPGADPTSV: MSCF does not have adapt data to restore\n");
	return (success);
    }

    if (site == NULL || strlen (site) == 0)
	site_name = ORPGMISC_get_site_name ("site");	/* e.g. KTLX */
    else
	site_name = (char *)site;

    /* If we are on the mscf, a restore is really an install */
    if (host_name != NULL && strcmp (host_name, "mscf") == 0) {
	success = ORPGADPTSV_install (dir, date, time, site_name, 
				(const char*)host_name, error_buf);
	if (success)
	    restore_archive_name = *error_buf;
    }
    else {
	char *s;

	if (cfgdir == NULL) {
	    sprintf (Last_emsg, "ORPGADPTSV: CFG_DIR not defined\n");
	    return (0);
	}
	if (dir == NULL) {
	    sprintf (Last_emsg, "ORPGADPTSV: src path not specified\n");
	    return (0);
	}

	/* search a matched file in dir */
	restore_archive_name = ORPGADPT_get_archive_name (dir, date, time, 
			(const char *)site_name, (const char *)host_name);
	if (restore_archive_name == NULL || 
				strlen (restore_archive_name) == 0) {
	    sprintf (Last_emsg, "ORPGADPTSV: Archive (date: %s, time %s, site %s, node %s) not found in %s\n", 
		Parm_to_str (date), Parm_to_str (time), 
		Parm_to_str (site_name), Parm_to_str (host_name), dir);
	    return (0);
	}

	Create_string (&cfg_path, cfgdir, "/adapt", NULL);
	if (!clean_directory ((const char *)cfg_path)) {
	    sprintf (Last_emsg, 
			"ORPGADPTSV: clean_directory %s failed\n", cfg_path);
	    return (0);
	}

	/* Create the necessary directories */
	if (!ORPGADPT_make_directory ((const char*)cfg_path)) {
	    sprintf (Last_emsg, 
			"ORPGADPTSV: make directory %s failed\n", cfg_path);
	    return (0);
	}

	/*  Append the appropriate destination path to the tar file */
	Create_string (&tar_file, cfg_path, "/", 
			MISC_string_basename (restore_archive_name), NULL);
        success = Call_rss_copy (restore_archive_name, tar_file, &status);
	if (status < 0 && 
		(s = Get_alternate_file_name (restore_archive_name)) != NULL)
	    success = Call_rss_copy (s, tar_file, &status);
    }

    if (success) {
	*error_buf = restore_archive_name;
	if (host_name != NULL && strcmp (host_name, "mscf") != 0)
	    LE_send_msg (GL_INFO, 
"Adaptation Data archive %s restored for installation on the next startup\n", 
				restore_archive_name);
    }
    return (success);
}

/***********************************************************************

    Installs ORPG adaptation data from an adaptation data archive. This 
    function will return an error if RPG is running when it is executed.

    dir(in) - directory where one or more adaptation archives exist.
	if NULL, the current working directory will be used.
    date(in) - date to install if NULL, today's date will be used.
    time(in) - time to install if NULL, the 23:59:59 will be used to 
	extract the latest time for the specified date.
    site(in) - name of the site to install if NULL, any site archive 
	will be selected for installation.
    alt_host_name(in) - alternate host name to use when searching for 
	archives, if NULL, any host name will be considered.
    error_buf(out) - on success, the name of the archive used for 
	installing is returned in this value, otherwise the error is 
	returned.
    Returns TRUE if successful, and FALSE otherwise.

***********************************************************************/

int ORPGADPTSV_install (const char *dir, const char *date, 
	const char* time, const char *site, const char *alt_host_name, 
	char **error_buf) {
    int success, status, ret_bytes, rpg_is_up;
    char *install_archive_name, *orpgdir;
    char *host_name, *site_name,*s;
    static char *temp_path = NULL, *temp_lb_name = NULL, *tar_command = NULL,
		*tar_file = NULL;
    char adapt_data_version[10], output_buf[250], *unique_string;

    success = 1;
    install_archive_name = NULL;
    set_error_buf (error_buf);
    Last_emsg[0] = '\0';

/*    ORPGADPTSV_init_log (); */
    create_log_directory();
    s = "orpgadptsv";
    LE_init (1, &s);
    LE_send_msg (GL_INFO, 
"ORPGADPTSV_install: dir %s, date %s, time %s, site %s, alt_host_name %s", 
		Parm_to_str (dir), Parm_to_str (date), Parm_to_str (time), 
		Parm_to_str (site), Parm_to_str (alt_host_name));
    RMT_time_out (600);	/* Set time out high for low bandwidth connection */

    status = 0;
    orpgdir = getenv ("ORPGDIR");
    output_buf[0] = '\0';
    rpg_is_up = 0;
    sprintf(adapt_data_version, "%05d", ORPGADPT_DATA_VERSION);

    if (orpgdir == NULL) {
	sprintf (Last_emsg, "ORPGADPTSV: ORPGDIR not defined\n");
	return (0);
    }
    if (dir == NULL) {
	sprintf (Last_emsg, "ORPGADPTSV: src path not specified\n");
	return (0);
    }

    /* search a matched file in dir */
    if (alt_host_name == NULL || strcmp (alt_host_name, "_localhost") == 0)
	host_name = ORPGMISC_get_site_name ("type");
    else
	host_name = (char *)alt_host_name;

    if (host_name != NULL && strcmp (host_name, "mscf") == 0) {
	sprintf (Last_emsg, 
		"ORPGADPTSV: MSCF does not have adapt data to install\n");
	return (success);
    }

    if (site == NULL || strlen (site) == 0)
	site_name = ORPGMISC_get_site_name ("site");	/* e.g. KTLX */
    else
	site_name = (char *)site;
    install_archive_name = ORPGADPT_get_archive_name (dir, date, time, 
						site_name, host_name);
    if (install_archive_name == NULL || strlen (install_archive_name) == 0) {
	sprintf (Last_emsg, "ORPGADPTSV: Archive (date: %s, time %s, site %s, node %s) not found in %s\n", 
		Parm_to_str (date), Parm_to_str (time), 
		Parm_to_str (site_name), Parm_to_str (host_name), dir);
	return (0);
    }

    /* Generate a unique string for tmp file name */
    unique_string = Get_unique_string ();

    if (host_name == NULL)
	host_name = "local_host";
    LE_send_msg (GL_INFO, "Installing adaptation data for host %s", 
						(const char *)host_name);

    Create_string (&temp_path, orpgdir, "/adapt/archive", unique_string, NULL);
    if (!clean_directory ((const char*)temp_path)) {
	sprintf (Last_emsg, 
			"ORPGADPTSV: clean_directory %s failed\n", temp_path);
	return (0);
    }

    /* Create the necessary directories */
    if (!ORPGADPT_make_directory ((const char*)temp_path)) {
	sprintf (Last_emsg, 
			"ORPGADPTSV: make directory %s failed\n", temp_path);
	return (0);
    }

    /* copy the archive file to the tmp dir */
    Create_string (&tar_file, temp_path, "/", Adapt_filename, 
		adapt_data_version, Adapt_filename_extension, ".Z", NULL);
    success = Call_rss_copy (install_archive_name, tar_file, &status);
    if (status < 0 && 
		(s = Get_alternate_file_name (install_archive_name)) != NULL)
	success = Call_rss_copy (s, tar_file, &status);
    if (success) {	/* untar the file in the tmp dir */
	/* extract_adapt - a script calling zcat and tar xf */
	Create_string (&tar_command, "extract_adapt -f ", tar_file, NULL);
	status = MISC_system_to_buffer (tar_command, output_buf, 
					sizeof (output_buf), &ret_bytes);
	if (status < 0) {
	    sprintf (Last_emsg, 
		"ORPGADPTSV: MISC_system_to_buffer %s failed\n", tar_command);
	    success = 0;
	}
    }

    if (success) {	/* install ASCII files */
	static char *temp_src_file = NULL, *temp_file_name = NULL;
	int j;

        for (j = 0; j < No_of_cfg_files; j++) {
	    Create_string (&temp_file_name, temp_path, "/", 
		Cfg_files[j].file_name, Adapt_filename_extension, NULL);
	    if (get_config_dir () == NULL) {
		success = 0;
		sprintf (Last_emsg, "ORPGADPTSV: CFG_DIR not found\n");
		break;
	    }
	    Create_string (&temp_src_file, get_config_dir (), 
					Cfg_files[j].file_name, NULL);
	    if ((strcmp (host_name, "mscf") != 0 &&
		     strcmp ("sys_cfg", Cfg_files[j].file_name) != 0 &&
		     strcmp ("site_info.dea", Cfg_files[j].file_name) != 0) ||
		(strcmp (host_name, "mscf") == 0 &&
		    (strcmp("sys_cfg", Cfg_files[j].file_name) == 0 ||
		     strcmp("site_info.dea", Cfg_files[j].file_name) == 0 ||
		     strcmp("algorithms", Cfg_files[j].file_name) == 0 ||
		     strcmp("prod_params", Cfg_files[j].file_name) == 0))) {
		success = Call_rss_copy (temp_file_name, temp_src_file, 
								&status);
		if (!success)
		    break;
	 	LE_send_msg (GL_INFO, 
			"Installed configuration file %s", temp_src_file);
	    }

	}
    }

    if (success && strcmp (host_name, "mscf") != 0) {	/* install LBs */
	int *data_ids, i;
	char *dest_dir_name;

	data_ids = build_data_id_list ();
	if (data_ids != NULL) {
	    for (i = 0; data_ids[i] != -1; i++) {
		Create_string (&temp_lb_name, temp_path, "/", 
			MISC_string_basename (
			    (char*)ORPGCFG_dataid_to_path (data_ids[i], NULL)),
			Adapt_filename_extension, NULL);

		/* Make sture the destination directory is created */
		dest_dir_name = MISC_string_dirname (
			(char*)ORPGCFG_dataid_to_path (data_ids[i], NULL));
		if (!ORPGADPT_make_directory (dest_dir_name)) {
		    sprintf (Last_emsg, 
			"ORPGADPTSV: make directory %s failed\n", 
							dest_dir_name);
		    success = 0;
		    break;
	   	}

		/* copy from the tmp LB to the ORPG data store */
		status = ORPGDA_copy_from_lb (data_ids[i], 
			(const char *)temp_lb_name, 
			ORPGDA_CREATE_DESTINATION | ORPGDA_CLEAR_DESTINATION);
                if (status < 0) {
		    success = 0;
		    sprintf (Last_emsg, 
			"ORPGADPTSV: ORPGDA_copy_from_lb (%d %s) failed\n", 
				data_ids[i], temp_lb_name);
                    break;
		}
		LE_send_msg (GL_INFO, "Installed adaptation file %s", 
			ORPGCFG_dataid_to_path (data_ids[i], NULL));
	    }
	    free(data_ids);
	}
    }

    clean_directory ((const char*)temp_path);
    if (success) {	/* Updating redundant adaptation date and time */
	*error_buf = install_archive_name;
	LE_send_msg (GL_INFO, "Adaptation Data installed from archive %s\n", 
			MISC_string_basename (install_archive_name));
	if (strcmp (host_name, "mscf") != 0 && 
		(ORPGSITE_get_int_prop (ORPGSITE_REDUNDANT_TYPE) 
						== ORPGSITE_FAA_REDUNDANT)) {
	    int status;
	    time_t archive_time;
	    LE_send_msg (GL_INFO, 
				"Updating redundant adaptation date and time");
	    archive_time = ORPGADPT_get_adapt_archive_time 
						(install_archive_name);
	    if (archive_time < 0)
		LE_send_msg (GL_ERROR, 
			"Error extracting archive date/time from archive %s",
						install_archive_name);
	    else {
		status = ORPGRED_update_adapt_dat_time (archive_time);
		if (status < 0)
		    LE_send_msg (GL_ERROR, 
"Error, could not update adaptation date/time for archive %s", 
						install_archive_name);
		else {
		    char update_time[50];
		    struct tm update_time_vals;
		    localtime_r (&archive_time, &update_time_vals);
		    strftime (update_time, sizeof (update_time), 
				"%d-%b-%Y,%H:%M:%S", &update_time_vals);
		    LE_send_msg (GL_INFO, 
			"Updated redundant adaptation date and time = %s", 
								update_time);
		}
	    }
	}
    }
    return (success);
}

/***********************************************************************

    Calls RSS_copy, checks error and sets error messages. Returns true
    on success of false on failure. "stp" returns status.

***********************************************************************/

static int Call_rss_copy (char *src_file, char *dest_file, int *stp) {
    int status, success;

    status = RSS_copy (src_file, dest_file);
    success = (status >= 0);
    *stp = status;
    if (!success) {
	if (status == -1)
	    sprintf (Last_emsg, 
"ORPGADPTSV: RSS_copy (%s %s) failed (file missing, invalid permissions or disk full)\n", 
					src_file, dest_file);
	else
	    sprintf (Last_emsg, "ORPGADPTSV: RSS_copy (%s %s) failed (%d)\n", 
					src_file, dest_file, status);
    }
    else
	LE_send_msg (GL_INFO, "Successfully copied %s to %s", 
						src_file, dest_file);
    return (success);
}

/***************************************************************************

    Returns the configuration file directory (CFG_DIR) path. The path
    always terminated by '/'. Returns NULL on failure.

***************************************************************************/

static char *get_config_dir () {
    static char *cfgdir = NULL;
    char *env_var;
    if (cfgdir != NULL)
	return (cfgdir);
    env_var = getenv ("CFG_DIR");
    if (env_var != NULL) {
	cfgdir = STR_copy (cfgdir, env_var);
	if (cfgdir[strlen (cfgdir) - 1] != '/')
	    cfgdir = STR_cat (cfgdir, "/");
    }
    return (cfgdir);
}

/***************************************************************************

    Initialize the log file for the ORPGADPTSV server.

***************************************************************************/

void ORPGADPTSV_init_log () {
    static int first_time = 1;
    int ret;

    char argv[] = "orpgadptsv"

    if (first_time) {
	create_log_directory();
   	ret = ORPGMISC_LE_init (1, &argv, 100, 0, -1, 0);
	if (ret < 0)
	    MISC_log ("ORPGADPTSV: ORPGMISC_init failed %d", ret);

	first_time = 0;
    }
}

/***************************************************************************

    Initializes this server - force libadapt.so to be dynamically loaded

***************************************************************************/

void ORPGADPTSV_init () {
    ORPGADPTSV_init_log();
}

/***************************************************************************

***************************************************************************/

static void set_error_buf (char **error_buf) {
     *error_buf = Last_emsg;
}

/***************************************************************************

***************************************************************************/

static void create_log_directory () {
    char *le_dir, *my_le_dir, *colon;

    le_dir = getenv ("LE_DIR_EVENT");
    if (le_dir == NULL) {
	MISC_log ("ORPGADPTSV: LE_DIR_EVENT not found\n");
	return;
    }

    my_le_dir = NULL;
    my_le_dir = STR_copy (my_le_dir, le_dir);
    colon = strstr (my_le_dir, ":");
    if (colon != NULL) {
	*colon = '\0';
	if (!ORPGADPT_make_directory (my_le_dir))
	    MISC_log ("ORPGADPTSV: make_directory %s failed\n", my_le_dir);
    }
    STR_free (my_le_dir);
}

/***************************************************************************

    Creates a string from a number of parts. "buf" must be a STR type of 
    pointer and the first part is copied. Other parts are appended. The
    arg list must be terminated by NULL.

***************************************************************************/

static void Create_string (char **buf, ...) {
    va_list args;
    char *p, *vb;
    int cnt;

    vb = *buf;
    va_start (args, buf);
    cnt = 0;
    while (1) {
	p = va_arg (args, char *);
	if (p == NULL)
	    break;
	if (cnt == 0)
	    vb = STR_copy (vb, p);
	else
	    vb = STR_cat (vb, p);
	cnt++;
    }
    *buf = vb;
    va_end (args);
}

/***************************************************************************

    Executes tar command to add a file to the adaptation data archive. 
    temp_path - path where the adaptation data to be archived exists.
    (in) src_file - name of the file to be added to the archive.
    (in) first_file - if true, this is the first file to be added to the 
    archive.
    Returns - TRUE if successful, FALSE otherwise.

***************************************************************************/

static int execute_tar_command (const char *tar_file, const char *temp_path, 
				const char *src_file, int first_file) {
    static char *tar_command = NULL;
    char *cmd;
    int success, status, ret_bytes;
    char output_buf[250];

    output_buf[0] = '\0';
    success = 1;
    if (first_file)
	cmd = "tar cvf ";
    else
	cmd = "tar rvf ";
    Create_string (&tar_command, cmd, tar_file, " -C ", temp_path, " ", 
				src_file, Adapt_filename_extension, NULL);

    status = MISC_system_to_buffer (tar_command, output_buf, 
				    sizeof (output_buf), &ret_bytes);
    if (status < 0) {
       sprintf (Last_emsg, "ORPGADPTSV: Executing %s failed (%d)\n", 
					    tar_command, status);
       success = 0;
    }
    return(success);
}

/***************************************************************************

    Gets the time tag for the current day and time. Returns a time string 
    for the current day and time. The returned string will be re-used on 
    the next call to this function.

***************************************************************************/

static char *get_time_tag () {
    static char buf[50];
    time_t curr_time;
    struct tm time_vals;
    int i;

    curr_time = time(NULL);
    localtime_r (&curr_time, &time_vals);
    for (i = 0; i < strlen (buf); i++) {
       if (buf[i] == ' ' || buf[i] == ':')
	    buf[i] = '_';
    }
    strftime (buf, sizeof (buf), "%d%b%Y-%H-%M-%S", &time_vals);
    return (buf);
}

/***************************************************************************

    Remove all temporary files from local directory "dir_path". Returns
    true on success or false on failure.

***************************************************************************/

static int clean_directory (const char *dir_path) {
    static char *rm_command = NULL;
    char output_buf[250];
    int ret_bytes, status;

    output_buf[0] = '\0';
    Create_string (&rm_command, "rm -rf ", dir_path, NULL);
    status = MISC_system_to_buffer (rm_command, output_buf, 
					sizeof (output_buf), &ret_bytes);
    if (status < 0 || ret_bytes != 0)
	return (0);
    return (1);
}

/***************************************************************************

    Function to build a list of ORPGDA data stores for saving/restoring.
    Returns an array of ORPG data store ids terminated by a -1 data store id,
    or NULL on failure. The caller needs to fee the returned pointer.

***************************************************************************/

static int *build_data_id_list () {
    int *list;

    /*  allocate the worst case */
    list = (int *)malloc (sizeof (int) * No_of_restore_files * 2);
    if (list != NULL) {
	int prev_data_id, j, i;

	prev_data_id = -1;
	j = 0;
	for (i = 0; i < No_of_restore_files; i++) {
	    if (Restore_files[i].src_data_id >= 0 && 
			Restore_files[i].src_data_id != prev_data_id) {
		prev_data_id = list[j] = Restore_files[i].src_data_id;
		j++;
	    }
	    if (Restore_files[i].dest_data_id >= 0 && 
			Restore_files[i].dest_data_id != prev_data_id) {
		prev_data_id = list[j] = Restore_files[i].dest_data_id;
		j++;
	    }
	}
	list[j] = -1;
    }
    return (list);
}

/***************************************************************************

    Replace "rpg" in file name "fname" by "rpg1" or vise versa. This will
    not be needed in the future.

***************************************************************************/

static char *Get_alternate_file_name (char *fname) {
    static char buf[256];
    char *cp, *tp;
    int len;

    cp = tp = fname;
    while (*tp != '\0') {
	if (*tp == '/')
	    cp = tp + 1;
	tp++;
    }
    cp = strstr (cp, "rpg");
    if (cp == NULL)
	return (NULL);
    len = cp - fname + 3;
    strncpy (buf, fname, len);
    if (cp[3] == '2')
	return (NULL);
    if (cp[3] == '1') {
	buf[len] = '\0';
	strcat (buf, cp + 4);
    }
    else {
	buf[len] = '1';
	buf[len + 1] = '\0';
	strcat (buf, cp + 3);
    }
    return (buf);
}

/***************************************************************************

    Returns "unknown" if "str" is NULL or empty, or "str" otherwise.

***************************************************************************/

static char *Parm_to_str (const char *str) {

    if (str == NULL || strlen (str) == 0)
	return ("unknown");
    return ((char *)str);
}

/***********************************************************************

    Generates a time based unique string for temporary directory name.

************************************************************************/

static char *Get_unique_string () {
    static char buf[64];
    struct timeval time_val;

    gettimeofday (&time_val, NULL);
    sprintf (buf, ".%ld.%ld.%ld", time_val.tv_sec, 
					time_val.tv_usec, getpid ());
    return (buf);
}

/***********************************************************************

    Runs "cksum" on file "file" and returns the check sum with "cksum".
    Returns 0 on success of -1 on failure. "file" may be on a remote host
    as indicated by the host name part of "file".

***********************************************************************/

static int Run_checksum (char *file, unsigned int *cksum) {
    char *cpt, *cmd, output_buf[128];
    int ret_bytes, status;

    cpt = file;
    while (*cpt != '\0' && *cpt != ':')
	cpt++;
    cmd = NULL;
    cmd = STR_copy (cmd, "cksum ");
    if (*cpt == ':') {		/* remote file */
	char *rpc_cmd;
	int ret;

	rpc_cmd = NULL;
	cmd = STR_cat (cmd, cpt + 1);
	rpc_cmd = STR_append (rpc_cmd, file, cpt - file + 1);
	rpc_cmd = STR_append (rpc_cmd, "MISC_system_to_buffer", 
				strlen ("MISC_system_to_buffer") + 1);
	ret = RSS_rpc (rpc_cmd, "i-r s-i ba-%d-o i-i ia-1-o", 128, 
			&status, cmd, output_buf, 128, &ret_bytes);
	STR_free (rpc_cmd);
	if (ret < 0)
	    return (-1);
    }
    else {			/* local file */
	cmd = STR_cat (cmd, file);
	status = MISC_system_to_buffer (cmd, output_buf, 128, &ret_bytes);
    }
    STR_free (cmd);
    if (status != 0 || ret_bytes <= 0 ||
	sscanf (output_buf, "%u", cksum) != 1)
	return (-1);
    return (0);
}
