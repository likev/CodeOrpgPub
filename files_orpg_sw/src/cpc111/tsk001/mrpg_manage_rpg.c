
/******************************************************************

	file: mrpg_manage_rpg.c

	Manage RPG.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2015/01/05 20:35:18 $
 * $Id: mrpg_manage_rpg.c,v 1.91 2015/01/05 20:35:18 steves Exp $
 * $Revision: 1.91 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <orpg.h> 
#include <infr.h> 
#include <mrpg.h>
#include <orpgred.h>          /*  ORPGRED api */
#include "mrpg_def.h"
#include <rss.h>

#define RPG_STATE_PUBLISH_PERIOD 10
				/* period for writing RPG state */
#define RPG_CHECK_CRON_JOB_PERIOD 2
#define RPG_MANAGE_PERIOD 4	/* period for managing RPG */
#define RPG_CHECK_DONE_PERIOD 2	/* period for checking panding procedure */
#define RPG_KILL_DUP_TIME 10	/* minimum life time a duplicated process is
				   terminated */
#define RPG_MIN_DEAD_TIME 30	/* minimum time in seconds after process 
				   failure before process respawning and 
				   saving logs can happen */

static int Manage_off = 0;	/* turns off the managing functions */

static char Mrpg_state_name[MRPG_NAME_SIZE] = "";
				/* name of the RPG state LB */
static int State_fd = -1;	/* file descriptor of the RPG state LB */
static Mrpg_state_t RPG_state;
static int New_rpg_state = 0;	/* The RPG state file is newly created */
static int Cr_state_time = 0;	/* systime of the latest RPG_state.st_time */

typedef struct {		/* struct for task history */
    int pid;			/* pid */
    time_t llife;		/* life time, in seconds, of pid */
    int lcpu;			/* cpu usage at time of "llife" */
    int used;
} Proc_history_t;

static int Cpu_limit = 100;	/* process CPU util, in percentage, limit */
static int Mem_limit = 0x7fffffff;	/* process memory util, in MB, limit */
static int Cpu_window = 12;	/* time window in seconds for CPU util */

/* values for argument "func" of function Store_process_history_info */
enum {PHS_START, PHS_GET, PHS_CLEANUP, PHS_RM_HISTORY};

static int Set_restart_bit = 0;	/* The "Restart" bit of RPG status needs to be
				   set */
static char Save_log_cmd[MRPG_NAME_SIZE] = "";

typedef struct {		/* struct for cron job */
    time_t scheduled_time;	/* The next scheduled job time */
    char *cmd;			/* the command to execute */
} Cron_job_t;

static void *Cron_job_tbl = NULL;	/* Cron job table id */
static Cron_job_t *Cron_jobs;		/* Cron job table. */
static int N_cron_jobs = 0;		/* size of the Cron job table */

static unsigned int Calculate_check_sum ();
static int Write_rpg_state ();
static void Respawn_op_processes (time_t cr_t);
static int Publish_state ();
static void Check_process_resource_utils ();
static void Check_a_process (Mrpg_tat_entry_t *pr);
static Proc_history_t *Store_process_history_info (int func, int pid);
static int Pid_cmp (void *e1, void *e2);
static void Verify_redundant_type ();
static int Publish_rpg_state ();
static void Save_logs (time_t cr_t);
static void Remove_unexpected_processes (time_t t);
static int Remove_duplicated_processes (int sig, time_t cr_t);


/******************************************************************

    Opens an existing RPG state file and lock it.

    Input:	wait - wait until the lock is gained.

    Returns 0 on success, MRPG_LOCKED if the mrpg state file is 
    locked, or -1 on failure.
	
******************************************************************/

int MMR_open_state (int wait) {
    int ret;

    if (MMR_state_file_name () == NULL)
	return (-1);

    State_fd = LB_open (Mrpg_state_name, LB_WRITE, NULL);
    if (State_fd >= 0) {
	while (1) {		/* try to lock the file */
	    ret = LB_lock (State_fd, LB_EXCLUSIVE_LOCK, LB_LB_LOCK);
	    if (ret == LB_HAS_BEEN_LOCKED) {
		if (!wait)
		    return (MRPG_LOCKED);
		else
		    msleep (200);
	    }
	    else if (ret < 0) {
		LE_send_msg (GL_ERROR,
			"LB_lock mrpg state failed (ret %d)", ret);
		return (-1);
	    }
	    else
		break;
	}
	if (LB_read (State_fd, (char *)&RPG_state, sizeof (Mrpg_state_t), 
				MRPG_STATE_MSGID) != sizeof (Mrpg_state_t) ||
	    Calculate_check_sum () != RPG_state.check_sum) {
	    LB_close (State_fd);
	    State_fd = -1;
	}
    }

    return (0);
}

/******************************************************************

    Saves resource name in the RPG state file. Returns 0 on success
    or a negetive error code.
	
******************************************************************/

int MMR_save_resource_name () {
    char res_name[MRPG_NAME_SIZE];
    int ret;

    MHR_get_resource_name (res_name, MRPG_NAME_SIZE);
    if ((ret = LB_write (State_fd, res_name, strlen (res_name) + 1, 
		MRPG_STATE_RES_NAME)) <= 0) {
	LE_send_msg (GL_ERROR, 
			"LB_write (RPG state res name) failed (ret %d)", ret);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Reads the resource name from the RPG state file and returns it
    in "buf" of size "buf_size". If the info is not found, it returns
    an empty string. It returns -1 if the state file does not exist 
    or it exists, but is not correct or been locked. Otherwise, it 
    returns 0.
	
******************************************************************/

int MMR_read_resource_name (char *buf, int buf_size) {
    int fd, failed;
    char b[512];

    buf[0] = '\0';
    failed = -1;
    if (MMR_state_file_name () == NULL) {
	LE_send_msg (0, "mrpg state file name not found");
	return (failed);
    }

    fd = LB_open (Mrpg_state_name, LB_READ, NULL);
    if (fd >= 0) {
	int ret;
	ret = LB_lock (fd, LB_EXCLUSIVE_LOCK, LB_LB_LOCK);
	if (ret != LB_HAS_BEEN_LOCKED) {
	    ret = LB_read (fd, b, 512, MRPG_STATE_RES_NAME);
	    if (ret > 0) {
		b[511] = '\0';
		strncpy (buf, b, buf_size);
		buf[buf_size - 1] = '\0';
		failed = 0;
	    }
	    else
		LE_send_msg (0, "LB_read resource name failed (%d)", ret);
	}
	else
	    LE_send_msg (0, "mrpg is currently running");
	LB_close (fd);
    }
    else
	LE_send_msg (0, "RPG state file not found");
    return (failed);
}

/******************************************************************

    Closes the RPG state file.
	
******************************************************************/

void MMR_close_state_file () {

    if (State_fd >= 0)
	LB_close (State_fd);
    State_fd = -1;
}

/******************************************************************

    Initializes this module.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MMR_init () {

    return (0);
}

/******************************************************************

    Verifies and resets redundant type.

******************************************************************/

static void Verify_redundant_type () {
    int active;

    active = MRPG_ST_ACTIVE;
    if (MMR_is_faa_redundant ())
	active = MRPG_ST_INACTIVE;
    if (active != RPG_state.active) {
	LE_send_msg (LE_VL0, "RPG active changed to %d", active);
	RPG_state.active = active;
    }
    return;
}

/******************************************************************

    Sets Cpu_limit and Mem_limit.
	
******************************************************************/

void MMR_set_limits (int cpu_limit, int mem_limit, int cpu_window) {
    Cpu_limit = cpu_limit;
    Mem_limit = mem_limit;
    Cpu_window = cpu_window;
}

/******************************************************************

    Accepts a command "cmd" for saving log files.
	
******************************************************************/

void MMR_set_save_log_cmd (char *cmd) {
    strncpy (Save_log_cmd, cmd, MRPG_NAME_SIZE);
    Save_log_cmd[MRPG_NAME_SIZE - 1] = '\0';
}

/******************************************************************

    MMR housekeeping function.
	
******************************************************************/

void MMR_housekeep () {
    static time_t last_state_time = 0;
    static time_t last_manage_time = 0;
    static time_t last_check_done_time = 0;
    static time_t last_cron_job_time = 0;
    time_t t;
    int i;

    t = MISC_systime (NULL);
    if (t >= last_state_time + RPG_STATE_PUBLISH_PERIOD) {
	last_state_time = t;
	Write_rpg_state ();
    }

    MHR_check_node_connectivity ();

    if (t >= last_manage_time + RPG_MANAGE_PERIOD) {
	last_manage_time = t;
	if (!Manage_off && RPG_state.state == MRPG_ST_OPERATING) {
	    Remove_duplicated_processes (SIGTERM, t);
	    Remove_unexpected_processes (t);
	    Save_logs (t);
	    Respawn_op_processes (t);
	    Check_process_resource_utils ();
	}
    }

    if (t >= last_check_done_time + RPG_CHECK_DONE_PERIOD) {
	last_check_done_time = t;
	if (RPG_state.state == MRPG_ST_TRANSITION && 
	    (RPG_state.cmd == MRPG_SHUTDOWN ||
	     RPG_state.cmd == MRPG_STANDBY)) {
	    int cnt = MMR_opp_count ();
	    if (cnt == 0 ||
		t >= Cr_state_time + MAX_PROCESS_TERM_TIME) {
		MPC_shutdown_done (&RPG_state, cnt);
	    }
	}
    }

    if (MAIN_is_operational () &&
	t >= last_cron_job_time + RPG_CHECK_CRON_JOB_PERIOD) {
	last_cron_job_time = t;
	for (i = 0; i < N_cron_jobs; i++) {
	    Cron_job_t *j;
	    j = Cron_jobs + i;
	    if (time (NULL) >= j->scheduled_time) {
		int ret;
		char *buf = MISC_malloc (strlen (j->cmd) + 32);
		sprintf (buf, "sh -c (%s)", j->cmd);
		ret = MISC_system_to_buffer (buf, NULL, 0, NULL);
		if (strlen (buf) > 100)
		    buf[100] = '\0';
		if (ret != 0)
		    LE_send_msg (GL_STATUS, 
			    "Cron job \"%s\" may be unsuccessful", buf);
		else
		    LE_send_msg (GL_INFO, "Run cron job: \"%s\"", buf);
		free (buf);
		j->scheduled_time += 24 * 60 * 60;
	    }
	}
    }
}

/******************************************************************

    Adds and schedules a new cron job. Returns 0 success or -1 on 
    failure.

******************************************************************/

int MMR_add_cron_job (int hour, int minute, char *cmd) {
    Cron_job_t *new;
    char *p;
    time_t cr_t, t;

    if (Cron_job_tbl == NULL &&
	(Cron_job_tbl = MISC_open_table (sizeof (Cron_job_t), 
			4, 0, &N_cron_jobs, (char **)&Cron_jobs)) == NULL)
	goto err;

    p = malloc (strlen (cmd) + 32);
    new = MISC_table_new_entry (Cron_job_tbl, NULL);
    if (p == NULL || new == NULL)
	goto err;
    cr_t = time (NULL);
    t = cr_t - (cr_t % (24 * 60 * 60));		/* beginning of today */
    t = t + (hour * 60 + minute) * 60;
    if (t < cr_t)
	t += 24 * 60 * 60;
    new->scheduled_time = t;
    new->cmd = p;
    strcpy (p, cmd);
    return (0);

err:
    LE_send_msg (GL_ERROR, "malloc failed\n");
    return (-1);
}

/******************************************************************

    Removes duplicated operatonal processes.

    Returns the number of processes killed.
	
******************************************************************/

static int Remove_duplicated_processes (int sig, time_t cr_t) {
    int first, cnt;
    Mrpg_tat_entry_t *pr;

    /* remove duplicated processes */
    cnt = 0;
    first = 1;
    while ((pr = MRT_get_next_task (first)) != NULL) {
	Mrpg_tat_entry_t *dup;
	int k;

	first = 0;
	if (pr->n_dupps == 0 ||
	    (pr->type & (MRPG_TT_MONITOR_ONLY | MRPG_TT_ALLOW_DUPLICATE))) {
	    continue;
	}
	dup = pr->next;
	for (k = 0; k < pr->n_dupps; k++) {
	    if (cr_t > 0 && cr_t < dup->st_time + RPG_KILL_DUP_TIME)
		continue;
	    if (!MHR_is_distributed ()) {	/* non-distributed */
		LE_send_msg (LE_VL0, "kill duplicated process %s (pid %d)\n", 
							dup->acmd, dup->pid);
		kill (dup->pid , sig);
	    }
	    else {
		LE_send_msg (LE_VL0, 
			"kill duplicated process %s (pid %d) on %s\n", 
					dup->acmd, dup->pid, dup->node->node);
		RSS_kill (dup->node->hname, dup->pid , sig);
	    }
	    cnt++;
	    dup = dup->next;
	}
    }
    return (cnt);
}

/******************************************************************

    Returns the number of operational processes that should be 
    terminated in the current state but still running.
	
******************************************************************/

int MMR_opp_count () {
    Mrpg_tat_entry_t *prc;
    int cnt, first;

    if (MPI_get_process_info () < 0)	/* get status info */
	return (0);

    first = 1;
    cnt = 0;
    while ((prc = MRT_get_next_task (first)) != NULL) {
	first = 0;
	if (prc->pid < 0 || (prc->type & MRPG_TT_MONITOR_ONLY))
	    continue;
 	if (prc->type & MRPG_TT_ALL_STATES)
	    continue;
	if ((prc->type & MRPG_TT_ALIVE_IN_STANDBY) && 
	    MMR_in_standby_state ())
	    continue;
	cnt++;
    }
    return (cnt);
}

/******************************************************************

    Returns non-zero if RPG is in standby state or in transition 
    to or from standby state.
	
******************************************************************/

int MMR_in_standby_state () {
    if (RPG_state.state == MRPG_ST_STANDBY ||
	(RPG_state.state == MRPG_ST_TRANSITION && 
			RPG_state.cmd == MRPG_STANDBY) ||
	(RPG_state.state == MRPG_ST_TRANSITION && 
			RPG_state.cmd == MRPG_RESTART))
	return (1);
    else
	return (0);
}

/******************************************************************

    Respawns failed RPG operational processes. "cr_t" is the current
    time.
	
******************************************************************/

static void Respawn_op_processes (time_t cr_t) {
    Mrpg_tat_entry_t *pr;
    int first;
    Mrpg_state_t *rpg_state;

    MMR_get_rpg_state (&rpg_state);

    first = 1;
    while ((pr = MRT_get_next_task (first)) != NULL) {
	first = 0;
	if ((pr->type & MRPG_TT_ACTIVE_CHANNEL_ONLY) && !rpg_state->active)
	    continue;
	if ((pr->type & MRPG_TT_ENABLED) &&
	    (pr->type &	MRPG_TT_RESPAWN) &&
	    !(pr->type & MRPG_TT_STOPPED) &&
	    pr->status == MRPG_PS_FAILED &&
	    cr_t > pr->st_time + RPG_MIN_DEAD_TIME) {
	    LE_send_msg (LE_VL2, "Respawn %s", pr->cmd);
	    MPC_execute_op_process (pr);
	}
    }
}

/******************************************************************

    Checks process CPU and memory utilization. Bad processes are 
    removed. All active processes are checked.
	
******************************************************************/

static void Check_process_resource_utils () {
    Mrpg_tat_entry_t *pr;
    int first;

    if (MPI_get_process_info () != 0)
	return;

    Store_process_history_info (PHS_START, 0);
    first = 1;
    while ((pr = MRT_get_next_task (first)) != NULL) {
	Mrpg_tat_entry_t *dup;
	int k;

	first = 0;
	if (pr->status == MRPG_PS_ACTIVE)
	    Check_a_process (pr);
	dup = pr->next;
	for (k = 0; k < pr->n_dupps; k++) {
	    if (dup->status == MRPG_PS_ACTIVE)
		Check_a_process (dup);
	    dup = dup->next;
	}
    }
    Store_process_history_info (PHS_CLEANUP, 0);
}

/******************************************************************

    Saves RPG log files in case an operational process fails. Logs
    are saved only if a process has failed for certain time. This will
    allow tasks such as cm_uconx to start a new instance. "cr_t" is
    the current time.
	
******************************************************************/

#define MAX_CMD_LENGTH 512

static void Save_logs (time_t cr_t) {
    static time_t log_time = 0;
    Mrpg_tat_entry_t *pr;
    int first, ret;
    char cmd[MAX_CMD_LENGTH];

    if (!MAIN_is_operational ())
	return;

    if (cr_t == log_time || Save_log_cmd[0] == '\0')
	return;
    first = 1;
    while ((pr = MRT_get_next_task (first)) != NULL) {

	first = 0;
	if ((pr->type & MRPG_TT_ENABLED) &&
	    !(pr->type & MRPG_TT_STOPPED) &&
	    pr->status == MRPG_PS_FAILED &&
	    (RPG_state.active ||
			!(pr->type & MRPG_TT_ACTIVE_CHANNEL_ONLY)) &&
	    pr->st_time >= log_time &&
	    cr_t >= pr->st_time + RPG_MIN_DEAD_TIME) {
	    char label[32];

	    strcpy (label, "mrpg_");
	    strncpy (label + strlen (label), pr->name, 16);
	    label[21] = '\0';
	    LE_send_msg (GL_STATUS, "Saved log due to %s failure", pr->name);
	    sprintf (cmd, "exec_on_all_nodes \"%s -s -a %s\" &", 
					Save_log_cmd, label);
	    ret = MISC_system_to_buffer (cmd, NULL, 0, NULL);
	    LE_send_msg (0, "Save log (ret %d, cmd %s", ret, cmd);

	    log_time = cr_t;
	    break;
	}
    }
}

/******************************************************************

    Checks CPU and memory utilization of process "pr". The process 
    is removed if any of its resource utils exceeds the limit.
	
******************************************************************/

static void Check_a_process (Mrpg_tat_entry_t *pr) {
    int bad, mem_lmt;
    char buf[256];

    bad = 0;
    mem_lmt = Mem_limit;
    if (pr->mem_limit > 0)
	mem_lmt = pr->mem_limit;
    if (pr->mem > mem_lmt * 1024 || pr->swap > mem_lmt * 2048) {
	sprintf (buf, 
	    "Process %s Memory Utilization (%d, %d) Too High - Process Killed",
            pr->name, pr->mem, pr->swap);
	bad = 1;
    }

    if (pr->cpu >= 0) {		/* cpu info available - check CPU */
	Proc_history_t *ph = Store_process_history_info (PHS_GET, 
			(pr->node->index << 24) | pr->pid); 							/* use a unique index in the distri. env */
	if (ph != NULL) {

	    if (ph->llife == 0 || pr->life < ph->llife) {
		ph->lcpu = pr->cpu;
		ph->llife = pr->life;
	    }
	    else {
		double diff = pr->cpu - ph->lcpu;
		if (diff < -(double)(0x7fffffff >> 1))
		    diff += 0x7fffffff;

		if (diff < 0.)
		    LE_send_msg (GL_STATUS | GL_ERROR,
			"Process %s CPU Utilization decreasing (%d %d %d)", 
			    pr->name, pr->cpu, ph->lcpu, pr->life - ph->llife);
		else if (ph->llife + Cpu_window < pr->life) {
		    int util;
		    util = diff / 10 / (pr->life - ph->llife);
		    if ((pr->cpu_limit > 0 && util > pr->cpu_limit) || 
			(pr->cpu_limit == 0 && util > Cpu_limit)) {
			if (util > 150)
			    LE_send_msg (GL_STATUS | GL_ERROR,
			  "Process %s CPU Utilization %d Too High (%d %d %d)",
				    pr->name, util, pr->cpu, ph->lcpu, 
				    pr->life - ph->llife);
			else {
			    if (util > 100)
				util = 100;
			    sprintf (buf, 
		"Process %s CPU Utilization %d Too High - Process Killed",
							    pr->name, util);
			    bad = 1;
			}
		    }
		    ph->lcpu = pr->cpu;
		    ph->llife = pr->life;
		}
	    }
	}
    }

    if (bad) {			/* We don't manage valgrind */
	char tok[128];
	if (MISC_get_token (pr->acmd, "", 0, tok, 128) > 0 &&
	    strcmp (MISC_basename (tok), "valgrind") == 0)
	    bad = 0;
    }

    if (bad) {
	if (MHR_is_distributed ()) {
	    ORPGTASK_print_stack (pr->node->hname, pr->pid);
	    sprintf (buf + strlen (buf), " (on %s)\n", pr->node->node);
	    RSS_kill (pr->node->hname, pr->pid, SIGKILL);
	}
	else {
	    ORPGTASK_print_stack (NULL, pr->pid);
	    kill (pr->pid, SIGKILL);
	}
	LE_send_msg (GL_STATUS | GL_ERROR, buf);
   }

    return;
}

/******************************************************************

    Manages process history info storage.

    Input: func - functional switch: PHS_START - sets used to 0 for 
		all entries, PHS_GET - gets an entry for "pid" or 
		creates a new entry if not found or PHS_CLEANUP - 
		deletes all entries that are not accessed (used = 0).
	   pid - pid used for PHS_GET.

    Returns the pointer to the record of pid when func is PHS_GET 
    and the record is found or NULL otherwise.

******************************************************************/

static Proc_history_t *Store_process_history_info (int func, int pid) {
    static void *ph_tbl = NULL;		/* process history table id */
    static Proc_history_t *phs;		/* process history table. Sorted by 
					   pid. */
    static int n_phs = 0;		/* size of process history table */
    int i, ind;
    Proc_history_t ent;

    if (ph_tbl == NULL &&
	(ph_tbl = MISC_open_table (sizeof (Proc_history_t), 
			128, 1, &n_phs, (char **)&phs)) == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (NULL);
    }

    if (func == PHS_START) {
	for (i = 0; i < n_phs; i++)
	    phs[i].used = 0;
	return (NULL);
    }
 
    if (func == PHS_CLEANUP) {
	for (i = n_phs - 1; i >= 0; i--) {
	    if (phs[i].used == 0)
		MISC_table_free_entry (ph_tbl, i);
	}
	return (NULL);
    }
 
    if (func == PHS_RM_HISTORY) {
	for (i = n_phs - 1; i >= 0; i--) {
	    MISC_table_free_entry (ph_tbl, i);
	}
	return (NULL);
    }

    ent.pid = pid;
    if (!MISC_table_search (ph_tbl, &ent, Pid_cmp, &ind)) {
	if ((ind = MISC_table_insert (ph_tbl, (void *)&ent, Pid_cmp)) < 0) {
	    LE_send_msg (GL_ERROR, "Malloc failed\n");
	    return (NULL);
	}
	phs[ind].llife = 0;
    }
    phs[ind].used = 1;
    return (phs + ind);
}

/************************************************************************

    This is called when nds is started/restarted. The resource history is
    removed because it cannot be used after nds restarts.

************************************************************************/

void MMR_nds_started () {
    Store_process_history_info (PHS_RM_HISTORY, 0);
}

/************************************************************************

    Comparison function for table Prcs name search.

************************************************************************/

static int Pid_cmp (void *e1, void *e2) {
    Proc_history_t *ph1, *ph2;
    ph1 = (Proc_history_t *)e1;
    ph2 = (Proc_history_t *)e2;
    return (ph1->pid - ph2->pid);
}

/******************************************************************

    Sets the manage_off flag.
	
******************************************************************/

void MMR_set_manage_off_flag (int manage_off) {
    Manage_off = manage_off;
}

/******************************************************************

    Sets the state and cmd fields in RPG_state to "new_state" and 
    "new_cmd" if any of them changes. Negative value of an 
    argument indicates no change for that argument.

    Returns 0.
	
******************************************************************/

int MMR_set_rpg_state (int new_state, int new_cmd) {
    int state, cmd;

    state = RPG_state.state;
    cmd = RPG_state.cmd;
    if (new_state >= 0)
	state = new_state;
    if (new_cmd >= 0)
	cmd = new_cmd;
    if (RPG_state.state == state && RPG_state.cmd == cmd)
	return (0);
    RPG_state.state = state;
    RPG_state.cmd = cmd;
    RPG_state.st_time = time (NULL);
    Cr_state_time = MISC_systime (NULL);
    Write_rpg_state ();
    Publish_state ();
    return (0);
}

/******************************************************************

    Sets the RPG active mode to "active".
	
******************************************************************/

void MMR_set_active (int active) {
    if (RPG_state.active == active)
	return;
    RPG_state.active = active;
    Write_rpg_state ();
    Publish_state ();
}

/******************************************************************

    Returns the RPG state.
	
******************************************************************/

int MMR_get_rpg_state (Mrpg_state_t **Rpg_state) {
    if (Rpg_state != NULL)
	*Rpg_state = &RPG_state;
    return (RPG_state.state);
}

/******************************************************************

    Returns if the RPG state file is newly created.
	
******************************************************************/

int MMR_is_rpg_state_new () {
    if (New_rpg_state)
	return (1);
    return (0);
}

/******************************************************************

    Initializes this state file name and returns it.

    Returns the name on success or NULL on failure.
	
******************************************************************/

char *MMR_state_file_name () {

    if (strlen (Mrpg_state_name) == 0) {
	if (ORPGMGR_state_file_name (Mrpg_state_name, MRPG_NAME_SIZE) < 0) {
	    LE_send_msg (GL_ERROR, "ORPGMGR_state_file_name failed");
	    return (NULL);
	}
    }
    return (Mrpg_state_name);
}

/******************************************************************

    Creates and initalize the RPG state file.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MMR_create_state_file () {
    char *state_name;
    LB_attr attr;

    if (MAIN_command () == MRPG_RESUME) {
	if (State_fd < 0) {
	    LE_send_msg (GL_ERROR, "State file missing - cannot resume");
	    return (-1);
	}
	return (0);
    }

    if (State_fd >= 0) {
	Verify_redundant_type ();
	return (0);
    }
    if ((state_name = MMR_state_file_name ()) == NULL)
	return (-1);
    strcpy (attr.remark, "RPG state file");
    attr.mode = 0664;
    attr.msg_size = sizeof (Mrpg_state_t);
    attr.maxn_msgs = 3;
    attr.types = LB_DB;
    attr.tag_size = (32 << NRA_SIZE_SHIFT) | 32;
    State_fd = LB_open (state_name, LB_CREATE, &attr);
    if (State_fd < 0) {
	LE_send_msg (GL_ERROR, 
		"LB_open (create RPG state) failed (ret %d)", State_fd);
	return (-1);
    }

    RPG_state.state = MRPG_ST_SHUTDOWN;
    RPG_state.test_mode = MRPG_TM_NONE;	/* no longer used */
    RPG_state.st_time = time (NULL);
    Cr_state_time = MISC_systime (NULL);
    RPG_state.cmd = MRPG_SHUTDOWN;
    RPG_state.active = MRPG_ST_ACTIVE;
    if (MMR_is_faa_redundant ())
	RPG_state.active = MRPG_ST_INACTIVE;
    RPG_state.fail_count = 0;
    RPG_state.fail_time = 0;
    if (Write_rpg_state () != 0)
	return (-1);
    LB_lock (State_fd, LB_EXCLUSIVE_LOCK, LB_LB_LOCK);

    LE_send_msg (LE_VL1, "RPG state file %s created", state_name);
    New_rpg_state = 1;
    return (0);
}

/******************************************************************

    Writes the RPG state to the state file.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Write_rpg_state () {
    int ret;
    RPG_state.alive_time = time (NULL);
    RPG_state.check_sum = Calculate_check_sum ();
    if ((ret = LB_write (State_fd, (char *)&RPG_state, sizeof (Mrpg_state_t), 
		MRPG_STATE_MSGID)) != sizeof (Mrpg_state_t)) {
	LE_send_msg (GL_ERROR, 
			"LB_write (init RPG state) failed (ret %d)", ret);
	return (-1);
    }
    return (0);
}

/******************************************************************

    Returns the RPG state struct check sum value.
	
******************************************************************/

static unsigned int Calculate_check_sum () {
    unsigned int *ipt, cnt;
    int i;

    ipt = (unsigned int *)&RPG_state;
    cnt = 0;
    for (i = 0; i < (sizeof (Mrpg_state_t) / sizeof (unsigned int)) - 1; i++) {
	cnt += *ipt;
	ipt++;
    }
    return (cnt);
}

/******************************************************************

    Publishes the RPG state.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

#define RPG_STATUS_UNINITIALIZED 0xffffffff

static int Publish_state () {
    int ret, ret_val, verbose;

    verbose = 1;
    if (RPG_state.state == MRPG_ST_TRANSITION)
	verbose = 0;

    if (!verbose)
	MAIN_disable_report (1);
    ret_val = Publish_rpg_state ();

    if ((ret = ORPGDA_write (ORPGDAT_TASK_STATUS, (char *)&RPG_state, 
			sizeof (Mrpg_state_t), MRPG_RPG_STATE_MSGID)) < 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGDA_write MRPG_RPG_STATE_MSGID failed (ret %d)", ret);
	ret_val = -1;
    }
    if (!verbose)
        MAIN_disable_report (0);

    return (ret_val);
}

/******************************************************************

    Publishes the ICD RPG state.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Publish_rpg_state () {
    static Orpginfo_statefl_t prev_st = 	/* previous RPG status */
				{0, 0, 0, RPG_STATUS_UNINITIALIZED, 0};
    static int rpg_online = -1;
    unsigned int status, online;
    int ret, ret_val;

    ret_val = 0;
    if (prev_st.rpg_status == RPG_STATUS_UNINITIALIZED) {
					/* reads RPG status in initially */
	ORPGDA_open (ORPGDAT_RPG_INFO, LB_WRITE);
	ret = ORPGDA_read (ORPGDAT_RPG_INFO, (char *)&prev_st, 
			sizeof (Orpginfo_statefl_t), ORPGINFO_STATEFL_MSGID);
	if (ret == LB_NOT_FOUND) {	/* initialize the message */
	    int r;
	    prev_st.rpg_status = 0;
	    r = ORPGDA_write (ORPGDAT_RPG_INFO, (char *)&prev_st, 
			sizeof (Orpginfo_statefl_t), ORPGINFO_STATEFL_MSGID);
	    if (r < 0) {
		LE_send_msg (GL_ERROR, 
		    "ORPGDA_write ORPGINFO_STATEFL_MSGID failed (ret %d)", r);
		return (-1);
	    }
	}
	else if (ret < 0 ) {
	    LE_send_msg (GL_ERROR, 
		"ORPGDA_read ORPGINFO_STATEFL_MSGID failed (ret %d)", ret);
	    return (-1);
	}
    }
    status = 0;				/* current status */
    if (RPG_state.state == MRPG_ST_SHUTDOWN)
	status |= ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN;
    else if (RPG_state.state == MRPG_ST_STANDBY)
	status |= ORPGINFO_STATEFL_RPGSTAT_STANDBY;
    else if (RPG_state.state == MRPG_ST_OPERATING)
	status |= ORPGINFO_STATEFL_RPGSTAT_OPERATE;
    if (RPG_state.test_mode > 0)
	status |= ORPGINFO_STATEFL_RPGSTAT_TEST;

    if (Set_restart_bit)
	status |= ORPGINFO_STATEFL_RPGSTAT_RESTART;

    if (prev_st.rpg_status != status) {
        Orpginfo_rpg_status_change_evtmsg_t evtmsg;

        if( (status & ORPGINFO_STATEFL_RPGSTAT_STANDBY) &&
            !(prev_st.rpg_status & ORPGINFO_STATEFL_RPGSTAT_STANDBY) )
           LE_send_msg( GL_STATUS, "RPG State: STANDBY\n" );

        else if( (status & ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN) &&
            !(prev_st.rpg_status & ORPGINFO_STATEFL_RPGSTAT_SHUTDOWN) )
           LE_send_msg( GL_STATUS, "RPG State: SHUTDOWN\n" );

        else if( (status & ORPGINFO_STATEFL_RPGSTAT_OPERATE) ){

           if( !(status & ORPGINFO_STATEFL_RPGSTAT_TEST) &&
               ( !(prev_st.rpg_status & ORPGINFO_STATEFL_RPGSTAT_OPERATE) 
                                  ||
                  (prev_st.rpg_status & ORPGINFO_STATEFL_RPGSTAT_TEST) ) )
           LE_send_msg( GL_STATUS, "RPG State: OPERATE\n" );

        }

        if( (status & ORPGINFO_STATEFL_RPGSTAT_TEST) ){

           if( ( (prev_st.rpg_status & ORPGINFO_STATEFL_RPGSTAT_OPERATE) &&
                !(prev_st.rpg_status & ORPGINFO_STATEFL_RPGSTAT_TEST) )
                                  ||
               ( !(prev_st.rpg_status & ORPGINFO_STATEFL_RPGSTAT_OPERATE) &&
                  (prev_st.rpg_status & ORPGINFO_STATEFL_RPGSTAT_TEST) ) )
           LE_send_msg( GL_STATUS, "RPG Mode: TEST MODE\n" );

        }

	prev_st.rpg_status = status;
	ret = ORPGDA_write (ORPGDAT_RPG_INFO, (char *)&prev_st, 
			sizeof (Orpginfo_statefl_t), ORPGINFO_STATEFL_MSGID);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
		"ORPGDA_write ORPGINFO_STATEFL_MSGID failed (ret %d)", ret);
	    return (-1);
	}
        evtmsg.rpg_status = status;
        EN_post (ORPGEVT_RPG_STATUS_CHANGE, &evtmsg,
                         sizeof(evtmsg), EN_POST_FLAG_DONT_NTFY_SENDER);

    }

    if (RPG_state.state == MRPG_ST_OPERATING)
	online = 1;
    else
	online = 0;
    if (rpg_online < 0 || rpg_online != online) {
	unsigned char rpgopstat ;  /* RPG Operability Status */

	rpg_online = online;
	if (online){

            ret = ORPGINFO_statefl_rpg_operability_status (
				ORPGINFO_STATEFL_RPGOPST_ONLINE,
				ORPGINFO_STATEFL_SET, &rpgopstat) ;
            LE_send_msg( GL_STATUS, "RPG Operability Status: ONLINE\n" );

        }
	else{

            ret = ORPGINFO_statefl_rpg_operability_status (
				ORPGINFO_STATEFL_RPGOPST_CMDSHDN,
				ORPGINFO_STATEFL_SET, &rpgopstat) ;
            LE_send_msg( GL_STATUS, "RPG Operability Status: COMMANDED SHUTDOWN\n" );

        }
	if (ret < 0) {
            LE_send_msg(GL_ERROR,
                        "Unable to set RPG Operability Status") ;
	    ret_val = -1;
	}
    }
    return (ret_val);
}

/******************************************************************

    Sets/resets the "Restart" bit in RPG status word.
	
******************************************************************/

void MMR_set_restart_bit (int set) {
    Set_restart_bit = set;
    Publish_state ();
}

/******************************************************************

    Kills unexpected processes.
	
******************************************************************/

static void Remove_unexpected_processes (time_t t) {
    int n_ups, i, done;
    Unexp_process_t *ups;

    n_ups = MPI_get_unexpected_processes (&ups);
    done = 1;
    for (i = 0; i < n_ups; i++) {
	if (ups[i].t > 0 && t - ups[i].t > 10)
	    ups[i].pid = 0;
	if (ups[i].pid == 0 || ups[i].t > 0)
	    continue;
	if (!MHR_is_distributed ()) {	/* non-distributed */
	    LE_send_msg (LE_VL0, "kill unexpected process (pid %d) %s\n", 
						    ups[i].pid, ups[i].cmd);
	    kill (ups[i].pid, SIGKILL);
	}
	else {
	    LE_send_msg (LE_VL0, "kill unexpected process (pid %d on %s) %s\n",
				    ups[i].pid, ups[i].node->node, ups[i].cmd);
	    RSS_kill (ups[i].node->hname, ups[i].pid, SIGKILL);
	}
	ups[i].t = t;
	done = 0;
    }
    if (done)
	MPI_get_unexpected_processes (NULL);
}

/************************************************************************

    Returns 1 if the RPG is FAA redundant or 0 otherwise.

************************************************************************/

int MMR_is_faa_redundant () {
    char *p;

    if (DEAU_get_string_values ("Redundant_info.redundant_type", &p) == 1 &&
	strcmp (p, "FAA Redundant") == 0)
	return (1);
    return (0);
}





