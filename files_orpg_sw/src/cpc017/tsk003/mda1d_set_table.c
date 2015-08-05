/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:18:07 $
 * $Id: mda1d_set_table.c,v 1.3 2005/03/03 22:18:07 ryans Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_set_table.c                                    *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This module sets the look-up tables for               *
 *  			strength ranks and the length of look-ahead mode      *
 *									      *
 *      Input:                                                                *
 *        range_vel[] - the bin range for velocity data                       *
 *      Output:                                                               *
 *        meso_vd_thr[][] - the threshold of vefl diff in strength ranks      *
 *        meso_shr_thr[][] - the threshold of shear in strength ranks         *
 *        meso_max_ahd[] - the max look-ahead mode length, function of range  *
 *        old_velocity - updated old_vel                                      *
 *      Return:         none                                                  *
 *      Global:         none                                                  *
 *      notes:          none                                                  *
 ******************************************************************************/

#include <stdio.h>
#include <math.h>
#include "mda1d_parameter.h"

#include <mda_adapt.h>
#define EXTERN extern /* prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"

#define 	MAX_RANK_RANGE  	30      /* Range of the rank*/
#define		RANGE_1			25.0    /* 25.0 km */
#define		RANGE_2			150.0   /* 150.0 km */
#define         SIX			6       /* just a number of 6 */
#define         TEN			10.0    /* just a number of 10.0 */
#define         FIVE			5.0     /* just a number of 5.0 */
#define         THREE			3.0     /* just a number of 3.0 */
#define         ZPSF			0.75    /* just a number of 0.75 */

/* acknowledge the global variables declared outside */
        extern double meso_vd_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        extern double meso_shr_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        extern int meso_max_ahd[BASEDATA_DOP_SIZE];


void mda1d_set_table(double range_vel[])
{
	/* declare the local variables */
	int i, j;
        double thr_vd_range_1, thr_shr_range_1;
        double thr_vd_range_2, thr_shr_range_2;
	
	/* compute range-dependent velocity difference, shear, and look-ahead*
         * thresholds for each range bin and each MDA rank */
 	for (i = 0; i < BASEDATA_DOP_SIZE; i++)
	 {
	  for (j = MESO_MIN_RANK; j <= MESO_MAX_RANK; j++)
           {
            thr_vd_range_1 = TEN + (j-1) * FIVE;
	    thr_shr_range_1 = THREE + (j-1) * ZPSF;

	    thr_vd_range_2 = thr_vd_range_1 * ZPSF;
	    thr_shr_range_2 = thr_shr_range_1 * 0.5;
 
            if (range_vel[i] <= mda_adapt.meso_vs_rng_1)
             {
       	      meso_vd_thr[i][j] = thr_vd_range_1;
       	      meso_shr_thr[i][j] = thr_shr_range_1;
             } /* END of if (range_vel[i] <= mda_adapt.meso_vs_rng_1) */ 
            else if(range_vel[i] >= mda_adapt.meso_vs_rng_2)
             {/* we are at the second range area 100 ~ 200 */
	      meso_vd_thr[i][j] =  thr_vd_range_2;
              meso_shr_thr[i][j] = thr_shr_range_2;
             }/* END of else if(range_vel[i] <= mda_adapt.meso_vs_rng_2) */
            else /* we are at the third range area */
             {
	      meso_vd_thr[i][j] = thr_vd_range_1 - 
		((thr_vd_range_1 - thr_vd_range_2) /
		 (mda_adapt.meso_vs_rng_2 - mda_adapt.meso_vs_rng_1)) *
		 (range_vel[i] - mda_adapt.meso_vs_rng_1);
              meso_shr_thr[i][j] = thr_shr_range_1 - 
                ((thr_shr_range_1 - thr_shr_range_2) /
                 (mda_adapt.meso_vs_rng_2 - mda_adapt.meso_vs_rng_1)) *
                 (range_vel[i] - mda_adapt.meso_vs_rng_1);

             } /* END of else */
           } /* END of for (j = MESO_MIN_RANK; i <= MESO_MAX_RANK; j++) */
          
          /* set the look-ahead table */
          if (MDA_LOOK_AHD == 1)
           {
	    if (range_vel[i] <= RANGE_1)
	     meso_max_ahd[i] = SIX;
            else if (range_vel[i] >= RANGE_2)
             meso_max_ahd[i] = 1;
  	    else
             meso_max_ahd[i] = (int)(RANGE_2 / range_vel[i] + 0.5);
           } /* END of if (MDA_LOOK_AHD == 1) */
          else
           meso_max_ahd[i] = 0;
         } /* END of for (i = 0; i < BASEDATA_DOP_SIZE; i++) */

} /* END of the function */
