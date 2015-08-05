/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/03/07 16:19:35 $
 * $Id: save_mpda_data.c,v 1.6 2007/03/07 16:19:35 ryans Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/***************************************************************************

   save_mpda_data.c

   PURPOSE:

   This routine saves the ref and vel radial data for MPDA processing. 

   CALLED FROM:

   apply_mpda1

   INPUTS:

   Arrays containing reflectivity, velocity, and spectrum width data
   
   CALLS:

   save_ref_data
   fix_nyq_value
   range_unfold_prf_scans
   fix_trip_marks
   align_prf_scans
   initialize_uif_table

   OUTPUTS:

   None 

   RETURNS:

   None

   NOTES:

   1) One foundation of this processing is that the first PRF serves as the
      scan to which the remaining PRFs are mapped. Thus the save.prf1 buffer
      contains all the header info, etc. The other save.prf buffers contain
      only velocity data.

   2) Reflectivity data is saved only for the first sweep. The first sweep
      will be either a base ref scan, or a batch scan. These data are used
      later to range dealias the velocity data. 
   
   HISTORY:

   D. Zittel, 02/2007 - Check # of bins in basedata header to see if
                        range unfolding has been done for 2nd/3rd Doppler
                        scans; new for Build 10
   D. Zittel, 10/2006 - Correct computation of save.ps_tot_prf1_rads for 
			Build 9
   D. Zittel, 02/2003 - Implementation phase cleanup and install default
                        switch case
   B. Conway, 5/00    - cleanup
   B. Conway, 5/97    - orginal development

***************************************************************************/

#include <a309.h>
#include <basedata.h>
#include <memory.h>
#include <orpg.h>
#include <rdacnt.h>
#include <vcp.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_vcp_info.h"
#include "mpda_range_unf_data.h"

#define VEL_PRF_OFF 3

void
fix_trip_marks(int radial);

void
initialize_uif_table(int resolution);

void
align_prf_scans( void );

void
range_unfold_prf_scans( void );

void
save_ref_data(unsigned short *refl);

void
fix_nyq_value(short *nyq);

Base_data_header rpg_hdr;

void
save_mpda_data(unsigned short *refl, unsigned short *vel, unsigned short *spw )
{

   int i, rad_num, cut_cnt;

/* 
   Assign data and message structures and housekeeping 
*/

   rad_num = rpg_hdr.azi_num - 1;
   cut_cnt = rpg_hdr.elev_num - 1;
   num_prfs = vcp.vel_num[cut_cnt];

/*
  Process according to the scan number. If it is 0 this is a base 
  reflectivity scan. If 1, save data for PRR 1, do housekeeping,
  if 2, save data for PRF 2, if 3, save PRF 3 data.
*/

   switch (num_prfs)
     {

     case 0 :
         if (rpg_hdr.status == BEG_ELEV || rpg_hdr.status == BEG_VOL)
             atm_atten_fac = (float)(rpg_hdr.atmos_atten)*ATTEN_CONV_FAC;
         save_ref_data(refl);  
     break;

/*
     If this is the first PRF, this will be the structure that the remaining
     data will be mapped to. Thus, the the header info is copied into the
     prf1_buff area. 

     The Nyquist velocity is rounded to the downward to the nearest integer
     division of 1/2 m/s. The number of unambiguous gates is calculated for
     later range dealiasing. 

     Each case below saves the necessary pieces of the header structure. The 
     velocity values are converted from Unysis Integer Format (UIF) to
     shorts are they are put into the respective prf structures.
     
*/

/*   1st PRF  */
     case 1 :                 
         if(rpg_hdr.status == BEG_ELEV || rpg_hdr.status == BEG_VOL)
            {
            if (rpg_hdr.dop_resolution != old_resolution)
               {
               initialize_uif_table(rpg_hdr.dop_resolution);
               old_resolution = rpg_hdr.dop_resolution;
               }
            if(vcp.cut_mode[cut_cnt] == VCP_WAVEFORM_BATCH) 
              atm_atten_fac = (float)(rpg_hdr.atmos_atten)*ATTEN_CONV_FAC;
            save.prf1_nyq        = rpg_hdr.nyquist_vel/INT_10;
            fix_nyq_value(&save.prf1_nyq);
            save.prf1_twice_nyq  = save.prf1_nyq * 2;
            save.prf1_number     = vcp.prf_num[cut_cnt] - VEL_PRF_OFF;
            save.prf1_unab_gates = 
               (int) (((float) (rpg_hdr.unamb_range * 100)) / ((float) rpg_hdr.dop_bin_size));
            save.prf1_n_dop_bins = rpg_hdr.n_dop_bins;
            if(init_prf_arry[num_prfs] == TRUE)
              {
              memset(save.az1, 0, sizeof(save.az1));
              memset(save.vel1, MISSING_BYTE, sizeof(save.vel1));
              memset(save.sw1, UIF_MISSING, sizeof(save.sw1));
              }
            /*  Set the flag to TRUE in case of elevation restart 
                it will be set to FALSE in apply_mpda when the end
                of elevation or volume is reached  */
            init_prf_arry[num_prfs] = TRUE;
            save.ps_tot_prf1_rads = 0;  /* Added for Build 9  WDZ 10/23/2006 */
            }

         if(vcp.cut_mode[cut_cnt] == VCP_WAVEFORM_BATCH)
           save_ref_data(refl);  

         memcpy(&save.ref_buff[rad_num], refl, rpg_hdr.n_surv_bins * sizeof(short));
         /*  The following if block was modified for Build 9 to check for hard 
	     end of elevation and hard end of volume radial status messages.  The 
	     variable save.ps_tot_prf1_rads is initialized to zero at the start
	     of the elevation cut.  Once it has a computed value other than zero,
	     the if block will not be reentered.  This ensures backward compatibility
	     with legacy level 2 data.  WDZ 10/23/2006
         */
         if (( rpg_hdr.status == PSEND_ELEV || rpg_hdr.status == PSEND_VOL  ||
               rpg_hdr.status == END_ELEV   || rpg_hdr.status == END_VOL )  &&
               save.ps_tot_prf1_rads == 0 ){
               save.ps_tot_prf1_rads = rad_num+1;
         }
         save.az1[rad_num]    = rpg_hdr.azimuth * INT_100;
         save.tot_prf1_rads   = rad_num+1;
      
/* Save velocity and spectrum width data */

         memcpy(&save.sw1[rad_num], spw, sizeof(short) * rpg_hdr.n_dop_bins);
         for(i=0;i< rpg_hdr.n_dop_bins; ++i)
            save.vel1[rad_num][i] = uif_table[vel[i]];

         fix_trip_marks( rad_num );
     break;

     case 2 :                 

         if(rpg_hdr.status == BEG_ELEV)
            {
            save.prf2_nyq        = rpg_hdr.nyquist_vel/INT_10;
            fix_nyq_value(&save.prf2_nyq);
            save.prf2_twice_nyq  = save.prf2_nyq * 2;
            save.prf2_number     = vcp.prf_num[cut_cnt] - VEL_PRF_OFF;
            base_scan.sv_dp_intrvl = rpg_hdr.surv_bin_size/rpg_hdr.dop_bin_size;
            save.prf2_unab_gates = 
               (int) (((float) (rpg_hdr.unamb_range * 100)) / ((float) rpg_hdr.dop_bin_size));
            if(init_prf_arry[num_prfs])
              {
              memset(save.az_flag2, FALSE, sizeof(save.az_flag2));
              memset(save.vel2, MISSING_BYTE, sizeof(save.vel2));
              memset(save.sw2, UIF_MISSING, sizeof(save.sw2));
              }
            /*  Set the flag to TRUE in case of elevation restart 
                it will be set to FALSE in apply_mpda when the end
                of elevation or volume is reached  */
            init_prf_arry[num_prfs] = TRUE;
            }

         save.az2             = rpg_hdr.azimuth * INT_100;

/* Clear the radial array and save the data to this array */

         memset(save.sw_rad, UIF_MISSING, sizeof(save.sw_rad));
         memset(save.prf2, MISSING_BYTE, sizeof(save.prf2)); 
         memcpy(save.sw_rad, spw, rpg_hdr.n_dop_bins * sizeof(short));
         for(i=0;i< rpg_hdr.n_dop_bins; ++i)
            save.prf2[i] = uif_table[vel[i]];

/* Range unfold the scan, fix the noise at the edge of the first trip
   and align the scan to the first PRF */
   
/* In Build 10 the ORDA range unfolds all three Doppler scans for the split cuts so range 
   unfolding here is not needed except for batch cuts.  To maintain backwards compatibility
   to pre-build 10 data sets, check that the number of doppler bins in the radial header is
   less than 900 */
   
	 if(rpg_hdr.n_dop_bins < 900)
            range_unfold_prf_scans();
         fix_trip_marks(rad_num);
         align_prf_scans();
     break;
 
     case 3 :                 

         if(rpg_hdr.status == BEG_ELEV)
            {
            save.prf3_nyq        = rpg_hdr.nyquist_vel/INT_10;
            fix_nyq_value(&save.prf3_nyq);
            save.prf3_twice_nyq  = save.prf3_nyq * 2;
            save.prf3_number     = vcp.prf_num[cut_cnt] - VEL_PRF_OFF;
            base_scan.sv_dp_intrvl = rpg_hdr.surv_bin_size/rpg_hdr.dop_bin_size;
            save.prf3_unab_gates = 
               (int) (((float) (rpg_hdr.unamb_range * 100)) / ((float) rpg_hdr.dop_bin_size));
            if(init_prf_arry[num_prfs])
              {
              memset(save.az_flag3, FALSE, sizeof(save.az_flag3));
              memset(save.vel3, MISSING_BYTE, sizeof(save.vel3));
              memset(save.sw3, UIF_MISSING, sizeof(save.sw3));
              }
            /*  Set the flag to TRUE in case of elevation restart 
                it will be set to FALSE in apply_mpda when the end
                of elevation or volume is reached  */
            init_prf_arry[num_prfs] = TRUE;   
            }

         save.az3             = rpg_hdr.azimuth * INT_100;

/* Clear the radial array and save the data to this array */

         memset(save.sw_rad, UIF_MISSING, sizeof(save.sw_rad));
         memset(save.prf3, MISSING_BYTE, sizeof(save.prf3));  
         memcpy(save.sw_rad, spw, rpg_hdr.n_dop_bins * sizeof(short));
         for(i=0;i< rpg_hdr.n_dop_bins; ++i)
            save.prf3[i] = uif_table[vel[i]];

/* Range unfold the scan, fix the noise at the edge of the first trip
   and align the scan to the first PRF */

/* In Build 10 the ORDA range unfolds all three Doppler scans for the split cuts so range 
   unfolding here is not needed except for batch cuts.  To maintain backwards compatibility
   to pre-build 10 data sets, check that the number of doppler bins in the radial header is
   less than 900 */
   
	 if(rpg_hdr.n_dop_bins < 900)
            range_unfold_prf_scans();
         fix_trip_marks(rad_num);
         align_prf_scans();

     break;
     
     default :
       LE_send_msg(GL_ERROR,"MPDA: save_mpda_data - invalid prf = %d\n", num_prfs);

     }
}
