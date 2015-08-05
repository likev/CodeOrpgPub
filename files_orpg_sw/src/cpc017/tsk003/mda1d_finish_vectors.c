/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/19 19:55:57 $
 * $Id: mda1d_finish_vectors.c,v 1.4 2009/03/19 19:55:57 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_finish_vectors.c                                           *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This module completes those vectors that are not      *
 *			closed upon reaching the end of an elevation          *
 *      Input:                                                               *
 *        vel[]  - the velocity of current radial                             *
 *        old_azm - the azimuth of previous radial                            *
 *        old_vel[]   - the velocity of previous radial                       *
 *        range_vel -  the range for the velocity data                        *
 *        ng_vel -  the number of velocity gates                              *
 *                                                                            *
 *      notes:          none                                                  *
 ******************************************************************************/

#include<stdio.h>

#include "mda1d_parameter.h"
#include "mda1d_acl.h"

/* acknowledge the global variables */
        extern double mda_vect_vel[MESO_MAX_LENGTH][BASEDATA_DOP_SIZE];
        extern double mda_vect_azm[MESO_MAX_LENGTH][BASEDATA_DOP_SIZE];
        extern int meso_vect_in[BASEDATA_DOP_SIZE];
        extern int length_vect[BASEDATA_DOP_SIZE];
        extern double save_azm[MESO_AZOVLP_THR_SR];
        extern int meso_max_ahd[BASEDATA_DOP_SIZE];
        extern double save_vel[MESO_AZOVLP_THR_SR][BASEDATA_DOP_SIZE];
        extern int num_look_ahead[BASEDATA_DOP_SIZE];
        extern double meso_vd_thr[BASEDATA_DOP_SIZE][99];
        extern double meso_shr_thr[BASEDATA_DOP_SIZE][99];
        extern int num_mda_vect;
        extern Shear_vect mda_shr_vect[MESO_MAX_VECT];
        extern int have_open_vector;
        extern int call_from_finish;
	extern double conv_max_rng_th;
	extern double shear_max_rng_th;


void mda1d_finish_vectors(double ng_vel, double old_vel[], double old_azm,
                          double range_vel[], int naz, int azm_reso)
{
	
	/* declare local variables */
	int  j;

        /* call "mda1d_find_shear"  to connect the first radial and *
         * the last radial */
        mda1d_find_shear(save_azm[0], save_vel[0], old_azm,
                          old_vel, range_vel, ng_vel, naz, azm_reso);
 	/* set "call_from_finish" to true (1) */
	call_from_finish = 1; 

	/* complete the unclosed shear vectors                 *
         *chech the azm resolution to determine which threshold*
         *should be used                                       */
        if(azm_reso == 2) {
          for (j = 1; j < MESO_AZOVLP_THR; j++)
           {
           /* call "mda1d_find_shear" to complete the unclosed vectors*/
           /* watch up here! naz never be increased, do something later on */
           mda1d_find_shear(save_azm[j], save_vel[j], save_azm[j-1],
                          save_vel[j-1], range_vel, ng_vel, naz, azm_reso);
           /* if all vectors have been closed, stop processing */
           if (have_open_vector == 0)
            break;

           } /* END of for (j = 1; j < MESO_AZOVLP_THR; j++) */
          }
         else {
           for (j = 1; j < MESO_AZOVLP_THR_SR; j++)
           {
           /* call "mda1d_find_shear" to complete the unclosed vectors*/
           /* watch up here! naz never be increased, do something later on */
           mda1d_find_shear(save_azm[j], save_vel[j], save_azm[j-1],
                            save_vel[j-1], range_vel, ng_vel, naz, azm_reso);
           /* if all vectors have been closed, stop processing */
           if (have_open_vector == 0)
            break;

           } /* END of for (j = 1; j < MESO_AZOVLP_THR_SR; j++) */
          }
	
} /* END of the function */
