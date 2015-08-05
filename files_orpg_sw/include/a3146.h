/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 16:28:29 $
 * $Id: a3146.h,v 1.4 2008/01/04 16:28:29 aamirn Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/***********************************************************************

	This file defines the macros that are needed by the ported RPG
	tasks.

	The contents in this file are derived from a3146.inc. The macros 
	must be consistent with those defined there. Thus if a3146.inc is 
	modified, this file has to be updated accordingly.

***********************************************************************/


#ifndef A3146_H
#define A3146_H

/* System include files */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <itc.h>

/*
 * Polar Grid Parameters
 */
#define KRADS	360
#define KBINS	115

/***a3146lfp ----------------------------------------------------------*/
/*version:1*/

/* LFM flag value parameters... */
#define  BEYOND_RANGE -99
#define  WITHIN_RANGE   0
#define  HOLE_FLAG    -88
#define  BEYOND_GRID  -77

/***a3146lfx ----------------------------------------------------------*/
/*version: 0*/

/* Polar to lfm conversion constants */
#define  lfm4_idx   0
#define  lfm16_idx  1
#define  lfm40_idx  2
#define  LFMMX_IDX  3
#define  ip           433.0
#define  jp           433.0
#define  angle_thresh 9.81e-6
#define  const_val    135.0
#define  LFM4_RNG     230.0
#define  LFM16_RNG    460.0
#define  LFM40_RNG    230.0
#define  r2ko         249.6348607
#define  prime        105.0
#define  re_proj      6380.0
#define  re_proj_sq   (re_proj*re_proj) /*6380.0*6380.0*/

/***a314c1 ----------------------------*/
/*version:0*/

/* Polar to lfm grid mapping common area and max rate value for prod #82 */
/* Note: a314c1_t struct is defined in itc.h file */
/* Declare global object struct */

a314c1_t a314c1;

#endif 		/* #ifndef A3146_H */
