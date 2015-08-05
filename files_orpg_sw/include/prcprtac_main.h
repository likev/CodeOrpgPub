/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 16:58:12 $
 * $Id: prcprtac_main.h,v 1.2 2006/02/09 16:58:12 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
#ifndef PRCPRTAC_MAIN_H
#define PRCPRTAC_MAIN_H
/* System and ORPG Includes------------------------------------------- */
#include <epre_main.h>
#include <siteadp.h>

/* Constant Definitions for PRCPRTAC Algorithms --------------------- */
/*#define C_HYZMESG  6     Size of precipitation status message data array 
#define C_HYZADPT 31       Size of adaptation data array                   
#define C_HYZSUPL 46       Size of suplemental data array              */

#define C_HYZLFM4 13      /* Size of 1/4 lfm grid array                */

#define MAX_AZMTH 360
#define MAX_HBINS 115

/* Local copy of the input buffer*/
EPRE_buf_t  hybscn;
EPRE_supl_t EpreSupl;

/* Buffer-sizing parameters (with & without accum. & hourly scans) */
#define SMLSIZ_ACUM sizeof(PRCPRTAC_smlbuf_t)
#define MEDSIZ_ACUM sizeof(PRCPRTAC_medbuf_t)
#define LRGSIZ_ACUM sizeof(PRCPRTAC_lrgbuf_t)
 
/* Define object for site adpt information struct */
Siteadp_adpt_t sadpt;

/* Data Structure for Output Data Type ------------------------------ */
typedef struct {
  int   HydrMesg[C_HYZMESG];             /* Message array definition          */
  float HydrAdapt[C_HYZADPT];            /* Adaptation data array definition  */
  int   HydrSupl[C_HYZSUPL];             /* Supplemental data array definition*/
  short LFM4grd[C_HYZLFM4][C_HYZLFM4];  /* 1/4 LFM grid array definition     */
}PRCPRTAC_smlbuf_t;

typedef struct {
  PRCPRTAC_smlbuf_t smlbuf_acum;
  short AcumScan[MAX_AZMTH][MAX_HBINS];/* Accumulation arrays scan_to_scan */
}PRCPRTAC_medbuf_t; 

typedef struct {
  PRCPRTAC_medbuf_t medbuf_acum;
  short AcumHrly[MAX_AZMTH][MAX_HBINS];/* Accumulation arrays (hourly scan)*/
}PRCPRTAC_lrgbuf_t; 

/* Declare Object Structure as Global Variables --------------------- */
/** For Rate Accumulation Alg. linear buffer */
  PRCPRTAC_smlbuf_t prcprtacbuf;

/* Declare function prototypes -------------------------------------- */
void initialize_file_io(int *iostatus);
void init_polar_to_lfm_tables(void);
void build_lfm_lookup(void);
void find_holes(double, double);
void lfm_to_azran (double, double, int, int, int, double*, double*);
void Rate_Buffer_Control(int *abort_flg);
void Accum_Buffer_Control(void);

#endif
