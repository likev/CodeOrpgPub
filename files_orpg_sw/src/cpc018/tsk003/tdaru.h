/*
* RCS info
* $Author: ccalvert $
* $Date: 2010/05/24 20:42:01 $
* $Locker:  $
* $Id: tdaru.h,v 1.8 2010/05/24 20:42:01 ccalvert Exp $
* $Revision: 1.8 $
* $State: Exp $
* $Log: tdaru.h,v $
* Revision 1.8  2010/05/24 20:42:01  ccalvert
* TDA SCIT 20km
*
* Revision 1.7  2007/05/24 16:44:56  ryans
* Remove macro references
*
* Revision 1.6  2005/04/11 19:02:22  steves
* issue 2-534
*
* Revision 1.5  2005-04-11 12:54:51-05  steves
* issue 2-591
*
* Revision 1.4  2004/02/10 22:14:11  ccalvert
* code cleanup
*
* Revision 1.3  2004/02/05 23:08:16  ccalvert
* new dea format
*
* Revision 1.2  2004/01/27 01:14:56  ccalvert
* make site adapt dea format
*
* Revision 1.1  2003/07/03 13:41:50  ccalvert
* Initial revision
*
* Revision 1.2  2003/06/18 19:13:16  nshen
* ready version
*
* Revision 1.1  2003/05/14 13:51:09  nshen
* Initial revision
*
* Revision 1.1  2003/05/02 19:36:49  nings
* Initial revision
*
*
*/

/*****************************************************************
**
**	File:		tdaru.h
**	Author:		Ning Shen
**	Date:		March 3, 2003
**	Version:	1.0
**
**	Description:
**	============
**
**
**	Chage History:
**	==============
**
*******************************************************************/


#ifndef TDARU_H
#define TDARU_H

#include <alg_adapt.h>
#include <a315buf.h>

#define OK     0
#define ERROR -1
#define TRUE   1
#define FALSE  0
#define ABORT -1
#define CONTINUE 0

#define CARET 0X5E        /* indicate the attribute is current */
#define SPACE 0X20
#define GREATER 0X3E
#define LESS  0X3C
#define OUTBUF_SIZE 27000  /* based on max 100 features
                           ** meg hdr and PDB   120 bytes
                           ** SYM block        2416 bytes 
                           ** GAB              9836 bytes
                           ** TAB             14092 bytes
                           */

#ifndef PI
#define PI 3.141592
#endif
#define SECONDS_PER_MINUTE 60
#define SEC_PER_DAY 86400
#define METERS_PER_KM 1000
#define DEG_TO_RAD    (PI/180.0)
#define KNOT_TO_M_PER_SEC (1852.0/3600.0) 
#define M_PER_SEC_TO_KNOT (1/KNOT_TO_M_PER_SEC) 
#define NM_PER_KM (1.0/1.852)
#define FT_PER_M (100.0/30.48)
#define KFT_PER_KM FT_PER_M 


#define TVSATR_RU   290    /* input buffer ID from TDA2D3DRU */
#define TRFRCATR    50     /* input buffer ID from SCIT */
#define MDATTNN     295    /* input buffer ID from MDA Track */
#define TRU         143    /* final output buffer ID */
#define ID_LEN      3      /* storm ID length */

  /* size for TVS structure */
#define TVFEAT_MAX  50   /* max number of TVSs + ETVSs */
#define TVFEAT_CHR  13   /* number of attrs. stored per TVS feature */
#define NTDA_ADP    30   /* number of adaptation elements in TDA */

  /* feature status */
#define NEW   20    /* new feature */
#define INC   21    /* increasing */
#define PER   22    /* persistent */
#define EXT   23    /* extrapolated */
#define EMPTY 24    /* no feature */

  /* feature type */
#define TVS  1
#define ETVS 2

  /* product block offsets in bytes */
#define MSG_HDR_OFFSET   0     /* msg header & PDB entry offset */
#define SYM_BLK_OFFSET   120   /* sym blk entry offset */
#define SYM_PKT_OFFSET 136  /* position of the first layer in sym */
 
  /* constants for Symbology Block */


  /* constants for GAB */
#define NUM_HORIZ_VEC  6      /* number of horizontal vectors/lines(packet 10) */
#define NUM_VERT_VEC   8      /* number of vertical vectors/lines(packet 10) */
#define FEATS_PER_PAGE 6      /* max number of features per page */
#define FIELD_WIDTH    70     /* I direction data field width in pixels */
#define FIELD_HEIGHT   10     /* J direction data field height in pixels */
#define YELLOW         3      /* color value for grid */ 

#define I_LENGTH (7 * FIELD_WIDTH)  /* total length in I direction */
#define J_LENGTH (5 * FIELD_HEIGHT) /* total length in J direction */

  /* constants for TAB */
#define LINES_PER_PAGE 17     /* max lines per TAB page */
#define LINE_WIDTH     80     /* Fixed line length for TAB page */
#define NUM_HDR_LINES  6      /* include title and empty lines */
#define NUM_FEAT_LINES 11     /* total feature lines per page */


  /* structures for packet 8 10 20 */
typedef struct
{
  short pkt_code;    /* packet code */
  short num_bytes;   /* block length not including self and pkt_code */
  short color_val;   /* color value of text/special symbol */
  short pos_i;       /* I starting coordinate (x direction) */
  short pos_j;       /* J starting coordinate (y direction) */
} pkt_8_t;

typedef struct
{
  short pkt_code;
  short num_bytes;   /* block length not including self and pkt_code */
  short color;       /* color value for vectors */
} pkt_10_hdr_t;

typedef struct
{
  short begin_i;
  short begin_j;
  short end_i;
  short end_j;
} pkt_10_data_t;

#define TVS_EXT  5 
#define ETVS_EXT 6 
#define TVS_UPDATED  7 
#define ETVS_UPDATED 8 

typedef struct
{
  short pkt_code;     /* pkt_code = 20 */
  short num_bytes;    /* bytes length not including self and pkt_code */
} pkt_20_hdr_t;

typedef struct
{
  short pos_i;        /* I starting position */
  short pos_j;        /* J starting position */
  short type;         /* TVS_INC, ... ETVS_EXT, see above #defines */
  short attr;
} pkt_20_point_t;

typedef struct
{
  pkt_20_hdr_t hdr;
  pkt_20_point_t point;
} pkt_20_t;

typedef struct 
{
  short pkt_code;      /* pkt_code = 15 */
  short num_bytes;     /* pkt length in bytes not including self & pkt_code */
} pkt_15_hdr_t;

typedef struct
{
  short pos_i;
  short pos_j;
  char  letter;    /* first letter in storm ID (A - Z) */
  char  number;    /* second letter in storm ID (0 - 9) */
} pkt_15_data_t;

typedef struct 
{
  pkt_15_hdr_t hdr;
  pkt_15_data_t data;
} pkt_15_t;

  /* structure for Symbology Block */
typedef struct
{
  short blk_divider;     /* -1 */
  short blk_id;          /* 1 */
  int   blk_length;      /* byte length from blk_divider to the end of blk */
  short num_layers;      /* number of data layers (1 for this product) */
  short lyr_divider;     /* -1 */
  int   lyr_length;      /* length not including self and lyr_divider */
} sym_hdr_t;

  /* structure for GAB */
typedef struct
{
  short blk_divider;  /* block divider -1 */
  short blk_id;       /* block id, always 2 */
  int   blk_length;   /* length in bytes including self, blk_id, and blk_divider */
  short num_pages;    /* total pages in GAB */
} GAB_hdr_t;

typedef struct
{
  short page_num;     /* current page number */
  short page_length;  /* total bytes of all the packets in the page */
} GAB_page_hdr_t;

  /* structure for TAB */
typedef struct
{
  short blk_divider;  /* block divider -1 */
  short blk_id;       /* block id is always 3 */
  int   blk_length;   /* from blk_divider to the end of the message in bytes */
} TAB_hdr1_t;

typedef struct
{
  short blk_divider;  /* -1 */
  short num_pages;    /* total number of pages in TAB */
} TAB_hdr2_t;

typedef struct
{
  short num_chars;    /* number of chars in a line */
  char data[80];      /* char data */
} TAB_line_t;

  /****** TVSATTR header (see fortran parameters in a317buf.inc ********/
typedef struct
{
  int vol_time;  /* time at the beginning of a volume (msecs since midnight) */
  int vol_date;  /* date at the beginning of a volume (Julian day) */
  int num_tvs;   /* Number of TVSs so far in current volume scan */
  int num_etvs;  /* Number of ETVSs so far in current volume scan */
} tvs_hdr_t;

  /********** TVSATTR TVS attributes structure (see a317buf.inc) ********/
typedef struct 
{
  float feature_type;      /* TVS (1) or ETVS (2) */
  float base_az;           /* feature base azimuth (deg) */
  float base_rng;          /* feature base slant range (km) */
  float low_level_DV;      /* low-level Delta Velocity (m/s) */
  float average_DV;        /* average Delta Velocity (m/s) */
  float max_DV;            /* maximum Delta Velocity (m/s) */
  float max_DV_height;     /* ARL (km) */
  float depth;             /* depth of a feature (km). Neg. value means Base or
                            * Top is on lowest or highest elevation, respectively 
                            */
  float base_height;       /* ARL (km). Neg. value means Base is on lowest elev */
  float base_elev;         /* base elevation (deg) */
  float top_height;        /* ARL (km). Neg value means Top is on highest elev */
  float max_shear;         /* 0.001/s */
  float max_shear_height;  /*ARL (km) */
} tvs_attr_t;

  /******** TVSATTR TDA adaptation data structure (see a317buf.inc) ***********/
typedef struct
{
  float DV1;               /* differential velocity #1 (m/s) */ 
  float DV2;               /* differential velocity #2 (m/s) */ 
  float DV3;               /* differential velocity #3 (m/s) */ 
  float DV4;               /* differential velocity #4 (m/s) */ 
  float DV5;               /* differential velocity #5 (m/s) */ 
  float DV6;               /* differential velocity #6 (m/s) */ 
  float vector_vel_diff;   /* vector velocity difference (m/s) */
  float feat2d_ratio;      /* 2D feature aspect ratio (km/km) */
  float max_vector_rng;    /* max pattern vector range (km) */ 
  float circ_radius1;      /* circulation radius #1 (km) */
  float circ_radius2;      /* circulation radius #2 (km) */
  float circ_radius_rng;   /* circulation radius range (km) */
  float min_tvs_base_hgt;  /* min base TVS base height (km) */
  float min_tvs_base_elev; /* min TVS base elevation (deg) */
  float max_vector_height; /* max pattern vector height (km) */
  float min_num_vectors;   /* min # vectors per 2D feature */
  float min_depth;         /* min TVS/ETVS depth (km) */
  float min_3d_DV;         /* min 3D feature low-level delta velocity (m/s) */
  float min_tvs_DV;        /* min TVS delta velocity (m/s) */
  float vec_rad_dist_2d;   /* 2D vector radial distance (km) */
  float vec_az_dist_2d;    /* 2D vector azimuth distance (deg) */
  float min_refl;          /* min reflectivity (dBZ) */
  float max_SAD;           /* max Storm Association Distance (km) */
  float min_num_2dfeat;    /* min # 2D features per 3D feature */
  float max_num_2dfeat;    /* max # 2D features */
  float max_num_3dfeat;    /* max # 3D features */
  float max_num_vectors;   /* max # pattern vectors */
  float max_num_etvs;      /* max # ETVSs */
  float max_num_tvs;       /* max # TVSs */
  float average_DV_height; /* average Delta Velocity height (km) */ 
} tda_adap_t;

  /*********** TVSATTR input buffer structure (see a317buf.inc) ***********/
typedef struct
{
  tvs_hdr_t hdr;
  tvs_attr_t features[TVFEAT_MAX];
  tda_adap_t tda_adapt; 
} tvs_feat_t;

  /************* TRU output data structure ***************/
typedef struct
{
  int vol_time;           /* volume beginning time(msecs since midnight) */
  int vol_date;           /* date at the beginning of volume(Julian day) */
  float current_elev;     /* current elevaiton */
  int num_tvs;            /* number of TVSs */
  int num_etvs;           /* number of ETVSs */
  int num_features;       /* total number of features in a product */ 
} tdaru_hdr_t;

  /********** TVS attributes update flags structure  ********/
typedef struct 
{
  char type_flg;      
  char az_flg;           
  char rng_flg;       
  char ll_DV_flg;      
  char avg_DV_flg;        
  char max_DV_flg;         
  char max_DV_hgt_flg;
  char depth_flg;    
  char base_hgt_flg; 
  char base_elev_flg; 
  char top_hgt_flg;  
  char max_shear_flg;   
  char max_shear_hgt_flg; 
} attr_flg_t;

typedef struct
{
  char storm_id[ID_LEN];  /* storm ID from SCIT */
  tvs_attr_t feature;     /* feature attributes */
  attr_flg_t attr_flag;   /* indicate the attribute is updated to current */
  int feat_status;        /* new, increasing, persistent, or extrapolated */
  int matched_flag;       /* TRUE or FALSE */ 
} tdaru_feat_t;

typedef struct
{
  tdaru_hdr_t hdr;
  tdaru_feat_t tda_feat[TVFEAT_MAX];
} tdaru_prod_t;

  /*************** MDA track structure from  *******************/

typedef struct
{
  int dummy;    /* the info TDARU does not care */
  float x_spd;  /* average motion of meso in m/s at east (x) direction */
  float y_spd;  /* average motion of meso in m/s at north (y) direction */
} mda_track_t;


/********** TRFRCATR structure copied from a318buf.h header file *****/

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
} scit_track_t;

/******************* end of TRFRCATR structure block ********************/

#endif 

