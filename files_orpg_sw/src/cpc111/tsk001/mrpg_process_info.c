
/******************************************************************

	file: mrpg_process_info.c

	Retrieve RPG process infomation.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2015/01/05 20:35:18 $
 * $Id: mrpg_process_info.c,v 1.54 2015/01/05 20:35:18 steves Exp $
 * $Revision: 1.54 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 
#include <mrpg.h>
#include <nds.h>
#include "mrpg_def.h"

#define PROCESS_START_TIME 10		/* maximu seconds needed for start a 
					   process. */
#define NDS_CHECK_TIME 20		/* period (seconds) checking nds */

#define STATUS_WAIT_TIME 8

static char *Nds_lb_name = "infr/nds.lb";	/* nds LB name */
static int Prev_failed = -1, Prev_control_failed = -1;
					/* for setting task failure alarms */

static Unexp_process_t *Unexp_procs = NULL;
static int N_unexp_procs = 0;

typedef struct {
    Node_attribute_t *node;
    int pid;
    Mrpg_tat_entry_t *prc;
} Unassigned_proc_t;

static char *Unassigned_prcs = NULL;
static int N_unassigned_prcs = 0;
static time_t Unassigned_rm_time = 0;


static void Un_callback (int fd, LB_id_t msgid, int msg_info, void *arg);
static int Read_status_info (int msgid, Node_attribute_t *node);
static int Send_process_names_to_nds (Node_attribute_t *node);
static void Discard_dupps (Mrpg_tat_entry_t *prc);
static int Add_dupps (Mrpg_tat_entry_t *prc, 
		Nds_ps_struct *ps, char *cmd, int inst, time_t cr_t);
static Nds_ps_struct *Get_next_process (Mrpg_tat_entry_t *prc, 
				char *cpt, int cnt, int *inst_ret);
static void Set_task_fail_alarm ();
static int Start_nds (Node_attribute_t *node);
static int Start_all_nds ();
static int Is_term_char (char c);
static int Is_same_command (char *cmd1, char *cmd2);
static int Byteswap_and_verify_process_info (int len, char *buf);
static int Check_cmd_line (Mrpg_tat_entry_t *prc, char *cmd, int pid);
static int Search_cmd (char *ps_list, char *cmd, int cnt, char **ps);


/******************************************************************

    Initializes this module.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MPI_init () {
    LB_attr attr;
    int n_nodes, i;
    Node_attribute_t *nodes;

    n_nodes = MHR_all_nodes (&nodes);
    if (MAIN_command () == MRPG_RESUME) { /* we must remove nds without pid */
	char tmp[128];
	strcpy (tmp, "prm -9 -quiet nds");
	for (i = 0; i < n_nodes; i++) {
	    if (!nodes[i].is_local) {
		if (MGC_system (nodes[i].hname, tmp, NULL) < 0) {
		    LE_send_msg (GL_ERROR, 
			"Failed in removing nds on %s", nodes[i].node);
		    return (-1);
		}
	    }
	}
	if (MGC_system ("", tmp, NULL) < 0) {
	    LE_send_msg (GL_ERROR, "Failed in removing local nds");
	    return (-1);
	}
    }

    strcpy (attr.remark, "nds LB");
    attr.mode = 0664;
    attr.msg_size = 0;
    attr.maxn_msgs = 8;
    attr.types = LB_DB;
    attr.tag_size = 32 << NRA_SIZE_SHIFT;

    for (i = 0; i < n_nodes; i++) {
	char buf[MRPG_NAME_SIZE * 3 + 16];
	int fd;
	Node_attribute_t *n;

	n = nodes + i;
	if (n->not_used)
	    continue;
	n->nds_fd = -1;
	n->nds_pid = 0;
	n->nds_ok = 0;
	n->proc_table_updated = 0;
	n->proc_status_updated = 0;
	n->nds_msg = NULL;

	sprintf (buf, "%s:%s/%s", n->hname, n->orpgdir, Nds_lb_name);
	fd = LB_open (buf, LB_WRITE, &attr);	/* check LB */
	if (fd < 0) {	/* we create it */
	    if (MCD_create_dir (buf) < 0)
		return (-1);
	    fd = LB_open (buf, LB_CREATE, &attr);
	    if (fd < 0) {
		LE_send_msg (GL_ERROR, 
		    "LB_open (create) nds LB %s failed (ret %d)", buf, fd);
		return (-1);
	    }
	}
	LB_close (fd);

	n->proc_table_updated = n->proc_status_updated = 0;
	if (Start_nds (n) < 0)
	    return (-1);
    }

    return (0);
}

/******************************************************************

    Returns the list of the unexpected process list. If "up_p" is NULL,
    discards the list.
	
******************************************************************/

int MPI_get_unexpected_processes (Unexp_process_t **up_p) {
    if (up_p == NULL) {
	STR_free ((char *)Unexp_procs);
	Unexp_procs = NULL;
	N_unexp_procs = 0;
	return (0);
    }
    *up_p = Unexp_procs;
    return (N_unexp_procs);
}

/******************************************************************

    Clears task failure alarms.

******************************************************************/

void MPI_clear_task_failure_alarm () {
    unsigned char alarm_flag;

    ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_RPGTSKFL,
			ORPGINFO_STATEFL_CLR, &alarm_flag);
    ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_RPGCTLFL,
			ORPGINFO_STATEFL_CLR, &alarm_flag);
    Prev_failed = -1;
    Prev_control_failed = -1;
}

/******************************************************************

    Starts nds on node "node" and sends process list to be monitored.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

static int Start_nds (Node_attribute_t *node) {
    int cnt, fd, ret;
    char buf[MRPG_NAME_SIZE * 2 + 32], *out;
    time_t st_tm;

    if (node->nds_ok && node->nds_fd > 0 &&
	LB_lock (node->nds_fd, LB_TEST_EXCLUSIVE_LOCK, LB_LB_LOCK) == 
			LB_HAS_BEEN_LOCKED) 
	return (0);		/* we don't need to start nds */

    if (!node->is_connected)
	return (0);

    node->nds_ok = 0;
    if (node->nds_pid > 0) {		/* kill nds */
	if (node->is_local)
	    kill (node->nds_pid, SIGKILL);
	else {
	    ret = RSS_kill (node->hname, node->nds_pid, SIGKILL);
	    if (ret < -1) {
		LE_send_msg (GL_INFO, 
			"RSS_kill nds failed (%s, pid %d, ret %d)", 
			node->node, node->nds_pid, ret);
		return (-1);
	    }
	}
	node->nds_pid = 0;

	/* wait until termination completed */
	st_tm = MISC_systime (NULL);
	while (LB_lock (node->nds_fd, LB_TEST_EXCLUSIVE_LOCK, 
			LB_LB_LOCK) == LB_HAS_BEEN_LOCKED) {
	    if (MISC_systime (NULL) <= st_tm + STATUS_WAIT_TIME)
		msleep (200);
	    else {
		LE_send_msg (GL_ERROR, 
			    "nds does not terminate on %s", node->node);
		break;
	    }
	}
    }

    if (node->nds_fd >= 0) {		/* reopen nds LB */
	EN_control (EN_DEREGISTER);
	LB_UN_register (node->nds_fd, LB_ANY, Un_callback);
	LB_close (node->nds_fd);
	node->nds_fd = -1;
    }
    sprintf (buf, "%s:%s/%s", node->hname, node->orpgdir, Nds_lb_name);
    fd = LB_open (buf, LB_WRITE, NULL);
    if (fd < 0) {
	LE_send_msg (GL_ERROR, 
		"LB_open (reopen) nds LB %s failed (ret %d)", buf, fd);
	return (-1);
    }

    if ((ret = LB_UN_register (fd, LB_ANY, Un_callback)) < 0) {
	LB_close (fd);
	LE_send_msg (GL_ERROR, 
		"LB_UN_register nds LB failed (ret %d)", ret);
	return (-1);
    }
    node->nds_fd = fd;

    /* start nds */
    if (node->is_local)
	sprintf (buf, "nds -m -f %s/%s &", node->orpgdir, Nds_lb_name);
    else
	sprintf (buf, "nds -f %s/%s &", node->orpgdir, Nds_lb_name);
    if ((ret = MGC_system (node->hname, buf, &out)) < 0) {
	if (node->node[0] != '\0')
	    LE_send_msg (GL_ERROR, "MGC_system nds on %s failed (ret %d)", 
						node->node, ret);
	else
	    LE_send_msg (GL_ERROR, "MGC_system nds failed (ret %d)", ret);
	if (out != NULL && out[0] != '\0')
	    LE_send_msg (GL_ERROR, "  - %s", out);
	return (-1);
    }

    node->nds_pid = ret;
    cnt = 0;
    while ((ret = LB_lock (node->nds_fd, 
		LB_TEST_EXCLUSIVE_LOCK, LB_LB_LOCK)) != LB_HAS_BEEN_LOCKED) {
	msleep (500);
	cnt++;
	if (cnt > 20) {
	    LE_send_msg (GL_ERROR, "Starting nds failed");
	    return (-1);
	}
    }

    if (Send_process_names_to_nds (node) < 0)
	return (-1);
    node->nds_ok = 1;
    MMR_nds_started ();

    return (0);
}

/******************************************************************

    Check and restart nds on all nodes. Returns 0 on success or 
    -1 on failure.
	
******************************************************************/

static int Start_all_nds () {
    int n_nodes, i;
    Node_attribute_t *nodes;

    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < n_nodes; i++) {
	if (Start_nds (nodes + i) < 0)
	    return (-1);
    }
    return (0);
}

/******************************************************************

    Sends all RPG process names to nds.

    Returns 0 on success or -1 on failure.
	
******************************************************************/

#define BUF_SIZE 10000

static int Send_process_names_to_nds (Node_attribute_t *node) {
    char *buf, *cpt, *prev_name;
    Mrpg_tat_entry_t *prcs;
    int n_prcs, ret, i;

    if ((buf = malloc (BUF_SIZE)) == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed");
	return (-1);
    }
    n_prcs = MRT_get_ops (&prcs);

    prev_name = NULL;
    cpt = buf;
    for (i = 0; i < n_prcs; i++) {
	int len;
	Mrpg_tat_entry_t *pr;

	pr = prcs + prcs[i].cmd_index;
/*
	while (pr != NULL) {
	    if (pr->node == node)
		break;
	    pr = pr->next_node;
	}
	if (pr == NULL)
	    continue;
*/

	if (!(pr->type & (MRPG_TT_ENABLED | MRPG_TT_MONITOR_ONLY)))
	    continue;
	if (prev_name != NULL && strcmp (pr->cmd_name, prev_name) == 0)
	    continue;

	len = strlen (pr->cmd_name);
	if (BUF_SIZE - (cpt - buf) <= len) {
	    LE_send_msg (GL_ERROR, "Too many process names");
	    free (buf);
	    return (-1);
	}
	memcpy (cpt, pr->cmd_name, len);
	prev_name = pr->cmd_name;
	cpt += len;
	*cpt = ' ';
	cpt++;
    }
    if ((ret = LB_write (node->nds_fd, buf, cpt - buf, NDS_PROC_LIST)) != 
							cpt - buf) {
	LE_send_msg (GL_ERROR, 
			"LB_write nds process list failed (ret %d)", ret);
	free (buf);
	return (-1);
    }
    free (buf);
    return (0);
}

/******************************************************************

    Housekeeping function of this. It is called periodically or an
    event is received.
	
******************************************************************/

void MPI_housekeep () {
    static time_t last_tm = 0;
    time_t tm;
    int updated, n_nodes, i;
    Node_attribute_t *nodes;

    tm = MISC_systime (NULL);
    if (tm > last_tm + NDS_CHECK_TIME) {	/* check nds */
	Start_all_nds ();
	last_tm = tm;
    }

    /* kills processes on unassigned node  */
    if (Unassigned_rm_time == 0)
	Unassigned_rm_time = tm;
    if (tm >= Unassigned_rm_time + 10) {	/* remove unassigned prcs */
	Unassigned_proc_t *usp = (Unassigned_proc_t *)Unassigned_prcs;
	for (i = 0; i < N_unassigned_prcs; i++) {
	    LE_send_msg (GL_INFO, "Kill unassignedified task %s (pid %d) on %s",
			usp->prc->name, usp->pid, usp->node->node);
	    RSS_kill (usp->node->hname, usp->pid , SIGKILL);
	    usp++;
	}
	Unassigned_prcs = STR_reset (Unassigned_prcs, 128);
	N_unassigned_prcs = 0;
	Unassigned_rm_time = tm;
    }

    updated = 0;
    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < n_nodes; i++) {
	if (nodes[i].proc_table_updated) {
	    nodes[i].proc_table_updated = 0;
	    Read_status_info (NDS_PROC_TABLE, nodes + i);
	    updated = 1;
	}
    }
    if (updated)
	Set_task_fail_alarm ();

    if (updated) {		/* publish process table */
	Mrpg_tat_entry_t *prc;
	int first, cnt, ret, name_size, off;
	Mrpg_process_table_t *pt;
	char *buf;

	cnt = 0;
	name_size = 0;
	first = 1;
	while ((prc = MRT_get_next_task (first)) != NULL) {
	    int status;
	    first = 0;
	    status = prc->status;
	    if (status == MRPG_PS_ACTIVE || status == MRPG_PS_FAILED) {
		cnt++;
		name_size += strlen (prc->name) + 1 + 4;
	    }
	}
	buf = (char *)malloc (cnt * sizeof (Mrpg_process_table_t) + name_size);
	if (buf == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed");
	    return;
	}
	off = 0;
	first = 1;
	while ((prc = MRT_get_next_task (first)) != NULL) {
	    first = 0;
	    pt = (Mrpg_process_table_t *)(buf + off);
	    if (prc->status == MRPG_PS_ACTIVE || 
					prc->status == MRPG_PS_FAILED) {
		pt->instance = prc->instance;
		pt->pid = prc->pid;
		pt->status = prc->status;
		pt->name_off = sizeof (Mrpg_process_table_t);
		strcpy ((char *)pt + sizeof (Mrpg_process_table_t), prc->name);
		pt->size = ALIGNED_SIZE (sizeof (Mrpg_process_table_t) + 
						strlen (prc->name) + 1);
		off += pt->size;
	    }
	}
	ret = ORPGDA_write (ORPGDAT_TASK_STATUS, buf, off, MRPG_PT_MSGID);
	free (buf);
	if (ret < 0) {
	    LE_send_msg (GL_ERROR, 
			"ORPGDA_write MRPG_PT_MSGID failed (ret %d)", ret);
	    return;
	}
    }
}

/******************************************************************

    Retrieves RPG process info. The info is in Prds (get with 
    MRT_get_ops function).

    Returns 0 on success or -1 on failure.
	
******************************************************************/

int MPI_get_process_info () {
    time_t st_tm;
    int n_nodes, ret, i, cnt;
    Node_attribute_t *nodes;

    n_nodes = MHR_all_hosts (&nodes);
    for (cnt = 0; cnt < 2; cnt++) {
	if (Start_all_nds () < 0)
	    return (-1);
    
	/* send process status request */
	for (i = 0; i < n_nodes; i++) {
	    if (!nodes[i].is_connected) {
		nodes[i].proc_status_updated = -1;
		continue;
	    }
	    nodes[i].proc_status_updated = 0;
	    if ((ret = LB_write (nodes[i].nds_fd, "request", 
						6, NDS_PS_REQ)) != 6) {
		LE_send_msg (GL_ERROR, 
		    "LB_write nds status request on %s failed (ret %d)", 
						nodes[i].node, ret);
		nodes[i].nds_ok = 0;
		return (-1);
	    }
	}
    
	/* wait for specified time */
	st_tm = MISC_systime (NULL);
	while (1) {
	    int done;

	    LB_NTF_control (LB_NTF_WAIT,  500);
	    done = 1;
	    for (i = 0; i < n_nodes; i++) {
		if (nodes[i].proc_status_updated == 1) {
		    nodes[i].proc_status_updated = -1;
		    Read_status_info (NDS_PROC_STATUS, nodes + i);
		}
		if (nodes[i].proc_status_updated != -1)
		    done = 0;
	    }
	    if (done) {
		Set_task_fail_alarm ();
		return (0);
	    }
	    if (MISC_systime (NULL) > st_tm + STATUS_WAIT_TIME) {
		for (i = 0; i < n_nodes; i++) {
		    Node_attribute_t *n;

		    n = nodes + i;
		    if (n->proc_status_updated != 0)
			continue;
		    if (n->node[0] == '\0')
			LE_send_msg (GL_INFO, "nds not responding");
		    else
			LE_send_msg (GL_INFO, 
			    "nds on %s not responding", n->node);
		    n->nds_ok = 0;
		}
		break;
	    }
	}
    }
    return (-1);
}

/******************************************************************

    Reads process info and puts it in the process table. We keep the
    data returned from LB_read so we can use name and cmd info in it.

    Returns 0 on success or -1 on failure.

******************************************************************/

static int Read_status_info (int msgid, Node_attribute_t *node) {
    char *buf;
    char *cpt;
    Mrpg_tat_entry_t *prc, *prcs;
    int len, cnt, t_ps, ind, n_prcs;
    time_t cr_t;
    Nds_ps_struct *ps, *pn;

    if (node->nds_msg != NULL) {
	free (node->nds_msg);
	node->nds_msg = NULL;
    }
    len = LB_read (node->nds_fd, (char *)&buf, LB_ALLOC_BUF, msgid);
    if (len < 0) {
	LE_send_msg (GL_ERROR, "LB_read nds status failed (ret %d)", len);
	node->nds_ok = 0;
	return (-1);
    }
    if ((t_ps = Byteswap_and_verify_process_info (len, buf)) < 0)
	return (-1);
    if (t_ps == 0)
	return (0);
    Search_cmd (NULL, NULL, 0, NULL);

    node->nds_msg = buf;
    cr_t = MISC_systime (NULL);		/* current time */
    n_prcs = MRT_get_ops (&prcs);
    for (ind = 0; ind < n_prcs; ind++) {
	int inst, ret;

	prc = prcs + ind;
	while (prc->node != node && prc->next_node != NULL)
	    prc = prc->next_node;

	if (prc->node == node) {
	    prc->pid = -1;
	    if (prc->status == MRPG_PS_ACTIVE ||
		(prc->status == MRPG_PS_STARTED && 
			    cr_t >= prc->st_time + PROCESS_START_TIME)) {
		if (prc->status != MRPG_PS_FAILED)
		    prc->st_time = cr_t;
		prc->status = MRPG_PS_FAILED;
	    }
	    if (prc->n_dupps > 0)
		Discard_dupps (prc);
	}
	else if (prc->type & MRPG_TT_SUPPORT_TASK)
	    continue;

	ret = Search_cmd (buf, prc->cmd_name, t_ps, &cpt);
	if (ret < 0)
	    continue;
	ps = (Nds_ps_struct *)cpt;
	cnt = t_ps - ret;
	if (cnt <= 0)
	    continue;

	if (prc->node != node) {
	    while ((pn = Get_next_process (prc, cpt, cnt, &inst)) != NULL && 
						inst >= -1) {
		int i;
		Unassigned_proc_t sp, *tp;
		tp = (Unassigned_proc_t *)Unassigned_prcs;
		for (i = 0; i < N_unassigned_prcs; i++) {
		    if (node == tp->node && pn->pid == tp->pid)
			break;
		    tp++;
		}
		if (i >= N_unassigned_prcs) {
		    sp.node = node;
		    sp.pid = pn->pid;
		    sp.prc = prc;
		    Unassigned_prcs = STR_append (Unassigned_prcs, 
				(char *)&sp, sizeof (Unassigned_proc_t));
		    N_unassigned_prcs++;
		}
		pn->pid = -1;
	    }
	    continue;
	}

	if ((pn = Get_next_process (prc, cpt, cnt, &inst)) != NULL && 
						inst >= -1) {
	    prc->pid = pn->pid;
	    prc->cpu = pn->cpu;
	    prc->mem = pn->mem;
	    prc->swap = pn->swap;
	    prc->info_t = pn->info_t;
	    prc->life = pn->st_t;
	    prc->st_time = cr_t - pn->st_t;
	    prc->status = MRPG_PS_ACTIVE;
	    prc->acmd = (char *)pn + pn->cmd_off;
	    pn->pid = -1;
	}
	else
	    continue;

	if (ind >= n_prcs) {
	    LE_send_msg (GL_ERROR, "Matching task entry not found");
	    return (-1);
	}
	if (ind == n_prcs - 1 || strcmp (prc->name, prcs[ind + 1].name) != 0) {
	    while ((pn = Get_next_process (prc, cpt, cnt, &inst)) != NULL) {
		Add_dupps (prc, pn, (char *)pn + ps->cmd_off, inst, cr_t);
		pn->pid = -1;
	    }
	}
	else {
	    while ((pn = Get_next_process (prc, cpt, cnt, &inst)) != NULL && 
							inst >= -1) {
		Add_dupps (prc, pn, (char *)pn + ps->cmd_off, inst, cr_t);
		pn->pid = -1;
	    }
	}
    }

    return (0);
}

/******************************************************************

    Byte swaps the process status message from nds. We check the 
    size field to determine if the byte swap is needed. The message
    size is also verified. Returns the number of entries in the
    message on success or -1 on failure.
	
******************************************************************/

static int Byteswap_and_verify_process_info (int len, char *buf) {
    Nds_ps_struct *ps;
    char *cpt;
    int cnt, max_size;

    if (len == 0)
	return (0);

    max_size = 0x10000;
    cpt = buf;
    cnt = 0;		/* find how many processes and verify the data */
    while (cpt - buf + sizeof (Nds_ps_struct) <= len) {
	unsigned int s;
	ps = (Nds_ps_struct *)cpt;
	s = ps->size;
	if (s >= max_size) {		/* need byte swap */
	    s = INT_BSWAP (s);
	    ps->size = s;
	    s = ps->name_off;
	    s = INT_BSWAP (s);
	    ps->name_off = s;
	    s = ps->cmd_off;
	    s = INT_BSWAP (s);
	    ps->cmd_off = s;
	    s = ps->cpu;
	    s = INT_BSWAP (s);
	    ps->cpu = s;
	    s = ps->mem;
	    s = INT_BSWAP (s);
	    ps->mem = s;
	    s = ps->swap;
	    s = INT_BSWAP (s);
	    ps->swap = s;
	    s = ps->pid;
	    s = INT_BSWAP (s);
	    ps->pid = s;
	    s = ps->st_t;
	    s = INT_BSWAP (s);
	    ps->st_t = s;
	    s = ps->info_t;
	    s = INT_BSWAP (s);
	    ps->info_t = s;
	}
	cpt += ps->size;
	cnt++;
    }
    if (cpt - buf != len) {
	LE_send_msg (GL_INFO, "Incorrect NDS data");
	return (-1);
    }
    return (cnt);
}

/******************************************************************

    Searches for the next process status entry in "cpt" of size "cnt"
    to match name and instance in "prc". The first entry already
    matchs name. It searches for the oldest process of all matched 
    entries. Entries with pid < 0 are ignored (deleted). "inst_ret"
    returns the instance number of the found entry (-1 for single
    instance task). If no instance match is found, we set "inst_ret"
    to -2 and returns any of the entry that matches name. In case
    of no entry found, we return NULL.
	
******************************************************************/

static Nds_ps_struct *Get_next_process (Mrpg_tat_entry_t *prc, 
				char *cpt, int cnt, int *inst_ret) {
    Nds_ps_struct *ps, *found_ps, *min_ps;
    int inst, i, k;
    time_t min;

    min = 0;
    found_ps = min_ps = ps = NULL;
    for (i = 0; i < cnt; i++) {
	if (ps != NULL)
	    cpt += ps->size;
	ps = (Nds_ps_struct *)cpt;
	if (i > 0 && strcmp (prc->cmd_name, cpt + ps->name_off) != 0)
	    break;
	if (ps->pid >= 0) {
	    int ret = Check_cmd_line (prc, cpt + ps->cmd_off, ps->pid);
	    if (ret <= 0) {
		if (ret < 0)
		    ps->pid = -1;
		continue;
	    }
	    if ((prc->type & MRPG_TT_MULTIPLE_INVOKE) &&
		!Is_same_command (prc->cmd, cpt + ps->cmd_off))
		continue;

	    if (found_ps == NULL)
		found_ps = ps;
	    inst = -1;
	    if (prc->instance >= 0 &&
		sscanf (MRT_get_last_token (cpt + ps->cmd_off, &k), 
						"%d", &inst) != 1)
		    LE_send_msg (0, "instance missing on cmd line (%s)", 
							cpt + ps->cmd_off);
	    if (prc->instance == inst && (ps->st_t < min || min == 0)) {
		min = ps->st_t;
		min_ps = ps;
	    }
	}
    }
    if (min_ps != NULL) {
	*inst_ret = inst;
	return (min_ps);
    }
    else if (found_ps != NULL) {
	*inst_ret = -2;
	return (found_ps);
    }
    else
	return (NULL);
}

/******************************************************************

    Checks if command line "cmd" matches task entry "prc" in terms
    of the task name. Returns 1 if matches or 0 if not. "cmd" is
    assumed to be already equal to prc->cmd_name. If the cmd is an
    unexpected process, it is killed and -1 is returned.
	
******************************************************************/

static int Check_cmd_line (Mrpg_tat_entry_t *prc, char *cmd, int pid) {
    char topt[128];

    if (!(prc->misc & MRPG_MISC_SHARE_CMD))
	return (1);

    MRT_get_t_option (cmd, topt, 128);
    if (topt[0] != '\0') {
	Mrpg_tat_entry_t *prcs;
	int n_prcs, i;
	if (strcmp (topt, prc->name) == 0)	/* topt matches this */
	    return (1);
	n_prcs = MRT_get_ops (&prcs);
	i = prc->first_cmd_index;
	while (i < n_prcs) {
	    Mrpg_tat_entry_t *p;
	    p = prcs + prcs[i].cmd_index;
	    if (strcmp (prc->cmd_name, p->cmd_name) != 0)
		break;
	    if (strcmp (p->name, topt) == 0) /* topt matches another task */
		return (0);
	    i++;
	}
    }

    /* put this process in the unexpected list */
    if (!(prc->type & (MRPG_TT_MONITOR_ONLY | MRPG_TT_ALLOW_DUPLICATE))) {
	int i;
	for (i = 0; i < N_unexp_procs; i++) {
	    if (Unexp_procs[i].pid == pid)
		break;
	}
	if (i >= N_unexp_procs) {
	    Unexp_process_t up;
	    up.pid = pid;
	    strncpy (up.cmd, cmd, UNEXP_CMD_SIZE);
	    up.cmd[UNEXP_CMD_SIZE - 1] = '\0';
	    up.node = prc->node;
	    up.t = 0;
	    Unexp_procs = (Unexp_process_t *)STR_append ((char *)Unexp_procs, 
				    (char *)&up, sizeof (Unexp_process_t));
	    N_unexp_procs++;
	}
    }
    return (-1);
}

/******************************************************************

    Compares two commands. Returns non-zero if they are the same
    or 0 otherwise. cmd2 may be truncated after 72 chars.
	
******************************************************************/

static int Is_same_command (char *cmd1, char *cmd2) {
    char *p1, *p2;

    p1 = cmd1;
    p2 = cmd2;
    while (*p1 == ' ')
	p1++;
    while (*p2 == ' ')
	p2++;
    while (1) {
	while (*p1 == ' ' && p1[1] == ' ')
	    p1++;
	while (*p2 == ' ' && p2[1] == ' ')
	    p2++;
	if (*p1 == ' ' && Is_term_char (p1[1]))
	    p1++;
	if (*p2 == ' ' && Is_term_char (p2[1]))
	    p2++;
	if (Is_term_char (*p1)) {
	    if (Is_term_char (*p2))
		return (1);
	    return (0);
	}
	if (Is_term_char (*p2)) {
	    if (*p2 == '\0' && p2 - cmd2 > 72)
		return (1);
	    return (0);
	}
	if (*p1 != *p2)
	    return (0);
	p1++;
	p2++;
    }
    return (0);
}

/******************************************************************

    Returns non-zero if character "c" is one of the command 
    terminating characters or 0 otherwise.
	
******************************************************************/

static int Is_term_char (char c) {
    if (c == '|' || c == '>' || c == '&' || c == '\0')
	return (1);
    return (0);
}

/******************************************************************

    The nds UN callback function.

    Input:	See LB man page.
	
******************************************************************/

static void Un_callback (int fd, LB_id_t msgid, int msg_info, void *arg) {
    int n_nodes, i;
    Node_attribute_t *nodes, *n;

    n_nodes = MHR_all_hosts (&nodes);
    for (i = 0; i < n_nodes; i++) {
	n = nodes + i;
	if (n->nds_fd == fd) {
	    if (msgid == NDS_PROC_TABLE)	/* pid info */
		n->proc_table_updated = 1;
	    if (msgid == NDS_PROC_STATUS)	/* status info */
		n->proc_status_updated = 1;
	    break;
	}
    }
}

/******************************************************************

    Deletes all duplicated processes in process table entry "prc".

******************************************************************/

static void Discard_dupps (Mrpg_tat_entry_t *prc) {
    int i;
    Mrpg_tat_entry_t *dup;

    dup = prc->next;
    for (i = 0; i < prc->n_dupps; i++) {
	void *p;

	p = dup;
	dup = dup->next;
	free (p);
    }
    prc->n_dupps = 0;
}

/******************************************************************

    Adds a new duplicated OP process to table entry "prc".

******************************************************************/

static int Add_dupps (Mrpg_tat_entry_t *prc, 
		Nds_ps_struct *ps, char *cmd, int inst, time_t cr_t) {
    int i;
    Mrpg_tat_entry_t *dup, *p;

    p = (Mrpg_tat_entry_t *)malloc (sizeof (Mrpg_tat_entry_t));
    if (p == NULL) {
	LE_send_msg (GL_ERROR, "Malloc failed\n");
	return (-1);
    }
    memcpy (p, prc, sizeof (Mrpg_tat_entry_t));
    p->pid = ps->pid;
    p->cpu = ps->cpu;
    p->mem = ps->mem;
    p->swap = ps->swap;
    p->info_t = ps->info_t;
    p->life = ps->st_t;
    p->st_time = cr_t - ps->st_t;
    p->status = MRPG_PS_ACTIVE;
    p->acmd = cmd;

    /* we always put the oldest instance in the first element for operational 
       processes */
    if (!(prc->type & (MRPG_TT_MONITOR_ONLY | MRPG_TT_ALLOW_DUPLICATE)) &&
	p->life > prc->life) {
	Mrpg_tat_entry_t tmp;
	memcpy (&tmp, prc, sizeof (Mrpg_tat_entry_t));
	memcpy (prc, p, sizeof (Mrpg_tat_entry_t));
	prc->next = tmp.next;
	prc->n_dupps = tmp.n_dupps;
	prc->misc = tmp.misc;
	memcpy (p, &tmp, sizeof (Mrpg_tat_entry_t));
    }

    dup = prc;
    for (i = 0; i < prc->n_dupps; i++)
	dup = dup->next;
    dup->next = p;
    (prc->n_dupps)++;
    return (0);
}

/******************************************************************

    Sets/clears the task failure (RPGTSKFL) and control task failure
    (RPGCTLFL) RPG alarm bits by checking and failed RPG processes.
    It also sends RPG status messages when a process fails or is
    restored.

******************************************************************/
	   
static void Set_task_fail_alarm () {
    int failed, control_failed, first;
    unsigned char alarm_flag;
    int ret;
    Mrpg_state_t *rpg_state;
    Mrpg_tat_entry_t *prc;

    if (!MHR_id_node_connectivity_OK () ||
	MMR_get_rpg_state (&rpg_state) != MRPG_ST_OPERATING)
	return;

    /* finding the failed processes */
    failed = 0;
    control_failed = 0;
    first = 1;
    while ((prc = MRT_get_next_task (first)) != NULL) {

	first = 0;
	if (!(prc->type & MRPG_TT_ENABLED) ||
	    (prc->type & MRPG_TT_STOPPED))
	    continue;
	if ((prc->type & MRPG_TT_ACTIVE_CHANNEL_ONLY) && !rpg_state->active)
	    continue;
	if (prc->status == MRPG_PS_FAILED) {
	    if (prc->type & MRPG_TT_CONTROL_TASK)
		control_failed = 1;
            else
	        failed = 1;
	    if ((prc->misc & MRPG_MISC_FAIL_REPORTED) == 0) {

                if( prc->instance < 0 )
		    LE_send_msg (GL_STATUS | GL_ERROR, 
					"Task %s Has Failed", prc->name);
		else
		    LE_send_msg (GL_STATUS | GL_ERROR, 
			"Task %s.%d Has Failed", prc->name, prc->instance);
		prc->misc |= MRPG_MISC_FAIL_REPORTED;
	    }
	}
	else if (prc->status == MRPG_PS_ACTIVE) {
	    if (prc->misc & MRPG_MISC_FAIL_REPORTED) {

		if( prc->instance < 0 )
		    LE_send_msg (GL_STATUS, "Task %s Restored", prc->name);
		else
		    LE_send_msg (GL_STATUS, 
			"Task %s.%d Restored", prc->name, prc->instance);
		prc->misc &= ~MRPG_MISC_FAIL_REPORTED;
	    }
	}
    }

    if (Prev_failed < 0 || Prev_failed != failed) {
	Prev_failed = failed;
	if (failed > 0)
	    ret = ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_RPGTSKFL,
			ORPGINFO_STATEFL_SET, &alarm_flag) ;
	else
	    ret = ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_RPGTSKFL,
			ORPGINFO_STATEFL_CLR, &alarm_flag) ;
	if (ret < 0)
	    LE_send_msg (GL_ERROR, "Failed in updating RPGTSKFL alarm bit");
    }
    if (Prev_control_failed < 0 || 
			Prev_control_failed != control_failed) {
	Prev_control_failed = control_failed;
	if (control_failed > 0)
	    ret = ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_RPGCTLFL,
			ORPGINFO_STATEFL_SET, &alarm_flag) ;
	else
	    ret = ORPGINFO_statefl_rpg_alarm (
			ORPGINFO_STATEFL_RPGALRM_RPGCTLFL,
			ORPGINFO_STATEFL_CLR, &alarm_flag) ;
	if (ret < 0)
	    LE_send_msg (GL_ERROR, "Failed in updating RPGCTLFL alarm bit");
    }
}

/************************************************************************

    Searches for the first entry in "ps_list" that has name of "cmd".
    ps_list is a list of Nds_ps_struct of size "cnt" sorted on field
    "name". The pointer to the found entry is returned with "ret_p".
    Returns the index in the list on success or -1 on failure.

************************************************************************/

static int Search_cmd (char *ps_list, char *cmd, int cnt, char **ret_p) {
    static int buf_size = 0;
    static char *buf = NULL;
    static char *prev_list = NULL;
    static int prev_cnt = 0;
    Nds_ps_struct **ps;
    int st, end, found;

    if (ps_list == NULL) {
	prev_list = NULL;
	prev_cnt = 0;
	return (0);
    }

    if (cnt > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = malloc ((cnt + 50) * sizeof (Nds_ps_struct *));
	if (buf == NULL) {
	    LE_send_msg (GL_ERROR, "Malloc failed in Search_cmd");
	    buf_size = 0;
	    return (-1);
	}
	buf_size = cnt + 50;
	prev_list = NULL;
    }
    ps = (Nds_ps_struct **)buf;
    if (ps_list != prev_list || cnt != prev_cnt) {
	int i;
	char *p = ps_list;
	for (i = 0; i < cnt; i++) {
	    ps[i] = (Nds_ps_struct *)p;
	    p += ps[i]->size;
	}
	prev_list = ps_list;
	prev_cnt = cnt;
    }

    /* binary search */
    st = 0;
    end = cnt - 1;
    found = -1;
    while (1) {
	int ind = (st + end) >> 1;
	if (ind == st) {
	    if (strcmp ((char *)ps[st] + ps[st]->name_off, cmd) == 0)
		found = st;
	    else if (strcmp ((char *)ps[end] + ps[end]->name_off, cmd) == 0)
		found = end;
	    break;
	}
	if (strcmp (cmd, (char *)ps[ind] + ps[ind]->name_off) <= 0)
	    end = ind;
	else
	    st = ind;
    }
    if (found >= 0)
	*ret_p = (char *)ps[found];
    return (found);
}

