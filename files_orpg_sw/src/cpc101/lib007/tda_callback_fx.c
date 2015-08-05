/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:13:11 $
 * $Id: tda_callback_fx.c,v 1.7 2007/01/30 23:13:11 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <tda.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define tda_callback_fx tda_callback_fx_
#endif

#ifdef LINUX
#define tda_callback_fx tda_callback_fx__
#endif

#endif


int tda_callback_fx( void *common_block_address )
{
  double get_value = 0.0;  /* used to get data element's value */
  int ret = -1;            /* return status */
  tda_t *tda = ( tda_t * )common_block_address;


  /* Get tornado detection algorithm data elements */

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".min_dbz_thresh", &get_value );
  if( ret == 0 )
  {
    tda -> min_dbz_thresh = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: min_dbz_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".vector_vel_diff", &get_value );
  if( ret == 0 )
  {
    tda -> vector_vel_diff = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: vector_vel_diff unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_pv_range", &get_value );
  if( ret == 0 )
  {
    tda -> max_pv_range = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_pv_range unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_pv_height", &get_value );
  if( ret == 0 )
  {
    tda -> max_pv_height = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_pv_height unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_pv_num", &get_value );
  if( ret == 0 )
  {
    tda -> max_pv_num = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_pv_num unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".diff_vel1", &get_value );
  if( ret == 0 )
  {
    tda -> diff_vel1 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: diff_vel1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".diff_vel2", &get_value );
  if( ret == 0 )
  {
    tda -> diff_vel2 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: diff_vel2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".diff_vel3", &get_value );
  if( ret == 0 )
  {
    tda -> diff_vel3 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: diff_vel3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".diff_vel4", &get_value );
  if( ret == 0 )
  {
    tda -> diff_vel4 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: diff_vel4 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".diff_vel5", &get_value );
  if( ret == 0 )
  {
    tda -> diff_vel5 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: diff_vel5 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".diff_vel6", &get_value );
  if( ret == 0 )
  {
    tda -> diff_vel6 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: diff_vel6 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".min_vectors_2d", &get_value );
  if( ret == 0 )
  {
    tda -> min_vectors_2d = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: min_vectors_2d unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".vector_rad_dist_2d", &get_value );
  if( ret == 0 )
  {
    tda -> vector_rad_dist_2d = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: vector_rad_dist_2d unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".vector_azi_dist_2d", &get_value );
  if( ret == 0 )
  {
    tda -> vector_azi_dist_2d = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: vector_azi_dist_2d unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_ratio_2d", &get_value );
  if( ret == 0 )
  {
    tda -> max_ratio_2d = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_ratio_2d unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".circ_radius1", &get_value );
  if( ret == 0 )
  {
    tda -> circ_radius1 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: circ_radius1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".circ_radius2", &get_value );
  if( ret == 0 )
  {
    tda -> circ_radius2 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: circ_radius2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".circ_radius_range", &get_value );
  if( ret == 0 )
  {
    tda -> circ_radius_range = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: circ_radius_range unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_2d_features", &get_value );
  if( ret == 0 )
  {
    tda -> max_2d_features = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_2d_features unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".min_2d_features", &get_value );
  if( ret == 0 )
  {
    tda -> min_2d_features = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: min_2d_features unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".min_depth_3d", &get_value );
  if( ret == 0 )
  {
    tda -> min_depth_3d = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: min_depth_3d unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".min_vel_3d", &get_value );
  if( ret == 0 )
  {
    tda -> min_vel_3d = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: min_vel_3d unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".min_tvs_vel", &get_value );
  if( ret == 0 )
  {
    tda -> min_tvs_vel = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: min_tvs_vel unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_3d_features", &get_value );
  if( ret == 0 )
  {
    tda -> max_3d_features = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_3d_features unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_tvs_features", &get_value );
  if( ret == 0 )
  {
    tda -> max_tvs_features = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_tvs_features unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_etvs_features", &get_value );
  if( ret == 0 )
  {
    tda -> max_etvs_features = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_etvs_features unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".min_tvs_height", &get_value );
  if( ret == 0 )
  {
    tda -> min_tvs_height = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: min_tvs_height unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".min_tvs_elev", &get_value );
  if( ret == 0 )
  {
    tda -> min_tvs_elev = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: min_tvs_elev unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".avg_vel_height", &get_value );
  if( ret == 0 )
  {
    tda -> avg_vel_height = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: avg_vel_height unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( TDA_DEA_NAME, ".max_storm_dist", &get_value );
  if( ret == 0 )
  {
    tda -> max_storm_dist = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "TDA: max_storm_dist unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "TDA: min_dbz_thresh \t\t= %d\n",  tda -> min_dbz_thresh );
   LE_send_msg( GL_INFO, "TDA: vector_vel_diff \t\t= %d\n",  tda -> vector_vel_diff );
   LE_send_msg( GL_INFO, "TDA: max_pv_range \t\t= %d\n",  tda -> max_pv_range );
   LE_send_msg( GL_INFO, "TDA: max_pv_height \t\t= %4.1f\n",  tda -> max_pv_height );
   LE_send_msg( GL_INFO, "TDA: diff_vel1 \t\t= %d\n",  tda -> diff_vel1 );
   LE_send_msg( GL_INFO, "TDA: diff_vel2 \t\t= %d\n",  tda -> diff_vel2 );
   LE_send_msg( GL_INFO, "TDA: diff_vel3 \t\t= %d\n",  tda -> diff_vel3 );
   LE_send_msg( GL_INFO, "TDA: diff_vel4 \t\t= %d\n",  tda -> diff_vel4 );
   LE_send_msg( GL_INFO, "TDA: diff_vel5 \t\t= %d\n",  tda -> diff_vel5 );
   LE_send_msg( GL_INFO, "TDA: diff_vel6 \t\t= %d\n",  tda -> diff_vel6 );
   LE_send_msg( GL_INFO, "TDA: min_vectors_2d \t\t= %d\n",  tda -> min_vectors_2d );
   LE_send_msg( GL_INFO, "TDA: vector_rad_dist_2d \t\t= %3.1f\n",  tda -> vector_rad_dist_2d );
   LE_send_msg( GL_INFO, "TDA: vector_azi_dist_2d \t\t= %3.1f\n",  tda -> vector_azi_dist_2d );
   LE_send_msg( GL_INFO, "TDA: max_ratio_2d \t\t= %4.1f\n",  tda -> max_ratio_2d );
   LE_send_msg( GL_INFO, "TDA: circ_radius1 \t\t= %4.1f\n",  tda -> circ_radius1 );
   LE_send_msg( GL_INFO, "TDA: circ_radius2 \t\t= %4.1f\n",  tda -> circ_radius2 );
   LE_send_msg( GL_INFO, "TDA: circ_radius_range \t\t= %d\n",  tda -> circ_radius_range );
   LE_send_msg( GL_INFO, "TDA: max_2d_features \t\t= %d\n",  tda -> max_2d_features );
   LE_send_msg( GL_INFO, "TDA: min_2d_features \t\t= %d\n",  tda -> min_2d_features );
   LE_send_msg( GL_INFO, "TDA: min_depth_3d \t\t= %3.1f\n",  tda -> min_depth_3d );
   LE_send_msg( GL_INFO, "TDA: min_vel_3d \t\t= %d\n",  tda -> min_vel_3d );
   LE_send_msg( GL_INFO, "TDA: min_tvs_vel \t\t= %d\n",  tda -> min_tvs_vel );
   LE_send_msg( GL_INFO, "TDA: max_3d_features \t\t= %d\n",  tda -> max_3d_features );
   LE_send_msg( GL_INFO, "TDA: max_tvs_features \t\t= %d\n",  tda -> max_tvs_features );
   LE_send_msg( GL_INFO, "TDA: max_etvs_features \t\t= %d\n",  tda -> max_etvs_features );
   LE_send_msg( GL_INFO, "TDA: min_tvs_height \t\t= %4.1f\n",  tda -> min_tvs_height );
   LE_send_msg( GL_INFO, "TDA: min_tvs_elev \t\t= %4.1f\n",  tda -> min_tvs_elev );
   LE_send_msg( GL_INFO, "TDA: avg_vel_height \t\t= %4.1f\n",  tda -> avg_vel_height );
#endif

  return 0;
}
