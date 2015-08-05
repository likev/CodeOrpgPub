
/******************************************************************

    This is a tool that reads and ingests NEXRAD radar data
    from volume files or tape archive. This is the main module.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/10 21:11:05 $
 * $Id: pa_main.c,v 1.11 2014/04/10 21:11:05 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 
#include "pa_def.h" 

#define MESSAGE_SIZE 2432		/* NCDC message size */
#define FILE_HEADER_SIZE 24		/* NCDC file header size */
#define BUFFER_SIZE 500000		/* 100 uncompressed radials */

static int Verbose;			/* verbose mode flag */
static char Dir_name[LOCAL_NAME_SIZE];	/* the directory to read from */
static char Lb_name[LOCAL_NAME_SIZE];	/* output LB name */
static char Prefix[LOCAL_NAME_SIZE];	/* data (radar) name */
static char List_file_name[LOCAL_NAME_SIZE];	/* volume list file name */
static char First_file_name[LOCAL_NAME_SIZE];	/* first volume file name */

static int Test_mode;		/* Test mode - print volume file names only */
static int Continue_mode;	/* Continuous loop mode */
static double Play_speed;	/* Playback speed */
static int Maxn_volumes;	/* Maximum number of volumes to play */
static int Interactive_mode;	/* Interactive mode */
static int Ignore_sails;	/* Ignore SAILS cuts during playback */
static int Reset_data_time;	/* Reset data time on playback data to the 
				   current time */
static int Full_speed_simulation;
				/* speed simulation includes time between 
				   volumes */
static int Convert_to_31;	/* converts message 1 to message 31 */

enum {IM_NONE = 0, IM_FILE, IM_TAPE};
				/* values for Interactive_mode */


static Ap_vol_file_t *Vol_files;	/* volume file list */
static int N_vol_files = 0;		/* size of array Vol_files */

static int *Signal_flag = NULL;	/* address of the SIGINT received flag */


/* local functions */
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Find_matched_files ();
static void Playback_files ();
static void Print_file_names ();
static void Sig_handle (int sig);
static char *Get_option_value (char *cpt);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {

    if (Read_options (argc, argv) != 0)
	exit (1);

    if (Interactive_mode)
	Verbose = 1;
    if (argc == 1 && strlen (PAM_get_options (OPTION_ENV)) > 1)
	Interactive_mode = IM_FILE;

    if (PAP_initialize (Play_speed, Verbose, 
                        Interactive_mode, Ignore_sails) < 0)
	exit (1);

    while (Interactive_mode) {
	if (Interactive_mode == IM_TAPE) {
	    PAR_main_loop ();
	    Interactive_mode = IM_FILE;
	}
	else if (Interactive_mode == IM_FILE) {
	    PAI_main_loop (Dir_name);
	    Interactive_mode = IM_TAPE;
	}
    }

    if (List_file_name[0] != 0)
	N_vol_files = PAF_get_listed_files (Dir_name, 
					List_file_name, &Vol_files);
    else {
	N_vol_files = PAF_search_volume_files (Dir_name, &Vol_files);
	if (N_vol_files > 0)
	    Find_matched_files ();
    }
    if (N_vol_files <= 0) {
	if (Verbose)
	    printf ("No volume file found in %s\n", Dir_name);
	exit (0);
    }

    if (Test_mode)
	Print_file_names ();
    else
	Playback_files ();

    exit (0);
}

/******************************************************************

    Returns the "Reset_data_time" flag. 

******************************************************************/

int PAM_reset_data_time () {
    return (Reset_data_time);
}

/******************************************************************

    Returns the "Full_speed_simulation" flag. 

******************************************************************/

int PAM_full_speed_simulation () {
    return (Full_speed_simulation);
}

/******************************************************************

    Returns the "Convert_to_31" flag. 

******************************************************************/

int PAM_convert_to_31 () {
    return (Convert_to_31);
}

/******************************************************************

    Returns the time that is no less than "st_time" and matches the
    seconds "seconds" in a day. "seconds" contains only the hours
    and mimutes. 

******************************************************************/

#define SECONDS_IN_DAY 86400

time_t PAM_get_time_by_seconds_in_a_day (time_t st_time, int seconds) {
    time_t t;

    t = (st_time / SECONDS_IN_DAY) * SECONDS_IN_DAY + seconds;
    if (t + 1800 < st_time)
	t += SECONDS_IN_DAY;
    return (t);
}

/******************************************************************

    The module in this program handles the interupt signal 
    independently. This function allows a module to gain control
    of the signal. Calling this with "flag" = NULL causes the
    program to exit upon the signal.

******************************************************************/

void PAM_get_signal (int *flag) {
    static int set = 0;

    if (set == 0) {		/* sigset only once */
	MISC_sig_sigset (SIGINT, Sig_handle);
	set = 1;
    }
    Signal_flag = flag;
}

/*******************************************************************

    Signal handler.

*******************************************************************/

static void Sig_handle (int sig) {
    if (Signal_flag != NULL) {
	*Signal_flag = 1;
	printf ("\n");
    }
    else
	exit (0);
}

/******************************************************************

    Returns option string for option "option_name".

******************************************************************/

char *PAM_get_options (int option_name) {
    static char env[LOCAL_NAME_SIZE] = "";
    static char *tape_device, *cdw_device, *cdw_speed, *convert_to_bz2;

    if (option_name == PLAYBACK_LB)
	return (Lb_name);

    if (env[0] == '\0') {
	char *cpt = getenv ("A2_OPTIONS");
	if (cpt != NULL) {
	    if (strlen (cpt) >= LOCAL_NAME_SIZE) {
		printf ("A2_OPTIONS is too long\n");
		exit (1);
	    }
	    strcpy (env, cpt);
	}
	else
	    strcpy (env, ".");	/* disable this work */
	tape_device = strstr (env, "TAPE_DEVICE");
	cdw_device = strstr (env, "CDW_DEVICE");
	cdw_speed = strstr (env, "CDW_SPEED");
	if (strstr (env, "CONVERT_TO_BZ2") != NULL)
	    convert_to_bz2 = "bz2";
	else
	    convert_to_bz2 = "";
	tape_device = Get_option_value (tape_device);
	cdw_device = Get_option_value (cdw_device);
	cdw_speed = Get_option_value (cdw_speed);
	if (tape_device == NULL)	    
	    tape_device = "/dev/rmt/0mb";
	if (cdw_speed == NULL)
	    cdw_speed = "4";
	if (cdw_device == NULL)
	    cdw_device = "";
    }

    if (option_name == OPTION_ENV)
	return (env);
    if (option_name == TAPE_DEVICE)
	return (tape_device);
    if (option_name == CDW_DEVICE)
	return (cdw_device);
    if (option_name == CDW_SPEED)
	return (cdw_speed);
    if (option_name == CONVERT_TO_BZ2)
	return (convert_to_bz2);
    return (NULL);
}

/******************************************************************

    Terminates and returns the option string pointed by "cpt".

******************************************************************/

static char *Get_option_value (char *cpt) {
    char *p;

    if (cpt == NULL)
	return (NULL);
    p = cpt;
    while (*p != '\0' && *p != '=')
	p++;
    if (*p != '=')
	return (NULL);
    p++;
    cpt = p;
    while (*p != '\0' && *p != ';' && *p != ':' && *p != '\n')
	p++;
    *p = '\0';
    return (cpt);
}

/******************************************************************

    Finds the files that match the command line options (-r and
    -s). The matched files are marked as selected.

******************************************************************/

static void Find_matched_files () {
    int st_ind, end_ind, i;

    st_ind = -1;
    if (strlen (First_file_name) > 0) {
	for (i = 0; i < N_vol_files; i++) {
	    if (strcmp (Vol_files[i].name, First_file_name) == 0 || 
		strcmp (Vol_files[i].path, First_file_name) == 0)
		break;
	}
	if (i >= N_vol_files) {
	    fprintf (stderr, "File %s not found\n", First_file_name);
	    exit (1);
	}
	if (strlen (Prefix) > 0 && strcmp (Vol_files[i].prefix, Prefix) != 0) {
	    fprintf (stderr, "Inconsistent -r and -s options\n");
	    exit (1);
	}
	if (strlen (Prefix) == 0)
	    strcpy (Prefix, Vol_files[i].prefix);
	st_ind = i;
    }

    if (strlen (Prefix) == 0) {
	st_ind = 0;
	end_ind = N_vol_files - 1;
    }
    else {
	end_ind = -1;
	for (i = 0; i < N_vol_files; i++) {
	    if (strcmp (Vol_files[i].prefix, Prefix) == 0) {
		if (st_ind < 0)
		    st_ind = i;
		end_ind = i;
	    }
	}
    }

    if (st_ind < 0) {
	if (Verbose)
	    printf ("No file found to match the command line options\n");
	exit (0);
    }
    for (i = st_ind; i <= end_ind; i++)
	Vol_files[i].selected = 1;
}

/******************************************************************

    Plays back the selected volume files;

******************************************************************/

static void Playback_files () {
    int i, cnt;

    if (Verbose)
	printf ("Playback...\n");
    PAP_session_start (1);
    cnt = 0;
    while (1) {
	for (i = 0; i <= N_vol_files; i++) {
	    if (!Vol_files[i].selected)
		continue;
	    PAP_playback_volume (Vol_files + i);
	    cnt++;
	    if (Maxn_volumes >= 0 && cnt >= Maxn_volumes) {
		if (Verbose)
		    printf ("Specified maximum number of volumes reached\n");
		exit (0);
	    }
	}
	if (!Continue_mode)
	    break;
    }
    PAP_session_start (0);
}

/******************************************************************

    Prints the names of selected volume files.

******************************************************************/

static void Print_file_names () {
    int i, cnt, size;

    cnt = size = 0;
    for (i = 0; i <= N_vol_files; i++) {
	if (Vol_files[i].selected) {
	    printf ("    %s\n", Vol_files[i].path);
	    size += Vol_files[i].size;
	    cnt++;
	}
    }
    printf ("%d files - total size %d\n", cnt, size);
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
    Test_mode = 0;
    Continue_mode = 0;
    Interactive_mode = IM_NONE;
    Ignore_sails = 0;
    Play_speed = 1.;
    Maxn_volumes = -1;
    Prefix[0] = '\0';
    List_file_name[0] = '\0';
    First_file_name[0] = '\0';
    Dir_name[0] = '\0';
    Lb_name[0] = '\0';
    Verbose = 1;
    Reset_data_time = 0;
    Full_speed_simulation = 0;
    Convert_to_31 = 0;
    while ((c = getopt (argc, argv, "Sd:s:p:r:n:x:o:CcaitRTqh?")) != EOF) {
	switch (c) {

            case 'S':
               Ignore_sails = 1;
               break;

            case 'd':
		strncpy (Dir_name, optarg, LOCAL_NAME_SIZE);
		Dir_name[LOCAL_NAME_SIZE - 1] = '\0';
                break;

            case 's':
		strncpy (First_file_name, optarg, LOCAL_NAME_SIZE);
		First_file_name[LOCAL_NAME_SIZE - 1] = '\0';
                break;

            case 'p':
		strncpy (List_file_name, optarg, LOCAL_NAME_SIZE);
		List_file_name[LOCAL_NAME_SIZE - 1] = '\0';
                break;

            case 'o':
		strncpy (Lb_name, optarg, LOCAL_NAME_SIZE);
		Lb_name[LOCAL_NAME_SIZE - 1] = '\0';
                break;

            case 'r':
		strncpy (Prefix, optarg, LOCAL_NAME_SIZE);
		Prefix[LOCAL_NAME_SIZE - 1] = '\0';
                break;

            case 'n':
		if (sscanf (optarg, "%d", &Maxn_volumes) != 1) {
		    fprintf (stderr, "unexpected -n specification\n");
		    err = -1;
		}
                break;

            case 'x':
		if (sscanf (optarg, "%lf", &Play_speed) != 1 ||
		    Play_speed < 0.) {
		    fprintf (stderr, "unexpected -x specification\n");
		    err = -1;
		}
                break;

	    case 'a':
		Interactive_mode = IM_TAPE;
		break;

	    case 'C':
		Convert_to_31 = 1;
		break;

	    case 'c':
		Continue_mode = 1;
		break;

	    case 'i':
		Interactive_mode = IM_FILE;
		break;

	    case 't':
		Test_mode = 1;
		break;

	    case 'R':
		Reset_data_time = 1;
		break;

	    case 'T':
		Full_speed_simulation = 1;
		break;

	    case 'q':
		Verbose = 0;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }
    if (Interactive_mode && List_file_name[0] != '\0') {
	if (Verbose)
	    printf ("Interactive mode - -p option ignored\n");
	List_file_name[0] = '\0';
    }
    if (List_file_name[0] != '\0') {
	if (First_file_name[0] != '\0' && Verbose)
	    printf ("-p option - -s option ignored\n");
	if (Prefix[0] != '\0' && Verbose)
	    printf ("-p option - -r option ignored\n");
    }

    return (err);
}

/**************************************************************************

    Prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
        Reads NEXRAD volume files or tape for generating radial\n\
        data input for RPG or creating an archive II data CD.\n\
        The volume file name must be:\n\
            prefix_yyyy?mm?dd?hh?mm?ss*.bz2 or\n\
            prefixyyyymmdd_hhmmss.Z\n\
            where \"prefix\" is the first part (often used for radar\n\
            name), \"prefix\" must not contain character \"_\",\n\
            \"?\" is any character and \"*\" is any string. \"?\"\n\
            and the first character of \"*\" must not be a number.\n\
        General options:\n\
          -S (Ignore SAILS cuts)\n\
          -i (Interactive mode file playback)\n\
          -a (Read tape instead of disk files)\n\
          -d dir (\"dir\" is the volume file directory to playback.\n\
                  The default is $AR2_DIR or the current directory\n\
                  if $AR2_DIR is not defined)\n\
          -o LB_name (\"LB_name\" is the name of playback output LB.\n\
                      The default is $ORPGDIR/ingest/resp.0)\n\
          -x speed (\"speed\" is the playback speed. E.g. 2 for twice and .5\n\
                    for half of the real time. The default is 1. 0 for data\n\
                    time independent playback - 20 radials per second.)\n\
          -R (Resets playback data time to the current time)\n\
          -C (Converts level II message 1 to message 31)\n\
          -T (Full playback speed simulation - time between volumes simulated)\n\
          -q (Runs quietly - Less status messages printed)\n\
          -h (Prints usage info)\n\
	Command line mode options:\n\
          -r prefix (\"prefix\" is the first part of the volume file\n\
                     name)\n\
          -s first_file_name (\"first_file_name\" is the name of the \n\
                              first volume file to playback)\n\
          -n n_volumes (\"n_volumes\" is the maximum number of volumes)\n\
                        to play)\n\
          -c (Continuous loop mode)\n\
          -t (Only prints volume file names would be played)\n\
	Play list mode option:\n\
          -p list_file_name (\"list_file_name\" is the name of the \n\
                             file that lists volume files)\n\
          -t (Only prints volume file names would be played)\n\
	Examples:\n\
	  play_a2\n\
		- Interactive volume file playback if A2_OPTIONS defined.\n\
	  play_a2 -a\n\
		- Tape playback.\n\
	  play_a2 -i\n\
		- Interactive mode volume file playback\n\
	  play_a2 -r KHGX -d /export/home/jing/ncdc\n\
		- Command line mode volume file playback\n\
	  play_a2 -p my_play_list\n\
		- Play list mode volume file playback\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}

