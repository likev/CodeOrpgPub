/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:57:09 $
 * $Id: vil_echo_tops.h,v 1.9 2007/01/30 22:57:09 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */
/*	This header file defines the structure for the VIL-Echo Tops	*
 *	algorithm.  It corresponds to common block VIL_ECHO_TOPS in the	*
 *	legacy code.							*/


#ifndef VIL_ECHO_TOPS_H
#define	VIL_ECHO_TOPS_H

#include <orpgctype.h>

#define VIL_ECHO_TOPS_DEA_NAME "alg.vil_echo_tops"

/*	VIL/Echo Tops Data	*/

typedef struct {

	freal	beam_width;	/*# @name "Beam Width  [BW]" @desc Angular width of the radar beam between the half-power points.
				    @units "degrees" @min 0.5 @max 2.0 @default 0.5  @precision 2 @legacy_name EBMWT
				*/
	freal	min_refl;	/*#  @name "Min Ref Threshold  [MRT]" @desc Minimum reflectivity used in computing vertically
									    integrated liquid value.
				    @units dBZ @min -33 @max 95 @default 0  @precision 2 @legacy_name ENREF
				*/
	fint	max_vil;	/*#  @name "Max VIL Threshold  [MVT]"
				     @desc Maximum allowable VIL product value.  All computed VIL values above this
					   threshold will be set to this threshold for product display.
				     @units "kg/m**2" @min 1 @max 200 @default 1
			        */
} vil_echo_tops_t;


#endif
