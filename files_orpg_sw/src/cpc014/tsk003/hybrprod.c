/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/03/18 14:40:37 $ */
/* $Id: hybrprod.c,v 1.3 2014/03/18 14:40:37 steves Exp $ */
/* $Revision: 1.3 $ */
/* $State: Exp $ */

#include <hybrprod.h>

typedef union equiv {

   int ivalue;
   float rvalue;

} Same; 

/*********************************************************************
   Description:
      This routine performs the buffer control operations for the 
      Hybrid Scan Reflectivity task, as well as the highest level of 
      logical and functional control. 

*********************************************************************/
void A31431_buffer_controller(){

    /* Local variables */
    int *ipr, *p32_ptr, *p33_ptr;
    int istat, vsnum, maxval, opstat32, opstat33;
    unsigned int lyrlen;

    Scan_Summary *summary;

    /* Get input buffer from Precip Preprocessing algorithm */
    ipr = (int *) RPGC_get_inbuf_by_name( "HYBRSCAN", &istat);

    /* If input buffer successfully acquired, proceed */
    if( istat == NORMAL ){

        /* Get current volume scan number */
	vsnum = RPGC_get_buffer_vol_num( ipr );

        /* Extract maximum data value on the Hybrid Scan grid, in dBZ */
	maxval = A31432_determ_maxval( ipr + HYO_HYBRID );

        /* Build graphical HYBRID SCAN REFLECTIVITY (HSR), Product 33 */
        /* Determine whether product has been requested, this volume scan */
	opstat33 = RPGC_check_data_by_name( "HYBRGREF" );
	if( opstat33 == NORMAL ){

            /* If product presently requested, get output buffer */
	    p33_ptr = (int *) RPGC_get_outbuf_by_name( "HYBRGREF", SIZE_P33, 
                                                       &opstat33 );

            /* If output buffer successfully acquired, proceed */
	    if( (opstat33 == NORMAL) && (p33_ptr != NULL) ){

                /* Define the INDEX to the COLOR TABLES for HSR, depending on 
                   Weather Mode */
		HSR_color_index = 0;
                summary = RPGC_get_scan_summary( vsnum );
		if( summary != NULL ){

                    if( summary->weather_mode == PRECIPITATION_MODE ) 
		        HSR_color_index = 0;
		    else 
		        HSR_color_index = 1;

                }
		
                /* Build the HSR product */
		A3143a_hsr_rle( (short *) (ipr + HYO_HYBRID), (short *) p33_ptr );

                /* Build the Product Header (i.e., Message Header & Product Description 
                   blocks) */
                Code_HSR = RPGC_get_code_from_name( "HYBRGREF" );
		A31436_hr_product_hdr( (short *) p33_ptr, vsnum, Code_HSR, maxval, 
                                        ipr + HYO_SUPL );

                /* Release output buffer of HSR Product data and forward to Storage */
		RPGC_rel_outbuf( p33_ptr, FORWARD );

            /* Else if output buffer is not available, abort processing */
	    }
            else 
		RPGC_abort_dataname_because( "HYBRGREF", opstat33 );

	}

        /* Build DIGITAL HYBRID SCAN REFLECTIVITY (DHR), Product 32 */
        /* Determine whether product has been requested, this volume scan */
	opstat32 = RPGC_check_data_by_name( "HYBRDREF" );
	if( opstat32 == NORMAL ){

            /* If product presently requested, get output buffer */
	    p32_ptr = (int *) RPGC_get_outbuf_by_name( "HYBRDREF", SIZE_P32, 
                                                       &opstat32 );

            /* If output buffer successfully acquired, proceed */
	    if( opstat32 == NORMAL ){

                /* Perform 256-level Radial Run Length Encoding of the Hybrid Scan */
                /* Reflectivity data, and fill the Product Symbology Block */
		lyrlen = A31433_dig_radial( (short *) (ipr + HYO_HYBRID), (short *) p32_ptr );

                /* Format ASCII data in Layer 2 of product buffer */
		A31434_append_ascii( ipr + HYO_MESG, (float *) ipr + HYO_ADAP, ipr + HYO_SUPL, 
                                     lyrlen, (unsigned short *) p32_ptr );

                /* Pack the Product Header (i.e., Message Header & Product Description 
                   blocks) */
                Code_DHR = RPGC_get_code_from_name( "HYBRDREF" );
		A31436_hr_product_hdr( (short *) p32_ptr, vsnum, Code_DHR, maxval, 
                                       ipr + HYO_SUPL );

                /* Release output buffer of DHR Product data and forward to Storage */
		RPGC_rel_outbuf( p32_ptr, FORWARD );

            /* Else if output buffer is not available, abort processing */
	    }
            else 
		RPGC_abort_dataname_because( "HYBRDREF", opstat32 );

	}

        /* Release input buffer of Hybrid Scan Reflectivity data */
	RPGC_rel_inbuf( ipr );

    }
    else{

        /* Cannot get input buffer...abort */
	RPGC_abort();

    }

/* End of A31431_buffer_controller(). */
} 

/********************************************************************

   Description:
      This routine determines the maximum reflectivity value on the 
      Digital Hybrid Scan grid, in dBZ units (nearest whole).


   Inputs:
      input - pointer to hybrid scan data.
    
   Returns:
      The maximum data value.

********************************************************************/
int A31432_determ_maxval( void *input ){

    int maxval, rad, bin;
    short *hybrscan = (short *) input;

    /* Initialize the maximum data value. */
    maxval = 0;

    /* Retrieve max reflectivity index value from among all sample bins 
       of Hybrid Scan grid (excluding bins flagged 'Missing') */
    for( rad = 0; rad < MAX_AZMTHS; ++rad ){

	for( bin = 0; bin < MAX_HYBINS; ++bin ){

	    if( (hybrscan[bin] < BASEDATA_INVALID) 
                              && 
                (hybrscan[bin] > maxval) )
                maxval = hybrscan[bin];
	    
	}

        /* Increment pointer to "point" to start of next radial in polar grid. */
	hybrscan += MAX_HYBINS;

    }

    /* Convert max reflectivity index to dBZ (rounded down to nearest whole) */
    maxval = RPGCS_reflectivity_to_dBZ( maxval );

    return maxval;

/* End of A31432_determ_maxval(). */
}


#define DHR_ICENTER		0
#define DHR_JCENTER		0
#define RANGE_SCALE_FACT	1000
#define SCALE_FACTOR		10
#define DELTA_ANGLE		10

/****************************************************************************

   Description:
      This module packs two (2) 256-level radial data  per (I*2) 
      halfword of the product buffer.  This corresponds to the format 
      described in Fig. 3-11c of the RPG-Associated PUP ICD for the 
      Digital Radial Data Array Packet. 

   Inputs:
      hybrscan - pointer to hybrid scan data
      prodbuf - pointer to start of product buffer

   Returns:
      Layer length, in shorts.
   
      
****************************************************************************/
unsigned int A31433_dig_radial( short *hybrscan, short *prodbuf ){

    /* Local variables */
    int last_bin, ind, first_bin, rad, bin, obfrind, lyrlen;
    unsigned int i4word;
    Packet_16_hdr_t *packet16_hdr = NULL;
    Symbology_block *sym = NULL; 

    static unsigned char data[BASEDATA_REF_SIZE];

    /* Find the start of the Symbology block. */
    obfrind = sizeof(Graphic_product)/sizeof(short);
    sym = (Symbology_block *) &prodbuf[obfrind];

    /* Find the start of the Packet 16 header. */
    obfrind += sizeof(Symbology_block)/sizeof(short);
    packet16_hdr = (Packet_16_hdr_t *) &prodbuf[obfrind];

    /* Find the start of the Packet 16 data. */
    obfrind += sizeof(Packet_16_hdr_t)/sizeof(short);

    /* Initialize variables: */
    rad = 0;
    first_bin = 0;
    last_bin = MAX_HYBINS - 1;

    /* Do for all radials in the input buffer: */
    while( rad < MAX_AZMTHS ){

        int nbytes = 0;

        /* Do for all sample bins: */
        ind = 0;
	for( bin = first_bin; bin <= last_bin; bin++ ){

            /* Restrict range to (0-255) by resetting bins flagged 'Missing 
               Radial Data' (i.e., 256) to flag for "Missing' data (i.e., 1): */
	    if (hybrscan[bin] == BASEDATA_INVALID )
		data[ind] = 1;
	    else
		data[ind] = (unsigned char) hybrscan[bin];
	     
            ind++;

	}

        /*  End of radial. */
        nbytes = RPGC_digital_radial_data_array( data, RPGC_BYTE_DATA, 0, 
                                                 MAX_HYBINS-1, 0, MAX_HYBINS, 
                                                 1, (short) (rad*SCALE_FACTOR),
                                                 (short) DELTA_ANGLE, 
                                                 (void *) &prodbuf[obfrind] );

        /* Next sample bin will be the first (of the new radial). */
	++rad;
	first_bin += MAX_HYBINS;
	last_bin += MAX_HYBINS;

        obfrind += nbytes / 2;

    /* End of "while" loop. */
    }

    /* Fill in packet header for Digital Radial Data Array Packet. */
    RPGC_digital_radial_data_hdr( 0, MAX_HYBINS, DHR_ICENTER, DHR_JCENTER,
                                  RANGE_SCALE_FACT, (short) rad, packet16_hdr );

    /* Fill in Product Symbology Block Header. */
    sym->divider = -1;
    sym->block_id = 1;
    sym->n_layers = 2;
    sym->layer_divider = -1;

    /* Layer Length for Digital Radial Data RLE Packet. */
    i4word = (unsigned int) ((char *) &prodbuf[obfrind] - (char *) packet16_hdr);
    RPGC_set_product_int( &sym->data_len, i4word);

    /* Calculate value of Layer 1 End. */
    i4word = (int) ((char *) &prodbuf[obfrind] - (char *) prodbuf);
    lyrlen = i4word / sizeof(short);

    return lyrlen;

/* End of A31433_dig_radial(). */
} 

/* Parameter Specifications for the ASCII data layer (Layer2) of the
   DIGITAL HYBID SCAN REFLECTIVITY (DHR) Product
 
    The ASCII data layer (Layer2) is composed of 4 sub-layers corresponding to the
    Precip Status Message,  Adaptation Data,  Supplemental Data (Preproc Alg.),  and
    BIAS related fields respectively. Each sub-layer is preceded by a sub-layer header
    line (Char*8) in the format of 'PSM (  )', 'ADAP(  )', 'SUPL(  )' and 'BIAS(  )'
    respectively.
 
 ---------------------
 
    NCL_MESG       = 1 (for sub-layer header element) + HYZ_MESG
    NCL_ADAP       = 1 (for sub-layer header element) + HYZ_ADAP + 1 (for Bias Applied Flag)
    NCL_SUPL_PRE   = 1 (for sub-layer header element) + SSIZ_PRE
    NCL_BIAS_ITEMS = 1 (for sub-layer header element) + N_BIAS_ITEMS
   
    NCHARLINE      = Total number of charline items =
                     NCL_MESG + NCL_ADAP + NCL_SUPL_PRE + NCL_BIAS_ITEMS
    NI2_PRODLINE   = Size in halfwords of charline 
  
*/
#define N_BIAS_ITEMS	11
#define NCL_MESG	(1+HYZ_MESG)
#define NCL_ADAP	(1+HYZ_ADAP+1)
#define NCL_SUPL_PRE	(1+SSIZ_PRE)
#define NCL_BIAS_ITEMS 	(1+N_BIAS_ITEMS)

#define NCHARLINE	(NCL_MESG+NCL_ADAP+NCL_SUPL_PRE+NCL_BIAS_ITEMS)
#define NI2_PRODLINE	(NCHARLINE*4)

static unsigned short charline[NI2_PRODLINE];

/***********************************************************************************

   Description:
      This  routine  is called  to  format  the ASCII data in Layer 2 of the 
      Digital Hybridscan Reflectivity (DHR) product buffer. The layer consists 
      of four sub-layers of information as follows: 

         1) Precip Status Message 
         2) Adaptation  Data  (for all  PPS  parameters) 
         3) Supplemental Data (Precip. Preprocessing alg. only) 
         4) Bias-related fields. Each sub-layer is preceded by a 8-character 
            descriptive header indicating the title and the number of fields 
            of the sub-layer to follow. 

   Inputs:
      hydrmesg - pointer to Precip Status Message
      hydradap - pointer to Adaptation Data
      supl_pre - pointer to Supplemental Data
      lyr1en - layer length

   Outputs:
      pbuff - pointer to product buffer.

***********************************************************************************/
void A31434_append_ascii( int *hydrmesg, float *hydradap, int *supl_pre, 
                          unsigned int lyr1en, unsigned short *pbuff ){

    int i, fwd;
    int clidx, lyr2en, lyr2st;
    int lyr2lng;
    Same same;
    char *ptr = (char *) pbuff;
    Symbology_block *sym;
    packet_1_t *packet_1;

    sym = (Symbology_block *) (ptr + sizeof(Graphic_product));

    lyr2st = lyr1en;
    lyr2en = lyr2st + 7;

    clidx = 0;

    /* Clear out the scratch buffer. */
    memset( charline, 0, NI2_PRODLINE );

    /* Transfer PRECIP STATUS MESSAGE (PSM) to intermediate I*2 buffer */
    sprintf( (char *) &charline[clidx], "PSM (%2d)", HYZ_MESG );
    for( i = 0; i < HYZ_MESG; ++i ){

	clidx += 4;
	sprintf( (char *) &charline[clidx], "%8d", hydrmesg[i] );

    }

    /* Transfer ADAPTATION Data Array to intermediate I*2 buffer */
    clidx += 4;
    sprintf( (char *) &charline[clidx], "ADAP(%2d)", HYZ_ADAP+1 );

    for( i = 0; i < HYZ_ADAP; ++i ){

	clidx += 4;
        sprintf( (char *) &charline[clidx], "%8.2f", hydradap[i] );

    }

    /* Bias Applied Flag */
    clidx += 4;
    if( Hyd_adj.bias_flag )
       sprintf( (char *) &charline[clidx], "       T" );

    else
       sprintf( (char *) &charline[clidx], "       F" );

    /* Transfer SUPPLEMENTAL Data (Enhanced Preproc Alg.) to intermediate 
       I*2 buffer */
    clidx += 4;
    sprintf( (char *) &charline[clidx], "SUPL(%2d)", SSIZ_PRE );

    for( i = AVG_SCNDAT; i <= TBIN_SMTH; ++i ){

	clidx += 4;
	sprintf( (char *) &charline[clidx], "%8d", supl_pre[i] );

    }

    for( i = HYS_FILL; i <= HIG_ELANG; ++i ){

	same.ivalue = supl_pre[i];
	clidx += 4;
	sprintf( (char *) &charline[clidx], "%8.2f", same.rvalue );
    }

    same.ivalue = supl_pre[RAIN_AREA];
    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8.1f", same.rvalue );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", supl_pre[VOL_STAT] );

    /* Transfer BIAS items in Global COMMON /A3136C3/ to intermediate I*2 buffer 
       via ITC block */
    clidx += 4;
    sprintf( (char *) &charline[clidx], "BIAS(%2d)", N_BIAS_ITEMS );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", A3136c3.tbupdt );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", A3136c3.dbupdt );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", A3136c3.tbtbl_upd );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", A3136c3.dbtbl_upd );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", A3136c3.tbtbl_obs );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", A3136c3.dbtbl_obs );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", A3136c3.tbtbl_gen );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8d", A3136c3.dbtbl_gen );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8.4f", A3136c3.bias );

    clidx += 4;
    sprintf( (char *) &charline[clidx], "%8.2f", A3136c3.grpsiz );

    clidx += 4;
    /* Using %f notation (mmmm.dddd), when d is 0, the decimal point
       is suppressed.  The original FORTRAN used F8.0.   In
       order to replicate the behavior, we used "%7.0f.". */
    sprintf( (char *) &charline[clidx], "%7.0f.", A3136c3.mspan );

    /* Transfer CHARLINE data to the product buffer */
    for( i = 0; i < NI2_PRODLINE; ++i ){

	 pbuff[lyr2en] = charline[i];
	++lyr2en;

    }

    /* Compute length of PRODUCT SYMBOLOGY BLOCK */
    fwd = lyr2en*sizeof(short) - sizeof(Graphic_product);
    RPGC_set_product_int( &sym->block_len, fwd );

    /* Establish the size of this part of the output buffer */
    lyr2lng = (lyr2en - lyr2st - 3) * sizeof(short);

    /* Set up layer header data */
    pbuff[ lyr2st ] = -1;
    RPGC_set_product_int( &pbuff[ lyr2st + 1], lyr2lng );

    packet_1 = (packet_1_t *) &pbuff[ lyr2st + 3 ];
    packet_1->hdr.code = 1;
    packet_1->hdr.num_bytes = (short) (lyr2lng - 4);
    packet_1->data.pos_i = 0;
    packet_1->data.pos_j = 0;

/* End of A31434_append_ascii(). */
} 

#define MIN_SCAL_DBZ	-320 
#define INC_SCAL_DBZ	5
#define MAX_DIG_LEVS 	256

/************************************************************************

   Description:
      Builds product header portion of buffer.

   Inputs:
      prodbuf - pointer to product buffer.
      vsnum - volume scan number
      prodcode - product code
      maxval - maximum data level
      hydrsupl - Supplemental Data


************************************************************************/
void A31436_hr_product_hdr( short *prodbuf, int vsnum, int prodcode, 
                            int maxval, int *hydrsupl ){

    static int i;
    int i4word;
    int prod_id_hsr, prod_id_dhr; 

    Graphic_product *hdr = (Graphic_product *) prodbuf;
    Symbology_block *sym = (Symbology_block *) 
                           ((char *) prodbuf + sizeof(Graphic_product));

    /* Initialize buffer to NULL. */
    memset( (void *) prodbuf, 0, sizeof(Graphic_product) );

    /* Get the product ID from the data name. */
    prod_id_dhr = RPGC_get_id_from_name( "HYBRDREF" );
    prod_id_hsr = RPGC_get_id_from_name( "HYBRGREF" );

    /* Product data levels. */
    if( prodcode == Code_DHR ){

        /* Initialize the product description block. */
        RPGC_prod_desc_block( (void *) prodbuf, prod_id_dhr, vsnum );

        /* Data levels specified as minimum (dBZ*10), increment (dBZ*10)
           and number of levels. */
	hdr->level_1 = MIN_SCAL_DBZ;
	hdr->level_2 = INC_SCAL_DBZ;
	hdr->level_3 = MAX_DIG_LEVS;

    }else if( prodcode == Code_HSR ){

        /* Initialize the product description block. */
        RPGC_prod_desc_block( (void *) prodbuf, prod_id_hsr, vsnum );

        /* Use HSR_COLOR_INDEX from A3143HSR include data file. */
        unsigned short *color = (unsigned short *) &hdr->level_1;
	for( i = 0; i < 16; ++i)
	    *(color + i) = Color_table.thresh[HSR_color_index][i];
    }

    /* Set maximum data level. */
    hdr->param_4 = (short) maxval;

    /* Average hybrid scan date (Modified Julian) & time (minutes). */
    hdr->param_5 = (short) hydrsupl[AVG_SCNDAT];
    hdr->param_6 = (short) ((hydrsupl[AVG_SCNTIM] + 30) / 60);

    /* Set spot blank status. */
    hdr->n_maps = (short) hydrsupl[VOL_STAT];

    /* Define product version number. */
    if( prodcode == Code_DHR ) 
	hdr->n_maps = (short) (0x0200 | hdr->n_maps);

    else if( prodcode == Code_HSR ) 
	hdr->n_maps = (short) (0x0000 | hdr->n_maps);
     
    /* Set block offsets in Product Description block. */
    RPGC_set_prod_block_offsets( prodbuf, sizeof(Graphic_product)/sizeof(short), 0, 0 );

    /* Finish product header. */  
    RPGC_get_product_int( &sym->block_len, &i4word );

    if( prodcode == Code_HSR )
        RPGC_prod_hdr( prodbuf, prod_id_hsr, &i4word );

    else if( prodcode == Code_DHR )
        RPGC_prod_hdr( prodbuf, prod_id_dhr, &i4word );

/* End of A31436_hr_product_hdr(). */
} 

#define NUMBINS			230
#define NUMRAD			360
#define HSR_ICENTER		256 
#define HSR_JCENTER		280

/**********************************************************************

   Description:
      Builds the graphic hybrid scan reflectivity (HSR) product (Run-
      Length encoded).

   Inputs:
      inbuf - Hybrid scan reflectivity data.
   
   Outputs:
      outbuf - product buffer containing Run Length Encode data.

**********************************************************************/
void A3143a_hsr_rle( short *inbuf, short *outbuf ){

    int rle_index, az, ptr,delta, start, i4word;
    int excess, buffind, rlebyts, est_per_rad = 63;

    Graphic_product *hdr = (Graphic_product *) outbuf;
    Symbology_block *sym = (Symbology_block *) 
                           ((char *) outbuf + sizeof(Graphic_product));
    packet_af1f_hdr_t *packet_af1f = (packet_af1f_hdr_t *) 
                           ((char *) sym + sizeof(Symbology_block));

    /* Set up radial data packet header. */
    packet_af1f->code = 0xaf1f;
    packet_af1f->index_first_range = 0;
    packet_af1f->num_range_bins = NUMBINS;
    packet_af1f->i_center = HSR_ICENTER;
    packet_af1f->j_center = HSR_JCENTER;
    packet_af1f->scale_factor = 1000;
    packet_af1f->num_radials = NUMRAD;

    buffind = (int) ((&packet_af1f->num_radials - outbuf) + 1);
    rle_index = 0;

    /* Do For Each azimuth. */
    for( az = 0; az <= 359; ++az ){

        /* Starting angle. */
	start = az * 10;

        /* Delta angle. */
	delta = 10;

        /* First radial is a special case to correct display system glitch ...
           need to start at 359 degrees and make delta two degrees. */
	if( az == 0 ){

	    start = 3590;
	    delta = 20;

	}

        /* Compute pointer to each radial. */
	ptr = az * NUMBINS;

        /* Calculate the number of words left in the product output buffer. 
           If there are enough left, run length encode the radial. */
	excess = SIZE_P33 - (rle_index + 150);
	if( excess > est_per_rad ){

	    RPGC_run_length_encode( start, delta, &inbuf[ptr], 0, NUMBINS-1, NUMBINS, 
                                    1, &Color_table.coldat[HSR_color_index][0], 
                                    &rlebyts, buffind, outbuf );

            /* update position counters. */
	    rle_index += rlebyts;
	    buffind += rlebyts / sizeof(short);

	}

    }

    /* Set up header to block1. */
    RPGC_set_product_int( &hdr->sym_off, sizeof(Graphic_product)/sizeof(short) );

    /* Fill in divider and block ID. */
    sym->divider = -1;
    sym->block_id = 1;

    /* Fill in layer length and layer divider. */
    i4word = ((rle_index / sizeof(short) ) * sizeof(short)) + sizeof(packet_af1f_hdr_t);
    RPGC_set_product_int( &sym->data_len, i4word );

    sym->n_layers = 1;
    sym->layer_divider = -1;

    i4word += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, i4word );

/* End of A3143a_hsr_rle(). */
} 

