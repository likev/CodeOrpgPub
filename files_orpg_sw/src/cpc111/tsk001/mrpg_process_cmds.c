
/******************************************************************

	file: mrpg_process_cmds.c

	Process mrpg commands.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/25 20:41:10 $
 * $Id: mrpg_process_cmds.c,v 1.108 2013/07/25 20:41:10 steves Exp $
 * $Revision: 1.108 $
 * $State: Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 
#include <mrpg.h>
#include "mrpg_def.h"

#define MAX_N_NODES 128

static int Cmd_fd = -1;		/* file descriptor of the RPG cmd LB */

static int Queued_command = 0;	/* queued command processing started */

/* variables for processing the ready-for-operation reports from processes */
static int Max_n_ready_pids = -1;
				/* The buffer size for Ready_pids. -1 indicates
				   that initialization is required. */
static int *Ready_pids = NULL;	/* Array for storing ready-for-operation 
				   process PIDs */
static int N_ready_pids = 0;	/* number of ready-for-operation process PIDs 
				   */
static int N_lost_ready_events = 0;	
				/* number of lost ready-for-operation events */
static int N_msgs_logged = 0;	/* number of LE messages logged in the CB */
static int Os_crash_start = 0;	/* OS crash startup is needed */


static void Un_callback (int fd, LB_id_t msgid, int msg_info, void *arg);
static int Stop_processes (int sig);
static int Execute_cmd_section (int type);
static int Start_operation (Mrpg_state_t *rpg_state);
static int Shutdown_rpg ();
static int Output_status ();
static int Get_status_size (Mrpg_tat_entry_t *ps);
static int Format_status (Mrpg_tat_entry_t *ps, char *buf, time_t cr_ct);
static int Stop_listed_processes (int sig, int types);
static int Auto_start ();
static void Process_activation (int command);
static int Standby_restart ();
static int Wait_for_cpu_drop (int wait_time, int level);
static int Start_listed_processes (int active, int forced, int types);
static int Post_ds_creation_event ();
static int Restore_data ();
static void Process_ready_event_cb (EN_id_t event, char *msg, 
					int msg_len, void *arg);
static void Init_process_ready_event_processing ();
static int Check_process_ready_reports ();
static char *Interpret_ret_value (int ret);
static int Restart_tasks (int n_tsk, char *tsk_name);
static int Stop_tasks (int n_tsk, char *tsk_name);
static void Process_task_control_cmd (char *cmd, int len);
static void Process_cmd_out (char *cmd, char *out);
static void Kill_a_process (Mrpg_tat_entry_t *pr, int sig);
static int Read_total_cpu (Node_attribute_t *node, 
			unsigned long long *tcpu, unsigned long long *idle);


/******************************************************************

    Initializes this module.

    Returns 0 on success, MRPG_LOCKED if the mrpg state file is 
    locked, or -1 on failure.
	
******************************************************************/

int MPC_init () {

    return (0);
}

/******************************************************************

    Sets Os_crash_start.
	
******************************************************************/

void MPC_set_os_crash_start (int set) {

    Os_crash_start = set;
}

/******************************************************************

    Processes a command.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MPC_process_command (int command) {
    static int auto_start = 0;
    int start_state, ret;
    Mrpg_state_t *rpg_state;

    start_state = MMR_get_rpg_state (&rpg_state);
    if (!auto_start &&
	start_state == MRPG_ST_POWERFAIL && command != MRPG_AUTO_START) {
	LE_send_msg (GL_ERROR, 
		"Command %d not accepted in powerfail state", command);
	return (-1);
    }

    switch (command) {
	case MRPG_STARTUP:
	MMR_set_rpg_state (MRPG_ST_TRANSITION, MRPG_STARTUP);
	LE_send_msg (GL_STATUS, "RPG System is STARTING UP");
	if ((Queued_command && MRT_process_install_cmds () < 0) ||
	    MAIN_clean_up_processes (0, Queued_command) < 0 ||
	    MCD_create_ds (MRPG_STARTUP) < 0 ||
	    MHR_publish_node_info () < 0 ||
	    MRD_publish_data_info () < 0 ||
	    Restore_data () < 0 ||
	    Execute_cmd_section (MRPG_STARTUP) < 0 ||
	    Post_ds_creation_event () < 0 ||
	    (!Queued_command && MAIN_switch_to_deau_db () < 0) ||
	    MRT_set_site_processes () < 0 ||
	    Start_operation (rpg_state) < 0) {
	    MMR_set_rpg_state (MRPG_ST_FAILED, MRPG_STARTUP);
	    return (-1);
	}
	Os_crash_start = 0;
	MMR_set_rpg_state (MRPG_ST_OPERATING, -1);
	if (auto_start)
	    LE_send_msg (GL_STATUS, "RPG System (Auto) Startup Completed");
	else
	    LE_send_msg (GL_STATUS, "RPG System Startup Completed");
	LE_send_msg (LE_VL1, "RPG startup completed");
	break;

	case MRPG_RESTART:
	if (!auto_start && rpg_state->state != MRPG_ST_STANDBY) {
	    LE_send_msg (GL_INFO, 
		"RPG restart command not processed - not in standby state");
	    return (-1);
	}
	MMR_set_rpg_state (MRPG_ST_TRANSITION, MRPG_RESTART);
	LE_send_msg (GL_STATUS, "The RPG system is RESTARTING");
	if ((auto_start && MCD_create_ds (MRPG_RESTART) < 0) ||
	    MHR_publish_node_info () < 0 ||
	    MRD_publish_data_info () < 0 ||
	    Stop_processes (SIGKILL) < 0 ||
	    Execute_cmd_section (MRPG_RESTART) < 0 ||
	    Start_operation (rpg_state) < 0) {
	    MMR_set_rpg_state (MRPG_ST_FAILED, MRPG_RESTART);
	    return (-1);
	}
	MMR_set_rpg_state (MRPG_ST_OPERATING, -1);
	if (auto_start)
	    LE_send_msg (GL_STATUS, "RPG System (Auto) Restart Completed");
	else
	    LE_send_msg (GL_STATUS, "RPG System Restart Completed");
	LE_send_msg (LE_VL1, "RPG restart completed");
	break;

	case MRPG_AUTO_START:
	auto_start = 1;
	ret = Auto_start ();
	if (ret >= 0)
	    LE_send_msg (GL_STATUS, 
			"COMPUTER POWER RESTORED / SYSTEM RESTARTED");
	auto_start = 0;
	return (ret);

	case MRPG_STANDBY_RESTART:
	LE_send_msg (GL_STATUS, "RPG System going to STANDBY/RESTART");
	return (Standby_restart ());

	case MRPG_SHUTDOWN:
	MMR_set_rpg_state (MRPG_ST_TRANSITION, MRPG_SHUTDOWN);
	LE_send_msg (GL_STATUS, "RPG System is SHUTTING DOWN");
	if (Shutdown_rpg () < 0) {
	    MMR_set_rpg_state (MRPG_ST_FAILED, -1);
	    return (-1);
	}
	LE_send_msg (LE_VL1, "RPG processes are notified to shutdown");
	break;

	case MRPG_STANDBY:
	if (!auto_start && rpg_state->state != MRPG_ST_OPERATING) {
	    LE_send_msg (GL_INFO, 
		"RPG standby command not processed - not in operate state");
	    return (-1);
	}
	MMR_set_rpg_state (MRPG_ST_TRANSITION, MRPG_STANDBY);
	LE_send_msg (GL_STATUS, "RPG System going to STANDBY");
	if (Shutdown_rpg () < 0) {
	    MMR_set_rpg_state (MRPG_ST_FAILED, -1);
	    return (-1);
	}
	LE_send_msg (LE_VL1, "RPG processes are notified to standby");
	break;

	case MRPG_POWERFAIL:
	rpg_state->pf_state = rpg_state->state;
	MMR_set_rpg_state (MRPG_ST_POWERFAIL, MRPG_POWERFAIL);
	LE_send_msg (GL_INFO, "Executing RPG power failure command");
	if (start_state == MRPG_ST_OPERATING) {
	    LE_send_msg (GL_STATUS, "RPG System SHUTTING DOWN due to POWER FAILURE");
	    Shutdown_rpg ();
	}
	MAIN_exit (0);
	break;

	case MRPG_STATUS:
	Output_status ();
	break;

	case MRPG_NO_MANAGE:
	MMR_set_manage_off_flag (1);
	break;

	case MRPG_MANAGE:
	MMR_set_manage_off_flag (0);
	break;

	case MRPG_ACTIVE:
	case MRPG_INACTIVE:
	Process_activation (command);
	break;

	case MRPG_INIT:
	if (MCD_create_ds (MRPG_STARTUP) < 0 ||
	    MRD_publish_data_info () < 0 ||
	    Execute_cmd_section (MRPG_INIT) < 0 ||
	    MAIN_switch_to_deau_db () < 0) {
	    return (-1);
	}
	LE_send_msg (GL_INFO, "mrpg init done");
	break;

	default:
	LE_send_msg (GL_ERROR, "unknown RPG command (%d)", command);
	break;
    }
    return (0);
}

/******************************************************************

    Processes the "standby_restart" command.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

#define MAX_TO_STANDBY_TIME 90

static int Standby_restart () {
    Mrpg_state_t *rpg_state;
    time_t st_t, cr_t;

    LE_send_msg (LE_VL1, "RPG standby and restart");
    MMR_set_restart_bit (1);
    if (MPC_process_command (MRPG_STANDBY) < 0) {
	MMR_set_restart_bit (0);
	return (-1);
    }

    MMR_get_rpg_state (&rpg_state);
    st_t = MISC_systime (NULL);
    while (rpg_state->state != MRPG_ST_STANDBY) {
	msleep (1000);
	MMR_housekeep ();
	MPI_housekeep ();
	cr_t = MISC_systime (NULL);
	if (cr_t > st_t + MAX_TO_STANDBY_TIME) {
	    LE_send_msg (GL_ERROR, "Putting to standby timed out");
	    MMR_set_restart_bit (0);
	    return (-1);
	}
    }

    if (MPC_process_command (MRPG_RESTART) < 0) {
	MMR_set_restart_bit (0);
	return (-1);
    }
    MMR_set_restart_bit (0);
    return (0);
}

/******************************************************************

    Processes the "auto_start" command called from OS reboot.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Auto_start () {
    Mrpg_state_t *rpg_st;
    int pre_state;

    LE_send_msg (LE_VL1, "RPG auto start");
    if (Os_crash_start) {
	LE_send_msg (GL_INFO, "Auto_start - after OS crash");
	return (MPC_process_command (MRPG_STARTUP));
    }
    if (MMR_is_rpg_state_new ()) {
	LE_send_msg (GL_INFO, "Auto_start - New state file");
	return (MPC_process_command (MRPG_STARTUP));
    }

    MMR_get_rpg_state (&rpg_st);
    pre_state = rpg_st->state;
    if (rpg_st->state == MRPG_ST_POWERFAIL)
	pre_state = rpg_st->pf_state;
    if (pre_state == MRPG_ST_FAILED ||
	pre_state == MRPG_ST_TRANSITION) {
	return (MPC_process_command (MRPG_STARTUP));
    }
    if (pre_state == MRPG_ST_SHUTDOWN) {
	MMR_set_rpg_state (MRPG_ST_TRANSITION, MRPG_AUTO_START);
	Start_listed_processes (1, 0, MRPG_TT_ALL_STATES);
	MMR_set_rpg_state (pre_state, -1);	/* issue 1-511 */
	LE_send_msg (GL_INFO, 
		"Auto_start to shutdown state - Done");
	return (0);
    }
    if (pre_state == MRPG_ST_STANDBY) {
	MMR_set_rpg_state (MRPG_ST_TRANSITION, MRPG_AUTO_START);
	if (MCD_create_ds (MRPG_RESTART) < 0 ||
	    MHR_publish_node_info () < 0 ||
	    MRD_publish_data_info () < 0) {
	    MMR_set_rpg_state (MRPG_ST_FAILED, -1);
	    return (-1);
	}
	Start_listed_processes (1, 0,
			MRPG_TT_ALL_STATES | MRPG_TT_ALIVE_IN_STANDBY);
	MMR_set_rpg_state (MRPG_ST_STANDBY, -1);
	LE_send_msg (GL_INFO, 
		"auto-start to standby state - Done");
	return (0);
    }

    if (time (NULL) < rpg_st->alive_time + 3600)
	return (MPC_process_command (MRPG_RESTART));
    else
	return (MPC_process_command (MRPG_STARTUP));
}

/******************************************************************

    Processes the MRPG_ACTIVE and MRPG_INACTIVE commands.
	
******************************************************************/

static void Process_activation (int command) {
    if (command == MRPG_ACTIVE) {
	Start_listed_processes (1, 0, 0);
	MMR_set_active (1);
    }
    else {
	Stop_listed_processes (SIGTERM, MRPG_TT_ACTIVE_CHANNEL_ONLY);
	MMR_set_active (0);
    }
}

/******************************************************************

    Terminates all ORPG processes except this. We send a signal "sig"
    to each of the process and, if sig == SIGKILL, wait for them to 
    die. 

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Stop_processes (int sig) {

    LE_send_msg (LE_VL1, "Terminating RPG operational processes");

    while (1) {
	int cnt;
	time_t t;

	Stop_listed_processes (sig, MRPG_TT_ALL_TYPES);
	if (sig != SIGKILL)
	    break;

	t = MISC_systime (NULL);
	msleep (1000);
	while ((cnt = MMR_opp_count ()) > 0 &&
		MISC_systime (NULL) <= t + 10)
	    msleep (1000);
	if (cnt <= 0)
	    break;
    }

    return (0);
}

/******************************************************************

    Terminates all listed ORPG processes, except this, that match
    one of the type bits in "types". "types" = MRPG_TT_ALL_TYPES
    indicates all types.

    Return:	Number of processes killed on success or -1 on failure.
	
******************************************************************/

static int Stop_listed_processes (int sig, int types) {
    Mrpg_tat_entry_t *pr;
    int first, cnt;

    if (MPI_get_process_info () < 0) {	/* get status info */
	LE_send_msg (GL_ERROR, "MPI_get_process_info failed\n");
	return (-1);
    }

    cnt = 0;
    first = 1;
    while ((pr = MRT_get_next_task (first)) != NULL) {

	first = 0;
	if (pr->pid < 0 || strcmp (pr->name, "mrpg") == 0 || 
			(pr->type & MRPG_TT_MONITOR_ONLY))
	    continue;
	if (pr->type & types) {
	    Mrpg_tat_entry_t *dup;
	    int k;

	    if (pr->type & MRPG_TT_ALL_STATES)
		continue;
	    if ((pr->type & MRPG_TT_ALIVE_IN_STANDBY) && 
		MMR_in_standby_state ())
		continue;

	    Kill_a_process (pr, sig);
	    dup = pr->next;
	    for (k = 0; k < pr->n_dupps; k++) {
		Kill_a_process (dup, sig);
		dup = dup->next;
	    }
	    cnt++;
	}
    }

    return (cnt);
}

/******************************************************************

    Kills process "pr" with signal "sig".
	
******************************************************************/

static void Kill_a_process (Mrpg_tat_entry_t *pr, int sig) {
    int ret;

    if (!MHR_is_distributed ()) {	/* non-distributed */
	LE_send_msg (LE_VL3, "    Kill %s (%d)\n", pr->cmd, pr->pid);
	if (kill (pr->pid, sig) < 0 && errno != ESRCH)
	    LE_send_msg (LE_VL0, "Kill %d failed, errno %d\n", 
					pr->pid, errno);
    }
    else {
	LE_send_msg (LE_VL3, "    Kill %s on %s  %d\n", 
					pr->cmd, pr->node->node, pr->pid);
	if ((ret = RSS_kill (pr->node->hname, pr->pid, sig)) < 0)
	    LE_send_msg (LE_VL0, 
		"RSS_kill %d on %s failed, returns %d\n", 
					pr->pid, pr->node->node, ret);
    }
}

/******************************************************************

    Executes intialization commands.

    Input:	type - initialization type.

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Execute_cmd_section (int type) {
    Mrpg_cmd_list_t *cmds;
    int n_cmds;

    if (type == MRPG_STARTUP) {
	LE_send_msg (LE_VL1, "Executing init commands - startup");
	n_cmds = MRT_get_startup_cmds (&cmds);
    }
    else if (type == MRPG_RESTART) {
	LE_send_msg (LE_VL1, "Executing init commands - restart");
	n_cmds = MRT_get_restart_cmds (&cmds);
    }
    else if (type == MRPG_SHUTDOWN) {
	LE_send_msg (LE_VL1, "Executing shutdown commands");
	n_cmds = MRT_get_shutdown_cmds (&cmds);
    }
    else if (type == MRPG_INIT) {
	LE_send_msg (LE_VL1, "Executing init commands");
	n_cmds = MRT_get_init_cmds (&cmds);
    }
    else
	return (0);

    return (MPC_execute_commands (n_cmds, cmds));
}

/******************************************************************

    Executes an array of commands in "cmds" of number "n_cmds".

    Return:	0 on success or -1 on failure.
	
******************************************************************/

int MPC_execute_commands (int n_cmds, Mrpg_cmd_list_t *cmds) {
    int i;

    for (i = 0; i < n_cmds; i++) {
	int ret, n, k;
	Mrpg_cmd_list_t *cd;
	char *out, name[MRPG_NAME_SIZE], *cp;
	Node_attribute_t **nodes;

	cd = cmds + i;
	strncpy (name, cd->cmd, MRPG_NAME_SIZE);  /* get command name */
	name[MRPG_NAME_SIZE - 1] = '\0';
	cp = name + strlen (name) - 1;
	while (cp >= name) {		/* get first token */
	    if (*cp == ' ' || *cp == '\t')
		*cp = '\0';
	    cp--;
	}
	cp = name + strlen (name) - 1;
	while (cp >= name) {		/* remove dir part of the name */
	    if (*cp == '/')
		break;
	    cp--;
	}
	n = MHR_get_task_hosts (cp + 1, &nodes);

	for (k = 0; k < n; k++) {	/* run the command on each node */

	    if (nodes[k]->is_local) {
		LE_send_msg (LE_VL3, "    cmd: %s", cd->cmd);
		ret = MGC_system ("", cd->cmd, &out);
	    }
	    else {
		LE_send_msg (LE_VL3, 
			"    cmd (on %s): %s", nodes[k]->node, cd->cmd);
		ret = MGC_system (nodes[k]->hname, cd->cmd, &out);
	    }
	    if (out != NULL && out[0] != '\0') {
		ORPGMISC_send_multi_line_le (GL_INFO, out);
		Process_cmd_out (cd->cmd, out);
	    }
	    if (ret != 0) {
		if (cd->type & MRPG_STOP_ON_ERROR) {
		    LE_send_msg (GL_ERROR, 
			"RPG init command (%s) failed (%s)", 
				cd->cmd, Interpret_ret_value (ret));
		    if (Queued_command)
			return (-1);
		    else
			MAIN_exit (1);
		}
		else if (ret != 0)
		    LE_send_msg (GL_ERROR, 
				"RPG init command (%s) status: %s", 
				cd->cmd, Interpret_ret_value (ret));
	    }
	}
    }

    return (0);
}

/******************************************************************

    Processes output "out" from command "cmd".
	
******************************************************************/

static void Process_cmd_out (char *cmd, char *out) {
    char tok[128];

    if (MISC_get_token (cmd, "", 0, tok ,128) > 0 &&
	strcmp (MISC_basename (tok), "install_adapt") == 0 &&
	strstr (out, "installed in RPG") != NULL) {
	int ret;

	ret = EN_post_event (ORPGEVT_DATA_STORE_CREATED);
	if (ret < 0)
	    LE_send_msg (GL_ERROR, 
		"EN_post_event (install_adapt) failed (%d)\n", ret);
	if (Queued_command) {		/* swith to the new DEA DB */
	    char *name;
	    DEAU_LB_name ("");
	    if ((name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA)) != NULL)
		DEAU_LB_name (name);
	    LE_send_msg (GL_INFO, "Switched to the new DEA DB");
	}
    }
}

/******************************************************************

    Starts all operational processes.

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Start_operation (Mrpg_state_t *rpg_state) {
    time_t st_time, last_get_time, ret;

    if (Max_n_ready_pids < 0)
	Init_process_ready_event_processing ();
    N_ready_pids = N_lost_ready_events = N_msgs_logged = 0;

    LE_send_msg (LE_VL1, "Starting operational processes");

    if (Start_listed_processes (rpg_state->active, 1, 0) < 0)
	return (-1);
    st_time = MISC_systime (NULL);	/* wait time starts */ 
    LE_send_msg (LE_VL1, "All operational processes started. Waiting for OP ready ...");

    /* wait, for a specified period, for processes to calm down (CPU < 40) */
    ret = Wait_for_cpu_drop (MAIN_wait_time (), 40);
    if (ret == 0)
	LE_send_msg (GL_INFO, "CPU is high after maximum waiting period");

    last_get_time = 0;	/* previous time we get process info */
    while (1) {		/* wait for ready-for-operation reports */
	time_t cr_time;

	cr_time = MISC_systime (NULL);
	if (cr_time > last_get_time) {
	    if (MPI_get_process_info () < 0) {	/* get status info */
		LE_send_msg (GL_ERROR, "MPI_get_process_info failed\n");
		return (-1);
	    }
	    last_get_time = cr_time;
	}
	if (Check_process_ready_reports (cr_time - st_time))
	    return (0);			/* done */
	if (cr_time > st_time + MAIN_wait_time ()) {
	    LE_send_msg (GL_ERROR, 
		"Timed out on waiting for operation ready conditions");
	    return (0);
	}
	msleep (2000);
    }

    return (-1);		/* never reached */
}

/******************************************************************

    Starts all listed ORPG operational processes. "active" is the
    RPG active channel flag. If types != 0, starts only tasks of "types".

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Start_listed_processes (int active, int forced, int types) {
    Mrpg_tat_entry_t *pr;
    int first;

    MPI_clear_task_failure_alarm ();
    first = 1;
    while ((pr = MRT_get_next_task (first)) != NULL) {

	first = 0;
	if (!forced && pr->pid >= 0)
	    continue;

	if (types != 0 && !(pr->type & types))
	    continue;

 	if ((pr->type & MRPG_TT_ALL_STATES) && pr->pid >= 0)
	    continue;

	if ((pr->type & MRPG_TT_ALIVE_IN_STANDBY) && 
	    MMR_in_standby_state () && pr->pid >= 0)
	    continue;

	if (pr->type & MRPG_TT_STOPPED)
	    pr->type &= ~MRPG_TT_STOPPED;
	if ((pr->type & MRPG_TT_ACTIVE_CHANNEL_ONLY) && !active)
	    continue;
	pr->misc &= ~MRPG_MISC_WAIT_FOR_READY;
	if ((pr->type & MRPG_TT_ENABLED) &&
	    (pr->type &	(MRPG_TT_START_ONCE | MRPG_TT_RESPAWN))) {
	    if (pr->type & MRPG_TT_REPORT_READY)
		pr->misc |= MRPG_MISC_WAIT_FOR_READY;
	    if (MPC_execute_op_process (pr) < 0)
		return (-1);
	    else
		msleep (100);
	}
        else if(!(pr->type & MRPG_TT_ENABLED) && 
                !(pr->type & MRPG_TT_MONITOR_ONLY) &&
                !(pr->type & MRPG_TT_DISABLED_CFG)) {
	    if (!Queued_command)
		LE_set_option ("also stderr", 0);
            LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
				"Process %s Disabled\n", pr->name );
	    if (!Queued_command)
		LE_set_option ("also stderr", 1);
	}
    }
    return (0);
}

/******************************************************************

    Completes the shutdown or standby process.
	
******************************************************************/

void MPC_shutdown_done (Mrpg_state_t *rpg_state, int n_processes_to_kill) {

    if (n_processes_to_kill > 0) {
	Stop_listed_processes (SIGKILL, MRPG_TT_ALL_TYPES);
	LE_send_msg (GL_INFO, 
	    "%d operational processes do not terminate - we kill them", 
					n_processes_to_kill);
    }

    if (rpg_state->cmd == MRPG_SHUTDOWN) {
	Execute_cmd_section (MRPG_SHUTDOWN);
	MMR_set_rpg_state (MRPG_ST_SHUTDOWN, MRPG_SHUTDOWN);
	LE_send_msg (LE_VL1, "RPG shutdown procedure done");
    }
    else if (rpg_state->cmd == MRPG_STANDBY) {
	MMR_set_rpg_state (MRPG_ST_STANDBY, MRPG_STANDBY);
	LE_send_msg (LE_VL1, "RPG standby procedure done");
    }
}

/******************************************************************

    Executes an operational processes. Both stderr and stdout are
	piped to a log file.

    Return:	0 on success or -1 on failure.
	
******************************************************************/

int MPC_execute_op_process (Mrpg_tat_entry_t *pr) {
    char buf[512], *out;
    int size, ret;

    if (pr->type & MRPG_TT_NO_LAUNCH)
	goto done;

    size = strlen (pr->cmd) + strlen (pr->node->logdir) + 
			strlen (pr->name) + strlen (pr->node->hname) + 64;
    if (size > 512) {
	LE_send_msg (GL_ERROR, "Executable path too long (%d)\n", size);
	return (-1);
    }

    if (pr->node->node[0] != '\0')
	LE_send_msg (LE_VL2, "    Execute op process (on %s): %s", 
						pr->node->node, pr->cmd);
    else
	LE_send_msg (LE_VL2, "    Execute op process: %s", pr->cmd);
    strcpy (buf, pr->cmd);
    if (pr->instance >= 0)
	sprintf (buf + strlen (buf), 
	    " > %s/%s.%d.output &", pr->node->logdir, pr->name, pr->instance);
    else
	sprintf (buf + strlen (buf), 
	    " > %s/%s.output &", pr->node->logdir, pr->name);

    /* rpgdbm may be started by other programs. So we make sure we kill it. */
    if (strcmp (pr->name, "rpgdbm") == 0) {
	ret = MGC_system (pr->node->hname, "prm -quiet -9 rpgdbm", &out);
	if (ret < 0)
	    LE_send_msg (GL_ERROR, 
		"prm rpgdbm failed (MGC_system returns %d)\n", ret);
	msleep (100);
    }

    MISC_system_setsid (0);
    ret = MGC_system (pr->node->hname, buf, &out);
    MISC_system_setsid (1);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, 
		"Spawning %s failed (MGC_system returns %d)\n", pr->cmd, ret);
	if (out != NULL && out[0] != '\0')
	    LE_send_msg (GL_ERROR, "    - %s\n", out);
	return (-1);
    }

done:
    if (pr->status != MRPG_PS_STARTED)
	pr->st_time = MISC_systime (NULL);
    pr->status = MRPG_PS_STARTED;

    pr->misc &= ~(MRPG_MISC_FAIL_REPORTED);
    return (0);
}

/******************************************************************

    Performs shutdown procedure.

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Shutdown_rpg () {

    /* command p_server to shutdown */
    ORPGNBC_send_NB_link_control_command (CMD_SHUTDOWN, 0);
    if (Stop_processes (SIGTERM) < 0)
	return (-1);
    return (0);
}

/******************************************************************

    Tells HCI that some LBs are recreated. Always return 0.
	
******************************************************************/

static int Post_ds_creation_event () {

    if (MCD_is_lb_created ()) {
	int ret;

	ret = LB_AN_post (ORPGEVT_DATA_STORE_CREATED, NULL, 0);
	if (ret < 0)
	    LE_send_msg (GL_ERROR, "LB_AN_post %d failed (ret %d)\n", 
					ORPGEVT_DATA_STORE_CREATED, ret);
    }

    return (0);
}

/******************************************************************

    Reads and processes commands in the queue.
	
******************************************************************/

int MPC_process_queued_cmds () {

    Queued_command = 1;
    while (1) {
	char buf[1024];
	Mrpg_cmd_t *c;
	int len;

	if (Cmd_fd < 0 && MPC_open_mrpg_cmds_lb () < 0)
	    return (-1);

	len = LB_read (Cmd_fd, buf, 1024, LB_NEXT);
	if (len == LB_TO_COME)
	    break;
	LB_set_tag (Cmd_fd, LB_previous_msgid (Cmd_fd), 1);
	if (len == LB_BUF_TOO_SMALL ||
	    (len > 0 && len < sizeof (Mrpg_cmd_t))) {
	    LE_send_msg (GL_ERROR, "Bad mrpg cmd size (%d)", len);
	    continue;
	}
	if (len == LB_EXPIRED) {
	    LE_send_msg (GL_ERROR, "mrpg cmd lost (LB_EXPIRED)");
	    continue;
	}
	if (len < 0) {
	    LE_send_msg (GL_ERROR, 
		"LB_read mrpg cmd failed (fd %d, ret %d)", Cmd_fd, len);
	    return (-1);
	}
	c = (Mrpg_cmd_t *)buf;
	if (c->id != MRPG_CMD_ID) {
	    LE_send_msg (GL_ERROR, "Bad mrpg cmd id (%d)", c->id );
	    continue;
	}
	if (c->mrpg_cmd == MRPG_STATUS)
	    LE_send_msg (LE_VL3, "Process queued command: %d\n", c->mrpg_cmd);
	else
	    LE_send_msg (LE_VL1, "Process queued command: %d\n", c->mrpg_cmd);
	if (c->mrpg_cmd == MRPG_TASK_CONTROL)
	    Process_task_control_cmd (buf, len);
	else
	    MPC_process_command (c->mrpg_cmd);
    }

    return (0);
}

/******************************************************************

    Opens the RPG command LB and registers UN for it.

    Return:	0 on success or -1 on failure.
	
******************************************************************/

int MPC_open_mrpg_cmds_lb () {
    char name[MRPG_NAME_SIZE];
    int ret;
    LB_info info;

    if (Cmd_fd >= 0)
	return (0);
    if (CS_entry ((char *)ORPGDAT_MRPG_CMDS, 
			1 | CS_INT_KEY, MRPG_NAME_SIZE, name) <= 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGDAT_MRPG_CMDS (%d) not found in system config", 
					ORPGDAT_MRPG_CMDS);
	return (-1);
    }
    Cmd_fd = LB_open (name, LB_WRITE, NULL);
    if (Cmd_fd < 0) {
	LE_send_msg (GL_ERROR, "LB_open %s failed (ret %d)", name, Cmd_fd);
	return (-1);
    }
    if ((ret = LB_UN_register (Cmd_fd, LB_ANY, Un_callback)) < 0) {
	LE_send_msg (GL_ERROR, 
	    "LB_UN_register (ORPGDAT_MRPG_CMDS) failed (ret %d)", ret);
	return (-1);
    }
    if ((ret = LB_lock (Cmd_fd, LB_EXCLUSIVE_LOCK, 1)) != 0) {
	LE_send_msg (GL_ERROR, 
	    "LB_lock (mrpg command LB) failed (ret %d)", ret);
	return (-1);
    }

    info.size = LB_SEEK_TO_COME;
    while (LB_seek (Cmd_fd, -1, LB_CURRENT, &info) >= 0) {
	Mrpg_cmd_t cmd;

	if (info.size == LB_SEEK_EXPIRED ||
	    info.size == LB_SEEK_TO_COME ||
	    info.mark != 0)
	    break;
	if (LB_read (Cmd_fd, (char *)&cmd, sizeof (Mrpg_cmd_t), LB_NEXT) 
						== sizeof (Mrpg_cmd_t)) {
	    if (cmd.time + 1 < MAIN_start_time ())
		break;
	    LB_seek (Cmd_fd, -1, LB_CURRENT, &info);
	}
	else {
	    LE_send_msg (GL_ERROR, 
			"LB_read (mrpg command) failed (ret %d)", ret);
	    break;
	}
    }
    if (info.size != LB_SEEK_TO_COME)
	LB_seek (Cmd_fd, 1, LB_CURRENT, NULL);
    return (0);
}

/******************************************************************

    ORPGDAT_MRPG_CMDS is recreated. Note that the UN registration 
    is automatically deregistered by LB_close.
	
******************************************************************/

void MPC_mrpg_cmds_lb_removed () {
    if (Cmd_fd >= 0) {
	LE_send_msg (LE_VL0, "Closing mrpg command LB");
	LB_close (Cmd_fd);
    }
    Cmd_fd = -1;
}

/******************************************************************

    The RPG command UN callback function.

    Input:	See LB man page.
	
******************************************************************/

static void Un_callback (int fd, LB_id_t msgid, int msg_info, void *arg) {
}

/******************************************************************

    Output RPG process status.

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Output_status () {
    Mrpg_tat_entry_t *dup, *ps;
    int first, size, k;
    char *buf, *cpt;
    Mrpg_state_t *rpg_st;
    time_t cr_t, cr_ct;

    if (MPI_get_process_info () < 0) {	/* get status info */
	Mrpg_process_status_t st;
	memset (&st, 0, sizeof (Mrpg_process_status_t));
	st.size = sizeof (Mrpg_process_status_t);
	ORPGDA_write (ORPGDAT_TASK_STATUS, 
		(char *)&st, sizeof (Mrpg_process_status_t), MRPG_PS_MSGID);
	return (-1);
    }

    MMR_get_rpg_state (&rpg_st);

    /* figure out the size */
    size = 0;
    first = 1;
    while ((ps = MRT_get_next_task (first)) != NULL) {
	first = 0;
	if (ps->type & (MRPG_TT_ENABLED | MRPG_TT_MONITOR_ONLY)) {
	    if ((rpg_st->state != MRPG_ST_OPERATING || 
			(ps->type & MRPG_TT_MONITOR_ONLY)) && ps->pid < 0)
		continue;
	    size += Get_status_size (ps);
	    dup = ps->next;
	    for (k = 0; k < ps->n_dupps; k++) {
		size += Get_status_size (dup);
		dup = dup->next;
	    }
	}
    }
    buf = (char *)malloc (size);
    if (buf == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed (size %d)", size);
	return (-1);
    }

    cr_t = MISC_systime (NULL);
    cr_ct = time (NULL);
    cpt = buf;
    first = 1;
    while ((ps = MRT_get_next_task (first)) != NULL) {
	first = 0;
	if (ps->type & (MRPG_TT_ENABLED | MRPG_TT_MONITOR_ONLY)) {
	    if ((rpg_st->state != MRPG_ST_OPERATING || 
		(ps->type & MRPG_TT_MONITOR_ONLY)) && ps->pid < 0)
		continue;
	    if ((ps->type & MRPG_TT_ACTIVE_CHANNEL_ONLY) && 
					!rpg_st->active && ps->pid < 0)
		continue;
	    cpt += Format_status (ps, cpt, cr_ct);
	    dup = ps->next;
	    for (k = 0; k < ps->n_dupps; k++) {
		cpt += Format_status (dup, cpt, cr_ct);
		dup = dup->next;
	    }
	}
    }
    size = ORPGDA_write (ORPGDAT_TASK_STATUS, buf, cpt - buf, MRPG_PS_MSGID);
    free (buf);
    if (size < 0) {
	LE_send_msg (GL_ERROR, 
			"ORPGDA_write MRPG_PS_MSGID failed (ret %d)", size);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Calculate the size of the status message for process "ps".

    Return:	0 on success or -1 on failure.
	
******************************************************************/

static int Get_status_size (Mrpg_tat_entry_t *ps) {
    int size;
    char *cmd;

    if (ps->pid >= 0)
	cmd = ps->acmd;
    else
	cmd = ps->cmd;
    size = sizeof (Mrpg_process_status_t) + strlen (ps->name) +
			strlen (ps->node->node) + strlen (cmd) + 8;

    return (size);
}

/******************************************************************

    Formats (serializes) the status info for process "ps" and puts 
    it in buffer "buf".

    Return:	The size of the serialized data.
	
******************************************************************/

static int Format_status (Mrpg_tat_entry_t *ps, char *buf, time_t cr_ct) {
    Mrpg_process_status_t *psout;
    char *node, *cmd;
    int s;

    psout = (Mrpg_process_status_t *)buf;
    if (ps->pid < 0)
	cmd = ps->cmd;
    else
	cmd = ps->acmd;
    node = ps->node->node;
    psout->instance = ps->instance;
    psout->pid = ps->pid;
    psout->cpu = ps->cpu;
    psout->mem = ps->mem;
    psout->info_t = cr_ct;
    psout->status = ps->status;
    psout->name_off = sizeof (Mrpg_process_status_t);
    psout->node_off = psout->name_off + strlen (ps->name) + 1;
    psout->cmd_off = psout->node_off + strlen (node) + 1;
    s = psout->cmd_off + strlen (cmd) + 1;
    s = ALIGNED_SIZE (s);
    strcpy (buf + psout->name_off, ps->name);
    strcpy (buf + psout->node_off, node);
    strcpy (buf + psout->cmd_off, cmd);
    if (ps->pid > 0) {
	psout->life = ps->life;
	if (psout->life < 0)
	    psout->life = 0;
    }
    else
	psout->life = 0;
    psout->size = s;
    return (s);
}

/******************************************************************

    Waits, on node "node",  for CPU level droping to "level" (per 
    cent). It waits up to "wait_time" seconds.

    Returns 1 on success, 0 on time-out or -1 on error.
	
******************************************************************/

static int Wait_for_cpu_drop (int wait_time, int level) {
    int ret;
    time_t st;
    int n_nodes, i, cnt;
    Node_attribute_t *nodes;

    n_nodes = MHR_all_hosts (&nodes);
    if (n_nodes > MAX_N_NODES) {
	LE_send_msg (GL_ERROR, "Too many nodes (%d)", n_nodes);
	return (-1);
    }

    st = MISC_systime (NULL);		/* start time */
    cnt = 0;
    while (1) {			/* reads iostat */
	float fcpu, t;
	int cpu, done_cnt, done[MAX_N_NODES];
	unsigned long long tcpu, idle, last_tcpu[MAX_N_NODES], 
						last_idle[MAX_N_NODES];

	done_cnt = 0;
	for (i = 0; i < n_nodes; i++) {

	    if (cnt == 0)
		done[i] = 0;
	    if (done[i]) {
		done_cnt++;
		continue;
	    }
	    ret = Read_total_cpu (nodes + i, &tcpu, &idle);
	    if (ret < 0) {
		done[i] = 1;
		ret = 0;
		done_cnt++;
		continue;
	    }
	    if (cnt == 0 || tcpu < last_tcpu[i] || idle < last_idle[i]) {
		last_tcpu[i] = tcpu;
		last_idle[i] = idle;
		continue;
	    }

	    t = (double)tcpu + (double)idle - 
			(double)last_tcpu[i] - (double)last_idle[i];
	    fcpu = 0.;
	    if (t > 0.)
		fcpu = ((double)tcpu - (double)last_tcpu[i]) * 100. / t;
	    last_tcpu[i] = tcpu;
	    last_idle[i] = idle;
	    if (t <= 0.)
		continue;

	    cpu = fcpu + .5;
	    if (cpu > level) {
		if (nodes[i].node[0] != '\0')
		    LE_send_msg (LE_VL1, 
			"system busy on %s (cpu: %d percent)", 
						nodes[i].node, cpu);
		else
		    LE_send_msg (LE_VL1, 
			"system busy (cpu: %d percent)", cpu);
	    }
	    if (cpu < level) {
		done[i] = 1;
		break;
	    }
	    if (done[i])
		done_cnt++;
	}
	if (done_cnt >= n_nodes) {
	    ret = 1;
	    break;
	}
	cnt++;
	for (i = 0; i < 10; i++)
	    msleep (100);
	if (MISC_systime (NULL) - st >= wait_time) {
	    ret = 0;
	    break;
	}
    }
    return (ret);
}

/***********************************************************************

    Read total cpu usage on node "node". The accumulated cpu usage and
    idle time are returned with "tcpu" and "idle". Returns 0 on success
    or -1 on failure.

***********************************************************************/

#define STAT_READ_BUF_SIZE 128

static int Read_total_cpu (Node_attribute_t *node, 
			unsigned long long *tcpu, unsigned long long *idle) {
    unsigned long long d1, d2, d3, d4;
    int fd, ret;
    char buf[STAT_READ_BUF_SIZE], tok[32], path[MRPG_NAME_SIZE];

    sprintf (path, "%s:/proc/stat", node->hname);
    fd = RSS_open (path, O_RDONLY, 0);
    if (fd < 0) {
	LE_send_msg (GL_ERROR, "RSS_open %s failed (%d)\n", path, fd);
	return (-1);
    }

    ret = RSS_read (fd, buf, STAT_READ_BUF_SIZE - 1);
    RSS_close (fd);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, "RSS_read %s failed (%d)\n", path, ret);
	return (-1);
    }
    buf[ret] = '\0';

    if (MISC_get_token (buf, "", 0, tok, 32) <= 0 ||
	strcmp (tok, "cpu") != 0) {
	LE_send_msg (GL_ERROR, "First token in /proc/stat is not cpu\n");
	return (-1);
    }
 
    if (sscanf (buf, "%*s %Ld %Ld %Ld %Ld", &d1, &d2, &d3, &d4) < 4) {
	LE_send_msg (GL_ERROR, "Expected fields not found in /proc/stat\n");
	return (-1);
    }
    *tcpu = d1 + d2 + d3;
    *idle = d4;

    return (0);
}

/******************************************************************

    Calls command "rescue_data restore" to verify and restore data.
	
******************************************************************/

static int Restore_data () {

    if (!Queued_command && Os_crash_start && MAIN_is_operational ()) {
	char buf[256];
	int ret;

	LE_send_msg (GL_STATUS, "Recovering From Operating System Crash...");
	LE_send_msg (GL_STATUS, "All Previous State and Historical Data lost");

	ret = MISC_system_to_buffer ("rescue_data restore", buf, 256, NULL);
	if (buf[0] != '\0')
	    ORPGMISC_send_multi_line_le (GL_INFO, buf);
	if (ret != 0)
	    LE_send_msg (GL_INFO, "rescue_data restore not possible");
    }
    return (0);
}

/******************************************************************

    Allocates memory and registers for event ORPGEVT_PROCESS_READY
    in preparison for receiving ready-for-operation reports from the 
    processes.
	
******************************************************************/

static void Init_process_ready_event_processing () {

    if (Max_n_ready_pids < 0) {
	int first, size, ret;
	Mrpg_tat_entry_t *pr;

	first = 1;
	size = 0;
	while ((pr = MRT_get_next_task (first)) != NULL) {
	    first = 0;
	    size++;
	}

	Ready_pids = (int *)malloc (size * sizeof (int));
	if (Ready_pids == NULL) {
	    LE_send_msg (GL_ERROR, 
		"malloc failed (size %d)", size * sizeof (int));
	    return;
	}

	ret = EN_register (ORPGEVT_PROCESS_READY, Process_ready_event_cb);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
		"EN_register (ORPGEVT_PROCESS_READY) failed (%d)\n", ret);
	    free (Ready_pids);
	    return;
	}
	Max_n_ready_pids = size;
    }
    return;
}

/******************************************************************

    ORPGEVT_PROCESS_READY event callback fuction.

******************************************************************/

#define MAX_MSGS_IN_CB 10	/* to prevent from too many mrpg messages */

static void Process_ready_event_cb (EN_id_t event, char *msg, 
					int msg_len, void *arg) {
    int pid, i;

    N_lost_ready_events += EN_event_lost ();

    if (msg_len <= 0) {
	if (N_msgs_logged < MAX_MSGS_IN_CB)
	    LE_send_msg (GL_ERROR, 
		"Unexpected ORPGEVT_PROCESS_READY event (no message)");
	N_msgs_logged++;
	return;
    }
    if (sscanf (msg, "%d", &pid) != 1) {
	msg[msg_len - 1] = '\0';
	if (N_msgs_logged < MAX_MSGS_IN_CB)
	    LE_send_msg (GL_ERROR, 
		"Unexpected ORPGEVT_PROCESS_READY event message (%s)", msg);
	N_msgs_logged++;
	return;
    }
    for (i = 0; i < N_ready_pids; i++) {
	if (pid == Ready_pids[i]) {
	    if (N_msgs_logged < MAX_MSGS_IN_CB)
		LE_send_msg (GL_ERROR, 
		    "Duplicated ORPGEVT_PROCESS_READY event (pid %d)", pid);
	    N_msgs_logged++;
	    return;
	}
    }
    if (N_ready_pids >= Max_n_ready_pids) {
	if (N_msgs_logged < MAX_MSGS_IN_CB)
	    LE_send_msg (GL_ERROR, 
		"Too many ORPGEVT_PROCESS_READY event (pid %d)", pid);
	N_msgs_logged++;
	N_lost_ready_events++;
	return;
    }
    Ready_pids[N_ready_pids] = pid;
    N_ready_pids++;
}

/******************************************************************

    Checks if ready-for-operation events are received from all 
    processes that are required to report. Returns 1 is further
    waiting is not necessary or 0 otherwise.
	
******************************************************************/

static int Check_process_ready_reports (int seconds_passed) {
    static int last_report = 0;
    Mrpg_tat_entry_t *prc;
    int first, cnt;
    char buf[128];

    buf[0] = '\0';
    cnt = 0;		/* number of processes that are to send report */
    first = 1;
    while ((prc = MRT_get_next_task (first)) != NULL) {
	int k;

	first = 0;
	if (!(prc->misc & MRPG_MISC_WAIT_FOR_READY))
	    continue;
	if (prc->pid < 0) {
	    prc->misc &= ~MRPG_MISC_WAIT_FOR_READY;
	    continue;
	}
	for (k = 0; k < N_ready_pids; k++) {
	    if (Ready_pids[k] == prc->pid) {
		prc->misc &= ~MRPG_MISC_WAIT_FOR_READY;
		break;
	    }
	}
	if (k < N_ready_pids)
	    continue;
	if (cnt < 5)
	    sprintf (buf + strlen (buf), "%s ", prc->name);
	cnt++;
    }
    if (cnt <= N_lost_ready_events) {
	if (N_lost_ready_events > 0)
	    LE_send_msg (GL_ERROR, 
		"Some (%d) ORPGEVT_PROCESS_READY event lost", 
					N_lost_ready_events);
	return (1);
    }
    if (seconds_passed - last_report > 10) {
	if (N_lost_ready_events == 0)
	    LE_send_msg (GL_INFO, 
		"Still waiting OP ready reports from %s...", buf);
	else
	    LE_send_msg (GL_INFO, 
		"Still waiting OP ready reports from, possibly, %s...", buf);
	last_report = seconds_passed;
    }
    return (0);
}

/******************************************************************

    Converts waitpid return value "ret" to a decriptive string and 
    returns it. Refer to "man wstat".
	
******************************************************************/

static char *Interpret_ret_value (int ret) {
    static char buf[64];

    if (WIFSIGNALED (ret))
	sprintf (buf, "killed by sig %d", WTERMSIG (ret));
    else if (WIFEXITED (ret))
	sprintf (buf, "exit %d", WEXITSTATUS (ret));
    else
	sprintf (buf, "%d", ret);
    return (buf);
}

/******************************************************************

    Processes mrpg command MRPG_TASK_CONTROL in "cmd" of length 
    "len".
	
******************************************************************/

static void Process_task_control_cmd (char *cmd, int len) {
    char *c, *tsk_name, *t, log[64];
    int n_tsk, event;

    cmd += sizeof (Mrpg_cmd_t);
    len -= sizeof (Mrpg_cmd_t);
    if (cmd[len - 1] != '\0')
	cmd[len - 1] = '\0';

    strncpy (log, cmd, 45);
    if (strlen (cmd) > 45)
	strcpy (log + 45, "...");
    LE_send_msg (GL_INFO, "Process task control command: %s", log);
    c = strtok (cmd, " \t\n");
    if (c == NULL) {
	LE_send_msg (GL_ERROR, "Bad task control command (%s)", log);
	return;
    }
    event = -1;
    n_tsk = 0;
    tsk_name = NULL;
    while ((t = strtok (NULL, " \t\n")) != NULL) {
	if (strncmp (t, "ACK-", 4) == 0) {
	    if (sscanf (t + 4, "%d", &event) != 1)
		event = -1;
	}
	else {
	    tsk_name = STR_append (tsk_name, t, strlen (t) + 1);
	    n_tsk++;
	}
    }

    if (strcmp (c, "STOP") == 0) {
	int ret;
	ret = Stop_tasks (n_tsk, tsk_name);
	if (ret >= 0 && event >= 0)
	    EN_post_event (event);
    }
    else if (strcmp (c, "RESTART") == 0) {
	Restart_tasks (n_tsk, tsk_name);
    }

    STR_free (tsk_name);
}

/******************************************************************

    Stops a list of "n_tsk" tasks in "tsk_name". It waits until
    all those processes are actually stopped. Returns 0 on success
    or -1 on failure.
	
******************************************************************/

static int Stop_tasks (int n_tsk, char *tsk_name) {
    int cnt, n, ret;
    time_t st_t;
    char *name;

    name = tsk_name;
    cnt = 0;
    for (n = 0; n < n_tsk; n++) {
	Mrpg_tat_entry_t *pr;
	int found, first;

	found = 0;
	first = 1;
	while ((pr = MRT_get_next_task (first)) != NULL) {
	    first = 0;
	    if (strcmp (pr->name, name) != 0)
		continue;
	    found = 1;
	    if ((pr->type & MRPG_TT_ENABLED) &&
		!(pr->type & MRPG_TT_STOPPED) &&
		!(pr->type & MRPG_TT_MONITOR_ONLY)) {
		char cmd[256], *out;

		sprintf (cmd, "prm -q -9 %s", pr->name);
		if (MHR_is_distributed ()) {
		    LE_send_msg (GL_INFO, 
				"Stoping %s on %s", pr->name, pr->node->node);
		    ret = MGC_system (pr->node->hname, cmd, &out);
		}
		else {
		    LE_send_msg (GL_INFO, "Stoping %s", pr->name);
		    ret = MGC_system ("", cmd, &out);
		}
		if (out != NULL && out[0] != '\0')
		    LE_send_msg (GL_INFO, "%ms", out);

		pr->type |= MRPG_TT_STOPPED; 
		cnt++;
	    }
	}
	if (!found)
	    LE_send_msg (GL_INFO, "Task stop - Task %s not found", name);
	name += strlen (name) + 1;
    }

    if (cnt <= 0)
	return (0);
    st_t = MISC_systime (NULL);
    while (1) {					/* wait for all task to stop */
	Mrpg_tat_entry_t *pr;

	pr = NULL;
	if (MPI_get_process_info () >= 0) {	/* get status info */
	    int done, first;

	    done = 1;
	    first = 1;
	    while ((pr = MRT_get_next_task (first)) != NULL) {
		first = 0;
		if ((pr->type & MRPG_TT_STOPPED) &&
		    pr->pid >= 0) {
		    done = 0;
		    break;
		}
	    }
	    if (done)
		return (0);
	}
	if (MISC_systime (NULL) >= st_t + 20) {
	    if (pr != NULL)
		LE_send_msg (GL_INFO, 
			"Task control - task %s does not stop", pr->name);
	    else 
		LE_send_msg (GL_INFO, 
			"Task control - task info not available");
	    return (-1);
	}
	msleep (200);
    }
    return (0);
}

/******************************************************************

    Restarts a list of "n_tsk" tasks in "tsk_name". Returns 0 on 
    success or -1 on failure.
	
******************************************************************/

static int Restart_tasks (int n_tsk, char *tsk_name) {
    int n;
    char *name;

/*    MPI_clear_task_failure_alarm (); */
    name = tsk_name;
    for (n = 0; n < n_tsk; n++) {
	Mrpg_tat_entry_t *pr;
	int found, first;

	found = 0;
	first = 1;
	while ((pr = MRT_get_next_task (first)) != NULL) {    
	    first = 0;
	    if (strcmp (pr->name, name) != 0)
		continue;
	    found = 1;
	    if ((pr->type & MRPG_TT_ENABLED) &&
		(pr->type & MRPG_TT_STOPPED) &&
		!(pr->type & MRPG_TT_MONITOR_ONLY) &&
		pr->pid < 0) {
		LE_send_msg (LE_VL2, "Restart %s", pr->cmd);
		MPC_execute_op_process (pr);
		pr->type &= ~MRPG_TT_STOPPED; 
		pr->status = MRPG_PS_STARTED;
	    }
	}
	if (!found)
	    LE_send_msg (GL_INFO, "Task restart - Task %s not found", name);
	name += strlen (name) + 1;
    }
    return (0);
}




