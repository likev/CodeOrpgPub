/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:42 $
 * $Id: hydromet_rate.h,v 1.7 2007/01/30 22:56:42 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef HYDROMET_RATE_H
#define	HYDROMET_RATE_H

#include <orpgctype.h>

#define HYDROMET_RATE_DEA_NAME "alg.hydromet_rate"

/*	Hydromet (Rate)			*/

typedef struct {

	fint	range_cutoff;	/*#  @name "Range Beyond Which to Apply Range-Effect Correction  [RNCUT]"
				     @desc Range beyond which a range effect correction must be applied.
				     @units km  @default 230  @min 0  @max 230
				     @legacy_name RNGCUTOFF
				*/
	freal	range_coef1;	/*#  @name "1st Coefficient of Range Effect Function  [COER1]"
				     @desc First range effect correction coefficient.
				     @units dBR  @default 0.0  @min 0.0  @max 3.0  @precision 1
				     @legacy_name RNGCOEF1
				*/
	freal	range_coef2;	/*#  @name "2nd Coefficient of Range Effect Function  [COER2]"
				     @desc Second range effect correction coefficient.
				     @units dBR  @default 1.0  @min 1.0  @max 10.0  @precision 1
				     @legacy_name RNGCOEF2
				*/
	freal	range_coef3;	/*#  @name "3rd Coefficient of Range Effect Function  [COER3]"
				     @desc Third range effect correction coefficient.
				     @units dBR  @default 0.0  @min 0.0  @max 1.0  @precision 1
				     @legacy_name RNGCOEF3
				*/
	freal	min_precip_rate; /*# @name "Min Rate Signifying Precipitation  [MNPRA]"
				     @desc Minimum precipitation rate allowed.
				     @units "mm/hr"  @default 0.0  @min  0.0  @max 10.0  @precision 1
				     @legacy_name MINPRATE
				 */
	freal	max_precip_rate; /*# @name "Max Precipitation Rate  [MXPRA]"
				     @desc Maximum precipitation rate allowed.
				     @units "mm/hr" @default 50  @min 50  @max 1600  @precision 1
				     @authority "READ | OSF_WRITE | URC_WRITE | AGENCY_WRITE"
				     @legacy_name MAXPRATE
				 */
	freal	zr_mult;	/*#  @name "Z-R Multiplier Coef. [CZM]"
				     @desc Multiplicity coefficient in the Z-R relationship.
				     @units coefficient @default 300.0  @min 30.0  @max 3000.0  @precision 0
				     @authority "READ | OSF_WRITE | URC_WRITE | AGENCY_WRITE"
				     @legacy_name CZM
				*/
	freal	zr_exp;		/*#  @name "Z-R Exponent Coef. [CZP]"
                                     @desc Power coefficient in the Z-R relationship.
                                     @units factor  @default 1.4  @min 1.0  @max 2.5 @precision 1
				     @authority "READ | OSF_WRITE | URC_WRITE | AGENCY_WRITE"
				     @legacy_name CZP
				*/
} hydromet_rate_t;

#endif
