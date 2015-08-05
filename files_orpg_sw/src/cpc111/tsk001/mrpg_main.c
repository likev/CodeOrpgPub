
/******************************************************************

	file: mrpg_main.c

	This is the main module for the mrpg (manage RPG) program.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/08 20:34:08 $
 * $Id: mrpg_main.c,v 1.119 2013/07/08 20:34:08 steves Exp $
 * $Revision: 1.119 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef LINUX
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <orpg.h>
#include <mrpg.h>
#include "mrpg_def.h"
#include <infr.h> 

static int Verbose = -1;		/* verbose level */
static time_t Start_time;		/* mrpg start time */
static int Report_off = 0;		/* temporarily stop reporting */
static int Wait_seconds = 120;		/* wait time before operation */
static char Init_section_name[MRPG_NAME_SIZE] = "";
					/* init section name */
static char Resource_config[MRPG_NAME_SIZE] = "";
					/* resource config name */

static int Start_in_foreground = 1;	/* mrpg runs in foreground */
static int Stay_in_foreground = 0;

static int Command = -1;		/* RPG commands from the cmd line */

static int Create_state_file = 0;	/* creates RPG state file and exit */
static int Startup_by_sending_command = 0;

static int Full_cleanup_needed = 0;	/* complete cleanup is requested */
static int Os_crashed = 0;		/* OS detected to be crashed */

static int Ppid;			/* mrpg parent pid */
static int Cpid;			/* mrpg child pid */

static int Sigpwr_received = 0;		/* SIGPWR signal received */
static int Signal_received_while_run_mrpg = 0;
					/* a signal received while running a
					   remote mrpg */
static int As_coprocessor = 0;

static unsigned int Keep_ip = 0;
static int Keep_pid = 0;
static int Ascii_adapt_used = 0;
static int Rm_log_only = 0;

static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static void Cs_err_func (char *msg);
static void Goto_background ();
static int Create_work_dirs ();
static int Term_handler (int sig, int status);
static void Terminate_mrpg (int lock);
static int Start_rssd ();
static void Signal_handler (int sig);
static int Cleanup_all_datastores ();
static void Run_mrpg_remotely (int argc, char **argv);
static void Run_mrpg_signal_handler (int sig);
static void My_exit (int status);
static int Verify_adapt ();
static void Backup_log_file ();
static void Set_os_crash_fixed ();
static int Was_os_crashed ();


/******************************************************************

    Description: The main function.

******************************************************************/

static void Get_rss_stdout (char *msg) {
    LE_send_msg (GL_ERROR, "%s\n", msg);
}

int main (int argc, char *argv[]) {
    int wait_for_mrpg_to_die, ret, missing_node;

    Start_time = time (NULL);
    Ppid = getpid ();

    /* read options */
    if (Read_options (argc, argv) != 0)
	exit (0);

    if (Verbose < 0) {
	Verbose = 0;
	if (Start_in_foreground)
	    Verbose = 1;
    }

    if ((Command == MRPG_STARTUP || Command == MRPG_AUTO_START) &&
		MAIN_is_operational () &&
		ORPGMGR_was_os_crashed ())	/* local host crashed */
	Backup_log_file ();

    LE_local_vl (Verbose);
    LE_set_option ("LB size", 5000);
    ret = LE_init (argc, argv);
    if (ret < 0)
	LE_send_msg (GL_INFO, "LE_init failed (%d)", ret);
    LE_local_vl (Verbose);
    if (Start_in_foreground) {
	LE_set_option ("also stderr", 1);
	LE_set_option ("no source info", GL_STATUS);
    }
    CS_error (Cs_err_func);
    RSS_rpc_stdout_redirect (Get_rss_stdout);

    if (Command == MRPG_INIT ||
	(!MAIN_is_operational () && 
	 (Command == MRPG_STARTUP || Command == MRPG_AUTO_START || 
		Command == MRPG_REMOVE_DATA || Command == MRPG_CLEANUP))) {
	if (Start_rssd () < 0) {
	    LE_send_msg (GL_ERROR, "start_rssd failed (%d)", ret);
	    My_exit (1);
	}
    }

    if (Command == MRPG_AUTO_START && !MAIN_is_operational ()) {
	LE_send_msg (GL_ERROR, 
		"Command MRPG_AUTO_START can only be used in operational RPG");
	My_exit (1);
    }

    if (Startup_by_sending_command ||
	(Command != MRPG_RESUME && Command != MRPG_REMOVE_DATA && 
	 Command != MRPG_INIT && Command != MRPG_AUTO_START && 
	 Command != MRPG_STARTUP && Command != MRPG_CLEANUP)) {
	ret = ORPGMGR_send_command (Command);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
			"Sending command (%d) to mrpg failed\n", Command);
	    My_exit (1);
	}
	else {
	    LE_send_msg (GL_INFO, "Command (%d) sent to mrpg\n", Command);
	    My_exit (0);
	}
    }

    if ((Command == MRPG_STARTUP || Command == MRPG_AUTO_START) && 
	MAIN_is_operational ()) {
	char buf[128];
	if (ORPGMISC_get_install_info (DEV_CONFIGURED_TAG, buf, 128) < 0 ||
	    strcmp (buf, "YES") != 0) {
	    LE_send_msg (GL_ERROR, "RPG hardware config not completed");
	    My_exit (1);
	}
	if (ORPGMISC_get_install_info (ADAPT_LOADED_TAG, buf, 128) < 0 ||
	    strcmp (buf, "YES") != 0) {
	    LE_send_msg (GL_ERROR, "RPG adaptation data not installed");
	    My_exit (1);
	}
    }

    if (Command == MRPG_RESUME &&	/* restart mrpg for managing RPG */
	MMR_read_resource_name (Resource_config, MRPG_NAME_SIZE) < 0)
	My_exit (0);
    if (Command == MRPG_INIT)
	Resource_config[0] = '\0';

    if (MHR_init (Resource_config, &missing_node) < 0) { /* read resource table */
	LE_send_msg (GL_INFO, "MHR_init failed");
	My_exit (1);
    }

    if (Command == MRPG_STARTUP && !MHR_is_mrpg_node ())
	Run_mrpg_remotely (argc, argv);

    if (Command == MRPG_AUTO_START && !MHR_is_mrpg_node ()) {
	LE_send_msg (GL_INFO, "Not mrpg node - mrpg auto_start terminates");
	My_exit (0);
    }

    if (Command == MRPG_RESUME && !MHR_is_mrpg_node ()) {
	LE_send_msg (GL_INFO, "Not mrpg node - mrpg resume terminates");
	My_exit (0);
    }

    if (Command == MRPG_CLEANUP) { 
	char buf[128];
	LE_set_option ("LE disable", 1);
	ret = ORPGMISC_get_install_info ("TYPE:", buf, 128);
	LE_set_option ("LE disable", 0);
	if (ret >= 0 &&
	    strcasecmp (buf, "MSCF") == 0) {
	    LE_send_msg (GL_ERROR, "mrpg cleanup does not run on mscf");
	    My_exit (1);
	}
	MISC_log_disable (1);
	if (MRT_init () < 0) {
	    LE_send_msg (GL_INFO,  "Failed in getting RPG task names");
	    My_exit (1);
	}
	MAIN_clean_up_processes (1, 0);
	My_exit (0);
    }

    if (Command == MRPG_REMOVE_DATA) {
	Cleanup_all_datastores ();
	My_exit (0);
    }

    if (Create_state_file) {
	if (MMR_create_state_file () < 0)
	    My_exit (1);
	My_exit (0);
    }

    wait_for_mrpg_to_die = 0;
    if (Command == MRPG_STARTUP || Command == MRPG_AUTO_START ||
	Command == MRPG_INIT) {
	Terminate_mrpg (1);
	wait_for_mrpg_to_die = 1;
    }

    if (!Stay_in_foreground) {
	LE_send_msg (LE_VL2, "mrpg goes to background");
	ORPGMGR_deregister_query_hosts ();
	Goto_background ();
    }

    if (MHR_register_lost_en_conn_event () < 0)
	My_exit (1);

    if (Command != MRPG_RESUME)
	DEAU_LB_name ("");	/* in non-dev env, adapt LB may be created and
				   need to be closed so it can be removed */

    if (Command == MRPG_STARTUP || Command == MRPG_AUTO_START) {
	Os_crashed = Was_os_crashed ();
	if (Os_crashed)
	    MPC_set_os_crash_start (1);
	Cleanup_all_datastores ();
    }

    if (Command == MRPG_STARTUP) {
	if (!MAIN_is_operational ())
	    LE_send_msg (GL_INFO, "Start up RPG - Non-operational");
	else
	    LE_send_msg (GL_INFO, "Start up RPG - Operational");
    }
    if (Command == MRPG_RESUME)
	LE_send_msg (GL_INFO, "mrpg resumes for managing");

    ret = MMR_open_state (wait_for_mrpg_to_die);
    if (ret == MRPG_LOCKED) {	/* mrpg is running */
	My_exit (0);
    }
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "MMR_open_state failed");
	My_exit (1);
    }

    /* create LE directory */
    if (Create_work_dirs () < 0)
	My_exit (1);

    /* Initialize the LE service */
    if (ORPGMISC_LE_init (argc, argv, 1000, 0, -1, 0) < 0)
		LE_send_msg (GL_INFO, "ORPGMISC_LE_init failed");

    ret = ORPGTASK_reg_term_handler (Term_handler);
    if (ret < 0) {
	LE_send_msg (GL_ERROR, "ORPGTASK_reg_term_handler failed (%d)", ret);
	My_exit (1) ;
    }
    sigset (SIGPWR, Signal_handler);

    if (MRD_init () < 0 ||		/* read data stores */
	MGC_init () < 0)		/* create sys_cfg */
	My_exit (1);

    if (Command == MRPG_RESUME)
	ORPGMISC_deau_init ();
    else if (!MAIN_is_operational ()) {
	DEAU_use_attribute_file ("site_info.dea", 0);
	Ascii_adapt_used = 1;
    }
    else {
	if (MRT_process_install_cmds () < 0)
	    My_exit (1);
	ORPGMISC_deau_init ();
    }

    if (MGC_complete_sys_cfg () < 0)
	My_exit (1);

    /* init modules (order is important) */
    if (MRD_read_comms_config () < 0 ||
	MMR_init () < 0 ||
	MRT_init () < 0 ||
	MCD_init () < 0 ||
	MPI_init () < 0 ||
	MPC_init () < 0 ||
	MMR_create_state_file () < 0)
	MAIN_exit (1);

    if (nice (10) < 0) {
	LE_send_msg (GL_ERROR, "nice failed");
	MAIN_exit (1);
    }

    if (Command != MRPG_RESUME && MPC_process_command (Command) < 0)
	MAIN_exit (1);
    MPC_set_os_crash_start (0);
    MCD_recreate_all_lbs (0);

    if (Start_in_foreground && !Stay_in_foreground) {
	kill (Ppid, SIGTERM);		/* terminate parent mrpg */
	if (As_coprocessor)
	    LE_set_option ("output fd", stderr);
	LE_set_option ("also stderr", 0);
	LE_set_option ("no source info", 0);
    }

    if (Command == MRPG_INIT)
	MAIN_exit (0);

    if (Command == MRPG_STARTUP || Command == MRPG_AUTO_START) {
	MMR_save_resource_name ();
	if (MAIN_is_operational ()) {
	    unsigned char alarm_flag;
	    int alarm_action;
	    if (missing_node)
		alarm_action = ORPGINFO_STATEFL_SET;
	    else
		alarm_action = ORPGINFO_STATEFL_CLR;
	    if (ORPGINFO_statefl_rpg_alarm (ORPGINFO_STATEFL_RPGALRM_NODE_CON,
			    alarm_action, &alarm_flag) < 0)
		LE_send_msg (GL_ERROR, 
			"Failed in setting/clearing NODE_CON alarm");
	}
    }

    /* the main loop */
    RMT_time_out (6);
    while (1) {
	if (MPC_process_queued_cmds () < 0) {
	    LE_send_msg (GL_ERROR, "mrpg terminates - Error detected");
	    My_exit (1);
	}
	MPI_housekeep ();
	MMR_housekeep ();
	if (Sigpwr_received) {
	    MPC_process_command (MRPG_POWERFAIL);
	    MAIN_exit (0);
	}
	msleep (500);
    }

    MAIN_exit (0);
}

/******************************************************************

    Rename the log file if OS crash is detected because the log file
    maybe corrupted and cannot be used.

******************************************************************/

static void Backup_log_file () {
    char *ledir, dir[200], oldname[256], newname[256];
    time_t tm;
    int y, mon, d, h, m, s;

    if (Command != MRPG_STARTUP && Command != MRPG_AUTO_START)
	return;
    ledir = getenv ("LE_DIR_EVENT");
    if (ledir == NULL)
	return;
    MISC_get_token (ledir, "S:", 0, dir, 200);
    sprintf (oldname, "%s/mrpg.log", dir);
    tm = time (NULL);
    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
    sprintf (newname, "%s/mrpg.%.2d%.2d%.2d.%.2d%.2d%.2d.log", 
					dir, mon, d, y, h, m, s);
    rename (oldname, newname);
}
/**************************************************************************

    If the ASCII adaptation data is used, we switch to the DEA DB because 
    the DB is now created.

**************************************************************************/

int MAIN_switch_to_deau_db () {

    if (Ascii_adapt_used) {
	DEAU_use_attribute_file (NULL, 0);
	DEAU_LB_name ("");
	ORPGMISC_deau_init ();
	Ascii_adapt_used = 0;
    }

    if ((Command == MRPG_STARTUP || Command == MRPG_AUTO_START || 
	 Command == MRPG_INIT) &&
	Verify_adapt () < 0)
	return (-1);

    return (0);
}

/**************************************************************************

    Returns mrpg start time.

**************************************************************************/

time_t MAIN_start_time () {
    return (Start_time);
}

/**************************************************************************

    Returns mrpg command.

**************************************************************************/

int MAIN_command () {
    return (Command);
}

/**************************************************************************

    Returns wait time before operation.

**************************************************************************/

int MAIN_wait_time () {
    return (Wait_seconds);
}

/**************************************************************************

    Returns Init_section_name.

**************************************************************************/

char *MAIN_init_section_name () {
    return (Init_section_name);
}

/**************************************************************************

    Returns non-zero if the RPG is running in operational environment or
    zero otherwise.

**************************************************************************/

int MAIN_is_operational () {
    static int operational = -1;

    if (Command == MRPG_INIT)
	return (0);
    if (operational < 0)
	operational = ORPGMISC_is_operational ();
    return (operational);
}

/**************************************************************************

    Runs mrpg on the default node and prints the output while in foreground.

**************************************************************************/

static void Run_mrpg_remotely (int argc, char **argv) {
    int size, cnt, pipefd, ret, i, n_chars;
    char *cmd, *rpc_buf, hname[MRPG_NAME_SIZE];
    char local_hname[MRPG_NAME_SIZE];

    if (Command == MRPG_AUTO_START)
	My_exit (0);

    sigignore (SIGTERM);	/* not to be killed by the remote mrpg */
    sigset (SIGINT, Run_mrpg_signal_handler);
    sigset (SIGTERM, Run_mrpg_signal_handler);

    if (MHR_get_mrpg_node_name (hname, MRPG_NAME_SIZE) < 0 ||
	MHR_get_local_node_name (local_hname, MRPG_NAME_SIZE) < 0) {
	LE_send_msg (GL_ERROR, "mrpg or local host name not found\n");
	My_exit (1);
    }

    size = 0;			/* for the -k option */
    for (i = 0; i < argc; i++)
	size += strlen (argv[i]) + 1;
    size += strlen (local_hname) + 32;		/* for -k option */
    cmd = malloc (size + 1);
    rpc_buf = malloc (64 + strlen (hname));
    if (cmd == NULL || rpc_buf == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	My_exit (1);
    }
    cnt = 0;
    for (i = 0; i < argc; i++) {
	sprintf (cmd + cnt, "%s ", argv[i]);
	cnt += strlen (argv[i]) + 1;
    }
    sprintf (cmd + cnt, "-k %s:%d", local_hname, getpid ());
    cnt += strlen (cmd + cnt) + 1;
    if (cnt == 0)
	cmd[0] = '\0';
    else
	cmd[cnt - 1] = '\0';

    LE_send_msg (GL_INFO, 
	"This is not the mrpg node - We run mrpg on node %s\n", hname);

    RMT_time_out (60);
    sprintf (rpc_buf, "%s:MISC_system_to_buffer", hname);
    if ((ret = RSS_rpc (rpc_buf, "i-r s-i i i i", 
		    &pipefd, cmd, NULL, -2, NULL)) < 0 ||
	pipefd < 0) {
	LE_send_msg (GL_ERROR, 
		"RSS_rpc %s failed (%d fd %d)", rpc_buf, ret, pipefd);
	My_exit (1);
    }

    sprintf (rpc_buf, "%s:read", hname);
    n_chars = 0;
    while (1) {
	char buf[80];
	ret = RSS_rpc (rpc_buf, "i-r i ba-4-o i", &cnt, pipefd, 
					buf + n_chars, 72 - n_chars);
	if (ret < 0 || cnt < 0) {
	    LE_send_msg (GL_ERROR, "RSS_rpc %s failed (%d)", "read", ret);
	    My_exit (1);
	}
	if (cnt > 0) {
	    n_chars += cnt;
	    buf[n_chars] = '\0';
	    if (strstr (buf, "mrpg parent received signal") != NULL) {
		printf (buf);
		My_exit (0);
	    }
	    if (Signal_received_while_run_mrpg) {
					/* terminate remote mrpg */
		MGC_system (hname, "prm -quiet -9 mrpg", NULL);
		My_exit (1);
	    }
	}
	else {
	    if (n_chars > 0)
		printf (buf);
	    break;
	}
    }
    close (pipefd);

    My_exit (0);
}

/**************************************************************************

    Signal callback function while running a remote mrpg.

**************************************************************************/

static void Run_mrpg_signal_handler (int sig) {

    if (sig == SIGTERM)
	return;
    LE_send_msg (GL_INFO, 
		"running remote mrpg is interrupted (signal %d)", sig);
    Signal_received_while_run_mrpg = 1;
    return;
}

/**************************************************************************

    Description: This function terminates this process.

**************************************************************************/

static int Term_handler (int sig, int status) {

    if (sig == SIGHUP) {
	LE_send_msg (GL_INFO, "Signal SIGHUP (%d) ignored", sig);
	return (1);
    }

    if (sig == SIGINT)
	My_exit (0);
    LE_send_msg (GL_INFO,  "mrpg terminates");
    return (0);
}

/*************************************************************************

    Kills existing instance of mrpg and its service processes.

*************************************************************************/

static void Terminate_mrpg (int lock) {
    static int fd = -1;
    char *name;
    Node_attribute_t *nodes;
    int n_nodes, i;

    if (!lock) {
	if (fd >= 0)
	    LB_close (fd);
	return;
    }
    if ((name = MMR_state_file_name ()) != NULL &&
	(fd = LB_open (name, LB_READ, NULL)) >= 0)
	LB_lock (fd, LB_EXCLUSIVE_LOCK | LB_BLOCK, MRPG_STATE_MSGID);
    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < n_nodes; i++) {
	char buf[128];
	if (!MHR_is_distributed () || nodes[i].is_local)
	    sprintf (buf, "prm -9 -quiet -keep-%d mrpg nds", (int)getpid ());
	else
	    sprintf (buf, "prm -quiet mrpg nds");
	if (MGC_system (nodes[i].hname, buf, NULL) != 0) {
	    LE_send_msg (GL_ERROR, "Removing (prm) existing mrpg failed");
	    My_exit (1) ;
	}
    }
/*    LB_close (fd); */
}

/*************************************************************************

    Starts the rssd server.

    Returns 1 on success, 0 if already running or -1 on failure.

*************************************************************************/

static int Start_rssd () {
    char buf[128];
    int ret;

    buf[0] = '\0';
    ret = MISC_system_to_buffer ("start_rssd > /dev/null", buf, 128, NULL);
    if (buf[0] != '\0')
	ORPGMISC_send_multi_line_le (GL_INFO, buf);
    if (ret == 0) {
	LE_send_msg (LE_VL3, "rssd started");
	return (1);
    }
    else if (ret == (2 << 8))	/* rssd already running */
	return (0);
    else
	return (-1);
}

/*************************************************************************

    Kills all RPG processes by name.

*************************************************************************/

#define PRM_BUFFER_SIZE 200

int MAIN_clean_up_processes (int all, int q_cmd) {
    Mrpg_tat_entry_t *prcs;
    Node_attribute_t *nodes;
    int n_prcs, n_nodes, i, le_code;

    if (all)
	le_code = GL_INFO;
    else {
	LE_send_msg (GL_INFO, "Removing all RPG operational tasks");
	le_code = LE_VL2;
    }

    n_nodes = MHR_all_nodes (&nodes);
    n_prcs = MRT_get_ops (&prcs);
    for (i = 0; i < n_nodes; i++) {
	int cnt, size;
	char *hname;

	hname = "";
	if (MHR_is_distributed ()) {
	    if (!nodes[i].is_local) {
		if (nodes[i].ip == 0)		/* not connected */
		    continue;
		hname = nodes[i].hname;
	    }
	    LE_send_msg (le_code,
		"Removing all RPG tasks on %s ...", nodes[i].node);
	}
	else
	    LE_send_msg (le_code, "Removing all RPG tasks ...");
	cnt = 0;
	size = 0;
	while (1) {
	    char *out, buf[PRM_BUFFER_SIZE], *pname;

	    if (cnt >= n_prcs) {		/* done */
		if (cnt == n_prcs)
		    pname = "nds lb_rep bcast brecv";	/* infr processes */
		else {
		    if (size > 0)
			MGC_system (hname, buf, &out);
		    break;
		}
	    }
	    else
		pname = prcs[cnt].cmd_name;
	    if (cnt > 0 && strcmp (pname, prcs[cnt - 1].cmd_name) == 0) {
		cnt++;
		continue;
	    }
	    if (!all && q_cmd && (prcs[cnt].type & MRPG_TT_ALL_STATES)) {
		cnt++;
		continue;
	    }
	    if (strcmp (pname, "mrpg") == 0) {
		char tmp[64];
		if (!MHR_is_distributed () || nodes[i].is_local)
		    sprintf (tmp, "prm -9 -quiet -keep-%d -keep-%d mrpg", 
					(int)getpid (), (int)getppid ());
		else {
		    if (Keep_ip != 0 && nodes[i].ip == Keep_ip) {
			sprintf (tmp, "prm -9 -quiet -keep-%d mrpg", Keep_pid);
		    }
		    else
			strcpy (tmp, "prm -9 -quiet mrpg");
		}
		MGC_system (hname, tmp, &out);
	    }
	    else if (strcmp (pname, "rssd") == 0) {
		if (!MAIN_is_operational () && nodes[i].is_local && all)
		    MGC_system (hname, "prm -quiet -9 rssd", &out);
	    }
	    else {
		if (size + strlen (pname) + 1 >= PRM_BUFFER_SIZE) {
		    MGC_system (hname, buf, &out);
		    size = 0;
		}
		if (size == 0) {
		    strcpy (buf, "prm -quiet -9 ");
		    size += strlen ("prm -quiet -9 ");
		}
		if (all || 
		(cnt < n_prcs && !(prcs[cnt].type & MRPG_TT_MONITOR_ONLY))) {
		    sprintf (buf + size, "%s ", pname);
		    size += strlen (buf + size);
		}
	    }
	    cnt++;
	}
    }

    return (0);
}

/******************************************************************

    Cleans up data stores that may be corrupted in an OS crash. This
    is done by saving existing data with command "rescue_data save"
    and then remove all files in "ORPGDIR" and "WORK_DIR" directories.
	
******************************************************************/

static int Cleanup_all_datastores () {

    if (MRT_init () < 0) {
	LE_send_msg (GL_INFO,  "Failed in first call to MRT_init");
	My_exit (1);
    }

    if (Command == MRPG_INIT || Command == MRPG_REMOVE_DATA ||
	Full_cleanup_needed || Os_crashed) {
	Node_attribute_t *nodes;
	int n_nodes, i, ret;
	char *out, buf[256];

	if (Command != MRPG_INIT && Command != MRPG_REMOVE_DATA &&
		MAIN_is_operational ()) {
	    ret = MISC_system_to_buffer ("rescue_data save", buf, 256, NULL);
	    if (buf[0] != '\0')
		ORPGMISC_send_multi_line_le (GL_INFO, buf);
	    if (ret != 0)
		LE_send_msg (GL_INFO, "rescue_data save not possible");
	}
	if (Full_cleanup_needed || Command == MRPG_INIT)
	    LE_send_msg (GL_INFO, "Cleaning up all data stores...");
	else if (Command != MRPG_REMOVE_DATA)
	    LE_send_msg (GL_INFO, 
		"Cleaning up all data stores due to OS crash...");

	MMR_close_state_file ();
	n_nodes = MHR_all_nodes (&nodes);
	for (i = 0; i < n_nodes; i++) {
	    char *cmd, *pf_names, *pf;
	    int n_p_files, k;

	    if (!nodes[i].is_local && strlen (nodes[i].hname) == 0)
		continue;

	    if (Command == MRPG_REMOVE_DATA) {
		if (Rm_log_only)
		    LE_send_msg (GL_INFO, 
			"Cleaning up all log files on %s", nodes[i].hname);
		else
		    LE_send_msg (GL_INFO, 
			"Cleaning up all data stores on %s", nodes[i].hname);
	    }

	    n_p_files = MRT_get_perm_file_names (&pf_names);

	    cmd = NULL;
	    cmd = STR_copy (cmd, "lb_rm");
	    pf = pf_names;
	    for (k = 0; k < n_p_files; k++) {
		cmd = STR_cat (cmd, " -k ");
		cmd = STR_cat (cmd, MISC_basename (pf));
		pf += strlen (pf) + 1;
	    }
	    cmd = STR_cat (cmd, " -k mrpg.*.log -k mrpg.log -a ");
	    cmd = STR_cat (cmd, nodes[i].orpgdir);
	    if (Rm_log_only)
		cmd = STR_cat (cmd, "/logs");
	    MGC_system (nodes[i].hname, cmd, &out);
	    if (!Rm_log_only) {
		cmd = STR_copy (cmd, "lb_rm -a ");
		cmd = STR_cat (cmd, nodes[i].workdir);
		MGC_system (nodes[i].hname, cmd, &out);
	    }
	    STR_free (cmd);
	}
    }
    if (Os_crashed)
	Set_os_crash_fixed ();
    return (0);
}

/******************************************************************

    Returns true if the OS on any of the nodes is detected to be 
    crashed before booting.
	
******************************************************************/

static int Was_os_crashed () {
    int n_nodes, i, os_crash;
    Node_attribute_t *nodes;

    if (!MAIN_is_operational ())
	return (0);

    os_crash = 0;
    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < n_nodes; i++) {
	char func[256];
	int os_c, ret;
	sprintf (func, "%s:liborpg.so,ORPGMGR_was_os_crashed", nodes[i].hname);
	if ((ret = RSS_rpc (func, "i-r", &os_c)) < 0 || os_c) {
	    LE_send_msg (0, "Node %s - OS crash", nodes[i].hname);
	    os_crash = 1;
	    break;
	}
    }

    return (os_crash);
}

/******************************************************************

    Sets the ETC_BOOT_TIME files on all nodes to record the fact
    that the OS crash has been processed. This is called after the
    RPG data stores are cleaned up to remove any possible corruption
    due to OS crash.
	
******************************************************************/

static void Set_os_crash_fixed () {
    int n_nodes, i;
    Node_attribute_t *nodes;

    if (!Os_crashed)
	return;

    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < n_nodes; i++) {
	char func[256];
	int failed;
	sprintf (func, "%s:liborpg.so,ORPGMGR_set_os_crash_fixed",
					nodes[i].hname);
	if (RSS_rpc (func, "i-r", &failed) < 0 || failed) {
	    LE_send_msg (0, "Fixing /etc/boot_time on %s failed - missing or permission?",
					nodes[i].hname);
	    My_exit (1);
	}
    }
    Os_crashed = 0;
}

/******************************************************************

    Description: Creates the RPG directories.

******************************************************************/

static int Create_work_dirs () {
    Node_attribute_t *nodes;
    int n_nodes, i;

    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < n_nodes; i++) {
	char dir[MRPG_NAME_SIZE * 2 + 8];

	sprintf (dir, "%s:%s/", nodes[i].hname, nodes[i].logdir);
	if (MCD_create_dir (dir) < 0)
	    return (-1);
	sprintf (dir, "%s:%s/", nodes[i].hname, nodes[i].workdir);
	if (MCD_create_dir (dir) < 0)
	    return (-1);
	sprintf (dir, "%s:%s/", nodes[i].hname, nodes[i].orpgdir);
	if (MCD_create_dir (dir) < 0)
	    return (-1);
    }
    return (0);
}

/**************************************************************************

    Description: This is the CS error reporting function.

    Input:	The error message.

**************************************************************************/

static void Cs_err_func (char *msg) {

    if (!Report_off)
	LE_send_msg (GL_ERROR | 7,  msg);
    return;
}

/**************************************************************************

    Description: Turns off/on error message reporting.

    Input:	yes - non-zero indicates turning off, otherwise on.

**************************************************************************/

void MAIN_disable_report (int yes) {
    if (yes) {
	Report_off = 1;
	LE_set_option ("LE disable", 1);
    }
    else {
	Report_off = 0;
	LE_set_option ("LE disable", 0);
    }
    return;
}

/**************************************************************************

    Returns 1 if the two strings "s1" and "s2" are non-empty and differ or
    0 otherwise.

**************************************************************************/

static int Str_diff (char *s1, char *s2) {
    if (strlen (s1) > 0 && strlen (s2) > 0 &&
	strcasecmp (s1, s2) != 0)
	return (1);
    return (0);
}

/**************************************************************************

    Checks if any two elements of the string array "strs" of size "n" are 
    different. If it is, an error message is posted and 0 is returned.
    Otherwise 1 is returned. "names" and "label" are used for the error
    message.

**************************************************************************/

static int Is_match (int n, char **strs, char **names, char *label) {
    int i, match;

    match = 1;
    for (i = 0; i < n; i++) {
	int k;
	for (k = i + 1; k < 4; k++) {
	    if (Str_diff (strs[i], strs[k])) {
		LE_send_msg (GL_ERROR, "    %s differ (%s %s, %s %s)", label, names[i], strs[i], names[k], strs[k]);
		match = 0;
	    }
	}
    }
    return (match);
}

/**************************************************************************

    Verifies adaptation data against installation info. Returns 0 if they 
    match, -1 otherwise. This function verifies consistency of adaptation
    data, rpg_install.info and site_data (node name is not verified).

**************************************************************************/

#define BUF_SIZE 128

static int Verify_adapt () {
    char dea_fname[256], *p, buf[BUF_SIZE];
    char a_site[BUF_SIZE], a_red[BUF_SIZE], a_chan[BUF_SIZE];
    char b_site[BUF_SIZE], b_red[BUF_SIZE], b_chan[BUF_SIZE];
    char i_site[BUF_SIZE], i_chan[BUF_SIZE];
    char s_site[BUF_SIZE], s_red[BUF_SIZE], s_chan[BUF_SIZE];
    char *name[4], *s[4];
    int matched, ret, i;

    /* read site_info.dea */
    a_site[0] = a_red[0] = a_chan[0] = '\0';
    if (!MAIN_is_operational () && MISC_get_cfg_dir (dea_fname, 200) >= 0) {
	FILE *fl;

	strcat (dea_fname, "/site_info.dea");
	fl = fopen (dea_fname, "r");
	while (fl != NULL &&
	    fgets (buf, BUF_SIZE, fl) != NULL) {
	    char *v, tok[BUF_SIZE];
	    int off;

	    v = NULL;
	    if ((off = MISC_get_token (buf, "", 0, tok, BUF_SIZE)) > 0) {
		if (strcmp (tok, "site_info.rpg_name:") == 0)
		    v = a_site;
		else if (strcmp (tok, "Redundant_info.redundant_type:") == 0)
		    v = a_red;
		else if (strcmp (tok, "Redundant_info.channel_number:") == 0)
		    v = a_chan;
	    }
	    if (v != NULL) {
		char *pe;
		p = buf + off;
		p += MISC_char_cnt (p, " \t");
		pe = p + strlen (p) - 1;
		while (pe >= p) {
		    if (*pe == '\n' || *pe == '\t' || *pe == ' ')
			*pe = '\0';
		    else
			break;
		    pe--;
		}
		if (strlen (p) > 0) {
		    strncpy (v, p, BUF_SIZE);
		    v[BUF_SIZE - 1] = '\0';
		}
	    }
	}
	fclose (fl);
    }

    /* read DEA DB */
    b_site[0] = b_red[0] = b_chan[0] = '\0';
    if (DEAU_get_string_values ("site_info.rpg_name", &p) == 1) {
	strncpy (b_site, p, BUF_SIZE);
	b_site[BUF_SIZE - 1] = '\0';
    }
    if (DEAU_get_string_values ("Redundant_info.redundant_type", &p) == 1) {
	strncpy (b_red, p, BUF_SIZE);
	b_red[BUF_SIZE - 1] = '\0';
    }
    if (DEAU_get_string_values ("Redundant_info.channel_number", &p) == 1) {
	strncpy (b_chan, p, BUF_SIZE);
	b_chan[BUF_SIZE - 1] = '\0';
    }

    /* read rpg_install.info */
    LE_set_option ("LE disable", 1);
    i_site[0] = i_chan[0] = '\0';
    ret = ORPGMISC_get_install_info ("ICAO:", buf, BUF_SIZE);
    if (ret == 0)
	strcpy (i_site, buf);
    ret = ORPGMISC_get_install_info ("CHANNEL:", buf, BUF_SIZE);
    if (ret == 0) {
	if (strcmp (buf, "1") == 0)
	    strcpy (i_chan, "Channel 1");
	else if (strcmp (buf, "2") == 0)
	    strcpy (i_chan, "Channel 2");
    }
    else if (ret == -2)
	strcpy (i_chan, "Channel 1");

    /* read site_data */
    s_site[0] = s_red[0] = s_chan[0] = '\0';
    ret = ORPGMISC_get_site_value ("SITE_NAME", buf, BUF_SIZE);
    if (ret == 0 || (strstr (buf, "Variable") != NULL && 
			strstr (buf, "not found") != NULL)) {
	if (ret == 0)
	    strcpy (s_site, buf);
	if (ORPGMISC_get_site_value ("FAA", buf, BUF_SIZE) == 0 &&
	    strcmp (buf, "YES") == 0)
	    strcpy (s_red, "FAA Redundant");
	else if (ORPGMISC_get_site_value ("NWS_RED", buf, BUF_SIZE) == 0 &&
	    strcmp (buf, "YES") == 0)
	    strcpy (s_red, "NWS Redundant");
	else
	    strcpy (s_red, "No Redundancy");
	strcpy (s_chan, "Channel 1");
	if (ORPGMISC_get_site_value ("FAA_CH1", buf, BUF_SIZE) == 0 &&
	    strcmp (buf, "YES") == 0)
	    strcpy (s_red, "FAA Redundant");
	if (ORPGMISC_get_site_value ("FAA_CH2", buf, BUF_SIZE) == 0 &&
	    strcmp (buf, "YES") == 0) {
	    strcpy (s_chan, "Channel 2");
	    strcpy (s_red, "FAA Redundant");
	}
    }
    LE_set_option ("LE disable", 0);

    /* compare */
    matched = 1;
    s[0] = a_site;
    s[1] = b_site;
    s[2] = i_site;
    s[3] = s_site;
    name[0] = "site_info.dea";
    name[1] = "DEA DB";
    name[2] = "rpg_install.info";
    name[3] = "site_data";
    if (!Is_match (4, s, name, "Site name"))
	matched = 0;

    s[0] = a_red;
    s[1] = b_red;
    s[2] = "";
    s[3] = s_red;
    if (!Is_match (4, s, name, "Redundancy"))
	matched = 0;

    for (i = 0; i < 4; i++) {
	if (strcmp (s[i], "FAA Redundant") == 0) {
	    s[0] = a_chan;
	    s[1] = b_chan;
	    s[2] = i_chan;
	    s[3] = s_chan;
	    if (!Is_match (4, s, name, "Channel"))
		matched = 0;
	    break;
	}
    }    
    if (!matched) {
	LE_send_msg (GL_ERROR, "Inconsistent site data detected");
	return (-1);
    }
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
    char cmd[MRPG_NAME_SIZE], hname[MRPG_NAME_SIZE];

    err = 0;
    while ((c = getopt (argc, argv, "hl:w:i:d:k:bcfsvpS?")) != EOF) {
	switch (c) {

	    case 'l':
		if (sscanf (optarg, "%d", &Verbose) != 1)
		    err = -1;
		break;

	    case 'i':
		strncpy (Init_section_name, optarg, MRPG_NAME_SIZE);
		Init_section_name[MRPG_NAME_SIZE - 1] = '\0';
		break;

	    case 'w':
		if (sscanf (optarg, "%d", &Wait_seconds) != 1)
		    err = -1;
		break;

	    case 'd':
		strncpy (Resource_config, optarg, MRPG_NAME_SIZE);
		Resource_config[MRPG_NAME_SIZE - 1] = '\0';
		break;

	    case 'k':
		if (MISC_get_token (optarg, "S:Ci", 1, 
					&Keep_pid, 0) > 0) {
		    MISC_get_token (optarg, "S:", 0, 
					hname, MRPG_NAME_SIZE);
		    Keep_ip = NET_get_ip_by_name (hname);
		    if (Keep_ip == INADDR_NONE) {
			LE_send_msg (GL_ERROR, 
				"IP address of host %s not found", hname);
			err = -1;
		    }  
		}
		else
		    err = -1;
		break;

	    case 'v':
		Verbose = 2;
		break;

	    case 'b':
		Start_in_foreground = 0;
		break;

	    case 'f':
		Stay_in_foreground = 1;
		break;

	    case 's':
		Create_state_file = 1;
		break;

	    case 'S':
		Startup_by_sending_command = 1;
		break;

	    case 'p':
		Full_cleanup_needed = 1;
		break;

	    case 'c':
		As_coprocessor = 1;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    if (optind <= argc - 1) {        /* get the command */
	strncpy (cmd, argv[optind], MRPG_NAME_SIZE);
	cmd[MRPG_NAME_SIZE - 1] = '\0';
	if (strcmp (cmd, "startup") == 0)
	    Command = MRPG_STARTUP;
	else if (strcmp (cmd, "restart") == 0)
	    Command = MRPG_RESTART;
	else if (strcmp (cmd, "standby") == 0)
	    Command = MRPG_STANDBY;
	else if (strcmp (cmd, "shutdown") == 0)
	    Command = MRPG_SHUTDOWN;
	else if (strcmp (cmd, "no_manage") == 0)
	    Command = MRPG_NO_MANAGE;
	else if (strcmp (cmd, "manage") == 0)
	    Command = MRPG_MANAGE;
	else if (strcmp (cmd, "status") == 0)
	    Command = MRPG_STATUS;
	else if (strcmp (cmd, "resume") == 0)
	    Command = MRPG_RESUME;
	else if (strcmp (cmd, "active") == 0)
	    Command = MRPG_ACTIVE;
	else if (strcmp (cmd, "inactive") == 0)
	    Command = MRPG_INACTIVE;
	else if (strcmp (cmd, "powerfail") == 0)
	    Command = MRPG_POWERFAIL;
	else if (strcmp (cmd, "auto_start") == 0)
	    Command = MRPG_AUTO_START;
	else if (strcmp (cmd, "cleanup") == 0)
	    Command = MRPG_CLEANUP;
	else if (strcmp (cmd, "standby_restart") == 0)
	    Command = MRPG_STANDBY_RESTART;
	else if (strcmp (cmd, "init") == 0) {
            MCD_recreate_all_lbs (1);	
	    Command = MRPG_INIT;
	}
	else if (strcmp (cmd, "remove_data") == 0)
	    Command = MRPG_REMOVE_DATA;
	else if (strcmp (cmd, "remove_log") == 0) {
	    Command = MRPG_REMOVE_DATA;
	    Rm_log_only = 1;
	}
	else {
	    LE_send_msg (GL_ERROR,  "Unexpected command %s", cmd);
	    err = -1;
	}
    }
    if (Command < 0) {
	LE_send_msg (GL_INFO,  "No command specified - Done");
	exit (0);
    }
    if (Command != MRPG_STARTUP)
	Startup_by_sending_command = 0;

    return (err);
}

/**************************************************************************

    Description: This function prints the usage info.

**************************************************************************/

static void Print_usage (char **argv) {
    printf ("Usage: %s (options) command\n", argv[0]);
    printf ("       command - one of the RPG commands: \n");
    printf ("          startup, restart, standby, shutdown, status, resume\n");
    printf ("          no_manage, manage, powerfail\n");
    printf ("          active, inactive, auto_start, cleanup, standby_restart\n");
    printf ("          init, remove_data, remove_log\n");
    printf ("       Options:\n");
    printf ("       -i icsname (use startup init command section of\n");
    printf ("          Startup_init_cmds_icsname. Default: Startup_init_cmds)\n");
    printf ("       -w wait_seconds (maximum waiting time before operation. Default: 120)\n");
    printf ("       -d resource_config (resource config name. Default:\n");
    printf ("          The first feasible in the resource table)\n");
    printf ("       -b (Starting in background. Default: foreground)\n");
    printf ("       -f (Staying in foreground. Default: Go to background)\n");
    printf ("       -p (Clean up all data stores before starting up)\n");
    printf ("       -v (Sets initial verbose level to 2. Default: 0)\n");
    printf ("       -l vbl (Sets initial verbose level to vbl. Default: 0)\n");
    printf ("       -s (Creates RPG state file and terminates)\n");
    printf ("       -c (Invoked as a co-processor)\n");
    printf ("       -S (Startup by sending command message)\n");
    exit (0);
}

/******************************************************************

    This function puts the calling process into the background
    by forking a child and exiting. Refer to rmtd.c.

******************************************************************/

static void Goto_background () {

    switch ((Cpid = fork ())) {
	int stat_loc;

 	case -1:			/* error in fork */
	LE_send_msg (GL_ERROR, "fork failed (errno %d)\n", errno);
	My_exit (1);

	case 0:
	if (As_coprocessor) {
	    FILE *fl;
	    int fd;
	    fd = dup (STDERR_FILENO);
	    fl = fdopen (fd, "w");
	    if (fl == NULL) {
		LE_send_msg (GL_ERROR,
			"fdopen (%d) failed (errno %d)\n", fd, errno);
		exit (1);
	    }
	    LE_set_option ("output fd", fl);
	    setsid ();
	    if ((fd = open ("/dev/null", O_RDWR, 0)) < 0 ||
		dup2 (fd, STDIN_FILENO) != STDIN_FILENO ||
		dup2 (fd, STDOUT_FILENO) != STDOUT_FILENO ||
		dup2 (fd, STDERR_FILENO) != STDERR_FILENO) {
		MISC_log ("Failed in setting the STDIO ports\n");
		exit (1);
	    }
	    if (fd != STDIN_FILENO && fd != STDOUT_FILENO && 
						fd != STDERR_FILENO)
		close (fd);
	}
	Terminate_mrpg (0);
	MISC_TO_add_fd (-1);
	return;			/* child */

	default:		/* parent */
	if (Start_in_foreground) {
	    sigset (SIGINT, Signal_handler);
	    sigset (SIGTERM, Signal_handler);
	    sleep (2);
	    Terminate_mrpg (0);
	    while (wait (&stat_loc) < 0) {
		if (errno != EINTR) {
		    LE_send_msg (GL_ERROR, "wait failed (errno %d)\n", errno);
		    My_exit (1);
		}
	    }
	    LE_send_msg (GL_INFO, "mrpg exits with %d\n", stat_loc >> 8);
	    My_exit (stat_loc >> 8);	/* either exit code or signal # */
	}
	My_exit (0);
    }
}

static void Signal_handler (int sig) {

    if (sig == SIGPWR) {
	LE_send_msg (GL_INFO, "Signal SIGPWR (%d) received", sig);
	Sigpwr_received = 1;
	return;
    }

    if (sig == SIGINT) {
	LE_send_msg (GL_INFO, "mrpg is interrupted");
/*	kill (Cpid, SIGKILL); */
    }
    if (Keep_pid != 0)	/* started by a remote mrpg */
	printf ("mrpg parent received signal %d\n", sig);
    My_exit (0);
}

static void My_exit (int status) {
    if (Command == MRPG_RESUME && status != 0) {
	unsigned char alarm_flag;
	ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_RPGCTLFL,
			ORPGINFO_STATEFL_SET, &alarm_flag);
	LE_send_msg (GL_ERROR, 
		"mrpg resume sets alarm before failing (%d)", status);
    }
    if (As_coprocessor && Ppid == getpid ())
	sleep (2);
    exit (status);
}

void MAIN_exit (int status) {
    char tmp[128];

    /* we remove nds so mrpg won't be restarted */
    strcpy (tmp, "prm -9 -quiet nds");
    if (MGC_system ("", tmp, NULL) < 0)
	LE_send_msg (GL_ERROR, "Failed in removing the local nds");

    My_exit (status);
}



