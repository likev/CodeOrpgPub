/*
 * Module: trfrcatr.h
 *  Storm Tracking and Forecast Algorithm buffer definitions
 *
 *      The contents in this file are derived from A315BUF.INC. 
 *      The structures must be consistent with those defined there.
 *      If A315BUF.INC is modified this file must be updated accordingly.
 *
 * Notes:
 *
 *      TRFRCATR   - Storm Tracking and Forecast Attributes Buffer. (See
 *                   A315BUF.INC)
 * 
 *
 */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2003/07/03 20:47:50 $
 * $Id: trfrcatr.h,v 1.1 2003/07/03 20:47:50 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef TRFRCATR_H
#define TRFRCATR_H

/*
 * Array size for Storm Tracking and Forecast Tables.
 */
#define NSTF_MAX    100         /* maximum number of storm        */
#define NSTF_INT      4         /* number of forecast intervals   */
#define NSTF_MPV     13         /* number of past volumes tracked */
#define NSTF_ADP     57         /* elements in storm tracking adaptation data */

/*
 * TRFRCATR Header (See analogous fortran parameters in A315BUF.INC)
 */
typedef struct
{
   int   bnt;      /* Number of storms                     */
   int   bno;      /* Number of storms with motion determined*/
   float bvs;      /* Average speed of all storms          */
   float bvd;      /* Average direction of all storms      */
} tracking_hdr_t;


/*
 * TRFRCATR ID-Type Table
 */
typedef struct
{
   int   num_id;   /* Numerical ID                            */
   int   type;     /* Storm type (new or continuing)          */
   int   nvl;      /* Number of volumes cell has been tracked */
} tracking_bsi_t;

/*
 * TRFRCATR Storm Cell Motion Table
 */
typedef struct
{
   float x0;       /* x-coordinate location (km)              */
   float y0;       /* y-coordinate location (km)              */
   float xsp;      /* x-coordinate speed (m/s)                */
   float ysp;      /* y-coordinate speed (m/s)                */
   float spd;      /* polar coordinate speed (m/s)            */
   float dir;      /* polar coordinate direction (deg)        */
   float err;      /* forecast error (km)                     */
   float mfe;      /* mean forecast error (km)                */
} tracking_bsm_t;

/*
 * TRFRCATR Storm Cell Forward Table
 */
typedef struct
{
   float xf[NSTF_INT]; /* forecast x-coordinate location (km) */
   float yf[NSTF_INT]; /* forecast y-coordinate location (km) */
} tracking_bsf_t;

/*
 * TRFRCATR Storm Cell Backward Table
 */
typedef struct
{
   float xb[NSTF_MPV]; /* past volume x-coordinate location (km)*/
   float yb[NSTF_MPV]; /* past volume y-coordinate location (km)*/
} tracking_bsb_t;

/*
 * TRFRCATR Storm Tracking Adaptation data
 */
typedef union
{
   int adp_i[NSTF_ADP];   /* integer based adaptation data  */
   float adp_f[NSTF_ADP]; /* float based adaptation data    */
} tracking_bfa_t;


/*
 * TRFRCATR
 */
typedef struct
{
   tracking_hdr_t   hdr;            /* Header                  */
   tracking_bsi_t   bsi[NSTF_MAX];  /* ID_type table (numerical IDs, types & # volumes tracked)*/
   tracking_bsm_t   bsm[NSTF_MAX];  /* Storm Cell Motion Table (current positions & speed) */
   tracking_bsf_t   bsf[NSTF_MAX];  /* Storm Cell Foward Table (forecast positions)*/
   tracking_bsb_t   bsb[NSTF_MAX];  /* Storm Cell Backward Table (past positions) */
   tracking_bfa_t   bfa;            /* Adaptation data         */
} tracking_t;

#endif
