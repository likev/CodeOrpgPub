
/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/02/23 22:55:35 $ */
/* $Id: bvel8bit.c,v 1.12 2009/02/23 22:55:35 steves Exp $ */
/* $Revision: 1.12 $ */
/* $State: Exp $ */

#include <bvel8bit.h>

/*********************************************************************

   Description:
      Buffer control module for the 8-bit base velocity product.

   Returns:
      Always returns 0.

*********************************************************************/
int BVEL8bit_buffer_control(){

    int ref_flag, vel_flag, wid_flag;
    int opstat;

    char *obrptr = NULL;
    char *bdataptr = NULL;

    /* Initialization. */
    opstat = 0;
    Endelcut = 0;
    Proc_rad = 1;

    /* Acquire output buffer for 256 data level Base Velocity     
       product, 0 TO 300 km. Range (.25KM X 1Deg resolution). */
    obrptr = RPGC_get_outbuf_by_name( "BVEL8BIT", OBUF_SIZE, &opstat );

    if( opstat == NORMAL ){

        /* Request first input buffer (radial base data) and process it. */
	bdataptr = RPGC_get_inbuf_by_name( "COMBBASE", &opstat);

        /* CHECK FOR BASE VELOCITY DISABLED */
	if( opstat == NORMAL ){

	    RPGC_what_moments( (Base_data_header *) bdataptr, &ref_flag, 
                               &vel_flag, &wid_flag );
	    if( !vel_flag ){

                /* Moment disabled .. Release input buffer and abort. */
		RPGC_rel_inbuf( bdataptr );
		RPGC_rel_outbuf( obrptr, DESTROY );
		RPGC_abort_because( PROD_DISABLED_MOMENT );
		return 0;

            }

	}

        /* Do For All input (Base data) radials until end of elevation. */
        while(1){

	    if( opstat == NORMAL ){

                /* Call the product generation control routine. */
	        BVEL8bit_product_generation_control( obrptr, bdataptr );

                /* Release the input radial. */
	        RPGC_rel_inbuf( bdataptr );

	        if( !Endelcut ){

                    /* Retrieve next input radial. */
		    bdataptr = RPGC_get_inbuf_by_name( "COMBBASE", &opstat );
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
                   release and destroy product buffer and return to wait for  
                   activation. */
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

/* End of BVEL8bit_buffer_control() */
} 

/********************************************************************* 

   Description:
      Controls the processing of data for the 256-level Base 
      Velocity product on a radial basis. 

   Inputs:
      obrptr - pointer to product buffer.
      bdataptr - pointer to radial message.

   Returns:
      Always returns 0.

******************************************************************* */
int BVEL8bit_product_generation_control( char *obrptr, char *bdataptr ){

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
    static RESO reso = 0;

    /* Beginning of product initialization. */
    if( (radhead->status == GOODBVOL) 
                    || 
        (radhead->status == GOODBEL) ){

        /* Initialize the max data level. */
	Mxpos = 0;
        Mxneg = 255;
        Zero_velocity = 129;

        /* Radial count initialization. */
	Radcount = 0;

        /* Buffer index counter and number of packet bytes counter 
           initialization. */
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
           scale factor for the projectio correction. */
	coselev = cos( elang * DEGTORAD );
	Vc = coselev * 1e3f;

        /* Compute slant range (km) to a height cutoff of 70K ft AGL.
           Comptue last bin of radial to process. */
	numbins = RPGC_num_rad_bins( bdataptr, MAXBINS, 1, 2 );

        /* Find the data value corresponding to 0 m/s. */
        reso = RPGCS_set_velocity_reso( (int) radhead->dop_resolution );
        Zero_velocity = RPGCS_ms_to_velocity( reso, 0.0 );

        /* Pack product header fields. */
	BVEL8bit_product_header( obrptr, volnumber, (int) radhead->dop_resolution );

    }

    /* Perform individual radial processing if radial not flagged 'BAD'. */
    if( Proc_rad ){

        /* Retrieve start angle and delta angle measurements from the 
           input radial header. */
	start = radhead->start_angle;
	delta = radhead->delta_angle;

        /* Calculate the start and end bin indices of data. */
	frst_rf = radhead->dop_range - 1;
	end_rf = frst_rf + radhead->n_dop_bins - 1;
	if( numbins < (end_rf + 1) )
           last_rf = numbins - 1;

        else
           last_rf = end_rf;

        /* Calculate remaining words available in output buffer. */
	excess = OBUF_SIZE - (ndpb + 150);
	if( excess > EST_PER_RAD ){

           Base_data_header *bdh = (Base_data_header *) bdataptr;
           short *vel = (short *) (bdataptr + bdh->vel_offset);

           /* Increment the radial count. */
	   Radcount++;

           /* Determine maximum negative and maximum non-negative velocity 
              in the elevation. */
	   BVEL8bit_maxdl( bdataptr, frst_rf, last_rf );

            /* If sufficient space available, pack present radial in the output 
               buffer (packet 16). */
            ndpbyts = RPGC_digital_radial_data_array( (void *) vel, RPGC_SHORT_DATA,
                                                      frst_rf, end_rf, 0,
                                                      numbins, 1, start, delta,
                                                      (void *) &outbuff[pbuffind] );

            /* Update buffer counters and pointers. */
	    ndpb += ndpbyts;
	    pbuffind += ndpbyts / 2;

	}
        else
           LE_send_msg( GL_INFO, "Excess: %d > EST_PER_RAD: %d\n",
                        excess, EST_PER_RAD );

    }

    /* Test for psuedo-end of elev or psuedo-end of volume. */
    if( (radhead->status == PGENDEL) || (radhead->status == PGENDVOL) )
	Proc_rad = 0;

    /* Test for last radial in the elevation cut. */
    if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) ){

	Endelcut = 1;

        /* If last radial encountered, fill remaining fields in product bufger. */
	BVEL8bit_end_of_product_processing( ndpb, numbins, obrptr );

    }

    /* Return to wait for activation. */
    return 0;

/* End of BVEL8bit_product_generation_control() */
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
int BVEL8bit_product_header( char *outbuff, int vol_num, int velreso ){

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = (Symbology_block *) 
                                 (outbuff + sizeof(Graphic_product));

    int elev_ind;
    short params[10];

    /* Fill in product description block fields. */
    RPGC_prod_desc_block( phd, DV_prod_id, vol_num );

    /* Set the product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );
    elev_ind = ORPGPAT_get_elevation_index( DV_prod_id );
    if( elev_ind >= 0 )
        params[elev_ind] = Elmeas;

    RPGC_set_dep_params( outbuff, params );

    /* Data level threshold codes. */
    phd->level_1 = -635*velreso;
    phd->level_2 = 5*velreso;
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

/* End of BVEL8bit_product_header() */
}


#define RNGFOLD		1

/******************************************************************
   
   Description:
      Maximum-data-level routine for 256-level Base Velocity 
      product generation program. 

   Inputs:
      ibatptr - Input buffer pointer.
      frst_vel - index of first velocity bin.
      last_vel - index of last velocity bin.

   Returns:
      Always returns 0.

******************************************************************/
int BVEL8bit_maxdl( char *ibatptr, int frst_vel, int last_vel ){

    int binindx;
    Base_data_header *bhd = (Base_data_header *) ibatptr;

    short *radial = (short *) (ibatptr + bhd->vel_offset); 

    /* Do For All velocity bins ..... */
    for( binindx = frst_vel;  binindx <= last_vel; binindx++ ){

        /* Only look at good velocity (not < SNR threshold and not
           range folded. */
        if( (radial[binindx] > RNGFOLD) 
                         && 
            (radial[binindx] < BASEDATA_INVALID) ){

            if( radial[binindx] < Zero_velocity ){

                /* Find the max negative value. */
                if( radial[binindx] < Mxneg )
                    Mxneg = radial[binindx];

            }
            else{

                /* Find the max non-negative value. */
                if( radial[binindx] > Mxpos ) 
		    Mxpos = radial[binindx];

            }

        }
	     
    }

    return 0;

/* End of BVEL8bit_maxdl() */
} 


/**********************************************************************

   Description:
      Fill remaining Product Header fields for 256-level Base 
      Velocity product. 

   Inputs:
      ndpd - number of bytes in the product so far.
      numbins - number of bins in a product radial.
      outbuff - pointer to output buffer.

   Returns:
      Always returns 0.

**********************************************************************/
int BVEL8bit_end_of_product_processing( int ndpb, int numbins, char *outbuff ){

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

    /* Assign the maximum data level to the product description
       block. */
    maxval = RPGCS_velocity_to_ms( Mxneg );
    phd->param_4 = (short) RPGC_NINT( maxval*MPS_TO_KTS );

    maxval = RPGCS_velocity_to_ms( Mxpos );
    phd->param_5 = (short) RPGC_NINT( maxval*MPS_TO_KTS );

    /* Calculate and store the product message length, the product block 
       length and the product layer length. */

    /* Length of product layer. */
    bytecnt = ndpb + sizeof(Packet_16_hdr_t);
    RPGC_set_product_int( &sym->data_len, bytecnt);

    /* Length of block. */
    bytecnt += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, bytecnt);

    /* Complete the product header. */
    RPGC_prod_hdr( outbuff, DV_prod_id, &bytecnt);

    /* Return to the product generation control routine. */
    return 0;

/* End of BVEL8bit_end_of_product_processing() */
} 

