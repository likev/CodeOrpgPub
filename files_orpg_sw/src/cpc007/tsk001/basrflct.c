/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2011/12/02 19:24:22 $ */
/* $Id: basrflct.c,v 1.5 2011/12/02 19:24:22 ccalvert Exp $ */
/* $Revision: 1.5 $ */
/* $State: Exp $ */

#include <basrflct.h>

static int Rfspt;		/* Beginning of Reflectivity data. */

static int Rf230end;		/* End of Reflectivity data. */

static int Rf460end;		/* End of Reflectivity data. */

static short *Comprad = NULL;	/* Compact Reflectivity data. */

/* Function Prototypes. */
static void Assign_brefptrs( int pindx );
static void Release_prodbuf( int relstat );
static void Product_generation_control( char *bdataptr );
static void Product_header( int pindx, char *radial );
static void Format_prod( int start, int delta, char *radial );
static void Compact_ref( Moment_t *ref );
static void Compact_ref_again( );
static void End_of_product_processing( int pindx );

/*\/////////////////////////////////////////////////////////////////

   Description:
      Buffer control routine for Base Reflectivity product. 

/////////////////////////////////////////////////////////////////\*/
void A30711_buffer_control(){

    int abortit, ref_flag, wid_flag, vel_flag;
    int no_mem_abort, pindx, opstat, relstat;
    char *bdataptr = NULL;

    opstat = 0;
    Endelcut = 0;
    relstat = FORWARD;

    /* Initialize the product information. */
    for( pindx = 0; pindx < NUMPRODS; ++pindx ){

	Prod_info[pindx].pflag = 0;
	Prod_info[pindx].no_mem_flag = 0;
        Prod_info[pindx].brefptrs = NULL;
        Prod_info[pindx].radcount = 0;

    }

    /* Request all possible output buffers to see at what ranges, 
       resolutions, and data levesl base reflectivity maps need 
       to be generated. */
    pindx = 0;

    /* Product code 16, 0 to 230 km. radius (1x1 km. resolution), 
       8 data level Base Reflectivity product buffer. */
    Assign_brefptrs( pindx );
    ++pindx;

    /* Product code 19, 0 to 230 km. radius (1x1 km. resolution), 
       16 data level Base Reflectivity product buffer. */
    Assign_brefptrs( pindx );
    ++pindx;

    /* Product code 17, 0 to 460 km. radius (2x2 km. resolution),
       8 data level Base Reflectivity product buffer. */
    Assign_brefptrs( pindx );
    ++pindx;

    /* Product code 20, 0 to 460 km. radius (2x2 km. resolution), 
       16 data level Base Reflectivity product buffer. */
    Assign_brefptrs( pindx );
    ++pindx;

    /* Product code 18, 0 to 460 km. radius (4x4 km. resolution), 
       8 data level Base Reflectivity product buffer. */
    Assign_brefptrs( pindx );
    ++pindx;

    /* Product code 21, 0 to 460 km. radius (4x4 km. resolution), 
       16 data level Base Reflectivity product buffer. */
    Assign_brefptrs( pindx );

    /* Determine if anything to do, if not abort the whole thing. */
    abortit = 1;
    no_mem_abort = 1;
    for( pindx = 0; pindx < NUMPRODS; ++pindx ){

        if(Prod_info[pindx].pflag ) 
	    abortit = 0;
	 
	if( !Prod_info[pindx].no_mem_flag ) 
	    no_mem_abort = 0;
	 
    }

    if( !abortit ){

        /* Request input radial buffers and process them. */

        /* Do Until opstat != NORMAL or ENDELCUT */
	bdataptr = RPGC_get_inbuf_by_name( "BASEDATA", &opstat );

        /* If the input data stream has been cancelled for some reason,  
           then release and destroy all of the product buffers obtained,
           abort and wait for activation. */
	if( opstat == NORMAL ){

            /* Check for Reflectivity disabled. */
	    RPGC_what_moments( (void *) bdataptr, &ref_flag, &vel_flag, &wid_flag );
	    if( !ref_flag ){

                /* Moment disabled....release buffers and abort. */
		RPGC_rel_inbuf( bdataptr );
		Release_prodbuf( DESTROY );
		RPGC_abort_because( PROD_DISABLED_MOMENT );
		return;

	    }

	}

        /* Call the product generation control routine. */
        while(1){

	    if( opstat == NORMAL ){

	        Product_generation_control( bdataptr );

                /* Release the input radial. */
	        RPGC_rel_inbuf( bdataptr );

                /* Test for end of the while loop. */
	        if( !Endelcut ){

		    bdataptr = RPGC_get_inbuf_by_name( "BASEDATA", &opstat );
		    continue;

	        }

	    }

	    if( opstat == NORMAL ) 
                relstat = FORWARD;

	    else 
                relstat = DESTROY;
	 
            Release_prodbuf( relstat );
            break;

        }

        /* Abort and return to waiting for activation. */
    }
    else{

	if( no_mem_abort )
	    RPGC_abort_because( PROD_MEM_SHED );

	else
	    RPGC_abort();
	 
    }

    return;

/* End of A30711_buffer_control() */
}

/************************************************************************

   Description:
      Function to acquire output buffer for product request.

   Inputs:
     pindx - product index

   Returns:
      Void function

************************************************************************/
static void Assign_brefptrs( int pindx ){

  int opstat = NORMAL;

  Prod_info[pindx].brefptrs = RPGC_get_outbuf_by_name( Prod_info[pindx].pname,
                                                       Prod_info[pindx].psize,
                                                       &opstat );
  if (opstat == NORMAL)
    Prod_info[pindx].pflag = 1;
   
  else if (opstat == NO_MEM){

    /* If no memory for product buffer, abort datatype. */
    RPGC_abort_dataname_because( Prod_info[pindx].pname, PROD_MEM_SHED );
    Prod_info[pindx].no_mem_flag = 1;

  }

/* End of Assign_brefptrs(). */
}


/*\////////////////////////////////////////////////////////////////////

   Description:
     Routine to release the product output buffer, and either 
     forward or destroy the buffer upon release, for the Base 
     Reflectivity mapping program. 

   Inputs:
      relstat - Release disposition.

////////////////////////////////////////////////////////////////////\*/
static void Release_prodbuf( int relstat ){

    int pindx;

    if( relstat == DESTROY ){

        /* DESTROY disposition ..... */
	for( pindx = 0; pindx < NUMPRODS; ++pindx ){

	    if( Prod_info[pindx].pflag ){

		RPGC_rel_outbuf( Prod_info[pindx].brefptrs, DESTROY );
                Prod_info[pindx].brefptrs = NULL;

            }

	}

    } 
    else{

        /* FORWARD disposition ..... */
        for( pindx = 0; pindx < NUMPRODS; ++pindx ){

	    if( Prod_info[pindx].pflag ){

		RPGC_rel_outbuf( Prod_info[pindx].brefptrs, FORWARD );
                Prod_info[pindx].brefptrs = NULL;

            }

	}

    }

    return;

/* End of Release_prodbuf(). */
} 

/*\///////////////////////////////////////////////////////////////////

   Description:
      Product Generation Control routine for BASE REFLECTIVITY 
      mapping program. 

   Inputs:
      radial - input radial buffer.

///////////////////////////////////////////////////////////////////\*/
static void Product_generation_control( char *radial ){

    /* Local variables */
    int delta, pindx, start, vcpnumber, elevindex;
    int radind, end_rf;
    float coselev, elang;

    Base_data_header *hdr = (Base_data_header *) radial;
    Moment_t *ref = (Moment_t *) (radial + hdr->ref_offset);

    /* Beginning of product initialization. */
    if( (hdr->status == GOODBEL) || (hdr->status == GOODBVOL) ){

        /* Maximum data level initialization. */
	Maxdl230 = 0;
	Maxdl460 = 0;

        /* Buffer index counter and number of run-length encoded bytes   
           counter initialization for all products. */
	for( pindx = 0; pindx < NUMPRODS; ++pindx) {

            Prod_info[pindx].pbuffind = 
                   (sizeof(Graphic_product) + sizeof(Symbology_block)
                   + sizeof(packet_af1f_hdr_t))/sizeof(short);
            Prod_info[pindx].nrleb = 0;
            Prod_info[pindx].radcount = 0;

	}

        /* Initialize color look-up and threshold table indicies. 
           Set up precipitation color tables/thresholds. */
	if( hdr->weather_mode == PRECIPITATION_MODE ){

	    Prod_info[PROD16].clthtind = REFNC8;
	    Prod_info[PROD17].clthtind = REFNC8;
	    Prod_info[PROD18].clthtind = REFNC8;
	    Prod_info[PROD19].clthtind = REFNC16;
	    Prod_info[PROD20].clthtind = REFNC16;
	    Prod_info[PROD21].clthtind = REFNC16;

        /* Set up Clear Air color tables/thresholds. */
	}else {

	    Prod_info[PROD16].clthtind = REFCL8;
	    Prod_info[PROD17].clthtind = REFCL8;
	    Prod_info[PROD18].clthtind = REFCL8;
	    Prod_info[PROD19].clthtind = REFCL16;
	    Prod_info[PROD20].clthtind = REFCL16;
	    Prod_info[PROD21].clthtind = REFCL16;

	}

        /* Save the Calibration Constant.   Needed for the Product
           Description block. */
        Calib_const = hdr->calib_const;

        /* Build all necessary product headers. */
	for( pindx = 0; pindx < NUMPRODS; ++pindx ){

	    if( Prod_info[pindx].pflag ){

                /* Numbins contains the number of bins in the product
                   which is the minimum of MAXBINS and 70 kft. */
		Prod_info[pindx].numbins = RPGC_num_rad_bins( radial, 
                                                   Prod_info[pindx].maxbins, 
                                                   Prod_info[pindx].radstep, 1 );
		Product_header( pindx, radial );

	    }

	}

        /* Compute scale factors. */ 
        vcpnumber = RPGC_get_buffer_vcp_num( radial );
        elevindex = RPGC_get_buffer_elev_index( radial );

        /* Get elevation angle from volume coverage pattern and elevation   
           index number. */
	Elmeas = (short) RPGCS_get_target_elev_ang( vcpnumber, elevindex );
	elang = Elmeas * .1f;
	coselev = cosf( elang * DEGTORAD );
	Vc1 = coselev * 1e3f;
	Vc2 = coselev * 2e3f;

    }

    /* Retrieve start angle and delta angle measurements from the input
       radial buffer for this radial.  */
    start = hdr->start_angle;
    delta = hdr->delta_angle;

    /* Initialize the pointers to the beginning and to the end of the 
       good bins of data in the input radial buffer. */
    Rfspt = hdr->surv_range - 1;
    Rf230end = MAXB230 - 1;
    end_rf = Rfspt + hdr->n_surv_bins - 1;
    if( end_rf < MAXB230 )
        Rf230end = end_rf;
	
    Rf460end = MAXB460 - 1;
    if( end_rf < Rf460end ) 
        Rf460end = end_rf;
	
    /* Format the radial for all products requested. */
    Format_prod( start, delta, radial );

    /* Determine if we need to calculate the 1-230 max data level. */
    if( (Prod_info[PROD16].pflag || Prod_info[PROD19].pflag) 
                                 && 
        ((!Prod_info[PROD17].pflag && !Prod_info[PROD18].pflag) 
                                 && 
         (!Prod_info[PROD20].pflag && !Prod_info[PROD21].pflag)) ){

	for( radind = Rfspt; radind <= Rf230end; ++radind ){

            if( ref[radind] > Maxdl230 ) 
	        Maxdl230 = radial[radind];

	}

    }

    /* Test for the last radial in the elevation cut. */
    if( (hdr->status == GENDEL) || (hdr->status == GENDVOL) ){

	Endelcut = 1;

        /* Do For All products requested. */
	for( pindx = 0; pindx < NUMPRODS; ++pindx ){

            if( Prod_info[pindx].pflag ) 
		End_of_product_processing( pindx );

	}

    }

    return;

/* End of Product_generation_control(). */
} 


/*\/////////////////////////////////////////////////////////////////

   Description:
      Product Header production routine for the Base Reflectivity
      product.      

   Inputs:
      pindx - Product index.
      radial - Input radial buffer.
     
/////////////////////////////////////////////////////////////////\*/
static void Product_header( int pindx, char *radial ){

    /* Local variables */
    int volno, trshind, offset;

    short *outbuff = (short *) Prod_info[pindx].brefptrs;

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym =
        (Symbology_block *) (((char *) outbuff) + sizeof(Graphic_product));
    packet_af1f_hdr_t *packet_af1f =
        (packet_af1f_hdr_t *) (((char *) outbuff) +
                        sizeof(Graphic_product) + sizeof(Symbology_block));

    /* Initialize the product header and description block. */
    memset( Prod_info[pindx].brefptrs, 0, sizeof(Graphic_product) );

    /* Fill out the product description block. */
    volno = RPGC_get_buffer_vol_num( radial );
    RPGC_prod_desc_block( outbuff, Prod_info[pindx].prod_id, volno );

    /* Data level threshold codes. */
    trshind = Prod_info[pindx].clthtind;
    phd->level_1 = Color_data.thresh[trshind][0];
    phd->level_2 = Color_data.thresh[trshind][1];
    phd->level_3 = Color_data.thresh[trshind][2];
    phd->level_4 = Color_data.thresh[trshind][3];
    phd->level_5 = Color_data.thresh[trshind][4];
    phd->level_6 = Color_data.thresh[trshind][5];
    phd->level_7 = Color_data.thresh[trshind][6];
    phd->level_8 = Color_data.thresh[trshind][7];
    phd->level_9 = Color_data.thresh[trshind][8];
    phd->level_10 = Color_data.thresh[trshind][9];
    phd->level_11 = Color_data.thresh[trshind][10];
    phd->level_12 = Color_data.thresh[trshind][11];
    phd->level_13 = Color_data.thresh[trshind][12];
    phd->level_14 = Color_data.thresh[trshind][13];
    phd->level_15 = Color_data.thresh[trshind][14];
    phd->level_16 = Color_data.thresh[trshind][15];

    /* Store offset to Symbology block. */
    offset = sizeof(Graphic_product)/sizeof(short);
    RPGC_set_prod_block_offsets( outbuff, offset, 0, 0 );

    /* Product block divider, block ID, number of layers and layer  
       divider. */
    sym->divider = -1;
    sym->block_id = 1;
    sym->n_layers = 1;
    sym->layer_divider = -1;

    /* Build the radial header for this product:
       Store operation code, range to first bins, number of bins   
       and I, J center of sweep. */
    packet_af1f->code = 44831;
    packet_af1f->index_first_range = 0;
    packet_af1f->num_range_bins = Prod_info[pindx].numbins;
    packet_af1f->i_center = 256;
    packet_af1f->j_center = 280;

    return;

/* End of Product_header(). */
}


/*\////////////////////////////////////////////////////////////////////////

   Description:
      This routine will handle the calls to the common  module for 
      RLE for all the Base Reflectivity products requested.  It 
      calculates the start and end index for the radial in question;
      it also calculates the number of preset runs of zero that can 
      be added to the front and back of the radial by then run-length 
      encoding process based on the start and end of the good data bins 
      in the radial. 

   Inputs:
      start - Start angle, in deg*10.
      delta - Delta angle, in deg*10.
      radial - Input radial buffer.

////////////////////////////////////////////////////////////////////////\*/
static void Format_prod( int start, int delta, char *radial ){

    int strt_bin, end_bin, excess, rlebyts, i;
    Base_data_header *hdr = (Base_data_header *) radial;
    Moment_t *ref = (Moment_t *) (radial + hdr->ref_offset );

    /* Allocate space for Comprad. */
    if( Comprad == NULL ){

        Comprad = (short *) calloc( 1, 230*sizeof(short) );
        if( Comprad == NULL ){

            RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                          230*sizeof(short) );
            RPGC_abort_task();

        }

    }

    /* Process the products that require no compaction first. */
    if( Prod_info[PROD16].pflag || Prod_info[PROD19].pflag ){

        for( i = PROD16; i <= PROD19; ++i ){

            int clthtind = Prod_info[i].clthtind;

	    if( Prod_info[i].pflag ){

                /* Handle the possibility of NUMBINS(I) being 
                   less than the number of bins in radial. */
		end_bin = Rf230end;
		if( end_bin > (Prod_info[i].numbins - 1) ) 
		    end_bin = Prod_info[i].numbins - 1;
		 
                /* Calculate number of words left in output buffer. */
		excess = Prod_info[i].psize - (Prod_info[i].nrleb + 150);
		if( excess > Prod_info[i].est_per_rad ){

                    /* Increment the radial count. */
                    Prod_info[i].radcount++;

		    RPGC_run_length_encode( start, delta, (void *) ref, Rfspt, end_bin, 
                                            Prod_info[i].numbins, 1, 
			                    &Color_data.coldat[clthtind][0], 
                                            &rlebyts, Prod_info[i].pbuffind, 
                                            (short *) Prod_info[i].brefptrs );

                    /* Update counters. */
		    Prod_info[i].nrleb += rlebyts;
		    Prod_info[i].pbuffind += rlebyts / 2;

		}

            }

	}

    }

    /* Now process the products that need compaction. */
    if( (Prod_info[PROD17].pflag || Prod_info[PROD20].pflag) 
                                 || 
        (Prod_info[PROD18].pflag || Prod_info[PROD21].pflag) ){

        /* Compact the data to a 2x2 resolution. */
	Compact_ref( ref );

        /* Process the products 17 and 20. */
        /* Calculate start and end of good data in compacted array. */
	strt_bin = Rfspt / 2;
        if( Rf460end > 0 )
	    end_bin = Rf460end / 2;

        else
            end_bin = (Rf460end - 1) / 2;
        
	for( i = PROD17; i <= PROD20; ++i ){

            int clthtind = Prod_info[i].clthtind;

	    if( Prod_info[i].pflag ){

                /* Handle the possibility of NUMBINS(I) being less than 
                   the number of bins in radial. */
		if( end_bin > (Prod_info[i].numbins - 1) ) 
		    end_bin = Prod_info[i].numbins - 1;

                /* Calculate number of words left in output buffer */
		excess = Prod_info[i].psize - (Prod_info[i].nrleb + 150);
		if( excess > Prod_info[i].est_per_rad ){

                    /* Increment the radial count. */
                    Prod_info[i].radcount++;

		    RPGC_run_length_encode( start, delta, Comprad, strt_bin, 
                                            end_bin, Prod_info[i].numbins, 1, 
			                    &Color_data.coldat[clthtind][0], 
			                    &rlebyts, Prod_info[i].pbuffind, 
                                            (short *) Prod_info[i].brefptrs );

                    /* Update counters */
		    Prod_info[i].nrleb += rlebyts;
		    Prod_info[i].pbuffind += rlebyts / 2;

		}

	    }

	}

        /* Now process the products that require 4x4 resolution */
	if( Prod_info[PROD18].pflag || Prod_info[PROD21].pflag ){

            /* Compact the radial once more by taking highest of every two. */
	    Compact_ref_again( );

            /* Calculate the start and end of the good bins of data. */
	    strt_bin = strt_bin  / 2;
            if( end_bin > 0 )
	        end_bin = end_bin / 2;

            else
	        end_bin = (end_bin - 1) / 2;

	    for( i = PROD18; i <= PROD21; ++i ){

		if( Prod_info[i].pflag ){

                    int clthtind = Prod_info[i].clthtind;

                    /* Handle the possibility of NUMBINS(I) being 
                       less than the number of bins in radial. */
		    if( end_bin > (Prod_info[i].numbins - 1) )
			end_bin = Prod_info[i].numbins - 1;
		    
                    /* Calculate number of words left in output buffer. */
		    excess = Prod_info[i].psize - (Prod_info[i].nrleb + 150);
		    if( excess > Prod_info[i].est_per_rad ){

                        /* Increment the radial count. */
                        Prod_info[i].radcount++;

			RPGC_run_length_encode( start, delta, Comprad, strt_bin, 
                                                end_bin, Prod_info[i].numbins, 1, 
				                &Color_data.coldat[clthtind][0], 
                                                &rlebyts, Prod_info[i].pbuffind, 
                                                (short *) Prod_info[i].brefptrs );

                        /* Update counters. */
			Prod_info[i].nrleb += rlebyts;
			Prod_info[i].pbuffind += rlebyts / 2;

		    }

		}

	    }

	}

    }

    return;

/* End of Format_prod(). */
} 

/*\//////////////////////////////////////////////////////////////////////

   Description:
      Performs bin-compaction and maximum-data-level out to 230KM 
      range for Base Reflectivity mapping product. 

   Inputs:
      ref - Reflectivity data.
      
//////////////////////////////////////////////////////////////////////\*/
static void Compact_ref( Moment_t *ref ){

    /* Local variables */
    int radind, max460m1, buffind;

    max460m1 = MAXB460 - 1;
    buffind = 0;

    for( radind = 0; radind < max460m1; radind += 2 ){

        /* Code added to ensure index does not "point" beyond 
           end of data.  All data past end of radial is set to
           "below threshold". */
        if( (radind > Rf460end) || ((radind+1) > Rf460end) )
           Comprad[buffind] = BASEDATA_RDBLTH;

	else if( ref[radind] > ref[radind + 1] ){

	    if( ref[radind] != BASEDATA_INVALID ) 
		Comprad[buffind] = ref[radind];

	    else 
		Comprad[buffind] = ref[radind + 1];
	    
	} 
        else{

            /* The second value is the larger or equal. */
	    if( ref[radind + 1] != BASEDATA_INVALID ) 
		Comprad[buffind] = ref[radind + 1];

	    else 
		Comprad[buffind] = ref[radind];
	    
	}

        /* Perform maximum data level testing. */
	if( (radind >= Rfspt) || (radind + 1 >= Rfspt) ){

	    if( radind <= Rf230end ){

		if( Comprad[buffind] > Maxdl230 ) 
		    Maxdl230 = Comprad[buffind];
		 
		if( Comprad[buffind] > Maxdl460 ) 
		    Maxdl460 = Comprad[buffind];
		 
	    } 
            else if( (Comprad[buffind] > Maxdl460)
                                && 
                     (radind <= Rf460end) )
		Maxdl460 = Comprad[buffind];
	    
	}

        /* Bump the temporary radial index. */
	++buffind;

    }

    return;

/* End of Compact_ref(). */
} 

/*\///////////////////////////////////////////////////////////////////

   Description:
      This module reduces the resolution of the reflectivity data 
      from a 2x2 buffer to a 4x4 buffer by taking the greater of 
      every two bins of data. 

///////////////////////////////////////////////////////////////////\*/
static void Compact_ref_again( ){

    int radind, buffind;

    buffind = 0;
    for( radind = 0; radind < MAXB230-1; radind += 2 ){

	if( Comprad[radind] > Comprad[radind + 1] ) 
	    Comprad[buffind] = Comprad[radind];

	else 
	    Comprad[buffind] = Comprad[radind + 1];

	++buffind;

    }

    return;

/* End of Compact_ref_again(). */
} 

/*\/////////////////////////////////////////////////////////////////
 
   Description:
      Performs end-of-product processing for each product. 

   Inputs:
      pindx - Product index.

/////////////////////////////////////////////////////////////////\*/
static void End_of_product_processing( int pindx ){

    /* Local variables */
    int bytecnt;
    float maxval;
    short params[10];

    short *outbuff = (short *) Prod_info[pindx].brefptrs;

    packet_af1f_hdr_t *packet_af1f = (packet_af1f_hdr_t *) (((char *) outbuff) +
                                      sizeof(Graphic_product) +
                                      sizeof(Symbology_block));
    Symbology_block *sym = (Symbology_block *) (((char *) outbuff) +
                            sizeof(Graphic_product));

    /* Initialize the product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );

    /* Assign the elevation angle measurement. */
    params[2] = Elmeas;

    /* Assign the scale factor. */
    if( (pindx == PROD18) || (pindx == PROD21) )
	packet_af1f->scale_factor = Vc2;

    else
	packet_af1f->scale_factor = Vc1;

    /* Assign the maximum data level for this product. */
    if( (pindx == PROD16) || (pindx == PROD19) ){

	maxval = RPGCS_reflectivity_to_dBZ( Maxdl230 );
	params[3] = (short) RPGC_NINT( maxval );

    } 
    else {

	maxval = RPGCS_reflectivity_to_dBZ( Maxdl460 );
	params[3] = (short) RPGC_NINT( maxval );

    }

    /* Store the calibration constant in the parameter list. */
    RPGC_set_product_float( &params[7], Calib_const );
 
    /* Set the product dependent parameters. */
    RPGC_set_dep_params( outbuff, params );

    /* Assign the radial count to the display header. */
    packet_af1f->num_radials = Prod_info[pindx].radcount;

    /* Calculate and store the product message length, the   
       product block length and the product layer length. */

    /* Length of product layer. */
    bytecnt = Prod_info[pindx].nrleb +  sizeof(packet_af1f_hdr_t);
    RPGC_set_product_int( &sym->data_len, bytecnt );

    /* Length of block. */
    bytecnt += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, bytecnt );

    /* Length of message. */
    RPGC_prod_hdr( outbuff, Prod_info[pindx].prod_id, &bytecnt );

    return;

/* End of End_of_product_processing(). */
} 

