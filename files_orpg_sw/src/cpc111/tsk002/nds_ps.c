
/******************************************************************

	file: nds_ps.c

	This is the nds module for updating process info.
	
******************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/06/24 18:16:02 $
 * $Id: nds_ps.c,v 1.32 2014/06/24 18:16:02 steves Exp $
 * $Revision: 1.32 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#ifdef SUNOS
#include <procfs.h>
#include <orpgerr.h>
#endif
#ifdef LINUX
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/param.h>
#endif

#include <orpgerr.h> 
#include <infr.h> 

#include "nds.h"
#include "nds_def.h"

#define PID_STR_SIZE 	32
#define ST_BUF_SIZE 800

typedef struct {			/* process status struct */
    char found;				/* this entry is found in new 
					   process list */
    char multitrd;			/* this is a multi-thread process */
    short n_len;			/* length of the first part of name */
    int ps_status_fd;			/* process status file fd. -1 not open.
					   -2 open failed (No status available)
					   */
    int ps_stat_fd;			/* the second file needed to get the
					   process info */
    int pid;				/* pid. < 0 for not runing. */
    long long cpu;			/* CPU non-idle time in ticks */
    double cpu_tm;			/* CPU read time in ms - multi-thread
					   process only. */
    long long rcpu;			/* CPU read from /proc, in ticks. 0 
					   for no value. */
    unsigned int mem;			/* memory in K (1024 bytes) */
    unsigned int swap;			/* swap (total) space used */
    unsigned int st_t;			/* proc start sys time */
    unsigned int info_t;		/* sys time getting the info */
    char *name;				/* process name */
    char *cmd;				/* command line */
    char pid_str[PID_STR_SIZE];		/* process id in ascii string */
} Ps_struct;

static void *Ps_tblid;			/* process table id */
static Ps_struct *Procs;		/* process list */
static int N_procs;			/* number of processes */

typedef struct {			/* All process registeration */
    short found;			/* this entry is found in /proc */
    short monitored;			/* this process is to be monitored */
    char pid_str[PID_STR_SIZE];		/* process id in ascii string */
    char *name;				/* process name */
    int pid;				/* process id */
    unsigned int st_t;			/* proc start sys time; 0: old proc */
} Apr_struct;

static void *Apr_tblid;			/* unused process reg table id */
static Apr_struct *Aprs;		/* unused process reg list */
static int N_aprs;			/* number of unused processes */

static int Lb_fd;

static int Process_list_updated = 0;	/* UN callback flag */
static int Process_util_requested = 0;	/* UN callback flag */
static int Status_changed = 0;		/* porcess status changed and output 
					   is needed. */
static double Mcpu_update_time = 0.;	/* last multi-thread cpu update time in
					   ms */
static double Ntrd_update_time = 0.;	/* last # threads update time for all
					   processes in ms */

enum {MRPG_STOPPED, MRPG_STARTED};
static int Mrpg_stat = MRPG_STOPPED;
static time_t Mrpg_tm = 0;		/* mrpg start/stop time */


static void Update_cb (int fd, LB_id_t msgid, int msg_len, void *arg);
static int Read_process_list ();
static void Process_unfound_entries ();
static void Read_process_info (Ps_struct *proc);
static int Copy_string (char **s1, char *s2);
static Ps_struct *Create_proc_entry (char *name);
static Apr_struct *Create_apr_entry (char *pid_str, 
				int pid, char *name, time_t st_t);
static void Delete_proc (Ps_struct *proc, int ind);
static void Delete_apr (Apr_struct *apr, int ind);
static void Publish_process_info ();
static void Output_status (LB_id_t msgid);
static int Get_process_info (char *pid_str, char **name, 
				char **cmd, time_t *st_t, time_t cr_t);
static int Cmp_pid (void *e1, void *e2);
static int Cmp_name (void *e1, void *e2);
static int Add_new_proc_reg (char *pid_str, 
			char *fname, int pid, char *cmd, time_t st_t);
static void Deregister_proc (int ind);
static void Update_cpu (Ps_struct *proc);
static void Update_mcpu (double dcr_t);
static void Init_proc_entry (Ps_struct *proc, char *name);
#ifdef LINUX
#define READ_BUF_SIZE 8292
static char Read_buf[READ_BUF_SIZE];
static time_t Get_btime ();
#endif

/**************************************************************************

    Description: Initializes this module.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

int NDSPS_init () {
    int ret;

    if ((Ps_tblid = MISC_open_table (sizeof (Ps_struct), 
			32, 1, &N_procs, (char **)&Procs)) == NULL ||
	(Apr_tblid = MISC_open_table (sizeof (Apr_struct), 
			64, 1, &N_aprs, (char **)&Aprs)) == NULL) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (-1);
    }

    Lb_fd = NDS_get_nds_fd ();
    if ((ret = LB_UN_register (Lb_fd, NDS_PROC_LIST, Update_cb)) < 0 ||
	(ret = LB_UN_register (Lb_fd, NDS_PS_REQ, Update_cb)) < 0) {
	LE_send_msg (GL_ERROR, "LB_UN_register failed, ret %d\n", ret);
	return (-1);
    }
    if (Read_process_list () != 0)
	return (-1);

    return (0);
}

/*************************************************************************

    UN callback function. We don't do processing here other than setting 
    up flags because the processing routines may need malloc.

*************************************************************************/

static void Update_cb (int fd, LB_id_t msgid, int msg_len, void *arg) {
    if (msgid == NDS_PROC_LIST)
	Process_list_updated = 1;
    if (msgid == NDS_PS_REQ)
	Process_util_requested = 1;
}

/**************************************************************************

    Description: Updates process information.

**************************************************************************/

void NDSPS_update () {
    static DIR *proc_dir = NULL;
    static int uid = -1;
    struct dirent *dp;
    int i, ms;
    time_t cr_t;
    double dcr_t;

    if (Process_list_updated) {
	Process_list_updated = 0;
	Read_process_list ();
    }

    if (uid < 0)
	uid = getuid ();
    if (proc_dir == NULL) {
	proc_dir = opendir ("/proc");
	if (proc_dir == NULL) {
	    LE_send_msg (GL_ERROR, "opendir /proc failed, errno %d\n", errno);
	    return;
	}
    }

    for (i = 0; i < N_aprs; i++)
	Aprs[i].found = 0;

    cr_t = MISC_systime (&ms);
    dcr_t = (double)cr_t + ms * .001;
    rewinddir (proc_dir);	/* rewind and refresh */
    while ((dp = readdir (proc_dir)) != NULL) {
	Apr_struct ent, *new;
	struct stat st;
	int ret, pid, apr_ind;
	char pname[NDS_NAME_SIZE], *pname_pt, *cmd;
	time_t st_t;

	pid = atoi (dp->d_name);
	if (pid == 0)		/* not a process dir */
	    continue;

	ent.pid = pid;
	if (MISC_table_search (Apr_tblid, &ent, Cmp_pid, &apr_ind)) {
	    if (Aprs[apr_ind].st_t > 0 && cr_t > Aprs[apr_ind].st_t + 20)
		Aprs[apr_ind].st_t = 0;
		/* process name is not assumed to change after 20 seconds */
	    Aprs[apr_ind].found = 1;
	    if (Aprs[apr_ind].st_t == 0)		/* old process */
		continue;
	}
	else
	    apr_ind = -1;

	pname[0] = '\0';
	if (apr_ind < 0) {		/* a new process */
	    char buf[NDS_NAME_SIZE];
	    sprintf (buf, "/proc/%s", dp->d_name);
	    ret = stat (buf, &st);
	    if (ret < 0) {
		if (errno != ENOENT)
		    LE_send_msg (GL_INFO, "stat %s, errno %d\n", buf, errno);
		continue;
	    }
	    if (st.st_uid != uid) {
		Create_apr_entry (dp->d_name, pid, pname, 0);
		continue;
	    }
	}

	if (Get_process_info (dp->d_name, &pname_pt, &cmd, &st_t, cr_t) < 0)
	    continue;
	strncpy (pname, pname_pt, NDS_NAME_SIZE);
	pname[NDS_NAME_SIZE - 1] = '\0';

	for (i = 0; i < N_procs; i++) { /* check script interpreter */
	    if (Procs[i].n_len <= 0)
		continue;
	    if (strncmp (Procs[i].name, pname, Procs[i].n_len) == 0 &&
		strlen (pname) == Procs[i].n_len) {
		char tk[NDS_NAME_SIZE];
		int c = 1;
		int done = 0;
		while (MISC_get_token (cmd, "", c, tk, NDS_NAME_SIZE) > 0) {
		    if (strcmp (MISC_basename (tk), 
				Procs[i].name + Procs[i].n_len + 1) == 0) {
			strncpy (pname, Procs[i].name, NDS_NAME_SIZE);
			pname[NDS_NAME_SIZE - 1] = '\0';
			done = 1;
			break;
		    }
		    c++;
		}
		if (done)
		    break;
	    }
	}

	if (apr_ind >= 0) {
	    for (i = 0; i < N_procs; i++) {
		if (Procs[i].pid == pid)
		    break;
	    }
	    if (i < N_procs) {
		if (strcmp (Procs[i].name, pname) == 0)
		    continue;		/* the name does not change */
		LE_send_msg (GL_INFO, "process %d rename: %s -> %s", 
					pid, Procs[i].name, pname);
	        Deregister_proc (i);
		Delete_apr (Aprs + apr_ind, apr_ind);
		apr_ind = -1;
	    }
	}

	ret = Add_new_proc_reg (dp->d_name, pname, pid, cmd, st_t);
	if (ret == -2)
	    continue;
	if (ret >= 0)
	    Status_changed = 1;

	/* add to all process table */
	if (apr_ind < 0)
	    new = Create_apr_entry (dp->d_name, pid, pname, st_t);
	else
	    new = Aprs + apr_ind;
	if (new != NULL) {		/* reset monitored */
	    new->monitored = 0;
	    if (ret >= 0)
		new->monitored = 1;
	}
    }

    Process_unfound_entries ();

    if (Mrpg_stat == MRPG_STOPPED && Mrpg_tm > 0 && cr_t >= Mrpg_tm + 5) {
	Mrpg_tm = cr_t;
	NDS_resume_mrpg ();
    }

    if (Process_util_requested) {
	Process_util_requested = 0;
	Publish_process_info ();
    }

    Update_mcpu (dcr_t);

    if (Status_changed)
	Output_status (NDS_PROC_TABLE);
    Status_changed = 0;

    return;
}

/******************************************************************

    Checks if process "pid" (name "fname", commnad line "cmd", 
    starting time "st_t" and ascii string pid "pid_str") is a 
    monitored process. If it is, this function registers the new 
    process and initializes the new process table entry. It returns 
    0 on success or -1 if the process is not monitored. It returns 
    -2 if an errro is found.

******************************************************************/

static int Add_new_proc_reg (char *pid_str, 
			char *fname, int pid, char *cmd, time_t st_t) {
    int i, found;
    Ps_struct *proc;
    char *cmd_p;

    /* go through the process table and find out if it is monitored */
    found = 0;
    for (i = 0; i < N_procs; i++) {
	if (strcmp (Procs[i].name, fname) == 0) {
	    found = 1;
	    if (Procs[i].pid < 0)
		break;
	}
    }
    if (!found)
	return (-1);		/* not to be monitored */

    cmd_p = NULL;
    Copy_string (&cmd_p, cmd);

    if (i < N_procs) {
	proc = Procs + i;
	Init_proc_entry (proc, fname);
    }
    else if ((proc = Create_proc_entry (fname)) == NULL) {
	free (cmd_p);
	return (-2);
    }

    /* initialize the entry */
    strncpy (proc->pid_str, pid_str, PID_STR_SIZE - 1);
    proc->pid_str[PID_STR_SIZE - 1] = '\0';
    if (proc->cmd != NULL)
	free (proc->cmd);
    proc->cmd = cmd_p;
    proc->pid = pid;
    proc->st_t = st_t;
    proc->info_t = MISC_systime (NULL);
    Read_process_info (proc);
    LE_send_msg (LE_VL3, "process %s (pid %d) added to monitor", fname, pid);

    return (0);
}

/**************************************************************************

    Description: Processes entries that is not found in the /proc dir in
		table Aprs. They are removed from Aprs and Procs except 
		the last one.

**************************************************************************/

static void Process_unfound_entries () {
    int i;

    for (i = N_aprs - 1; i >= 0; i--) {
	if (Aprs[i].found)
	    continue;

	if (Aprs[i].monitored) {
	    int pid, k;

	    pid = Aprs[i].pid;
	    for (k = 0; k < N_procs; k++) {
		if (Procs[k].pid == pid) {
		    if (Procs[k].name != NULL)
			LE_send_msg (LE_VL3, 
				"process to monitor %s (pid %d) dead", 
						Procs[k].name, Procs[k].pid);
		    else
			LE_send_msg (LE_VL3, 
				"process to monitor (pid %d) dead", 
						Procs[k].pid); 
	            Deregister_proc (k);
		    Status_changed = 1;
		    break;
		}
	    }
	}
	if (Aprs[i].name != NULL)
	    LE_send_msg (LE_VL3, "process %s (pid %d) dead", 
					Aprs[i].name, Aprs[i].pid);
	else
	    LE_send_msg (LE_VL3, "process (pid %d) dead", Aprs[i].pid);
	Delete_apr (Aprs + i, i);
    }
}

/********************************************************************

    Deregisters a process registration in table Procs.

    Input:	ind - table index of the entry.

********************************************************************/

static void Deregister_proc (int ind) {
    int last;
    Ps_struct *proc;

    proc = Procs + ind;
    last = 1;
    if (ind > 0 && 
	strcmp (proc->name, Procs[ind - 1].name) == 0)
	last = 0;
    else if (ind < N_procs - 1 &&
	strcmp (proc->name, Procs[ind + 1].name) == 0)
	last = 0;

    if (last) {		/* we can't remove the last entry */
	proc->pid_str[0] = '\0';
	proc->pid = -1;
	if (proc->ps_status_fd > 0)
	    MISC_close (proc->ps_status_fd);
	proc->ps_status_fd = -1;
	if (proc->ps_stat_fd > 0)
	    MISC_close (proc->ps_stat_fd);
	proc->ps_stat_fd = -1;
    }
    else
	Delete_proc (Procs + ind, ind);
}

/**************************************************************************

    Description: Publishes the current process info.

**************************************************************************/

static void Publish_process_info () {
    time_t t;
    double dcr_t;
    int ms, i;

    t = MISC_systime (&ms);
    dcr_t = (double)t + ms * .001;
    for (i = 0; i < N_procs; i++) {
	Procs[i].info_t = t;
	Read_process_info (Procs + i);
    }
    Ntrd_update_time = dcr_t;
    Mcpu_update_time = dcr_t;
    Output_status (NDS_PROC_STATUS);
}

/**************************************************************************

    Description: Reads process info for process "proc".

**************************************************************************/

#ifdef SUNOS
static void Read_process_info (Ps_struct *proc) {
    pstatus_t pst;
    int len;
    psinfo_t info;

    if (proc->pid < 0)
	return;
    if (proc->ps_status_fd == -1) {	/* open status file */
	char name[128];
	sprintf (name, "/proc/%s/status", proc->pid_str);
	proc->ps_status_fd = MISC_open (name, O_RDONLY, 0);
	if (proc->ps_status_fd < 0) {
	    if (errno == EINTR || errno == ENOENT)
		return;
	    LE_send_msg (GL_INFO, "open status %s failed, errno %d\n", 
						name, errno);
	    proc->ps_status_fd = -2;
	    proc->cpu = NDSPS_CPU_NOT_AVAILABLE;
	    return;
	}
    }
    if (proc->ps_status_fd < 0)
	return;

    if (proc->ps_stat_fd == -1) {	/* open psinfo for memory size */
	char buf[128];
	int fd;
	sprintf (buf, "/proc/%s/psinfo", proc->pid_str);
	fd = MISC_open (buf, O_RDONLY, 0);
	if (fd < 0) {
	    LE_send_msg (GL_INFO, "open psinfo %s failed, errno %d\n", 
						buf, errno);
	    return;
	}
	proc->ps_stat_fd = fd;
    }
    proc->mem = proc->swap = 0;
    if (proc->ps_stat_fd >= 0 &&
	lseek (proc->ps_stat_fd, 0, SEEK_SET) >= 0 &&
	(len = MISC_read (proc->ps_stat_fd, (char *)&info, 
				sizeof (psinfo_t))) == sizeof (psinfo_t)) {
	proc->mem = info.pr_rssize;
	proc->swap = info.pr_size;
    }

    lseek (proc->ps_status_fd, 0, SEEK_SET);
    len = MISC_read (proc->ps_status_fd, (char *)&pst, sizeof (pstatus_t));
    if (len != sizeof (pstatus_t)) {
	LE_send_msg (GL_INFO, 
	    "read status failed, fd %d, ret %d, errno %d, name %s, pid %d\n",
		proc->ps_status_fd, len, errno, proc->name, proc->pid);
	return;
    }
    proc->cpu = ((pst.pr_utime.tv_sec + pst.pr_stime.tv_sec) * 1000 +
		(pst.pr_utime.tv_nsec + pst.pr_stime.tv_nsec) / 1000000) %
		0x7fffffff;
/*    proc->mem += (pst.pr_brksize + pst.pr_stksize) / 1024; */
}
#endif

#ifdef LINUX

static void Print_status_text (char *st) {
    char *p;

    p = st;
    while (1) {
	int len;
	char c;

	len = strlen (p);
	if (len <= 0)
	    return;
	if (len > 60) {
	    len = 60;
	    c = p[len];
	    p[len] = '\0';
	}
	else
	    c = '\0';
	LE_send_msg (GL_INFO, "%s", p);
	if (c != '\0')
	    p[len] = c;
	p += len;
    }
}

static void Read_process_info (Ps_struct *proc) {
    char pst[ST_BUF_SIZE];
    int len;
    int ressize, tsize, sharedsize;

    if (proc->pid < 0)
	return;
    proc->mem = proc->swap = 0;

    if (proc->ps_status_fd == -1) {	/* open status file */
	char name[128];
	sprintf (name, "/proc/%s/statm", proc->pid_str);
	proc->ps_status_fd = MISC_open (name, O_RDONLY, 0);
	if (proc->ps_status_fd < 0) {
	    if (errno != ENOENT)
		LE_send_msg (GL_INFO, "open status %s failed, errno %d\n", 
						name, errno);
	    proc->ps_status_fd = -2;
	    return;
	}
    }
    if (proc->ps_stat_fd == -1) {	/* open stat file */
	char name[128];
	sprintf (name, "/proc/%s/stat", proc->pid_str);
	proc->ps_stat_fd = MISC_open (name, O_RDONLY, 0);
	if (proc->ps_stat_fd < 0) {
	    if (errno != ENOENT)
		LE_send_msg (GL_INFO, "open stat %s failed, errno %d\n", 
						name, errno);
	    proc->ps_stat_fd = -1;
	    return;
	}
    }

    if (proc->ps_status_fd < 0 || proc->ps_stat_fd < 0)
	return;

    lseek (proc->ps_status_fd, 0, SEEK_SET);
    len = MISC_read (proc->ps_status_fd , pst, ST_BUF_SIZE);
		/* if this file is incomplete, we will report less mem */
    if (len < 0) {
	LE_send_msg (GL_INFO, "read status failed, pid %s, errno %d\n",
					proc->pid_str, errno);
	return;
    }
    if (len >= ST_BUF_SIZE)
	len = ST_BUF_SIZE - 1;
    pst[len] = '\0';
    if (sscanf (pst, "%d %d %d", &tsize, &ressize, &sharedsize) != 3)
	LE_send_msg (GL_INFO, "incomplete mem, pid %s\n", proc->pid_str);
    else {
	static int page_size = 0;
	if (page_size == 0)
	    page_size = getpagesize ();
	proc->mem = (double)(ressize - sharedsize) * page_size / 1024;
	proc->swap = proc->mem;
    }

    Update_cpu (proc);
}

#endif

/**************************************************************************

    Description: Reads the process list message and updates the table of
		monitored processes. Unmonitored processes are removed 
		from the table. Already existing processes reuse their
		entries. We must use static buffer since this can be 
		called from UN callback. The open file resources are 
		removed before an entry is deleted. New entries are 
		initialized.

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_process_list () {
    static char buf[2048], *cpt;
    int len, i;

    for (i = 0; i < N_procs; i++)
	Procs[i].found = 0;

    len = LB_read (Lb_fd, buf, 2048, NDS_PROC_LIST);
    if (len == LB_NOT_FOUND)
	buf[0] = '\0';
    else if (len < 0) {
	LE_send_msg (GL_ERROR, "LB_read process list failed, ret %d\n", len);
	return (-1);
    }
    cpt = strtok (buf, "\n \t");
    while (cpt != NULL) {
	int found;

	found = 0;
	for (i = 0; i < N_procs; i++) {
	    if (strcmp (Procs[i].name, cpt) == 0) {	/* already in table */
		Procs[i].found = 1;	/* all entries need to be set */
		found = 1;
	    }
	}
	if (!found) {			/* add a new entry */
	    Ps_struct *ps;
	    if ((ps = Create_proc_entry (cpt)) == NULL)
		return (-1);

	    /* we must remove the processes from the apr table */
	    for (i = N_aprs - 1; i >= 0; i--) {
		if (strcmp (Aprs[i].name, ps->name) == 0)
		    Delete_apr (Aprs + i, i);
	    }
	}
	cpt = strtok (NULL, "\n \t");
    }

    /* remove unused entries */
    for (i = N_procs - 1; i >= 0; i--) {
	if (!Procs[i].found)
	    Delete_proc (Procs + i, i);
    }
    Status_changed = 1;
/*
{
    for (i = 0; i < N_procs; i++)
	printf ("process regs: %s\n", Procs[i].name);
}
*/
    LE_send_msg (0, "%d processes to monitor", N_procs);
    return (0);
}

/********************************************************************

     Malloc space and copy string.

********************************************************************/

static int Copy_string (char **s1, char *s2) {
    int len;

    if (*s1 != NULL)
	free (*s1);
    len = strlen (s2) + 1;
    *s1 = MISC_malloc (len);
    strcpy (*s1, s2);
    return (0);
}

/********************************************************************

    Creates and initializes a Ps_struct table entry.

    Input:	name - process name.

    Return: Pointer to the new entry on success or NULL on failure.

********************************************************************/

static Ps_struct *Create_proc_entry (char *name) {
    Ps_struct proc;
    int ind;

    proc.name = proc.cmd = NULL;
    Copy_string (&(proc.name), name);
    Init_proc_entry (&proc, name);

    ind = MISC_table_insert (Ps_tblid, (void *)&proc, Cmp_name);
    if (ind < 0) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (NULL);
    }
    return (Procs + ind);
}

/********************************************************************

    Initializes process structure "proc".

********************************************************************/

static void Init_proc_entry (Ps_struct *proc, char *name) {
    char *p;

    proc->ps_status_fd = -1;
    proc->ps_stat_fd = -1;
    proc->pid_str[0] = '\0';
    proc->cpu = proc->rcpu = 0;
    proc->cpu_tm = 0.;
    proc->multitrd = 0;
    proc->mem = 0;
    proc->swap = 0;
    proc->pid = -1;
    proc->found = 1;
    proc->n_len = 0;
    p = name;
    while (*p != '\0') {
	if (*p == '@') {
	    if (p > name && p[1] != '\0')
		proc->n_len = p - name;
	    break;
	}
	p++;
    }
}

/********************************************************************

    Creates and initializes a Apr_struct table entry.

    Input:	pid_str - pid in ascii form.
		pid - process id.
		name - process name.
		st_t - process start time.

    Return: Pointer to the new entry on success or NULL on failure.

********************************************************************/

static Apr_struct *Create_apr_entry (char *pid_str, 
				int pid, char *name, time_t st_t) {
    Apr_struct apr;
    int ind;

    apr.pid = pid;
    strncpy (apr.pid_str, pid_str, PID_STR_SIZE - 1);
    apr.pid_str[PID_STR_SIZE - 1] = '\0';
    apr.name = NULL;
    Copy_string (&(apr.name), name);
    apr.found = 1;
    apr.monitored = 0;
    apr.st_t = st_t;

    ind = MISC_table_insert (Apr_tblid, &apr, Cmp_pid);
    if (ind < 0) {
	LE_send_msg (GL_ERROR, "malloc failed\n");
	return (NULL);
    }
    LE_send_msg (LE_VL3, "process %s (pid %d) added to ap table", name, pid);
    return (Aprs + ind);
}

/********************************************************************

    Deletes a Ps_struct table entry. If ind < 0, we search for the
    index by proc.

    Input:	proc - the entry to remove.
		ind - table index of the entry.

********************************************************************/

static void Delete_proc (Ps_struct *proc, int ind) {

    if (ind < 0) {		/* find the table index */
	for (ind = 0; ind < N_procs; ind++) {
	    if (Procs + ind == proc)
		break;
	}
	if (ind >= N_procs)	/* not found */
	    return;
    }

    if (proc->name != NULL)
	LE_send_msg (LE_VL3, 
	     "process to monitor %s (pid %d) removed", proc->name, proc->pid);
    else
	LE_send_msg (LE_VL3, 
	     "process to monitor (pid %d) removed", proc->pid);
	
    if (proc->name != NULL)
	free (proc->name);
    if (proc->cmd != NULL)
	free (proc->cmd);
    if (proc->ps_status_fd >= 0)
	MISC_close (proc->ps_status_fd);
    if (proc->ps_stat_fd >= 0)
	MISC_close (proc->ps_stat_fd);

    MISC_table_free_entry (Ps_tblid, ind);
}

/********************************************************************

    Deletes a Apr_struct table entry.

    Input:	apr - the entry to remove.
		ind - table index of the entry.

********************************************************************/

static void Delete_apr (Apr_struct *apr, int ind) {

/* printf ("remove apr: pid %s\n", apr->pid_str); */
    LE_send_msg (LE_VL3, "process (pid %s) removed from table", 
					apr->pid_str); 
    if (apr->name != NULL)
	free (apr->name);
    MISC_table_free_entry (Apr_tblid, ind);
}

/**************************************************************************

    Description: Outputs the current process status.

**************************************************************************/

static void Output_status (LB_id_t msgid) {
    static char *buf = NULL;
    static int buf_size = 0;
    int cnt, i, ret, mrpg_found;
    time_t cr_t;

    cnt = 0;
    cr_t = MISC_systime (NULL);
    mrpg_found = 0;
    for (i = 0; i < N_procs; i++) {
	Ps_struct *proc;
	Nds_ps_struct *ps;
	int s, name_len;

	proc = Procs + i;
	if (proc->pid < 0)
	    continue;
	if (strcmp (proc->name, "mrpg") == 0)
	    mrpg_found = 1;
	name_len = strlen (proc->name);
	s = sizeof (Nds_ps_struct) + 
			name_len + strlen (proc->cmd) + 6;
	s -= (s % sizeof (int));

	if (cnt + s >= buf_size) {	/* increase buf size */
	    char *pt;

	    pt = malloc (buf_size + 32 * s);
	    if (pt == NULL) {
		LE_send_msg (GL_ERROR, "malloc failed");
		return;
	    }
	    if (buf != NULL) {
		memcpy (pt, buf, buf_size);
		free (buf);
	    }
	    buf = pt;
	    buf_size += 32 * s;
	}
/*
printf ("output: pid %d name %s cpu %d mem %d cmd %s\n", 
			proc->pid, proc->name, proc->cpu, proc->mem, proc->cmd);*/

	ps = (Nds_ps_struct *)(buf + cnt);
	ps->cpu = (proc->cpu * 1000 / HZ) % 0x7ffffff;
	ps->mem = proc->mem;
	ps->swap = proc->swap;
	ps->pid = proc->pid;
	ps->st_t = cr_t - proc->st_t;
	ps->info_t = proc->info_t;
	ps->size = s;
	ps->name_off = sizeof (Nds_ps_struct);
	ps->cmd_off = ps->name_off + name_len + 1;
	strcpy (buf + cnt + ps->name_off , proc->name);
	strcpy (buf + cnt + ps->cmd_off , proc->cmd);
	cnt += s;
    }
    if (mrpg_found) {
	if (Mrpg_stat != MRPG_STARTED) {
	    Mrpg_tm = cr_t;
	    Mrpg_stat = MRPG_STARTED;
	}
    }
    else {
	if (Mrpg_stat != MRPG_STOPPED) {
	    Mrpg_tm = cr_t;
	    Mrpg_stat = MRPG_STOPPED;
	}
    }

    if ((ret = LB_write (Lb_fd, buf, cnt, msgid)) != cnt)
	LE_send_msg (GL_ERROR, "LB_write %d failed (ret %d)", msgid, ret);
/* printf ("write status size %d\n", cnt); */
}

/**********************************************************************

    Retrieves process name and command line given pid "pid_str". Returns
    them in "name" and "cmd". Returns 0 on success or -1 on failure.

**********************************************************************/

#ifdef SUNOS
static int Get_process_info (char *pid_str, char **name, 
				char **cmd, time_t *st_t, time_t cr_t) {
    static psinfo_t info;
    char buf[NDS_NAME_SIZE];
    int fd, len;

    /* open the process file and find the process name */
    sprintf (buf, "/proc/%s/psinfo", pid_str);
    fd = MISC_open (buf, O_RDONLY, 0);
    if (fd < 0) {
	LE_send_msg (GL_INFO, "open psinfo %s failed, errno %d\n", buf, errno);
	return (-1);
    }
    len = MISC_read (fd, (char *)&info, sizeof (psinfo_t));
    MISC_close (fd);
    if (len != sizeof (psinfo_t)) {
	LE_send_msg (GL_INFO, "read psinfo %s failed, ret %d, errno %d\n",
					buf, len, errno);
	return (-1);
    }
    *name = info.pr_fname;
    *cmd = info.pr_psargs;
    *st_t = info.pr_start.tv_sec;
    return (0);
}
#endif 

#ifdef LINUX
static int Get_process_info (char *pid_str, char **name, 
				char **cmd, time_t *st_t, time_t cr_t) {
    static char buf[200], name_buf[32];
    static time_t btime = 0, last_btime_upd_time = 0;
    char tmp[NDS_NAME_SIZE], *cpt, tok[256];
    int fd, len, s, f, ret1, ret2;

    if (cr_t > last_btime_upd_time + 600) {	/* update btime 10 minutes */
	btime = 0;
	last_btime_upd_time = cr_t;
    }

    while (btime == 0) {		/* get system boot time in systiem */
	time_t stime, tm, bt1, bt2;

	if ((bt1 = Get_btime ()) == 0)
	    return (-1);
	stime = MISC_systime (NULL);
	tm = time (NULL);
	if ((bt2 = Get_btime ()) == 0)
	    return (-1);
	if (bt1 != bt2)			/* machine clock reset */
	    continue;
	if (stime >= tm)
	    btime = bt1 + (stime - tm);
	else
	    btime = bt1 - (tm - stime);
/*	if (btime <= 0) {
	    LE_send_msg (GL_ERROR, "Unexpected btime (%d)\n", btime);
	    btime = 0;
	    return (-1);
	}
*/
    }

    /* open the process stat file and find the process start time */
    sprintf (tmp, "/proc/%s/stat", pid_str);
    fd = MISC_open (tmp, O_RDONLY, 0);
    if (fd < 0) {
	if (errno != ESRCH)
	    LE_send_msg (GL_INFO, "open stat failed, pid %s, errno %d\n", 
						pid_str, errno);
	return (-1);
    }
    len = MISC_read (fd, Read_buf, READ_BUF_SIZE);
    MISC_close (fd);
    if (len < 0) {
	if (errno != ESRCH)
	    LE_send_msg (GL_INFO, "read stat failed, pid %s, errno %d\n",
						pid_str, errno);
	return (-1);
    }
    if (len >= READ_BUF_SIZE)
	len = READ_BUF_SIZE - 1;
    Read_buf[len] = '\0';
    if (MISC_get_token (Read_buf, "", 21, tok, 256) <= 0) {
	LE_send_msg (GL_INFO, "incomplete stat (start time), pid %s\n",
						pid_str);
	return (-1);
    }
    len = strlen (tok);
    s = 0;
    f = 0;
    if (len > 2)
	ret1 = sscanf (tok + (len - 3), "%d", &f);
    else
	ret1 = sscanf (tok, "%d", &f);
    if (len > 3) {
	*(tok + (len - 3)) = '\0';
	ret2 = sscanf (tok, "%d", &s);
    }
    else
	ret2 = 1;
    if (ret1 != 1 || ret2 != 1) {
	LE_send_msg (GL_INFO, "Unexpected token 21 (%s), pid %s\n",
						tok, pid_str);
	return (-1);
    }
    *st_t = btime + (int)((((double)s * 1000. + f) / HZ) + .5);

    /* open the process cmdline file and find the process name */
    sprintf (tmp, "/proc/%s/cmdline", pid_str);
    fd = MISC_open (tmp, O_RDONLY, 0);
    if (fd < 0) {
	LE_send_msg (GL_INFO, "open cmdline failed, pid %s, errno %d\n", 
							pid_str, errno);
	return (-1);
    }
    len = MISC_read (fd, buf, 200);
    MISC_close (fd);
    if (len < 0) {
	LE_send_msg (GL_INFO, "read cmdline failed, pid %s, errno %d\n",
							pid_str, errno);
	return (-1);
    }
    if (len == 0)	/* cmdline not be available for <defunc> process */
	strcpy (buf, " ");
    if (len >= 200)
	len = 199;
    buf[len] = '\0';

    strncpy (name_buf, MISC_basename (buf), 31);
    name_buf[31] = '\0';
    cpt = buf;
    while (cpt - buf < len - 1) {
	if (*cpt == '\0')
	    *cpt = ' ';
	cpt++;
    }

    if (strcmp (name_buf, "valgrind") == 0) {	/* get cmd run by valgrind */
	int off;
	cpt = buf + MISC_get_token (buf, "", 0, tok, 256);
	while ((off = MISC_get_token (cpt, "", 0, tok, 256)) > 0) {
	    if (tok[0] != '-') {
		strncpy (name_buf, MISC_basename (tok), 31);
		break;
	    }
	    cpt += off;
	}
    }
    *name = name_buf;
    *cmd = buf;
    return (0);
}

/************************************************************************

    Gets and returns btime from /proc. Returns 0 on failure.

************************************************************************/

static time_t Get_btime () {
    int fd, len;
    char *cpt;
    unsigned int btime;

    fd = MISC_open ("/proc/stat", O_RDONLY, 0);
    if (fd < 0) {
	LE_send_msg (GL_ERROR, "open /proc/stat failed, errno %d\n", errno);
	return (0);
    }
    len = MISC_read (fd, Read_buf, READ_BUF_SIZE);
    MISC_close (fd);
    if (len < 0) {
	LE_send_msg (GL_ERROR, "read /proc/stat failed, errno %d\n", errno);
	return (0);
    }
    if (len >= READ_BUF_SIZE)
	len = READ_BUF_SIZE - 1;
    Read_buf[len] = '\0';
    if ((cpt = strstr (Read_buf, "btime")) == NULL ||
	sscanf (cpt, "%*s %u\n", &btime) != 1) {
	LE_send_msg (GL_ERROR, "failed in getting btime\n");
	return (0);
    }
    return ((time_t)btime);
}
#endif 

/**********************************************************************

    Updates cpu for all multiple threaded processes. Updates number of
    threads for all processes at least every 3 seconds. This function 
    must be called frequently (once each second).

**********************************************************************/

static void Update_mcpu (double dcr_t) {
    int trd_upd, i;

    if (Mcpu_update_time > 0. && dcr_t < Mcpu_update_time + .8)
	return;
    Mcpu_update_time = dcr_t;

    trd_upd = 0;
    if (Ntrd_update_time <= 0. || dcr_t >= Ntrd_update_time + 3.) {
	trd_upd = 1;		/* need to update # thread of all processes */
	Ntrd_update_time = dcr_t;
    }

    for (i = 0; i < N_procs; i++) {
	if (trd_upd || Procs[i].multitrd)
	    Update_cpu (Procs + i);
    }
}

/**********************************************************************

    Updates cpu and number of threads for process "proc". For threaded
    process we are interested in its idle time (all threads are almost
    idle - total cpu for all threads are less than the time interval).
    Thus we define its cpu as interval_time - idle_time. We assume
    that a process is threaded if it is detected to have more than one
    thread at any time (once detected, it is assumed to stay in
    threaded). To estimate cpu of such a process, we must frequently
    check its cpu to find out the idle time. We assume that, a normal
    process must in idle state in some time intervals of more than 1
    second. Thus 1 second sampling rate is fine. We only do this high
    frequency sampling for threaded processes. We must also check if
    any processes turns into threaded frequently. We do this every 3
    seconds. In the transitional interval to threaded, we assume all
    threads are used to get a idle time estimate of near its lower
    limit.

**********************************************************************/

static void Update_cpu (Ps_struct *proc) {
    char pst[ST_BUF_SIZE], buf[16];
    int len, i, n_thrs;
    long long utime, stime, rcpu;

    if (proc->pid < 0)
	return;
    lseek (proc->ps_stat_fd, 0, SEEK_SET);
    len = MISC_read (proc->ps_stat_fd, pst, ST_BUF_SIZE);
    if (len < 0) {
	LE_send_msg (GL_INFO, 
	    "CPU reading failed, pid %s, errno %d\n", proc->pid_str, errno);
	return;
    }
    if (len >= ST_BUF_SIZE)
	len = ST_BUF_SIZE - 1;
    pst[len] = '\0';

    if (MISC_get_token (pst, "", 2, buf, 16) > 0 &&
	(buf[0] == 'X' || buf[0] == 'Z'))  /* process dead - No CPU reading */
	return;

    if ((i = MISC_get_token (pst, "", 12, buf, 16)) <= 0 ||
	sscanf (pst + i, "%Ld %Ld %*d %*d %*d %*d %d", &utime, &stime, &n_thrs) != 3) {
	LE_send_msg (GL_INFO, 
			"Incomplete CPU, pid %s (%d)\n", proc->pid_str, len);
	Print_status_text (pst);
	return;
    }

    if (n_thrs > 1)
	proc->multitrd = 1;
    rcpu = utime + stime;
    if (proc->rcpu > 0) {
	int diff;
	if (rcpu < proc->rcpu) {
	    LE_send_msg (GL_INFO, 
	    "CPU util decreasing (new %ld old %ld)\n", rcpu, proc->rcpu);
	    Print_status_text (pst);
	    return;
	}
	diff = rcpu - proc->rcpu;
	if (proc->multitrd) {
	    int ms;
	    time_t cr_t = MISC_systime (&ms);
	    double dcr_t = (double)cr_t + ms * .001;
	    if (proc->cpu_tm > 0) {
		int tdf = (dcr_t - proc->cpu_tm) * HZ + .5;
		if (diff > tdf)
		    diff = tdf;
	    }
	    else	/* just turned to multi-thread */
		diff = diff / n_thrs;
	    proc->cpu_tm = dcr_t;
	}
	proc->cpu += diff;
    }
    proc->rcpu = rcpu;
}

/************************************************************************

    Pid comparison function for table Aprs insertion.

************************************************************************/

static int Cmp_pid (void *e1, void *e2) {
    Apr_struct *apr1, *apr2;
    apr1 = (Apr_struct *)e1;
    apr2 = (Apr_struct *)e2;
    return (apr1->pid - apr2->pid);
}

/************************************************************************

    process name comparison function for table Procs insertion.

************************************************************************/

static int Cmp_name (void *e1, void *e2) {
    Ps_struct *proc1, *proc2;
    proc1 = (Ps_struct *)e1;
    proc2 = (Ps_struct *)e2;
    return (strcmp (proc1->name, proc2->name));
}





