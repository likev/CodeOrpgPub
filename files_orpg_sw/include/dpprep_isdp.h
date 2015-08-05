/*==================================================================
 *
 * Module Name: dpprep_isdp.h 
 *
 * Module Version: 1.0
 *
 * Module Language: c
 *
 * Change History:
 *
 * Date    Version    Programmer  Notes
 * ----------------------------------------------------------------------
 * 04/29/14  1.0    Brian Klein   Creation
 * 
 *
 * Description:
 *  This header file supports estimation of intial system differential
 *  phase (ISDP) in the RPG;
 *
 *=============================================================================*/

/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/06/23 20:24:05 $ */
/* $Id: dpprep_isdp.h,v 1.1 2014/06/23 20:24:05 steves Exp $ */
/* $Revision:  */
/* $State: */

#ifndef DPPREP_ISDP_H
#define DPPREP_ISDP_H

#define CALC_SYSTEM_PHIDP_INTERVAL 1  /* Timer interval (in hours)                */
#define MAX_TOO_CLOSE             90  /* Max allowable radials per elevation with */
                                      /* weather too close to the radar.          */
#define ISDP_WARNING_THRESH       25  /* Degrees difference in estimate and target*/
                                      /* ISDP for warning message in status log   */

typedef struct {	

  int    isdp_est;    /* estimated ISDP (deg)                           */
  int    isdp_yy;     /* 2-digit year of ISDP estimate                  */
  int    isdp_mm;     /* month of ISDP estimate                         */
  int    isdp_dd;     /* day of ISDP estimate                           */
  int    isdp_hr;     /* hour of ISDP estimate                          */
  int    isdp_min;    /* minute of ISDP estimate                        */
  int    isdp_sec;    /* second of ISDP estimate                        */

} Dpp_isdp_est_t;

#endif
