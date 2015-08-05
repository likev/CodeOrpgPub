/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:19:19 $
 * $Id: mda2d_set_table.c,v 1.3 2005/03/03 22:19:19 ryans Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda2d_set_table.c                                    *
 *      Author:         Yukuan Song                                           *
 *      Created:        Oct. 31, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This module sets the look-up tables for               *
 *  			strength ranks.					      *
 *									      *
 *      Input:                                                                *
 *      Output:                                                               *
 *        meso_vd_thr[][] - the threshold of vefl diff in strength ranks      *
 *        meso_shr_thr[][] - the threshold of shear in strength ranks         *
 *      notes:          none                                                  *
 ******************************************************************************/

#include <stdio.h>
#include <math.h>
#include "mda2d_parameter.h"

#include <mda_adapt.h>
#define EXTERN extern /* prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"

#define		BIN_SIZE	0.25 /* the bin size */
#define		HALF_BIN_SIZE	0.125 /*the half of bin size */
#define		TEN		10.0   /* just anumber 10.0 */
#define		FIVE		5.0    /* just a number of 5.0 */
#define		THREE		3.0    /* just a number of 3.0 */
#define         ZPSF		0.75   /* just a number 0.75 */

/* acknowledge the global variables declared outside */
        extern float meso_vd_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        extern float meso_shr_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];

void mda2d_set_table()
{
	/* declare the local variables */
	int i, j;
        float range_vel[BASEDATA_DOP_SIZE];
        float thr_vd_range_1, thr_shr_range_1;
        float thr_vd_range_2, thr_shr_range_2;
	
	/* compute range-dependent velocity difference, shear, and look-ahead*
         * thresholds for each range bin and each MDA rank */
 	for (i = 0; i < BASEDATA_DOP_SIZE; i++)
	 {
          /* calculate the range of each bin */
	  range_vel[i] = (i)*BIN_SIZE + HALF_BIN_SIZE;

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
          
         } /* END of for (i = 0; i < BASEDATA_DOP_SIZE; i++) */

} /* END of the function */
