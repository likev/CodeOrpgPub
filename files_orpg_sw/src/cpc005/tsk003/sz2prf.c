/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:51:09 $
 * $Id: sz2prf.c,v 1.5 2014/03/13 19:51:09 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
#define SZ2PRF_C
#include <prfselect.h>
#include <math.h>

#define ASCALE           	0.001
#define LOGFAC         		-20.0
#define NOISE_FACTOR		0.7
#define MAX_DBZ			95.0

#define NUMALPRF         	1
#define VCPINDEX         	2
#define ALWB_OFF         	1

#define TSCALE           	0.1
#define RDBLTH           	0
#define RDRNGF           	1
#define RDMSNG         		256

#define MAX_OVERLAID 		100000000000000000.0
#define MAX_ELANG               7.0

#define RNG_MISSING		-999.0f

/* Macros Defining Which Map to Read. */
#define READ_BYPASS_MAP		1	
#define READ_CLUTTER_MAP	2	

/* Clutter Map Operator Select Codes. */
#define FORCE_BYPASS            0
#define BYPASS_MAP              1
#define FORCE_FILTER            2

/* Maximum Number of Clutter Map Zones. */
#define MAXZONES		20

#define STRONG_TRIP_THRESHOLD	-3.0f
#define WEAK_TRIP_THRESHOLD2	45.0f

/* Type Definitions. */
typedef struct Bin {

   float power;			/* Signal power, in dB. */

   double spower;		/* Signal power. */

   int trip;

} Bin_t;

typedef struct P_array {

   Bin_t bin[4];

} P_array_t;

/* Static Variables. */
static unsigned char Clutter[ORDA_BYPASS_MAP_RADIALS][MAX_BASEDATA_REF_SIZE];

static ORDA_clutter_map_msg_t *Clutter_map = NULL;
static int Clutter_map_avail = 0;

static ORDA_bypass_map_msg_t *Bypass_map = NULL;
static int Bypass_map_avail = 0;

/* Function Prototypes. */
static int Find_start_end_bin( Base_data_header *rad_hdr, int *fgbin, int *lgbin );
static int Read_map( int map, int *avail );
static unsigned char*  Get_clutter( Base_data_header *rad_hdr );
static int Comp_elements( const void *array_1, const void *array_2 );

/*\/////////////////////////////////////////////////////////////////////////////////////////

   Description:
      Reads the clutter map and bypass map.  Builds a 2-dimensional clutter table. 
      This table sets a 1 if 0.25 km X 1 deg cell was clutter filtered, or 0 otherwise. 

/////////////////////////////////////////////////////////////////////////////////////////\*/
int SZ2_read_clutter(){

   int i, clm_ptr, status_cm = -1;
   short *clm_map = NULL;

   /* Read the Clutter Map and Bypass Map. */
   status_cm = Read_map( READ_CLUTTER_MAP, &Clutter_map_avail );

   if( status_cm >= 0 )
      Read_map( READ_BYPASS_MAP, &Bypass_map_avail );

   /* Initialize all bins to No Clutter. */
   memset( Clutter, 0, ORDA_BYPASS_MAP_RADIALS*BASEDATA_REF_SIZE ); 

   /* If maps read, fill Clutter array.  Note:  A value set means clutter,
      a value not set means no cluttter.  Also we are assuming that the
      first elevation segment is the segment that includes the lowest
      elevation cut of the VCP. */
   clm_ptr = 0;
   if( Clutter_map_avail && Bypass_map_avail ){

      ORDA_clutter_map_segment_t *cm_segment = 
                                 (ORDA_clutter_map_segment_t *) &Clutter_map->map.data[clm_ptr];
      ORDA_bypass_map_segment_t *bm_segment = 
                                (ORDA_bypass_map_segment_t *) &Bypass_map->bypass_map.segment[0];
      int op_code, range_end, zone, loop_start, loop_end, num_zones = 0;
      int range_index, bitset;

      /* Since the clutter map is variable length, we can not use the data structure
         in orda_clutter_map.h but rather treat the data as an array of short. */
      clm_map = (short *) cm_segment;

      for( i = 0; i < ORDA_BYPASS_MAP_RADIALS; i++ ){

         /* Determine the number of zones for this azimuth segment */
         num_zones = cm_segment->num_zones;

         if( (num_zones < 1) || (num_zones > MAXZONES) ){

            RPGC_log_msg( GL_INFO, "Number of Clutter Censor Zones Invalid (%d)\n",
                          num_zones );

            /* Initialize all bins to No Clutter. */
            memset( Clutter, 0, ORDA_BYPASS_MAP_RADIALS*BASEDATA_REF_SIZE ); 

            /* Return error. */
            return -1;

         }

         /* Process the CENSOR ZONES for this azimuth radial and elevation 
            segment. */
         loop_start = 0;
         loop_end = BASEDATA_REF_SIZE;
         for( zone = 0; zone < num_zones; ++zone ){

            /* Set check for LOOP_START >= MAX_RANGE. */
            if( loop_start < BASEDATA_REF_SIZE ){

               op_code = cm_segment->filter[zone].op_code;
               range_end = cm_segment->filter[zone].range;

               /* Calculate the end range for this range zone. */
               if( range_end < BASEDATA_REF_SIZE )
                  loop_end = range_end;

               else
                  loop_end = BASEDATA_REF_SIZE;

               /* Check the operator select code.  Product data level depends 
                  on value. */
               if( op_code == FORCE_BYPASS ){

                  /* Operator select code is force bypass of filter. */
                  for( range_index = loop_start; range_index < loop_end; ++range_index )
                     Clutter[i][range_index] = (unsigned char) 0;

               }
               else if( op_code == BYPASS_MAP ){

                  /* Operator select code is Bypass Map in control of filtering. */
                  for( range_index = loop_start; range_index < loop_end; ++range_index ){

                     /* Check if the Bypass Map bit is set. */
                     bitset = RPGCS_bit_test_short( (unsigned char *) &bm_segment->data[i][0],
                                                    range_index );
                     if( !bitset) {

                        /* Bit is not set.  Filter in control. */
                        Clutter[i][range_index] = (unsigned char) 1;

                     }
                     else{

                        /* Bit is set. Bypass filter. */
                        Clutter[i][range_index] = (unsigned char) 0;

                     }

                  }

               }
               else if( op_code == FORCE_FILTER ){

                  /* Operator select code is force clutter filtering. */
                  for( range_index = loop_start; range_index < loop_end; ++range_index )
                     Clutter[i][range_index] = (unsigned char) 1;

               }
               else{

                  RPGC_log_msg( GL_INFO, "Unrecognized Op Code: %d\n", op_code );
                  return -1;

               }

               /* Set loop start. */
               loop_start = loop_end;

            }

         }

         /* Adjust clm_ptr by the size of this azimuth segment.  Prepare
            for the next segment. */
         clm_ptr += (num_zones * sizeof(ORDA_clutter_map_filter_t)/sizeof(short)) + 1;
         cm_segment = (ORDA_clutter_map_segment_t *) &clm_map[clm_ptr];
         
      }

   }
   
   /* Return normal. */
   return 0;

/* End of SZ2_read_clutter(). */
}

/*\/////////////////////////////////////////////////////////////////////////////////////////

   Description:
      Returns clutter flags radial for given azimuth.   

/////////////////////////////////////////////////////////////////////////////////////////\*/
static unsigned char*  Get_clutter( Base_data_header *rad_hdr ){

   int i_azimuth;
   float azimuth = rad_hdr->azimuth;
   
   /* Truncate azimuth to nearest whole degree. */
   i_azimuth = (int) azimuth;

   return( &Clutter[i_azimuth][0] );

/* End of Get_clutter() */
}


/*\/////////////////////////////////////////////////////////////////////////////////////////

   Description:
      Computes the echo overlay along a radial.

   Inputs:
      radial - radial buffer.
      epwr - echo power lookup table.
      pwr_lookup - power lookup table.

   Notes:
      SZ2 can only "recover" the second strongest return.

/////////////////////////////////////////////////////////////////////////////////////////\*/
int SZ2_echo_overlay( Base_data_header *rad_hdr, unsigned short *radial, float *epwr, 
                      float *pwr_lookup ){

   int index, i, k, ur, prf, fgbin, lgbin, begbin, endbin, *unambrng = NULL;
   int num_bins, clutter_filtered_s = 0, clutter_filtered_w = 0;
   double dBZ, tpwr, noise, noisep, below_snr_p;
   unsigned char *clutter = NULL;

   static P_array_t p_array[MAX_REF_SIZE];
   static int num_array[MAX_REF_SIZE];
   static char range_folded[DOP_PRF_END+1][MAX_REF_SIZE];
   static double spwr[MAX_REF_SIZE];
   static float l_overlaid[DOP_PRF_END+1];
   static float l_overlaid_cnt[DOP_PRF_END+1];


   /* Initialize the local versions of overlaid and overlaid_cnt
      and initialize the range-folded array. */
   memset( &l_overlaid[0], 0, sizeof(float)*(DOP_PRF_END+1) );
   memset( &l_overlaid_cnt[0], 0, sizeof(int)*(DOP_PRF_END+1) );

   for( i = 0; i <= DOP_PRF_END; i++ )
      memset( &range_folded[i][0], 0, sizeof(char)*MAX_REF_SIZE );

   /* Get the first and last good bins. */
   fgbin = rad_hdr->surv_range;
   lgbin = fgbin + rad_hdr->n_surv_bins - 1;
   if( lgbin >= MAX_REF_SIZE )
      lgbin = MAX_REF_SIZE;

   /* Flag echo powers for bins less than the first good bin. */
   endbin = fgbin - 1;
   for( i = 1; i <= endbin; i++ ){

      epwr[i] = FLAG_NO_PWR;
      spwr[i] = 0;

   }
 
   /* Compute echo power from reflectivity using lookup table. */
   for( i = fgbin; i <= lgbin; i++ ){

      if( (radial[i-1] != RDBLTH) 
                    && 
          (radial[i-1] != RDRNGF)
                    &&
          (radial[i-1] != RDMSNG) ){

         dBZ = RPGCS_reflectivity_to_dBZ( radial[i-1] );
         epwr[i] = dBZ + pwr_lookup[i];
         tpwr = (double) epwr[i] / 10.0;
         spwr[i] = pow( 10.0, tpwr );

      }
      else{
 
         /* No reflectivity data, set echo power to flag value. */
         epwr[i] = FLAG_NO_PWR;
         spwr[i] = 0;

      }

   }

   /* Determine the number of bins at the beginning of the radial that 
      need to be set .... the first 2 km are set at the RDA to below 
      SNR threshold.   We wish these to be set to a very high reflectivity
      value to force out of trip returns to be obscurred. */
   num_bins = 2000/rad_hdr->surv_bin_size;
   if( num_bins <= lgbin ){

      double p = MAX_DBZ + (double) pwr_lookup[0];
      double sp = pow( 10.0, p/10.0 );

      for( i = 0; i < num_bins; i++ ){

         epwr[i] = p;
         spwr[i] = sp;

      }

   }

 
   /* Set bins greater than last good bin (to max of 460 km) to flag value. */
   begbin = lgbin + 1;
   for( i = begbin; i <= MAX_REF_SIZE; i++ ){

      epwr[i] = FLAG_NO_PWR;
      spwr[i] = 0;

   }
 
   /* Get noise value from radial header. */
   noise = rad_hdr->horiz_noise;
   tpwr = (double) noise / 10.0;
   noisep = pow( 10.0, tpwr );

   /* Assume below SNR values have power equal to 0.7*10^((noise + Z_snr)/10) */
   below_snr_p = NOISE_FACTOR*pow( 10.0, (noise + Z_snr)/10.0 );

   /* Initialize the number of valid bins array. */
   memset( &num_array[0], 0, sizeof(int)*MAX_REF_SIZE );

   /* Initialize the power/trip array. */
   memset( &p_array[0], 0, sizeof(P_array_t)*MAX_REF_SIZE );

   /* Find the start and end bins along this radial for determining
      the overlay.  The bins to process may be different if storm or 
      cell based PRF selection is turned on. */
   Find_start_end_bin( rad_hdr, &fgbin, &lgbin );

   /* Get the closest clutter radial to this radial's azimuth. */
   clutter = Get_clutter( rad_hdr );
   
   unambrng = ORPGVCP_unambiguous_range_table_ptr( PS_delta_pri );
   if( unambrng == NULL )
      exit(0);

   /* Do For All Doppler PRF indices. */
   for( prf = Min_PRF; prf <= DOP_PRF_END; prf++ ){

      int trips, tripw, rngs, rngw, j;
      float ps, pw;
      double pwp = 0.0;

      /* Get the unambiguous range. */
      ur = unambrng[prf-1];

      /* Do For All bins in the Doppler 1st trip. */
      for( i = fgbin; i < ur; i ++ ){

         P_array_t *p = &p_array[i];
         
         /* Initialize the bin index. */
         index = 0;

         p->bin[0].power = epwr[i];
         p->bin[0].spower = spwr[i];
         p->bin[0].trip = 0;
         num_array[i] = 1;

         /* Range index for second trip. */
         k = i + ur;

         /* Increment the bin index. */
         index++;

         /* Add second trip. */
         if( (k < lgbin) && (epwr[k] != FLAG_NO_PWR) ){

            p->bin[index].power = epwr[k];
            p->bin[index].spower = spwr[k];
            p->bin[index].trip = 1;
            num_array[i]++;

            /* Increment the bin index. */
            index++;

         }

         /* Range index for third trip. */
         k += ur;

         /* Add third trip. */
         if( (k < lgbin) && (epwr[k] != FLAG_NO_PWR) ){

            p->bin[index].power = epwr[k];
            p->bin[index].spower = spwr[k];
            p->bin[index].trip = 2;
            num_array[i]++;

            /* Increment the bin index. */
            index++;

         }

         /* Range index for fourth trip. */
         k += ur;

         /* Add fourth trip. */
         if( (k < lgbin) && (epwr[k] != FLAG_NO_PWR) ){

            p->bin[index].power = epwr[k];
            p->bin[index].spower = spwr[k];
            p->bin[index].trip = 3;
            num_array[i]++;

         }

         /* Sort the values. */
         qsort( &p->bin[0].power, num_array[i], sizeof(Bin_t), Comp_elements );

      }

      /* For valid bins <= 230 km range.  The Folded bin locations are
         retrieved from lookup tables initialized in the main routine. */
      if( Max_proc_bin < lgbin )
         endbin = Max_proc_bin;

      else
         endbin = lgbin;

      /* Process data out to the unambiguous range.  The loop need not go beyond
         230 since all the trips are potentially processed at once. */
      for( i = fgbin; i < ur; i++ ){
 
         P_array_t *p = &p_array[i];

         /* Get information on strongest trip, Ps. */
         j = num_array[i]-1;
         ps = p->bin[j].power;
         trips = p->bin[j].trip;
         rngs = i + ur*trips;

         /* This should not be possible .... */
         if( rngs >= MAX_RNG ){

            RPGC_log_msg( GL_INFO, "Strongest Power Outside %d Range????\n",
                          MAX_RNG );

            continue;

         }

         /* Set the clutter filtered flags. */
         clutter_filtered_s = clutter[rngs];
         clutter_filtered_w = 0;

         /* Check if there is a weak trip (Pw).  If so and strong 
            trip (Ps) was clutter filtered, mark Pw as overlaid. */
         if( clutter_filtered_s ){

            k = 2;
            while( k <= 4 ){

               /* Check if any more trips. */
               j = num_array[i]-k;
               if( j < 0 )
                  break;

               /* Get information on weaker trips, Pw. */
               rngw = i + ur*p->bin[j].trip;

               if( rngw < Max_proc_bin ){

                  l_overlaid[prf] += Bin_range[rngw];
                  l_overlaid_cnt[prf]++;

                  range_folded[prf][rngw] = 1;

               }

               k++;

            } /* End of while() loop. */
        
         }
         else{

            /* Strongest trip was not clutter filtered.  The second strongest
               return is at index num_array[i] - 2.  Determine the ratio:
               ps /(pw + p3 + p4 + noise).  Note:  If p3 or p4 are in the
               first trip but are below Z SNR threshold, assume power is:
               p = NOISE_FACTOR*10^((noise + Z_snr)/10.0). */
            k = 2;
            tpwr = 0.0;
            tripw = 0;
            while( k <= 4 ){

               /* Check if any more trips to check. */
               j = num_array[i]-k;
               if( j < 0 )
                  break;

               tpwr += p->bin[j].spower;

               /* There is a strong enough weak signal pw.  Save its trip number. */
               if( k == 2 )
                  tripw = p->bin[j].trip;

               /* Prepare for next trip. */
               k++;

            } /* End of while() loop. */

            /* If the strongest trip is not the first trip and there is a weak trip
               signal also not in the first trip, assume the first trip signal is 
               equal to below_snr .... */
            if( (trips != 0) && (tripw != 0) )
               tpwr += below_snr_p;

            /* If there is a weak signal, then .... */
            if( tpwr > 0.0 ){

               /* Add noise, then convert back to dB (tpwr is Pw + P3 + P4 + Noise). */
               tpwr += noisep;
               pw = 10.0*log10( tpwr );

               /* Compare ratio ps/(pw+noise) >= threshold to declare ps good. */
               if( ((ps - pw) < STRONG_TRIP_THRESHOLD)
                              &&
                   (rngs < Max_proc_bin) ){

                  l_overlaid[prf] += Bin_range[rngs];
                  l_overlaid_cnt[prf]++;

                  range_folded[prf][rngs] = 1;

                  /* If ps flagged as overlaid, all weaker trips need
                     to be flagged as well. */
                  k = 2;
                  while( k <= 4 ){

                     /* Check if any more trips to check. */
                     j = num_array[i]-k;
                     if( j < 0 )
                        break;

                     rngw = i + ur*p->bin[j].trip;
                     if( (rngw < Max_proc_bin) && (p->bin[j].power != FLAG_NO_PWR) )
                        range_folded[prf][rngw] = 1;

                     k++;

                  }


               }

            }

         }
               
         /* Do For all weaker trips. */
         k = 2;
         while ( k <= 4 ){


            /* Check if any more trips to check. */
            j = num_array[i]-k;
            if( j < 0 )
               break;

            pw = p_array[i].bin[j].power;
            rngw = i + ur*p_array[i].bin[j].trip;
            if( (rngw < Max_proc_bin)
                         &&
                (pw != FLAG_NO_PWR) ){

               /* Valid reflectivity. */
               pwp = p_array[i].bin[j].spower;

               /* If the second strongest return was clutter filtered,
                  signal cannot be recovered nor can any weak returns. */
               if( (k == 2) && clutter[rngw] )
                  clutter_filtered_w = 1;

               /* If weaker trip clutter filtered, mark as overlaid. */
               if( clutter_filtered_s || clutter_filtered_w ){

                  if( rngw < Max_proc_bin ){

                     l_overlaid[prf] += Bin_range[rngw];
                     l_overlaid_cnt[prf]++;

                     range_folded[prf][rngw] = 1;

                  }

               }
                  
               /* Compare ratio pw/noise >= VCP velocity threshold to declare pw good. */
               else if( k == 2 ){

                  if( (pw - noise) < V_snr ){

                     if( rngw < Max_proc_bin ){

                        l_overlaid[prf] += Bin_range[rngw];
                        l_overlaid_cnt[prf]++;

                        range_folded[prf][rngw] = 1;

                     }

                  }

                  /* Compare ratio ps/(pw+noise) >= threshold to declare pw good. */
                  else{

                     /* Compute Pw + Noise, in dB. */
                     tpwr = pwp + noisep;
                     pw = 10.0*log10( tpwr );

                     if( (ps - pw) > WEAK_TRIP_THRESHOLD2 ){

                        if( rngw < Max_proc_bin ){

                           l_overlaid[prf] += Bin_range[rngw];
                           l_overlaid_cnt[prf]++;

                           range_folded[prf][rngw] = 1;

                        }

                     }

                  }

               }               
               else{

                  /* All other trips other than ps and pw are marked overlaid. */
                  if( rngw < Max_proc_bin ){

                     l_overlaid[prf] += Bin_range[rngw];
                     l_overlaid_cnt[prf]++;

                     range_folded[prf][rngw] = 1;

                  }

               }

            }

            /* Prepare for the next trip. */
            k++;

         } /* End of while() loop. */

      } /* End of "for( i = " loop. */

   } /* End of "for( prf = " loop. */

   /* Additional processing for Storm and Cell-based PRF 
      selection.*/
   if( (!Local_storm_based_PRF_selection)
                   &&
       (!Cell_based_PRF_selection) ){

      /* Do For All Doppler PRF indices. */
      for( prf = Min_PRF; prf <= DOP_PRF_END; prf++ ){

         /* Transfer local information to global arrays. */
         Overlaid[prf] += l_overlaid[prf];
         Overlaid_cnt[prf] += l_overlaid_cnt[prf];

      }

      /* Nothing more to do ... return to caller. */
      return 0;

   }

   /* Storm or Cell-based PRF is active.   More to do. */
   if( Local_storm_based_PRF_selection )
      ST_start_end_bin( rad_hdr, &fgbin, &lgbin );

   else if( Cell_based_PRF_selection )
      CT_start_end_bin( rad_hdr, &fgbin, &lgbin );

   /* If Storm/Cell-based PRF Selection, then ..... test if bit is set for this range bin
      on this radial. */
   for( prf = Min_PRF; prf <= DOP_PRF_END; prf++ ){

      /* If Storm/Cell-based PRF Selection, test if bit is set for this range bin
         on this radial. */
      for( i = fgbin; i <= lgbin; i++ ){

         if( RPGCS_bit_test( (unsigned char *) Bitmap, i ) <= 0 )
            continue;

         /* Bit is set.  Test if range folded for this PRF and bin. */ 
         if( range_folded[prf][i] ){

            /* Update the Overlaid and Overlaid_cnt arrays. */
            Overlaid[prf] += Bin_range[i];
            Overlaid_cnt[prf]++;

         }

      }

   }

   return 0;

} /* End of SZ2_echo_overlay() */

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

/*\////////////////////////////////////////////////////////////////////////

   Description:
      This module reads the specified clutter map defined by "type" from 
      the Clutter Map Linear Buffer. 

   Inputs:
      type - Data type(READ_BYPASS_MAP or READ_CLUTTER_MAP).

   Outputs:
      avail - Pointer to flag indicating availability.

   Returns:
      Status of the LB read operation.

////////////////////////////////////////////////////////////////////////\*/
static int Read_map( int type, int *avail ){

   int config, msg_id, data_id, ret_size;
   char *buf = NULL;
   short *map = NULL;

   /* Determine if RDA is legacy or ORDA */
   config = ORPGRDA_get_rda_config( NULL );
   if( config != ORPGRDA_ORDA_CONFIG ){

      RPGC_log_msg( GL_INFO, "RDA Configuration Not ORDA.\n");
      return -1;

   }

   data_id = ORPGDAT_CLUTTERMAP;
   msg_id = -1;

   /* Set the flag that the data is not available. */
   *avail = 0;

   /* Set the message ID for the data we want to read. */
   if( type == READ_BYPASS_MAP ){

      if( Bypass_map != NULL ){
      
         free( Bypass_map );
         Bypass_map = NULL;

      }

      msg_id = LBID_BYPASSMAP_ORDA;

   }
   else if( type == READ_CLUTTER_MAP ){

      if( Clutter_map != NULL ){
      
         free( Clutter_map );
         Clutter_map = NULL;

      }

      msg_id = LBID_CLUTTERMAP_ORDA;

   }

   /* Read in data from LB. */
   ret_size = RPGC_data_access_read( data_id, (void *) &buf, LB_ALLOC_BUF, msg_id );
   if( ret_size > 0 ){

      map = (short *) buf;

      /* Validate the date/time field.  If date is <= 1, the map 
         is not valid. */
      if( map[0] <= 1 )
         LE_send_msg( GL_INFO, "Map date/time Invalid\n" );

      else
         *avail = 1;

   }
   else{

      /* Bad return value from ORPGDA_read. */
      RPGC_log_msg( GL_INFO | GL_ERROR, "RDA Clutter/Bypass Map Read Failed (%d)\n", ret_size );
      return -1;

   }

   if( type == READ_BYPASS_MAP )
      Bypass_map = (ORDA_bypass_map_msg_t *) buf;

   else if (type == READ_CLUTTER_MAP )
      Clutter_map = (ORDA_clutter_map_msg_t *) buf;

   /* Return normal. */
   return 0;

} /* End of Read_map() */

/*\////////////////////////////////////////////////////////////////////

   Description:
      This module provides the comparision function for 
      qsort.   See man page for qsort for more information
      on function behavior.

///////////////////////////////////////////////////////////////////\*/
static int Comp_elements( const void *array_1, const void *array_2 ){

   Bin_t *e1 = (Bin_t *) array_1, *e2 = (Bin_t *) array_2;

   if( e1->power < e2->power ) 
      return -1;

   else if( e1->power > e2->power ) 
      return 1;

   return 0;

/* End of comp_elements() */
}
