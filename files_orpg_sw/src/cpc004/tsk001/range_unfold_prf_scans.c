/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:20 $
 * $Id: range_unfold_prf_scans.c,v 1.2 2003/07/17 15:08:20 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************

   range_unfold_prf_scans.c

   PURPOSE:

   This routine is the driver for range unfolding the multi-PRF scans. It
   uses the reflectivity collected from the surveillance or batch scans
   to range dealias the additional velocity scans.

   CALLED FROM:

   save_mpda_data

   INPUTS:

   None.

   CALLS:

   get_trip_num
   get_closest_radial
   rng_unf_chks_okc
   assign_vel_trip

   OUTPUTS:

   None.

   RETURNS:

   None.

   NOTES:

   1) This routine basically repeats the same processing for each PRF. The first
      PRF is range dealiased at the RDA. Thus this processes the 2nd and 3rd
      PRFs.

   HISTORY:

   B. Conway, 10/96      - original development
   B. Conway, 11/00      - cleanup 
   D. Zittel, 02/2003    - Implementation phase cleanup and add switch default
                           case
   R. May, 04/2003	 - Added support for creating spectrum width field

****************************************************************************/
#include <orpg.h>
#include "mpda_adapt_params.h"
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_range_unf_data.h"

int 
get_trip_num(int cur_az, int bin_num,
             int unabg_gates, float tover, int veldis);
int 
get_closest_radial (int azm, int *az2, int num_rads);

int
rng_unf_chks_ok(short *trip1_vel, unsigned short *trip1_sw, int vel2_loc,
                float trip1_pwr, float trip2_pwr);

void
assign_vel_trip(int trip_gate, int num_trip1_gates, float trip1_pwr, 
                float trip2_pwr, short *trip1_vel, short *tripgt_vel,
                short *trip2_vel, unsigned short *trip1_sw,
                unsigned short *tripgt_sw, unsigned short *trip2_sw);

void
range_unfold_prf_scans( void )
{
   
   int   j, trip_gate, ref_az;
   int   vel2_loc, ref1_loc,ref2_loc;

/* 
   This loop process the 2nd prf available. The steps are to loop through
   each radial for the 2nd prf and first find the azimuth from the base ref 
   scan that is closest to the radial in question. The next step is to check
   that the power at the 1st and 2nd trips are valid and assign a gate for
   the trip. Finally, assign a velocity to the correct trip. 
*/

     switch(num_prfs)
     {
     
     case 2 :
       ref_az = get_closest_radial(save.az2, &base_scan.az[0],
                                   base_scan.tot_ref_rads);
       if(ref_az > base_scan.tot_ref_rads)
          return;

/*
      Check each gate along the radial and try to assign the correct 
      velocity at the correct trip gate. Note the ref locations must
      be incremented by 1 to work. 
*/

      for(j=0; j< save.prf2_unab_gates; ++j)
         {
         vel2_loc = j+save.prf2_unab_gates;
         ref1_loc = j/base_scan.sv_dp_intrvl+1;
         ref2_loc = vel2_loc/base_scan.sv_dp_intrvl+1;
         if(ref2_loc >= base_scan.num_gates[ref_az])
           base_scan.pwr[ref_az][ref2_loc] = MISSING_PWR;
         
/*
         If there are no valid power values, the velocity gate is
         set to MISSING.
*/

         if (rng_unf_chks_ok(&save.prf2[j], &save.sw_rad[j], vel2_loc, 
             base_scan.pwr[ref_az][ref1_loc],base_scan.pwr[ref_az][ref2_loc]) 
               == FALSE)
                   continue;

/*
         Find the correct gate for the trip number to assign the
         velocity to
*/

         if(vel2_loc > MAX_GATES-1)
            vel2_loc = MAX_GATES-1;
         trip_gate = get_trip_num(ref_az, j, save.prf2_unab_gates, 
                         mpda_tover, base_scan.sv_dp_intrvl);

/*
        If the trip_gate returned is < 0, it means the gate is 
        ambigious. In this case the velocity will be rng folded if
        there is actually power at the gate and the 2nd trip gate,
        otherwise the velocity is set to MISSING.
*/

        if(trip_gate == MISSING_PWR)
          {
          save.prf2[j] = (base_scan.pwr[ref_az][ref1_loc]>PWR_THR) ?  
                             RNG_FLD : MISSING;
          save.sw_rad[j] = (base_scan.pwr[ref_az][ref1_loc]>PWR_THR) ?  
                             UIF_RNG_FLD : UIF_MISSING;
          if(vel2_loc < MAX_GATES)
            if(base_scan.pwr[ref_az][ref2_loc]>PWR_THR)
               {
               save.prf2[vel2_loc] = RNG_FLD;
               save.sw_rad[vel2_loc] = UIF_RNG_FLD;
               }
          }
        else

/*
          A valid trip range was found for the velocity in question. So
          assign it depending on the powers in the different trips.
*/

          assign_vel_trip(trip_gate, save.prf2_unab_gates,
                          base_scan.pwr[ref_az][ref1_loc],
                          base_scan.pwr[ref_az][ref2_loc],&save.prf2[j],
                          &save.prf2[trip_gate],&save.prf2[vel2_loc],
                          &save.sw_rad[j], &save.sw_rad[trip_gate],
                          &save.sw_rad[vel2_loc]);
         }

       break;

/*
  Perform this processing if we have a radial of PRF3
*/


    case 3 :

       ref_az = get_closest_radial(save.az3, &base_scan.az[0],
                                   base_scan.tot_ref_rads);

       if(ref_az > base_scan.tot_ref_rads)
          return;
 
/*
      Check each gate along the radial and try to assign the correct
      velocity at the correct trip gate. Note the ref locations must
      be incremented by 1 to work. 
*/

      for(j=0; j< save.prf3_unab_gates; ++j)
         {
         vel2_loc = j+save.prf3_unab_gates;
         ref1_loc = j/base_scan.sv_dp_intrvl+1;
         ref2_loc = vel2_loc/base_scan.sv_dp_intrvl+1;
         if(ref2_loc >= base_scan.num_gates[ref_az])
           base_scan.pwr[ref_az][ref2_loc] = MISSING_PWR;
 
/*
         If there are no power valid power values, the velocity gate is
         set to MISSING.
*/

         if (rng_unf_chks_ok(&save.prf3[j], &save.sw_rad[j], vel2_loc,
             base_scan.pwr[ref_az][ref1_loc],base_scan.pwr[ref_az][ref2_loc])
               == FALSE)
            continue;
 
/*
         Find the correct gate for the trip number to assign the
         velocity to
*/

         if(vel2_loc > MAX_GATES-1)
            vel2_loc = MAX_GATES-1;
         trip_gate = get_trip_num(ref_az, j, save.prf3_unab_gates,
                         mpda_tover, base_scan.sv_dp_intrvl);
 
/*
        If the trip_gate returned is < 0, it means the gate is
        ambigious. In this case the velocity will be rng folded if
        there is actually power at the gate and the 2nd trip range,
        MISSING otherwise.
*/

        if(trip_gate == MISSING_PWR)
          {
          save.prf3[j] = (base_scan.pwr[ref_az][ref1_loc]>PWR_THR) ?
                             RNG_FLD : MISSING;
          save.sw_rad[j] = (base_scan.pwr[ref_az][ref1_loc]>PWR_THR) ?  
                             UIF_RNG_FLD : UIF_MISSING;
          if(vel2_loc < MAX_GATES)
            if(base_scan.pwr[ref_az][ref2_loc]>PWR_THR)
               {
               save.prf3[vel2_loc] = RNG_FLD;
               save.sw_rad[vel2_loc] = UIF_RNG_FLD;
               }
          }
        else

/*
          A valid trip range was found for the velocity in question. So
          assign it depending on the powers in the different trips.
*/

          assign_vel_trip(trip_gate, save.prf3_unab_gates,
                           base_scan.pwr[ref_az][ref1_loc],
                           base_scan.pwr[ref_az][ref2_loc],&save.prf3[j],
                           &save.prf3[trip_gate],&save.prf3[vel2_loc],
                           &save.sw_rad[j], &save.sw_rad[trip_gate],
                           &save.sw_rad[vel2_loc]);
        }
       break;
       
       default:
         LE_send_msg(GL_ERROR,"MPDA: range_unfold_prf_scans - invalid prf = %d\n",num_prfs);
       }
}
