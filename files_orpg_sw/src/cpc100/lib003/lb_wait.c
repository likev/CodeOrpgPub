/****************************************************************
		
    Module: lb_wait.c	
				
    Description: This module implements the LB_wait function.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2000/12/19 16:22:22 $
 * $Id: lb_wait.c,v 1.24 2000/12/19 16:22:22 jing Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */


/* System include files */ 

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

/* Local include files */

#include <lb.h>
#ifndef LBLIB
#include <net.h>
#else
#define INADDR_NONE 0Xffffffff
#endif
#include <misc.h>
#define LB_NTF_EXTERN_AS_STATIC
#include "lb_extern.h"
#include "lb_def.h"


static int (*LB_EXT_check_update) (int , unsigned int , int , int) = NULL;

static int Lb_signal_cnt = 0;

static void Callback (int sig);
static int Set_wakeup_request (LB_struct *lb, int host, 
						int pid, int action);
static int Lb_wait_internal (int list_size, LB_wait_list_t *list, int wait);
static int Check_update_all (int list_size,  
				LB_wait_list_t *list, int action);
static int Get_time_to_go (struct timeval *exp_time);

/********************************************************************
			
    Description: This function blocks the execution waiting for update
		of one of the listed open LBs. This function sets/resets
		the timer and calls Lb_wait_internal to do the main
		job.

    Input:	list_size - size of the LB list "list";
		list - list of the LBs to wait;
		wait - time-out value/action flag;

    Returns:	This function returns the number of updated LBs on 
		success or a negative LB error number. It returns 0
		if it is waked up by other signals.

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_wait (int list_size, LB_wait_list_t *list, int wait)
{
    int ret;

    if (list_size <= 0)
	return (0);

    if (LB_EXT_check_update == NULL) {
	if (LB_get_extern () != 0)
	    LB_EXT_check_update = LB_check_update;
    }

    LB_internal_block_NTF ();
    ret = Lb_wait_internal (list_size, list, wait);
    LB_internal_unblock_NTF ();

    return (ret);
}

/********************************************************************
			
    Description: This function sets external functions for this module.

    Input:	check_update - the new lb check update function.

********************************************************************/

void LB_wait_set_externs (int (*check_update)())
{

    LB_EXT_check_update = check_update;
    return;
}

/********************************************************************
			
    Description: This is the main implementation of LB_wait.

    Input:	list_size - size of the LB list "list";
		list - list of the LBs to wait;
		wait - time-out value/action flag;

    Returns:	This function returns the number of updated LBs on 
		success or a negative LB error number. It returns 0
		if it is waked up by other signals.

********************************************************************/

static int Lb_wait_internal (int list_size, LB_wait_list_t *list, int wait)
{
    static int sig_trapped = 0;
    struct timeval exp_time;

    if (!sig_trapped) {
	if (MISC_sig_sigset (LB_SIGNAL, Callback) == SIG_ERR)
	    return (LB_SIGSET_FAILED);
	sig_trapped = 1;
    }

    exp_time.tv_sec = 0;
    if (wait > 0 || wait == LB_WAIT_BLOCK) {
	int n_upd, ret;

	/* get expiration time */
	if (exp_time.tv_sec == 0) {
	    gettimeofday (&exp_time, NULL);
	    exp_time.tv_sec += (wait / 1000);
	    exp_time.tv_usec += (wait % 1000) * 1000;
	    if (exp_time.tv_usec > 1000000) {
		exp_time.tv_sec++;
		exp_time.tv_usec -= 1000000;
	    }
	}

	/* block the signal */
	if (MISC_sig_sighold (LB_SIGNAL) < 0)
	    return (LB_SIGHOLD_FAILED);

	while (1) {	/* repeat until an update of interest happens */

	    /* perform initial check and set wake-up requests */
	    if ((n_upd = Check_update_all (list_size,  
					list, LB_CHECK_AND_REQ)) < 0) {
		(void) MISC_sig_sigrelse (LB_SIGNAL);
		return (n_upd);
	    }

	    if (n_upd > 0) {	/* LB already updated */
		ret = Check_update_all (list_size, list, LB_FREE_REQ);
		(void) MISC_sig_sigrelse (LB_SIGNAL);
		if (ret < 0)
	            return (ret);
		else
		    return (n_upd);
	    }

	    Lb_signal_cnt = 0;
	    ret = LB_sig_wait (LB_SIGNAL, 
			Get_time_to_go (&exp_time)); /* wait for a signal */

	    if (errno != EINTR) {
		Check_update_all (list_size, list, LB_FREE_REQ);
		return (LB_SIGPAUSE_FAILED);
	    }

	    if (Lb_signal_cnt == 0) { 	/* interrupted by other signals */
		int i;

		for (i = 0; i < list_size; i++)
		    list[i].status = LB_NOT_UPDATED;

		ret = Check_update_all (list_size, list, LB_FREE_REQ);
		if (ret < 0)
	            return (ret);
		else
		    return (0);
	    }
	    else {				/* interrupted by LB_SIGNAL */

		/* we do two step check because the first will often succeed
		   without requiring a sighold/sigrelse cycle and the chance
		   that the second check is ever called is very small */
		Lb_signal_cnt = 0;		
		ret = Check_update_all (list_size, list, LB_CHECK_AND_FREE);
		if (ret > 0)
		    return (ret);

		(void) MISC_sig_sighold (LB_SIGNAL);
		if (Lb_signal_cnt > 0) {	/* received a signal before 
						   sighold */
		    ret = Check_update_all (list_size, list, LB_CHECK_ONLY);
		    if (ret > 0) {
			(void) MISC_sig_sigrelse (LB_SIGNAL);
			return (ret);
		    }
		}
	    }
	}
    }


    else if (wait == 0) {

	return (Check_update_all (list_size, list, LB_CHECK_ONLY));
    }

    else
	return (LB_BAD_ARGUMENT);
}

/********************************************************************
			
    Description: This returns time to go (in milli-seconds) to the 
		expiration time "exp_time".

    Input:	exp_time - the expiration time;

    Returns:	time to go in ms.

********************************************************************/

static int Get_time_to_go (struct timeval *exp_time)
{
    struct timeval cr_time;
    int togo;

    gettimeofday (&cr_time, NULL);
    if (exp_time->tv_usec < cr_time.tv_usec) {
	exp_time->tv_usec += 1000000;
	exp_time->tv_sec -= 1;
    }
    togo = (exp_time->tv_sec - cr_time.tv_sec) * 1000 + 
		(exp_time->tv_usec - cr_time.tv_usec) / 1000;
    if (togo < 0)
	togo = 0;
    return (togo);
}

/********************************************************************
			
    Description: This function checks LB update status and sets the 
		wake-up requests for a list of LBs.

    Input:	list_size - size of the LB list "list".
		list - list of the LBs.
		action - the action flag -
			LB_CHECK_AND_FREE: check LB update and 
				remove wake-up requests.
			LB_CHECK_AND_REQ: check LB update and set 
				one-time wake-up request; Exclusive
				LB lock is applied.
			LB_CHECK_ONLY: check LB update only.
			LB_FREE_REQ: remove wake-up requests.

    Output:	list - the LB update status.

    Returns:	This function returns the number of updated LBs (0
		if LB update check is not required) on success or a 
		negative LB error number.

********************************************************************/

static int Check_update_all (int list_size, 
				LB_wait_list_t *list, int action)
{
    static unsigned int lhost = INADDR_NONE;
					/* IP address of local machine */
    static int pid = -1;
    int i, ret, cnt;

    if (pid < 0)
	pid = getpid ();

    if (lhost == INADDR_NONE &&
	(lhost = LB_get_local_ip ()) == INADDR_NONE)
	    return (LB_LOCAL_IP_NOT_FOUND);

    cnt = 0;
    for (i = 0; i < list_size; i++) {

	ret = LB_EXT_check_update (list[i].fd, lhost, pid, action);
	if (ret < 0)
	    break;
	if (ret != LB_NOT_CHECKED)
	    list[i].status = ret;
	if (ret == LB_UPDATED)
	    cnt++;
    }

    if (ret < 0) {		/* remove all requests */
	int k;

	for (k = 0; k < i; k++)
	    LB_EXT_check_update (list[k].fd, lhost, pid, LB_FREE_REQ);
	return (ret);
    }

    return (cnt);
}

/********************************************************************
			
    Description: This function checks LB update status and sets the 
		wake-up request for LB "fd". This function needs to
		be implemented remotely.

    Input:	fd - the LB involved.
		host - the caller's host IP address.
		pid - the caller's pid.
		action - the action flag -
			LB_CHECK_AND_FREE: check LB update and 
				remove wake-up requests.
			LB_CHECK_AND_REQ: check LB update and set 
				one-time wake-up request; Exclusive
				LB lock is applied.
			LB_CHECK_ONLY: check LB update only.
			LB_FREE_REQ: remove wake-up requests.

    Returns:	This function returns the LB update status 
		(LB_NOT_UPDATED or LB_UPDATED) on success or a 
		negative LB error number.

********************************************************************/

int LB_check_update (int fd, unsigned int host, int pid, int action)
{
    LB_struct *lb;
    int ret, perm, lock, status;

    /* get the LB structure, lock and mmap the file */
    lb = LB_Get_lb_structure (fd, &ret);
    if (lb == NULL)
	return (ret);
    if (action != LB_CHECK_ONLY)
	perm = WRITE_PERM;
    else
	perm = READ_PERM;
    if (action == LB_CHECK_AND_REQ || action == LB_CHECK_AND_FREE)
	lock = EXC_LOCK;
    else
	lock = NO_LOCK;
    if ((ret = LB_lock_mmap (lb, perm, lock)) < 0)
	return (LB_Unlock_return (lb, ret));

    /* check update status */
    status = LB_NOT_CHECKED;
    if (action == LB_CHECK_AND_REQ || action == LB_CHECK_AND_FREE || 
						action == LB_CHECK_ONLY) {
	if (LB_stat_check (lb) == LB_UPDATED)
	    status = LB_UPDATED;
	else
	    status = LB_NOT_UPDATED;
    }

    /* set wake-up requests */
    if (action != LB_CHECK_ONLY) {
	ret = Set_wakeup_request (lb, host, pid, action);
	if (ret < 0)
	    return (LB_Unlock_return (lb, ret));
    }

    return (LB_Unlock_return (lb, status));
}

/********************************************************************
			
    Description: This function sets a wake-up request in "lb".

    Input:	lb - The LB involved.
		host - the caller's host IP address.
		pid - the caller's pid.
		action - the action (see LB_check_update).

    Return:	This function returns LB_SUCCESS on success or a 
		negative LB error number.

********************************************************************/

static int Set_wakeup_request (LB_struct *lb, int host, 
						int pid, int action)
{
    int n_rec, k, offset;
    LB_nr_t *rec, *r;
    int ret;

    if (host == 0)
	return (LB_ZERO_HOST_ADDRESS);

    n_rec = LB_get_nra_size (lb);
    offset = lb->off_nra;
    rec = (LB_nr_t *)(lb->pld->lb_pt + offset);

    if (action == LB_FREE_REQ || action == LB_CHECK_AND_FREE) {
	for (k = 0; k < n_rec; k++) {		/* rm matched records */

	    r = rec + k;
	    if (r->host == 0)
		break;
	    if (r->flag != LB_NR_WAIT)
		continue;
	    if (r->host == host && LB_SHORT_BSWAP (r->pid) == pid &&
				r->lock != 0) {
		LB_process_lock (UNSET_LOCK, lb, 
				SHARED_LOCK, LB_NR_LOCK_OFF + k);
		r->lock = 0;
		r->flag = LB_NR_NON;
	    }
	}
    }
    else if (action == LB_CHECK_AND_REQ) {

	for (k = 0; k < n_rec; k++) {	/* try any existing record */

	    r = rec + k;
	    if (r->host == 0)
		break;
	    if (r->flag == LB_NR_NOTIFY)
		continue;
	    if (r->host == host && 
				LB_SHORT_BSWAP (r->pid) == pid) {
		ret = LB_process_lock (SET_LOCK, lb, 
					SHARED_LOCK, LB_NR_LOCK_OFF + k);
	        if (ret < 0)
		    return (ret);
		rec[k].lock = 1;
		rec[k].flag = LB_NR_WAIT;
		return (LB_SUCCESS);
	    }
	}

	/* add a new entry */
	for (k = 0; k < n_rec; k++) {
	    short spid;

	    ret = LB_process_lock (TEST_LOCK, lb, EXC_LOCK, LB_NR_LOCK_OFF + k);
	    if (ret < 0)
		return (ret);
	    if (ret == LB_LOCKED)
		continue;

	    r = rec + k;
	    rec->lock = 1;
	    rec->host = host;
	    spid = pid;
	    rec->pid = LB_SHORT_BSWAP (spid);
	    rec->flag = LB_NR_WAIT;
	    LB_process_lock (SET_LOCK, lb, SHARED_LOCK, LB_NR_LOCK_OFF + k);
	    return (LB_SUCCESS);
	}
	if (k >= n_rec) 		/* all slots have been used */
	    return (LB_TOO_MANY_WAKEUP_REQ);
    }

    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: The LB_SIGNAL callback function.

    Input:	sig - the signal number;

********************************************************************/

static void Callback (int sig)
{

    if (sig == LB_SIGNAL)
	Lb_signal_cnt++;
    return;
}

