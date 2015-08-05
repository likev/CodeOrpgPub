/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:39 $
 * $Id: hydromet_acc.h,v 1.6 2007/01/30 22:56:39 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef HYDROMET_ACC_H
#define	HYDROMET_ACC_H

#include <orpgctype.h>

#define HYDROMET_ACC_DEA_NAME "alg.hydromet_acc"

/*	Hydromet (Accumulation)		*/

typedef struct {

	fint	restart_time;	/*# @name "Re-initialization Time Lapse Threshold (For Accum Process)  [TIMRS]"
				    @desc Elapsed time to restart.
				    @units mins @default 60  @min 45  @max 60
   				    @legacy_name TIMRESTRT
				*/
	fint	max_interp_time; /*# @name "Max Time Difference Between Scans For Interpolation  [MXTIN]"
				     @desc  Maximum period over which accumulation can be computed using two
					    precipitation rate scans.
				     @units mins  @default 30  @min 0  @max 60
				     @legacy_name MAXTIMINT
				 */
	int	min_time_period; /*# @name "Min Time Needed to Accumulate Hourly Totals  [MXTIP]"
				     @desc Minimum period of accumulation scan data withing an hourly
					   accumulation scan required to estimate hourly accumulation.
				     @units mins @default 54  @min  0   @max  60
				     @legacy_name MINTIMPD
				 */
	fint	hourly_outlier;	/*# @name "Threshold for Hourly Outlier Accumulation  [THRLI]"
				    @desc Maximum hourly rainfall amount allowed.
				    @units mm  @default  400  @min 50  @max 800
				    @legacy_name THOURLI
				*/
	fint	end_gage_time;	/*# @name "Hourly Gage Accumulation Scan Ending Time  [ENGAG]"
				    @desc Time within each hour when gage and radar accumlations are required for adjustment.
				    @units mins  @default 0  @min  0   @max 59
				    @legacy_name ENTIMGAG
				*/
	fint	max_period_acc;	/*# @name "Max Accumulation per Scan-to-Scan Period  [MXPAC]"
				    @desc Max Accumulation per scan
				    @units mm  @default 400  @min 50  @max 400
				    @legacy_name MAXPRDVAL
				*/
	fint	max_hourly_acc;	/*#
				    @name "Max Accumulation per Hourly Period  [MXHAC]"
				    @desc Max Accumulation per Hour
				    @units mm  @default 800 @min 50  @max 1600
				    @legacy_name MAXHLYVAL
				*/

} hydromet_acc_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/

#endif
