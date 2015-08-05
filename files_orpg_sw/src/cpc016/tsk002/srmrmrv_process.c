/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/08/28 21:12:01 $ */
/* $Id: srmrmrv_process.c,v 1.3 2009/08/28 21:12:01 steves Exp $ */
/* $Revision: 1.3 $ */
/* $State: Exp $ */

#include <srmrmrv.h>

/* Macro definitions. */
#define EST_RADIAL_RLE		115

/* Function prototypes. */
static void Srmrmrv_lookup( int ip, float azm, short *smrv );
static void Srmrmrv_header( char *bufout, int ctlidx );
static void Remove_stm_mtn( short *iv, int lastbin, int firstbin, short *smrv, 
                            short *srv, Tbufctl_t *tbufctl );

/*\//////////////////////////////////////////////////////////////////////

   Description:
      This module controls the removal of the specified storm motion 
      from the radial velocity field.  A check is made to make sure 
      the run-length-encoding of the current radial will not cause 
      the product buffer to exceed it's allocated limits. If it 
      appears that the limit will be exceeded, the product is 
      immmediatly released. 

   Inputs:
      azm - Azimuth angle of radial (deg).
      hdr - Base data header.
      idelta - Radial delta angle (deg*10).
      istart - Radial start angle (deg*10). 

///////////////////////////////////////////////////////////////////////\*/
void Srmrmrv_process( float azm, Base_data_header *hdr, int idelta, 
                      int istart ){

    /* Local variables */
    int ip, maxgoodbin, numbufel, firstbin;
    int bufstp, endindx, lastbin, numrleb, strindx;
    short *vel = (short *) ((char *) hdr + hdr->vel_offset);

    static short srv[231], smrv[MAXSRMRV][257];	

    maxgoodbin = hdr->dop_range + hdr->n_dop_bins - 1;

    /* Do for all products .... */
    for( ip = 0; ip < Tnumprod; ++ip ){

        /* Do only if this product neededs the current radial. */
	if( Tbufctl[ip].need == 1 ){

            /* Check if the run length encoding of this radial will 
               cause the product output buffer to overflow, if it will, 
               then don't process it and release the product immmediatly */
	    if( Tbufctl[ip].bufcnt + EST_RADIAL_RLE > Tbufctl[ip].maxi2_in_obuf ) {

                /* This product done with the radial, set NEED flag to NO. */
		Tbufctl[ip].need = 0;
		Srmrmrv_release_prod( ip, FORWARD );

	    } 
            else{

                /* Else.. the run length encoding will fit in the product 
                   buffer, so carry on. */

                /* * Build the lookup table used to remove the storm 
                     motion from the radial velocity in this radial. */
		Srmrmrv_lookup( ip, azm, &smrv[ip][0] );

                /* Find the first and last bin values to process. */
		if( maxgoodbin < Tbufctl[ip].max_bin ) 
		    lastbin = maxgoodbin - 1;

		else 
		    lastbin = Tbufctl[ip].max_bin - 1;
		

		if( hdr->dop_range > Tbufctl[ip].min_bin ) 
		    firstbin = hdr->dop_range - 1;

		else 
		    firstbin = Tbufctl[ip].min_bin - 1;
		
                /* Remove the storm motion from the radial velocities. */
		Remove_stm_mtn( vel, lastbin, firstbin, &smrv[ip][0], 
                                srv, &Tbufctl[ip] );
		strindx = 0;

                /* We may want to check if the radials min and max bin
                   numbers are such that the RLE needs to be done on a 
                   smaller region than the window or whole radial. */
		endindx = Tbufctl[ip].maxobin - 1;
		numbufel = Tbufctl[ip].maxobin;
		bufstp = 1;

                /* Run length encode the storm relative velocity radial 
                   and put it in the product output buffer */
		RPGC_run_length_encode( istart, idelta, srv, strindx, 
			                endindx, numbufel, bufstp, 
                                        &Colrtbl.coldat[Tbufctl[ip].color_index][0], 
			                &numrleb, Tbufctl[ip].bufcnt, 
			                (short *) Tbufctl[ip].outbuf );

                /* Update the total number of run length encoded bytes */
		Tbufctl[ip].nrleb += numrleb;

                /* Update number of INT*2 words in the product so far. */
		Tbufctl[ip].bufcnt += numrleb / 2;

                /* set NEED flag to NO since this product is through with this 
                   radial. */
		Tbufctl[ip].need = 0;

                /* Set status flag to indicate that this window is still open. */
		Tbufctl[ip].status = 1;

                /* Update the total number of radials in this window so far. */
		++Tbufctl[ip].num_radials;

	    }

	}

    }


/* End of Srmrmrv_process(). */
} 

/*\//////////////////////////////////////////////////////////////////

   Description:
      This module releases product buffers. If the product to be 
      released index is negative, all outstanding products will be 
      released. If the disposition of the release is FORWARD, the 
      product header block is completed, and then released. After 
      releasing products, the number of window, and full elevation 
      scan products still to be completed is updated. 

   Inputs:
      idbuf - Product to be released.
      disposition - Either FORWARD or DESTROY.

//////////////////////////////////////////////////////////////////\*/
void Srmrmrv_release_prod( int idbuf, int disposition ){

    /* Local variables */
    int ip, ibeg, iend;

    /* If operation is equal to ALL (-1), then set up do loop 
       indices to release all products set up in the product 
       control list. */
    if( idbuf == -1 ){

	ibeg = 0;
	iend = Tnumprod - 1;

    } 
    else{

        /* Else... set up do loop indices to only release the specified product */
	ibeg = idbuf;
	iend = idbuf;

    }

    /* Do for all specified products. */
    for( ip = ibeg; ip <= iend; ++ip ){

        /* Only release products not already released. */
	if( Tbufctl[ip].status != RELEASED ){

            /* If buffer operation is forward (i.e. good product), then 
               build the product header block. */
	    if( disposition == FORWARD )
		Srmrmrv_header( Tbufctl[ip].outbuf, ip ); 

            /* Release the output buffer. */
	    RPGC_rel_outbuf( Tbufctl[ip].outbuf, disposition );
	    Tbufctl[ip].status = RELEASED;

            /* Decrement the released product from the running number of 
               outstanding products. */
	    if( Tbufctl[ip].prod_id == Srmrvreg_id ) 
		Numwin--;

	    else 
		Numful--;
	    
	}

    }

    /* Compute the total number of outstanding products to complete. */
    Onumprod = Numwin + Numful;

/* End of Release_prod(). */
} 

#define MXVLIX			256
#define MXDTIX			255
#define MNDTIX			2
#define MNDTIXM1		1
#define MXDTIXP1		256
#define MXDTIXM1		254

/*\///////////////////////////////////////////////////////////////////

   Description:
      This module computes a lookup table which is used to remove 
      the storm motion from the radial velocity field. This lookup 
      table is 257 elements long (exactly same length as the number 
      of possible scaled (biased) radial velocity values). Each 
      position of the lookup table corresponds to a possible scaled 
      velocity value (i.e. 0 to 256). The lookup table operates by 
      using the scaled radial velocity as a index into the lookup 
      table. The lookup table has the added feature of only computing 
      a new lookup table when the offset changes.  Also, if a radial 
      velocity value is missing (=256), the lookup table changes it 
      to zero to eliminate checks for missing data. 

   Inputs:
      ip - Index into the product control table.
      azm - Azimuth angle of the radial, in deg.

   Outputs:
      smrv - Lookup table to remove storm motion.  Biased and scaled
             velocity with storm motion removed.

///////////////////////////////////////////////////////////////////\*/
static void Srmrmrv_lookup( int ip, float azm, short *smrv ){

    /* Initialized data */

    static float reso[2] = { .2f,.1f };
    static int last_voff[MAXSRMRV] = { 666,666,666,666,666,666,666,666,666,666,
	                               666,666,666,666,666,666,666,666,666,666 };

    /* Local variables */
    int ilu, voff;
    float r, stdir;

    /* Compute the velocity index offset that this storm motion and 
       azimuth angle produces. */
    stdir = Tbufctl[ip].storm_dir * 0.10f;
    r = Tbufctl[ip].storm_speed * cos((stdir-azm)*DEGTORAD) * reso[Dop_reso-1];
    voff = RPGC_NINT(r);

    /* Limit VOFF to the interval [-254, 254] to prevent array overflow. */
    if( voff < 0 ){

        if( voff < -MXDTIXM1 )
            voff = -MXDTIXM1;

    } 
    else{

        if( voff > MXDTIXM1 )
	    voff = MXDTIXM1;

    }

    /* Only update the lookup table if the offset has changed from the 
       last radials offset. */
    if( voff != last_voff[ip] ){


        /* Build lookup table.  Set below threshold and range folded values 
           to be same */
	smrv[0] = 0;
	smrv[1] = 1;

        /* Set missing data value to be below threshold (saves "IF's" later). */
	smrv[MXVLIX] = 0;

        /* Modify the lookup table on the maximum negative and positive
           radial velocity ends.  Set to maximum negative or postive velocity. */
	if( voff <= 0 ){

            /* Build the inner part of the lookup table. */
	    for( ilu = MNDTIXM1 - voff; ilu <= MXDTIX; ++ilu ) 
		smrv[ilu] = (short) (ilu + voff);

            /* Modify the lookup table on the maximum negative velocity end. */
	    for( ilu = MNDTIX; ilu <= MNDTIXM1 - voff; ++ilu ) 
		smrv[ilu] = MNDTIX;

	} 
        else{

            /* Else.... VOFF > 0, build the inner part of the lookup table and 
               modify table on the max. positive end. */

	    for( ilu = MNDTIX; ilu <= MXDTIXP1 - voff; ++ilu ) 
		smrv[ilu] = (short) (ilu + voff);

	    for( ilu = MXDTIXP1 - voff; ilu <= MXDTIX; ++ilu ) 
		smrv[ilu] = MXDTIX;

	}

        /* Save the offset for future checks */
	last_voff[ip] = voff;

    }

/* End of Srmrmrv_lookup(). */
} 

/*\//////////////////////////////////////////////////////////////////

   Description:
      This module stores the product header data into the output 
      buffer for the storm relative mean radial velocity window 
      defined by the index into the local product control table. 

   Inputs:
      bufout - product buffer.
      ctlidx - product control table index.

//////////////////////////////////////////////////////////////////\*/
static void Srmrmrv_header( char *bufout, int ctlidx ){

    /* Local variables */
    int i, totleng, tabix;
    short params[10], *levels = NULL;
    float anginrad, hgt, slrng, r;
    Graphic_product *phd = (Graphic_product *) bufout;
    Symbology_block *sym = (Symbology_block *) 
                           ((char*) bufout + sizeof(Graphic_product) );
    packet_af1f_hdr_t *packet_af1f = (packet_af1f_hdr_t *) 
                                     ((char *) sym + sizeof(Symbology_block));

    /* Init header to 0. */
    memset( bufout, 0, sizeof(Graphic_product) );

    /* Fill in product header and description block fields. */
    RPGC_prod_desc_block( bufout, Tbufctl[ctlidx].prod_id, Volnum );

    /* Fill in product dependent parameters. */
    memset( params, 0, 10*sizeof(short) );

    /* Azimuth and range of window center. */
    params[0] = (short) Tbufctl[ctlidx].center_azm;
    params[1] = (short) Tbufctl[ctlidx].center_range;

    /* Elevation angle. */
    params[2] = (short) Elev_ang;

    /* Threshold tables. */
    tabix = Tbufctl[ctlidx].color_index;
    levels = &phd->level_1;
    for( i = 0; i < 16; ++i ){

	*levels = Colrtbl.thresh[tabix][i];
        levels++;

    }

    /* Offset to product information. */
    RPGC_set_prod_block_offsets( bufout, PHEADLNG, 0, 0 );

    /* Max data values. */

    /* Verify that the biased maximum negative velocity is within
       range of the biased velocity (0 - 256). */ 
    if( (Tbufctl[ctlidx].maxneg < 0) 
                       || 
        (Tbufctl[ctlidx].maxneg > 256) )
	Tbufctl[ctlidx].maxneg = 0;
     
    RPGCS_set_velocity_reso( Dop_reso );
    r = RPGCS_velocity_to_ms( Tbufctl[ctlidx].maxneg ) * MPS_TO_KTS; 
    params[3] = (short) RPGC_NINT(r);

    /* Verify that the biased maximum positive velocity is within
       range of the biased velocity (0 - 256). */
    if( (Tbufctl[ctlidx].maxpos < 0)
                       || 
        (Tbufctl[ctlidx].maxpos > 256) )
	Tbufctl[ctlidx].maxpos = 256;
     
    r = RPGCS_velocity_to_ms( Tbufctl[ctlidx].maxpos ) * MPS_TO_KTS; 
    params[4] = (short) RPGC_NINT(r);

    /* Origin of storm motion (-1 = algorithm, 0 = user specified) */
    params[5] = (short) Tbufctl[ctlidx].storm_info;

    /* Set storm speed and direction. */
    params[7] = (short) Tbufctl[ctlidx].kstorm_speed;
    params[8] = (short) Tbufctl[ctlidx].storm_dir;


    /* Divider. */
    sym->divider = -1;

    /* Block ID. */
    sym->block_id = 1;

    /* Number of layers. */
    sym->n_layers = 1;

    /* Layer divider. */
    sym->layer_divider = -1;

    /* Radial header information. */
    packet_af1f->code = 0xaf1f;

    /* Calculate scale factor. */
    anginrad = Elev_ang / 10.f * DEGTORAD;

     /* Set min and max pixel number and scale factor depending on 
        if the product is a full scan (1km) or a window (0.5 km) product */
    if( Tbufctl[ctlidx].bin_incr == 2 ){

	packet_af1f->scale_factor = cos(anginrad) * 4000;
	packet_af1f->index_first_range = (short) (Tbufctl[ctlidx].min_range*2);

        /* Must set total range bins to start from radar. */
	packet_af1f->num_range_bins = (short) Tbufctl[ctlidx].maxobin;

    } 
    else{

	packet_af1f->scale_factor = cos(anginrad) * 1000;
	packet_af1f->index_first_range = (short) Tbufctl[ctlidx].min_range;

        /* Must set total range bins to start from radar. */
	packet_af1f->num_range_bins = (short) Tbufctl[ctlidx].max_range;

    }

    /* Compute window height. */
    slrng = Tbufctl[ctlidx].center_range / 10.f * NM_TO_KM;
    hgt = (slrng * slrng / 15417.82f + slrng * sin(anginrad)) * KM_TO_FT;
    r = hgt * .001f;
    params[6] = (short) RPGC_NINT(r);

    /* Set the product dependent parameters. */
    RPGC_set_dep_params( bufout, params );

    /* Number of radials. */
    packet_af1f->num_radials = (short) Tbufctl[ctlidx].num_radials;

    /* Center I,J. */
    packet_af1f->i_center = 0;
    packet_af1f->j_center = 0;

    /* Offsets and length. */

    /* Layer length. */
    totleng = Tbufctl[ctlidx].nrleb + sizeof(packet_af1f_hdr_t);
    RPGC_set_product_int( &sym->data_len, totleng );

    /* Length of block. */
    totleng += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, totleng );

    /* Finish product header. */
    RPGC_prod_hdr( bufout, Tbufctl[ctlidx].prod_id, &totleng );

/* End of Srmrmrv_header(). */
} 

/*\//////////////////////////////////////////////////////////////////////

   Description:
      This module removes the storm motion from each radial velocity, 
      compacts the radial (from 250m range spacing to either 500 m 
      or 1000m - depending on the product resolution) by finding the 
      velocity with the maxi magnitude for each set of 2 or 4 bins 
      (depending on if the product resolution is 500 or 1000 meters), 
      ad updates the maximum positive and negative velocity found 
      within the entire product. 

   Inputs:

   Outputs:

//////////////////////////////////////////////////////////////////////\*/
static void Remove_stm_mtn( short *iv, int lastbin, int firstbin, short *smrv, 
                            short *srv, Tbufctl_t *tbufctl ){

    /* Local variables */
    int ib, vc, ibn, negv, posv;
    int gdata, fnegv, rfold, fposv;
    int srmrvi;

    /* Initialize bin values to zeros. */
    for( ib = 0; ib < tbufctl->maxobin; ++ib ) 
	srv[ib] = 0;

    /* Do For All bins in the window, incremented by product resolution 
       divided by the bin range spacing */
    for( ib = firstbin; ib < lastbin-tbufctl->bin_incr+1; ib += tbufctl->bin_incr ){

        /* Initialize max. positive and negative velocity indicies to midpoint. */
	negv = 129;
	posv = 129;

        /* Initialize flags indicating positive and negative values were found. */
	fnegv = 0;
	fposv = 0;

        /* Initialize range folded data flag to false. */
	rfold = 0;

        /* Initialize value control flag to zero. */
	vc = 0;

        /* Initialize good data flag to false. */
	gdata = 0;

        /* Do for a bin set (2 or 4 adjacient bins). */
	for( ibn = 0; ibn < tbufctl->bin_incr; ++ibn ){

            /* Remove storm motion wind from the radial velocity. */
	    srmrvi = smrv[iv[ib + ibn]];

            /* Check if the data is below threshold */
	    if( srmrvi < 1 ){

            /* Check if the data is "good data" */
	    } 
            else if( srmrvi > 1 ){

		gdata = 1;

                /* Check if this velocity is less than the running minimum 
                   of this bin set of velocity bins, if it is set the minimum 
                   to this value and set the found negative velocity to true. */
		if( srmrvi < negv ){

		    fnegv = 1;
		    negv = srmrvi;

		} 
                else if( srmrvi > posv ){

                    /* If this velocity is greater than the running maximum 
                       of this bin set of velocity bins, set this value to be 
                       the maximum value and set the found positive velocity 
                       flag (FPOSV) to true. */
		    fposv = 1;
		    posv = srmrvi;

		}

            /* Else the data is range folded. */
	    } 
            else 
		rfold = 1;
	    
	}

        /* First check if the data is all below threshold. */
	if( (!gdata) && (!rfold) ){

            /* If out of the (2 or 4) bins in the bin set, a good velocity data 
               value was found, see if it is bigger than the total product 
               maximum positive and negative storm relative mean radial velocities 
               also, figure out which value will be put in radial to be run- 
               length-encoded. */

	} 
        else if( gdata ){

            /* If a negative velocity was found..... */
	    if( fnegv ){

                /* Set value control flag to -1. more on this later...... */
		vc = -1;

	    }

            /* If a positive velocity was found...... */
	    if( fposv ){

                /* Increment value control flag by one, more on this later.... */
		++vc;

	    }

            /* Branch to statement labels 100, 200, or 300 depending on 
               if value of VC is -1, 0, or +1 respectivily. This flag works 
               as follows: 

               if only a negative velocity is found in the bin set, VC=-1, 
               if only a positive velocity is found in the bin set, VC=+1, 
               if both negative and positive velocities are found, VC=0. 

               This will not be confused with the initialization of VC=0 since 
               the GDATA flag will indicate that at least one good data value 
               was found in the bin set (if one wasn't then, this if block can 
               not be reached). */
	    if( vc < 0 ) 
		goto L100;

	    else if( vc == 0 ) 
		goto L200;

	    else 
		goto L300;
	    

            /* Save the negative velocity */
L100:
	    srv[(ib-tbufctl->min_bin)/tbufctl->bin_incr+1] = (short) negv;

            /* Check if this negative velocity is larger than the running max. 
               then exit loop */
	    if( negv < tbufctl->maxneg ) 
		tbufctl->maxneg = negv;
	    
	    goto L400;

            /* Save the maximum magnitude velocity of the negative and 
               positive velocities. */
L200:
	    if( posv < (257-negv) ){

		srv[(ib-tbufctl->min_bin)/tbufctl->bin_incr+1] = (short) negv;

                /* Check if this negative velocity is larger than the running max. */
		if( negv < tbufctl->maxneg ) 
		    tbufctl->maxneg = negv;
		 
	    } 
            else{

		srv[(ib-tbufctl->min_bin)/tbufctl->bin_incr+1] = (short) posv;

                /* Update max. positive velocity if the current is larger than 
                   the running maximum and then exit loop. */
		if( posv > tbufctl->maxpos ) 
		    tbufctl->maxpos = posv;
		 
	    }
	    goto L400;

            /* Save the positive velocity. */
L300:
	    srv[(ib-tbufctl->min_bin)/tbufctl->bin_incr+1] = (short) posv;

            /* Update max. positive velocity if the current is larger than the 
               running maximum. */
	    if( posv > tbufctl->maxpos ) 
		tbufctl->maxpos = posv;
	    
L400:
	    ;
	} 
        else{

            /* The best we could find is range folded. */
	    srv[(ib-tbufctl->min_bin)/tbufctl->bin_incr+1] = (short) 1;

	}

    }
    
/* End of Remove_stm_mtn(). */
} 

