
/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:27:27 $
 * $Id: hca_memLookup.c,v 1.3 2009/03/03 18:27:27 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         MemLookup                                             *
 *      Author:         Brian Klein, Yukuan Song                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    Precomputes equations that are dependent upon         *
 *                      the sample bin's reflectivity and used in the         *
 *                      two-dimensional membership functions.                 *
 *                      By creating a look-up table at startup these          *
 *                      computations do not have to be performed at run-time. *
 *                                                                            *
 *      Change History:                                                       *
 ******************************************************************************/

#include <rpgcs_data_conversion.h>
#include "hca_local.h"
#include "hca_adapt.h"

#define EXTERN extern
#include "hca_adapt_object.h"

 /****************************************************************************
>    Description:
>       Compute the two-dimensional membership function equations
>    Input:
>       Processed reflectivity
>    Output:
>       f1, f2, f3, g1, g2
>    Returns:
>    Globals:
>    Notes:
>  ***************************************************************************/
void MemLookup(){

    int Zindex;
    float Z_data;

    /* Initialize the invalid data flags */
    f1[MIN_VALUE]   = INVALID_DATA;
    f1[MIN_VALUE+1] = INVALID_DATA;
    f2[MIN_VALUE]   = INVALID_DATA;
    f2[MIN_VALUE+1] = INVALID_DATA;
    f3[MIN_VALUE]   = INVALID_DATA;
    f3[MIN_VALUE+1] = INVALID_DATA;


    /* Loop for all posssible valid reflectivity indexes */
    /* CPT&E label A */
    for( Zindex = MIN_DATA_VALUE; Zindex < MAX_DATA_VALUE; Zindex++ ) {

	/* Convert the Z index to an acutal reflectivity value in dBZ */
	Z_data = RPGCS_reflectivity_to_dBZ(Zindex);

	/* Compute the two-dimensional membership function equations */
        f1[Zindex] = (hca_adapt.f1_a*Z_data*Z_data) + (hca_adapt.f1_b*Z_data) + hca_adapt.f1_c;
        f2[Zindex] = (hca_adapt.f2_a*Z_data*Z_data) + (hca_adapt.f2_b*Z_data) + hca_adapt.f2_c;
        f3[Zindex] = (hca_adapt.f3_a*Z_data*Z_data) + (hca_adapt.f3_b*Z_data) + hca_adapt.f3_c;
        g1[Zindex] =                                  (hca_adapt.g1_b*Z_data) + hca_adapt.g1_c;
        g2[Zindex] =                                  (hca_adapt.g2_b*Z_data) + hca_adapt.g2_c;
    } /* end for all reflectivity values */

/* End of MemLookup() */
} 

