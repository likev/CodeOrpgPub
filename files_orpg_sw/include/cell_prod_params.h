/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:33 $
 * $Id: cell_prod_params.h,v 1.12 2007/01/30 22:56:33 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/*	This header file defines the structure for cell product		*
 *	parameters.  It corresponds to common block CELL_PROD_PARAMS	*
 *	in the legacy code.						*/



#ifndef CELL_PROD_PARAMS_H
#define	CELL_PROD_PARAMS_H

#include <orpgctype.h>

#define CELL_PROD_DEA_NAME "cell_prod_params_t"


/*	Cell Product Parameters

	Note to Algorithm Developers:  When adding new fields, add them to the
	end of each data structure.
*/

typedef struct {

	fint	sti_alpha;		/*# @name "Max # Cells - STI Alphanumeric Product"
					    @desc Maximum number of cells in STI alphanumeric product.
					    @default 34  @min 7  @max 100  @units "storm cells"
					*/
	fint	ss_alpha;		/*# @name "Max # Cells - SS Alphanumeric Product"
					    @desc Maximum number of cells in SS alphanumeric product.
					    @default 40  @min 10  @max 100 @units "storm cells"
					*/
	fint	hail_alpha;		/*# @name "Max # Cells - Hail Alphanumeric Product"
					    @desc Maximum number of cells in hail alphanumeric product.
					    @default 40  @min 10  @max 100  @units "storm cells"
					*/
	fint	sti_attrib;		/*# @name "Max # Cells - STI Attribute Table"
					    @desc Maximum number of cells in STI attribute table.
					    @default 36  @min 6  @max 100 @units "storm cells"
					*/
	fint	comb_attrib;		/*# @name "Max # Cells - Combined Attribute Table"
					    @desc Maximum number of cells in combined attributes table.
					    @default 32  @min 4  @max 100  @units "storm cells"
					*/
	fint	hail_attrib;		/*# @name "Max # Cells - Hail Attribute Table"
					    @desc Maximum number of cells in hail attribute table.
					    @default 36  @min 4  @max 100  @units "storm cells"
					*/

} cell_prod_params_t;


/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/

#endif
