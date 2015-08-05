/****************************************************************
		
    Module: lb_common.c	
				
    Description: This module contains the internal common 
	functions used by the LB module.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:51 $
 * $Id: lb_common.c,v 1.66 2012/06/14 18:57:51 jing Exp $
 * $Revision: 1.66 $
 * $State: Exp $
 */

/* System include files */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   /* for memcpy */
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#if defined(LINUX)
#include <sys/statfs.h>
#endif

/* Local include files */

#include <lb.h>
#include <misc.h>
#include <en.h>
#include "lb_def.h"


#if (defined (SUNOS) || defined (LINUX) || defined (IRIX))
    #define MAP_FILE 0
    #define MAP_VARIABLE 0
#endif
#if (defined (SUNOS))
#ifdef __cplusplus
extern "C"
{
#endif

void madvise (char *cpt, int map_len, int type);
#ifdef __cplusplus
}
#endif
#endif

/* static functions */
static unsigned int Check_parity (LB_struct *lb);
static void Set_protected_mode (LB_struct *lb);
static int Check_upd_flag (LB_struct *lb, int perm);
static char *Call_mmap (int size, int offset, int fd, 
					int write_p, int is_private);

	
/********************************************************************
			
    Description: This function updates the current number-pointer
		combination and the pointer page in terms of the current
		number-pointer combination, page, number of added
		new messages and number of removed messages. 

    Input:	lb - the LB structure;
		n_add - number of added messages;
		n_rm - number of removed messages;

    Notes:	n_add can only be either 0 or 1.

********************************************************************/

void LB_Update_pointer (LB_struct *lb, int n_add, int n_rm)
{
    unsigned int num_ptr;
    int nmsgs, ptr, page;

    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    ptr = GET_POINTER(num_ptr);

    page = lb->hd->ptr_page;
    page = LB_Get_corrected_page_number (lb, ptr, page);

    if (n_add > 0) {
	ptr = (ptr + 1) % lb->ptr_range;
	if ((ptr % lb->page_size) == 0)	 /* update page number */
	    page++; 
	if (page != lb->hd->ptr_page)
	    lb->hd->ptr_page = page;
    }

    nmsgs = nmsgs + n_add - n_rm;
    if (nmsgs < 0)
	nmsgs = 0;
    if (nmsgs > lb->maxn_msgs)
	nmsgs = lb->maxn_msgs;
    num_ptr = COMBINE_NUMPT (nmsgs, ptr);
    lb->hd->num_pt = num_ptr;

    return;
}
	
/********************************************************************
			
    Description: This function computes the new pointer and page 
		values given a starting pointer and page and an
		increment value. 

    Input:	lb - the LB structure;
		pt - starting pointer;
		page - starting page;
		inc - number of increments (plus or minus);

    output:	n_page - The page number for the new pointer

    Returns:	This function returns the new pointer.

********************************************************************/

int LB_Compute_pointer (LB_struct *lb, int pt, int page, int inc,
		int *n_page)
{
    int newpt;		/* new pointer */
    int page_size;
    int newpage;	/* new page */

    newpt = pt + inc;
    if (newpt < 0)
	newpt = newpt + (1 - newpt / lb->ptr_range) * lb->ptr_range;
    newpt = newpt % lb->ptr_range;

    /* update page number */
    page_size = lb->page_size;
    if (inc >= 0) {
	int t;

	t = inc - (page_size - (pt % page_size));
	if (t >= 0) 
	    newpage = page + (1 + t / page_size);
	else 
	    newpage = page;
    }
    else {
	int t;

	t = -inc - (pt % page_size) - 1;
	if (t >= 0) 
	    newpage = page - (1 + t / page_size);
	else 
	    newpage = page;
    }

    *n_page = newpage;
    return (newpt);
}

/********************************************************************
			
    Description: This function checks if a message pointed by "ptr" 
		of "page" is available in terms of the current LB 
		status. 

    Input:	lb - the LB structure;
		ptr - a message pointer
		page - page number of the pointer

    Returns:	This function returns RP_OK, RP_EXPIRED or RP_TO_COME
		indicating that the message pointed to by the pointer
		is available, expired or yet-to-come respectively. 

********************************************************************/

int LB_Check_pointer (LB_struct *lb, int ptr, int page)
{
    unsigned int num_ptr;
    int max, min;
    int nmsgs, pt, ptr_page;

    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    pt = GET_POINTER(num_ptr);
    ptr_page = lb->hd->ptr_page;
    ptr_page = LB_Get_corrected_page_number (lb, pt, ptr_page);

    if (page > ptr_page || 
	(page >= ptr_page && ptr >= pt))
	return (RP_TO_COME);

    if (ptr_page - page > 2)
	return (RP_EXPIRED);

    max = pt - 1;		/* min and max define the pointer range of 
				   available msgs */
    if (max < 0)
	max += lb->ptr_range;
    min = max - nmsgs + 1;
    if (min < 0) {
	if (ptr <= max)
	    ptr += lb->ptr_range;
	max += lb->ptr_range;
	min = max - nmsgs + 1;
    }
    if (ptr <= max && ptr >= min) {
	return (RP_OK);
    }
    else
	return (RP_EXPIRED);
}
	
/********************************************************************
			
    Description: This function finds out the corrected page number. 

    Input:	lb - the LB structure;
		pointer - a message pointer
		ref_page - page number of the pointer read from the header

    Returns:	This function returns the corrected page number.

    Notes:	Because the shared page number is not updated atomicly
		with the pointer update, it is possible that the 
		page number read from the LB header can be offset by
		1 from its correct value. This offset can be corrected due 
		to the fact that the pointer range covers at least 4 pages.
		This function returns a message's corrected page number 
		based on its message pointer and its reference page number.

		In normal use, nc in the function can never be 2. But we
		don't check this and process it anyway.

********************************************************************/

int LB_Get_corrected_page_number (
    LB_struct *lb,	/* the LB structure */
    int pointer,	/* the message pointer */
    int ref_page	/* reference page number of the message */
) {
    int np, onp, nc;

    np = pointer / lb->page_size;
    onp = ref_page % N_PAGES;
    nc = (np - onp + N_PAGES) % N_PAGES;
    if (nc == 3)
	nc = -1;

    return (ref_page + nc);
}

/********************************************************************
			
    Description: This function compares two message pointers.

    Input:	pt1 - the first message pointer;
		pt2 - the second message pointer;

    Returns:	This function returns 1, 0 or -1 respectively when
		pt1 > pt2, pt1 == pt2 or pt1 < pt2.

    Notes:	The distance between the two pointers is assumed to 
		be no larger than maxn_msgs and pointer range is no
		less than 4 * maxn_msgs.

********************************************************************/

int LB_Ptr_compare (LB_struct *lb, int pt1, int pt2)
{
    int diff;

    if (pt1 == pt2)
	return (0);

    diff = pt1 - pt2;
    if (diff < 0)
	diff = -diff;

    if (diff < 2 * lb->maxn_msgs) {
	if (pt1 > pt2)
	    return (1);
	else
	    return (-1);
    }
    else {
	if (pt1 > pt2)
	    return (-1);
	else
	    return (1);
    }
}

/********************************************************************
			
    Description: This function searches the message directory area to 
		find the pointer and page of the latest message with
		"id".

    Input:	lb - the LB structure;
		id - message id to be searched for;

    Output:	pt - the message pointer found;
		page - pointer page;

    Returns:	This function returns LB_SUCCESS on success or 
		LB_FAILURE if the message is not found.

    Notes:	We might verify nmsgs read from the header to be more
		save. But we can not fully protect the LB from damages
		caused by illegal uses.

********************************************************************/

#define N_BINARY_SEARCH_MSGS	8	/* min number of msgs to use
					   binary search */

int LB_Search_msg (LB_struct *lb, LB_id_t id, int *pt, int *page)
{
    unsigned int num_ptr, id_mask;
    int nmsgs, ptr, ptr_page;
    int n_slots, s, n, i;
    LB_dir *slot, *first_slot;
    LB_id_t masked_id;

    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    ptr = GET_POINTER(num_ptr);
    ptr_page = lb->hd->ptr_page;
    ptr_page = LB_Get_corrected_page_number (lb, ptr, ptr_page);

    if (id == LB_LATEST) {
	*pt = LB_Compute_pointer (lb, ptr, ptr_page, -1, page);
	return (LB_SUCCESS);
    }

    n_slots = lb->n_slots;
    first_slot = lb->dir;

    id_mask = 0xffffffff;
    if ((lb->flags & LB_DB) && !(lb->hd->miscflags & LB_ID_BY_USER))
	id_mask = LB_MSG_DB_ID_MASK;
    masked_id = id & id_mask;
    if (nmsgs >= N_BINARY_SEARCH_MSGS &&	/* use binary search */
	(lb->hd->non_dec_id & 1)) {
	int st, end, ind;

	end = ptr - 1;
	st = end - nmsgs + 1;
	if (st < 0) {
	    end += n_slots;
	    st += n_slots;
	}
	while (1) {
	    ind = (st + end) >> 1;
	    if (ind == st) {
		slot = lb->dir + ((end + 1) % n_slots);
		s = ptr - end - 1;
		if (s < 0)
		    s += n_slots;
		n = s + 2;
		break;
	    }
	    if (masked_id >= ((first_slot[ind % n_slots].id) & id_mask))
		st = ind;
	    else
		end = ind;
	}
    }
    else {
	slot = lb->dir + (ptr % n_slots);
	s = 0;
	n = nmsgs;
    }

    for (i = s; i < n; i++) {

	slot--;
	if (slot < first_slot)
	    slot += n_slots;

	if (slot->id == id) {
	    *pt = LB_Compute_pointer (lb, ptr, ptr_page, -(i + 1), page);
	    return (LB_SUCCESS);
	}
    }

    return (LB_FAILURE);
}

/********************************************************************
			
    Description: This function finds out the id, location and length of 
		the message pointed to by "rpt" assuming that the 
		message is available. This works for any sequential LB.

    Input:	lb - the LB structure;
		rpt - the message pointer;

    Output:	loc - if not NULL, message location (offset in the 
			buffer area);
		id - if not NULL, the message id;

    Returns:	the function returns the length of the message;

********************************************************************/

int LB_Get_message_info (LB_struct *lb, int rpt, int *loc, LB_id_t *id)
{
    int ppt;		/* pointer to the previous message */
    LB_dir *curr, *prev;
    int len, ind;

    ind = rpt % lb->n_slots;

    if (lb->flags & LB_DB) {
	LB_msg_info_t *info;
	int ucnt;

	do {
	    ucnt = lb->dir[ind].loc;
	    info = ((LB_msg_info_t *)lb->msginfo) + DB_WORD_OFFSET (ind, ucnt);
	    if (loc != NULL)
		*loc = info->loc;
	    if (id != NULL)
		*id = lb->dir[ind].id;
	    len = info->len;
	} while (ucnt != lb->dir[ind].loc);
	return (len);
    }

    curr = lb->dir + ind;
    if (id != NULL)
	*id = curr->id;
    if (lb->ma_size == 0) {
	if (loc != NULL)
	    *loc = curr->loc;
	return (((LB_msg_info_seq_t *)lb->msginfo + ind)->len);
    }

    ppt = rpt - 1;
    if (ppt < 0)
	ppt += lb->ptr_range;

    prev = lb->dir + (ppt % lb->n_slots);
    if (loc != NULL)
	*loc = prev->loc;
    len = curr->loc - prev->loc;
    if (len <= 0) {
	len = len + lb->ma_size - lb->hd->unused_bytes;
	if (loc != NULL)
	    *loc = ((*loc + lb->hd->unused_bytes) % lb->ma_size);
    }

    return (len);
}
	
/********************************************************************
			
    Description: This function conducts a LB_struct parity check, 
		locks, if necessary, the LB file and attaches to 
		the shared memory or mmaps the LB file.

    Input:	lbd - the LB descriptor;
		perm - access permission (WRITE_PERM or READ_PERM);
		lock - lock type (NO_LOCK, EXC_LOCK or SHARED_LOCK);

    Returns:	This function returns LB_SUCCESS on success or a 
		negative LB error return value on failure.

********************************************************************/

int LB_lock_mmap (LB_struct *lb, int perm, int lock)
{
    int ret;

#ifdef LB_THREADED
    pthread_mutex_lock (&lb->pld->access_mutex);
#endif

    /* parity check */
    if (perm == WRITE_PERM &&
	lb->parity != Check_parity (lb))
	return (LB_PARITY_ERROR);

    if ((ret = LB_mmap (lb, perm)) < 0)
	return (ret);

    if (lb->upd_flag_check) {		/* check update flag */
	if ((ret = Check_upd_flag (lb, perm)) < 0)
	    return (ret);
    }

    /* lock the file */
    if (lock != NO_LOCK && lb->locked == LB_UNLOCKED) {
	if ((ret = LB_process_lock (SET_LOCK_WAIT, lb, lock, 
				LB_EXCLUSIVE_LOCK_OFF)) < 0) {
	    return (ret);
	}

	lb->locked = LB_LB_LOCKED;
    }

    return (LB_SUCCESS);
}
	
/********************************************************************
			
    Description: This function checks the LB update flag. If the flag
		is set, the function blocks until the LB exclusive
		access lock is gained. Then it resets the flag. This
		is needed to guarantee that we always access updated
		LB after receiving an UN.

    Input:	lB - the LB structure;
		perm - access permission (WRITE_PERM or READ_PERM);

    Returns:	This function returns LB_SUCCESS on success or a 
		negative LB error return value on failure.

********************************************************************/

static int Check_upd_flag (LB_struct *lb, int perm)
{
    int ret;

    if (lb->hd->upd_flag) {		/* LB update in progress */
	if ((ret = LB_process_lock (SET_LOCK_WAIT, lb, EXC_LOCK, 
			LB_EXCLUSIVE_LOCK_OFF)) < 0)	/* wait */
	    return (ret);
	LB_process_lock (UNSET_LOCK, lb, EXC_LOCK, LB_EXCLUSIVE_LOCK_OFF);
	if (lb->hd->upd_flag && perm != WRITE_PERM) {
					/* this happens only if the writer dies 
					   before completing write */
	    Set_protected_mode (lb);
	    if ((ret = LB_mmap (lb, WRITE_PERM)) < 0)
		return (ret);
	    lb->hd->upd_flag = 0;
	    Set_protected_mode (lb);
	    if ((ret = LB_mmap (lb, perm)) < 0)
		return (ret);
	}
    }
    return (LB_SUCCESS);
}
	
/********************************************************************
			
    Description: This function attaches to 
		the shared memory or mmaps the LB file.

    Input:	lbd - the LB descriptor;
		perm - access permission (WRITE_PERM or READ_PERM);

    Returns:	This function returns LB_SUCCESS on success or a 
		negative LB error return value on failure.

********************************************************************/

int LB_mmap (LB_struct *lb, int perm)
{
    static int prev_perm = READ_PERM;
    char *cpt;
    Per_lb_data_t *pld;

    if (perm != PREV_PERM)
	prev_perm = perm;
    else
	perm = prev_perm;

    /* mmap the file or shared memory */
    pld = lb->pld;
    cpt = NULL;			/* not necessary - turn off gcc warning */
    if (pld->lb_pt == NULL) {

	if (lb->flags & LB_MEMORY) {
	    if (perm == WRITE_PERM)
		cpt = (char *)shmat (pld->shmid, (char *)0, 0);
	    else
		cpt = (char *)shmat (pld->shmid, (char *)0, SHM_RDONLY);
	    if ((int)cpt == -1) {
		MISC_log ("shmat failed (errno %d)\n", errno);
		return (LB_MMAP_FAILED);
	    }
	}
	else {
	    int wp = 0;
	    if (perm == WRITE_PERM)
		wp = 1;
	    cpt = Call_mmap (pld->cntr_mlen, 0, pld->fd, wp, 0);
	    if (cpt == NULL)
		return (LB_MMAP_FAILED);
	}
    }
    else if (perm == WRITE_PERM) {	/* turn on write permission */

	if (lb->flags & LB_MEMORY) {
	    if (pld->cw_perm == LB_FALSE) {
		if (shmdt ((char *)pld->lb_pt) < 0)
		    return (LB_SHMDT_FAILED);
		pld->lb_pt = NULL;
		if ((int)(cpt = (char *)shmat (pld->shmid, (char *)0, 0)) == -1)
		    return (LB_MMAP_FAILED);
	    }
	}
	else {
	    if (pld->lb_pt != NULL && pld->cw_perm == LB_FALSE &&
		mprotect (pld->lb_pt, pld->cntr_mlen, 
					PROT_READ | PROT_WRITE) < 0)
		return (LB_MPROTECT_FAILED);
	    pld->cw_perm = LB_TRUE;

	    if (pld->map_pt != NULL && pld->map_off != 0 && 
		pld->dw_perm == LB_FALSE &&
		mprotect (pld->map_pt, pld->map_len, 
					PROT_READ | PROT_WRITE) < 0)
		return (LB_MPROTECT_FAILED);
	    pld->dw_perm = LB_TRUE;
	}
    }

    /* set the lb pointer fields */
    if (pld->lb_pt == NULL) {
	pld->lb_pt = cpt;
	pld->cw_perm = LB_FALSE;
	if (perm == WRITE_PERM)
	    pld->cw_perm = LB_TRUE;
	if (pld->rsid != NULL)
	    pld->rsid = NULL;
    }

    cpt = pld->lb_pt;
    lb->hd = (Lb_header_t *)cpt;
    lb->dir = (LB_dir *)(cpt + sizeof (Lb_header_t));
    if ((lb->flags & LB_DB) || lb->ma_size == 0) {
	lb->msginfo = (void *)(cpt + lb->off_msginfo);
    }
    lb->tag = (lb_t *)(cpt + lb->off_tag);

    return (LB_SUCCESS);
}

/*******************************************************************

    Description: This function releases the lock for LB "lb" if it is 
		locked and the LB is not a single writer LB. It also 
		either unmaps the LB memory or remove the write 
		permission if the LB is protected. It returns the 
		value "ret".

    Input:	lb - the LB structure;
		ret - return value

    Returns:	This function returns the argument "ret". 

********************************************************************/

int LB_Unlock_return (LB_struct *lb, int ret)
{

    if (lb->upd_flag_set) {
	lb->hd->upd_flag = 0;
	lb->upd_flag_set = 0;
    }

    if (lb->locked == LB_LB_LOCKED) {
	LB_process_lock (UNSET_LOCK, lb, 0, LB_EXCLUSIVE_LOCK_OFF);
	lb->locked = LB_UNLOCKED;
    }

    Set_protected_mode (lb);
    LB_Set_parity (lb);
    LB_reset_inuse (lb);

#ifdef LB_THREADED
    pthread_mutex_unlock (&lb->pld->access_mutex);
#endif

    return (ret);
}

/*******************************************************************

    Description: This function puts the LB space into write protected
		mode.

    Input:	lb - the LB structure;

********************************************************************/

static void Set_protected_mode (LB_struct *lb)
{
    Per_lb_data_t *pld;

    pld = lb->pld;
    if (!(lb->flags & LB_UNPROTECTED)) {
	if (lb->flags & LB_MEMORY) {
	    if (pld->lb_pt != NULL && pld->cw_perm == LB_TRUE) {
		shmdt ((char *)pld->lb_pt);
		pld->lb_pt = NULL;
	    }
	}
	else {
	    if (pld->lb_pt != NULL && pld->cw_perm == LB_TRUE) {
		mprotect ((char *)pld->lb_pt, pld->cntr_mlen, PROT_READ);
		pld->cw_perm = LB_FALSE;
	    }

	    if (pld->map_pt != NULL && pld->map_off != 0 && 
					pld->dw_perm == LB_TRUE && 
		!(lb->direct_access && (lb->flags & LB_WRITE))) {
/*
		munmap ((char *)pld->map_pt, pld->map_len);
		pld->map_pt = NULL;
*/
		mprotect ((char *)pld->map_pt, pld->map_len, PROT_READ);
		pld->dw_perm = LB_FALSE;
	    }
	}
    }
#ifdef MMAP_UNMAP_NEEDED
    if (lb->ma_size == 0) {	/* for variable size LB we always unmap the 
				   data area */
	if (pld->map_pt != NULL && pld->map_off != 0)
	    munmap ((char *)pld->map_pt, pld->map_len);
	pld->map_pt = NULL;
    }
#endif
    return;
}

/*******************************************************************

    Description: This function resets the inuse flag and unsets the 
		UN block. This function must be called after 
		LB_Get_lb_structure returns success. It must be 
		called right before returning from an LB function.

    Input:	lb - the LB structure;

********************************************************************/

void LB_reset_inuse (LB_struct *lb)
{
    lb->inuse = 0;
    EN_internal_unblock_NTF ();
    return;
}

/*******************************************************************

    Description: This function unmap all mamory mapped file segments.

    Input:	lb - the LB structure;

********************************************************************/

void LB_unmap (LB_struct *lb)
{
    Per_lb_data_t *pld;

    pld = lb->pld;
    if (lb->flags & LB_MEMORY) 
	return;

    if (pld->lb_pt != NULL) {
	munmap ((char *)pld->lb_pt, pld->cntr_mlen);
	pld->lb_pt = NULL;
    }
    if (pld->map_pt != NULL && pld->map_off != 0) {
	munmap ((char *)pld->map_pt, pld->map_len);
	pld->map_pt = NULL;
    }
    return;
}

/********************************************************************

    Description: This function returns the NRA (notification request 
		area) size.

    Input:	lb - the LB structure;

    Return:	The NRA size.

********************************************************************/

int LB_get_nra_size (LB_struct *lb)
{
    return (lb->hd->nra_size * LB_NR_FACTOR);
}

/********************************************************************
			
    Description: This function transfers data of length "len" in "buf" 
		to/from (sw = WRITE_TRANS / READ_TRANS) a mmapped file 
		started from offset "loc".

    Input:	sw - functional switch (WRITE_TRANS or READ_TRANS);
		lb - the LB structure;
		buf - the data buffer;
		len - data length;
		loc - location (offset) in the file;

    Output:	buf - data read from the file (when sw = READ_TRANS);

    Returns:	This function returns LB_SUCCESS on success or
		a negative LB error number on failure.

    Notes:	The mmapped area for the data transfer is not unmapped. 
		The limitation that one can not mmap overlapped areas 
		is appropriately processed here. For better performance
		we use private mmap for read when either the message or
		the LB is large. The private mmap has to be released
		to allow next updated read.

********************************************************************/

#define MAX_MMAP_PAGES 1		/* maximum mmapped pages */
			/* If we use MAX_MMAP_PAGES other than 1, we need
			   to pre-partition the entire message area */

int LB_data_transfer (int sw, LB_struct *lb,char *buf, int len, int loc)
{
    int private_type;	/* 1: use private map for read; 2: private mmapped */
    Per_lb_data_t *pld;

    if (len < 0 || loc < 0)
	return (LB_LB_ERROR);
    if (len == 0)
	return (LB_SUCCESS);

    pld = lb->pld;	/* we need to redo mmap since file may be trancated */
#ifdef MMAP_UNMAP_NEEDED
    if (lb->ma_size == 0 && pld->lb_pt != NULL && loc < pld->cntr_mlen) {
	LB_unmap (lb);
	LB_mmap (lb, PREV_PERM);
    }
#endif

    if (pld->map_pt != NULL && pld->map_off == 0) {
		/* in case that pld->lb_pt is changed or unmapped */	
	pld->map_pt = pld->lb_pt;	
	pld->map_len = pld->cntr_mlen;
    }

    if (len > 1000 || lb->ma_size > 1000000)
	private_type = 1;		/* use private mmap for read */
    else
	private_type = 0;		/* use shared mmap for read */

    while (1) {				/* do until all data transferred */

	/* transfer data located within the mmapped area */
	if (pld->map_pt != NULL && loc >= pld->map_off && 
				  loc < pld->map_off + pld->map_len) {
	    int clen;

	    clen = pld->map_off + pld->map_len - loc;	/* length to copy */
	    if (clen > len)
		clen = len;

	    if (sw == WRITE_TRANS)
		memcpy (pld->map_pt + loc - pld->map_off, buf, clen);
	    else
		memcpy (buf, pld->map_pt + loc - pld->map_off, clen);

	    len -= clen;
	    loc += clen;
	    buf += clen;
	}

	/* check whether the job is done */
	if (len <= 0) {
	    if ((pld->map_pt != NULL && pld->lb_pt == NULL && 
					pld->map_off < pld->cntr_mlen) ||
		private_type == 2) {
		munmap ((char *)pld->map_pt, pld->map_len);
		pld->map_pt = NULL;	/* make sure the control area can be 
					   mmapped later or private map has to 
					   be unmapped */
	    }
	    return (LB_SUCCESS);
	}

	/* unmap the area */
	if (pld->map_pt != NULL && pld->map_pt != pld->lb_pt &&
	    munmap ((char *)pld->map_pt, pld->map_len) < 0)
	    return (LB_LB_ERROR);
	pld->map_pt = NULL;

	/* mmap a new area */
	if (pld->lb_pt != NULL && loc < pld->cntr_mlen) {
				/* use the control mmapped area */

	    if (sw == WRITE_TRANS && pld->cw_perm == LB_FALSE &&
		mprotect ((char *)pld->lb_pt, pld->cntr_mlen, 
					PROT_READ | PROT_WRITE) < 0)

		return (LB_MPROTECT_FAILED);

	    pld->map_pt = pld->lb_pt;
	    pld->map_off = 0;
	    pld->map_len = pld->cntr_mlen;
	    if (sw == WRITE_TRANS)
		pld->cw_perm = LB_TRUE;
	}
	else {			/* mmap a new area */
	    int map_off, map_len, mpg_size;
	    char *cpt;

	    mpg_size = lb->mpg_size;
	    map_off = (loc / mpg_size) * mpg_size;
	    map_len = len + (loc % mpg_size);
/*	    if (map_len > MAX_MMAP_PAGES * mpg_size)
		map_len = MAX_MMAP_PAGES * mpg_size; */
if (map_len > mpg_size)
    map_len = ((map_len - 1) / mpg_size) * mpg_size;
	    if (map_len < mpg_size)
		map_len = mpg_size;
/*	    if (map_off + map_len > lb->lb_size)
		map_len = lb->lb_size - map_off;   */
/* printf ("%d %d    %d %d\n", loc, len, map_len, map_off); */
	    if (map_len <= 0 || map_off < 0)
		return (LB_LB_ERROR);

	    if (sw == WRITE_TRANS) {
		cpt = Call_mmap (map_len, map_off, pld->fd, 1, 0);
		pld->dw_perm = LB_TRUE;
	    }
	    else {
		int p;

		p = 0;
		if (private_type)
		    p = 1;
		cpt = Call_mmap (map_len, map_off, pld->fd, 0, p);
		pld->dw_perm = LB_FALSE;
		if (private_type)
		    private_type = 2;		/* need unmap */
	    }
	    if (cpt == NULL) {
		pld->map_pt = NULL;
		return (LB_MMAP_FAILED);
	    }
#if (defined (HPUX) || defined (SUNOS))
	    madvise (cpt, map_len, MADV_SEQUENTIAL);
#endif
	    pld->map_pt = cpt;
	    pld->map_off = map_off;
	    pld->map_len = map_len;
	}
    }
}

/***********************************************************************

    This provides support for controlling direct access to file LB "lb".
    "lock_sw" controls the function to perform. "perm" is WRITE_PERM
    or READ_PERM. Return LB_SUCCESS or a negative error code.

***********************************************************************/

int LB_direct_access_lock (LB_struct *lb, int lock_sw, int perm) {
    Per_lb_data_t *pld;
    int ret, need_lock;

    if (!(lb->flags & LB_DIRECT))
	return (LB_SUCCESS);

    need_lock = 0;
    if ((lb->flags & LB_DB) || lb->ma_size == 0)
	need_lock = 1;
    pld = lb->pld;
    if (lock_sw == SET_LOCK) {		/* set lock to allow direct access */
	if (need_lock) {
	    ret = LB_process_lock (lock_sw, lb, 
				SHARED_LOCK, LB_DIRECT_ACCESS_LOCK_OFF);
	    if (ret == LB_SELF_LOCKED)
		return (LB_SUCCESS);
	    if (ret != LB_SUCCESS)
		return (LB_LOCK_FAILED);
	}
	if (!(lb->direct_access) && !(lb->flags & LB_MEMORY)) {
	    int off, fsize, wp;
	    char *cpt;

	    if (pld->map_pt != NULL && pld->map_off > 0)
		munmap ((char *)pld->map_pt, pld->map_len);
	    pld->map_pt = NULL;
	    off = lseek (pld->fd, 0, SEEK_CUR);
	    fsize = lseek (pld->fd, 0, SEEK_END);
	    if (off < 0 || fsize < 0) 
		return (LB_SEEK_FAILED);
	    lseek (pld->fd, off, SEEK_SET);
	    wp = 0;
	    if (perm == WRITE_PERM)
		wp = 1;
	    cpt = Call_mmap (fsize - lb->off_a, lb->off_a, pld->fd, wp, 0);
	    if (cpt == NULL)
		return (LB_MMAP_FAILED);
	    pld->map_pt = cpt;
	    pld->map_len = fsize - lb->off_a;
	    pld->map_off = lb->off_a;
	    lb->direct_access = 1;
	}
	return (LB_SUCCESS);
    }
    else if (lock_sw == UNSET_LOCK) {	/* unset direct access lock */
	if (need_lock)
	    LB_process_lock (lock_sw, lb, 
				SHARED_LOCK, LB_DIRECT_ACCESS_LOCK_OFF);
	if (lb->direct_access) {
	    if (pld->map_pt != NULL && pld->map_off > 0)
		munmap ((char *)pld->map_pt, pld->map_len);
	    pld->map_pt = NULL;
	    lb->direct_access = 0;
	}
	return (LB_SUCCESS);
    }
    else if (lock_sw == TEST_LOCK) {	/* test direct access lock */
	if (need_lock)
	    return (LB_process_lock (lock_sw, lb, 
				EXC_LOCK, LB_DIRECT_ACCESS_LOCK_OFF));
    }
    return (LB_SUCCESS);
}

/**************************************************************************

    Memery maps "size" bytes at "offset" of file "fd". If "write_p" it true.
    write permission is used. If "is_private" is true, provately mapped. 
    Returns the pointer on success or NULL on failure.

***************************************************************************/

static char *Call_mmap (int size, int offset, int fd, 
						int write_p, int is_private) {
    char *cpt;
    int p, f;

    p = PROT_READ;
    if (write_p)
	p |= PROT_WRITE;
    f = MAP_FILE | MAP_VARIABLE;
    if (is_private)
	f |= MAP_PRIVATE;
    else
	f |= MAP_SHARED;
    cpt = (char *)mmap (NULL, size, p, f, fd, offset);
    if ((int)cpt == -1) {
	MISC_log ("mmap (size %d, offset %d) failed (errno %d)\n", 
					size, offset, errno);
	return (NULL);
    }
    return (cpt);
}

/********************************************************************
			
    Description: This function adds all field values in the LB_struct 
		"lb" as a parity check value and returns it.

    Input:	lb - the LB structure;

    Returns:	The parity check value;

********************************************************************/

#define CHECK_SUM_SEED	9823758		/* check_sum seed number */

static unsigned int Check_parity (LB_struct *lb)
{
    unsigned int pa, *ipt, *ept;

    pa = CHECK_SUM_SEED;
    ipt = (unsigned int *)lb->pld;
    ept = ipt + N_PLD_VARIABLE_FIELDS;
    while (ipt < ept)
	pa += *ipt++;
    if (pa != lb->pld->parity)
	return (0);

    pa = CHECK_SUM_SEED;
    ipt = (unsigned int *)lb;
    ept = ipt + N_LBS_CHECKED_FIELDS;
    while (ipt < ept)
	pa += *ipt++;

    return (pa);
}

/********************************************************************
			
    Description: This function computes the check-sum of LB_struct 
		"lb" for later parity check.

    Input:	lb - the LB structure;

********************************************************************/

void LB_Set_parity (LB_struct *lb)
{
    unsigned int pa, *ipt, *ept;

    if (lb->partial == 0) {

	pa = CHECK_SUM_SEED;
	ipt = (unsigned int *)lb;
	ept = ipt + N_LBS_CHECKED_FIELDS;
	ipt += N_LBS_VARIABLE_FIELDS;
	while (ipt < ept)
	    pa += *ipt++;
	lb->partial = pa;
    }

    pa = lb->partial;
    ipt = (unsigned int *)lb;
    ept = ipt + N_LBS_VARIABLE_FIELDS;
    while (ipt < ept)
	pa += *ipt++;
    lb->parity = pa;

    pa = CHECK_SUM_SEED;
    ipt = (unsigned int *)lb->pld;
    ept = ipt + N_PLD_VARIABLE_FIELDS;
    while (ipt < ept)
	pa += *ipt++;
    lb->pld->parity = pa;

    return;
}

/********************************************************************
			
    Description: This function tests whether an external lock file is 
		needed. In this function we first mmap the
		the file and then try to lock it. If the lock call
		returns EAGAIN, we know that the external lock file is
		required.

    Input:	fd - the LB file descriptor;
		flags - the LB flags.

    Return:	Returns 1 if an external lock file is needed or 0 
		otherwise.

********************************************************************/

int LB_test_file_lock (int fd, int flags)
{
    char *cpt;
    int map_len = 256;
    int ret;
    LB_struct lb;
    Per_lb_data_t pld;
#if defined(LINUX)
    struct statfs sfs;
#endif

    if (flags & LB_MEMORY)
	return (0);			/* lock test is not needed */

    /* External locking is required for NFS files in Linux */
#if defined(LINUX)
    fstatfs (fd, &sfs);
    if (sfs.f_type == 0x6969)		/* NFS_SUPER_MAGIC = 0x6969 */
	return (1);
#endif

    cpt = Call_mmap (map_len, 0, fd, 1, 0);
    if (cpt == NULL)
	return (0);

    lb.pld = &pld;
    lb.pld->lockfd = fd;	/* set tmp lb for calling LB_process_lock */
    lb.off_nra = 0;
    ret = LB_process_lock (SET_LOCK_WAIT, &lb, EXC_LOCK, LB_TEST_LOCK_OFF);

    if (ret == LB_FCNTL_LOCK_NOT_SUPPORTED)
	return (1);
    else if (ret == LB_SUCCESS)
	LB_process_lock (UNSET_LOCK, &lb, EXC_LOCK, LB_TEST_LOCK_OFF);
    munmap (cpt, map_len);

    return (0);
}

/********************************************************************
			
    Description: This function processes byte swapping.

    Input:	x - the input word;

    Return:	The byte swapped x.

********************************************************************/

lb_t LB_byte_swap (lb_t x)
{
    return (((x & 0xff) << 24) | ((x & 0xff00) << 8) | 
			((x >> 8) & 0xff00) | ((x >> 24) & 0xff));
}
 
/********************************************************************
			
    Description: This function sleeps until a signal is 
		available. Signal "signo" is currently blocked and
		needs to be detected.

    Input:	signo - the signal.
		ms - timer value in milli-seconds;

    Return:	0 on success or -1 on failure.

********************************************************************/

int LB_sig_wait (int signo, int ms)
{
    sigset_t signalSet;
    struct timespec timeout;

    sigemptyset (&signalSet);
    sigaddset(&signalSet, signo);
    timeout.tv_sec = ms / 1000;
    timeout.tv_nsec = (ms % 1000) * 1000000;
#ifdef __WIN32__
    /* FIXME - no equivalent to sigtimedwait in Interix? */
    return -1;
#else
    if (sigtimedwait (&signalSet, NULL, &timeout) >= 0)
	return (0);
    else
	return (-1);
#endif
}

