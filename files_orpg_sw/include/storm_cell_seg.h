/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:57:01 $
 * $Id: storm_cell_seg.h,v 1.8 2007/01/30 22:57:01 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/*	This header file defines the structure for the storm cell	*
 *	Id and tracking algorithms.  It corresponds to common block	*
 *	STORM_CELL_SEG in the legacy code.				*/


#ifndef STORM_CELL_SEG_H
#define	STORM_CELL_SEG_H

#include <orpgctype.h>

#define STORM_CELL_SEG_DEA_NAME "alg.storm_cell_segment"


/*	Storm Cell Segments Data	*/

#ifndef STORM_CELL_MAX_REF_THRESH
#define	STORM_CELL_MAX_REF_THRESH	7
#endif

/*	NOTE: The order of the elements in the data structures match	*
 *	the order in the legacy UCP menus (see Operator Guidance	*
 *	Handbook on Adaptable Parameters; bld10).			*/

typedef struct {
	fint	refl_threshold[STORM_CELL_MAX_REF_THRESH];
					/*# @name "Thresh - Reflectivity #"
					    @desc Minimum effective reflectivity which the sample
						  volumes must meet to be included in the same segment.
					    @default 60, 55, 50, 45, 40, 35, 30
                                            @min 0  @max 80 @units dBZ
					    @legacy_name REFLECTH
					*/
	freal	seg_length[STORM_CELL_MAX_REF_THRESH];
					/*#
					    @name "Thresh - Segment Length #"
					    @desc "Minimum length of consecutive sample bins with ",
						  "reflectivity exceeding the reflectivity threshold, ",
						  "for identification of that run of bins as a segment."
                                            @default 1.9  @min 1.0  @max 5.0 @precision 1 @units km
					    @legacy_name SEGLENTH
					*/
	fint	dropout_refl_diff;	/*# @name "Thresh (Dropout Ref Diff)"
					    @desc The difference in effective reflectivity of sample volumes
						  below reflectivity threshold that may still be included in
						  a segment identified with that reflectivity threshold.
					    @default 5  @min 0  @max 10  @units dBZ
					    @legacy_name DRREFDFF
					*/
	fint	dropout_count;		/*# @name "Thresh (Dropout Count)"
					    @desc Maximum number of contiguous sample volumes with a
						  reflectivity factor below reflectivity threshold by less
						  than or equal to dropout reflectivity difference threshold
						  that may be included in a segment identified with that
						  reflectivity threshold.
					    @default 2  @min 0  @max 5  @units volumes
					    @legacy_name NDROPBIN
					*/
	fint	num_refl_levels;	/*# @name "Nbr Reflectivity Levels"
					    @desc Number of reflectivity thresholds used to search for segments.
					    @default 7  @min 0  @max 7
					    @legacy_name NREFLEVL
					*/
	fint	max_segment_range;	/*# @name "Thresh (Max Segment Range)"
					    @desc Maximum slant range for processing or identifying segments.
					    @default  460  @min 230  @max 460  @units km
					    @legacy_name SEGRNGMX
					*/
	fint	max_segs_per_radial;	/*# @name "Max # of Segments/Radial"
					    @desc Maximum number of segments to be processed per radial.
					    @default 15  @min  10  @max 50  @units "storm segments"
					    @legacy_name RADSEGMX
					*/
	fint	max_segs_per_elev;	/*# @name "Max # of Segments/Elevation"
					    @desc Maximum number of segments to be processed per elevation.
					    @default 6000 @min 4000  @max 6000  @units "storm segments"
					    @legacy_name NUMSEGMX
					*/
	fint	refl_ave_factor;	/*# @name "Reflectivity Avg Factor"
					    @desc Number of sample volumes used for determining the maximum
						  average reflectivity factor.
					    @default 3  @min 1  @max 5  @units bins
					    @legacy_name NUMAVGBN
					*/
	freal	mass_weight_factor;	/*# @name "Mass Weighted Factor"
					    @desc Factor for converting precipitation intensity to mass of
						  liquid water per unit volume of a sample bin.
					    @default 53000  @min 50000  @max 60000  @units "hr/kg/km**4"
					    @precision 1  @legacy_name MWGTFCTR
					*/
	freal	mass_mult_factor;	/*# @name "Mass Multiplicative Factor"
					    @desc Multiplicative factor used in determining precipitation
						  intensity.  Multiplicative coefficient in the reflectivity
						  (power) to rainfall rate (i.e., Z to R) conversion formula.
					    @default 486.0  @min 450  @max 550  @units "(mm**6/m**3)*(hr/mm)**n)"
					    @precision 1 @legacy_name MULTFCTR
					*/
	freal	mass_coef_factor;	/*# @name "Mass Coefficient Factor"
					    @desc Power coefficient in the reflectivity to rainfall rate
						  (i.e., Z to R) conversion formula.  The power to which the
						  precip intensity is raised in calculating the effective
						  reflectivity factor.
                                            @default 1.37  @min 1.20  @max 1.50  @units "exponent"
					    @precision 2  @legacy_name MCOEFCTR
					*/
       freal	filter_kernel_size;     /* size of the kernel used to filter reflectivity

                                         */
       freal    filter_fract_req;      /* fraction required to filter out reflectivity */
       fint     filter_on;              /* Filter on/off flag */

} storm_cell_seg_t;

#endif
