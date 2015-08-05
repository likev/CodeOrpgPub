
/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:27:26 $
 * $Id: hca_defineMembershipAndWeights.c,v 1.3 2009/03/03 18:27:26 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         DefineMembershipFuncsAndWeights                       *
 *      Author:         Brian Klein, Yukuan Song                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    Transfers all membership function definition          *
 *                      point data and input variable weight data from        *
 *                      dea-defined one-dimensional arays to multi-dimensioal *
 *                      arrays for easier processing.                         *
 *                                                                            *
 *      Change History:                                                       *
 ******************************************************************************/


#include "hca_local.h"
#include "hca_adapt.h"

#define EXTERN extern
#include "hca_adapt_object.h"

 /****************************************************************************
    Description:
       Fill the three-dimensional convenience arrays with the values
       from dea data
    Input:
       dea data
    Output:
       3-D membership array, weight array
    Returns:
    Globals:
    Notes:
  ***************************************************************************/

void DefineMembershipFuncsAndWeights() {

     int  i, x, c; /* loop index */
     int  offset;

	for (i = 0; i < NUM_FL_INPUTS; i++) {
	   for (x = X1; x < NUM_X; x++) {
           offset = i*NUM_X;

           /* These are the membership function definition values */
	      membership[RA][i][x] = hca_adapt.memRA[offset+x];
	      membership[HR][i][x] = hca_adapt.memHR[offset+x];
	      membership[RH][i][x] = hca_adapt.memRH[offset+x];
	      membership[BD][i][x] = hca_adapt.memBD[offset+x];
	      membership[BI][i][x] = hca_adapt.memBI[offset+x];
	      membership[GC][i][x] = hca_adapt.memGC[offset+x];
	      membership[DS][i][x] = hca_adapt.memDS[offset+x];
	      membership[WS][i][x] = hca_adapt.memWS[offset+x];
	      membership[IC][i][x] = hca_adapt.memIC[offset+x];
	      membership[GR][i][x] = hca_adapt.memGR[offset+x];

           /* These are the two-dimensional membership function flags */
	      membershipFlag[RA][i][x] = hca_adapt.memFlagRA[offset+x];
	      membershipFlag[HR][i][x] = hca_adapt.memFlagHR[offset+x];
	      membershipFlag[RH][i][x] = hca_adapt.memFlagRH[offset+x];
	      membershipFlag[BD][i][x] = hca_adapt.memFlagBD[offset+x];
	      membershipFlag[BI][i][x] = hca_adapt.memFlagBI[offset+x];
	      membershipFlag[GC][i][x] = hca_adapt.memFlagGC[offset+x];
	      membershipFlag[DS][i][x] = hca_adapt.memFlagDS[offset+x];
	      membershipFlag[WS][i][x] = hca_adapt.memFlagWS[offset+x];
	      membershipFlag[IC][i][x] = hca_adapt.memFlagIC[offset+x];
	      membershipFlag[GR][i][x] = hca_adapt.memFlagGR[offset+x];

	   } /* end for all membership defintion points */
	} /*end for all input types */

     /* These are the weighting factors of each input field for each */
     /* hydroclass that is determine by a membership function.       */
	for (c = FIRST_FL_CLASS; c <= LAST_FL_CLASS; c++) {
	   weight[SMZ][c]  = hca_adapt.weight_Z[c];
	   weight[ZDR][c]  = hca_adapt.weight_Zdr[c];
	   weight[RHO][c]  = hca_adapt.weight_RHOhv[c];
	   weight[LKDP][c] = hca_adapt.weight_LKdp[c];
	   weight[SDZ][c]  = hca_adapt.weight_SDZ[c];
	   weight[SDP][c]  = hca_adapt.weight_SDPHIdp[c];
	} /* end for all hydro classes */

/* End of DefineMembershipFuncsAndWeights() */
} 

