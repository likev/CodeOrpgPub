/* RCS info */
/* $Author: cmn $ */
/* $Locker:  $ */
/* $Date: 2008/09/03 20:18:36 $ */
/* $Id: basspect.c,v 1.6 2008/09/03 20:18:36 cmn Exp $ */
/* $Revision: 1.6 $ */
/* $State: Exp $ */

#include <basspect.h>

/**********************************************************************

   Description:
      Buffer control routine for Base Spectrum Width mapping program 

   Returns:
      Always returns 0.

**********************************************************************/
int a30731_buffer_control() {

    int ref_flag, wid_flag, vel_flag;
    int *bdataptr, *bspcptrs[NUMPRODS], i;
    int no_mem_flag[NUMPRODS];
    int no_mem_abort, pindx, opstat, abortit, relstat;

    opstat = NORMAL;
    Endelcut = 0;

    relstat = FORWARD;

    for (pindx = 0; pindx < NUMPRODS; ++pindx) {

        Pflag[pindx] = 0;
	no_mem_flag[pindx] = 0;

    }

    /* REQUEST ALL POSSIBLE OUTPUT BUFFERS TO SEE AT WHAT RANGES, 
       RESOLUTIONS, AND DATA LEVELS BASE SPECTRUM WIDTH MAPS NEED TO BE 
       GENERATED. */
    pindx = 0;

    /* PRODUCT CODE 28, 0 TO 60 KM. RADIUS (.25X.25 KM. RESOLUTION), 
       8 DATA LEVEL BASE SPECTRUM WIDTH PRODUCT BUFFER. */
    bspcptrs[pindx] = RPGC_get_outbuf( BSPC28, BSIZ28, &opstat );
    if (opstat == NORMAL) 
	Pflag[pindx] = 1;

    else {

	if (opstat == NO_MEM) {

            /* IF NO MEMORY FOR PRODUCT BUFFER, ABORT DATATYPE. */
	    RPGC_abort_datatype_because( BSPC28, NO_MEM );
	    no_mem_flag[pindx] = 1;

	}

    }

    ++pindx;

    /* PRODUCT CODE 29, 0 TO 115 KM. RADIUS (.5X.5 KM. RESOLUTION), 
       8 DATA LEVEL BASE SPECTRUM WIDTH PRODUCT BUFFER. */
    bspcptrs[pindx] = RPGC_get_outbuf( BSPC29, BSIZ29, &opstat );
    if (opstat == NORMAL) 
	Pflag[pindx] = 1;

    else {

	if (opstat == NO_MEM) {

            /* IF NO MEMORY FOR PRODUCT BUFFER, ABORT DATATYPE. */
	    RPGC_abort_datatype_because( BSPC29, NO_MEM );
	    no_mem_flag[pindx] = 1;

	}

    }

    ++pindx;

    /* PRODUCT CODE 30, 0 TO 230 KM. RADIUS (1X1 KM. RESOLUTION),
       8 DATA LEVEL BASE SPECTRUM WIDTH MAP PRODUCT BUFFER. */
    bspcptrs[pindx] = RPGC_get_outbuf( BSPC30, BSIZ30, &opstat);
    if (opstat == NORMAL) 
	Pflag[pindx] = 1;

    else {

	if (opstat == NO_MEM) {

            /* IF NO MEMORY FOR PRODUCT BUFFER, ABORT DATATYPE. */
	    RPGC_abort_datatype_because( BSPC30, NO_MEM );
	    no_mem_flag[pindx] = 1;
	}

    }

    /* DETERMINE IF ANYTHING TO DO, IF NOT ABORT THE WHOLE THING. */
    abortit = 1;
    no_mem_abort = 1;
    for (i = 0; i < NUMPRODS; ++i) {

	if (Pflag[i]) 
	    abortit = 0;
	 
	if (!no_mem_flag[i]) 
	    no_mem_abort = 0;
	 
    }

    if (!abortit) {

        /* REQUEST INPUT BUFFERS (RADIAL BASE DATA) AND PROCESS THEM 
           UNTIL THE END OF THE ELEVATION CUT IS REACHED. */
	bdataptr = RPGC_get_inbuf( BASEDATA, &opstat );

        /* CHECK FOR BASE WIDTH DISABLED. */
	if (opstat == NORMAL) {

	    RPGC_what_moments( (Base_data_header *) bdataptr, &ref_flag, 
                               &vel_flag, &wid_flag );
	    if (!wid_flag) {

                /* MOMENT DISABLED....RELEASE INPUT BUFFER AND DO ABORT PROCESSING. */
		RPGC_rel_inbuf( bdataptr );
		a30736_rel_prodbuf( bspcptrs, DESTROY );
		RPGC_abort_because( PROD_DISABLED_MOMENT );
		return 0;

	    }

	}

        /* CALL THE PRODUCT GENERATION CONTROL ROUTINE. */
        while(1){ 

	    if (opstat == NORMAL) {
 
	        a30732_product_generation_control( bspcptrs, bdataptr );

                /* RELEASE THE INPUT RADIAL. */
	        RPGC_rel_inbuf( bdataptr );

	    } else {

                /* IF THE INPUT DATA STREAM HAS BEEN CANCELED FOR SOME REASON, 
                   THEN RELEASE AND DESTROY ALL OF THE PRODUCT BUFFERS OBTAINED, 
                   AND RETURN TO THE PARAMETER TRAP ROUTER ROUTINE TO WAIT FOR 
                   FURTHER INPUT. */
	        relstat = DESTROY;
	        a30736_rel_prodbuf( bspcptrs, relstat );
	        return 0;

	    }

            /* TEST FOR THE END OF THE ELEVATION CUT (IF THE LAST RADIAL IN   
               THE ELEVATION CUT HAS JUST BEEN PROCESSED, THEN RELEASE AND   
               FORWARD ALL OF THE PRODUCT BUFFERS FOR THE PRODUCTS   
               GENERATED).*/
	    if (Endelcut) {

	        a30736_rel_prodbuf( bspcptrs, relstat );
	        return 0;

	    }

            /* RETRIEVE NEXT INPUT RADIAL. */
	    bdataptr = RPGC_get_inbuf( BASEDATA, &opstat );

        }

    } else {

        /* DO ABORT PROCESSING. */
	RPGC_abort_because( opstat );

    }

    /* RETURN TO THE PARAMETER TRAP ROUTER ROUTINE TO DETERMINE IF   
       ANY BASE SPECTRUM WIDTH MAPS ARE TO BE GENERATED ON THE NEXT   
       ELEVATION CUT. */

    return 0;

} 

/************************************************************************

   Description:
      Product Generation Control routine for Base Spectrum Width.
 
   Inputs:
      bspcptrs - array of pointers to output product buffers.
      bdataptr - pointer to basedata radial.

   Returns:
      Always returns 0.

************************************************************************/
int a30732_product_generation_control( int *bspcptrs[], void *bdataptr ){

    /* INITIALIZED DATA. */
    static short pcode[NUMPRODS] = { 28,29,30 };
    static int maxbins[NUMPRODS] = { 240,230,230 };
    static int radstep[NUMPRODS] = { 1,2,4 };
    static int est_per_rad[NUMPRODS] = { 252,252,252 };
    static int bufsiz[NUMPRODS] = { BSIZ28, BSIZ29, BSIZ30 };

    /* NOTE: THE lastbin VALUES ASSUME A 0-INDEXED DATA ARRAY. */
    static int lastbin[NUMPRODS] = { 240,460,920 };

    /* LOCAL VARIABLES. */
    int frst_spw, end_spw, nrlebytes;
    int delta, pindx, start;
    int stop_spw, excess;

    /* LOCAL STATIC VARIABLES. */
    static int pbuffind[NUMPRODS], clthtind[NUMPRODS], nrleb[NUMPRODS];
    static short numbins[NUMPRODS];

    Base_data_header *radhead = (Base_data_header *) bdataptr;
    short *spw_data = (short *) ( (char *) bdataptr + radhead->spw_offset);

    /* BEGINNING OF PRODUCT INITIALIZATION. */
    if( (radhead->status == GOODBVOL) 
                      || 
        (radhead->status == GOODBEL) ){

        int elevindex, vcpnumber;
        double elang, coselev;

        /* MAXIMUM DATA LEVEL INITIALIZATION. */
	Maxdl60 = SCALED_ZERO;
	Maxdl115 = SCALED_ZERO;
	Maxdl230 = SCALED_ZERO;

        /* RADIAL COUNT INITIALIZATION. */
	Radcount = 0;

        /* BUFFER INDEX COUNTER AND NUMBER OF RUN-LENGTH ENCODED BYTES 
           COUNTER INITIALIZATION FOR ALL PRODUCTS. */
	for( pindx = 0; pindx < NUMPRODS; ++pindx ){

	    pbuffind[pindx] = (sizeof(Graphic_product) + sizeof(Symbology_block) 
                              + sizeof(packet_af1f_hdr_t))/sizeof(short);
	    nrleb[pindx] = 0;

	}

        /* INITIALIZE COLOR LOOK-UP AND THRESHOLD TABLE INDICIES: */
	if( radhead->weather_mode == PRECIPITATION_MODE ){

	    clthtind[0] = SPNC8;
	    clthtind[1] = SPNC8;
	    clthtind[2] = SPNC8;

	}
        else{

	    clthtind[0] = SPCL8;
	    clthtind[1] = SPCL8;
	    clthtind[2] = SPCL8;

	}

        /* BUILD ALL NECESSARY PRODUCT HEADERS. */
	for( pindx = 0; pindx < NUMPRODS; ++pindx ){

	    if( Pflag[pindx] ){

		numbins[pindx] = RPGC_num_rad_bins( bdataptr, maxbins[pindx], radstep[pindx], 2 );

		lastbin[pindx] = numbins[pindx] * radstep[pindx];
		a30733_product_header( (short *) bspcptrs[pindx], bdataptr, clthtind[pindx], 
                                       pcode[pindx], numbins[pindx] );

	    }

	}

        /* COSINE OF ELEVATION ANGLE COMPUTATION FOR COMBINATION WITH 
           SCALE FACTOR FOR THE VERTICAL CORRELATION OF BASE SPECTRUM WIDTH
           MAPS. */
        vcpnumber = RPGC_get_buffer_vcp_num( bdataptr );
        elevindex = RPGC_get_buffer_elev_index( bdataptr );

        /* GET ELEVATION ANGLE. */
        Elmeas = (short) RPGCS_get_target_elev_ang( vcpnumber, elevindex ); 
        elang = Elmeas * 0.1f;
        coselev = cos( elang * DEGTORAD );
        Vc1 = (short) (coselev * 1000.0);
        Vc2 = (short) (coselev * 1000.0);

        /* END OF PRODUCT INITIALIZATION. */

    }

    /* PERFORM INDIVIDUAL RADIAL PROCESSING. */

    /* RETRIEVE START ANGLE AND DELTA ANGLE MEASUREMENTS FROM THE */
    /* INPUT RADIAL BUFFER FOR THIS RADIAL. */
    start = radhead->start_angle;
    delta = radhead->delta_angle;

    /* INCREMENT THE RADIAL COUNT. */
    Radcount++;

    /* CALCULATE THE START AND END OF THE GOOD DATA.  FRST_SPW IS THE 
       START INDEX OF GOOD DATA, END_SPW IS THE INDEX OF THE LAST 
       GOOD SPECTRUM WIDTH DATA. */
    frst_spw = radhead->dop_range - 1;
    end_spw = frst_spw + radhead->n_dop_bins - 1;

    /* PERFORM MAXIMUM DATA LEVEL OPERATIONS FOR THIS RADHEAD. */
    a30734_maxdl( bdataptr, frst_spw, end_spw );

    /* RUN-LENGTH ENCODE THIS RADIAL FOR ALL PRODUCTS REQUESTED. */
    for( pindx = 0; pindx < NUMPRODS; ++pindx ){

        if( Pflag[pindx] ){

            /* CALCULATE WORDS AVAILABLE IN BUFFER. */
	    excess = bufsiz[pindx] - (nrleb[pindx] + 150);
	    if (excess > est_per_rad[pindx]) {

                /* COMPUTING MIN. */
		stop_spw = end_spw;
                if( (lastbin[pindx] - 1) < end_spw )
                   stop_spw = lastbin[pindx] - 1;

		RPGC_run_length_encode( start, delta, spw_data, frst_spw, 
                                        stop_spw, lastbin[pindx], radstep[pindx], 
                                        &color_data.coldat[clthtind[pindx]][0], 
                                        &nrlebytes, pbuffind[pindx], 
                                        (short *) bspcptrs[pindx] ); 

                /* UPDATE COUNTERS. */
		nrleb[pindx] += nrlebytes;
		pbuffind[pindx] += nrlebytes / 2;

	    }

        }

    }

    /* TEST FOR THE LAST RADIAL IN THE ELEVATION CUT. */
    if( (radhead->status == GENDVOL) || (radhead->status == GENDEL) ){ 

	Endelcut = 1;

        /* DO FOR ALL PRODUCTS REQUESTED. */
	for( pindx = 0; pindx < NUMPRODS; ++pindx) {

	    if( Pflag[pindx] )
		a30735_end_of_product_processing( (short *) bspcptrs[pindx], nrleb[pindx], 
                                                  pcode[pindx] );

	}

    }

    /* RETURN TO BUFFER CONTROL ROUTINE. */
    return 0;

} 

/*********************************************************************

   Description:
     Product Header production routine for BASE SPECTRUM WIDTH 

   Inputs:
      outbuff - pointer to product output buffer.
      bdataptr - pointer to basedata radial.
      trshind - Color Table threshold index.
      pcode - product code.
      numbins - number of range bins in the product.

   Returns:
      Always returns 0.

**********************************************************************/
int a30733_product_header( short *outbuff, void *bdataptr, int trshind, 
                           short pcode, short numbins ){

    int volno, pid, offset;

    Graphic_product *phd = (Graphic_product *) outbuff;
    Symbology_block *sym = 
        (Symbology_block *) (((char *) outbuff) + sizeof(Graphic_product));
    packet_af1f_hdr_t *packet_af1f = 
        (packet_af1f_hdr_t *) (((char *) outbuff) + 
                        sizeof(Graphic_product) + sizeof(Symbology_block));

    /* FILL OUT THE PRODUCT DESCRIPTION BLOCK. */
    volno = RPGC_get_buffer_vol_num( bdataptr );
    pid = ORPGPAT_get_prod_id_from_code( pcode );
    RPGC_prod_desc_block( outbuff, pid, volno );

    /* STORE DATA LEVEL THRESHOLD CODES. */
    phd->level_1 = color_data.thresh[trshind][0];
    phd->level_2 = color_data.thresh[trshind][1];
    phd->level_3 = color_data.thresh[trshind][2];
    phd->level_4 = color_data.thresh[trshind][3];
    phd->level_5 = color_data.thresh[trshind][4];
    phd->level_6 = color_data.thresh[trshind][5];
    phd->level_7 = color_data.thresh[trshind][6];
    phd->level_8 = color_data.thresh[trshind][7];
    phd->level_9 = color_data.thresh[trshind][8];
    phd->level_10 = color_data.thresh[trshind][9];
    phd->level_11 = color_data.thresh[trshind][10];
    phd->level_12 = color_data.thresh[trshind][11];
    phd->level_13 = color_data.thresh[trshind][12];
    phd->level_14 = color_data.thresh[trshind][13];
    phd->level_15 = color_data.thresh[trshind][14];
    phd->level_16 = color_data.thresh[trshind][15];

    /* SET OFFSET TO PRODUCT SYMBOLOGY BLOCK, GRAPHIC ALPHANUMERIC
       AND TABULAR ALPHANUMERIC. */
    offset = sizeof(Graphic_product) / sizeof(short);
    RPGC_set_prod_block_offsets( outbuff, offset, 0, 0 );

    /* PRODUCT BLOCK DIVIDER, BLOCK ID, NUMBER OF LAYERS AND LAYER 
       DIVIDER. */
    sym->divider = -1;
    sym->block_id = 1;
    sym->n_layers = 1;
    sym->layer_divider = -1;

    /* BUILD THE RADIAL HEADER FOR THIS PRODUCT: 
       STORE OPERATION CODE, RANGE TO FIRST BIN, NUMBER OF RANGE 
       BINS AND I,J CENTER OF SWEEP. */
    packet_af1f->code = 44831;
    packet_af1f->index_first_range = 0;
    packet_af1f->num_range_bins = numbins;
    packet_af1f->i_center = 256;
    packet_af1f->j_center = 280;

    /* LENGTH OF LAYER,LENGTH OF BLOCK,LENGTH OF MESSAGE 
       SCALE FACTOR,NUMBER OF RADIALS ARE STORED IN THE 
       END-OF-PRODUCT MODULE. */

    return 0;

} 

#define MAXB230		920
#define MAXB115		460
#define MAXB60 		240

/**********************************************************************

   Description:
      Maximum-data-level routine for Base Spectrum Width mapping program 

   Inputs:
      bdataptr - pointer to basedata radial.
      first_spw - first spectrum width bin.
      end_spw - last spectrum width bin.

   Returns:
      Always returns 0.

**********************************************************************/
int a30734_maxdl( char *bdataptr, int frst_spw, int end_spw ){

    int endb60, endb230, endb115, binindx;
    int i;

    Base_data_header *bdh = (Base_data_header *) bdataptr;
    short *radial = (short *) (bdataptr + bdh->spw_offset);

    /* *** EXECUTABLE CODE: */

    /*  CALCULATE THE LOOP LIMITS FOR THE DATA */
    endb230 = MAXB230;
    if( endb230 > end_spw )
        endb230 = end_spw;

    endb115 = MAXB115;
    if( endb115 > end_spw );
        endb115 = end_spw;

    endb60 = MAXB60;
    if( endb60 > end_spw )
       endb60 = end_spw;

    /* *** TEST FOR 230 KM. RANGE PRODUCT: */
    if( Pflag[2] ){

	i = endb230;
	for( binindx = frst_spw; binindx < i; ++binindx ){

	    if( radial[binindx] > 1 ){

                /* TEST FOR 230 KM. RANGE MAXIMUM DATA LEVEL */
		if( radial[binindx] > Maxdl230 )
		    Maxdl230 = radial[binindx];

                /* TEST FOR 115 KM. RANGE MAXIMUM DATA LEVEL */
		if( binindx < 460 ){

		    if( radial[binindx] > Maxdl115 )
			Maxdl115 = radial[binindx];

		}

                /* TEST FOR 60 KM. RANGE MAXIMUM DATA LEVEL */
		if( binindx < 240 ){

		    if( radial[binindx] > Maxdl60 )
			Maxdl60 = radial[binindx];

		}

	    }

	}

        /* TEST FOR 115 KM. RANGE PRODUCT */

    } 
    else if( Pflag[1] ){

	i = endb115;
	for( binindx = frst_spw; binindx < i; ++binindx ){

	    if( radial[binindx] > 1 ){

                /* TEST FOR 115 KM. RANGE MAXIMUM DATA LEVEL */
		if( radial[binindx] > Maxdl115 )
		    Maxdl115 = radial[binindx];

                /* TEST FOR 60 KM. RANGE MAXIMUM DATA LEVEL */
		if( binindx < 240 ){

		    if( radial[binindx] > Maxdl60 )
			Maxdl60 = radial[binindx];

		}

	    }

	}

    /* ELSE, ONLY HAVE TO PRODUCE 60 KM. RANGE PRODUCT */
    } 
    else{

	i = endb60;
	for( binindx = frst_spw; binindx < i; ++binindx ){

	    if( radial[binindx] > 1 ){

                /* TEST FOR 60 KM. RANGE MAXIMUM DATA LEVEL */
		if( radial[binindx] > Maxdl60 )
		    Maxdl60 = radial[binindx];

	    }

	}

    }

    /* RETURN TO THE PRODUCT GENERATION CONTROL ROUTINE */
    return 0;

}


/**********************************************************************

   Description:
      Maximum data level routine for Base Spectrum Width

   Inputs:
      outbuff - pointer to output product buffer.
      nrleb - number of run-length-encoded bytes in the product.
      pcode - product code.

   Returns:
      Always returns 0.

**********************************************************************/
int a30735_end_of_product_processing( short *outbuff, int nrleb, int pcode ){

    short params[10];
    int bytecnt, pid;
    float r;

    packet_af1f_hdr_t *packet_af1f = (packet_af1f_hdr_t *) (((char *) outbuff) +
                                     sizeof(Graphic_product) + sizeof(Symbology_block));
    Symbology_block *sym = (Symbology_block *) (((char *) outbuff) + sizeof(Graphic_product));

    /* INITIALIZE THE PRODUCT PARAMETERS TO 0. */
    memset( params, 0, 10*sizeof(short) );

    /* PRODUCT ELEVATION. */
    params[2] = Elmeas;

    /* GET THR PRODUCT ID. */
    pid = ORPGPAT_get_prod_id_from_code( pcode );

    /* ASSIGN THE SCALE FACTOR (WITH THE VERTICAL CORRELATION 
       CORRECTION ALREADY APPLIED FOR THIS ELEVATION ANGLE) TO THE 
       DISPLAY HEADER. */
    if( pid == BSPC28 )
        packet_af1f->scale_factor = Vc1;

    else 
        packet_af1f->scale_factor = Vc2;
     
    /* ASSIGN THE MAXIMUM DATA LEVEL FOR THIS PRODUCT TO THE PRODUCT 
       HEADER CONVERTING SPECTRUM WIDTH TO KNOTS FROM M/S */
    if( pid == BSPC28 ){

        /* 60 KM PRODUCT */
	r = RPGCS_spectrum_width_to_ms( Maxdl60 ) * MPS_TO_KTS;
	params[3] = (short) RPGC_NINT( r );

    }
    else if( pid == BSPC29 ){

        /* 115 KM PRODUCT */
	r = RPGCS_spectrum_width_to_ms( Maxdl115 ) * MPS_TO_KTS;
	params[3] = (short) RPGC_NINT( r );

    }
    else{

        /* 230 KM PRODUCT */
	r = RPGCS_spectrum_width_to_ms( Maxdl230 ) * MPS_TO_KTS;
        params[3] = (short) RPGC_NINT( r );

    }

    /* SET THE PRODUCT DEPENDENT PARAMETERS. */
    RPGC_set_dep_params( outbuff, params );

    /* ASSIGN THE RADIAL COUNT FOR THIS PRODUCT TO THE DISPLAY HEADER. */
    packet_af1f->num_radials = Radcount;

    /* CALCULATE AND STORE THE PRODUCT MESSAGE LENGTH, THE 
       PRODUCT BLOCK LENGTH, AND THE PRODUCT LAYER LENGTH. */

    /* LENGTH OF PRODUCT LAYER */
    bytecnt = nrleb + sizeof( packet_af1f_hdr_t );
    RPGC_set_product_int( &sym->data_len, bytecnt);

    /* LENGTH OF BLOCK */
    bytecnt += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, bytecnt);

    /* LENGTH OF MESSAGE */
    RPGC_prod_hdr( outbuff, pid, &bytecnt);

    /* RETURN TO THE PRODUCT GENERATION CONTROL ROUTINE: */
    return 0;

} 

/**********************************************************************

   Description:
      Routine to release the product output buffer, and either forward 
      or destroy the buffer upon release, for Base Spectrum Width. 

   Inputs:
      bspcptrs - pointers to output products.
      relstat - release disposition.

   Returns:
      Always returns 0.

**********************************************************************/
int a30736_rel_prodbuf( int *bspcptrs[], int relstat ){

    int pindx;

    /* IF INPUT DATA STREAM WAS INTERUPTED FOR SOME REASON, THEN 
       RELEASE AND DESTROY ALL PRODUCT BUFFERS OBTAINED, ELSE 
       RELEASE AND FORWARD ALL FINISHED PRODUCTS TO ON-LINE STORAGE. */
    for( pindx = 0; pindx < NUMPRODS; ++pindx ){

        if( Pflag[pindx])
	    RPGC_rel_outbuf( bspcptrs[pindx], relstat );

    } 

    /* RETURN TO BUFFER CONTROL ROUTINE: */
    return 0;

} 
