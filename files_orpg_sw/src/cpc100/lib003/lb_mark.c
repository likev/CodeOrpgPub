/****************************************************************
		
    Module: lb_mark.c	
				
    Description: This module contains the message tag (mark)
		related functions.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:44:23 $
 * $Id: lb_mark.c,v 1.35 2004/05/27 16:44:23 jing Exp $
 * $Revision: 1.35 $
 * $State: Exp $
 * $Log: lb_mark.c,v $
 * Revision 1.35  2004/05/27 16:44:23  jing
 * Update
 *
 * Revision 1.33  2002/03/12 16:51:29  jing
 * Update
 *
 * Revision 1.32  2000/08/21 20:49:45  jing
 * @
 *
 * Revision 1.31  1999/08/05 16:22:21  jing
 * @
 *
 * Revision 1.30  1999/06/29 21:19:07  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.28  1999/05/27 02:26:55  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.25  1999/05/03 20:57:35  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.14  1998/06/19 21:18:43  Jing
 * posix update
 *
 * Revision 1.13  1998/05/29 22:15:36  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/22 13:07:12  cm
 * SunOS 5.5 modifications
 *
*/


/* System include files */

#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

/* Local include files */

#include <lb.h>
#include "lb_def.h"

/* static functions */
static int Lb_find_tag (LB_struct *lb, int index, 
		int *shift, lb_t *mask, lb_t **word, int add_db_off);


/********************************************************************
			
    Description: This function assigns the value "tag" to the tag of
		message "id" in the LB "lbd".

    Input:	lbd - LB descriptor;
		id - the message id;
		tag - the tag;

    Returns:	returns LB_SUCCESS on success or a negative 
		number to indicate an error condition.

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

		We have to use an exclusive lock for this function 
		because competing tag updating can cause undefined 
		results.

********************************************************************/

int LB_set_tag (int lbd, LB_id_t id, int tag)
{
    LB_struct *lb;
    int pt, page;
    int ret, ind;

    if (id > LB_MAX_ID)
	return (LB_BAD_ARGUMENT);

    /* get the LB structure, lock and mmap the file */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);
    if ((ret = LB_lock_mmap (lb, WRITE_PERM, EXC_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));

    /* search for the message */
    if (LB_Search_msg (lb, id, &pt, &page) != LB_SUCCESS)
	return (LB_Unlock_return (lb, LB_NOT_FOUND));

    /* update the tag */
    ind = pt % lb->n_slots;
    LB_write_tag (lb, ind, tag, 0);

    return (LB_Unlock_return (lb, LB_SUCCESS));
}

/********************************************************************
			
    Description: This function assigns the value "tag" to the tag
		of message of index "ind" in the LB "lb".

    Input:	lb - LB descriptor;
		ind - the message index;
		tag - the tag;
		add_db_off - additional double buffer offset (0 or 1);

********************************************************************/

void LB_write_tag (LB_struct *lb, int ind, int tag, int add_db_off)
{
    int shift;
    lb_t mask, *word;

    if (Lb_find_tag (lb, ind, &shift, &mask, &word, add_db_off) == LB_SUCCESS)
	*word = LB_T_BSWAP (
		(LB_T_BSWAP (*word) & (~(mask << shift))) | 
		((tag & mask) << shift));

    return;
}

/********************************************************************
			
    Description: This function reads and returns the tag for the message 
		of index "ind" in the LB "lb".

    Input:	lb - LB descriptor;
		ind - the message index;

    Return:	the tag for the message or 0 if there is no tag (tag
		size is 0).

********************************************************************/

lb_t LB_read_tag (LB_struct *lb, int ind)
{
    int shift;
    lb_t mask, *word;

    if (Lb_find_tag (lb, ind, &shift, &mask, &word, 0) == LB_SUCCESS)
	return (lb_t)((LB_T_BSWAP (*word) >> shift) & mask);
    else
	return (0);
}

/********************************************************************
			
    Description: This function finds the tag location for message of
		"index".

    Input:	lb - LB descriptor;
		ind - the message index;
		add_db_off - additional double buffer offset (0 or 1);

    Output:	shift, mask and word - define the tag:
			(*word >> shift) & mask

    Returns:	returns LB_SUCCESS on success or LB_FAILURE 
		on failure.

********************************************************************/

static int Lb_find_tag (LB_struct *lb, int index, 
		int *shift, lb_t *mask, lb_t **word, int add_db_off)
{
    int tag_s, word_s, npw, woff, boff, i;
    lb_t m;

    tag_s = lb->hd->tag_size;
    if (tag_s <= 0 || tag_s > LB_MAXN_TAG_BITS)
	return (LB_FAILURE);
    word_s = sizeof (lb_t) * CHAR_SIZE;

    npw = word_s / tag_s;		/* number of tags per word */
    woff = index / npw;			/* word offset */
    boff = (index % npw) * tag_s;	/* bit offset */

    /* the mask */
    m = 0;
    for (i = 0; i < tag_s; i++)
	m |= (1 << i);

    *shift = boff;
    *mask = m;
    if (lb->flags & LB_DB)
	*word = lb->tag + DB_WORD_OFFSET (woff, 
				lb->dir[index].loc + add_db_off);
    else
	*word = lb->tag + woff;

    return (0);
}

