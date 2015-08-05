/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/12/28 19:33:08 $
 * $Id: makeTVSAssociation.c,v 1.4 2004/12/28 19:33:08 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/******************************************************************************
 *	Module:         makeTVSAssociations.c			              *
 *	Author:		Brian Klein					      *
 *   	Created:	Feb. 19, 2003					      *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file contains logic taken from the               *
 *                      NSSL MDA Tracking algorithm.                          *
 *									      *
 *      Notes:       	Returns 1 if TVS associated.                          *
 ******************************************************************************/

#define DOUBLE

#include <math.h>
#include "mdattnn_params.h"
#include "rpgcs_coordinates.h"

int  makeTVSAssociation(const cplt_t*      new_cplt,
                        const polar_loc_t* tvs,
                        const int          nbr_tvs)
{
      int    j, l;         /* loop indices                                 */
      float  dist;
      double tvs_x, tvs_y;

      l = 0;
      tvs_x = 0.0;
      tvs_y = 0.0;

      for (j = 0; j < nbr_tvs; j++) {

/*      Convert the TVS locations from polar to cartesian.             */

        RPGCS_azran_to_xy(tvs[j].range, tvs[j].azimuth, &tvs_x, &tvs_y);
        
/*C***************************************************************************/
/*C  Compute distance between new detection and the TVSes from this volume  **/
/*C***************************************************************************/

        dist = sqrt((new_cplt->llx - tvs_x) * (new_cplt->llx - tvs_x) +
                    (new_cplt->lly - tvs_y) * (new_cplt->lly - tvs_y) );
                    
        if (dist <= ((new_cplt->ll_diam/2.) + MAX_TDA_DIST))  return (int)1;

     } /* end for all TVSes */

     return (int)0;

} /* end of function makeTVSAssociation() */

