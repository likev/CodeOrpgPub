/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/05/12 18:44:44 $ */
/* $Id: a304d2.c,v 1.6 2009/05/12 18:44:44 steves Exp $ */
/* $Revision: 1.6 $ */
/* $State: Exp $ */

#include <veldeal.h>

/* Global variables. */
static int Num_bin_fstchk;
static int Fst_good_dpbin; 
static int Lst_good_dpbin;

/* Global variables for SPRT */

static int nyqvel;
static int Data_reso;

/* Function Prototypes. */
static int Set_prevaz( short *veloc );
static int Replace_velocity( short *veloc );

/*****************************************************************************

   Description:
      The driver module for the RADAZVD velocity algorithm.

   Inputs:
      input_ptr - pointer to input radial.

   Returns:
      Always returns 0.

*****************************************************************************/
int A304d2_rad_azm_unfold( Base_data_header *radhdr, short *veloc ){

    int last_bin_gdvel, tol_fstchk, binnum, diff;

    /*  Set some values for resetting adaptable parameters after call to 
        Replace_velocity when using Staggered PRT waveform */
    nyqvel = radhdr->nyquist_vel;
    Data_reso = 3 - radhdr->dop_resolution;

    /* Initialize variables. */
    Con_bins_rej = 0;
    Num_deleted = 0;
    Vel_jump = 0;
    Bin_lrg_azjump = 0;

    /* These values should be registered since they don't change during
       the course of processing the radial. */
    Fst_good_dpbin = A304dd.fst_good_dpbin; 
    Lst_good_dpbin = A304dd.lst_good_dpbin;
    Num_bin_fstchk = A304di.num_bin_fstchk;

    tol_fstchk = A304dg.tol_fstchk;

    /* Process all radial bins from the first good Doppler bin to the last
       good Doppler bin. */
    last_bin_gdvel = -32700;

    for( binnum = Fst_good_dpbin; binnum <= Lst_good_dpbin; ++binnum ){

        /* If velocity is BAD, initialize the number of continuous bins 
           rejected to 0. */
	if( veloc[binnum] <= BAD ){

	    Con_bins_rej = 0;
            continue;

        }

        /* Bias the velocity .... */
        veloc[binnum] += VEL_BIAS;
       
        /* Check if bin number - last bin with good velocity data on the 
           current radial is within the number of bins to look back for   
           initial radial continuity check. */
	if( (binnum - last_bin_gdvel) <= Num_bin_fstchk ){

	    diff = veloc[binnum] - veloc[last_bin_gdvel]; 
	    if( abs(diff) < tol_fstchk ){

                /* If radial continuity is establihsed without attempting to unfold  
                   the data, the last bin with good velocity data is set to the current
                   bin location and error checking commences with call to A304d8... 
                   Reinitialize the number of consecutive bins rejected to 0. */   
		A304dj.jmpchk = 1;
		last_bin_gdvel = binnum;
		A304d8_jump_check( veloc, binnum, last_bin_gdvel );
		Con_bins_rej = 0;
                continue;

            }
	    else {

                /* If the difference between the current velocity data and the
                   last bin with good velocity is not within threshold difference 
                   to unfold, call A304d3 to minimize this velocity difference. */
	        A304dj.no_chk_flg = 0;
		A304d3_min_vel_dif( &veloc[binnum], veloc[last_bin_gdvel], 
                                    tol_fstchk );

                /* If no check flag is false, need to find other measns to  
                   find a velocity for comparison.   Set the jump check flag  
                   for velocity jump checking along the radial.   Clear the not  
                   within bins flag indicating that the nearest previous good  
                   velocity is within Num_bin_fstchk. */
		if( !A304dj.no_chk_flg ){

		    A304dj.not_wibins_flg = 0;
		    A304dj.jmpchk = 0;
		    last_bin_gdvel = A304dv_find_other_means( (short *) radhdr, 
                                                              veloc, binnum,
                                                              last_bin_gdvel );

	        }
                else{

                    /* Otherwise, set the bin location of the last good velocity on   
                       the current radial to the current bin location.  Call A304d8
                       to check for large velocity jumps in azimuth.   Reinitialize  
                       the number of consecutive bins rejected to 0. */
		    A304dj.jmpchk = 1;
		    last_bin_gdvel = binnum;
		    A304d8_jump_check( veloc, binnum, last_bin_gdvel );
		    Con_bins_rej = 0;

	        }

	    }

	}
        else{


            /* There are no good values along current radial within num_bins_fstchk.
               Need to call A304dv to find a velocity to compare current velocity to. */
	    A304dj.no_chk_flg = 0;
	    A304dj.jmpchk = 1;
	    A304dj.not_wibins_flg = 1;
	    last_bin_gdvel = A304dv_find_other_means( (short *) radhdr, veloc, 
                                                      binnum, last_bin_gdvel );

        }

    } /* End of "for loop" */

    /* Copy current radial data into a saved table. */
    Set_prevaz( veloc );
    return 0;

/* A304d2_rad_azm_unfold() */
}

/*********************************************************************

   Description:
      Makes a copy of current radial for use on next radial when
      dealiasing.

   Inputs:
      veloc - pointer to velocity data.

   Returns:
      Always returns 0.

*********************************************************************/
static int Set_prevaz( short *veloc ){

    /* Local variables */
    int rngi, result;

    /* Large velocity jump exist on the current radial. */
    if( Vel_jump ){

	++Num_jump_conrad;

        /* If mnumber of radial with large velocity jump >= the threshold, 
           initialize the number of radials with large velocity jump. */
	if( Num_jump_conrad >= A304di.th_max_conazjmp ){

	    Num_jump_conrad = 0;
	    A304db.status = BAD_PREV_RADIAL;
	    A304db.fstgd_dpbin_prevaz = VELDEAL_MAX_SIZE + 1;
	    A304db.lstgd_dpbin_prevaz = -1;

	}

        /* Replace any velocity values which had been discarded previoulsy 
           and not replaced. */
	if( A304di.rep_rejected_vel && (Num_deleted > 0) ){
        /*  If Waveform is staggered PRT rescale adaptable parameters
            based on 1/2 the extended Nyquist velocity                */  
            if(Waveform == VCP_WAVEFORM_STP && A304di.use_sprt_replace_rej) {
 	       Old_nyvel = nyqvel/2;
	       Nyvel = nyqvel * Data_reso * .005f;

               /* Compute new Nyquist velocity dependent thresholds. */
	       result = Define_edit_parm( );
            }
	    Replace_velocity( veloc );
         }  

        /* Set the velocity at all bins from the first good to the last good 
           Doppler bin to the unbiased velocity. */
	for( rngi = Fst_good_dpbin; rngi <= Lst_good_dpbin; ++rngi){

            /* Make sure velocity is within range of Unbias_vel [0: LOOKUP_SIZE] */
            if( veloc[rngi] > LOOKUP_SIZE )
               veloc[rngi] = LOOKUP_SIZE;

            else if( veloc[rngi] < BASEDATA_RDBLTH )
               veloc[rngi] = 2;

	    veloc[rngi] = Unbias_vel[veloc[rngi]];

        }

    /* No large velocity jumps exist on the current radial. */
    }
    else{

	Num_jump_conrad = 0;
	A304db.status = GOOD_PREV_RADIAL;

        /* Set first and last good velocity bin on the previous radial to the
           first and last good bin on the current radial. */
	A304db.fstgd_dpbin_prevaz = Fst_good_dpbin;
	A304db.lstgd_dpbin_prevaz = Lst_good_dpbin;

        /* Set the previous saved radial velocity to the current radial velocity. */
	for( rngi = Fst_good_dpbin; rngi <= Lst_good_dpbin; ++rngi) 
	    A304db.vel_prevaz[GOOD_PREV_RADIAL][rngi] = veloc[rngi];

        /* Replace any velocity values which had been discarded previously and not
           replaced. */
	if( A304di.rep_rejected_vel && (Num_deleted > 0) ){
        /*  If Waveform is staggered PRT rescale adaptable parameters
            based on 1/2 the extended Nyquist velocity                */  
            if(Waveform == VCP_WAVEFORM_STP && A304di.use_sprt_replace_rej) {
 	       Old_nyvel = nyqvel/2;
	       Nyvel = nyqvel * Data_reso * .005f;

               /* Compute new Nyquist velocity dependent thresholds. */
	       result = Define_edit_parm( );
            }
	    Replace_velocity( veloc );
         }  

        /* Unbias the velocity data. */
	for( rngi = Fst_good_dpbin; rngi <= Lst_good_dpbin; ++rngi){

            /* Make sure velocity is within range of Unbias_vel [0: LOOKUP_SIZE] */
            if( veloc[rngi] > LOOKUP_SIZE )
               veloc[rngi] = LOOKUP_SIZE;

            else if( veloc[rngi] < BASEDATA_RDBLTH )
               veloc[rngi] = 2;

	    veloc[rngi] = Unbias_vel[veloc[rngi]];

        }

    }

    return 0;

/* End of Set_prevaz() */
} 


/*****************************************************************

   Description:
      Replace velocities which were previously removed and couldn't
      be replaced.

   Inputs:
      veloc - pointer to velocity data.

   Returns:
      Always returns 0.

*****************************************************************/
static int Replace_velocity( short *veloc ){

    /* Local variables */
    int i, l, diff;
    short bin, chk_vel, min_bin, max_bin, number_replaced;

    /* The following section starts at a missing gate where the 
       original value was not BAD. The loop looks away from the 
       radar for the first good gate. It then uses that gate to 
       try and replace the current BAD gate by either putting it 
       back as is or unfolding it. */
    number_replaced = 0;
    for( i = Num_deleted - 1; i >= 0; --i ){

	A304dj.no_chk_flg = 0;
	bin = Deleted_bin[i];
	chk_vel = Deleted_vel[i];

        /* Computing MIN */
	if(A304dd.lst_good_dpbin < (bin + Num_bin_fstchk) )
	    max_bin = (short) A304dd.lst_good_dpbin;

        else
	    max_bin = (short) (bin + Num_bin_fstchk);

	l = bin + 1;

        /* Find the first good gate away from the radar beyond 
           the current bad gate.  If a good gate was found, check 
           it for gate to gate continuity. If the original velocity 
           is not within the threshold, try to unfold it into that 
           range. */
        while( l <= max_bin ){

  	    if( veloc[l] > BAD ){

	        diff = chk_vel - veloc[l];
                if( abs(diff) > A304dg.th_diff_unf_relax ){

		    A304d3_min_vel_dif( &chk_vel, veloc[l], 
		                        A304dg.th_diff_unf_relax );
		    if( A304dj.no_chk_flg ){

                        /* Velocity dealiased to within required threshold. 
                           Replace velocity, assign the bin number for this 
                           replaced velocity to flag value, and increment 
                           the number of velocities replaced. */
			Deleted_bin[i] = -1;
			veloc[bin] = chk_vel;
			number_replaced++;

		    }

		}
                else{

                    /* Velocity is within required threshold.  Replace 
                       velocity, assign the bin number for this replaced 
                       velocity to flag value, and increment the number of 
                       velocities replaced. */
		    Deleted_bin[i] = -1;
		    veloc[bin] = chk_vel;
		    number_replaced++;

		}

                break;

	    }
            else{

                /* Increment "l" to look at the next bin along the radial. 
                   Repeat above sequence. */
		++l;

	    }

        }

    } /* End of "for loop" */

    /* If all deleted velocities have been replaced, quit. */
    if( number_replaced == Num_deleted ) 
	return 0;

    /* Try to pass through the data going from the radar to the end 
       of the radial using the same unfolding/correcting as above. */
    for( i = 0; i < Num_deleted; ++i ){

        /* First extract the bin number.  If less than 0, this 
           velocity has already been replaced. */
	bin = Deleted_bin[i];
	if( bin < 0 )
	    continue;
	 
	veloc[bin] = Deleted_vel[i];
	A304dj.no_chk_flg = 0;

        /* Computing MAX */
        if( Fst_good_dpbin < (bin - Num_bin_fstchk) )
            min_bin = bin - Num_bin_fstchk;

        else
	    min_bin = Fst_good_dpbin;

	l = bin - 1;

        /* Find the first good gate away from the radar beyond 
           the current bad gate.  If a good gate was found, check 
           it for gate to gate continuity. If the original velocity 
           is not within the threshold, try to unfold it into that 
           range. */
        while( l >= min_bin ){

	    if( veloc[l] > BAD ){

                diff = veloc[bin] - veloc[l];
                if( abs(diff) > A304dg.th_diff_unf_relax )
		    A304d3_min_vel_dif( &veloc[bin], veloc[l], 
			                A304dg.th_diff_unf_relax );

                break;
		    
	    }
            else{

                /* Decrement L to look at the next bin along the radial. 
                   Repeat above sequence. */
		--l;

            }

	}

    } /* End of "for loop" */

    return 0;

/* End of Replace_velocity() */
} 

