/*  
 * $rcs 
 * $author$
 * $locker$   
 * $date$
 * $id$
 * $revision$   
 * $state$ 
 */ 
#ifndef A313HBUF_H
#define A313HBUF_H

#include <a313hparm.h>

/* Data structure for enhanced preprocessing ...*/ 
EpreAdapt_t epre_adpt;

/* Data structure for Precipitation rate ... */
RateAdapt_t rate_adpt; 

/* Data structure for Precipitation accumulation ...*/
AcumAdapt_t acum_adpt;

/* Data Structure for Rate Supplemental Data Type */
PRCPRTAC_rate_supl_t RateSupl; 

/* Data Structure for Accumulation Supplemental Data Type */
/* Declare global object struct */
PRCPRTAC_acc_supl_t AcumSupl;

/*/**a313hyd3 ---------------------------------------------------*/
/*//version: 0*/

/* Hybrid scan array */
short HybrScan[MAX_AZMTHS][MAX_HYBINS];

/*/**a313hyd4 ----------------------------------------------------*/
/*//version: 0*/

/* Precipitation rate definition file */
/* Rate scan array */
short RateScan[MAX_AZMTHS][MAX_RABINS];

/* 1/4 LFM grid */
short lfm4Grd[HYZ_LFM4*HYZ_LFM4];
 
#endif        /* #ifndef A313HBUF_H */
