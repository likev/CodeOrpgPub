/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/08 22:32:54 $
 * $Id: save_ref_data.c,v 1.3 2006/09/08 22:32:54 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/***************************************************************************
   save_ref_data.c

   PURPOSE:

   This routine saves the base reflectivity data for range dealiasing
   the velocity scans as well as reflectivity output.

   CALLED FROM:

   save_mpda_data

   INPUTS:

   Array containing reflectivity data 

   CALLS:

   None

   OUTPUTS:

   None

   RETURNS:

   None

   HISTORY:

   B Conway, 5/00    - cleanup
   B Conway, 5/97    - orginal development

************************************************************************/
#include <memory.h>
#include "mpda_constants.h"
#include "mpda_range_unf_data.h"
#include "mpda_structs.h"
#include "basedata.h"

Base_data_header rpg_hdr;
struct mpda_data save;

void save_ref_data(unsigned short *refl)
{

   float rng;
   int i, rad_num, sv_cnt;

/* 
   Assign data and message structures and housekeeping for each radial 
*/

   sv_cnt = rpg_hdr.n_surv_bins;
   rad_num = rpg_hdr.azi_num - 1;

   base_scan.az[rad_num]        = rpg_hdr.azimuth * INT_100;
   base_scan.num_gates[rad_num] = sv_cnt; 

/*
   The loop below assigns power values according to the equation in
   the Level II documentation. Note the range used is in km for the
   power calculations.
*/
   
   for (i=0; i<sv_cnt; ++i)
       if (refl[i] > 1)
          {
          rng = (float)
                (rpg_hdr.surv_range + i) * rpg_hdr.surv_bin_size * M_TO_KM;
          base_scan.pwr[rad_num][i] = base_scan.dBZ[refl[i]] - 
               base_scan.log10_rng[i] - atm_atten_fac * rng;
          }
       else
          base_scan.pwr[rad_num][i] = MISSING_PWR;

   base_scan.tot_ref_rads = rpg_hdr.azi_num;
}
