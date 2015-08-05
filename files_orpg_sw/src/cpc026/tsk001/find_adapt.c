
/******************************************************************

    This is the main module for find_adapt. The -L and -F options
    are being phased out. The code supporting them should be 
    removed later.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/09/02 22:51:43 $
 * $Id: find_adapt.c,v 1.30 2009/09/02 22:51:43 ccalvert Exp $
 * $Revision: 1.30 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include <orpg.h>
#include <orpgadpt.h>
#include <infr.h>

#define MY_NAME_SIZE 256
#define MY_BUF_SIZE 1024

/* command line options */
static char *Source_dir = ".";
static char *Spec_date = "";
static char *Spec_time = "";
static char *Spec_site = "";
static char *Spec_channel = "";
static char *Spec_node = "";
static int Spec_ver = -1;
static int Print_all = 0;
static int Print_version_major = 0;
static int Print_version_minor = 0;
static int Creat_arch_name = 0;
static int Print_channel = 0;
static int Print_site = 0;
static int Basename = 0;
static int Print_work_dir = 0;
static int Print_node_info = 0;
static int Hub_load_mscf = 0;
static int Hub_load_rpg = 0;
static int Print_install_dir = 0;
static int Print_node_name = 0;
static char Var_check[MY_NAME_SIZE] = "";
static int Print_system_type = 0;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Output_names_all_channel ();
static void Output_site_info ();
static void Create_archive_name ();
static void Output_hub_load_mscf ();
static void Output_hub_load_rpg ();
static void Check_site_variable ();


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {
    int ret;
    char *afname;

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Print_system_type) {
	char *sys = ORPGMISC_get_site_name ("system");
	if (sys != NULL && strlen (sys) > 0) {
	    printf ("%s\n", sys);
	    exit (0);
	}
	else
	    exit (1);
    }

    if (strlen (Var_check) > 0)
	Check_site_variable ();

    if (Print_node_info) {
	ret = ORPGMGR_print_node_info ();
	if (ret < 0) {
	    LE_send_msg (0, "ORPGMGR_print_node_info failed (%d)", ret);
	    exit (1);
	}
	exit (0);
    }

    if ((ret = LE_init (argc, argv)) < 0) {
	LE_send_msg (0, "LE_init failed (%d)", ret);
	exit (1);
    }

    if (Print_work_dir) {
	char dir[256];
	if (MISC_get_work_dir (dir, 256) > 0)
	    printf ("%s\n", dir);
	else
	    printf ("%s\n", ".");
	exit (0);
    }

    if (Spec_site[0] == '\0')
	Spec_site = ORPGMISC_get_site_name ("site");
    if (Spec_channel[0] == '\0')
	Spec_channel = ORPGMISC_get_site_name ("channel_num");
    if (Spec_site == NULL || Spec_channel == NULL) {
	printf ("ORPGMISC_get_site_name - site info not found\n");
	exit (1);
    }
    if (strcmp (Spec_channel, "2") == 0)
	Spec_channel = "rpg2";
    else if (strcmp (Spec_channel, "1") == 0)
	Spec_channel = "rpg1";

    if (Hub_load_mscf)
	Output_hub_load_mscf ();

    if (Hub_load_rpg)
	Output_hub_load_rpg ();

    if (Print_all)
	Output_names_all_channel ();

    if (Print_version_major || Print_version_minor || Print_node_name ||
			Print_site || Print_channel || Print_install_dir)
	Output_site_info ();

    if (Creat_arch_name)
	Create_archive_name ();

    afname = ORPGADPTU_find_archive_name (Source_dir, Spec_date, 
		Spec_time, Spec_site, Spec_channel, Spec_ver, NULL, NULL);
    if (afname != NULL && strlen (afname) > 0) {
	if (Basename)
	    printf ("%s\n", MISC_basename (afname));
	else
	    printf ("%s\n", afname);
	exit (0);
    }

    exit (1);
}

/************************************************************************

    Prints the site name if the site is a HUB loading MSCF site.

************************************************************************/

static void Output_hub_load_mscf () {
    int ret;
    char buf[MY_BUF_SIZE];

    ret = ORPGMISC_get_site_value ("HLMSCF", buf, MY_BUF_SIZE);
    if (ret < 0 || strcmp (buf, "YES") != 0)
	exit (1);
    printf ("%s\n", Spec_site);
    exit (0);
}

/************************************************************************

    Prints the site name if the site is a HUB loading RPG site.

************************************************************************/

static void Output_hub_load_rpg () {
    int ret;
    char buf[MY_BUF_SIZE];

    ret = ORPGMISC_get_site_value ("HLRPG", buf, MY_BUF_SIZE);
    if (ret < 0 || strcmp (buf, "YES") != 0)
	exit (1);
    printf ("%s\n", Spec_site);
    exit (0);
}

/************************************************************************

    Prints the value of site variable "Var_check".

************************************************************************/

static void Check_site_variable () {
    int ret;
    char buf[MY_BUF_SIZE];

    ret = ORPGMISC_get_site_value (Var_check, buf, MY_BUF_SIZE);
    if (ret < 0) {
	fprintf (stderr, "%s\n", buf);
	if (strstr (buf, "Variable") != NULL &&
	    strstr (buf, "not found") != NULL)
	    exit (2);
	exit (1);
    }
    printf ("%s\n", buf);
    exit (0);
}

/************************************************************************

    Prints RPG adapt archive for all channels in "Source_dir" that match
    "Spec_date", "Spec_time" and "Spec_site".

************************************************************************/

static void Output_names_all_channel () {
    int i;

    for (i = 0; i < 2; i++) {
	char *chan, *fname;
	if (i == 0)
	    chan = "rpg1";
	else
	    chan = "rpg2";
	fname = ORPGADPTU_find_archive_name (Source_dir, Spec_date, 
		Spec_time, Spec_site, chan, Spec_ver, NULL, NULL);
	if (fname != NULL) {
	    if (Basename)
		printf ("%s\n", MISC_basename (fname));
	    else
		printf ("%s\n", fname);
	}
    }
    exit (0);
}

/************************************************************************

    Create RPG archive name and prints it.

************************************************************************/

static void Create_archive_name () {
    char *name;

    name = ORPGADPTU_create_archive_name (Spec_channel, 
					Spec_site, Spec_ver, 0);
    if (name == NULL)
	exit (1);
    printf ("%s\n", name);
    exit (0);
}

/************************************************************************

    Prints RPG adapt archive for all channels in "Source_dir" that match
    "Spec_date", "Spec_time" and "Spec_site".

************************************************************************/

static void Output_site_info () {
    int ver, err;

    err = 0;
    if (Print_site) {
	if (strlen (Spec_site) == 0) {
	    printf ("RPG site info not found\n");
	    err = 1;
	}
	else
	    printf ("%s\n", Spec_site);
    }
    else if (Print_channel) {
	if (strlen (Spec_channel) == 0) {
	    printf ("RPG site info not found\n");
	    err = 1;
	}
	else
	    printf ("%s\n", Spec_channel);
    }
    else if (Print_install_dir) {
	char *dir;
	int chan = 0;
	if (strcmp (Spec_channel, "rpg2") == 0)
	    chan = 2;
	else if (strcmp (Spec_channel, "rpg1") == 0)
	    chan = 1;
	dir = ORPGADPTU_install_dir (Spec_node, Spec_site, 
			chan, ORPGMISC_RPG_adapt_version_number ());
	if (dir == NULL) {
	    printf ("install info not found\n");
	    err = 1;
	}
	else
	    printf ("%s\n", dir);
    }
    else if (Print_node_name) {
	char *node;
	if (Spec_node[0] == '\0')
	    node = ORPGMISC_get_site_name ("type");
	else
	    node = Spec_node;
	if (node == NULL) {
	    printf ("Node name not found\n");
	    err = 1;
	}
	else
	    printf ("%s\n", node);
    }
    else {
	ver = ORPGMISC_RPG_adapt_version_number ();
	if (Print_version_major && Print_version_minor)
	    printf ("%d.%d\n", ver / 10, ver % 10);
	else if (Print_version_major)
	    printf ("%d\n", ver / 10);
	else if (Print_version_minor)
	    printf ("%d\n", ver % 10);
    }
    exit (err);
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
    while ((c = getopt (argc, argv, "D:d:t:s:H:c:v:n:V:TfILFSCMNmaqbwih?")) != EOF) {
	switch (c) {

	    case 'D':
		Source_dir = optarg;
		break;

	    case 'd':
		Spec_date = optarg;
		break;

	    case 't':
		Spec_time = optarg;
		break;

	    case 's':
		Spec_site = optarg;
		break;

	    case 'n':
		Spec_node = optarg;
		break;

	    case 'c':
		Spec_channel = optarg;
		break;

	    case 'a':
		Print_all = 1;
		break;

	    case 'f':
		Creat_arch_name = 1;
		break;

	    case 'N':
		Print_node_name = 1;
		break;

	    case 'M':
		Print_version_major = 1;
		break;

	    case 'm':
		Print_version_minor = 1;
		break;

	    case 'S':
		Print_site = 1;
		break;

	    case 'C':
		Print_channel = 1;
		break;

	    case 'b':
		Basename = 1;
		break;

	    case 'T':
		Print_system_type = 1;
		break;

	    case 'V':
		strncpy (Var_check, optarg, MY_NAME_SIZE);
		Var_check[MY_NAME_SIZE - 1] = '\0';
		break;

	    case 'L':
		Hub_load_mscf = 1;
		break;

	    case 'F':
		Hub_load_rpg = 1;
		break;

	    case 'w':
		Print_work_dir = 1;
		break;

	    case 'i':
		Print_node_info = 1;
		break;

	    case 'I':
		Print_install_dir = 1;
		break;

	    case 'v':
		if (strcmp (optarg, ".") == 0)
		    Spec_ver = -2;
		else if (sscanf (optarg, "%d", &Spec_ver) != 1)
		    Print_usage (argv);
		break;

	    case 'H':
	    case 'q':
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
    char *usage = "\
    Searches an RPG adaptation data archive in a specified directory,\n\
    which must be local, based on specified site, channel, version, date \n\
    and time, and prints the full file name. It also prints other RPG\n\
    info with various options. This is designed for being used by scripts.\n\
    Options:\n\
	-D dir (Specifies the directory where RPG adapt archive files \n\
	   to be found. Default: The current directory)\n\
	-d date (mm/dd/yyyy - Archive file date to find. Default: Today)\n\
	-t time (hh:mm:ss - Archive file time to find. Default: The \n\
	   current time)\n\
	   Note: If date is specified but not time, the latest file of the \n\
	   date is the best match. Otherwise, the file closest to the \n\
	   specified time is the best match.\n\
	-s site (Specifies the site name, e.g. KTLX. Default: The local site)\n\
	-v version (Specifies the adapt version. e.g. 80. \".\" indicates any \n\
           version. Default: The local version)\n\
	-n node (Specifies the node (rpg? or mscf). Default: The local node)\n\
	-c channel (Specifies the RPG channel (rpg1, rpg2 or none). Default:\n\
           The local channel)\n\
	-b (Base name, instead of the full file name, is printed)\n\
	-a (Prints adaptation archive file names for all redundant channels\n\
           of the specified site)\n\
           \n\
           The following options do not involve in an adapt archive search.\n\
	-f (Prints the RPG adapt archive name for the specified site, channel\n\
           and version)\n\
	-I (Prints install dir for the specified site, node, channel, version)\n\
	-M (Prints the major of local RPG adapt version number)\n\
	-m (Prints the minor of local RPG adapt version number)\n\
           Note: If both -M and -m are specified, full version number is \n\
           printed.\n\
	-S (Prints site name)\n\
	-C (Prints channel name)\n\
	-N (Prints node name)\n\
	-w (Prints work directory)\n\
	-i (Prints info of all nodes)\n\
	   Data source labels: I install; D default; B DEA DB; A ASCII DEA.\n\
	-T (Prints system type. Exit 0 on success or 1 on failure.)\n\
	-V var@file (Prints the value of site variable \"var\" got from \"file\". \n\
	   Exits with 0 on success, 2 if \"var\" is not defined or 1 on error.\n\
	   \"@file\" is optional. The default is $CFG_DIR/site_data.)\n\
	-h (Prints usage info)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}


