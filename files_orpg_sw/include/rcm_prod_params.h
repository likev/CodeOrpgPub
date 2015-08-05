/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:56 $
 * $Id: rcm_prod_params.h,v 1.8 2007/01/30 22:56:56 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/*	This header file defines the structure for cell product		*
 *	parameters.  It corresponds to common block CELL_PROD_PARAMS	*
 *	in the legacy code.						*/


#ifndef RCM_PROD_PARAMS_H
#define	RCM_PROD_PARAMS_H

#include <orpgctype.h>

#define	RCM_PROD_DEA_NAME "rcm_prod_params_t"


/*	The following structure defines the parameters for the RCM	*
 *	product which are defined in the legacy block PRODSEL.		*/

typedef	struct {

	float	range_thresh;		/*# @name "Range Threshold"
					    @desc "Reflectivity level that defines",
						  " the 2 color codes for RCM outside",
						  " 124 nm (-33 - 94 dBZ) "
					    @default 20.0  @min -33.0  @max 94.0  @units dBZ
					    @legacy_name THRESH_GT230 @precision 1
					*/
	int	num_storms;		/*# @name "Centroid Count"
					    @desc "Maximum number of centroids (storms) in",
						  " the RCM Product"
					    @default 12  @min 0  @max 20
					   @legacy_name NCENTROIDS
					*/

/*	The last set of fields are part of the RCM product adaptation data *
 *	but do not require an interface to the user to update them.	   */

	freal	box_size;		/*# @name "Box Size"
					    @desc "Mesh Length of the 1/16 LFM grid box MMM (RDA is located within",
					 	  " 1/4 LFM grid box MM)"
					    @default 9.9  @min 5.0  @max 20.0  @units km  @precision 3
					    @authority "READ" @legacy_name BOXSIZ
					*/
	freal	angle_of_rotation;	/*# @name "Angle of Rotation"
					    @desc "Difference be west of 105 deg and - for sites west of 105 deg)."
					    @default 12.6  @min -180  @max 180  @precision 3  @units degrees
					    @legacy_name DELTATHE
					*/
	freal	x_axis_distance;	/*# @name "X-Offset"
				            @desc "Delta X distance from the radar relative to the upper right",
						  " corner of the unrotated MM box in the LFM grid. "
					    @default 5.750  @min 0  @precision 3
					    @units km  @internal_units "km/8" @authority "READ" @legacy_name XOFF
					*/
	freal	y_axis_distance;	/*# @name "Y-Offset"
					    @desc "Delta Y distance from the radar relative to the lower right",
						  " corner of the unrotated MM box in the LFM grid."
					    @default 0.620  @min 0.0  @units km  @internal_units "km/8"
					    @precision 3 @authority "READ"  @legacy_name YOFF
					*/
} rcm_prod_params_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/

#endif


