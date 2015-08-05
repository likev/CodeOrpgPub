
/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:54 $
 * $Id: lb_notify.c,v 1.74 2012/06/14 18:57:54 jing Exp $
 * $Revision: 1.74 $
 * $State: Exp $
 */

/* System include files */

#include <config.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* Local include files */

#include <misc.h>
#include <en.h>
#include <lb.h>

#include "lb_def.h"

#ifdef LB_NTF_SERVICE		/* This section is needed only if we need NTF */
/****************************************************************
		
    This module implements the client part of the LB notification
    functions.

*****************************************************************/

/****************************************************************
						
    Description: This registers a UN callback function.

    Input:	fd - LB file descriptor;
		msgid - message id;
		notify_func - the notification callback function;

    Returns:	This function returns LB_SUCCESS on success or a 
		negative LB error number.

****************************************************************/

int LB_UN_register (int fd, LB_id_t msgid, 
		void (*notify_func)(int, EN_id_t, int, void *)) {

    if (fd == EN_AN_CODE ||
	(msgid > EN_MAX_ID && msgid != EN_ANY && msgid != LB_MSG_EXPIRED &&
	(msgid & LB_UN_MSGID_TEST) != LB_UN_MSGID_TEST))
	return (LB_BAD_ARGUMENT);

    EN_set_lb_constants (LB_MSG_EXPIRED, RSS_LB_set_nr);

    return (EN_internal_register (fd, msgid, 
		(void (*)(EN_id_t, char *, int, void *))notify_func));
}

/********************************************************************
			
    Description: This function sets/resets a UN request.
		If pid < 0, it performs reset. This function
		needs to be implemented remotely. Note that this
		function guarantees that only one active (locked)
		entry existing for (host, msgid) and no (a_fd, a_pid)
		returned are from a trashed record.

    Input:	cfd - the LB fd (combined; See RSS - rss_def.h);
		host - callers host address;
		pid - callers pid; < 0 indicates resetting the record.
		msgid - the LB message id;

    Output:	a_pid - aliased pid;
		a_fd - aliased LB fd;

    Returns:	This function returns LB_SUCCESS on success or a 
		negative LB error number.

********************************************************************/

int LB_set_nr (int cfd, EN_id_t msgid, unsigned int host, 
				int pid, int *a_pid, int *a_fd) {
    LB_struct *lb;
    int fd, n_rec, offset, i, ret;
    LB_nr_t *rec, *r;

    fd = cfd & 0xffff;		/* see rss_def.h for extracting the LB fd */

    if (host == 0)
	return (LB_ZERO_HOST_ADDRESS);

    /* get the LB structure, lock and mmap the file */
    lb = LB_Get_lb_structure (fd, &ret);
    if (lb == NULL)
	return (ret);

    if ((ret = LB_lock_mmap (lb, WRITE_PERM, EXC_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));

    n_rec = LB_get_nra_size (lb);
    offset = lb->off_nra;
    rec = (LB_nr_t *)(lb->pld->lb_pt + offset);
    lb->upd_flag_check = 1;

    if (pid < 0) {		/* remove the record by unlock it */
	for (i = 0; i < n_rec; i++) {
	    r = rec + i;
	    if (r->host == 0)
		break;
	    if (Get_nr_flag (r->flag) == LB_NR_NOTIFY && 
		(unsigned int)r->host == host && 
		(LB_id_t)(LB_T_BSWAP (r->msgid)) == msgid)
		LB_process_lock (UNSET_LOCK, lb, 
				SHARED_LOCK, LB_NR_LOCK_OFF + i);
	}
	return (LB_Unlock_return (lb, LB_SUCCESS));
    }

    /* search for an alias */
    if (a_fd != NULL)
	*a_fd = -1;
#ifdef USE_MEMORY_CHECKER
    if (a_pid != NULL)	/* unnecessary - for eliminating an purify error */
	*a_pid = -1;
#endif
    for (i = 0; i < n_rec; i++) {
	r = rec + i;
	if (r->host == 0)
	    break;
	if (Get_nr_flag (r->flag) == LB_NR_NOTIFY && r->host == host && 
	    (LB_id_t)(LB_T_BSWAP (r->msgid)) == msgid) {
	    if ((ret = LB_process_lock (TEST_LOCK, lb, EXC_LOCK, 
				LB_NR_LOCK_OFF + i)) == LB_LOCKED) {
		LB_process_lock (SET_LOCK, lb, SHARED_LOCK, LB_NR_LOCK_OFF + i);
		r->lock |= 1;
		if (a_pid != NULL) {
		    *a_pid = LB_SHORT_BSWAP (r->pid);
		    *a_pid = Get_big_pid (*a_pid, r->lock, r->flag);
		}
		if (a_fd != NULL)
		    *a_fd = LB_T_BSWAP (r->fd);
		return (LB_Unlock_return (lb, LB_SUCCESS));
	    }
	    else if (ret < 0)
		return (LB_Unlock_return (lb, ret));
	}
    }

    /* add a new entry */
    for (i = 0; i < n_rec; i++) {
	int ret;
	short spid;

	ret = LB_process_lock (TEST_LOCK, lb, EXC_LOCK, LB_NR_LOCK_OFF + i);
	if (ret < 0)
	    return (LB_Unlock_return (lb, ret));
	if (ret == LB_LOCKED) {
	    if (rec[i].host == 0)
		return (LB_Unlock_return (lb, LB_LB_ERROR));
	    continue;
	}
	r = rec + i;
	r->lock = 1;
	r->host = host;
	spid = pid;
	r->pid = LB_SHORT_BSWAP (spid);
	r->msgid = LB_T_BSWAP (msgid);
	r->fd = LB_T_BSWAP (cfd);
	r->flag = LB_NR_NOTIFY;
	r->lock |= Pid_in_lock (pid);
	r->flag |= Pid_in_flag (pid);

	LB_process_lock (SET_LOCK, lb, SHARED_LOCK, LB_NR_LOCK_OFF + i);
	break;
    }
    if (i >= n_rec)
	return (LB_Unlock_return (lb, LB_TOO_MANY_WAKEUP_REQ));

    return (LB_Unlock_return (lb, LB_SUCCESS));
}

#else			/* #ifdef LB_NTF_SERVICE */

/********************************************************************

    Description: The following templates are used when EN is not
		supported.

********************************************************************/

void EN_internal_unblock_NTF () {
    return;
}

void EN_internal_block_NTF () {
    return;
}

int EN_send_to_server (unsigned int host, char *msg, int msg_len) {
    return (LB_NOT_SUPPORTED);
}

void EN_close_notify (int fd) {
    return;
}

void EN_print_unreached_host (unsigned int host) {
    return;
}

EN_per_thread_data_t *EN_get_per_thread_data (void) {
    static EN_per_thread_data_t ptd = {0, 0};
    return (&ptd);
}

void EN_parameters (int func, unsigned short *sender_id, int *notify_send) {
    return;
}

#endif			/* #ifdef LB_NTF_SERVICE */
