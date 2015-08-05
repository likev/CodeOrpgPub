
/******************************************************************

	file: cmp_main.c

	This is the main module for the cm_ping task - a  
	Internet connection monitor.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2001/01/10 20:25:18 $
 * $Id: cmp_main.c,v 1.24 2001/01/10 20:25:18 jing Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h>
#include <cmp_def.h>
#include <orpgerr.h>
#include <cm_ping.h>

int Verbose;				/* verbose mode */

static char In_name[NAME_SIZE];		/* Input LB name */
static char Out_name[NAME_SIZE];	/* output LB name */
static char Cm_ping_lb_dir[NAME_LEN + 4] = "";
				/* cm_ping LB dir */

static int Test_period;

static int Background;			/* folk a child and run in background */
static int Init_only;			/* exits after initialization */
static int No_init;			/* run in foreground without 
					   initialization phase */

enum {INPUT_LB, OUTPUT_LB};		/* argument values for Get_LB_name */

/* local functions */
static int Read_options (int argc, char **argv);
static void Read_input ();
static void Goto_background ();
static int Create_and_clear_LBs ();
static char *Get_LB_name (int which);


/******************************************************************

    Description: The main function of comm_manager.

    Input:	argc - argument count;
		argv - argument array;

******************************************************************/

int main (int argc, char **argv)
{
    int ret;

    LE_send_msg (LE_VL3, "cm_ping starts");
    if (Read_options (argc, argv) < 0)
	exit (1);

    if ((ret = LE_create_lb (argv[0], 200, LB_SINGLE_WRITER, -1)) < 0) {
	LE_send_msg (GL_ERROR,  "LE_create_lb failed (ret %d)", ret);
	exit (1);
    }
    if ((ret = LE_init (argc, argv)) < 0) {
	LE_send_msg (GL_ERROR | 1001,  "LE_init failed, ret %d", ret);
	exit (1);
    }

    if (Verbose)
	LE_local_vl (3);

    if (strlen (Cm_ping_lb_dir) == 0) {
	if (MISC_get_work_dir (Cm_ping_lb_dir, NAME_LEN - 1) < 0) {
	    printf ("failed in getting default work directory path\n");
	    exit (1);
	}
	else
	    strcat (Cm_ping_lb_dir, "/");
	LE_send_msg (LE_VL1, "Cm_ping_lb_dir %s\n", Cm_ping_lb_dir);
    }

    /* clear input LB */
    if (!No_init && Create_and_clear_LBs () < 0)
	exit (1);
    if (Init_only)
	exit (0);

    /* goto back ground - to make sure no process to be monitored can start
	before clearing up the input LB */
    if (Background)
        Goto_background ();

    /* open the LB to exclude another instance */
    CMP_output (NULL, 0);

    if (CMP_initialize (Test_period) < 0)
	exit (1);

    while (1) {

	Read_input ();
	CMPT_check_TCP ();
	CMPP_check_processes ();
	sleep (Test_period);
    }
}

/**************************************************************************

    Description: This function terminates this program.

**************************************************************************/

int CMP_terminate ()
{

    LE_send_msg (GL_STATUS | 1002,  "cm_ping terminates");
    exit (0);
}

/**************************************************************************

    Description: This function reads in any inputs.

**************************************************************************/

static void Read_input ()
{
    static int fd = -1;
    char msg[CMP_MAX_INP_MSG_SIZE];
    int len;

    if (fd < 0) {
	fd = LB_open (Get_LB_name (INPUT_LB), LB_READ, NULL);
	if (fd < 0) {
	    LE_send_msg (GL_ERROR | 1003,  
			"LB_open cm_ping input failed (ret %d)", fd);
	    CMP_terminate ();
	}
	LB_seek (fd, 0, LB_FIRST, NULL);
    }

    while ((len = LB_read (fd, msg, CMP_MAX_INP_MSG_SIZE, LB_NEXT)) >= 0) {
	int type;

	if (len >= (int)sizeof (int))
	    type = *((int *)msg);
	else
	    type = 0;
	switch (type) {

	    case CMP_IN_TCP_MON:
		CMPT_process_input (msg, len);
		break;

	    case CMP_IN_PROC_MON:
		CMPP_process_input (msg, len);
		break;

	    default:
		LE_send_msg (GL_ERROR | 1004,  
			"bad input msg (len %d, type %d)", len, type);
		break;
	}

    }

    if (len != LB_TO_COME) 
	LE_send_msg (GL_ERROR | 1005,  "LB_read failed (ret %d)", len);

    return;
}

/**************************************************************************

    Description: This function writes a message to the cm_ping output
		LB.

    Input:	buf - buffer holding the message to output.
		size - size of the message in "buf".

**************************************************************************/

void CMP_output (char *buf, int size)
{
    static int fd = -1;
    int ret;

    if (fd < 0) {
	fd = LB_open (Get_LB_name (OUTPUT_LB), LB_WRITE, NULL);
	if (fd < 0) {
	    LE_send_msg (GL_ERROR | 1006,  
			"LB_open cm_ping output failed (ret %d)", fd);
	    CMP_terminate ();
	}
    }

    if (size == 0)
	return;
    ret = LB_write (fd, buf, size, LB_ANY);
    if (ret != size)
	LE_send_msg (GL_ERROR | 1007,  "LB_write failed (ret %d)", ret);

    return;
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    strcpy (In_name, "cmp_in.lb");
    strcpy (Out_name, "cmp_out.lb");
    strcpy (Cm_ping_lb_dir, "");
    Background = 1;
    Init_only = 0;
    No_init = 0;
    Test_period = 5;
    Verbose = 0;
    err = 0;
    while ((c = getopt (argc, argv, "i:o:p:d:vhtab?")) != EOF) {
	switch (c) {

	    case 'i':
		if (sscanf (optarg, "%s", In_name) != 1)
		    err = 1;
		break;

	    case 'o':
		if (sscanf (optarg, "%s", Out_name) != 1)
		    err = 1;
		break;

	    case 'p':
		if (sscanf (optarg, "%d", &Test_period) != 1 ||
		    Test_period <= 0)
		    err = 1;
		break;

	    case 'd':
		strncpy (Cm_ping_lb_dir, optarg, NAME_LEN);
		Cm_ping_lb_dir[NAME_LEN - 1] = 0;
		if (Cm_ping_lb_dir[strlen (Cm_ping_lb_dir) - 1] != '/')
		    strcat (Cm_ping_lb_dir, "/");
		break;

	    case 'v':
		Verbose = 1;
		break;

	    case 't':
		Background = 0;
		break;

	    case 'a':
		Background = 0;
		Init_only = 1;
		break;

	    case 'b':
		Background = 0;
		No_init = 1;
		break;

	    case 'h':
	    case '?':
		err = 1;
		break;
	}
    }

    if (err == 1) {              /* Print usage message */
	printf ("Usage: %s [options]\n", argv[0]);
	printf ("       options:\n");
	printf ("       -p test_period (in seconds; default: 5);\n");
	printf ("       -i input_lb_name (default: cmp_in.lb);\n");
	printf ("       -o output_lb_name (default: cmp_out.lb);\n");
	printf ("       -d LB dir (directory for input_lb_name and output_lb_name\n");
	printf ("          The default is the project work dir (See MISC));\n");
	printf ("       -t (test mode - running in foreground);\n");
	printf ("       -a (mode a - terminates after initialization);\n");
	printf ("       -b (mode b - runs in foreground without initialization);\n");
	printf ("           The default is mode c - runs in background after initialization;\n");
	printf ("       -v (verbose mode);\n");
	printf ("       -h (print usage info);\n");

	return (-1);
    }

    return (0);
}

/******************************************************************

	This function returns the input/output LB name.

    Input:	which - INPUT_LB or OUTPUT_LB.

    Return:	The LB name.

******************************************************************/

static char *Get_LB_name (int which)
{
    static char name[2 * NAME_LEN + 12];

    if (which == INPUT_LB) {
	strcpy (name, Cm_ping_lb_dir);
	strcat (name, In_name);
    }
    else {
	strcpy (name, Cm_ping_lb_dir);
	strcat (name, Out_name);
    }
    return (name);
}

/******************************************************************

	This function creates the cm_ping input and output LBs if
	they do not exist. It also removes all messages in the input
	LB.

    Return:	0 on success or -1 on failure.

******************************************************************/

static int Create_and_clear_LBs ()
{
    char *name;
    int fd, ret;

    /* create the output LB */
    fd = LB_open ((name = Get_LB_name (OUTPUT_LB)), LB_WRITE, NULL);
    if (fd < 0) {
	LB_attr attr;

	if (fd == LB_TOO_MANY_WRITERS) {
	    LE_send_msg (GL_ERROR | 1008,  "another cm_ping is running - terminate");
	    return (-1);
	}

	if (fd != LB_OPEN_FAILED) {
	    LE_send_msg (GL_ERROR | 1009,  
			"LB_open %s (existing) failed (ret %d)", name, fd);
	    return (-1);
	}
	LE_send_msg (LE_VL3, "Creating output LB %s", name);
	strcpy (attr.remark, "cm_ping output");
	attr.mode = 0666;
	attr.msg_size = 160;
	attr.maxn_msgs = 100;
	attr.types = LB_SINGLE_WRITER;
	attr.tag_size = 0;
	fd = LB_open (name, LB_CREATE, &attr);
	if (fd < 0) {
	    LE_send_msg (GL_ERROR | 1010,  
			"LB_open %s (create) failed (ret %d)", name, fd);
	    return (-1);
	}
    }
    LB_close (fd);

    /* create the input LB */
    fd = LB_open ((name = Get_LB_name (INPUT_LB)), LB_WRITE, NULL);
    if (fd < 0) {
	LB_attr attr;

	if (fd != LB_OPEN_FAILED) {
	    LE_send_msg (GL_ERROR | 1011,  
			"LB_open %s (existing) failed (ret %d)", name, fd);
	    return (-1);
	}
	LE_send_msg (LE_VL3, "Creating input LB %s", name);
	strcpy (attr.remark, "cm_ping input");
	attr.mode = 0666;
	attr.msg_size = CMP_MAX_INP_MSG_SIZE;
	attr.maxn_msgs = 100;
	attr.types = 0;
	attr.tag_size = 0;
	fd = LB_open (name, LB_CREATE, &attr);
	if (fd < 0) {
	    LE_send_msg (GL_ERROR | 1012,  
			"LB_open %s (create) failed (ret %d)", name, fd);
	    return (-1);
	}
    }

    LE_send_msg (LE_VL3, "Clearing all messages in input LB %s", name);
    ret = LB_clear (fd, LB_ALL);
    if (ret < 0)
	LE_send_msg (GL_ERROR, "LB_clear failed (ret %d)", ret);
    LB_close (fd);

    return (0);
}

/******************************************************************

	This function puts the calling process into the background
	by forking a child and exiting. It closes the standard input
	standard output and standard error ports before forking the
	child. If it fails to fork a child, it will print an error 
	message and exit.

******************************************************************/

static void Goto_background ()
{

    switch (fork ()) {
    case -1:			/* error in fork */
	printf ("Failed in Goto_background - fork\n");
	exit (1);
    case 0:
	/* we must close these to allow rsh to run rmtd in the background */
	close (0);		
	close (1);
	close (2);

	/* make sure that the first 3 fds are occupied. Rsh may write to those 
	   fds and generate errors. The user applications started by the rmtd
	   will not have standard io. However this problem can be fixed by using 
	   a shell (sh -c "exec application") */
	open ("/dev/null", O_RDONLY, 0);
	open ("/dev/null", O_RDONLY, 0);
	open ("/dev/null", O_RDONLY, 0);
	return;			/* child */
    default:
	exit (0);
    }
}


