/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:50 $
 * $Id: mda_callback_fx.c,v 1.5 2007/01/30 23:12:50 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <mda_adapt.h>

/*** Local Include Files ***/

#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define mda_callback_fx mda_callback_fx_
#endif

#ifdef LINUX
#define mda_callback_fx mda_callback_fx__
#endif

#endif

int mda_callback_fx( void *struct_address )
{
  double get_value = 0.0;  /* used to get data element's value */
  int ret = -1;            /* return status */
  mda_adapt_t *mda = (mda_adapt_t *)struct_address;


  /* Get mda data elements. */

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".min_refl",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> min_refl = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: min_refl unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".overlap_filter_on",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> overlap_filter_on = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: overlap_filter_on unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".min_filter_rank",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> min_filter_rank = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: min_filter_rank unavailable, abort task\n" );
    RPG_abort_task();
  }
  
  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".beam_width",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> beam_width = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: beam_width unavailable, abort task\n" );
    RPG_abort_task();
  }
  
  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_min_nsv",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_min_nsv = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_min_nsv unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_vs_rng_1",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_vs_rng_1 = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_vs_rng_1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_vs_rng_2",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_vs_rng_2 = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_vs_rng_2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_max_vect_len",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_max_vect_len = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_max_vect_len unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_max_core_len",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_max_core_len = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_max_core_len unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_max_ratio",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_max_ratio = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_max_ratio unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_min_ratio",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_min_ratio = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_min_ratio unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_min_radim",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_min_radim = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_min_radim unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_max_dia",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_max_dia = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_max_dia unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_2d_dist",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_2d_dist = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_2d_dist unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_min_rng",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_min_rng = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_min_rng unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".v_d_th_lo",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> v_d_th_lo = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: v_d_th_lo unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".v_d_th_hi",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> v_d_th_hi = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: v_d_th_hi unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".conv_max_lookahd",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> conv_max_lookahd = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: conv_max_lookahd unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".conv_rng_dist",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> conv_rng_dist = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: conv_rng_dist unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_conv_buff",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_conv_buff = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_conv_buff unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_ll_conv_ht",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_ll_conv_ht = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_ll_conv_ht unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_ml_conv_ht1",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_ml_conv_ht1 = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_ml_conv_ht1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_ml_conv_ht2",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_ml_conv_ht2 = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_ml_conv_ht2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".mda_no_shallow",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> mda_no_shallow = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: mda_no_shallow unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_min_rank_shal",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_min_rank_shal = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_min_rank_shal unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_min_depth_shal",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_min_depth_shal = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_min_depth_shal unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MDA_DEA_NAME,
                                 ".meso_max_top_shal",
                                 &get_value );
  if( ret == 0 )
  {
    mda -> meso_max_top_shal = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MDA: meso_max_top_shal unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "MDA: min_refl \t\t= %d\n", mda -> min_refl );
   LE_send_msg( GL_INFO, "MDA: overlap_filter_on \t\t= %d\n", mda -> overlap_filter_on );
   LE_send_msg( GL_INFO, "MDA: min_filter_rank \t\t= %d\n", mda -> min_filter_rank );
   LE_send_msg( GL_INFO, "MDA: beam_width \t\t= %f\n", mda -> beam_width );
   LE_send_msg( GL_INFO, "MDA: meso_min_nsv \t\t= %d\n", mda -> meso_min_nsv );
   LE_send_msg( GL_INFO, "MDA: meso_vs_rng_1 \t\t= %d\n", mda -> meso_vs_rng_1 );
   LE_send_msg( GL_INFO, "MDA: meso_vs_rng_2 \t\t= %d\n", mda -> meso_vs_rng_2 );
   LE_send_msg( GL_INFO, "MDA: meso_max_vect_len \t\t= %d\n", mda -> meso_max_vect_len );
   LE_send_msg( GL_INFO, "MDA: meso_max_core_len \t\t= %d\n", mda -> meso_max_core_len );
   LE_send_msg( GL_INFO, "MDA: meso_max_ratio \t\t= %f\n", mda -> meso_max_ratio );
   LE_send_msg( GL_INFO, "MDA: meso_min_ratio \t\t= %f\n", mda -> meso_min_ratio );
   LE_send_msg( GL_INFO, "MDA: meso_min_radim \t\t= %f\n", mda -> meso_min_radim );
   LE_send_msg( GL_INFO, "MDA: meso_max_dia \t\t= %d\n", mda -> meso_max_dia );
   LE_send_msg( GL_INFO, "MDA: meso_2d_dist \t\t= %f\n", mda -> meso_2d_dist );
   LE_send_msg( GL_INFO, "MDA: meso_min_rng \t\t= %d\n", mda -> meso_min_rng );
   LE_send_msg( GL_INFO, "MDA: v_d_th_lo \t\t= %d\n", mda -> v_d_th_lo );
   LE_send_msg( GL_INFO, "MDA: v_d_th_hi \t\t= %d\n", mda -> v_d_th_hi );
   LE_send_msg( GL_INFO, "MDA: conv_max_lookahd \t\t= %d\n", mda -> conv_max_lookahd );
   LE_send_msg( GL_INFO, "MDA: conv_rng_dist \t\t= %f\n", mda -> conv_rng_dist );
   LE_send_msg( GL_INFO, "MDA: meso_conv_buff \t\t= %d\n", mda -> meso_conv_buff );
   LE_send_msg( GL_INFO, "MDA: meso_ll_conv_ht \t\t= %d\n", mda -> meso_ll_conv_ht );
   LE_send_msg( GL_INFO, "MDA: meso_ml_conv_ht1 \t\t= %d\n", mda -> meso_ml_conv_ht1 );
   LE_send_msg( GL_INFO, "MDA: meso_ml_conv_ht2 \t\t= %d\n", mda -> meso_ml_conv_ht2 );
   LE_send_msg( GL_INFO, "MDA: mda_no_shallow \t\t= %d\n", mda -> mda_no_shallow );
   LE_send_msg( GL_INFO, "MDA: meso_min_rank_shal \t\t= %d\n", mda -> meso_min_rank_shal );
   LE_send_msg( GL_INFO, "MDA: meso_min_depth_shal \t\t= %f\n", mda -> meso_min_depth_shal );
   LE_send_msg( GL_INFO, "MDA: meso_max_top_shal \t\t= %d\n", mda -> meso_max_top_shal );
#endif

  return 0;
}
