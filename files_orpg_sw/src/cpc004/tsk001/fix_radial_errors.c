/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:48 $
 * $Id: fix_radial_errors.c,v 1.2 2003/07/17 15:07:48 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*****************************************************************************

   fix_radial_errors.c

   PURPOSE:

   This routine checks for large jumps along the radials and sets them
   to missing for further processing. 
 
   CALLED FROM:

   process_mpda_scans

   INPUTS:

   None.

   CALLS:

   None.

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   HISTORY:

   D. Zittel, 01/02     - remove redundant computations
   B. Conway, 10/00     - cleanup
   B. Conway, 7/99      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"

void
fix_radial_errors()
{

   int i,j,jtemp,k,jmp_st,gt_cnt;
   int tot_prf1_rads, loc_max_rad_jmp, loc_max_rad_bin_cnt;

/* 
   Make local copy of global variables
*/
   tot_prf1_rads = save.tot_prf1_rads;
   loc_max_rad_jmp = max_rad_jmp;
   loc_max_rad_bin_cnt = max_rad_bin_cnt;

/*
   Pass through each radial and compare the two radially adjacent gates.
   If there is a large jump, continue onward and try to find its 
   counterpart radial jump. Reset the gates between the two jumps to
   MISSING for further processing.
*/

   for(i=0; i<tot_prf1_rads; ++i){

      jmp_st = 0;
      gt_cnt = 0;

/*
      For each gate, check its adjacent gates on both sides. 
*/
      jtemp = save.vel_limit[i];
      for(j=1; j<jtemp; ++j)
        {
         ++gt_cnt;
/*
         Check the prior gate. If there is a large jump, set the jmp_st variable
         to the current gate and set the gate count to 0.
*/
         if((save.final_vel[i][j]< DATA_THR) && (save.final_vel[i][j-1] < DATA_THR))
           {
           if( abs(save.final_vel[i][j]-save.final_vel[i][j-1]) > loc_max_rad_jmp)
             {
             if(jmp_st==0)
               jmp_st=j;
             else
               {
               for(k=j-1; k>=jmp_st; --k)
                   if(save.final_vel[i][k] < DATA_THR)
                      save.final_vel[i][k] = MISSING;
               jmp_st = 0;
               }
             gt_cnt = 0;
             }
           }
/*      
      Periodically reset the gt_cnt so that errors are not introduced.
      Here, as a first cut, just using max_az_jmps.
*/
        if(gt_cnt > loc_max_rad_bin_cnt + 1)
          {
          jmp_st =0;
          gt_cnt = 0;
          }
      }

   }

}
