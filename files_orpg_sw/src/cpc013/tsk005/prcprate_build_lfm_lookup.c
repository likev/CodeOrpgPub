/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:42 $
 * $Id: prcprate_build_lfm_lookup.c,v 1.1 2005/03/09 15:43:42 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/****************************************************************************
   Filename: prcprate_build_lfm_lookup.c

   Description
   ===========
    This function initializes polar to LFM conversion tables.  

   Change History
   =============
   10/13/92      0000      Bradley Sutker       CCR# NA92-28001
   03/25/93      0001      Toolset              SPR NA93-06801
   01/28/94      0002      Toolset              SPR NA94-01101
   03/03/94      0003      Toolset              SPR NA94-05501
   04/11/96      0004      Toolset              CCR NA95-11802
   12/23/96      0005      Toolset              CCR NA95-11807
   03/16/99      0006      Toolset              CCR NA98-23803
   11/25/04      0007      Cham Pham            CCR NA05-01303
*****************************************************************************/
/*** Global include files  */
#include <prcprtac_main.h>
#include <a3146.h>

/*** Local include file  */
#include "prcprtac_Constants.h"

#define scale_factor   1000.0
#define DTR            0.01745329 
#define HALF           0.5
#define RNG_INC        2.0

void build_lfm_lookup()
{
double b, r, ls, lamdas, pre_gxs, gis,gjs, gi, gj, rl,
       cos_dlamda, sin_dlamda, sin_s, cos_s,
       sin_ls, cos_ls, sin_l, cos_l, sin_b, cos_b,
       cos_lamdas_prime, sin_lamdas_prime;

int    is[LFMMX_IDX],js[LFMMX_IDX],iaz,irg,
       lfmbox,lfm_i,lfm_j,
       status;

static int    ika[]    = {1, 4, 10};
static double ka[]     = {1.0, 4.0, 10.0};
static int    offset[] = {7, 49, 66};

  sin_ls = 0.;
  cos_ls = 0.;

/* If the site latitude or longitude doesnt match the
   table latitude or longitude .... create the tables*/
  if ((sadpt.rda_lat != a314c1.grid_lat)||(sadpt.rda_lon != a314c1.grid_lon)||
      (sadpt.rda_lat != a314c1.end_lat) ||(sadpt.rda_lon != a314c1.end_lon))
  {
/** Convert adaptation data lat/long to double precision data*/
    ls = sadpt.rda_lat/scale_factor;
    lamdas = sadpt.rda_lon/scale_factor;

/** Initialize tables before starting lookup table generation*/
    init_polar_to_lfm_tables( );

/** Compute common part of the gis and gjs equations*/
     pre_gxs=r2ko*cos_ls/(ONE+sin_ls);

/** Compute parts of equations used multiple times later*/
     cos_lamdas_prime = cos((lamdas+prime)*DTR);
     sin_lamdas_prime = sin((lamdas+prime)*DTR);
     sin_ls = sin(ls*DTR);
     cos_ls = cos(ls*DTR);

/** Compute common part of the gis and gjs equations*/
     pre_gxs = r2ko*cos_ls/(ONE+sin_ls);

/** Compute regerence grid box coordinates*/
     gis = pre_gxs*sin_lamdas_prime+ip;
     gjs = pre_gxs*cos_lamdas_prime+jp;

/** Compute grid box numbers for box 0,0 of local grids*/
     is[lfm4_idx] = (int)gis-offset[lfm4_idx];
     js[lfm4_idx] = (int)gjs-offset[lfm4_idx];

     is[lfm16_idx] = ika[lfm16_idx]*(int)gis-offset[lfm16_idx];
     js[lfm16_idx] = ika[lfm16_idx]*(int)gjs-offset[lfm16_idx];

     is[lfm40_idx] = (int)(ka[lfm40_idx]*gis)-offset[lfm40_idx];
     js[lfm40_idx] = (int)(ka[lfm40_idx]*gjs)-offset[lfm40_idx];

/** Initialize bearing*/
     b=-HALF;

/** Do for all bearings*/
    for ( iaz=0; iaz<MAX_AZMTH; iaz++ ) 
    {
       b = b+ONE;
       sin_b = sin(b*DTR);
       cos_b = cos(b*DTR);

/** Initialize range*/
       r = -HALF;

/** Do for each input data range values (comp refl to rcm first)*/
       for ( irg=0; irg<RNG_LFM16; irg++ ) 
       {
          r = r+ONE;
          sin_s = (r/re_proj)*(ONE-(const_val*r/re_proj_sq));
          cos_s = sqrt(ONE-sin_s*sin_s);
          sin_l = sin_ls*cos_s+cos_ls*sin_s*cos_b;
          cos_l = sqrt(ONE-sin_l*sin_l);
          sin_dlamda = sin_s*sin_b/cos_l;
          cos_dlamda = sqrt(ONE-sin_dlamda*sin_dlamda);
          rl = r2ko*cos_l/(ONE+sin_l);
          gi = rl*(sin_dlamda*cos_lamdas_prime+
                 cos_dlamda*sin_lamdas_prime)+ip;
          gj = rl*(cos_dlamda*cos_lamdas_prime-
                 sin_dlamda*sin_lamdas_prime)+jp;

/** Compute 1/16 lfm i and j coordinates of the range/azimuth bin*/
          lfm_i = (int)(gi*ka[lfm16_idx])-is[lfm16_idx];
          lfm_j = (int)(gj*ka[lfm16_idx])-js[lfm16_idx];

/** If lfm coordinates are within local grid save it into the lookup
    table and set the 1/16 lfm box range number table to within range*/
          if ((lfm_i>0)&&(lfm_i<=HYZ_LFM16)&&(lfm_j>0)&&(lfm_j<=HYZ_LFM16))
          {
            lfmbox = (lfm_j-1)*HYZ_LFM16+lfm_i;
            a314c1.lfm16grid[iaz][irg] = lfmbox;
            a314c1.lfm16flag[FLAG_RNG][lfmbox-1] = WITHIN_RANGE;
          }
       }/*end loop RNG_LFM16*/

/** Initialize range*/
       r = -ONE;

/** Do for each input data range values (hydro application next)*/
       for ( irg=0; irg<MAX_HBINS; irg++ ) 
       {
          r = r+RNG_INC;
          sin_s = (r/re_proj)*(ONE-(const_val*r/re_proj_sq));
          cos_s = sqrt(ONE-sin_s*sin_s);
          sin_l = sin_ls*cos_s+cos_ls*sin_s*cos_b;
          cos_l = sqrt(ONE-sin_l*sin_l);
          sin_dlamda = sin_s*sin_b/cos_l;
          cos_dlamda = sqrt(ONE-sin_dlamda*sin_dlamda);
          rl = r2ko*cos_l/(ONE+sin_l);
          gi = rl*(sin_dlamda*cos_lamdas_prime+
                 cos_dlamda*sin_lamdas_prime)+ip;
          gj = rl*(cos_dlamda*cos_lamdas_prime-
                 sin_dlamda*sin_lamdas_prime)+jp;

/** Compute 1/4 lfm i and j coordinates of the range/azimuth bin*/
          lfm_i = (int)(gi*ka[lfm4_idx])-is[lfm4_idx];
          lfm_j = (int)(gj*ka[lfm4_idx])-js[lfm4_idx];

/** If lfm coordinates are within local grid save it into the lookup
    table and set the 1/4 lfm box range number table to within range*/
          if ((lfm_i>0)&&(lfm_i<=HYZ_LFM4)&&(lfm_j>0)&&(lfm_j<=HYZ_LFM4))
          {
            lfmbox = (lfm_j-1)*HYZ_LFM4+lfm_i;
            a314c1.lfm4grid[iaz][irg] = lfmbox;
            a314c1.lfm4flag[FLAG_RNG][lfmbox-1] = WITHIN_RANGE;
          }

/** Compute 1/40 lfm i and j coordinates of the range/azimuth bin*/
          lfm_i = (int)(gi*ka[lfm40_idx])-is[lfm40_idx];
          lfm_j = (int)(gj*ka[lfm40_idx])-js[lfm40_idx];

/** If lfm coordinates are within local grid save it into the lookup
    table and set the 1/40 lfm box range number table to within range*/
          if ((lfm_i>0)&&(lfm_i<=HYZ_LFM40)&&(lfm_j>0)&&(lfm_j<=HYZ_LFM40))
          {
            lfmbox = (lfm_j-1)*HYZ_LFM40+lfm_i;
            a314c1.lfm40grid[iaz][irg] = lfmbox;
            a314c1.lfm40flag[FLAG_RNG][lfmbox-1] = WITHIN_RANGE;
          }

       } /*end loop MAX_HBINS*/

    } /*end loop MAX_AZMTH*/
    
/** Find holes and determine hole filling data*/
    find_holes( ls, lamdas );

/** Set grid_lat and grid_lon ... then write to A314C1 (ITC)*/
    a314c1.grid_lat=sadpt.rda_lat;
    a314c1.grid_lon=sadpt.rda_lon;
    a314c1.end_lat=sadpt.rda_lat;
    a314c1.end_lon=sadpt.rda_lon;

/* Write LFM grids to Inter-Task Common block A314C1 */
    RPGC_itc_write( A314C1, &status );
    if ( status != 0 )
    {
      RPGC_log_msg( GL_ERROR,"Failed to call RPGC_itc_write..( %d )\n",
                                                              status );
    }

  }
}
