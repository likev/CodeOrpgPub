
/******************************************************************

    Description: This module contains the functions for monitoring 
		the connection status to the comms servers.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/03/16 14:36:44 $
 * $Id: cmc_monitor.c,v 1.20 2007/03/16 14:36:44 jing Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */  


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <infr.h>
#include <comm_manager.h>
#include <cmc_def.h>
#include <cm_ping.h>

#define MAX_N_RHOSTS  4		/* max number of remote comms servers */
#define UPDATE_PERIOD 3		/* status update period in seconds */
#define RE_REG_TIME 200		/* address re-registration period for 
				   getting cm_ping service. */
#define TIME_WINDOW  20		/* cm_ping status time window */

typedef struct {		/* the conection status record */
    int addr;			/* remote host address */
    int qtime;			/* quiet (disconnected) time */
    time_t update_time;		/* time when this record is updated */
} Rhost_record;

static Rhost_record Rhosts[MAX_N_RHOSTS];
				/* remote hosts table */
static int N_rhosts = 0;	/* number of registered remote hosts */

static char Cm_ping_lb_dir[NAME_LEN + 4] = "";
				/* cm_ping LB dir */

/* local functions */
static int Write_requests (int st_ind);
static void Read_status (time_t cr_time);
static int Get_cm_ping_input_fd ();
static void Set_default_dir ();

/**************************************************************************

    Description: This function receives the cm_ping dir path.

    Inputs:	dir - the cm_ping dir path;

**************************************************************************/

void CMMON_cm_ping_lb_dir (char *dir)
{

    strncpy (Cm_ping_lb_dir, dir, NAME_LEN);
    Cm_ping_lb_dir[NAME_LEN - 2] = 0;
    if (Cm_ping_lb_dir[strlen (Cm_ping_lb_dir) - 1] != '/')
	strcat (Cm_ping_lb_dir, "/");
    return;
}

/**************************************************************************

    Description: This function accepts a remote host address to which the
		connection will be monitored. If too many addresses are
		registered, the registration will fail and an error 
		message will be printed. This function also retrieve
		the project work directory as the default directory
		for cm_ping input/output LBs.

    Inputs:	addr - the internet address of the remote host (in local
		endian format);

    Return:	0 on success or -1 on failure.

**************************************************************************/

int CMMON_register_address (int addr)
{

    if (N_rhosts >= MAX_N_RHOSTS) {
	LE_send_msg (GL_ERROR | 21,  "too many registered addresses\n");
	return (-1);
    }

    Rhosts[N_rhosts].addr = addr;
    Rhosts[N_rhosts].qtime = -1;
    Rhosts[N_rhosts].update_time = 0;
    N_rhosts++;
    return (0);
}

/**************************************************************************

    Description: This function returns the disconnected time to host
		"addr". If the status is temporarily unavailable, this 
		will return -1.

    Inputs:	addr - the internet address of the remote host (in local
		endian format);

    Return:	the disconnected time on success or -1 on failure (The 
		status is temporarily unavailable or "addr" is not 
		registered).

**************************************************************************/

int CMMON_get_connect_status (int addr)
{
    int i;

    for (i = 0; i < N_rhosts; i++)
	if (addr == Rhosts[i].addr)
	    break;

    if (i >= N_rhosts) {
	LE_send_msg (GL_ERROR | 22,  "unregistered address (%x)\n", 
						(unsigned int)addr);
	return (-1);
    }

    if (MISC_systime (NULL) <= Rhosts[i].update_time + TIME_WINDOW)
	return (Rhosts[i].qtime);
    else
	return (-1);
}

/**************************************************************************

    Description: This function performs periodic updates of the 
		connection status table. This function must be called 
		frequently.

    Input:	cr_time - The current time.

**************************************************************************/

void CMMON_update (time_t cr_time)
{
    static time_t last_time = 0;
    static int last_n_rhosts = 0;
    static int last_request_time = 0;
    int min_upd_time, i;

    if (N_rhosts == 0)
	return;

    if (cr_time - last_time < UPDATE_PERIOD)	/* update frequency */
	return;
    last_time = cr_time;

    if (cr_time - last_request_time > RE_REG_TIME) {	
						/* re-register in cm_ping */
	last_n_rhosts = 0;
	last_request_time = cr_time;
    }

    /* check whether recently updated */
    min_upd_time = 0xffffff;
    for (i = 0; i < N_rhosts; i++) {
	int upd_time;

	upd_time = cr_time - Rhosts[i].update_time;
	if (upd_time < min_upd_time)
	    min_upd_time = upd_time;
    }
    if (min_upd_time > 15)
	last_n_rhosts = 0;

    if (N_rhosts > last_n_rhosts)		/* register in cm_ping */
	last_n_rhosts = Write_requests (last_n_rhosts);
    else
	last_n_rhosts = N_rhosts;

    Read_status (cr_time);			/* read cm_ping */
    
    return;
}

/**************************************************************************

    Description: This function sends a restart request.

    Input:	cmd - the command line for restart.
		wp - waiting period in seconds before starting a new
			instance. -1 cancels the previous restart request.

**************************************************************************/

void CMMON_restart_request (char *cmd, int wp)
{
    int fd, ret;
    Cmp_proc_mon_req_t msg;

    fd = Get_cm_ping_input_fd ();
    if (fd < 0)
	return;

    msg.type = CMP_IN_PROC_MON; 
    msg.pid = getpid ();
    msg.wp = wp;
    strncpy (msg.cmd, cmd, CMP_MAX_CMD_LEN);
    msg.cmd[CMP_MAX_CMD_LEN - 1] = '\0';

    ret = LB_write (fd, (char *)&msg, sizeof (Cmp_proc_mon_req_t), LB_ANY);
    if (ret != sizeof (Cmp_proc_mon_req_t))
	LE_send_msg (GL_ERROR | 23,  "LB_write cm_ping input failed (ret %d)", ret);

    return;
}

/**************************************************************************

    Description: This function writes request messages to the cm_ping
		input LB.

    Input:	st_ind - starting index in the remote host table.

    Return:	Index in the remote host table that need to be registered.

**************************************************************************/

static int Write_requests (int st_ind)
{
    int fd, i;

    fd = Get_cm_ping_input_fd ();
    if (fd < 0)
	return (st_ind);

    for (i = st_ind; i < N_rhosts; i++) {
	Cmp_tcp_mon_req_t msg;
	int ret;

	msg.type = CMP_IN_TCP_MON;
	msg.addr = Rhosts[i].addr;
	ret = LB_write (fd, (char *)&msg, sizeof (Cmp_tcp_mon_req_t), LB_ANY);
	if (ret != sizeof (Cmp_tcp_mon_req_t)) {
	    LE_send_msg (LE_VL0 | 24,  "LB_write cmp_in.lb failed (ret %d)", ret);
	    return (i);
	}
    }

    return (i);
}

/**************************************************************************

    Description: This function returns the fd for the cm_ping input LB.

    Return:	the fd on success or -1 on failure.

**************************************************************************/

static int Get_cm_ping_input_fd ()
{
    static int fd = -1;
    char name[NAME_LEN + 32];
    static int printed = 0;		/* open failed msg printed before */

    if (fd >= 0)
	return (fd);

    if (Cm_ping_lb_dir[0] == '\0')
	Set_default_dir ();
    strcpy (name, Cm_ping_lb_dir);
    strcat (name, "cmp_in.lb");
    fd = LB_open (name, LB_WRITE, NULL);
    if (fd < 0) {
	if (!printed) {
	    LE_send_msg (LE_VL0 | 25,  
		"LB_open %s failed (ret %d); cm_ping not available", name, fd);
	    printed = 1;
	}
	    return (-1);
    }
    if (printed)
	LE_send_msg (LE_VL1 | 26,  "LB_open %s succeeded", name);

    return (fd);
}

/**************************************************************************

    Description: This function reads cm_ping output LB to get the latest
		connection status.

    Input:	cr_time - the current time.

**************************************************************************/

#define BUF_SIZE 1024

static void Read_status (time_t cr_time)
{
    static int fd = -1;
    int len;
    char buf[BUF_SIZE];

    if (fd < 0) {			/* open the cm_ping output LB */
	char name[NAME_LEN + 32];
	static int printed = 0;		/* open failed msg printed before */

	if (Cm_ping_lb_dir[0] == '\0')
	    Set_default_dir ();
	strcpy (name, Cm_ping_lb_dir);
	strcat (name, "cmp_out.lb");
	fd = LB_open (name, LB_READ, NULL);
	if (fd < 0) {
	    if (!printed) {
		LE_send_msg (LE_VL0 | 27,  
			"LB_open %s failed (ret %d); cm_ping not available", 
								name, fd);
		printed = 1;
	    }
	    return;
	}
	if (printed)
	    LE_send_msg (LE_VL1 | 28,  "LB_open %s succeeded", name);
    }

    while ((len = LB_read (fd, buf, BUF_SIZE, LB_NEXT)) >= 0 || 
						len == LB_EXPIRED) {
	Cmp_out_msg_t *msg;
	int i;
	time_t cr_t;

	if (len == LB_EXPIRED)
	    continue;

	msg = (Cmp_out_msg_t *)buf;
	if (len < (int)sizeof (Cmp_out_msg_t) - (int)sizeof (Cmp_record_t) ||
	    len != (msg->n_records - 1) * sizeof (Cmp_record_t) + 
						sizeof (Cmp_out_msg_t)) {
	    LE_send_msg (GL_ERROR | 29,  "bad cm_ping message len (%d)", len);
	    continue;
	}

	cr_t = MISC_systime (NULL);
	for (i = 0; i < msg->n_records; i++) {
	    int addr, k;

	    addr = msg->rec[i].addr;
	    for (k = 0; k < N_rhosts; k++) {
		if (addr == Rhosts[k].addr) {
		    Rhosts[k].qtime = msg->rec[i].q_time;
		    Rhosts[k].update_time = cr_t;
		}
	    }
	}
    }

    if (len < 0 && len != LB_TO_COME) {
	LE_send_msg (LE_VL0 | 30,  "LB_read cmp_out.lb failed (ret %d)", len);
    }

    return;
}

/**************************************************************************

    Description: This function sets the default path for the cm_ping LBs.

**************************************************************************/

static void Set_default_dir ()
{

    if (MISC_get_work_dir (Cm_ping_lb_dir, NAME_LEN - 1) < 0) {
	LE_send_msg (GL_ERROR | 31,  
		"failed in getting default work directory path\n");
	CM_terminate ();
    }
    else
	strcat (Cm_ping_lb_dir, "/");
    return;
}





