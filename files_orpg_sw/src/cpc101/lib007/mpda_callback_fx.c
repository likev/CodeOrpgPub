/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/05/03 14:32:16 $
 * $Id: mpda_callback_fx.c,v 1.3 2011/05/03 14:32:16 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/***************************************************************************/

/*** System Include Files ***/
#include <alg_adapt.h>
#include <mpda_parameters.h>

/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define mpda_callback_fx mpda_callback_fx_
#endif

#ifdef LINUX
#define mpda_callback_fx mpda_callback_fx__
#endif

#endif


int mpda_callback_fx( void *struct_address )
{
  double get_value = 0.0;  /* used to get data element's value */
  int ret = -1;            /* return status */
  mpda_adapt_params_t *mpda = ( mpda_adapt_params_t *)struct_address;

  /* Get mpda data elements. */

  ret = RPG_ade_get_values( MPDA_DEA_NAME, ".gui_mpda_tover", &get_value );
  if( ret == 0 )
  {
    mpda -> gui_mpda_tover = ( float )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MPDA: gui_mpda_tover unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MPDA_DEA_NAME, ".gui_min_trip_fix", &get_value );
  if( ret == 0 )
  {
    mpda -> gui_min_trip_fix = ( short )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MPDA: gui_min_trip_fix unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MPDA_DEA_NAME, ".gui_max_trip_fix", &get_value );
  if( ret == 0 )
  {
    mpda -> gui_max_trip_fix = ( short )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MPDA: gui_max_trip_fix unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MPDA_DEA_NAME, ".gui_th_overlap_size", &get_value );
  if( ret == 0 )
  {
    mpda -> gui_th_overlap_size = ( short )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MPDA: gui_th_overlap_size unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( MPDA_DEA_NAME, ".gui_th_overlap_relax", &get_value );
  if( ret == 0 )
  {
    mpda -> gui_th_overlap_relax = ( short )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "MPDA: gui_th_overlap_relax unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "MPDA: gui_mpda_tover \t\t= %5.2f\n", mpda -> gui_mpda_tover );
   LE_send_msg( GL_INFO, "MPDA: gui_min_trip_fix \t\t= %d\n", mpda -> gui_min_trip_fix );
   LE_send_msg( GL_INFO, "MPDA: gui_max_trip_fix \t\t= %d\n", mpda -> gui_max_trip_fix );
   LE_send_msg( GL_INFO, "MPDA: gui_th_overlap_size \t\t= %d\n", mpda -> gui_th_overlap_size );
   LE_send_msg( GL_INFO, "MPDA: gui_th_overlap_relax \t\t= %d\n", mpda -> gui_th_overlap_relax );
#endif

  return 0;
}
