#ifndef A313HPARM_H
#define A313HPARM_H

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/05/14 16:32:38 $
 * $Id: a313hparm.h,v 1.4 2008/05/14 16:32:38 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/*******************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A313HBUF.INC
*.
*.  INCLUDE FILE VERSION: 06
*.
*.  INCLUDE FILE LANGUAGE: FORTRAN
*.
*.  CHANGE HISTORY:
*.
*.  DATE         RV   SPR#          PROGRAMMER         NOTES
*.  ---------    --   -----         ----------------   --------------
*.  01 04 91     00   90888         PAUL JENDROWSKI    A313HYPP
*.  02 20 91     00   91685         PAUL JENDROWSKI    A313HYPP
*.  10 20 92     01   NA92-28001    BRADLEY SUTKER     HYPP
*.  06 02 95     02   NA94-35301    R. RIERSON         A313HYPP
*.  07 26 95     02   NA95-13201    Richard Fulton     A313HYPP
*.  10 10 95     02   NA94-33202    Dong-Jun Seo       A313HYPP
*.  01/31/02     03   NA01-27501    D.Miller, F.Ding   A3CD70C2, A3CD70C4
*.  07/31/02     03   NA02-11605    C. Pham, D. Miller A313HYPP
*.  06/30/03     04   NA02-06508    C. Pham, D. Miller A313HYPP, A313HYP3
*.  01/13/05     05   NA04-33201    C. Pham, D. Miller A313HYPP
*.  01/26/07     06   NA07-01702    Jihong Liu         A313HYPP 
*.*******************************************************************
**A313HYPP
*VERSION: 0003
C************* HYDROMET SHARED BUFFER PARAMETER FILE ***************/

/* SIZING PARAMETERS FOR SHARED ARRAYS */ 

/* .....WITHIN ADAPTATION DATA ARRAY */
#define ASIZ_PRE	14
#define ASIZ_RATE	6 
#define	ASIZ_ACUM	7 
#define ASIZ_ADJU	4

/* .....WITHIN SUPPLEMENTAL DATA ARRAY */
#define SSIZ_PRE	15
#define SSIZ_RATE	10
#define SSIZ_ACUM	16
#define SSIZ_ADJU	5

/* .....ALL SHARED ARRAYS */
#define HYZ_MESG	6
#define HYZ_ADAP 	(ASIZ_PRE+ASIZ_RATE+ASIZ_ACUM+ASIZ_ADJU)
#define HYZ_SUPL	(SSIZ_PRE+SSIZ_RATE+SSIZ_ACUM+SSIZ_ADJU)
#define HYZ_LFM4	13
#define MAX_AZMTHS	360
#define MAX_RADS	400

/* OFFSET PARAMETERS FOR SHARED ARRAYS */
#define HYO_MESG	0
#define HYO_ADAP	(HYO_MESG+HYZ_MESG)
#define HYO_SUPL	(HYO_ADAP+HYZ_ADAP)
#define HYO_LFM4	(HYO_SUPL + HYZ_SUPL)
 
/*------------------------------------------------------------------- */
 
/* SIZING PARAMETERS FOR PREPROC. ARRAYS */
#define MAX_HYBINS	230
 
/* OFFSET PARAMETERS FOR PREPROC. ARRAYS */
#define HYO_HYBRID	(HYO_SUPL + HYZ_SUPL)
#define HYO_HYBRID_E	(HYO_HYBRID + (MAX_HYBINS*MAX_AZMTHS+1)/2)
 
/* SIZING PARAMETERS FOR RATE ARRAYS */
#define MAX_RABINS	115
 
/* OFFSET PARAMETERS FOR RATE ARRAYS */
#define HYO_RATE	(HYO_LFM4 + (HYZ_LFM4*HYZ_LFM4+1)/2)
 
/* SIZING PARAMETERS FOR ACCUMULATION ARRAYS */
#define MAX_ACUBINS	MAX_RABINS
 
/* OFFSET PARAMETERS FOR ACCUMULATION ARRAYS */
#define HYO_ACUSCAN	(HYO_LFM4+(HYZ_LFM4*HYZ_LFM4+1)/2)
#define HYO_ACUHRLY	(HYO_ACUSCAN+(MAX_ACUBINS*MAX_AZMTHS+1)/2)
 
/* SIZING PARAMETERS FOR ADJUSTMENT ARRAYS */
#define N_BIAS_FLDS	5 
#define N_BIAS_LINES	10
#define HYZ_BHEADER	1
#define HYZ_BTABL  	(N_BIAS_FLDS*N_BIAS_LINES)
#define MAX_ADJBINS	MAX_ACUBINS
 
/* OFFSET PARAMETERS FOR ADJUSTMENT ARRAYS */
#define HYO_BHEADER  	(HYO_LFM4+(HYZ_LFM4*HYZ_LFM4+1)/2)
#define HYO_BTABL  	(HYO_BHEADER + HYZ_BHEADER)
#define HYO_ADJSCAN 	(HYO_BTABL + HYZ_BTABL)
#define HYO_ADJHRLY	(HYO_ADJSCAN+(MAX_ADJBINS*MAX_AZMTHS+1)/2)
 
/*-------------------------------------------------------------------*/
 
/* POSITIONAL PARAMETERS WITHIN PRECIP_STATUS_MESSAGE ARRAY */
#define DAT_STAMP  	1 
#define TIM_STAMP 	2
#define DAT_LAST	3 
#define TIM_LAST 	4
#define PCP_CATCUR 	5 
#define PCP_CATLST 	6
 
/*-------------------------------------------------------------------*/
 
/* OFFSET PARAMETERS WITHIN ADAPTATION DATA ARRAY */
#define AOFF_PRE	0
#define AOFF_RATE	(AOFF_PRE+ASIZ_PRE)
#define AOFF_ACUM 	(AOFF_RATE+ASIZ_RATE)
#define AOFF_ADJU 	(AOFF_ACUM+ASIZ_ACUM)
 
/* POSITIONAL PARAMETERS WITHIN ADAPTATION DATA ARRAY */
 
/* ...ENHANCED PREPROCESSING */
#define BEAM_WDTH 	1 
#define BLK_THRES 	2
#define CLUT_THRES 	3 
#define WEIG_THRES  	4
#define FHYS_THRES 	5 
#define LOWZ_THRES  	6
#define RAIN_ZTHR  	7 
#define RAIN_ATHR   	8
#define RAIN_TMTHR 	9 
#define MLT_ZRCOEF 	10
#define PWR_ZRCOEF	11
#define MIN_ZREFL  	12
#define MAX_ZREFL 	13 
#define NUM_EXZONE 	14
 
/* ...PRECIPITATION RATE */
#define RNG_CUTOFF	1 
#define RNG_E1COEF 	2
#define RNG_E2COEF 	3 
#define RNG_E3COEF 	4
#define MIN_PRATE	5 
#define MAX_PRATE  	6
 
/* ...PRECIPITATION ACCUMULATION */
#define	TIM_RESTRT 	1 
#define MAX_TIMINT 	2 
#define MIN_TIMPRD 	3
#define THR_HLYOUT 	4 
#define END_TIMGAG 	5
#define MAX_PRDVAL 	6 
#define MAX_HLYVAL 	7
 
/* ...PRECIPITATION ADJUSTMENT */
#define TIM_BIEST  	1 
#define THR_NPAIRS 	2
#define RES_BIAS   	3 
#define LNGST_LAG  	4
 
/*-------------------------------------------------------------------*/
 
/* OFFSET PARAMETERS WITHIN SUPPLEMENTAL DATA ARRAY */
#define SOFF_PRE	0
#define SOFF_RATE 	(SOFF_PRE+SSIZ_PRE)
#define SOFF_ACUM 	(SOFF_RATE+SSIZ_RATE)
#define SOFF_ADJU 	(SOFF_ACUM+SSIZ_ACUM)
 
/* POSITIONAL PARAMETERS WITHIN SUPPLEMENTAL DATA ARRAY */
 
/* ...ENHANCED PREPROCESSING */
#define AVG_SCNDAT 	(SOFF_PRE+0)
#define AVG_SCNTIM 	(SOFF_PRE+1)
#define FLG_ZEROHY 	(SOFF_PRE+2) 
#define FLG_RAINDET 	(SOFF_PRE+3)
#define FLG_RESTP 	(SOFF_PRE+4)
#define FLG_PCPBEG 	(SOFF_PRE+5)
#define LST_DATRN 	(SOFF_PRE+6)
#define LST_TIMRN 	(SOFF_PRE+7)
#define BLKG_CNT  	(SOFF_PRE+8)
#define CLUTR_CNT 	(SOFF_PRE+9)
#define TBIN_SMTH 	(SOFF_PRE+10)
#define HYS_FILL 	(SOFF_PRE+11)
#define HIG_ELANG 	(SOFF_PRE+12)
#define RAIN_AREA 	(SOFF_PRE+13)
#define VOL_STAT  	(SOFF_PRE+14)
 
/* ...PRECIPITATION RATE */
#define FLG_ZERATE 	(SOFF_RATE+0)
#define FLG_BADSCN 	(SOFF_RATE+1)
#define CNT_BADSCN 	(SOFF_RATE+2)
#define FLG_ZERREF 	(SOFF_RATE+3) 
#define REF_SCNDAT	(SOFF_RATE+4)
#define REF_SCNTIM 	(SOFF_RATE+5)
#define TIM_SCNDIF 	(SOFF_RATE+6)
#define BAD_SCNPTR 	(SOFF_RATE+7)
#define REF_SCNGOD 	(SOFF_RATE+8)
#define SCAN_SB_STAT 	(SOFF_RATE+9)
 
/* ...PRECIPITATION ACCUMULATION */
#define FLG_ZERSCN 	(SOFF_ACUM+0)
#define FLG_MSGPRD 	(SOFF_ACUM+1)
#define BEG_MISDAT 	(SOFF_ACUM+2) 
#define BEG_MISTIM	(SOFF_ACUM+3)
#define END_MISDAT 	(SOFF_ACUM+4) 
#define END_MISTIM 	(SOFF_ACUM+5)
#define FLG_ZERHLY 	(SOFF_ACUM+6)
#define FLG_NOHRLY	(SOFF_ACUM+7)
#define BEG_HRDATE 	(SOFF_ACUM+8)
#define BEG_HRTIME 	(SOFF_ACUM+9)
#define END_HRDATE 	(SOFF_ACUM+10)
#define END_HRTIME 	(SOFF_ACUM+11)
#define HLY_SCNTYP 	(SOFF_ACUM+12)
#define NUM_INTOUT 	(SOFF_ACUM+13)
#define MAX_HLYACU 	(SOFF_ACUM+14)
#define FLG_SPOT_BLANK	(SOFF_ACUM+15)
 
/* ...PRECIPITATION ADJUSTMENT */
#define CUR_BIAS   	(SOFF_ADJU+0) 
#define CUR_GRPSIZ 	(SOFF_ADJU+1)
#define DAT_BCALC  	(SOFF_ADJU+2) 
#define TIM_BCALC  	(SOFF_ADJU+3)
#define CUR_MSPAN  	(SOFF_ADJU+4)
 
/*-------------------------------------------------------------------*/
 
/* HYDROMET SHARED PARAMETER VALUES */
 
#define	FLG_MISSNG	-99 
#define FLG_NOBIAS 	-98
#define FLAG_CLEAR	0 
#define FLAG_SET	1
#define END_CURR 	0 
#define END_CLOCK 	1 
#define END_GAGE 	2 
#define END_BOTH 	3
#define RATE_SCALING 	10.0
/* Ward 20080407 - Conflicts with definition in ~/lib/include/rpgcs_coordinates.h
#define PI 		3.14159 */
#define PI 		3.14159265
#define DEG_TO_RAD 	0.017453
 
/* ALARM FLAG FOR A3CM08 CALLS FOR DISK I/O ERRORS */
#define HYALARM_FLG	0
 
/* FLAG VALUE FOR ILLEGAL TIME DIFFERENCE THAT REQUIRES DATABASE RE-INITIALIZATION */
#define FLAG_TIME_DIF 	1999999999

/* Data structure for enhanced preprocessing ...*/
typedef struct {

  float beam_width;
  float block_thresh;
  float clutter_thresh;
  float weight_thresh;
  float full_hys_thresh;
  float low_dbz_thresh;
  float rain_dbz_thresh;
  float rain_area_thresh;
  float rain_time_thresh;
  float zr_mult;
  float zr_exp;
  float min_refl_rate;
  float max_refl_rate;
  float num_zone;

}EpreAdapt_t;

/* Data structure for Precipitation rate ... */
typedef struct {

  float rng_cutoff;
  float rng_e1coff;
  float rng_e2coff;
  float rng_e3coff;
  float min_prate;
  float max_prate;

}RateAdapt_t;

/* Data structure for Precipitation accumulation ...*/
typedef struct {
 
 float tim_restrt;
  float max_timint;
  float min_timprd;
  float thr_hlyout;
  float end_timgag;
  float max_prdval;
  float max_hlyval;

}AcumAdapt_t;

/* Data Structure for Rate Supplemental Data Type */
typedef struct {

  int flg_zerate;
  int flg_badscn;
  int cnt_badscn;
  int flg_zerref;
  int ref_scndat;
  int ref_scntim;
  int tim_scndif;
  int bad_scnptr;
  int ref_scngod;
  int scan_sb_stat;

}PRCPRTAC_rate_supl_t;

/* Data Structure for Accumulation Supplemental Data Type */
typedef struct {

  int flg_zerscn;
  int flg_msgprd;
  int beg_misdat;
  int beg_mistim;
  int end_misdat;
  int end_mistim;
  int flg_zerhly;
  int flg_nohrly;
  int beg_hrdate;
  int beg_hrtime;
  int end_hrdate;
  int end_hrtime;
  int hly_scntyp;
  int num_intout;
  int max_hlyacu;
  int flg_spot_blank;

}PRCPRTAC_acc_supl_t;


#endif
