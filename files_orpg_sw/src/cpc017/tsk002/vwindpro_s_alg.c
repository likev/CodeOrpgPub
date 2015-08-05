/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2010/11/04 20:24:02 $ */
/* $Id: vwindpro_s_alg.c,v 1.4 2010/11/04 20:24:02 steves Exp $ */
/* $Revision: 1.4 $ */
/* $State: Exp $ */

#include <vwindpro.h>

/* Static Global Variables. */
static int Valid_cnt;

typedef struct {

   int elv_ind;

   int ht_ind;

} Save_info_t;

static Save_info_t Save_info[MAX_VAD_HTS];

/* Function Prototypes. */
static int Find_closest_to_average_wind( int *s_elv, int *s_ht );


/*\////////////////////////////////////////////////////////////////

   Description:
      This module is the main processing routine of the vad 
      algorithm. It receives data via the calling sequence and 
      the common area A317VI. It calls the various routines to 
      perform the various tasks required to generate the wind 
      speed and wind direction for the region of the atmosphere 
      enclosed within the slant range VAD_RNG and azimuth limits 
      AZM_BEG and AZM_END. 

   Inputs:
      cvht - current height index.

   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////\*/
int A317t2_vad_proc_hts( int cvht ){

    /* Local variables */
    int i, sym, fit_tests, elev_indx;
    double r1, r2, elv_rad, avg_elv;
    float speed;

    /* AVG_ELV is the average elevation angle for the scan, in degrees. */
    avg_elv = A317vi->sum_elv / A317vi->tnradials;

    /* AVG_RAD is the average elevation angle for the vad data in radians. */
    elv_rad = (double) avg_elv * DEGTORAD;

    /* Get the current elevation index. */
    elev_indx = A317vi->celv;

    /* If there are no radials... go to end and exit. */
    if( A317vs->nrads[elev_indx][cvht] < A317va->minpts ){
#ifdef DEBUG
        RPGC_log_msg( GL_INFO, "A317vs->nrads[%3d][%3d]: %3d < A317va->minpts: %3d\n",
                      elev_indx, cvht, A317vs->nrads[elev_indx][cvht], A317va->minpts );
#endif
	return 0;

    }

    /* Do For All fit tests. */
    fit_tests = A317va->fit_tests;

    RPGC_log_msg( GL_INFO, "Processing Ht: %10.4f\n", A317vs->vhtg[elev_indx][cvht] );
    for( i = 0; i < fit_tests; ++i ){

        /* Compute coefficients of least squares fitted curve, in m/s. */
	A317h2_vad_lsf( A317vs->nrads[elev_indx][cvht], 
                        &A317vs->hazm[elev_indx][cvht][0], 
		        &A317vs->hvel[elev_indx][cvht][0], 
                        &A317vs->vnpt[elev_indx][cvht], 
                        &A317vs->hcf1[elev_indx][cvht], 
                        &A317vs->hcf2[elev_indx][cvht], 
                        &A317vs->hcf3[elev_indx][cvht] );

        /* If no data exists for vad analysis then CF1, CF2, and CF3 will 
           all be equal to missing, and NPT(CVOL,CELV) will equal 0. 
           Exit this module. */
	if( A317vs->vnpt[elev_indx][cvht] < A317va->minpts ){
#ifdef DEBUG
            RPGC_log_msg( GL_INFO, "--->A317vs->vnpt[%3d][%3d] Too Few Points: %4d (Thresh: %4d)\n",
                          elev_indx, cvht, A317vs->vnpt[elev_indx][cvht], A317va->minpts );
#endif 
	    return 0;

        }

        /* Compute horizontal wind direction, in degrees (& check arguments). */
	if( (A317vs->hcf3[elev_indx][cvht] != 0.f) 
                          && 
            (A317vs->hcf2[elev_indx][cvht] != 0.f) )
	    A317vs->vhwd[elev_indx][cvht] = PI_CONST - 
                atan2( (double) A317vs->hcf3[elev_indx][cvht], 
                       (double) A317vs->hcf2[elev_indx][cvht] );

	else 
	    A317vs->vhwd[elev_indx][cvht] = 0.f;
	
        /* Check if wind direction computes to be negative, then convert to 
           degrees. */
	if( A317vs->vhwd[elev_indx][cvht] < 0.f ) 
	    A317vs->vhwd[elev_indx][cvht] += 2.0*PI_CONST;
	
	A317vs->vhwd[elev_indx][cvht] /= DEGTORAD;

        /* Compute the square root of the mean squared deviations between the 
           velocity data points and the least squares fitted curve, in m/s. */
	A317i2_vad_rms(  A317vs->nrads[elev_indx][cvht], 
                         &A317vs->hazm[elev_indx][cvht][0], 
                         &A317vs->hvel[elev_indx][cvht][0], 
                         A317vs->vhwd[elev_indx][cvht], 
                         A317vs->hcf1[elev_indx][cvht], 
                         A317vs->hcf2[elev_indx][cvht], 
                         A317vs->hcf3[elev_indx][cvht], 
                         &A317vs->vrms[elev_indx][cvht] );

        /* If not the last time through this loop, perform fit test. 
           If the last time through this loop, skip fit test since it will 
           only perform un-needed calculations. */
	if( i < (fit_tests-1) ){


            /* Perform fit test to remove velocity data which lies more than 
               one rms away from the least squares fitted curve and toward 
               the zero velocity line, then go back up and perform least 
               squares fitting again. */
            A317j2_fit_test( A317vs->nrads[elev_indx][cvht], 
                             &A317vs->hazm[elev_indx][cvht][0], 
                             &A317vs->hvel[elev_indx][cvht][0], 
                             A317vs->hcf1[elev_indx][cvht], 
                             A317vs->hcf2[elev_indx][cvht], 
                             A317vs->hcf3[elev_indx][cvht], 
	                     A317vs->vhwd[elev_indx][cvht], 
                             A317vs->vrms[elev_indx][cvht] );

	}
        else{

            RPGC_log_msg( GL_INFO, "--->Last Pass: RMS: %10.4f\n",
                          A317vs->vrms[elev_indx][cvht] );

        }

    }

    /* Compute symmetry of the least squares fitted curve. */
    A317k2_sym_chk( A317vs->hcf1[elev_indx][cvht], 
                    A317vs->hcf2[elev_indx][cvht], 
                    A317vs->hcf3[elev_indx][cvht], A317va->tsmy, &sym );

    /* Only continue with calculations if the rms is less than the 
       threshold and the fit is symmetric. */
    if( (A317vs->vrms[elev_indx][cvht] < A317va->th_rms) && sym ){

        int sum_ref = 0;
        int num_ref = 0;

        /* Compute horizontal wind speed in m/s. */
	r1 = (double) A317vs->hcf2[elev_indx][cvht];
	r2 = (double) A317vs->hcf3[elev_indx][cvht];
	speed = (float) sqrt( (r1*r1) + (r2*r2) ) / cos(elv_rad);

        /* If speed is greater than 2*RMS, set speed in data structure. */
        if( speed >= (2.0f*A317vs->vrms[elev_indx][cvht]) ){

	    A317vs->vshw[elev_indx][cvht] = speed;
            A317vs->velv[elev_indx][cvht] = elv_rad;

            /* Calculate the average reflectivity value. */
            for( i = 0; i < A317vs->nrads[elev_indx][cvht]; i++ ){ 

                if( A317vs->hvel[elev_indx][cvht][i] > MISSING ){

                    sum_ref += A317vs->href[elev_indx][cvht][i];
                    num_ref++;    

                }

            }

            /* If there are enough data values (there should always be),
               calculate the average. */
            if( num_ref > 0 ){

                int avg_ref = (int) RPGC_NINT( (float) sum_ref / (float) num_ref );
                A317vs->href_avg[elev_indx][cvht] = RPGCS_reflectivity_to_dBZ( avg_ref );

            }

        }
        else{
#ifdef DEBUG
            RPGC_log_msg( GL_INFO, "--->A317vs->vshw[%3d][%3d] Failed RMS Test\n",
                          elev_indx, cvht );
            RPGC_log_msg( GL_INFO, "------>speed: %10.4f, 2.0*RMS: %10.4f\n",
                          speed, 2.0*A317vs->vrms[elev_indx][cvht] );
#endif
	    A317vs->vshw[elev_indx][cvht] = MISSING;
        }

    }
    else{
#ifdef DEBUG

        r1 = (double) A317vs->hcf2[elev_indx][cvht];
        r2 = (double) A317vs->hcf3[elev_indx][cvht];
        speed = (float) sqrt( (r1*r1) + (r2*r2) );

        RPGC_log_msg( GL_INFO, "--->A317vs->vrms[%3d][%3d]: %10.4f Failed RMS or Symmetry Test: %3d\n",
                      elev_indx, cvht, A317vs->vrms[elev_indx][cvht], sym );
        RPGC_log_msg( GL_INFO, "------>fabs(cf1): %10.4f ?< A317va->tsmy: %10.4f\n",
                      fabs(A317vs->hcf1[elev_indx][cvht]), A317va->tsmy );
        RPGC_log_msg( GL_INFO, "------>fabs(cf1): %10.4f - speed: %10.4f = %10.4f\n",
                      fabs(A317vs->hcf1[elev_indx][cvht]), speed, 
                      fabs(A317vs->hcf1[elev_indx][cvht]) - speed );
#endif
    }

#ifdef DEBUG
    /* Only print out the winds which have been calculated, i.e., are not 
       missing. */
    if( A317vs->vshw[elev_indx][cvht] != MISSING ){

        LE_send_msg( GL_INFO, "--->A317vs->vhtg[%d][%d]:        %10.4f\n",
                     elev_indx, cvht, A317vs->vhtg[elev_indx][cvht] );
        LE_send_msg( GL_INFO, "---->A317vs->slrn[%d][%d]:         %10.4f\n",
                     elev_indx, cvht, A317vs->slrn[elev_indx][cvht] );
        LE_send_msg( GL_INFO, "---->A317vs->nrads[%d][%d]:        %4d\n",
                     elev_indx, cvht, A317vs->nrads[elev_indx][cvht] );
        LE_send_msg( GL_INFO, "---->A317vs->vnpt[%d][%d]:     %4d\n",
                     elev_indx, cvht, A317vs->vnpt[elev_indx][cvht] );
        LE_send_msg( GL_INFO, "---->A317vs->vshw[%d][%d]:     %10.4f\n",
                     elev_indx, cvht, A317vs->vshw[elev_indx][cvht] );
        LE_send_msg( GL_INFO, "---->A317vs->vhwd[%d][%d]:     %10.4f\n",
                     elev_indx, cvht, A317vs->vhwd[elev_indx][cvht] );
        LE_send_msg( GL_INFO, "---->A317vs->vrms[%d][%d]:     %10.4f\n",
                     elev_indx, cvht, A317vs->vrms[elev_indx][cvht] );
        LE_send_msg( GL_INFO, "---->A317vs->href_avg[%d][%d]:     %10.4f\n",
                     elev_indx, cvht, A317vs->href_avg[elev_indx][cvht] );

    }
#endif
    
    return 0;

} /* End of A317t2_vad_proc_hts(). */


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
int A317s2_vad_data_hts( char *ipr, int ht ){

    /* Local variables */
    int i, hts_nv, bi_dbz, firstv, lastv, firstr, lastr;
    int elev_indx, n_dop_bins;
    float pfv, ht_sea, elevation, azimuth;
    float hts_sum_vel;
    double r1, d1;

    Base_data_header *bdh = (Base_data_header *) ipr;
    Moment_t vel, *iv = (Moment_t *) (ipr + bdh->vel_offset);
    Moment_t ref, *iz = (Moment_t *) (ipr + bdh->ref_offset);

    static float hts_htcon[ECUTMAX][MAX_VAD_HTS];

    /* Get the azimuth angle (degrees) of the radial. */
    azimuth = bdh->azimuth;

    /* Get the elevation angle (degrees) of the radial. */
    elevation = bdh->elevation;

    /* Get the RPG elevation cut index. */
    elev_indx = A317vi->celv;

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
    if( A317vs->nrads[elev_indx][ht] < 0 ){

	A317vs->nrads[elev_indx][ht] = 0;

        /* Compute parts of the HT_SEA equation that are repeative below */
	r1 = A317vs->slrn[elev_indx][ht]*1e3f;;
	hts_htcon[elev_indx][ht] = (A317va->rh + (r1*r1)/HTS_CONST)/1e3f;

        /* Set initial values for Nyquist velocity and azimuth sector start. */
	A317vs->ambig_range_ptr[elev_indx][ht] = 0;
	A317vs->ambig_range[ht][elev_indx][A317vs->ambig_range_ptr[elev_indx][ht]] = 
                               bdh->nyquist_vel;
	A317vs->ambig_azlim[ht][elev_indx][A317vs->ambig_range_ptr[elev_indx][ht]] = 
                               azimuth * 10.f;

        RPGCS_set_velocity_reso( (int) bdh->dop_resolution );

    } 
    else{

       int ind = A317vs->ambig_range_ptr[elev_indx][ht];


       /* Save range of ambiguous ranges for Nyquist velocity plotting. */
	if( bdh->nyquist_vel != A317vs->ambig_range[ht][elev_indx][ind] ){

	    ++A317vs->ambig_range_ptr[elev_indx][ht];

	    if( A317vs->ambig_range_ptr[elev_indx][ht] >= MAX_NUM_AMBRNG ) 
		A317vs->ambig_range_ptr[elev_indx][ht] = MAX_NUM_AMBRNG - 1;
	    
	    A317vs->ambig_range[ht][elev_indx][ind] = bdh->nyquist_vel;
	    A317vs->ambig_azlim[ht][elev_indx][ind] = azimuth * 10.f;

	}

    }

    /* If weather mode is clear air (WMODE=1) then, average the three 
       adjacient gates in range for each radial, and we don't need to 
       sum the reflectivity. */
    if( A317va->wmode == CLEAR_AIR_MODE ){

	hts_sum_vel = 0.f;
	hts_nv = 0;
	for( i = -1; i <= 1; ++i ){

            /* Make sure velocity data is not below threshold or 
               before or after the first or last good bin, respectively */
	    if( ((A317vs->velbin[elev_indx][ht] + i) <= lastv)
                              && 
                ((A317vs->velbin[elev_indx][ht] + i) >= firstv) ){

                vel = iv[A317vs->velbin[elev_indx][ht] + i];
		if( vel > BASEDATA_RDRNGF) {

                    /* Sum the velocities & increment the number of bins 
                       in the sumation */
		    hts_sum_vel += RPGCS_velocity_to_ms( vel );
		    ++hts_nv;

		}

	    }

	}

        /* Compute average velocity and store in HREF, HVEL, & HAZM arrays */
	if( hts_nv > 0 ){

	    A317vs->hvel[elev_indx][ht][A317vs->nrads[elev_indx][ht]] = hts_sum_vel / hts_nv;

            /* If reflectivity is good store the IZ value ... */
	    if( (A317vs->refbin[elev_indx][ht] >= firstr)
                              && 
                (A317vs->refbin[elev_indx][ht] <= lastr) ){

                ref =  iz[A317vs->refbin[elev_indx][ht]];

                if( ref > BASEDATA_RDRNGF) 
		    A317vs->href[elev_indx][ht][A317vs->nrads[elev_indx][ht]] = ref;

	    } 
            else{

                /* Otherwise store the Threshold value */
		A317vs->href[elev_indx][ht][A317vs->nrads[elev_indx][ht]] = 2;

	    }

	    A317vs->hazm[elev_indx][ht][A317vs->nrads[elev_indx][ht]] = azimuth;
	    ++A317vs->nrads[elev_indx][ht];

	}

    } 
    else{

        /* Since we're here, it's not clear air. So sum reflectivity, 
           and put velocity and azimuth angles in HREF, HVEL, & HAZM arrays. 
           also, correct velocity for precipitation fall speed 

           Make sure velocity data is not below threshold or 
           before or after the first or last good bin, respectively */
	if( (A317vs->velbin[elev_indx][ht] >= firstv)
                             && 
            (A317vs->velbin[elev_indx][ht] <= lastv) ){

            vel = iv[A317vs->velbin[elev_indx][ht]];

            if( vel > BASEDATA_RDRNGF ){

                /* Account for bad reflectivity data, set it to the 
                   low reflectivity threshold, 2 in biased units. */
	        if( (A317vs->refbin[elev_indx][ht] >= firstr)
                               && 
                    (A317vs->refbin[elev_indx][ht] <= lastr) ){

                    ref = iz[A317vs->refbin[elev_indx][ht]];
		    if( ref <= BASEDATA_RDRNGF ) 
		        bi_dbz = 2;

		    else
		        bi_dbz = ref;

		}
                else 
		    bi_dbz = 2;
	    
                /* Store the vel, ref and azimuth. */
	        A317vs->hvel[elev_indx][ht][A317vs->nrads[elev_indx][ht]] = 
                            RPGCS_velocity_to_ms( vel ); 
	        A317vs->href[elev_indx][ht][A317vs->nrads[elev_indx][ht]] = bi_dbz;
	        A317vs->hazm[elev_indx][ht][A317vs->nrads[elev_indx][ht]] = azimuth;

                /* ZE=10**(REF(IZ(IRBIN))/10.)  see comments below re: HT_SEA & PFV */

                /* HT_SEA equation modified so that the portion which is 
                   the same for each radial is only computed once per 
                   elevation scan.  Essentially: the first part of the 
                   equation is repeative and vad range is converted to 
                   kilometers 
                   before:   
                      HT_SEA=RH+VAD_RN/G**2/(2*IR*RE)+VAD_RNG*SIN(ELEVATION*DTR)
                      HT_SEA=HT_SEA/1000.0 */
	        ht_sea = hts_htcon[elev_indx][ht] + 
                         (A317vs->slrn[elev_indx][ht] * sin(elevation * DEGTORAD));

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
	        A317vs->hvel[elev_indx][ht][A317vs->nrads[elev_indx][ht]] += 
                            (pfv*sin(elevation * DEGTORAD));

                /* Increment the radial counter. */
	        ++A317vs->nrads[elev_indx][ht];

            }

	}

    }

    return 0;

} /* End of A317s2_vad_data_hts(). */


/*\////////////////////////////////////////////////////////////////////

   Description:
      Choose the height from A317vs with the lowest RMS, meeting
      symmetry and number of points criteria and replace
      data in A317ve.

   Return:
      Always returns 0.

////////////////////////////////////////////////////////////////////\*/
int A317r2_choose_height(){

    int i, k, elv, ht, c_elv, c_ht, n_elevs, replace_wind;
    float speed, diff;
   
    /* Initialization. */
    diff = 0.0;
    c_ht = -1;
    c_elv = -1;

    n_elevs = A317vs->vnels;
#ifdef DEBUG
     RPGC_log_msg( GL_INFO, "****** EVWP ADAPTATION DATA ******\n" );
     RPGC_log_msg( GL_INFO, "--->Min Points:      %4d\n", Vad.min_points );
     RPGC_log_msg( GL_INFO, "--->Min Symmetry:    %10.4f\n", Vad.min_symmetry );
     RPGC_log_msg( GL_INFO, "--->Scale RMS:       %10.4f\n", Vad.scale_rms );
     RPGC_log_msg( GL_INFO, "--->Min Range:       %10.4f\n", Vad.min_proc_range );
     RPGC_log_msg( GL_INFO, "--->Max Range:       %10.4f\n", Vad.max_proc_range );
     RPGC_log_msg( GL_INFO, "--->Enhanced VAD:    %4d\n", Vad.enhanced_vad );
     RPGC_log_msg( GL_INFO, "****************************************\n" );
#endif


    /* Loop through all VAD heights defined in adaptation data. */
    for( i = 0; i < MAX_VAD_HTS; i++ ){

#ifdef DEBUG
        LE_send_msg( 0, "Processing Hgt: %6d\n", Prodsel.vad_rcm_heights.vad[i] );
#endif

        /* Save information about all valid winds so we can later choose the best
           one. */
        Valid_cnt = 0;
        memset( &Save_info, 0, sizeof(Save_info_t)*MAX_VAD_HTS );

        for( elv = 0; elv < n_elevs; elv++ ){

            for( ht = 0; ht < MAX_VAD_HTS; ht++ ){

                if( (A317vs->vhtg[elv][ht] == Prodsel.vad_rcm_heights.vad[i])
                                          &&
                    (A317vs->vhtg[elv][ht] > 0) ){

                    if( A317vs->vshw[elv][ht] > MISSING ){

#ifdef DEBUG
                        LE_send_msg( 0, "--->Checking Wind: Speed/Dir[%3d][%3d]: %10.4f/%10.4f, RMS: %10.4f, NPTS: %4d, SR: %10.4f\n", 
                                     elv, ht, A317vs->vshw[elv][ht], A317vs->vhwd[elv][ht],
                                     A317vs->vrms[elv][ht], A317vs->vnpt[elv][ht], A317vs->slrn[elv][ht] );
#endif

                        /* If minimum rms is valid, check for valid symmetry. */
                        speed = sqrt( (A317vs->hcf2[elv][ht]*A317vs->hcf2[elv][ht]) +
                                      (A317vs->hcf3[elv][ht]*A317vs->hcf3[elv][ht]) );
                        diff = fabsf(A317vs->hcf1[elv][ht]) - speed;


                        if( (A317vs->vshw[elv][ht] >= (Vad.scale_rms*A317vs->vrms[elv][ht]))
                                             &&
                            (diff < Vad.min_symmetry) 
                                             &&
                            (A317vs->vnpt[elv][ht] >= Vad.min_points) ){

                            Save_info[Valid_cnt].elv_ind = elv;
                            Save_info[Valid_cnt].ht_ind = ht;

#ifdef DEBUG
                            LE_send_msg( 0, "------>Save_info[%d].elv_ind: %d, Save_info[%d].ht_ind: %d\n",
                                         Valid_cnt, Save_info[Valid_cnt].elv_ind, Valid_cnt, Save_info[Valid_cnt].ht_ind );
                            LE_send_msg( 0, "------>speed: %10.4f, hcf1: %10.4f, diff: %10.4f, min_sym: %10.4f\n",
                                         speed, A317vs->hcf1[elv][ht], diff, Vad.min_symmetry );
#endif
                            Valid_cnt++;

                        }
#ifdef DEBUG
                        else{

                           LE_send_msg( GL_INFO, "------>Wind Discarded .... \n");
                           LE_send_msg( GL_INFO, "--------->Speed: %10.4f >=? Scale_rms*RMS: %10.4f\n",
                                        A317vs->vshw[elv][ht], Vad.scale_rms*A317vs->vrms[elv][ht] );
                           LE_send_msg( GL_INFO, "--------->Diff: %10.4f <? Min Symmetry: %10.4f\n",
                                        diff, Vad.min_symmetry );
                           LE_send_msg( GL_INFO, "--------->Npts: %d >=? Min Points: %d\n",
                                        A317vs->vnpt[elv][ht], Vad.min_points );
                        }
#endif

                    }

                }

            }

        }

        /* If 4 or more valid wind estimates are available, then compute the vector 
           average of the available winds.   Choose the wind estimate that is closest 
           to the vector average.  If three or less valid wind estimates are available, 
           pick the one with the lowest RMS / SPEED/ # PTS ratio. */
        if( Valid_cnt >= 4 ){

#ifdef DEBUG
            LE_send_msg( 0, "--># Winds: %d >= 4 ... Compute Average Wind Vector\n", Valid_cnt );
#endif

            /* Find the closest to the vector average. */
            Find_closest_to_average_wind( &c_elv, &c_ht );
#ifdef DEBUG

            LE_send_msg( 0, "---->Closest to Vector Avg Wind:    RMS: %f, Speed/Direction: %10.4f/%10.4f\n", 
                         A317vs->vrms[c_elv][c_ht], A317vs->vshw[c_elv][c_ht], A317vs->vhwd[c_elv][c_ht] );
#endif

        }
        else{

            float min_ratio = 999.0, ratio;
            int  j;

            /* Initialize variables. */
            min_ratio = 999.0;
            c_elv = -1;
            c_ht = -1;

            /* Find the wind with the lowest RMS / SPEED / # PTS ratio. */
            for( j = 0; j < Valid_cnt; j++ ){

                elv = Save_info[j].elv_ind;
                ht = Save_info[j].ht_ind;

                ratio = A317vs->vrms[elv][ht] / A317vs->vshw[elv][ht];
                ratio /= (float) A317vs->vnpt[elv][ht];

                if( ratio < min_ratio ){

                    min_ratio = ratio;
                    c_elv = elv;
                    c_ht = ht;

                }

            }

#ifdef DEBUG
            if( (c_elv >= 0) && (c_ht >= 0) )
                LE_send_msg( 0, "---->Minimum Ratio:  %10.5f, RMS: %10.4f, Speed/Direction: %10.4f/%10.4f\n",
                             min_ratio, A317vs->vrms[c_elv][c_ht], A317vs->vshw[c_elv][c_ht],
                             A317vs->vhwd[c_elv][c_ht] );
#endif

        }

        /* The following logic decides whether or not to replace the original
           with a supplemental wind. */
        if( (c_elv >= 0) && (c_ht >= 0) ){
 
            /* Is original VAD wind missing OR the ratio RMS/SPEED/# PTS is less than the
               ratio for the original wind. */
            replace_wind = 0;
            if( A317ve->vshw[Cvol][c_ht] == MISSING ) 
                replace_wind = 1;

            else {

                float weight_a, weight_o;

                weight_a = A317vs->vrms[c_elv][c_ht] / A317vs->vshw[c_elv][c_ht];
                weight_a /= (float) A317vs->vnpt[c_elv][c_ht];

                weight_o = A317ve->vrms[Cvol][c_ht] / A317ve->vshw[Cvol][c_ht];
                weight_o /= (float) A317ve->vnpt[Cvol][c_ht];

#ifdef DEBUG
                LE_send_msg( 0, "--->Supplemental: RMS: %10.4f, Speed: %10.4f, # Pts: %3d\n",
                             A317vs->vrms[c_elv][c_ht], A317vs->vshw[c_elv][c_ht],
                             A317vs->vnpt[c_elv][c_ht] );
                LE_send_msg( 0, "------>Weight: %10.6f\n", weight_a );
                LE_send_msg( 0, "--->Original:     RMS: %10.4f, Speed: %10.4f, # Pts: %3d\n",
                             A317ve->vrms[Cvol][c_ht], A317ve->vshw[Cvol][c_ht],
                             A317ve->vnpt[Cvol][c_ht] );
                LE_send_msg( 0, "------>Weight: %10.6f\n", weight_o );
#endif

                if( weight_a < weight_o )
                    replace_wind = 1;

            }

            /* We are replacing the original wind estimate with a supplemental wind. */
            if( replace_wind ){

                int ltime, elang10;
                float elang;

                /* Get the volume scan time. */
                A3183a_cnvtime( A317vd->time[Cvol]*1000, &ltime );

                /* Get the elevation angle. */
                elang10 = RPGCS_get_target_elev_ang( Vcpno, A317vs->elcn[c_elv][c_ht] );
                elang = (float) elang10/10.0f;
#ifdef DEBUG
                LE_send_msg( 0, "Supplement Wind Replacing Original .....\n" );

                LE_send_msg( 0, "--->Volume Scan Time (hhmm):          %4.4d\n", ltime );
                LE_send_msg( 0, "--->A317vs->vhtg[%3d][%3d]:      %10.4f\n",
                             c_elv, c_ht, A317vs->vhtg[c_elv][c_ht] );
                LE_send_msg( 0, "--->A317vs->slrn[%3d][%3d]:           %10.4f\n",
                             c_elv, c_ht, A317vs->slrn[c_elv][c_ht] );
                LE_send_msg( 0, "--->Elevation Angle (deg):            %10.4f\n", elang );
                LE_send_msg( 0, "--->A317vs->vshw[%3d][%3d]:      %10.4f\n",
                             c_elv, c_ht, A317vs->vshw[c_elv][c_ht] );
                LE_send_msg( 0, "--->A317vs->vhwd[%3d][%3d]:      %10.4f\n",
                             c_elv, c_ht, A317vs->vhwd[c_elv][c_ht] );
                LE_send_msg( 0, "--->A317vs->vrms[%3d][%3d]:      %10.4f\n",
                             c_elv, c_ht, A317vs->vrms[c_elv][c_ht] );
                LE_send_msg( 0, "--->A317vs->vnpt[%3d][%3d]:      %4d\n",
                             c_elv, c_ht, A317vs->vnpt[c_elv][c_ht] );
                LE_send_msg( 0, "--->A317vs->href_avg[%3d][%3d]:       %10.4f\n",
                             c_elv, c_ht, A317vs->href_avg[c_elv][c_ht] );
#endif

                A317ve->nrads[c_ht] = A317vs->nrads[c_elv][c_ht];
                A317ve->elcn[c_ht] = A317vs->elcn[c_elv][c_ht];
                A317ve->slrn[c_ht] = A317vs->slrn[c_elv][c_ht];
                A317ve->hcf1[c_ht] = A317vs->hcf1[c_elv][c_ht];
                A317ve->hcf2[c_ht] = A317vs->hcf2[c_elv][c_ht];
                A317ve->hcf3[c_ht] = A317vs->hcf3[c_elv][c_ht];

                /* Note: The following value should be identical. */
                if( A317ve->vhtg[Cvol][c_ht] != A317vs->vhtg[c_elv][c_ht] )
                    LE_send_msg( 0, "Mismatch In Height Values.\n" );
                A317ve->vhtg[Cvol][c_ht] = A317vs->vhtg[c_elv][c_ht];

                A317ve->vrms[Cvol][c_ht] = A317vs->vrms[c_elv][c_ht];
                A317ve->vhwd[Cvol][c_ht] = A317vs->vhwd[c_elv][c_ht];
                A317ve->vshw[Cvol][c_ht] = A317vs->vshw[c_elv][c_ht];
                A317ve->vnpt[Cvol][c_ht] = A317vs->vnpt[c_elv][c_ht];

                A317ve->ambig_range_ptr[c_ht] = A317vs->ambig_range_ptr[c_elv][c_ht];
                for( k = 0; k < MAX_NUM_AMBRNG; k++ ){
    
                    A317ve->ambig_range[k][c_ht] = A317vs->ambig_range[k][c_elv][c_ht];
                    A317ve->ambig_azlim[k][c_ht] = A317vs->ambig_azlim[k][c_elv][c_ht];

                }

                A317ve->velbin[c_ht] = A317vs->velbin[c_elv][c_ht];
                A317ve->refbin[c_ht] = A317vs->refbin[c_elv][c_ht];
    
                memcpy( &A317ve->hvel[c_ht][k], &A317vs->hvel[c_elv][c_ht][k], 
                        NAZIMS*sizeof(float) );
                memcpy( &A317ve->href[c_ht][k], &A317vs->href[c_elv][c_ht][k], 
                        NAZIMS*sizeof(float) );
                memcpy( &A317ve->hazm[c_ht][k], &A317vs->hazm[c_elv][c_ht][k], 
                        NAZIMS*sizeof(float) );

            }

        }

        /* Reinitialize variables. */
        diff = 0;
        c_elv = -1;
        c_ht = -1;

    }

    return 0;

} /* End of A317r2_choose_height(). */

/*\////////////////////////////////////////////////////////////////////

   Description:
      Find the estimated wind which is closest to the average of the
      estimated winds.

   Outputs:
      s_elv - holds the elv index of the closest to average.
      s_ht - holds the ht index of the closest to average.

   Return:
      Always returns 0.

////////////////////////////////////////////////////////////////////\*/
static int Find_closest_to_average_wind( int *s_elv, int *s_ht ){

    float avg_u = 0.0f, avg_v = 0.0f;
    float avg_speed = 0.0f, avg_dir = 0.0f;
    double dist2 = 0.0, min_dist2 = 9999.0;
    int j, elv, ht, min_ind = -1;

    float u[MAX_VAD_HTS], v[MAX_VAD_HTS];

    /* Initialize u and v. */
    memset( u, 0, MAX_VAD_HTS*sizeof(float) );
    memset( v, 0, MAX_VAD_HTS*sizeof(float) );

    /* Initialize indicies. */
    *s_elv = *s_ht = -1;

    /* Compute the average wind speed. */
    for( j = 0; j < Valid_cnt; j++ ){

        elv = Save_info[j].elv_ind;
        ht = Save_info[j].ht_ind;

        u[j] = A317vs->hcf2[elv][ht] / cos( A317vs->velv[elv][ht] );
        avg_u += u[j];

        v[j] = A317vs->hcf3[elv][ht] / cos( A317vs->velv[elv][ht] );
        avg_v += v[j];

    }  

    avg_u /= (float) Valid_cnt;
    avg_v /= (float) Valid_cnt;
    avg_speed = sqrt( (avg_u*avg_u) + (avg_v*avg_v) );
    if( (avg_u != 0.f) && (avg_v != 0.f) )
        avg_dir = PI_CONST - atan2( (double) avg_v, avg_u );

    else
        avg_dir = 0.f;

    /* Check if wind direction computes to be negative, then convert to 
       degrees. */
    if( avg_dir < 0.f )
        avg_dir += 2.0*PI_CONST;

    avg_dir /= DEGTORAD;

    /* Find the wind estimate closest to the average. */
    for( j = 0; j < Valid_cnt; j++ ){

        dist2 = (u[j] - avg_u)*(u[j] - avg_u) +
                (v[j] - avg_v)*(v[j] - avg_v);

        if( dist2 < min_dist2 ){

            min_dist2 = dist2;
            min_ind = j; 

        }

    }

    /* Return minimum. */
    *s_elv = Save_info[min_ind].elv_ind;
    *s_ht = Save_info[min_ind].ht_ind;

    return 0;

/* End of Find_closest_to_average_wind() */
}

