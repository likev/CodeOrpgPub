/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:07:46 $
 * $Id: fix_azimuthal_errors.c,v 1.2 2003/07/17 15:07:46 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*****************************************************************************

   fix_azimuthal_errors.c

   PURPOSE:

   This routine checks for areas along azimuths that have unusually high
   azimuthal shears over unrealistic distances. If these type of shear
   runs are found, the bins producing high shear are set to MISSING.

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

   The basis of this routine is to check the velocity field for azimuthal
   jumps. The routine keeps a running count of large azimuthal jumps taking
   into account both the forward and previous radial differences compared
   to the radial in question.  When a critical number of differences are
   detected, they are set to missing awaiting solution during a later 
   step.

   HISTORY:

   D. Zittel, 02/2003   - Implementation phase final cleanup and remove call to 
                          unf_azimuthal_errors
   B. Conway, 10/00     - cleanup
   B. Conway, 7/99      - original development

****************************************************************************/

#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_adapt_params.h"

void fix_azimuthal_errors(){

   int i,j, jtemp;
   int fst_jmp, prev_az, for_az, az_jmps, jmp_cnt, end_jmp;

   int tot_prf1_rads = save.tot_prf1_rads;
   int rad_offset = save.rad_offset;
   
/*
   Run along each radial searching for runs of large azimuthal jumps.
   First set the for and prev radials
*/

   prev_az = tot_prf1_rads - (rad_offset + 1);
   for_az  = 1;
   for(i = 0; i < tot_prf1_rads; ++i){
  
      if( for_az == tot_prf1_rads ) 
         for_az = rad_offset;

      fst_jmp = -1;
      az_jmps = 0;
      jmp_cnt = 0;

/*    Run along each gate  and check the adjacent gates on the for
      and prev radials for large azimuthal jumps.
*/
      jtemp = save.vel_limit[i];
      for(j=0; j< jtemp; ++j){

         if(save.final_vel[i][j] < DATA_THR){
 
/*
      Check the adjacent gate on the previous radial for a large azimuthal
      jump. If a jump exists, set the counters.
*/

            if(save.final_vel[prev_az][j] < DATA_THR){

               if (abs(save.final_vel[i][j] - save.final_vel[prev_az][j]) 
                       > th_qc_chk){

                  if(fst_jmp == -1)fst_jmp = j;
                  if(jmp_cnt == 0)jmp_cnt = 1;
                  ++az_jmps;
               }

            }
/*
      Check the adjacent gate on the forward radial for a large azimuthal
      jump. If a jump exists, set the counters.
*/
 
            if(save.final_vel[for_az][j] < DATA_THR){
 
               if (abs(save.final_vel[i][j] - save.final_vel[for_az][j]) > th_qc_chk){

                  if(fst_jmp == -1)fst_jmp = j;
                  if(jmp_cnt == 0)jmp_cnt = 1;
                  ++az_jmps;
               }

            } 
 
/*
      If the number of jumps found exceeds the adaptable threshold set
      the save.final_vel data to MISSING unless it's range-folded  
*/ 
            if(az_jmps >= max_az_bin_cnt){
 
               for ( end_jmp = j; end_jmp >= fst_jmp; --end_jmp)
                     save.final_vel[i][end_jmp] = save.final_vel[i][end_jmp] < DATA_THR ?
                     MISSING : save.final_vel[i][end_jmp];

               fst_jmp = -1;
               az_jmps = 0;
               jmp_cnt = 0;

            } 
/*
      Keep track of the number of gates checked in order to reset the
      counts so as not to carry over counted jumps from too far down
      the radial (towards the radar). Note - this is set at 
      2*max_az_bin_cnt.
*/
 
            if(jmp_cnt != 0)++jmp_cnt;

            if(jmp_cnt > 2 * max_az_bin_cnt){
              fst_jmp = -1;
              jmp_cnt = 0;
              az_jmps = 0;
            } 

         } /* final_vel < DATA_THR  */

      }    /* end j loop  */

      prev_az = i;
      for_az++;

   }       /* end i loop  */

}

