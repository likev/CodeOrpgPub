/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:17:50 $
 * $Id: mda1d_shear_attributes.c,v 1.2 2003/07/11 19:17:50 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_shear_attributes.c                                    *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This module calculate the shear attributes:	      *
 * 			velocity difference, arc length, shear.		      *
 *									      *
 *      Input:                                                                *
 *        beg_vel - the begining velocity                                     *
 *        end_vel - the end velocity                                          *
 *        beg_azm  - begining azimuth                                         *
 *        end_azm  - end azimuth                                              *
 *        range_vel  - the range for the velocity data                        *
 *                                                                            *
 *      Output:                                                               *
 *        vel_diff - velocity difference                                      *
 *        arc_length - arc_length of the shear vector                         *
 *        shear - the strength of the shear vector                            *
 *      Return:         none                                                  *
 *      Global:         none                                                  *
 *      notes:          none                                                  *
 ******************************************************************************/

#include <stdio.h>
#include <math.h>

#include "mda1d_parameter.h"

void mda1d_shear_attributes(double beg_vel, double end_vel,
				double beg_azm, double end_azm, double range_vel,
				double *vel_diff, double *arc_length, double *shear)
{
	/* declare the local variable */
	double azm_diff;

        if (HEM == 1)
          *vel_diff = end_vel - beg_vel;
  	else
	  *vel_diff = beg_vel - end_vel;

	/* calculate the azimuth difference */
	azm_diff = fabs(end_azm - beg_azm);
        if (azm_diff > 180.0)
	 azm_diff = 360.0 - azm_diff;

	/* calculate the arc length */
	*arc_length = azm_diff * DEGREE_TO_RADIAN * range_vel; 
   
        /* calculate shear strength of the shear vector */
	if (*arc_length != 0)
	 *shear = (*vel_diff) / (*arc_length);
 	else
         *shear = 0;

}/* END of the function */
