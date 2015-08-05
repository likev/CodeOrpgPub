
/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:27:26 $
 * $Id: hca_degreeMembership.c,v 1.3 2009/03/03 18:27:26 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         Hca_degreeMembership                                  *
 *      Author:         Yukuan Song, Brian Klein                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    Calculate the degree of membership;                   *
 *                      Return the value of F(Dj), F(Dj,Z)                    *
 *                                                                            *
 *      Change History:                                                       *
 ******************************************************************************/

#include "hca_local.h"

#define MAX_MEMBERSHIP  1.0
#define MIN_MEMBERSHIP  0.0

 /****************************************************************************
    Description:
       Caculation of membership degree (hydro class)
    Input:
       FL data: Z, ZDR, KDP, RHOHV, SDZ, SDPHIDP
       Float points: membership points: X1, X2, X3, X4
    Output:
       F(Dj), F(Dj,Z)
    Returns:
    Globals:
    Notes:
  ***************************************************************************/

/* CPT&E label A */
float  Hca_degreeMembership(float D, float points[NUM_X])
{

/* Input data                                             *
 * float D  -- input FL data, Z,ZDR,KDP,RHOHV,SDZ,SDPHIDP *
 * Float points -- membership points, X1, X2, X3, X4      */ 

/* Determine the degree of the membership                 */
 if((points[X1] > points[X2]) || (points[X2] > points[X3]) || (points[X3] > points[X4])){
   return MIN_MEMBERSHIP;
 }
 else{
   if(D >= points[X2] && D <=points[X3])
     return MAX_MEMBERSHIP;
   else if(D <= points[X1] || D >= points[X4])
     return MIN_MEMBERSHIP;
   else if(D > points[X1] && D < points[X2])
     return (D - points[X1])/(points[X2]-points[X1]);
   else if(D > points[X3] && D < points[X4])
     return (points[X4] - D)/(points[X4] - points[X3]);
 }
 return (MIN_MEMBERSHIP);
}
