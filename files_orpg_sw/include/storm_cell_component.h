/* 
 * RCS info 
 * $Author: ccalvert $ 
 * $Locker:  $ 
 * $Date: 2007/01/30 22:57:00 $ 
 * $Id: storm_cell_component.h,v 1.7 2007/01/30 22:57:00 ccalvert Exp $ 
 * $Revision: 1.7 $ 
 * $State: Exp $ 
 */ 
/*	This header file defines the structure for the storm cell	*
 *	Id and tracking algorithms.  It corresponds to common block	*
 *	STORM_CELL_COMP in the legacy code.				*/


#ifndef STORM_CELL_COMP_H
#define	STORM_CELL_COMP_H

#include <orpgctype.h>


/*	Storm Cell Components/Centroids Data
	The declarations in this file are duplicated in storm_cell.h
	because of time constraints for OB1.  The declarations for
	storm_cell_comp_t in this file should be included in
	storm_cell.h for OB2.
*/

#ifndef STORM_CELL_MAX_REF_THRESH
#define STORM_CELL_MAX_REF_THRESH 7
#endif

#define	STORM_CELL_MAX_RADIUS_THRESH	3
#define STORM_CELL_COMP_DEA_NAME "alg.storm_cell_component"


typedef	struct {

	fint	segment_overlap;	/*# @name "Thresh (Segment Overlap)"
					    @desc Minimum overlap in sample volumes in order for
						  segments to be identified as part of the same component.
					    @default 2  @min 0  @max 5  @units bins
					    @legacy_name OVLAPADJ
					*/
	fint	max_pot_comp_per_elev;	/*# @name "Thresh (Max Pot Comps/Elv)"
					    @desc Maximum number of components that can be defined in each
						  elevation scan.
                                            @default 70  @min 10  @max 100  @units "storm components"
					    @legacy_name MXPOTCMP
					*/
	fint	max_comp_per_elev;	/*# @name "Thresh (Max Comps/Elev)"
					    @desc "Maximum number of storm cells that can be detected per ",
						  " volume scan (prior to deletion/merger)."
					    @default 120 @min 20  @max 120  @units "storm components"
					    @legacy_name NUMCMPMX
					*/
	fint	max_detect_cells;	/*# @name "Thresh (Max Detected Cells)"
					    @desc "Maximum number of storm cells that can be detected per",
						  " volume scan (prior to deletion/merger)."
					    @default 130  @min 20  @max 130  @units "storm cells"
					    @legacy_name MXDETSTM
					*/
	fint	num_segs_per_comp;	/*# @name "Thresh (# Segments/Comp)"
					    @desc Minimum number of segments required in a component.
					    @default 2  @min 1  @max 4  @units "storm segments"
					    @legacy_name NBRSEGMN
					*/
	fint	max_cells_per_vol;	/*# @name "Thresh (Max Cells/Volume)"
					    @desc Maximum number of cells that can be defined per volume
						  scan.
					    @default 100  @min 20  @max 100  @units "storm cells"
					    @legacy_name NUMSTMMX
					*/
	fint	max_vil;		/*# @name "Thresh (Max Cell Based VIL)"
					    @desc Maximum vertically integrated liquid in a storm cell.
						  VILs are clipped at this value.
					    @default 120  @min  1  @max 120  @units "kg/m**2"
					    @legacy_name STMVILMX
					*/
	freal	comp_area[STORM_CELL_MAX_REF_THRESH];
					/*#  @name "Thresh - Component Area #"
					     @desc Minimum area of component composed of segments
						   identified with relevant threshold(reflectivity).
					     @default 10.0  @min 10.0  @max 30.0  @units "km**2"  @precision 1
					     @legacy_name CMPARETH
					*/
	freal	search_radius [STORM_CELL_MAX_RADIUS_THRESH];
					/*#  @name "Thresh - Search Radius #"
					     @desc "Maximum horizontal distance between two components' ",
						   "centroids on adjacent elevation angles for correlating ",
						   "into the same storm cell."
					     @default 5.0, 7.5, 10.0  @min 1.0
					     @max 10.0, 12.5, 15.0    @units km   @precision 1
					     @legacy_name RADIUSTH
					*/
	freal	depth_delete;		/*# @name "Thresh (Depth Delete)"
					    @desc Maximum difference in depths of two storm cells required to
						  delete one of the storm cells.
					    @default 4.0  @min 0.0  @max 10.0  @units km  @precision 1
					    @legacy_name DEPTHDEL
					*/
	freal	horiz_delete;		/*# @name "Thresh (Horizontal Delete)"
					    @desc Maximum horizontal difference between two centroids required
						  to delete one of the storm cells.
					    @default 5.0  @min 3.0  @max 30.0  @units km  @precision 1
					    @legacy_name HORIZDEL
					*/
	freal	elev_merge;		/*# @name "Thresh (Elevation Merge)"
					    @desc Maximum difference in elevation angles between the top of
						  one storm cell and the bottom of another. Required to merge
						  the storm cells.
					    @default 3.0  @min 1.0  @max 5.0  @units degrees  @precision 1
					    @legacy_name ELVMERGE
					*/
	freal	height_merge;		/*# @name "Thresh (Height Merge)"
					    @desc Maximum difference in height between the top of one storm cell
						  and the bottom of another.  Required to merge the storm cells.
					    @default 4.0  @min 1.0  @max 8.0  @units km  @precision 1
					    @legacy_name HGTMERGE
					*/
	freal	horiz_merge;		/*# @name "Thresh (Horizontal Merge)"
					    @desc Maximum horizontal distance between two centroids. Required to
						  merge the storm cells.
					    @default 10.0  @min 5.0  @max 20.0  @units km  @precision 1
					    @legacy_name HRZMERGE
					*/
	freal	azi_separation;		/*# @name "Thresh (Az Separation)"
					    @desc Maximum azimuthal separation for segments to be identified as
						  part of he same component.
					    @units degrees  @default 1.5  @min 1.5  @max 3.5  @precision 1
					    @legacy_name AZMDLTHR
					*/
} storm_cell_comp_t;


#endif


