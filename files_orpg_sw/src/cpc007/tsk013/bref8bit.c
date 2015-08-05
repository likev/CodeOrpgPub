
/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/02/23 22:59:24 $ */
/* $Id: bref8bit.c,v 1.8 2009/02/23 22:59:24 steves Exp $ */
/* $Revision: 1.8 $ */
/* $State: Exp $ */

#include <bref8bit.h>

/*********************************************************************

   Description:
      Buffer control module for the 8-bit base reflectivity product.

   Returns:
      Currently, always returns 0.

*********************************************************************/
int BREF8bit_buffer_control(){

    int ref_flag, vel_flag, wid_flag;
    int opstat;

    char *obrptr = NULL;
    char *bdataptr = NULL;

    /* Initialization. */
    opstat = 0;
    Endelcut = 0;
    Proc_rad = 1;

    /* Acquire output buffer for 256 data level Base Reflectivity 
       product, 0 TO 460 km. Range (1km X 1deg resolution). */
    obrptr = RPGC_get_outbuf_by_name( "BREF8BIT", OBUF_SIZE, &opstat );

    if( opstat == NORMAL ){

        /* Request first input buffer (Radial base data) and process it. */
	bdataptr = RPGC_get_inbuf_by_name( "REFLDATA", &opstat);

        /* Check for Base Reflectivity disabled. */
	if( opstat == NORMAL ){

	    RPGC_what_moments( (Base_data_header *) bdataptr, &ref_flag, 
                               &vel_flag, &wid_flag );
	    if( !ref_flag ){

                /* Moment disabled .. Release input buffer and do abort processing. */
		RPGC_rel_inbuf( bdataptr );
		RPGC_rel_outbuf( obrptr, DESTROY );
		RPGC_abort_because( PROD_DISABLED_MOMENT );
		return 0;

            }

	}

        /* Do For All input (Base data) radials until end of elevation encountered. */
        while(1){

	    if( opstat == NORMAL ){

                /* Call the product generation control routine. */
	        BREF8bit_product_generation_control( obrptr, bdataptr );

                /* Release the input radial. */
	        RPGC_rel_inbuf( bdataptr );

	        if( !Endelcut ){

                    /* Retrieve next input radial. */
		    bdataptr = RPGC_get_inbuf_by_name( "REFLDATA", &opstat );
		    continue;

	        }
                else{

                    /* Elevation cut completed. */
		    RPGC_rel_outbuf( obrptr, FORWARD );
                    break;

	        }

	    }
            else {

                /* If the input data stream has been canceled for some reason, 
                   then release and destroy the product buffer, and return to 
                   wait for activation. */
	        RPGC_rel_outbuf( obrptr, DESTROY );
                RPGC_abort();
                break;

	    }

        }

    }
    else 
       RPGC_abort_because( opstat );
	 
    /* Return to wait for activation. */
    return 0;

/* End of BREF8bit_buffer_control() */
} 


/********************************************************************* 

   Description:
      Controls the processing of data for the 256-level Base 
      Reflectivity product on a radial basis. 

   Inputs:
      obrptr - pointer to product buffer.
      bdataptr - pointer to radial message.

   Returns:
      Always returns 0.

******************************************************************* */
int BREF8bit_product_generation_control( char *obrptr, char *bdataptr ){

    /* Local variables */
    int volnumber, vcpnumber, elevindex, frst_rf; 
    int ndpbyts, delta, start, end_rf, excess;

    double coselev, elang;
    
    Scan_Summary *summary = NULL;
    Base_data_header *radhead = (Base_data_header *) bdataptr;

    /* Static variables. */
    static int pbuffind = 0, ndpb = 0;
    static int numbins = MAXBINS, last_rf = MAXBINS;
    static short *outbuff = NULL;

    /* Beginning of product initialization. */
    if( (radhead->status == GOODBVOL) 
                    || 
        (radhead->status == GOODBEL) ){

        /* Initialize the max data level. */
	Mxpos = 0;

        /* Initialize the radial counter. */
	Radcount = 0;

        /* Initialize Buffer index counter and number of packet bytes. */
	pbuffind = ndpb = 0;
        outbuff = (short *) (obrptr + sizeof(Graphic_product) + 
                             sizeof(Symbology_block) + sizeof(Packet_16_hdr_t));

        /* Get elevation angle from volume coverage pattern and elevation 
           index number. */
	volnumber = RPGC_get_buffer_vol_num( bdataptr );
        summary = RPGC_get_scan_summary( volnumber );
	vcpnumber = summary->vcp_number;
	elevindex = RPGC_get_buffer_elev_index( bdataptr );

	Elmeas = (int) RPGCS_get_target_elev_ang(  vcpnumber, elevindex );
	elang = Elmeas * .1f;

        /* Cosine of elevation angle computation for combination with 
           scale factor for the scan projection correction. */
	coselev = cos( elang * DEGTORAD );
	Vc = coselev * 1e3f;

        /* Compute slant range (KM) to a height cutoff of 70K ft AGL.
           Compute last bin of radial to process. */
	numbins = RPGC_num_rad_bins( bdataptr, MAXBINS, 1, 1 );

        /* Pack product header fields. */
	BREF8bit_product_header( obrptr, volnumber );

    }

    /* Perform individual radial processing if radial not flagged 'BAD'. */
    if( Proc_rad ){

        /* Retrieve start angle and delta angle measurements from the 
           input radial header. */
	start = radhead->start_angle;
	delta = radhead->delta_angle;

        /* Calculate the start and end bin indices of the good data. */
	frst_rf = radhead->surv_range - 1;
	end_rf = frst_rf + radhead->n_surv_bins - 1;
	if( numbins < (end_rf + 1) )
           last_rf = numbins - 1;
        else
           last_rf = end_rf;

        /* Calculate remaining words available in output buffer. */
	excess = OBUF_SIZE - (ndpb + 150);
	if( excess > EST_PER_RAD ){

            Base_data_header *bdh = (Base_data_header *) bdataptr;
            short *refl = (short *) (bdataptr + bdh->ref_offset); 

            /* Increment the radial count. */
	    Radcount++;

            /* Determine maximum positive reflectivity in the elevation. */
	    BREF8bit_maxdl( bdataptr, frst_rf, last_rf );

            /* If sufficient space available, pack present radial in the output 
               buffer (packet 16 format). */
            ndpbyts = RPGC_digital_radial_data_array( (void *) refl, RPGC_SHORT_DATA,
                                                      frst_rf, end_rf, 0,
                                                      numbins, 1, start, delta,
                                                      (void *) &outbuff[pbuffind] );

            /* Update buffer counters and pointers. */
	    ndpb += ndpbyts;
	    pbuffind += ndpbyts / 2;

	}

    }

    /* Test for psuedo-end of elev or pseudo-end of volume. */
    if( (radhead->status == PGENDEL) || (radhead->status == PGENDVOL) )
	Proc_rad = 0;

    /* Test for last radial in the elevation cut. */
    if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) ){

	Endelcut = 1;

        /* If last radial encountered, fill remaining fields in product buffer. */
	BREF8bit_end_of_product_processing( ndpb, numbins, obrptr );

    }

    /* Return to buffer control routine. */
    return 0;

/* End of BREF8bit_product_generation_control() */
} 


/********************************************************************
   
   Description:
      Fills in product description block, symbology block information.

   Inputs:
      outbuff - pointer to output buffer.
      vol_num - volume scan number.

   Returns:
      Always returns 0.

********************************************************************/
int BREF8bit_product_header( char *outbuff, int vol_num ){

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = (Symbology_block *) 
                                 (outbuff + sizeof(Graphic_product));
    int elev_ind;
    short params[10];

    /* Fill in product description block fields. */
    RPGC_prod_desc_block( phd, DR_prod_id, vol_num );

    /* Set the product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );
    elev_ind = ORPGPAT_get_elevation_index( DR_prod_id );
    if( elev_ind >= 0 )
        params[elev_ind] = Elmeas;

    RPGC_set_dep_params( outbuff, params );

    /* Data level threshold codes. */
    phd->level_1 = -320;
    phd->level_2 = 5;
    phd->level_3 = 254;

    /* Store offset to symbology. */
    RPGC_set_prod_block_offsets( phd, sizeof(Graphic_product)/sizeof(short), 0, 0 );

    /* Store product block divider, block ID, number of layers and 
       layer divider. */
    sym->divider = -1;
    sym->block_id = 1;
    sym->n_layers = 1;
    sym->layer_divider = -1;

    /* Length of layer, length of block, length of message, scale factor,
       number of radials are stored in the end-of-product module. */
    return 0;

/* End of BREF8bit_product_header() */
}


/******************************************************************
   
   Description:
      Maximum-data-level routine for 256-level Base Reflectivity 
      product generation program. 

   Inputs:
      ibatptr - Input buffer pointer.
      frst_ref - index of first reflectivity bin.
      last_ref - index of last reflectivity bin.

   Returns:
      Always returns 0.

******************************************************************/
int BREF8bit_maxdl( char *ibatptr, int frst_ref, int last_ref ){

    int binindx;
    Base_data_header *bhd = (Base_data_header *) ibatptr;

    short *radial = (short *) (ibatptr + bhd->ref_offset); 

    /* Do For All reflectivity bins ..... */
    for( binindx = frst_ref;  binindx <= last_ref; binindx++ ){

        /* Find the maximum value. */
        if( (radial[binindx] < BASEDATA_INVALID) && (radial[binindx] > Mxpos) ) 
		Mxpos = radial[binindx];
	 
    }

    return 0;

/* End of  BREF8bit_maxdl() */
} 


/**********************************************************************

   Description:
      Fill remaining Product Header fields for 256-level Base 
      Reflectivity product. 

   Inputs:
      ndpd - number of bytes in the product so far.
      numbins - number of bins in a product radial.
      outbuff - pointer to output buffer.

   Returns:
      Always returns 0.

**********************************************************************/
int BREF8bit_end_of_product_processing( int ndpb, int numbins, 
                                        char *outbuff ){

    int bytecnt;
    float maxval;

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = (Symbology_block *) 
                           (outbuff + sizeof(Graphic_product));
    Packet_16_hdr_t *packet_16 = (Packet_16_hdr_t *) 
              (outbuff + sizeof(Graphic_product) + sizeof(Symbology_block));

    /* Complete the packet 16 header. */
    RPGC_digital_radial_data_hdr( 0, numbins, 0, 0, Vc, Radcount, 
                                  (void *) packet_16 );

    /* Assign the maximum data level to the product header. */
    maxval = RPGCS_reflectivity_to_dBZ( Mxpos );
    phd->param_4 = (short) RPGC_NINT( maxval );

    /* Calculate and store the product message length, the product block 
       length and the product layer length. */

    /* Length of product layer. */
    bytecnt = ndpb + sizeof(Packet_16_hdr_t);
    RPGC_set_product_int( &sym->data_len, bytecnt);

    /* Length of block. */
    bytecnt += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, bytecnt);

    /* Complete the product header. */
    RPGC_prod_hdr( outbuff, DR_prod_id, &bytecnt);

    /* Return to the product generation control routine. */
    return 0;

/* End of BREF8bit_end_of_product_processing() */
} 
