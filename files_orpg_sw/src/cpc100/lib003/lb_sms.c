/****************************************************************
		
    Module: lb_sms.c	
				
    Description: This module contains the storage space management 
		service functions.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:55 $
 * $Id: lb_sms.c,v 1.36 2012/06/14 18:57:55 jing Exp $
 * $Revision: 1.36 $
 * $State: Exp $
 * $Log: lb_sms.c,v $
 * Revision 1.36  2012/06/14 18:57:55  jing
 * Update
 *
 * Revision 1.27  2002/03/12 16:51:41  jing
 * Update
 *
 * Revision 1.24  2000/08/21 20:50:05  jing
 * 
 *
 * Revision 1.23  2000/03/13 15:28:54  jing
 * @
 *
 * Revision 1.18  1999/06/30 13:56:16  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.15  1999/05/27 02:27:26  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.12  1999/05/04 20:38:22  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.1  1999/01/13 15:09:42  jing
 * Initial revision
 *
 *
*/

#include <config.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

/* Local include files */

#include <lb.h>
#include <misc.h>
#include <en.h>
#include "lb_def.h"


typedef struct {		/* header structure of the LB SMS area */
    lb_t total_space;		/* total space currently used */
    lb_t big_endian;		/* byte order; non-zero for BIG_ENDIAN, zero 
				   for LITTLE_ENDIAN (we define in this way
				   such that no swap is needed) */
    lb_t space_used;		/* space used for messages */
    lb_t msg_cnt;		/* message count */
    lb_t free_seg_cnt;		/* free segment count */
    lb_t reserved;
    lb_t reserved1;
    lb_t reserved2;
} Sms_header_t;

typedef struct {		/* the free segment struct */
    int loc;			/* free segment location (offset) */
    int len;			/* length of the segment */
} Sms_free_seg_t;		/* Total of maxn_msgs + 1 free segments may
				   exist. */

typedef struct {		/* temporary table used by Sm_generate_table 
				   for holding all message in the LB. */
    int loc;			/* message location */
    int len;			/* message length */
} Msg_table_t;

enum {LOC_KEY, LEN_KEY};	/* search key indices */

static void *Tmp_space_p = NULL;
static int Tmp_space_size = 0;

static int Sm_generate_table (LB_struct *lb);
static void Sort (int n, Msg_table_t *ra);
static int Right_endian (LB_struct *lb);
static int Check_table (LB_struct *lb);
static int Get_sequential_free_space (LB_struct *lb, int length, int *loc);
static int Get_random_free_space (LB_struct *lb, int len, int *is_new);
static int Compare (int which_key, void *r1, void *r2);
static int Sm_gen_table_ret (int in_cb, char *msgs, int ret);
static int Get_sorted_msg_list (LB_struct *lb, 
			Msg_table_t **msgs_pt, int *in_cb_pt, int *sused_pt);


/********************************************************************
			
    Description: This function returns the size of the total message
		area space used by the LB.

    Input:	lb - the LB structure.

********************************************************************/

int LB_sms_space_used (LB_struct *lb)
{
    Sms_header_t *smshd;
    int ret;

    if ((ret = Check_table (lb)) < 0)
	return (ret);

    smshd = (Sms_header_t *)(lb->pld->lb_pt + lb->off_sms);
    return (smshd->total_space);
}

/********************************************************************
			
    Description: This function returns the SMS control area size. 

    Input:	n_msgs - number of message of the LB.
		version - LB version number.

********************************************************************/

int LB_sms_cntl_size (int n_msgs, int version)
{
    int n;

    n = n_msgs + 1;
    if (version == 5)	/* version 5 has a big in RSIS_size - for backward 
			   compatibily */
	n = -n;
    return (RSIS_size (n, 2, sizeof (Sms_free_seg_t)) + 
					sizeof (Sms_header_t));
}

/********************************************************************
			
    Description: This function frees a segment. We don't actually 
		reduce the total space, when the segments at the end
		of the file is freed, and the file size for the moment.
		An attemp to free a piece of free or out-of-boundary 
		space is not allowed.

    Input:	lb - the LB struct.
		loc - location of the space to free.
		len - length of the segment to free.

    Return:	LB_SUCCESS or LB_FAILURE.

********************************************************************/

int LB_sms_free_space (LB_struct *lb, int loc, int len)
{
    Sms_free_seg_t seg, *rec, *left, *right;
    int left_ind, right_ind, err, ind, fcnt, new_ind, updated;
    Sms_header_t *smshd;
    char *rsid;

    if (loc < 0 || len <= 0)
	return (LB_LB_ERROR);

    if ((updated = Check_table (lb)) < 0)
	return (updated);
    if (updated) /* if FST is updated, we don't need to and cannot free */
	return (LB_SUCCESS);

    /* find neighboring free segments */
    seg.loc = loc;
    rsid = lb->pld->rsid;
    ind = RSIS_find (rsid, LOC_KEY, &seg, &rec);
    left_ind = right_ind = -1;
    err = 0;
    if (ind >= 0) {
	if (rec->loc <= loc) {
	    left_ind = ind;
	    left = rec;
	    right_ind = RSIS_traverse (rsid, LOC_KEY, RSIS_RIGHT, ind, &right);
	}
	else {
	    right_ind = ind;
	    right = rec;
	    left_ind = RSIS_traverse (rsid, LOC_KEY, RSIS_LEFT, ind, &left);
	}
    }
    if ((left_ind >= 0 && left->loc >= loc) ||
	(right_ind >= 0 && right->loc <= loc))		/* unexpected loc */
	err = 1;

    smshd = (Sms_header_t *)(lb->pld->lb_pt + lb->off_sms);
    if (loc + len > smshd->total_space)
	err = 1;

    if (!err) {		/* check relations with neighboring segments */
	int end;

	if (left_ind >= 0) {				/* check left touch */
	    end = left->loc + left->len;
	    if (end < loc)				/* untouched */
		left_ind = -1;
	    else if (end > loc)				/* overlapped */
		err = 1; 
	}
	if (right_ind >= 0) {				/* check right touch */
	    end = loc + len;
	    if (end < right->loc)			/* untouched */
		right_ind = -1;
	    else if (end > right->loc)			/* overlapped */
		err = 1; 
	}
    }

    fcnt = 0;
    new_ind = -1;
    lb->hd->sms_ok = 0;
    if (!err) {		/* update the index tables */
	int new_loc, new_len;

	new_loc = loc;	/* new segment */
	new_len = len;
	if (left_ind >= 0 && right_ind < 0) {
	    new_loc = left->loc;
	    new_len = left->len + len;
	    RSIS_delete (rsid, left_ind);
	    fcnt--;
	}
	else if (right_ind >= 0 && left_ind < 0) {
	    new_loc = loc;
	    new_len = right->len + len;
	    RSIS_delete (rsid, right_ind);
	    fcnt--;
	}
	else if (right_ind >= 0 && left_ind >= 0) {
	    new_loc = left->loc;
	    new_len = left->len + right->len + len;
	    RSIS_delete (rsid, left_ind);
	    RSIS_delete (rsid, right_ind);
	    fcnt -= 2;
	}
	seg.loc = new_loc;
	seg.len = new_len;
	new_ind = RSIS_insert (rsid, &seg);
	fcnt++;
    }

    if (err) {
	MISC_log ("LB_sms_free_space error number %d\n", err);
	return (LB_LB_ERROR);
    }

    smshd->space_used -= len;
    smshd->msg_cnt--;
    smshd->free_seg_cnt += fcnt;
    lb->hd->sms_ok = 1; 
    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function returns a free space segment of size
		"len". If there is no free space it extends the total
		space. If the free space table is changed, we set
		sms_ok to 0 and expect LB_write to set it to 1
		after it succeds. If the requested size if zero, we
		always return 0.

    Input:	lb - the LB struct.
		len - length of the segment.

    Output:	new - flag indicating the space has not been initialized.

    Return:	The location of the space on success or a negative 
		LB error number.

********************************************************************/

static int Get_random_free_space (LB_struct *lb, int len, int *is_new)
{
    Sms_free_seg_t seg, *rec;
    int loc, ret, ind, fcnt;
    Sms_header_t *smshd;
    char *rsid;

    if (len < 0)
	return (LB_LB_ERROR);
    if (len == 0) {
	*is_new = 0;
	return (0);
    }

    if ((ret = Check_table (lb)) < 0)
	return (ret);

    /* find a segment */
    seg.len = len;
    rsid = lb->pld->rsid;
    ind = RSIS_find (rsid, LEN_KEY, &seg, &rec);
    while (ind >= 0 && rec->len < len)
	ind = RSIS_right (rsid, &rec);

    smshd = (Sms_header_t *)(lb->pld->lb_pt + lb->off_sms);
    fcnt = 0;
    if (ind >= 0) {			/* found */
	lb->hd->sms_ok = 0;
	loc = rec->loc;
	RSIS_delete (rsid, ind);
	fcnt--;
	if (rec->len > len) {
	    seg.loc = rec->loc + len;
	    seg.len = rec->len - len;
	    RSIS_insert (rsid, &seg);
	    fcnt++;
	}
	*is_new = 0;
    }
    else {				/* extend the total space */
	if (lb->ma_size == 0 ||
	    smshd->total_space + len <= lb->ma_size) {
	    if (lb->off_a + smshd->total_space + len > MAX_LB_SIZE)
		return (LB_MSG_TOO_LARGE);
	    loc = smshd->total_space;
	    smshd->total_space += len;
	    *is_new = 1;
	}
	else
	    return (LB_FULL);
    }
    smshd->msg_cnt++;
    smshd->free_seg_cnt += fcnt;
    smshd->space_used += len;
    return (loc);
}

/********************************************************************
			
    Description: This function checks the SMS table and updates it
		if necessary. It also sets the pointers to the
		SMS header and the free segment table.

    Input:	lb - the LB structure.

    Return:	1 if FST is updated, 0 if not or a negative LB 
		error number.

********************************************************************/

static int Check_table (LB_struct *lb)
{
    int ret, updated;
    Per_lb_data_t *pld;
    Sms_header_t *smshd;

    updated = 0;
    pld = lb->pld;
    if (!(lb->hd->sms_ok) || !Right_endian (lb)) {
	if ((ret = Sm_generate_table (lb)) < 0)
	    return (ret);
	updated = 1;
    }

    smshd = (Sms_header_t *)(pld->lb_pt + lb->off_sms);
    if (pld->rsid == NULL) {		/* lb_pt changed */
	pld->rsid = RSIS_localize 
		((char *)smshd + sizeof (Sms_header_t), pld->rsid_buf, Compare);
	if (pld->rsid == NULL)
	    return (LB_RSIS_FAILED);
    }
    else if (RSIS_is_corrupted (pld->rsid)) /* rsid damaged by memory leak */
	return (LB_RSIS_CHECK_SUM_ERROR);

    return (updated);
}

/********************************************************************
			
    Validate the LB header area. Currently we validate the SMS tables
    against the message dir.

    Input:	lb - the LB structure.

    Return:	LB_SUCCESS or an LB error number.

********************************************************************/

int LB_sms_validate_header (LB_struct *lb) {
    Sms_header_t *smshd;
    Per_lb_data_t *pld;
    Msg_table_t *msgs;
    int cnt, i, sused, off, ns, masize, in_cb;
    Sms_free_seg_t seg, *rec;

    if (!(lb->flags & LB_DB) && lb->ma_size > 0)
	return (LB_SUCCESS);
    if (lb->hd->sms_ok == 0)
	return (LB_SUCCESS);
    if ((i = Check_table (lb)) < 0)
	return (i);

    if ((cnt = Get_sorted_msg_list (lb, &msgs, &in_cb, &sused)) < 0)
	return (cnt);

    pld = lb->pld;
    ns = 0;
    off = 0;			/* current starting offset */
    for (i = 0; i < cnt; i++) {
	Msg_table_t *curt;

	curt = msgs + i;
	if (curt->loc > off) {
	    seg.loc = off;
	    if (RSIS_find (pld->rsid, LOC_KEY, &seg, &rec) < 0 ||
		    		rec->len != curt->loc - off)
		return (Sm_gen_table_ret (in_cb, 
				    	(char *)msgs, LB_LB_ERROR));
/*
if (RSIS_find (pld->rsid, LOC_KEY, &seg, &rec) >= 0)
*/
/* printf ("   %d  %d  %d\n", seg.loc, rec->len, curt->loc - off); */

	    ns++;
	}
	else if (curt->loc < off)
	    return (Sm_gen_table_ret (in_cb, (char *)msgs, LB_LB_ERROR));
	off = curt->loc + curt->len;
    }
    Sm_gen_table_ret (in_cb, (char *)msgs, 0);

    for (i = 0; i < ns; i++) {
	int ind, prev_len;

	ind = prev_len = 0;	/* not necessary - turn off gcc warning */
	if (i == 0) {
	    seg.len = 0;
	    if ((ind = RSIS_find (pld->rsid, LEN_KEY, &seg, &rec)) < 0)
		return (LB_LB_ERROR);
	    prev_len = rec->len;
	}
	else {
	    ind = RSIS_traverse (pld->rsid, 
				    LEN_KEY, RSIS_RIGHT, ind, &rec);
	    if (ind < 0 || rec->len < prev_len)
		return (LB_LB_ERROR);
	    prev_len = rec->len;
	}
/* printf ("prev_len  %d\n", prev_len); */
    }

    smshd = (Sms_header_t *)(pld->lb_pt + lb->off_sms);
    masize = lseek (pld->fd, 0, SEEK_END) - lb->off_a;	/* msg area size */
    if (masize < off)
	return (LB_LENGTH_ERROR);
    if (masize > off)
	ns++;
/*
printf ("  %d %d    %d %d    %d %d    %d %d    %d %d\n", masize, off, smshd->total_space , masize, smshd->space_used, sused, smshd->msg_cnt, cnt, smshd->free_seg_cnt, ns);
*/
    if (masize < off ||
	smshd->total_space != masize ||
	smshd->space_used != sused ||
	smshd->msg_cnt != cnt ||
	smshd->free_seg_cnt != ns)
	return (LB_LB_ERROR);

    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function generates the free space table. 

    Input:	lb - the LB structure.

    Return:	LB_SUCCESS or an LB error number.

********************************************************************/

static int Sm_generate_table (LB_struct *lb)
{
    Sms_header_t *smshd;
    Per_lb_data_t *pld;
    Msg_table_t *msgs;
    int cnt, i, sused, off, ns, masize, in_cb;

    if ((cnt = Get_sorted_msg_list (lb, &msgs, &in_cb, &sused)) < 0)
	return (cnt);

    /* build the free space table */
    lb->hd->sms_ok = 0;
    pld = lb->pld;
    smshd = (Sms_header_t *)(pld->lb_pt + lb->off_sms);
    pld->rsid = RSIS_init (lb->maxn_msgs + 1, 2, sizeof (Sms_free_seg_t), 
		(char *)smshd + sizeof (Sms_header_t), pld->rsid_buf, Compare);
    if (pld->rsid == NULL)
	return (Sm_gen_table_ret (in_cb, (char *)msgs, LB_RSIS_FAILED));

    ns = 0;
    off = 0;			/* current starting offset */
    for (i = 0; i < cnt; i++) {
	Msg_table_t *curt;
	Sms_free_seg_t seg;

	curt = msgs + i;
	if (curt->loc > off) {
	    seg.loc = off;
	    seg.len = curt->loc - off;
	    if (RSIS_insert (pld->rsid, &seg) < 0)
		return (Sm_gen_table_ret (in_cb, (char *)msgs, LB_RSIS_FAILED));
	    ns++;
	}
	off = curt->loc + curt->len;
    }
    Sm_gen_table_ret (in_cb, (char *)msgs,0);

    masize = lseek (pld->fd, 0, SEEK_END) - lb->off_a;	/* msg area size */
    if (masize < off)
	return (LB_LENGTH_ERROR);
    else if (masize > off) {
	Sms_free_seg_t seg;

	seg.loc = off;
	seg.len = masize - off;
	if (RSIS_insert (pld->rsid, &seg) < 0)
	    return (LB_RSIS_FAILED);
	ns++;
    }

    /* set the header */
    smshd->total_space = masize;
    smshd->big_endian = MISC_i_am_bigendian ();
    smshd->space_used = sused;
    smshd->msg_cnt = cnt;
    smshd->free_seg_cnt = ns;

    lb->hd->sms_ok = 1;
    return (LB_SUCCESS);
}

/********************************************************************
			
    Generates the sorted message list. 

    Input:	lb - the LB structure.

    Output:	msgs_pt - the message list.
		in_cb_pt - in-callback flag.
		sused_pt - space used.

    Return:	Number messages or an LB error number.

********************************************************************/

static int Get_sorted_msg_list (LB_struct *lb, 
			Msg_table_t **msgs_pt, int *in_cb_pt, int *sused_pt) {
    Msg_table_t *msgs;
    unsigned int num_ptr;
    int nmsgs, pt, ind, cnt, i, sused, in_cb;
    EN_per_thread_data_t *ptd;

    ptd = EN_get_per_thread_data ();
    in_cb = ptd->in_callback;
    if (in_cb) {
	Tmp_space_size = lb->maxn_msgs * sizeof (Msg_table_t);
	Tmp_space_p = MISC_ind_malloc (Tmp_space_size);
	msgs = (Msg_table_t *)Tmp_space_p;
    }
    else
	msgs = (Msg_table_t *)malloc (lb->maxn_msgs * sizeof (Msg_table_t));
    if (msgs == NULL)
	return (LB_MALLOC_FAILED);

    /* go through the msg dir to get all messages */
    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    pt = GET_POINTER(num_ptr) - nmsgs;	/* pointer to the first message */
    if (pt < 0)
	pt += lb->ptr_range;

    sused = 0;				/* space in use */
    ind = pt % lb->n_slots;
    cnt = 0;
    if (lb->flags & LB_DB) {
	for (i = 0; i < nmsgs; i++) {
	    unsigned int ucnt;
	    int msg_len;
	    LB_msg_info_t *msginfo;

	    ucnt = lb->dir[ind].loc;
	    msginfo = (LB_msg_info_t *)lb->msginfo + DB_WORD_OFFSET (ind, ucnt);
	    msg_len = msginfo->len;
	    if (msg_len > 0) {
		msgs[cnt].loc = msginfo->loc;
		msgs[cnt].len = msg_len;
		sused += msg_len;
		cnt++;
	    }
	    ind = (ind + 1) % lb->n_slots;
	}
    }
    else {
	for (i = 0; i < nmsgs; i++) {
	    LB_msg_info_seq_t *msginfo;
	    int msg_len;

	    msginfo = (LB_msg_info_seq_t *)lb->msginfo + ind;
	    msg_len = msginfo->len;
	    if (msg_len > 0) {
		msgs[cnt].loc = lb->dir[ind].loc;
		msgs[cnt].len = msg_len;
		sused += msg_len;
		cnt++;
	    }
	    ind = (ind + 1) % lb->n_slots;
	}
    }

    /* sort the messages */
    Sort (cnt, msgs);

    *msgs_pt = msgs;
    *in_cb_pt = in_cb;
    *sused_pt = sused;
    return (cnt);
}

/********************************************************************
			
    Cleanup and return function for Sm_generate_table.

    Input:	in_cb - in NTF callback or not.
		msgs - pointer to free.
		ret - return value.

    Return:	"ret".

********************************************************************/

static int Sm_gen_table_ret (int in_cb, char *msgs, int ret) {
    if (in_cb) {
	if (Tmp_space_p != NULL)
	    MISC_ind_free (Tmp_space_p, Tmp_space_size);
	Tmp_space_p = NULL;
    }
    else
	free (msgs);
    return (ret);
}

/********************************************************************
			
    Description: This is the comprison function for the RSIS module.

    Input:	r1 - pointer to the first record.
		r1 - pointer to the second record.

    Return:	RSIS_LESS, RSIS_EQUAL or RSIS_GREATER.

********************************************************************/

static int Compare (int which_key, void *r1, void *r2)
{
    int *k1, *k2;

    k1 = (int *)r1 + which_key;
    k2 = (int *)r2 + which_key;
    if (*k1 < *k2)
	return (RSIS_LESS);
    else if (*k1 == *k2)
	return (RSIS_EQUAL);
    else
	return (RSIS_GREATER);
}

/********************************************************************
			
    Description: This returns non-zero if the byte order in the SMS
		is the same as the local host byte order. Otherwise 
		it returns 0.

********************************************************************/

static int Right_endian (LB_struct *lb)
{
    Sms_header_t *smshd;
    int i_am_bigendian;

    smshd = (Sms_header_t *)(lb->pld->lb_pt + lb->off_sms);
    i_am_bigendian = MISC_i_am_bigendian ();
    if ((i_am_bigendian && smshd->big_endian) ||
	(!i_am_bigendian && !smshd->big_endian))
	return (1);
    else
	return (0);
}


/********************************************************************
			
    Description: This function searches for a free space in the LB
		message area. It calls Get_random_free_space or 
		Get_sequential_free_space to do the job.

    Input:	lb - the LB structure
		length - length of the requested free space

    Output:	is_new - flag indicating the space has not been initialized.

    Returns:	This function returns the free space location on success, 
		or a negative LB error number on failure.

********************************************************************/

int LB_sms_get_free_space (LB_struct *lb, int length, int *is_new)
{
    int tloc, size, part1, more, loc;
    int ma_size;

    if (lb->flags & LB_DB)
	return (Get_random_free_space (lb, length, is_new));

    if (lb->ma_size == 0) {
	if ((lb->flags & LB_NOEXPIRE) ||
	    ((lb->flags & LB_MUST_READ) && 
		LB_Ptr_compare (lb, 
			(GET_POINTER (lb->hd->num_pt) + lb->ptr_range -
			lb->maxn_msgs) % lb->ptr_range,
			(int)lb->hd->ptr_read) >= 0))
	    return (LB_FULL);
	return (Get_random_free_space (lb, length, is_new));
    }

    *is_new = 0;
    if (!(lb->flags & LB_DIRECT)) {
	size = Get_sequential_free_space (lb, length, &loc);
	if (size < 0)
	    return (size);
	return (loc);
    }

    size = Get_sequential_free_space (lb, length, &tloc);
    if (size < 0)
	return (size);

    loc = tloc;
    ma_size = lb->ma_size;
    part1 = ma_size - tloc;
    if (part1 >= length) {
	if (tloc + length == ma_size)
	    lb->hd->unused_bytes = 0;
	return (loc);		/* the required contiguous space found */
    }

    /* we need more space */
    more = length + part1;
    if (more > ma_size)
	more = ma_size;
    size = Get_sequential_free_space (lb, more, &tloc);
    if (size < 0)
	return (size);

    if (size == ma_size) {	/* empty LB */
	part1 = 0;
	Get_sequential_free_space (lb, more, &tloc);	/* reset dir.loc to 0 */
    }

    lb->hd->unused_bytes = part1;
    loc = 0;

    return (loc);
}

/********************************************************************
			
     Description: This function searchs for a free space of "length"
		int an sequential LB. it first verifies if the free 
		space in the LB is at least "length". If it is not 
		true, it removes the appropriate number of messages 
		to gain the required free space. If the 
		requirement can not be satisfied (due to restrictions 
		posed by LB_NOEXPIRE or LB_MUST_READ flags 
		or the message is too large) no message is removed. 

    Input:	lb - the LB structure
		length - length of the requested free space

    Output:	loc - the offset of the free space in the message area.

    Returns:	This function returns the free space size on success, 
		or a negative LB error number to indicate an error 
		condition.

    Notes:	If the free space is no less than "length" and the 
		number of messages equals maxn_msgs, we do not need
		to remove a message because additional slots have been
		set up in the msg directory. This will save a 
		LB_Update_pointer call. However, we have to check if
		the oldest message is allowed to be removed. This
		is implemented here. Refer to the two lines with comment
		"done".

		When the LB is empty, we alway reset the message offset
		to 0 such that the new message will be stored at the 
		beginning of the message area. This reset has to be done
		here since reset is necessary after all messages are 
		cleared.

********************************************************************/

static int Get_sequential_free_space (LB_struct *lb, int length, int *loc)
{
    unsigned int num_ptr;
    int nmsgs, ptn;
    int msg_o, free_o;
    int pt;
    int n_slots, i, cnt;
    int size;
    LB_dir *dir;

    if (length > lb->ma_size)
	return (LB_MSG_TOO_LARGE);

    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    ptn = GET_POINTER(num_ptr);		/* pointer to the new message */

    n_slots = lb->n_slots;
    dir = lb->dir;

    /* offset of the first message and the free space */
    ptn--;
    if (ptn < 0)
	ptn = lb->ptr_range - 1;
    if (nmsgs == 0)			/* reset the message location */
	(dir [ptn % n_slots]).loc = 0;
    free_o = dir[ptn % n_slots].loc;
    *loc = free_o;
    pt = (ptn - nmsgs + lb->ptr_range) % lb->ptr_range;	/* the one before the 
							   first msg */
    msg_o = dir[pt % n_slots].loc;

    if (free_o > msg_o)			/* free space size */
	size = lb->ma_size - (free_o - msg_o);
    else if (free_o < msg_o)
	size = msg_o - free_o;
    else {
	if (nmsgs > 0)
	    size = 0;
	else
	    size = lb->ma_size;
    }

    if (size < 0 || size > lb->ma_size)
	return (LB_LB_ERROR);

    /* counting how many messages must be removed */
    cnt = 0;
    for (i = 0; i < nmsgs; i++) {
	int ppt;
	int s;

	if (size >= length && nmsgs - cnt < lb->maxn_msgs)	/* done */
	    break;

	pt = (pt + 1) % lb->ptr_range;	/* started from the first msg */

	if ((lb->flags & LB_NOEXPIRE) ||
	    ((lb->flags & LB_MUST_READ) && 
		LB_Ptr_compare (lb, pt, 
				(int)lb->hd->ptr_read) >= 0))
	    return (LB_FULL);

	if (size >= length)		/* done */
	    break;

	ppt = pt - 1;			/* the previous message */
	if (ppt < 0)
	    ppt = lb->ptr_range - 1;
	s = dir[pt % n_slots].loc - dir[ppt % n_slots].loc;
	if (s <= 0)
	    s += lb->ma_size;
	size += s;
	cnt++;
    }
    if (size < length)
	return (LB_LB_ERROR);

    if (cnt > 0) 
	LB_Update_pointer (lb, 0, cnt);	/* remove the messages */

    return (size);
}

/********************************************************************
			
    Description: This is the Heapsort algorithm from "Numerical recipes
		in C". Refer to the book.

    Input:	n - array size.
		ra - array to sort into ascent order.

********************************************************************/

static void Sort (int n, Msg_table_t *ra)
{
    int l, j, ir, i;
    Msg_table_t rra;				/* type dependent */

    if (n <= 1)
	return;
    ra--;
    l = (n >> 1) + 1;
    ir = n;
    for (;;) {
	if (l > 1)
	    rra = ra[--l];
	else {
	    rra = ra[ir];
	    ra[ir] = ra[1];
	    if (--ir == 1) {
		ra[1] = rra;
		return;
	    }
	}
	i = l;
	j = l << 1;
	while (j <= ir) {
	    if (j < ir && ra[j].loc < ra[j+1].loc)	/* type dependent */
		++j;
	    if (rra.loc < ra[j].loc) {		/* type dependent */
		ra[i] = ra[j];
		j += (i = j);
	    }
	    else
		j = ir + 1;
	}
	ra[i] = rra;
    }
}



