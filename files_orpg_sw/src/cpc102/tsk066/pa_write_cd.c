
/******************************************************************

    This is a tool that reads and ingests NEXRAD radar data
    from volume files or tape archive.  This is the module that 
    writes volume files to CD.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/03/23 17:50:34 $
 * $Id: pa_write_cd.c,v 1.5 2005/03/23 17:50:34 jing Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <infr.h> 
#include "pa_def.h" 

#define CMD_RM "/bin/rm"
#define CMD_MKDIR "mkdir"
#define CMD_LN "ln"
#define CMD_CP "cp"

#define CMD_ERR_FILE "play_a2.log"	/* cmd execution log file */

#define MAX_N_SESSIONS 128		/* max number of selected sessions */
#define CD_MAX_SIZE 700000000

static int Convert_to_bz2 = 0;

static int Signal_received = 0;		/* interrupt signal received */

static Ap_vol_file_t *Vol_files;	/* all volume files */
static int N_vol_files;			/* array size of vol_files */
static int N_sessions;			/* total number of sessions */
static int N_ses_selected;		/* number of selected sessions */
static int Ses_selected[MAX_N_SESSIONS];/* list of selected session numbers */


static int Create_cd_image (Ap_vol_file_t *vol_files, int n_files);
static int Write_cd (int total_size);
static int Get_selected_size ();
static int Get_n_volumes (int sesesion);
static void Deselect_session ();
static void Select_session ();
static int Is_selected (int ind);


/******************************************************************

    Writes volume files to CDs.
	
******************************************************************/

int PAW_write_cd (Ap_vol_file_t *vol_files, int n_vol_files) {
    int cr_size;
    char *menu = "\
\n\
        1: Select session\n\
	2: Deselect session\n\
	3: Write CD\n\
	4: Done\n\
Enter a selection (1 to 4): ";

    if (n_vol_files <= 0) {
	printf ("No files found to write to CD\n");
	return (0);
    }

    /* catch the signals */
    PAM_get_signal (&Signal_received);

    N_vol_files = n_vol_files;
    Vol_files = vol_files;
    N_sessions = PAI_set_sessions (Vol_files, N_vol_files, 600);
    N_ses_selected = 0;
    if (N_sessions <= 2) {		/* select all by default */
	Ses_selected[0] = 0;
	Ses_selected[1] = 1;
	N_ses_selected = N_sessions;
    }

    while (1) {				/* The main loop */
	int func, i;

	Signal_received = 0;
	printf ("\n\n");
	printf ("    Available sessions:\n");
	PAI_print_session (Vol_files, N_vol_files);
	printf ("\n    Selected sessions:\n");
	for (i = 0; i < N_ses_selected; i++) {
	    printf ("      %d (%s) %12d vols\n",
		Ses_selected[i] + 1, Vol_files[Ses_selected[i]].prefix, 
		Get_n_volumes (Ses_selected[i]));
	}
	cr_size = Get_selected_size ();
	printf ("    Total size is %d KB\n", (cr_size + 1023) / 1024);
	printf (menu);

	while (sscanf (PAI_gets (), "%d", &func) != 1 || 
						func <= 0 || func > 4) {
	    if (Signal_received)
		return (0);
	    printf ("Bad input - not accepted - Enter a selection: ");
	}

	switch (func) {

	    case 1:
		Select_session ();
		break;

	    case 2:
		Deselect_session ();
		break;

	    case 3:
		Write_cd (cr_size);
		break;

	    case 4:
		return (0);

	    default:
		break;
	}
    }
    return (0);
}

/******************************************************************

    Selects a session or all sessions interactively.
	
******************************************************************/

static void Select_session () {

    while (1) {
	int t, i, ret;
	char str[STR_SIZE];

	printf ("Enter a session number (1 to %d) or 'a' for all: ", 
							N_sessions);
	ret = sscanf (PAI_gets (), "%s", str);
	if (Signal_received)
	    break;
	if (ret != 1)
	    printf ("Bad input - not accepted\n");
	else if (strcmp (str, "a") == 0) {
	    for (i = 0; i < N_sessions; i++)
		Ses_selected[i] = i;
	    N_ses_selected = N_sessions;
	    break;
	}
	else if (sscanf (str, "%d", &t) != 1 || t <= 0 || t > N_sessions)
	    printf ("Bad input - not accepted\n");
	else {
	    t--;
	    for (i = 0; i < N_ses_selected; i++)
		if (Ses_selected[i] == t)
		    break;
	    if (i >= N_ses_selected) {
		if (N_ses_selected >= MAX_N_SESSIONS)
		    printf ("Too many sessions selected\n");
		else {
		    Ses_selected[N_ses_selected] = t;
		    N_ses_selected++;
		}
	    }
	    break;
	}
    }
}

/******************************************************************

    Deselects a session or all sessions interactively.
	
******************************************************************/

static void Deselect_session () {

    while (1) {
	int t, i, ret;
	char str[STR_SIZE];

	printf ("Enter a session number (1 to %d) or 'a' for all: ", 
							N_sessions);
	ret = sscanf (PAI_gets (), "%s", str);
	if (Signal_received)
	    break;
	if (ret != 1)
	    printf ("Bad input - not accepted\n");
	else if (strcmp (str, "a") == 0) {
	    N_ses_selected = 0;
	    break;
	}
	else if (sscanf (str, "%d", &t) != 1 || t <= 0 || t > N_sessions)
	    printf ("Bad input - not accepted\n");
	else {
	    t--;
	    for (i = 0; i < N_ses_selected; i++) {
		int k;
		if (Ses_selected[i] == t) {
		    for (k = i; k < N_ses_selected - 1; k++)
			Ses_selected[k] = Ses_selected[k + 1];
		    N_ses_selected--;
		    break;
		}
	    }
	    break;
	}
    }
}

/******************************************************************

    Calculates the total size of the selected volumes.
	
******************************************************************/

static int Get_selected_size () {
    int size, i;

    size = 0;
    for (i = 0; i < N_vol_files; i++) {
	int ses, k;

	ses = Vol_files[i].session;
	for (k = 0; k < N_ses_selected; k++) {
	    if (Ses_selected[k] == ses) {
		size += Vol_files[i].size;
		break;
	    }
	}
    }
    return (size);
}

/******************************************************************

    Calculates the number of volumes for any selected "sesesion".
	
******************************************************************/

static int Get_n_volumes (int sesesion) {
    int n_volumes, i;

    n_volumes = 0;
    for (i = 0; i < N_vol_files; i++) {
	if (Vol_files[i].session == sesesion)
	    n_volumes++;
    }
    return (n_volumes);
}

/******************************************************************

    Returns true (1) if the "ind"-th volume in Vol_files is 
    selected or false (0) otherwise.
	
******************************************************************/

static int Is_selected (int ind) {
    int ses, k;

    ses = Vol_files[ind].session;
    for (k = 0; k < N_ses_selected; k++) {
	if (Ses_selected[k] == ses)
	    return (1);
    }
    return (0);
}

/******************************************************************

    Writes the selected volume files in "Vol_files" to CD. 
    "total_size" is the total size of the selected volume files. 
    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Write_cd (int total_size) {
    char cmd[LOCAL_NAME_SIZE * 2 + 64], *dev;
    int size, speed, ret_value;

    if (total_size <= 0) {
	printf ("No volume file selected\n");
	return (-1);
    }
    else if (total_size > CD_MAX_SIZE) {
	printf ("Too many volume files selected\n");
	return (-1);;
    }

    Convert_to_bz2 = 0;
    if (strcmp (PAM_get_options (CONVERT_TO_BZ2), "bz2") == 0)
	Convert_to_bz2 = 1;
    sscanf (PAM_get_options (CDW_SPEED), "%d", &speed);
    dev = PAM_get_options (CDW_DEVICE);

    printf ("    CD device: %s, speed %d\n", dev, speed);
    printf ("    See %s for status info\n",
		PAF_get_full_path (PAP_get_work_dir (), CMD_ERR_FILE));

    printf ("Checking the CD drive...\n");
    sprintf (cmd, "cdrecord -checkdrive dev=%s", dev);
    if (PAW_execute_command (cmd) != 0)
	return (-1);

    printf ("Creating the CD image...\n");
    size = Create_cd_image (Vol_files, N_vol_files);
    if (size <= 0)
	return (size);

    sprintf (cmd, "cdrecord speed=%d dev=%s %s", speed, dev,
		PAF_get_full_path (PAP_get_work_dir (), "cd_image"));
    ret_value = 0;
    while (1) {
	int t;

	printf ("Writing CD ...\n");
	if (PAW_execute_command (cmd) != 0) {
	    printf ("Writing CD failed - See %s for further info\n",
		PAF_get_full_path (PAP_get_work_dir (), CMD_ERR_FILE));
	    ret_value = -1;
	    break;
	}

	printf (
	    "CD write completed - Enter 1 to duplicate the CD or 2 if done: ");
	while (sscanf (PAI_gets (), "%d", &t) != 1 || t < 1 || t > 2)
	    printf ("Bad input - try again (1 or 2): ");
	if (t == 2)
	    break;
    }

    sprintf (cmd, "%s -rf %s", CMD_RM, 
		PAF_get_full_path (PAP_get_work_dir (), "cd_image"));
    if (PAW_execute_command (cmd) != 0)
	return (-1);

    return (ret_value);
}

/******************************************************************

    Creates a CD image of all selected volume files. The image is
    work_dir/cd_image. We build temporary file links instead of 
    actually copying the volume files. It returns the total size
    of all volume files on the CD.
	
******************************************************************/

static int Create_cd_image (Ap_vol_file_t *Vol_files, int n_files) {
    char cmd[LOCAL_NAME_SIZE * 2 + 64], tmp_dir[LOCAL_NAME_SIZE + 32];
    int i, size;

    strcpy (tmp_dir, PAF_get_full_path (PAP_get_work_dir (), "cd_dir/"));
    sprintf (cmd, "%s -rf %s %s", CMD_RM, tmp_dir,
		PAF_get_full_path (PAP_get_work_dir (), "cd_image"));
    if (PAW_execute_command (cmd) != 0)
	return (-1);
    sprintf (cmd, "%s %s", CMD_MKDIR, tmp_dir);
    if (PAW_execute_command (cmd) != 0)
	return (-1);
    size = 0;
    for (i = 0; i < n_files; i++) {
	if (!Is_selected (i))
	    continue;
	size += Vol_files[i].size;
	if (size > CD_MAX_SIZE) {
	    printf ("Too many files for the CD space: Files not processed\n");
	    break;
	}
	if (Convert_to_bz2 && Vol_files[i].compress_type == AP_COMP_Z) {
	    char *tname;

	    sprintf (cmd, "%s %s %s", CMD_CP, Vol_files[i].path, 
			PAF_get_full_path (tmp_dir, Vol_files[i].name));
	    if (PAW_execute_command (cmd) != 0)
		return (-1);
	    sprintf (cmd, "uncompress %s", 
			PAF_get_full_path (tmp_dir, Vol_files[i].name));
	    if (PAW_execute_command (cmd) != 0)
		return (-1);
	    tname = PAF_get_full_path (tmp_dir, Vol_files[i].name);
	    tname[strlen (tname) - 2] = '\0';	/* remove .Z */
	    sprintf (cmd, "bzip2 %s", tname);
	    if (PAW_execute_command (cmd) != 0)
		return (-1);
	}
	else {			/* make a link */
	    sprintf (cmd, "%s -s %s %s", CMD_LN, Vol_files[i].path, 
			PAF_get_full_path (tmp_dir, Vol_files[i].name));
	    if (PAW_execute_command (cmd) != 0)
		return (-1);
	}
    }
    sprintf (cmd, "mkisofs -l -R -f -o %s %s", 
	    PAF_get_full_path (PAP_get_work_dir (), "cd_image"), tmp_dir);
    if (PAW_execute_command (cmd) != 0)
	return (-1);
    sprintf (cmd, "%s -rf %s", CMD_RM, tmp_dir);
    if (PAW_execute_command (cmd) != 0)
	return (-1);

    return (size);
}

/******************************************************************

    Execute command "cmd". The output of the command is piped into
    the play_a2 error file. Returns 0 on success or -1 on failure.
	
******************************************************************/

#define BUF_SIZE 4000

int PAW_execute_command (char *cmd) {
    static int fd = -1;
    char buf[BUF_SIZE];
    int ret, n_bytes;

    if (fd < 0) {
	char *name = PAF_get_full_path (PAP_get_work_dir (), CMD_ERR_FILE);
	fd = open (name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
	    fprintf (stderr, 
		"open %s for writing failed (errno %d)\n", name, errno);
	    return (-1);
	}
    }

    write (fd, cmd, strlen (cmd));
    write (fd, "\n", 1);
    ret = MISC_system_to_buffer (cmd, buf, BUF_SIZE, &n_bytes);
    if (n_bytes > 0)
	write (fd, buf, n_bytes);
    if (ret != 0) {
	fprintf (stderr, "Failed (%d) in executing \"%s\"\n", ret, cmd);
	return (-1);
    }
    return (0);
}



