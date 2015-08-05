/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:17:34 $
 * $Id: mda1d_clean_duplicate.c,v 1.2 2003/07/11 19:17:34 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_clean_duplicate.c                               *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This module finds the duplicate vectors and then      *
 *			compress vector array				      *
 *      Input:                                                                *
 *        mda_shr_vect[] - an array conatining all shear vectors              *
 *                                                                            *
 *      Output:                                                               *
 *        mda_shr_vect[] - updated mda_shr_vect[]                             *
 *      Return:         none                                                  *
 *      Global:         none                                                  *
 *      notes:          none                                                  *
 ******************************************************************************/
#include <stdlib.h>

#include "mda1d_acl.h"
#include "mda1d_parameter.h"

#define		NINE_NINE_NINE	999.0 /* represents an invalid data */
#define		NINETY_NINE	99.0  


/* acknowledge global variables */
        extern int num_mda_vect;
        extern Shear_vect mda_shr_vect[MESO_MAX_VECT];

void mda1d_clean_duplicate() 
{
	/* delcare local variables */
	int i, j; /* loop index */
        int num_mda_vect_new;

	/* check for duplicate vectors */
        num_mda_vect_new = num_mda_vect;

        for (i = 0; i < num_mda_vect -1; i++)
	 {
          for (j = i+1; j < num_mda_vect -1; j++)
           {
            if (mda_shr_vect[i].range == mda_shr_vect[j].range )
             {
	      if ((mda_shr_vect[i].fvm == NINETY_NINE || mda_shr_vect[j].fvm  ==NINETY_NINE) &&
 		  mda_shr_vect[i].end_azm == mda_shr_vect[j].end_azm) 
               {
                num_mda_vect_new -= 1;
                if (mda_shr_vect[i].vel_diff < mda_shr_vect[j].vel_diff)
		 mda_shr_vect[i].fvm = NINE_NINE_NINE;
                else
                 mda_shr_vect[j].fvm = NINE_NINE_NINE; 
               }
             }/* END of if (mda_shr_vect[i].range == mda_shr_vect[j].range ) */
            else
             {
              break;
             } 
           }/* END of for (j = i+1; j < num_mda_vect -1; j++) */
         } /* END of for (i = 0; i < num_mda_vect -1; i++) */

	/* if duplicate vectors found, compress vector array *
         * and reset num_mda_vect to the new smaller value.  */
        if (num_mda_vect_new < num_mda_vect)
         {
	  for (i = 0; i < num_mda_vect_new; i++)
	   {
            while (mda_shr_vect[i].fvm == NINE_NINE_NINE)
             {
              num_mda_vect -= 1;
              for (j = i; j < num_mda_vect; j++)
               {
                mda_shr_vect[j] = mda_shr_vect[j+1];
               }
             }/* END of while (mda_shr_vect[i].fvd == NINE_NINE_NINE) */
           } /* for (i = 0; i < num_mda_vect_new; i++) */
          } /* END of if (num_mda_vect_new < num_mda_vect) */
 		
	
} /* END of this function */
