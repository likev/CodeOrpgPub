
/******************************************************************

	file: operate_rpg.c

	This is the main module for the operate_rpg program.
	
******************************************************************/

/* 
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2006/08/23 20:27:30 $
 * $Id: operate_rpg.c,v 1.13 2006/08/23 20:27:30 cmn Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define OR_NAME_SIZE 128
#define MAX_N_UIDS 256		/* max number of RPG operator accounts */

static char Config_file_name[OR_NAME_SIZE];
				/* operate_rpg config file name */
static char Cmd_label[OR_NAME_SIZE] = "";
				/* command label */
static int Verbose = 0;		/* verbose mode is on */

static int RPG_uid = -1;		/* uid of RPG account */
static int RPG_gid = -1;		/* gid of RPG account */
static char RPG_home[OR_NAME_SIZE] = "";
					/* home path of RPG account */
static char Labeled_cmd[OR_NAME_SIZE] = "";
					/* The labeled command */
static int Uids[MAX_N_UIDS];		/* list of RPG operator account uids */
static int N_uids = 0;			/* size of Uids list */

static FILE *Err_fl;			/* error reporting file handle */


static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static int Read_config_file ();
static int Check_file (char *file_name);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    int uid, i, fd;
    char buf[OR_NAME_SIZE + 128];
    char *log_file;
    FILE *log_fl;
    time_t t;

    Err_fl = stderr;

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (0);

    /* open log file */
    log_file = "/var/log/operate_rpg.log";
    if (Check_file (log_file) != 0)
	exit (1);
    fd = open (log_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0 || (log_fl = fdopen (fd, "a")) == NULL) {
	fprintf (Err_fl, "Can not open log file %s\n", log_file);
	exit (1);
    }
    if (strlen (Cmd_label) > 0)
	Err_fl = log_fl;

    if (Read_config_file () != 0)
	exit (1);

    uid = getuid ();
    for (i = 0; i < N_uids; i++)
	if (uid == Uids[i])
	    break;
    if (i >= N_uids) {
	fprintf (Err_fl, "User permission denied\n");
	exit (1);
    }

    if (setgid (RPG_gid) < 0) {
	fprintf (Err_fl, "setgid failed\n");
	exit (1);
    }
    if (setuid (RPG_uid) < 0) {
	fprintf (Err_fl, "setuid failed\n");
	exit (1);
    }
    sprintf (buf, "HOME=%s", RPG_home);
    putenv (buf);

    t = time (NULL);
    if (strlen (Cmd_label) > 0) {
	sprintf (buf, "HOME=%s; ", RPG_home);
	strcat (buf, "export HOME; cd $HOME; /bin/bash --login -c \"");
	fprintf (log_fl, "%s  User %d starts cmd: %s\n", 
						ctime (&t), uid, Cmd_label);
	strcat (buf, Labeled_cmd);
	strcat (buf, "\"");
	fprintf (log_fl, "%s  Run command: %s\n", ctime (&t), buf);
	system (buf);
    }
    else {
	fprintf (log_fl, "%s  User %d starts operating RPG\n", 
						ctime (&t), uid);
	fflush (log_fl);
	system ("/bin/bash --login");
	t = time (NULL);
	fprintf (log_fl, "%s  User %d stops operating RPG\n", 
						ctime (&t), uid);
	fflush (log_fl);
    }

    exit (0);
}

/******************************************************************

    Checks file ownership and permission.

    Returns 0 on success or -1 on failure.

******************************************************************/

static int Check_file (char *file_name) {
    struct stat st;

    if (stat (file_name, &st) == -1) {
	if (errno == ENOENT)
	    return (0);
	fprintf (stderr, "Failed in finding file (%s) status\n", file_name);
	return (-1);
    }
    if (st.st_uid != 0) {
	fprintf (stderr, "File (%s) not owned by root\n", file_name);
	return (-1);
    }
    if ((st.st_mode & 033)!= 0) {
	fprintf (stderr, "File (%s) permission problem\n", file_name);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Reads the configuration file.

    Returns 0 on success or -1 on failure.

******************************************************************/

#define BUF_SIZE 256

static int Read_config_file () {
    FILE *fl;
    char buf[BUF_SIZE];

    if (Check_file (Config_file_name) != 0)
	return (-1);

    fl = fopen (Config_file_name, "r");
    if (fl == NULL) {
	fprintf (Err_fl, "Can not open file %s\n", Config_file_name);
	return (-1);
    }

    while (fgets (buf, BUF_SIZE, fl) != NULL) {
	char *cpt;
	char saved[BUF_SIZE];
	int uid;

	strcpy (saved, buf);
	cpt = strtok (buf, " \n\t");
	if (cpt == NULL || cpt[0] == '#')
	    continue;

	if (strcmp (cpt, "RPG_uid") == 0) {
	    cpt = strtok (NULL, " \n\t");
	    if (cpt == NULL || sscanf (cpt, "%d", &RPG_uid) != 1) {
		fprintf (Err_fl, "Bad RPG_uid line in %s\n", Config_file_name);
		return (-1);
	    }
	}
	else if (strcmp (cpt, "RPG_gid") == 0) {
	    cpt = strtok (NULL, " \n\t");
	    if (cpt == NULL || sscanf (cpt, "%d", &RPG_gid) != 1) {
		fprintf (Err_fl, "Bad RPG_gid line in %s\n", Config_file_name);
		return (-1);
	    }
	}
	else if (strcmp (cpt, "RPG_home") == 0) {
	    cpt = strtok (NULL, " \n\t");
	    if (cpt == NULL || strlen (cpt) == 0) {
		fprintf (Err_fl, 
			"Bad RPG_home line in %s\n", Config_file_name);
		return (-1);
	    }
	    strncpy (RPG_home, cpt, OR_NAME_SIZE);
	    RPG_home[OR_NAME_SIZE - 1] = '\0';
	}
	else if (strstr (cpt, "_command") != NULL) {
	    cpt[strlen (cpt) - 8] = '\0';
	    if (Cmd_label[0] == '\0' || strcmp (cpt, Cmd_label) != 0)
		continue;
	    cpt = strtok (NULL, " \n\t");
	    if (cpt == NULL || strlen (cpt) == 0) {
		fprintf (Err_fl, "Bad command (%s_command) in %s\n", 
					Cmd_label, Config_file_name);
		return (-1);
	    }
	    cpt = strstr (saved, Cmd_label);
	    cpt += strlen (Cmd_label) + 9;
	    while (*cpt != '\0' && (*cpt == ' ' || *cpt == '\t'))
		cpt++;
	    strncpy (Labeled_cmd, cpt, OR_NAME_SIZE);
	    Labeled_cmd[OR_NAME_SIZE - 1] = '\0';
	    cpt = Labeled_cmd + strlen (Labeled_cmd) - 1;
	    while (*cpt == '\n' || *cpt == ' ' || *cpt == '\t')
		*cpt-- = '\0';
	}
	else if (strcmp (cpt, "Operator_uid") == 0) {
	    cpt = strtok (NULL, " \n\t");
	    if (cpt == NULL || sscanf (cpt, "%d", &uid) != 1) {
		fprintf (Err_fl, 
			"Bad Operator_uid line in %s\n", Config_file_name);
		return (-1);
	    }
	    if (N_uids >= MAX_N_UIDS) {
		fprintf (Err_fl, "Too many RPG users - %d discarded\n", uid);
	    }
	    else {
		Uids[N_uids] = uid;
		N_uids++;
	    }
	}
	else {
	    fprintf (Err_fl, 
			"Unexpected key %s in %s\n", cpt, Config_file_name);
	    return (-1);
	}
    }
    if (RPG_uid < 0) {
	fprintf (Err_fl, "RPG_uid not found in %s\n", Config_file_name);
	return (-1);
    }
    if (RPG_gid < 0) {
	fprintf (Err_fl, "RPG_gid not found in %s\n", Config_file_name);
	return (-1);
    }
    if (strlen (RPG_home) == 0) {
	fprintf (Err_fl, "RPG_home not found in %s\n", Config_file_name);
	return (-1);
    }
    if (strlen (Cmd_label) > 0 && strlen (Labeled_cmd) == 0) {
	fprintf (Err_fl, "%s_command not found in %s\n", 
						Cmd_label, Config_file_name);
	return (-1);
    }
    if (N_uids == 0 && Verbose)
	printf ("No RPG operator account specified\n");
    fclose (fl);
    return (0);
}




/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;	/* used by getopt */
    extern int optind;
    int c;			/* used by getopt */
    int err;			/* error flag */

    err = 0;
    strcpy (Config_file_name, "/etc/operate_rpg.conf");
    Verbose = 0;
    while ((c = getopt (argc, argv, "bhf:c:v?")) != EOF) {
	switch (c) {

	    case 'f':
		strncpy (Config_file_name, optarg, OR_NAME_SIZE);
		Config_file_name[OR_NAME_SIZE - 1] = '\0';
		break;

	    case 'c':
		strncpy (Cmd_label, optarg, OR_NAME_SIZE);
		Cmd_label[OR_NAME_SIZE - 1] = '\0';
		break;

	    case 'b':
		strcpy (Cmd_label, "RPG_boot_start");
		break;

	    case 'v':
		Verbose = 1;
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
    printf ("Usage: %s (options)\n", argv[0]);
    printf ("       Options:\n");
    printf ("       -f config_file_name (default: /etc/operate_rpg.conf)\n");
    printf ("       -b (called from OS init after reboot)\n");
    printf ("       -c label (run command lead by \"lable\"_command)\n");
    printf ("       -v (verbose mode)\n");
    exit (0);
}

