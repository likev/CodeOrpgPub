
/******************************************************************

    This is a tool that reads the existing adaptation data files and
    retrieves site info.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2008/01/29 16:33:58 $
 * $Id: ma_main.c,v 1.8 2008/01/29 16:33:58 jing Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 
#define NEED_COMMS_STRING_CONST
#include "manage_adapt.h"

#define MAX_N_SITES 300

static char Input_dir[LOCAL_NAME_SIZE] = "";
static char Output_dir[LOCAL_NAME_SIZE] = "";
static char Comp_dir[LOCAL_NAME_SIZE] = "";
static char Cm_db_dir[LOCAL_NAME_SIZE] = "";
static char Version[LOCAL_NAME_SIZE] = "";
static char Site_category[SHORT_NAME_SIZE] = "";
static char Site_names[MAX_N_SITES][SHORT_NAME_SIZE];
static int N_site_names = 0;
static int Tar_and_compress = 0;
static int Read_users = 0;
static int All_cat = 0;			/* All site categories are selected */
static char Pformat[LOCAL_NAME_SIZE] = "";
static int No_cat_in_creation = 0;
static int Merge_csv_files = 0;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Create_all_sites ();
static void Compare_master_dirs ();
static int print_difference (site_info_t *s1, site_info_t *s2);
static void Print_users ();
static void Create_master_dir ();
static void Set_site_cat (char *sname, char *scat);
static void Get_dir (char *dir, char *argv);
static void Create_user_tables ();
static char *Process_duplicated_diff (char *msg, char *site);
static void Merge_a_csv_file (char *src_name, char *dest_name);
static void Merge_csv_files_subdirs ();


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Merge_csv_files) {
	Merge_csv_files_subdirs ();
	exit (0);
    }

    if (Input_dir[0] != '\0' && Comp_dir[0] != '\0') {
	sprintf (Cm_db_dir, "%s/src_data", Comp_dir);
	Compare_master_dirs ();
	exit (0);
    }

    if (Read_users && Input_dir[0] != '\0') {
	Print_users ();
	exit (0);
    }

    if (Output_dir[0] != '\0') {
	sprintf (Cm_db_dir, "%s/src_data", Output_dir);
	Create_master_dir ();
	exit (0);
    }

    exit (0);
}

/************************************************************************

    Merges csv files in subdirs of the current dir and put them in the
    current dir.

************************************************************************/

static void Merge_csv_files_subdirs () {

     Merge_a_csv_file ("CommsLink.csv", "commslink.csv");
     Merge_a_csv_file ("hcipassword.csv", "hcipassword.csv");
     Merge_a_csv_file ("siteinfo.csv", "siteinfo.csv");
     system ("cp ./release/MasterList.csv ./masterlist.csv");
     system ("cp ./release/masterlistRSHI.csv ./masterlistrshi.csv");
     system ("cp ./release/masterlistRCWF.csv ./masterlistrcwf.csv");
}

/************************************************************************

    Merges file "src_name" in subdirs and puts in the current dir as 
    "dest_name".

************************************************************************/

static void Merge_a_csv_file (char *src_name, char *dest_name) {
    char *dir_names[] = {"hq", "system", "release"};
    int i, fcnt, lcnt;
    FILE *f_dest, *f_src;

    f_dest = fopen (dest_name, "w");
    if (f_dest == NULL) {
	printf ("Cannot open file %s for writing\n", dest_name);
	return;
    }
    fcnt = lcnt = 0;
    for (i = 0; i < 3; i++) {
	char name[256], buf[2048];
	int cnt;

	sprintf (name, "./%s/%s", dir_names[i], src_name);
	f_src = fopen (name, "r");
	if (f_src == NULL)
	    continue;
	cnt = 0;
	while (fgets (buf, 2048, f_src) != NULL) {
	    buf[2047] = '\0';
	    if (!(fcnt > 0 && cnt == 0))
		fwrite (buf, 1, strlen (buf), f_dest);
	    cnt++;
	    lcnt++;
	}
	fcnt++;
	fclose (f_src);
    }
    fclose (f_dest);
    printf ("%d files merged to %s - %d lines\n", fcnt, dest_name, lcnt);
}

/************************************************************************

    Print all product users.

************************************************************************/

static void Print_users () {
    site_info_t *site;

    site = (site_info_t *)MISC_malloc (MAX_N_SITES * sizeof (site_info_t));
    RF_read_site_master_dir (Input_dir, site, MAX_N_SITES);
}

/************************************************************************

    Compare adapt data in two master dirs and prints the differences.

************************************************************************/

static void Compare_master_dirs () {
    int n1, n2, i1, i2;
    site_info_t *site1, *site2, *s1, *s2;

    site1 = (site_info_t *)MISC_malloc (MAX_N_SITES * sizeof (site_info_t));
    site2 = (site_info_t *)MISC_malloc (MAX_N_SITES * sizeof (site_info_t));
    printf ("Reading data from %s...\n", Input_dir);
    n1 = RF_read_site_master_dir (Input_dir, site1, MAX_N_SITES);
    printf ("Reading data from %s...\n", Comp_dir);
    n2 = RF_read_site_master_dir (Comp_dir, site2, MAX_N_SITES);
    printf ("%d sites for %s and %d sites for %s\n", 
					n1, Input_dir, n2, Comp_dir);

    printf ("Printing adapt differences...\n");
    for (i1 = 0; i1 < n1; i1++) {
	s1 = site1 + i1;
 	for (i2 = 0; i2 < n2; i2++) {
	    s2 = site2 + i2;
	    if (strcmp (s2->name, s1->name) == 0 &&
		strcmp (s2->node, s1->node) == 0)
		break;
	}
	if (i2 >= n2)
	    continue;
	if (!MAIN_is_site_selected (s1))
	    continue;
	print_difference (s1, s2);
    }
    Process_duplicated_diff (NULL, NULL);
}

/************************************************************************

    Prints differences between two sites "s1" and "s2".

************************************************************************/

static int print_difference (site_info_t *s1, site_info_t *s2) {
    int see_more;
    char buf[512];

    printf ("%s %s:\n", s1->name, s1->node);
    if (memcmp (s1, s2, (char *)(s1->cat) - (char *)s1) == 0)
	return (0);

    see_more = 0;
    if (s1->lat != s2->lat)
	printf ("    Radar latitude     %8d %8d\n", s1->lat, s2->lat);
    if (s1->lon != s2->lon)
	printf ("    Radar longitude    %8d %8d\n", s1->lon, s2->lon);
    if (s1->elev != s2->elev)
	printf ("    Radar altitude     %8d %8d\n", s1->elev, s2->elev);
    if (s1->rpg_id != s2->rpg_id)
	printf ("    rpg_id             %8d %8d\n", s1->rpg_id, s2->rpg_id);
    if (s1->wx_mode != s2->wx_mode)
	printf ("    wx_mode            %8d %8d\n", s1->wx_mode, s2->wx_mode);
    if (s1->vcp_a != s2->vcp_a)
	printf ("    vcp_a              %8d %8d\n", s1->vcp_a, s2->vcp_a);
    if (s1->vcp_b != s2->vcp_b)
	printf ("    vcp_b              %8d %8d\n", s1->vcp_b, s2->vcp_b);
    if (s1->mlos != s2->mlos)
	printf ("    mlos               %8d %8d\n", s1->mlos, s2->mlos);
    if (s1->rms != s2->rms)
	printf ("    rms                %8d %8d\n", s1->rms, s2->rms);
    if (s1->bdds != s2->bdds)
	printf ("    bdds               %8d %8d\n", s1->bdds, s2->bdds);
    if (s1->a_3 != s2->a_3)
	printf ("    archive_3          %8d %8d\n", s1->a_3, s2->a_3);
    if (s1->p_code != s2->p_code)
	printf ("    p_code             %8d %8d\n", s1->p_code, s2->p_code);
    if (s1->redun_t != s2->redun_t)
	printf ("    redun_t            %8d %8d\n", s1->redun_t, s2->redun_t);
    if (s1->channel != s2->channel)
	printf ("    channel            %8d %8d\n", s1->channel, s2->channel);
    if (s1->n_mlos != s2->n_mlos)
	printf ("    n_mlos             %8d %8d\n", s1->n_mlos, s2->n_mlos);
    if (memcmp (s1->mlos_t, s2->mlos_t, N_MLOS * sizeof (int)) != 0)
	printf ("    mlos               %d %d %d %d    %d %d %d %d\n", 
	s1->mlos_t[0], s1->mlos_t[1], s1->mlos_t[2], s1->mlos_t[3], 
	s2->mlos_t[0], s2->mlos_t[1], s2->mlos_t[2], s2->mlos_t[3]);

    if (strcmp (s1->roc_pwd, s2->roc_pwd) != 0)
	printf ("    ROC HCI pswd       %s %s\n", s1->roc_pwd, s2->roc_pwd);
    if (strcmp (s1->agency_pwd, s2->agency_pwd) != 0)
	printf ("    AGENCY HCI pswd    %s %s\n", s1->agency_pwd, s2->agency_pwd);
    if (strcmp (s1->urc_pwd, s2->urc_pwd) != 0)
	printf ("    URC HCI pswd       %s %s\n", s1->urc_pwd, s2->urc_pwd);

    if (strcmp (Pformat, "site") == 0)
	return (0);
   
    if (s1->n_links != s2->n_links) {
	sprintf (buf, 
		"    # of comms links   %8d %8d", s1->n_links, s2->n_links);
	Process_duplicated_diff (buf, s1->name);
    }
    if (memcmp (&(s1->n_links), &(s2->n_links),
				s1->cat - (char *)(s1->n_links)) != 0) {
	int i;
	int min = s1->n_links;
	if (s2->n_links < min)
	    min = s2->n_links;
	for (i = 0; i < min; i++) {
	    sprintf (buf, "    Link %d: ", i);
	    if (s1->psn[i] != s2->psn[i])
		sprintf (buf + strlen (buf), "psn %d %d; ", 
						s1->psn[i], s2->psn[i]);
	    if (s1->cmn[i] != s2->cmn[i])
		sprintf (buf + strlen (buf), "cmn %d %d; ", 
						s1->cmn[i], s2->cmn[i]);
	    if (s1->devn[i] != s2->devn[i])
		sprintf (buf + strlen (buf), "devn %d %d; ", 
						s1->devn[i], s2->devn[i]);
	    if (s1->portn[i] != s2->portn[i])
		sprintf (buf + strlen (buf), "portn %d %d; ", 
						s1->portn[i], s2->portn[i]);
	    if (s1->type[i] != s2->type[i])
		sprintf (buf + strlen (buf), "type %s %s; ", 
				Ln_type[s1->type[i]], Ln_type[s2->type[i]]);
	    if (s1->baud[i] != s2->baud[i])
		sprintf (buf + strlen (buf), "baud %d %d; ", 
						s1->baud[i], s2->baud[i]);
	    if (s1->cm_name[i] != s2->cm_name[i])
		sprintf (buf + strlen (buf), "cm_name %s %s; ", 
			Cm_name[s1->cm_name[i]], Cm_name[s2->cm_name[i]]);
	    if (s1->psize[i] != s2->psize[i])
		sprintf (buf + strlen (buf), "psize %d %d; ", 
						s1->psize[i], s2->psize[i]);
	    if (s1->npvc[i] != s2->npvc[i])
		sprintf (buf + strlen (buf), "npvc %d %d; ", 
						s1->npvc[i], s2->npvc[i]);
	    if (s1->lstate[i] != s2->lstate[i])
		sprintf (buf + strlen (buf), "lstate %d %d; ", 
						s1->lstate[i], s2->lstate[i]);
	    if (s1->ntf[i] != s2->ntf[i])
		sprintf (buf + strlen (buf), "ntf %d %d; ", 
						s1->ntf[i], s2->ntf[i]);
	    if (s1->class[i] != s2->class[i])
		sprintf (buf + strlen (buf), "class %d %d; ", 
						s1->class[i], s2->class[i]);
	    if (strcmp (s1->pswd[i], s2->pswd[i]) != 0)
		sprintf (buf + strlen (buf), "pswd %s %s; ", 
						s1->pswd[i], s2->pswd[i]);
	    if (buf[strlen (buf) - 2] != ':') {
		if (Process_duplicated_diff (buf, s1->name) == NULL)
		    see_more = 1;
	    }
	}
    }
    if (see_more)
	printf ("    More differences - See common diffs later.\n");

    return (0);
}

/************************************************************************

    Creates a new adapt master dir.

************************************************************************/

static char *Process_duplicated_diff (char *msg, char *site) {
    typedef struct {
	char *msg;
	int cnt;
	char *sites;
    } msg_t;
    static msg_t **msgs = NULL;
    static int n_msgs = 0;
    int i;

    if (msg == NULL) {			/* print all duplicated messages */
	for (i = 0; i < n_msgs; i++) {
	    if (i == 0)
		printf ("\nDifferences common to multiple sites:\n");
	    if (msgs[i]->cnt <= 1)
		continue;
	    printf ("%s:\n", msgs[i]->sites);
	    printf ("%s\n", msgs[i]->msg);
	}
	return (NULL);
    }

    if (strcmp (Pformat, "comp") != 0) {
	printf ("%s\n", msg);
	return (msg);
    }

    for (i = 0; i < n_msgs; i++) {
	if (strcmp (msgs[i]->msg, msg) == 0)
	    break;
    }
    if (i >= n_msgs) {
	msg_t *new_msg;
	new_msg = (msg_t *)MISC_malloc (sizeof (msg_t) + strlen (msg) + 1);
	new_msg->msg = (char *)new_msg + sizeof (msg_t);
	strcpy (new_msg->msg, msg);
	new_msg->cnt = 1;
	new_msg->sites = NULL;
	new_msg->sites = STR_cat (new_msg->sites, site);
	msgs = (msg_t **)STR_append ((char *)msgs, (char *)&new_msg, 
							sizeof (msg_t *));
	n_msgs++;
	printf ("%s\n", msg);
	return (msg);
    }
    msgs[i]->cnt++;
    msgs[i]->sites = STR_cat (msgs[i]->sites, " ");
    msgs[i]->sites = STR_cat (msgs[i]->sites, site);
    return (NULL);
}

/************************************************************************

    Creates a new adapt master dir.

************************************************************************/

static void Create_master_dir () {
    int n_inps, n_cms, i;
    site_info_t *inp_s, *cm_s, *s1, *s2;

    if (strlen (Version) == 0) {
	int (*ORPGMISC_RPG_adapt_version_number) () = 
				(int (*) ())MISC_get_func ("liborpg.so", 
				"ORPGMISC_RPG_adapt_version_number", 1);
	if (ORPGMISC_RPG_adapt_version_number == NULL) {
	    printf ("Cannot find RPG adapt version - use default\n");
	    strcpy (Version, "8.0");
	}
	else {
	    int v = ORPGMISC_RPG_adapt_version_number ();
	    sprintf (Version, "%d.%d", v / 10, v % 10);
	}
    }

    WF_read_template_files (Cm_db_dir);

    All_cat = 1;
    inp_s = (site_info_t *)MISC_malloc (MAX_N_SITES * sizeof (site_info_t));
    n_inps = n_cms = 0;
    if (Input_dir[0] != '\0') {
	n_inps = RF_read_site_master_dir (Input_dir, inp_s, MAX_N_SITES);
	printf ("Adapt read from %d sites from %s\n", n_inps, Input_dir);
    }

    if (Cm_db_dir[0] != '\0') {
	cm_s = (site_info_t *)MISC_malloc 
				(MAX_N_SITES * sizeof (site_info_t));
	n_cms = RCS_read_cm_sites (Cm_db_dir, cm_s, MAX_N_SITES);
	printf ("Adapt read from %d sites from %s\n", n_cms, Cm_db_dir);
    }

    /* merge the two inputs */
    for (i = 0; i < n_inps; i++) {
	s1 = inp_s + i;
	s2 = RCS_search_site (s1->name, s1->node, cm_s, n_cms);
	if (s2 != NULL) {
	    memcpy (s1, s2, (char *)(s1->roc_pwd) - (char *)s1);
	    s2->name[0] = '\0';
	    s1->src = SRC_PREV_BUILD | SRC_CM_DB;
	}
	else
	    s1->src = SRC_PREV_BUILD;
	Set_site_cat (s1->name, s1->cat);
    }
    for (i = 0; i < n_cms; i++) {
	s2 = cm_s + i;
	if (s2->name[0] == '\0')
	    continue;
	if (n_inps >= MAX_N_SITES) {
	    fprintf (stderr, "Too many sites in Create_master_dir\n");
	    exit (1);
	}
	s1 = inp_s + n_inps;
	memcpy (s1, s2, sizeof (site_info_t));
	s1->src = SRC_CM_DB;
	Set_site_cat (s1->name, s1->cat);
	n_inps++;
    }
    RF_sort_verify_sites (inp_s, n_inps);

    RCS_read_cm_hci_pswd (Cm_db_dir, inp_s, n_inps);
    RCS_read_cm_comms (Cm_db_dir, inp_s, n_inps);

    All_cat = 0;
    Create_all_sites (inp_s, n_inps);

    Create_user_tables ();
}

/************************************************************************

    Creates the site adaptation data files for CM using info in "Sites".

************************************************************************/

static void Create_user_tables () {
    int n_users;
    User_t *users;

    users = MISC_malloc (MAX_N_USERS * sizeof (User_t));
    n_users = RCS_read_cm_users (Cm_db_dir, 
				"masterlist.csv", users, MAX_N_USERS);
    WF_create_user_table (Output_dir, "product_user_table", users, n_users);
    n_users = RCS_read_cm_users (Cm_db_dir, 
				"masterlistrcwf.csv", users, MAX_N_USERS);
    WF_create_user_table (Output_dir, 
				"product_user_table_rcwf_", users, n_users);
    n_users = RCS_read_cm_users (Cm_db_dir, 
				"masterlistrshi.csv", users, MAX_N_USERS);
    WF_create_user_table (Output_dir, 
				"product_user_table_rshi_", users, n_users);
    if (Tar_and_compress) {
	int ret;
	char cmd[512]; 
	sprintf (cmd, "%s \"cd %s; tar cvf %s %s; gzip -f %s\"", 
	    MISC_SYSTEM_SH, Output_dir, "shared.tar", "shared", "shared.tar");
	ret = MISC_system_to_buffer (cmd, NULL, 0, NULL);
	if (ret != 0) {
	    fprintf (stderr, 
		"MISC_system_to_buffer (%s) failed (%d)\n", cmd, ret);
	    exit (1);
	}
    }	
}

/************************************************************************

    Creates the site adaptation data files for CM using info in "Sites".

************************************************************************/

static void Create_all_sites (site_info_t *sites, int n_sites) {
    int i, cnt;

    cnt = 0;
    for (i = 0; i < n_sites; i++) {
	site_info_t *s;

	s = sites + i;
	if (!MAIN_is_site_selected (s))
	    continue;
	WF_create_site_adapt (s, Output_dir);
	if (Tar_and_compress)
	    WF_tar_compress_site_adapt (s, Output_dir);
	cnt++;
    }
    printf ("Site adaptation files created for %d sites\n", cnt);
}

/************************************************************************

    Sets "scat", the site category, in terms of "sname", the site name,
    according to the site category assignment defined in file 
    "site_category".

************************************************************************/

static void Set_site_cat (char *sname, char *scat) {
    typedef struct {
	char name[SHORT_NAME_SIZE];
	char cat[CAT_SIZE];
    } site_cat_t;
    static char *site_cats = NULL;
    static int n_site_cats = 0, site_cat_read = 0;
    int i;

    if (!site_cat_read) {
	site_cat_t scat;
	char fname[LOCAL_NAME_SIZE];

	sprintf (fname, "%s/%s", Cm_db_dir, "site_category");
	CS_cfg_name (fname);
	CS_control (CS_COMMENT | '#');
	CS_control (CS_KEY_OPTIONAL);
	memset (&scat, 0, sizeof (site_cat_t));
	while (CS_entry (CS_NEXT_LINE, 0, CAT_SIZE, scat.cat) > 0 &&
	       CS_entry (CS_THIS_LINE, 1, SHORT_NAME_SIZE, scat.name) > 0) {
	    site_cats = STR_append (site_cats, 
					(char *)&scat, sizeof (site_cat_t));
	    memset (&scat, 0, sizeof (site_cat_t));
	    n_site_cats++;
	}
	CS_control (CS_CLOSE);
	site_cat_read = 1;
    }

    for (i = 0; i < n_site_cats; i++) {
	site_cat_t *sc = (site_cat_t *)site_cats + i;
	if (strcmp (sc->name, sname) == 0) {
	    strcpy (scat, sc->cat);
	    break;
	}
    }
}

/************************************************************************

    Returns 1 if site "s" is selected for processing or 0 otherwise.

************************************************************************/

int MAIN_is_site_selected (site_info_t *s) {

    if (!All_cat &&
	(strcmp (Site_category, "R") == 0 || 
	 strcmp (Site_category, "G") == 0 ||
	 strcmp (Site_category, "B") == 0) &&
	strcmp (s->cat, Site_category) != 0)
	return (0);
    if (N_site_names > 0) {
	int k;
	for (k = 0; k < N_site_names; k++) {
	    if (strcmp (s->name, Site_names[k]) == 0)
		break;
	}
	if (k >= N_site_names)
	    return (0);
    }
    return (1);
}

/**************************************************************************

    Returns Read_users.

**************************************************************************/

int MAIN_read_user_ids () {
    return (Read_users);
}

/**************************************************************************

    Returns Version.

**************************************************************************/

char *MAIN_get_version () {
    return (Version);
}

/**************************************************************************

    Returns No_cat_in_creation.

**************************************************************************/

int MAIN_no_cat_in_creation () {
    return (No_cat_in_creation);
}

/**************************************************************************

    Reads command line arguments.

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
    while ((c = getopt (argc, argv, "i:d:c:o:s:v:p:munth?")) != EOF) {
	switch (c) {
	    int i;

            case 's':
		for (i = 0; i < N_site_names; i++) {
		    if (strcmp (optarg, Site_names[i]) == 0)
			break;
		}
		if (i < N_site_names)
		    break;
		if (N_site_names >= MAX_N_SITES) {
		    fprintf (stderr, "Too many site names specified\n");
		    Print_usage (argv);
		}
		strncpy (Site_names[N_site_names], optarg, SHORT_NAME_SIZE);
		Site_names[N_site_names][7] = '\0';
		N_site_names++;
                break;

            case 'i':
		Get_dir (Input_dir, optarg);
                break;

            case 'o':
		Get_dir (Output_dir, optarg);
                break;

            case 'd':
		Get_dir (Comp_dir, optarg);
                break;

            case 'c':
		if (strcmp (optarg, "A") != 0 && 
		    strcmp (optarg, "B") != 0 && 
		    strcmp (optarg, "G") != 0 && 
		    strcmp (optarg, "R") != 0) {
		    fprintf (stderr, "Bad -c option specified (%s)\n", optarg);
		    Print_usage (argv);
		}
		strcpy (Site_category, optarg);
                break;

            case 't':
		Tar_and_compress = 1;
                break;

            case 'm':
		Merge_csv_files = 1;
                break;

            case 'u':
		Read_users = 1;
                break;

            case 'n':
		No_cat_in_creation = 1;
                break;

            case 'v':
		strncpy (Version, optarg, LOCAL_NAME_SIZE);
		Version[LOCAL_NAME_SIZE - 1] = '\0';
                break;

            case 'p':
		strncpy (Pformat, optarg, LOCAL_NAME_SIZE);
		Pformat[LOCAL_NAME_SIZE - 1] = '\0';
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

    Copies directory input from "argv" to "dir".

**************************************************************************/

static void Get_dir (char *dir, char *argv) {
    int len;

    dir[0] = '\0';
    if (argv[0] != '/')
	strcpy (dir, "./");
    strncpy (dir + strlen (dir), argv, LOCAL_NAME_SIZE - 2);
    dir[LOCAL_NAME_SIZE - 1] = '\0';
    len = strlen (dir);
    if (len > 0 && dir[len - 1] == '/')
	dir[len - 1] = '\0';
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
    Generates site adapt files for CM or compares adapt data in two\n\
    master dirs. Files used for generation: comms_link.conf, site_category\n\
    siteinfo.csv, commslink.csv, hcipassword.csv and masterlist*.csv. These\n\
    files must be in the \"src_data\" subdir of the dir specified by -o\n\
    option.\n\
    Options:\n\
	-i input_dir (Specifies the input master dir)\n\
	-d comp_dir (Specifies the master dir to compare for difference)\n\
	-o output_dir (Specifies the output master dir for generation)\n\
	-t (tars and compresses the site adapt files after creation)\n\
	-m (Merge .csv files from subdir.)\n\
	-s site_name (specifies a site for processing. Multiple -s\n\
	   is acceptable. The default is all sites.)\n\
	-c category (\"A\", \"B\", \"G\" or \"R\". The default is all.)\n\
	   R - ROC; G - Generic; B - Beta sites; A - All other sites.\n\
	-u (Reads, verifies and prints the users.)\n\
	-v ver (Specifies a version number (e.g. 8.0) for master dir\n\
	   creation. The default is retrieved from the local RPG environ.)\n\
	-p format (\"site\" or \"comp\". Print format of differences.\n\
	   \"site\" - Site diff only; \"comp\" compact format.)\n\
	-n (No category subdirectories are used in master dir creation.)\n\
	-h (Prints usage info.)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
