/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/02/24 14:37:34 $
 * $Id: hca_allowedHydroClass.c,v 1.8 2011/02/24 14:37:34 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         Hca_allowedHydroClass                                 *
 *      Author:         Yukuan Song, Brian Klein                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    By setting the weighted aggregate for a hydro class   *
 *                      type to a flag we effectively disallow that type      *
 *                                                                            *
 *      Change History:                                                       *
 ******************************************************************************/

/* include files */
#include <math.h>
#include <rpgc.h>
#include "hca_local.h"
#include "hca_adapt.h"
 
#define EXTERN extern
#include "hca_adapt_object.h"

/* Global Variables */

 /****************************************************************************
    Description:
       A set of rules will be determined to fix apparently wrong
       designations as a final step in classification procedure
    Input:
       Processed reflectivity; differential reflectivity;
       correlation coefficient; differential phase;
         radial velocity; bin number and azimuth number/
    Output:
       Weighted aggregation
    Returns:
    Globals:
    Notes:
  ***************************************************************************/
void Hca_allowedHydroClass(int bin_number,
                           float Z_data,
                           float Zdr_data,
                           float RhoHV_data,
                           float PhiDP_data,
			   float V_data,
                           int atten_rad,
                           float agg[NUM_CLASSES],
                           ML_bin_t Melting_layer)
{


 /* local variables */
    int i; /* loop index */

 /* Class types checked                               *
  * RA -- Light or moderate rain                      *
  * HR -- Heavy rain                                  *
  * RH -- Rain and hail                               *
  * BD -- Big drop                                    *
  * BI -- Biological                                  *
  * GC -- Ground clutter                              *
  * DS -- Dry snow                                    *
  * WS -- Wet snow                                    *
  * IC -- Ice crystals                                *
  * GR -- Graupel                                     */

 /* adaptable variables                               *
  * min_V_GC -- minimum GC velocity                   *
  * max_Z_RA -- maximum RA reflectivity               *
  * min_Z_RH -- minimum RH Reflectivity               *
  * min_Z_HR -- minimum HR Reflectivity               *
  * min_Zdr_HR -- minimum HR Diff. Reflectivity       *
  * max_Z_IC -- maximum IC Reflectivity               *
  * min_Z_GR -- Minimum GR Reflectivity               *
  * max_Z_GR -- Maximum GR Reflectivity               *
  * max_Zdr_GR -- Maximum GR Diff. Reflectivity       *
  * min_Z_BD -- Minimum BD Reflectivity               *
  * min_Zdr_BD -- minimum BD Diff. Reflectivity       *
  * min_Z_WS -- Minimum WS Reflectivity               *
  * min_Zdr_WS -- Minimum WS Reflectivity             *
  * max_Rhohv_BI -- Maximum BI Correlation Coefficent *
  * max_Zdr_DS -- Maximum DS Diff. Reflectivity       */

 /* CPT&E label A */
 /* The first two classes are place holders for the     */
 /* encoded radial data values NO_DATA and RANGE FOLDED */
 /* which are not used by HCA so always set them to     */
 /* INVALID_CLASS.                                      */
 agg[U0] = INVALID_CLASS;
 agg[U1] = INVALID_CLASS;

 /* High Velocity values are not possible in GC       */
 if ( V_data != HCA_NO_DATA && V_data != HCA_RF_DATA) {
    if (fabs(V_data) > hca_adapt.min_V_GC) 
       agg[GC] = INVALID_CLASS;
 }

 /* High Z cutoff for Moderate Rain */
 if ( Z_data > hca_adapt.max_Z_RA) 
   agg[RA] = INVALID_CLASS;

 /* Low Z cutoff for Hail */
 if ( Z_data < hca_adapt.min_Z_RH) 
   agg[RH] = INVALID_CLASS;

 /* Low Z cutoff for Heavy Rain */
 if ( Z_data <  hca_adapt.min_Z_HR || Zdr_data < hca_adapt.min_Zdr_HR) 
   agg[HR] = INVALID_CLASS;

 /* High Z cutoff for Crystals */
 if (Z_data > hca_adapt.max_Z_IC) 
   agg[IC] = INVALID_CLASS;

 /* Z Bounds Check for Grauple */
 if ( Z_data < hca_adapt.min_Z_GR || Z_data > hca_adapt.max_Z_GR || Zdr_data > hca_adapt.max_Zdr_GR) 
   agg[GR] = INVALID_CLASS;

 /* Low Z cutoff for Big Drops */
 if (Z_data < hca_adapt.min_Z_BD || Zdr_data < hca_adapt.min_Zdr_BD) 
   agg[BD] = INVALID_CLASS;

 /* Low Z cutoff for Wet Snow */
 if (Z_data < hca_adapt.min_Z_WS || Zdr_data <hca_adapt.min_Zdr_WS) 
   agg[WS] = INVALID_CLASS;

 /*High Zdr cutoff for Dry Snow*/
 if (Zdr_data > hca_adapt.max_Zdr_DS)
   agg[DS] = INVALID_CLASS;

 if ((hca_adapt.atten_control) && (atten_rad)){
   /* Biolgical RhoHV Maximum  applied only to highly attenuated radials */
   if (RhoHV_data > hca_adapt.max_Rhohv_BI)
      agg[BI] = INVALID_CLASS;
 }
 else {
   /* Biological RhoHV Maximum and Biological Z Maximum appled everywhere */
   if ((RhoHV_data > hca_adapt.max_Rhohv_BI) || (Z_data > hca_adapt.max_Z_BI))
      agg[BI] = INVALID_CLASS;
 }

 /*check for RA */
 if (RhoHV_data < hca_adapt.min_RHO_RA && PhiDP_data < hca_adapt.min_PHIDP_RA)
    agg[RA] = INVALID_CLASS; 

 /*check with melting-layer information */
 /* CPT&E label B */
 if (bin_number < Melting_layer.bin_bb) {
    /* Entire beam is below melting layer.
       Allowed categories are GC,BI,BD,RA,HR,RH */
     for(i=0; i < NUM_CLASSES; i++) {
       if ( i == GC || i == BI || i == BD || i == RA || i == HR || i == RH ) {
       /* Do nothing.  These categories are allowed. */
       } else {
       /* Remove all other categories. */
         agg[i] = INVALID_CLASS;
       }
     }
   } else if ( bin_number >= Melting_layer.bin_bb && 
               bin_number <  Melting_layer.bin_b) {
    /* Beam is part below and part in the melting layer.
       Allowed categories are GC BI WS GR BD RA HR RH */ 
    for(i = 0;i < NUM_CLASSES; i++) {
      if ( i == GC || i == BI || i == WS || i == GR || i == BD || i == RA || i == HR || i == RH) {
      /* Do nothing these categories are allowed. */
      } else {
      /* Remove all other categories. */
        agg[i] = INVALID_CLASS;
      }
    }

  } else if (bin_number >= Melting_layer.bin_b &&
             bin_number <  Melting_layer.bin_t) {
    /* Beam is completely in the melting layer.
       Allowed categories are GC BI DS WS GR BD RH */
    for(i = 0;i < NUM_CLASSES; i++) {
      if ( i == GC || i == BI || i == DS || i == WS || i == GR || i == BD || i == RH) {
      /* Do nothing. These categories are allowed. */
      } else {
      /* Remove all other categories. */
        agg[i] = INVALID_CLASS;
      }
    }

 } else if (bin_number >= Melting_layer.bin_t &&
            bin_number <  Melting_layer.bin_tt) {
    /* Beam is part in and part above the melting layer.
       Allowed categories are GC DS WS IC GR BD RH */ 
    for(i = 0; i < NUM_CLASSES; i++) {
      if ( i == GC ||  i == DS || i == WS || i == IC || i == GR || i == BD || i == RH) {
      /* Do nothing these categories are allowed. */
      } else {
      /* Remove all other categories. */
        agg[i] = INVALID_CLASS;
      }
    }
  } else if ( bin_number >= Melting_layer.bin_tt) {
    /* Beam is completely above the melting layer.
       Allowed categories are DS IC GR RH */ 
    for(i = 0; i < NUM_CLASSES; i++) {
      if ( i == DS || i == IC || i == GR || i == RH) {
      /* Do nothing these categories are allowed. */
      } else {
      /* Remove all other categories. */
        agg[i] = INVALID_CLASS;
      }
    }

  } else {
    /* Error */
    RPGC_log_msg(GL_ERROR,"There is an error in the beam broadening computation");

    exit(-1);
  }
}
