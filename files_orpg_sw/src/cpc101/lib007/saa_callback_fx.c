/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:13:02 $
 * $Id: saa_callback_fx.c,v 1.2 2007/01/30 23:13:02 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <saa_params.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define saa_callback_fx saa_callback_fx_
#endif

#ifdef LINUX
#define saa_callback_fx saa_callback_fx__
#endif

#endif


int saa_callback_fx( void *struct_address )
{
  double get_value = 0.0;  /* used to get data element's value */
  int ret = -1;            /* return status */
  saa_adapt_params_t *saa = (saa_adapt_params_t *)struct_address;


  /* Get saa data elements. */

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_cf_ZS_mult",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_cf_ZS_mult = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_cf_ZS_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_cf_ZS_power",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_cf_ZS_power = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_cf_ZS_power unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_sw_ratio",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_sw_ratio = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_sw_ratio unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_thr_mn_hgt_corr",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_thr_mn_hgt_corr = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_thr_mn_hgt_corr unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_cf1_rng_hgt",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_cf1_rng_hgt = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_cf1_rng_hgt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_cf2_rng_hgt",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_cf2_rng_hgt = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_cf2_rng_hgt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_cf3_rng_hgt",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_cf3_rng_hgt = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_cf3_rng_hgt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_use_RCA_flag",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_use_RCA_flag = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_use_RCA_flag unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_thr_lo_dBZ",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_thr_lo_dBZ = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_thr_lo_dBZ unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_thr_hi_dBZ",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_thr_hi_dBZ = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_thr_hi_dBZ unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_thr_time_span",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_thr_time_span = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_thr_time_span unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_thr_mn_time",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_thr_mn_time = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_thr_mn_time unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SAA_DEA_NAME,
                             ".g_rhc_base_elev",
                             &get_value );
  if( ret == 0 )
  {
    saa -> g_rhc_base_elev = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SAA: g_rhc_base_elev unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "SAA: g_cf_ZS_mult \t\t= %6.1f\n", saa -> g_cf_ZS_mult );
   LE_send_msg( GL_INFO, "SAA: g_cf_ZS_power \t\t= %4.2f\n", saa -> g_cf_ZS_power );
   LE_send_msg( GL_INFO, "SAA: g_sw_ratio \t\t= %5.1f\n", saa -> g_sw_ratio );
   LE_send_msg( GL_INFO, "SAA: g_thr_mn_hgt_corr \t\t= %5.2f\n", saa -> g_thr_mn_hgt_corr );
   LE_send_msg( GL_INFO, "SAA: g_cf1_rng_hgt \t\t= %8.5f\n", saa -> g_cf1_rng_hgt );
   LE_send_msg( GL_INFO, "SAA: g_cf2_rng_hgt \t\t= %7.4f\n", saa -> g_cf2_rng_hgt );
   LE_send_msg( GL_INFO, "SAA: g_cf3_rng_hgt \t\t= %7.4f\n", saa -> g_cf3_rng_hgt );
   LE_send_msg( GL_INFO, "SAA: g_use_RCA_flag \t\t= %d\n", saa -> g_use_RCA_flag );
   LE_send_msg( GL_INFO, "SAA: g_thr_lo_dBZ \t\t= %5.1f\n", saa -> g_thr_lo_dBZ );
   LE_send_msg( GL_INFO, "SAA: g_thr_hi_dBZ \t\t= %4.1f\n", saa -> g_thr_hi_dBZ );
   LE_send_msg( GL_INFO, "SAA: g_thr_time_span \t\t= %d\n", saa -> g_thr_time_span );
   LE_send_msg( GL_INFO, "SAA: g_thr_mn_time \t\t= %d\n", saa -> g_thr_mn_time );
   LE_send_msg( GL_INFO, "SAA: g_rhc_base_elev \t\t= %3.1f\n", saa -> g_rhc_base_elev );
#endif

  return 0;
}
