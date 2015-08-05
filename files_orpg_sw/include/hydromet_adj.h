/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:40 $
 * $Id: hydromet_adj.h,v 1.11 2007/01/30 22:56:40 ccalvert Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

#ifndef HYDROMET_ADJ_H
#define	HYDROMET_ADJ_H

#include <orpgctype.h>

#define HYDROMET_ADJ_DEA_NAME "alg.hydromet_adj"

/*	Hydromet (Adjustment)	*/

typedef struct {

	fint	time_bias;	/*# @name "Minutes After Clock Hour When Bias is Updated  [TBIES]"
				    @desc Time within each hour to begin bias computation process.
				    @units mins  @min 50 @max 59 @default 50
				    @legacy_name TIMBIEST
				*/
	fint	num_grpairs;	/*# @name "Threshold # of Gage/Radar Pairs Needed to Select Bias from Table [NGRPS]"
				    @desc Threshold # of pairs for selection of a bias value from among 
				          timespans of analysis in the Bias Table.
				    @default 10  @min  6  @max 30
				    @legacy_name MINNPAIRS
				*/
	freal	reset_bias;	/*# @name "Reset Value of Gage/Radar Bias Estimate  [RESBI]"
				    @desc Reset (long term mean) multiplicative factor used to adjust
					  radar bias when compared to gage data.
				    @default 1.0  @min 0.5  @max 2.0  @precision 1
				    @legacy_name RESETBI
				*/
	fint	longst_lag;	/*# @name "Longest Time Lag for Using Bias Value from Table [LGLAG]"
				    @desc Longest allowable lag since receipt of a valid bias table for use of
					  a bias value from that table.
				    @units hrs  @default 168  @min 100  @max 1000
				    @legacy_name LONGSTLAG
				*/																							
	flogical	bias_flag;	/*# @name "Bias Flag"
				    @desc Flag for whether to apply bias to products
				    @enum_values "False", "True"
				    @default "False"
				    @legacy_name BIAS_FLAG
				*/

} hydromet_adj_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/

#endif
