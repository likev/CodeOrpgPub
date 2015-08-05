/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/04/02 21:29:46 $
 * $Id: mda_ru_extrapolate.c,v 1.4 2004/04/02 21:29:46 cheryls Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/*123456789012345678901234567890123456789012345678901234567890123456789012345*/
/******************************************************************************
 *	Module:         mda_ru_extrapolate.c					      *
 *	Author:		Yukuan Song					      *
 *   	Created:	Oct. 1, 2003					      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:    This program is to find out the 3D couplets in        *
 *                      the previous volume that are qualified for being      *
 *                      extrapolated into the current volume, Features are    * 
 *                      checked to nake sure they are vertically within       *
 *                      MDA_TASSOC_DZ of the feature from the previous volume *
 *                      scan.                                                 *
 *      Input:          int   n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT]           *
 *      output:         int*  iadd                                            *
 *                      int   addnew[MAX_MDA_FEAT]                          *
 *									      *
 *      Notes:       							      *
 ******************************************************************************/

/*	System include files	*/
#include <math.h>
#include "rpgc.h"

#define FLOAT
#include "rpgcs_coordinates.h"

/*	local include files	*/
#include "mdattnn_params.h"
#include "mda_ru_extrapolate.h"

/*      Prototypes              */

/* Global variable */
extern  int    Nbr_old_cplts;
extern  cplt_t Old_cplt[];

/* declare global parameters */
#define PROCESS	1	/* A boolean to control the algorithm loop    */ 


void mda_ru_extrapolate(int* iadd, int addnew[], int n2o_order[MAX_MDA_FEAT][MAX_MDA_FEAT],
			float elevation, int num_features, int first_tilt) {

	/* declare the local varaibles */
	int ichk[MAX_MDA_FEAT];
       	int i, j;  /* loop index */	
        float cr, ht; /* central range, central height */

        /* This function is called for all elevations.  However, for the */
        /* first elevation, the input is 2D features, not 3D features.   */
        /* Because the sizing for the maximum number of 2D feature is    */
        /* different than that for 3D features (currently, 500 vs. 100)  */
        /* As a safety measure, if we exceed 100 features, we'll set it  */
        /* back to 100 and report a problem.                             */
        
        if (num_features > MAX_MDA_FEAT) {
           num_features = MAX_MDA_FEAT;
           RPGC_log_msg(GL_INFO, "MDA_RU_EXT: Too many features, using first 100\n");
        }
        	
	/* Initialize the array */
	for (i = 0; i < MAX_MDA_FEAT; i++) {
	 ichk[i] = 0;
        } /* END of for (i = 0; i < MAX_MDA_FEAT; i++) */ 

	/* Check the n2o_order array and mark ichk when a match is found */
        for (i = 0; i < num_features; i++) {
	 if ( n2o_order[i][0] != NO_ORDER) 
          ichk[n2o_order[i][0]] = 1;
	} /* END of for (i = 0; i < nbr_new_cplts; i++) */

	/* Features are checked to make sure they are vertically within
         * MDA_TASSOC_DZ of the feature from previous volume scan. */

	i = 0;
        j = 0;
        (*iadd) = 0;

        if (Nbr_old_cplts == 0) return;

        while ( j == 0) {

         if (ichk[i] == 0) {
	  cr = sqrt(Old_cplt[i].llx * Old_cplt[i].llx +
                    Old_cplt[i].lly * Old_cplt[i].lly);
	  ht = cr * sin(elevation * DTR ) + (cr * cr) / (2.0 * IR * RE);

        if ( Old_cplt[i].strength_rank > 0) {
         if ( first_tilt == 0 ) {
          if ((Old_cplt[i].strength_rank >= MESO_SR_THRESH) &&
              (Old_cplt[i].strength_rank != UNDEFINED) && 
              (Old_cplt[i].nssl_base     != UNDEFINED)) {
	    if (ht - abs(Old_cplt[i].nssl_base) <= MDA_TASSOC_DZ) {
             addnew[(*iadd)] = i;
	     (*iadd)++;
            } /* END of if (ht - abs(Old_cplt[i].base) <= MDA_TASSOC_DZ) */
          }
          else if (Old_cplt[i].core_base != UNDEFINED) {
	   if (ht - abs(Old_cplt[i].core_base) <= MDA_TASSOC_DZ) {
            addnew[(*iadd)] = i;
            (*iadd)++;
           } /* END of if (ht - abs(Old_cplt[i].core_base) <= MDA_TASSOC_DZ) */
          }
          else {
	   if (ht - abs(Old_cplt[i].base) <= MDA_TASSOC_DZ) {
            addnew[(*iadd)] = i;
            (*iadd)++;
           } /* END of if (ht - abs(Old_cplt[i].base) <= MDA_TASSOC_DZ) */
          } /* END of else */
          }/* END of if ( first_tilt == 0 ) */
         else {
	   if (ht - abs(Old_cplt[i].base) <= MDA_TASSOC_DZ) {
            addnew[(*iadd)] = i;
            (*iadd)++;
           } /* END of if (ht - abs(Old_cplt[i].base) <= MDA_TASSOC_DZ) */
	 } /* END of else */
         } /* END of if ( Old_cplt[]i].strength_rank > 0) */
        } /* END of if (ichk[i] == 0) */ 

	/* increase "i" by one */
	i++;

         if ( i == Nbr_old_cplts)
          j = 1;
         if ( i == MAX_MDA_FEAT)
          j = 1;

        } /* END of while (j == 0) */


} /* END of the function                                               */
