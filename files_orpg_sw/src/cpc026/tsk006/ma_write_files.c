
/******************************************************************

    This contains routines that create the site adaptation files.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/01/08 22:31:07 $
 * $Id: ma_write_files.c,v 1.10 2010/01/08 22:31:07 ccalvert Exp $
 * $Revision: 1.10 $
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

#define LINE_BUF_SIZE 256

static char *comms_part1 = NULL;
static char *comms_part2 = NULL;
static char *siteinfo = NULL;
static int n_siteinfo_lines = 0;

static void Set_dest_dir (site_info_t *s, char *out_dir, char *dest_dir);
static char *Get_cm_name (char *cm_name, char *site_name, int is_rda_link);

/************************************************************************

    Reads the template files for creating adaptation files.

************************************************************************/

int WF_read_template_files (char *dir) {
    char fname[LOCAL_NAME_SIZE], **str;
    FILE *fl;
    int line;
    char buf[LINE_BUF_SIZE];

    /* read commd_link.conf file template */
    sprintf (fname, "%s/%s", dir, "comms_link.conf");
    fl = fopen (fname, "r");
    if (fl == NULL) {
	fprintf (stderr, "comms_link.conf not found in %s\n", dir);
	exit (1);
    }
    str = &comms_part1;
    while (fgets (buf, LINE_BUF_SIZE, fl) != NULL) {
	if (sscanf (buf, "%d", &line) == 1) {
	    str = &comms_part2;
	}
	else {
	    if (strstr (buf, "number_links") != NULL)
		continue;
	    *str = STR_cat (*str, buf);
	}
    }
    fclose (fl);

    /* read site_info.dea file template */
    sprintf (fname, "%s/%s", dir, "site_info.dea");
    fl = fopen (fname, "r");
    if (fl == NULL) {
	fprintf (stderr, "site_info.dea not found in %s\n", dir);
	exit (1);
    }
    n_siteinfo_lines = 0;
    while (fgets (buf, LINE_BUF_SIZE, fl) != NULL) {
	siteinfo = STR_append (siteinfo, buf, strlen (buf) + 1);
	n_siteinfo_lines++;
    }
    fclose (fl);

    return (0);
}

/************************************************************************

    Creates site adapt files for site "s" in "out_dir". Returns 0 on 
    success or -1 on failure.

************************************************************************/

int WF_create_site_adapt (site_info_t *s, char *out_dir) {
    char wdir[LOCAL_NAME_SIZE], *line;
    char dir[LOCAL_NAME_SIZE], fname[LOCAL_NAME_SIZE], date[LOCAL_NAME_SIZE];
    FILE *fl;
    int ret, i;
    time_t t;

    if (s->channel == 0)
	printf ("Create adapt files for %s ", s->name);
    else
	printf ("Create adapt files for %s (channel 2) ", s->name);
    if (s->src & SRC_CM_DB)
	printf ("from CM DB.\n");
    else
	printf ("from existing build.\n");


    Set_dest_dir (s, out_dir, wdir);

    sprintf (dir, "%s/%s/rpg%d", wdir, s->name, s->channel + 1);
    ret = MISC_mkdir (dir);
    if (ret < 0) {
	fprintf (stderr, "Failed in creating DIR %s\n", dir);
	exit (1);
    }

    strcpy (fname, dir);
    strcat (fname, "/site_info.dea");
    fl = fopen (fname, "w+");
    if (fl == NULL) {
	fprintf (stderr, "Failed in creating file %s\n", fname);
	exit (1);
    }
    t = time (NULL);
    strcpy (date, ctime (&t));
    date[strlen (date) - 1] = '\0';
    fprintf (fl, "#%s %s V%s\n\n", s->name, date, MAIN_get_version ());

    line = siteinfo;
    for (i = 0; i < n_siteinfo_lines; i++) {
	char key[256];
	int n_s;
	if ((n_s = MISC_get_token (line, "", 0, key, 256)) <= 0 ||
	    key[0] == '#')
	    key[0] = '\0';
	n_s -= strlen (key);
	if (strcmp (key, "site_info.rpg_name:") == 0)
	    fprintf (fl, "    site_info.rpg_name:\t\t\t%s\n", s->name);
	else if (strcmp (key, "site_info.rda_lat:") == 0)
	    fprintf (fl, "    site_info.rda_lat:\t\t\t%d\n", s->lat);
	else if (strcmp (key, "site_info.rda_lon:") == 0)
	    fprintf (fl, "    site_info.rda_lon:\t\t\t%d\n", s->lon);
	else if (strcmp (key, "site_info.rda_elev:") == 0)
	    fprintf (fl, "    site_info.rda_elev:\t\t\t%d\n", s->elev);
	else if (strcmp (key, "site_info.rpg_id:") == 0)
	    fprintf (fl, "    site_info.rpg_id:\t\t\t%d\n", s->rpg_id);
	else if (strcmp (key, "site_info.wx_mode:") == 0)
	    fprintf (fl, "    site_info.wx_mode:\t\t\t%s\n", 
						Wx_mode[s->wx_mode]);
	else if (strcmp (key, "site_info.def_mode_A_vcp:") == 0)
	    fprintf (fl, "    site_info.def_mode_A_vcp:\t\t%d\n", s->vcp_a);
	else if (strcmp (key, "site_info.def_mode_B_vcp:") == 0)
	    fprintf (fl, "    site_info.def_mode_B_vcp:\t\t%d\n", s->vcp_b);
	else if (strcmp (key, "site_info.has_mlos:") == 0)
	    fprintf (fl, "    site_info.has_mlos:\t\t\t%s\n", Yes_no[s->mlos]);
	else if (strcmp (key, "site_info.has_rms:") == 0)
	    fprintf (fl, "    site_info.has_rms:\t\t\t%s\n", Yes_no[s->rms]);
	else if (strcmp (key, "site_info.has_bdds:") == 0)
	    fprintf (fl, "    site_info.has_bdds:\t\t\t%s\n", Yes_no[s->bdds]);
	else if (strcmp (key, "site_info.has_archive_III:") == 0)
	    fprintf (fl, "    site_info.has_archive_III:\t\t%s\n", 
							Yes_no[s->a_3]);
	else if (strcmp (key, "site_info.is_orda:") == 0)
	    fprintf (fl, "    site_info.is_orda:\t\t\t%s\n", Yes_no[s->orda]);
	else if (strcmp (key, "site_info.product_code:") == 0)
	    fprintf (fl, "    site_info.product_code:\t\t%d\n", s->p_code);
	else if (strcmp (key, "Redundant_info.redundant_type:") == 0)
	    fprintf (fl, "    Redundant_info.redundant_type:\t%s\n", 
							Redun_t[s->redun_t]);
	else if (strcmp (key, "Redundant_info.channel_number:") == 0)
	    if (s->channel == 0)
		fprintf (fl, "    Redundant_info.channel_number:\t%s\n", 
								Channel[0]);
	    else
		fprintf (fl, "    Redundant_info.channel_number:\t%s\n", 
								Channel[1]);
	else if (strcmp (key, "mlos_info.no_of_mlos_stations:") == 0)
	    fprintf (fl, "    mlos_info.no_of_mlos_stations:\t%d\n", 
								s->n_mlos);
	else if (strcmp (key, "mlos_info.station_type:") == 0)
	    fprintf (fl, "    mlos_info.station_type:\t\t%s, %s, %s, %s\n", 
				Mlos_t[s->mlos_t[0]],  Mlos_t[s->mlos_t[1]], 
				Mlos_t[s->mlos_t[2]],  Mlos_t[s->mlos_t[3]]); 
	else if (strcmp (key, "site_info.enable_sr:") == 0)
	    fprintf (fl, "    site_info.enable_sr:\t\t%s\n", 
							Yes_no[s->enable_sr]);
	else
	    fprintf (fl, "%s", line);
	line += strlen (line) + 1;
    }
    fclose (fl);

    strcpy (fname, dir);
    strcat (fname, "/hci_passwords");
    fl = fopen (fname, "w+");
    if (fl == NULL) {
	fprintf (stderr, "Failed in creating file %s\n", fname);
	exit (1);
    }
    fprintf (fl, "#%s %s V%s\n\n", s->name, date, MAIN_get_version ());
    fprintf (fl, " \"Passwords\"\n {\n");
    fprintf (fl, "     roc_password %s\n", s->roc_pwd);
    fprintf (fl, "     agency_password %s\n", s->agency_pwd);
    fprintf (fl, "     urc_password %s\n", s->urc_pwd);
    fprintf (fl, " }\n");
    fclose (fl);

    strcpy (fname, dir);
    strcat (fname, "/comms_link.conf");
    fl = fopen (fname, "w+");
    if (fl == NULL) {
	fprintf (stderr, "Failed in creating file %s\n", fname);
	exit (1);
    }
    fprintf (fl, "%s", comms_part1);
    for (i = 0; i < s->n_links; i++) {
	int cm_name, npvc, is_rda_link;

	if (i == s->rda_link) {
	    if (s->orda) {
		cm_name = 2;		/* cm_tcp */
		npvc = 1;
	    }
	    else {
		cm_name = 0;		/* cm_atlas */
		npvc = 0;
	    }
	    is_rda_link = 1;
	}
	else {
	    cm_name = s->cm_name[i];
	    npvc = s->npvc[i];
	    is_rda_link = 0;
	}
	fprintf (fl, "%4d%4d%4d%4d%4d%8s%8d%10s%6d%3d%4d%4d%4d", 
		i, s->psn[i], s->cmn[i], s->devn[i], s->portn[i], 
		Ln_type[s->type[i]], s->baud[i], 
		Get_cm_name (Cm_name[cm_name], s->name, is_rda_link), 
		s->psize[i], npvc, s->lstate[i], s->ntf[i], s->class[i]);
	if (s->type[i] != 0)
	    fprintf (fl, "%4d%7s", s->tout[i], s->pswd[i]);
	fprintf (fl, "\n");
    }

    fprintf (fl, "\n  number_links  %d\n", s->n_links);
    fprintf (fl, "%s", comms_part2);
    fclose (fl);

    return (0);
}

/************************************************************************

    Appends "_" (disabling the comms manager process for certain test beds)
    if the site name is NUX? (for No UconX) and the link is RDA link or the
    comms manager name is cm_uconx.

************************************************************************/

static char *Get_cm_name (char *cm_name, char *site_name, int is_rda_link) {
    static char buf[128];

    if (strncmp (site_name, "NUX", 3) == 0 &&
	(is_rda_link || strncmp (cm_name, "cm_uconx", 8) == 0)) {
	strncpy (buf, cm_name, 128);
	buf[126] = '\0';
	strcat (buf, "_");
	return (buf);
    }
    return (cm_name);
}

/************************************************************************

    Tars and compresses site adapt files for site "s" in "out_dir".
    Returns 0 on success or exit on failure.

************************************************************************/

int WF_tar_compress_site_adapt (site_info_t *s, char *out_dir) {
    char wdir[LOCAL_NAME_SIZE];
    char cmd[LINE_BUF_SIZE], fname[LOCAL_NAME_SIZE], obuf[LINE_BUF_SIZE];
    int n_bytes, ret;

    Set_dest_dir (s, out_dir, wdir);

    /* tar and compress the files for a site */
    sprintf (fname, "%s.V.%s.tar", s->name, MAIN_get_version ());
    sprintf (cmd, "%s \"cd %s; tar cvf %s %s; gzip -f %s\"", 
			MISC_SYSTEM_SH, wdir, fname, s->name, fname);
    ret = MISC_system_to_buffer (cmd, obuf, LINE_BUF_SIZE, &n_bytes);
    if (ret != 0) {
	fprintf (stderr, 
	    "MISC_system_to_buffer (%s) failed (%d)\n", cmd, ret);
	exit (1);
    }
    return (0);
}

/**************************************************************************

    Returns site category based dir, with "dest_dir", for site "s" in
    "out_dir".

**************************************************************************/

static void Set_dest_dir (site_info_t *s, char *out_dir, char *dest_dir) {

    if (MAIN_no_cat_in_creation ()) {
	sprintf (dest_dir, "%s", out_dir);
	return;
    }

    if (strcmp (s->cat, "R") == 0)
	sprintf (dest_dir, "%s/roc", out_dir);
    else if (strcmp (s->cat, "G") == 0)
	sprintf (dest_dir, "%s/generic", out_dir);
    else if (strcmp (s->cat, "B") == 0)
	sprintf (dest_dir, "%s/beta", out_dir);
    else
	sprintf (dest_dir, "%s/release", out_dir);
}

/************************************************************************

    Creates user table named "name" in "dir" of users in array "users" of
    size "n_users". Returns 0 on success or -1 on failure.

************************************************************************/

int WF_create_user_table (char *odir, char *name, User_t *users, int n_users) {
    char fname[LOCAL_NAME_SIZE];
    FILE *fl;
    int ret, i;

    sprintf (fname, "%s/shared", odir);
    ret = MISC_mkdir (fname);
    if (ret < 0) {
	fprintf (stderr, "Failed in creating DIR %s\n", fname);
	exit (1);
    }

    sprintf (fname, "%s/shared/%s", odir, name);
    fl = fopen (fname, "w+");
    if (fl == NULL) {
	fprintf (stderr, "Failed in creating file %s\n", fname);
	exit (1);
    }
    fprintf (fl, "\n# The product user table\n\n");
    fprintf (fl, "# user ID      user name                     description     access word\n\n");
    for (i = 0; i < n_users; i++) {
	char desc[UDESC_SIZE + 8];

	if (users[i].desc[0] == '\0')
	    sprintf (desc, "%s", users[i].name);
	else
	    sprintf (desc, "\"%s\"", users[i].desc);
	fprintf (fl, "%8d%16s%32s%16s\n", users[i].id, users[i].name, 
							desc, users[i].pswd);
    }

    fclose (fl);
    return (0);
}

