/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/09/30 22:17:53 $
 * $Id: mdattnn_data.h,v 1.5 2011/09/30 22:17:53 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/************************************************************************
Module:         mdattnn_data.h

Description:    header file for the mdattnn process global data.  
      
************************************************************************/

#ifndef MDATTNN_DATA_H 
#define MDATTNN_DATA_H

#include "mda_ru.h"

int               Nbr_old_cplts, Nbr_cplt_trks;
int               Old_time, Old_date;
cplt_t            Old_cplt[MAX_MDA_FEAT];
cplt_t            new_cplt[MAX_MDA_FEAT]; /* 3D features this volume           */
cplt_t            new_cplt_non_zero[MAX_MDA_FEAT]; /* 3D features without 0 SR */
tracking_t        scit_tracks;

int nbr_prev_newcplt; /* the num of the new cplts found at previous elevation*/
Prev_newcplt prev_newcplt[MAX_MDA_FEAT]; /* the new cplt array at previous elevation */

int nbr_first_elev_newcplt; /* num of the new cplt at the first elevation */
cplt_t first_elev_newcplt[MAX_MDA_FEAT]; /* the cplt array at the first elev */

/* The following are the global variables for TREND */
/**trendmeso_t old_trendmeso[MESO_MAX_FEAT][MAX_TREND_TIME];**/
/**th_xs_t old_th[MESO_MAX_FEAT][MAX_TREND_TIME][MESO_NUM_ELEVS];**/
/**int old_trend_time[MESO_MAX_FEAT];**/
/**int old_elev_levels[MESO_MAX_FEAT][MAX_TREND_TIME];**/

#endif
