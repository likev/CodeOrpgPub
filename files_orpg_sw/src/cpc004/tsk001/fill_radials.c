/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/02/20 16:09:36 $
 * $Id: fill_radials.c,v 1.11 2009/02/20 16:09:36 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/***************************************************************************

   fill_radials.c
 
   PURPOSE:
 
   This routine fills in empty radials left from initial alignment on a 
   radial by radial basis.
   
   CALLED FROM:

   apply_mpda_
 
   INPUTS:
 
   None.  

   CALLS:

   get_closest_radial
 
   OUTPUTS:
 
   None.
 
   RETURNS:
 
   None.
 
   HISTORY:
 
   B. Conway, 6/96      - original development
   B. Conway, 8/00      - cleanup
   W. Zittel, 12/06     - Updated pointers for ORPG Build 9 
   W. Zittel, 01/2007	- For Build 10 
			  1) Replace pointers with elements of
			     Base_data_header structure in basedata.h 
                          2) Add logic to fill VE & SW final output 
			     range folded bins differently for split cuts
                             where SZ-2 is used
 
****************************************************************************/

#include <stdio.h>
#include <memory.h>
#include "mpda_adapt_params.h"
#include "mpda_constants.h"
#include "mpda_structs.h"
#include <itc.h>
#include <rpgc.h>

int get_closest_radial (int az1, int *az2, int num_rads);

/* Next line added for Build 10, WDZ 01/2007  */
Base_data_header rpg_hdr;

void fill_radials( int *PCT_OBS_VOL_TIME)
{
static float PI = 3.14159;
static float TOTAL_AREA = 282743.1;  /*  PI*300*300  */

  int gate_num, rad_num, status, itc_id = PCT_OBS;
  int fnl_az, temp_az, good_vel_edge, rpg_elv_index = 0;
  float arc_length, bin_size = 0.0;
  float area_factor = 0.0, fbin_range = 0.0, twice_binsize = 0.0;
  float pct_rf = 999.0, area_const = 0.0;

/*
    The following variables are used to compute the range-folded area for
    use by the PRF selection tool.  The formula is derived from the formula
    for the area of an annulus divided by the number radials in a full circle
    
    PI * (R2*R2 - R1*R1)/(No. of radials in the circle)

    Although there may be slightly more than 360 deg. of data, it is assumed
    that the data all fit within 360 degrees.  The result is a close 
    approximation to the real percent coverage of range folding.
*/
  rpg_elv_index = rpg_hdr.rpg_elev_ind;  /*  Changed for Build 10, WDZ 01/2007 */
  if( rpg_elv_index == 1 )
    { 
    arc_length = PI/save.tot_prf1_rads;
    bin_size = rpg_hdr.dop_bin_size * M_TO_KM; /* Changed for Build 10, WDZ 01/2007 */
    twice_binsize = 2.0 * bin_size;
    fbin_range = rpg_hdr.dop_range * bin_size;  /* Changed for Build 10, WDZ 01/2007 */
    area_factor = (float)INT_100 * arc_length * bin_size/TOTAL_AREA;
    area_const = 2.0 * fbin_range - bin_size;
    pct_rf = 0.0;
    }

  for(rad_num=0;rad_num<save.tot_prf1_rads;++rad_num)
    {

/* 
  This processing will fill in "holes" left in PRF2 by the inability to match
  radials in the initial align_prf_scans module.  It essentially finds the
  array index for the radial closest to the one left blank.  However, it uses
  the azimuths for PRF1 because the azimuths for PRF2/PRF3 are unavailable.
*/

    if (save.az_flag2[rad_num] == FALSE )
       {

/* Save the azimuth from PRF1 that corresponds to the empty PRF2 radial */

       temp_az = save.az1[rad_num];

/*
   Essentially remove the saved azimuth so that get closest will return the
   closest radial to the saved one -- blanking it out ensures that you don't
   return the exact same radial, merely the closest one.
*/

       save.az1[rad_num] = 99950;
       fnl_az = get_closest_radial(temp_az, &save.az1[0], save.tot_prf1_rads);

/*  Restore the saved azimuth */

       save.az1[rad_num] = temp_az;

/*
   If the radial is less than the total PRF1 radials, set the flag that
   the radial was filled and copy data from desired radial to the empty
   radial
*/

       if(fnl_az < save.tot_prf1_rads)
          {
          save.az_flag2[rad_num] = TRUE;
          memcpy(&save.vel2[rad_num], &save.vel2[fnl_az],
              MAX_GATES*sizeof(short));
          memcpy(&save.sw2[rad_num], &save.sw2[fnl_az],
              sizeof(save.sw2[fnl_az]));
          }
       }

/* Perform same processing on PRF3 */

    if (save.az_flag3[rad_num] == FALSE )
       {
       temp_az = save.az1[rad_num];
       save.az1[rad_num] = 99950;
       fnl_az = get_closest_radial(temp_az, &save.az1[0], save.tot_prf1_rads);
       save.az1[rad_num] = temp_az;
       if(fnl_az < save.tot_prf1_rads)
          {
          save.az_flag3[rad_num] = TRUE;
          memcpy(&save.vel3[rad_num], &save.vel3[fnl_az],
              MAX_GATES*sizeof(short));
          memcpy(&save.sw3[rad_num], &save.sw3[fnl_az],
              sizeof(save.sw3[fnl_az]));
          }
       }

/* Find the extent of valid data on each radial, this will be used as a limit
   for later processing */

    good_vel_edge = 0;
    for(gate_num=0;gate_num<MAX_GATES;++gate_num)
      {

/*
   For elevations above the SZ-2 split cuts, if any two of the 
   aligned velocity bins are range folded while the last is missing
   or range folded, assign the final output velocity and spectrum
   width values as range folded.
*/

      if(save.vel1[rad_num][gate_num] > DATA_THR &&
         save.vel2[rad_num][gate_num] > DATA_THR &&
         save.vel3[rad_num][gate_num] > DATA_THR)
         {
         if(rpg_elv_index > 2)
	    {
	    if(save.vel1[rad_num][gate_num] == RNG_FLD &&
              (save.vel2[rad_num][gate_num] == RNG_FLD ||
               save.vel3[rad_num][gate_num] == RNG_FLD ))
                 {
                 save.final_vel[rad_num][gate_num] = RNG_FLD;
                 save.final_sw[rad_num][gate_num] = UIF_RNG_FLD;
                 }              

            else if(save.vel2[rad_num][gate_num] == RNG_FLD &&
                    save.vel3[rad_num][gate_num] == RNG_FLD)
                 {
                 save.final_vel[rad_num][gate_num] = RNG_FLD;
                 save.final_sw[rad_num][gate_num] = UIF_RNG_FLD;
                 }
            }
         else
            {  
/*
   If the first Doppler scan's velocity bins (SZ-2) are range folded and the 
   velocity in second and third scan is either missing or range folded, assign
   the output velocity and spectrum width (final) as range folded. 
   Added for Build 10 1/2007 WDZ.
*/
	    if(save.vel1[rad_num][gate_num] == RNG_FLD &&
              (save.vel2[rad_num][gate_num] > DATA_THR &&
               save.vel3[rad_num][gate_num] > DATA_THR ))
               {
/* 
   Compute the percentage of total area that is range folded
*/
               if(rpg_elv_index == 1)
                  pct_rf += area_factor * ((float)gate_num * twice_binsize - area_const);
               save.final_vel[rad_num][gate_num] = RNG_FLD;
               save.final_sw[rad_num][gate_num] = UIF_RNG_FLD;
               }              
            }
         }

      if(save.vel1[rad_num][gate_num] < DATA_THR)
        good_vel_edge = gate_num;
      if(save.vel2[rad_num][gate_num] < DATA_THR)
        good_vel_edge = gate_num;
      if(save.vel3[rad_num][gate_num] < DATA_THR)
        good_vel_edge = gate_num;
          
      save.vel_limit[rad_num] = good_vel_edge + gates_for + 1 < MAX_GATES ?
                                good_vel_edge + gates_for + 1 : MAX_GATES - 1;
      }
    }

    if (rpg_elv_index == 1){

       float *pct_obs = (float * ) (PCT_OBS_VOL_TIME + 2);
       *pct_obs = pct_rf;
       RPGC_itc_write( itc_id, &status );
 
#ifdef DEBUG
    printf("pct_rf = %10.4f\n",pct_rf);
#endif

    }
}
