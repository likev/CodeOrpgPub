/****************************************************************
		
    Module: lb_read.c	
				
    Description: This module contains the LB_read function.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:55 $
 * $Id: lb_read.c,v 1.69 2012/06/14 18:57:55 jing Exp $
 * $Revision: 1.69 $
 * $State: Exp $
 * $Log: lb_read.c,v $
 * Revision 1.69  2012/06/14 18:57:55  jing
 * Update
 *
 * Revision 1.65  2007/06/22 18:43:16  jing
 * Update
 *
 * Revision 1.51  2002/05/20 20:23:29  jing
 * Update
 *
 * Revision 1.50  2002/03/12 16:51:36  jing
 * Update
 *
 * Revision 1.49  2000/09/12 18:58:58  jing
 * @
 *
 * Revision 1.48  2000/08/21 20:49:57  jing
 * @
 *
 * Revision 1.47  2000/03/22 15:37:40  jing
 * @
 *
 * Revision 1.42  1999/06/29 21:19:33  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.40  1999/05/27 02:27:13  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.37  1999/05/03 20:57:53  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.20  1998/06/19 21:18:45  Jing
 * posix update
 *
 * Revision 1.19  1998/05/29 22:15:48  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/22 13:07:27  cm
 * SunOS 5.5 modifications
 *
*/


/* System include files */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

/* Local include files */

#include <lb.h>
#ifdef LB_NTF_SERVICE
#include <en.h>
#endif
#include <misc.h>
#include "lb_def.h"


/* Definitions / macros / types */
#define READ_A_MESSAGE_NEED_RETRY -1
				/* LB_read_a_message return value - must be 
				   negative and not equal to any LB public 
				   error return values */
#define READ_A_MESSAGE_MSG_DELETED -2
				/* LB_read_a_message return value - must be 
				   negative and not equal to any LB public 
				   error return values */

#define RET_MSG_POINTER	0x7fffffff
				/* special value used for argument buflen
				   of Lb_read_internal () */
				

/* static functions */
static int Read_msg_body (LB_struct *lb, int loc, int rlen, char *buf);
static int Lb_read_internal (int lbd, void *buf, int buflen, LB_id_t id);

static int Check_file_boundary (LB_struct *lb, int loc, int rlen);
static int Read_nrs (int lbd, void *user_buf, int buflen);
static int Lb_read_multiple (int lbd, void *user_buf, int buflen, 
					int n_msgs, int full, int comp);
static int Read_entire_lb (LB_struct *lb, void *buf, int buflen);
static void Update_must_read_common_pointer (LB_struct *lb);

/********************************************************************
			
    Description: This function reads a message and copies it to "buf".

    Input:	lbd - the LB descriptor;
		buflen - size of the buffer for the message;
		id - the message id to be read;

    Output:	buf - the message read

    Returns:	This function returns the length of the message read
		on success or a non-positive error number. 

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_read (int lbd, void *buf, int buflen, LB_id_t id) 
{

    if (buflen == RET_MSG_POINTER)
	return (LB_BAD_ARGUMENT);

#ifdef LB_NTF_SERVICE
    if (buflen == LB_ALLOC_BUF) {
	if (EN_control (EN_GET_IN_CALLBACK))
	    return (LB_NOT_SUPPORTED);
    }
#endif

    if (id <= LB_MAX_ID || id == LB_NEXT || id == LB_LATEST || id == LB_SHARED_READ || id == LB_READ_ENTIRE_LB)
	return (Lb_read_internal (lbd, (void *)buf, buflen, id));

    if (id == LB_GET_NRS)
	return (Read_nrs (lbd, buf, buflen));

    {
	int multi, full;
        multi = (id & LB_MULTI_READ) == LB_MULTI_READ;
        full = !multi && ((id & LB_MULTI_READ_FULL) == LB_MULTI_READ_FULL);
        if (multi || full)
	    return (Lb_read_multiple (lbd, buf, buflen,
              id & LB_MULTI_READ_NUM, full, (id >> 14) & 0x3));
    }

    return (LB_BAD_ARGUMENT);
}
 
/********************************************************************
			
    Description: This function sets a LB_read window.

    Input:	lbd - LB descriptor;
		offset - window offset;
		size - window size;

    Returns:	returns LB_SUCCESS on success or a negative 
		number to indicate an error condition.

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_read_window (int lbd, int offset, int size)
{
    LB_struct *lb;
    int ret;

    if (offset < 0 || size < 0)
	return (LB_BAD_ARGUMENT);

    /* get the LB structure */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);

    lb->read_offset = offset;
    lb->read_size = size;
    LB_reset_inuse (lb);
    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function returns the pointer in the LB to a 
		message.

    Input:	lbd - the LB descriptor;
		id - the message id to be returned;

    Output:	ptr - the pointer to the message;

    Returns:	This function returns the length of the message
		on success or a non-positive error number. 

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_direct (int lbd, char **ptr, LB_id_t id) 
{

    return (Lb_read_internal (lbd, (void *)ptr, RET_MSG_POINTER, id));
}
 
 
/********************************************************************
			
    Description: This function sets the polling parameters for reading 
		messages that are to come.

    Input:	lbd - the LB descriptor.
		max_poll - maximum number of seconds to poll before 
			   giving up; Value 0 disable the polling.
		wait_time - waiting time (in ms) between each poll;

    Returns:	The function returns 0 on success or an LB negative 
		error number on failure. 

********************************************************************/

int LB_set_poll (int lbd, int max_poll, int wait_time)
{
    LB_struct *lb;
    int err;

    lb = LB_Get_lb_structure (lbd, &err); /* get lb structure */
    if (lb == NULL)
	return (err);
    if (max_poll < 0 || wait_time < 0) {
	LB_reset_inuse (lb);
	return (LB_BAD_ARGUMENT);
    }
    if (wait_time < 20)			/* we set the minimum to 20 ms */
	wait_time = 20;
    lb->max_poll = max_poll;
    lb->wait_time = wait_time;

    LB_reset_inuse (lb);
    return (LB_SUCCESS);
}
 
/********************************************************************
			
    Description: This function returns the id of the message previously
		read/written.

    Input:	lbd - the LB descriptor.

    Returns:	The id of the message previously LB_read. 

********************************************************************/

LB_id_t LB_previous_msgid (int lbd)
{
    LB_struct *lb;
    int err;

    lb = LB_Get_lb_structure (lbd, &err); /* get lb structure */
    if (lb == NULL)
	return (LB_PREV_MSGID_FAILED);

    LB_reset_inuse (lb);
    return (lb->prev_id);
}

/********************************************************************
			
    Description: This is the basic function for reading a message.
		It copies the message to "buf" if 
		buflen != RET_MSG_POINTER. Otherwise it returns the 
		pointer to the message. We must use a very large 
		number for RET_MSG_POINTER to bypass buffer size 
		checks while specifying pointer return.

    Input:	lbd - the LB descriptor;
		buflen - size of the buffer for the message; if
		RET_MSG_POINTER, returns the pointer to the message.
		id - the message id to be read;

    Output:	buf - returns the message read or the pointer to the
		message.

    Returns:	This function returns the length of the message read
		on success or a non-positive error number. 

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

static int Lb_read_internal (int lbd, void *buf, int buflen, LB_id_t id) 
{
    LB_struct *lb;
    int ret;
    int rpt, rpage;
    int i;

    if (buflen < 0 || buf == NULL)	/* check arguments */
	return (LB_BAD_ARGUMENT);

    /* get the LB structure */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);

    if ((lb->flags & LB_DIRECT) && (lb->flags & LB_UNPROTECTED))
	ret = LB_lock_mmap (lb, WRITE_PERM, NO_LOCK);
    else if (((lb->flags & LB_MUST_READ) && !(lb->flags & LB_SHARE_STREAM)) ||
		id == LB_SHARED_READ || id == LB_READ_ENTIRE_LB)
	ret = LB_lock_mmap (lb, WRITE_PERM, EXC_LOCK);
    else
	ret = LB_lock_mmap (lb, READ_PERM, NO_LOCK);
    if (ret < 0)
	return (LB_Unlock_return (lb, ret));

    if (buflen == RET_MSG_POINTER && !(lb->flags & LB_DIRECT))
	return (LB_Unlock_return (lb, LB_BAD_ACCESS));

    if (id == LB_READ_ENTIRE_LB)
	return (LB_Unlock_return (lb, Read_entire_lb (lb, buf, buflen)));

    if (lb->hd->non_dec_id > 1) {	/* must_read with milti-readers */
	if (id != LB_NEXT)
	    return (LB_Unlock_return (lb, LB_BAD_ARGUMENT));
    }

    /* get the read pointer */
    if (id == LB_NEXT) {
	rpt = lb->rpt;
	rpage = lb->rpage;
    }
    else if (id == LB_SHARED_READ) {
	if (!(lb->flags & LB_SHARE_STREAM))
	    return (LB_Unlock_return (lb, LB_BAD_ARGUMENT));
	rpt = lb->hd->ptr_read;
	rpage = lb->rpage;
    }
    else if (LB_Search_msg (lb, id, &rpt, &rpage) < 0)
	return (LB_Unlock_return (lb, LB_NOT_FOUND));

    /* need to retry for replaceable LB; We retry a sufficiently large number 
       (500) of times before giving up */
    for (i = 0; i < 500; i++) {
	ret = LB_read_a_message (lb, buf, buflen, rpt, rpage);
	if (ret == READ_A_MESSAGE_MSG_DELETED) {
	    if (id == LB_NEXT) 
		ret = 0;
	    else
		ret = LB_EXPIRED;
	}
	if (ret != READ_A_MESSAGE_NEED_RETRY)
	    break;
    }

    if (i >= 500)
	return (LB_Unlock_return (lb, LB_LB_ERROR));

    if (lb->hd->non_dec_id > 1 && ret == LB_TO_COME)
	Update_must_read_common_pointer (lb);

    /* increment read pointer, page and the PTR_READ field in LB header */
    if (ret >= 0 || ret == LB_BUF_TOO_SMALL || ret == LB_EXPIRED) {
	lb->rpt = (rpt + 1) % lb->ptr_range;
	lb->rpage = LB_Get_corrected_page_number (lb, lb->rpt, rpage);
	if ((lb->flags & LB_SHARE_STREAM) && id == LB_SHARED_READ)
	    lb->hd->ptr_read = (rpt + 1) % lb->ptr_range;
	else if ((lb->flags & LB_MUST_READ) &&
				 !(lb->flags & LB_SHARE_STREAM)) {
	    if (lb->hd->non_dec_id > 1)
		Update_must_read_common_pointer (lb);
	    else {			/* old must_read function */
		if (ret == LB_EXPIRED)
		    LB_search_for_message (lb, LB_FIRST, -1, &rpt, &rpage);
		if (LB_Ptr_compare (lb, rpt, lb->hd->ptr_read) >= 0)
		    lb->hd->ptr_read = (rpt + 1) % lb->ptr_range;
	    }
	}
    }

    return (LB_Unlock_return (lb, ret));
}
	
/********************************************************************
			
    Adjusts the common read pointer and read_cnt for the multi-reader
    must_read reader.

********************************************************************/

static void Update_must_read_common_pointer (LB_struct *lb) {

    if (lb->rpt_incd != lb->hd->ptr_read && 
	LB_Ptr_compare (lb, lb->rpt, 
			lb->hd->ptr_read + lb->maxn_msgs / 2) >= 0) {
	lb->hd->read_cnt++;
	lb->rpt_incd = lb->hd->ptr_read;
    }
    if (lb->hd->read_cnt >= (lb->hd->non_dec_id >> 1)) {
	int n_page;
	lb->hd->ptr_read = LB_Compute_pointer (lb, lb->hd->ptr_read, 
			lb->rpage, lb->maxn_msgs / 2, &n_page);
	lb->hd->read_cnt = 0;    
    }
}

/********************************************************************
			
    Description: This function reads a message pointed to by "rpt" of
		page "rpage". If buflen == RET_MSG_POINTER, the message
		pointer is returned instead of copying the message.

    Input:	lb - the LB structure;
		buflen - size of the buffer for the message;
		rpt - the message pointer to be read;
		rpage - pointer page;

    Output:	buf - the message read or the message pointer.

    Returns:	This function returns the length of the message read
		on success, READ_A_MESSAGE_NEED_RETRY if ucnt is changed 
		or a non-positive LB_read error number. It returns 
		READ_A_MESSAGE_MSG_DELETED if the message is deleted.

********************************************************************/

int LB_read_a_message (LB_struct *lb, void *buf, int buflen, 
			int rpt, int rpage) 
{
    int ucnt, ind, ma_sz;
    int loc, ret;
    int num_ptr;
    int poll;
    int msg_len, deleted, st_time;
    char *alloc_buf;

    /* check read pointer */
    num_ptr = lb->hd->num_pt;	/* for skipping LB_Check_pointer 
						   calls later */

    poll = 0;			/* poll count */
    st_time = 0;		/* not needed - turn off gcc warning */
    while (1) {			/* if the message is to come, we poll */

	ret = LB_Check_pointer (lb, rpt, rpage);
	if (ret == RP_EXPIRED)
	    return (LB_EXPIRED);
	else if (ret == RP_TO_COME) {
	    int poll_period;

	    if (lb->max_poll <= 0)
		return (LB_TO_COME);
	    poll_period = 1000 / lb->wait_time + 1;
			/* time check is needed every poll_period polls */
	    if (poll == 0) 
		st_time = MISC_systime (NULL);
	    else if ((poll % poll_period) == poll_period - 1 &&
		     MISC_systime (NULL) - st_time > lb->max_poll)
		return (LB_TO_COME);
	    msleep (lb->wait_time);
	    poll++;
	}
	else
	    break;
    }

    /* get message info. We need to make sure loc/len/ucnt are consistent and
       not expired */
    deleted = 0;
    ind = rpt % lb->n_slots;
    if (lb->flags & LB_DB) {
	do {
	    LB_msg_info_t *info;
	    ucnt = lb->dir[ind].loc;
	    info = ((LB_msg_info_t *)lb->msginfo) + DB_WORD_OFFSET (ind, ucnt);
	    loc = info->loc;
	    msg_len = info->len;
	    lb->prev_id = lb->dir[ind].id;
	} while (ucnt != lb->dir[ind].loc);
	if (msg_len < 0) {
	    msg_len = 0;
	    deleted = 1;
	}
    }
    else {
	msg_len = LB_Get_message_info (lb, rpt, &loc, &(lb->prev_id));
	ucnt = 0;
    }

    if (lb->umid != NULL)
	*lb->umid = lb->prev_id;

    /* check again - this is needed to make sure that loc and msg_len are not
       expired */
    if (lb->hd->num_pt != num_ptr && 
	LB_Check_pointer (lb, rpt, rpage) != RP_OK)
	return (LB_EXPIRED);

    /* read the tag */
    if (lb->utag != NULL)
	*(lb->utag) = LB_read_tag (lb, ind);

    /* read the message */
    ma_sz = lb->ma_size;
    if (ma_sz == 0)		/* disable some of the following checks */
	ma_sz = 0x7fffffff;
    if (loc < 0 || msg_len < 0 || 
			msg_len > ma_sz || loc >= ma_sz)
	return (LB_LB_ERROR);
    alloc_buf = NULL;
    if (buflen == RET_MSG_POINTER) {
	char **pt;
	int perm;

	if (msg_len + loc > ma_sz)
	    return (LB_LB_ERROR);
	perm = READ_PERM;
	if (lb->flags & LB_WRITE)
	    perm = WRITE_PERM;
	ret = LB_direct_access_lock (lb, SET_LOCK, perm);
	if (ret < 0)
	    return (ret);
	pt = (char **)buf;
	if (lb->flags & LB_MEMORY)
	    *pt = lb->pld->lb_pt + loc + lb->off_a;
	else
	    *pt = lb->pld->map_pt + loc;
    }
    else {
	int rlen;
	char *rbuf;

	if (lb->read_size > 0 || lb->read_offset > 0) {
	    loc = (loc + lb->read_offset) % ma_sz;
	    msg_len -= lb->read_offset;
	    if (msg_len < 0)
		msg_len = 0;
	    if (lb->read_size > 0 && msg_len > lb->read_size)
		msg_len = lb->read_size;
	}
	if (buflen != LB_ALLOC_BUF && msg_len > buflen)
	    rlen = buflen;
	else 
	    rlen = msg_len;
	if (buflen == LB_ALLOC_BUF) {
	    alloc_buf = (char *)malloc (rlen);
	    if (alloc_buf == NULL)
		return (LB_MALLOC_FAILED);
	    rbuf = alloc_buf;
	}
	else
	    rbuf = (char *)buf;

	ret = Read_msg_body (lb, loc, rlen, rbuf);
	if (ret < 0) {
	    if (alloc_buf != NULL)
		free (alloc_buf);
	    return (ret);
	}
    }

    /* check expiration again */
    if (lb->hd->num_pt != num_ptr && 
	LB_Check_pointer (lb, rpt, rpage) != RP_OK) {
	if (alloc_buf != NULL)
	    free (alloc_buf);
	return (LB_EXPIRED);
    }

    /* record num_pt and ucnt for LB update check */
    LB_stat_update (lb, num_ptr, ucnt, ind);

    /* check ucnt */
    if (lb->flags & LB_DB) {
	if (ucnt != lb->dir[ind].loc) {
	    if (alloc_buf != NULL)
		free (alloc_buf);
	    return (READ_A_MESSAGE_NEED_RETRY);
	}
    }

    if (buflen == LB_ALLOC_BUF)
	*((char **)buf) = alloc_buf;
    if (deleted)
	return (READ_A_MESSAGE_MSG_DELETED);
    else if (buflen != LB_ALLOC_BUF && msg_len > buflen)
	return (LB_BUF_TOO_SMALL);
    else
	return (msg_len);
}

/********************************************************************
			
    Description: This function reads "rlen" bytes starting from offset
		"loc" in the message area of LB "lb" and puts them in
		"buf". 

    Input:	lb - the LB structure;
		loc - the message offset in the message area;
		rlen - number of bytes to read;

    Output:	buf - the message read;

    Returns:	This function returns the length read or a negative
		LB error number.

********************************************************************/

static int Read_msg_body (LB_struct *lb, int loc, int rlen, char *buf)
{
    int nread;
    int size, ret;

    if (rlen <= 0)
	return (0);

    size = lb->ma_size;		/* message area size */
    if (size == 0 &&
	(ret = Check_file_boundary (lb, loc, rlen)) < 0)
	return (ret);

    /* read the message */
    nread = 0;				/* bytes read */
    while (nread < rlen) {
	int len, ret;

	len = rlen - nread;
	if (size > 0 && len > size - loc)
	    len = size - loc;

	if (lb->flags & LB_MEMORY) {
	    memcpy ((char *)buf + nread, lb->pld->lb_pt + loc + lb->off_a, len);
	}
	else {
	    ret = LB_data_transfer (READ_TRANS, lb, (char *)buf + nread, 
						len, loc + lb->off_a);
	    if (ret < 0)
		return (ret);
	}

	nread += len;
	loc = loc + len;
	if (size > 0)
	    loc = loc % size;
    }

    return (nread);
}

/********************************************************************
			
    Description: This function checks if the message to read is
		within the existing file boundary to avoid core dump
		in mmaped memory access.

    Input:	lb - the LB structure;
		loc - read offset in message area;
		rlen - read length;

    Returns:	0 on success or an LB error number.

********************************************************************/

static int Check_file_boundary (LB_struct *lb, int loc, int rlen)
{
    int fsize;

    if ((fsize = lseek (lb->pld->fd, 0, SEEK_END)) < 0)
	return (LB_SEEK_FAILED);

    if (loc + rlen + lb->off_a > fsize)
	return (LB_LB_ERROR);
    else
	return (0);
}

/********************************************************************
			
    Description: This is an LB service function. It reads all active 
		notification request records, converts them into an 
		ASCII message and returns it.

    Input:	lbd - the LB file descriptor;
		buflen - size of the buffer for the message;

    Output:	user_buf - pointer to the return message.

    Returns:	This function returns the length of the message 
		on success or an LB error number on failure. 

********************************************************************/

static int Read_nrs (int lbd, void *user_buf, int buflen)
{
    LB_struct *lb;
    LB_nr_t *nr_recs;
    char *buf, *cpt;
    int err, n_nrs, buf_size;
    int max_msg_size;
    int len, i;

    lb = LB_Get_lb_structure (lbd, &err); /* get lb structure */
    if (lb == NULL)
	return (err);
    if ((err = LB_lock_mmap (lb, WRITE_PERM, EXC_LOCK)) < 0)
	return (LB_Unlock_return (lb, err));

    n_nrs = LB_read_nrs (lb, &nr_recs);
    if (n_nrs <= 0)
	return (LB_Unlock_return (lb, n_nrs));

    max_msg_size = 72;		/* maximum length of each NR record text msg */
    if (buflen == LB_ALLOC_BUF) {
	buf_size = max_msg_size * n_nrs;
	buf = (char *)malloc (buf_size);
	if (buf == NULL) {
	    free (nr_recs);
	    return (LB_Unlock_return (lb, LB_MALLOC_FAILED));
	}
	cpt = buf;
    }
    else {
	cpt = (char *)user_buf;
	buf_size = buflen;
	buf = NULL;
    }

    len = 0;
    for (i = 0; i < n_nrs; i++) {
	LB_nr_t *r;

	r = nr_recs + i;
	sprintf (cpt + len, "host %x, fd %d, msgid %d, pid %d\n", 
			LB_T_BSWAP (r->host), LB_T_BSWAP (r->fd), 
			LB_T_BSWAP (r->msgid), LB_SHORT_BSWAP (r->pid));
	len += strlen (cpt + len) + 1;
	if (len + max_msg_size > buf_size)
	    break;
    }
    free (nr_recs);
    if (cpt == buf) 
	*((char **)user_buf) = (char *)buf;
    return (LB_Unlock_return (lb, len));
}

/********************************************************************
			
    Reads the next "n_msgs" messages started from the current read
    pointer in LB "lbd". The output is put in user_buf of size buflen.
    If "full" is true, buf begins with msg count and length and offset
    of all messages. If "comp" >= 0, the data in buf is compressed.

********************************************************************/

static int Lb_read_multiple (int lbd, void *user_buf, int buflen, 
					int n_msgs, int full, int comp)
{
    int len, i, hd_s, ret, cnt, max_poll;
    char *buf;
    LB_struct *lb;

    cnt = 0;
    buf = NULL;
    hd_s = 0;
    if (comp)
	hd_s = 1;
    len = hd_s * sizeof (int);
    if (full) {
	hd_s++;
        len = (n_msgs * 2 + hd_s) * sizeof (int);
    }

    /* reserve space for the compression header, count, lengths and offsets */
    if (len > 0) {
        if (buflen == LB_ALLOC_BUF) {
            buf = malloc (len);
            if (buf == NULL)
                return (LB_MALLOC_FAILED);
            memset (buf, 0, len);
        }
        else {
            if (buflen >= len)
                memset (user_buf, 0, len);
            else {
                memset (user_buf, 0, buflen); /* we read 0 messages... */
                return (LB_BUF_TOO_SMALL);
            }
        }
    }

    max_poll = 0;
    lb = LB_Get_lb_structure (lbd, &ret);
    LB_reset_inuse (lb);
    for (i = 0; i < n_msgs; i++) {
	int *iptr;

	if (i == 1 && lb != NULL && lb->max_poll > 0) {
	    max_poll = lb->max_poll;
	    lb->max_poll = 0;
	}

	if (buflen == LB_ALLOC_BUF) {
	    char *ret_buf;
	    ret = Lb_read_internal (lbd, &ret_buf, buflen, LB_NEXT);
	    if (ret > 0) {
		buf = (char *)realloc (buf, len + ALIGNED_SIZE (ret));
		if (buf == NULL) {
		    len = LB_MALLOC_FAILED;
		    break;
		}
		memcpy (buf + len, ret_buf, ret);
		free (ret_buf);
	    }
	}
	else {
	    if (len >= buflen)
		ret = LB_BUF_TOO_SMALL;
	    else {
		ret = Lb_read_internal (lbd, 
			(char *)user_buf + len, buflen - len, LB_NEXT);
		if (ret == LB_BUF_TOO_SMALL)
		    LB_seek (lbd, -1, LB_CURRENT, NULL);
	    }
	}

	iptr = (int *)(buflen == LB_ALLOC_BUF ? buf : user_buf);
	if (ret >= 0) {
	    cnt++;
	    if (full) {
		iptr[hd_s - 1] = INT_BSWAP_L (cnt);
		iptr += (hd_s + 2 * (cnt - 1));
		*iptr++ = INT_BSWAP_L (ret);	/* store the length */
		if (comp)
		    *iptr = INT_BSWAP_L (len - sizeof (int));
		else
		    *iptr = INT_BSWAP_L (len); /* store the offset */
	    }
	    len += ALIGNED_SIZE (ret);
	}
	else if (ret == LB_TO_COME) {
	    if (full)
		iptr[hd_s + 2 * cnt] = 0;
	    break;
	}
	else if (ret == LB_BUF_TOO_SMALL) {
	    if (full)
		iptr[hd_s + 2 * cnt] = INT_BSWAP_L (1);
	    break;
	}
	else {
	    len = ret;
	    break;
	}
    }
    if (max_poll > 0)
	lb->max_poll = max_poll;

    if (len > 0 && comp) {
	char *dest, *src;
	int l = len - sizeof (int);
	src = (buflen == LB_ALLOC_BUF ? buf : user_buf);
	if (len <= 256)
	    *((int *)src) = 0;
	else if ((dest = malloc (l)) == NULL)
	    len = LB_MALLOC_FAILED;
	else {
	    ret = MISC_compress (comp - 1, src + sizeof (int), l, dest, l);
	    if (ret > 0) {
		*((int *)src) = INT_BSWAP_L (l);
		memcpy (src + sizeof (int), dest, ret);
		len = ret + sizeof (int);
	    }
	    else if (ret == MISC_BUF_TOO_SMALL)
		*((int *)src) = 0;
	    else
		len = ret;
	    free (dest);
	}
    }

    if (len <= 0) {
	if (buf != NULL)
	    free (buf);
    }
    else if (buflen == LB_ALLOC_BUF)
	*((char **)user_buf) = buf;

    return (len);
}

/********************************************************************
			
    Returns the entire LB with "bufp" of size "buflen". If "buflen" 
    = LB_ALLOC_BUF, the buffer is allocated. Returns the number of 
    bytes in "bufp" on success or a negative error code.

********************************************************************/

static int Read_entire_lb (LB_struct *lb, void *bufp, int buflen) {
    int s;
    char *buf;

    if ((lb->flags & LB_DB) || lb->ma_size == 0)
	s = lb->off_a + LB_sms_space_used (lb);
    else
	s = lb->off_a + lb->ma_size;
    if (s <= 0)
	return (s);

    if (buflen == LB_ALLOC_BUF)
	buf = MISC_malloc (s);
    else {
	if (s > buflen)
	    return (LB_BUF_TOO_SMALL);
	buf = (char *)bufp;
    }

    if (lb->flags & LB_MEMORY)
	memcpy (buf, (char *)lb->pld->lb_pt, s);
    else {
	int fd, cr_off, ret;
	fd = lb->pld->fd;
	if ((cr_off = lseek (fd, 0, SEEK_CUR)) < 0 ||
	    lseek (fd, 0, SEEK_SET) < 0) {
	    MISC_log ("lseek failed in Read_entire_lb\n");
	    if (buflen == LB_ALLOC_BUF)
		free (buf);
	    return (LB_SEEK_FAILED);
	}
	if ((ret = MISC_read (fd, buf, s)) != s) {
	    MISC_log ("read failed in Read_entire_lb (%d, ret %d)\n", s, ret);
	    if (buflen == LB_ALLOC_BUF)
		free (buf);
	    return (LB_LB_ERROR);
	}
	lseek (fd, cr_off, SEEK_SET);
    }
    if (buflen == LB_ALLOC_BUF)
	*(char **)bufp = buf;
    return (s);
}
