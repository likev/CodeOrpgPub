/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2011/06/30 15:00:28 $ */
/* $Id: mpda_buf_cntrl.c,v 1.12 2011/06/30 15:00:28 steves Exp $ */
/* $Revision: 1.12 $ */
/* $State: Exp $ */

#include <veldeal.h>
#include <mpda_constants.h>

extern int Verbose_mode;

/*************************************************************************
   Description: 
      This module contains the buffer control function for the Multi-PRF 
      Dealiasing Algorithm. 
*************************************************************************/
int mpda_buf_cntrl( int current_vcp ){

    /* Initialized data */
    static int previous_vcp = -1;
/* Two lines added for Build 10  WDZ 05/29/07  */
    static int prev_rpg_index = -1;
    static int sweep_count = -1;
/*  Line added for Build 12 WDZ 02/26/09   */
    static int rng_unf_array_flg = 0;

    static int cuts_per_elv[ECUTMAX], cmode[ECUTMAX], lastflg[ECUTMAX];

    /* Local variables */
    int *obuf_ptr, *ibuf_ptr;
    int vcp_changed, mpda_flg, ref_flag, vel_flag, wid_flag;
    int ldx, vcp_status, el_cut_num, last_scan_flg, in_status, out_status;
    int radial_status, rpg_ind, length;

    /* Set up flags for the mpda VCP to control processing */
    mpda_flg = 1;
    last_scan_flg = 0;

    /* If this is the first time into the MPDA or if we have 
       switched to a different MPDA VCP set up vcp parameters */
    vcp_changed = 0;
    if( current_vcp != previous_vcp ){

	vcp_status = vcp_setup( cuts_per_elv, cmode, lastflg, current_vcp );
	if( vcp_status != NORMAL ){

	    RPGC_abort();
	    return 0;

	}

	previous_vcp = current_vcp;
	vcp_changed = 1;
    }

    /* Check moments.  If reflectivity or velocity moment disabled, 
       set mpda_flg to FALSE. */
    get_moments_status( &ref_flag, &vel_flag, &wid_flag );
    if( !ref_flag || !vel_flag ) 
	mpda_flg = 0;
     
L100:

    /* Get a radial of RAW BASE DATA. */
    ibuf_ptr = RPGC_get_inbuf_by_name( "RAWDATA", &in_status );

    /* If status is OK, check if mpda processing is needed */
    if( in_status == NORMAL ){

        /* If status is OK, get a buffer for the BASEDATA storage. */
	length = RPGC_get_inbuf_len( ibuf_ptr );

        /* Determine the elevation cut number.  Go into VCP setup arrays 
           for number of scans/elv, and for Build 12 (dual pol) length is
           passed */
	build_rad_header( (Base_data_header *) ibuf_ptr, &el_cut_num, 
                          &rpg_ind, &radial_status, length );

        /* If number of scans is one (1) then use VDA */
	if( cuts_per_elv[rpg_ind - 1] <= 1 )
	    mpda_flg = 0;
	
/* Two if blocks added for Build 10 WDZ 05/29/07	  */
	if(rpg_ind != prev_rpg_index){
           sweep_count = 0;
           prev_rpg_index = rpg_ind;
        }
        if((radial_status == 0 || radial_status == 3)
                               &&
          (cmode[el_cut_num-1] >  VCP_WAVEFORM_CS)){
          sweep_count = sweep_count + 1;
        }                            

        /* If mpda processing is not needed then we will be using 
           the VDA.  If so, try to get output buffer */
	if( !mpda_flg ){

            /* For BATCH cuts which are part of a split cut, throw 
               the data away. */
/*  New if block for sweep_count replaces if block for for Build 10 WDZ 05/29/07  */
            if(sweep_count > 1){
                /* Release the input buffer and get another radial of data */
		RPGC_rel_inbuf( ibuf_ptr );
		goto L100;

	    }

	    obuf_ptr = RPGC_get_outbuf_by_name( "BASEDATA", length, &out_status );

            /* If status is OK, copy input data to output buffer. */
	    if( out_status == NORMAL ){

                /* ;** The following is normal VDA processing */
		memcpy( obuf_ptr, ibuf_ptr, length );
		A304db_update_adaptation( ibuf_ptr );
		A304dc_process_radial( obuf_ptr );

                /* Release the output buffer with FORWARD disposition */
		RPGC_rel_outbuf( obuf_ptr, FORWARD );

                /* ;** Release the input buffer */
		RPGC_rel_inbuf( ibuf_ptr );

	    }
            else{

                /* Status return from A31215 was not NORMAL, so ABORT! 
                   First release the input buffer */
		RPGC_rel_inbuf( ibuf_ptr );
		RPGC_abort_because( out_status );

		return 0;

	    }

	}
        else{

            /* We are processing an MPDA elevation scan.  Initialize MPDA 
               arrays and get adaptable parameters */
	    if( ((radial_status == GOODBVOL) || (radial_status == GOODBEL)) 
                                     && 
                ((cmode[el_cut_num-1] == VCP_WAVEFORM_CS) 
                                      || 
                 (cmode[el_cut_num-1] == VCP_WAVEFORM_BATCH))){

		if( (vcp_changed == 1) 
                           ||
                    (!rng_unf_array_flg) ){

                    /* The size of the refl bin may have changed.
                       Make sure the range unfold arrays get built. 
                                              Build 12 WDZ 2/26/09  */
		    build_rng_unf_arrys();
                    rng_unf_array_flg = 1;

		}

		initialize_data_arrys();
		get_adapt_params( &Mpda.gui_mpda_tover );

                /* Added to support 2D Velocity Dealiasing Field Test. */
                if( (radial_status == GOODBVOL) && (Verbose_mode) )
                    RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG,
                                  "VELDEAL: Multi-PRF Dealiasing Algorithm Being Used\n" );
	    }

            /* Now we need to get the environmental wind status and put it 
               into our internal ewt structure. */
	    if( (radial_status == GOODBEL) 
                          && 
                lastflg[el_cut_num - 1]  ){
 
		update_ewt_status( ibuf_ptr );
		build_ewt_struct();

	    }

            /* If this is a surveillance scan we need to copy the reflectivity 
               to the output buffer for other users. */
	    if( cmode[el_cut_num-1] == VCP_WAVEFORM_CS ){

                /* get an output buffer */
		obuf_ptr = RPGC_get_outbuf_by_name( "BASEDATA", length, &out_status );

                /* If status is OK, copy input data to output buffer. 
                   Release the output buffer with FORWARD disposition */
		if( out_status == NORMAL ){

		    memcpy( obuf_ptr, ibuf_ptr, length );
		    RPGC_rel_outbuf( obuf_ptr, FORWARD );

		}
                else{

                    /* Status return from A31215 was not NORMAL, so ABORT! 
                       First release the input buffer */
		    RPGC_rel_inbuf( ibuf_ptr );
		    RPGC_abort_because( out_status );

		    return 0;

		}

	    }

            /* call the mpda driver subroutine */
	    radial_status = apply_mpda( ibuf_ptr );

            /* Release the input buffer */
	    RPGC_rel_inbuf( ibuf_ptr );

	}

    }
    else{

        /* Status returned from RPGC_get_inbuf... was not NORMAL so ABORT */
	RPGC_abort();
	return 0;

    }

    /* Process data till end of elevation or volume scan. */
    if( (radial_status != GENDEL) && (radial_status != GENDVOL) )
	goto L100;
     
    /* If last_scan_flg is true and end of elevation is true 
       copy the mpda output into output buffer and release. */
    if( lastflg[el_cut_num - 1] == 1 )
	last_scan_flg = 1;
     
    /* Check if we need to output MPDA data */
    if( last_scan_flg && mpda_flg ){

	for( ldx = 0; ldx < MAX_RADS; ++ldx ){

            /* acquire an output buffer */
	    obuf_ptr = RPGC_get_outbuf_by_name( "BASEDATA", MAX_GENERIC_BASEDATA_SIZE*sizeof(short), 
                                                &out_status );

            /* If status is OK, copy mpda data to output buffer. */
	    if( out_status == NORMAL ){

                int size;

		radial_status = output_mpda_data( ldx, obuf_ptr, &size );

                /* Release the output buffer with FORWARD disposition */

		RPGC_rel_outbuf( obuf_ptr, FORWARD | RPGC_EXTEND_ARGS, size );
		if( (radial_status == GENDEL) || (radial_status == GENDVOL) )
		    return 0;
		 
	    }
            else{

                /* Status return from A31215 was not NORMAL, so ABORT! */
		RPGC_abort_because( out_status );
		return 0;

	    }

	}
    }

    /* Return to caller. */
    return 0;

} 
