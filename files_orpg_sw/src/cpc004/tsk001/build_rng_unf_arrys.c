/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/08 12:52:43 $
 * $Id: build_rng_unf_arrys.c,v 1.3 2006/09/08 12:52:43 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/**************************************************************************************
     build_rng_unf_arrys.c
     
     PURPOSE:
        
     Module to build lookup tables used in range unfolding the 2nd and 3rd
     velocity cuts.
     
     CALLED FROM:
     
     mpda_buf_cntrl.c
     
     INPUTS:
     
     None
     
     CALLS:
     
     None
     
     RETURNS:
     
     None
     
     HISTORY:
     
     D. Zittel, 12/14/2001  - original development
     
*************************************************************************************/

#include <math.h>
#include <basedata.h>
#include "mpda_constants.h"
#include "mpda_range_unf_data.h"

Base_data_header rpg_hdr;

void build_rng_unf_arrys()
{
   int i;
   
   /*  Compute the reflectivity (dBZ) from the biased integer  */
   
   for(i=2; i<MAX_NUM_UIF;++i)
      base_scan.dBZ[i] = (float)(i*0.5) - 33.0;
      
   /*  Compute 20 * log10 for ranges in whole km steps  */

   for (i = 1; i < MAX_REF_GATES ; ++i)
     base_scan.log10_rng[i] = 20. * log10((float)
                (rpg_hdr.surv_range + i) * rpg_hdr.surv_bin_size * M_TO_KM);

   /*  Account for undefined case of log(0) = undefined */

   base_scan.log10_rng[0] = base_scan.log10_rng[1];

   return;
}
