/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:17:25 $
 * $Id: getAverageMotion.c,v 1.2 2003/07/11 19:17:25 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         getAverageMotion.c			              *
 *	Author:		Brian Klein					      *
 *   	Created:	Feb. 19, 2003					      *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file contains logic taken from the               *
 *                      NSSL MDA Tracking algorithm.                          *
 *									      *
 *      Notes:       	Units for motions and speeds are meters per second.   *
 ******************************************************************************/

#include "mdattnn_params.h"

void getAverageMotion(const cplt_t* old_cplt,
                      const int     nbr_old_cplts,
                            float*  avg_mu,
                            float*  avg_mv)
{
     int    i, j;
     float  su, sv;

     j  = 0;
     su = 0.0;
     sv = 0.0;

     /* Sum u and v motions for each 3D couplet.     */
     
     for (i = 0; i < nbr_old_cplts; i++){
        if (old_cplt[i].u_motion != UNDEFINED) {
           j++;
           su += old_cplt[i].u_motion;
           sv += old_cplt[i].v_motion;
        } /* end if */
     } /* end for */

     /* Compute average motion or set as undefined.  */
     
     if (j > 0) {
       *avg_mu = su / (float)j;
       *avg_mv = sv / (float)j;
     } else {
       *avg_mu = UNDEFINED;
       *avg_mv = UNDEFINED;
     } /* end if */      
     return;
}
