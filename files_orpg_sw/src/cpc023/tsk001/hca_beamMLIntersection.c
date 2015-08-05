/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 23:03:38 $
 * $Id: hca_beamMLIntersection.c,v 1.6 2009/10/27 23:03:38 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         Hca_beamMLintersection                                *
 *      Author:         Yukuan Song, Brian Klein                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    Calculate the range of start of ML contamination,     *
 *                      range of beam-center in ML,                           *
 *                      range of beam-center at top of ML,                    *
 *                      range of end of ML contamination                      *
 *                                                                            *
 *      Change History:                                                       *
 ******************************************************************************/

#include <math.h>
#include <rpgc.h>
#include "hca_local.h"

/****************************************************************************
>    Description:
>       Proximity of Radar Beam to Melting Layer
>    Input:
>       elevation angle, azimuth angle (as truncated integer), bin size
>    Output:
>       bin numbers
>    Returns:
>    Globals:
>    Notes:
>  ***************************************************************************/
void Hca_beamMLintersection( float elev, int azimuth, float bin_size, float ML_top[MAXRADIALS],
                             float ML_bottom[MAXRADIALS], ML_bin_t *Melting_layer)
{
/* calculate the following ranges:            *
 * r_bb -- range of start of ML contamination *
 * r_b -- range of beam-center in ML          * 
 * r_t -- range of beam-center at top of ML   *
 * r_tt -- range of end of ML contamination   */

 ML_r_t   ML_r;

const float beam_width = 1.0;
const float DEGtoRAD = 0.0174533;
const float half_beam_width = (beam_width/2.0) * DEGtoRAD;
/*const float Ae = 8498.67;*/ /* effective Earth Radius */
const float Ae = 7708.91; /* Effective Earth radius (per RPG requirements) */


/* Convert degrees to radians */
  elev *= DEGtoRAD;

/* Compute slant range */
/* CPT&E label A */
  ML_r.r_bb = sqrt(2.0 * ML_bottom[azimuth] * Ae + Ae * Ae * sin(elev + half_beam_width) *
                   sin(elev + half_beam_width)) -
              Ae * sin(elev + half_beam_width);

  ML_r.r_b  = sqrt(2.0 * ML_bottom[azimuth] * Ae + Ae * Ae * sin(elev) * sin(elev)) -
              Ae * sin(elev);

  ML_r.r_t  = sqrt(2.0 * ML_top[azimuth] * Ae + Ae * Ae * sin(elev) * sin(elev)) -
              Ae * sin(elev);

  ML_r.r_tt = sqrt(2.0 * ML_top[azimuth] * Ae + Ae * Ae * sin(elev - half_beam_width) *
                   sin(elev - half_beam_width)) -
              Ae * sin(elev - half_beam_width);

/* Convert slant ranges to bin numbers */
/* CPT&E label B */
  Melting_layer->bin_bb = (int)RPGC_NINT(ML_r.r_bb / bin_size);
  Melting_layer->bin_b  = (int)RPGC_NINT(ML_r.r_b  / bin_size);
  Melting_layer->bin_t  = (int)RPGC_NINT(ML_r.r_t  / bin_size);
  Melting_layer->bin_tt = (int)RPGC_NINT(ML_r.r_tt / bin_size);

  return;
}

