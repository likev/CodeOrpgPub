/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:57:06 $
 * $Id: tda.h,v 1.10 2007/01/30 22:57:06 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/*	This header file defines the structure for the TDA		*
 *	algorithm.  It corresponds to common block TDA in the		*
 *	legacy code.							*/


#ifndef TDA_H
#define	TDA_H

#include <orpgctype.h>

#define TDA_DEA_NAME "alg.tda"


/*	Tornado Detection Algorithm (TDA) Data		*/

typedef struct {

	fint	min_dbz_thresh;		/*# @name "Min Reflectivity"
					    @desc Minimum reflectivity value required in a
						  sample volume for it to be used as a pattern vector.
					    @authority "READ | OSF_WRITE | URC_WRITE"  @units dBZ
					    @default  0  @min -20  @max 20  @legacy_name MINREFL
					*/
	fint	vector_vel_diff;	/*# @name "Vector Velocity Difference"
					    @desc The minimum required gate-to-gate velocity difference
						  required for pattern vectors.
					    @default 11  @min 10  @max 75  @units "m/s"
					    @legacy_name MINPVDV
					*/
	fint	max_pv_range;		/*# @name "Max Pattern Vector Range"
					    @desc Maximum slant range at which pattern vectors are
						  identified.
					    @default  100  @min  0  @max 230  @units km
					    @authority "READ | OSF_WRITE | URC_WRITE"  @legacy_name MAXPVRNG
					*/
	freal	max_pv_height;		/*# @name "Max Pattern Vector Height"
					    @desc Maximum height at which pattern vectors are identified.
					    @default 10.0   @min 0.0  @max 15.0  @units km  @precision 0
				            @legacy_name MAXPVHT
					*/
	fint	max_pv_num;		/*# @name "Max # of Pattern Vectors"
					    @desc Maximum number of pattern vectors the algorithm can
						  process per elevation scan.
					    @default 2500  @min 1500  @max 3000
					    @legacy_name MAXNUMPV
					*/
	fint	diff_vel1;		/*# @name "Differential Velocity #1"
					    @desc "The 1st of six velocity difference threshold used as",
						  " criteria for building 2-D features. "
					    @default 11  @min 10  @max 75  @units "m/s"
					    @legacy_name TH2DDV1
					*/
	fint	diff_vel2;              /*# @name "Differential Velocity #2"
					    @desc "The 2nd of six velocity difference threshold used as",
						  " criteria for building 2-D features. "
					    @default 15  @min 15  @max 80  @units "m/s"
					    @legacy_name TH2DDV2
					*/
	fint	diff_vel3;		/*# @name "Differential Velocity #3"
					    @desc "The 3rd of six velocity difference threshold used as",
						  " criteria for building 2-D features. "
					    @default 20  @min 20  @max 85  @units "m/s"
					    @legacy_name TH2DDV3
					*/
	fint	diff_vel4;		/*# @name "Differential Velocity #4"
					    @desc "The 4th of six velocity difference threshold used as",
						  " criteria for building 2-D features. "
					    @default 25  @min 25  @max 90  @units "m/s"
					    @legacy_name TH2DDV4
					*/
	fint	diff_vel5;		/*# @name "Differential Velocity #5"
					    @desc "The 5th of six velocity difference threshold used as",
						  " criteria for building 2-D features. "
					    @default 30  @min 30  @max 95  @units "m/s"
					    @legacy_name TH2DDV5
					*/
	fint	diff_vel6;		/*# @name "Differential Velocity #6"
					    @desc "The 6th of six velocity difference threshold used as",
						  " criteria for building 2-D features. "
					    @default 35  @min 35  @max 100  @units "m/s"
					    @legacy_name TH2DDV1
					*/
	fint	min_vectors_2d;		/*# @name "Min # Vectors/2D Feature"
                                            @desc "Minimum number of pattern vectors required to declare",
						  "a 2-D feature."
					    @default 3  @min 1  @max 10
					    @legacy_name MIN1DP2D
					*/
	freal	vector_rad_dist_2d;	/*# @name "2D Vector Radial Distance"
					    @desc "Maximum radial distance between two pattern vectors",
						  " to be associated into the same 2-D feature."
					    @default 0.5  @min 0.0  @max 3.0  @units km
				 	    @legacy_name MAXPVRD  @precision 1
					*/
	freal	vector_azi_dist_2d;	/*# @name "2D Vector Azimuthal Distance"
					    @desc "Maximum azimuthal distance between two pattern vectors",
						  " to be associated into the same 2-D feature."
					    @default 1.5  @min 0.0  @max 4.0  @units degrees
					    @legacy_name MAXPVAD  @precision 1
					*/
	freal	max_ratio_2d;		/*# @name "2D Feature Aspect Ratio"
					    @desc "Maximum allowable aspect ratio (delta slant ",
						  "range/delta azimuth) for a 2-D feature."
					    @default 4.0   @min 1.0  @max 10.0  @units ratio
					    @legacy_name MAX2DAR  @precision 1
					*/
	freal	circ_radius1;		/*# @name "Circulation Radius 1"
					    @desc "The maximum horizontal radius used for searching for ",
						  "2-D features on adjacent or the same elevation scans ",
						  "in building a 3-D feature.  Used when the slant range ",
						  "of an assigned 2-D feature is less than or equal to ",
						  "Circulation Radius Range."
					    @default 2.5  @min 0.0  @max 10.0  @precision 1  @units km
					    @legacy_name THCR1
					*/
	freal	circ_radius2;		/*# @name "Circulation Radius 2"
					    @desc "The maximum horizontal radius used for searching for ",
						  " 2-D features on adjacent or the same elevation scans ",
						  "in building a 3-D feature.  Used when the slant range ",
						  "of an assigned 3-D feature is less than or equal to ",
						  "Circulation Radius Range."
					    @default 2.5  @min 0.0  @max 10.0  @units km
					    @legacy_name THCR2
					*/
	fint	circ_radius_range;	/*# @name "Circulation Radius Range"
					    @desc "Range(Slant) beyond which circulation radius 2 ",
						  "threshold is invoked. otherwise circulation radius ",
						  "1 threshold is used."
					    @default 80  @min 1  @max 230  @units km
					    @legacy_name THCRR
					*/
	fint	max_2d_features;	/*# @name "Max # 2D Features"
					    @desc "Maximum number of 2-D features the algorithm can ",
						  "process per volume scan."
					    @default 600   @min 600  @max 800
					    @legacy_name MAXNUM2D
					*/
	fint	min_2d_features;	/*# @name "Min # 2D Features/3D Feature"
					    @desc "Minimum number of 2-D features needed to make a 3-D ",
						  "feature."
					    @default 3  @min 1  @max 10  @legacy_name MIN2D3D
					*/
	freal	min_depth_3d;		/*# @name "Min 3D Feature Depth"
					    @desc "Minimum depth required to declare a TVS or ETVS."
					    @default 1.5  @min 0.0  @max 5.0  @units km  @precision 1
					    @legacy_name MINTVSD
					    @authority "READ | OSF_WRITE | URC_WRITE | AGENCY_WRITE"
					*/
	fint	min_vel_3d;		/*# @name "Min 3D Feat Low-Lvl Delta Vel"
					    @desc "Minimum radial velocity difference at the base ",
						  "elevation scan required to declare a TVS or ETVS."
                                            @default 25  @min 0  @max 100  @units "m/s"
					    @authority "READ | OSF_WRITE | URC_WRITE | AGENCY_WRITE"
					    @legacy_name MINLLDV
					*/
	fint	min_tvs_vel;		/*# @name "Min TVS Delta Velocity"
					    @desc "Minumum radial velocity difference of the maximum ",
						  "3-D feature delta velocity required to declare a TVS."
					    @default 36  @min 0  @max 100 @units "m/s"
					    @authority "READ | OSF_WRITE | URC_WRITE | AGENCY_WRITE"
					    @legacy_name MINMTDV
					*/
	fint	max_3d_features;	/*# @name "Max # 3D Features"
					    @desc "Maximum number of 3-D features the algorithm can ",
						  "process per volume scan."
					    @default 35  @min 30  @max 50
					    @legacy_name MAXNUM3D
					*/
	fint	max_tvs_features;	/*# @name "Max # TVS's"
					    @desc "Maximum number of TVS's the algorithm can process ",
						  "per volume scan."
					    @default 15  @min 15  @max 25
					    @legacy_name MAXNUMTV
					*/
	fint	max_etvs_features;	/*# @name "Max # Elevated TVS's"
					    @desc "Maximum number of elevated TVS's the algorithm can ",
						  "process per volume scan."
					    @authority "READ | OSF_WRITE | URC_WRITE"
					    @default 0  @min 0  @max 25
					    @legacy_name MAXNUMET
					*/
	freal	min_tvs_height;		/*# @name "Min TVS Base Height"
					    @desc "Minimum height AGL to which the base of a 3-D ",
						  "feature must extend to be declared a TVS."
					    @default 0.6  @min 0.0  @max 10.0  @units km  @precision 1
					    @legacy_name MINTVSBH
					*/
	freal	min_tvs_elev;		/*# @name "Min TVS Elevation"
					    @desc "Lowest elevation angle to which the base of a 3-D ",
						  "feature must extend to be declared a TVS."
					    @default 1.0  @min 0.0  @max 10.0  @units degrees
					    @legacy_name MINTVSBE  @precision 1
					*/
	freal	avg_vel_height;		/*# @name "Min Avg Delta Velocity Height"
					    @desc "Minimum height AGL below which all 2D circulations ",
						  "comprising a 3D feature are assigned an equal ",
						  "weighting of 1."
					    @default 3.0  @min 0.0  @max 10.0  @units km  @precision 1
					    @legacy_name MINADVHT
					*/
	freal	max_storm_dist;		/*# @name "Max Storm Association Dist."
					    @desc "Maximum distance from a storm within which to ",
						  "associate TVS and ETVS detections with storm cell ",
						  "detections.  Association is not required to declare ",
					    	  "a TVS or ETVS detection."
					    @default 20.0  @min 0.0  @max 20.0  @units km  @precision 1
					    @legacy_name MAXTSTMD
					*/
} tda_t;


#endif
