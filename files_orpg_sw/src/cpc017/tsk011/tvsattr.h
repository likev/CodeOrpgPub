/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/11 19:18:27 $
 * $Id: tvsattr.h,v 1.2 2003/07/11 19:18:27 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************
Module:         tvsattr.h

Description:    Header file designed after a317buf.inc, **A317TP9
                Note that any change to that include block must be
                made here also.
      
****************************************************************************/

#ifndef tvsattr_H 
#define tvsattr_H

/* TVSATTR buffer created by the TDA2D3D process.                            */

#define   TVFEAT_MAX   50 /* Maximum number of features. (TVS_T + ETVS_T)    */
#define   TVS_T         1 /* Tornadic Vortex Signature type.                 */
#define   ETVS_T        2 /* Elevated Tornadic Vortex Signature type.        */

typedef struct {
   float  type;          /* TVS_T or ETVS_T (tvs or elevated tvs).           */
   float  azimuth;       /* Degrees.                                         */
   float  range;         /* Kilometers.                                      */
   float  ll_dv;         /* Low level delta velocity, meters/second.         */
   float  avg_dv;        /* Average delta velocity, meters/second.           */
   float  max_dv;        /* Maximum delta velocity, meters/second.           */
   float  hgt_max_dv;    /* Height of the max delta velocity,kilometers.     */
   float  depth;         /* Kilometers (negative means base or               */
                         /*    top is on lowest or highest elevation).       */
   float  hgt_base;      /* Height of the base, kilometers ARL.              */
   float  elev_base;     /* Elevation of the base, degrees.                  */
   float  hgt_top;       /* Height of the top, kilometers ARL.               */
   float  max_shear;     /* Maximum shear, meters/second/kilometer.          */
   float  hgt_max_shear; /* Height of the maximum shear, kilometers ARL.     */
} tvs_t;

typedef struct {
   int    v_time;        /* Volume scan time, milliseconds since midnight.   */
   int    v_date;        /* Volume scan date, NEXRAD Julian.                 */
   int    num_tvs;       /* Number of TVS_T (if < 0 TVFEAT_MAX was exceeded. */
   int    num_etvs;      /* Number of ETVS_T (if < 0 TVFEAT_MAX was exceeded.*/
   tvs_t  tvsattr[TVFEAT_MAX]; /* See tvs_t structure.                       */
} tvsattr_t;

#endif
