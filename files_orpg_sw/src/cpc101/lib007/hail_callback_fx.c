/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:42 $
 * $Id: hail_callback_fx.c,v 1.2 2007/01/30 23:12:42 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <hail_algorithm.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define hail_callback_fx hail_callback_fx_
#endif

#ifdef LINUX
#define hail_callback_fx hail_callback_fx__
#endif

#endif


int hail_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  hail_algorithm_t *hail = ( hail_algorithm_t * )common_block_address;


  /* Get hail data elements */

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hke_ref_wgt_low", &get_value );
  if( ret == 0 )
  {
    hail -> hke_ref_wgt_low = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hke_ref_wgt_low unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hke_ref_wgt_high", &get_value );
  if( ret == 0 )
  {
    hail -> hke_ref_wgt_high = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hke_ref_wgt_high unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_min_ref", &get_value );
  if( ret == 0 )
  {
    hail -> poh_min_ref = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_min_ref unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hke_coef1", &get_value );
  if( ret == 0 )
  {
    hail -> hke_coef1 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hke_coef1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hke_coef2", &get_value );
  if( ret == 0 )
  {
    hail -> hke_coef2 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hke_coef2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hke_coef3", &get_value );
  if( ret == 0 )
  {
    hail -> hke_coef3 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hke_coef3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".posh_coef", &get_value );
  if( ret == 0 )
  {
    hail -> posh_coef = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: posh_coef unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".posh_offset", &get_value );
  if( ret == 0 )
  {
    hail -> posh_offset = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: posh_offset unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".max_hail_range", &get_value );
  if( ret == 0 )
  {
    hail -> max_hail_range = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: max_hail_range unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".shi_hail_size_coef", &get_value );
  if( ret == 0 )
  {
    hail -> shi_hail_size_coef = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: shi_hail_size_coef unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".shi_hail_size_exp", &get_value );
  if( ret == 0 )
  {
    hail -> shi_hail_size_exp = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: shi_hail_size_exp unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".warn_thr_sel_mod_coef", &get_value );
  if( ret == 0 )
  {
    hail -> warn_thr_sel_mod_coef = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: warn_thr_sel_mod_coef unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".warn_thr_sel_mod_off", &get_value );
  if( ret == 0 )
  {
    hail -> warn_thr_sel_mod_off = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: warn_thr_sel_mod_off unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff1", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff1 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff2", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff2 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff3", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff3 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff4", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff4 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff4 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff5", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff5 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff5 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff6", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff6 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff6 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff7", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff7 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff7 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff8", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff8 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff8 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff9", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff9 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff9 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".poh_height_diff10", &get_value );
  if( ret == 0 )
  {
    hail -> poh_height_diff10 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: poh_height_diff10 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".rcm_probable_hail", &get_value );
  if( ret == 0 )
  {
    hail -> rcm_probable_hail = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: rcm_probable_hail unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".rcm_positive_hail", &get_value );
  if( ret == 0 )
  {
    hail -> rcm_positive_hail = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: rcm_positive_hail unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".height_0", &get_value );
  if( ret == 0 )
  {
    hail -> height_0 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: height_0 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".height_minus_20", &get_value );
  if( ret == 0 )
  {
    hail -> height_minus_20 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: height_minus_20 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hail_date_yy", &get_value );
  if( ret == 0 )
  {
    hail -> hail_date_yy = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hail_date_yy unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hail_date_mm", &get_value );
  if( ret == 0 )
  {
    hail -> hail_date_mm = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hail_date_mm unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hail_date_dd", &get_value );
  if( ret == 0 )
  {
    hail -> hail_date_dd = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hail_date_dd unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hail_time_hr", &get_value );
  if( ret == 0 )
  {
    hail -> hail_time_hr = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hail_time_hr unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hail_time_min", &get_value );
  if( ret == 0 )
  {
    hail -> hail_time_min = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hail_time_min unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HAIL_DEA_NAME, ".hail_time_sec", &get_value );
  if( ret == 0 )
  {
    hail -> hail_time_sec = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HAIL: hail_time_sec unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "HAIL: hke_ref_wgt_low \t\t= %d\n",  hail -> hke_ref_wgt_low );
   LE_send_msg( GL_INFO, "HAIL: hke_ref_wgt_high \t\t= %d\n",  hail -> hke_ref_wgt_high );
   LE_send_msg( GL_INFO, "HAIL: poh_min_ref \t\t= %d\n",  hail -> poh_min_ref );
   LE_send_msg( GL_INFO, "HAIL: hke_coef1 \t\t= %12.10f\n",  hail -> hke_coef1 );
   LE_send_msg( GL_INFO, "HAIL: hke_coef2 \t\t= %5.3f\n",  hail -> hke_coef2 );
   LE_send_msg( GL_INFO, "HAIL: hke_coef3 \t\t= %5.1f\n",  hail -> hke_coef3 );
   LE_send_msg( GL_INFO, "HAIL: posh_coef \t\t= %5.1f\n",  hail -> posh_coef );
   LE_send_msg( GL_INFO, "HAIL: posh_offset \t\t= %d\n",  hail -> posh_offset );
   LE_send_msg( GL_INFO, "HAIL: max_hail_range \t\t= %d\n",  hail -> max_hail_range );
   LE_send_msg( GL_INFO, "HAIL: shi_hail_size_coef \t\t= %4.2f\n",  hail -> shi_hail_size_coef );
   LE_send_msg( GL_INFO, "HAIL: shi_hail_size_exp \t\t= %3.1f\n",  hail -> shi_hail_size_exp );
   LE_send_msg( GL_INFO, "HAIL: warn_thr_sel_mod_coef \t\t= %5.1f\n",  hail -> warn_thr_sel_mod_coef );
   LE_send_msg( GL_INFO, "HAIL: warn_thr_sel_mod_off \t\t= %6.1f\n",  hail -> warn_thr_sel_mod_off );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff1 \t\t= %4.1f\n",  hail -> poh_height_diff1 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff2 \t\t= %4.1f\n",  hail -> poh_height_diff2 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff3 \t\t= %4.1f\n",  hail -> poh_height_diff3 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff4 \t\t= %4.1f\n",  hail -> poh_height_diff4 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff5 \t\t= %4.1f\n",  hail -> poh_height_diff5 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff6 \t\t= %4.1f\n",  hail -> poh_height_diff6 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff7 \t\t= %4.1f\n",  hail -> poh_height_diff7 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff8 \t\t= %4.1f\n",  hail -> poh_height_diff8 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff9 \t\t= %4.1f\n",  hail -> poh_height_diff9 );
   LE_send_msg( GL_INFO, "HAIL: poh_height_diff10 \t\t= %4.1f\n",  hail -> poh_height_diff10 );
   LE_send_msg( GL_INFO, "HAIL: rcm_probable_hail \t\t= %d\n",  hail -> rcm_probable_hail );
   LE_send_msg( GL_INFO, "HAIL: rcm_positive_hail \t\t= %d\n",  hail -> rcm_positive_hail );
   LE_send_msg( GL_INFO, "HAIL: height_0 \t\t= %4.1f\n",  hail -> height_0 );
   LE_send_msg( GL_INFO, "HAIL: height_minus_20 \t\t= %4.1f\n",  hail -> height_minus_20 );
   LE_send_msg( GL_INFO, "HAIL: hail_date[%d] \t\t= %d\n", hail -> hail_date_yy );
   LE_send_msg( GL_INFO, "HAIL: hail_date[%d] \t\t= %d\n", hail -> hail_date_mm );
   LE_send_msg( GL_INFO, "HAIL: hail_date[%d] \t\t= %d\n", hail -> hail_date_dd );
   LE_send_msg( GL_INFO, "HAIL: hail_time[%d] \t\t= %d\n", hail -> hail_time_hr );
   LE_send_msg( GL_INFO, "HAIL: hail_time[%d] \t\t= %d\n", hail -> hail_time_min );
   LE_send_msg( GL_INFO, "HAIL: hail_time[%d] \t\t= %d\n", hail -> hail_time_sec );
#endif

  return 0;
}
