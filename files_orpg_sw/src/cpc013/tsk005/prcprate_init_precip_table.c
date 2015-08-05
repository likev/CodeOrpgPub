/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:59 $
 * $Id: prcprate_init_precip_table.c,v 1.2 2006/02/09 18:19:59 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_init_precip_table.c

   Description
   ===========
     This function initializes local copies of the adaptation parameters
   pertaining to the reflectivity-to-precip-rate conversion table, and if
   any have changed since the last volume scan, or upon rpg initialization,
   this function calls the function which initializes the conversion table 
   with precip rate values (mmx10/hr) for each reflectivity index value.   

   Change History
   =============
   08/29/88      0000      Greg Umstead         SPR # 80390
   04/05/90      0001      Dave Hozlock         SPR # 90697
   02/22/91      0002      Bayard Johnston      SPR # 91254
   02/15/91      0002      John Dephilip        SPR # 91762
   12/03/91      0003      Steve Anderson       SPR # 92740
   12/10/91      0004      Ed Nichlas           SPR 92637 PDL Removal
   04/24/92      0005      Toolset              SPR 91895
   10/20/92      0006      Bradley Sutker       CCR NA92-28001
   03/25/93      0007      Toolset              SPR NA93-06801
   01/28/94      0008      Toolset              SPR NA94-01101
   03/03/94      0009      Toolset              SPR NA94-05501
   04/11/96      0010      Toolset              CCR NA95-11802
   12/23/96      0011      Toolset              CCR NA95-11807
   03/16/99      0012      Toolset              CCR NA98-23803
   02/20/05      0013      Cham Pham            CCR NA05-01303
   10/26/05      0014      Cham Pham            CCR NA05-21401
****************************************************************************/
/* Global include files */
#include <prcprtac_main.h>
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

/* Global variables */
extern double zr_pwr_coeff, zr_mlt_coeff, min_dbz, max_dbz;

/* Funtion prototype */
extern void fill_precip_table( double mindbz, double maxdbz,
                               double zrmlt_coeff, double zrpwr_coeff );

void init_precip_table()
{

double zrpwr_coeff,             /*Power coefficient for converting reflectivity
                                  to precipitation rate.                      */
       zrmlt_coeff,             /*Multiplicative coefficient "      "     "   */
       mindbz, maxdbz;          /*Minimum and Maximum reflectivity for  
                                  conversion to precipitation rate.           */
int    update_table = FALSE;    /*Flag indicating whether to update precip rate
                                  conversion table.                           */
  
/* Copy Z-R relationship and min/max reflectivity values from last volume scan
   to local use 
 */
  zrpwr_coeff = zr_pwr_coeff;
  zrmlt_coeff = zr_mlt_coeff;
  mindbz = min_dbz;
  maxdbz = max_dbz;

/* Check if z-r relationship has changed */
  if ( DEBUG ) 
  {
    fprintf(stderr,"ZR_PWR_COEFF: %f ne epre_adpt.zr_exp: %f\n",
              zrpwr_coeff,epre_adpt.zr_exp);
    fprintf(stderr,"ZR_MLT_COEFF: %f ne epre_adpt.zr_mult: %f\n",
              zrmlt_coeff,epre_adpt.zr_mult);
    fprintf(stderr,"MIN_DBZ: %f ne epre_adpt.min_refl_rate: %f\n",
              mindbz,epre_adpt.min_refl_rate);
    fprintf(stderr,"MAX_DBZ: %f ne epre_adpt.max_refl_rate: %f\n",
                  maxdbz,epre_adpt.max_refl_rate); 
  }

  if ( zrpwr_coeff != epre_adpt.zr_exp )  update_table = TRUE;
  if ( zrmlt_coeff != epre_adpt.zr_mult ) update_table = TRUE;
  if ( mindbz != epre_adpt.min_refl_rate ) update_table = TRUE;
  if ( maxdbz != epre_adpt.max_refl_rate ) update_table = TRUE;

  if (DEBUG) 
    {fprintf(stderr,"UPDATE_TABLE: %d\n",update_table);}

  if ( update_table ) 
  {
  /* Reinitialize local values and recompute rate_table */
    zrpwr_coeff = epre_adpt.zr_exp;
    zrmlt_coeff = epre_adpt.zr_mult;
    mindbz = epre_adpt.min_refl_rate;
    maxdbz = epre_adpt.max_refl_rate;
    fill_precip_table( mindbz, maxdbz, zrmlt_coeff, zrpwr_coeff );

  /* Copy local values to global variables */ 
    zr_pwr_coeff = zrpwr_coeff;
    zr_mlt_coeff = zrmlt_coeff;
    min_dbz = mindbz;
    max_dbz = maxdbz;
  }

}
