/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/05 16:21:26 $
 * $Id: a315buf.h,v 1.2 2012/09/05 16:21:26 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef A315BUF_H
#define A315BUF_H

/******************************************************************
                                                                    
       THIS FILE CONTAINS OUTPUT BUFFER PARAMETER AND DEFINITION    
                           FILES FOR CPC-15                         
                                                                    
*******************************************************************/
/********************* SEGMENTS PARAMETER FILE ********************/

/* Offset Parameters: */
#define BHF			0
#define BFP			1
#define BNP			2
#define BAP			3
#define BAE			4
#define BDA			5
#define BRC			6
#define BET			7
#define BES			8

/* Buffer-size parameter: */
#define SIZ_FILT_REFL 		250000
#define SIZ_SEG  		(BES+1)
 
/* Shared Segment A3CD09 Format Offsets */
 
/* Dimensional Parameters: */
#define NSEG_ATR		7 
#define NSEG_MAX		6000
#define NSEG_NID		2
#define NSEG_REF		7 
#define NRAD_ELV		400 
#define NSTM_ADP 		57 
 
/* Positional Parameters: */
#define SEG_LEN			0
#define SEG_MWL			1
#define SEG_XCN			2
#define SEG_YCN			3
#define SEG_MRF			4
#define SEG_FBN			5 
#define SEG_LBN			6

/********************** CENTROIDS PARAMETER FILE *********************/
/**************************** (A315P5C.PRM) **************************/
 
/* Dimensional Parameters: */
#define NSTM_CHR		14 
#define NSTM_MAX		100
#define NLEV_MAX		25 
#define NSTK_CHR		3
#define NCMP_MAX 		200
#define NSTK_MAX 		(NCMP_MAX*NLEV_MAX)
#define NCEN_ADP		57
 
/* Positional Parameters: 
  STM_AZM - Index to centroid's azimuth in centroid array.
  STM_RAN - Index to centroid's range in centroids array.
  STM_XCN - Index to storm cell x-position in an array of centroid data.
  STM_YCN - Index to storm cell y-position in an array of centroid data.
  STM_ZCN - Index to storm cell z-position in an array of centroid data.
  STM_MRF - Index to maximum refelctivity in centroids array.
  STM_RFH - Index to height in centroids array.
  STM_VIL - Index to cell based VIL in array of centroid data.
  STM_NCP - Index to number of componets in centroid array.
  STM_ENT - Index to first componet in centroid array.
  STM_TOP - Index to storm top in array of centroid data.
  STM_LCT - Index to flag value indicating storm top is from highest 
            elevation cut in centroid array.
  STM_BAS - Index to centroid's base in centroid array.
  STM_LCB - Index to flag value indicating storm base is from lowest 
            elevation cut in centroid array.
*/

#define STM_AZM			0 
#define STM_RAN			1 
#define STM_XCN 		2 
#define STM_YCN 		3 
#define STM_ZCN 		4
#define STM_MRF 		5 
#define STM_RFH 		6 
#define STM_VIL 		7 
#define STM_NCP 		8 
#define STM_ENT			9
#define STM_TOP 		10 
#define STM_LCT 		11 
#define STM_BAS 		12 
#define STM_LCB 		13

#define STK_ZCN 		0 
#define STK_MRF 		1 
#define STK_PTR 		2
 
/* Offset Parameters: */
#define BVT 			0 
#define BNS 			1 
#define BNK 			2 
#define BST 			3 
#define BSK   			(BST+NSTM_CHR*NSTM_MAX)
#define BSA   			(BSK+NSTK_CHR*NSTK_MAX)
 
/* Buffer-size parameter: */
#define SIZ_CEN  		(BSA+NCEN_ADP)

/*********************** FORECAST PARAMETER FILE ********************/
/**************************** (A315P7F.PRM) *************************/
 
/* Dimensional Parameters: */
#define NSTF_IDT		3 
#define NSTF_MOT 		8 
#define NSTF_FOR 		2 
#define NSTF_BAK		2
#define NSTF_MPV 		13 
#define NSTF_INT 		4 
#define NSTF_MAX 		100 
#define NSTF_ADP 		57 
 
/* Positional Parameters: */
 
/*  -- Storm Cell ID-Type table: */
#define STF_ID			0 
#define STF_TYP 		1 
#define STF_NVL 		2 
 
/*  -- Storm Cell Motion table: */
#define STF_X0 			0 
#define STF_Y0 			1 
#define STF_XSP 		2 
#define STF_YSP 		3 
#define STF_SPD 		4
#define STF_DIR 		5 
#define STF_ERR 		6 
#define STF_MFE 		7
 
/*  -- Storm Cell Forward table: */
#define STF_XF 			0 
#define STF_YF 			1 

/*  -- Storm Cell Backward table: */
#define STF_XB 			0 
#define STF_YB 			1 
 
/* Flagging Parameters: */
#define TYP_CON 		0 
#define TYP_NEW 		1 
#define TYP_CHG 		2 
 
#define UNDEF 			-999.99 
 
/* Offset Parameters: */
#define BNT			0 
#define BNO 			1 
#define BVS 			2 
#define BVD 			3 
#define BSI 			4
#define BSM  			(BSI+NSTF_IDT*NSTF_MAX)
#define BSF  			(BSM+NSTF_MOT*NSTF_MAX)
#define BSB  			(BSF+NSTF_FOR*NSTF_INT*NSTF_MAX)
#define BFA 			(BSB+NSTF_BAK*NSTF_MPV*NSTF_MAX)
 
/* Buffer-size parameter: */
#define SIZ_FOR  		(BFA + NSTF_ADP)


/****************HAIL TREND/STRUCTURE PARAMETER FILE ****************/
/************************** (A315P8T.PRM) ***************************/
 
/* Dimensional Parameters: */
#define	NSTR_MAX 		100 
 
/* Offset Parameters: */
#define BNR 			0 
#define BRI 			1 
 
/* Buffer-size parameter: */
#define SIZ_STR  		(BRI+NSTR_MAX)


/************************ HAIL PARAMETER FILE ************************/
/*************************** (A315P9H.PRM) ***************************/
 
/* Dimensional Parameters: */
#define NHAL_STS 		6 
#define NHAL_MAX 		100 
#define NHAL_ADP 		32 
 
/* Positional Parameters within HAILSTATS: */
#define H_POH  			0
#define H_PSH  			1
#define H_MHS  			2
#define H_RNG  			3
#define H_AZM  			4
#define H_CID 			5
 
/* Flagging Hail Label Parameters for the RCM: */
#define LAB_POS 		1 
#define LAB_PRB 		2 
#define LAB_NEG 		3 
#define LAB_UNK 		4 
 
/* Flag value for UNKNOWN or cells out of range */
#define UNKNOWN_HAIL  		-999
 
/* Offset Parameters within the Output Buffer (HAILATTR): */
#define BNH  			0
#define BHS  			1
#define BHL  			(BHS+NHAL_STS*NHAL_MAX)
#define BHA  			(BHL+NHAL_MAX)
 
/* Buffer-size parameter: */
#define SIZ_HAL  		(BHA+NHAL_ADP)


/******************************************************************
 A315PSAD    -  PARAMETER FILE DEFINING LENGTHS AND OFFSETS    
                FOR STORMS ADAPTATION DATA                     
******************************************************************/
                                                                    
/* Total size of STORMS Adaptation Data Buffer: */
#define NSTA_ADP  		57 
 
/* Maximum number of Reflectivity Levels: */
#define MAX_REF_LEVELS  	7 
 
/* Maximum number of search radii. */
#define MAX_SEARCH_RADIUS  	3 
 
/* Offset to beginning of primary field of local Adaptation Data: */
#define OFF_STA  		0 
 
/* Offsets within primary field of local Adaptation Data: */
#define STA_REF1  		0
#define STA_REF2  		1 
#define STA_REF3  		2 
#define STA_REF4  		3
#define STA_REF5  		4 
#define STA_REF6  		5 
#define STA_REF7  		6 
#define STA_SGL1  		7 
#define STA_SGL2  		8 
#define STA_SGL3  		9 
#define STA_SGL4  		10 
#define STA_SGL5  		11 
#define STA_SGL6  		12
#define STA_SGL7  		13 
#define STA_CPA1  		14 
#define STA_CPA2  		15 
#define STA_CPA3  		16 
#define STA_CPA4  		17 
#define STA_CPA5  		18 
#define STA_CPA6  		19 
#define STA_CPA7  		20 
#define STA_RLVL  		21 
#define STA_NDRO  		22 
#define STA_RDIF  		23 
#define STA_NAVG  		24 
#define STA_MWTF 		25
#define STA_MULF  		26 
#define STA_MCOF  		27 
#define STA_SGMX  		28 
#define STA_RGMX  		29 
#define STA_OVLP  		30 
#define STA_AZMD  		31 
#define STA_SRD1  		32 
#define STA_SRD2 		33
#define STA_SRD3  		34 
#define STA_DDEL  		35 
#define STA_HDEL  		36 
#define STA_ELMR  		37
#define STA_HEMR  		38 
#define STA_HOMR  		39
#define STA_MNSG  		40
#define STA_CMPX  		41 
#define STA_STMX  		42 
#define STA_VILX  		43 
#define STA_DFDI  		44 
#define STA_DFSP  		45 
#define STA_MAXT  		46 
#define STA_PVOL  		47 
#define STA_COSP  		48 
#define STA_SPMN  		49 
#define STA_ALER 		50 
#define STA_FINT  		51 
#define STA_NFOR  		52 
#define STA_ERIN  		53 
#define STA_RSGM  		54 
#define STA_MXPC  		55 
#define STA_MXDS  		56

/*******************************************************************
   PARAMETER FILE DEFINING LENGTHS AND OFFSETS FOR HAIL ADAPTATION 
   DATA
*******************************************************************/
 
/* Offset to beginning of primary field of local Adaptation Data: */
#define OFF_HA 			0 
 
/* Offsets within primary field of local Adaptation Data: */
#define HA_H0 			0  
#define HA_H20 			1 
#define HA_KE1 			2 
#define HA_KE2 			3 
#define HA_KE3 			4 
#define HA_PSC 			5 
#define HA_PSO 			6 
#define HA_HSC		 	7
#define HA_HSE 			8 
#define HA_RWL 			9 
#define HA_RWU 			10 
#define HA_WTC 			11 
#define HA_WTO 			12 
#define HA_PO1 			13 
#define HA_PO2 			14 
#define HA_PO3 			15 
#define HA_PO4 			16 
#define HA_PO5 			17 
#define HA_PO6 			18 
#define HA_PO7 			19 
#define HA_PO8 			20 
#define HA_PO9 			21 
#define HA_PO0 			22 
#define HA_MRP 			23 
#define HA_RHL 			24 
#define HA_SHL 			25 
#define HA_XRG 			26 

/* Offsets to Hail Temperature Altitude Time/Date Stamp: */
#define HA_THR 			27 
#define HA_TMN 			28 
#define HA_TDA 			29 
#define HA_TMO 			30 
#define HA_TYR 			31 

/****************** SCIT ALGORITHMS ADAPTATION DATA ******************/
/***************************** (A315CSAD) ****************************/
typedef struct Storm_adapt {

   int adprflev[MAX_REF_LEVELS];	/* Threshold - Reflectivity. */

   float adpsgl[MAX_REF_LEVELS];	/* Threshold - Segment Length. */

   float adpareth[MAX_REF_LEVELS];	/* Threshold - Component Area. */

   int adpnlvls;			/* Number Ref Levels. */

   int adpndro;				/* Threshold - Dropout Count */

   int adprdif;				/* Threshold - Dropout Ref Difference. */

   int adpnavg;				/* Threshold - Ref Average Factor. */

   float adpmwtf;			/* Mass Weighted Factor. */

   float adpmult;			/* Mass Multiplicative Factor. */

   float adpmcof;			/* Mass Coefficient Factor. */

   int adpsgmx;				/* Max Segments Per Component. */

   int adprgmx;				/* Max Segment Range. */

   int adpovlap;			/* Threshold - Segment Overlap. */

   float adpdelaz;			/* Threshold - Azximuth Separation. */

   float adprdsth[MAX_SEARCH_RADIUS];	/* Threshold - Search Radius. */

   float adpdpdel; 			/* Threshold - Depth Delete. */

   float adphzdel;			/* Threshold - Horizontal Delete. */
   
   float adpelmrg;			/* Threshold - Elevation Merge. */

   float adphtmrg;			/* Threshold - Height Merge. */

   float adphzmrg;			/* Threshold - Horizontal Merge. */

   int adpsegmn;			/* Threshold - Segments/Component. */

   int adpcmpx;				/* Maximum Components Per Elevation. */

   int adpstmx;				/* Maximum Cells Per Volume. */

   int adpvilx;				/* Threshold - Max Cell-Based VIL. */

   int adpdfdi;				/* Default Direction. */

   float adpdfspd;			/* Default Speed. */

   int adpmaxt;				/* Maximum Time. */

   int adppvol;				/* Number of Past Volumes. */

   float adpcosp;			/* Correlation Speed. */

   float adpspmn;			/* Threshold - Minimum Speed. */

   int adpaler;				/* Allowable Error. */

   int adpfint;				/* Forecast Interval. */

   int adpnfor;				/* Number Intervals. */

   int adperin;				/* Error Interval. */

   int adprsgm;				/* Maximum Segments Per Radial. */

   int adpmxpc;				/* Threshold - Max Pot Comps/Elevation. */

   int adpmxds;				/* Maximum Detected Cells. */

} Storm_adapt_t;
   
   
/* Cell Trend and Volume Scan List */
#define NSTF_HKT		1 
#define NSTF_HKV 		2 
#define NSTF_NTF 		8 
#define NSTF_NTV 		10
 
/*  -- Storm Cell Trend Data list and Volume Scan list: */
#define PER_CELL 		(NSTF_HKT+(NSTF_NTV*NSTF_NTF))
#define TREND_SIZE 		(PER_CELL*NSTF_MAX)
#define TREND_SIZE4 		(TREND_SIZE/2+1)
#define VOLUME_SIZE 		(NSTF_HKV+NSTF_NTV)
#define VOLUME_SIZE4 		(VOLUME_SIZE/2+1)
 
/*  --Offsets used in volume time list. */
#define NVOL_CVOL 		0
#define TIMEOFF  		0
 
/*  --Offsets used in cell trend list. */
#define NVOLOFF 		0
#define CTOPOFF 		0
#define CBASEOFF 		(CTOPOFF+NSTF_NTV)
#define MAXZHOFF 		(CBASEOFF+NSTF_NTV)
#define POHOFF 			(MAXZHOFF+NSTF_NTV)
#define POSHOFF 		(POHOFF+NSTF_NTV)
#define VILOFF 			(POSHOFF+NSTF_NTV)
#define MAXZOFF 		(VILOFF+NSTF_NTV)
#define CHGTOFF 		(MAXZOFF+NSTF_NTV)
 
/*  --Trend codes in trend data list: */
#define CELL_TOP_CODE 		1
#define CELL_BASE_CODE 		2
#define MAXZ_HGT_CODE 		3
#define POH_CODE 		4
#define POSH_CODE 		5
#define VIL_CODE 		6
#define MAXZ_CODE 		7
#define CENTROID_HGT_CODE 	8
 
#define MAX_LABELS  		260
#define TPBASOFF 		1000

/******************* SEGMENT BUFFER LOCK DEFINITIONS *****************/
#define LOW  			0 
#define HIGH  			1

#define UNAVAILABLE 		1 
#define AVAILABLE		0

#endif
