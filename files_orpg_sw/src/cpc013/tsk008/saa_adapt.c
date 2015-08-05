/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:43 $
 * $Id: saa_adapt.c,v 1.2 2008/01/04 20:54:43 aamirn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/********************************************************************
  File	: saa_adapt.c
  Details: Initializes the temporary saa_adapt struct.
*********************************************************************/

#include "saa_adapt.h"
#include <orpg.h>
#include <rpgcs.h>
/****************************************************************
Method: initializeAdaptData
Details:initializes the adapatation structure
  @@@This is for temporary use@@@
****************************************************************/
void initializeAdaptData(saa_adapt_t* saa_adapt){
	
	/*Check for NULL pointer*/
	if(saa_adapt == NULL){
		LE_send_msg(GL_ERROR,"SAA:initializeAdaptData - NULL pointer passed in as argument.\n");
		return;
	}
	
	saa_adapt->cf_ZS_mult 		= 120;
	saa_adapt->cf_ZS_power 		= 2.0;
	saa_adapt->s_w_ratio 		= 11.8;
	saa_adapt->thr_lo_dBZ 		= 5.0;
	saa_adapt->thr_hi_dBZ 		= 40.0;
	saa_adapt->thr_mn_hgt_corr 	= 0.45;
	saa_adapt->cf1_rng_hgt 		= 0.8414;
	saa_adapt->cf2_rng_hgt 		= 0.0040;
	saa_adapt->cf3_rng_hgt 		= 0.0000;
	saa_adapt->thr_time_span 	= 11/60.0; /* 11 minutes ~ 0.183333 */
	saa_adapt->thr_mn_pct_time 	= 0.90;
	saa_adapt->flag_reset_accum	= 0;
	saa_adapt->use_RCA_flag 	= 0;
	saa_adapt->rhc_base_elev 	= 0.5;

	return;

}/*end initializeAdaptData*/ 
/***************************************************************/
