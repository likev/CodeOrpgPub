/****************************************************************
		
    Module: lb_write.c	
				
    Description: This module contains the LB_write function.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:32 $
 * $Id: lb_write.c,v 1.77 2012/07/27 19:33:32 jing Exp $
 * $Revision: 1.77 $
 * $State: Exp $
 * $Log: lb_write.c,v $
 * Revision 1.77  2012/07/27 19:33:32  jing
 * Update
 *
 * Revision 1.63  2002/03/12 16:51:44  jing
 * Update
 *
 * Revision 1.60  2000/08/21 20:50:10  jing
 * @
 *
 * Revision 1.59  2000/04/25 20:04:22  jing
 * @
 *
 * Revision 1.52  1999/06/29 21:20:16  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.46  1999/05/27 02:27:37  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.42  1999/05/03 20:58:19  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.23  1998/06/25 17:58:21  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.20  1998/06/01 17:23:45  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/22 13:08:04  cm
 * SunOS 5.5 modifications
 *
*/


/* System include files */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

/* Local include files */

#include <lb.h>
#ifndef LBLIB
#include <net.h>
#else
#define INADDR_NONE 0xffffffff
#endif
#include <misc.h>
#include <en.h>
#include <rmt.h>
#include "lb_def.h"


/* Definitions / macros / types */

#define MSG_NOT_REPLACED -1		/* one of the return values of
					   Replace_message () */
#define DELETE_FLAG ((void *)1)		/* special value for argument "msg"
					   of LB_write */

static unsigned short Sender_id = 0;	/* Local NTF sender's ID */
static int C_and_w = 0;
extern int LB_check_and_write;


/* static functions */
static int LB_write_internal (int lbd, const char *msg, int length, LB_id_t id);
static int Write_msg_body (LB_struct *lb, int loc, int wlen, 
				const char *buf, int new_space);

static int Write_with_write (LB_struct *lb, int loc, 
					int wlen, const char *buf);
static int Replace_message (LB_struct *lb, LB_id_t id, 
				int length, const char *msg, int tag);
static int Send_to_server (LB_struct *lb, 
		LB_nr_t *nr, int msg_type, int len, LB_id_t lbmsgid);
static int Find_expired_msgs (LB_struct *lb, 
			int n_rms, unsigned int onp, int *ind_first);
static int Find_replace_msgpt (LB_struct *lb, LB_id_t *id);
static int Check_nr_lock (LB_struct *lb);
static int Check_msg (LB_struct *lb, const char *msg, int length, LB_id_t id);


/********************************************************************
			
    Description: This function delets a message of id "id" in LB "lbd".
		Replaceable LB only. 

    Input:	lbd - LB descriptor
		id - id of the message

    Returns:	This function returns LB_SUCCESS on success, or a 
		negative number to indicate an error condition.

********************************************************************/

int LB_delete (int lbd, LB_id_t id)
{
    return (LB_write_internal (lbd, (const char *)DELETE_FLAG, -1, id));
}

/********************************************************************
			
    Description: This function writes a message "msg" of length 
		"length" and id "id" to LB "lbd". 

    Input:	lbd - LB descriptor
		msg - pointer to the message to be written
		length - length of the message
		id - id of the message

    Returns:	This function returns the message length on success, or a 
		negative number to indicate an error condition.

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_write (int lbd, const char *msg, int length, LB_id_t id)
{
    C_and_w = LB_check_and_write;
    LB_check_and_write = 0;
    if (length == 0 && msg == DELETE_FLAG)
	return (LB_BAD_ARGUMENT);
    return (LB_write_internal (lbd, msg, length, id));
}

/********************************************************************
			
    Description: Internal implementation of LB_write. See LB_write prolog.

********************************************************************/

static int LB_write_internal (int lbd, const char *msg, int length, LB_id_t id)
{
    LB_struct *lb;
    unsigned int num_ptr, org_num_ptr;
    int ptr, nmsgs;
    int n_rm;
    LB_dir *dir;
    int loc, new_space;
    int ret, ind, tag;

    if ((length < 0 && msg != DELETE_FLAG) || (length > 0 && msg == NULL) ||
	(id > LB_MAX_ID && id != LB_ANY))	/* check arguments */
	return (LB_BAD_ARGUMENT);

    /* get the LB structure, lock and mmap the file */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);
    if ((ret = LB_lock_mmap (lb, WRITE_PERM, EXC_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));

    if (!(lb->flags & LB_WRITE))	/* check write flag */
	return (LB_Unlock_return (lb, LB_BAD_ACCESS));

    if ((lb->flags & LB_DIRECT) &&	/* check direct access lock */
	LB_direct_access_lock (lb, TEST_LOCK, 0) != 0)
	return (LB_Unlock_return (lb, LB_BAD_ACCESS));

    if (length <= 0 && !(lb->flags & LB_DB))
	return (LB_Unlock_return (lb, LB_BAD_ARGUMENT));

    if (msg == DELETE_FLAG && (lb->hd->miscflags & LB_ID_BY_USER))
	return (LB_Unlock_return (lb, LB_NOT_SUPPORTED));

    if (lb->active_test) {		/* test LB_ACTIVE_SV_LOCK_OFF lock */
	ret = LB_process_lock (TEST_LOCK, lb, EXC_LOCK, LB_ACTIVE_SV_LOCK_OFF);
	if (ret != LB_LOCKED)
	    return (LB_Unlock_return (lb, LB_NOT_ACTIVE));
    }

    if (C_and_w && Check_msg (lb, msg, length, id))
	return (LB_Unlock_return (lb, 0));

    org_num_ptr = lb->hd->num_pt;	/* save for evaluating # expired msgs */

    if (lb->utag != NULL)
	tag = *(lb->utag);
    else
	tag = 0;

    /* process message replacing */
    if (lb->flags & LB_DB) {
	ret = Replace_message (lb, id, length, msg, tag);
	if (ret != MSG_NOT_REPLACED)
	    return (LB_Unlock_return (lb, ret));
    }

    /* find the write pointer and the new dir slot */
    num_ptr = lb->hd->num_pt;
    ptr = GET_POINTER (num_ptr);
    ind = ptr % lb->n_slots;		/* new slot index */
    nmsgs = GET_NMSG (num_ptr) + 1;	/* msg number including the new */

    if (lb->flags & LB_DB) {
	if (id != LB_ANY && (lb->hd->miscflags & LB_MSG_DELETED))
	    return (LB_Unlock_return (lb, LB_NOT_SUPPORTED));
	if (nmsgs > lb->maxn_msgs)
	    return (LB_Unlock_return (lb, LB_FULL));
    }

    /* get free space */
    loc = new_space = 0;
    if (length > 0)
	loc = LB_sms_get_free_space (lb, length, &new_space);
    if (loc < 0)
	return (LB_Unlock_return (lb, loc));

    /* write the message */
    if (length > 0 &&
	(ret = Write_msg_body (lb, loc, length, msg, new_space)) < 0)
	return (LB_Unlock_return (lb, ret));

    /* update the dir slot and the msg info */
    dir = lb->dir + ind;
    if (lb->flags & LB_DB) {
	LB_msg_info_t *msginfo;
	dir->loc = 0;			/* for ucnt */
	msginfo = (LB_msg_info_t *)lb->msginfo + DB_WORD_OFFSET (ind, 0);
	msginfo->len = length;
	msginfo->loc = loc;
    }
    else if (lb->ma_size == 0) {
	LB_msg_info_seq_t *msginfo;
	dir->loc = loc;
	msginfo = (LB_msg_info_seq_t *)lb->msginfo + ind;
	msginfo->len = length;
    }
    else
	dir->loc = (loc + length) % lb->ma_size;
    {	/* find the previous id and check if the LB is of non-decreasing ID */
	LB_id_t prid;

	/* this works since the entire control area is initialized to 0 */
	if (nmsgs > 1) {	/* find previous id */
	    int ppt;		/* pointer to the previous slot */
	    ppt = ptr - 1;
	    if (ppt < 0)
		ppt = lb->ptr_range - 1;
	    prid = (lb->dir [ppt % lb->n_slots]).id;
	}
	else
	    prid = 0;
	if (id == LB_ANY)		/* new id */
	    id = (lb_t)((prid + 1) % (unsigned int)(LB_MAX_ID + 1));
	else
	    lb->hd->miscflags |= LB_ID_BY_USER;
	if (id < prid && nmsgs > 1)	/* The LB is not of non-decreasing ID */
	    lb->hd->non_dec_id &= 0xfe;
    }
    dir->id = id;

    /* write the tag for the new message */
    if (lb->hd->tag_size > 0)
	LB_write_tag (lb, ind, tag, 0);

    /* process nrs and update LB time */
    if (lb->off_nra > 0 &&
	(ret = LB_process_nr (lb, id, length, tag,
				N_MSG_RM_COMPU, org_num_ptr)) < 0)
	    return (LB_Unlock_return (lb, ret));
    lb->hd->lb_time = time (NULL);

    /* update num_ptr and page number */
    if (nmsgs > lb->maxn_msgs)
	n_rm = nmsgs - lb->maxn_msgs;	/* number of messages to remove */
    else
	n_rm = 0;
    LB_Update_pointer (lb, 1, n_rm);	/* we add one new message */

    if (((lb->flags & LB_DB) || lb->ma_size == 0) &&
	length > 0)			/* sets sms_ok flag */
	lb->hd->sms_ok = 1;

    if (nmsgs > lb->maxn_msgs && lb->ma_size == 0) {
	int ln, lc, p;

	p = ptr - nmsgs + 1 + lb->n_slots;
	ln = LB_Get_message_info (lb, p, &lc, NULL);
	LB_sms_free_space (lb, lc, ln);
    }

    if (lb->flags & LB_SHARE_STREAM) {
	int exppt = (GET_POINTER (lb->hd->num_pt) + lb->ptr_range -
			(lb->maxn_msgs + 1)) % lb->ptr_range;
	if (LB_Ptr_compare (lb, exppt, (int)lb->hd->ptr_read) > 0)
	    lb->hd->ptr_read = exppt;
    }		/* advance ptr_read to keep close to the available msgs */

    lb->prev_id = id;
    if (lb->umid != NULL)
	*lb->umid = id;
    if (length < 0)
	length = 0;
    return (LB_Unlock_return (lb, length));
}

/********************************************************************
			
    Description: This function searches for the message to replace. 
		If id is LB_ANY, we choose a freed (zero size) msg
		to replace. We use a circular search here. It should
		be efficient unless the LB is very close to full.

    Input:	lb - the LB structure;

    Input/Output:	id - id of the message.

    Return:	This function returns the found LB msg pointer on 
		success or LB_FAILURE if the messege ID is not found
		or the LB is full in case of "id" is LB_ANY.

********************************************************************/

static int Find_replace_msgpt (LB_struct *lb, LB_id_t *id)
{
    int msgpt, page;

    if (*id == LB_ANY) {
	static int ss_off = 0;			/* circular search offset */
	unsigned int num_ptr;
	int pt0, cnt, turn, nmsgs, ind;

	if (!(lb->hd->miscflags & LB_MSG_DELETED))
	    return (LB_FAILURE);

	num_ptr = lb->hd->num_pt;
	nmsgs = GET_NMSG (num_ptr);		/* msg number */
	pt0 = GET_POINTER(num_ptr) - nmsgs;	/* pointer to the first msg */
	if (pt0 < 0)
	    pt0 += lb->ptr_range;

	if (nmsgs < lb->maxn_msgs)
	    return (LB_FAILURE);
	cnt = 0;
	if (ss_off >= nmsgs)
	    ss_off = 0;
	ind = (pt0 + ss_off) % lb->n_slots;
	turn = nmsgs - ss_off;
	while (1) {				/* search for a 0 size msg */
	    unsigned int ucnt;
	    LB_msg_info_t *msginfo;

	    ucnt = lb->dir[ind].loc;
	    msginfo = (LB_msg_info_t *)lb->msginfo + DB_WORD_OFFSET (ind, ucnt);
	    if (msginfo->len < 0)		/* found */
		break;
	    cnt++;
	    if (cnt == turn)			/* back to the first msg */
		ind = pt0 % lb->n_slots;
	    if (cnt >= nmsgs) {			/* LB is full */
		ss_off = (ss_off + cnt) % nmsgs;
		return (LB_FAILURE);
	    }
	    ind = (ind + 1) % lb->n_slots;
	}
	ss_off = (ss_off + cnt) % nmsgs;
	msgpt = ind;
	*id = lb->dir[ind].id + LB_MSG_DB_ID_MASK + 1;
	if (*id > LB_MAX_ID)
	    *id = (lb->dir[ind].id) & LB_MSG_DB_ID_MASK;
	lb->dir[ind].id = *id;
    }
    else if (LB_Search_msg (lb, *id, &msgpt, &page) < 0) /* not found */
	return (LB_FAILURE);

    lb->prev_id = *id;
    if (lb->umid != NULL)
	*lb->umid = *id;

    return (msgpt);
}

/********************************************************************
			
    Description: This function replaces a message in the LB. 

    Input:	lb - the LB structure;
		id - id of the message
		length - length of the message
		msg - pointer to the message to be written
		tag - tag value for the new message.

    Return:	This function returns the new message length on success
		or a negative LB error number on error conditions. It
		returns MSG_NOT_REPLACED in case the msg is not found
		or the LB is full while id is LB_ANY.

********************************************************************/

static int Replace_message (LB_struct *lb, LB_id_t id, 
			int length, const char *msg, int tag)
{
    int msgpt, ucnt, ind, ret, loc, len, newloc, is_new;
    LB_msg_info_t *msginfo;
    LB_id_t old_id;

    old_id = id;
    if ((msgpt = Find_replace_msgpt (lb, &id)) == LB_FAILURE) {
	if (msg == DELETE_FLAG)
	    return (LB_NOT_FOUND);
	return (MSG_NOT_REPLACED);
    }

    /* replace the message */
    ind = msgpt % lb->n_slots;
    ucnt = lb->dir[ind].loc;	/* get ucnt */
    msginfo = (LB_msg_info_t *)lb->msginfo + DB_WORD_OFFSET (ind, ucnt);
    len = msginfo->len;
    loc = msginfo->loc;
    if (len < 0 && old_id != LB_ANY)
	return (LB_NOT_FOUND);

    if (length > 0) {		/* write to the LB */
	if ((newloc = LB_sms_get_free_space (lb, length, &is_new)) < 0)
	    return (newloc);
	if ((ret = Write_msg_body (lb, newloc, length, msg, is_new)) < 0)
	    return (ret);
    }
    else {
	if (msg == DELETE_FLAG && len < 0)
	    return (LB_NOT_FOUND);
	newloc = 0;		/* not to be used */
    }

    /* write the tag for the new message */
    if (length >= 0 && lb->hd->tag_size > 0) {
	if (old_id == id)		/* update an old message - keep tag */
	    tag = LB_read_tag (lb, ind);
	LB_write_tag (lb, ind, tag, 1);
    }

    /* update msginfo */
    msginfo = (LB_msg_info_t *)lb->msginfo + DB_WORD_OFFSET (ind, ucnt + 1);
    msginfo->len = length;
    msginfo->loc = newloc;
    if (length < 0)
	lb->hd->miscflags |= LB_MSG_DELETED;

    /* process nrs, update LB time and ucnt */
    if (lb->off_nra > 0 &&
	    (ret = LB_process_nr (lb, id, length, tag, 0, 0)) < 0)
	return (ret);
    lb->hd->lb_time = time (NULL);
    lb->dir[ind].loc = ucnt + 1;

    /* set sms_ok flag and free old space */
    if (length > 0)
	lb->hd->sms_ok = 1;
    if (len > 0)
	LB_sms_free_space (lb, loc, len);

    if (length < 0)
	length = 0;
    return (length);
}

/********************************************************************
			
    Description: This function writes "wlen" bytes stored in "buf" 
		starting from offset "loc" in the message area of LB 
	 	"lb". "db_ind" defines which message area buffer
		to use.

    Input:	lb - the LB structure;
		loc - the message offset in the message area;
		wlen - number of bytes to write;
		buf - the message to write;
		new_space - writes in a newly allocated space.

    Returns:	This function returns "wlen" on success or a negative
		LB error number.

********************************************************************/

static int Write_msg_body (LB_struct *lb, int loc, int wlen, 
				const char *buf, int new_space)
{
    int nwritten;
    int ma_sz, ret;

    ma_sz = lb->ma_size;		/* message area size */
    if (ma_sz == 0)	/* disable some of the following oreparations */
	ma_sz = 0x7fffffff;
    if (loc < 0 || loc >= ma_sz || wlen < 0 || wlen > ma_sz)
	return (LB_LB_ERROR);
    if (wlen == 0)
	return (0);

    if (new_space && !(lb->flags & LB_MEMORY)) {
#ifdef MMAP_UNMAP_NEEDED
	LB_unmap (lb);
#endif
	ret = Write_with_write (lb, loc, wlen, buf);
#ifdef MMAP_UNMAP_NEEDED
	LB_mmap (lb, PREV_PERM);
#endif
	if (ret < 0)
	    return (ret);
	if (ret > 0)
	    return (wlen);
    }

    /* write the message */
    nwritten = 0;				/* bytes read */
    while (nwritten < wlen) {
	int len;

	len = wlen - nwritten;
	if (len > ma_sz - loc)
	    len = ma_sz - loc;

	if (lb->flags & LB_MEMORY) {
	    memcpy (lb->pld->lb_pt + loc + lb->off_a, buf + nwritten, len);
	}
	else {
	    int ret;

	    ret = LB_data_transfer (WRITE_TRANS, lb, 
			(char *)(buf + nwritten), len, loc + lb->off_a);
	    if (ret < 0)
		return (ret);

	}
	nwritten += len;
	loc = (loc + len) % ma_sz;
    }

    return (nwritten);
}

/********************************************************************
			
    Description: This function writes a message with the "write" call
		to extend the file boundary.

    Input:	lb - the LB structure;
		loc - the message offset in the message area;
		wlen - number of bytes to write;
		buf - the message to write;

    Returns:	This function returns "wlen" on success, a negative
		LB error number or 0 if the write is within the file
		boundary.

********************************************************************/

static int Write_with_write (LB_struct *lb, int loc, 
					int wlen, const char *buf)
{
    int fd, fsize, ret;

    fd = lb->pld->fd;
    if ((fsize = lseek (fd, 0, SEEK_END)) < 0)
	return (LB_SEEK_FAILED);

    if (loc + wlen + lb->off_a <= fsize)
	return (0);
/*
    if (loc + lb->off_a > fsize)
	return (LB_LB_ERROR);
*/

    if (lseek (fd, loc + lb->off_a, SEEK_SET) < 0)
	return (LB_SEEK_FAILED);

    ret = MISC_write (fd, buf, wlen);
    if (ret != wlen)
	return (ret);

    return (wlen);
}

/********************************************************************
			
    Description: This function, called by LB_write and LB_clear, 
		processes notification requests.

    Input:	lb - the LB involved.
		new_msgid - msg id of the new message. 
			LB_MSG_EXPIRED: there is no new message.
		msg_len - length of the new message.
		tag - message tag.
		n_rms - number of messages to be removed. N_MSG_RM_COMPU
			n_rms needs computation.
		onp - original number-pointer combination used for 
		      evaluating number of removed/to-be-removed messages
		      where n_rms = N_MSG_RM_COMPU.

    Return:	LB_SUCCESS on success or a negative LB error number.

********************************************************************/

#define LOCK_CHECK_PERIOD 2

int LB_process_nr (LB_struct *lb, LB_id_t new_msgid, 
		int msg_len, int tag, int n_rms, unsigned int onp)
{
    int n_rec, i, n_to_send, ret, send_msg;
    LB_nr_t *rec;
    int n_exps, ind_first;
    time_t crt;

    EN_parameters (EN_GET, &Sender_id, &send_msg);
    if (send_msg == LB_UN_SEND_TMP_DISABLE) {	/* rm tmp disabling */
	i = 1;
	EN_parameters (EN_SET, NULL, &i);
    }

    /* perform lock check */
    if ((crt = MISC_systime (NULL)) >= lb->nr_lock_check_time + LOCK_CHECK_PERIOD) {
	int diff = crt - lb->nr_lock_check_time;
	if (diff > 100 || diff < 0)
	    diff = 100;
	diff += lb->maint_param;
	if (diff > 100)
	    diff = 100;
	lb->maint_param = diff;
	ret = Check_nr_lock (lb);
	if (ret < 0)
	    return (ret);
	lb->nr_lock_check_time = crt;
    }

    n_rec = LB_get_nra_size (lb);
    rec = (LB_nr_t *)(lb->pld->lb_pt + lb->off_nra);

    /* process all nr records */
    n_exps = -1; /* make sure Find_expired_msgs will be called at most once */
    ind_first = 0;	/* useless - To remove gcc warning */
    n_to_send = 0;
    for (i = 0; i < n_rec; i++) {
	LB_id_t msgid;
	LB_nr_t *r;

	r = rec + i;
	if (r->host == 0)
	    break;
	if (Get_nr_lock (r->lock) == 0)
	    continue;

	if (send_msg != LB_TRUE)
	    continue;

	/* match the new events */
	msgid = LB_T_BSWAP (r->msgid);
	if (Get_nr_flag (r->flag) == LB_NR_NOTIFY) {

	    if (new_msgid != LB_MSG_EXPIRED && 
		(msg_len > 0 || ((lb->flags & LB_DB) && msg_len == 0)) &&
		(msgid == LB_ANY || msgid == new_msgid || 
		(
		    (msgid & LB_UN_MSGID_TEST) == 
						LB_UN_MSGID_TEST &&
		    (msgid & LB_UN_MSGID_MASK) == 
				((unsigned int)tag & LB_UN_MSGID_MASK)
		)
	    )) {			/* msg written or updated */
		int un_msg;

		if (lb->flags & LB_UN_TAG)
		    un_msg = tag;
		else
		    un_msg = msg_len;
		n_to_send = Send_to_server (lb, r, 
				FROM_LB_WRITE_UN, un_msg, new_msgid);
	    }
	    else if (new_msgid != LB_MSG_EXPIRED && msg_len < 0 &&
		    msgid == LB_MSG_EXPIRED && (lb->flags & LB_DB)) {
							/* msg deleted */
		n_to_send = Send_to_server (lb, r, 
				FROM_LB_WRITE_UN, 0, new_msgid);
	    }
	    else if (msgid == LB_MSG_EXPIRED) {		/* msgs expired */
		int k;

		if (n_exps < 0)
		    n_exps = Find_expired_msgs (lb, n_rms, onp, &ind_first);
		for (k = 0; k < n_exps; k++) {
		    LB_dir *dir;

		    dir = lb->dir + ((ind_first + k) % lb->n_slots);
		    n_to_send = Send_to_server (lb, r, 
				FROM_LB_WRITE_UN, 0, dir->id);
		    if (n_to_send < 0)
			break;
		}
	    }
	}
	if (n_to_send < 0)
	    return (n_to_send);

    }
    if (n_to_send > 0 &&
	(ret = Send_to_server (lb, NULL, 0, 0, 0)) < 0)
	return (ret);

    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function calculates number of msgs expired/to-
		be-expired and the slot index in LB dir of the first 
		expired message.

    Input:	lb - the LB structure;
		n_rms - number of messages to be expired. N_MSG_RM_COMPU
			n_rms needs computation.
		onp - original number-pointer combination used for 
		      evaluating number of expired/to-be-expired messages
		      where n_rms = N_MSG_RM_COMPU.

    Output:	ind_first - index in the LB dir to the first expired/to-
		be-expired message.

    Return:	This function returns the number of expired/to-
		be-expired messages.

********************************************************************/

static int Find_expired_msgs (LB_struct *lb, 
			int n_rms, unsigned int onp, int *ind_first)
{
    unsigned int cr_num_ptr;
    int n_exps, cr_nmsgs, off_first;

    cr_num_ptr = lb->hd->num_pt;
    cr_nmsgs = GET_NMSG (cr_num_ptr);	/* current # msgs */

    if (n_rms != N_MSG_RM_COMPU) {
	n_exps = n_rms;
	off_first = cr_nmsgs;
    }
    else {				/* evaluate number of expired/to-
					   be_expired msgs */
	off_first = GET_NMSG (onp);		/* # msgs before expiring */
	n_exps = off_first - cr_nmsgs;		/* # expired msgs */
	if (off_first >= lb->maxn_msgs)		/* one more msg will expire 
						   when the new msg is added */
	    n_exps++;
    }

    /* find index of the first expired/to-be-expired message */
    if (n_exps > 0) {
	int ptr;

	ptr = GET_POINTER (cr_num_ptr);
	*ind_first = (ptr - off_first + lb->n_slots) % lb->n_slots;
					/* slot index of the first msg */
    }
    else				/* avoid any negative return */
	n_exps = 0;

    return (n_exps);
}

/********************************************************************
			
    Description: This function checks locks of all NR records. It
		resets the lock flag if a record is not locked. It
		returns the number of locked records on success.

    Input:	lb - the LB involved.

    Return:	the number of locked records on success or a negative 
		LB error number.

********************************************************************/

static int Check_nr_lock (LB_struct *lb)
{
    LB_nr_t *rec;
    int n_rec, offset, cnt, check, i;

    n_rec = LB_get_nra_size (lb);
    offset = lb->off_nra;
    rec = (LB_nr_t *)(lb->pld->lb_pt + offset);

    check = 0;
    if (lb->maint_param >= 60) {
	check = 1;
	lb->maint_param = 0;
    }
    cnt = 0;
    for (i = 0; i < n_rec; i++) {

	if (rec->host == 0)
	    break;

	if (Get_nr_lock (rec->lock) != 0) {
	    int ret = LB_process_lock (TEST_LOCK, lb, EXC_LOCK, 
						LB_NR_LOCK_OFF + i);
	    if (ret < 0)
		return (ret);
	    if (ret != LB_LOCKED)
		rec->lock = 0;
	    else
		cnt++;
	}
	else if (check) {	/* test if a bug has been fixed */
	    if (LB_process_lock (TEST_LOCK, lb, EXC_LOCK, 
					LB_NR_LOCK_OFF + i) == LB_LOCKED) {
		MISC_log ("Fatal error - rec->lock (%s, %d) reset to 0\n", 
						lb->pld->lb_name, i);
		exit (1);
	    }
	}
	rec++;
    }
    return (cnt);
}

/********************************************************************
			
    Description: This function prepares a UN message for sending to  
		a server. The message is sent to the server or saved
		in a buffer for later sending (for better performance
		through fewer messages). This function also
		sets the LB update flag before an UN is sent.

    Input:	lb - the LB involved.
		nr - the nitification request struct matched. If NULL, 
			no new message.
		msg_type - From_lb_write_t.msg_type.
		msg_len - From_lb_write_t.msg_len.
		lbmsgid - From_lb_write_t.lbmsgid.

    Return:	The number of messages saved yet to be sent or a 
		negative LB error number.

********************************************************************/

#define MSG_BUF_SIZE 4

static int Send_to_server (LB_struct *lb, LB_nr_t *nr, 
			int msg_type, int msg_len, LB_id_t lbmsgid)
{
    static From_lb_write_t smsg[MSG_BUF_SIZE];
    static int cnt = 0;
    static unsigned int cr_host;

    if (cnt > 0 &&
	(nr == NULL || nr->host != cr_host || cnt >= MSG_BUF_SIZE)) {
	int ret;
	unsigned int ip;

	if (cr_host == 0xffffffff) {	/* This should not happen */
	    ret = EN_send_to_server (cr_host, (char *)smsg, 
					cnt * sizeof (From_lb_write_t));
	    ip = 0xffffffff;
	    MISC_log ("LB notification to local - tell ROC\n");
	}
	else {
	    ret = RMT_lookup_host_index (RMT_LHI_IX2I, &ip, cr_host);
	    if (ret >= 0)
		ret = EN_send_to_server (ip, (char *)smsg, 
					cnt * sizeof (From_lb_write_t));
	    else			/* This should not happen */
		MISC_log ("Unexpected host in LB NTF record - tell ROC\n");
	}

	cnt = 0;
	if (ret < 0) {		/* register failed host */
	    lb->urhost = ip;
	    EN_print_unreached_host (ip);
/*	    return (ret); */
	}
    }

    if (nr != NULL) {
	unsigned int pid;

	if (!lb->upd_flag_set) {	/* set LB update flag */
	    lb->hd->upd_flag = 1;
	    lb->upd_flag_set = 1;
	}

	smsg[cnt].msg_type = msg_type;
	pid = LB_SHORT_BSWAP (nr->pid);
	pid = Get_big_pid (pid, nr->lock, nr->flag);
	smsg[cnt].pid = LB_T_BSWAP (pid);
	smsg[cnt].fd = nr->fd;
	smsg[cnt].lbmsgid = LB_T_BSWAP (lbmsgid);
	smsg[cnt].msgid = nr->msgid;
	smsg[cnt].msg_len = LB_T_BSWAP (msg_len);
	smsg[cnt].sender_id = LB_SHORT_BSWAP (Sender_id);
	cr_host = nr->host;

	cnt++;
    }

    return (cnt);
}

/********************************************************************
			
    Description: This function gets/sets the UN sending parameters,
		Sender_id and Notify_send, for passing to the remote
		LB_write.

    Input:	un_params - the packed UN parameters.

    Return:	The packed UN parameters if "un_params" = LB_UN_PARAMS_GET.

********************************************************************/

unsigned int LB_UN_parameters (unsigned int un_params)
{
    unsigned short sender_id;
    int notify_send;

    if (un_params == LB_UN_PARAMS_GET) {
	EN_parameters (EN_GET, &sender_id, &notify_send);
	return ((notify_send << 16) | (sender_id & 0xffff));
    }
    else {
	notify_send = un_params >> 16;
	sender_id = un_params & 0xffff;
	EN_parameters (EN_SET, &sender_id, &notify_send);
    }
    return (0);
}

/********************************************************************
			
    Description: This function returns the IP address of the 
		unreachable host while LB_write sends NR messages.

    Input:	lbd - the LB descriptor.

    Returns:	The IP address of unreachable host. On failure, it
		returns 0.

********************************************************************/

unsigned int LB_write_failed_host (int lbd)
{
    LB_struct *lb;
    int err;

    lb = LB_Get_lb_structure (lbd, &err); /* get lb structure */
    if (lb == NULL)
	return (0);

    LB_reset_inuse (lb);
    return (lb->urhost);
}

/********************************************************************
			
    Description: This is an LB service function. It returns the list 
		of current active notification request records. The 
		caller has to free the buffer for returning the list 
		if not NULL.

    Input:	lb - the LB involved.

    Output:	nr_recs - the list of NR records.

    Return:	The number of NR records on success or a negative 
		LB error number.

********************************************************************/

int LB_read_nrs (LB_struct *lb, LB_nr_t **nr_recs)
{
    LB_nr_t *buf;
    int buf_size;
    int n_rec, i, n_nrs, ret;
    LB_nr_t *rec;

    /* perform lock check */
    ret = Check_nr_lock (lb);
    if (ret < 0)
	return (ret);

    n_rec = LB_get_nra_size (lb);
    rec = (LB_nr_t *)(lb->pld->lb_pt + lb->off_nra);

    /* read all nr records */
    n_nrs = 0;
    buf_size = 0;
    buf = NULL;
    for (i = 0; i < n_rec; i++) {
	LB_nr_t *r;

	r = rec + i;
	if (r->host == 0)
	    break;
	if (Get_nr_lock (r->lock) == 0)
	    continue;

	if (n_nrs >= buf_size) {		/* increase the buffer size */
	    int new_size;

	    new_size = n_nrs * 2 + 32;	/* We use this scheme for increasing
					   the buffer size */
	    buf = (LB_nr_t *)realloc (buf, new_size * sizeof (LB_nr_t));
	    if (buf == NULL) 
		return (LB_MALLOC_FAILED);
	    buf_size = new_size;
	}
	memcpy (buf + n_nrs, r, sizeof (LB_nr_t));
	n_nrs++;
    }
    *nr_recs = buf;

    return (n_nrs);
}

/********************************************************************
			
    Returns 1 if "msg" of "length" bytes is identical to message "id"
    in LB "lb". Returns 1 if yes or 0 otherwise. If "id" is LB_ANY,
    the latest message in the LB is checked.

********************************************************************/

static int Check_msg (LB_struct *lb, const char *msg, int length, LB_id_t id) {
    int rpt, page, ret;
    char *buf;
    LB_id_t msg_id;

    if (id == LB_ANY)
	msg_id = LB_LATEST;
    else
	msg_id = id;
    ret = LB_search_for_message (lb, msg_id, 0, &rpt, &page);
    if (ret >= 0) {
	LB_struct tlb;
	memcpy (&tlb, lb, sizeof (LB_struct));
	ret = LB_read_a_message (&tlb, &buf, LB_ALLOC_BUF, rpt, page);
	if (ret > 0) {
	    if (ret == length && memcmp (buf, msg, length) == 0) {
		free (buf);
		return (1);
	    }
	    free (buf);
	}
    }
    return (0);
}


