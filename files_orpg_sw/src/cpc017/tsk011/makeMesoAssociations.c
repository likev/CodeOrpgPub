/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:17:28 $
 * $Id: makeMesoAssociations.c,v 1.2 2003/07/11 19:17:28 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         makeMesoAssociations.c			              *
 *	Author:		Brian Klein					      *
 *   	Created:	Feb. 19, 2003					      *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file contains logic taken from the               *
 *                      NSSL MDA Tracking algorithm.                          *
 *									      *
 *      Notes:       	                                                      *
 ******************************************************************************/

#include <math.h>
#include "mdattnn_params.h"
#include "rpgc.h"

void makeMesoAssociations(const cplt_t   new_cplt,
                          const cplt_t   old_cplt[],
                          const int      nbr_old_cplts,
                          const int      time_diff,
                                float    n2o_dist[],
                                int      n2o_order[])
{
      int   j, l;         /* loop indices                                 */
      float u, v;         /* average motion (km/msec) i.e. (m/sec)        */
      float diff_z;       /* difference in bases of features between scans*/
      float t_a_th;       /* time association distance threshold (km)     */
      float max_dist_thr; /* maximum time association distance (km)       */
      float spd, dist;

/*    Ensure the time difference is reasonable.                           */

      if (time_diff == BAD_TIME || time_diff < 0) return;

/*    Compute the maximum distance threshold (in kilometers)              */

      max_dist_thr = (float)time_diff * MESO_MAX_SPD / SECPERMIN;
      
      if (new_cplt.strength_rank > 0 &&
          new_cplt.circ_class    != SHALLOW_CIRC) {

        l = 0;

        t_a_th = 2.0; /* Initial search radius in km */

        do {

          for (j = 0; j < nbr_old_cplts; j++) {

            if (old_cplt[j].strength_rank == 0            ||
                old_cplt[j].circ_class    <= SHALLOW_CIRC ||
                n2o_dist[j]               != UNDEFINED) continue;

/*               NOTE:  dist calculation moved down a few lines.             */

/*               Perform a vertical continuity check.  Ensure the difference */
/*               in the features bases is within the threshold distance.     */

                 if ((new_cplt.strength_rank >= MESO_SR_THRESH) &&
                     (new_cplt.strength_rank != UNDEFINED) &&
                     (new_cplt.nssl_base     != UNDEFINED))
                  diff_z = (float)(fabs(new_cplt.nssl_base) -
                                   fabs(old_cplt[j].nssl_base));
               
                 else if (new_cplt.core_base != UNDEFINED)
                  diff_z = (float)(fabs((double)new_cplt.core_base) -
                                   fabs((double)old_cplt[j].core_base));
               
                 else
                  diff_z = (float)(fabs(new_cplt.base) -
                                   fabs(old_cplt[j].base));
                

/*C**************************************************************************/
/*C  Compute average motion vector (spd) and if greater than MESO_MAX_SPD, **/
/*C  do not use this association as a candidate.                           **/
/*C**************************************************************************/

                 u = (new_cplt.llx - old_cplt[j].llx) * KILO / (float)time_diff;
                 v = (new_cplt.lly - old_cplt[j].lly) * KILO / (float)time_diff;

                 if (old_cplt[j].u_motion != UNDEFINED) {
                   u = (u + old_cplt[j].u_motion) * HALF;
                   v = (v + old_cplt[j].v_motion) * HALF;
                 }

                 spd = sqrt(u*u + v*v);

                 if (spd > MESO_MAX_SPD * KILO / SECPERMIN) continue;

/*C***************************************************************************/
/*C  Compute distance between new detection and old detection's first guess **/
/*C  position.                                                              **/
/*C***************************************************************************/
/*               This dist calculation was moved here for performance        */
/*               reasons.  Why compute it if spd check above fails?          */

                 dist = sqrt((new_cplt.llx - old_cplt[j].fgx) *
                             (new_cplt.llx - old_cplt[j].fgx) +
                             (new_cplt.lly - old_cplt[j].fgy) *
                             (new_cplt.lly - old_cplt[j].fgy) );

/*C**********************************************************************/
/*C  Time associate by finding the strongest old detection whose first **/
/*C  guess position is within the T_A_TH distance threshold of the     **/
/*C  current new detction.  This would be the first old detection      **/
/*C  encountered in the loop since they have been sorted by strength.  **/
/*C**********************************************************************/

                 if (dist <= t_a_th && fabs(diff_z) <= THRESH_VERT_DIST) {
                   n2o_dist[j]  = dist;
                   n2o_order[l] = j;    /* Note these two lines are          */
                   l++;                 /* reversed in order intentionally.  */
                 } /* end if distance within threshold */

               } /* end for all old detections */

               t_a_th += RADIUS_INCREMENT; /* Increment the search radius    */

/*             Because the threshold (t_a_th) is initialized as an integer   */
/*             and incremented with an integer but compared with a float     */
/*             (max_dist_thr), we need to account for the last iteration     */
/*             and set them equal or couplets within 1 km of the             */
/*             radius won't be found.                                        */
/*             To fix this, once our radius is beyond the max value but      */
/*             within the increment value, just set it to the max value.     */

/***               if ((t_a_th > max_dist_thr && 
                  ((t_a_th - max_dist_thr) < RADIUS_INCREMENT ))
                  t_a_th = max_dist_thr;***/

             } while (t_a_th <= floor(max_dist_thr+0.5)); /* end do while */

           } /* end if circulation type and strength qualify */

      return;

} /* end of function makeMesoAssociations() */

