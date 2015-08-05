/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:17:51 $
 * $Id: mda1d_vect_sort.c,v 1.2 2003/07/11 19:17:51 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         mda1d_vect_sort.c                                     *
 *      Author:         Yukuan Song                                           *
 *      Created:        August 26, 2002                                       *
 *      References:     WDSS MDA Fortran Source code                          *
 *                      ORPG MDA AEL                                          *
 *                                                                            *
 *      Description:    This module sort shear vectors by decreasing range    *
 *      Input:                                                                *
 *        mda_shr_vect[] - an array conatining all shear vectors              *
 *                                                                            *
 *      Output:                                                               *
 *        mda_shr_vect[] - updated mda_shr_vect[]			      *
 *      Return:         none                                                  *
 *      Global:         none                                                  *
 *      notes:          none                                                  *
 ******************************************************************************/
#include <stdlib.h>

#include "mda1d_acl.h"
#include "mda1d_parameter.h"

typedef struct {
  double range;
  int num;
} mesoshr;
int shrsort(const void *p1,const void *p2);

/* acknowledge global variables */
        extern int num_mda_vect;
        extern Shear_vect mda_shr_vect[MESO_MAX_VECT];

void mda1d_vect_sort()
{

	/* declare local variables */
	int i, j;
        mesoshr shr[MESO_MAX_VECT];
	Shear_vect mda_shr_vect_tmp[MESO_MAX_VECT];

	/* sort the vector by decreasing range */
        for (i = 0; i< num_mda_vect; i++)
         {
          shr[i].range = mda_shr_vect[i].range;
          shr[i].num = i;
         }  
        
        /* call libray function qsort() */
        qsort(shr,num_mda_vect,sizeof(shr[0]),shrsort);

	/* create a tmp new shear array in which the shear *
	 * have been sorted by the decreasing range        */
        for (j = 0; j < num_mda_vect; j ++)
         {
          mda_shr_vect_tmp[j] = mda_shr_vect[shr[j].num];
         }
        for (j = 0; j < num_mda_vect; j ++)
         {
          mda_shr_vect[j] = mda_shr_vect_tmp[j];
	 }
} /* END of this function */

/* ===============================================================
   Description:
	comparison function 
   Inputs:
	two pointers to the compared values
   Outputs:
        none 
   Returns:
        0, 1, -1 
   Notes:

============================================================ */

int shrsort(const void *p1,const void *p2)
{
  double diff;
  diff = ((mesoshr *)p1)->range - ((mesoshr *)p2)->range;
  if (diff > 0) return(1);
  else if (diff < 0) return(-1);
  return(0);
/*  return(((mesoshr *)p1)->range - ((mesoshr *)p2)->range);
*/
}

