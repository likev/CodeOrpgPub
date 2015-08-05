/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/07 17:54:08 $
 * $Id: mda_ru_sort.c,v 1.1 2004/01/07 17:54:08 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         mda_ru_sort.c					      *
 *	Author:		Yukuan Song					      *
 *   	Created:	Oct. 1, 2003					      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:    This program is to sort the n2o_order array  so that  *
 *                      the time associated candidates are listed within      *
 *                      n2o_order from the closest to furthest.               *
 *      Input:          int   n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT]       *
 *			float   n2o_dist[MAX_MDA_FEAT][MAX_MDA_FEAT]        *
 *      output:         int   n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT]       *
 *									      *
 *      Notes:       							      *
 ******************************************************************************/

/*	System include files	*/

/*	local include files	*/
#include "mdattnn_params.h"
#include "mda_ru_extrapolate.h"

/*      Prototypes              */

/* Global variable */

void mda_ru_sort(float n2o_dist[MAX_MDA_FEAT][MAX_MDA_FEAT], 
                         int n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT],
				int num_features) {

	/* declare the local varaibles */
       	int i, j, ijk, inum;  /* loop index */	
        int itemp; /* A temporary variable to hold order index */

	for (i = 0; i< num_features; i++) {
	 j = 0;
         ijk = 0;
         inum = 0;

         while (j == 0) {
          if(n2o_order[i][ijk] == NO_ORDER) {
	   inum = ijk;
           j = 1;
	  } /* END of if(n2o_order[i][ijk] == NO_ORDER) */
         if (ijk  > MAX_MDA_FEAT)
          j = 1;
         
	 ijk++;
         } /* END of while (j == 0) */

	/* if inum is less than or equal to 1, no need to loop through */

	if (inum > 1) {
	 for (ijk = 0; ijk < inum; ijk++)
	  {
          for (j = 1; j < inum; j++)
           {
	    if ( n2o_dist[i][n2o_order[i][j-1]] >
                 n2o_dist[i][n2o_order[i][j]] ) {
	     itemp = n2o_order[i][j-1];
	     n2o_order[i][j-1] = n2o_order[i][j];
	     n2o_order[i][j] = itemp;
	    } 
           } /* END of for (j = 1; j < inum; j++) */
          } /* END of for (ijk = 0; ijk < inum; ijk++) */
        } /* END of if (inum > 1) */


	} /* END of for (i = 0; i< num_features; i++) */
	

} /* END of the function                                               */
