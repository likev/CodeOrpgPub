/* RCS info */
/* $Author: cmn $ */
/* $Locker:  $ */
/* $Date: 2007/10/23 15:22:17 $ */
/* $Id: itwsdbv.c,v 1.4 2007/10/23 15:22:17 cmn Exp $ */
/* $Revision: 1.4 $ */
/* $State: Exp $ */

#include <itwsdbv.h>

/* Function Prototypes. */
int A30762_product_generation_control(short *idbvptr, int *bdataptr);
int A30763_product_header(char *outbuff, int vol_num, int velresol);
int A30764_maxdl(short *radial, int frst_vel, int last_vel);
int A30765_end_of_product_processing(int ndpb, int numbins, 
                                     int velreso, char *outbuff);
int A30767_slant_max(double elang, double hgtmx, double mxrange);
int A30768_dig_vel_packet(int start, int delta, short *inbuff, 
                          int ix_st_gd, int ix_end_gd, int ix_end, 
                          int *nbytes, char *pbuff_strt, int velresol);

/**********************************************************************

   Description:
      Buffer control routine for ITWS Digital Base Velocity product-generation
      task.  This task generates a 256-level, digital representation of
      the Base Velocity field on a grid of approximately 1 degree by 1 km
      resolution to a maximum range of 115 km on an elevation basis.

   Returns:
      Always returns 0.

**********************************************************************/
int A30761_buffer_control(){

  int ref_flag, wid_flag, vel_flag;
  int *bdataptr, *idbvptr;
  int  opstat;

  opstat = NORMAL;
  Endelcut = 0;
  Proc_rad = 1;


  /* Acquire output buffer for 256 data level ITWS Digital Base Velocity   
     product, 0 to 115 km. Range (1km X 1Deg resolution). */
  idbvptr  = RPGC_get_outbuf_by_name("ITWSDBV", BSIZ87, &opstat);

  if (opstat == NORMAL){

    /* Request first input buffer (Radial Base Data) and prodces it. */
    bdataptr = RPGC_get_inbuf_by_name("COMBBASE", &opstat);

    /* Check for Base Velocity DISABLED */
    if (opstat == NORMAL){

      RPGC_what_moments((Base_data_header *) bdataptr, &ref_flag, 
                         &vel_flag, &wid_flag);
      if (!vel_flag){

        /* Moment DISABLED....Release input buffer and do abort processing. */
        RPGC_rel_inbuf( bdataptr );
        RPGC_rel_outbuf(idbvptr, DESTROY);
        RPGC_abort_because(PROD_DISABLED_MOMENT);
        return 0;

      }

    }

    /* Do For All Input (Base Data) Radials until end of elevation encountered. */
L20:
    if (opstat == NORMAL){

      /* Call the product generation control routine. */
      A30762_product_generation_control((short *) idbvptr, bdataptr);

      /* Release the input radial. */
      RPGC_rel_inbuf(bdataptr);

      if (! Endelcut){

        /* Retrieve next input radial. */
        bdataptr = RPGC_get_inbuf_by_name("COMBBASE", &opstat);
        goto L20;

      } 
      else{

        /* Elevation cut completed. */
        RPGC_rel_outbuf(idbvptr, FORWARD);

      }

    }
    else{

      /* If the input data stream has been canceled for some reason,   
         release and destroy all of the product buffers obtained and     
         return to waiting for input. */
      RPGC_rel_outbuf(idbvptr, DESTROY);

    }

  }
  else{

    /* Do abort processing. */
    RPGC_abort_because( opstat );

  }

  return 0;

/* End of A30761_buffer_control(). */
}

/**********************************************************************

   Description:
      Controls the processing of data for the ITWS Digital Base Velocity 
      product on a radial basis.

   Inputs:
      idbvptr - pointer to digital base velocity data in the product
                buffer.
      bdataptr - pointer to buffer of base data.

   Returns:
      Always returns 0.

**********************************************************************/
int A30762_product_generation_control(short *idbvptr, int *bdataptr){

    /* Initialized data */
    static int bufsiz = BSIZ87;

    /* Local variables */
    int  volnumber, vcpnumber, elevindex, frst_vel, end_vel;
    int tmax, delta, start, excess;
    float elang, coselev;

    Base_data_header *radhead = (Base_data_header *) bdataptr;
    short *radial = (short *) (((char *) bdataptr) + radhead->vel_offset);

    static int pbuffind, ndpb, ndpbyts, last_vel, lastbin;
    static int numbins, velresol;

/* *********************** E X E C U T A B L E  ********************* */

    /* Beginning of product initialization. */
    if ((radhead->status == GOODBVOL) || (radhead->status == GOODBEL)){

        /* Initialize the max data level variables to biased zero. */
        Mxneg = SCALED_ZERO;
        Mxpos = SCALED_ZERO;

        /* Radial count initialization. */
        Radcount = 0;

        /* Buffer index counter and number of digital packet bytes.  
           counter initialization. */
        pbuffind = (sizeof(Graphic_product) + sizeof(Symbology_block)
                    + sizeof(Packet_16_hdr_t))/sizeof(short);
        ndpb = 0;

        /* Initialize the velocity resolution. */
        velresol = radhead->dop_resolution;

        /* Build product header. */

        /* Get elevation angle form volume coverage pattern and elevation   
           index number. */
        volnumber = RPGC_get_buffer_vol_num( bdataptr );
        vcpnumber = RPGC_get_buffer_vcp_num( bdataptr );
        elevindex = RPGC_get_buffer_elev_index( bdataptr );

        Elmeas = (short) RPGCS_get_target_elev_ang( vcpnumber, elevindex );
        elang = Elmeas * 0.1f;

        /* Cosine of elevation angle computation for combination with   
           scale factor for the vertical correlation correction. */
        coselev = cos(elang * DEGTORAD);
        Vc = (short) (coselev * 1000.0);

        /* Compute last bin of radial to process given range and height 
           cutoffs. */
        tmax = A30767_slant_max(elang, HGTMX, MAXBINS);

        numbins = (short) tmax;
        lastbin = (tmax * RADSTEP) - 1;

        /* Pack product header fields. */
        A30763_product_header( (void *) idbvptr, volnumber, velresol);

    } /* End of product initialization. */

    /* Perform individual radial processing if radial not flagged Psuedo 
       End of Elevation/Volume. */
    if( Proc_rad ){


        /* Retrieve start angle and delta angle measurements from the 
           input radial buffer header. */
        start = radhead->start_angle;
        delta = radhead->delta_angle;

        /* Calculate the start and end bin indices of the good data. */
        frst_vel = radhead->dop_range - 1;
        end_vel = frst_vel + radhead->n_dop_bins - 1;
        last_vel = end_vel;
        if( lastbin < end_vel )
            last_vel = lastbin;

        /* Increment the radial count. */
        Radcount++;

        /* Determine maximum positive and maximum negative velocities 
           in the elevation through the present radial. */
        A30764_maxdl(radial, frst_vel, last_vel);

        /* Calculate remaining words available in output buffer. */
        excess = bufsiz - (ndpb + 150) / 4;
        if (excess > EST_PER_RAD){

            /* If sufficient space available, pack present radial in the 
               output buffer in byte-compacted format (see ICD 3-11C). */
            A30768_dig_vel_packet(start, delta, radial, frst_vel, end_vel, 
                                  lastbin, &ndpbyts, (char *) &idbvptr[pbuffind], 
                                  velresol);

            /* Update buffer counters and pointers. */
            ndpb += ndpbyts;
            pbuffind += (ndpbyts+1)/sizeof(short);

        }

    }

    if( (radhead->status == PGENDVOL) || (radhead->status == PGENDEL) )
        Proc_rad = 0;

    /* Test for last radial in the elevation cut. */
    if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) ){

        Endelcut = 1;

        /* If last radial encountered, fill remaining fields in product buffer. */
        A30765_end_of_product_processing(ndpb, numbins, velresol, (void *) idbvptr);

    }

    return 0;

/* End of A30762_product_generation_control(). */
} 


/**********************************************************************

   Description:
      Fill Product Header fields for ITWS Digital Base Velocity product.

   Inputs:
      outbuff - pointer to output buffer.
      vol_num - volume scan number.
      velresol - velocity resolution, from radial header.

   Returns:
      Always returns 0.

**********************************************************************/
int A30763_product_header(char *outbuff, int vol_num, int velresol){


    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = (Symbology_block *) 
                                 (outbuff + sizeof(Graphic_product));

    int elev_ind;
    short params[10];

    /* Fill in product description block fields. */
    RPGC_prod_desc_block( phd, ITWS_prod_id, vol_num );

    /* Set the product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );
    elev_ind = ORPGPAT_get_elevation_index( ITWS_prod_id );
    if( elev_ind >= 0 )
        params[elev_ind] = Elmeas;

    RPGC_set_dep_params( outbuff, params );

    /* Data level threshold codes. */
    phd->level_1 = -635;
    phd->level_2 = 5;
    phd->level_3 = 256;

    /* Store Doppler velocity resolution code (1=0.5 m/s, 2=1.0 m/s) */
    phd->param_7 = velresol;

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

/* End of A30763_product_header(). */
}

/**********************************************************************

   Description:
      Maximum-data-level routine for ITWS Digital Base Velocity product
      generation program.

   Inputs:
      bdataptr - Input buffer of preprocessed radial velocity data.
      frst_vel - Index of the first good velocity data.
      last_vel - Index of the last good velocity data.

   Returns:
      Always returns 0.

**********************************************************************/
int A30764_maxdl(short *radial, int frst_vel, int last_vel){

    /* Local variables */
    static int binindx;

    /* Determine maximum positive and maximum negative velocity   
       in biased, internal units. */
    for (binindx = 0; RADSTEP < 0 ? binindx >= last_vel : binindx <= last_vel;
        binindx += RADSTEP){
 
        if (binindx >= frst_vel){

            if( (radial[binindx] > BASEDATA_RDRNGF) 
                                 &&
                (radial[binindx] < BASEDATA_INVALID) ){

                if (radial[binindx] < SCALED_ZERO){

                    if (radial[binindx] < Mxneg) 
                        Mxneg = radial[binindx];
           
                }
                else{

                    if (radial[binindx] > Mxpos) 
                        Mxpos = radial[binindx];
           
                }

            }

        }

    } /* End of for(binindx ..... ) loop. */

  return 0;

/* End of A30764_maxdl(). */
}

/**********************************************************************

   Description:

   Inputs:
      ndpd - number of bytes in the product so far.
      numbins - number of bins in a product radial.
      velreso - velocity resolution, from radial header.
      outbuff - pointer to output buffer.

   Returns:
      Always returns 0.

**********************************************************************/
int A30765_end_of_product_processing(int ndpb, int numbins, 
                                     int velreso, char *outbuff){

    int bytecnt;
    float maxval;
    RESO reso;

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
    reso = RPGCS_set_velocity_reso( velreso );
    maxval = RPGCS_velocity_to_ms( Mxneg );
    phd->param_4 = (short) RPGC_NINT( maxval*MPS_TO_KTS );
    if( (reso == LOW_RES) && (phd->param_4 < NEG_VEL_CAP) )
        phd->param_4 = NEG_VEL_CAP;

    maxval = RPGCS_velocity_to_ms( Mxpos );
    phd->param_5 = (short) RPGC_NINT( maxval*MPS_TO_KTS );
    if( (reso == LOW_RES) && (phd->param_5 > POS_VEL_CAP) )
        phd->param_5 = POS_VEL_CAP;

    /* Calculate and store the product message length, the product block 
       length and the product layer length. */

    /* Length of product layer. */
    bytecnt = ndpb + sizeof(Packet_16_hdr_t);
    RPGC_set_product_int( &sym->data_len, bytecnt);

    /* Length of block. */
    bytecnt += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, bytecnt);

    /* Complete the product header. */
    RPGC_prod_hdr( outbuff, ITWS_prod_id, &bytecnt);

    /* Return to the product generation control routine. */
    return 0;

/* End of a30765_end_of_product_processing() */
}

/************************************************************************

   Description:
      This function computes the radar slant range as determined for a
      specified elevation angle.  The computed slant range is limited by a
      specified height above the flat earth plane and by a range cutoff,
      both of which are passed in from the calling routine.  The returned
      value of the function is the number of bins to process based on an
      assumption of 1km effective range resolution.
 
   Inputs:
      elang - Radar elevation angle (degrees)
      hgtmx - Height maximum above a flat-earth plane (km) equivalent to
              18,000 ft.
      mxrange - Maximum range for output product (km).

   Returns:
      Always returns 0.

************************************************************************/
int A30767_slant_max(double elang, double hgtmx, double mxrange){

    /* Local variables */
    float fmax;
    int smax;

    /* Compute slant range, first compute flat range. */
    fmax = hgtmx / tanf( (float) (elang * DEGTORAD));
    smax = RPGC_NINT(sqrtf(powf(hgtmx, 2.0) + powf(fmax, 2.0)));

    /* Compare to range maximum, replace if larger. */
    if (smax > mxrange)
        smax = mxrange;

    return smax;

/* End of A30767_slant_max() */
}

/************************************************************************

   Description:
      Encode an output buffer of data a per opcode=16 format described
      in RPG to Class 1 ICD.
 
   Inputs:
      start - start angle of radial, in deg*10.
      delta - delta angle of radial, in deg*10.
      inbuff - input data.
      ix_st_gd - index to the start of good velocities in inbuff.
      ix_end_gd - index to the end of good velocities in inbuff.
      ix_end - index to end of inout buffer.
      velresol - velocity resolution.

   Outputs:
      nbytes - number of encoded bytes in output buffer.
      pbuff_strt - pointer to start of output buffer.

   Returns:

************************************************************************/
int A30768_dig_vel_packet(int start, int delta, short *inbuff, 
                          int ix_st_gd, int ix_end_gd, int ix_end, 
                          int *nbytes, char *pbuff_strt, int velresol){

    /* Local variables */
    int numbins, pix_value, bin;

    static char *data = NULL;

    /* Initialize data for packing of good data. */
    *nbytes = 0;

    /* malloc space for temporary data buffer. */
    if (data == NULL){

         data = (char *) calloc( 1, BASEDATA_VEL_SIZE );
         if (data == NULL){

            RPGC_log_msg( GL_INFO, "calloc Failed For %d Bytes.\n",
                          BASEDATA_VEL_SIZE );

            *nbytes = 0;
            return 0;
 
         }

    }
             
    /* Initialize the number of bins to 0. */
    numbins = 0;

    /* Initialize the memory used to store the data. */
    memset( data, 0, BASEDATA_VEL_SIZE );

    /* Process all the data designated as good data in radial   
       with test for velocity resolution. */
    if(velresol == 1) {

        /* Base velocity resolution = 0.5 m/s. */
        for (bin = 0; bin <= ix_end; bin += RADSTEP){

           /* Outsize good range. */
           if ( (bin < ix_st_gd) || (bin > ix_end_gd) )
               pix_value = 0;

           /* Intermediate pixel. */
           else if ((inbuff[bin] >= 256) || (inbuff[bin] < 0))
               pix_value = 0;
           else 
               pix_value = inbuff[bin];

           data[numbins] = pix_value;
           numbins++;

        }

    } else {

        /* Base velocity resolution = 1.0 m/s. */
        for (bin = 0; bin <= ix_end; bin += RADSTEP) {

            /* Outside good range. */
            if ( (bin < ix_st_gd) || (bin > ix_end_gd) )
                pix_value = 0;

            /* Intermediate pixel. */
            else if ((inbuff[bin] >= 256) || (inbuff[bin] < 0))
                pix_value = 0;

            else if (inbuff[bin] <= 1) 
                pix_value = inbuff[bin];

            else{

                pix_value = ((inbuff[bin] - SCALED_ZERO) << 1) + SCALED_ZERO;
                if (pix_value < 2) 
                    pix_value = 2;
    
                else if (pix_value > 255) 
                    pix_value = 255;

            }

            data[numbins] = pix_value;
            numbins++;

        }

    }

    /* The number of bytes returned by RPGP_set_packet_16_radial does not 
       include the packet 16 radial header. */
    *nbytes = RPGP_set_packet_16_radial (pbuff_strt, start, delta, data, 
                                         numbins);

    /* Add in the size of the packet 16 radial header. */
    *nbytes += (sizeof(Packet_16_data_t) - sizeof(short));

    /* Completed packet encoding processing for this buffer. */
    return 0;

/* End of A30768_dig_vel_packet(). */
} 

