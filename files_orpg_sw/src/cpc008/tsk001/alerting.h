/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/05/02 14:17:47 $
 * $Id: alerting.h,v 1.6 2011/05/02 14:17:47 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef ALERTING_H
#define ALERTING_H

#include <rpgc.h>
#include <rpgcs.h>
#include <hydromet_adj.h>
#include <prodsel.h>
#include <alg_adapt.h>
#include <a307buf.h>
#include <a308buf.h>
#include <a313hparm.h>
#include <a313buf.h>
#include <legacy_prod.h>
#include <prod_user_msg.h>
#include <product_parameters.h>
#include <alert_threshold.h>

#define DTR			((float) DEG_TO_RAD)

/* Macro definitions. */
#define NUM_TYPS		7
#define NACT_SCNS		2
#define MAX_CLASS1		41
#define NWDS_MAX_CLASS1		2
#define NUM_ALERT_AREAS 	2
#define MAX_ALERT_CATS 		10
#define ALTAELEM		10
#define ALCATMAX		41
#define HW_PER_CAT		3
#define NUM_ALERT_ROWS		58
#define HW_PER_ROW		4
#define ALRT_GRID_DIM           (MAX_ALERT_CATS*HW_PER_CAT + NUM_ALERT_ROWS*HW_PER_ROW)

#define	NO_ALERT		0
#define	NEW_ALERT		1
#define	OLD_ALERT		2
#define	END_ALERT		3

#define UNDEFINED_CAT_TYPE	-1
#define GRID_CAT_TYPE		0
#define VOLUME_CAT_TYPE		1
#define FORECAST_CAT_TYPE	2

/********************* A308P1 ****************************
*                                                       *
*  THIS ELEMENT CONTAINS THE PARAMETER OFFSET INTO      *
*  THE MEM BUFFERS THAT CONTAIN THE COPIES OF THE       *
*  USER ALERT GRIDS AND AREA DEFINITIONS WHICH ARE      *
*  USED BY THE ALERTING TASK.                           *
*                                                       *
*********************************************************
C
C  GDOFF IS THE OFFSET TO THE ALERT GRID .
C  ACOFF IS THE OFFSET TO THE ACTIVE CATEGORIES TABLE
C  THOFF IS THE OFFSET TO THE THRESHOLDS OF THE CATEGORIES
C  PRQOFF IS THE OFFSET TO THE PRODUCT REQUEST FLAG PER CATEGORY
C  CSTATOFF IS THE OFF SET TO THE CURRENT STATUS DIRECTORY. */
#define GDOFF			0
#define ACOFF			116
#define THOFF			121
#define PRQOFF			126
#define CSTATOFF		131

#define TLINES			20
#define NROWS			40
#define NROWS_BYTES		(NROWS*sizeof(short))
#define BROWS			4
#define BCOLS			58
#define BSZKM			16.0f
#define NOLNS			8
#define MNOLNS			16
#define I2S_PER_LN		40
#define CHAR_PER_LN		80

/************************************************
                  A3085CA                       *
    ALERT PROCESSING: LOCAL COMMON TABLE DEF.   *
************************************************/
/* For Composite Reflectivity Grid. */
#define TSZE			2
#define GRSTZ			7
#define GRENZ 			50

#define JTYPE			0
#define ITYPE			1

/* For Vil/Echo Tops/Velocity Grids. (Note: These are 
   0-indexed values) */
#define GRSTART			14
#define GREND			43


#define MIDPNTI			28.0f	/* 0-indexed values. */
#define MIDPNTJ			28.0f	/* 0-indexed values. */
#define ISIZE			16.0f
#define JSIZE			16.0f

/* Structure definition for Alert Product Request. */
typedef struct Alert_prod_req {

  /* ICD Product Header. */
  Prod_msg_header_icd phd;

  /* Product Requests. */
  Pd_request_products req;

} Alert_prod_req_t;

/* The following structure information is populated by a3081c.c */
typedef struct {

   float az;

   float ran;

   float elevang;

   char stmid[4];

   float stmspd;

   float stmdir;

   float exval;

   int exval1;

   int threshold;

} A308c3_t;


/* Valid buffer names ingested by this module.  Note:  If you
   add new input types, the macro defining types may need 
   to change. */
#ifdef ALERTING
char *Valid_bufs[NUM_TYPS] = { "BASEVGD",
                               "CRCG460",
                               "ETTAB",
                               "VILTABL",
                               "VADPARAM",
                               "COMBATTR",
                               "HYADJSCN" };
#else
extern char *Valid_bufs[NUM_TYPS];
#endif

#define GRID_BUFFER_MIN		0
#define GRID_BUFFER_MAX		3
#define BVG_BUFFER		0
#define CR_BUFFER		1
#define ET_BUFFER		2
#define VIL_BUFFER		3
#define VAD_BUFFER		4
#define CAT_BUFFER		5
#define MAX_RAIN_BUFFER		6

/* Note:  These macros must match the alert adaptation data
          category definitions. */
#define VELOCITY_ALERT		1
#define CR_ALERT		2
#define ET_ALERT		3
#define VIL_ALERT		6
#define VAD_ALERT		7
#define MAX_RAIN_ALERT		15

/* User Alert Message ID. */
int Alert_msg_id;

/* Alert Product ID. */
int Alert_prod_id;

/* Product IDs associated with "Valid_bufs" */
int Valid_ids[NUM_TYPS];

/* User Grid Change Flag. */
unsigned char User_chg_flg[MAX_CLASS1][NUM_ALERT_AREAS];

typedef struct {

   short category;
   
   short threshold;

   short product_request;

} Alert_grid_t;

/* Alert Grids. */
short Alert_grid[NUM_ALERT_AREAS][MAX_CLASS1][ALRT_GRID_DIM];

/* Line Cross Reference Array. */
int Lnxref[MAX_CLASS1];

/* Hydromet Adjustment Adaptation Data. */
hydromet_adj_t Hydromet_adj;

/* Alerting Adaptation Data. */
alert_threshold_t Alttable;

/* Volume Status. */
Vol_stat_gsm_t Vol_stat;

/* Product Selectable Parameters. */
Prodsel_t Prodsel;

/* Site Adaptation Data. */
Siteadp_adpt_t Siteadp;

/* Active Scan table. */
int Active_scns[NACT_SCNS];

/* Number of expected buffers for scan. */
int Num_ebufs[NACT_SCNS];

/* Expected buffers */
int Bufs_expected[NACT_SCNS][NUM_TYPS];

/* Buffers Done */
int Bufs_done[NACT_SCNS][NUM_TYPS];

/* Number of done buffers for scan. */
int Num_dbufs[NACT_SCNS];

/* User expected buffers. */
int User_ebufs[NACT_SCNS][NUM_TYPS][NWDS_MAX_CLASS1];

/* User Alert Grid Pointers. */
int *Uaptr[NACT_SCNS][NUM_ALERT_AREAS][MAX_CLASS1];

/* Users Alert Product Pointers. */
int *Uam_ptrs[NACT_SCNS][MAX_CLASS1];

/* Storm Information. */
int Strmdir[CAT_MXSTMS];
int Fordir[CAT_MXSTMS];
int Fposdir[CAT_MXSTMS][MAX_FPOSITS];

/* Storm and Forecast Attributes. */
A308c3_t A308c3;


/**************************************************************
C*                   VAD ALERT                               **
C*                   PARAMETERS                              **
C*                    A317AL2P                               **
C**************************************************************
C
C* Offset parameters
C
C* PARG = Pointer to the lowest level vad range, km (INT*4)
C* PABA = Pointer to the beginning azimuth angle processed, deg(INT*4)
C* PAEA = Pointer to the ending azimuth angle processed, deg (INT*4)
C* PAHT = Pointer to the height of the vad analysis, feet (INT*4)
C* PASP = Pointer to the speed at the lowest height, knots (INT*4)
C* PADR = Pointer to the wind direction at the lowest height, deg(I*4)
C* PAMS = Pointer to the missing data flag, (INT*4)
C*
C* Dimensional parameters
C*
C* LEN_VAD_ALERT = Length (full words) of the vad alert buffer */
 
#define PARG				0
#define PABA				1
#define PAEA				2
#define PAHT				3
#define PASP				4
#define PADR				5
#define PAMS				6
#define LEN_VAD_ALERT			7

/* Product Product Generation. */
char Tbuf[TLINES][NROWS_BYTES];
int Ndx[NACT_SCNS][MAX_CLASS1];
int It[NACT_SCNS][MAX_CLASS1];
int Np[NACT_SCNS][MAX_CLASS1];

/* Function Prototypes. */

/* The following functions are defined in alerting_buffer_control.c. */
void A30811_alerting_ctl( int parm );
void ABC_initialize_alerting_buffer_control();
unsigned int ABC_get_update_time();


/* The following functions are defined in alerting_combined_attributes.c. */
int A30810_comb_att( int nstorms, int numfpos, int *ipr1, int vscnix );


/* The following functions are defined in alerting_grid_processing.c. */
int A30851_grid_alert_processing( int *ipr, int dtype, int vscnix );


/* The following functions are defined in alerting_helpers.c. */
int A3081t_setup_direct( short *active_cats, short *thresh, short *prodreq, 
                         short *cstat, int usernum, int areanum );
int A30859_get_cat( short *active_cats, short *thresh, int dtype, int *cat_cde, 
                    int *thrcode );
int A3081b_alert_status( int new_cond, short *cstat, int catix, int *alert_status );
int A3081r_no_prd_msg( int i, int vscnix );
int AH_get_alert_threshold( int cat_code, int thresh_code );
int AH_get_alert_paired_prod( int cat_code );


/* The following functions are defined in alerting_max_rain_alert.c. */
int A3081k_maxrain( int maxaccu, int vscnix );


/* The following functions are defined in alerting_vad_alert.c. */
int A3081j_process_vad_alert( int vscnix, int *VAD_alert );


/* The following functions are defined in alerting_product_generation.c. */
int A30817_do_alerting( int user, int area_num, int stmix, char *stormid,
                        float az, float ran, int threshold, int threshold_code,
                        float exval, int exval1, int alert_status, float elevang, 
                        float strm_spd, float strm_dir, int cat_idx, int vscnix );
int A3081s_store_alert_packet( int i, short *buf, int vscnix );



#endif
