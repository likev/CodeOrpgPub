/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/02/24 14:37:33 $
 * $Id: hca_adapt.h,v 1.6 2011/02/24 14:37:33 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef HCA_ADAPT_H
#define HCA_ADAPT_H


#define HCA_DEA_NAME "alg.hca"


/********************************************************
 * Hydrometeor Classification Algorithm Adaptation Data *
 ********************************************************/
 
/*** Refer to file dea/hca.alg for descriptions of these structure members ***/    

typedef struct
   {
   float  min_V_GC;    
   int    max_Z_RA;				
   float  min_RHO_RA;				
   int    min_PHIDP_RA;
   int    min_Z_RH;
   int    min_Z_HR;
   float  min_Zdr_HR;
   int    max_Z_IC;
   int    min_Z_GR;
   int    max_Z_GR;
   float  max_Zdr_GR;
   int    min_Z_BD;
   float  min_Zdr_BD;
   int    min_Z_WS;
   float  min_Zdr_WS;
   float  max_Rhohv_BI;
   int    max_Z_BI;
   float  max_Zdr_DS;
   float  min_Agg;
   float  min_Dif_Agg;
   float  min_snr;
   int    atten_control;
   float  f1_a;
   float  f1_b;
   float  f1_c;
   float  f2_a;
   float  f2_b;
   float  f2_c;
   float  f3_a;
   float  f3_b;
   float  f3_c;
   float  g1_b;
   float  g1_c;
   float  g2_b;
   float  g2_c;

/* Weights applied to each fuzzy logic input variable for
   each hydrometeor class.                                */

   float  weight_Z[NUM_CLASSES];
   float  weight_Zdr[NUM_CLASSES];
   float  weight_RHOhv[NUM_CLASSES];
   float  weight_LKdp[NUM_CLASSES];
   float  weight_SDZ[NUM_CLASSES];
   float  weight_SDPHIdp[NUM_CLASSES];
/* Membership function definition points */

   float  memRA[NUM_FL_INPUTS*NUM_X];
   float  memHR[NUM_FL_INPUTS*NUM_X];
   float  memRH[NUM_FL_INPUTS*NUM_X];
   float  memBD[NUM_FL_INPUTS*NUM_X];
   float  memBI[NUM_FL_INPUTS*NUM_X];
   float  memGC[NUM_FL_INPUTS*NUM_X];
   float  memDS[NUM_FL_INPUTS*NUM_X];
   float  memWS[NUM_FL_INPUTS*NUM_X];
   float  memIC[NUM_FL_INPUTS*NUM_X];
   float  memGR[NUM_FL_INPUTS*NUM_X];

/* Flag arrays indicating which, if any, 2-dimensional membership
   function equation is to be used.                               */

   int    memFlagRA[NUM_FL_INPUTS*NUM_X];
   int    memFlagHR[NUM_FL_INPUTS*NUM_X];
   int    memFlagRH[NUM_FL_INPUTS*NUM_X];
   int    memFlagBD[NUM_FL_INPUTS*NUM_X];
   int    memFlagBI[NUM_FL_INPUTS*NUM_X];
   int    memFlagGC[NUM_FL_INPUTS*NUM_X];
   int    memFlagDS[NUM_FL_INPUTS*NUM_X];
   int    memFlagWS[NUM_FL_INPUTS*NUM_X];
   int    memFlagIC[NUM_FL_INPUTS*NUM_X];
   int    memFlagGR[NUM_FL_INPUTS*NUM_X];
   } hca_adapt_t;

#endif /* HCA_ADAPT_H */
