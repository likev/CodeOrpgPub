/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:35:16 $
 * $Id: mdattnn_params.h,v 1.14 2005/03/03 22:35:16 ryans Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

/************************************************************************
Module:         mdattnn_params.h

Description:    include file for the mdattnn process parameters.  
      
************************************************************************/

#ifndef MDATTNN_PARAMS_H 
#define MDATTNN_PARAMS_H

/* mda_sort flags */

#define SORT_BY_RANK_THEN_TYPE 0
#define SORT_BY_RANK_THEN_MSI  1

/* Misc. parameters */

#define NO             0
#define YES            1
#define MESO_MAX_DT   15.0     /* minutes (from ssaparm.dat)               */
#define MESO_MAX_SPD   2.0     /* kilometers per minute (from ssaparm.dat) */
#define MDA_DEF_U      0.0191  /* computed using logic in parminit.ftn     */
#define MDA_DEF_V      0.0256  /* computed using logic in parminit.ftn     */
#define RADIUS_INCREMENT 1.0   /* kilometers                               */
#define MAX_PAST        10     /* max number of past positions             */
#define MAX_FCST         6     /* max number of forecast positions         */
#define SCIT_TIMEOUT_SEC 4     /* seconds to wait for SCIT input data      */
#define TDA_TIMEOUT_SEC 10     /* seconds to wait for TDA input data       */
#define BAD_TIME        -1
#define FIRST          1
#define HALF           0.5
#define HALF_CIRC    180.0
#define CIRC         360.0
#define KILO        1000.0
#define SECPERMIN     60.0
#define SECPERHOUR  3600.0
#define SECPERDAY  86400
#define UNDEFINED    999.0
#define UNDEFINED_INT  -999999
#define UNDEFINED_SHORT -32768
#define NO_ORDER    9999
#define MESO_MAX_EVENT 999
#define MAX_TDA_DIST   2       /* kilometers. Max TVS association distance */
#define TVFEAT_MAX    50       /* from a317buf.inc,**A317TP9               */
#define THRESH_VERT_DIST 3     /* kilometers.  Vertical asssociation thresh*/
#define STM_ID_SIZE    3

/* mda_th_xs size */

#define MESO_NUM_ELEVS  22

typedef struct {        /* This data comes entirely from mda3d.  */
   int   tilt_num;      /* Refer to that process for units.      */
   int   height;
   int   diam;
   int   rot_vel;
   int   shear;
   int   gtgmax;
   int   rank;
   float ca;
   float cr;
} th_xs_t;

/* new_cplt and old_cplt sizes */

#define MAX_MDA_FEAT 500 /* maximum number of features processed by mdattnn */
#define MAX_MDA_PROD 100 /* max features passed to mdaprod from mdattnn     */

/* new_cplt and old_cplt structures (zero relative!)*/
typedef struct {
   int    meso_event_id;       /* Zero-relative, retained volume to volume    */
   char   storm_id[STM_ID_SIZE]; /* Associated storm ID                         */
   float  u_motion;            /* m/sec                                       */
   float  v_motion;            /* m/sec                                       */
   int    time_code;           /* Set when couplet is time-associated         */
#define     ASSOCIATED      1 
#define     NOT_ASSOCIATED  0
   float  age;                 /* Minutes (for time associated couplets only) */
   int    circ_class;          /* Circulation class (i.e. strength rank type) */
#define     SHALLOW_CIRC    1
#define     COUPLET_3D      2
#define     LOW_TOPPED_MESO 3
#define     WEAK_LOW_TOPPED_MESO  7

   int    num2D;               /* number of 2D features in this 3D couplet    */
   int    user_meso;           /* Was NSSL meso. See mda3d for criteria.      */
   int    tvs_near;
#define     TVS_YES         1
#define     TVS_NO          0
#define     TVS_UNKNOWN    -1   
   float  prop_spd;            
   float  prop_dir;
   float  ll_azm;              /* Low level 2d component's azimuth. (degrees) */
   float  ll_rng;              /* Low level 2d component's range. (kilometers)*/
   float  llx;                 /* Low level 2d component's X-position in      */
                               /*   kilometers from radar. (positive eastward)*/
   float  lly;                 /* Low level 2d component's Y-position in      */
                               /*   kilometers from radar.(positive northward)*/
   float  base;                /* Kilometers. 3D feature base.                */
   float  depth;               /* Kilometers. 3D feature depth.               */
   float  nssl_base;           /* Kilometers. NSSL criteria base.             */
   float  nssl_top;            /* Kilometers. NSSL criteria top.              */
   float  nssl_depth;          /* Kilometers. NSSL criteria depth.            */
   float  core_base;           /* Kilometers. Base of core circulation.       */
   float  fgx;                 /* Kilometers. First guess X-position.         */
   float  fgy;                 /* Kilometers. First guess Y-position.         */
   int    num_past_pos;        /* Number of positions in past_x and past_y.   */
   float  past_x[MAX_PAST];    /* Meters from radar. Past X-positions.        */
   float  past_y[MAX_PAST];    /* Meters from radar. Past Y-positions.        */
   th_xs_t mda_th_xs[MESO_NUM_ELEVS];/* Current time-height cross section.    */
   int    num_fcst_pos;        /* Number of positions in fcst_x and fcst_y.   */
   float  fcst_x[MAX_FCST];    /* Meters from radar. Forecast X-positions.    */
   float  fcst_y[MAX_FCST];    /* Meters from radar. Forecast Y-positions.    */
   int    strength_rank;       /* See mda3d.                                  */
#define      MESO_SR_THRESH 5  /* Threshold used for setting NSSL meso field. */
   float  ll_diam;
   float  max_diam;
   float  ll_rot_vel;
   float  max_rot_vel;
   float  ll_shear;
   float  max_shear;
   float  ll_gtg_vel_dif;
   float  max_gtg_vel_dif;
   float  height_max_diam;
   float  height_max_rot_vel;
   float  height_max_shear;
   float  height_max_gtg_vel_dif;
   float  core_top;
   float  core_depth;
   int    display_filter;
   float  msi;
   int    msi_rank;
   float  storm_rel_depth;
   float  ll_convergence;
   float  ml_convergence;
   float  vert_int_rot_vel;
   float  vert_int_shear;
   float  vert_int_gtg_vel_dif;
   float  delta_v_slope;
   float  trend_delta_v_slope;
   float  prob_of_tornado;
   float  prob_of_svr_wind;
   int    detection_status;    /* Detection Status                            */
#define     TOPPED          1  /* Last elevation cut passed over top of feature*/   
#define     EXTRAPOLATED    2  /* Feature extrapolated from previous volume   */
} cplt_t;

typedef struct {
   float  azimuth;
   float  range;
} polar_loc_t;

typedef struct {
   int     num_cplts;
   float   avg_u;
   float   avg_v;
   int     elev_time[MESO_NUM_ELEVS];
} mda_ttnn_hdr_t;

#endif
