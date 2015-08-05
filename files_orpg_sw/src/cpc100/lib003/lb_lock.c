/****************************************************************
		
    Module: lb_lock.c	
				
    Description: This module contains the lock functions needed
		by internal LB functions and the public LB_lock 
		function.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/02/21 22:48:06 $
 * $Id: lb_lock.c,v 1.57 2007/02/21 22:48:06 jing Exp $
 * $Revision: 1.57 $
 * $State: Exp $
 * $Log: lb_lock.c,v $
 * Revision 1.57  2007/02/21 22:48:06  jing
 * Update
 *
 * Revision 1.53  2002/03/26 21:25:12  jing
 * Update
 *
 * Revision 1.52  2002/03/12 16:51:27  jing
 * Update
 *
 * Revision 1.51  2000/08/21 20:49:43  jing
 * @
 *
 * Revision 1.50  2000/01/10 16:14:21  jing
 * @
 *
 * Revision 1.39  1999/06/29 21:19:02  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.37  1999/05/27 02:26:52  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.33  1999/05/03 20:57:32  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.19  1998/05/29 22:15:31  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/22 13:07:04  cm
 * SunOS 5.5 modifications
 *
*/


/* System include files */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

/* Local include files */

#include <misc.h>
#include <en.h>
#include <lb.h>
#include "lb_def.h"


#ifdef LB_THREADED
#include <pthread.h>
#endif

enum {ADD_LREC, REMOVE_LREC, UPDATE_LREC};
				/* values for arg "sw" of Update_lock_table */

/* static functions */
static int Reset_inuse_return (LB_struct *lb, int ret);
static int Get_IP_lock (LB_struct *lb, int lock_id, int type, int block);
static int File_lock (int sw, int fd, int type, int st_off);
static int Update_lock_table (int sw, LB_struct *lb, 
					int lock_id, int lock_type);
static void Remove_lock_record (Per_lb_data_t *pld, int index);
static int Test_lock (LB_struct *lb, int type, int lock_id);
static int Release_lock (LB_struct *lb, int lock_id);
static int Request_lock (LB_struct *lb, int type, int lock_id, int block);


/********************************************************************
			
    Description: This function acquires or releases a lock on message
		"id" in LB "lbd".

    Input:	lbd - the LB descriptor;
		command - LB_SHARED_LOCK, LB_EXCLUSIVE_LOCK or LB_UNLOCK;
			  ORed by LB_BLOCK.
		id - The message id.

    Returns:	This function returns LB_SUCCESS or a negative error 
		number on failure. 

    Notes:	Refer to lb.3 for a detailed description of this 
		function.

********************************************************************/

int LB_lock (int lbd, int command, LB_id_t id)
{
    LB_struct *lb;
    int ret, loff, cmd;
    int pt, page;
    int type, sw;

    if (id > LB_MAX_ID && id != LB_LB_LOCK && id != LB_TAG_LOCK)
	return (LB_BAD_ARGUMENT);

    /* get the LB structure */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);

    /* the message slot */
    if (id == LB_TAG_LOCK)		/* lock file offset */
	loff = LB_TAG_LOCK_OFF;
    else if (id == LB_LB_LOCK)
	loff = LB_LB_LOCK_OFF;
    else if (lb->flags & LB_DB) {
	if (LB_Search_msg (lb, id, &pt, &page) < 0)
	    return (Reset_inuse_return (lb, LB_NOT_FOUND));
	loff = LB_MSG_LOCK_OFF + pt;
    }
    else {
	loff = LB_MSG_LOCK_OFF + id;
    }

    cmd = command & LB_LOCK_COMMAND;
    if (cmd == LB_SHARED_LOCK || cmd == LB_EXCLUSIVE_LOCK) {
	if (cmd == LB_EXCLUSIVE_LOCK)
	    type = EXC_LOCK;
	else 
	    type = SHARED_LOCK;
	if (command & LB_BLOCK)
	    sw = SET_LOCK_WAIT;
	else
	    sw = SET_LOCK;
    }
    else if (cmd == LB_UNLOCK) {
	sw = UNSET_LOCK;
	type = EXC_LOCK;		/* not used */
    }
    else {
	if (cmd == LB_TEST_EXCLUSIVE_LOCK)
	    type = EXC_LOCK;
	else 
	    type = SHARED_LOCK;
	sw = TEST_LOCK;
    }

    ret = LB_process_lock (sw, lb, type, loff);

    if (ret == LB_LOCKED || ret == LB_SELF_LOCKED)
	return (Reset_inuse_return (lb, LB_HAS_BEEN_LOCKED));
    else if (ret < 0)
	return (Reset_inuse_return (lb, ret));
    else
	return (Reset_inuse_return (lb, LB_SUCCESS));
}

/*******************************************************************

    Description: This function resets the inuse flag for LB "lb". 
		 It returns the value of argument "ret".

    Input:	lb - the LB structure;
		ret - return value

    Returns:	This function returns the argument "ret". 

********************************************************************/

static int Reset_inuse_return (LB_struct *lb, int ret)
{

    lb->inuse = 0;
    EN_internal_unblock_NTF ();
    return (ret);
}

/*******************************************************************

    Description: Initializes the per-LB lock record table.

    Input:	pld - the per LB data struct;

    Returns:	LB_SUCCESS on success or a negative LB error number. 

********************************************************************/

int LB_init_lock_table (Per_lb_data_t *pld)
{

    pld->n_lock_recs = 0;
    pld->lock_recs_tid = MISC_open_table (sizeof (Lock_record_t), 16, 
			1, &(pld->n_lock_recs), (char **)&(pld->lock_recs));
    if (pld->lock_recs_tid == NULL)
	return (LB_MALLOC_FAILED);

#ifdef LB_THREADED
    pthread_mutex_init (&(pld->access_mutex), NULL);
    pthread_cond_init (&(pld->recs_ok), NULL);
    pthread_mutex_init (&(pld->lock_recs_mutex), NULL);
#endif

    return (LB_SUCCESS);
}

/*******************************************************************

    Description: Cleans up the per-LB lock record table.

    Input:	pld - the per LB data struct;

********************************************************************/

void LB_cleanup_lock_table (Per_lb_data_t *pld)
{

    if (pld->lock_recs_tid != NULL)
	MISC_free_table (pld->lock_recs_tid);

#ifdef LB_THREADED
    pthread_mutex_destroy (&(pld->access_mutex));
    pthread_cond_destroy (&(pld->recs_ok));
    pthread_mutex_destroy (&(pld->lock_recs_mutex));
#endif

}

/********************************************************************
			
    Description: This function removes all lock records owned by 
		"lb".

    Input:	lb - the lb involved.

********************************************************************/

void LB_cleanup_locks (LB_struct *lb)
{
    Per_lb_data_t *pld;
    int removed, i;

    pld = lb->pld;

    /* remove all locks */
#ifdef LB_THREADED
    pthread_mutex_lock (&(pld->lock_recs_mutex));
#endif
    removed = 0;
    for (i = pld->n_lock_recs - 1; i >= 0; i--) {
	if (pld->lock_recs[i].lb == lb) {
	    MISC_table_free_entry (pld->lock_recs_tid, i);
	    removed = 1;
	}
    }
#ifdef LB_THREADED
    pthread_mutex_unlock (&(pld->lock_recs_mutex));
    if (removed)
	pthread_cond_broadcast (&(pld->recs_ok));
#endif
}

/*******************************************************************

    Description: This is the basic LB internal lock function. A lock
		is identified by a number "lock_id". One can have 
		many independent locks. In this implementation, 
		inter-process lock uses the file lock on the byte 
		of offset "lock_id" in the LB lock file. Inter-thread
		lock is implemented using condition waiting.

    Input:	sw - function switch (SET_LOCK_WAIT, SET_LOCK or 
				UNSET_LOCK or TEST_LOCK)
		lb - the LB involved.
		type - lock type (EXC_LOCK or SHARED_LOCK)
		lock_id - the lock ID number.

    Returns:	This function returns LB_SUCCESS on success or 
		LB_LOCKED if a lock can not be obtained. It returns 
		LB_SELF_LOCKED if the lock is held by the same fd. 
		When sw = TEST_LOCK, it returns LB_SUCCESS, if nobody 
		holds the lock, or LB_LOCKED if anybody (including 
		the fd itself) holds the lock. It returns 
		negative LB error numbers on failure conditions.

********************************************************************/

int LB_process_lock (int sw, LB_struct *lb, int type, int lock_id)
{

    if (lock_id == LB_EXCLUSIVE_LOCK_OFF) {
	if ((lb->flags & LB_SINGLE_WRITER) && lb->off_nra == 0)
	    return (LB_SUCCESS);	/* LB access lock is not needed */
	else
	    return (File_lock (sw, lb->pld->lockfd, type, lock_id));
    }
    else if (lock_id == LB_TEST_LOCK_OFF)
	return (File_lock (sw, lb->pld->lockfd, type, lock_id));

    switch (sw) {

	case UNSET_LOCK:
	    return (Release_lock (lb, lock_id));

	case TEST_LOCK:
	    return (Test_lock (lb, type, lock_id));

	case SET_LOCK_WAIT:
	    return (Request_lock (lb, type, lock_id, 1));

	case SET_LOCK:
	    return (Request_lock (lb, type, lock_id, 0));
    }
    return (LB_BAD_ARGUMENT);
}

/********************************************************************

    Description: Tests a lock. Note that the function returns 
		LB_LOCKED even if it is locked by the fd itself.

    Input:	lb - the LB struct.
		type - lock type (EXC_LOCK or SHARED_LOCK)
		lock_id - lock ID.

    Return:	LB_SUCCESS if the lock is free, LB_LOCKED if the
		lock is not available, or a negative LB error number.

*********************************************************************/

static int Test_lock (LB_struct *lb, int type, int lock_id)
{
    Per_lb_data_t *pld;
    int retv, i;

    pld = lb->pld;
#ifdef LB_THREADED
    pthread_mutex_lock (&(pld->lock_recs_mutex));
#endif

    retv = LB_SUCCESS;
    for (i = 0; i < pld->n_lock_recs; i++) {	
	Lock_record_t *rec;

	rec = pld->lock_recs + i;
	if (rec->lock_id != lock_id)
	    continue;

	if (rec->lock_type == LRT_HOLD_EXCLUSIVE || 
	    rec->lock_type == LRT_LOCK_WAIT) {
	    retv = LB_LOCKED;
	    break;
	}
	else {			/* rec->lock_type == LRT_HOLD_SHARED */
	    if (type == EXC_LOCK) {
		retv = LB_LOCKED;
		break;
	    }
	}
    }
#ifdef LB_THREADED
	pthread_mutex_unlock (&(pld->lock_recs_mutex));
#endif

    /* see whether other process holds the lock */
    if (retv == LB_SUCCESS)
	retv = File_lock (TEST_LOCK, pld->lockfd, type, lock_id);

    return (retv);
}

/********************************************************************

    Description: Releases a lock. Because each LB fd can only have 
		one lock record, we do not need to specify lock type.

    Input:	lb - the LB struct.
		lock_id - lock ID.

    Return:	LB_SUCCESS on success or a negative LB error number.

*********************************************************************/

static int Release_lock (LB_struct *lb, int lock_id)
{
    Per_lb_data_t *pld;
    int index, cnt, i;
    Lock_record_t *rec;

    pld = lb->pld;
#ifdef LB_THREADED
    pthread_mutex_lock (&(pld->lock_recs_mutex));
#endif

    cnt = 0;			/* lock count */
    index = -1;			/* record index */
    for (i = 0; i < pld->n_lock_recs; i++) {	
	rec = pld->lock_recs + i;
	if (rec->lock_id != lock_id)
	    continue;
	if (rec->lock_type == LRT_LOCK_WAIT)
	    break;

	cnt++;
	if (rec->lb == lb)
	    index = i;
	if (rec->lock_type == LRT_HOLD_EXCLUSIVE)
	    break;
    }

    if (index >= 0) {
	int wait;

	/* remove the record */
	Remove_lock_record (pld, index);

	if (cnt == 1)		/* remove inter-process lock. argument EXC_LOCK 
				   will not be used */
	    File_lock (UNSET_LOCK, pld->lockfd, EXC_LOCK, lock_id);

	wait = 0;		/* boolean: anybody is waiting for the lock? */
#ifdef LB_THREADED
	if (cnt == 1) {			/* find out if anybody is waiting */
	    int k;
	    for (k = pld->n_lock_recs - 1; k >= i; k--) {
		rec = pld->lock_recs + k;
		if (rec->lock_id == lock_id) {
		    if (rec->lock_type == LRT_LOCK_WAIT)
			wait = 1;	/* some body is waiting */
		    break;
		}
	    }
	}
	pthread_mutex_unlock (&(pld->lock_recs_mutex));
	if (wait)	/* send signal to wake up blocking threads */
	    pthread_cond_broadcast (&(pld->recs_ok));
#endif
	return (LB_SUCCESS);
    }
    else {
#ifdef LB_THREADED
	pthread_mutex_unlock (&(pld->lock_recs_mutex));
#endif
	return (LB_NOT_FOUND);
    }
}

/********************************************************************

    Description: Requests a lock. A lock is identified by an LB and
		a lock ID number. It can be either shared or exclusive.
		When an LB fd acquires a lock, it writes a lock record
		in the lock record table. The record is removed when
		the lock is released. Locks are owned by LB fds that
		acquire them. A lock can only be released by its owner.
		When a thread terminates, all its locks are released
		automatically. When a thread waits for a lock, it 
		puts a LRT_LOCK_WAIT lock record in the table. The 
		record is updated to LRT_HOLD_* or removed before 
		returning from this function. The LRT_LOCK_WAIT records
		are used for implementing the "first waiting
		fd gets the lock first" policy. To support this, 
		the order of all WAIT records must be maintained in 
		the lock record table. Thus there can be at most one 
		LRT_LOCK_WAIT record for each thread in the table.
		For each lock_id, there is no non-LRT_LOCK_WAIT 
		records after the first LRT_LOCK_WAIT record. These 
		properties are used in several functions in this 
		module.

    Input:	lb - the LB struct.
		type - lock type (EXC_LOCK or SHARED_LOCK)
		lock_id - lock ID.
		block - boolean - block or not.

    Return:	LB_SUCCESS on success. LB_LOCKED if the lock is held
		by another process. LB_SELF_LOCKED if the lock is 
		held by the same fd. Or a negative LB error number.

*********************************************************************/

static int Request_lock (LB_struct *lb, int type, int lock_id, int block)
{
    Per_lb_data_t *pld;
    int wait_state, retv;

    pld = lb->pld;
#ifdef LB_THREADED
    pthread_mutex_lock (&(pld->lock_recs_mutex));
#endif
    wait_state = 0;		/* not in lock waiting state */
    while (1) {			/* self locks and same-thread conflicting locks 
				   are processed in this loop */
	enum {NOT_LOCKED, LOCKED_EXCLUSIVE, LOCKED_SHARED, OTHER_WAIT};
				/* for status value */
	int status, i;

	status = NOT_LOCKED;
	for (i = 0; i < pld->n_lock_recs; i++) {	/* check the table */
	    Lock_record_t *rec;

	    rec = pld->lock_recs + i;
	    if (rec->lock_id != lock_id)
		continue;

#ifdef LB_THREADED
	    if (rec->lock_type == LRT_LOCK_WAIT) {
		if (rec->lb != lb)
		    status = OTHER_WAIT;
		break;
	    }
#endif
	    
	    if (rec->lb == lb) {		/* self locked */
		retv = LB_SELF_LOCKED;
		goto done;
	    }

	    if (rec->lock_type == LRT_HOLD_EXCLUSIVE) {
		if (rec->lb->thread_id == lb->thread_id) {
		    retv = LB_LOCKED;
		    goto done;
		}
		else {
		    status = LOCKED_EXCLUSIVE;
		    break;
		}
	    }
	    else {		/* rec->lock_type == LRT_HOLD_SHARED */
		if (status == NOT_LOCKED)
		    status = LOCKED_SHARED;
		if (rec->lb->thread_id == lb->thread_id &&
		    type == EXC_LOCK) {
		    retv = LB_LOCKED;
		    goto done;
		}
	    }
	}

	switch (status) {

	    case NOT_LOCKED:		/* not locked and no fd is waiting 
					   in front of this */
		retv = Get_IP_lock (lb, lock_id, type, block);
		goto done;

	    case LOCKED_EXCLUSIVE:	/* excl. locked by another thread */
	    case OTHER_WAIT: 		/* another fd is waiting in front of 
					   this */
		break;			/* we have to wait */

	    case LOCKED_SHARED:		/* shared locked and no other fd waits 
					   in front of this */
		if (type == SHARED_LOCK) {
		    retv = Update_lock_table (ADD_LREC, 
					lb, lock_id, LRT_HOLD_SHARED);
		    goto done;
		}
		break;			/* we have to wait */
	}

	/* process wait */
	if (!block) {
	    retv = LB_LOCKED;
	    goto done;
	}

#ifdef LB_THREADED
	if (wait_state == 0) {
	    retv = Update_lock_table (ADD_LREC, lb, lock_id, LRT_LOCK_WAIT);
	    if (retv != LB_SUCCESS)
		goto done;
	    wait_state = 1;		/* enter in wait state */
	}
	pthread_cond_wait (&(pld->recs_ok), &(pld->lock_recs_mutex));
#endif
    }

done:

#ifdef LB_THREADED
    if (retv != LB_SUCCESS && wait_state)
	Update_lock_table (REMOVE_LREC, lb, lock_id, LRT_LOCK_WAIT);
    pthread_mutex_unlock (&(pld->lock_recs_mutex));
    if (retv != LB_SUCCESS && block)
	pthread_cond_broadcast (&(pld->recs_ok));
#endif
    return (retv);
}

/********************************************************************

    Description: Requests an interprocess lock. For blocking and 
		multiple threaded case, we have to first put 
		LRT_HOLD_* record in the table and then wait for the 
		IP lock.

    Input:	lb - the LB struct.
		lock_id - lock ID.
		type - lock type (EXC_LOCK or SHARED_LOCK).
		block - boolean - block or not.

    Return:	LB_SUCCESS on success or a negative LB error number.

*********************************************************************/

static int Get_IP_lock (LB_struct *lb, int lock_id, int type, int block)
{
    Per_lb_data_t *pld;
    int lock_type, ret;

    pld = lb->pld;
    if (type == EXC_LOCK)
	lock_type = LRT_HOLD_EXCLUSIVE;
    else
	lock_type = LRT_HOLD_SHARED;

    if (block) {
	ret = Update_lock_table (UPDATE_LREC, lb, lock_id, lock_type);
	if (ret != LB_SUCCESS)
	    return (ret);
#ifdef LB_THREADED
	pthread_mutex_unlock (&(pld->lock_recs_mutex));
		/* we must unlock to allow other lock procedures to go */
#endif
    }

    if (block)
        ret = File_lock (SET_LOCK_WAIT, pld->lockfd, type, lock_id);
    else
        ret = File_lock (SET_LOCK, pld->lockfd, type, lock_id);

#ifdef LB_THREADED
    if (block)
	pthread_mutex_lock (&(pld->lock_recs_mutex));
#endif

    if (ret == LB_SUCCESS) {
	if (!block) {
	    ret = Update_lock_table (ADD_LREC, lb, lock_id, lock_type);
	    if (ret < 0)
		return (ret);
	}
	return (LB_SUCCESS);
    }
    else {

	if (block)
	    Update_lock_table (REMOVE_LREC, lb, lock_id, lock_type);
	return (ret);
    }
}

/*******************************************************************

    Description: Updates the lock record table.

    Input:	sw - function switch:
			ADD_LREC: Adds a record.
			REMOVE_LREC: Removes a record.
			UPDATE_LREC: IF a WAIT record is found, 
				updates it to HOLD. Otherwise adds 
				a new record.
		lb - the LB struct;
		lock_id - lock ID;
		lock_type - lock type;

    Return:	LB_SUCCESS on success or a negative LB error number.

********************************************************************/

static int Update_lock_table (int sw, LB_struct *lb, 
					int lock_id, int lock_type)
{
    Per_lb_data_t *pld;
    Lock_record_t *rec, *new_rec;
    int i;

    pld = lb->pld;
    if (sw == REMOVE_LREC) {
	for (i = 0; i < pld->n_lock_recs; i++) {	/* check the table */
	    rec = pld->lock_recs + i;
	    if (rec->lock_id == lock_id && 
		rec->lb == lb && 
		rec->lock_type == lock_type) {
		Remove_lock_record (pld, i);
		break;
	    }
	}
	return (LB_SUCCESS);
    }

    new_rec = NULL;
    if (sw == UPDATE_LREC) {	/* find the record to update */
	for (i = pld->n_lock_recs - 1; i >= 0; i--) {
	    rec = pld->lock_recs + i;
	    if (rec->lock_id == lock_id) {
		if (rec->lock_type != LRT_LOCK_WAIT)
		    break;
		else if (rec->lb == lb) {
		    new_rec = rec;
		    break;
		}
	    }
	}
    }

    if (new_rec == NULL)
	new_rec = (Lock_record_t *)MISC_table_new_entry 
					(pld->lock_recs_tid, NULL);
    if (new_rec == NULL)
	return (LB_MALLOC_FAILED);
    new_rec->lb = lb;
    new_rec->lock_id = lock_id;
    new_rec->lock_type = lock_type;
    return (LB_SUCCESS);
}

/********************************************************************

    Description: Remove lock record of "index". Because non-WAIT
		records does not have to be in order, we use a more
		efficient procedure to delete it.

    Input:	pld - the per-LB data struct.
		index - record index to be deleted in the lock record table.

*********************************************************************/

static void Remove_lock_record (Per_lb_data_t *pld, int index)
{
    Lock_record_t *recs;
    int i;

    recs = pld->lock_recs;

    /* find the last non-WAIT record */
    for (i = pld->n_lock_recs - 1; i >= 0; i--) {
	if (recs[i].lock_type != LRT_LOCK_WAIT)
	    break;
    }

    if (i >= 0) {
	if (index != i)
	    recs[index] = recs[i];	/* copy rec i to rec index */
	MISC_table_free_entry (pld->lock_recs_tid, i);	/* remove rec i */
    }
    else		/* no non-WAIT record found */
	MISC_table_free_entry (pld->lock_recs_tid, index);
    return;
}

/*******************************************************************

    Description: This function locks, tests or unlocks a byte of 
		the LB lock file.

    Input:	sw - function switch (SET_LOCK_WAIT, SET_LOCK or 
				UNSET_LOCK or TEST_LOCK)
		fd - the file fd.
		type - lock type (EXC_LOCK or SHARED_LOCK)
		st_off - starting offset

    Returns:	This function returns LB_SUCCESS on success or when,
		sw = TEST_LOCK, the section is not locked, LB_LOCKED 
		if a lock can not be obtained or 
		LB_FCNTL_LOCK_FAILED if the "fcntl" call returns 
		with an error. 

********************************************************************/

static int File_lock (int sw, int fd, int type, int st_off)
{
    struct flock fl;		/* structure used by fcntl */
    int flag;
    int err;

    fl.l_whence = SEEK_SET;
    fl.l_start = st_off;
    fl.l_len = 1;

    if (sw != UNSET_LOCK) {
	if (type == EXC_LOCK)
	    fl.l_type = F_WRLCK;
	else 
	    fl.l_type = F_RDLCK;
	if (sw == SET_LOCK_WAIT)
	    flag = F_SETLKW;
	else if (sw == SET_LOCK)
	    flag = F_SETLK;
	else
	    flag = F_GETLK;
    }
    else {
	flag = F_SETLKW;
	fl.l_type = F_UNLCK;
    }

    while ((err = fcntl (fd, flag, &fl)) == -1 && errno == EINTR);

    if (err == -1) {
#ifdef SUNOS
	if (sw == SET_LOCK && (errno == EAGAIN || errno == EACCES))
#elif LINUX
	if (sw == SET_LOCK && (errno == EAGAIN || errno == EACCES))
#else
	if (sw == SET_LOCK && errno == EACCES)
#endif
	    return (LB_LOCKED);
	else if (errno == EAGAIN)
	    return (LB_FCNTL_LOCK_NOT_SUPPORTED);
	else {
	    MISC_log ("fcntl (file lock) failed (errno %d)\n", errno);
	    return (LB_FCNTL_LOCK_FAILED);
	}
    }

    if (sw == TEST_LOCK) {
	if (fl.l_type == F_UNLCK) /* not locked or locked by this process */
	    return (LB_SUCCESS);
	else
	    return (LB_LOCKED);
    }
    else
	return (LB_SUCCESS);
}



#ifdef OLD_CODE

/*******************************************************************

    Description: This function locks/unlocks a section of the LB
		lock file.

    Input:	sw - function switch (SET_LOCK_WAIT, SET_LOCK or 
				UNSET_LOCK or TEST_LOCK)
		lb - the LB involved.
		type - lock type (EXC_LOCK or SHARED_LOCK)
		st_off - starting offset

    Returns:	This function returns LB_SUCCESS on success or when,
		sw = TEST_LOCK, the section is not locked, LB_LOCKED 
		if a lock can not be obtained or an LB error number
		if the "fcntl" call returns with an error. 

********************************************************************/

int LB_lock_section (int sw, LB_struct *lb, int type, int st_off)
{
    struct flock fl;		/* structure used by fcntl */
    int flag, fd;
    int err;

    fd = lb->lockfd;
    fl.l_whence = SEEK_SET;
    fl.l_start = st_off;
    fl.l_len = 1;

    if (sw != UNSET_LOCK) {
	if (type == EXC_LOCK)
	    fl.l_type = F_WRLCK;
	else 
	    fl.l_type = F_RDLCK;
	if (sw == SET_LOCK_WAIT)
	    flag = F_SETLKW;
	else if (sw == SET_LOCK)
	    flag = F_SETLK;
	else
	    flag = F_GETLK;
    }
    else {
	flag = F_SETLKW;
	fl.l_type = F_UNLCK;
    }

    while ((err = fcntl (fd, flag, &fl)) == -1 && errno == EINTR);

    if (err == -1) {
#ifdef SUNOS
	if (sw == SET_LOCK && (errno == EAGAIN || errno == EACCES))
#else
	if (sw == SET_LOCK && errno == EACCES)
#endif
	    return (LB_LOCKED);
	else
	    return (LB_FCNTL_LOCK_FAILED);
    }

    if (sw == TEST_LOCK) {
	if (fl.l_type == F_UNLCK) {
	    if (lb->off_nra > 0 && st_off >= lb->off_nra &&
		Local_lock_record (lb, sw, st_off))	/* locally locked? */
		return (LB_LOCKED);
	    return (LB_SUCCESS);
	}
	else
	    return (LB_LOCKED);
    }
    else {
	if (lb->off_nra > 0 && st_off >= lb->off_nra)
	    Local_lock_record (lb, sw, st_off);
	return (LB_SUCCESS);
    }
}

/*******************************************************************

    Description: This function processes the local lock recording.

    Input:	lb, sw, st_off - See LB_lock_section.

    Returns:	returns non-zero if sw = TEST_LOCK and st_off is 
		locally locked or 0 otherwise. 

********************************************************************/

static int Local_lock_record (LB_struct *lb, int sw, int st_off)
{
    int ind;
    unsigned char bit_mask, *llrb;

    ind = st_off - lb->off_nra;
    if (ind >= LB_get_nra_size (lb))
	return (0);
    bit_mask = 1 << (ind % 8);
    llrb = lb->llr + (ind / 8);

    if (sw == TEST_LOCK) {
	if (*llrb & bit_mask)
	    return (1);
	else
	    return (0);
    }

    if (sw == UNSET_LOCK) 
	*llrb &= (~bit_mask);
    else if (sw == SET_LOCK_WAIT || sw == SET_LOCK)
	*llrb |= bit_mask;

    return (0);
}

/* The following section was in Initialize_lb_struct (lb_open.c) right before 
   final return */

    if (lb->off_nra > 0) {		/* allocate local lock records buffer */
	int nb;

	nb = (n_nrs * LB_NR_FACTOR + 7) / 8;	/* number of bytes */
	lb->llr = malloc (nb);
	if (lb->llr == NULL)
	    return (LB_MALLOC_FAILED);
	memset (lb->llr, 0, nb);
    }

/* The following was in LB_struct (lb_def.h) before "int read_offset" */

    unsigned char *llr;		/* bit array storing local lock records. This
				   is needed since UNIX test lock does not
				   tell about locks gained by the local 
				   process */


#endif		/* OLD_CODE */

