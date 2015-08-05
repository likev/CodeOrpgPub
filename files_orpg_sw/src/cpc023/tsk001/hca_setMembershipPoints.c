/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:17:01 $
 * $Id: hca_setMembershipPoints.c,v 1.4 2012/03/12 13:17:01 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         hca_setMembershipPoints.c                             *
 *      Author:         Brian Klein, Yukuan Song                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    Sets the membership function definition points for a  *
 *                      given hydr class and fuzzy logic input varible by     *
 *                      setting the x1, x2, x3 and x4 membership function     *
 *                      definition points.  There are simple one-dimensional  *
 *                      membership functions and more complex two-dimensional *
 *                      membership functions.  The later use equations        *
 *                      (f1, f2, f3, g1 or g2) to determine the membership    *
 *                      function points.                                      *
 *                                                                            *
 *      Change History:                                                       *
 *       Jan 2012: NA11-00387 Added partial beam blockage modification. BKlein*
 ******************************************************************************/

#include "hca_local.h"
#include "hca_adapt.h"
#include <rpgc.h>

#define EXTERN extern
#include "hca_adapt_object.h"

 /****************************************************************************
    Description:
       Set the x1, x2, x3 and x4 membership function definition points
    Input:
       f1, f2, f3, g1, g2
    Output:
       Membership points
    Returns:
    Globals:
    Notes:
  ***************************************************************************/
void Hca_setMembershipPoints(int    h_class,   /* (IN) Hydrometeor class    */
			     int    fl_input,  /* (IN) Fuzzy logic input    */
			     float  z_fshield, /* (IN) F-shield adjusted Reflectivity */
			     float  points[]   /* (OUT) Membership points   */)
{
	int   x;
	float eqnValue = 0.0;  /* value of f1, f2 f3, g1 or g2 equation */

        /* CPT&E label A */
	for (x = X1; x < NUM_X; x++) {
	
	   /*  Check the membership flag to see if this is a one or
               two-dimensional membership function.                 */

           /* CPT&E label B */
	   if (membershipFlag[h_class][fl_input][x] == MEMFLAG_NONE) {

	      /*  This is for a one-dimensional membership function */
	      points[x] = membership[h_class][fl_input][x];
   	   }
	   else { 
           /* CPT&E label C */

	   /*  This is for a two-dimensional membership function.
	       The membershipFlag array indicates which equation
	       to use.  The equation value is obtained from a look-up
            table and based on the reflectivity value for this bin.
            The equation value is added to the membership value to
            obtain the actual membership function defintion point. */

	      if      (membershipFlag[h_class][fl_input][x] == MEMFLAG_F1)
              eqnValue = (hca_adapt.f1_a*z_fshield*z_fshield) + (hca_adapt.f1_b*z_fshield) + hca_adapt.f1_c;

	      else if (membershipFlag[h_class][fl_input][x] == MEMFLAG_F2)
              eqnValue = (hca_adapt.f2_a*z_fshield*z_fshield) + (hca_adapt.f2_b*z_fshield) + hca_adapt.f2_c;

	      else if (membershipFlag[h_class][fl_input][x] == MEMFLAG_F3)
              eqnValue = (hca_adapt.f3_a*z_fshield*z_fshield) + (hca_adapt.f3_b*z_fshield) + hca_adapt.f3_c;

	      else if (membershipFlag[h_class][fl_input][x] == MEMFLAG_G1)
              eqnValue =                                  (hca_adapt.g1_b*z_fshield) + hca_adapt.g1_c;

	      else if (membershipFlag[h_class][fl_input][x] == MEMFLAG_G2)
              eqnValue =                                  (hca_adapt.g2_b*z_fshield) + hca_adapt.g2_c;

           else {
              RPGC_log_msg(GL_ERROR,"HCA: Invalid adaptation data: membershipFlag");
              RPGC_abort();
           }

	      /* Now, add the equation value to the value from the membership
	         definition in dea data.                                     */

	      points[x] = eqnValue + membership[h_class][fl_input][x];
	   } /* end else */
	} /* end for all membership function definition points */

/* End of setMembershipPoints() */
} 

