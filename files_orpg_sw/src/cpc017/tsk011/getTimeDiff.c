/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/04/02 21:30:10 $
 * $Id: getTimeDiff.c,v 1.4 2004/04/02 21:30:10 cheryls Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/******************************************************************************
 *	Module:         getTimeDiff.c					      *
 *	Author:		Brian Klein					      *
 *   	Created:	Feb. 19, 2003					      *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file contains logic taken from the               *
 *                      NSSL MDA Tracking algorithm.                          *
 *									      *
 *      Notes:       	Returns BAD_TIME if first volume or difference between*
 *                      volumes is too great.                                 *
 ******************************************************************************/
#include "mdattnn_params.h"

int getTimeDiff(const int new_time, const int new_date,
                const int old_time, const int old_date)
{
        int time_diff;

        if (old_date == 0) return BAD_TIME;  /* Must be the first volume */
        
        if (old_date > new_date) return BAD_TIME; /* Must be going backwards */

        if (new_date == old_date)
           time_diff = new_time - old_time;
        else
           time_diff = SECPERDAY - old_time + new_time;
   
        if (time_diff < 0) return BAD_TIME; /* Must be going backwards in same day*/   

        /* Make sure too much time hasn't elapsed between volumes */

        if (time_diff > MESO_MAX_DT * SECPERMIN) time_diff = BAD_TIME;
        
        return time_diff;

} /* end of function getTimeDiff() */

