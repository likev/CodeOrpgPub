
/******************************************************************

    This file contains routines that reads site info in a adapt
    master dir.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/03/08 20:15:38 $
 * $Id: ma_read_files.c,v 1.6 2007/03/08 20:15:38 jing Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <infr.h> 
#define NEED_SITE_STRING_CONST
#define NEED_COMMS_STRING_CONST
#include "manage_adapt.h"

#define MAX_SEARCH_DEPTH 8
#define TMP_BUF_SIZE 128

static int Search_depth = 0;	/* Current directory search depth */

static site_info_t *Sites;
static int Site_size = 0;
static int N_sites = 0;

static User_t *Users = NULL;
static int N_users = 0;

static void Search_site_files (char *d_name);
static char *Get_full_path (char *dir, char *name);
static void Process_a_file_name (char *dir, char *name);
static void Verify_redundant_channel (site_info_t *sites, int n_sites);
static int Compare_sites (const void *s1p, const void *s2p);
static int Compare_users (const void *u1p, const void *u2p);
static int Read_site_info (char *dir, char *name);
static void Read_user_profile (char *dir, site_info_t *site);
static void Process_user_ids (char *key, char *fname, char *site, char *node);
static void Print_all_users ();


/******************************************************************

    Read site info in a site adapt master dir "d_name". The site
    info are returned with "sites" of buffer size "buf_size". It
    returns the number of sites found. It exits on failure.

******************************************************************/

int RF_read_site_master_dir (char *d_name, site_info_t *sites, int buf_size) {

    Sites = sites;
    Site_size = buf_size;
    N_sites = 0;
    N_users = 0;
    Search_site_files (d_name);
    RF_sort_verify_sites (Sites, N_sites);

    if (MAIN_read_user_ids ())
	Print_all_users ();

    return (N_sites);
}

/******************************************************************

    Sorts and verifies the site table.

******************************************************************/

int RF_sort_verify_sites (site_info_t *sites, int n_sites) {
    qsort (sites, n_sites, sizeof (site_info_t), Compare_sites);
    Verify_redundant_channel (sites, n_sites);
    return (0);
}

/******************************************************************

    Searches the site file in directory "dir_name" to get all 
    site file names.

******************************************************************/

static void Search_site_files (char *d_name) {
    DIR *dir;
    struct dirent *dp;

    Search_depth++;
    if (Search_depth > MAX_SEARCH_DEPTH)
	return;

    dir = opendir (d_name);
    if (dir == NULL) {
	fprintf (stderr, "opendir (%s) failed, errno %d\n", d_name, errno);
	return;
    }

    while ((dp = readdir (dir)) != NULL) {
	int ret, file_size;
	struct stat st;

	if (strcmp (dp->d_name, ".") == 0 || strcmp (dp->d_name, "..") == 0)
	    continue;

	ret = stat (Get_full_path (d_name, dp->d_name), &st);
	if (ret < 0) 
	    continue;
	file_size = st.st_size;

	if (S_ISDIR (st.st_mode)) {	/* a directory */
	    char full_name[LOCAL_NAME_SIZE + 4];
	    strcpy (full_name, Get_full_path (d_name, dp->d_name));
	    strcat (full_name, "/");
	    Search_site_files (full_name);
	}
	else if (S_ISREG (st.st_mode))
	    Process_a_file_name (d_name, dp->d_name);
    }
    closedir (dir);
    Search_depth--;
    return;
}

/************************************************************************

    Returns the full path of directory "dir" and file name "name".

************************************************************************/

static char *Get_full_path (char *dir, char *name) {
    static char namebuf[LOCAL_NAME_SIZE] = "";

    if (strlen (dir) + strlen (name) + 1 >= LOCAL_NAME_SIZE) {
	fprintf (stderr, "Full path %s%s too long\n", dir, name);
	exit (1);
    }
    if (dir[strlen (dir) - 1] == '/')
	sprintf (namebuf, "%s%s", dir, name);
    else
	sprintf (namebuf, "%s/%s", dir, name);
    return (namebuf);
}

/************************************************************************

    Checks if file "name" in directory "dir" is a site file. It reads
    all adapt files in the dir if it is.

************************************************************************/

static void Process_a_file_name (char *dir, char *name) {

    if (strstr (dir, "/rpg") == NULL)
	return;
    if (strcmp (name, "site_info.dea") == 0) {
	if (Read_site_info (dir, name) < 0)
	    return;
	if (RF_read_comms_link_conf (dir, Sites + N_sites - 1) != 0)
	    Read_user_profile (dir, Sites + N_sites - 1);
    }
    return;
}

/************************************************************************

    Read site_info.dea and sets the current site data struct. Return 0 
    on success or -1 on failure.

************************************************************************/

static int Read_site_info (char *dir, char *name) {
    char *full_name, *cpt, buf[TMP_BUF_SIZE];
    site_info_t site;
    int len, i;

    full_name = Get_full_path (dir, name);
    memset (&site, 0, sizeof (site_info_t));

    if (strstr (full_name, "roc") != NULL)
	strcpy (site.cat, "R");
    else if (strstr (full_name, "generic") != NULL)
	strcpy (site.cat, "G");
    else
	strcpy (site.cat, "A");

    CS_cfg_name (full_name);
    CS_control (CS_COMMENT | '#');
    if (CS_entry ("site_info.rpg_name:", 1, SHORT_NAME_SIZE, site.name) <= 0) {
	fprintf (stderr, "site_info.rpg_name not found at %s\n", full_name);
	exit (1);
    }
    if (strstr (full_name, site.name) == NULL) {
	fprintf (stderr, "site name (%s) does not match file name %s\n", 
					site.name, full_name);
	exit (1);
    }
    if (! MAIN_is_site_selected (&site))
	return (-1);

    if (CS_entry ("site_info.rda_lat:", 1 | CS_INT, 0, 
						(char *)&(site.lat)) <= 0 ||
	CS_entry ("site_info.rda_lon:", 1 | CS_INT, 0, 
						(char *)&(site.lon)) <= 0 ||
	CS_entry ("site_info.rda_elev:", 1 | CS_INT, 0, 
						(char *)&(site.elev)) <= 0) {
	fprintf (stderr, "site location not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("site_info.rpg_id:", 1 | CS_INT, 0, 
						(char *)&(site.rpg_id)) <= 0) {
	fprintf (stderr, "site rpg_id not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("site_info.wx_mode:", CS_ALL_TOKENS | 1, 
						TMP_BUF_SIZE, buf) <= 0 ||
	(site.wx_mode = RF_get_enum (Wx_mode, buf)) < 0) {
	fprintf (stderr, "site wx_mode not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("site_info.def_mode_A_vcp:", 1 | CS_INT, 0, 
						(char *)&(site.vcp_a)) <= 0 ||
	CS_entry ("site_info.def_mode_B_vcp:", 1 | CS_INT, 0, 
						(char *)&(site.vcp_b)) <= 0) {
	fprintf (stderr, "site VCP not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("site_info.has_mlos:", 1, TMP_BUF_SIZE, buf) <= 0 ||
	(site.mlos = RF_get_enum (Yes_no, buf)) < 0) {
	fprintf (stderr, "site has_mlos not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("site_info.has_rms:", 1, TMP_BUF_SIZE, buf) <= 0 ||
	(site.rms = RF_get_enum (Yes_no, buf)) < 0) {
	fprintf (stderr, "site has_rms not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("site_info.has_bdds:", 1, TMP_BUF_SIZE, buf) <= 0 ||
	(site.bdds = RF_get_enum (Yes_no, buf)) < 0) {
	fprintf (stderr, "site has_bdds not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("site_info.has_archive_III:", 1, TMP_BUF_SIZE, buf) <= 0 ||
	(site.a_3 = RF_get_enum (Yes_no, buf)) < 0) {
	fprintf (stderr, "site has_archive_III not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("site_info.is_orda:", 1, TMP_BUF_SIZE, buf) > 0) {
	if ((site.orda = RF_get_enum (Yes_no, buf)) < 0) {
	    fprintf (stderr, "site orda not found at %s\n", full_name);
	    exit (1);
	}
    }
    else
	site.orda = RF_get_enum (Yes_no, "No");

    if (CS_entry ("site_info.product_code:", 1 | CS_INT, 0, 
					(char *)&(site.p_code)) <= 0) {
	fprintf (stderr, "site product_code not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("Redundant_info.redundant_type:", CS_ALL_TOKENS | 1, 
						TMP_BUF_SIZE, buf) <= 0 ||
	(site.redun_t = RF_get_enum (Redun_t, buf)) < 0) {
	fprintf (stderr, "site redundant_type not found at %s\n", full_name);
	exit (1);
    }
    if (CS_entry ("Redundant_info.channel_number:", CS_ALL_TOKENS | 1, 
						TMP_BUF_SIZE, buf) <= 0 ||
	(site.channel = RF_get_enum (Channel, buf)) < 0) {
	fprintf (stderr, "site channel_number not found at %s\n", full_name);
	exit (1);
    }
    if (site.channel == 0)
	strcpy (site.node, "RPG1");
    else
	strcpy (site.node, "RPG2");
    if (CS_entry ("mlos_info.no_of_mlos_stations:", 1 | CS_INT, 
				0, (char *)&(site.n_mlos)) <= 0) {
	fprintf (stderr, "no_of_mlos_stations not found at %s\n", full_name);
	exit (1);
    }
    buf[0] = '\0';
    len = CS_entry ("mlos_info.station_type:", CS_ALL_TOKENS + 1, 
						TMP_BUF_SIZE, buf);
    cpt = buf;
    for (i = 0; i < N_MLOS; i++) {
	char *p, *next;
	if (cpt - buf >= len) {
	    fprintf (stderr, "site mlos_type too short at %s\n", full_name);
	    exit (1);
	}
	while (*cpt == ' ' || *cpt == '\t')
	    cpt++;
	p = cpt;
	while (*p != '\0') {
	    if (*p == ',') {
		*p = '\0';
		break;
	    }
	    p++;
	}
	next = p + 1;
	p--;
	while (p >= cpt && (*p == ' ' || *p == '\t'))
	    *p-- = '\0';
	if ((site.mlos_t[i] = RF_get_enum (Mlos_t, cpt)) < 0) {
	    fprintf (stderr, "site mlos_type not found at %s\n", full_name);
	    exit (1);
	}
	cpt = next;
    }
    CS_control (CS_CLOSE);

    full_name = Get_full_path (dir, "hci_passwords");
    CS_cfg_name (full_name);
    CS_control (CS_COMMENT | '#');
    CS_control (CS_RESET);
    if (CS_entry ("Passwords", 0, TMP_BUF_SIZE, buf) <= 0 ||
	CS_level (CS_DOWN_LEVEL) != 0) {
	fprintf (stderr, "password does not found in dir %s\n", dir);
	exit (1);
    }
    if (CS_entry ("roc_password", 1, PSWD_SIZE, site.roc_pwd) <= 0 ||
	CS_entry ("agency_password", 1, PSWD_SIZE, site.agency_pwd) <= 0 ||
	CS_entry ("urc_password", 1, PSWD_SIZE, site.urc_pwd) <= 0) {
	fprintf (stderr, "password not found in %s\n", full_name);
	exit (1);
    }
    CS_control (CS_CLOSE);

    if (N_sites >= Site_size) {
	fprintf (stderr, "Sites buffer too small (%d)\n", N_sites);
	exit (1);
    }
    memcpy (Sites + N_sites, &site, sizeof (site_info_t));
    N_sites++;
    return (0);
}

/************************************************************************

    Verifies redundant channel info. Every thing should be identical except
    the channel number.

************************************************************************/

static void Verify_redundant_channel (site_info_t *sites, int n_sites) {
    int i, ret;

    for (i = 0; i < n_sites; i++) {
	site_info_t *rsite, *site = sites + i;

	rsite = sites + i - 1;
	if (site->channel == 0) {
	    if (i > 0 && strcmp (rsite->name, site->name) == 0) {
		if (memcmp (rsite, site, sizeof (site_info_t)) != 0) {
		    fprintf (stderr, "Duplicated site info for %s\n", site->name);
		    exit (1);
		}
		else
		    strcpy (site->cat, "D");	/* mark as duplicated */
	    }
	    continue;
	}
	if (site->channel != 1 ||
	    i <= 0 ||
	    rsite->channel != 0) {
	    fprintf (stderr, "Redundant channel not found for %s\n", site->name);
	    exit (1);
	}
	rsite->channel = 1;
	rsite->node[3] = '2';
	if (site->n_links == 0 || rsite->n_links == 0)
	    ret = memcmp (rsite, site, (char *)(site->roc_pwd) - (char *)site);
	else
	    ret = memcmp (rsite, site, sizeof (site_info_t));
	if (ret != 0) {
	    fprintf (stderr, 
			"Redundant channel not correct for %s\n", site->name);
	    exit (1);
	}
	rsite->channel = 0;
	rsite->node[3] = '1';
    }
    return;
}

/**************************************************************************

    Comparison function used for sorting the sites.

**************************************************************************/

static int Compare_sites (const void *s1p, const void *s2p) {
    site_info_t *s1, *s2;
    int ret;

    s1 = (site_info_t *)s1p;
    s2 = (site_info_t *)s2p;
    if ((ret = strcmp (s1->name, s2->name)) != 0)
	return (ret);
    return (strcmp (s1->node, s2->node));
}

/************************************************************************

    Reads the comms_link.conf in "dir" and puts data in "s".

************************************************************************/

int RF_read_comms_link_conf (char *dir, site_info_t *s) {
    char buf[TMP_BUF_SIZE], *full_name;
    int user_profile_required, i;

    full_name = Get_full_path (dir, "comms_link.conf");
    CS_cfg_name (full_name);
    CS_control (CS_COMMENT | '#');
    if (CS_entry ("number_links", 1 | CS_INT, 1, 
				(char *)&(s->n_links)) <= 0) {
	fprintf (stderr, "number_links not found at %s\n", full_name);
	exit (1);
    }
    if (s->n_links <= 0 || s->n_links > MAX_N_LINKS) {
	fprintf (stderr, "Unexpected number_links (%d) at %s\n", 
						s->n_links, full_name);
	exit (1);
    }

    user_profile_required = 1;
    for (i = 0; i < s->n_links; i++) {
	int uclass;

	if (CS_entry_int_key (i, 1 | CS_INT, 1, (char *)&(s->psn[i])) <= 0 ||
	    CS_entry_int_key (i, 2 | CS_INT, 1, (char *)&(s->cmn[i])) <= 0 ||
	    CS_entry_int_key (i, 3 | CS_INT, 1, (char *)&(s->devn[i])) <= 0 ||
	    CS_entry_int_key (i, 4 | CS_INT, 1, (char *)&(s->portn[i])) <= 0 ||
	    CS_entry_int_key (i, 5, TMP_BUF_SIZE, buf) <= 0 ||
	    (s->type[i] = RF_get_enum (Ln_type, buf)) < 0 ||
	    CS_entry_int_key (i, 6 | CS_INT, 1, (char *)&(s->baud[i])) <= 0 ||
	    CS_entry_int_key (i, 7, TMP_BUF_SIZE, buf) <= 0 ||
	    (s->cm_name[i] = RF_get_enum (Cm_name, buf)) < 0 ||
	    CS_entry_int_key (i, 8 | CS_INT, 1, (char *)&(s->psize[i])) <= 0 ||
	    CS_entry_int_key (i, 9 | CS_INT, 1, (char *)&(s->npvc[i])) <= 0 ||		    CS_entry_int_key (i, 10 | CS_INT, 1, 
					(char *)&(s->lstate[i])) <= 0 ||
	    CS_entry_int_key (i, 11 | CS_INT, 1, (char *)&(s->ntf[i])) <= 0) {
	    fprintf (stderr, "Line (%d) not correct at %s\n", i, full_name);
	    exit (1);
	}
	if (CS_entry_int_key (i, 12 | CS_INT, 1, (char *)&uclass) > 0) {
	    user_profile_required = 0;
	    s->class[i] = uclass;
	    if (CS_entry_int_key (i, 13 | CS_INT, 1, 
						(char *)&(s->tout[i])) <= 0)
		s->tout[i] = 0;
	    if (CS_entry_int_key (i, 14, SHORT_NAME_SIZE, s->pswd[i]) <= 0)
		s->pswd[i][0] = '\0';
	}
    }
    if (CS_entry ("RDA_link", 1 | CS_INT,1,  (char *)&(s->rda_link)) <= 0) {
	fprintf (stderr, "RDA_link not found at %s\n", full_name);
	exit (1);
    }
    CS_control (CS_CLOSE);
    return (user_profile_required);
}

/************************************************************************

    Reads "user_profile" in "dir" and puts data in "site".

************************************************************************/

static void Read_user_profile (char *dir, site_info_t *site) {
    char *line, buf[TMP_BUF_SIZE], *full_name;

    full_name = Get_full_path (dir, "user_profiles");
    CS_cfg_name (full_name);
    CS_control (CS_COMMENT | '#');
    CS_control (CS_KEY_OPTIONAL);
    line = CS_THIS_LINE;
    while (1) {
	int ind, class;

	if (CS_entry (line, 0, TMP_BUF_SIZE, buf) <= 0)
	    break;
	line = CS_NEXT_LINE;

	if (MAIN_read_user_ids ()) {
	    if (strcmp (buf, "Dial_user") == 0 ||
		strcmp (buf, "Dedicated_user") == 0)
		Process_user_ids (buf, full_name, site->name, site->node);
	}

	if (strcmp (buf, "Line_user") != 0)
	    continue;

	if (CS_level (CS_DOWN_LEVEL) < 0) {
	    fprintf (stderr, "Bad line_user in %s\n", full_name);
	    exit (1);
	}

	if (CS_entry ("line_ind", 1 | CS_INT, 1, (char *)&ind) <= 0 ||
	    CS_entry ("class", 1 | CS_INT, 1, (char *)&class) <= 0) {
	    fprintf (stderr, "Bad line_user (c/l) in %s\n", full_name);
	    exit (1);
	}
	if (ind < 0 || ind >= MAX_N_LINKS) {
	    fprintf (stderr, "Bad line_user (ind %d) in %s\n", ind, full_name);
	    exit (1);
	}
	site->class[ind] = class;
	if (CS_entry ("port_password", 1, TMP_BUF_SIZE, buf) > 0)
	    strncpy (site->pswd[ind], buf, SHORT_NAME_SIZE);
	else 
	    strcpy (site->pswd[ind], "");
	site->pswd[ind][7] = '\0';

	CS_level (CS_UP_LEVEL);
    }
    CS_control (CS_CLOSE);
}

/************************************************************************

    Returns enum value of "str" in terms of enum definition in "enums".

************************************************************************/

int RF_get_enum (char **enums, char *str) {
    int ind, len;
    char *p;

    p = str;
    while (*p == ' ')
	p++;
    len = strlen (p);
    while (len > 0 && (p[len - 1] == '_' || p[len - 1] == ' '))
	len--;
    ind = 0;
    while (1) {
	if (strlen (enums[ind]) == 0)
	    return (-1);
	if (strncmp (enums[ind], p, len) == 0)
	    return (ind);
	ind++;
    }
}

/************************************************************************

    Reads user ID, name and password from the user profile, verifies them
    and stores them in the user table.

************************************************************************/

static void Process_user_ids (char *key, char *fname, char *site, char *node) {
    int uid, i, dial;
    char user_pswd[UNAME_SIZE], user_name[UNAME_SIZE];

    if (Users == NULL)
	Users = MISC_malloc (MAX_N_USERS * sizeof (User_t));

    if (CS_level (CS_DOWN_LEVEL) < 0) {
	fprintf (stderr, "Bad line_user in %s\n", fname);
	exit (1);
    }

    if (CS_entry ("user_id", 1 | CS_INT, 1, (char *)&uid) <= 0 ||
	CS_entry ("user_name", 1, UNAME_SIZE, user_name) <= 0) {
	fprintf (stderr, "Bad user section in %s\n", fname);
	exit (1);
    }
    user_pswd[0] = '\0';
    dial = 0;
    if (strcmp (key, "Dial_user") == 0) {
	if (CS_entry ("user_password", 1, UNAME_SIZE, user_pswd) <= 0) {
	    fprintf (stderr, "Bad user section (password) in %s\n", fname);
	    exit (1);
	}
	dial = 1;
    }

    for (i = 0; i < N_users; i++) {
	if (Users[i].id == uid && Users[i].dial == dial &&
	    strcmp (Users[i].pswd, user_pswd) == 0 &&
	    strcmp (Users[i].name, user_name) == 0)
	    break;
    }
    if (i >= N_users) {
	if (N_users > MAX_N_USERS) {
	    fprintf (stderr, "Too many users found\n");
	    exit (1);
	}
	Users[N_users].id = uid;
	Users[N_users].dial = dial;
	strcpy (Users[N_users].pswd, user_pswd);
	strcpy (Users[N_users].name, user_name);
	strcpy (Users[N_users].site, site);
	strcpy (Users[N_users].node, node);
	N_users++;
    }

    CS_level (CS_UP_LEVEL);
}

static void Print_all_users () {
    int i;

    qsort (Users, N_users, sizeof (User_t), Compare_users);

    printf ("User table:\n");
    printf ("    id dial         name     password\n\n");
    for (i = 0; i < N_users; i++) {
	int uid, dial, k;

	uid = Users[i].id;
	dial = Users[i].dial;
	if (i > 0 && uid == Users[i - 1].id && dial == Users[i - 1].dial)
	    continue;

	printf ("%6d %4d %12s %12s\n", 
	    Users[i].id, Users[i].dial, Users[i].name, Users[i].pswd);
	for (k = i + 1; k < N_users; k++) {
	    if (Users[k].id == uid) {
		if (strcmp (Users[k].name, Users[i].name) != 0)
		    printf ("            %12s\n", Users[k].name);
		if (Users[k].dial && dial &&
		    strcmp (Users[k].pswd, Users[i].pswd) != 0)
		    printf ("                         %12s\n", Users[k].pswd);
/*
		if (Users[k].dial != dial)
		    printf ("        User dial conflict: (%d)\n", 
						Users[k].dial);
*/
	    }
	}
    }
}

/**************************************************************************

    Comparison function used for sorting the sites.

**************************************************************************/

static int Compare_users (const void *u1p, const void *u2p) {
    User_t *s1, *s2;

    s1 = (User_t *)u1p;
    s2 = (User_t *)u2p;
    if (s1->id == s2->id)
	return (s1->dial - s2->dial);
    return (s1->id - s2->id);
}


