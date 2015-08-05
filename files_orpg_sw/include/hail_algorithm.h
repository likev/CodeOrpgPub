/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:36 $
 * $Id: hail_algorithm.h,v 1.9 2007/01/30 22:56:36 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/*	This header file defines the structure for the hail detection	*
 *	algorithm.  It corresponds to common block HAIL_ALGORITHM	*
 *	in the legacy code.						*/


#ifndef HAIL_ALGORITHM_H
#define	HAIL_ALGORITHM_H


/*
	Declarations in this file are duplicated with the definitions in
	hail_detection.h.  The hail_detection.h include file should be phased
	out for OB2.  New algorithms should add new fields to data structures
	defined in this include file.  New fields should always be added to
	the end of existing data structures.  Until, hail_detection.h is
	phased out, it will be updated when a new algorithm is integrated
	into the system.
*/

#include <orpgctype.h>

#define HAIL_DEA_NAME "alg.hail"


/*	Hail Algorithm  Data	*/

typedef struct {

	fint	hke_ref_wgt_low;	/*#  @name "Thr HKE Ref Wgt Lower Limit"
					     @desc Lower limit of reflectivity used in the reflectivity
						   weighting function.
					     @default 40  @min 20  @max 60  @units dBZ
					     @legacy_name REF_W_LL
					*/
	fint	hke_ref_wgt_high;	/*#  @name "Thr HKE Ref Wgt Upper Limit"
					     @desc Upper limit of reflectivity used in the reflectivity
						   weighting function.
					     @default 50  @min 30  @max 70  @units dBZ
					     @legacy_name REF_W_UL
					*/
	fint	poh_min_ref;		/*#  @name "Thr Min Reflectivity POH"
					     @desc Minimum maximum reflectivity of component used in the
						   calculation of the probability of hail.
					     @default 45  @min 30  @max 60  @units dBZ
					     @legacy_name MNPOHREF
					*/
	freal	hke_coef1;		/*#  @name "HKE Coefficient 1"
					     @desc Multiplicative factor used in computing
						   the hailfall kinetic energy.
					     @default 0.0005 @min 0.000001 @max 1.0  @precision 6 @units coefficient
					     @legacy_name HKECOEF1
					*/
	freal	hke_coef2;		/*#  @name "HKE Coefficient 2"
					     @desc  Multiplicative exponential factor used in computing the
						    hailfall kinetic energy.
					     @default 0.084  @min 0.005  @max 0.5  @units coefficient
					     @legacy_name HKECOEF2
					*/
	freal	hke_coef3;		/*#  @name "HKE Coefficient 3"
					     @desc Operand factor used in computing hailfall kinetic energy.
					     @default 10.0  @min 1.0  @max 100.0  @units coefficient  @precision 1
					     @legacy_name HKECOEF3
					*/
	freal	posh_coef;		/*#  @name "POSH Coefficient"
					     @desc Multiplicative factor used in computing the probability of severe
						   hail from the severe hail index.
					     @default 29.0  @min 1.0  @max 100.0  @units coefficient  @precision 1
					     @legacy_name POSHCOEF
					*/
	fint	posh_offset;		/*#  @name "POSH Offset"
					     @desc Offset used in computing the probability of severe hail from the
						   severe hail index.
					     @default 50  @min 1  @max 100  @units %  @authority "READ | OSF_WRITE | URC_WRITE"
					     @legacy_name POSHOFST
					*/
	fint	max_hail_range;		/*#  @name "Max Hail Processing Range"
					     @desc Maximum range of the storm cell (centroid) that is processed by the
						   hail algorithm.
					     @default 230 @min 200 @max 460  @units km
					     @legacy_name MXHALRNG
					*/
	freal	shi_hail_size_coef;	/*#  @name "SHI Hail Size Coefficient"
					     @desc Multiplicative factor used in computing the maximum expected hail
						   size from the severe hail index.
					     @default 0.10  @min 0.01  @max 100.0  @precision 2 @units coefficient
					     @legacy_name HAILSZCF
					*/
	freal	shi_hail_size_exp;	/*#  @name "SHI Hail Size Exponent"
					     @desc Power to which the severe hail index is raised in computing the
						   maximum expected hail size.
					     @default 0.5  @min 0.1  @max 100.0  @units exponent  @precision 1
					     @legacy_name HAILSZEX
					*/
	freal	warn_thr_sel_mod_coef;	/*#  @name "WTSM Coefficient"
					     @desc "Factor applied by the Height 0 deg C in the warning threshold selection model."
					     @default 57.5  @min 0.0  @max 500.0  @precision 1  @units "100 * joules/meter**2/sec"
					     @legacy_name WT_SM_CF
					*/
	freal	warn_thr_sel_mod_off;	/*#  @name "WTSM Offset"
					     @desc Offset used in the warning threshold selection model.
					     @default -121.0  @min -500.0  @max 500.0  @units "10**5 Joules/meter/sec"
					     @legacy_name WT_SM_OF  @precision 1
					*/
	freal	poh_height_diff1;	/*#  @name "POH Height Difference #1"
					     @desc "Maximum height difference which correlates to a 0% POH."
					     @default 1.6  @min 0.0  @max 20.0  @units km  @precision 1
					     @legacy_name POHTDIF1
					*/
	freal	poh_height_diff2;	/*#  @name "POH Height Difference #2"
					     @desc "Maximum height difference which correlates to a 10% POH."
					     @default 1.9  @min 0.0  @max 20.0  @units km  @precision 1
					     @legacy_name POHTDIF2
					*/
	freal	poh_height_diff3; 	/*#  @name "POH Height Difference #3"
					     @desc "Maximum height difference which correlates to a 20% POH."
					     @default 2.1  @min 0.0  @max 20.0  @units km  @precision 1
					     @legacy_name POHTDIF3
					*/
	freal	poh_height_diff4;	/*#  @name "POH Height Difference #4"
					     @desc "Maximum height difference which correlates to a 30% POH."
					     @default 2.4  @min 0.0  @max 20.0  @units km  @precision 1
					     @legacy_name POHTDIF4
					*/
	freal	poh_height_diff5;	/*#  @name "POH Height Difference #5"
					     @desc "Maximum height difference which correlates to a 40% POH."
					     @default 2.6 @min 0.0  @max 20.0  @units km  @precision 1
					     @legacy_name POHTDIF5
					*/
	freal	poh_height_diff6;	/*#  @name "POH Height Difference #6"
					     @desc "Maximum height difference which correlates to a 50% POH."
					     @default 2.9  @min 0.0  @max 20.0  @units km  @precision 1
					     @legacy_name POHTDIF6
					*/
	freal	poh_height_diff7;       /*#  @name "POH Height Difference #7"
					     @desc "Maximum height difference which correlates to a 60% POH."
					     @default 3.3  @min 0.0  @max 20.0  @units km  @precision 1
					     @legacy_name POHTDIF7
					*/
	freal	poh_height_diff8;	/*#  @name "POH Height Difference #8"
					     @desc "Maximum height difference which correlates to a 70% POH."
					     @default 3.8  @min 0.0  @max 20.0  @units km  @precision 1
					     @legacy_name POHTDIF8
					*/
	freal	poh_height_diff9;	/*#  @name "POH Height Difference #9"
					     @desc "Maximum height difference which correlates to a 80% POH."
					     @default 4.5  @min 0.0  @max 20.0  @units km  @precision 1
 					     @legacy_name POHTDIF9
					*/
	freal	poh_height_diff10;	/*# @name "POH Height Difference #10"
					    @desc "Maximum height difference which correlates to a 90% POH."
					    @default 5.5 @min 0.0  @max 20.0  @units km  @precision 1
					    @legacy_name POHTDIF0 */
	fint	rcm_probable_hail;	/*# @name "Thresh (RCM probable hail)"
					    @desc Threshold of probability of severe hail at which a cell
						  is assigned a hail label of Probable for the RCM.
					    @default 30  @min 0  @max 100  @units "%"
					    @legacy_name RCMPRBHL
					*/
	fint	rcm_positive_hail;	/*# @name "RCM positive hail"
					    @desc Threshold of probability of severe hail POSH at which a
						  cell is assigned a hail label of Positive for the RCM.
					    @default 50 @min 0  @max 100 @units "%"
					    @legacy_name RCMPOSHL
					*/
	freal	height_0;		/*# @name "Height (0 Deg Celsius)"
					    @desc "Height(MSL) of the 0 degree Celsius environmental temperature."
					    @default 10.5  @min 0  @max 0  @units kft  @authority "READ | OSF_WRITE | URC_WRITE"
					    @legacy_name HT0  @precision 1
					*/
	freal	height_minus_20;	/*# @name "Height (-20 Deg Celsius)"
					    @desc "Height(MSL) of the -20 degree celsius environmental temperature."
					    @default 20.0  @min 0  @max 70  @units kft  @precision 1
					    @authority "READ | OSF_WRITE | URC_WRITE"
					    @legacy_name HT20
					*/
	fint	hail_date_yy;		/*# @name "Hail Date YY"
					    @desc "Year of the last modification to the 0 degree and -20 degree temperature data."
					    @default 96  @min 0  @max 99  @units year
					    @legacy_name HADATE
					*/
	fint	hail_date_mm;		/*# @name "Hail Date MM"
					    @desc "Month of the last modification to the 0 degree and -20 degree temperature data."
					    @default 1  @min  1  @max 12  @units month
					    @legacy_name HADATE
					*/
	fint  	hail_date_dd;		/*# @name "Hail Date DD"
					    @desc "Day of the last modification to the 0 degree and -20 degree temperature data."
					    @default 1  @min 1  @max 31  @units day
					    @legacy_name HADATE
					*/
	fint	hail_time_hr;		/*# @name "Hail Time HH"
					    @desc "Hour of the last modification to the 0 degree and -20 degree temperature data."
					    @default 12 @min 0  @max 23  @units hrs
					    @legacy_name HATIME
					*/
	fint 	hail_time_min; 		/*# @name "Hail Time MM"
					    @desc "Minute of the last modification to the 0 degree and -20 degree temperature data."
					    @default 0  @min 0  @max 59  @units mins
					    @legacy_name HATIME
					*/
	fint 	hail_time_sec; 		/*# @name "Hail Time SS"
					    @desc "Seconds of the last modification to the 0 degree and -20 degree temperature data."
					    @default 0  @min 0  @max 59  @units secs
					    @legacy_name HATIME
					*/
} hail_algorithm_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/

#endif
