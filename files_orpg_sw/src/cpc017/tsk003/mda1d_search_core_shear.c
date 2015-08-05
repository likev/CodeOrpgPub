/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:18:06 $
 * $Id: mda1d_search_core_shear.c,v 1.3 2005/03/03 22:18:06 ryans Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_search_core_shear.c                                    *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This module check two adjacent azimuthal velocities   *
 *                      to search for cyclonic shear segments. Cyclonic shear *
 *                      is defined as increasing (decreasing) velocity with   *
 *                      increasing azimuth for Northern (Southern) Hemisphere *
 *                      radar sites.                                          *
 *      Input:                                                                *
 *        azimth - the azimuth of current radial                              *
 *        vel[]  - the velocity of current radial                             *
 *        old_azimuth - the azimuth of previous radial                        *
 *        old_vel[]   - the velocity of previous radial                       *
 *        range_vel -  the range for the velocity data                        *
 *        ng_vel -  the number of velocity gates                              *
 *                                                                            *
 *      Output:                                                               *
 *        old_azimuth - updated old_azimuth                                   *
 *        old_velocity - updated old_vel                                      *
 *      Return:         none                                                  *
 *      Global:         none                                                  *
 *      notes:          none                                                  *
 ******************************************************************************/
#include <math.h>
#include <stdio.h>

#include "mda1d_parameter.h"
#include "mda1d_acl.h"

#include <mda_adapt.h>
#define EXTERN extern /* prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"

#define 	MAX_RANK_RANGE  30      /* Range of the rank*/
#define		NINE_NINE_NINE  999.0   


	/* acknowledge the global variables */
        extern double mda_vect_vel[MESO_MAX_LENGTH][BASEDATA_DOP_SIZE];
        extern double mda_vect_azm[MESO_MAX_LENGTH][BASEDATA_DOP_SIZE];
        extern int length_vect[BASEDATA_DOP_SIZE];
        extern double meso_vd_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        extern double meso_shr_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];


void mda1d_search_core_shear(double range_vel[], int range_index,
				double *cs_beg_vel, double *cs_beg_azm,
				double *cs_end_vel, double *cs_end_azm,
				double *cs_vel_diff, double *cs_shear,
				double *cs_rank, double *cs_cart_length)
{
	/* declare the local variables */
	double cs_beg_vel_tmp, cs_end_vel_tmp;
	double cs_beg_azm_tmp, cs_end_azm_tmp;
	double cs_vel_diff_tmp, cs_arc_length_tmp, cs_shear_tmp;
        double cs_arc_length;
	double x1, y1, x2, y2;

        int i, j; /* loop index */

	/* set a boolean variables to determine if the begining/end of core       *
	 * shear has been found: 1 means that the begining of core shear found    *
         *                       0 means that the begining of core shear not found*/
	int found_begining = 0;
	int found_end = 0;

	/* initialize the core shear variables */
	*cs_beg_vel = NINE_NINE_NINE;
	*cs_beg_azm = NINE_NINE_NINE;
	*cs_end_vel = NINE_NINE_NINE;
	*cs_end_azm = NINE_NINE_NINE;
	*cs_cart_length = NINE_NINE_NINE;
	*cs_vel_diff = NINE_NINE_NINE;
	*cs_shear = NINE_NINE_NINE;
	*cs_rank = 0.0;
	cs_beg_vel_tmp = NINE_NINE_NINE;
        cs_beg_azm_tmp = NINE_NINE_NINE;
        cs_end_vel_tmp = NINE_NINE_NINE;
        cs_end_azm_tmp = NINE_NINE_NINE;

	for (i = MESO_MIN_RANK; i <= MESO_MAX_RANK; i++)
         {
	  for (j = 0; j < length_vect[range_index] - 1; j++)
           {
	    /* calculate the gate-to-gate shear */
	    mda1d_shear_attributes(mda_vect_vel[j][range_index], mda_vect_vel[j+1][range_index],
				   mda_vect_azm[j][range_index], mda_vect_azm[j+1][range_index],
                                   range_vel[range_index], &cs_vel_diff_tmp, &cs_arc_length_tmp, 
                                   &cs_shear_tmp);
            if (found_begining == 0)
             { /* the core shear begining has not been found yet */
	      if (cs_shear_tmp >= meso_shr_thr[range_index][i])
	       {
                cs_beg_vel_tmp = mda_vect_vel[j][range_index];
                cs_beg_azm_tmp = mda_vect_azm[j][range_index];
                found_begining = 1; /* since we have found the begining of core shear */
	       }
             } /* END of if (found_begining == 0) */
            else /* we already found the begining of the core shear */
             {
              if (cs_shear_tmp < meso_shr_thr[range_index][i])
               {/* found the end of the core shear */
                cs_end_vel_tmp = mda_vect_vel[j][range_index];
                cs_end_azm_tmp = mda_vect_azm[j][range_index];
                found_end = 1; /* we found the end of core shear */
                break; /* get out of the inter loop */
               }
             } /* END of else if (found_begining == 0) */

           } /* END of for (j = 0; j < length_vect[range_index]; j++) */
          /* to see if we found the end of core; if not, *
           * set the last sample to be the end of core shear */
          if (found_end ==0 && found_begining ==1)
           {
            cs_end_vel_tmp = mda_vect_vel[length_vect[range_index]-1][range_index];
            cs_end_azm_tmp = mda_vect_azm[length_vect[range_index]-1][range_index];
	   }

          /* calculate shear attributes */
	  if (  cs_beg_vel_tmp == NINE_NINE_NINE ||
		cs_beg_azm_tmp == NINE_NINE_NINE ||
	 	cs_end_vel_tmp == NINE_NINE_NINE ||
		cs_end_azm_tmp == NINE_NINE_NINE ) 
	   {/* didn't find the begining of the core shear */
            break; /* get out the loop */
           }
          else /* found the core shear */
	   {
	    /* calculate the cartesian length of a vector */
              x1 = range_vel[range_index] * sin(cs_beg_azm_tmp * DEGREE_TO_RADIAN);
              y1 = range_vel[range_index] * cos(cs_beg_azm_tmp * DEGREE_TO_RADIAN);
              x2 = range_vel[range_index] * sin(cs_end_azm_tmp * DEGREE_TO_RADIAN);
              y2 = range_vel[range_index] * cos(cs_end_azm_tmp * DEGREE_TO_RADIAN);
              *cs_cart_length = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));

	    /* if the cartesian length of the core shear is smaller than *
             * mda_adapt.meso_max_core_len, calculate the attributes of core shear */
            if (*cs_cart_length <= mda_adapt.meso_max_core_len)
	     {
              *cs_beg_vel = cs_beg_vel_tmp;
              *cs_beg_azm = cs_beg_azm_tmp;
              *cs_end_vel = cs_end_vel_tmp;
              *cs_end_azm = cs_end_azm_tmp;
              mda1d_shear_attributes(*cs_beg_vel, *cs_end_vel,
                                   *cs_beg_azm, *cs_end_azm,
                                   range_vel[range_index], cs_vel_diff, &cs_arc_length,
                                   cs_shear);
              /* calculate cs_rank */
	      *cs_rank = (double)i;
              break; /* get out of the loop */
             } /* END of if ( *cs_cart_length <= mda_adapt.meso_max_core_len) */
           } /* END of else if ( cs_beg_vel_tmp == NINE_NINE_NINE || */ 

	  /* reset the "found_begining", "found_end"  to 0 */
          found_begining = 0;
          found_end = 0;

         } /* END of for (i = MESO_MIN_RANK; i <= MESO_MAX_RANK; i++) */	
       
} /* END of this function */
