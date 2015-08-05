/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/03/03 23:23:10 $
 * $Id: radazvd_callback_fx.c,v 1.13 2009/03/03 23:23:10 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <radazvd.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define radazvd_callback_fx radazvd_callback_fx_
#endif

#ifdef LINUX
#define radazvd_callback_fx radazvd_callback_fx__
#endif

#endif


int radazvd_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  radazvd_t *radazvd = ( radazvd_t * )common_block_address;


  /* Get radazvd data elements */
  /* Short pulse */

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_replace_lookahead_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_replace_lookahead_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_replace_lookahead_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_replace_lookback_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_replace_lookback_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_replace_lookback_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_lookback_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_lookback_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_lookback_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_lookforward_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_lookforward_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_lookforward_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_consecutive_rejected_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_consecutive_rejected_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_consecutive_rejected_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_maximum_missing_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_maximum_missing_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_maximum_missing_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_reunfold_previous_azm_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_reunfold_previous_azm_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_reunfold_previous_azm_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_reunfold_current_azm_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_reunfold_current_azm_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_reunfold_current_azm_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_difference_unfold_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_difference_unfold_s = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_difference_unfold_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_max_bins_with_jump_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_max_bins_with_jump_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_max_bins_with_jump_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_vel_jump_fraction_radial_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_vel_jump_fraction_radial_s = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_vel_jump_fraction_radial_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_vel_jump_fraction_azm_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_vel_jump_fraction_azm_s = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_vel_jump_fraction_azm_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_scale_standard_dev_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_scale_standard_dev_s = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_scale_standard_dev_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_max_azm_with_jump_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_max_azm_with_jump_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_max_azm_with_jump_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_scale_diff_unfold_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_scale_diff_unfold_s = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_scale_diff_unfold_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_bins_large_azm_jump_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_bins_large_azm_jump_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_bins_large_azm_jump_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_bin_first_check_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_bin_first_check_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_bin_first_check_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".env_winds_timeout_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> env_winds_timeout_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: env_winds_timeout_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".use_sounding_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> use_sounding_s = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: use_sounding_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".replace_rejected_vel_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> replace_rejected_vel_s = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: replace_rejected_vel_s unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_interval_checks_s", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_interval_checks_s = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_interval_checks_s unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "RADAZVD: num_replace_lookahead_s \t\t= %d\n",  radazvd -> num_replace_lookahead_s );
   LE_send_msg( GL_INFO, "RADAZVD: num_replace_lookback_s \t\t= %d\n",  radazvd -> num_replace_lookback_s );
   LE_send_msg( GL_INFO, "RADAZVD: num_lookback_s \t\t= %d\n",  radazvd -> num_lookback_s );
   LE_send_msg( GL_INFO, "RADAZVD: num_lookforward_s \t\t= %d\n",  radazvd -> num_lookforward_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_consecutive_rejected_s \t\t= %d\n",  radazvd -> th_consecutive_rejected_s);
   LE_send_msg( GL_INFO, "RADAZVD: th_maximum_missing_s \t\t= %d\n",  radazvd -> th_maximum_missing_s );
   LE_send_msg( GL_INFO, "RADAZVD: num_reunfold_previous_azm_s \t\t= %d\n",  radazvd -> num_reunfold_previous_azm_s );
   LE_send_msg( GL_INFO, "RADAZVD: num_reunfold_current_azm_s \t\t= %d\n",  radazvd -> num_reunfold_current_azm_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_difference_unfold_s \t\t= %4.1f\n",  radazvd -> th_difference_unfold_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_max_bins_with_jump_s \t\t= %d\n",  radazvd -> th_max_bins_with_jump_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_vel_jump_fraction_radial_s \t\t= %4.2f\n",  radazvd -> th_vel_jump_fraction_radial_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_vel_jump_fraction_azm_s \t\t= %4.2f\n",  radazvd -> th_vel_jump_fraction_azm_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_scale_standard_dev_s \t\t= %4.2f\n",  radazvd -> th_scale_standard_dev_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_max_azm_with_jump_s \t\t= %d\n",  radazvd -> th_max_azm_with_jump_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_scale_diff_unfold_s \t\t= %4.2f\n",  radazvd -> th_scale_diff_unfold_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_bins_large_azm_jump_s \t\t= %d\n",  radazvd -> th_bins_large_azm_jump_s );
   LE_send_msg( GL_INFO, "RADAZVD: th_bin_first_check_s \t\t= %d\n",  radazvd -> th_bin_first_check_s );
   LE_send_msg( GL_INFO, "RADAZVD: env_winds_timeout_s \t\t= %d\n",  radazvd -> env_winds_timeout_s );
   LE_send_msg( GL_INFO, "RADAZVD: use_sounding_s \t\t= %d\n",  radazvd -> use_sounding_s );
   LE_send_msg( GL_INFO, "RADAZVD: replace_rejected_vel_s \t\t= %d\n",  radazvd -> replace_rejected_vel_s );
   LE_send_msg( GL_INFO, "RADAZVD: num_interval_checks_s \t\t= %d\n",  radazvd -> num_interval_checks_s );
#endif

  /* Long pulse */

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_replace_lookahead_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_replace_lookahead_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_replace_lookahead_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_replace_lookback_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_replace_lookback_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_replace_lookback_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_lookback_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_lookback_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_lookback_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_lookforward_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_lookforward_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_lookforward_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_consecutive_rejected_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_consecutive_rejected_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_consecutive_rejected_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_maximum_missing_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_maximum_missing_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_maximum_missing_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_reunfold_previous_azm_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_reunfold_previous_azm_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_reunfold_previous_azm_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_reunfold_current_azm_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_reunfold_current_azm_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_reunfold_current_azm_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_difference_unfold_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_difference_unfold_l = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_difference_unfold_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_max_bins_with_jump_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_max_bins_with_jump_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_max_bins_with_jump_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_vel_jump_fraction_radial_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_vel_jump_fraction_radial_l = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_vel_jump_fraction_radial_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_vel_jump_fraction_azm_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_vel_jump_fraction_azm_l = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_vel_jump_fraction_azm_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_scale_standard_dev_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_scale_standard_dev_l = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_scale_standard_dev_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_max_azm_with_jump_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_max_azm_with_jump_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_max_azm_with_jump_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_scale_diff_unfold_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_scale_diff_unfold_l = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_scale_diff_unfold_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_bins_large_azm_jump_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_bins_large_azm_jump_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_bins_large_azm_jump_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_bin_first_check_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> th_bin_first_check_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_bin_first_check_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".env_winds_timeout_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> env_winds_timeout_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: env_winds_timeout_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".use_sounding_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> use_sounding_l = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: use_sounding_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".replace_rejected_vel_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> replace_rejected_vel_l = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: replace_rejected_vel_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".num_interval_checks_l", &get_value );
  if( ret == 0 )
  {
    radazvd -> num_interval_checks_l = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: num_interval_checks_l unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".disable_radaz_dealiasing", &get_value );
  if( ret == 0 )
  {
    radazvd -> disable_radaz_dealiasing = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: disable_radaz_dealiasing unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".use_sprt_replace_rej", &get_value );
  if( ret == 0)
  {
    radazvd -> use_sprt_replace_rej = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: use_sprt_replace_rej unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".use_2D_dealiasing", &get_value );
  if( ret == 0)
  {
    radazvd -> use_2D_dealiasing = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: use_2D_dealiasing unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_consecutive_rejected_sprt", &get_value );
  if( ret == 0)
  {
    radazvd -> th_consecutive_rejected_sprt = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_consecutive_rejected_sprt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RADAZVD_DEA_NAME, ".th_max_azm_with_jump_sprt", &get_value );
  if( ret == 0)
  {
    radazvd -> th_max_azm_with_jump_sprt = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RADAZVD: th_max_azm_with_jump_sprt unavailable, abort task\n" );
    RPG_abort_task();
  }


#ifdef DEBUG
   LE_send_msg( GL_INFO, "RADAZVD: num_replace_lookahead_l \t\t= %d\n",  radazvd -> num_replace_lookahead_l );
   LE_send_msg( GL_INFO, "RADAZVD: num_replace_lookback_l \t\t= %d\n",  radazvd -> num_replace_lookback_l );
   LE_send_msg( GL_INFO, "RADAZVD: num_lookback_l \t\t= %d\n",  radazvd -> num_lookback_l );
   LE_send_msg( GL_INFO, "RADAZVD: num_lookforward_l \t\t= %d\n",  radazvd -> num_lookforward_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_consecutive_rejected_l \t\t= %d\n",  radazvd -> th_consecutive_rejected_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_maximum_missing_l \t\t= %d\n",  radazvd -> th_maximum_missing_l );
   LE_send_msg( GL_INFO, "RADAZVD: num_reunfold_previous_azm_l \t\t= %d\n",  radazvd -> num_reunfold_previous_azm_l );
   LE_send_msg( GL_INFO, "RADAZVD: num_reunfold_current_azm_l \t\t= %d\n",  radazvd -> num_reunfold_current_azm_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_difference_unfold_l \t\t= %4.1f\n",  radazvd -> th_difference_unfold_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_max_bins_with_jump_l \t\t= %d\n",  radazvd -> th_max_bins_with_jump_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_vel_jump_fraction_radial_l \t\t= %4.2f\n",  radazvd -> th_vel_jump_fraction_radial_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_vel_jump_fraction_azm_l \t\t= %4.2f\n",  radazvd -> th_vel_jump_fraction_azm_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_scale_standard_dev_l \t\t= %4.2f\n",  radazvd -> th_scale_standard_dev_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_max_azm_with_jump_l \t\t= %d\n",  radazvd -> th_max_azm_with_jump_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_scale_diff_unfold_l \t\t= %4.2f\n",  radazvd -> th_scale_diff_unfold_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_bins_large_azm_jump_l \t\t= %d\n",  radazvd -> th_bins_large_azm_jump_l );
   LE_send_msg( GL_INFO, "RADAZVD: th_bin_first_check_l \t\t= %d\n",  radazvd -> th_bin_first_check_l );
   LE_send_msg( GL_INFO, "RADAZVD: env_winds_timeout_l \t\t= %d\n",  radazvd -> env_winds_timeout_l );
   LE_send_msg( GL_INFO, "RADAZVD: use_sounding_l \t\t= %d\n",  radazvd -> use_sounding_l );
   LE_send_msg( GL_INFO, "RADAZVD: replace_rejected_vel_l \t\t= %d\n",  radazvd -> replace_rejected_vel_l );
   LE_send_msg( GL_INFO, "RADAZVD: num_interval_checks_l \t\t= %d\n",  radazvd -> num_interval_checks_l );
   LE_send_msg( GL_INFO, "RADAZVD: disable_radaz_dealiasing \t\t= %d\n",  radazvd -> disable_radaz_dealiasing );
   LE_send_msg( GL_INFO, "RADAZVD: use_sprt_replace_rej \t\t= %d\n",  radazvd -> use_sprt_replace_rej );
   LE_send_msg( GL_INFO, "RADAZVD: th_consecutive_rejected_sprt \t\t= %d\n",  radazvd -> th_consecutive_rejected_sprt );
   LE_send_msg( GL_INFO, "RADAZVD: th_max_azm_with_jump_sprt \t\t= %d\n",  radazvd -> th_max_azm_with_jump_sprt );

   LE_send_msg( GL_INFO, "RADAZVD: use_2D_dealiasing \t\t= %d\n",  radazvd -> use_2D_dealiasing );
#endif

  return 0;
}
