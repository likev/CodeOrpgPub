
/******************************************************************

    This is the main module for exec_on_all_nodes.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/03/22 16:16:39 $
 * $Id: exec_on_all_nodes.c,v 1.3 2007/03/22 16:16:39 jing Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include <orpg.h>
#include <mrpg.h>
#include <infr.h>

static char *Cmd = NULL;

static void Print_usage (char **argv);
static int Read_options (int argc, char **argv);
static int Exec_cmd (char *ip, char *hname, char *site_info);


/******************************************************************

    The main function.

******************************************************************/

int main (int argc, char **argv) {
    int ret;

    if (Read_options (argc, argv) != 0)
	exit (1);

    ret = LE_init (argc, argv);
    if (ret < 0) {
	LE_send_msg (0, "LE_init failed (%d)", ret);
	exit (1);
    }

    ret = ORPGMGR_each_node (Exec_cmd);
    if (ret < 0)
	exit (1);
    exit (0);
}

#define BUF_SIZE 128

static int Exec_cmd (char *ip, char *hname, char *site_info) {
    int ret, fret, n;
    char buf[BUF_SIZE], func[BUF_SIZE];

    if (strncmp (site_info, "No ", 3) == 0) {
	LE_send_msg (0, "%s (%s): %s", ip, hname, site_info);
	return (0);
    }
    LE_send_msg (0, "%s (%s): %s", ip, hname, site_info);

    if (strcmp (ip, "local") == 0)
	sprintf (func, ":MISC_system_to_buffer");
    else
	sprintf (func, "%s:MISC_system_to_buffer", ip);
    LE_send_msg (0, "Run %s on %s", Cmd, ip);
    ret = RSS_rpc (func, "i-r s-i ba-%d-o i-i ia-1-o", BUF_SIZE, &fret, 
				Cmd, buf, BUF_SIZE, &n);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "RSS_rpc (%s) failed (%d)\n", func, ret);
	return (0);
    }
    if (fret < 0) {
	LE_send_msg (GL_ERROR, "%s failed (%d)\n", func, fret);
	return (0);
    }
    LE_send_msg (0, "Cmd (ret %d) output: %s\n", fret, buf);

    return (0);
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
    while ((c = getopt (argc, argv, "h?")) != EOF) {
	switch (c) {

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind == argc - 1)		/* get the command name  */
	Cmd = STR_copy (Cmd, argv[optind]);
    else {
	LE_send_msg (GL_ERROR, "Command not specified");
	exit (1);
    }

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    char *usage = "\
    Execute command \"cmd\" on all nodes defined in rssd configuration.\n\
        Options:\n\
	-h (Prints usage info.)\n\
";

    printf ("Usage:  %s [options] \"cmd cmd_options\"\n", argv[0]);
    printf ("%s\n", usage);

    exit (0);
}
