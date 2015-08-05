/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2010/10/18 15:47:51 $
 * $Id: vad.h,v 1.10 2010/10/18 15:47:51 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/*	This header file defines the structure for the VAD		*
 *	algorithm.  It corresponds to common block VAD in the	*
 *	legacy code.							*/


#ifndef VAD_H
#define	VAD_H

#include <orpgctype.h>

#define VAD_DEA_NAME "alg.vad"


/*	VAD Data	*/

typedef struct {

   float thresh_velocity;	

   int num_fit_tests;	

   int min_samples;

   float anal_range;

   float start_azimuth;

   float end_azimuth;

   float symmetry;

/* Enhanced VWP adaptable parameters. */

   int   enhanced_vad;

   int   min_points;

   float min_symmetry;

   float scale_rms;

   float min_proc_range;

   float max_proc_range;

} vad_t;


#endif
