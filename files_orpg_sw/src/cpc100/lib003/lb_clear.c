/****************************************************************
		
    Module: lb_clear.c	
				
    Description: This module contains the LB_clear function.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/06/14 18:57:51 $
 * $Id: lb_clear.c,v 1.39 2012/06/14 18:57:51 jing Exp $
 * $Revision: 1.39 $
 * $State: Exp $
 * $Log: lb_clear.c,v $
 * Revision 1.39  2012/06/14 18:57:51  jing
 * Update
 *
 * Revision 1.34  2002/03/12 16:51:19  jing
 * Update
 *
 * Revision 1.33  1999/08/05 16:22:13  jing
 * @
 *
 * Revision 1.31  1999/06/29 21:18:33  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.29  1999/05/27 02:26:35  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.26  1999/05/03 20:57:13  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.16  1998/06/19 21:18:41  Jing
 * posix update
 *
 * Revision 1.15  1998/05/29 22:15:05  jing
 * NO COMMENT SUPPLIED
 *
 * Revision 1.3  1996/08/22 13:06:18  cm
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
#include "lb_def.h"


/* static functions */



/********************************************************************
			
    Description: This function removes "nrms" oldest unexpired 
		messages in LB "lbd". 

    Input:	lbd - LB descriptor
		nrms - number of messages to remove

    Returns:	This function returns the number of messages removed 
		or a negative number to indicate an error condition.

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

		One can not clear an LB by simply resetting the new 
		message pointer to 0 because the page number can not 
		be reset atomically.

********************************************************************/

int 
LB_clear (
    int lbd, 		/* LB descriptor */
    int nrms		/* number of messages to remove */
)
{
    LB_struct *lb;
    int nmsgs, ret;
    unsigned int num_ptr;

    if (nrms <= 0)
	return (0);

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

    if ((lb->flags & LB_DB) || lb->ma_size == 0)
	lb->hd->sms_ok = 0;

    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);

    if (nrms > nmsgs)
	nrms = nmsgs;

    if (lb->off_nra > 0 &&
	(ret = LB_process_nr (lb, LB_MSG_EXPIRED, 0, 0, nrms, 0)) < 0)
	    return (LB_Unlock_return (lb, ret));
    lb->hd->lb_time = time (NULL);

    LB_Update_pointer (lb, 0, nrms);

    if (nrms + 1 >= nmsgs)	/* this is a sufficient (but not necessary) 
				   condition to set non_dec_id */
	lb->hd->non_dec_id |= 1;

    if (lb->ma_size == 0)	/* free file spaces */
	LB_sms_free_space (lb, -1, 0);	/* force a sms table update */

    return (LB_Unlock_return (lb, nrms));
}



