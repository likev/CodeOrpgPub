/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:45 $
 * $Id: layer_reflectivity.h,v 1.10 2007/01/30 22:56:45 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
/*	This header file defines the structure for the layer reflect	*
 *	AP Removal function.  It corresponds to common block LAYER_DBZ	*
 *	in the legacy code.						*/


#ifndef LAYER_REFLECTIVITY_H
#define	LAYER_REFLECTIVITY_H

#include <orpgctype.h>

#define LAYER_REF_DEA_NAME "alg.layer_reflectivity"

/*	Layer Reflectivity/AP Removal Data		*/
typedef struct {

	freal	min_cltr_dbz;		/*#  @name "Min Clutter Reflectivity"
					     @desc Minimum clutter @units dBZ @min 5.0 @max 20.0
					     @default 10.0  @precision 0 @legacy_name AMINDBZ
					*/
	fint	altitude_omit;		/*# @name "Omit All Altitude"
					    @desc Omit All Altitude
					    @units km @min 0   @max 5   @default 5  @legacy_name ADALTLIM1
					*/
	fint	altitude_accept;	/*# @name "Accept if Altitude"
					    @desc Accept if Altitude
					    @units km @min 0 @max 10 @default 3 @legacy_name ADALTLIM2
					*/
	fint	distance_omit;		/*# @name "Omit All Distance (Outer)"
					    @desc Omit All Distance Outer
					    @units km @min 0 @max 100 @default 45 @legacy_name ADDISLIM1
					*/
	fint	distance_accept;	/*# @name "Accept if Distance (Outer)"
					    @desc Accept if distance outer
					    @units km  @min 0  @max 300 @default 103  @legacy_name ADDISLIM2
					*/
	fint	distance_reject;	/*# @name "Reject if Distance (Outer)"
					    @desc Reject if distance outer
					    @units km  @min 0  @max 300  @default 230 @legacy_name ADDISLIM3
					*/
	freal	elevation_accept;	/*# @name "Accept if Maximum Elevation"
					    @desc Accept if Maximum Elevation
					    @units degrees @min 0.0  @max 5.0  @default 0.5  @precision 1 @legacy_name ADELVLIM1
					*/
	freal	elevation_reject;	/*# @name "Reject if Maximum Elevation"
					    @desc Reject if Maximum Elevation
					    @units degrees @min 0.0  @max 15.0 @default 5.0  @precision 1 @legacy_name ADELVLIM2
					*/
	freal	velocity_reject;	/*# @name "Reject if Minimum Velocity"
					    @desc Reject if minimum velocity
					    @units "m/s" @min 0.0 @max 5.0 @default 1.0 @precision 1
					    @legacy_name ADREJVEL
					*/
	freal	width_reject;		/*# @name "Reject if Minimum Spectrum Width"
					    @desc Reject if minimum spectrum width
					    @units km  @min 0.0 @max 5.0  @default 0.5  @precision 1
					    @legacy_name ADREJWID
					*/
	freal	velocity_accept;	/*# @name "Accept if Minimum Velocity"
					    @desc Accept if minimum velocity
					    @units "m/s"  @min 0.0 @max 5.0  @default 1.0  @precision 1
					    @legacy_name ADACPTVEL
					*/
	freal	width_accept;		/*# @name "Accept if Minimum Spectrum Width"
					    @desc Accept if minimum spectrum width
					    @units "m/s"  @min 0.0  @max 5.0  @default 0.5  @precision 1
					    @legacy_name ADACPTWID
					*/
	flogical	cbd_phase;	/*# @name "Clutter Bloom/Dilation(CBD) Phase"
					    @desc "Clutter Bloom/Dilation(CBD) Phase"
					    @enum_values "No", "Yes"
					    @default "Yes"
					    @legacy_name ADIFEXTND
					*/
	fint	cbd_bins;		/*# @name "CBD Maximum # Range Bins"
					    @desc CBD Maximum # Range Bins
					    @units bins @min 0 @max 20 @default 4
					    @legacy_name ADRNGGATE
					*/
	freal	cbd_dbz;		/*# @name "CBD Maximum Reflectivity"
					    @desc CBD Maximum Reflectivity
					    @units dBZ  @min 0.0  @max 30.0  @default 10.0 @precision 0
					    @legacy_name ADDBZDIFF
					*/
	flogical	median_phase;	/*# @name "Median Averaging(MA) Phase"
					    @desc Median Averaging(MA) Phase
					    @enum_values "No", "Yes"
					    @default "Yes"
					    @legacy_name ADIFMEDIAN
					*/
	fint	median_bins;		/*# @name "MA Maximum Range Bin Difference"
					    @desc "MA Maximum Range Bin Difference"
					    @units bins @min 0 @max 5 @default 1
					    @legacy_name ADRNGEDIAN
					*/
	fint	median_range;		/*# @name "MA Maximum Cross Range"
					    @desc "MA Maximum Cross Range"
					    @units km @min 0  @max 10  @default 2
					    @legacy_name ADCRMEDIAN
					*/
	freal	median_dbz;		/*# @name "Median Filter Percent Good"
					    @desc Median averaging filter percent good
					    @units "%"  @min 0  @max 100  @default 50  @precision 1
					    @legacy_name ADPGDMEDIAN
					*/
} layer_dbz_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/ 
#endif
