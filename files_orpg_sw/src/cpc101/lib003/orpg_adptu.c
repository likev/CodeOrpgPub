
/******************************************************************

    The shared routines for RPG adaptation data utility programs..
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/08/09 15:36:10 $
 * $Id: orpg_adptu.c,v 1.18 2012/08/09 15:36:10 jing Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <string.h>

#include <orpg.h>
#include <orpgadpt.h>
#include <orpgsite.h>
#include <orpgred.h>
#include <infr.h> 

#define OUT_SIZE 256

static int N_cfg_files = 0;
static char *Cfg_files[] = {
    "alert_table"
};
static int N_bin_files = 2;
static int Bin_files[] = {
    ORPGDAT_USER_PROFILES,
    ORPGDAT_ADAPT_DATA
};
static char Adapt_filename_extension[] = ".adapt##";
static char *mon_name[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char *Out_buf = NULL;

static char *Get_full_path (char *dir, char *name);
static int Parse_file_name (char *name, int *verp, 
			char **sitep, char **nodep, time_t *tp);
static int Get_t_spec (char *date, char *tm, int *d_spec, time_t *t_spec);
static time_t Get_local_time ();
static char *Get_tmp_dir ();
static char *Get_config_dir ();
static void Cleanup_dir (char *tmp_dir);
static int Execute_cmd (char *cmd);
static int Copy_file (char *src, char *dest);
static int Make_dir (char *dir);


/************************************************************************

    Performs step 1 remote actions of the restore_adapt function. It 
    creates the cfg/adapt dir and returns the RPG site, config dir and 
    adapt version number. Return -1 on failure.

************************************************************************/

int ORPGADPTU_restore_1 (char *sitep, char *cfgp, int buf_size) {
    char *site;
    static char *dir = NULL;

    site = ORPGMISC_get_site_name ("site");
    if (site == NULL) {
	LE_send_msg (0, "ORPGMISC_get_site_name site failed");
	return (-1);
    }
    strncpy (sitep, site, buf_size);
    sitep[buf_size - 1] = '\0';
    if (strlen (Get_config_dir ()) <= 0) {
	LE_send_msg (0, "Config dir not defined");
	return (-1);
    }
    strncpy (cfgp, Get_config_dir (), buf_size);
    cfgp[buf_size - 1] = '\0';

    dir = STR_gen (dir, Get_config_dir (), "/adapt", NULL);
    if (Make_dir (dir) < 0)
	return (-1);

    return (ORPGMISC_RPG_adapt_version_number ());    
}

/************************************************************************

    Performs step 2 remote actions of the restore_adapt function. It cleans
    up the $CFG_DIR/adapt and rename the file. Return -1 on failure.

************************************************************************/

int ORPGADPTU_restore_2 (char *fname) {
    static char *cmd = NULL;

    cmd = STR_gen (cmd, "rm -f ", 
		Get_config_dir (), "/adapt/*.Z", NULL);
    Execute_cmd (cmd);
    cmd = STR_gen (cmd, "rm -f ", 
		Get_config_dir (), "/adapt/installed/*.Z", NULL);
    Execute_cmd (cmd);
    cmd = STR_gen (cmd, "cd ", Get_config_dir (), "/adapt; ", "mv -f ",
	fname, ".t ", fname, NULL);
    Execute_cmd (cmd);
    return (0);    
}

/************************************************************************

    Creates an archive of the adaptation data on the local host and saves
    it in local dir "cfg/adapt/installed".
    All old archive in "cfg/adapt/installed" are removed. Returns the full
    path of the adapt archive file. Returns NULL on failure.

************************************************************************/

char *ORPGADPTU_save_adapt () {
    char *tmp_dir, *a_name;
    static char *src = NULL, *dest = NULL, *cmd = NULL;
    int i;

    tmp_dir = Get_tmp_dir ();		/* get a tmp work dir */
    if (tmp_dir == NULL ||
	Make_dir (tmp_dir) < 0)
	return (NULL);
    cmd = STR_gen (cmd, "rm -f ", tmp_dir, "/*", NULL);
    Execute_cmd (cmd);
    LE_send_msg (0, "tmp_dir is %s", tmp_dir);

    for (i = 0; i < N_cfg_files; i++) {	/* copy cfg files to tmp_dir */
	dest = STR_gen (dest, tmp_dir, "/", 
			Cfg_files[i], Adapt_filename_extension, NULL);
	src = STR_gen (src, Get_config_dir (), "/", Cfg_files[i], NULL);
	if (Copy_file (src, dest) < 0)	/* copy the file to tmp_dir */
	    goto failed;
	LE_send_msg (0, "File %s copied to tmp_dir", Cfg_files[i]);
    }

    for (i = 0; i < N_bin_files; i++) {	/* copy binary files to tmp_dir */
	char b[256];
	char *name = ORPGDA_lbname (Bin_files[i]);
	if (name == NULL) {
	    LE_send_msg (0, "ORPGDA_lbname %d failed", Bin_files[i]);
	    goto failed;
	}
	name = MISC_expand_env (name, b, 256);
	dest = STR_gen (dest, tmp_dir, "/", 
			MISC_basename (name), Adapt_filename_extension, NULL);
	if (Copy_file (name, dest) < 0)	/* copy the file to tmp_dir */
	    goto failed;
	LE_send_msg (0, "File %s copied to tmp_dir", name);
    }

    a_name = ORPGADPTU_create_archive_name (NULL, NULL, -1, 0);
    if (a_name == NULL)
	return (NULL);
    cmd = STR_gen (cmd, "cd ", tmp_dir, "; tar cf ", a_name, 
					" *; compress adapt0*", NULL);
    if (Execute_cmd (cmd) < 0)
	goto failed;
    LE_send_msg (0, "File %s packed", a_name);

    /* clean up adapt/installed and copy the new archive to it. */
    cmd = STR_gen (cmd, "rm -f ", Get_config_dir (), 
					"/adapt/installed/*.Z", NULL);
    Execute_cmd (cmd);
    src = STR_gen (src, tmp_dir, "/", a_name, ".Z", NULL);
    dest = STR_gen (dest, Get_config_dir (), "/adapt/installed", NULL);
    if (Make_dir (dest) < 0)
	goto failed;
    dest = STR_gen (dest, Get_config_dir (), 
				"/adapt/installed/", a_name, ".Z", NULL);
    if (Copy_file (src, dest) < 0)
	goto failed;
    LE_send_msg (0, "File %s copied to %s", src, dest);

    Cleanup_dir (tmp_dir);
    LE_send_msg (0, "save_adapt (%s) completed", dest);

    return (dest);

failed:
    Cleanup_dir (tmp_dir);
    return (NULL);
}

/************************************************************************

    Returns the dir name for install files for the specified node, site
    and version. "node" must be "rpg?" or "mscf". If "chan" is not either 
    1 or 2, no channel number is assumed.

************************************************************************/

char *ORPGADPTU_install_dir (char *node, char *site, int chan, int ver) {
    static char *fname = NULL;
    char v[32], c[32];

    if (node == NULL || strlen (node) == 0)
	node = ORPGMISC_get_site_name ("type");
    if (chan < 0) {
	char *ch = ORPGMISC_get_site_name ("channel_num");
	if (ch != NULL && node != NULL && strncmp (node, "rpg", 3) == 0)
	    sscanf (ch, "%d", &chan);
    }
    if (site == NULL)
	site = ORPGMISC_get_site_name ("site");
    if (ver == 0)
	ver = ORPGMISC_RPG_adapt_version_number ();
    sprintf (v, "%d0", ver / 10);
    if (chan <= 0 || chan > 2 || (node != NULL && strcmp (node, "mscf") == 0))
	c[0] = '\0';
    else
	sprintf (c, "%d", chan);
    if (node == NULL || site == NULL) {
	LE_send_msg (0, "ORPGMISC_get_site_name - site info not found");
	return (NULL);
    }
    fname = STR_gen (fname, "install0", v, ".", site, ".", node, c, NULL);
    return (fname);
}

/************************************************************************

    Copies file from "src" to "dest". Returns 0 on success or -1 on failure.

************************************************************************/

static int Copy_file (char *src, char *dest) {
    int ret;
    if ((ret = RSS_copy (src, dest)) < 0) {
	LE_send_msg (0, "RSS_copy from %s to %s failed (%d)", src, dest, ret);
	return (-1);
    }
    return (0);
}

/************************************************************************

    Makes directory "dir". Returns 0 on success or -1 on failure.

************************************************************************/

static int Make_dir (char *dir) {
    int ret;
    char *p;

    if ((p = strstr (dir, ":")) != NULL)
	dir = p + 1;
    if ((ret = MISC_mkdir (dir)) < 0) {
	LE_send_msg (0, "Cannot make DIR %s (%d)", dir, ret);
	return (-1);
    }
    return (0);
}

/************************************************************************

    Finds the requested RPG adaptation archive file in dir "src_dir" 
    assording to specified data "s_date" and time "s_time", unpacks it and 
    installs the unpacked files in the RPG environment. If need_file_move
    is non-zero, the archive files are moved to installed and not_installed 
    sub dirs. Returns 0 on success or -1 on failure.

************************************************************************/

int ORPGADPTU_install_adapt (char *src_dir, char *s_date, 
					char *s_time, int need_file_move) {
    char *afname, *node, *site, *tmp_dir, dir_buf[256];
    static char *src = NULL, *dest = NULL, *cmd = NULL, *adapt_files = NULL;
    int i, n_adapt_files;

    node = ORPGMISC_get_site_name ("type");
    if (node != NULL && strncmp (node, "rpg", 3) == 0) {
	char *ch = ORPGMISC_get_site_name ("channel_num");
	if (strcmp (ch, "2") == 0)
	    node = "rpg2";
	else
	    node = "rpg1";
    }
    site = ORPGMISC_get_site_name ("site");
    if (node == NULL || site == NULL) {
	LE_send_msg (0, "ORPGMISC_get_site_name - site info not found");
	return (-1);
    }
    LE_send_msg (0, "Installing adapt (node %s, site %s) from %s ...", 
						node, site, src_dir);
    afname = ORPGADPTU_find_archive_name (src_dir, s_date, 
		s_time, site, node, -1, &n_adapt_files, &adapt_files);
    if (afname == NULL)
	return (-1);

    tmp_dir = Get_tmp_dir ();		/* get a tmp work dir */
    if (tmp_dir == NULL ||
	Make_dir (tmp_dir) < 0)
	return (-1);
    dest = STR_gen (dest, tmp_dir, "/", MISC_basename (afname), NULL);
    if (Copy_file (afname, dest) < 0)	/* copy the file to the tmp dir */
	goto failed;
    LE_send_msg (0, "File %s copied", afname);
    LE_send_msg (0, "    to %s", tmp_dir);

    cmd = STR_gen (cmd, "cd ", tmp_dir, "; zcat ", dest, " | tar xf -", NULL);
    if (Execute_cmd (cmd) < 0)
	goto failed;
    LE_send_msg (0, "File %s unpacked", MISC_basename (afname));

    for (i = 0; i < N_cfg_files; i++) {	/* install cfg files */
	src = STR_gen (src, tmp_dir, "/", 
			Cfg_files[i], Adapt_filename_extension, NULL);
	dest = STR_gen (dest, Get_config_dir (), "/", Cfg_files[i], NULL);
	if (Make_dir (MISC_dirname (dest, dir_buf, 256)) < 0)
	    goto failed;
	if (Copy_file (src, dest) < 0)	/* copy the file to cfg */
	    goto failed;
	LE_send_msg (0, "File %s installed in %s", 
				Cfg_files[i], Get_config_dir ());
    }

    for (i = 0; i < N_bin_files; i++) {	/* install binary files */
	char b[256];
	char *name = ORPGDA_lbname (Bin_files[i]);
	if (name == NULL) {
	    LE_send_msg (0, "ORPGDA_lbname %d failed", Bin_files[i]);
	    goto failed;
	}
	name = MISC_expand_env (name, b, 256);
	src = STR_gen (src, tmp_dir, "/", 
			MISC_basename (name), Adapt_filename_extension, NULL);
	if (Make_dir (MISC_dirname (name, dir_buf, 256)) < 0)
	    goto failed;
	LB_remove (name);
	if (Copy_file (src, name) < 0)	/* copy the file to cfg */
	    goto failed;
	LE_send_msg (0, "File %s installed", name);
    }
    printf ("Adapt files installed in RPG\n");
    Cleanup_dir (tmp_dir);

    /* Updating redundant adaptation date and time */
    if (ORPGSITE_get_int_prop (ORPGSITE_REDUNDANT_TYPE) 
						== ORPGSITE_FAA_REDUNDANT) {
	time_t a_t;
	int ret;
	LE_send_msg (GL_INFO, "Updating redundant adaptation date and time");
	if (Parse_file_name (MISC_basename (afname), 
						NULL, NULL, NULL, &a_t) < 0)
	    LE_send_msg (GL_ERROR, "Parse_file_name %s failed", 
						MISC_basename (afname));
	else if ((ret = ORPGRED_update_adapt_dat_time (a_t)) < 0)
	    LE_send_msg (GL_ERROR, 
			"ORPGRED_update_adapt_dat_time failed (%d)", ret);
    }

    if (need_file_move) {	/* move archive files to installed/not_i... */
	char *d, *ins, *nins, *file;
	char *dir = MISC_dirname (afname, dir_buf, 256);

	LE_send_msg (0, 
	    "Moving adapt archive files to installed and not_installed");
	ins = nins = NULL;
	ins = STR_gen (ins, dir, "/installed", NULL);
	nins = STR_gen (nins, dir, "/not_installed", NULL);
	cmd = STR_gen (cmd, "rm -f ", ins, "/*.Z", NULL);
	Execute_cmd (cmd);
	Make_dir (ins);
	Make_dir (nins);
	file = adapt_files;
	for (i = 0; i < n_adapt_files; i++) {
	    if (strcmp (file, MISC_basename (afname)) == 0)
		d = ins;
	    else
		d = nins;
	    cmd = STR_gen (cmd, "mv -f ", dir, "/", file, " ", d, NULL);
	    Execute_cmd (cmd);		/* move the file */
	    file += strlen (file) + 1 ;
	}
    }

    LE_send_msg (0, "RPG adaptation data installation completed");
    return (0);

failed:
    Cleanup_dir (tmp_dir);
    return (-1);
}

/***************************************************************************

    Exucutes command "cmd". Returns 0 on success of -1 on failure.

***************************************************************************/

static int Execute_cmd (char *cmd) {
    static char *shcmd = NULL;
    int ret;

    if (Out_buf == NULL)
	Out_buf = MISC_malloc (OUT_SIZE);
    shcmd = STR_copy (shcmd, "sh -c \"");
    shcmd = STR_cat (shcmd, cmd);
    shcmd = STR_cat (shcmd, "\"");
    if ((ret = MISC_system_to_buffer (shcmd, Out_buf, OUT_SIZE, NULL)) != 0) {
	LE_send_msg (0, "%d returned - %s", ret, cmd);
	return (-1);
    }
    return (0);
}

/***************************************************************************

    Cleans up "tmp_dir".

***************************************************************************/

static void Cleanup_dir (char *tmp_dir) {
    static char *cmd = NULL;

    cmd = STR_gen (cmd, "rm -rf ", tmp_dir, NULL);
    if (Execute_cmd (cmd) == 0)		/* cleanup tmp dir */
	LE_send_msg (0, "DIR %s removed", tmp_dir);
}

/***************************************************************************

    Returns the pointer to the CFG dir.

***************************************************************************/

static char *Get_config_dir () {
    char buf[256];
    static char *dir = NULL;

    if (MISC_get_cfg_dir (buf, 256) < 0) {
	LE_send_msg (0, "MISC_get_cfg_dir failed");
	return ("");
    }
    dir = STR_copy (dir, buf);
    return (dir);
}

/***************************************************************************

    Returns a unique tmp directory name.

***************************************************************************/

static char *Get_tmp_dir () {
    static char *dir = NULL;
    char buf[256];

    if (MISC_get_tmp_path (buf, 256) <= 0) {
	LE_send_msg (0, "MISC_get_tmp_path failed");
	return (NULL);
    }
    dir = STR_copy (dir, buf);
    return (dir);
}

/************************************************************************

    Searches in directory "src_dir" to find the RPG adaptation data file
    which name matches "date", "time_str" "site_name", "node_name" and
    RPG adapt version number "ver". The time spec is the local time.
    "src_dir", must be local. NULL or empty string is accepted and
    assumed to be unspecified for any of the string input arguments. A
    -1 "ver" is assumed to be the local RPG adapt version number. Other
    negative "ver" is assumed to be "any" version. Returns NULL if no
    file is found or the file name on success. "site_name" and
    "node_name" are case insensitive. If "n_files" is not NULL, the
    names of all adapt archive files are returned with "files" and file
    number is returned with "n_files". "node_name" must be "rpg", "rpg1",
    "rpg2" or "mscf".

************************************************************************/

char *ORPGADPTU_find_archive_name (char *src_dir, char *date, char *time_str, 
      char *site_name, char *node_name, int ver, int *n_files, char **files) {
    static char name[ADPTU_NAME_SIZE];
    DIR* dp;
    static struct dirent *dir;
    char *site, *node;
    time_t tm, t_spec;
    int v, diff, min, d_spec;

    if (Get_t_spec (date, time_str, &d_spec, &t_spec) < 0) {
	LE_send_msg (0, "Bad date (%s) or time (%s) spec\n", date, time_str);
	return (NULL);
    }

    LE_send_msg (0, "Finding adapt archive in %s ...\n", src_dir);
    name[0] = '\0';
    dp = opendir (src_dir);
    if (dp == NULL) {
	LE_send_msg (0, "opendir (%s) failed", src_dir);
	return (NULL);
    }

    if (ver == -1)
	ver = ORPGMISC_RPG_adapt_version_number () / 10;
    if (n_files != NULL) {
	*n_files = 0;
	*files = STR_reset (*files, 0);
    }
    min = 0x7fffffff;
    while ((dir = readdir (dp)) != NULL) {
	struct stat st;

	if (stat (Get_full_path (src_dir, dir->d_name), &st) >= 0 &&
	    !(st.st_mode & S_IFREG))
	    continue;
	if (Parse_file_name (dir->d_name, &v, &site, &node, &tm) < 0) {
	    if (strcmp (dir->d_name, "adapt0") == 0)
		LE_send_msg (0, 
		"    File %s is not an RPG adapt archive\n", dir->d_name);
	    continue;
	}
	if (n_files != NULL) {
	    *n_files = *n_files + 1;
	    *files = STR_append (*files, 
				dir->d_name, strlen (dir->d_name) + 1);
	}

	if (ver >= 0 && v != ver && (v / 10) != ver) {
	    LE_send_msg (0, "    File %s not accepted: version %d (RPG %d)\n",
		dir->d_name, v, ver);
	    continue;
	}
	if (site_name != NULL && strlen (site_name) > 0 &&
	    strcasecmp (site, site_name) != 0) {
	    LE_send_msg (0, "    File %s not accepted: site %s (ask %s)\n",
				    dir->d_name, site, site_name);
	    continue;
	}
	if (node_name != NULL && strlen (node_name) > 0) {
	    char *n1, *n2;
	    n1 = node_name;
	    if (strcasecmp (node_name, "rpg") == 0)
		n1 = "rpg1";
	    n2 = node;
	    if (strcasecmp (node, "rpg") == 0)
		n2 = "rpg1";
	    if (strcasecmp (n1, n2) != 0) {
		LE_send_msg (0,"    File %s not accepted: node %s (ask %s)\n",
				    dir->d_name, node, node_name);
		continue;
	    }
	}
	if (d_spec) {		/* matching date */
	    int d, t;
	    d = tm / 86400;
	    t = tm % 86400;
	    if (d_spec != d) {
	        LE_send_msg (0, 
			"    File %s not accepted - date %d (ask %d)\n", 
				    dir->d_name, d, d_spec);
		continue;
	    }
	    diff = 86400 - t;
	}
	else {			/* matching time */
	    diff = t_spec - tm;
	    if (diff < 0)
		diff = -diff;
	}
	if (diff < min) {
	    min = diff;
	    strncpy (name, Get_full_path (src_dir, dir->d_name), 
					    ADPTU_NAME_SIZE);
	    name[ADPTU_NAME_SIZE - 1] = '\0';
	}
    }
    closedir (dp);
    if (strlen (name) == 0) {
	LE_send_msg (0, "No matching adapt file name found in (%s)", src_dir);
	return (NULL);
    }
    else
	LE_send_msg (0, "Best match found: %s", name);
    return (name);
}

/************************************************************************

    Parses time specification string "date" and "tm". If only "date" is
    specified, *d_spec is set to date and *t_spec is set to 0. Otherwise
    *d_spec is set to 0 and *t_spec is set to the time of "date" and "tm".
    If date is missing, the current date is assumed. If both are missing,
    the current time is assumed. Returns 0 on success or -1 on failure. 
    Local time is used here.

************************************************************************/

static int Get_t_spec (char *date, char *tm, int *d_spec, time_t *t_spec) {
    time_t cr_t;
    int d, mon, y, h, m, s;

    *d_spec = 0;
    cr_t = Get_local_time ();
    unix_time (&cr_t, &y, &mon, &d, &h, &m, &s);
    if (date != NULL && strlen (date) > 0) {
	if (sscanf (date, "%d%*c%d%*c%d", &mon, &d, &y) != 3)
	    return (-1);
	    if (y < 40)
		y += 2000;
	    else if (y < 100)
		y += 1900;
	if (tm == NULL || strlen (tm) == 0) {
	    cr_t = 0;
	    h = m = s = 0;
	    unix_time (&cr_t, &y, &mon, &d, &h, &m, &s);
	    *d_spec = cr_t / 86400;
	    *t_spec = 0;
	    return (0);
	}
    }
    if (tm != NULL && strlen (tm) > 0) {
	if (sscanf (tm, "%d%*c%d%*c%d", &h, &m, &s) != 3)
	    return (-1);
    }
    cr_t = 0;
    unix_time (&cr_t, &y, &mon, &d, &h, &m, &s);
    *t_spec = cr_t;
    return (0);
}

/************************************************************************

    Returns the current local time.

************************************************************************/

static time_t Get_local_time () {
    struct tm *lt;
    int y, mon, d, h, m, s;
    time_t t;

    t = time (NULL);
    lt = localtime (&t);
    y = lt->tm_year + 1900;
    mon = lt->tm_mon + 1;
    d = lt->tm_mday;
    h = lt->tm_hour;
    m = lt->tm_min;
    s = lt->tm_sec;
    t = 0;
    unix_time (&t, &y, &mon, &d, &h, &m, &s);
    return (t);
}

/************************************************************************

    Parses RPG adapt archive files name "name" and returns version,
    site, node and time with the respective arguments. Returns 0 on
    success or -1 on failure.

************************************************************************/

static int Parse_file_name (char *name, int *verp, 
				char **sitep, char **nodep, time_t *tp) {
    static char site[8], node[8], *p, *pe;
    time_t t;
    int ver, d, mon, y, h, m, s;
    char c;

    p = strstr (name, "adapt0");
    if (p == NULL)
	return (-1);
    p += 6;
    pe = p;
    while (*pe != '.' && *pe != '\0')
	pe++;
    if (*pe != '.' || pe - p >= 8)
	return (-1);
    *pe = '\0';
    if (sscanf (p, "%d%c", &ver, &c) != 1)
	return (-1);
    *pe = '.';
    p = pe + 1;
    pe = p;
    while (*pe != '.' && *pe != '\0')
	pe++;
    if (*pe != '.' || pe - p >= 8)
	return (-1);
    strncpy (site, p, pe - p);
    site[pe - p] = '\0';
    p = pe + 1;
    pe = p;
    while (*pe != '.' && *pe != '\0')
	pe++;
    if (*pe != '.' || pe - p >= 8)
	return (-1);
    strncpy (node, p, pe - p);
    node[pe - p] = '\0';
    p = pe + 1;
    if (strlen (p) < 18 ||
	sscanf (p, "%d", &d) != 1)
	return (-1);
    p += 2;
    for (mon = 0; mon < 12; mon++) {
	if (strncmp (p, mon_name[mon], 3) == 0)
	    break;
    }
    if (mon >= 12)
	return (-1);
    mon++;
    p += 3;
    if (sscanf (p, "%d", &y) != 1)
	return (-1);
    p += 5;
    if (sscanf (p, "%d", &h) != 1 ||
	sscanf (p + 3, "%d", &m) != 1 ||
	sscanf (p + 6, "%d", &s) != 1 ||
	h < 0 ||
	m < 0 ||
	s < 0)
	return (-1);
    t = 0;
    unix_time (&t, &y, &mon, &d, &h, &m, &s);
    if (verp != NULL)
	*verp = ver;
    if (sitep != NULL)
	*sitep = site;
    if (nodep != NULL)
	*nodep = node;
    if (tp != NULL)
	*tp = t;
    return (0);
}

/************************************************************************

    Returns the full path of file name "name".

************************************************************************/

static char *Get_full_path (char *dir, char *name) {
    static char *fname = NULL;

    fname = STR_copy (fname, dir);
    fname = STR_cat (fname, "/");
    fname = STR_cat (fname, name);
    return (fname);
}

/************************************************************************

    Creates and returns the RPG adapt archive file name based on "node",
    "site", "ver" and "tm". Returns NULL on failure. "node" must be 
    "rpg1", "rpg2" or "mscf".

************************************************************************/

char *ORPGADPTU_create_archive_name (char *node, 
				char *site, int ver, time_t tm) {
    static char buf[128];
    int d, mon, y, h, m, s;

    if (node == NULL || node[0] == '\0') {
	node = ORPGMISC_get_site_name ("type");
	if (node != NULL && strncmp (node, "rpg", 3) == 0) {
	    char *ch = ORPGMISC_get_site_name ("channel_num");
	    if (strcmp (ch, "2") == 0)
		node = "rpg2";
	    else
		node = "rpg1";
	}
    }
    if (site == NULL || site[0] == '\0')
	site = ORPGMISC_get_site_name ("site");
    if (ver < 0)
	ver = ORPGMISC_RPG_adapt_version_number ();
    if (node == NULL || site == NULL) {
	LE_send_msg (0, "ORPGMISC_get_site_name - site info not found");
	return (NULL);
    }
    if (tm == 0)
	tm = Get_local_time ();
    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
    sprintf (buf, "adapt0%.4d.%s.%s.%.2d%s%.4d-%.2d-%.2d-%.2d", 
			ver, site, node, d, mon_name[mon - 1], y, h, m, s);
    return (buf);
}
