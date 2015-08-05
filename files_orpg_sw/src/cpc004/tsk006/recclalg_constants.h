/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/08/04 14:35:41 $
 * $Id: recclalg_constants.h,v 1.4 2005/08/04 14:35:41 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef RECCLALG_CONSTANTS_H
#define RECCLALG_CONSTANTS_H

/* Constant definitions for REC algorithm */
#define REC_NO_DATA_FLAG     -1	/* Flag used to tag bins with no data */
#define REC_MISSING_FLAG    -99	/* Flag used to tag bins with data below SNR threshold	*/
#define REC_RF_FLAG	    -88	/* Flag used to tag bins with range folded data	*/
#define REC_BELOW_SNR_CODE    0	/* Encoded value for data below SNR threshold */
#define REC_RANGE_FOLDED_CODE 1 /* Encoded value for range folded base data */

#define DOP_RES_FACTOR     0.5  /* Factor to convert Doppler resolution to m/s  */

#endif

