/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/06/22 21:51:43 $
 * $Id: a30531.c,v 1.17 2012/06/22 21:51:43 steves Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */
#define A30531_C
#include <prfselect.h>
#include <math.h>

#define ASCALE           0.001
#define LOGFAC         -20.0

#define NUMALPRF         1
#define VCPINDEX         2
#define ALWB_OFF         1

#define TSCALE           0.1
#define RDBLTH           0
#define RDRNGF           1
#define RDMSNG         256

#define MAX_OVERLAID 100000000000000000.0
#define MAX_ELANG                     7.0

#define RNG_MISSING	-999.0f

/* SZ2 pulse counts and scan rates for PRFs. */
typedef struct {

   int pulse_cnt;

   float scan_rate;

   unsigned short scan_rate_bams;

   unsigned short spare;

} SZ2_rate_t;

SZ2_rate_t SZ2_rate[PRFMAX] = { {0, 0.0, 0, 0},
                                {0, 0.0, 0, 0},
                                {0, 0.0, 0, 0},
                               {64, 14.663, 10667, 0}, 
                               {64, 15.612, 11368, 0},
                               {64, 16.897, 12305, 0},
                               {64, 18.237, 13280, 0},
                               {64, 19.753, 14384, 0} };

/* Function Prototypes. */
static void Write_valid_prfs();
static int Find_start_end_bin( Base_data_header *rad_hdr, int *fgbin, int *lgbin );

/*\///////////////////////////////////////////////////////////////////////////////////////////

   Description:
      Performs initialization for the PRF selection function.  

   Inputs:
      rad_hdr - pointer to radial header.
      outbuf_epwr - buffer to hold echo power lookup table.
      outbuf_pwr_lookup - buffer to hold power lookup table.

///////////////////////////////////////////////////////////////////////////////////////////\*/
int A30531_prf_init( Base_data_header *rad_hdr, void *outbuf_epwr, void *outbuf_pwr_lookup ){

   static int old_delta_pri = 0;
   
   int ur2, ur3, prf, num_alwblprf = 0, num_bins, i;
   float atmos, syscal, noise, bin_res, range_start;

   short *alwblprf = NULL;
   int *unambigr = NULL;

   float *epwr = (float *) outbuf_epwr;
   float *pwr_lookup = (float *) outbuf_pwr_lookup;

   /* Validate PS_delta_pri.  If out of range, return error. */
   if( (PS_delta_pri <= 0) || (PS_delta_pri > DELPRF_MAX) ){

      RPGC_log_msg( GL_INFO, "PS_delta_pri %d Invalid\n", PS_delta_pri );
      return -1;

   }

   /* Initialize sum of overlaid ranges and flag for valid PRF. */
   for( i = Min_PRF; i <= DOP_PRF_END; i++ ){

      Overlaid[i] = 0.0;
      Overlaid_cnt[i] = 0;
      Validprf[i] = 0;

   }
 
   /* Check which PRF's are allowable with the current VCP. */
   if( (alwblprf = ORPGVCP_allowable_prf_ptr( PS_rpgvcpid ) ) == NULL ){

      RPGC_log_msg( GL_INFO, "ORPGVCP_allowable_prf_ptr( %d ) Returned NULL\n",
                    PS_rpgvcpid );
      return(-1);

   }

   num_alwblprf = alwblprf[ NUMALPRF ];
   for( i = 1; i <= num_alwblprf; i++ ){

      prf = alwblprf[i+ALWB_OFF];
      if( (prf >= Min_PRF) && (prf <= DOP_PRF_END) ){

         /* Set flag and save allowable PRF index. */
         Validprf[prf] = 1;

      }

   }

   /* Inform operator of the valid PRFs for this VCP. */
   Write_valid_prfs();

   /* Get SYSCAL and ATMOS from the radial header. */
   noise = rad_hdr->horiz_noise;
   syscal = rad_hdr->calib_const - noise;
   atmos = rad_hdr->atmos_atten*ASCALE;

   /* Get reflectivity bin resolution and range to the start of the 
      first bin from radial header. */
   bin_res = rad_hdr->surv_bin_size*M_TO_KM;
   range_start = rad_hdr->range_beg_surv*M_TO_KM;

   /* Get the number of surveillance bins and compute the maximum 
      processing range. */
   num_bins = rad_hdr->n_surv_bins;
   Max_proc_bin = (int) ((float) MAX_PROC_RNG / bin_res);

   RPGC_log_msg( GL_INFO, "Set-up Parameters for this Volume Scan:\n" );
   RPGC_log_msg( GL_INFO, "--->Bin Res: %f, Start Range: %f, # Bins: %d, Max Processing Bin: %d\n",
                 bin_res, range_start, num_bins, Max_proc_bin );
   RPGC_log_msg( GL_INFO, "--->Noise: %f, Syscal: %f, Atmos: %f\n",
                 noise, syscal, atmos );
 
   /* Compute power lookup tables. */
   for( i = 1; i <= num_bins; i++ ){

      Bin_range[i] = (i - 0.5)*bin_res + range_start;
      pwr_lookup[i] = LOGFAC*log10( (double) Bin_range[i] ) +
                      Bin_range[i]*atmos - syscal;
 
   }
 
   /* Flag bins greater than 460 km with NO POWER flag. */
   for( i = num_bins + 1; i <= EPWR_SIZE; i++ )
      epwr[i] = FLAG_NO_PWR;

   /* Initialize folded bin look-up tables if not already initialized. */
   if( old_delta_pri != PS_delta_pri ){
 
      if( (unambigr = ORPGVCP_unambiguous_range_table_ptr( PS_delta_pri )) == NULL ){

         RPGC_log_msg( GL_INFO, "ORPGVCP_unambiguous_range_table_ptr( %d) Returned NULL\n",
                       PS_delta_pri );
         return(-1);

      }

      /* Set 1st, 2nd, 3rd and 4th trip ranges from unambiguous ranges for
         the valid Doppler PRF's.  Note: we need to subtract 1 from array
         index because arrays are 0 indexed (vice unit indexed). */
      for( prf = Min_PRF; prf <= DOP_PRF_END; prf++ ){

         ur2 = unambigr[prf-1]*2;
         ur3 = unambigr[prf-1]*3;
 
         /* Do For bins 1 to the unambiguous range. */
         for( i = 1; i <= unambigr[prf-1]; i++ ){
 
            /* Set up look-up tables for the locations of the 2nd, 3rd and 4th
               trip bins */
            Folded_bin1[prf][i] = i + unambigr[prf-1];
            Folded_bin2[prf][i] = i + ur2;
            Folded_bin3[prf][i] = i + ur3;

         }
 
         /* Do For bins from 1 + the unambiguous range to 230 km. */
         for( i = unambigr[prf-1]+1; i <= Max_proc_bin; i++ ){
 
            /* Set up look-up tables for the 1st, 3rd and 4th trip bins. */
            Folded_bin1[prf][i] = i - unambigr[prf-1];
            Folded_bin2[prf][i] = i + unambigr[prf-1];
            Folded_bin3[prf][i] = i + ur2;

         }

      }

   }
 
   /* Save current delta pri number. */
   old_delta_pri = PS_delta_pri;

   return 0;

} /* End of A30531_prf_init() */

/*\//////////////////////////////////////////////////////////////////////////////////////////////

   Description:
      Computes the echo overlay along a radial.

   Inputs:
      radial - radial buffer.
      epwr - echo power lookup table.
      pwr_lookup - power lookup table.

//////////////////////////////////////////////////////////////////////////////////////////////\*/
int A30532_echo_overlay( Base_data_header *rad_hdr, unsigned short *radial, float *epwr, 
                         float *pwr_lookup ){

   int i, prf, fgbin, lgbin, begbin, endbin;
   float dBZ, tpwr, tover;

   /* Get the first and last good bins. */
   fgbin = rad_hdr->surv_range;
   lgbin = fgbin + rad_hdr->n_surv_bins - 1;
   if( lgbin >= MAX_REF_SIZE )
      lgbin = MAX_REF_SIZE;

   /* Flag echo powers for bins less than the first good bin. */
   endbin = fgbin - 1;
   for( i = 1; i <= endbin; i++ )
      epwr[i] = FLAG_NO_PWR;
 
   /* Compute echo power from reflectivity using lookup table. */
   for( i = fgbin; i <= lgbin; i++ ){

      if( (radial[i-1] != RDBLTH) 
                    && 
          (radial[i-1] != RDRNGF)
                    &&
          (radial[i-1] != RDMSNG) ){

         dBZ = RPGCS_reflectivity_to_dBZ( radial[i-1] );
         epwr[i] = dBZ + pwr_lookup[i];

      }
      else{
 
         /* No reflectivity data, set echo power to flag value. */
         epwr[i] = FLAG_NO_PWR;

      }

   }
 
   /* Set bins greater than last good bin (to max of 460 km) to flag value. */
   begbin = lgbin + 1;
   for( i = begbin; i <= MAX_REF_SIZE; i++ )
      epwr[i] = FLAG_NO_PWR;
 
   /* Get overlaid threshold from radial header. */
   tover = rad_hdr->vel_tover*TSCALE;

   /* Find the start and end bins along this radial for determining
      the overlay.  The bins to process may be different if storm or 
      celli based PRF selection is turned on. */
   Find_start_end_bin( rad_hdr, &fgbin, &lgbin );

   /* Do For All Doppler PRF indices. */
   for( prf = Min_PRF; prf <= DOP_PRF_END; prf++ ){
 
      /* For valid bins <= 230 km range.  The Folded bin locations are
         retrieved from lookup tables initialized in the main routine. */
      if( Max_proc_bin < lgbin )
         endbin = Max_proc_bin;

      else
         endbin = lgbin;

      for( i = fgbin; i <= endbin; i++ ){
 
          /* If Storm/Cell-based PRF Selection, test if bit is set for this range bin
             on this radial. */
          if( (Local_storm_based_PRF_selection)
                          ||
              (Cell_based_PRF_selection) ){

             if( RPGCS_bit_test( (unsigned char *) Bitmap, i ) <= 0 )
                continue;

         }

         /* Check for valid power. */
         if( epwr[i] != FLAG_NO_PWR ){

            tpwr = epwr[i] - tover;
 
            /* Check second trip overlay. */
            if( tpwr <= epwr[ Folded_bin1[prf][i] ] ){

               Overlaid[prf] += Bin_range[i];
               Overlaid_cnt[prf]++;

            }
 
            /* Check third trip overlay. */
            else if( tpwr <= epwr[ Folded_bin2[prf][i] ] ){

               Overlaid[prf] += Bin_range[i];
               Overlaid_cnt[prf]++;
 
            }
            /* Check fourth trip overlay. */
            else if( tpwr <= epwr[ Folded_bin3[prf][i] ] ){

               Overlaid[prf] += Bin_range[i];
               Overlaid_cnt[prf]++;

            }

         }

      } /* End of "for( i =" loop. */

   } /* End of "for( prf = " loop. */

   return 0;

} /* End of A30532_echo_overlay() */

/*\///////////////////////////////////////////////////////////////////

   Description:
      Modifies the VCP table with the PRF which minimizes the echo
      overlay.
  
   Inputs:
      vcpatnum - VCP number.
      vcp_num_cuts - number of elevation slices in the VCP.
      elev_attr - pointer to the elevation attribute data.

   Outputs:
      disposition - holds the disposition of the modified VCP.  Either
                    the VCP is modified (disposition = FORWARD) or is
                    not modified (disposition = DESTROY).
   
///////////////////////////////////////////////////////////////////\*/
int A30533_change_vcp( int vcpatnum, int vcp_num_cuts, Ele_attr *elev_attr, 
                       int *disposition ){

   int i, rpg_elev_num, prf, pulse_cnt, elcut, newprf, rpgvcpid;
   int newprf_cnt, useprf, usepulsecnt, min_overlaid_cnt, change_rate;
   unsigned short wavetype, phase, rate_bams, userate;
   double min_overlaid;
   float elev_angle_deg;

   /* Find index into rdcvcpta, rdccon, alwblprf, etc. tables for this vcp. */
   rpgvcpid = ORPGVCP_index( vcpatnum );
   if( rpgvcpid < 0 ){
 
      RPGC_log_msg( GL_INFO, "A30533_change_vcp:  ORPGVCP( %d ) Failed: %d\n", 
                    vcpatnum, rpgvcpid );
      *disposition = DESTROY;
      return -1;

   }

   /* Check which PRF's are allowable with the current VCP. */
   for( i = 1; i <= ORPGVCP_get_allowable_prfs( vcpatnum ); i++ ){

      prf = ORPGVCP_get_allowable_prf( vcpatnum, i );
      if( (prf >= Min_PRF) && (prf <= DOP_PRF_END) ){

         /* Set flag for allowable PRF. */
         Validprf[prf] = 1;

      }

   }

   newprf = 0;
   newprf_cnt = 0;
   min_overlaid = MAX_OVERLAID;
   min_overlaid_cnt = 9999999;
 
   /* Find PRF with minimum overlaid ranges. */
   for( i = Min_PRF; i <= DOP_PRF_END; i++ ){

      /* Check if this PRF is valid for the current VCP. */
      if( Validprf[i] ){
 
         RPGC_log_msg( GL_INFO, "Overlaid[%d]: %f\n", i, Overlaid[i] );
         RPGC_log_msg( GL_INFO, "Overlaid_cnt[%d]: %d\n", i, Overlaid_cnt[i] );

         if( Overlaid[i] <= min_overlaid ){
 
            /* New minimum, save values. */
            newprf = i;
            min_overlaid = Overlaid[i];

         }

         if( Overlaid_cnt[i] <= min_overlaid_cnt ){
 
            /* New minimum, save values. */
            newprf_cnt = i;
            min_overlaid_cnt = Overlaid_cnt[i];

         }

      }

   }
 
   RPGC_log_msg( GL_INFO, "Number of Overlapping Bins: %d\n", Set_count );

   /* Check if there are any valid PRFs. */
   if( newprf == 0 ){

      /* Do nothing ........ */
      *disposition = DESTROY;

      RPGC_log_msg( GL_INFO, "PRF Selection VCP Disposition <- DESTROY Because:\n" );
      RPGC_log_msg( GL_INFO, "--->No Valid PRFs for VCP (%d)\n", vcpatnum );

      return -1;

   }
 
   RPGC_log_msg( GL_INFO, "--->Minimum Overlay Area %f With PRF %d\n",
                 min_overlaid, newprf );
   RPGC_log_msg( GL_INFO, "--->Minimum Overlay Bin Count %d With PRF %d\n",
                 min_overlaid_cnt, newprf_cnt );

   /* If using Storm/Cell-based PRF Selection, use the PRF with minumum bin count.  Otherwise
      use PRF with minimum area. */
   useprf = newprf;
   if( (Local_storm_based_PRF_selection)
                  ||
       (Cell_based_PRF_selection) )
      useprf = newprf_cnt;

   /* Insert new PRF into current VCP. */
   *disposition = FORWARD;
   elcut = 1;

   elev_angle_deg = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                         elev_attr->ele_angle );

   /* Do For All elevation cuts in VCP that are below 7 degrees. */
   while(1){

      /* Determin wave type.  Insert new PRF only if Doppler wavetype. */
      wavetype = elev_attr->wave_type;
      phase = elev_attr->phase;
      if( ((wavetype == VCP_WAVEFORM_CD) 
                     || 
           (wavetype == VCP_WAVEFORM_CDBATCH) 
                     || 
           (wavetype == VCP_WAVEFORM_BATCH)) ){

         /* Get the RPG elevation number corresponding to the RDA elevation cut. */
         rpg_elev_num  = ORPGVCP_get_rpg_elevation_num( vcpatnum, elcut-1 ) - 1;

         /* Get the default PRF and pulse counts for this elevation cut. */
         prf = ORPGVCP_get_allowable_prf_default( vcpatnum, rpg_elev_num );
         pulse_cnt = ORPGVCP_get_allowable_prf_pulse_count( vcpatnum, rpg_elev_num, prf );

         /* The scan rate is based on SZ2.  If not an SZ2 cut, don't change the rate. */
         rate_bams = SZ2_rate[useprf-1].scan_rate_bams;
         change_rate = 0;

         /* Is this an SZ2 cut? */
         if( phase != VCP_PHASE_SZ2 ) /* No. */
            usepulsecnt = ORPGVCP_get_allowable_prf_pulse_count( vcpatnum,
                                                                 rpg_elev_num,
                                                                 useprf );

         else{ /* Yes. */

            /* Get pulse counts and scan rate from lookup table. */
            usepulsecnt = SZ2_rate[useprf-1].pulse_cnt;
            userate = SZ2_rate[useprf-1].scan_rate_bams;
            change_rate = 1;

            /* This should not happen, but .... */
            if( usepulsecnt == 0 ){

               /* Lookup table has 0 pulse counts.  Set PRF, pulse counts and rate
                  to default. */
               usepulsecnt = pulse_cnt;
               useprf = prf;
               userate = rate_bams;
               change_rate = 0;

            }

         }

         /* If this is an SZ2 cut, all PRF sectors must be the same. */
         if( change_rate ){

            elev_attr->dop_prf_num_1 = elev_attr->dop_prf_num_2 = elev_attr->dop_prf_num_3 = useprf;
            elev_attr->pulse_cnt_1 = elev_attr->pulse_cnt_2 = elev_attr->pulse_cnt_3 = usepulsecnt;
            elev_attr->azi_rate = userate;

         }
         else{ 

            /* Insert new PRF and pulse count into this elevation cut only if the pulse
               count is not zero. */
            if( elev_attr->pulse_cnt_1 != 0 ){

               elev_attr->dop_prf_num_1 = useprf;
               elev_attr->pulse_cnt_1 = ORPGVCP_get_allowable_prf_pulse_count( vcpatnum,
                                                                            rpg_elev_num,
                                                                            useprf );
            }
            else{

               /* Pulse count is 0.  Set the default PRF and pulse counts. */
               elev_attr->dop_prf_num_1 = prf;
               elev_attr->pulse_cnt_1 = pulse_cnt;

            }
 
            /* Second azimuth sector. */
            if( elev_attr->pulse_cnt_2 != 0 ){

               elev_attr->dop_prf_num_2 = useprf;
               elev_attr->pulse_cnt_2 = ORPGVCP_get_allowable_prf_pulse_count( vcpatnum,
                                                                               rpg_elev_num,
                                                                               useprf );
            }
            else{

               /* Pulse count is 0.  Set the default PRF and pulse counts. */
               elev_attr->dop_prf_num_2 = prf;
               elev_attr->pulse_cnt_2 = pulse_cnt;

            }

            /** Third azimuth sector. */
            if( elev_attr->pulse_cnt_3 != 0 ){

               elev_attr->dop_prf_num_3 = useprf;
               elev_attr->pulse_cnt_3 = ORPGVCP_get_allowable_prf_pulse_count( vcpatnum,
                                                                               rpg_elev_num,
                                                                               useprf );
            }
            else{

               /* Pulse count is 0.  Set the default PRF and pulse counts. */
               elev_attr->dop_prf_num_3 = prf;
               elev_attr->pulse_cnt_3 = pulse_cnt;

            }

         }

         RPGC_log_msg( GL_INFO, "PRF Sector Settings for Elevation Angle %7.2f (%d)\n",
                       elev_angle_deg, elcut );
         RPGC_log_msg( GL_INFO, "--->Sector 1 - PRF: %d, Pulse Count: %d, Scan Rate: %f\n",
                       elev_attr->dop_prf_num_1, elev_attr->pulse_cnt_1,
                       ORPGVCP_get_azimuth_rate( vcpatnum, elcut- 1 ) );
         RPGC_log_msg( GL_INFO, "--->Sector 2 - PRF: %d, Pulse Count: %d, Scan Rate: %f\n",
                       elev_attr->dop_prf_num_2, elev_attr->pulse_cnt_2,
                       ORPGVCP_get_azimuth_rate( vcpatnum, elcut - 1 ) );
         RPGC_log_msg( GL_INFO, "--->Sector 3 - PRF: %d, Pulse Count: %d, Scan Rate: %f\n",
                       elev_attr->dop_prf_num_3, elev_attr->pulse_cnt_3,
                       ORPGVCP_get_azimuth_rate( vcpatnum, elcut - 1 ) );

      }
 
      /* Increment elevation cut number. */
      elcut++; 
      elev_attr++;

      /* Simulate Do For All Elevations below 7 degrees with while(1) loop. */
      if( elcut <= vcp_num_cuts ){

         elev_angle_deg = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                               elev_attr->ele_angle );
         if( elev_angle_deg < MAX_ELANG )
            continue;

      }

      break;

   }

   return 0;

} /* End of A30533_change_vcp( ) */

/*\////////////////////////////////////////////////////////////////////////

   Description:
      Writes valid PRFs for this VCP to task log file.   Used for debugging.

////////////////////////////////////////////////////////////////////////\*/
static void Write_valid_prfs(){

   short *rdcvcpta = NULL;
   int prf;
   char *prf_str = NULL, *text_str = NULL;

   if( (rdcvcpta = ORPGVCP_ptr( PS_rpgvcpid )) == NULL ){

      RPGC_log_msg( GL_INFO, "ORPGVCP_ptr( %d) Returned NULL\n",
                    PS_rpgvcpid );
      return;

   }

   RPGC_log_msg( GL_INFO, "The Following PRFs Allowed for VCP %d\n", rdcvcpta[VCPINDEX] );
   prf_str = calloc( 1, 80 );
   text_str = calloc( 1, 80 );

   if( (prf_str != NULL) && (text_str != NULL) ){

      for( prf = Min_PRF; prf <= DOP_PRF_END; prf++ ){

         if( Validprf[prf] ){

            sprintf( prf_str, " %1d", prf );
            strcat( text_str, prf_str );

         }

      }

      RPGC_log_msg( GL_INFO, "--->%s", text_str );

   }

   if( prf_str != NULL )
      free( prf_str );

   if( text_str != NULL )
      free( text_str );

} /* End of Write_valid_prfs() */

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Determines the start and end bin to process on this radial.  For
      cell tracking, the bins are determined by the cell centroid and the
      20 km circle around the cell. If cell tracking is disabled, then
      the start and end bins are based on the start of good data on the 
      radial and the end of good data on the radial.

   Inputs:
      rad_hdr - pointer to radial header.

   Outputs:
      fgbin - first bin to process on radial.
      lgbin - last bin to process on a radial.

/////////////////////////////////////////////////////////////////////////\*/
static int Find_start_end_bin( Base_data_header *rad_hdr, int *fgbin, int *lgbin ){

   int fg, lg;

   /* Get the first and last good bins. */
   fg = rad_hdr->surv_range;
   lg = fg + rad_hdr->n_surv_bins - 1;
   if( lg >= MAX_REF_SIZE )
      lg = MAX_REF_SIZE;

   if( Local_storm_based_PRF_selection )
      ST_start_end_bin( rad_hdr, &fg, &lg );

   if( Cell_based_PRF_selection )
      CT_start_end_bin( rad_hdr, &fg, &lg );

   *fgbin = fg;
   *lgbin = lg;

   return 0;

} /* End of Find_start_end_bin() */
