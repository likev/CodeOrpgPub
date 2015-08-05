/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:37 $
 * $Id: hail_detection.h,v 1.12 2007/01/30 22:56:37 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/*	This header file defines the structure for the hail detection	*
 *	algorithm.  It corresponds to common block HAIL_ALGORITHM 	*
 *	in the legacy code.						*/


#ifndef HAIL_DETECTION_H
#define	HAIL_DETECTION_H

#include <orpgctype.h>

#define HAIL_DEA_NAME "alg.hail"


/*
	Declarations in this file are duplicated with the definitions in
	hail_algorithm.h.  This include file should be phased out for OB2.
	New algorithms should add new fields to the hail_algorithm.h include
	data structures.  Until this file is phased out.  New fields will be
	added to the data structures below when new algorithms are integrated
*/

typedef struct {

	fint	hke_ref_wgt_low;	/* REF_W_LL */
					/*  Hailfall Kinetic Energy	*
					 *  reflect lower limit	(dBZ).	*
					 *  Range: 20 to 60		*/
	fint	hke_ref_wgt_high;	/* REF_W_UL */
					/*  Hailfall Kinetic Energy	*
					 *  reflect upper limit (dBZ).	*
					 *  Range: 30 to 70		*/
	fint	poh_min_ref;		/* MNPOHREF */
					/*  Probability of Hail minimum	*
					 *  reflectivity (dBZ).		*
					 *  Range: 30 to 60		*/
	freal	hke_coef1;		/* HKECOEF1 */
					/*  Hailfall Kinetic Energy	*
					 *  coefficient 1.		*
					 *  Range: 0.0000000001 to 1.0	*/
	freal	hke_coef2;		/* HKECOEF2 */
					/*  Hailfall Kinetic Energy	*
					 *  coefficient 2.		*
					 *  Range: 0.005 to 0.5		*/
	freal	hke_coef3;		/* HKECOEF3 */
					/*  Hailfall Kinetic Energy	*
					 *  coefficient 3.		*
					 *  Range: 1.0 to 100.0		*/
	freal	posh_coef;		/* POSHCOEF */
					/*  Probability of Severe Hail	*
					 *  coefficient.		*
					 *  Range: 1.0 to 100.0		*/
	fint	posh_offset;		/* POSHOFST */
					/*  Probability of Severe Hail	*
					 *  offset (%).			*
					 *  Range: 1 to 100		*/
	fint	max_hail_range;		/* MXHALRNG */
					/*  Maximim hail processing	*
					 *  range (km).			*
					 *  Range: 200 to 460		*/
	freal	shi_hail_size_coef;	/* HAILSZCF */
					/*  Severe Hail Index hail size	*
					 *  coefficient.		*
					 *  Range: 0.01 to 1.0		*/
	freal	shi_hail_size_exp;	/* HAILSZEX */
					/*  Severe Hail Index hail size	*
					 *  exponent.			*
					 *  Range: 0.1 to 1.0		*/
	freal	warn_thr_sel_mod_coef;	/* WT_SM_CF */
					/*  Warning threshold select	*
					 *  model coeff (100 J/m**2/s)	*
					 *  Range: 0.0 to 500.0		*/
	freal	warn_thr_sel_mod_off;	/* WT_SM_OF */
					/*  Warning threshold select	*
					 *  model offset (10**5 J/m/s)	*
					 *  Range: -500.0 to 500.0	*/
	freal	poh_height_diff1;	/* POHTDIF1 */
					/*  Probability of Hail height	*
					 *  difference #1 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff2;	/* POHTDIF2 */
					/*  Probability of Hail height	*
					 *  difference #2 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff3;	/* POHTDIF3 */
					/*  Probability of Hail height	*
					 *  difference #3 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff4;	/* POHTDIF4 */
					/*  Probability of Hail height	*
					 *  difference #4 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff5;	/* POHTDIF5 */
					/*  Probability of Hail height	*
					 *  difference #5 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff6;	/* POHTDIF6 */
					/*  Probability of Hail height	*
					 *  difference #6 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff7;	/* POHTDIF7 */
					/*  Probability of Hail height	*
					 *  difference #7 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff8;	/* POHTDIF8 */
					/*  Probability of Hail height	*
					 *  difference #8 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff9;	/* POHTDIF9 */
					/*  Probability of Hail height	*
					 *  difference #9 (km).		*
					 *  Range: 0.0 to 20.0		*/
	freal	poh_height_diff10;	/* POHTDIF0 */
					/*  Probability of Hail height	*
					 *  difference #10 (km).	*
					 *  Range: 0.0 to 20.0		*/
	fint	rcm_probable_hail;	/* RCMPRBHL */
					/*  Radar Coded Message probable*
					 *  hail (%).			*
					 *  Range: 0 to 100		*/
	fint	rcm_positive_hail;	/* RCMPOSHL */
					/*  Radar Coded Message positive*
					 *  hail (%).			*
					 *  Range: 0 to 100		*/

} hail_detect_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/
#ifdef __cplusplus
#endif

typedef	struct {

	freal	height_0;		/* HT0 */
					/*  Height of 0C level (kft)	*
					 *  Range: 0.0 to 70.0		*/
	freal	height_minus_20;	/* HT20 */
					/*  Height of -20C level (kft).	*
					 *  Range: 0.0 to 70.0		*/
	fint	hail_date [3];		/* HADATE */
					/*  Date environmental data	*
					 *  updated. (YY,MM, DD)	*/
	fint	hail_time [3];		/* HATIME */
					/*  Time environmental data	*
					 *  updated. (HH, MM, SS)	*/

} hail_environ_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/
#ifdef __cplusplus
#endif

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/
typedef	struct {

	hail_detect_t	detect;
	hail_environ_t	environ;

} hail_t;

#ifdef __cplusplus
#endif

#endif
