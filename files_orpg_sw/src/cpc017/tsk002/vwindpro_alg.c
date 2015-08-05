/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/04/25 17:19:06 $ */
/* $Id: vwindpro_alg.c,v 1.8 2014/04/25 17:19:06 steves Exp $ */
/* $Revision: 1.8 $ */
/* $State: Exp $ */

#include <vwindpro.h>

/* Static Global Variables. */
static Avset_t Avset = { 0, 0, 0, 0, 0.0, 0, 0, 0.0 };

/* Static Function Prototypes. */
static int Get_rda_avset_status();

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      Buffer control routine for vwindpro algorithm and product task.

   Return:
      -1 on error, 0 otherwise.

////////////////////////////////////////////////////////////////////////\*/
int A317a2_vad_buffer_control(){

    /* Local variables */
    int ostat, opstat_vadtmhgt, opstat_vadparam, opstat_hplots;
    int vad_alert, envstat, ref_flag, vel_flag, wid_flag;
    int ht, itc_status, abort, nrstat, vad_product;
    int statl, year, mon, day, hr, min, sec;
    time_t current_time;
    char *ipr = NULL, *optrv = NULL, *optr = NULL;

    /* Information for the supplemental VADs. */
    static int npt_ewt[2];
    static float htg_ewt[2], rms_ewt[2], hwd_ewt[2], shw_ewt[2];

    /* Initialize ABORT flag to false. ABORT is used to determine if the 
       current run was ABORTED for any reason */
    abort = 0;
    opstat_vadtmhgt = NORMAL;
    opstat_vadparam = NORMAL;
    opstat_hplots = NORMAL;

    /* Check if ALERT needs the vad parameters buffer. */
    ostat = RPGC_check_data_by_name( "VADPARAM" );

    /* If ALERT needs the vad data, set VAD_ALERT to true. */
    if( ostat == NORMAL ) 
	vad_alert = 1;

    else
	vad_alert = 0;

    /* Check if vad product is required.  If vad product is needed, 
       VAD_PRODUCT to true. */
    ostat = RPGC_check_data_by_name( "HPLOTS" );

    if( ostat == NORMAL ) 
	vad_product = 1;

    else
	vad_product = 0;

    /* Get input buffer. */
    ipr = RPGC_get_inbuf_by_name( "COMBBASE", &ostat );

    /* Check for disabled moments. */
    if( ostat == NORMAL ){

	RPGC_what_moments( (Base_data_header *) ipr, &ref_flag, 
                           &vel_flag, &wid_flag );

        /* Disabled moments...do abort processing and goto exit. */
	if( !ref_flag || !vel_flag ){

	    RPGC_rel_inbuf( ipr );
	    RPGC_abort_because( PROD_DISABLED_MOMENT );
	    return -1;

	}

    }

    /* If input buffer status is normal, then continue. */
    while(1){

        if( ostat == NORMAL ){

            /* Call A317e2_vad_init() to initialize task variables on a task, 
               volume, or elevation basis. */
	    A317e2_vad_init( ipr, &nrstat, htg_ewt, npt_ewt, rms_ewt, 
                             hwd_ewt, shw_ewt );

            /* If the radial is "GOOD", then call A317f2_vad_data(). */
	    if( nrstat <= GOODTHRHI ){

	        A317f2_vad_data( ipr );

                /* If mimumum height is not zero .... */
	        if( Ceminht >= 0 ){

		    for( ht = Ceminht; ht <= Cemaxht; ++ht )
		        A317d2_vad_data_hts( ipr, ht );

	        }

                if( Ceminht_s >= 0 ){

		    for( ht = Ceminht_s; ht <= Cemaxht_s; ++ht )
		        A317s2_vad_data_hts( ipr, ht );

                }

                /* Call A317u2_vad_data_ewt() to obtain vad data for 
                   supplemental vads */
	        if( (A317vi->celv < NELVEWT) && Ewt.envwndflg ) 
		    A317u2_vad_data_ewt( ipr );

	    }

            /* If this radial is not a good intermediate radial, then 
               if the current radial is the last one of the current elevation 
               scan then only call A317g2_vad_proc() to vad analyze the data */
	    if( nrstat != GOODINT ){

	        if( nrstat == GENDEL ){

                    /* Call A317g2_vad_proc() to perform vad analysis FOR 
                       CONSTANT SLANT RANGE to calculate vertical winds and
                       divergence.  */
		    A317g2_vad_proc();

                    /* Call A317c2 to perform VAD analysis on set of slant 
                       ranges for this elevation angle. */
		    if( Ceminht >= 0 ){

                        double avg_elv;

                        /* AVG_ELV is the average elevation angle for the 
                           scan, in degrees. */
                        avg_elv = A317vi->sum_elv / A317vi->tnradials;
#ifdef DEBUG

                        LE_send_msg( GL_INFO, " \n" );
                        LE_send_msg( GL_INFO,
                           "----> Original: Processing Elevation Angle: %6.2f <----\n", 
                           avg_elv );
#endif

		        for( ht = Ceminht; ht <= Cemaxht; ++ht ) 
			    A317c2_vad_proc_hts( ht );

                    }

                    if( Ceminht_s >= 0 ){

                        double avg_elv;

                        /* AVG_ELV is the average elevation angle for the 
                           scan, in degrees. */
                        avg_elv = A317vi->sum_elv / A317vi->tnradials;

#ifdef DEBUG
                        LE_send_msg( GL_INFO, " \n" );
                        LE_send_msg( GL_INFO,
                           "----> Supplemental: Processing Elevation Angle: %6.2f <----\n", 
                           avg_elv );
#endif

		        for( ht = Ceminht_s; ht <= Cemaxht_s; ++ht ) 
			    A317t2_vad_proc_hts( ht );

		    }

                    /* Call A317v2_vad_proc_ewt to perform VAD analysis for the 
                       supplemental VADs */
		    if( (A317vi->celv < NELVEWT) && Ewt.envwndflg ) 
		        A317v2_vad_proc_ewt( htg_ewt, rms_ewt, npt_ewt, 
			                     hwd_ewt, shw_ewt );

                    /* Else if the current radial is the last one of the i
                       current volume scan, call A317g2_vad_proc() to vad 
                       analyze the data and then call A317o2_vad_prod() to 
                       generate the product. */
	        } 
                else if( nrstat == GENDVOL ){

                    /* Call A317g2_vad_proc() to perform VAD analysis. */
		    A317g2_vad_proc();

                    /* Call A317c2 to perform VAD analysis on set of slant 
                       ranges for this elevation angle. */
		    if( Ceminht >= 0 ){

		        for( ht = Ceminht; ht <= Cemaxht; ++ht )
			    A317c2_vad_proc_hts( ht );

		    }

                    /* Perform Enhanced VAD analysis. */
		    if( (Ceminht_s >= 0) && (Vad.enhanced_vad) ){

		        for( ht = Ceminht_s; ht <= Cemaxht_s; ++ht )
			    A317t2_vad_proc_hts( ht );

		    }
 
                    /* Get output buffer for "VADTMHGT". */
		    optrv = (char *) RPGC_get_outbuf_by_name( "VADTMHGT", VADTMHGT_SIZE, 
                                                              &opstat_vadtmhgt );

                    /* Look at the data in A317ve and A317vs and for each height
                       in A317vs, pick the one with the lowest RMS that satisfies
                       symmetry criteria and number of points.  Replace the value 
                       in A317ve. */
                    if( Vad.enhanced_vad )
                        A317r2_choose_height();

                    /* Build output buffer "VADTMHGT". */
		    if( opstat_vadtmhgt == NORMAL ){

		        A317p2_vad_tmhgt( optrv );
		        RPGC_rel_outbuf( optrv, FORWARD );

                    /* Output buffer NO MEMORY...Abort datatype */
		    } 
                    else{

		        if( opstat_vadtmhgt == NO_MEM )
			    RPGC_abort_datatype_because( Vadtmhgt_id, NO_MEM );

		    }

                    /* Re-read environmental wind table in the event it has   
                       changed. */
		    RPGC_itc_read( A3CD97, &itc_status );
		    if( Ewt.envwndflg ){

		        envstat = 1;
		        A317z2_envwndtab_entries( htg_ewt, hwd_ewt, shw_ewt, 
                                                  &statl );
		        envstat = statl;

                        /* If environmental status is TRUE, then post event
                           indicating newly updated table is available. */
		        if( envstat ){

                            /* Write the environmental wind table to the ITC. */
			    RPGC_itc_write( A3CD97, &itc_status );

                            /* Write parameter to ITC to indicate wind table 
                               has been updated. */
			    Envvad.parameter_from_vad[0] = QUE4_ENVWNDVAD;
                            current_time = time(NULL);
			    unix_time( &current_time, &year, &mon, &day, &hr, &min, &sec );
                            Envvad.parameter_from_vad[1] = hr*3600 + min*60 + sec;
			    RPGC_itc_write( ENVVAD, &itc_status );

		        }

		    }

                    /* If alerting needs the vad data, get the output buffer i
                       and fill it.  If vad product is needed, get output 
                       buffer and generate product. */
		    if( vad_alert ){

		        optr = RPGC_get_outbuf_by_name( "VADPARAM", 
                                                        VADPARAM_SIZE, 
                                                        &opstat_vadparam );

                        /* If buffer obtained, set up alert info. Call 
                           A317o2_vad_alert() to fill the alert array. */
		        if( opstat_vadparam == NORMAL ){

			    A317o2_vad_alert( (int *) optr );
			    RPGC_rel_outbuf( optr, FORWARD );

		        } 
                        else{

			    if( opstat_vadparam == NO_MEM ) 
			        RPGC_abort_datatype_because( Vadparam_id, 
                                                             NO_MEM );
			
		        }

		    }

                    /* If VAD product generation required, get output buffer, 
                       and fill it with the product. */
		    if( vad_product ){

		        optr = RPGC_get_outbuf_by_name( "HPLOTS", HPLOTS_SIZE, 
                                                        &opstat_hplots );

                        /* If buffer obtained, then .... */
                        /* Call A31831_vad_prod() to generate the vad product. 
                           Release the product to who ever wants it. */
		        if( opstat_hplots == NORMAL ){

			    A31831_vad_prod( htg_ewt, rms_ewt, hwd_ewt, shw_ewt, 
                                             ipr, optr ); 
			    RPGC_rel_outbuf( optr, FORWARD );

                        /* Else buffer not obtained, don't generate product. */
		        } 
                        else{

			    if( opstat_hplots == NO_MEM ) 
			        RPGC_abort_datatype_because( Hplots_id, 
                                                             NO_MEM );
			
		        }

		    }

                    /* Release the input buffer and exit module. */
		    RPGC_rel_inbuf( ipr );
		    break;
	        }

	    }

            /* Release the current radial. Go get another radial. */
            RPGC_rel_inbuf( ipr );
	    ipr = RPGC_get_inbuf_by_name( "COMBBASE", &ostat );

            /* Else, get output buffer status is not normal, end processing. 
               Else... buffer status bad on attempt to get a input buffer 
               set abort flag to true since the input data stream has halted */
        } 
        else{

	    abort = 1;
            break;

        }

    } /* End of "while(1)" */
    
    /* If program was prematurely aborted .... */
    if( abort ) 
	RPGC_abort();
    
    if( (opstat_vadtmhgt == NO_MEM) 
                  && 
        (opstat_vadparam == NO_MEM) 
                  && 
        (opstat_hplots == NO_MEM) ) 
	RPGC_abort_because( PROD_MEM_SHED );
    
    return 0;

/* End of A317a2_vad_buffer_control(). */
}


/*\///////////////////////////////////////////////////////////////////////////

   Description:
      This module check if the current radial is the first radial 
      of the task, volume, or elevation scan and initializes task 
      variables appropiatly. It also makes a local copy of the 
      adaptable parameters once each volume scan for use during 
      the following volume scan.

      The weather mode currently is set up as either clear air 
      or convective mode. However, the vad algorithm would like 
      to know the type of the precipitation (i.e. rain vs. snow)
      The rain/snow distinction is not currently possible with
      the existing set of algorithms. Since, light rain and snow 
      are both associated with weak reflectivity and the 
      precipitation fall speed is slow, the errors in applying 
      the rain equation to a snow situation will be very small. 

   Inputs:

      iptr - pointer to radial.

   Ouputs:
      nrstat - radial status.
      htg_ewt - heights for supplemental VADS.
      npt_ewt - number of points for supplemental VADs.
      rms_ewt - RMS error for supplemental VADs.
      hwd_ewt - horizontal wind direction for supplemental VADs.
      shw_ewt - horizontal wind speed for supplemental VADs.

   Returns:
      Always returns 0.

/////////////////////////////////////////////////////////////////////////\*/
int A317e2_vad_init( char *iptr, int *nrstat, float *htg_ewt, int *npt_ewt, 
                     float *rms_ewt, float *hwd_ewt, float *shw_ewt ){

    /* Initialized data */
    static int new_task = 1;
    static int prev_volno;

    /* Local variables. */
    int i, j, k, ht, beg_elv, beg_vlm, celv, ltime;
    Base_data_header *bhd = (Base_data_header *) iptr;

    /* Get radial status flag. */
    *nrstat = bhd->status;

    /* If radial is not a good intermediate radial, perform more checks. */
    if( *nrstat == GOODINT )
        return 0;

    /* Set BEG_VLM to true if radial is the beginning of a volume scan. 
       Set BEG_ELV to true if radial is the beginning of a elevation scan */
    beg_vlm = 0;
    if( *nrstat == GOODBVOL )
        beg_vlm = 1;

    beg_elv = 0;
    if( *nrstat == GOODBEL )
        beg_elv = 1;

    /* If the current radial is the beginning of a elevation or volume 
       scan, or if VAD is a new task, go into initialization if block. */
    if( beg_elv || beg_vlm || new_task ){

        /* Update the AVSET information. */
        if( beg_elv && bhd->last_ele_flag ){

            RPGC_log_msg( GL_INFO, "Beginning of Last Elevation In VCP\n" );
            A317w2_update_avset_info( bhd );

        }

        /* Set the azimuthal elevation based save areas to missing. */
	for( i = 0; i < NAZIMS; ++i ){

	    A317vi->azm[i] = MISSING;
	    A317vi->ve[i] = MISSING;
	    A317ec->azm_ewt[i] = MISSING;
	    A317ec->ve_ewt[i] = MISSING;

        }

        /* Reset the number of radials and elevation angle average values. */
	A317vi->tnradials = 0;
	A317ec->tnradials_ewt = 0;
	A317vi->nradials = -1;
	A317ec->nradials_ewt = -1;

	A317vi->sum_elv = 0.f;
	A317ec->sum_elv_ewt = 0.f;
	A317vi->sum_ref = 0.f;

        /* Perform the following initializations if radial is the beginning 
           of a volume scan or if VAD is a new task. */
	if( beg_vlm || new_task ){

            /* Save a copy of the algorithm and site adaptation data. 
               Convert the parameters to MKS if they are not already in MKS. */

            /* Convert vad range to meters */
	    A317va->vad_rng = Vad.anal_range * 1e3f;
	    A317ec->vad_rng_ewt = A317va->vad_rng * SCALE_RNG_EWT;

            /* Compute offset to the bin of the vad_rng and vad_rng_ewt.  Need to 
               subtract 1 to account for 0 (C) vs unit (FORTRAN) indexed arrays. 
               These "bin" values are used to index the vel and ref moment data. */
	    A317vi->irbin = 
               (int) ((double) A317va->vad_rng / (double) bhd->surv_bin_size) - 1;
	    A317ec->irbin_ewt = 
               (int) ((double) A317ec->vad_rng_ewt / (double) bhd->surv_bin_size) - 1;
	    A317vi->ivbin = 
               (int) ((double) A317va->vad_rng / (double) bhd->dop_bin_size) - 1;
	    A317ec->ivbin_ewt = 
               (int) ((double) A317ec->vad_rng_ewt / (double) bhd->dop_bin_size) - 1;

            /* Save the adaptation data parameters for this volume which are as 
               follows:  Minimum number of points, Azimuth start angle, Azimuth 
               ending angle, RMS threshold, Symetry threshold, and number of 
               fit tests. */
	    A317va->minpts = Vad.min_samples;
	    A317va->azm_beg = Vad.start_azimuth;
	    A317va->azm_end = Vad.end_azimuth;
	    A317va->th_rms = Vad.thresh_velocity;
	    A317va->tsmy = Vad.symmetry;
	    A317va->fit_tests = Vad.num_fit_tests;

            /* Convert radar height from feet to meters. */
	    A317va->rh = ((float) Siteadp.rda_elev)*FT_TO_M;

            /* Save weather mode */
	    if( bhd->weather_mode == CLEAR_AIR_MODE ) 
	        A317va->wmode = 1;

	    else 
	        A317va->wmode = 2;

	    A317vi->celv = 0;
	    A317vi->aztst = A317va->azm_beg >= A317va->azm_end;
	    A317vi->dec_last = RE_M;
	    A317vi->svw_last = 0.f;

            /* Perform the following if VAD is a new task. */
	    if( new_task ){

	        new_task = 0;

                /* Set the current volume to first volume, previous volume 
                   number to negative number and assign the date as missing. */
		Cvol = 0;
		prev_volno = -1;
		A317vd->date = MISSING;

                /* For all save areas which are volume based do the following 
                   assign the time as missing */
		for( i = 0; i < NVOL; ++i ){

		    A317vd->time[i] = MISSING;

                    /* For the height based volumetric save areas assign them 
                       all to missing. */
		    for( j = 0; j < MAX_VAD_HTS; ++j ){

			A317ve->vhtg[i][j] = MISSING;
			A317ve->vnpt[i][j] = MISSING;
			A317ve->vrms[i][j] = MISSING;
			A317ve->vhwd[i][j] = MISSING;
			A317ve->vshw[i][j] = MISSING;

		    }

		}

                /* Set the elevation based save areas to missing. */
		for( j = 0; j < MAX_VAD_ELVS; ++j ){

		    A317vd->htg[j] = MISSING;
		    A317vd->rms[j] = MISSING;
		    A317vd->hwd[j] = MISSING;
		    A317vd->shw[j] = MISSING;
		    A317vd->svw[j] = MISSING;
		    A317vd->div[j] = MISSING;

		}

                /* Set the supplemental Vad save areas to missing */
		for( j = 0; j < NELVEWT; ++j ){

		    htg_ewt[j] = MISSING;
		    npt_ewt[j] = MISSING;
		    rms_ewt[j] = MISSING;
		    hwd_ewt[j] = MISSING;
		    shw_ewt[j] = MISSING;

		}

	    }

            /* Perform the following initialization if radial status 
               indicates beginning of a volume scan. */
	    if( beg_vlm ){

                /* Get the current volume number. */
                /* If the previous volume number NE current volume number then   
                   increment current volume ptr and update previous volume number. */
		Volno = RPGC_get_buffer_vol_num( iptr );

		if( prev_volno != Volno ){

		    ++Cvol;
		    prev_volno = Volno;

		}

                /* If the current volume wraps around to the beginning then   
                   set the current pointer back to the beginning. */
		if( Cvol >= NVOL ) 
		    Cvol -= NVOL;
		    
                /* Save the current volume date and time from the Scan Summary Table. */
                Summary = RPGC_get_scan_summary( Vol_stat.volume_scan );
                if( Summary != NULL ){

		    A317vd->time[Cvol] = Summary->volume_start_time;
		    A317vd->date = Summary->volume_start_date;
                    Vcpno = Summary->vcp_number;

                    A3183a_cnvtime( A317vd->time[Cvol]*1000, &ltime );

#ifdef DEBUG
                    RPGC_log_msg( GL_INFO, "---------------------------------------\n" );
                    RPGC_log_msg( GL_INFO, "    VOLUME TIME: %4.4d\n", ltime );
                    RPGC_log_msg( GL_INFO, "---------------------------------------\n" );
#endif

                }
                else {

                    RPGC_log_msg( GL_ERROR, "RPGC_get_scan_summary(%d) Failed\n",
                                  Vol_stat.volume_scan );
		    A317vd->time[Cvol] = 0;
		    A317vd->date = 0;
                    Vcpno = 0;

                }

                /* Set the height based Vad save areas for this volume to the missing 
                   values.  The number of radials count is set to a negative flag */
		for( j = 0; j < MAX_VAD_HTS; ++j ){

		    A317ve->vhtg[Cvol][j] = MISSING;
		    A317ve->vnpt[Cvol][j] = MISSING;
		    A317ve->vrms[Cvol][j] = MISSING;
		    A317ve->vhwd[Cvol][j] = MISSING;
		    A317ve->vshw[Cvol][j] = MISSING;

		    A317ve->hcf1[j] = MISSING;
		    A317ve->hcf2[j] = MISSING;
		    A317ve->hcf3[j] = MISSING;
		    A317ve->nrads[j] = -1;
		    A317ve->ambig_range_ptr[j] = 0;

                    /* Set Azimuthial height based Vad save areas to the missing value */
		    for( i = 0; i < NAZIMS; ++i ){

		        A317ve->hazm[j][i] = MISSING;
		        A317ve->hvel[j][i] = MISSING;
		        A317ve->href[j][i] = MISSING;

		    }

                    /* Set the ambiguos range based save areas to initial value */
		    for( i = 0; i < MAX_NUM_AMBRNG; ++i )
		        A317ve->ambig_range[j][i] = 0;

	        }

                /* Do For All possible elevation cuts. */
                for( k = 0; k < ECUTMAX; k++ ){

                    A317vs->nrads[k][j] = -1;

                    /* Do For All heights. */
                    for( j = 0; j < MAX_VAD_HTS; j++ ){

                        A317vs->vhtg[k][j] = MISSING;
                        A317vs->vrms[k][j] = MISSING;
                        A317vs->vhwd[k][j] = MISSING;
                        A317vs->vshw[k][j] = MISSING;
                        A317vs->velv[k][j] = MISSING;

                        A317vs->hcf1[k][j] = MISSING;
                        A317vs->hcf2[k][j] = MISSING;
                        A317vs->hcf3[k][j] = MISSING;
                        A317vs->ambig_range_ptr[k][j] = 0;

                        /* Set Azimuthial height based Vad save areas to the missing value */
                        for( i = 0; i < NAZIMS; ++i ){

                            A317vs->hazm[k][j][i] = MISSING;
                            A317vs->hvel[k][j][i] = MISSING;
                            A317vs->href[k][j][i] = MISSING;

                        }

                        A317vs->href_avg[k][j] = MISSING;

                        /* Set the ambiguos range based save areas to initial value */
                        for( i = 0; i < MAX_NUM_AMBRNG; ++i )
                            A317vs->ambig_range[k][j][i] = 0;

                    }

                }

                /* Set the elevation based save variables to missing value */
		for( j = 0; j < MAX_VAD_ELVS; ++j ){

		    A317vd->htg[j] = MISSING;
		    A317vd->rms[j] = MISSING;
		    A317vd->hwd[j] = MISSING;
		    A317vd->shw[j] = MISSING;
		    A317vd->svw[j] = MISSING;
		    A317vd->div[j] = MISSING;

		}

                /* Set the extra vad elevation based save variables to missing */
		for( j = 0; j < NELVEWT; ++j ){

		    htg_ewt[j] = MISSING;
		    npt_ewt[j] = MISSING;
		    rms_ewt[j] = MISSING;
		    hwd_ewt[j] = MISSING;
		    shw_ewt[j] = MISSING;

                }

                /* Initialize the Vad number of heights.  Then extract the 
                   heights from adaptation data.  The criteria is a number 
                   greater than zero is a good height.  When found the number 
                   of Vad heights for this volume is incremented and the 
                   height value saved */
		A317ve->vnhts[Cvol] = 0;
		A317vs->vnhts = 0;
                A317vs->vnels = 0;

		for( i = 0; i < MAX_VAD_HTS; ++i ){

		    if( Prodsel.vad_rcm_heights.vad[i] > 0 ){

		        A317ve->vhtg[Cvol][A317ve->vnhts[Cvol]] = 
                                (float) Prodsel.vad_rcm_heights.vad[i];
			++A317ve->vnhts[Cvol];

                        for( k = 0; k < ECUTMAX; k++ )
		            A317vs->vhtg[k][A317vs->vnhts] = 
                                    (float) Prodsel.vad_rcm_heights.vad[i];

			++A317vs->vnhts;

		    }

	        }

                /* Calculate the optimum slant ranges with elevation cuts 
                   associated for the heights from adaptation data */
		A317b2_calc_opt_slrng( bhd );

	    }

	}

        /* If this is the beginning of elevation or volume then get the 
           current elevation, and set the elevation based save areas to 
           the missing value. */
	if( beg_elv || beg_vlm ){

            /* Need to subtract 1 to account for unit vs. zero indexing
               of arrays. */
	    A317vi->celv = Remapped_rpg_elev_ind[bhd->rpg_elev_ind] - 1;

	    A317vd->htg[A317vi->celv] = MISSING;
	    A317vd->rms[A317vi->celv] = MISSING;
	    A317vd->hwd[A317vi->celv] = MISSING;
	    A317vd->shw[A317vi->celv] = MISSING;
	    A317vd->svw[A317vi->celv] = MISSING;
	    A317vd->div[A317vi->celv] = MISSING;

            /* If the current elevation is < the number of elevations then 
               set the supplemental VAD save areas to the missing value. */
	    if( A317vi->celv < MAX_NELVEWT ){

	        htg_ewt[A317vi->celv] = MISSING;
	        npt_ewt[A317vi->celv] = MISSING;
	        rms_ewt[A317vi->celv] = MISSING;
	        hwd_ewt[A317vi->celv] = MISSING;
	        shw_ewt[A317vi->celv] = MISSING;

	    }

            /* Set the max and min table heights to their initial values. */
	    Ceminht = -1;
	    Cemaxht = -1;
            Ceminht_s = -1;
            Cemaxht_s = -1;

            /* Loop through the elevation cut numbers assigned for each height 
               until the current elevation cut is found.  Then, save the height 
               as the max and min heights.  The max height is always updated as 
               each height is detected. The min height is only updated on the 
               first detected height */
	    for( ht = 0; ht < MAX_VAD_HTS; ++ht ){

	        if( A317ve->elcn[ht] == bhd->rpg_elev_ind ){

		    Cemaxht = ht;
		    if( Ceminht < 0 ) 
		        Ceminht = ht;
			
		    A317ve->vnpt[Cvol][ht] = MISSING;
		    A317ve->vrms[Cvol][ht] = MISSING;
	            A317ve->vhwd[Cvol][ht] = MISSING;
		    A317ve->vshw[Cvol][ht] = MISSING;


		    A317ve->hcf1[ht] = MISSING;
		    A317ve->hcf2[ht] = MISSING;
		    A317ve->hcf3[ht] = MISSING;

		    A317ve->nrads[ht] = -1;
                    A317ve->ambig_range_ptr[ht] = 0;

                    /* Set Azimuthial height based Vad save areas to missing value */
		    for( i = 0; i < NAZIMS; ++i ){

		        A317ve->hazm[ht][i] = MISSING;
		        A317ve->hvel[ht][i] = MISSING;
		        A317ve->href[ht][i] = MISSING;

		    }

                    /* Set the ambiguous range based save areas to initial value */
		    for( i = 0; i < MAX_NUM_AMBRNG; ++i )
		        A317ve->ambig_range[ht][i] = 0;

		}

	    }

            /* Use the RPG elevation index as array index. */
            celv = Remapped_rpg_elev_ind[bhd->rpg_elev_ind] - 1;

            for( ht = 0; ht < MAX_VAD_HTS; ++ht ){

                if( A317vs->elcn[celv][ht] == bhd->rpg_elev_ind ){

                    Cemaxht_s = ht;
                    if( Ceminht_s < 0 )
                        Ceminht_s = ht;

                    A317vs->vrms[celv][ht] = MISSING;
                    A317vs->vhwd[celv][ht] = MISSING;
                    A317vs->vshw[celv][ht] = MISSING;


                    A317vs->hcf1[celv][ht] = MISSING;
                    A317vs->hcf2[celv][ht] = MISSING;
                    A317vs->hcf3[celv][ht] = MISSING;

                    A317vs->nrads[celv][ht] = -1;
                    A317vs->ambig_range_ptr[celv][ht] = 0;

                    /* Set Azimuthial height based Vad save areas to missing value */
                    for( i = 0; i < NAZIMS; ++i ){

                        A317vs->hazm[celv][ht][i] = MISSING;
                        A317vs->hvel[celv][ht][i] = MISSING;
                        A317vs->href[celv][ht][i] = MISSING;

                    }

                    A317vs->href_avg[celv][ht] = MISSING;

                    /* Set the ambiguous range based save areas to initial value */
                    for( i = 0; i < MAX_NUM_AMBRNG; ++i )
                        A317vs->ambig_range[celv][ht][i] = 0;

                }

            }

	}

    }

    return 0;

} /* End of A317e2_vad_init(). */


/*\///////////////////////////////////////////////////////////////////////////

   Description:
      The function computes the slant range closest to optimum.

   Input:
      hdr - base data radial header.

   Returns:
      -1 on error, 0 on success.

///////////////////////////////////////////////////////////////////////////\*/
int A317b2_calc_opt_slrng( Base_data_header *hdr ){

    /* Local variables */
    int rpgvcp, uniq_cuts, prev_ind, ht, elv, cnelv;
    int i, j, n_heights;
    double vad_rng, sin_elv, csqrt, dsl, mindsl;
    double elang[ECUTMAX], slrng[MAX_VAD_ELVS][MAX_VAD_HTS];

    Vcp_struct *vcp_data = NULL;
    Ele_attr *ele = NULL;
    short *rdccon = NULL;
    unsigned short *suppl = NULL, suppl_flag = 0;

    /* AVSET support. */
    int avset_active, avset_vcp;
    float max_range, avset_last_elev, avset_last_elev_m1;

    /* Check if AVSET is active.  If not active, set the last elevation
       angle to 60.0. */
    A317y2_get_avset_info( &avset_active, &avset_vcp, &avset_last_elev,
                           &avset_last_elev_m1 );

    if( !avset_active )
        avset_last_elev_m1 = 60.0;

    else if( avset_last_elev_m1 < 6.0 )
        avset_last_elev_m1 = avset_last_elev;

    else
        RPGC_log_msg( GL_INFO, "AVSET Active.  Last Elev to Process: %10.2f\n",
                      avset_last_elev_m1 );

    /* Initialize elang and slrng arrays.  This prevent valgrind complaints. */
    for( i = 0; i < ECUTMAX; i++ )
       elang[i] = -1.0;

    for( i = 0; i < MAX_VAD_ELVS; i++ ){

       for( j = 0; j < MAX_VAD_HTS; j++ )
          slrng[i][j] = MISSING;

    }

    /* Get the VCP number from radial header. */
    rpgvcp = hdr->vcp_num;

    /* Extract all elevation angles in the current VCP. */
    vcp_data = RPGCS_get_vcp_data( rpgvcp );
    if( vcp_data == NULL ){

       RPGC_log_msg( GL_ERROR, "Error Accessing VCP Data\n" );
       return -1;

    }

    cnelv = vcp_data->n_ele;
    rdccon = (short *) RPGCS_get_elev_index_table( rpgvcp );
    if( rdccon == NULL ){

       RPGC_log_msg( GL_ERROR, "Error Accessing RDCCON Data\n" );
       return -1;

    }

    suppl = (unsigned short *) RPGCS_get_suppl_flags_table( rpgvcp );
    if( suppl == NULL )
       RPGC_log_msg( GL_ERROR, "Error Accessing Supplemental Flags Data\n" );

    /* Initialize the RPG elevation index remapping. */
    for( elv = 0; elv < cnelv; ++elv ){

       Remapped_rpg_elev_ind[elv] = UNDEFINED_INDEX;
       Rpg_elev_ind[elv] = UNDEFINED_INDEX;

    }

    /* Do For All Elevations in the current volume coverage pattern */
    uniq_cuts = 0;
    prev_ind = 0;
    for( elv = 0; elv < cnelv; ++elv ){

        suppl_flag = 0;
        if( suppl != NULL )
           suppl_flag = suppl[elv];

	if( (rdccon[elv] != prev_ind)
                         &&
            !(suppl_flag & RDACNT_SUPPL_SCAN) ){

            /* Assign Ele_attr pointer. */
            ele = (Ele_attr *) &vcp_data->vcp_ele[elv][0];

            /* Convert angle from BAMS to deg. */
            elang[uniq_cuts] = 
               (double) ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, ele->ele_angle );

            /* Track the remapped RPG elevation index.  Need to add one since 
               elevation index is unit indexed.  Rpg_elev_ind maps internal cut
               number, which is sequential, into actual rpg cut number for this
               VCP.  */
            Remapped_rpg_elev_ind[rdccon[elv]] = uniq_cuts + 1;
            Rpg_elev_ind[uniq_cuts] = rdccon[elv];

            if( elang[uniq_cuts] > avset_last_elev_m1 )
               uniq_cuts--;

	    ++uniq_cuts;
            prev_ind = rdccon[elv];

	}

    }

#ifdef DEBUG
    LE_send_msg( GL_INFO, "RPG Elevation Index Remapping Table\n" );
    for( elv = 0; elv < cnelv; ++elv ){
       i = Remapped_rpg_elev_ind[rdccon[elv]]; 
       if( i != UNDEFINED_INDEX )
          LE_send_msg( GL_INFO, "rdccon[%d]: %d, remap_cut: %d, elang: %6.2f\n",
                       elv, rdccon[elv], i, elang[i-1] );
    }

    LE_send_msg( GL_INFO, "RPG Elevation Index Table\n" );
    for( elv = 0; elv < uniq_cuts; ++elv ){
       i = Rpg_elev_ind[elv]; 
       if( i != UNDEFINED_INDEX )
          LE_send_msg( GL_INFO, "Cut #: %d, RPG Elev Index: %d\n", elv, i );
    }
#endif

    /* Free memory associated with VCP data and RDCCON data. */
    free( vcp_data );
    free( rdccon );
    if( suppl != NULL )
       free( suppl );

    /* Calculate the sign of the elevation angle. */
    for( elv = 0; elv < uniq_cuts; ++elv ){

	sin_elv = sin(elang[elv] * DEGTORAD);

        /* Do For All heights in current volume. */
	n_heights = A317ve->vnhts[Cvol];
	for( ht = 0; ht < n_heights; ++ht ){

            /* Calculate the value under radical in slant range equation
               equation to do domain check. */
	    csqrt = (sin_elv*sin_elv) + (A317ve->vhtg[Cvol][ht]
		    - (float) Siteadp.rda_elev)*SLCON*FT_TO_KM;
	    if( csqrt > 0.0 )
		slrng[elv][ht] = (-sin_elv + sqrt(csqrt)) * RE_IR_KM;
	     
            else{ 

                /* Illegal value under radical, mark element as missing */
		slrng[elv][ht] = MISSING;

	    }

            /* Check if slant range determined is within the interval   
               allowed. */
	    if( slrng[elv][ht] > MAXSLR ) 
		slrng[elv][ht] = MISSING;
	    
	    if( slrng[elv][ht] < 0.f ) 
		slrng[elv][ht] = MISSING;
	    
	}

        /* Initialize unused entries in slrng to MISSING */
	for( ht = n_heights+1; ht < MAX_VAD_HTS; ++ht) 
	    slrng[elv][ht] = MISSING;
	
    }

    /* Clear out the current volume storage area for optinum slant range   
       and elevation angle to find it on. */
    for( ht = 0; ht < MAX_VAD_HTS; ++ht ){

	A317ve->slrn[ht] = MISSING;
	A317ve->elcn[ht] = MISSING;


    }

    /* Clear out the current volume storage agea for slant range and 
       elevation angle to do the VAD. */
    for( elv = 0; elv < ECUTMAX; elv++ ){
 
        for( ht = 0; ht < MAX_VAD_HTS; ht++ ){

	    A317vs->slrn[elv][ht] = MISSING;
	    A317vs->elcn[elv][ht] = MISSING;

        }

    }

    /* Compute the slant range closest to optimum. */
    vad_rng = A317va->vad_rng / 1000.0;
    for( ht = 0; ht < MAX_VAD_HTS; ++ht ){

	mindsl = 999999.f;
	for( elv = 0; elv < uniq_cuts; ++elv ){

            /* For EVWP, need to cap the range based on elevation angle. */
            if( Vad.max_proc_range <= CLIPPED_RANGE )
               max_range = Vad.max_proc_range;

            else{

                if( elang[elv] <= 0.5 /* degrees */ )
                    max_range = CLIPPED_RANGE;

                else
                    max_range = CLIPPED_RANGE + (elang[elv] - 0.5)*20.0f; /* 2 km for each 0.1 deg change */
 
                if( max_range > Vad.max_proc_range )
                    max_range = Vad.max_proc_range;

#ifdef DEBUG
                if( ht == 0 )
                    LE_send_msg( GL_INFO, "Max Range at %10.4f deg Clipped to %10.4f km\n",
                                 elang[elv], max_range );
#endif
            }

	    if( slrng[elv][ht] != MISSING ){

		dsl = fabs( slrng[elv][ht] - vad_rng );
		if( dsl < mindsl ){

		    mindsl = dsl;
		    A317ve->slrn[ht] = slrng[elv][ht];
		    A317ve->elcn[ht] = Rpg_elev_ind[elv];
		    A317ve->refbin[ht] = (int) (A317ve->slrn[ht] / .001f / (double) hdr->surv_bin_size);
		    A317ve->velbin[ht] = (int) (A317ve->slrn[ht] / .001f / (double) hdr->dop_bin_size);

                    /* Subtract 1 to account for 0 vs unit indexed moment data. */
                    --A317ve->refbin[ht];
                    --A317ve->velbin[ht];

		}
  
                if( (slrng[elv][ht] >= Vad.min_proc_range) 
                               &&
                    (slrng[elv][ht] <= max_range) ){

                    /* Save data for new method of VAD. */
		    A317vs->slrn[elv][ht] = slrng[elv][ht];
		    A317vs->elcn[elv][ht] = Rpg_elev_ind[elv];
		    A317vs->refbin[elv][ht] = 
                       (int) (A317vs->slrn[elv][ht] / .001f / (double) hdr->surv_bin_size);
		    A317vs->velbin[elv][ht] = 
                       (int) (A317vs->slrn[elv][ht] / .001f / (double) hdr->dop_bin_size);

                    /* Subtract 1 to account for 0 vs unit indexed moment data. */
                    --A317vs->refbin[elv][ht];
                    --A317vs->velbin[elv][ht];

                }

	    }

	}

    }

    /* Store the number of elevation cuts this VCP. */
    A317vs->vnels = uniq_cuts;
    
    return 0;

} /* End of A317b2_calc_opt_slrng(). */


/*\////////////////////////////////////////////////////////////////

   Description:
      This module is the main processing routine of the vad 
      algorithm. It receives data via the calling sequence and 
      the common area A317VI. It calls the various routines to 
      perform the various tasks required to generate the wind 
      speed, wind direction, divergence, and vertical velocity 
      for the region of the atmosphere enclosed within the slant 
      range VAD_RNG and azimuth limits AZM_BEG and AZM_END. 

   Inputs:
      cvht - current height index.

   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////\*/
int A317c2_vad_proc_hts( int cvht ){

    /* Local variables */
    int i, sym, fit_tests;
    double r1, r2, elv_rad, avg_elv;

    /* AVG_ELV is the average elevation angle for the scan, in degrees. */
    avg_elv = A317vi->sum_elv / A317vi->tnradials;

    /* AVG_RAD is the average elevation angle for the vad data in radians. */
    elv_rad = (double) avg_elv * DEGTORAD;

    /* If there are no radials... go to end and exit. */
    if( A317ve->nrads[cvht] < A317va->minpts )
	return 0;

    /* Do For All fit tests. */
    fit_tests = A317va->fit_tests;
    for( i = 0; i < fit_tests; ++i ){

        /* Compute coefficients of least squares fitted curve, in m/s. */
	A317h2_vad_lsf( A317ve->nrads[cvht], &A317ve->hazm[cvht][0], 
		        &A317ve->hvel[cvht][0], &A317ve->vnpt[Cvol][cvht], 
                        &A317ve->hcf1[cvht], &A317ve->hcf2[cvht], 
                        &A317ve->hcf3[cvht] );

        /* If no data exists for vad analysis then CF1, CF2, and CF3 will 
           all be equal to missing, and NPT(CVOL,CELV) will equal 0. 
           Exit this module. */
	if( A317ve->vnpt[Cvol][cvht] < A317va->minpts )
	    return 0;

        /* Compute horizontal wind direction, in degrees (& check arguments). */
	if( (A317ve->hcf3[cvht] != 0.f) && (A317ve->hcf2[cvht] != 0.f) )
	    A317ve->vhwd[Cvol][cvht] = PI_CONST - 
                atan2( (double) A317ve->hcf3[cvht], (double) A317ve->hcf2[cvht]);

	else 
	    A317ve->vhwd[Cvol][cvht] = 0.f;
	
        /* Check if wind direction computes to be negative, then convert to 
           degrees. */
	if( A317ve->vhwd[Cvol][cvht] < 0.f ) 
	    A317ve->vhwd[Cvol][cvht] += 2.0*PI_CONST;
	
	A317ve->vhwd[Cvol][cvht] /= DEGTORAD;

        /* Compute the square root of the mean squared deviations between the 
           velocity data points and the least squares fitted curve, in m/s. */
	A317i2_vad_rms(  A317ve->nrads[cvht], &A317ve->hazm[cvht][0], 
                         &A317ve->hvel[cvht][0], A317ve->vhwd[Cvol][cvht], 
                         A317ve->hcf1[cvht], A317ve->hcf2[cvht], 
                         A317ve->hcf3[cvht], &A317ve->vrms[Cvol][cvht] );

        /* If not the last time through this loop, perform fit test. 
           If the last time through this loop, skip fit test since it will 
           only perform un-needed calculations. */
	if( i < (fit_tests-1) ){

            /* Perform fit test to remove velocity data which lies more than 
               one rms away from the least squares fitted curve and toward 
               the zero velocity line, then go back up and perform least 
               squares fitting again. */
	    A317j2_fit_test( A317ve->nrads[cvht], &A317ve->hazm[cvht][0], 
                             &A317ve->hvel[cvht][0], A317ve->hcf1[cvht], 
                             A317ve->hcf2[cvht], A317ve->hcf3[cvht], 
		             A317ve->vhwd[Cvol][cvht], 
                             A317ve->vrms[Cvol][cvht] );

	}

    }

    /* Compute symmetry of the least squares fitted curve. */
    A317k2_sym_chk( A317ve->hcf1[cvht], A317ve->hcf2[cvht], 
                    A317ve->hcf3[cvht], A317va->tsmy, &sym );

    /* Only continue with calculations if the rms is less than the 
       threshold and the fit is symmetric. */
    if( (A317ve->vrms[Cvol][cvht] < A317va->th_rms) && sym ){

        /* Compute horizontal wind speed in m/s. */
	r1 = (double) A317ve->hcf2[cvht];
	r2 = (double) A317ve->hcf3[cvht];
	A317ve->vshw[Cvol][cvht] = sqrt( (r1*r1) + (r2*r2) ) / cos(elv_rad);

    }
    
#ifdef DEBUG
    /* Only print the winds that are not missing. */
    if( A317ve->vshw[Cvol][cvht] != MISSING ){

        LE_send_msg( GL_INFO, "A317ve->vhtg[%d][%d]:        %10.4f\n",
                     Cvol, cvht, A317ve->vhtg[Cvol][cvht] );
        LE_send_msg( GL_INFO, "-->A317ve->slrn[%d]:         %10.4f\n",
                     cvht, A317ve->slrn[cvht] );
        LE_send_msg( GL_INFO, "-->A317ve->nrads[%d]:        %4d\n",
                     cvht, A317ve->nrads[cvht] );
        LE_send_msg( GL_INFO, "-->A317ve->vnpt[%d][%d]:     %4d\n",
                     Cvol, cvht, A317ve->vnpt[Cvol][cvht] );
        LE_send_msg( GL_INFO, "-->A317ve->vshw[%d][%d]:     %10.4f\n",
                     Cvol, cvht, A317ve->vshw[Cvol][cvht] );
        LE_send_msg( GL_INFO, "-->A317ve->vhwd[%d][%d]:     %10.4f\n",
                     Cvol, cvht, A317ve->vhwd[Cvol][cvht] );
        LE_send_msg( GL_INFO, "-->A317ve->vrms[%d][%d]:     %10.4f\n",
                     Cvol, cvht, A317ve->vrms[Cvol][cvht] );

    }
#endif

    return 0;

} /* End of A317c2_vad_proc_hts(). */


#define RPFV1_D2		0.0114
#define RPFV2_D2		2.6789115
#define RPFV3_D2		0.0758695
#define RPFV4_D2		0.0068635

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      This module decodes radial base data and saves the data that 
      is needed by the VAD processing of selected heights. 

   Inputs:
      ipr - pointer to radial message.
      ht - height index.

   Returns:
      Always returns 0.

/////////////////////////////////////////////////////////////////////////\*/
int A317d2_vad_data_hts( char *ipr, int ht ){

    /* Local variables */
    int i, hts_nv[MAX_VAD_HTS], bi_dbz, firstv, lastv, firstr, lastr;
    int n_dop_bins;
    float pfv, ht_sea, elevation, azimuth;
    float hts_sum_vel[MAX_VAD_HTS];
    double r1, d1;

    Base_data_header *bdh = (Base_data_header *) ipr;
    Moment_t vel, *iv = (Moment_t *) (ipr + bdh->vel_offset);
    Moment_t ref, *iz = (Moment_t *) (ipr + bdh->ref_offset);

    static float hts_htcon[MAX_VAD_HTS];

    /* Get the azimuth angle (degrees) of the radial. */
    azimuth = bdh->azimuth;

    /* Get the elevation angle (degrees) of the radial. */
    elevation = bdh->elevation;

    /* To avoid compiler warning. */
    bi_dbz = 0;

    /* If azimuth limits are equal, no sector is specified, skip checks */
    if( A317va->azm_end != A317va->azm_beg ){

        /* AZTST is a logical variable indicating whether or not the vad 
           azimuth sectors include the 360/0 degree crossover. 

              1 = it does include the 0 degree crossover. 
              0  = it does NOT include the 0 degree crossover. 

           It is assumed that AZM_END  is clockwise with respect to AZM_BEG, 
           and that the desired sector is between these limits (i.e. the 
           sector greater than AZM_BEG and less than AZM_END) */
	if( A317vi->aztst ){

	    if( (azimuth < A317va->azm_beg) 
                        && 
                (azimuth > A317va->azm_end) ){ 

                /* if radial is not within sector, do not process it. */
		return 0;

	    }

	} 
        else{

	    if( (azimuth < A317va->azm_beg) 
                        || 
                (azimuth > A317va->azm_end) ){ 

                /* If radial is not within sector, do not process it. */
		return 0;

	    }

	}

    }

    /* Get the first and last good velocity bins of the radial.  Make sure
       we don't process beyond BASEDATA_VEL_SIZE bins.  */
    firstv = bdh->dop_range - 1;
    n_dop_bins = bdh->n_dop_bins;
    if( n_dop_bins > BASEDATA_VEL_SIZE )
       n_dop_bins = BASEDATA_VEL_SIZE;
    lastv = firstv + n_dop_bins - 1;

    /* Get the first and last good reflectivity bins of the radial */
    firstr = bdh->surv_range - 1;
    lastr = firstr + bdh->n_surv_bins - 1;

    /* If this is the first radial of a new elevation scan */
    if( A317ve->nrads[ht] < 0 ){

	A317ve->nrads[ht] = 0;

        /* Compute parts of the HT_SEA equation that are repeative below */
	r1 = A317ve->slrn[ht]*1e3f;;
	hts_htcon[ht] = (A317va->rh + (r1*r1)/HTS_CONST)/1e3f;

        /* Set initial values for Nyquist velocity and azimuth sector start. */
	A317ve->ambig_range_ptr[ht] = 0;
	A317ve->ambig_range[ht][A317ve->ambig_range_ptr[ht]] = bdh->nyquist_vel;
	A317ve->ambig_azlim[ht][A317ve->ambig_range_ptr[ht]] = azimuth * 10.f;

        RPGCS_set_velocity_reso( (int) bdh->dop_resolution );

    } 
    else{

       int ind = A317ve->ambig_range_ptr[ht];

       /* Save range of ambiguous ranges for Nyquist velocity plotting. */
	if( bdh->nyquist_vel != A317ve->ambig_range[ht][ind] ){

	    ++A317ve->ambig_range_ptr[ht];

	    if( A317ve->ambig_range_ptr[ht] >= MAX_NUM_AMBRNG ) 
		A317ve->ambig_range_ptr[ht] = MAX_NUM_AMBRNG - 1;
	    
	    A317ve->ambig_range[ht][ind] = bdh->nyquist_vel;
	    A317ve->ambig_azlim[ht][ind] = azimuth * 10.f;

	}

    }

    /* If weather mode is clear air (WMODE=1) then, average the three 
       adjacient gates in range for each radial, and we don't need to 
       sum the reflectivity. */
    if( A317va->wmode == CLEAR_AIR_MODE ){

	hts_sum_vel[ht] = 0.f;
	hts_nv[ht] = 0;
	for( i = -1; i <= 1; ++i ){

            /* Make sure velocity data is not below threshold or 
               before or after the first or last good bin, respectively */
	    if( ((A317ve->velbin[ht] + i) <= lastv)
                              && 
                ((A317ve->velbin[ht] + i) >= firstv) ){

                vel = iv[A317ve->velbin[ht] + i];
		if( vel > BASEDATA_RDRNGF) {

                    /* Sum the velocities & increment the number of bins 
                       in the sumation */
		    hts_sum_vel[ht] += RPGCS_velocity_to_ms( vel );
		    ++hts_nv[ht];

		}

	    }

	}

        /* Compute average velocity and store in HREF, HVEL, & HAZM arrays */
	if( hts_nv[ht] > 0 ){

	    A317ve->hvel[ht][A317ve->nrads[ht]] = hts_sum_vel[ht] / hts_nv[ht];

            /* If reflectivity is good store the IZ value ... */
	    if( (A317ve->refbin[ht] >= firstr)
                              && 
                (A317ve->refbin[ht] <= lastr) ){

                ref =  iz[A317ve->refbin[ht]];

                if( ref > BASEDATA_RDRNGF) 
		    A317ve->href[ht][A317ve->nrads[ht]] = ref;

	    } 
            else{

                /* Otherwise store the Threshold value */
		A317ve->href[ht][A317ve->nrads[ht]] = 2;

	    }

	    A317ve->hazm[ht][A317ve->nrads[ht]] = azimuth;
	    ++A317ve->nrads[ht];

	}

    } 
    else{

        /* Since we're here, it's not clear air. So sum reflectivity, 
           and put velocity and azimuth angles in HREF, HVEL, & HAZM arrays. 
           also, correct velocity for precipitation fall speed 

           Make sure velocity data is not below threshold or 
           before or after the first or last good bin, respectively */
	if( (A317ve->velbin[ht] >= firstv)
                       && 
            (A317ve->velbin[ht] <= lastv) ){

            vel = iv[A317ve->velbin[ht]];

            if( vel > BASEDATA_RDRNGF ){

                /* Account for bad reflectivity data, set it to the 
                   low reflectivity threshold, 2 in biased units. */
	        if( (A317ve->refbin[ht] >= firstr)
                               && 
                    (A317ve->refbin[ht] <= lastr) ){

                    ref = iz[A317ve->refbin[ht]];
		    if( ref <= BASEDATA_RDRNGF ) 
		        bi_dbz = 2;

		    else
		        bi_dbz = ref;

		}
                else 
		    bi_dbz = 2;
	    
                /* Store the vel, ref and azimuth. */
	        A317ve->hvel[ht][A317ve->nrads[ht]] = RPGCS_velocity_to_ms( vel ); 
	        A317ve->href[ht][A317ve->nrads[ht]] = bi_dbz;
	        A317ve->hazm[ht][A317ve->nrads[ht]] = azimuth;

                /* ZE=10**(REF(IZ(IRBIN))/10.)  see comments below re: HT_SEA & PFV */

                /* HT_SEA equation modified so that the portion which is 
                   the same for each radial is only computed once per 
                   elevation scan.  Essentially: the first part of the 
                   equation is repeative and vad range is converted to 
                   kilometers 
                   before:   
                       HT_SEA=RH+VAD_RN/G**2/(2*IR*RE)+VAD_RNG*SIN(ELEVATION*DTR)
                       HT_SEA=HT_SEA/1000.0 */
	        ht_sea = hts_htcon[ht] + (A317ve->slrn[ht] * sin(elevation * DEGTORAD));

                /* The PFV equation has been modified such that the constant 2.65 is 
                   combined with the constants in the later part of the equation, 
                   i.e. 2.6789115=2.65*1.01091, 0.0758695=2.65*0.02863, and 
                   0.0068635=2.65*0.00259. Also, the conversion of dBZ to 
                   mm**6/m**3 is combined in the front part of the PFV equation, 
                   i.e. before: ZE=10**(REF(IZ(IRBIN))/10), then PFV=2.65*(Z'E'**0.114). 
                   after: PFV=(10**(REF(IZ(IRBIN))*0.0114))..... 
                   before   PFV=2.65*(ZE**0.114)*(1.01091+0.02863*HT_SEA+ 
                   before        0.00259*HT_SEA**2) */
	        d1 = (double) (RPGCS_reflectivity_to_dBZ( bi_dbz ) * RPFV1_D2);
	        pfv = pow(10.0, d1) * (RPFV2_D2 + (ht_sea*RPFV3_D2) + (RPFV4_D2*ht_sea*ht_sea));
	        A317ve->hvel[ht][A317ve->nrads[ht]] += (pfv*sin(elevation * DEGTORAD));

                /* Increment the radial counter. */
	        ++A317ve->nrads[ht];

            }

	}

    }

    return 0;

} /* End of A317d2_vad_data_hts(). */


#define RPFV1_F2		0.0114
#define RPFV2_F2		2.6789115
#define RPFV3_F2		0.0758695
#define RPFV4_F2		0.0068635

/*\/////////////////////////////////////////////////////////////////////
 
   Description:
      This module decodes radial base data and saves the data that 
      is needed by the VAD processing software. 

   Inputs:
      ipr - pointer to radial message.

   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////////\*/
int A317f2_vad_data( char *ipr ){

    /* Local variables */
    Base_data_header *bdh = (Base_data_header *) ipr;
    int i, firstr, firstv, lastr, lastv, nv;
    float sum_vel, elevation, azimuth, dbz;
    double pfv, ht_sea, r1, d1;

    static float htcon = 0.0, vad_rng_km = 0.0;

    Moment_t *iv = (Moment_t *) (ipr + bdh->vel_offset);
    Moment_t *iz = (Moment_t *) (ipr + bdh->ref_offset);
    Moment_t vel, ref;

    /* Decode azimuth angle (degrees) of the radial. */
    azimuth = bdh->azimuth;

    /* Decode elevation angle (degrees) of radial. */
    elevation = bdh->elevation;

    /* Sum elevation and number of radials in elevation scan */
    A317vi->sum_elv += elevation;
    ++A317vi->tnradials;

    /* If azimuth limits are equal, no sector is specified, skip checks */
    if( A317va->azm_end != A317va->azm_beg ){

        /* AZTST is a logical variable indicating whether or not the vad 
           azimuth sectors include the 360/0 degree crossover. 

           true = it does include the 0 degree crossover. 
           false = it does NOT include the 0 degree crossover. 

           it is assumed that AZM_END  is clockwise with respect to AZM_BEG, 
           and that the desired sector is between these limits (i.e. the 
           sector greater than AZM_BEG and less than AZM_END) */
	if( A317vi->aztst ){

	    if( (azimuth < A317va->azm_beg) && (azimuth > A317va->azm_end) ){

                /* If radial is not within sector, do not process it. */
		return 0;

	    }

	} 
        else{

	    if( (azimuth < A317va->azm_beg) || (azimuth > A317va->azm_end) ){ 
		    
                /* If radial is not within sector, do not process it. */
		return 0;

	    }

	}

    }

    /* Get the first and last good velocity bins of the radial. */
    firstv = bdh->dop_range - 1;
    lastv = firstv + bdh->n_dop_bins - 1;

    /* Get velocity resolution flag. */
    RPGCS_set_velocity_reso( (int) bdh->dop_resolution );

    /* If this is the first radial of a new elevation scan */
    if( A317vi->nradials < 0 ){

	A317vi->nradials = 0;

        /* Compute parts of the HT_SEA equation that are repetitive below */
	r1 = A317va->vad_rng;
	vad_rng_km = r1 / 1e3f;
	htcon = (A317va->rh + (r1*r1)/HTS_CONST)/1e3f;

    }

    /* If weather mode is clear air (WMODE=1) then, average the three 
       adjacient gates in range for each radial, and we don't need to 
       sum the reflectivity. */
    if( A317va->wmode == CLEAR_AIR_MODE ){

	nv = 0;
	sum_vel = 0.f;
	for (i = -1; i <= 1; ++i ){

            /* Make sure velocity data is not below threshold or 
               before or after the first and last good bins, respectively */
	    if( ((A317vi->ivbin + i) <= lastv) 
                          && 
                ((A317vi->ivbin + i) >= firstv) ){

                vel = iv[A317vi->ivbin + i];
		if( vel > BASEDATA_RDRNGF ){

                    /* Sum the velocities & increment the number of bins 
                       in the sumation */
		    sum_vel += RPGCS_velocity_to_ms( vel );
		    ++nv;

		}

	    }

	}

        /* Compute average velocity and include in the VE() and AZM() arrays */
	if( nv > 0 ){

	    A317vi->ve[A317vi->nradials] = sum_vel / nv;
	    A317vi->azm[A317vi->nradials] = azimuth;
	    ++A317vi->nradials;

	}

    } 
    else{

        /* Get the first and last good reflectivity bins of the radial */
	firstr = bdh->surv_range - 1;
	lastr = firstr + bdh->n_surv_bins - 1;

        /* Since we're here, it's not clear air. So sum reflectivity, 
           and put velocity and azimuth angles in VE( ) and AZM( ) arrays. 
           also, correct velocity for precipitation fall speed */
        vel = iv[A317vi->ivbin];
        ref = iz[A317vi->irbin];

        /* Make sure velocity data is not below threshold or 
           before or after the first or last good bin, respectively. */
	if( (A317vi->ivbin >= firstv) 
                       && 
            (A317vi->ivbin <= lastv) 
                       && 
            (iv[A317vi->ivbin] > BASEDATA_RDRNGF) ){

            /* Account for bad reflectivity data, set it to the   
               low reflectivity threshold, = -32 dBZ */
	    if( (A317vi->irbin >= firstr)
                           && 
                (A317vi->irbin <= lastr) ){

		if( ref <= BASEDATA_RDRNGF ) 
		    dbz = -32.f;

		else 
		    dbz = RPGCS_reflectivity_to_dBZ( ref );

		
	    } 
            else 
		dbz = -32.f;
	    
            /* Store velocity and azimuth. */
	    A317vi->ve[A317vi->nradials] = RPGCS_velocity_to_ms( vel );
	    A317vi->azm[A317vi->nradials] = azimuth;

            /* ZE=10**(REF(IZ(IRBIN))/10.)  see comments below: HT_SEA & PFV */

            /* HT_SEA equation modified so that the portion which is 
               the same for each radial is only computed once per 
               elevation scan.  Essentially: the first part of the 
               equation is repeative and vad range is converted to 
               kilometers 
               before:  
                     HT_SEA=RH+VAD_RNG**2/(2*IR*RE)+VAD_RNG*SIN(ELEVATION*DTR)
                     HT_SEA=HT_SEA/1000.0 */
	    ht_sea = htcon + (vad_rng_km*sin(elevation*DEGTORAD));

            /* The PFV equation has been modified such that the constant 
               2.65 is combined with the constants in the later part of 
               the equation, i.e. 2.6789115=2.65*1.01091, 0.0758695=
               2.65*0.02863, and 0.0068635=2.65*0.00259. Also, the 
               conversion of dBZ to mm**6/m**3 is combined in the front 
               part of the PFV equation, i.e. 
               before: 
                   ZE=10**(REF(IZ(IRBIN))/10), then PFV=2.65*(Z'E'**0.114).
               after: 
                   PFV=(10**(REF(IZ(IRBIN))*0.0114))..... 

               before:   
                   PFV=2.65*(ZE**0.114)*(1.01091+0.02863*HT_SEA+ 
               before:        0.00259*HT_SEA**2) */

	    d1 = (double) (dbz * RPFV1_F2);
	    pfv = pow(10.0, d1) * (RPFV2_F2 + (RPFV3_F2*ht_sea) +
			                 (RPFV4_F2 * (ht_sea*ht_sea)) );
	    A317vi->ve[A317vi->nradials] += pfv*sin(elevation * DEGTORAD );

            /* Increment the radial counter. */
	    ++A317vi->nradials;

	}

    }

    return 0;

} /* End of A317f2_vad_data() */


#define RPFV1_G2		2.65
#define RPFV2_G2		0.114
#define RPFV3_G2		1.01091
#define RPFV4_G2		0.02863
#define RPFV5_G2		0.00259

/*\////////////////////////////////////////////////////////////////////

   Description:
      This module is the main processing routine of the vad 
      algorithm. It receives data via the calling sequence and 
      the common area A317VI. It calls the various routines to 
      perform the various tasks required to generate the wind 
      speed, wind direction, divergence, and vertical velocity 
      for the region of the atmosphere enclosed within the slant 
      range VAD_RNG and azimuth limits AZM_BEG and AZM_END. 

   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////////\*/
int A317g2_vad_proc( void ){

    /* Local variables */
    int i, fit_tests, npt[MAX_VAD_ELVS], sym;
    float cf1, cf2, cf3, ht_sea, avg_ref, elv_rad, avg_elv;
    double pfv = 0.0, r1, d1;

    /* AVG_ELV is the average elevation angle for the scan, in degrees. */
    avg_elv = A317vi->sum_elv / A317vi->tnradials;

    /* AVG_RAD is the average elevation angle for the vad data in radians. */
    elv_rad = avg_elv * DEGTORAD;

    /* Compute height of the data above ground in meters. */
    r1 = A317va->vad_rng;
    A317vd->htg[A317vi->celv] = ((r1*r1)/HTS_CONST) + (r1*sin(elv_rad));

    /* If there are no radials ... exit. */
    if( A317vi->nradials < 1 ) 
	return 0;
    
    /* AVG_REF is the average reflectivity for the vad data in mm**6/m**3. */
    avg_ref = A317vi->sum_ref / A317vi->nradials;

    fit_tests = A317va->fit_tests;
    for( i = 0; i < fit_tests; ++i ){

        /* Compute coefficients of least squares fitted curve, in m/s. */
	A317h2_vad_lsf( A317vi->nradials, A317vi->azm, A317vi->ve, &npt[A317vi->celv], 
                        &cf1, &cf2, &cf3);

        /* If no data exists for vad analysis then CF1, CF2, and CF3 will 
           all be equal to missing, and NPT(CELV) will equal 0. 
           then exit module. */
	if( npt[A317vi->celv] < A317va->minpts )
	    return 0;
	
        /* Compute horizontal wind direction, in degrees (& check arguments). */
	if( (cf3 != 0.f) && (cf2 != 0.f) )
	    A317vd->hwd[A317vi->celv] = PI_CONST - atan2((double) cf3, (double) cf2);
	
        else 
	    A317vd->hwd[A317vi->celv] = 0.f;

        /* Check if wind direction computes to be negative, then convert to 
           degrees. */
	if( A317vd->hwd[A317vi->celv ] < 0.f )
	    A317vd->hwd[A317vi->celv] += 2.0*PI_CONST;
	
	A317vd->hwd[A317vi->celv] /= DEGTORAD;

        /* Compute the square root of the mean squared deviations between the 
           velocity data points and the least squares fitted curve, in m/s. */
	A317i2_vad_rms( A317vi->nradials, A317vi->azm, A317vi->ve, A317vd->hwd[A317vi->celv], 
		        cf1, cf2, cf3, &A317vd->rms[A317vi->celv] );

        /* If not the last time through this loop, perform fit test. 
           if the last time through this loop, skip fit test since it will 
           only perform un-needed calculations. */
	if( i < (A317va->fit_tests-1) ){

            /* Perform fit test to remove velocity data which lies more than 
               one rms away from the least squares fitted curve and toward 
               the zero velocity line, then go back up and perform least squares 
               fitting again. */
	    A317j2_fit_test( A317vi->nradials, A317vi->azm, A317vi->ve, cf1, cf2, cf3, 
		             A317vd->hwd[A317vi->celv], A317vd->rms[A317vi->celv] );

	}

    }

    /* Compute symmetry of the least squares fitted curve. */
    A317k2_sym_chk( cf1, cf2, cf3, A317va->tsmy, &sym );

    /* Only continue with calculations if the rms is less than the 
       threshold and the fit is symmetric. */
    if( (A317vd->rms[A317vi->celv] < A317va->th_rms) && sym ){

        /* If the atmosphere is void of precipitation within the vad 
           analysis region, the precip. fall speed is zero. */
	if( A317va->wmode == CLEAR_AIR_MODE ) 
	    pfv = 0.f;

	else if( A317va->wmode == PRECIPITATION_MODE ){

            /* HT_SEA is the height of the vad data above sea level in kilometers. */
	    ht_sea = (A317vd->htg[A317vi->celv] + A317va->rh) / 1e3f;

            /* precip. fall speed for rain, in m/s. */
	    d1 = (double) avg_ref;
	    r1 = ht_sea;
	    pfv = pow(d1, RPFV2_G2) * RPFV1_G2 * (ht_sea * RPFV4_G2 + 
		  RPFV3_G2 + (r1*r1) * RPFV5_G2);

	} 

        /* Compute horizontal wind speed in m/s. */
	A317vd->shw[A317vi->celv] = sqrt((cf2*cf2) + (cf3*cf3)) / cos(elv_rad);

        /* Compute vertical velocity in m/s and divergence in 1/s. */
	A317l2_vv_div( avg_elv, cf1, pfv, A317vd->htg[A317vi->celv], 
                       &A317vi->dec_last, &A317vi->svw_last, A317va->vad_rng, 
                       A317va->rh, &A317vd->svw[A317vi->celv], 
                       &A317vd->div[A317vi->celv] );

    }

    return 0;

} /* End of A317g2_vad_proc(). */


/*\///////////////////////////////////////////////////////////////////////

   Description:
      This module least squares fits a sine-wave curve to velocity 
      data points. Data used to perform the fitting is in the form 
      of Doppler velocity v.s. azimuth angle for a specific slant 
      range. 

   Inputs:
      azm - The azimuth angles of the radials between AZM_BEG & 
            AZM_END, in degrees. RNG:[0,360].
      dnpt - Number of data points used to perform the least  
             squares fitting, Dummy variable. Rng:[0,NAZIMS].
      nradials - A variable indicating the number of radials of
                 data available for least squares fitting. Rng:[0,NAZIMS]
      ve - Array of Doppler velocities at slant range, VAD_RNG and 
           azimuth angles, AZM_BEG, AZM-END, within the current 
           elevation scan. 

   Outputs:
      cf1 - Fourier coefficient (zeroth harmonic). Rng:[-100,+100]
      cf2 - Fourier coefficient (real part of first harmonic). 
            Rng:[-100,+100]
      cf3 - Fourier coefficient (imaginary part of first harmonic. 
            Rng:[-100,+100]
      dnpt - Number of data points used to perform the least
             squares fitting, Dummy variable. Rng:[0,NAZIMS].

   Returns:
      Always returns 0.

///////////////////////////////////////////////////////////////////////\*/
int A317h2_vad_lsf( int nradials, float *azm, float *ve, int *dnpt, 
                    float *cf1, float *cf2, float *cf3 ){

    /* Local variables */
    Complex_t int_coeff, q0, q1, q2, q3, q4, q5, qq;
    Complex_t ccj_q4, qq_int, temp0, temp1, temp2;
    double r1, r2, sum_q3i, sum_q4i, sum_q5i, sum_q0r, sum_q3r, sum_q4r, sum_q5r;
    double azm_rad, cos_az, sin_az, tempr;
    int npt, i, two_n;

    /* Zero out variables used for summations. */
    npt = 0;

    /* The following variables are used to get the real and imaginary 
       parts of the variables Q3 through Q5. */
    sum_q0r = 0.f;
    sum_q5r = 0.f;
    sum_q5i = 0.f;
    sum_q4r = 0.f;
    sum_q4i = 0.f;
    sum_q3r = 0.f;
    sum_q3i = 0.f;

    /* Perform summations for all good data points. */
    for( i = 0; i < nradials; ++i ){

	if( ve[i] > MISSING ){

            /* Convert azimuth angle from degrees to radians. */
	    azm_rad = azm[i] * DEGTORAD;

            /* Compute sine and cosine of azimuth angle since it is used 
               several times. */
	    sin_az = sin(azm_rad);
	    cos_az = cos(azm_rad);

            /* Incriment number of good data points. */
	    ++npt;

             /* Perform summations used to construct complex variables 
                Q3 -> Q5. */
	    sum_q0r += ve[i];
	    sum_q5r += cos(azm_rad * 2.0);
	    sum_q5i += sin(azm_rad * 2.0);
	    sum_q4r += cos_az;
	    sum_q4i += sin_az;
	    sum_q3r += ve[i] * cos_az;
	    sum_q3i += ve[i] * sin_az;

	}

    }

    /* If there is at least one good data point, complete calculations. */
    if( npt > 0 ){

	two_n = npt << 1;

	r1 = sum_q0r / (float) npt;
	q0.real = r1, q0.img = 0.0;

	r1 = sum_q5r / (float) two_n;
	r2 = -sum_q5i / (float) two_n;
	q5.real = r1, q5.img = r2;

	r1 = sum_q4r / two_n;
	r2 = sum_q4i / two_n;
	q4.real = r1, q4.img = r2;

	r1 = sum_q3r / (float) npt;
	r2 = -sum_q3i / (float) npt;
	q3.real = r1, q3.img = r2;

        /* Compute conjucate of Q4 since it is used several times */
	ccj_q4.real = q4.real, ccj_q4.img = -q4.img;

        /* Compute QQ (intermediate step) to save computations and to avoid 
           zero divide errors and floating point overflow errors */
        temp0.real = 1, temp0.img = 0;
	temp1.real = 4.0*ccj_q4.real, temp1.img = 4.0*ccj_q4.img;
	Cmplx_div( &temp0, &temp1, &temp2 );
	qq.real = q4.real - temp2.real, qq.img = q4.img - temp2.img;

	if( (qq.real != 0.f) || (qq.img != 0.f) ){

	    temp0.real = 2.0*ccj_q4.real, temp0.img = 2.0*ccj_q4.img;
	    Cmplx_div( &q5, &temp0, &temp1 );
	    temp2.real = ccj_q4.real - temp1.real, temp2.img = ccj_q4.img - temp1.img;
	    Cmplx_div( &temp2, &qq, &q2 );

	    Cmplx_div( &q3, &temp0, &temp1 );
	    temp2.real = q0.real - temp1.real, temp2.img = q0.img - temp1.img;
	    Cmplx_div( &temp2, &qq, &q1 );

            /* Compute INT_COEFF since it is used several times.  Compute 
               QQ_INT here, to avoid a zero divisor error. */
	    Cmplx_abs( &q2, &tempr );
	    qq_int.real = (1.0 - (tempr*tempr)), qq_int.img = 0.0;

	    if( (qq_int.real != 0.f) || (qq_int.img != 0.f) ){

		temp0.real = q1.real, temp0.img = -q1.img;
		Cmplx_mult( &q2, &temp0, &temp1 );
		temp2.real = q1.real - temp1.real, temp2.img = q1.img - temp1.img;
		Cmplx_div( &temp2, &qq_int, &int_coeff );
		*cf3 = int_coeff.img;
		*cf2 = int_coeff.real;
                Cmplx_mult( &int_coeff, &q4, &temp0 ); 
		temp0.real = 2.0*temp0.real, temp0.img = 2.0*temp0.img;
		*cf1 = q0.real - temp0.real;

            /* Otherwise, Fourier coefficients are missing. */
	    } 
            else {

		*cf1 = MISSING;
		*cf2 = MISSING;
		*cf3 = MISSING;
	    }

        /* Otherwise, Fourier coefficients are missing. */

	} 
        else{

	    *cf1 = MISSING;
	    *cf2 = MISSING;
	    *cf3 = MISSING;

	}

    } 
    else{

        /* If here, there were not enough data points to compute the 
           coefficients CF1, CF2, CF3. */
	*cf1 = MISSING;
	*cf2 = MISSING;
	*cf3 = MISSING;

    }

    *dnpt = npt;

    return 0;

} /* End of Aa317h2_vad_lsf(). */

/*\///////////////////////////////////////////////////////////////////

   Description:
      This module computes the square root of the mean squared 
      deviations between the least squares fitted curve and the 
      velocity data points. This is better known as the RMS. 

   Inputs:
      nradials - number of radials for this VAD
      azm - azimuth angles of radials, in degrees.
      ve - velocity at each azimuth, in m/s.
      dhwd - horizontal wind direction.
      cf1 - Fourier coefficent.
      cf2 - Fourier coefficent.
      cf3 - Fourier coefficent.

   Outputs:
      drms = RMS error.

   Returns:
      Always returns 0.

///////////////////////////////////////////////////////////////////\*/
int A317i2_vad_rms( int nradials, float *azm, float *ve, float dhwd, 
                     float cf1, float cf2, float cf3, float *drms ){

    /* Local variables */
    int i, dnpt;
    double speed, sum_dev, r1;

    /* Zero variables used for summations. */
    sum_dev = 0.0;
    dnpt = 0;

    /* Compute SPEED since it is used several times. */
    speed = sqrt( (cf2*cf2) + (cf3*cf3) );

    /* Perform summations of the squared deviations between the 
       least squares fitted curve and the velocity data values. */
    for( i = 0; i < nradials; ++i ){

        /* Do only for valid velocities. */
	if( ve[i] > MISSING ){

	    r1 = (-cos((azm[i] - dhwd) * DEGTORAD) * speed) + cf1 - ve[i];
	    sum_dev += (r1*r1);
	    ++dnpt;

	}

    }

    /* Compute DRMS if there is at least one good data point. */
    if( dnpt > 0 )
	*drms = (float) sqrt( (double) sum_dev / (double) dnpt);

    else{

        /* There is not enough data points to compute a rms. */
	*drms = MISSING;
    }

    return 0;

} /* End of A317i2_vad_rms() */


/*\////////////////////////////////////////////////////////////////////////

   Description:
      This module performs the "fit test". The purpose of this module 
      is to remove data which: Lies farther than 1 RMS away from the 
      least squares fitted curve AND toward the zero velocity line. 
      This procedure removes low magnatude velocity "outliers". 

   Inputs:
      nradials - number of radials in VAD.
      azm - azimuth angles of radials, in degrees.
      ve - velocity at radials, in m/s.
      cf1 - Fourier coefficient.
      cf2 - Fourier coefficient.
      cf3 - Fourier coefficient.
      dhwd - horizontal wind direction.
      drms - RMS value from VAD.

   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////////////\*/
int A317j2_fit_test( int nradials, float *azm, float *ve, float cf1, 
                     float cf2, float cf3, float dhwd, float drms ){
 
    /* Local variables */
    int i;
    double fit, speed;

    /* Compute SPEED since it is used several times. */
    speed = sqrt( (cf2*cf2) + (cf3*cf3) );

    for( i = 0; i < nradials; ++i ){

        /* Only process azimith if the measured velocity data exists. */
	if( ve[i] > MISSING ){

	    fit = -cos((azm[i] - dhwd) * DEGTORAD) * speed + cf1;

            /* Check if vad fitted curve is above the zero velocity line. */
	    if( fit > 0.f ){

                /* If velocity point is farther than 1 rms away from the 
                   vad fitted curve and toward the zero velocity line (i.e. 
                   less positive) then set it to missing. */
		if( (fit - ve[i]) > drms ) 
		    ve[i] = MISSING;

	    } 
            else {

                /* If your here, vad fitted curve is below the zero velocity 
                   line.  Next... if velocity point is more than 1 rms away 
                   from the vad fitted curve and toward the zero velocity line 
                   (i.e.less negative), then set it to missing. */
		if( (ve[i] - fit) > drms ) 
		    ve[i] = MISSING;

	    }

	}

    }

    return 0;

} /* End of A317j2_fit_test(). */


/*\////////////////////////////////////////////////////////////////////

   Description:
      This module determines if the least squares fitted curve is 
      "symmetric" around the zero velocity line, within the limits 
      set by TSMY. 

   Inputs:
      cf1 - Fourier coefficient.
      cf2 - Fourier coefficient.
      cf3 - Fourier coefficient.
      tsmy - symmetry threshold.

   Outputs:
      sym - flag if set, symmetric.  If not set, not symmetric.

   Returns:
      Always returns 0.
////////////////////////////////////////////////////////////////////\*/
int A317k2_sym_chk( float cf1, float cf2, float cf3, float tsmy, 
                    int *sym ){

    /* Local variables */
    float speed;

    speed = sqrt( (cf2*cf2) + (cf3*cf3) );

    /* The fit is symmetric if: 1) CF1 < TSMY (i.e. mean of the vad 
       fitted curve is sufficiently small), and 2) |CF1| - SPEED < 0 
       (i.e. mean of the fitted curve is smaller than the amplitude 
       of the curve). */
    if( (fabs(cf1) < tsmy) && (fabs(cf1) - speed) <= 0.f ) 
	*sym = 1;

    else
	*sym = 0;

#ifdef DEBUG
    LE_send_msg( GL_INFO, "Sym Check: speed: %10.4f, (fabs(cf1: %10.4f) - speed): %10.4f, sym: %d\n",
                 speed, fabs(cf1), fabs(cf1)-speed, *sym );
#endif
    return 0;

} /* End of A317k2_sym_chk() */


#define Q6CON_L2		(4.0/3.0)
#define E_L2			(RE_M*Q6CON_L2)
#define E_SQD_L2		(E_L2*E_L2)
#define TWO_PI			(2.0*PI_CONST)
#define SVCCON_L2		(-3.0/2.0)
#define TF_L2			(1.0/Q6CON_L2)

/*\/////////////////////////////////////////////////////////////////////

   Description:
      This module computes the vertical velocity and divergence of 
      the atmosphere analyized by vad processing. 

   Inputs:
      avg_elv - The average elevation angle of all radials
                between AZM_BEG and AZM_END in degrees.
                Rng:[-5,+90].
      cf1 - Fourier coefficient (zeroth harmonic.
            Rng:[-100,+100]
      pfv - The precipitation fall speed of data at range
            VAD_RNG and between azimuths AZM_BEG and
            AZM_END in m/s Rng:[0,10]
      dhtg - Height avove ground, in meters. Rng[0, 30000]
      vad_rng -  ADAPTABLE PARAMETER: The slant at which the VAD
                 analysis is performed,in meters.
                 RNG:[250,230000].
      rh - SITE ADAPTABLE PARAMETER:The height of the
           radar antenna above sea level in meters.
           Rng:[-2?,+?]. 

   Outputs:
      dec_last - Distance to center ofearth for the most recent
                 elevation scan within the same volume scan as
                 the current elevation scan. mtrs
                 Rng:[6371000,6400000]
      svw_last - The vertical velocity for the most recent
                 elevation scan within the same volume scan as
                 the current elevation scan in m/s. Rng:[-50,50]
      dsvw - The vertical wind speed, in m/s. Rng:[-50,50].
      ddiv - Divergence 1/s. Rng[-1,1]

   Returns:
      Always returns 0.

/////////////////////////////////////////////////////////////////////\*/
int A317l2_vv_div( float avg_elv, float cf1, float pfv, float dhtg, 
                   float *dec_last, float *svw_last, float vad_rng, 
                   float rh, float *dsvw, float *ddiv ){

    /* Local variables */
    double q6, as, dec, cfh, asc, svc, ddec;
    double elv_rad, hor_rng, d1, r1, d2, htsea;
    float rho, drho;

    /* Convert average elevation angle to radians. */
    elv_rad = (double) avg_elv * DEGTORAD;

    /* DEC is the distance from the center of the earth to the height 
       of the vad processed data in meters. 

       DEC=RE-E+E*SQRT(1+(VAD_RNG/E)**TWO+TWO*VAD_RNG*SIN(ELV_RAD)/E) 

       the following simplification to the DEC equation is done because 
       of the recomendation by sperry, great neck regarding the various 
       forms of "height above ground equations", investigation revealed 
       that DEC is simply the height above ground plus the radius of the 
       earth.     istok 8/6/85 */
    dec = dhtg + RE_M;

    /* Q6 is merely an intermediate variable used only for convienence. */
    d1 = dec + RE_M/3.0;
    d2 = d1*d1;
    r1 = vad_rng*vad_rng;
    q6 = Q6CON_L2*acos((d2+E_SQD_L2-r1)/(d1*E_L2*2.0));

    /* AS is the area of the surface enclosed by the vad processed data in 
       square meters. */
    as = TWO_PI*(dec*dec) * (1.0-cos(q6));

    /* ASC is the change in surface area from the last elevation angle 
       to the current elevation angle vad data in meters. */
    asc = (2.0*as/dec) - (PI_CONST*(dec*dec)*sin(q6)/(sin(q6*TF_L2)*RE_M)) 
          *(1.0+(r1 - E_SQD_L2)/(d1*d1));

    /* Call density to get the density and density gradient at the HTSEA */
    htsea = dhtg + rh;
    A317n2_density( htsea, &rho, &drho);

    /* SVC is the change in vertical wind from the last elevation angle 
       data to the current elevation angle vad data in 1/sec. */
    svc = ((SVCCON_L2*PI_CONST*vad_rng*cf1/(as*RE_M)) * d1) -
          (asc/as + ((double) drho/ (double) rho))*(*svw_last);

    /* DDEC is the difference in distance to the center of the earth from 
       the vad data between the last and current elevation angle of data 
       in meters. */
    ddec = dec - (*dec_last);

    /* SVW is the vertical velocity in m/s. */
    *dsvw = *svw_last + svc*ddec;

    /* CFH is a intermediate variable used to compute divergence. */
    cfh = cf1 - *dsvw * sin(elv_rad);

    /* DDIV is the divergence in 1/sec. */
    hor_rng = vad_rng * cos(elv_rad);

    *ddiv = cfh*2 / (hor_rng * cos(elv_rad)) + pfv*2 * tan(elv_rad)/hor_rng;

    /* Save DEC and DSVW, it is used to compute vertical velocity on the 
       next elevation angle of data. */
    *dec_last = dec;
    *svw_last = *dsvw;

    return 0;

} /* End of A317l2_vv_div() */


#define HTLMT1			-1125.0
#define HTLMT2			11125.0
#define HTLMT3			21750.0

/*\//////////////////////////////////////////////////////////////////////

   Description:
      This module returns atmospheric density and density gradient 
      values when given the height above sea level in meters. 
      This module is called by A317L2__VV_DIV 

   Inputs:
      The height of the current elevation scan at
      VAD_RNG above sea level, m. Rng:[-2000,30000].

   Outputs:
      rho -  The atmospheric density value for a specified
             height above sea level in kg/m**3.
             Rng:[1.32,0.0698].
      drho - The atmospheric density gradient for a specified
             height above sea level, in kg/m**4.
             Rng:[-1.12E-05,-1.24E-04].

   Returns:
      Always returns 0.

//////////////////////////////////////////////////////////////////////\*/
int A317n2_density( float htsea, float *rho, float *drho ){

    /* Initialized data */

    static float den[70] = { 1.35f,1.32f,1.28f,1.25f,1.22f,1.2f,1.17f,1.14f,
	    1.11f,1.08f,1.06f,1.03f,1.01f,.982f,.957f,.933f,.909f,.886f,.863f,
	    .841f,.819f,.798f,.777f,.757f,.736f,.717f,.697f,.679f,.66f,.642f,
	    .624f,.607f,.59f,.573f,.557f,.541f,.526f,.511f,.496f,4.81f,.467f,
	    .453f,.44f,.426f,.414f,.401f,.389f,.377f,.365f,.337f,.312f,.288f,
	    .267f,.246f,.228f,.211f,.195f,.18f,.166f,.154f,.142f,.132f,.122f,
	    .112f,.104f,.0962f,.0889f,.0821f,.0757f,.0699f };

    static float dden[70] = { -1.26e-4f,-1.24e-4f,-1.22e-4f,-1.2e-4f,-1.18e-4f,
	    -1.15e-4f,-1.13e-4f,-1.11e-4f,-1.09e-4f,-1.07e-4f,-1.05e-4f,
	    -1.03e-4f,-1.01e-4f,-9.93e-5f,-9.73e-5f,-9.54e-5f,-9.35e-5f,
	    -9.17e-5f,-8.99e-5f,-8.81e-5f,-8.63e-5f,-8.46e-5f,-8.29e-5f,
	    -8.12e-5f,-7.95e-5f,-7.79e-5f,-7.63e-5f,-7.47e-5f,-7.31e-5f,
	    -7.16e-5f,-7.01e-5f,-6.86e-5f,-6.71e-5f,-6.57e-5f,-6.42e-5f,
	    -6.28e-5f,-6.14e-5f,-6.01e-5f,-5.87e-5f,-5.74e-5f,-5.61e-5f,
	    -5.48e-5f,-5.35e-5f,-5.23e-5f,-5.11e-5f,-4.99e-5f,-4.87e-5f,
	    -4.75e-5f,-5.21e-5f,-5.29e-5f,-4.9e-5f,-4.53e-5f,-4.19e-5f,
	    -3.87e-5f,-3.58e-5f,-3.31e-5f,-3.06e-5f,-2.83e-5f,-2.61e-5f,
	    -2.42e-5f,-2.23e-5f,-2.06e-5f,-1.91e-5f,-1.76e-5f,-1.63e-5f,
	    -1.51e-5f,-1.41e-5f,-1.32e-5f,-1.22e-5f,-1.12e-5f };

    int indz;

    /* INDZ is a index into a lookup table of density & density gradients 
       for the atmosphere up to 70,000 feet. The lookup table is expressed 
       in meters above sea level and kg/m**3 and kg/m**4. */

    /* If height is within lookup table range then proceed. */
    if( (htsea >= HTLMT1) && (htsea < HTLMT3) ){

	if( htsea < HTLMT2 ){

            /* If height is less than HTLMT2 m (or 11.125 km) ASL then the 
               lookup table has densities every 250 meters starting at -1.125 km. */
	    indz = htsea / 250.f + 5.5f;

	} 
        else{

            /* Else the height is above 11.125 km, then the lookup table contains 
               densities every 500 meters. */
	    indz = htsea / 500.f + 27.5f;

	}

    } 
    else{

        /* Is height is less than lowest height in table, set to lowest value */
	if( htsea < HTLMT1 ) 
	    indz = 1;

	else {

            /* Else height is higher than highest height, set to highest value. */
	    indz = 70;

	}

    }

    /* RHO is the density in kg/m**3. */
    *rho = den[indz - 1];

    /* DRHO is the density gradient in kg/m**4. */
    *drho = dden[indz - 1];

    return 0;

} /* End of A317n2_density(). */


/*\///////////////////////////////////////////////////////////

   Description:
      This module fills the output buffer for ALERT 

   Outputs:
      vad_alert - VAD alert buffer.

   Returns:
      Always returns 0.

///////////////////////////////////////////////////////////\*/
int A317o2_vad_alert( int *vad_alert ){

    float r1;

    /* Convert vad range to kilometers */
    r1 = A317va->vad_rng / 1e3f;

    vad_alert[0] = RPGC_NINT( r1 );

    /* Set beginning and ending azimuth */
    vad_alert[1] = RPGC_NINT( A317va->azm_beg );
    vad_alert[2] = RPGC_NINT( A317va->azm_end );

    /* Set height in feet */
    vad_alert[3] = RPGC_NINT( A317ve->vhtg[Cvol][0] );

    if( A317ve->vshw[Cvol][0] > MISSING ){

        /* If wind speed is not missing, convert to knots and save */
	vad_alert[4] = A317ve->vshw[Cvol][0] * MPS_TO_KTS;

        /* Save wind direction */
	vad_alert[5] = RPGC_NINT( A317ve->vhwd[Cvol][0] );

    } 
    else {

        /* If data is missing, set wind speed and direction to missing */
	vad_alert[4] = MISSING;
	vad_alert[5] = MISSING;
    }

    vad_alert[6] = MISSING;

    return 0;

} /* End of A317o2_vad_alert(). */


/*\///////////////////////////////////////////////////////////////////

   Description:
      Fills the output buffer VADTMHGT for Radar Coded Message.

///////////////////////////////////////////////////////////////////\*/
int A317p2_vad_tmhgt( char *optr ){

    /* Local variables */
    int k, amr, iazm;
    Vad_params_t *vad_params = (Vad_params_t *) optr;

    /* Save the current volume scans date and number. */
    vad_params->vol_date = A317vd->date;
    vad_params->vol_num = Cvol;

    /* Save the time data. */
    for( k = 0; k < MAX_VAD_VOLS; ++k ) 
	vad_params->times[k] = A317vd->time[Cvol];

    vad_params->missing_data_val = MISSING;

    /* Save the height, RMS, direction and speed values. */
    for (k = 0; k < MAX_VAD_HTS; ++k) {

	vad_params->vad_data_hts[k][VAD_HTG] = A317ve->vhtg[Cvol][k];
	vad_params->vad_data_hts[k][VAD_RMS] = A317ve->vrms[Cvol][k];
	vad_params->vad_data_hts[k][VAD_HWD] = A317ve->vhwd[Cvol][k];
	vad_params->vad_data_hts[k][VAD_SHW] = A317ve->vshw[Cvol][k];

        /* Save the number of radials, cf1, cf2, cf3, slant range, and   
           elevation cut number. */
	vad_params->vad_data_hts[k][VAD_NRADS] = (float) A317ve->nrads[k];
	vad_params->vad_data_hts[k][VAD_CF1] = A317ve->hcf1[k];
	vad_params->vad_data_hts[k][VAD_CF2] = A317ve->hcf2[k];
	vad_params->vad_data_hts[k][VAD_CF3] = A317ve->hcf3[k];
	vad_params->vad_data_hts[k][VAD_SLR] = A317ve->slrn[k];
	vad_params->vad_data_hts[k][VAD_LCN] = (float) A317ve->elcn[k];

        /* Save number of ambiguous ranges (ie PRF sectors). */
	vad_params->vad_data_hts[k][VAD_ARP] = (float) A317ve->ambig_range_ptr[k];

        /* Save the good data points for all heights selected. */
	for( iazm = 0; iazm < A317ve->nrads[k]; ++iazm ){

	    vad_params->vad_data_azm[k][iazm][VAD_AZM] = A317ve->hazm[k][iazm];
	    vad_params->vad_data_azm[k][iazm][VAD_REF] = (float) A317ve->href[k][iazm];
	    vad_params->vad_data_azm[k][iazm][VAD_VEL] = A317ve->hvel[k][iazm];

	}

        /* Save the actual ambiguous ranges found. */
	for( amr = 0; amr < MAX_NUM_AMBRNG; ++amr ){

	    vad_params->vad_data_ar[k][amr][VAD_ART] = (float) A317ve->ambig_range[k][amr];
	    vad_params->vad_data_ar[k][amr][VAD_ARL] = (float) A317ve->ambig_azlim[k][amr];

	}

    }

    return 0;

} /* End of A317p2_vad_tmhgt() */


#define RPFV1_U2		0.0114
#define RPFV2_U2		2.6789115
#define RPFV3_U2		0.0758695
#define RPFV4_U2		0.0068635

/*\////////////////////////////////////////////////////////////////////////

   Description:
      This module decodes radial base data and saves the data that 
      is needed by the VAD processing software which is subsequently 
      used in the environmental winds table. 

   Inputs:
      ipr - radial pointer.

   Returns: Always returns 0.

////////////////////////////////////////////////////////////////////////\*/
int A317u2_vad_data_ewt( char *ipr ){

    /* Local variables */
    int i, nv, n_dop_bins, lastr, lastv, firstr, firstv;
    float r1, d1, pfv, sum_vel, dbz, azimuth, elevation, ht_sea;

    Base_data_header *bdh = (Base_data_header *) ipr;
    Moment_t vel, *iv = (Moment_t *) (ipr + bdh->vel_offset);
    Moment_t ref, *iz = (Moment_t *) (ipr + bdh->ref_offset);

    static float vad_rng_km_ewt, htcon;

    /* Get azimuth angle (degrees) of radial. */
    azimuth = bdh->azimuth;

    /* Get elevation angle (degrees) of radial. */
    elevation = bdh->elevation;

    /* Sum elevation and number of radials in elevation scan */
    A317ec->sum_elv_ewt += elevation;
    ++A317ec->tnradials_ewt;

    /* Get the first and last good velocity bins of the radial. */
    firstv = bdh->dop_range;
    n_dop_bins = bdh->n_dop_bins;
    if( n_dop_bins > BASEDATA_VEL_SIZE )
        n_dop_bins = BASEDATA_VEL_SIZE;
    lastv = firstv + n_dop_bins - 1;

    /* Set velocity resolution flag. */
    RPGCS_set_velocity_reso( (int) bdh->dop_resolution );

    /* If this is the first radial in the volume scan */
    if( A317ec->nradials_ewt < 0 ){

	A317ec->nradials_ewt = 0;

        /* compute parts of the HT_SEA equation that are repetitive below */
	r1 = A317ec->vad_rng_ewt;

	vad_rng_km_ewt = r1 / 1e3f;
	htcon = (A317va->rh + (r1*r1)/HTS_CONST)/1e3f;

    }

    /* If weather mode is clear air (WMODE=1) then, average the three 
       adjacient gates in range for each radial, and we don't need to 
       sum the reflectivity. */
    if( A317va->wmode == CLEAR_AIR_MODE ){

	nv = 0;
	sum_vel = 0.f;
	for( i = -1; i <= 1; ++i ){

            /* Make sure velocity data is not below threshold or 
               before or after the first or last good bin, respectively. */
	    if( (A317ec->ivbin_ewt + i <= lastv)
                            && 
                (A317ec->ivbin_ewt + i >= firstv) ){

                vel = iv[A317ec->ivbin_ewt+i];
		if( vel > BASEDATA_RDRNGF ){

		    sum_vel += RPGCS_velocity_to_ms( vel );
		    ++nv;

		}

	    }

	}

        /* Compute average velocity and include in VE_EWT and AZM_EWT */
	if( nv > 0 ){

	    A317ec->ve_ewt[A317ec->nradials_ewt] = sum_vel / nv;
	    A317ec->azm_ewt[A317ec->nradials_ewt] = azimuth;
	    ++A317ec->nradials_ewt;

	}

    } 
    else{

        /* Get the first and last good reflectivity bins of the radial */
	firstr = bdh->surv_range - 1;
	lastr = firstr + bdh->n_surv_bins - 1;

        /* Since we're here, it's not clear air. So sum reflectivity, 
           and put velocity and azimuth angles in VE_EWT and AZM_EWT. 
           also, correct velocity for precipitation fall speed 
           make sure velocity data is not below threshold or before or 
           after the first or last good bin, respectively. */
	if( (A317ec->ivbin_ewt <= lastv)
                         && 
            (A317ec->ivbin_ewt >= firstv) ){

            vel = iv[A317ec->ivbin_ewt]; 
                        
	    if( vel > BASEDATA_RDRNGF ){

                /* Account for bad reflectivity data, set it to the low
                   reflectivity threshold, = -32 DBZ. */
	        if( (A317ec->irbin_ewt >= firstr) 
                             && 
                    (A317ec->irbin_ewt <= lastr) ){

		    ref = iz[A317ec->irbin_ewt];

		    if( ref <= BASEDATA_RDRNGF ) 
		        dbz = -32.f;

		    else
		        dbz = RPGCS_reflectivity_to_dBZ( ref );
		}
	        else 
		    dbz = -32.f;

                /* Store the velocity and azimuth. */
	        A317ec->ve_ewt[A317ec->nradials_ewt] = RPGCS_velocity_to_ms( vel );
	        A317ec->azm_ewt[A317ec->nradials_ewt] = azimuth;

                /* HT_SEA equation modified so that the portion which is the 
                   same for each radial is only computed once per elevation scan. */
		ht_sea = htcon + (vad_rng_km_ewt*sin(elevation * DEGTORAD));

                /* The PFV equation has been modified such that the constant 2.65 is 
                   combined with the constants in the later part of the equation. */
		d1 = (double) (dbz * RPFV1_U2);
		pfv = pow(10.0, d1) * (ht_sea*RPFV3_U2 + RPFV2_U2 + ht_sea*RPFV4_U2*ht_sea);

		A317ec->ve_ewt[A317ec->nradials_ewt] += (pfv*sin(elevation*DEGTORAD));

                /* Increment the radial counter. */
	        ++A317ec->nradials_ewt;

	    }

	}

    }

    return 0;

} /* End of A317u2vad_data_ewt(). */


/*\///////////////////////////////////////////////////////////////////////

   Description:
      This module is the main processing routine of the vad 
      algorithm for the supplemental VADs used in the environmental 
      wind table.  It receives data via the calling sequence and 
      the common area A317VI. It calls the various routines to 
      perform the various tasks required to generate the wind 
      speed, and wind direction for the region of the atmsophere 
      enclosed within the slant range VAD_RNG_EWT. 

   Outputs:
      htg_ewt - Height values AGL for up to NELVEWT elevation
                scans per volume scan used solely for the
                supplemental VADs which are in the Environmental
                Winds Table.
      rms_ewt - RMS values for up to NELVEWT elevation scans per
                volume scan used solely for the supplemental
                VADs which are in the Environmental Winds Table.
      npt_ewt - No. points used to perform least squares fit for
                up to NELVEWT elevation scans per volume scan.
                Solely for supplemental VADs in Environmental
                Winds Table.
      hwd_ewt - Wind direction values for up to NELVEWT
                elevation scan per volume scan used solely for
                the supplemental VADs which are in the
                Environmental Winds Table.
      shw_ewt - Horizontal wind speeds for up to NELVEWT
                elevation scans per volume scan used solely for
                the supplemental VADs which are in the
                Environmental Winds Table.

   Returns:
      Always returns 0.

///////////////////////////////////////////////////////////////////////\*/
int A317v2_vad_proc_ewt( float *htg_ewt, float *rms_ewt, int *npt_ewt, 
                         float *hwd_ewt, float *shw_ewt ){

    /* Local variables */
    int i, fit_tests, sym;
    float cf1, cf2, cf3;
    double r1, elv_rad, avg_elv_ewt;

    /* AVG_ELV_EWT is the average elevation angle for the vad data, in degrees. */
    avg_elv_ewt = (double) A317ec->sum_elv_ewt / (double) A317ec->tnradials_ewt;

    /* ELV_RAD is the average elevation angle for the vad data in radians. */
    elv_rad = avg_elv_ewt * DEGTORAD;

    /* Compute height of the data above ground in meters. */
    r1 = A317ec->vad_rng_ewt;
    htg_ewt[A317vi->celv] = (float) (((r1*r1) / HTS_CONST) + r1*sin(elv_rad));

    /* If there are no radials .. exit */
    if( A317ec->nradials_ewt < 1 ) 
	return 0;

    /* Do For All fit tests. */
    fit_tests = A317va->fit_tests;
    for( i = 0; i < fit_tests; ++i ){

        /* Compute coefficients of least squares fitted curve, in m/s. */
	A317h2_vad_lsf( A317ec->nradials_ewt, A317ec->azm_ewt, A317ec->ve_ewt, 
                        &npt_ewt[A317vi->celv], &cf1, &cf2, &cf3 );

        /* If no data exists for vad analysis then CF1, CF2, and CF3 will 
           all be equal to missing, and NPT_EWT(CELV) will equal 0. 
           then exit module. */
	if( npt_ewt[A317vi->celv] < A317va->minpts ) 
	    return 0;

        /* Compute horizontal wind direction, in degrees (& check arguments). */
	if( (cf3 != 0.f) && (cf2 != 0.f) )
	    hwd_ewt[A317vi->celv] = PI_CONST - atan2((double) cf3, (double) cf2);
	
        else 
	    hwd_ewt[A317vi->celv] = 0.f;

        /* Check if wind direction computes to be negative, then convert to 
           degrees. */
	if( hwd_ewt[A317vi->celv] < 0.f ) 
	    hwd_ewt[A317vi->celv] += 2.*PI_CONST;
	
	hwd_ewt[A317vi->celv] /= DEGTORAD;

        /* Compute the square root of the mean squared deviations between the 
           velocity data points and the least squares fitted curve, in m/s. */
	A317i2_vad_rms( A317ec->nradials_ewt, A317ec->azm_ewt, A317ec->ve_ewt, 
                        hwd_ewt[A317vi->celv], cf1, cf2, cf3, &rms_ewt[A317vi->celv] );

        /* If not the last time through this loop, perform fit test. 
           If the last time through this loop, skip fit test since it will 
           only perform un-needed calculations. */
	if( i < (fit_tests-1) ){

            /* Perform fit test to remove velocity data which lies more than 
               one rms away from the least squares fitted curve and toward 
               the zero velocity line, then go back up and perform least squares 
               fitting again. */
	    A317j2_fit_test( A317ec->nradials_ewt, A317ec->azm_ewt, A317ec->ve_ewt, 
                             cf1, cf2, cf3, hwd_ewt[A317vi->celv], rms_ewt[A317vi->celv] );

	}

    }

    /* Compute symmetry of the least squares fitted curve. */
    A317k2_sym_chk( cf1, cf2, cf3, A317va->tsmy, &sym );

    /* Only continue with calculations if the rms is less than the 
       threshold and the fit is symmetric. */
    if( (rms_ewt[A317vi->celv] < A317va->th_rms) && sym ){

        /* Compute horizontal wind speed in m/s. */
	shw_ewt[A317vi->celv] = sqrt((cf2*cf2) + (cf3*cf3)) / cos(elv_rad);

    }

    return 0;

} /* End of A317v2_vad_proc_ewt(). */



/*\////////////////////////////////////////////////////////////////////////////////

   Description:
      Updates information used by AVSET in controlling which elevations are
      processed.

   Inputs:
      bdh - base data header.

////////////////////////////////////////////////////////////////////////////////\*/
void A317w2_update_avset_info( Base_data_header *bdh ){

   int ind, vcp_num_elev, avset_status;

   /* Check if AVSET is active. */

   /* Get the AVSET status from the RDA. */
   avset_status = Get_rda_avset_status();

   /* If flag is set, AVSET is enabled. */
   if( avset_status ){

      /* Set the AVSET active flag. */
      Avset.active = 1;

      /* Store VCP number, RDA elevation cut number, and elevation angle. */
      Avset.vcp = bdh->vcp_num;
      Avset.last_elev_cut = bdh->elev_num;
      Avset.last_rpg_elev_cut = bdh->rpg_elev_ind;
      Avset.last_elev_angle = (float) bdh->target_elev/10.0;

      RPGC_log_msg( GL_INFO, "AVSET VCP: %d, Last RDA Cut #: %d, Last Elev Angle: %f\n",
                    Avset.vcp, Avset.last_elev_cut, Avset.last_elev_angle );

      /* Find the number of elevations in this VCP.  If the current elevation
         is the last elevation in the VCP definition, set the previous cut 
         to the current cut.   Otherwise, set the previous cut to the current
         cut minus 1. */
      vcp_num_elev = RPGCS_get_num_elevations( Avset.vcp );
      if( Avset.last_elev_cut < vcp_num_elev ){

         Avset.prev_elev_cut = bdh->elev_num - 1;
         ind = RPGCS_get_rpg_elevation_num( Avset.vcp, Avset.prev_elev_cut );
         if( ind >= 0 ){

            /* Note: We have to subtract 1 from cut number when making this API
                     call because cut numbers are 0 indexed whereas cut numbers 
                     in the radial are unit indexed. */
            Avset.prev_elev_angle = RPGCS_get_elevation_angle( Avset.vcp, 
                                                               Avset.prev_elev_cut - 1 );
            RPGC_log_msg( GL_INFO, "--->Previous (to Last) Elevation Angle: %f\n",
                         Avset.prev_elev_angle );
            Avset.prev_rpg_elev_cut = ind;

         }

      }
      else{

         Avset.prev_elev_cut = Avset.last_elev_cut;
         Avset.prev_rpg_elev_cut = Avset.last_rpg_elev_cut;
         Avset.prev_elev_angle = Avset.last_elev_angle;

      }

   }
   else
      Avset.active = 0;

/* End of A317w2_update_avset_info() */
}
       
/*\////////////////////////////////////////////////////////////////////////////////

   Description:
      Retrieves AVSET information to be used in controlling which elevations
      are processed.

   Outputs:
      active - active/inactive flag
      vcp - VCP active in the previous volume scan.
      elev_angle - elevation angle where AVSET terminated the volume scan.
      prev_elev_angle - elevation angle of cut just prior to the cut that 
                        was the last cut in the VCP.

////////////////////////////////////////////////////////////////////////////////\*/
void A317y2_get_avset_info( int *active, int *vcp, float *elev_angle,
                            float *prev_elev_angle ){

   /* Return the AVSET information. */
   *active = Avset.active;
   *vcp = Avset.vcp;
   *elev_angle = Avset.last_elev_angle;
   *prev_elev_angle = Avset.prev_elev_angle;

   RPGC_log_msg( GL_INFO, "Active: %d, VCP: %d, Elev Angle: %f, Prev Elev Angle; %f\n",
                 *active, *vcp, *elev_angle, *prev_elev_angle );

/* End of A317y2_get_avset_info(). */
}

/*\///////////////////////////////////////////////////////////////////////

   Description:
      This module performs the function of interpolating the gaps in
      the EWTAB table of the environmental winds.

   Returns:
      Always returns 0,

///////////////////////////////////////////////////////////////////////\*/
int A317x2_interpolate_ewtab(void){

    int lastidx, h, npts, ptr, idx, jdate, curr_time;
    float rptr, e1, e2, n1, n2, em, nm, tec, tnc;

    /* Initialize. */
    lastidx = -1;
    npts = 0;
    n1 = 0.f;
    e1 = 0.f;

    /* Get the current Julian date and time to generate a time stamp 
       for the sounding information. */
    RPGCS_get_date_time( &curr_time, &jdate );

    /* Convert the Julian date and time into minutes since 010170. */
    Ewt.sound_time = (jdate-1)*1440 + curr_time/60;

    /* Loop through all elements in the table. */
    for( idx = 0; idx < LEN_EWTAB; ++idx ){

        /* Process only when current element is not the check value. */
	if( Ewt.ewtab[WNDDIR][idx] != 32767.f ){

            /* Calculate the north and east components of the wind vector. */  
	    n2 = -Ewt.ewtab[WNDSPD][idx] * cos(Ewt.ewtab[WNDDIR][idx]*DEGTORAD);
	    e2 = -Ewt.ewtab[WNDSPD][idx] * sin(Ewt.ewtab[WNDDIR][idx]*DEGTORAD);
	    Ewt.newndtab[idx][NCOMP] = (short) RPGC_NINT(n2);
	    Ewt.newndtab[idx][ECOMP] = (short) RPGC_NINT(e2);
	    if( lastidx == -1 ){

                /* Index of actual wind direction is the first found. */
		lastidx = idx;

	    } 
            else{

                /* Interpolate between heights determined. */
		npts = (short) (idx - lastidx - 1);
		if( npts > 0 ){

                    /* Calculate components of this height and previous. */
		    e1 = -(Ewt.ewtab[WNDSPD][lastidx] * sin(Ewt.ewtab[WNDDIR][lastidx] * DEGTORAD));
		    n1 = -(Ewt.ewtab[WNDSPD][lastidx] * cos(Ewt.ewtab[WNDDIR][lastidx] * DEGTORAD));

                    /* Calculate slopes versus height between components. */
		    em = (e2 - e1) / (float) (idx - lastidx);
		    nm = (n2 - n1) / (float) (idx - lastidx);

                    /* Do For All points to produce. */
		    for( ptr = 1; ptr <= npts; ++ptr ){

			h = ptr + lastidx;
			rptr = ptr + 0.f;

                        /* Calculate new components from slope. */
			tnc = n1 + (nm * rptr);
			tec = e1 + (em * rptr);
			Ewt.newndtab[h][NCOMP] = (short) RPGC_NINT(tnc);
			Ewt.newndtab[h][ECOMP] = (short) RPGC_NINT(tec);

                        /* Calculate new speed and direction from components. */
			Ewt.ewtab[WNDSPD][h] = sqrt((tnc*tnc) + (tec*tec)); 
			if( tec < 0.f )
			    Ewt.ewtab[WNDDIR][h] = 90.f - 
                                             atan((double) tnc / (double) tec)/DEGTORAD;

			else if( tec > 0.f ) 
			    Ewt.ewtab[WNDDIR][h] = 270.f - 
                                             atan((double) tnc / (double) tec)/DEGTORAD; 
			
                        else{

			    if( tnc < 0.f ) 
				Ewt.ewtab[WNDDIR][h] = 0.f;

			    else 
				Ewt.ewtab[WNDDIR][h] = 180.f;

                        }

		    }

		}

                /* Save the current index as last built. */
		lastidx = idx;

	    }

	}

    }

    return 0;

} /* End of A317x2_interpolate_ewtab(). */


#define ARRAY_SIZE		(MAX_VAD_ELVS+NELVEWT+MAX_VAD_HTS)

/*\//////////////////////////////////////////////////////////////////////

   Description:
      This module is used to enter wind data into the environmental 
      winds table.  Data are entered to the nearest 1000 ft height 
      layers.  Supplemental VADs are used to enter into the lowest 
      layer only. 

   Inputs:
      htg_ewt - Height values AGL for up to NELVEWT
                elevation scans per volume scan used solely
                for the supplemental VADs which are in the
                Environmental Winds Table.
      hwd_ewt - Wind direction values for up to NELVEWT
                elevation scan per volume scan used solely
                for the supplemental VADs which are in the
                Environmental Winds Table.

   Outputs:
      shw_ewt - Horizontal wind speeds for up to NELVEWT
                elevation scans per volume scan used solely for
                the supplemental VADs which are in the
                Environmental Winds Table.
      statl - Status logical flag - whether the environmental
              winds table is being updated.

   Returns:
      Always returns 0.

//////////////////////////////////////////////////////////////////////\*/
int A317z2_envwndtab_entries( float *htg_ewt, float *hwd_ewt, float *shw_ewt, 
                              int *statl ){

    /* Local variables */
    int min_lvl, max_lvl, vnhts, i, pc, elv, hght, height_level;
    float slope;

    static short lwdtab[ARRAY_SIZE], lwstab[ARRAY_SIZE], lhtab[ARRAY_SIZE];

    /* Check local environmental winds flag to further process. */
    *statl = 0;
    if( Ewt.envwndflg ){

        /* Initialize the point counter. */
	pc = 0;

        /* Check all VAD produced data points for valid data. 
           If point is valid, increment point count and save value. */

        /* Do For the supplemental VADS first. */
	for( elv = 0; elv < NELVEWT; ++elv ){

	    if( (hwd_ewt[elv] != MISSING) && (shw_ewt[elv] != MISSING) ){

                /* Mod for level 1 wind speed to interpolate to 500 ft ht. */
		if( elv >= (NELVEWT-1) ){

		    slope = .5f / (htg_ewt[elv] * M_TO_KFT);
		    shw_ewt[elv] = slope * shw_ewt[elv];

		}

		lhtab[pc] = 0;
		lwstab[pc] = (short) RPGC_NINT(shw_ewt[elv]);
		lwdtab[pc] = (short) RPGC_NINT(hwd_ewt[elv]);
		++pc;

                /* Terminate loop, go to Do For the rest of the VAD data. */
		break;

	    }

	}

        /* Do For the rest of the VAD data. */
	for( elv = 0; elv < MAX_VAD_ELVS; ++elv ){

	    if( (A317vd->hwd[elv] != MISSING) && (A317vd->shw[elv] != MISSING) ){

		height_level = (int) (A317vd->htg[elv] * M_TO_KFT);

		if( height_level > 1 ){

		    lhtab[pc] = (short) height_level;
		    lwstab[pc] = (short) RPGC_NINT(A317vd->shw[elv]);
		    lwdtab[pc] = (short) RPGC_NINT(A317vd->hwd[elv]);
		    ++pc;

		}

	    }

	}

        /* Store the wind used in the VWP product. */
	vnhts = A317ve->vnhts[Cvol];
	for (hght = 0; hght < vnhts; ++hght ){

            /* Check for good data. */
	    if( (A317ve->vhwd[Cvol][hght] != MISSING) 
                               &&
		(A317ve->vshw[Cvol][hght] != MISSING) ){

                /* Correct heights from above sea level to above ground. */
		height_level = 
                   (int) ((A317ve->vhtg[Cvol][hght] - Siteadp.rda_elev) * .001f);

		lhtab[pc] = (short) height_level;
		lwstab[pc] = (short) RPGC_NINT(A317ve->vshw[Cvol][hght]);
		lwdtab[pc] = (short) RPGC_NINT(A317ve->vhwd[Cvol][hght]);
		++pc;

	    }

	}

        /* If there is new VAD data, produce a new environmental winds table,
           otherwise exit the routine. */
	if( pc > 0 ){

	    *statl = 1;

            /* Initialize min and max levels to large and small numbers. */
	    max_lvl = 0;
	    min_lvl = 70;

            /* Find minimum and maximum levels of new data. */
	    for( i = 0; i < pc; ++i ){

		if( lhtab[i] < min_lvl ) 
		    min_lvl = lhtab[i];

		if( lhtab[i] > max_lvl ) 
		    max_lvl = lhtab[i];

	    }

            /* Clear the environmental winds table wind speed and direction. */
	    for( i = min_lvl; i <= max_lvl; ++i ){

		Ewt.ewtab[WNDDIR][i] = 32767.f;
		Ewt.ewtab[WNDSPD][i] = 32767.f;

	    }

            /* Copy the good data points onto the cleared data field. */
	    for( i = 0; i < pc; ++i ){

		Ewt.ewtab[WNDDIR][lhtab[i]] = (float) lwdtab[i];
		Ewt.ewtab[WNDSPD][lhtab[i]] = (float) lwstab[i];

	    }

            /* Call routine to interpolate the interval of points produced. */
	    A317x2_interpolate_ewtab();

	}

    }

    return 0;

} /* End of A317z2_envwndtab_entries(). */

/*\///////////////////////////////////////////////////////////////////////////////

   Description:
      Returns the status of AVSET from the RDA's perspective ...

   Returns:
      0 - if AVSET disabled or status unknown.
      1 - if AVSET enabled.

///////////////////////////////////////////////////////////////////////////////\*/
static int Get_rda_avset_status(){

   int avset_status = ORPGRDA_get_status( RS_AVSET );

   if( avset_status == ORPGRDA_DATA_NOT_FOUND ){

      LE_send_msg( GL_INFO, "Unable to retrieve AVSET status.\n" );
      return 0;

   }

   if( avset_status == AVSET_ENABLED )
      return 1;

   return 0;

/* End of Get_rda_avset_status(). */
}

