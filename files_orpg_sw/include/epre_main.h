/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 16:57:26 $
 * $Id: epre_main.h,v 1.5 2006/02/09 16:57:26 ryans Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef EPRE_MAIN_H_
#define EPRE_MAIN_H_

/* System and ORPG Includes------------------------------------------- */
#include <itc.h>
#include <vcp.h>
#include <rpgc.h>
#include <rpgcs.h>
#include <orpg.h> 
#include <orpgctype.h>
#include <product.h>
#include <orpgpat.h>
#include <orpgsite.h>
#include <rpg.h>
#include <basedata.h>

/* HYDROMET Adaptation Includes ------------------------------------- */
#include <hydromet_prep.h>
#include <hydromet_rate.h>
#include <hydromet_acc.h>
#include <hydromet_adj.h>

/* EPRE Constants Include -------------------------------------------*/
#include"epreConstants.h" 

/* Constant Definitions for EPRE Algorithm -------------------------- */
#define C_HYZMESG 6       /* Size of precipitation status message
                             data array                               */
#define C_HYZADPT 31      /* Size of adaptation data array            */
#define C_HYZSUPL 46      /* Size of suplemental data array           */
#define LRGSIZ_HYBRID sizeof(EPRE_buf_t) /*331572*/
#define PCODE 0           /* Indicate as intermediate product         */
#define MAX_NUM_ZONE 20
#define EXZONE_FIELDS 5


/* Data Structure for Supplemental Data Type */   
typedef struct
{
  int avgdate;
  int avgtime;
  int zerohybrd;
  int rain_detec_flg;
  int reset_stp_flg;
  int prcp_begin_flg;
  int last_date_rain;
  int last_time_rain;
  int rej_blkg_cnt;
  int rej_cltr_cnt;
  int tot_bins_smooth;
  float pct_hys_filled;
  float highest_elang;
  float sum_area;
  int vol_sb;
 
}EPRE_supl_t;

/* Data Structure for Output Data Type ------------------------------ */
typedef struct
{
  int   HydroMesg[C_HYZMESG];
  float HydroAdapt[C_HYZADPT];
  int   HydroSupl[C_HYZSUPL];
  short HyScanZ[MAX_AZM][MAX_RNG];
  short HyScanE[MAX_AZM][MAX_RNG];

} EPRE_buf_t;

/* Declare Object Structure as Global Variables --------------------- */

/** For Adaptation data **/
  hydromet_prep_t hyd_epre;
  hydromet_rate_t hyd_rate;
  hydromet_acc_t  hyd_acc;
  hydromet_adj_t  hyd_adj;

/** For Supplemental data **/
  EPRE_supl_t a3133c7;

/** For Output buffer **/
  EPRE_buf_t epre_buf;

  double exzone[MAX_NUM_ZONE][EXZONE_FIELDS];

#endif  /* _EPRE_MAIN_H */
