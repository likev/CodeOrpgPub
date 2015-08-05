/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:57:02 $
 * $Id: storm_cell_track.h,v 1.5 2007/01/30 22:57:02 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef STORM_CELL_TRACK_H
#define	STORM_CELL_TRACK_H

#include <orpgctype.h>

#define STORM_CELL_TRACK_DEA_NAME "alg.storm_cell_track"


/*	Storm Cell Tracking/Position Forecast Data	*/
typedef	struct {

	fint	num_past_vols;		/*# @name "Number of Past Volumes"
					    @desc Maximum number of volume scans used in correlation table.
						  Includes present scan.
					    @default 10  @min 7  @max 13  @units volumes
					    @legacy_name NPASTVOL
					*/
	fint	num_intvls;		/*# @name "Number of Intervals"
					    @desc Number of projected positions in the storm forecast.
					    @default 4  @min 1  @max 4
					    @legacy_name NUMFRCST
					*/
	fint	forecast_intvl;		/*# @name "Forecast Interval"
					    @desc Time interval for which centroid positions may be projected
						  into the future.
					    @default 15  @min 5  @max 30  @units mins
					    @precision 0  @legacy_name FRCINTVL  @round_to_nearest 5
					*/
	fint	allow_err;		/*# @name "Allowable Error"
					    @desc Maximum error in the track of a storm cell allowed for minimum
						  forecast interval.
					    @default 20  @min 10  @max 60  @units km
					    @legacy_name ALLOWERR
					*/
	fint	err_intvl;		/*# @name "Error Interval"
					    @desc Interval used in determining the allowable error.
					    @default 15  @min 5  @max 30  @units mins
					    @legacy_name ERRINTVL  @round_to_nearest 5
					*/
	fint	default_dir;		/*# @name "Default (Direction)"
					    @desc Default direction assigned to storm cell when storm cell
						  direction is not available.  Also used by the hail algorithm.
					    @default 225  @min 0  @max 360  @units degrees
					    @authority "READ | OSF_WRITE | URC_WRITE" @legacy_name DEFDIREC
					*/
	fint	max_time;		/*# @name "Time (Maximum)"
					    @desc Maximum time between successive volume scans for the storm cell
						  histories to be retained in the correlations table.
					    @default 20  @min 10  @max 60 @units mins
					    @legacy_name MAXVTIME
					*/
	freal	default_spd;		/*# @name "Default (Speed)"
					    @desc Default average speed assigned to storm cell when storm cell
						  speed is not available.  Also used by the hail algorithm.
					    @default 25.0   @min 0  @max 99.9  @units knots  @precision 1
					    @legacy_name DEFSPEED  @authority "READ | OSF_WRITE | URC_WRITE"
					*/
	freal	correlation_spd;	/*# @name "Correlation Speed"
					    @desc Speed used to compute the correlation distance.
					    @default 30  @min 10  @max 99.9 @units "m/s"
					    @legacy_name CORSPEED  @precision 1
					*/
	freal	minimum_spd;		/*# @name "Thresh (Minimum Speed)"
					    @desc Threshold of speed below which a storm is labeled as slow and
						  no motion is projected.
					    @default 2.5  @min 0.0  @max 10.0  @units "km/hr"  @precision 1
					    @legacy_name SPEEDMIN
					*/
} storm_cell_track_t;

#endif
