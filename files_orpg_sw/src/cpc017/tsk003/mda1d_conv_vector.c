/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:18:00 $
 * $Id: mda1d_conv_vector.c,v 1.3 2005/03/03 22:18:00 ryans Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_conv_vector.c                                   *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ALGORITHM (MDA) DESCRIPTION                           *
 *                                                                            *
 *      Description:    This module finds convergence vectors along one       *
 *			radial. A convergence vector is defined as a vector   *
 *			which contains all range bins from a radial of data   *
 *			in which Smooth Velocity values decrease with 	      *
 *			increasing range from the radar. The decreasing values*
 *			need not to be consecutive, up to THRESHOLD           *
 *			(Convergence look ahead) bins are allowed to be       *
 *                      examined and contribute to the velocity changes in    *
 *			order to skip over missing and range fold data bins.  *
 *      Input:          azm - azimuth of this radial                          *
 *			vel - velocity array				      *
 *                      range_vel - range array				      *
 *                      ng_vel - range-number array    			      *
 *									      *
 *      Output:         none                                                  *
 *      Return:         none                                                  *
 *      Global:                                                               *
 *			num_conv_vect, number of convergence vectors          *
 * 			mda_conv_vect[], array of convergence vectors         *
 *      notes:          none                                                  *
 ******************************************************************************/
 

#include <stdio.h>
#include <math.h>
#include "mda1d_parameter.h"
#include "mda1d_acl.h"

#include <mda_adapt.h>
#define EXTERN extern /* prevent multiple adaptation data object instantiation */
#include "mda_adapt_object.h"

#define 	TRUE	1 /* boolean variable */
#define 	FALSE 	0 /* boolean variable */
#define         NINE_NINE_NINE       999.0 /* represents an invalid data */
#define         SEVEN_SEVEN_SEVEN     777.0 /* represents an invalid data */

	/* acknowledge globle variables declared outside */
	extern int num_conv_vect;
        extern Conv_vect mda_conv_vect[CONV_MAX_VECT];

void mda1d_conv_vector (double vel[], double azm, double range_vel[], 
                        int ng_vel, double conv_max_rng_th)
{

	/* declare local variables */
	int i, j, k;
   	int cur_index, start_index, max_index;
	double max_vel, min_vel, vel_diff;
        int max_vel_index, min_vel_index;
        int max_tmp, min_tmp;
	double distant, shear;
        double sum_vel;
	double *smooth_vel;


	int vector_opened;  /* TRUE: convergence vector has been opened */
	int discard_this_vector; /* TRUE: discard this vector */

	/* initialize start_index */
        start_index = -99;

	/* allocate a space to pointer smooth_vel */
        smooth_vel = (double *) calloc((ng_vel + mda_adapt.conv_max_lookahd), sizeof(double));

	/* smooth the velocity data using a five-point-running average. *
	 * at least three good velocity data are needed, or else the    *
         * smoothed value is set to missing.				*/
	for(i=2; i < ng_vel-2; i++)
         {
	  sum_vel = 0;
          k = 0;

          for(j=1; j <= 5; j++)
           {
            if (vel[i+j-3] < SEVEN_SEVEN_SEVEN)
             {
              sum_vel += vel[i+j-3];
              k += 1;
             }
           }/* END of for(j=1; j <= 5; j++) */
          /* if the number of good data is  less than 3, set to missing */
          if (k <= 2)
           smooth_vel[i] = NINE_NINE_NINE;
          else
           smooth_vel[i] = sum_vel / k;
         }/* END of for(i=2; i < ng_vel-2; i++) */

        /* assign values to last two bins */
        smooth_vel[ng_vel-2] = vel[ng_vel-2];
        smooth_vel[ng_vel-1] = vel[ng_vel-1];

        /* add sufficient missing data to terminate look-ahead *
 	 * mode while searching for convergent segments        */
	for (i = ng_vel; i <= ng_vel+mda_adapt.conv_max_lookahd-1; i++)
         smooth_vel[i] = NINE_NINE_NINE;
	
	/* initilize variables */
	vector_opened = FALSE;   /* FALSE: no vector has been found */

        /* search for radial convergence 			   *
	 * a good data bin must be (1) within *
         * the maximum range of velocity data (2) below a height    *
         * threshold, and (3) not missing or range-folded           */

        cur_index = 5;   /* ignore the first 5 bins */
        while (cur_index < ng_vel) 
         {
           /* if the height is beyong the THRESHOLD, drop the radial and*
           * return. */ 
          if ( range_vel[cur_index] >= conv_max_rng_th)
            break;

	   /* if the data is invalid, goto next bin */ 
          if ( smooth_vel[cur_index] > SEVEN_SEVEN_SEVEN)
           {
            cur_index +=1;
            continue;
           }   

          /* at this point, a valid bin has been found, 
           * cur_index has been set to the valid bin index, then */
          /* initialize index of max velocity to cur_index */
          max_index = cur_index; 

	  /* search for the index of the max velocity in the next *
           * mda_adapt.conv_max_lookahd bins behind cur_index.	     		          */
          for (i = 1; i <= mda_adapt.conv_max_lookahd; i++)
           {
            if ( smooth_vel[cur_index + i] < SEVEN_SEVEN_SEVEN)
             {
              if (max_index == cur_index)
               {/* first find a bin whose value is not larger than curren bin */
                if (smooth_vel[cur_index] >= smooth_vel[cur_index+ i])
                max_index = cur_index + i ;
               }
              else
               {/* then find a bin whose value is the biggest in the next *
                 * mda_adapt.conv_max_lookahd(4, by default) bins, but still not    *
                 * larger than current bin                                */
                if ((smooth_vel[cur_index] >= smooth_vel[cur_index+ i])
                   && (smooth_vel[cur_index+ i] >= smooth_vel[max_index]))
	         max_index = cur_index + i;
               }
             } /* END of if ( smooth_vel[cur_index + i] < SEVEN_SEVEN_SEVEN) */
          }/* END of for (i = 1; i <= mda_adapt.conv_max_lookahd; i++) */
	
          /* next we will use boolean values "vector_opened" and
	     "max_index > cur_index" to determine the status of
	      a convergence vector.
              There are 4 combinations:
              		vector_opened	max_index > cur_index
		 1 	   TRUE			TRUE
		 2         TRUE			FALSE
		 3	   FALSE		TRUE
		 4	   FALSE		FALSE
              note: "max_index" will never be smaller than "cur_index" 
            Each combination represents one status of convergence vector.
            combination 1: within a vector and in a look-ahead process
	    combination 2: complted a vector
            combination 3: start a new vector
            combination 4: no vector has been found              */ 

          if ( vector_opened == FALSE && max_index > cur_index)
           { /* start a new vector, and reset cur_index */
	    vector_opened = TRUE;
            start_index = cur_index;
	    cur_index = max_index;
           } /* END of if ( vector_opened != TRUE && max_index > cur_vel) */
          else if (vector_opened == FALSE && max_index == cur_index) 
                { /* no vector has been found */
                 cur_index += 1;
                }
               else if (vector_opened == TRUE && max_index > cur_index)
                { /* within a vector and in a look-ahead process */
                 cur_index = max_index;
                }
               else /* ( vector_opened == TRUE && max_index == cur_index) */
                {/* complete a vector */
                 
                 /* initialize variables */
	    	 max_vel =-NINE_NINE_NINE;
            	 min_vel = NINE_NINE_NINE;
            	 max_vel_index = 0;
            	 min_vel_index = 0;
          
            	 /* find the max and min velocities */
		 mda1d_find_max_min (smooth_vel, start_index, cur_index,
                         &max_vel, &min_vel, &max_vel_index, &min_vel_index);

                /* calculate velocity difference */
		vel_diff = max_vel - min_vel;
                
                /* discard vectors with weak convergence */
                if ( vel_diff < CONV_DELTV_TH)
                 {
                  vector_opened = FALSE;
                  cur_index += 1;
                  continue;
                 }

		/* calculate the distant of the vector */
                distant = range_vel[min_vel_index] - range_vel[max_vel_index];
                /* discard vector with a length shorter than THRESHOLD */
                if ( distant < mda_adapt.conv_rng_dist)
                 {
                  vector_opened = FALSE;
                  cur_index += 1;
                  continue;
                 }

                 /* calculate shear */
 		 shear = vel_diff / distant;

		/* if the shear is weak, adjust vector to find core shear region *
                * incrementely remove velocity samples from each end of the     *
                * vector until core shear passes threshold. if it does, reassign*
                * vector to its core. if the shear never surpass the threshold, *
                * discard the vector. */

		if (shear < CONV_SHR_TH)
                 {
                /* initialize discard_this_vector to FALSE */
                discard_this_vector = FALSE;

		while(discard_this_vector == FALSE)
                 {
                  /* call subroutine to find the core vector */
		  conv_core_vector(&max_vel, &max_vel_index, &min_vel, 
                                   &min_vel_index, &vel_diff, &shear,
                                   &discard_this_vector, 
 				   range_vel, smooth_vel);
                   if (discard_this_vector == TRUE)
                    break; /* get out of this loop */

                   /* if shear pass the THRESHOLD, get out of loop */
                   if (shear > CONV_SHR_TH && vel_diff > CONV_DELTV_TH)
                   {
                    break;
                   }
                 }
             
             /* check to see if the vector has been discarded *
              * if not, find the new max and min vel in the vector *
              * otherwise, throw away this vector and reset vector_opened *
              * and cur_index. */
             if (discard_this_vector == FALSE)
              {
               max_tmp = max_vel_index;
               min_tmp = min_vel_index;

	       /* find the max and min values */
 	       mda1d_find_max_min (smooth_vel, max_tmp, min_tmp,
                         &max_vel, &min_vel, &max_vel_index, &min_vel_index);

               vel_diff = max_vel - min_vel;
              } /* END of if (discard_this_vector == FALSE) */
             else
              {/* the vector is discarded, reset cur_index */
               vector_opened = FALSE;
	       cur_index += 1;
               continue; 
              } 
            } /* END of if (shear < CONV_SHR_TH) */

           /* we have a new completed vector, if the num of vect isn't too big *
            * save the vector */
           if ( num_conv_vect >= CONV_MAX_VECT)
            break;
           
           /* save the vector */
           mda_conv_vect[num_conv_vect].azm = azm;
           mda_conv_vect[num_conv_vect].range_max = range_vel[max_vel_index];
           mda_conv_vect[num_conv_vect].range_min = range_vel[min_vel_index];
           mda_conv_vect[num_conv_vect].max_vel = max_vel;
           mda_conv_vect[num_conv_vect].min_vel = min_vel;
           mda_conv_vect[num_conv_vect].vel_diff = vel_diff;

           num_conv_vect += 1;

           /* begin searching another new vectr */
           vector_opened = FALSE;
           cur_index += 1;
           }
         } /* END of while (cur_index < ng_vel) */
	free(smooth_vel); /* dealocate the memory pointed by smooth_vel*/

} /* END of the function */

/**********************************************************************
Description: conv_core_vector(), search the core convergence vector   *
                                                                      *
Input:      max_vel, pointer to max velocity                          *
            min_vel, pointer to min velocity                          *
            max_vel_index, pointer to max velocity index              *
            min_vel_index, pointer to min velocity index              *
            rane_vel[], array of range for vel data 		      *
	    smooth_vel[], array of smoothed velocity		      *
Outputs:                                                              *
	    max_vel, pointer to max velocity                          *
            min_vel, pointer to min velocity                          *
            max_vel_index, pointer to max velocity index              *
            min_vel_index, pointer to min velocity index              *
            vel_diff, pointer to velocity difference                  *
            discard_this_vector, pointer to a boolean variable        *
Returns:                                                              *
Globals:                                                              *
Notes:                                                                *
***********************************************************************/
void conv_core_vector(double *max_vel, int *max_vel_index,
                      double *min_vel, int *min_vel_index,
                      double *vel_diff, double *shear, 
		      int *discard_this_vector, 
                      double range_vel[], double smooth_vel[])
{
	/* declare local variables */
	double max_vel_c, min_vel_c;
	double distant;
	double vd1, vd2;
        int max_vel_index_c, min_vel_index_c;

        /* initialize core-ralated local variables */
        max_vel_c = *max_vel;
        max_vel_index_c = *max_vel_index;
        min_vel_c = *min_vel;
        min_vel_index_c = *min_vel_index;

	/* find a valid data bin whose index in smaller than *
	 * current min velocity index but larger than        *
         * current max velocity index. Then calculate        *
         * velocity difference between current min volocity  *
         * bin and the found valid data bin                  */
        while(TRUE)
         {
          min_vel_index_c -= 1;
          if (min_vel_index_c <= max_vel_index_c)
           {
            *discard_this_vector = TRUE;
            return;
           }
          if ( smooth_vel[min_vel_index_c] < SEVEN_SEVEN_SEVEN)
            {
             vd1 = smooth_vel[min_vel_index_c] - min_vel_c;
             break; /* get out of the loop */
            }
         }

	/* find a valid data bin whose index in  larger than *
         * current max velocity index but smaller than       *
         * current min velocity index. Then calculate        * 
         * velocity difference between current max volocity  *
         * bin and the found valid data bin                  */
        while (TRUE) 
         {
          max_vel_index_c += 1;
          if (max_vel_index_c >= min_vel_index_c)
           {
            *discard_this_vector = TRUE;
            return;
           }
          if ( smooth_vel[max_vel_index_c] < SEVEN_SEVEN_SEVEN)
            {
             vd2 =  max_vel_c - smooth_vel[max_vel_index_c];
             break;
            }
         }

        /* if vd1 is more significant than vd2,   *
         * update the min velocity and its index, *
         * otherwise, update the max velocity     *
         * and its index.			  */
	if (vd1 <= vd2)
         {
          *min_vel = smooth_vel[min_vel_index_c];
          *min_vel_index = min_vel_index_c;
         }
        else
         {
          *max_vel = smooth_vel[max_vel_index_c];
          *max_vel_index = max_vel_index_c;
         }

        *vel_diff = *max_vel - *min_vel;
         distant = range_vel[*min_vel_index] - range_vel[*max_vel_index];

         /* discard the vector if its distant is smaller than threshold */
         if (distant < mda_adapt.conv_rng_dist)
          {
           *discard_this_vector = TRUE;
           return;
          }
        *shear = *vel_diff / distant;

}/* END of the function */


/**********************************************************************
Description: mda1d_find_max_min(), find the max and min in a section  *
             of an array					      *
Inputs:
            smooth_vel[], array of smoothed velocity                  *
            start_ind, start index of the array			      *
	    end_ind, end index of the array			      *
Outputs:                                                              *
            max_vel, pointer to max velocity                          *
            min_vel, pointer to min velocity                          *
            max_vel_index, pointer to max velocity index              *
            min_vel_index, pointer to min velocity index              *
Returns:                                                              *
Globals:                                                              *
Notes:                                                                *
***********************************************************************/

void mda1d_find_max_min (double smooth_vel[], int start_ind, int end_ind,
		         double *max_vel, double *min_vel,
			 int *max_vel_index, int *min_vel_index)
{
        /* declare local variables */
	int i;

	/* find the max and min velocities */
        for ( i = start_ind; i <= end_ind; i++)
         {
          if (smooth_vel[i] < SEVEN_SEVEN_SEVEN)
           {
            if (smooth_vel[i] < *min_vel)
             {
              *min_vel = smooth_vel[i];
              *min_vel_index = i;
             }

          if ( smooth_vel[i] > *max_vel)
           {
            *max_vel = smooth_vel[i];
            *max_vel_index = i;
           }
          } /* END of if (smooth_vel[i] < SEVEN_SEVEN_SEVEN) */
         }/* END of for ( i = start_ind; i <= end_ind; i++) */

}
