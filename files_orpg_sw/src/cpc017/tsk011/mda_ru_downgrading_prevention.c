/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/07 17:54:04 $
 * $Id: mda_ru_downgrading_prevention.c,v 1.1 2004/01/07 17:54:04 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         mda_ru_downgrading_prevention.c			      *
 *	Author:		Yukuan Song					      *
 *   	Created:	Oct. 1, 2003					      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:    This program is to check if a feature is downgraded   *
 *      		Increase the downgraded attributes                    *
 *      Input:          cplt_t new_cplt[]       		              *
 *			int nbr_new_cplts				      *
 *			cplt_t first_elev_newcplt[]			      *
 *                      int nbr_first_elev_newcplt			      *
 *      output:         int nbr_new_cplts                                     *
 *									      *
 *      Notes:       							      *
 ******************************************************************************/

/*	System include files	*/
#include <stdio.h>

/*	local include files	*/
#include "mda_ru.h"
#include "mdattnn_params.h"

/* declare global parameters */
#define DEBUG	0


void mda_ru_downgrading_prevention(cplt_t new_cplt[], int nbr_new_cplts,
			    cplt_t first_elev_newcplt[], int nbr_first_elev_newcplt) {

	/* declare the local varaibles */
	int i, j;


	for ( i = 0; i < nbr_new_cplts; i++){
	 if (new_cplt[i].detection_status != 1) { 
	 for ( j = 0; j < nbr_first_elev_newcplt; j++) {
	  if (new_cplt[i].ll_azm == first_elev_newcplt[j].ll_azm &&
              new_cplt[i].ll_rng == first_elev_newcplt[j].ll_rng) {
	   if ( new_cplt[i].strength_rank < first_elev_newcplt[j].strength_rank) {
	    new_cplt[i].strength_rank = first_elev_newcplt[j].strength_rank;
            break;
	   }
          }
         }
        }
	}

} /* END of the function                                               */
