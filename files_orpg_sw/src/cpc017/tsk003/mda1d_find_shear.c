/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/12/03 20:47:38 $
 * $Id: mda1d_find_shear.c,v 1.8 2010/12/03 20:47:38 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_find_shear.c                                    *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *			ORPG MDA AEL					      *
 *                                                                            *
 *      Description:    This module check two adjacent azimuthal velocities   *
 *		    	to search for cyclonic shear segments. Cyclonic shear *
 *			is defined as increasing (decreasing) velocity with   *
 *			increasing azimuth for Northern (Southern) Hemisphere *
 * 			radar sites.					      *	
 *      Input:                                                                *
 *        azm - the azimuth of current radial			      *
 * 	  vel[]  - the velocity of current radial			      *
 *        old_azm - the azimuth of previous radial			      *
 *        old_vel[]   - the velocity of previous radial			      *
 *        range_vel -  the range for the velocity data			      *
 * 	  shear_max_rng_th -  max range to process for shear calculations     * 
 *                                                                            *
 *      Output:                                                               *
 *        old_azimuth - updated old_azimuth				      *
 *        old_velocity - updated old_vel 				      *
 *      Return:         none                                                  *
 *      notes:          none                                                  *
 ******************************************************************************/

#include "mda1d_parameter.h"
#include "mda1d_acl.h"
#include <math.h>
#include <rpgcs.h>

#include <mda_adapt.h>
#define EXTERN extern /* prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"

#define MAX_RANK_RANGE  30      /* Range of the rank*/
#define         NINE_NINE_NINE       999.0 /* represents an invalid data */
#define         NINETY_NINE          99.0  /* invalid value */
#define         SEVEN_SEVEN_SEVEN     777.0 /* represents an invalid data */
#define		DEG_CIR			360.0 /* degree of a circle */
#define 	THREE_HUNDRED_THIRTY    330.0 /* just a number of 330.0 */
#define 	TRUE    1 /* logical variable */


/* acknowlege the global variables declared outside*/
        extern double mda_vect_vel[MESO_MAX_LENGTH][BASEDATA_DOP_SIZE];
        extern double mda_vect_azm[MESO_MAX_LENGTH][BASEDATA_DOP_SIZE];
        extern int meso_vect_in[BASEDATA_DOP_SIZE];
        extern int length_vect[BASEDATA_DOP_SIZE];
        extern double save_azm[MESO_AZOVLP_THR_SR];
        extern int meso_max_ahd[BASEDATA_DOP_SIZE];
        extern double save_vel[MESO_AZOVLP_THR_SR][BASEDATA_DOP_SIZE];
        extern int num_look_ahead[BASEDATA_DOP_SIZE];
        extern double meso_vd_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        extern double meso_shr_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        extern int num_mda_vect;
        extern Shear_vect mda_shr_vect[MESO_MAX_VECT];
        extern int have_open_vector;
        extern int call_from_finish;
	extern double conv_max_rng_th;
	extern double shear_max_rng_th;
        extern int Wxmode;
        extern int overflow_flg;

void mda1d_find_shear(double azm, double vel[], double old_azm, 
	double old_vel[], double range_vel[], int ng_vel, int naz,int azm_reso)
{
	/* declare static variable */
	static double beg_vel[BASEDATA_DOP_SIZE], end_vel[BASEDATA_DOP_SIZE];
        static double beg_azm[BASEDATA_DOP_SIZE], end_azm[BASEDATA_DOP_SIZE];
        static double max_gtg_vd[BASEDATA_DOP_SIZE];/* max G_T_G vel dif*/
        static double gtg_azm[BASEDATA_DOP_SIZE];/* mid_azm where max_gtg_vd*/ 


	/* delcare the local variables */
	double mid_azm;     /* Gate-to-gate azimuth (midway in between)*/
        double shr_trend;
	double vel_diff, arc_length, cart_length, shear; 
	double rank = 0.0;
	int i, j, k; /* loop index */

	int n_vel;

        /* declare locale variables for core shear */
	double cs_beg_vel, cs_beg_azm, cs_end_vel, cs_end_azm,
	       cs_vel_diff, cs_shear, cs_rank, cs_cart_length;
        double cs_vec_flag; /* cs_vec_flag = 0: Do not use vector, cs too long *
			       cs_vec_flag = 1: use cs portion of vector       *
			       cs_vec_flag = 2: use the entire vector          */ 
        double x1, y1, x2, y2;

	/*====================================================================*/
	/* first we assume there is not any open vector at all ranges */
	have_open_vector = 0;

	/* calculate the midway between the current and previous azimuth */
	if (fabs(azm - old_azm) > THREE_HUNDRED_THIRTY)
	 {
          mid_azm = 0.5 * (azm + old_azm + DEG_CIR);
          if (mid_azm >= DEG_CIR) 
           mid_azm = mid_azm -DEG_CIR;
         }
        else
	 mid_azm = 0.5 * (azm +old_azm);

	/* go through each range gate checking for azimuthal shear */
	for (i = 0; i < ng_vel; i++)
	 {
          /* Do not process beyond the range representing the max height */
          /* for shear vector calculations.                              */
          
/***          if (range_vel[i] > shear_max_rng_th) break;***/
          
   check_again:

          /* check to see if we are within a vector  at this range */  
          if (meso_vect_in[i] == 0)
           {/* we are not within any vector at the range now */
          
            /* if this module is called from  "mda1d_finish_vectors", 	 *
             * no new vector should be created; what this module    	 *
             * is supposed to do is to complete the unclosed vectors	 *
             * So do nothing if it is called from "mda1d_finish_vectors" */
            if (call_from_finish == 1)
              continue;

            /* if the previous velocity is invalid, do nothing */
            if (old_vel[i]  > SEVEN_SEVEN_SEVEN ) 
		continue; /* go to next range */ 

	    /* if the previous velocity is valid while current *
             * velocity is invalid, begin a vector and start   *
             * look ahead mode			               */
	    if (vel[i] > SEVEN_SEVEN_SEVEN) 
             {
              meso_vect_in[i] = 1;
              beg_vel[i] = old_vel[i];
	      beg_azm[i] = old_azm;
	      num_look_ahead[i] = 1;
              end_vel[i] = old_vel[i];
              end_azm[i] = old_azm;
	      max_gtg_vd[i] = 0.0;
              gtg_azm[i] = NINE_NINE_NINE;
              mda_vect_vel[0][i] = old_vel[i];
	      mda_vect_azm[0][i] = old_azm;
              mda_vect_vel[1][i] = vel[i];
	      mda_vect_azm[1][i] = azm;
              length_vect[i] = 2;
             }/* END of if (vel[i] > SEVEN_SEVEN_SEVEN) */
	    else
	     {/* both previous and current velocities are valid */
	      if (HEM == 1)
               shr_trend = vel[i] - old_vel[i];
	      else
               shr_trend = old_vel[i] - vel[i];

              /* if shr_trend is not larger than 0, go to next range */
              /* otherwise, a cyclonic shear is present, start a vector*/
              if (shr_trend <= 0)
               {
                continue;	       
               }
              else /* a cyclonic shear is present */  
               {
                meso_vect_in[i] = 1;
                beg_vel[i] = old_vel[i];
                beg_azm[i] = old_azm;
                num_look_ahead[i] = 0;
                max_gtg_vd[i] = shr_trend;
                gtg_azm[i] = mid_azm;
                mda_vect_vel[0][i] = old_vel[i];
                mda_vect_azm[0][i] = old_azm;
                mda_vect_vel[1][i] = vel[i];
                mda_vect_azm[1][i] = azm;
                length_vect[i] = 2;
               } 
             }/* else END of if (vel[i] > SEVEN_SEVEN_SEVEN) */ 
           } /* END of if (meso_vect_in[i] == 0)*/
          else /* we are within a vector at the range */
           {
	    /* once we get here, that means at least there is one *
	     * vector that is open, so set "have_open_vector".    */
	     have_open_vector = 1;

            if (num_look_ahead[i] == 0)
             {/* we within a vector and not in look ahead mode */
              if (vel[i] > SEVEN_SEVEN_SEVEN)
               {/* the current velocity is invalid */
                num_look_ahead[i] = 1;
                end_vel[i] = old_vel[i];
                end_azm[i] = old_azm;
		mda_vect_vel[length_vect[i] - 1][i] = old_vel[i];
                mda_vect_azm[length_vect[i] - 1][i] = old_azm;
                length_vect[i] += 1;
               }/* END of if (vel[i] > SEVEN_SEVEN_SEVEN) */
              else /* the current velocity is valid */
               {
                /* compute the cyclonic shear trend */
 	        if (HEM == 1)
                 shr_trend = vel[i] - old_vel[i];
                else
                 shr_trend = old_vel[i] - vel[i];

                /* if the shear trend is still cyclonic, *
		 * keep the vector open */
		if (shr_trend > 0)
                 {
                  if (shr_trend > max_gtg_vd[i])
                   {/* update max_gtg_vd with shr_trend */
                    max_gtg_vd[i]  = shr_trend;
                    gtg_azm[i] = mid_azm;
                   }
  	 	  mda_vect_vel[length_vect[i] - 1][i] = old_vel[i];
                  mda_vect_azm[length_vect[i] - 1][i] = old_azm;
                  length_vect[i] += 1;
                 }/* END of if (shr_trend > 0) */
                else /* it seems turning to anticyclonic */
		 {
                  /* For the Northern Hemispher: 
 		   * if the current velocity is not larger than 
		   * the begining velocity AND the length of
	           * current shear is not larger than max look
		   * ahead number (MESO_MAX_AHD), cancel this vector.
		   * For the Southern Hemisphere:
                   * if the current velocity is not smaller than 
		   * the begining velocity AND the length of
                   * current shear is not larger than max look
                   * ahead number (MESO_MAX_AHD), cancel this vector.*/
                  if (((HEM ==1 && vel[i] <= beg_vel[i]) ||
		        (HEM ==0 && vel[i] >= beg_vel[i])) &&
			   length_vect[i] <= meso_max_ahd[i])
                   {
  		    meso_vect_in[i] = 0;
                    length_vect[i] = 0;
                   }
                  else /* otherwise we end the cyclonic shear trend,
                        * and start a look ahead mode */
                   {
		    num_look_ahead[i] = 1;
                    end_vel[i] = old_vel[i];
                    end_azm[i] = old_azm;
                    mda_vect_vel[length_vect[i] - 1][i] = old_vel[i];
                    mda_vect_azm[length_vect[i] - 1][i] = old_azm;
                    length_vect[i] += 1;
                   }/* else END of if (((HEM ==1 && vel[i] <= beg_vel[i]) || */
                 }/* else END of if (shr_trend > 0) */ 
               }/* else END of if (vel[i] > SEVEN_SEVEN_SEVEN) */
             }/* END of if (num_look_ahead[i] == 0) */
            else if (num_look_ahead[i] <= meso_max_ahd[i])
             /* we are within a vector and in look ahead mode */
             {
	      if (vel[i] > SEVEN_SEVEN_SEVEN)
               {/* the current velocity is invalid */
                num_look_ahead[i] += 1;
                mda_vect_vel[length_vect[i] - 1][i] = old_vel[i];
                mda_vect_azm[length_vect[i] - 1][i] = old_azm;
                length_vect[i] += 1;
               }/* END of if (vel[i] > SEVEN_SEVEN_SEVEN) */
              else /* the current velocity is valid */
               {
                /* compute the cyclonic shear trend */
                if (HEM == 1)
                 shr_trend = vel[i] - end_vel[i];
                else
                 shr_trend = end_vel[i] - vel[i];
                
                if (shr_trend <= 0)
                 {/* it seems turning to anticyclonic */
		  /* For the Northern Hemispher:
                   * if the current velocity is not larger than
                   * the begining velocity AND the length of
                   * current shear is not larger than max look
                   * ahead number (meso_max_ahd[i]), cancel this vector.
                   * For the Southern Hemisphere:
                   * if the current velocity is not smaller than
                   * the begining velocity AND the length of
                   * current shear is not larger than max look
                   * ahead number (meso_max_ahd[i]), cancel this vector.*/
                  if (((HEM ==1 && vel[i] <= beg_vel[i]) ||
                        (HEM ==0 && vel[i] >= beg_vel[i])) &&
                           length_vect[i] <= meso_max_ahd[i])
                   {
                    meso_vect_in[i] = 0;
                    length_vect[i] = 0;
                   }
                  else /* otherwise we end the cyclonic shear trend,
                        * and start a look ahead mode */
		   {
                    num_look_ahead[i] += 1;
                    mda_vect_vel[length_vect[i] - 1][i] = old_vel[i];
                    mda_vect_azm[length_vect[i] - 1][i] = old_azm;
                    length_vect[i] += 1;
                   }/* END if (((HEM ==1 && vel[i] <= beg_vel[i]) ||*/ 
                 }/* END of if (shr_trend <= 0) */
                else /* shr_trend > 0, we turn back to cyclonic */
                 {
                  /* we cancel look ahead mode and the vector open */
                  if (old_vel[i] < SEVEN_SEVEN_SEVEN)
                   {/* old_vel is valid */
		    /* compute the cyclonic shear trend */
                    if (HEM == 1)
                     shr_trend = vel[i] - old_vel[i];
                    else
                     shr_trend = old_vel[i] - vel[i];

		    if (shr_trend > max_gtg_vd[i])
                     {
                      /* update max_gtg_vd */
                      max_gtg_vd[i] = shr_trend;
                      gtg_azm[i] = mid_azm;
                     }                    

                   }/* END of if (old_vel[i] < SEVEN_SEVEN_SEVEN)*/ 
                  num_look_ahead[i] = 0;/* cancel the look ahead mode */
                  mda_vect_vel[length_vect[i] - 1][i] = old_vel[i];
  		  mda_vect_azm[length_vect[i] - 1][i] = old_azm;
		  length_vect[i] += 1;
                 }/* else END of if (shr_trend <= 0) */
               }/* else END of if (vel[i] > SEVEN_SEVEN_SEVEN) */
             }/* END of else if (num_look_ahead[i] <= MESO_MAX_AHD[i]) */
            else
             {
            /* we have completed a vector  */
	    mda_vect_vel[length_vect[i] - 1][i] = old_vel[i];
            mda_vect_azm[length_vect[i] - 1][i] = old_azm;
	    
            length_vect[i] -= num_look_ahead[i];  /* cut off the data saved *
						   * in the look-ahead mode */
             /* set meso_in_vect[i] to 0 */
             meso_vect_in[i] = 0;
             
            /* removing the invalid data in the mda_vect_(vel/azm) arrays */
            n_vel = length_vect[i];
	    for (j = n_vel-1; j >= 1; j--)
	     {
              if (mda_vect_vel[j-1][i] > SEVEN_SEVEN_SEVEN)
               {
                for (k = j; k <= n_vel - 1; k++)
                 {
 		  mda_vect_vel[k-1][i] = mda_vect_vel[k][i];
 		  mda_vect_azm[k-1][i] = mda_vect_azm[k][i];
                 }
                length_vect[i] -= 1;
               } 
	     } /* END of for (j = n_vel-1; j >= 1; j--) */

            /* calculate attributes: velocity difference, arc length, shear */
	    mda1d_shear_attributes(beg_vel[i], end_vel[i], beg_azm[i], end_azm[i],
				   range_vel[i], &vel_diff, &arc_length, &shear);
            /* if shear = 0, go to next bin */
            if (shear == 0.0)
             {
              goto check_again; 
             }

	    /* only handle the shear vectors that are above thresholds */
	    if ((vel_diff >= meso_vd_thr[i][MESO_MIN_RANK] &&
		shear >= meso_shr_thr[i][MESO_MIN_RANK]) ||
		max_gtg_vd[i] >= meso_vd_thr[i][MESO_MIN_RANK])
             {
   	      /* calculate the cartesian length of a vector */
              x1 = range_vel[i] * sin(beg_azm[i] * DEGREE_TO_RADIAN);
              y1 = range_vel[i] * cos(beg_azm[i] * DEGREE_TO_RADIAN);
              x2 = range_vel[i] * sin(end_azm[i] * DEGREE_TO_RADIAN);
              y2 = range_vel[i] * cos(end_azm[i] * DEGREE_TO_RADIAN);
              cart_length = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
	      
              /* determine the core shear region of vector, cartesian length of *
	       * the core shear, and strength rank of the core */
              if (cart_length > mda_adapt.meso_max_vect_len)
               mda1d_search_core_shear(range_vel, i, &cs_beg_vel, &cs_beg_azm,
					&cs_end_vel, &cs_end_azm, &cs_vel_diff,
					&cs_shear, &cs_rank, &cs_cart_length);

	      /* Threshold vectors that are too long. use "core shear" as *
               * a second test						  */
              if ( cart_length > mda_adapt.meso_max_vect_len)
	       {
		if (cs_cart_length > mda_adapt.meso_max_core_len)
                 {
		  cs_vec_flag = 0.0; /* disregard this vector */
                 } /* END of if (cs_cart_length > mda_adapt.meso_max_core_len) */
                else 
                 {
                  cart_length = cs_cart_length;
		  beg_vel[i] = cs_beg_vel;
 		  beg_azm[i] = cs_beg_azm;
		  end_vel[i] = cs_end_vel;
		  end_azm[i] = cs_end_azm;
                  vel_diff = cs_vel_diff;
                  shear = cs_shear;
	          rank = cs_rank;
		  cs_vec_flag = 1.0;
                 } /* END of else if (cs_cart_length > mda_adapt.meso_max_core_len) */
               } /* END of if ( cart_length > mda_adapt.meso_max_vect_len) */  
              else 
               { /* use the entire patern vector */
                cs_vec_flag = 2.0;
               } /* END of else if ( cart_length > mda_adapt.meso_max_vect_len) */

              /* assign the rank to vector */
              for ( k = MESO_MIN_RANK; k <= MESO_MAX_RANK; k++)
               {
                if ((vel_diff >= meso_vd_thr[i][k] &&
		    shear >= meso_shr_thr[i][k]) ||
		    max_gtg_vd[i] >= meso_vd_thr[i][k])
		  rank = (double)k;
               } 

	      /* if there are no more available array locations - terminate */
              if (num_mda_vect >= MESO_MAX_VECT)
               {
                 /* Set the overflow flag so that a message can be reported
                    in mda3d if there are valid 3D features.                */
                 overflow_flg = TRUE;
                 break; /* get out of loop */
               }
              else 
               {
                /* save the vector to mda_shr_vect array */
                mda_shr_vect[num_mda_vect].range = range_vel[i];                
                mda_shr_vect[num_mda_vect].beg_azm = beg_azm[i];                
                mda_shr_vect[num_mda_vect].end_azm = end_azm[i];                
                mda_shr_vect[num_mda_vect].beg_vel = beg_vel[i];                
                mda_shr_vect[num_mda_vect].end_vel = end_vel[i];                
                mda_shr_vect[num_mda_vect].vel_diff = vel_diff;                
                mda_shr_vect[num_mda_vect].shear = shear;                
                mda_shr_vect[num_mda_vect].maxgtgvd = max_gtg_vd[i];                
                mda_shr_vect[num_mda_vect].gtg_azm = gtg_azm[i];                
                if (call_from_finish ==  0)
                 mda_shr_vect[num_mda_vect].fvm = 0.0;                
                else
                 mda_shr_vect[num_mda_vect].fvm = NINETY_NINE;
                mda_shr_vect[num_mda_vect].rank = rank;                
                mda_shr_vect[num_mda_vect].cs_vec_flag = cs_vec_flag;                
               } /* END of else if if (num_mda_vect > MESO_MAX_VECT) */

	      /* increment meso shear vector counter */
              num_mda_vect +=1;


             } /* END of if ((vel_diff >= meso_vd_thr[i][MESO_MIN_RANK] && */ 

            /* in order not to lost the current data vel[i], *
             * we have to try to start a new vector from     *
               old_vel[i], so need to goto "check_again"     */
            goto check_again;

           } /* END of else if (num_look_ahead == 0) */
          }/* else END of if (meso_vect_in[i] == 0) */ 
	
         } /* END of for (i = 0; i < shear_max_bin_th; i++) */

	/* save data in order to complete shear which has not been *
         * closed upon reaching the end of the elevation scan.     */
          if(azm_reso == 2) {
          if (naz < MESO_AZOVLP_THR)
            save_azm[naz] = azm;
          }
         else{
          if (naz < MESO_AZOVLP_THR_SR)
            save_azm[naz] = azm;
          }

        for (j = 0; j < ng_vel; j++)
         {
          old_vel[j] = vel[j];
          if(azm_reso == 2) {
            if ( naz < MESO_AZOVLP_THR)
             save_vel[naz][j] = vel[j];
            }
          else{
            if ( naz < MESO_AZOVLP_THR_SR)
             save_vel[naz][j] = vel[j];
            }
           }
/* END of the function */
}

