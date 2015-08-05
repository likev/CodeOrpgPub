/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:09:58 $
 * $Id: a317.h,v 1.3 2002/12/11 22:09:58 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/*
 * Module: a317.h
 * cpc017 Kinematic Algorithms definitions
 *
 *      The contents in this file are derived from a317.inc. The macros
 *      must be consistent with those defined there. Thus, if a317.inc is
 *      modified, this file has to be updated accordingly.
 *
 * Notes:
 *	MES2DATR - MESO/TVS 2D ATTRIBUTES
 *
 * References: Par. 3.5.1.1.18.1 w. of the C5-Specification
 */


#ifndef A317_H
#define A317_H

#include <orpgctype.h>

#define NEWVOL 1
#define ENDVOL 4

/*
 * Array size for Potential Mesocyclone, TVS, and TVS Features Tables.
 */
#define PMAX       50		/* maximum number of potential features */

/*
 * MES2DATR Header
 */
typedef struct {
   fint  vol_stat ;		/* Volume Status (e.g., begin/end vol)  */
   fint  vol_time ;		/* Volume Scan Time                     */
   fint  el_time ;		/* Elevation Scan Time                  */
   freal el;                    /* Elevation Angle                 (deg)*/
   fint  num_el_pmeso ;		/* # of Pot. Meso detected in elev scan */
   fint  num_vol_pmeso ;	/* # of Pot. Meso detected in vol scan  */
   fint  num_el_ptvs ;		/* # of Pot. TVS detected in elev scan  */
} mes2datr_header_t ;

/*
 * MES2DATR Potential Mesocyclone Attribute Data
 * NOTE: the legacy comments were confusing ... looked like they swapped
 *       the "rotational speed" and "max rotation speed" fields!
 */
typedef struct {
   fint pmnum[PMAX] ;		/* Feature Number Label                 */
   fint pmmesot[PMAX] ;		/* Feature Type (mesocyclone or shear)  */
   freal cntr_az[PMAX];		/* Feature Center Azimuth          (deg)*/
   freal cntr_rng[PMAX];	/* Feature Center Range             (km)*/
   freal az[PMAX];		/* Azimuth                         (deg)*/
   freal rng[PMAX];		/* Range                            (km)*/
   freal avg_shr[PMAX];		/* Average Shear                 (1/sec)*/
   freal max_ts[PMAX];		/* Maximum Tangential Shear (TS)(units?)*/
   freal max_spdr[PMAX];	/* Maximum Rotational Speed        (mps)*/
   freal max_ts_rng[PMAX];	/* Maximum TS Range                 (km)*/
   freal max_ts_begaz[PMAX];	/* Maximum TS Begin Azimuth        (deg)*/
   freal max_ts_endaz[PMAX];	/* Maximum TS End Azimuth          (deg)*/
   freal beg_rng[PMAX];		/* Expanded Search Begin Range      (km)*/
   freal end_rng[PMAX];		/* Expanded Search End Range        (km)*/
   freal begaz[PMAX];		/* Expanded Search Begin Azimuth   (deg)*/
   freal endaz[PMAX];		/* Expanded Search End Azimuth     (deg)*/
   freal spdr[PMAX];		/* Average Rotational Speed        (mps)*/
   fint ptvs_flag[PMAX];	/* Potential TVS Flag                   */
} mes2datr_pmeso_t ;

/*
 * MES2DATR Potential TVS Attribute Data
 * NOTE: untested as of 14APR97 ... need some TVS data ...
 */
typedef struct {
   freal az[PMAX];		/* Azimuth                         (deg)*/
   freal rng[PMAX];		/* Range                            (km)*/
   freal shear[PMAX] ;		/* Potential TVS Shear Value    (units?)*/
   freal orientation[PMAX];	/* Potential TVS Orientation    (units?)*/
   freal rotation[PMAX];	/* Potential TVS Rotation Dir   (units?)*/
} mes2datr_ptvs_t ;

/*
 * MES2DATR Potential Feature Attribute Data (TVS Processing)
 * NOTE: untested as of 14APR97 ... need some TVS data ...
 */
typedef struct {
   freal vmax[PMAX];		/* Velocity max for expanded srch  (mps)*/
   freal vmax_az[PMAX];		/* Azimuthal pos of exp srch vmax  (deg)*/
   freal vmax_rng[PMAX];	/* Radial pos of exp srch vmax      (km)*/
   freal vmin[PMAX];		/* Velocity min for expanded srch  (mps)*/
   freal vmin_az[PMAX];		/* Azimuthal pos of exp srch vmin  (deg)*/
   freal vmin_rng[PMAX];	/* Radial pos of exp srch vmin      (km)*/
   freal beg_rng[PMAX];		/* Expanded Search Begin Range      (km)*/
   freal end_rng[PMAX];		/* Expanded Search End Range        (km)*/
   freal begaz[PMAX];		/* Expanded Search Begin Azimuth   (deg)*/
   freal endaz[PMAX];		/* Expanded Search End Azimuth     (deg)*/
} mes2datr_feature_t ;


/*
 * MES2DATR
 */
typedef struct {
   mes2datr_header_t hdr ;
   mes2datr_pmeso_t pmeso ;
   mes2datr_ptvs_t ptvs ;
   mes2datr_feature_t feats ;
} mes2datr_t;

#endif /* DO NOT REMOVE! */
