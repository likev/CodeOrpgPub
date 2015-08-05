
/********************************************************************

    This module is for task prfbmap.

********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/13 20:17:52 $
 * $Id: prfbmap_prod_gen.c,v 1.3 2012/09/13 20:17:52 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#define PRFBMAP_PROD_GEN_C
#include <prf_bitmap.h>

/* Macro definitions. */
#define ASCALE				0.001f
#define TSCALE				0.1f
#define LOGFAC				-20.0f
#define MAXB230				229
#define BITS_PER_BYTE       		8
	
/* Static Function Prototypes. */
static int Prf_init( char *bdataptr, void *outbuf_epwr, void *outbuf_pwr_lookup );
static void Build_bit_map( int start_angle, int delta_ang, int start_bin, 
                           int end_bin, void *outbuf );
static void End_of_product_processing( int vol_time, int vcp_num, int vol_num,
                                       short vol_date, int elev_ang,
                                       void *outbuf  );
static void Echo_overlay( char *bdataptr, float *epwr, 
                          float *pwr_lookup );
static void Get_bit( unsigned int bin, int *index, int *bit );

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Product Generation Control routine for PRF product program.

///////////////////////////////////////////////////////////////////////\*/
void A30549_product_generation_control( char *bufptr, char *bdataptr, 
                                        char *scrptr ){

    /* ********** L o c a l   d e c l a r a t i o n s ************** */
    int elevindex, end_rf, start, delta, rfspt, rf230end;
    Base_data_header *radhead =  (Base_data_header *) bdataptr;
    Scan_Summary *scansum = NULL;

    static float elmeas = 0.0f;
    static int elang = 0, voltime = 0, voldate = 0;
    static int volnumber, vcpnumber;

    /* ********* E x e c u t a b l e   c o d e ********************* */
    /* Beginning of product initialization. */
    if( (radhead->status == GOODBVOL) || (radhead->status == GOODBEL) ){

        /* Radial count Initialization: */
        Radcount = 0;

        /* Get volume number and vcp number. */
        volnumber = RPGC_get_buffer_vol_num( bdataptr );
        scansum = RPGC_get_scan_summary( volnumber );
        vcpnumber = scansum->vcp_number;

        /* Milli-seconds after midnight for the volume scan start   
           time of this product, Julian date. */
        voltime = scansum->volume_start_time;
        voldate = scansum->volume_start_date;

        /* Get elevation angle (in degrees *10) from VCP number and    
           elevation index. */
        elevindex = radhead->elev_num - 1;
        elmeas = RPGCS_get_elevation_angle( vcpnumber, elevindex );
        elang = elmeas * 10.0f;

        /* Determine delta_pri number from unambiguous range. */
        PS_delta_pri = ORPGVCP_get_delta_pri();

        /* Initialize data for PRFs. */
        if( Prf_init( bdataptr, (void *) scrptr, 
                      (void *) (scrptr + (EPWR_SIZE*sizeof(float))) ) < 0 ){

            RPGC_log_msg( GL_INFO, "Aborting Because: Prf_init Failed\n" ) ;
            return;

        }

    }

    /* Perform individual radial processing. */

    /* Retrieve start angle and delta angle measurements from the    
       input radial buffer for this radial. */
    start = radhead->start_angle;
    delta = radhead->delta_angle;

    /* Increment the radial count: */
    Radcount++;

    /* Initialize the pointers to the beginning and to the end of the   
       good bins of data in the input radial buffer. */
    rfspt = radhead->surv_range - 1;
    rf230end = MAXB230;
    end_rf = rfspt + radhead->n_surv_bins - 1;
    if( end_rf < MAXB230 ) 
        rf230end = end_rf;

    /* Determine overlay on this radial. */
    if( !Is_SZ2 )
        Echo_overlay( bdataptr, (void *) scrptr, 
                      (void *) (scrptr + (EPWR_SIZE*sizeof(float))) );

    else
        SZ2_echo_overlay( bdataptr, (void *) scrptr, 
                          (void *) (scrptr + (EPWR_SIZE*sizeof(float))) );


    /* Add this radial to bit map.... */
    Build_bit_map( start, delta, rfspt, rf230end, (void *) bufptr );

    /* Test for the last radial in the elevation cut. */
    if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) ){

        Endelcut = 1;

        /* Do end of product processing. */
        End_of_product_processing( voltime, vcpnumber, volnumber, 
                                   voldate, elang, bufptr );
    }

    return;

/* End of A30549_product_generation_control() */ 
}

/*\//////////////////////////////////////////////////////////////////////

   Description:
      This module initializes the echo power array and the power
      look-up table.

///////////////////////////////////////////////////////////////////////\*/
static int Prf_init( char *bdataptr, void *outbuf_epwr, 
                     void *outbuf_pwr_lookup ) {

    /* ***************** L o c a l   d e c l a r a t i o n s ************** */
    int i, j, prf, ur2, ur3;
    float atmos, syscal, bin_res, noise, bin_range;
    Base_data_header *rad_hdr = (Base_data_header *) bdataptr;

    static int old_delta_pri = 0;
    static int *unambigr = NULL;

    float *epwr = (float *) outbuf_epwr;
    float *pwr_lookup = (float *) outbuf_pwr_lookup;

    /* ***************** E x e c u t a b l e   c o d e ******************** */

    /* Initialize sum of overlaid ranges and flag for valid prfs */
    for( i = 0; i <= MAX_ALWBPRFS; i++ ) 
        Validprf[i] = 0;

    /* Check which prf's are allowable with the current vcp */
    for( i = 0; i < Num_alwbprfs; i++ ){

        prf = Allowable_prfs[i] - 1;

        /* Set flag and save allowable prf index */
        Validprf[prf] = 1;

        /* Initialize the obscuration area for this prf to zero */
        memset( &Overlaid[prf][0], 0, MAXBINS*sizeof(unsigned short) );

    }

    /* Get SYSCAL and ATMOS from the radial header.  Note:  syscal is actually 
       dbZ0 so it should be corrected for noise.  Since it represents a constant
       offset, it does not matter if not adjustment is made.  */
    noise = rad_hdr->horiz_noise;
    syscal = rad_hdr->calib_const - noise;
    atmos = rad_hdr->atmos_atten*ASCALE;

    /* Get BASE DATA TOVER value from the radial header */
    Tover = (float) rad_hdr->vel_tover*TSCALE;

    LE_send_msg( GL_INFO, "Atmos: %10.5f, Syscal: %10.5f, Tover: %10.5f\n",
                 atmos, syscal, Tover );

    /* Get reflectivity bin resolution from radial header. */
    bin_res = rad_hdr->surv_bin_size*M_TO_KM;

    /* Compute power lookup tables */
    for( i = 1; i <= PREFSIZE; i++ ){

        bin_range = ( (float) i - 0.5) * bin_res;
        pwr_lookup[i-1] = LOGFAC*log10(bin_range) + 
                        bin_range*atmos - syscal;

    }

    /* Flag bins with no power flag. */
    for( i = 0; i < EPWR_SIZE; i++ )
        epwr[i] = FLAG_NO_PWR;

    /* ******************************************************************** */
    /* Initialize folded bin look-up tables if not already initialized. */
    if( old_delta_pri != PS_delta_pri ){

        if( ( unambigr = ORPGVCP_unambiguous_range_table_ptr( PS_delta_pri )) == NULL ){

            RPGC_log_msg( GL_INFO, "ORPGVCP_unambiguous_range_table_ptr( %d) Returned NULL\n",
                          PS_delta_pri );
            return(-1);

        }

        /* Set 1st, 2nd, 3rd and 4th trip ranges from unambiguous ranges for   
           the valid Doppler PRF's. */
        for( j = 0; j < Num_alwbprfs; j++ ){

            prf = Allowable_prfs[j] - 1;

            ur2 = unambigr[prf]*2;
            ur3 = ur2 + unambigr[prf];

            /* Do For Bins 1 To The Unambiguous Range.  */
            for( i = 0; i < unambigr[prf]; i++ ){

                /* Set up look-up tables for the locations of the 2nd, 3rd, and 4th   
                   trip bins */
                Folded_bin1[prf][i] = i + unambigr[prf];
                Folded_bin2[prf][i] = i + ur2;
                Folded_bin3[prf][i] = i + ur3;

            }

            /* Do For Bins from 1 + The Unambiguous Range To 230 */
            for( i = unambigr[prf]; i <= MAX_FOLD_BIN; i++ ){

                /* Set up look-up tables for the 1st, 3rd, and 4th trip bins */
                Folded_bin1[prf][i] = i - unambigr[prf];
                Folded_bin2[prf][i] = i + unambigr[prf];
                Folded_bin3[prf][i] = i + ur2;

            }

        }

        /* Save current delta pri number */
        old_delta_pri = PS_delta_pri;

    }

    /* ********************************************************************* */
   return 0;

/* End of Prf_init() */
}

/***************************************************************************

   Description:
      This module checks the "overlaid" flag for each allowable prf and
      each range bin along the current radial.  If "overlaid" flag is set,
      the appropriate bit is set in the obscuration bit map output buffer.

   Inputs:
      start_angle - starting azimuth angle of current radial.  In degrees
                    * 10.
      delta_ang - azimuth separation, in degrees * 10.
      start_bin - bin index of first valid data in radial. 
      start_bin - bin index of last valid data in radial. 

   Outputs:
      outbuf - output buffer containing prf obscuration bit map.

   Returns:
      There are no return values defined for this function.

***************************************************************************/
static void Build_bit_map( int start_angle, int delta_ang, int start_bin, 
                           int end_bin, void *outbuf ){

   Prfbmap_prod_t *prf_prod;
   Prfbmap_t *bit_map;
   Radbmap_t *rad_bit_map;
   
   short prf;
   int   index, bit, i, j;

   /* Cast output buffer address to address of type Prfbmap_prod_t */
   prf_prod = (Prfbmap_prod_t *) outbuf;

   /* Do For All allowable PRFs */
   for( i = 0; i < Num_alwbprfs; i++ ){

      /* Extract an allowable PRF. */
      prf = Allowable_prfs[i] - 1;
      bit_map = (Prfbmap_t *) &prf_prod->bmap[prf];

      /* Set the start azimuth angle (degrees * 10) and delta azimuth angle 
         (degrees * 10) for this radial. */
      rad_bit_map = &bit_map->prfbmap[Radcount - 1];
      rad_bit_map->azm = start_angle;
      rad_bit_map->delta_azm = delta_ang;

      /* Build the bit map for this radial and this PRF. */
      for( j = start_bin; j <= end_bin; j++ ){

         /* If this bin is overlaid, then set appropriate bit in 
            bit map. */
         if( Overlaid[prf][j] ){

            /* Determine bit to set. */
            Get_bit( (unsigned int) j, &index, &bit ); 

            /* Set the bit. */
            rad_bit_map->radbmap[index] |= (1 << bit);

         }

      }

   }
  
/* End of Build_bit_map() */
}


/*********************************************************************

   Description:
      Performs end of product processing.  Fills in the product header
      with appropriate data.

   Inputs:
      vol_time - volume scan start time, in millisecs past midnight.
      vcp_num - the volume coverage pattern.
      vol_num - the volume scan sequence number.
      vol_date - the volume scan start date (modified Julian).
      elev_ang - the elevation angle of the product.

   Outputs:
      outbuf - the product output buffer which receives the input
               data.

   Returns:
      There is no return value defined for this function.

*********************************************************************/
static void End_of_product_processing( int vol_time, int vcp_num, 
                                       int vol_num, short vol_date, 
                                       int elev_ang, void *outbuf  ){

   Prfbmap_prod_t *prf_prod;
   unsigned int prf;
   int i;

   /* Cast output buffer address to address of type Prfbmap_prod_t */
   prf_prod = (Prfbmap_prod_t *) outbuf;

   /* Set product header items. */
   prf_prod->hdr.num_radials = Radcount;
   prf_prod->hdr.time = vol_time;
   prf_prod->hdr.num_prfs = (unsigned short) Num_alwbprfs;

   for( i = 0; i < Num_alwbprfs; i++ ){

      prf = Allowable_prfs[i] - 1;
      prf_prod->hdr.prf_nums |= ( 1 << prf );

   }

   prf_prod->hdr.vcp_num = (unsigned short) vcp_num;
   prf_prod->hdr.vol_num = (unsigned short) vol_num;
   prf_prod->hdr.date = vol_date;
   prf_prod->hdr.elev_angle = elev_ang;
   
/* End of End_of_product_processing() */ 
}

/*********************************************************************

   Description:
      Determines the bit map index and bit number given the input bin
      number.

   Inputs:
      bin - bin number.

   Outputs:
      index - receives the bit map index corresponding to bin.
      bit - receives the bit map bit number at index corresponding to
            bin.

   Returns:
      There is no return value define for this function.

   Notes:
      If the index value is greater than MAX_BYTES or less than 0,
      the process terminates.

*********************************************************************/
static void Get_bit( unsigned int bin, int *index, int *bit ){

   /* Determine index into bit map, and bit number at index. */
   *index = bin / BITS_PER_BYTE; 
   *bit = bin % BITS_PER_BYTE;

   /* Validate the bit map index.  Terminate process if invalid. */
   if( *index >= MAX_BYTES || *index < 0 ){

      LE_send_msg( GL_ERROR, "Invalid Bit Map Index %d For Bin %d\n", 
                   *index, bin );
      ORPGTASK_exit( GL_ERROR );

   }

/* End of Get_bit() */
}

/*\//////////////////////////////////////////////////////////////////////

   Description:
      This module converts reflectivity to echo power using a
      look-up table.  The echo power is then used to determine the
      amount of range obscuration for each of the valid Doppler PRFs.
      Bins are checked for overlaid echoes between 1 and 230 km using
      the full 460 km of coverage.
      First trip echoes have ranges <= The unambiguous range UR.
      Second trip: UR < bin index <= 2*UR, Third trip:
      2*UR < bin index <= 3*UR, Fourth trip : 3*UR < bin index <= 4*UR.

///////////////////////////////////////////////////////////////////////\*/
static void Echo_overlay( char *bdataptr, float *epwr, float *pwr_lookup ){

    /* *************** L o c a l   d e c l a r a t i o n s ***************** */
    int i, prf, fgbin, lgbin, begbin, endbin;
    float tpwr;

    Base_data_header *rad_hdr = (Base_data_header *) bdataptr;
    short *radial = (short *) (bdataptr + rad_hdr->ref_offset);

    /* *************** E x e c u t a b l e   c o d e *********************** */

    /* Get the first and last good bins this radial */
    fgbin = rad_hdr->surv_range - 1;
    lgbin = fgbin + rad_hdr->n_surv_bins - 1;

    /* Flag echo powers for bins less than the first good bin */
    endbin = fgbin - 1;
    for( i = 0; i < endbin; i++ )
        epwr[i] = FLAG_NO_PWR;

    /* Compute echo power from reflectivity using lookup table */
    for( i = fgbin; i <= lgbin; i++ ){

        float ref_dbZ;

        if( radial[i] != BASEDATA_RDBLTH ){

            ref_dbZ = RPGCS_reflectivity_to_dBZ( radial[i] );
            epwr[i] = ref_dbZ + pwr_lookup[i];
        
        }
        else{

            /* No reflectivity data, set echo power to flag value */
            epwr[i] = FLAG_NO_PWR;

        }

    }


    /* Set bins greater than last good bin to max of 460 to flag value */
    begbin = lgbin + 1;
    for( i = begbin - 1; i < PREFSIZE; i++ )
        epwr[i] = FLAG_NO_PWR;

    /* Do For All Doppler PRF indeces */
    for( prf = 0; prf < MAX_ALWBPRFS; prf++ ){

        /* Check if this PRF number is valid for the current vcp */
        if( Validprf[prf] ){

            /* Do For Valid bins <= 230 km range.  The folded bin locations are   
               retrieved from precomputed lookup tables. */
            endbin = lgbin;
            if( MAX_FOLD_BIN < lgbin )
               endbin = MAX_FOLD_BIN;
            for( i = fgbin; i <= endbin; i++ ){

                /* Initialize the overlaid value to 0 */
                Overlaid[prf][i] = 0;

                /* Check for valid power */
                if( epwr[i] != FLAG_NO_PWR ){

                    tpwr = epwr[i] - Tover;

                    /* Check for second trip overlay */
                    if( tpwr <= epwr[Folded_bin1[prf][i]] )
                        Overlaid[prf][i] = 1;

                    /* Check for third trip overlay */
                    else if( tpwr <= epwr[Folded_bin2[prf][i]] )
                        Overlaid[prf][i] = 1;

                    /* Check for fourth trip overlay */
                    else if( tpwr <= epwr[Folded_bin3[prf][i]] )
                        Overlaid[prf][i] = 1;

                }

            }

        }

    }

/* End of Echo_overlay() */
}
