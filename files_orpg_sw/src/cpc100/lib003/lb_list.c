/****************************************************************
		
    Module: lb_list.c	
				
    Description: This module contains the LB_list function.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/05/27 16:44:10 $
 * $Id: lb_list.c,v 1.33 2004/05/27 16:44:10 jing Exp $
 * $Revision: 1.33 $
 * $State: Exp $
 * $Log: lb_list.c,v $
 * Revision 1.33  2004/05/27 16:44:10  jing
 * Update
 *
 * Revision 1.32  1999/08/05 16:22:18  jing
 * @
 *
 * Revision 1.31  1999/06/29 21:18:56  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.29  1999/05/27 02:26:49  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.26  1999/05/03 20:57:28  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.17  1998/06/19 21:18:42  Jing
 * posix update
 *
 * Revision 1.16  1998/05/29 22:15:26  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.2  1996/08/22 13:06:57  cm
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

/* Local include files */

#include <lb.h>
#include "lb_def.h"


/* static functions */


/********************************************************************
			
    Description: This function returns, in "list", information about
		the latest "nlist" messages in the LB "lbd".

    Input:	lbd - the LB descriptor;
		nlist - length of the list to return;

    Output:	list - a list of LB_info structures.

    Returns:	This function returns the number of messages found. If no 
		message is found, it returns 0. It returns a negative 
		error number on failure. 

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_list (int lbd, LB_info *list, int nlist)
{
    LB_struct *lb;
    unsigned int num_ptr;
    LB_dir *dir;
    int ptr, nmsgs, page;
    int ppt;
    int i, ret;
    int cnt;

    if (nlist < 0 || list == NULL)	/* check arguments */
	return (LB_BAD_ARGUMENT);

    if (nlist == 0)
	return (0);

    /* get the LB structure */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);
    if ((ret = LB_lock_mmap (lb, READ_PERM, NO_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));

    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    ptr = GET_POINTER(num_ptr);		/* point to the new message */
    page = lb->hd->ptr_page;
    page = LB_Get_corrected_page_number (lb, ptr, page);

    dir = lb->dir;

    if (nmsgs < nlist)
	nlist = nmsgs;
    ppt = ptr - nlist;		/* point to the first interested msg */
    if (ppt < 0)
	ppt += lb->ptr_range;

    /* go through the msg directory */
    for (i = 0; i < nlist; i++) {
	int ind;

	ind = ppt % lb->n_slots;
	list[i].id = dir[ind].id;
	list[i].size = LB_Get_message_info (lb, ppt, NULL, NULL);
	list[i].mark = LB_read_tag (lb, ind);
	ppt++;
    }

    /* remove expired msgs since the dir contents may be corrupted for those */

    ptr = LB_Compute_pointer (lb, ptr, page, -nlist, &page);	/* find the 
			pointer and page for the first list message */

    cnt = 0;
    while (cnt < nlist &&
	   LB_Check_pointer (lb, ptr, page) == RP_EXPIRED) {
	cnt++;
	ptr = LB_Compute_pointer (lb, ptr, page, 1, &page);
    }

    if (cnt > 0) {
	for (i = 0; i < nlist - cnt; i++)
	    list [i] = list [i + cnt];
    }

    return (LB_Unlock_return (lb, nlist - cnt));
}


