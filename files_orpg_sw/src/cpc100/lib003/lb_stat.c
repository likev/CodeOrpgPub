/****************************************************************
		
    Module: lb_stat.c	
				
    Description: This module contains the functions that maintain
	and check the LB update info.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2011/05/19 19:35:09 $
 * $Id: lb_stat.c,v 1.40 2011/05/19 19:35:09 jing Exp $
 * $Revision: 1.40 $
 * $State: Exp $
 * $Log: lb_stat.c,v $
 * Revision 1.40  2011/05/19 19:35:09  jing
 * Update
 *
 * Revision 1.36  2002/03/12 16:51:43  jing
 * Update
 *
 * Revision 1.35  1999/08/29 20:18:06  jing
 * @
 *
 * Revision 1.32  1999/06/29 21:20:01  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.30  1999/05/27 02:27:30  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.27  1999/05/03 20:58:11  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.15  1998/06/19 21:18:47  Jing
 * posix update
 *
 * Revision 1.14  1998/05/29 22:16:15  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/22 13:07:56  cm
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
#include <string.h>

/* Local include files */

#include <lb.h>
#include "lb_def.h"

#define LB_MAX_MSGSTAT 500

typedef struct {		/* save update status information */
    unsigned int num_pt;	/* the LB num_pt value */
    int page;			/* the LB page number */
    int n_check;		/* The number of messages to check */
    int *check_list;		/* messages to check (slot pointers) */
    lb_t *ucnt;			/* the LB ucnt array (not byte swapped) */
} Status_save_t;

/* static functions */


/********************************************************************
			
    Description: This function intializes the LB update status 
		structure for LB "lb".

    Input:	lb - the open LB involved;

    Returns:	This function returns LB_SUCCESS on success or a 
		negative error number. 

********************************************************************/

int LB_stat_init (LB_struct *lb)
{
    Status_save_t *st_save;

    lb->stat_info = NULL;
    st_save = (Status_save_t *)malloc (sizeof (Status_save_t));
    if (st_save == NULL)
	return (LB_MALLOC_FAILED);

    st_save->num_pt = lb->hd->num_pt;
    st_save->page = lb->hd->ptr_page;
    st_save->n_check = 0;
    st_save->check_list = NULL;

    if ((lb->flags & LB_DB) && lb->maxn_msgs <= LB_MAX_MSGSTAT) {
	int i;

	st_save->ucnt = (lb_t *)malloc (lb->n_slots * sizeof (lb_t));
	if (st_save->ucnt == NULL) {
	    free (st_save);
	    return (LB_MALLOC_FAILED);
	}

	for (i = 0; i < lb->n_slots; i++)	/* init the saved ucnt array */
	    st_save->ucnt[i] = 0xffffffff;
    }
    else {
	st_save->ucnt = NULL;
    }
	    
    lb->stat_info = (void *)st_save;

   return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function frees the LB update status structure 
		for LB "lb".

    Input:	lb - the open LB involved;

    Returns:	This function returns LB_SUCCESS on success or a 
		negative error number. 

********************************************************************/

int LB_stat_free (LB_struct *lb)
{

    if (lb->stat_info != NULL) {
	Status_save_t *st_save;

	st_save = (Status_save_t *)lb->stat_info;
	if (st_save->check_list != NULL)
	    free (st_save->check_list);
	if (st_save->ucnt != NULL)
	    free (st_save->ucnt);
	free (lb->stat_info);
	lb->stat_info = NULL;
    }

    return (LB_SUCCESS);
}
 
/********************************************************************
			
    Description: This function updates the LB update status info. It
		is called by LB_read.

    Input:	lb - the open LB involved;
		num_pt - the message's num_pt;
		ucnt - the message's ucnt;
		ind - the message's slot index;

********************************************************************/

void LB_stat_update (LB_struct *lb, int num_pt, int ucnt, int ind)
{
    Status_save_t *st_save;

    st_save = (Status_save_t *)lb->stat_info;
    st_save->num_pt = num_pt;
    st_save->page = lb->hd->ptr_page;
    if (st_save->ucnt != NULL)
	st_save->ucnt[ind] = ucnt;

    return;
}

/********************************************************************
			
    Description: This function returns, in argument "status", the 
		status information of LB "lbd".

    Input:	lbd - the LB descriptor;
		status - the LB_status data structure.

    Output:	status - the LB_status data structure.

    Returns:	This function returns LB_SUCCESS on success or a 
		negative error number. 

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

		Any dynamic field read from the file, such as 
		lb->hd->ptr_page  and lb->ucnt[], can only be read 
		once. This is necessary in order to guarantee that 
		no update event is ever missed by LB_stat.

********************************************************************/

int LB_stat (int lbd, LB_status *status)
{
    LB_struct *lb;
    unsigned int num_ptr;
    int page;
    Status_save_t *st_save;
    int i, ret, cnt;

    /* get the LB structure */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);
    if ((ret = LB_lock_mmap (lb, READ_PERM, NO_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));

    if (status->attr != NULL) {		/* LB_attr info */
	LB_attr *attr;

	attr = status->attr;
	memcpy (attr->remark, lb->pld->lb_pt, LB_REMARK_LENGTH);
	attr->mode = lb->hd->acc_mode;
	attr->msg_size = lb->hd->msg_size;
	attr->maxn_msgs = lb->hd->n_msgs;
	attr->types = lb->hd->lb_types & (~3);
	attr->tag_size = ((lb->hd->nra_size * LB_NR_FACTOR) << NRA_SIZE_SHIFT)
						 | lb->hd->tag_size;
	attr->version = GET_VERSION (lb->flags);
    }

    status->time = lb->hd->lb_time;

    num_ptr = lb->hd->num_pt;
    status->n_msgs = GET_NMSG (num_ptr);
    page = lb->hd->ptr_page;

    /* the update info */
    st_save = (Status_save_t *)lb->stat_info;
    if (st_save->ucnt == NULL && status->n_check != 0)
	return (LB_Unlock_return (lb, LB_N_CHECK_ERROR));
    if (st_save->num_pt != num_ptr || 
	LB_Get_corrected_page_number (lb, 
		GET_POINTER(st_save->num_pt), st_save->page) != 
	LB_Get_corrected_page_number (lb, GET_POINTER(num_ptr), page))
	status->updated = LB_TRUE;
    else
	status->updated = LB_FALSE;

    if (status->n_check > st_save->n_check) {
	if (st_save->check_list != NULL)
	    free (st_save->check_list);
	st_save->n_check = 0;
	st_save->check_list = (int *)malloc (status->n_check * sizeof (int));
	if (st_save->check_list == NULL)
	    return (LB_Unlock_return (lb, LB_MALLOC_FAILED));
    }

    /* process check_list */
    cnt = 0;
    for (i = 0; i < status->n_check; i++) {
	int pt, pg;

	if (LB_Search_msg (lb, status->check_list[i].id, 
						&pt, &pg) != LB_SUCCESS)
	    status->check_list[i].status = LB_MSG_NOT_FOUND;
	else {
	    int ind;

	    ind = pt % lb->n_slots;
	    if (st_save->ucnt[ind] == lb->dir[ind].loc)
		status->check_list[i].status = LB_MSG_NOCHANGE;
	    else {
	        status->check_list[i].status = LB_MSG_UPDATED;
	        status->updated = LB_TRUE;
	    }
	    st_save->check_list[cnt] = ind;
	    cnt++;
	}
    }
    st_save->n_check = cnt;

   return (LB_Unlock_return (lb, LB_SUCCESS));
}

