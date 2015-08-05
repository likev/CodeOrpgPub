/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2011/09/07 20:43:18 $ */
/* $Id: clutprod_prod_gen.c,v 1.6 2011/09/07 20:43:18 steves Exp $ */
/* $Revision: 1.6 $ */
/* $State: Exp $ */

#include <clutprod.h>
#include <packet_af1f.h>
#include <rpgcs.h>

/* Map Sizing Information. */
#define CLBY_MAXRAD_LEGACY	256
#define CLBY_MAXRAD_ORDA	360

#define NWM_MAXSEG_LEGACY	2
#define NWM_MAXSEG_ORDA		5
#define NWM_MAX_ZONES		16

/* Product Buffer Size, in Bytes, and Other Information. */
#define PROD_BUF_SIZE		32000
#define MAX_RANGE		230
#define EST_BYTES_PER_RADIAL 	29

/* Operator Select Codes. */
#define FORCE_BYPASS		0
#define BYPASS_MAP              1
#define FORCE_FILTER		2

/* Filter Channel (Legacy Only) */
#define SURVEILLANCE_CHANNEL	0
#define DOPPLER_CHANNEL		1

/* Static Global Variables. */
static short Pixel_value[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static short Cfcdata[MAX_RANGE];

/* Function Prototypes. */
static void Add_product_header( short *outbuf, short product_bit_map );
static void Clutprod_prod_gen_legacy( short *outbuf, int segment_number, 
                                      int channel_type, int *total_bytes, 
                                      int *num_radials );
static int Clutprod_prod_gen_orda( short *outbuf, int segment_number, 
                                   int *total_bytes, int *num_radials );

/**************************************************************************

   Description:
      Controls the generation of Clutter Filter Control products. 

   Returns:
      Currently always returns 0.

***************************************************************************/
int Clutprod_generation_control( void ){

    /* Local variables */
    int i, number_products, channel_type;
    int seg, disposition, status;
    int segment_number, total_bytes;
    int num_radials, *bufptr = NULL;

    short master_list[MAX_ORDA_PRODUCTS];

    disposition = FORWARD;

    /* Update Volume Status data. */
    RPGC_read_volume_status();

    /* Get the rda config (legacy or orda). */
    Rda_config = Clutprod_get_rda_config( );

    /* Determine how many products are to be generated.  If 
      GENERATE_ALL_PRODUCTS flag is set, then generate all products. 
      Otherwise, just generate those listed in REQUEST_LIST. */
    if( Clutinfo.generate_all_products ){

        LE_send_msg( GL_INFO, "Generate All Products\n" );

        /* Copy the preset product generation list into the master list. */
	if( Rda_config == ORPGRDA_LEGACY_CONFIG ){

	    number_products = MAX_PRODUCTS;
	    for( i = 0; i < MAX_PRODUCTS; ++i ) 
		master_list[i] = Preset_list_legacy[i];

	} 
        else{

	    number_products = Clm.clm_map_orda.num_elevation_segs;
	    if( number_products > MAX_ORDA_PRODUCTS ){

		RPGC_log_msg( GL_INFO, "Number of CLM Segments %d > Maximum %d\n",
                              number_products, MAX_ORDA_PRODUCTS );
		number_products = MAX_ORDA_PRODUCTS;

	    }

	    for( i = 0; i < MAX_ORDA_PRODUCTS; ++i ){

		if( i < number_products )
		    master_list[i] = Preset_list_orda[i];

		else 
		    master_list[i] = 0;

	    }

	}

	RPGC_log_msg( GL_INFO, "Generate All Products (%d)\n", 
                      number_products );

    } 
    else{

        /* Copy the narrowband user request list into the master list. */
	number_products = 0;
	for( i = 0; i < MAX_ORDA_PRODUCTS; ++i ){

	    if( Request_list[i] != 0 ){

		master_list[number_products] = 
			           Request_list[i];
                LE_send_msg( GL_INFO, "Request With Product Parameter: %d\n", 
                             master_list[number_products] );
		++number_products;

	    }

        }

        LE_send_msg( GL_INFO, "Total Number of Requested Products: %d\n", number_products );

    }

    /* Initialize local copies of some important variables to prevent them 
       from changing during product generation */
    Local_data.loc_nwm_gentime = Cnwm.nwm_map_legacy.time;
    Local_data.loc_nwm_gendate = Cnwm.nwm_map_legacy.date;
    Local_data.loc_clm_gentime = Clm.clm_map_orda.time;
    Local_data.loc_clm_gendate = Clm.clm_map_orda.date;
    Local_data.loc_bpm_gentime = Cbpm_legacy.clby_map_legacy.time;
    Local_data.loc_bpm_gendate = Cbpm_legacy.clby_map_legacy.date;
    Local_data.loc_bpm_gentime_orda = Cbpm_orda.clby_map_orda.time;
    Local_data.loc_bpm_gendate_orda = Cbpm_orda.clby_map_orda.date;
    Local_data.loc_bpm_cmd_generated = Cbpm_orda.cmd_generated;

    /* Cycle through MASTER_LIST. */
    for( i = 0; i < number_products; ++i ){

        /* Ask for product buffer. */
	bufptr = RPGC_get_outbuf_by_name( "CFCPROD", PROD_BUF_SIZE, &status );

        /* If acquisition NORMAL, then ... */
	if( status == NORMAL ){

	    if( Rda_config == ORPGRDA_ORDA_CONFIG ) {

                /* Set SEGMENT_NUMBER based on the request bit map value. 
                   (Assumption:  Up to 5 possible for ORDA.) */
		segment_number = 0;
		for( seg = 0; seg < MAX_ORDA_PRODUCTS; ++seg ){

		    if( master_list[i] == Preset_list_orda[seg] ){

			segment_number = seg;
			break;
		    }

		}

                /* Get the product generated. */
		RPGC_log_msg( GL_INFO, "Generate Product For Segment %d\n", 
                              segment_number);

		status = Clutprod_prod_gen_orda( (short *) bufptr, segment_number, 
                                                 &total_bytes, &num_radials );

                /* Finish product generation. */
		if( status >= 0 )
		    Add_product_header( (short *) bufptr, master_list[i] );

		else 
		    disposition = DESTROY;
		 
	    } 
            else{

                /* Set SEGMENT_NUMBER and CHANNEL_TYPE based on the request 
                   bit map value.  (Assumption:  Only two elevation segments 
                   possible for Legacy, 1 and 2) */
		segment_number = (master_list[i] / 2) - 1;
		if( (master_list[i] & 1) == 1 ){

                    /* Bit 15 must be set because MASTER_LIST_LEGACY is odd. */
		    channel_type = 1;
		} 
                else 
		    channel_type = 0;
		 
		Clutprod_prod_gen_legacy( (short *) bufptr, segment_number, 
                                          channel_type, &total_bytes, &num_radials );

                /* Finish product generation. */
		Add_product_header( (short *) bufptr, master_list[i] );

            }

            /* If product generation successful, put on the finishing touches. */
            if( disposition == FORWARD ){

                packet_af1f_hdr_t *packet_af1f = 
                    (packet_af1f_hdr_t *) (((char *) bufptr) +
                                 sizeof(Graphic_product) + sizeof(Symbology_block));
                Symbology_block *sym = 
                    (Symbology_block *) (((char *) bufptr) + sizeof(Graphic_product));

                /* Set AF1F packet fields. */
                packet_af1f->num_radials = num_radials;
                packet_af1f->code = 0xaf1f;
                packet_af1f->index_first_range = 0;
                packet_af1f->num_range_bins = MAX_RANGE;
                packet_af1f->i_center = 256;
                packet_af1f->j_center = 280;
                packet_af1f->scale_factor = 1000;

                /* Set the Symbology Block data layer length. */
                total_bytes = total_bytes  + sizeof( packet_af1f_hdr_t );
                RPGC_set_product_int( &sym->data_len, total_bytes );

                /* Set Symbology Block fields. */
                total_bytes += sizeof(Symbology_block);
                RPGC_set_product_int( &sym->block_len, total_bytes );

                sym->divider = -1;
                sym->block_id = 1;
                sym->n_layers = 1;
                sym->layer_divider = -1;

                /* Set the Symbology Block offset in Product Description Block. */
                RPGC_set_prod_block_offsets( bufptr, sizeof(Graphic_product)/sizeof(short),
                                             0, 0 );
 
                /* Finish with the product header. */
                RPGC_prod_hdr( bufptr, CFCprod_id, &total_bytes );

	    }

            /* Release product to Buffer Management. */
	    RPGC_rel_outbuf( bufptr, disposition );
	    if( disposition == DESTROY ) {

                /* Problem with product generation.   Abort further processing. */
		RPGC_abort();
		goto L400;

	    }

	} 
        else{

            LE_send_msg( GL_INFO, "RPGC_get_outbuf_by_name for CFCPROD returned BAD %d\n", status );

            /* Notify scheduler the product was memory loadshed only if 
               a user requested this particular product. */
	    if( Request_list[master_list[i]] != 0 ) 
		Clutprod_request_response( PGM_MEM_LOADSHED, master_list[i] );
	     
	}

    }

    /* Done with product generation.  Reset PRODUCT_REQUEST_PENDING 
       flag, and clear the user product request list, REQUEST_LIST. */

L400:

    Clutinfo.product_request_pending = 0;
    for (i = 0; i < MAX_ORDA_PRODUCTS; ++i ) 
	Request_list[i] = 0;

    /* Return to Calling Module. */
    return 0;

/* End of Clutprod_generation_control() */
}

/*************************************************************************

   Description:
      Build product Message Header Block, Product Description Block, 
      and portions of the Product Symbology Block. 

   Inputs:
      outbuf - product buffer.
      product_bit_map - product dependent parameter (request bit map).

*************************************************************************/
static void Add_product_header( short *outbuf, short product_bit_map ){

    Graphic_product *phd = (Graphic_product *) outbuf;
    short params[10];
    short elevation_segment;

    /* Get the rda config (legacy or orda). */
    Rda_config = Clutprod_get_rda_config();

    /* Store product code, source id, and number of blocks. */
    RPGC_prod_desc_block( outbuf, CFCprod_id, Vol_stat.volume_scan );

    /* Store product dependent information. */
    params[0] = product_bit_map;
    params[1] = (short) 0;
    if( Rda_config == ORPGRDA_LEGACY_CONFIG ){

	params[7] = (short) Local_data.loc_nwm_gentime;
	params[6] = (short) Local_data.loc_nwm_gendate;
	params[4] = (short) Local_data.loc_bpm_gendate;
	params[5] = (short) Local_data.loc_bpm_gentime;

    } 
    else{

	params[7] = (short) Local_data.loc_clm_gentime;
	params[6] = (short) Local_data.loc_clm_gendate;
	params[4] = (short) Local_data.loc_bpm_gendate_orda;
	params[5] = (short) Local_data.loc_bpm_gentime_orda;

        elevation_segment = product_bit_map >> 1;
        if( elevation_segment & Local_data.loc_bpm_cmd_generated ){

           LE_send_msg( GL_INFO, "Setting CMD Generated Flag in Product\n" );
           params[1] = 1;

        }

    }

    RPGC_set_dep_params( outbuf, params );

    /* Store product version number.  If legacy, version=0, 
       else if orda, version=1. */
    if( Rda_config == ORPGRDA_ORDA_CONFIG ) 
	phd->n_maps = (short) (256 | phd->n_maps);

/* End of Add_product_header() */
} 


#define FULL_CIRCLE		3600.0f
#define DELTA_AZIMUTH		(FULL_CIRCLE/256.0)
#define START_AZIMUTH		(FULL_CIRCLE-(DELTA_AZIMUTH/2.0))
#define IFULL_CIRCLE		3600

#define BYPASS_OFFSET_LEGACY	0
#define CONTROL_OFFSET_LEGACY	1
#define FORCE_OFFSET_LEGACY	4

/************************************************************************

   Description:
      Extracts the Bypass and Notchwidth data, color encodes, and 
      calls routine to run-length-encode. 

   Inputs:
      outbuf - output buffer.
      segment_number - elevation segment number.
      channel_type - filter channel.
  
   Outputs:
      total_bytes - total bytes in the AF1F packet.
      num_radials - number of radials in the AF1F packet.

************************************************************************/
static void Clutprod_prod_gen_legacy( short *outbuf, int segment_number, 
                                      int channel_type, int *total_bytes, 
                                      int *num_radials ){

    /* Local variables */
    float real_azimuth;
    int integer_azimuth, last_azimuth, bytes_remaining, range_end;
    int azimuth_index, loop_start, loop_end, op_code, suplev;
    int bitset, prodbuff_index, range_index, azm_incr, zone;
    int d_suplev, z_suplev, rlebytes, index;

    RDA_bypass_map_segment_t *segment = NULL;

    /* Initialize the position of the first azimuth radial, the 
       length of the data block and the buffer start index. */
    real_azimuth = START_AZIMUTH;
    integer_azimuth = (int) real_azimuth;
    *total_bytes = 0;
    prodbuff_index = (sizeof(Graphic_product) + sizeof(Symbology_block) 
                     + sizeof(packet_af1f_hdr_t))/sizeof(short);
    azm_incr = RPGC_NINT( (float) DELTA_AZIMUTH );

    /* Initialize the number of radials in the product to 0. */
    *num_radials = 0;

    /* Initialize the Cfcdata array. */
    memset( Cfcdata, 0, MAX_RANGE*sizeof(short) );

    /* Process all azimuth segments on this elevation segment. */
    segment = &Cbpm_legacy.clby_map_legacy.segment[segment_number];
    for( index = 0; index <= CLBY_MAXRAD_LEGACY; ++index ){

	if( index >= CLBY_MAXRAD_LEGACY ) 
	    azimuth_index = index - CLBY_MAXRAD_LEGACY;

        else 
	    azimuth_index = index;
	
        /* No need to process this radial if output buffer doesn't have 
           enough room. */
	bytes_remaining = PROD_BUF_SIZE - (*total_bytes) + 150;
	if( bytes_remaining < EST_BYTES_PER_RADIAL )
	    break;

        /* Appears to be enough room in the output buffer to process this 
           radial. */
        (*num_radials)++;
	loop_start = 0;

        /* Process the CENSOR ZONES for this azimuth radial and elevation 
           segment. */
	for( zone = 0; zone < NWM_MAX_ZONES; ++zone ){

	    op_code = 
               Cnwm.nwm_map_legacy.data[zone].filter[azimuth_index][segment_number].op_code;
	    range_end = 
               Cnwm.nwm_map_legacy.data[zone].filter[azimuth_index][segment_number].range;
	    z_suplev = 
               Cnwm.nwm_map_legacy.data[zone].suppr[azimuth_index][segment_number].surv_width;
	    d_suplev = 
               Cnwm.nwm_map_legacy.data[zone].suppr[azimuth_index][segment_number].dplr_width;
	    if( channel_type == DOPPLER_CHANNEL ){

                /* Doppler suppression levels selected. */
		suplev = d_suplev;

	    } 
            else {

                /* Surveillance suppression levels selected. */
		suplev = z_suplev;

	    }

            /* Calculate the end range for this range zone. */
            if( MAX_RANGE < 2*range_end )
                loop_end = MAX_RANGE;

            else
	        loop_end = 2*range_end;

            /* Check the operator select code.  Product data level depends 
               on value. */
	    if( op_code == FORCE_BYPASS ){

                /* Operator select code is force bypass of filter. */
		for( range_index = loop_start; range_index < loop_end; ++range_index )
		    Cfcdata[range_index] = Pixel_value[BYPASS_OFFSET_LEGACY];
		
	    } 
            else if( op_code == BYPASS_MAP ){

                /* Operator select code is Bypass Map in control of filtering. */
		for( range_index = loop_start; range_index < loop_end; ++range_index ){

                   /* Check if the Bypass Map bit is set. */
                    bitset = RPGCS_bit_test_short( (unsigned char *) &segment->data[azimuth_index][0], 
                                                   range_index );
		    if( !bitset ){

                        /* Bit is not set.  Filter in control. */
			Cfcdata[range_index] = Pixel_value[suplev + CONTROL_OFFSET_LEGACY];

		    } 
                    else{

                        /* Bit is set. Bypass filter. */
			Cfcdata[range_index] = Pixel_value[CONTROL_OFFSET_LEGACY];

		    }

		}

	    } 
            else if( op_code == FORCE_FILTER ) {

                /* Operator select code is force clutter filtering. */
		for( range_index = loop_start; range_index < loop_end; ++range_index ) 
		    Cfcdata[range_index] = Pixel_value[suplev + FORCE_OFFSET_LEGACY];

	    }

            /* Set LOOP_START.  Check for LOOP_START >= MAX_RANGE. */
	    loop_start = loop_end;
	    if( loop_start >= MAX_RANGE ) 
		break;
	    
	}

        /* Save the current azimuth position */
	last_azimuth = integer_azimuth;

        /* Calculate the true azimuth position.  Convert to integer value. */
	real_azimuth += DELTA_AZIMUTH;
	if( real_azimuth >= FULL_CIRCLE ) 
	    real_azimuth += -FULL_CIRCLE;
	 
	integer_azimuth = (int) real_azimuth;

        /* Calculate the azimuth difference.  Account for REAL/INTEGER 
           conversion factors */
	azm_incr = integer_azimuth - last_azimuth;

        /* Correct for negative values */
	if( azm_incr < 0 ) 
	    azm_incr += IFULL_CIRCLE;

        /* Ensure last_azimuth is in the range [0, 3599]. */
        if( last_azimuth >= 3600 )
           last_azimuth -= 3600;
	 
        /* Call routine to run-length-encode the data on this azimuth 
           radial. */
	RPGC_run_length_encode( last_azimuth, azm_incr, Cfcdata, 0,
		                loop_end-1, loop_end, 1, &Coldat.coldat[29][0], 
                                &rlebytes, prodbuff_index, outbuf );

        /* Increment the number of run-length-encoded bytes and product 
           buffer index. */
	*total_bytes += rlebytes;
	prodbuff_index += rlebytes / 2;

    }

    /* Done with processing. */

    /* Return to Calling Routine */
    return;

/* End of Clutprod_prod_gen_legacy() */
} 

#define DELTA_AZIMUTH_ORDA		10.0f
#define START_AZIMUTH_ORDA		FULL_CIRCLE
#define NUMELEV_OFFSET			0
#define CLM_MAXZONES_ORDA		20

#define BYPASS_OFFSET_ORDA		0
#define CONTROL_OFFSET_NO_CLUTTER_ORDA	1
#define CONTROL_OFFSET_CLUTTER_ORDA	4
#define FORCE_OFFSET_ORDA		7

/**********************************************************************

   Description:
      Extracts the Bypass and Clutter Map data, color encodes, and 
      calls routine to run-length-encode. 

   Inputs:
      outbuf - product buffer.
      segment_number - elevation segment number of product.

   Outputs:
      total_bytes - total number of bytes in the AF1F packet.
      num_radials - number of radials in the AF1F packet.

   Returns:
      0 on success, -1 on failure.

**********************************************************************/
static int Clutprod_prod_gen_orda( short *outbuf, int segment_number, 
                                   int *total_bytes, int *num_radials ){

    /* Local variables */
    float real_azimuth;
    int op_code, rlebytes, bitset, prodbuff_index, bytes_remaining;
    int num_segments, last_azimuth, azimuth_index, clm_ptr;
    int integer_azimuth, num_zones, range_index, azm_incr;
    int zone, loop_start, loop_end, range_end, segment_counter;

    ORDA_clutter_map_segment_t *cm_segment = NULL;
    ORDA_bypass_map_segment_t *bm_segment = NULL;
    short *clm_map = (short *) &Clm.clm_map_orda.date;

    /* Initialize the position of the first azimuth radial, the length of 
       the data block and the buffer start index. */
    real_azimuth = START_AZIMUTH_ORDA;
    integer_azimuth = (int) real_azimuth;
    *total_bytes = 0;
    prodbuff_index = (sizeof(Graphic_product) + sizeof(Symbology_block) 
                     + sizeof(packet_af1f_hdr_t))/sizeof(short);
    azm_incr = RPGC_NINT(DELTA_AZIMUTH);

    /* Initial the number of radials in the product to 0. */
    *num_radials = 0;

    /* Verify that the requested segment number data is available within
       the map data. */
    num_segments = Clm.clm_map_orda.num_elevation_segs;
    if( segment_number >= num_segments )
	return -1;

    /* Initialize the Cfcdata array. */
    memset( Cfcdata, 0, MAX_RANGE*sizeof(short) );

    /* Skip to the correct elevation segment number. (Note: The data structure
       defined in orda_clutter.h can not be used to deference the data.  The
       clutter map radial segments are variable length.) */
    clm_ptr = (int) ((short *) &Clm.clm_map_orda.data[0] - (short *) &clm_map[0]);
    segment_counter = 0;
    azimuth_index = 0;
    while( segment_number > 0 ){

        /* Get the number of range zones this radial segment. */
        num_zones = clm_map[clm_ptr];
        if( (num_zones < 1) || (num_zones > MAX_RANGE_ZONES_ORDA) ){

            RPGC_log_msg( GL_INFO, "Number Zones (%d) Exceeds Maximum (%d)\n",
                          num_zones, MAX_RANGE_ZONES_ORDA );
            return -1;

        }

        /* Adjust the clm_ptr by the number of zones.  Account for the number 
           of zones field. */
        clm_ptr += (num_zones * sizeof(ORDA_clutter_map_filter_t)/sizeof(short)) + 1;
        ++azimuth_index;
        if( azimuth_index >= CLM_MAXRAD_ORDA ){

            /* At the end of this elevation segment.  Increment the elevation 
               segment counter. */
            azimuth_index = 0;
            ++segment_counter;

            /* If at the desired elevation segment, break out of loop. */
            if( segment_counter == segment_number ) 
                break;

        }

    }

    /* Process all azimuth segments on this elevation segment. */
    cm_segment = (ORDA_clutter_map_segment_t *) &clm_map[clm_ptr];
    bm_segment = (ORDA_bypass_map_segment_t *) 
                              &Cbpm_orda.clby_map_orda.segment[segment_number];

    for( azimuth_index = 0; azimuth_index < CLM_MAXRAD_ORDA; ++azimuth_index ){

        /* No need to process this radial if output buffer doesn't have 
           enough room. */
	bytes_remaining = PROD_BUF_SIZE - (*total_bytes + 150);
	if( bytes_remaining < EST_BYTES_PER_RADIAL ) 
	    break;

        /* Appears to be enough room in the output buffer to process this 
           radial. */
	*num_radials += 1;
	loop_start = 0;
        loop_end = MAX_RANGE;

        /* Determine the number of zones for this azimuth segment */
	num_zones = cm_segment->num_zones;
	if( (num_zones < 1) || (num_zones > CLM_MAXZONES_ORDA) ){

	    RPGC_log_msg( GL_INFO, "Number of Clutter Censor Zones Invalid (%d)\n",
                          num_zones );
	    return -1;

	}

        /* Process the CENSOR ZONES for this azimuth radial and elevation 
           segment. */
	for( zone = 0; zone < num_zones; ++zone ){

            /* Set check for LOOP_START >= MAX_RANGE. */
	    if( loop_start < MAX_RANGE ){

	        op_code = cm_segment->filter[zone].op_code;
	        range_end = cm_segment->filter[zone].range;

                /* Calculate the end range for this range zone. */
	        if( range_end < MAX_RANGE )
                    loop_end = range_end;

                else
                    loop_end = MAX_RANGE;

                /* Check the operator select code.  Product data level depends 
                   on value. */
	        if( op_code == FORCE_BYPASS ){

                    /* Operator select code is force bypass of filter. */
		    for( range_index = loop_start; range_index < loop_end; ++range_index )
		        Cfcdata[range_index] = Pixel_value[BYPASS_OFFSET_ORDA];

	        } 
                else if( op_code == BYPASS_MAP ){

                    /* Operator select code is Bypass Map in control of filtering. */
		    for( range_index = loop_start; range_index < loop_end; ++range_index ){

                        /* Check if the Bypass Map bit is set. */
                        bitset = RPGCS_bit_test_short( (unsigned char *) &bm_segment->data[azimuth_index][0],
                                                       range_index );
		        if( !bitset) {

                            /* Bit is not set.  Filter in control. */
			    Cfcdata[range_index] = Pixel_value[CONTROL_OFFSET_CLUTTER_ORDA];

		        } 
                        else{

                            /* Bit is set. Bypass filter. */
			    Cfcdata[range_index] = Pixel_value[CONTROL_OFFSET_NO_CLUTTER_ORDA];

		        }

		    }

	        } 
                else if( op_code == FORCE_FILTER ){

                    /* Operator select code is force clutter filtering. */
		    for( range_index = loop_start; range_index < loop_end; ++range_index ) 
		        Cfcdata[range_index] = Pixel_value[FORCE_OFFSET_ORDA];

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

        /* Save the current azimuth position */
	last_azimuth = integer_azimuth;

        /* Calculate the true azimuth position.  Convert to integer value. */
	real_azimuth += DELTA_AZIMUTH_ORDA;
	if( real_azimuth >= FULL_CIRCLE )
	    real_azimuth -= FULL_CIRCLE;
	 
	integer_azimuth = (int) real_azimuth;

        /* Calculate the azimuth difference.  Account for REAL/INTEGER 
           conversion factors */
	azm_incr = integer_azimuth - last_azimuth;

        /* Correct for negative values */
	if( azm_incr < 0 ) 
	    azm_incr += FULL_CIRCLE;

        /* Ensure last_azimuth is in the range [0, 3599]. */
        if( last_azimuth >= 3600 )
           last_azimuth -= 3600;
	 
        /* Call routine to run-length-encode the data on this azimuth 
           radial. */
	RPGC_run_length_encode( last_azimuth, azm_incr, Cfcdata, 0, 
                                loop_end-1, loop_end, 1, &Coldat.coldat[29][0], 
                                &rlebytes, prodbuff_index, outbuf );

        /* Increment the number of run-length-encoded bytes and product 
           buffer index. */
	*total_bytes += rlebytes;
	prodbuff_index += rlebytes / 2;

    }

    /* Done with processing. */

   /* Return to Calling Routine */
    return 0;

/* End of Clutprod_prod_gen_orda() */
} 

