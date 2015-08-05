/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:44 $
 * $Id: layer_prod_params.h,v 1.6 2007/01/30 22:56:44 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/*	This header file defines the structure for cell product		*
 *	parameters.  It corresponds to common block CELL_PROD_PARAMS	*
 *	in the legacy code.						*/


#ifndef LAYER_PROD_PARAMS_H
#define	LAYER_PROD_PARAMS_H

#include <orpgctype.h>

#define	LAYER_PROD_DEA_NAME "layer_prod_params_t"


/*	The following structure defines the parameters for the controur
 *	products (composite reflectivity and echo tops).  They can be
 *	found in legacy block PRODSEL.
	Note to Algorithm Developers:  When adding new fields, add them to the
	end of each data structure.
*/

typedef	struct {

	int	first_dbz_level;	/*# @name "Layer 0 Height"
					    @desc "Top of the 1st reflectivity layer"
					    @default 2  @min 0  @max 52  @units kft
					    @legacy_name PSCPNL0D
					*/
	int	second_dbz_level;	/*# @name "Layer 1 Height"
					    @desc "Top of the 2nd reflectivity layer"
					    @default 24  @min 6  @max 58  @units kft
					    @legacy_name PSCPNL1D
					*/
	int	third_dbz_level;	/*# @name "Layer 2 Height"
					    @desc "Top of the 3rd reflectivity layer"
					    @default 33  @min 12  @max 64  @units kft
					    @legacy_name PSCPNL2D
					*/
	int	fourth_dbz_level;	/*# @name "Layer 3 Height"
					    @desc "Top of the 4th reflectivity layer"
					    @default 60  @min 18  @max 70  @units kft
					    @legacy_name  PSCPNL3D
					*/
	int	dbz_range;		/*# @name "Range Limit"
					    @desc Radial range limit for the product
					    @default 460  @min 40  @max 460  @units km
					    @legacy_name  PSCPNRNG
					*/
} layer_prod_params_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/

#endif
