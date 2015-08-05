
/******************************************************************

	This is the main module for the sdmqm program - the Simple 
	Data Base Query Manager.
	
******************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/30 16:55:17 $
 * $Id: sdqs_main.c,v 1.17 2012/07/30 16:55:17 jing Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */


#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 

#include "sdqs_def.h"


#define SDQS_NAME_SIZE 128
#define MAX_PORT_INC 4

static char *Prog_name;				/* name of this program */
static char Conf_name[SDQS_NAME_SIZE] = "";	/* config file name */
static char Environ_name[SDQS_NAME_SIZE] = "";	/* environment name */
static char Shared_lib_name[SDQS_NAME_SIZE] = "sdqs";
						/* shared library name */
static char Sys_cfg_name[SDQS_NAME_SIZE] = "";

int Verbose;					/* verbose mode */
int Allow_missing_LBs = 0;
static int Log_size = 300;			/* size of the log file */
static int No_compilation;			/* run without compilation */
static int Display_on_stderr;			/* also print msgs on stderr */
static int Compile_only;			/* terminate after compiling */
static char Port_number[SDQS_NAME_SIZE] = "";	/* query port number */
static int Port_opt = -1;			/* port number from option */
static int Run_seconds;

static int In_termination_phase = 0;
static int St_time;

/* local functions */
static void Print_cs_error (char *msg);
static void Print_usage (char **argv);
static int Read_options (int argc, char **argv);
static void Signal_handler (int sig);
static int Process_client_request (char *req, int req_len, char **resp);
static int House_keeping ();
static void Ntf_fd_ready (int fd, int ready_flag);
static void Redirect_to_le (char *msg);


/******************************************************************

    Description: The main function.

******************************************************************/

int main (int argc, char **argv) {
    int ret;
    char addr[64];

    LB_NTF_control (LB_NTF_SIGNAL, LB_NTF_NO_SIGNAL);

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (1);
    Prog_name = argv[0];

    if (Sys_cfg_name[0] != '\0')
	CS_set_sys_cfg (Sys_cfg_name);

    /* Initialize the LE and CS services */
    if (Verbose)
	LE_local_vl (3);
    if (Log_size > 0) {
	char buf[256];
	strcpy (buf, MISC_basename (argv[0]));
	if (Port_opt > 0 && Port_opt <= MAX_PORT_INC)
	    sprintf (buf + strlen (buf), ".%d", Port_opt);
	LE_set_option ("LE name", buf);
	LE_set_option ("LB size", Log_size);
	LE_set_option ("LB type", LB_SINGLE_WRITER);
	ret = LE_init (argc, argv);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, "LE_init failed (ret %d)", ret);
	    exit (1);
	}
    }
    CS_error (Print_cs_error);	/* asking CS to print error via 
				   Print_cs_error */
    MISC_log_reg_callback (Redirect_to_le);
    if (Display_on_stderr)
	LE_also_print_stderr (1);

    sigset (SIGTERM, Signal_handler); 
    sigset (SIGHUP, Signal_handler);
    sigset (SIGINT, Signal_handler); 
    sigignore (SIGPIPE);

    if (!No_compilation) {
	SDQSC_read_conf_file (Conf_name);
	SDQSC_generate_code ();
    }
    if (Compile_only)
	exit (0);
    SDQSO_init (No_compilation);
    St_time = MISC_systime (NULL);

    /* the main loop */
    if ((ret = CSS_add_poll_fd (LB_NTF_control (LB_GET_NTF_FD), 
				POLL_IN_FLAGS, Ntf_fd_ready)) < 0) {
	LE_send_msg (GL_ERROR, "CSS_add_poll_fd failed (ret %d)", ret);
	exit (1);
    }
    sprintf (addr, "%d", SDQSO_get_css_port ());
    CSS_sv_main (addr, MAXN_CONNS, 2, Process_client_request, House_keeping);

    exit (0);
}

/**************************************************************************

    Redirects the libinfr error message to LE.

**************************************************************************/

static void Redirect_to_le (char *msg) {
    LE_send_msg (0, "%s", msg);
}

/**************************************************************************

    Returns Environ_name.

**************************************************************************/

char *SDQSM_get_environ_name () {
    return (Environ_name);
}

/**************************************************************************

    Returns DL library base name.

**************************************************************************/

char *SDQSM_get_shared_lib_basename () {
    static char *tmp = NULL;
    if (tmp == NULL) {
	tmp = SDQSM_malloc (strlen (Shared_lib_name) + 32);
	strcpy (tmp, Shared_lib_name);
	if (Port_opt > MAX_PORT_INC)
	    sprintf (tmp + strlen (tmp), "_%d", Port_opt);
    }
    return (tmp);
}

/**************************************************************************

    Returns DL library name.

**************************************************************************/

char *SDQSM_get_shared_lib_name () {
    static char *tmp = NULL;
    if (tmp == NULL) {
	char *base_name = SDQSM_get_shared_lib_basename ();
	tmp = SDQSM_malloc (strlen (base_name) + 16);
	sprintf (tmp, "lib%s.so", base_name);
    }
    return (tmp);
}

/**************************************************************************

    Returns SMI_get_info function name.

**************************************************************************/

char *SDQSM_get_smi_func_name () {
    static char *tmp = NULL;
    if (tmp == NULL) {
	tmp = SDQSM_malloc (strlen (Shared_lib_name) + 48);
	sprintf (tmp, "SDQS_SMI_get_info_%s", Shared_lib_name);
	if (Port_opt > MAX_PORT_INC)
	    sprintf (tmp + strlen (tmp), "_%d", Port_opt);
    }
    return (tmp);
}

/**************************************************************************

    Sets the query port number.

**************************************************************************/

void SDQSM_set_port_number (char *port) {
    int p;

    if (sscanf (port, "%d", &p) != 1 ||
	p < 10000)
	fprintf (stderr, 
		"Bad port number (%s) from conf file - not used\n", port);
    else
	Port_opt = p;
}

/******************************************************************

    Returns the query port number.

******************************************************************/

char *SDQSM_get_port () {

    if (Port_number[0] != '\0')
	return (Port_number);

    if (Port_opt > MAX_PORT_INC)
	sprintf (Port_number, "%d", Port_opt);
    else {
	char *env, *p;
	int port = -1;
	env = getenv ("RMTPORT");
	if (env != NULL) {
	    if ((p = strstr (env, ":")) != NULL)
		env = p + 1;
	    if (sscanf (env, "%d", &port) == 1)
		port += 2;
	    else
		port = -1;
	}
	if (port < 0)
	    port = SDQ_DEFAULT_PORT;
	if (Port_opt > 0 && Port_opt <= MAX_PORT_INC)
	    port += Port_opt;
	sprintf (Port_number, "%d", port);
    }
    return (Port_number);
}

/**************************************************************************

    This is called when there is an event ready to receive.

**************************************************************************/

static void Ntf_fd_ready (int fd, int ready_flag) {
    LB_NTF_control (LB_NTF_WAIT, 0);
}

/**************************************************************************

    Memory allocation of size "size".

**************************************************************************/

char *SDQSM_malloc (int size) {
    char *p;
    p = malloc (size);
    if (p == NULL)
	LE_send_msg (GL_ERROR, "malloc (size %d) failed\n", size);
    return (p);
}

/******************************************************************

    Terminations handler - called by termination signal.

******************************************************************/

static void Signal_handler (int sig) {

    LE_send_msg (GL_INFO, "Signal %d received", sig);
    In_termination_phase = 1;
    return;
}

/******************************************************************

    Description: This is the basic client request processing function.

    Inputs:	req - the request message;
		req_len - length of the request message;

    Output:	resp - returns the response message.

    Return:	length of the response message.

******************************************************************/

static int Process_client_request (char *req, int req_len, char **resp) {
    int ret;
    void *r;

    r = NULL;
    *resp = NULL;
    if (strncmp (req, "Select: ", 8) == 0) {
	char *lb_name, *t;
	int mode, max_n, is_big_endian;

	if ((t = strtok (req, " \t\n")) == NULL ||
	    (lb_name = strtok (NULL, " \t\n")) == NULL ||
	    (t = strtok (NULL, " \t\n")) == NULL ||
	    sscanf (t, "%d", &mode) != 1 ||
	    (t = strtok (NULL, " \t\n")) == NULL ||
	    sscanf (t, "%d", &is_big_endian) != 1 ||
	    (t = strtok (NULL, " \t\n")) == NULL ||
	    sscanf (t, "%d", &max_n) != 1)
	    return (SDQ_REQ_SYNT_ERROR);
	t += strlen (t) + 1;
	while (*t == ' ' || *t == '\t')
	    t++;
	LB_NTF_control (LB_NTF_BLOCK);
	ret = SDQS_select (lb_name, mode, max_n, is_big_endian, t, &r);
	LB_NTF_control (LB_NTF_UNBLOCK);
	if (ret <= 0) {
	    SDQM_query_results *err_res;
	    err_res = (SDQM_query_results *)
				SDQSM_malloc (sizeof (SDQM_query_results));
	    if (err_res == NULL)
		return (SDQM_MALLOC_FAILED);
	    err_res->msg_size = sizeof (SDQM_query_results);
	    err_res->type = SDQM_QUERY;
	    err_res->n_recs = err_res->n_recs_found = 0;
	    err_res->err_code = -ret;
	    *resp = (char *)err_res;
	    if (ret < 0)
		LE_send_msg (LE_VL1, "SDQS_select failed (ret %d)", ret);
	    return (err_res->msg_size);
	}
    }
    else {
	LB_NTF_control (LB_NTF_BLOCK);
	ret = SDQM_process_query (req, req_len, 
				SDQM_SERIALIZED_RESULT, &r);
	LB_NTF_control (LB_NTF_UNBLOCK);
	if (ret <= 0)
	    LE_send_msg (LE_VL1, "SDQM_process_query failed (ret %d)", ret);
    }
    *resp = (char *)r;
    return (ret);
}

/******************************************************************

    Description: This is the basic server housekeeping function.
		It is called periodically.

    Return:	return value is not used.

******************************************************************/

static int House_keeping () {

    SDQSO_house_keep (In_termination_phase);
    if (In_termination_phase) {
	LE_send_msg (GL_INFO, "%s terminates", Prog_name);
	exit (0);
    }
    if (Run_seconds > 0 && MISC_systime (NULL) > St_time + Run_seconds)
	In_termination_phase = 1;
    return (0);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv)
{
    extern char *optarg;	/* used by getopt */
/*    extern int optind; */
    int c;			/* used by getopt */
    int err;           		/* error flag */

    Verbose = Display_on_stderr = Compile_only = 0;
    No_compilation = 1;
    Run_seconds = 0;
    err = 0;
    while ((c = getopt (argc, argv, "hc:e:s:p:t:g:mvdo?")) != EOF) {
	switch (c) {

	    case 'c':
		strncpy (Conf_name, optarg, SDQS_NAME_SIZE);
		Conf_name[SDQS_NAME_SIZE - 1] = '\0';
		No_compilation = 0;
		break;

	    case 'e':
		strncpy (Environ_name, optarg, SDQS_NAME_SIZE);
		Environ_name[SDQS_NAME_SIZE - 1] = '\0';
		break;

	    case 's':
		strncpy (Shared_lib_name, optarg, SDQS_NAME_SIZE);
		Shared_lib_name[SDQS_NAME_SIZE - 1] = '\0';
		break;

	    case 'g':
		strncpy (Sys_cfg_name, optarg, SDQS_NAME_SIZE);
		Sys_cfg_name[SDQS_NAME_SIZE - 1] = '\0';
		break;

	    case 'p':
		if (sscanf (optarg, "%d", &Port_opt) != 1 ||
		    (Port_opt < 10000 && Port_opt > MAX_PORT_INC)) {
		    fprintf (stderr, "Bad -p option: %s\n", optarg);
		    err = -1;
		}
		break;

	    case 't':
		if (sscanf (optarg, "%d", &Run_seconds) != 1) {
		    fprintf (stderr, "Bad -t option: %s\n", optarg);
		    err = -1;
		}
		break;

	    case 'd':
		Display_on_stderr = 1;
		break;

	    case 'm':
		Allow_missing_LBs = 1;
		break;

	    case 'o':
		Compile_only = 1;
		Log_size = 0;
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

static void Print_usage (char **argv)
{
    printf ("Usage: %s (options)\n", argv[0]);
    printf ("       %s - the simple data base query server\n", argv[0]);
    printf ("       Options:\n");
    printf ("       -c configuration_file_name (The default is running\n");
    printf ("          without compilation)\n");
    printf ("       -e environ_name (The default is non)\n");
    printf ("       -s shared_lib_name (The default is sdqs)\n");
    printf ("       -p port_number (Used if running without compilation.\n");
    printf ("          If < 3, used for increment. The default is 30245)\n");
    printf ("       -o (Compilation only)\n");
    printf ("       -v (Turns on verbose mode)\n");
    printf ("       -d (Displays messages on stderr also)\n");
    printf ("       -t seconds (Time to run. The default is forever.)\n");
    printf ("       -g sys_cfg (Alternate system config name for run time)\n");
    printf ("       -m (Allowing missing managed LBs, in run time, and only local LBs are managed\n");
    printf ("       -h (Prints usage info and terminates)\n");
    exit (1);
}

/**************************************************************************

    Description: This function sends error messages while reading the link 
		configuration file.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static void Print_cs_error (char *msg) {

    LE_send_msg (GL_ERROR, msg);
}


