/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:59 $
 * $Id: prcprate_lfm_to_azran.c,v 1.1 2005/03/09 15:43:59 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_lfm_to_azran.c

   Description
   ===========
    This function initializes polar to lfm conversion tables.

   Change History
   =============
   10/13/92      0000      Bradley Sutker       ccr na92-28001
   03/25/93      0001      toolset              spr na93-06801 
   01/28/94      0002      toolset              spr na94-01101
   03/03/94      0003      toolset              spr na94-05501
   04/11/96      0004      toolset              ccr na95-11802
   12/23/96      0005      toolset              ccr na95-11807
   03/16/99      0006      toolset              ccr na98-23803
   11/25/04      0007      Cham Pham            ccr NA05-01303
*****************************************************************************/
/* Global include file */
#include <a3146.h>

/* Local include file */
#include "prcprtac_Constants.h"

#define R_360 360.0		/* Constant representing the value 360.0      */
#define NINTY  90.0		/* Constant representing the value 90.0       */
#define ZERO    0.0		/* Constant representing the value 0.0        */
#define TWO     2.0		/* Constant representing the value 2.0        */
#define ONE     1.0		/* Constant representing the value 1.0        */
#define DTR     0.01745329	/* Degrees to radians coversion factor        */
#define HALF    0.5		/* Parameter equals to 0.5                    */

void lfm_to_azran ( double ls, double lamdas, int lfmsize, int lfm_i,
                    int lfm_j, double *theta_cij, double *rg_ij )
{
double pre_gxs,gis,gjs,
       cos_lamdas_prime,sin_lamdas_prime,sin_dlamda,
       ai[3],aj[3],cii,cjj,sin_ls,cos_ls,
       aa,bb,cos_ss,sin_ss,cos_lij,sin_lij,
       lamda_ij,l_ij;
int    is[3], js[3], i;

static double b_con[]  = {1.0, 0.25, 0.10};
static int    ika[]    = {1, 4, 10};
static double ka[]     = {1.0, 4.0, 10.0};
static int    offset[] = {7, 49, 66};
int first_time = TRUE;
cos_ls = 0.0;
sin_ls = 0.0;

/* If first call ... initialize ONE time compute variables*/
   if ( first_time == TRUE ) 
   {
     first_time = FALSE;
     cos_lamdas_prime = cos((lamdas+prime)*DTR);
     sin_lamdas_prime = sin((lamdas+prime)*DTR);
     sin_ls = sin(ls*DTR);
     cos_ls = cos(ls*DTR);

/* Compute common part of the gis and gjs equations*/
     pre_gxs = r2ko*cos_ls/(ONE+sin_ls);

/* Compute regerence grid box coordinates*/
     gis = pre_gxs*sin_lamdas_prime+ip;
     gjs = pre_gxs*cos_lamdas_prime+jp;

/* Compute grid box numbers for box 0,0 of local grids*/
     is[lfm4_idx] = (int)gis-offset[lfm4_idx];
     js[lfm4_idx] = (int)gjs-offset[lfm4_idx];
     is[lfm16_idx]= ika[lfm16_idx]*(int)gis-offset[lfm16_idx];
     js[lfm16_idx]= ika[lfm16_idx]*(int)gjs-offset[lfm16_idx];
     is[lfm40_idx]= (int)(ka[lfm40_idx]*gis)-offset[lfm40_idx];
     js[lfm40_idx]= (int)(ka[lfm40_idx]*gjs)-offset[lfm40_idx];

/* Compute ai and aj constants*/
     for ( i=0; i<LFMMX_IDX; i++ ) 
     {
        ai[i] = (double)((is[i]-ip*ka[i]+HALF)/ka[i]);
        aj[i] = (double)((js[i]-jp*ka[i]+HALF)/ka[i]);
     }
   }

   cii = ai[lfmsize]+b_con[lfmsize]*lfm_i;
   cjj = aj[lfmsize]+b_con[lfmsize]*lfm_j;
   l_ij= DTR*NINTY-TWO*atan(sqrt(cii*cii+cjj*cjj)/r2ko);

/* If both inputs to atan2 are 0, dont call function*/
   if ( (cii == ZERO) && (cjj == ZERO) ) 
   {
     lamda_ij = ZERO;
   }
/* Otherwise compute lamda_ij */
   else 
   {
     lamda_ij = -prime*DTR+atan2(cii,cjj);
   }

/* Compute intermediate values*/
   cos_lij = cos(l_ij);
   sin_lij = sin(l_ij);
   sin_dlamda = sin(lamda_ij-DTR*lamdas);
   aa = cos_lij*sin_dlamda;
   bb = cos_ls*sin_lij-sin_ls*cos_lij*cos(lamda_ij-DTR*lamdas);
   sin_ss = sqrt(aa*aa+bb*bb);
   cos_ss = sqrt(ONE-sin_ss*sin_ss);

/* Compute range*/
   *rg_ij = (const_val*sin_ss+re_proj)*sin_ss;

/* If sin_ss is greater than a small positive number compute theta_cij */
   if ( (double)sin_ss>=(double)angle_thresh ) 
   {
     *theta_cij = atan2(cos_lij*cos_ls*sin_dlamda,(sin_lij-sin_ls*cos_ss));
   } 
/* Otherwise, set theta_cij to 0*/
   else 
   {
     *theta_cij = ZERO;
   }

   *theta_cij = *theta_cij/DTR;

/* If angle is less than 0 ... add 360 degrees*/
   if ( *theta_cij < ZERO ) 
   {
     *theta_cij = *theta_cij+R_360;
   }
}
