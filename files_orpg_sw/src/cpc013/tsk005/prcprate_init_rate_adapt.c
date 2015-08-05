/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:51 $
 * $Id: prcprate_init_rate_adapt.c,v 1.1 2005/03/09 15:43:51 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_init_rate_adapt.c

   Description
   ===========
      This functions initializes adaptation data common area from input buffer
   snapshort, performing any unit conversions needed locally.

   Change History
   =============
   08/29/88      0000      Greg Umstead         spr # 80390
   04/05/90      0001      Dave Hozlock         spr # 90697
   02/22/91      0002      Bayard Johnston      spr # 91254
   02/15/91      0002      John Dephilip        spr # 91762
   12/03/91      0003      Steve Anderson       spr # 92740
   12/10/91      0004      Ed Nichlas           spr 92637 pdl removal
   04/24/92      0005      Toolset              spr 91895
   10/20/92      0006      Bradley Sutker       ccr na92-28001
   03/25/93      0007      Toolset              spr na93-06801
   01/28/94      0008      Toolset              spr na94-01101
   03/03/94      0009      Toolset              spr na94-05501
   04/11/96      0010      Toolset              ccr na95-11802
   12/23/96      0011      Toolset              ccr na95-11807
   03/16/99      0012      Toolset              ccr na98-23803
   01/13/05      0013      D. Miller; J. Liu    CCR NA04-33201
   01/20/05      0014      Cham Pham            CCR NA05-01303
*****************************************************************************/ 
/* Global include files */
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

void init_rate_adapt( )
{
/* Debug writes... */
  if ( DEBUG ) {fprintf(stderr," ***** begin init_rate_adapt()\n");}

/* Make local copies of adaptation data... */
/* Copy cutoff range... */
  a313hadp.ad_cutoff_rng = rate_adpt.rng_cutoff ;

/* Copy range effect coefficients...*/
  a313hadp.ad_c01 = rate_adpt.rng_e1coff ;
  a313hadp.ad_c02 = rate_adpt.rng_e2coff;
  a313hadp.ad_c03 = rate_adpt.rng_e3coff;

/* Copy minimum and maximum rate values...*/
  a313hadp.ad_minrat = rate_adpt.min_prate*RATE_SCALING;
  a313hadp.ad_maxrat = rate_adpt.max_prate*RATE_SCALING;

/** Debug writes...*/
  if ( DEBUG ) 
  {
    fprintf(stderr,"     adaptation data...\n");
    fprintf(stderr,"cutoff range - %8.2f\n",a313hadp.ad_cutoff_rng);
    fprintf(stderr,"r-e coefs - %f -- %f -- %f\n",
                   a313hadp.ad_c01,a313hadp.ad_c02,a313hadp.ad_c03);
    fprintf(stderr,"rate min-max - %d -- %d\n",
                   a313hadp.ad_minrat,a313hadp.ad_maxrat);
    fprintf(stderr," ***** end module init_rate_adapt()\n");
  }
}
