/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2004/10/15 14:09:37 $
 * $Id: tvs.h,v 1.3 2004/10/15 14:09:37 ryans Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	This header file defines the structure for the TVS		*
 *	algorithm.  It corresponds to common block CP17ALG in the	*
 *	legacy code.							*/


#ifndef TVS_H
#define	TVS_H

#include <orpgctype.h>

/*	Tornado Vortex Signature (TVS) Data		*/

typedef struct {

	freal	min_shear_value;	/*  Minimum shear value for	*
					 *  TVS (1/s).			*
					 *  Range: 18.0 to 1800.0	*/
	freal	exp_search_area;	/*  Expanded search area for	*
					 *  TVS (%).			*
					 *  Range: 0.0 to 50.0		*/

} tvs_t;

#ifdef __cplusplus
#endif

#endif
