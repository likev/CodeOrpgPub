/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/04/11 17:13:59 $
 * $Id: itc.h,v 1.52 2012/04/11 17:13:59 steves Exp $
 * $Revision: 1.52 $
 * $State: Exp $
 */
/*******************************************************************
      This file contains the definitions that are needed to support
      ITC (inter_task communication).
*******************************************************************/


#ifndef ITC_H
#define ITC_H

#include <vcp.h>
#include <rpg_port.h>

#define SGMTS09             ((ITC_MIN+1) * ITC_IDRANGE + 1)
#define LBID_SGMTS09        SGMTS09 % ITC_IDRANGE 

#define NSEG_ATR              7 
#define NSEG_MAX              6000
#define NSEG_REF              7
#define NRAD_ELV              400
#define NSEG_NID              2

typedef struct {

   float  segmain[ NSEG_MAX*2 ][ NSEG_ATR ]; /* Storm cell segment 
                                               attribute table. */

   short  segindx[ NRAD_ELV*2 ][ NSEG_REF ][ NSEG_NID ];
                                             /* Storm cell segment index 
                                                array. */

   float  segazim[ NRAD_ELV*2 ];             /* Storm cell segment 
                                               azimuths. */

   float  segbuf_th[ NSEG_REF ];             /* Storm cell segment thresholds
                                                per reflectivity threshold. */

} sgmts09;


 
#define A315CSAD            ((ITC_MIN+1) * ITC_IDRANGE + 2)
#define LBID_A315CSAD       A315CSAD % ITC_IDRANGE 

#define NSTA_ADP           57

typedef struct{

   int strmadap[ NSTA_ADP ];		/* SCIT adaptation data - local copy. */

} a315csad;



#define A315LOCK            ((ITC_MIN+1) * ITC_IDRANGE + 3)
#define LBID_A315LOCK       A315LOCK % ITC_IDRANGE 
 
#define HIGH                 2

typedef struct{

   int seg_buf_lock[ HIGH ]; 	/* Segments data base locking flag. */
   
} a315lock;

 

#define A315TRND            ((ITC_MIN+1) * ITC_IDRANGE + 4)
#define LBID_A315TRND       A315TRND % ITC_IDRANGE 

#define NSTF_HKT             1
#define NSTF_HKV             2
#define NSTF_NTV             10
#define NSTF_NTF             8
#define NSTF_MAX             100
#define PER_CELL             (NSTF_HKT+(NSTF_NTV*NSTF_NTF))
#define TREND_SIZE           (PER_CELL*NSTF_MAX)
#define TREND_SIZE4          (TREND_SIZE/2 + 1)
#define MAX_LABELS           260
#define VOLUME_SIZE          (NSTF_HKV+NSTF_NTV)

typedef struct {

   int   cell_trend_data4[ TREND_SIZE4 ];	/* Storm cell trend data. */

   int   cell_pointers[ NSTF_MAX + 2 ];		/* Table of available index pointers 
                                                   in cell_trend_data4 array. */

   int   volume_counter;     			/* Number of volume scans in 
                                                   volume_times array. */

   int   timeptr;                               /* Pointer to current volume scan
                                                   in volume times array. */

   short volume_times[ VOLUME_SIZE ];		/* Array of volume scan start times. */

   short cell_trend_index[ MAX_LABELS ];	/* Table of index pointers in 
                                                   cell_trend_data4 array, indexed 
                                                   by cell id. */

} a315trnd;

 

#define A3CD09              ((ITC_MIN+1) * ITC_IDRANGE + 5)
#define LBID_A3CD09         A3CD09 % ITC_IDRANGE 

#define NSTR_TOT            100
#define NSTR_MOV             12
#define NSTR_INX              2

/* Index values of structure element strmove. 
 STR_XPO - Positional Storm Parameter Index for X Position of the Storm.
 STR_YPO - Positional Storm Parameter Index for Y Position of the Storm.
 STR_DIR - Positional Storm Parameter Index for the Vector Motion of 
           the Storm.
 STR_SPD - Positional Storm Parameter Index for the Relative Speed of 
           the Storm.
 STR_XSP - Positional Storm Parameter Index for X Component of Speed of 
           the Storm.
 STR_YSP - Positional Storm Parameter Index for Y Component of Speed of 
           the Storm.
 STR_AZM - Positional Storm Parameter Index for Azimuth Position of 
           the Storm.
 STR_RAN - Positional Storm Parameter Index for Range Position of 
           the Storm.
 STR_MRF - Positional Storm Parameter Index for Maximum Reflectivity of 
           the Storm.
 STR_RFH - Positional Storm Parameter Index for Height of Maximum Reflectivity
           of the Storm.
 STR_VIL - Positional Storm Parameter Index for Cell-based VIL of the Storm.
 STR_TOP - Positional Storm Parameter Index for Top of the Storm.
*/

#define STR_XP0		      0
#define STR_YP0		      1
#define STR_XSP		      2
#define STR_YSP		      3
#define STR_SPD		      4
#define STR_DIR		      5
#define STR_AZM               6
#define STR_RAN               7
#define STR_MRF               8
#define STR_RFH               9
#define STR_VIL              10
#define STR_TOP              11


typedef struct {

   int   numstrm;		/* Number of storms detected
                                   by SCIT. */

   float timetag;               /* Beginning of volume scan time,
                                   in millisecs past midnight. */

   float avgstspd;              /* Average speed of all storms
                                   detected in scan. */

   float avgstdir;              /* Average direction of all storms
                                   detected in scan. */

   int   strmid[ NSTR_TOT ];	/* ID-labels for all detected
                                   storms. */

   float strmove[ NSTR_TOT ][ NSTR_MOV ]; /* Storm motion data.  Contains:
                                             current x-position,
                                             current y-position,
                                             x-component of storm motion,
                                             y-component of storm motion,
                                             storm speed,
                                             storm direction. */

   int   lokid;			/* Data base locking flag. */

} a3cd09;


#define PVECS09             ((ITC_MIN+1) * ITC_IDRANGE + 6)
#define LBID_PVECS09        PVECS09 % ITC_IDRANGE 

#define N1D_MAX             3000
#define N1D_MAX2            (2 * N1D_MAX)
#define N1D_ATR             4
#define N1D_NID             2
#define NRAD_ELEV           400
#define NRAD_ELEV2          (2 * 400)

typedef struct {

   float tdamain[N1D_MAX2][N1D_ATR];
  				/* Pattern vector attributes. */

   short pv_indx[NRAD_ELEV2][N1D_NID];
		               	/* Pointers to pattern vector by
			 	   radial number. */

   float pv_azim[NRAD_ELEV2]; 	/* Pattern vector azimuths. */

} pvecs09;
 

#define A317CTAD            ((ITC_MIN+1) * ITC_IDRANGE + 7)
#define LBID_A317CTAD       A315CSAD % ITC_IDRANGE 

#define NTDA_AD             30

typedef struct{

   int tdaadap[NTDA_AD];	/* TDA adaptation data - local copy. */

} a317ctad;


#define A317LOCK            ((ITC_MIN+1) * ITC_IDRANGE + 8)
#define LBID_A317LOCK       A317LOCK % ITC_IDRANGE 
 
#define HIGH                 2

typedef struct{

   int tda_buf_lock[ HIGH ]; 	/* TDA data base locking flag. */
   
} a317lock;

#define A3CD11              ((ITC_MIN+1) * ITC_IDRANGE + 9)
#define LBID_A3CD11         A3CD11 % ITC_IDRANGE 

typedef struct{

   int  imxnfeat;
   int  imxnmes;
   int  inpvthr;
   float ihmthr;
   float ilmthr;
   float imsthr;
   float ilsthr;
   float imrthr;
   float ifmrthr;
   float inrthr;
   float ifnrthr;
   float irngthr;
   float idisthr;
   float iazthr;
   float iazthr1;
   float ifhthr;

} a3cd11;
    
/*
 * Selected element of legacy RPG State File (refer to a309.inc).  These
 * elements support the USP product generation and distribution.
 */
#define ITC_CD07_USP       ((ITC_MIN+2) * ITC_IDRANGE + 4)
#define LBID_CD07_USP      ITC_CD07_USP % ITC_IDRANGE
typedef struct {

      int last_date_hrdb;         /* Date hourly database update (Modified Julian) */
      int last_time_hrdb;         /* Time hourly database update (Hour of Day) */

} Hrdb_date_time;

#define A3CD97        ((ITC_MIN + 4) * ITC_IDRANGE + 1)
#define LBID_A3CD97   A3CD97 % ITC_IDRANGE 

#define NPARMS        2
#define LEN_EWTAB     70          /* Size field for ewtab */
#define WNDDIR        0           /* Position of wind direction in 
                                     ewtab */
#define WNDSPD        1           /* Position of wind speed in ewtab */
#define NEPARAMS      2           /* Size field for newndtab */
#define NCOMP         0           /* Position of northerly component
                                     in newndtab */
#define ECOMP         1           /* Position of easterly component
                                     in newndtab */
#define BASEHGT       0           /* Base height of the ewtab, in feet */
#define HGTINC        1000        /* Height increment of ewtab, in feet. */
#define MTTABLE       32767.0     /* Missing value indicator for 
                                     ewtab */
#define MTTABLE_INT   32767       /* Missing value indicator for 
                                     newndtab */
typedef struct{

   int envwndflg;                 /* Flag used to indicate whether VAD/Model
                                     allowed to update table. 1 - yes,
                                     0 - no. */

   float ewtab[NPARMS][LEN_EWTAB]; /* Table of wind speed/direction.
                                     For each of the LEN_EWTAB entries:
                                     WNDDIR - direction in degrees.
                                     WNDSPD - speed in m/s. */

   int   basehgt;		   /* The radar height. */

   short newndtab[LEN_EWTAB][NEPARAMS]; /* Same as ewtab, except data 
                                     in northerly/easterly components.
                                     NCOMP - northerly component.
                                     ECOMP - easterly component. */

   int sound_time;                /* Time the wind table was last 
                                     updated.  In minutes since 
                                     1/1/70. */

} A3cd97;


#define ENVVAD          ((ITC_MIN + 4) * ITC_IDRANGE + 2)
#define LBID_ENVVAD     ENVVAD % ITC_IDRANGE
#define QUE4_ENVWNDVAD  417

typedef struct{

   int parameter_from_vad[2];

} Envvad_t;

#define PCT_OBS		((ITC_MIN + 4) * ITC_IDRANGE + 3)
#define LBID_PCT_OBS	PCT_OBS % ITC_IDRANGE

typedef struct{

   int vol_time;		/* Volume scan start time (secs after midnight) */

   int vol_date;		/* Volume scan start date (modified Julian) */

   float percent_obscured;	/* Percent area obscured by RF */

} Pct_obs;


#define MODEL_EWT       ((ITC_MIN + 4) * ITC_IDRANGE + 4)
#define LBID_MODEL_EWT  MODEL_EWT % ITC_IDRANGE 

/* Has the same data structure as A3CD97. */


#define EWT_UPT		((ITC_MIN + 4) * ITC_IDRANGE + 5)
#define LBID_EWT_UPT	EWT_UPT % ITC_IDRANGE

#define MODEL_UPDATE_DISALLOWED      0
#define MODEL_UPDATE_ALLOWED         1

typedef struct EWT_update {

   int flag;			/* 1 if model data is allowed to update
                                   A3cd97, 0 otherwise. */

} EWT_update_t;


#define MODEL_HAIL	((ITC_MIN + 4) * ITC_IDRANGE + 6)
#define LBID_MODEL_HAIL MODEL_HAIL % ITC_IDRANGE

typedef struct Hail_temps {

   double height_0;

   double height_minus_20;

   int hail_date_yy;

   int hail_date_mm;

   int hail_date_dd;

   int hail_time_hr;

   int hail_time_min;

   int hail_time_sec;

} Hail_temps_t;


#define A3136C3       ((ITC_MIN + 5) * ITC_IDRANGE + 5)
#define LBID_A3136C3  A3136C3 % ITC_IDRANGE

typedef struct{

   int tbupdt;		/* Time (sec) of the bias update (from 0
 			   to 86400. */

   int dbupdt;		/* Date of the biased update. */

   int tbtbl_upd;       /* Time (sec) of last update of local
                           Bias Table. */

   int dbtbl_upd;       /* Date (modified Julian) of last update
                           of local Bias Table. */

   int tbtbl_obs;       /* Observation time (sec) of latest
                           Bias Table. */

   int dbtbl_obs;       /* Observation date (modified Julian)
                           of latest Bias Table. */

   int tbtbl_gen;       /* Generation time (sec) of latest
                           Bias Table. */

   int dbtbl_gen;       /* Generation date (modified Julian)
                           of latest Bias Table. */

   float bias;          /* Bias between rain gage and radar */

   float grpsiz;        /* Effective gage-radar pair (sample)
                           size corresponding to selected bias. */

   float mspan;         /* Memory span (hours) corresponding
                           to selected bias. */

   int timecur;		/* Current time, in seconds of the volume
                           scan (from 0 to 86400). */

   int datecur;		/* Current date. */
   
} A3136C3_t;

#define A304C2         ((ITC_MIN + 7) * ITC_IDRANGE + 1)
#define LBID_A304C2    A304C2 % ITC_IDRANGE 
#define DATAID_A304C2  (A304C2 / ITC_IDRANGE) * ITC_IDRANGE

typedef struct {

   int nw_map_request_pending;		/* Flag indicating outstanding request
				   	   for notchwidth map data */

   int bypass_map_request_pending;	/* Flag indicating outstanding request
					   for bypass map data */

   int unsolicited_nw_received;		/* Flag indicating an unsolicited 
					   notchwidth map received */

} A304c2;

#define CD07_BYPASSMAP         ((ITC_MIN + 7) * ITC_IDRANGE + 3)
#define LBID_CD07_BYPASSMAP    CD07_BYPASSMAP % ITC_IDRANGE 
#define DATAID_CD07_BYPASSMAP  (CD07_BYPASSMAP / ITC_IDRANGE) * ITC_IDRANGE

typedef struct {

   int first_spare;			/* Spare */

   unsigned short bm_gendate;		/* Bypass map generation date, 
                                           modified Julian */

   short bm_gentime;			/* Bypass map generation time, in 
      					   seconds since midnight  */


   int last_spare;			/* Spare */

} cd07_bypassmap;

#define A314C1        ((ITC_MIN + 5) * ITC_IDRANGE + 1) /*(1000+5)*100+1*/
#define LBID_A314C1   A314C1 % ITC_IDRANGE
#define DATAID_A314C1 (A314C1 / ITC_IDRANGE) * ITC_IDRANGE

/* Polar Grid Parameters */
#define  NRADS   360
#define  NBINS   115
#define  FLAG_AZ  0
#define  FLAG_RNG 1
#define  FLAG_SIZE 2
#define  HYZ_LFM4    13
#define  HYZ_LFM16  100
#define  RNG_LFM16  460
#define  HYZ_LFM40  131
#define  NUM_LFM4  (HYZ_LFM4*HYZ_LFM4)
#define  NUM_LFM16 (HYZ_LFM16*HYZ_LFM16)
#define  NUM_LFM40 (HYZ_LFM40*HYZ_LFM40)

typedef struct {
   int grid_lat;
   int grid_lon;
   short lfm4grid[NRADS][NBINS];
   short lfm4flag[FLAG_SIZE][NUM_LFM4];
   short lfm16grid[NRADS][RNG_LFM16];
   short lfm16flag[FLAG_SIZE][NUM_LFM16];
   short lfm40grid[NRADS][NBINS];
   short lfm40flag[FLAG_SIZE][NUM_LFM40];
   int max82val;
   int end_lat;
   int end_lon;
}a314c1_t;


#endif
