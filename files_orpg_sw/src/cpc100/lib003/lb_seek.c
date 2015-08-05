/****************************************************************
		
    Module: lb_seek.c	
				
    Description: This module contains the LB_seek function.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/12/09 19:28:31 $
 * $Id: lb_seek.c,v 1.38 2010/12/09 19:28:31 jing Exp $
 * $Revision: 1.38 $
 * $State: Exp $
 * $Log: lb_seek.c,v $
 * Revision 1.38  2010/12/09 19:28:31  jing
 * Update
 *
 * Revision 1.33  1999/08/05 16:22:37  jing
 * @
 *
 * Revision 1.32  1999/06/29 21:19:50  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.30  1999/05/27 02:27:23  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.27  1999/05/03 20:58:04  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.18  1998/06/19 20:57:56  Jing
 * posix update
 *
 * Revision 1.17  1998/06/19 16:31:27  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/22 13:07:41  cm
 * SunOS 5.5 modifications
 *
*/


/* System include files */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

/* Local include files */

#include <lb.h>
#include <misc.h>
#include "lb_def.h"


/* static functions */
static void Read_msg_info (LB_struct *lb, int pt, LB_info *info);
static int Get_number_unread (LB_struct *lb, LB_info *info);


/********************************************************************
			
    Description: This function moves the read pointer for "lbd".

    Input:	lbd - the LB descriptor;
		offset - the amount of pointer movement;
		id - the message id to start from;

    Output:	info - if not NULL, the info of the message pointed by 
		the new read pointer.

    Returns:	This function returns LB_SUCCESS on success or a 
		negative error number. 

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_seek (int lbd, int offset, LB_id_t id, LB_info *info)
{
    LB_struct *lb;
    int new_pt, new_page;
    int ret;

    /* get the LB structure */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);
    if ((ret = LB_lock_mmap (lb, READ_PERM, NO_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));

    /* where to start from */
    ret = LB_search_for_message (lb, id, offset, &new_pt, &new_page);
    if (ret < 0)
	return (LB_Unlock_return (lb, ret));

    lb->rpt = new_pt;
    lb->rpage = new_page;

    /* the message id pointed to by the new read pointer */
    if (info != NULL) {
	int poll, st_time;

	poll = 0;			/* poll count */
	st_time = 0;			/* not needed - turn off gcc warning */
	while (1) {			/* if the message is to come, we poll */

	    ret = LB_Check_pointer (lb, new_pt, new_page);
	    if (ret == RP_EXPIRED) {
		info->size = LB_SEEK_EXPIRED;
		break;
	    }
	    else if (ret == RP_TO_COME) {
		int poll_period;

		info->size = LB_SEEK_TO_COME;
		if (lb->max_poll <= 0)
		    break;
	        poll_period = 1000 / lb->wait_time + 1;
			/* time check is needed every poll_period polls */
		if (poll == 0) 
		    st_time = MISC_systime (NULL);
		else if ((poll % poll_period) == poll_period - 1 &&
		 	 MISC_systime (NULL) - st_time > lb->max_poll)
		    break;
		msleep (lb->wait_time);
		poll++;
	    }
	    else {

		Read_msg_info (lb, new_pt, info);
		break;
	    }
	}
    }

    return (LB_Unlock_return (lb, LB_SUCCESS));
}

/********************************************************************
			
    Description: This function returns the info of message "id" in
		LB "lbd".

    Input:	lbd - the LB descriptor;
		id - the message id;

    Output:	info - the info of the message.

    Returns:	This function returns LB_SUCCESS on success or a 
		negative error number. 

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_msg_info (int lbd, LB_id_t id, LB_info *info)
{
    LB_struct *lb;
    int pt, page;
    int ret;

    /* get the LB structure */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);
    if ((ret = LB_lock_mmap (lb, READ_PERM, NO_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));

    if (id == LB_N_UNREAD)
	return (LB_Unlock_return (lb, Get_number_unread (lb, info)));

    /* where to start from */
    ret = LB_search_for_message (lb, id, 0, &pt, &page);
    if (ret < 0)
	return (LB_Unlock_return (lb, ret));

    ret = LB_Check_pointer (lb, pt, page);
    if (ret == RP_EXPIRED)
	return (LB_Unlock_return (lb, LB_EXPIRED));
    if (ret == RP_TO_COME)
	return (LB_Unlock_return (lb, LB_TO_COME));

    Read_msg_info (lb, pt, info);
    return (LB_Unlock_return (lb, LB_SUCCESS));
}

/********************************************************************
			
    Description: This function seaches for a message that is offseted
		by "offset" messages in the LB in terms of the message
		of ID "id". "id" can also take values of LB_CURRENT, 
		LB_LATEST or LB_FIRST. When "id" is not explicitly 
		given (e.g. LB_CURRENT etc), the existance of the 
		message is not checked.

    Input:	lb - the LB struct;
		id - the message id to search;
		offset - pointer offset;

    Output:	pt - the message pointer;
		page - the message page number;

    Returns:	This function returns LB_SUCCESS on success or a 
		negative error number if the message is not found. 

********************************************************************/

int LB_search_for_message (LB_struct *lb, LB_id_t id, int offset,
						int *pt, int *page)
{
    int inc;
    int ref_pt, ref_page;

    inc = offset;		/* pointer increment value */
    if (id == LB_CURRENT) {
	ref_pt = lb->rpt;	
	ref_page = lb->rpage;
    }
    else if (id == LB_LATEST || id == LB_FIRST) {
	unsigned int num_ptr;
	int nmsgs;

	num_ptr = lb->hd->num_pt;
	nmsgs = GET_NMSG (num_ptr);
	ref_pt = GET_POINTER(num_ptr);
	ref_page = lb->hd->ptr_page;
	ref_page = LB_Get_corrected_page_number (lb, ref_pt, ref_page);

	if (id == LB_FIRST)
	    inc -= nmsgs;
	else
	    inc--;
	if (nmsgs == 0)
	    inc = 0;
    }
    else {
	if (LB_Search_msg (lb, id, &ref_pt, &ref_page) != LB_SUCCESS)
	    return (LB_NOT_FOUND);
    }
    *pt = LB_Compute_pointer (lb, ref_pt, ref_page, inc, page);

    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function reads the message info pointed by "pt".

    Input:	lb - the LB structure;
		pt - pointer to the message;

    Output:	info - the info of the message.

********************************************************************/

static void Read_msg_info (LB_struct *lb, int pt, LB_info *info)
{

    int ind;

    ind = pt % lb->n_slots;
    info->size = LB_Get_message_info (lb, pt, NULL, &(info->id));
    info->mark = LB_read_tag (lb, ind);

    return;
}

/********************************************************************
			
    Returns with "info" the number of unread messages and the max n msgs.

********************************************************************/

static int Get_number_unread (LB_struct *lb, LB_info *info) {
    int r_pt, r_page, nmsgs, w_pt, w_page, unread;
    unsigned int num_ptr;

    r_pt = lb->rpt;	
    r_page = LB_Get_corrected_page_number (lb, r_pt, lb->rpage);
    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    w_pt = GET_POINTER(num_ptr);
    w_page = LB_Get_corrected_page_number (lb, w_pt, lb->hd->ptr_page);
    unread = w_pt - r_pt;
    while (unread < 0)
	unread += lb->ptr_range;
    if (r_page + 2 < w_page || unread > nmsgs)
	unread = nmsgs;
    info->id = unread;
    info->size = lb->maxn_msgs;
    return (0);
}
