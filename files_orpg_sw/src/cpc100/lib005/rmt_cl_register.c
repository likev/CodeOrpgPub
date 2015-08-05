/****************************************************************
		
	File: cl_register.c	
				
	Purpose: This module contains client registration 
	functions for the RMT server. 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:57 $
 * $Id: rmt_cl_register.c,v 1.15 2012/06/14 18:57:57 jing Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */  

#include <config.h>
#ifdef __WIN32__
#define __INTERIX
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HPUX
#include <arpa/inet.h>
#endif

#include <net.h>
#include <misc.h>
#include <rmt.h>
#include <rmt_def.h>

#define CLIENT_TBL_MIN  16	/* increment num of entries in client table */


/* specified limits */
static int Max_N_Clients = 0;	/* maximum number of child servers */

/* client registration */

static void *ClList_tblId = NULL;
static Client_regist *Client_list;
static int N_Clients = 0;
static int Sigchild_received = 0;

/* local functions */
static void Process_child_die (int pid);

/*****************************************************************************

    Description: This function registers a new child server process serving 
		a new client. 

    Inputs:	sockfd - socket fd for the new client.
		addr - internet address of the new client's host.

    Return:	SUCCESS on success or FAILURE failure (malloc failed or
		too many clients are already connected).

*****************************************************************************/

int CLRG_register_client (Client_regist *newCl)
{
    int i;
    Client_regist *newEnt;

    CLRG_process_sigchild ();
    while (ClList_tblId == NULL) {
	ClList_tblId = MISC_open_table (sizeof (Client_regist), 
		    CLIENT_TBL_MIN, 0, &N_Clients, (char **)&Client_list);
	if (Client_list == NULL)
	    msleep(100);
    }

    newEnt = (Client_regist *)MISC_table_new_entry (ClList_tblId, &i);
    if (newEnt == NULL){
	MISC_log ("Error in MISC_table_new_entry\n");
	return (FAILURE);
    }

    if (i >= Max_N_Clients) {
	MISC_log ("Too many child server forked");
	MISC_table_free_entry (ClList_tblId, i);
	return (FAILURE);
    }
    memset(newEnt,0,sizeof(*newEnt));

    newEnt->childPid = newCl->childPid;
    newEnt->addr = newCl->addr;			/* LBO */
    newEnt->clientPid = newCl->clientPid;
    newEnt->pipeFd = newCl->pipeFd;

    return (SUCCESS);
}

/*****************************************************************************

    Description: This function sets up the maximum number of child servers
		can be forked and the time period (in seconds) a child will
		survive after a connection problem is detected. It also
		performs some initialization tasks.

    Inputs:	n_child - maximum number of child server processes.

    Return:	SUCCESS on success or FAILURE on failure.

*****************************************************************************/

int CLRG_initialize (int n_child)
{

    Max_N_Clients = n_child;
    return (SUCCESS);
}


/*****************************************************************************

    Description: This function deregisters a child process after it is 
		detected to be terminated.

    Inputs:	pid - pid of the dead child.

*****************************************************************************/

static void Process_child_die (int pid)
{
    int i, fd;

    for (i = 0; i < N_Clients; i++) {
	if (Client_list[i].childPid == pid){
	    fd = Client_list[i].pipeFd;
	    if (close (fd) < 0)
		MISC_log ("Process_child_die close %d, errno %d\n", fd, errno);
	    MISC_table_free_entry (ClList_tblId, i);
	    break;
	}
    } 

    return;
}

/*****************************************************************************

    Description: This function terminates services to host "hname".

*****************************************************************************/

void CLRG_terminate (char *hname)
{
    int addr;

    MISC_log ("service to (%s) is terminated", hname);

    if ((unsigned int)(addr = NET_get_ip_by_name (hname)) == INADDR_NONE)
	return;

    RMT_kill_clients (ntohl (addr));

    return;
}

/*****************************************************************************

    Description: This function terminates all child servers serving address
		"addr".

    Input:	addr - The IP address of the service to be terminated (LBO).

*****************************************************************************/

void RMT_kill_clients (int addr) {

    CLRG_term_client (0, addr);
    return;
}

/*****************************************************************************

    Description: This function checks the client table for a partcular client
		on a remote host.  If the client pid is there in the cient
		table, it returns the child process pipe fd for that client.
		addr is in LBO.

*****************************************************************************/

int CLRG_get_client_info (int client_id, int addr) {
    int i;

    CLRG_process_sigchild ();
    for (i = 0; i < N_Clients; i++) {
	if (Client_list[i].clientPid == client_id && 
					Client_list[i].addr == addr)
	    return (Client_list[i].pipeFd);
    }
    return (FAILURE);
}

/***********************************************************************

    Closes other RPC clients fds and fees the client table. Called by 
    child server.

    Input:	fd - This client's RPC fd.

***********************************************************************/

void CLRG_close_other_client_fd (int fd) {
    int i, f;

    for (i = 0; i < N_Clients; i++) {
	f = Client_list[i].pipeFd;
	if (f >= 0 && f != fd && close (f) < 0)
	    MISC_log ("close_other close %d, errno %d\n", f, errno);
    }
    if (ClList_tblId != NULL)
	MISC_free_table (ClList_tblId);
    ClList_tblId = NULL;
    N_Clients = 0;
}

/*****************************************************************************

    Removes all child processes that serve client from address "cl_addr" of 
    pid "cl_pid".

    Input:	cl_pid - pid of the client.
		cl_addr - The IP address of the client (LBO).

*****************************************************************************/

void CLRG_term_client (int cl_pid, int cl_addr) {
    int k;

    CLRG_process_sigchild ();
    for (k = N_Clients - 1; k >= 0; k--) {
	int ret, fd;

	if (Client_list[k].addr != cl_addr || 
			(cl_pid != 0 && Client_list[k].clientPid != cl_pid))
	    continue;

	ret = kill (Client_list[k].childPid, SIGTERM);
	if (ret < 0 && errno != ESRCH)
	    MISC_log ("kill failed (errno = %d)", errno);
	fd = Client_list[k].pipeFd;
	if (close (fd) < 0)
	    MISC_log ("CLRG_term_client close %d, errno %d\n", fd, errno);
	MISC_table_free_entry (ClList_tblId, k);
    }

    return;
}

/*****************************************************************************

    Retrieves terminated children's pids and remove them from registration.

*****************************************************************************/

void CLRG_process_sigchild () {
    int pid;

    if (!Sigchild_received)
	return;

    Sigchild_received = 0;
#if defined(SUNOS4)
    while ((pid = wait3 (NULL, WNOHANG, NULL)) > 0)
#else
    while ((pid = waitpid ((pid_t)-1, NULL, WNOHANG)) > 0)
#endif
	Process_child_die (pid);	/* deregistered the child */

    return;
}

/*****************************************************************************

    Receives sigchild signal notification.

*****************************************************************************/

void CLRG_sigchld () {
    Sigchild_received = 1;
}


