
/******************************************************************

	file: cmp_proc_mon.c

	This is the cm_ping module for process monitoring.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/03/16 14:36:46 $
 * $Id: cmp_proc_mon.c,v 1.16 2007/03/16 14:36:46 jing Exp $
 * $Revision: 1.16 $
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

typedef struct {		/* struct for registering processes
				   to be monitored */
    int pid;			/* process pid */
    int wp;			/* waiting period in seconds */
    char cmd[CMP_MAX_CMD_LEN];	/* process command line; ""
				   indicates monitor/restart cancelation */
    time_t t_recv;		/* time the process mon req received */ 
} Process_mon_t;

static Process_mon_t *Procs = NULL;
				/* process table */
static void *Tblid = NULL;	/* table id for Procs */
static int N_procs = 0;		/* # of entries in Procs array */



/**************************************************************************

    Description: This function processes the process monitoring request
		messages.

    Inputs:	buf - buffer holding the request message.
		len - size of the message.

**************************************************************************/

void CMPP_process_input (char *buf, int len)
{
    int i;
    Cmp_proc_mon_req_t *msg;
    time_t cr_tm;
    Process_mon_t *proc;

    if (len != sizeof (Cmp_proc_mon_req_t)) {
	LE_send_msg (GL_ERROR | 1024,  
			"bad process mon req msg lenghth (%d)", len);
	return;
    }
    msg = (Cmp_proc_mon_req_t *)buf;
    for (i = 0; i < N_procs; i++) 
	if (msg->pid == Procs[i].pid)
	    break;

    cr_tm = MISC_systime (NULL);
    if (i >= N_procs) {		/* a new process */

	if (Tblid == NULL)
	    Tblid = MISC_create_table (sizeof (Process_mon_t), 16);

	/* get a new entry */
	proc = (Process_mon_t *)MISC_table_new_entry (Tblid, NULL);
	if (proc == NULL) {
	    LE_send_msg (GL_ERROR | 1025,  "malloc failed");
	    return;
	}
	Procs = (Process_mon_t *)MISC_get_table (Tblid, &N_procs);
    }
    else			/* existing */
	proc = Procs + i;

    proc->pid = msg->pid;
    proc->wp = msg->wp;
    strncpy (proc->cmd, msg->cmd, CMP_MAX_CMD_LEN);
    proc->cmd[CMP_MAX_CMD_LEN - 1] = '\0';
    proc->t_recv = cr_tm;
    LE_send_msg (LE_VL3, "process mon req: pid %d, wait %d, cmd %s", 
				proc->pid, proc->wp, proc->cmd);

    return;
}

/**************************************************************************

    Description: This function conducts the process monitoring.

**************************************************************************/

void CMPP_check_processes ()
{
    time_t cr_tm;
    int i;

    cr_tm = MISC_systime (NULL);
    for (i = N_procs - 1; i >= 0; i--) {
	Process_mon_t *proc;
	char tmp[CMP_MAX_CMD_LEN + 20];

	proc = Procs + i;
	if (cr_tm >= proc->t_recv + proc->wp) {
	    if (proc->wp >= 0) {
		char *cpt;

		kill (proc->pid, SIGKILL);
		msleep (100);
		strcpy (tmp, "sh -c \"");
		strcat (tmp, proc->cmd);
		cpt = tmp + (strlen (tmp) - 1);
		while (cpt >= tmp && (*cpt == ' ' || *cpt == '&')) {
		    *cpt = '\0';
		    cpt--;
		}
		strcat (tmp, " &\"");
		system (tmp);
		LE_send_msg (GL_INFO | 1026,  "start process with cmd: %s", tmp);
	    }
	    MISC_table_free_entry (Tblid, i);
	    Procs = (Process_mon_t *)MISC_get_table (Tblid, &N_procs);
	}
    }

}

