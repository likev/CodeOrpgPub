/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2008/02/29 18:08:00 $ */
/* $Id: a304di.c,v 1.2 2008/02/29 18:08:00 ccalvert Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <veldeal.h>

/*********************************************************************

   Description:
      Initialize working copy of velocity dealiasing adaptable 
      parameters for this volume scan based on parameter passed
      to this module (pulse_length).  Velocity dealiasing adaptation
      data is dimensioned based on short and long pulse, respectively.
      The parameter pulse_length is defined by the RDA/RPG ICD as 2
      for short pulse and 4 for long pulse.

   Input:
      pulse_length - used to derive an index into adaptation data.

*********************************************************************/
int A304di_vd_local_copy( int pulse_length ){

    /* Local variables */
    int curr_time, delta_time, ndx;

    ndx = pulse_length / 2;

    /* Set old nyvel to invalid Nyquist velocity. */
    Old_nyvel = -100;

    /* Initialize working copy of adaptation data. */
    if( ndx == 1 ){

	A304di.num_bin_fstchk = Radazvd.th_bin_first_check_s;
	A304di.num_rep_lkahd = Radazvd.num_replace_lookahead_s;
	A304di.num_rep_lkbk = Radazvd.num_replace_lookback_s;
	A304di.num_lkbk = Radazvd.num_lookback_s;
	A304di.num_lkfor = Radazvd.num_lookforward_s;
	A304di.th_conbin_rej = Radazvd.th_consecutive_rejected_s;
	A304di.th_max_conazjmp = Radazvd.th_max_azm_with_jump_s;
	A304di.th_mxmiss = Radazvd.th_maximum_missing_s;
	A304di.num_reunf_prazs = Radazvd.num_reunfold_previous_azm_s;
	A304di.th_mxbins_jmp = Radazvd.th_max_bins_with_jump_s;
	A304di.th_diff_unfold = Radazvd.th_difference_unfold_s;
	A304di.th_bins_lrg_azjmp = Radazvd.th_bins_large_azm_jump_s;
	A304di.num_reunf_cazs = Radazvd.num_reunfold_current_azm_s;
	A304di.th_vel_jmp_frad = Radazvd.th_vel_jump_fraction_radial_s;
	A304di.th_vel_jmp_faz = Radazvd.th_vel_jump_fraction_azm_s;
	A304di.th_scl_stdev = Radazvd.th_scale_standard_dev_s;
	A304di.th_scl_diff_unfold = Radazvd.th_scale_diff_unfold_s;
	A304di.env_sound_to = Radazvd.env_winds_timeout_s;
	A304di.use_soundings_flag = Radazvd.use_sounding_s;
	A304di.rep_rejected_vel = Radazvd.replace_rejected_vel_s;
	Num_intrvl_chk = (short) Radazvd.num_interval_checks_s;

    }
    else{

	A304di.num_bin_fstchk = Radazvd.th_bin_first_check_l;
	A304di.num_rep_lkahd = Radazvd.num_replace_lookahead_l;
	A304di.num_rep_lkbk = Radazvd.num_replace_lookback_l;
	A304di.num_lkbk = Radazvd.num_lookback_l;
	A304di.num_lkfor = Radazvd.num_lookforward_l;
	A304di.th_conbin_rej = Radazvd.th_consecutive_rejected_l;
	A304di.th_max_conazjmp = Radazvd.th_max_azm_with_jump_l;
	A304di.th_mxmiss = Radazvd.th_maximum_missing_l;
	A304di.num_reunf_prazs = Radazvd.num_reunfold_previous_azm_l;
	A304di.th_mxbins_jmp = Radazvd.th_max_bins_with_jump_l;
	A304di.th_diff_unfold = Radazvd.th_difference_unfold_l;
	A304di.th_bins_lrg_azjmp = Radazvd.th_bins_large_azm_jump_l;
	A304di.num_reunf_cazs = Radazvd.num_reunfold_current_azm_l;
	A304di.th_vel_jmp_frad = Radazvd.th_vel_jump_fraction_radial_l;
	A304di.th_vel_jmp_faz = Radazvd.th_vel_jump_fraction_azm_l;
	A304di.th_scl_stdev = Radazvd.th_scale_standard_dev_l;
	A304di.th_scl_diff_unfold = Radazvd.th_scale_diff_unfold_l;
	A304di.env_sound_to = Radazvd.env_winds_timeout_l;
	A304di.use_soundings_flag = Radazvd.use_sounding_l;
	A304di.rep_rejected_vel = Radazvd.replace_rejected_vel_l;
	Num_intrvl_chk = (short) Radazvd.num_interval_checks_l; 

    }

    A304di.disable_radaz_dealiasing = Radazvd.disable_radaz_dealiasing;
    A304di.use_sprt_replace_rej = Radazvd.use_sprt_replace_rej;

    /* Get the current Julian time and convert to Julian minutes.  
       Compute the delta time from the sounding time to the
       current time. */
    curr_time = time( NULL ) / 60;
    delta_time = curr_time - Ewt.sound_time;

    /* If the delta time is greater than or equal to the time out for
       environmental winds and there are valid soundings, send a message
       to the status log indicating that the environmental winds data 
       has timed-out.   Set the valids soundings flag to 0. */
    if( delta_time >= A304di.env_sound_to ){

	if( Valid_soundings ) 
	    RPGC_log_msg( GL_STATUS, "Environmental Winds Data has TIMED OUT\n" );

	Valid_soundings = 0;

    }

    /* Set the sounding available flag. */
    A304dg.sounding_avail = Valid_soundings && A304di.use_soundings_flag;

    /* Return to calling routine. */
    return 0;

/* End of A304di_vd_local_copy() */
} 
