
/******************************************************************

    This is a tool that reads and ingests NEXRAD radar data
    from volume files or tape archive. This is the module that 
    implements tha interactive mode operations.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/02/15 23:15:38 $
 * $Id: pa_interact.c,v 1.9 2005/02/15 23:15:38 jing Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 
#include "pa_def.h" 

static char Dir_name[LOCAL_NAME_SIZE];	/* the directory to read from */

static int Signal_received = 0;		/* a signal is received */
static int Playback_aborted = 0;	/* The current playback is aborted */
static int Pause_mode = 0;		/* In pause mode */
static int In_playback = 0;		/* In playback mode */

static Ap_vol_file_t *Vol_files;	/* volume file list */
static int N_vol_files = 0;		/* size of array Vol_files */
static int Loop_mode = 0;		/* looping mode */
static int Vol_gap = 15 * 60;		/* maximum time gap between volumes in
					   seconds */
static int Pause_at = 0;

static int N_sessions = 0;		/* total number of sessions */
static int Cr_session = -1;		/* current session number */
static int Cr_n_vols = 0;		/* number of volumes in current 
					   session */
static int Cr_ses_ind = 0;		/* first index of current session */

static int Pb_start_ind = 0;		/* playback start volume index in the 
					   session */
static int N_pb_vols = 0;		/* # of playback volumes */
static int Interactive_mode = 0;

enum {PAUSE_AT_RADIAL, PAUSE_AT_ELEVATION, PAUSE_AT_VOLUME};
					/* values for Pause_at */


static void Print_main_menu ();
static void Start_new_dir ();
static void Check_sessions ();
static void Print_options_menu ();
static int Set_options ();
static void Play_back ();
static int Set_current_session (int cr_ses);
static int Input_start_time_and_n_volumes ();


/******************************************************************

    The main loop of the interactive operation.
	
******************************************************************/

void PAI_main_loop (char *dir_name) {

    Interactive_mode = 1;

    strcpy (Dir_name, dir_name);

    PAM_get_signal (&Signal_received);

    Start_new_dir ();

    while (1) {				/* The main loop */
	int func;

	Signal_received = 0;
	Print_main_menu ();

	while (sscanf (PAI_gets (), "%d", &func) != 1 || 
					func <= 0 || func > 6) {
	    printf ("Bad input - not accepted - Enter a selection: ");
	}

	switch (func) {
	    char tmp[STR_SIZE];
	    int t, cnt;

	    case 1:
		if (N_sessions <= 0) {
		    printf ("No session to select\n");
		    break;
		}
		while (1) {
		    printf ("Enter session number (1 to %d): ", N_sessions);
		    cnt = sscanf (PAI_gets (), "%d", &t);
		    if (Signal_received)
			break;
		    if (cnt != 1 || t < 1 || t > N_sessions)
			printf ("Bad input - not accepted\n");
		    else {
			Set_current_session (t - 1);
			break;
		    }
		}
		break;

	    case 2:
		Input_start_time_and_n_volumes ();
		break;

	    case 3:
		while (1) {
		    printf ("Enter a new data directory: ");
		    tmp[0] = '\0';
		    sscanf (PAI_gets (), "%s", tmp);
		    if (Signal_received)
			break;
		    if (strlen (tmp) == 0)
			printf ("Bad input - not accepted\n");
		    else {
			strncpy (Dir_name, tmp, LOCAL_NAME_SIZE);
			Dir_name[LOCAL_NAME_SIZE - 1] = '\0';
			Start_new_dir ();
			break;
		    }
		}
		break;

	    case 4:
		Play_back ();
		break;

	    case 5:
		if (Set_options () != 0)
		    return;
		break;

	    case 6:
		exit (0);

	    default:
		break;
	}
    }
}

/******************************************************************

    Inputs the starting volume and the number of volumes;

******************************************************************/

static int Input_start_time_and_n_volumes () {

    if (N_sessions <= 0) {
	printf ("No session defined\n");
	return (0);
    }

    while (1) {
	char *cpt;
	time_t tm, st_t;
	int cnt, t;

	printf ("Enter start vol time (yy:mm:dd:hh:mn or hh:mn) or index (1 to %d)\n", Cr_n_vols);
	printf ("  and, optionally (separated by space), # volumes: ");

	cpt = PAI_gets ();
	if (Signal_received)
	    break;
	cnt = PMI_parse_input_time (cpt, &tm);

	if (cnt == 5 || cnt == 2) {
	    int i;

	    st_t = Vol_files[Cr_ses_ind].time;
	    if (cnt == 2)
		tm = PAM_get_time_by_seconds_in_a_day (st_t, tm);
	    for (i = 0; i < Cr_n_vols; i++) {
		if (Vol_files[Cr_ses_ind + i].time >= tm)
		    break;
	    }
	    if (i >= Cr_n_vols) {
		printf ("The time to search is not found\n");
		continue;
	    }
	    Pb_start_ind = i;
	    N_pb_vols = Cr_n_vols;
	}
	else if (cnt == 1) {
	    if (tm >= 1 && tm <= Cr_n_vols) {
		Pb_start_ind = tm - 1;
		N_pb_vols = Cr_n_vols;
	    }
	    else {
		printf ("Volume index is out of range\n");
		continue;
	    }
	}
	else {
	    printf ("Bad input - not accepted\n");
	    continue;
	}

	while (*cpt != '\0' && *cpt != ' ')
	    cpt++;
	if (*cpt == ' ') {
	    cpt++;
	    if (sscanf (cpt, "%d", &t) == 1 && t > 0)
		N_pb_vols = t;
	}

	if (N_pb_vols + Pb_start_ind > Cr_n_vols)
	    N_pb_vols = Cr_n_vols - Pb_start_ind;
	break;
    }
    return (0);
}

/******************************************************************

    Parses string "in" to calculate the input time. The input time
    is returned with "t_in" if the values found is 5, 2, or 1.
    Returns the number of values found.

******************************************************************/

int PMI_parse_input_time (char *in, time_t *t_in) {
    time_t tm;
    int args_read, yy, mm, dd, hh, mn, ss;
    char *cpt, buf[32];

    strncpy (buf, in, 32);
    buf[31] = '\0';
    cpt = buf;
    while (*cpt != '\0') {
	if (*cpt == ' ') {
	    *cpt = '\0';
	    break;
	}
	cpt++;
    }
    args_read = sscanf (buf,
	    "%d%*c%d%*c%d%*c%d%*c%d", &yy, &mm, &dd, &hh, &mn);
    tm = 0;
    if (args_read == 5) {
	if (yy <= 30)
	    yy += 2000;
	else if (yy <= 1900)
	    yy += 1900;
	ss = 0;
	unix_time (&tm, &yy, &mm, &dd, &hh, &mn, &ss);
    }
    if (args_read == 2)
	tm = yy * 3600 + mm * 60;
    if (args_read == 1)
	tm = yy;
    *t_in = tm;

    return (args_read);
}

/******************************************************************

    Plays back the selected volume files;

******************************************************************/

static void Play_back () {

    Signal_received = 0;
    Playback_aborted = 0;
    Pause_mode = 0;
    In_playback = 1;
    printf ("Playback started (Use ctrl-c to stop)\n");
    PAP_session_start (1);
    do {
	int i, ind, ret;

	ind = Cr_ses_ind + Pb_start_ind;
	for (i = 0; i < N_pb_vols; i++) {
	    ret = PAP_playback_volume (Vol_files + ind);
	    if (ret < 0 || Playback_aborted) {
	        printf ("Playback aborted\n");
		PAP_session_start (0);
		In_playback = 0;
		return;
	    }
	    ind++;
	    if (ind >= N_vol_files)
		break;
	}
    } while (Loop_mode);
    printf ("\n");
    PAP_session_start (0);
    In_playback = 0;
}

/*******************************************************************

    Checks and processes pause mode. This function is called every
    time before a radial data is sent to output LB. Return 0 for
    continuing playback, 1 for terminate playback, 2 for continuing
    after pause.

*******************************************************************/

int PAI_pause (int radial_status, time_t data_time) {
    static int cnt = 0;
    int pause, t;

    cnt++;
    if ((cnt % 50) == 0) {
	if (Interactive_mode)
	    printf ("\r%s", PAI_ascii_time (data_time) + 11);
	fflush (stdout);
    }

    if (Signal_received) {
	Signal_received = 0;
	if (Pause_mode) {
	    Playback_aborted = 1;
	    return (1);
	}
	Pause_mode = 1;
	if (Pause_at == PAUSE_AT_ELEVATION)
	    printf (" Will pause at the end of elevation (Use ctrl-c to abort)\n");
	if (Pause_at == PAUSE_AT_VOLUME)
	    printf (" Will pause at the end of volume (Use ctrl-c to abort)\n");
    }

    pause = 0;
    if (Pause_mode) {
	if (Pause_at == PAUSE_AT_RADIAL ||
	    (Pause_at == PAUSE_AT_ELEVATION && 
			(radial_status == 0 || radial_status == 3)) ||
	    (Pause_at == PAUSE_AT_VOLUME && radial_status == 3)) {
	    pause = 1;
	    Pause_mode = 0;
	}
    }

    if (pause) {
	printf ("Paused - Enter 1 to continue or 2 to abort playback: ");
	while (sscanf (PAI_gets (), "%d", &t) != 1 || t < 1 || t > 2)
	    printf ("Bad input - try again (1 or 2): ");
	if (t == 2) {
	    Playback_aborted = 1;
	    return (1);
	}
	else {
	    printf ("Playback resumed (Use ctrl-c to stop)\n");
	    return (2);
	}
    }
    return (0);
}

/*******************************************************************

    Processes the options menu. Return 1 if returning to the top
    or 0 otherwise.

*******************************************************************/

static int Set_options () {

    while (1) {				/* The options loop */
	int func;

	Signal_received = 0;
	Print_options_menu ();

	while (sscanf (PAI_gets (), "%d", &func) != 1 || 
					func <= 0 || func > 7) {
	    if (Signal_received)
		return (0);
	    printf ("Bad input - not accepted - Enter a selection: ");
	}

	switch (func) {
	    int t, ret;
	    double d;

	    case 1:
		while (1) {
		    printf ("Enter a new playback speed (> 0.): ");
		    ret = sscanf (PAI_gets (), "%lf", &d);
		    if (Signal_received)
			break;
		    if (ret != 1 || d <= 0.)
			printf ("Bad input - not accepted\n");
		    else {
			PAP_set_speed (d);
			break;
		    }
		}
		break;

	    case 2:
		while (1) {
		    printf ("Enter a new maximum volume time gap in seconds (>= 60): ");
		    ret = sscanf (PAI_gets (), "%d", &t);
		    if (Signal_received)
			break;
		    if (ret != 1 || t < 60)
			printf ("Bad input - not accepted\n");
		    else if (Vol_gap != t) {
			Vol_gap = t;
			Check_sessions ();
			break;
		    }
		}
		break;

	    case 3:
		while (1) {
		    printf ("Enter a new pause mode: (1 for end of radial; 2 for end of elevation;\n");
		    printf ("                         3 for end of volume): ");
		    ret = sscanf (PAI_gets (), "%d", &t);
		    if (Signal_received)
			break;
		    if (ret != 1 || t < 1 || t > 3)
			printf ("Bad input - not accepted\n");
		    else {
			Pause_at = t - 1;
			break;
		    }
		}
		break;

	    case 4:
		if (Loop_mode)
		    Loop_mode = 0;
		else
		    Loop_mode = 1;
		break;

	    case 5:
		PAW_write_cd (Vol_files, N_vol_files);
		PAM_get_signal (&Signal_received);
		return (0);

	    case 6:
		return (1);

	    case 7:
		return (0);

	    default:
		break;
	}
    }
    return (0);
}

/*******************************************************************

    Prints the options menu.

*******************************************************************/

static void Print_options_menu () {

    printf ("\n");
    printf ("        1: Select a new speed (NOW %4.2f)\n", PAP_get_speed ());
    printf ("        2: Select a new max volume time gap (NOW %d seconds)\n", 
							Vol_gap);
    printf ("        3: Select a new pause mode ");
    if (Pause_at == PAUSE_AT_RADIAL)
	printf ("(NOW at the end of radial)\n");
    else if (Pause_at == PAUSE_AT_ELEVATION)
	printf ("(NOW at the end of elevation)\n");
    else
	printf ("(NOW at the end of volume)\n");
    if (Loop_mode)
	printf ("        4: Select playback once (NOW repeated playback)\n");
    else
	printf ("        4: Select Repeated playback (NOW playback once)\n");
    printf ("        5: Write volume files to CD\n");
    printf ("        6: Go to tape playback mode\n");
    printf ("        7: Return to the main menu\n");
    printf ("Enter a selection (1 to 7): ");
}

/*******************************************************************

    Performs a search of the data directory "Dir_name" and 
    reinitializes all playback control variables.

*******************************************************************/

static void Start_new_dir () {

    N_vol_files = PAF_search_volume_files (Dir_name, &Vol_files);
    if (N_vol_files < 0) {
	printf ("Searching directory %s failed\n", Dir_name);
	return;
    }

    Check_sessions ();
}

/*******************************************************************

    Sets the session number for each volume file. The current 
    session is reset to 0.

*******************************************************************/

static void Check_sessions () {

    Cr_session = -1;
    N_sessions = PAI_set_sessions (Vol_files, N_vol_files, Vol_gap);
    Set_current_session (0);
}

/*******************************************************************

    Sets of the current session to "cr_ses".

*******************************************************************/

static int Set_current_session (int cr_ses) {

    if (cr_ses >= 0 && cr_ses < N_sessions && Cr_session != cr_ses) {
	int i, cnt, ses_ind;
    
	cnt = ses_ind = 0;
	for (i = 0; i < N_vol_files; i++) {
	    if (Vol_files[i].session != cr_ses) {
		if (cnt == 0)
		    continue;
		else
		    break;
	    }
	    if (cnt == 0)
		ses_ind = i;
	    cnt++;
	}
	Cr_session = cr_ses;
	Pb_start_ind = 0;
	Cr_n_vols = cnt;
	N_pb_vols = Cr_n_vols;
	Cr_ses_ind = ses_ind;
    }
    return (0);
}

/*******************************************************************

    Sets the session number for each volume file based on 
    "vol_time_gap". Returns the number of sessions. "changed" returns
    the status about if any session is changed.

*******************************************************************/

int PAI_set_sessions (Ap_vol_file_t *vol_files, 
				int n_vol_files, int vol_time_gap) {
    int cr_ses, i;

    cr_ses = 0;
    for (i = 0; i < n_vol_files; i++) {
	if (i > 0 && 
		(vol_files[i].time - vol_files[i - 1].time > vol_time_gap || 
		 strcmp (vol_files[i].prefix, vol_files[i - 1].prefix) != 0))
	    cr_ses++;
	vol_files[i].session = cr_ses;
    }
    if (n_vol_files > 0)
	return (cr_ses + 1);
    else
	return (0);
}

/*******************************************************************

    Prints the main menu.

*******************************************************************/

static void Print_main_menu  () {

    printf ("\n\n");
    printf ("    Current dir: %s\n", Dir_name);
    PAI_print_session (Vol_files, N_vol_files);
    if (N_vol_files > 0)
	printf ("\n    Selected session: %d (%s), start vol %d (%s), %d vols\n",
		Cr_session + 1, Vol_files[Cr_ses_ind].prefix, 
		Pb_start_ind + 1, 
		PAI_ascii_time (Vol_files[Cr_ses_ind + Pb_start_ind].time), 
		N_pb_vols);
    if (N_vol_files == 0)
	printf ("    There is no volume file found in the directory\n");
    printf ("\n");
    if (N_vol_files > 0) {
	printf ("        1: Select a session\n");
	printf ("        2: Select starting volume and number of volumes for playback\n");
    }
    printf ("        3: Select a new data directory\n");
    printf ("        4: Playback\n");
    printf ("        5: Options\n");
    printf ("        6: Exit\n");
    printf ("Enter a selection (1 to 6): ");
}

/*******************************************************************

    Prints the sessions.

*******************************************************************/

void PAI_print_session (Ap_vol_file_t *vol_files, int n_vol_files) {
    int cr_ses, i;

    cr_ses = -1;
    for (i = 0; i < n_vol_files; i++) {
	if (vol_files[i].session != cr_ses) {
	    int total, k;
	    time_t last_time;

	    cr_ses = vol_files[i].session;
	    total = 0;
	    for (k = i; k < n_vol_files; k++) {
		if (vol_files[k].session != cr_ses)
		    break;
		last_time = vol_files[k].time;
		total++;
	    }
	    if (total > 0) {
		printf ("   %4d%10s%4d Vols from ", 
				cr_ses + 1, vol_files[i].prefix, total);
		printf ("%s to ", PAI_ascii_time (vol_files[i].time));
		printf ("%s\n", PAI_ascii_time (last_time));
	    }
	    i += (total - 1);
	}
    }
}

/*******************************************************************

    Returns the ASCII representation of the time "t".

*******************************************************************/

char *PAI_ascii_time (time_t t) {
    static char buffer[64];
    int y, mon, d, h, m, s;

    unix_time (&t, &y, &mon, &d, &h, &m, &s);
    sprintf (buffer, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d", mon, d, y, h, m, s);
    return (buffer);
}

/*******************************************************************

    Reads standard input up to 127 characters. It discards any 
    remaining characters in the buffer before reading.

*******************************************************************/

char *PAI_gets () {
    static char buf[STR_SIZE];

    fseek (stdin, 0, 2);
    buf[0] = '\0';
    fgets (buf, STR_SIZE, stdin);
    return (buf);
}



