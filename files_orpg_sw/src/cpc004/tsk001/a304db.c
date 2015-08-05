/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2012/09/13 17:11:17 $ */
/* $Ida304db.ftn,v 1.10 2005/11/08 21:09:39 steves Exp $ */
/* $Revision: 1.6 $ */
/* $State: Exp $ */

#include <veldeal.h>

/* Function Prototypes. */
static int Update_sprt_adaptation( int save_pulse_width );

/*********************************************************************** 

   Description: 
      This module controls the updating of the local copy of adaptation 
      data for velocity dealiasing. 

   Inputs:
      input_ptr - pointer to input radial.

   Returns:
      Currently always returns 0.

***********************************************************************/
int A304db_update_adaptation( void *input_ptr ){

    /* Initialized data */
    static int from_vad = 0;
    static int from_hci = 0;
    static int last_update = -1;
    static int first_time = 1;
    static int save_pulse_width = 2;  /*  Build 12 WDZ  */

    /* Local variables */
    int i, itc_status, pulse_width;
    short current_vcp;                
    int vol_scan_num;                 /*  Build 12 WDZ  */

    Base_data_header *radhdr = (Base_data_header *) input_ptr;

    /* One-time initialization. */
    if( first_time ){

        /* Initialize the previous radial to BELOW_THR. */
        memset( A304db.vel_prevaz[BAD_PREV_RADIAL], BELOW_THR,
                VELDEAL_MAX_SIZE*sizeof(short) );
        memset( A304db.vel_prevaz[GOOD_PREV_RADIAL], BELOW_THR,
                VELDEAL_MAX_SIZE*sizeof(short) );

        first_time = 0;

    }

    /* Check radial status for beginning of volume.  If at beginning and 
       there is Doppler data, set the pulse length based on VCP data, 
       then call A304dI to set local copy of adaptation values. */
    if( (radhdr->status == GOODBVOL) 
                    && 
        (radhdr->n_dop_bins != 0)
                    && 
        (radhdr->nyquist_vel != 0) ){

	pulse_width = ORPGVCP_SHORT_PULSE;
	current_vcp = radhdr->vcp_num;

	for( i = 0; i < VCPMAX; ++i ){

            Vcp_struct *vcp = (Vcp_struct *) RDA_cntl.rdcvcpta[i];

	    if( vcp->vcp_num == current_vcp){

		pulse_width = vcp->pulse_width;
		break;

	    }

	}

	A304di_vd_local_copy( pulse_width );
        save_pulse_width = pulse_width;           /* Build 12 WDZ  */

    }


    /* If at beginning of elevation or volume, check if new environmental wind 
       table data is available.  Only required if radial has Doppler data.  
       Also initialize some variables and get Waveform. */
    if( ((radhdr->status == GOODBEL) 
                      || 
         (radhdr->status == GOODBVOL)) 
                      && 
        ((radhdr->n_dop_bins != 0) 
                      && 
         (radhdr->nyquist_vel != 0)) ){

        /* Update the environmental wind table data if sounding data available. */
	RPGC_itc_read( ENVVAD, &itc_status );
	if( itc_status == 0 ){

	    if( Envvad.parameter_from_vad[0] == QUE4_ENVWNDVAD ) 
		A304dg.sounding_avail = 1;
	    
	}

	from_vad = 0;
	from_hci = 0;
	if( ((A304dg.sounding_avail) 
                     && 
             (Envvad.parameter_from_vad[1] != last_update)) 
                       || 
             (Env_wnd_tbl_updt == 1) ){

	    Env_wnd_tbl_updt = 0;
	    if( (Envvad.parameter_from_vad[0] == QUE4_ENVWNDVAD)
                                     && 
		 (Envvad.parameter_from_vad[1] != last_update) ) 
		from_vad = 1;
	     
            else 
		from_hci = 1;
	    
            /*  New environmental wind data is available */
	    RPGC_itc_read( A3CD97, &itc_status );

            /* If status is not normal, clear SOUNDING_AVAIL */
	    if( itc_status != 0 )
		A304dg.sounding_avail = 0;

            else{

		if( from_vad == 1 )
		    last_update = Envvad.parameter_from_vad[1];
		
                /*  Report update if required. */
		if( !Valid_soundings ){

                    /* Send a message saying new VAD data has arrived. */
		    if( from_vad == 1 )
			RPGC_log_msg( GL_STATUS, 
                                      "New Environmental Winds Data Received from VAD\n" );

		    else 
			RPGC_log_msg( GL_STATUS, 
                                      "New Environmental Winds Data Received from HCI\n" );
		    
		    Valid_soundings = 1;

		}

	    }

            if( Valid_soundings ){

                /* If sounding is valid, compute the minimum and maximum tables entries
                   where there is data. */
                Min_ewt_entry = LEN_EWTAB;
                Max_ewt_entry = -1;

                for( i = 0; i < LEN_EWTAB; i++ ){

                   if( Ewt.newndtab[i][NCOMP] != MTTABLE_INT ){

                      if( i < Min_ewt_entry )
                          Min_ewt_entry = i;

                      if( i > Max_ewt_entry )
                          Max_ewt_entry = i;

                   }

                }

                LE_send_msg( GL_INFO, "Minimum EWT Entry: %d, Maximum EWT Entry: %d\n",
                             Min_ewt_entry, Max_ewt_entry );
            }

	}

        /* Set the SOUNDING_AVAIL flag if sounding data now available. */
	A304dg.sounding_avail = Valid_soundings && A304di.use_soundings_flag;

        /* Initialize some variables. */
	Num_jump_conrad = 0;
	A304db.status = BAD_PREV_RADIAL;
	A304db.lstgd_dpbin_prevaz = -1;
	A304db.fstgd_dpbin_prevaz = LARGE_VALUE;

        /* Initialize the VEL_PREVAZ array to flag value. */
	memset( A304db.vel_prevaz[GOOD_PREV_RADIAL], BELOW_THR, 
                VELDEAL_MAX_SIZE*sizeof(short) );

        /* Code to get Waveform moved here for Build 12 WDZ 10/30/2008 */
        Scan_Summary *summary = NULL;
        vol_scan_num = RPGC_get_current_vol_num();
        summary = RPGC_get_scan_summary( vol_scan_num );
        Waveform = RPGCS_get_elev_waveform( summary->vcp_number, radhdr->elev_num - 1);
        RPGC_log_msg( GL_INFO, "Cut: %d, Waveform: %d\n", radhdr->elev_num - 1,
                      Waveform );
        /*  End get waveform code  */
        
        Update_sprt_adaptation( save_pulse_width );  /*  Build 12  */

    }

    /* Return to caller. */
    return 0;

/* End of A304db_update_adaptation() */
}

/*********************************************************************** 

   Description: 
      This function updates two local VDA adaptation parameters
      data when the staggered PRT waveform is detected and resets 
      them based on the pulse width if the waveform reverts to something
      other than staggered PRT. 

   Inputs:
      save_pulse_width - pulse width for the current vcp.

   Returns:
      Always returns 0

   Function new for Build 12

***********************************************************************/

static int Update_sprt_adaptation( int save_pulse_width ){

    /* Initialized data */
    static int first_time = 1;

    /*  local variables  */
    int prev_waveform;

    /* One-time initialization. */
    if(first_time){
       prev_waveform = -1;
       first_time = 0;
    }

    /*  Set two adaptable parameters based on the waveform   */
    if( Waveform == VCP_WAVEFORM_STP){
	A304di.th_conbin_rej = Radazvd.th_consecutive_rejected_sprt;
	A304di.th_max_conazjmp = Radazvd.th_max_azm_with_jump_sprt;
    }        
    else{
       if (prev_waveform == VCP_WAVEFORM_STP){
          if(save_pulse_width == ORPGVCP_SHORT_PULSE){
             A304di.th_conbin_rej   = Radazvd.th_consecutive_rejected_s;
             A304di.th_max_conazjmp = Radazvd.th_max_azm_with_jump_s;
          }
          else{
             A304di.th_conbin_rej   = Radazvd.th_consecutive_rejected_l;
             A304di.th_max_conazjmp = Radazvd.th_max_azm_with_jump_l;
          }
       }
    }
    prev_waveform = Waveform;
    return 0;

/*  End of Update_sprt_adaptation  */
}
