/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:34 $
 * $Id: combined_shear.h,v 1.12 2007/01/30 22:56:34 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/*	This header file defines the structure for the combined shear	*
 *	algorithm.  It corresponds to common block COMBINED_SHEAR	*
 *	in the legacy code.						*/


#ifndef COMBINED_SHEAR_H
#define	COMBINED_SHEAR_H

#include <orpgctype.h>

#define COMBINED_SHEAR_DEA_NAME "alg.combined_shear"


/*	Combined Shear Data		*/

typedef struct {

	freal	domain_res;		/*# @name "Domain (Resolution)  [DOR]"
					    @desc Grid point spacing for the cartesian grid.
					    @units km @possible_values 0.5, 1.0, 2.0, 4.0  @default 1.0
					    @authority "READ | OSF_WRITE | URC_WRITE"
					    @legacy_name DOMAINRES */
	freal	domain_x_min;		/*# @name "Domain (X Minimum)"
					    @desc "Lower left X(W-E) coordinate relative to the radar for the cartesian grid."
					    @units km  @min -116.0  @max 0.0  @default -116.0 @precision 1
					    @authority "READ"
					    @legacy_name DOMXMIN
					*/
	freal	domain_y_min;		/*# @name "Domain (Y Minimum)"
					    @desc "Lower Left Y (S-N) coordinate relative to the radar for the cartesian grid."
					    @units km @min -116.0 @max 0.0  @default -116.0  @precision 1
					    @authority "READ"
					    @legacy_name DOMYMIN */
	freal	domain_x_size;		/*# @name "Domain (X Size)"
					    @desc "Length of the W-E side of the cartesian grid domain."
					    @units km  @min 0.0  @max 232.0  @default 232.0  @precision 1
					    @authority "READ"
					    @legacy_name DOMXSIZ */
	freal	domain_y_size;		/*# @name "Domain (Y Size)"
					    @desc "Length of the S-N side of the cartesian grid domain."
					    @units km  @min 0.0  @max 232.0  @default 232.0  @precision 1
					    @authority "READ"
				            @legacy_name DOMYSIZ
					*/
	freal	flag;			/*# @name "Flag Value"
					    @desc Default value for filtered radial shear, filtered azimuthal
						  shear, and combined shear.
					    @units "1/sec" @min -999.9  @max -1.0  @default -999.9
					    @precision 1  @authority "READ"   @legacy_name FLAGVAL
					*/
	fint	max_radius;		/*# @name "Maximum Samples (Radial)  [MSR]"
					    @desc "Maximum number of sample volumes in one radial."
					    @min 650  @max 660  @default 660
					    @legacy_name MAXSAMPRAD
					*/
	fint	points;			/*# @name "Number (Filter)  [NFL]"
					    @desc Number of data points used in the filter applied to
						  the azimuthal and radial mean shear fields.
					    @possible_values 1, 9, 25
					    @default 9
					    @legacy_name NUMPTSFILT
					*/
	fint	samples_averaged;	/*# @name "Number (Sample Volumes)  [NSV]"
					    @desc Number of contiguous sample volumes to be averaged
						  to produce estimate of average radial velocity.
					    @possible_values 3, 5
					    @default 3  @authority "READ | URC_WRITE"
					    @legacy_name NUMSAMPAVG
					*/
	freal	threshold_fraction;	/*# @name "Threshold (Number)  [NTH]"
					    @desc Minimum ratio of actual to potential azimuthal
						  and radial differences for the calculation of
						  respective shears.
					    @units ratio @min 0.01 @max 0.99 @default 0.75 @precision 2
					    @legacy_name THRNUMFRAC
					*/
	freal	threshold_non_zero;	/*# @name "Threshold (Combined Shear)  [THCS]"
					    @desc Minimum combined shear value allowed for acceptance
						  into final shear field.
                                            @authority "READ | OSF_WRITE | URC_WRITE"
					    @units "E10-3/sec" @min 0 @max 5 @default 2 @precision 1
					    @legacy_name THRCOMBSHR */
	fint	elevation;		/*# @name "Elevation Cut  [ELEV]"
         				    @desc Elevation cut number to run algorithm.
					    @units "elevation no" @authority "READ | OSF_WRITE | URC_WRITE"
					    @min 1  @max 20  @default 1  @legacy_name CMBSHREVEL
					*/
} combined_shear_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/
#endif
