
/******************************************************************

    This is the main module for save_adapt.
	
******************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/09/25 17:02:37 $
 * $Id: save_adapt.c,v 1.26 2009/09/25 17:02:37 ccalvert Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <orpg.h>
#include <orpgadpt.h>
#include <infr.h>
#include <medcp.h>

#define BUF_SIZE 512
#define CMD_SIZE 256

/* command line options */
static char *Dest_dir = NULL;
static char *Spec_channel = "";
static int Verbose = 1;
static int To_cd = 0;
static int To_floppy = 0;
static int Rm_files = 0;
static char *Tmp_dir = NULL;
static int Local_host = 0;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Test_dest_file (char *fname);
static char *Get_stdin ();
static int Copy_to_media (char *src);
static int Run_command (char *cmd);
static void Term_on_error (int);
static void Cleanup_tmp_dir ();


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {
    int ret, chan, local_rpg;
    char mrpg_hn[ADPTU_NAME_SIZE], *name;
    static char *func = NULL, *fname = NULL, *src = NULL, *cp_dest, *medcp_src;

    if (Read_options (argc, argv) != 0)
	exit (1);

    if ((ret = LE_init (argc, argv)) < 0) {
	LE_send_msg (0, "LE_init failed (%d)", ret);
	Term_on_error (1);
    }

    if (strcasecmp (Spec_channel, "mscf") == 0) {
	LE_send_msg (0, "No adaptation data on mscf to save");
	Term_on_error (1);
    }
    else if (strlen (Spec_channel) > 0 &&
	     strcasecmp (Spec_channel, "rpg1") != 0 &&
	     strcasecmp (Spec_channel, "rpg2") != 0) {
	LE_send_msg (0, "Unexpected RPG channel (%s) specified", Spec_channel);
	Term_on_error (1);
    }

    chan = 0;
    if (Local_host) {
	if (strlen (Spec_channel) == 0 || 
	    strcasecmp (Spec_channel, "rpg1") == 0)
	    chan = 1;
	else
	    chan = 2;
	mrpg_hn[0] = '\0';
    }
    else {
	if (strlen (Spec_channel) == 0)
	    chan = ORPGMGR_search_mrpg_host_name (0, mrpg_hn, ADPTU_NAME_SIZE);
	if (chan <= 0) {
	    if (strlen (Spec_channel) == 0 ||
		strcasecmp (Spec_channel, "rpg1") == 0)
		chan = ORPGMGR_search_mrpg_host_name (1, mrpg_hn, 
							ADPTU_NAME_SIZE);
	    else if (strcasecmp (Spec_channel, "rpg2") == 0)
		chan = ORPGMGR_search_mrpg_host_name (2, mrpg_hn, 
							ADPTU_NAME_SIZE);
	}
    }
    if (chan <= 0) {
	LE_send_msg (0, "specified RPG (%s) not found", Spec_channel);
	Term_on_error (1);
    }
    LE_send_msg (0, "Saving RPG adaptation data (on %s) ...\n", mrpg_hn);

    func = STR_gen (func, mrpg_hn, ":liborpg.so,ORPGADPTU_save_adapt", NULL);
    RMT_time_out (60);
    ret = RSS_rpc (func, "s-r", &name);
    if (ret < 0) {
	LE_send_msg (0, "RSS_rpc %s failed (%d)", func, ret);
	Term_on_error (1);
    }
    if (name == NULL) {
	LE_send_msg (0, "%s failed", func);
	Term_on_error (1);
    }
    fname = STR_copy (fname, name);
    local_rpg = 0;
    if (NET_get_ip_by_name (mrpg_hn) == NET_get_ip_by_name (""))
	local_rpg = 1;

    cp_dest = NULL;
    medcp_src = NULL;
    src = STR_gen (src, mrpg_hn, ":", fname, NULL);
    if (strlen (mrpg_hn) == 0)
	strcpy (mrpg_hn, "local host");
    if (To_cd || To_floppy) {
	if (local_rpg)
	    medcp_src = fname;
	else {
	    char b[256];
	    if ((ret = MISC_get_tmp_path (b, 256)) <= 0) {
		LE_send_msg (0, "MISC_get_tmp_path failed (%d)", ret);
		Term_on_error (1);
	    }
	    cp_dest = STR_gen (cp_dest, b, "/", MISC_basename (fname), NULL);
	    medcp_src = cp_dest;
	    if (MISC_mkdir (b) < 0) {
		LE_send_msg (0, "Cannot make DIR %s", b);
		Term_on_error (1);
	    }
	    Tmp_dir = STR_copy (Tmp_dir, b);
	}
    }
    else {
	if (Dest_dir != NULL) {
	    if (Local_host) {
		cp_dest = STR_gen (cp_dest, Dest_dir, 
			ORPGADPTU_create_archive_name (Spec_channel, 
			MISC_basename (Dest_dir), -1, 0), ".Z", NULL);
	    }
	    else
		cp_dest = STR_gen (cp_dest, Dest_dir, 
						MISC_basename (fname), NULL);
	}
	if (Dest_dir == NULL ||
	    (local_rpg && strcmp (cp_dest, fname) == 0)) {
	    LE_send_msg (0, "No copy needed - save_adapt completed");
	    if (Verbose)
		printf ("save_adapt RPG on %s (chan %d) completed\n", mrpg_hn, chan);
	    exit (0);
	}
	if (MISC_mkdir (Dest_dir) < 0) {
	    LE_send_msg (0, "Cannot make DIR %s", Dest_dir);
	    Term_on_error (1);
	}
    }
    if (cp_dest != NULL) {
	ret = RSS_copy (src, cp_dest);		/* copy adapt archive over */
	if (ret < 0) {
	    LE_send_msg (0, "RSS_copy from %s to %s failed", src, cp_dest);
	    Term_on_error (1);
	}
	LE_send_msg (0, "File %s copied to %s", src, cp_dest);
	Test_dest_file (cp_dest);
    }
    if (medcp_src != NULL) {			/* copy to the media */
	LE_send_msg (0, "Copy %s to the media", medcp_src);
	ret = Copy_to_media (medcp_src);
	if (ret != 0) {
	    LE_send_msg (0, "Failed in writing the media\n");
	    Term_on_error (ret);
	}
    }
    else
	LE_send_msg (0, "Saved adaptation data into %s\n", Dest_dir);

    Cleanup_tmp_dir ();
    LE_send_msg (0, "save_adapt completed");

    if (Verbose)
	printf ("save_adapt (RPG on %s, chan %d) completed\n", mrpg_hn, chan);
    exit (0);
}

/**************************************************************************

    Termanates this process on error.

**************************************************************************/

static void Term_on_error (int err) {
    Cleanup_tmp_dir ();
    if (Verbose)
	printf ("save_adapt failed - See save_adapt log (lem save_adapt)\n");
    exit (err);
}

/**************************************************************************

    Removes the tmp directory.

**************************************************************************/

static void Cleanup_tmp_dir () {

    if (Tmp_dir != NULL && Tmp_dir[0] != '\0') {
	char cmd[CMD_SIZE];
	sprintf (cmd, "rm -rf %s", Tmp_dir);
	Run_command (cmd);
    }
    Tmp_dir = STR_copy (Tmp_dir, "");
}

/**************************************************************************

    Copy to CD or floppy.

**************************************************************************/

static int Copy_to_media (char *src) {
    char *device, cmd[CMD_SIZE];
    int ret;

    if (To_cd)
	device = "cd";
    else
	device = "floppy";
    if (Verbose) {
	printf ("--->Saving Adaptation Data\n");
	printf ("--->Insert a new %s into the drive\n", device);
	printf ("--->Hit return when ready\n");
	Get_stdin ();
	printf ("--->Saving RPG adaptation data to %s\n", device);
    }

    if (Rm_files) {
	sprintf (cmd, "medcp -l save_adapt -b %s", device);
	ret = Run_command (cmd);
	if (ret != 0) {
	    LE_send_msg (0, 
		    "Blanking %s failed - Check save_adapt log.\n", device);
	    return (ret);
	}
    }

    sprintf (cmd, "medcp -ce -l save_adapt %s %s", src, device);
    ret = Run_command (cmd);
    if (ret != 0 && ret != MEDCP_EJECT_T_FAILED ) {
	LE_send_msg (0, 
		"Copying to %s failed - Check save_adapt log.\n", device);
	return (ret);
    }
    return (0);
}

/*******************************************************************

    Reads standard input up to 256 characters. It discards any 
    remaining characters in the buffer before reading.

*******************************************************************/

static int Run_command (char *cmd) {
    char buf[BUF_SIZE];
    int n_bytes, ret;

    n_bytes = 0;
    ret = ( MISC_system_to_buffer (cmd, buf, BUF_SIZE, &n_bytes) >> 8 );
    if (n_bytes > 0)
	LE_send_msg (0, "%ms", buf);
    return (ret);
}

/*******************************************************************

    Reads standard input up to 256 characters. It discards any 
    remaining characters in the buffer before reading.

*******************************************************************/

static char *Get_stdin () {
    static char buf[256];

    fseek (stdin, 0, 2);
    buf[0] = '\0';
    fgets (buf, 256, stdin);
    return (buf);
}

/**************************************************************************

    Tests the integrity of file "fname".

**************************************************************************/

static void Test_dest_file (char *fname) {
    int fd;

    fd = open (fname, O_RDONLY);
    if (fd < 0) {
	LE_send_msg (0, "Opening %s (test saved file) failed", fname);
	Term_on_error (1);
    }
    while (1) {
	char buf[512];
	int s = read (fd, buf, 512);
	if (s < 0) {
	    LE_send_msg (0, "Reading %s (test saved file) failed", fname);
	    Term_on_error (1);
	}
	if (s < 512)
	    break;
    }
    close (fd);
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
    while ((c = getopt (argc, argv, "D:o:lcfrhq?")) != EOF) {
	switch (c) {

	    case 'D':
		Dest_dir = STR_copy (Dest_dir, optarg);
		if (Dest_dir[strlen (Dest_dir) - 1] != '/')
		    Dest_dir = STR_cat (Dest_dir, "/");
		break;

	    case 'o':
		Spec_channel = optarg;
		break;

	    case 'l':
		Local_host = 1;
		break;

	    case 'c':
		To_cd = 1;
		break;

	    case 'f':
		To_floppy = 1;
		break;

	    case 'r':
		Rm_files = 1;
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

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
    Saves the RPG adaptation data into an archive file and puts it in \n\
    $CFG_DIR/adapt/installed. This command also removes all existing adapt \n\
    archive files in it. If a local destination dir, or device, is specified\n\
    with the -D, -f or -c option, the archive file is also copied to that \n\
    dir or device. This command can be invoked on any of the connected nodes\n\
    (mscf, rpg1 etc).\n\
    Options:\n\
	-D dir (Specifies the directory where the RPG adapt archive file is\n\
	   stored. Default: The current directory)\n\
	-o channel (rpg1 or rpg2. Default: The RPG on the local host. If the\n\
	   local host is not an RPG node, rpg1 is assumed)\n\
        -l (Assume the local host is the required \"channel\")\n\
        -c (Save the adaptation data to the CD on the local host)\n\
        -f (Save the adaptation data to the diskette on the local host)\n\
        -r (Remove all files on the floppy or CD before saving the data)\n\
	-q (Quiet mode)\n\
	-h (Print usage info)\n\
";

    printf ("Usage:  %s [options]\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}


