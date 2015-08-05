/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/11/24 23:40:51 $
 * $Id: read_adapt_supl.c,v 1.7 2009/11/24 23:40:51 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/*********************************************************************
  copy_adapt_supl() initializes local copies of all the adaptation
                    parameters needed for the Enhanced Preprocessing,
                    Rate, Accummulation, and Adjustment algorithms
                    from RPG common block (Fortran).
    Change History
    ============
    DATE          VERSION    PROGRAMMER         NOTES
    ----------    -------    ---------------    ------------------
    10/26/05      0001       C. Pham            CCR NA05-21401
    07/22/09      0003       Zhan Zhang      Comment out the code snippet that
                                             swap the begin/end azimuths 
                                             if end azimuth < begin azimuth.
*********************************************************************/

/* ORPG includes ---------------------------------------------------- */
#include <rpg_globals.h>
#include <orpgctype.h>

/* Local include ---------------------------------------------------- */
#include "epre_main.h"


void copy_adapt_supl(float *adapt, int *supl)
{
  int i;

  /* Copy RPG adaptation parameters to output buffer */

  /* Enhanced Preprocessing parameters */
  adapt[0]=hyd_epre.beam_width;
  adapt[1]=hyd_epre.block_thresh;
  adapt[2]=hyd_epre.clutter_thresh;
  adapt[3]=hyd_epre.weight_thresh;
  adapt[4]=hyd_epre.full_hys_thresh;
  adapt[5]=hyd_epre.low_dbz_thresh;
  adapt[6]=hyd_epre.rain_dbz_thresh;
  adapt[7]=hyd_epre.rain_area_thresh;
  adapt[8]=hyd_epre.rain_time_thresh;
  adapt[9]=hyd_rate.zr_mult;
  adapt[10]=hyd_rate.zr_exp;
  adapt[11]=hyd_epre.min_refl_rate;
  adapt[12]=hyd_epre.max_refl_rate;
  adapt[13]=hyd_epre.num_zone;

  /* Exclusion Zones.*/
  exzone[0][0]=(double)hyd_epre.Beg_azm1;
  exzone[0][1]=(double)hyd_epre.End_azm1;
  exzone[0][2]=(double)hyd_epre.Beg_rng1 * NM_TO_KM;
  exzone[0][3]=(double)hyd_epre.End_rng1 * NM_TO_KM;
  exzone[0][4]=(double)hyd_epre.Elev_agl1;

  exzone[1][0]=(double)hyd_epre.Beg_azm2;
  exzone[1][1]=(double)hyd_epre.End_azm2;
  exzone[1][2]=(double)hyd_epre.Beg_rng2 * NM_TO_KM;
  exzone[1][3]=(double)hyd_epre.End_rng2 * NM_TO_KM;
  exzone[1][4]=(double)hyd_epre.Elev_agl2;

  exzone[2][0]=(double)hyd_epre.Beg_azm3;
  exzone[2][1]=(double)hyd_epre.End_azm3;
  exzone[2][2]=(double)hyd_epre.Beg_rng3 * NM_TO_KM;
  exzone[2][3]=(double)hyd_epre.End_rng3 * NM_TO_KM;
  exzone[2][4]=(double)hyd_epre.Elev_agl3;

  exzone[3][0]=(double)hyd_epre.Beg_azm4;
  exzone[3][1]=(double)hyd_epre.End_azm4;
  exzone[3][2]=(double)hyd_epre.Beg_rng4 * NM_TO_KM;
  exzone[3][3]=(double)hyd_epre.End_rng4 * NM_TO_KM;
  exzone[3][4]=(double)hyd_epre.Elev_agl4;

  exzone[4][0]=(double)hyd_epre.Beg_azm5;
  exzone[4][1]=(double)hyd_epre.End_azm5;
  exzone[4][2]=(double)hyd_epre.Beg_rng5 * NM_TO_KM;
  exzone[4][3]=(double)hyd_epre.End_rng5 * NM_TO_KM;
  exzone[4][4]=(double)hyd_epre.Elev_agl5;

  exzone[5][0]=(double)hyd_epre.Beg_azm6;
  exzone[5][1]=(double)hyd_epre.End_azm6;
  exzone[5][2]=(double)hyd_epre.Beg_rng6 * NM_TO_KM;
  exzone[5][3]=(double)hyd_epre.End_rng6 * NM_TO_KM;
  exzone[5][4]=(double)hyd_epre.Elev_agl6;

  exzone[6][0]=(double)hyd_epre.Beg_azm7;
  exzone[6][1]=(double)hyd_epre.End_azm7;
  exzone[6][2]=(double)hyd_epre.Beg_rng7 * NM_TO_KM;
  exzone[6][3]=(double)hyd_epre.End_rng7 * NM_TO_KM;
  exzone[6][4]=(double)hyd_epre.Elev_agl7;

  exzone[7][0]=(double)hyd_epre.Beg_azm8;
  exzone[7][1]=(double)hyd_epre.End_azm8;
  exzone[7][2]=(double)hyd_epre.Beg_rng8 * NM_TO_KM;
  exzone[7][3]=(double)hyd_epre.End_rng8 * NM_TO_KM;
  exzone[7][4]=(double)hyd_epre.Elev_agl8;

  exzone[8][0]=(double)hyd_epre.Beg_azm9;
  exzone[8][1]=(double)hyd_epre.End_azm9;
  exzone[8][2]=(double)hyd_epre.Beg_rng9 * NM_TO_KM;
  exzone[8][3]=(double)hyd_epre.End_rng9 * NM_TO_KM;
  exzone[8][4]=(double)hyd_epre.Elev_agl9;

  exzone[9][0]=(double)hyd_epre.Beg_azm10;
  exzone[9][1]=(double)hyd_epre.End_azm10;
  exzone[9][2]=(double)hyd_epre.Beg_rng10 * NM_TO_KM;
  exzone[9][3]=(double)hyd_epre.End_rng10 * NM_TO_KM;
  exzone[9][4]=(double)hyd_epre.Elev_agl10;

  exzone[10][0]=(double)hyd_epre.Beg_azm11;
  exzone[10][1]=(double)hyd_epre.End_azm11;
  exzone[10][2]=(double)hyd_epre.Beg_rng11 * NM_TO_KM;
  exzone[10][3]=(double)hyd_epre.End_rng11 * NM_TO_KM;
  exzone[10][4]=(double)hyd_epre.Elev_agl11;

  exzone[11][0]=(double)hyd_epre.Beg_azm12;
  exzone[11][1]=(double)hyd_epre.End_azm12;
  exzone[11][2]=(double)hyd_epre.Beg_rng12 * NM_TO_KM;
  exzone[11][3]=(double)hyd_epre.End_rng12 * NM_TO_KM;
  exzone[11][4]=(double)hyd_epre.Elev_agl12;

  exzone[12][0]=(double)hyd_epre.Beg_azm13;
  exzone[12][1]=(double)hyd_epre.End_azm13;
  exzone[12][2]=(double)hyd_epre.Beg_rng13 * NM_TO_KM;
  exzone[12][3]=(double)hyd_epre.End_rng13 * NM_TO_KM;
  exzone[12][4]=(double)hyd_epre.Elev_agl13;

  exzone[13][0]=(double)hyd_epre.Beg_azm14;
  exzone[13][1]=(double)hyd_epre.End_azm14;
  exzone[13][2]=(double)hyd_epre.Beg_rng14 * NM_TO_KM;
  exzone[13][3]=(double)hyd_epre.End_rng14 * NM_TO_KM;
  exzone[13][4]=(double)hyd_epre.Elev_agl14;

  exzone[14][0]=(double)hyd_epre.Beg_azm15;
  exzone[14][1]=(double)hyd_epre.End_azm15;
  exzone[14][2]=(double)hyd_epre.Beg_rng15 * NM_TO_KM;
  exzone[14][3]=(double)hyd_epre.End_rng15 * NM_TO_KM;
  exzone[14][4]=(double)hyd_epre.Elev_agl15;

  exzone[15][0]=(double)hyd_epre.Beg_azm16;
  exzone[15][1]=(double)hyd_epre.End_azm16;
  exzone[15][2]=(double)hyd_epre.Beg_rng16 * NM_TO_KM;
  exzone[15][3]=(double)hyd_epre.End_rng16 * NM_TO_KM;
  exzone[15][4]=(double)hyd_epre.Elev_agl16;

  exzone[16][0]=(double)hyd_epre.Beg_azm17;
  exzone[16][1]=(double)hyd_epre.End_azm17;
  exzone[16][2]=(double)hyd_epre.Beg_rng17 * NM_TO_KM;
  exzone[16][3]=(double)hyd_epre.End_rng17 * NM_TO_KM;
  exzone[16][4]=(double)hyd_epre.Elev_agl17;

  exzone[17][0]=(double)hyd_epre.Beg_azm18;
  exzone[17][1]=(double)hyd_epre.End_azm18;
  exzone[17][2]=(double)hyd_epre.Beg_rng18 * NM_TO_KM;
  exzone[17][3]=(double)hyd_epre.End_rng18 * NM_TO_KM;
  exzone[17][4]=(double)hyd_epre.Elev_agl18;

  exzone[18][0]=(double)hyd_epre.Beg_azm19;
  exzone[18][1]=(double)hyd_epre.End_azm19;
  exzone[18][2]=(double)hyd_epre.Beg_rng19 * NM_TO_KM;
  exzone[18][3]=(double)hyd_epre.End_rng19 * NM_TO_KM;
  exzone[18][4]=(double)hyd_epre.Elev_agl19;

  exzone[19][0]=(double)hyd_epre.Beg_azm20;
  exzone[19][1]=(double)hyd_epre.End_azm20;
  exzone[19][2]=(double)hyd_epre.Beg_rng20 * NM_TO_KM;
  exzone[19][3]=(double)hyd_epre.End_rng20 * NM_TO_KM;
  exzone[19][4]=(double)hyd_epre.Elev_agl20;

  /* Do some validation of the exclusion zone definitions ( assert 
     beginning range < ending range && beginning azimuth < ending
     azimuth ). */
  for( i = 0; i < hyd_epre.num_zone; i++ ){

     double tmp;

     /* Swap the begin/end ranges if end range < begin range. */
     if( exzone[i][3] < exzone[i][2] ){

        tmp = exzone[i][3];
        exzone[i][3] = exzone[i][2];
        exzone[i][2] = tmp;

     }

    
     /* Swap the begin/end azimuths if end azimuth < begin azimuth. 
        NOTE: It is assumed there are no exclusion zones which cross
              north. */
     /* comment out this code */  
   /*  if( exzone[i][1] < exzone[i][0] ){

        tmp = exzone[i][1];
        exzone[i][1] = exzone[i][0];
        exzone[i][0] = tmp;

     }
   */  
  }

  /* Rate adaptation parameters */
  adapt[14]=hyd_rate.range_cutoff;
  adapt[15]=hyd_rate.range_coef1;
  adapt[16]=hyd_rate.range_coef2;
  adapt[17]=hyd_rate.range_coef3;
  adapt[18]=hyd_rate.min_precip_rate;
  adapt[19]=hyd_rate.max_precip_rate;
  
  /* Accumulation adaptation parameters */
  adapt[20]=hyd_acc.restart_time;
  adapt[21]=hyd_acc.max_interp_time;
  adapt[22]=hyd_acc.min_time_period;
  adapt[23]=hyd_acc.hourly_outlier;
  adapt[24]=hyd_acc.end_gage_time;
  adapt[25]=hyd_acc.max_period_acc;
  adapt[26]=hyd_acc.max_hourly_acc;

  /* Adjustment adaptation parameters */
  adapt[27]=hyd_adj.time_bias;
  adapt[28]=hyd_adj.num_grpairs;
  adapt[29]=hyd_adj.reset_bias;
  adapt[30]=hyd_adj.longst_lag;

  /* Initialize Supplemental data to 0 */
  for ( i=0; i<C_HYZSUPL; i++ )
      *supl++=ZERO;
}
