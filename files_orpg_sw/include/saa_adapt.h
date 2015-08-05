/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2004/10/15 14:09:26 $
 * $Id: saa_adapt.h,v 1.2 2004/10/15 14:09:26 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/********************************************************************
  Created on Aug. 5, 2003; Reji Zachariah
  Temporary Module for the SAA Adapatation Data
*********************************************************************/

#ifndef SAA_LOCAL_ADAPT_H
#define SAA_LOCAL_ADAPT_H

/*
#include <orpgctype.h>
*/
#include <stdlib.h>

/****************************************************************/
/*Structure that holds adaptation data*/
typedef struct{
	float cf_ZS_mult;	    /*Z-S Multiplicative Coefficient*/
	float cf_ZS_power;	    /*Z-S Power Coefficient*/
	float s_w_ratio;        /*Snow - Water Ratio*/
	float thr_lo_dBZ;       /*Low reflectivity/isolated bin threshold (dBZ)*/
	float thr_hi_dBZ;       /*High reflectivity/outlier bin threshold (dbZ)*/
	float thr_mn_hgt_corr;  /*Minimum height correction threshold (km)*/
	float cf1_rng_hgt;	    /*Range Height Correction Coefficient #1*/
	float cf2_rng_hgt;	    /*Range Height Correction Coefficient #2*/
	float cf3_rng_hgt;	    /*Range Height Correction Coefficient #3*/
	float thr_time_span;	/*Time Span Threshold (hr)*/
	float thr_mn_pct_time;	/*Minimum percent time threshold*/
	int flag_reset_accum;	/*Accumulation reset flag*/
	int use_RCA_flag;	    /* Select whether to use the RCA correction factors
                            or the default SAA climatological range height
                            correction.  Will be made URC adaptable when the
                            RCA algorithm is implemented. */
	float rhc_base_elev;    /* Base elevation for computing climatological SAA 
                            range height correction */		
}saa_adapt_t;
/****************************************************************/

/****************************************************************
Method: initializeAdaptData
Details:initializes the adapatation structure
  @@@This is for temporary use@@@
****************************************************************/
void initializeAdaptData(saa_adapt_t* saa_adapt);

/***************************************************************/
#endif	
