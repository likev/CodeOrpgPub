/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:13:07 $
 * $Id: storm_cell_track_callback_fx.c,v 1.7 2007/01/30 23:13:07 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <storm_cell_track.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define storm_cell_track_callback_fx storm_cell_track_callback_fx_
#endif

#ifdef LINUX
#define storm_cell_track_callback_fx storm_cell_track_callback_fx__
#endif

#endif


int storm_cell_track_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  storm_cell_track_t *storm_cell_track = ( storm_cell_track_t * )common_block_address;


  /* Get storm cell track data elements */

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".num_past_vols", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> num_past_vols = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: num_past_vols unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".num_intvls", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> num_intvls = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: num_intvls unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".forecast_intvl", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> forecast_intvl = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: forecast_intvl unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".allow_err", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> allow_err = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: allow_err unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".err_intvl", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> err_intvl = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: err_intvl unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".default_dir", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> default_dir = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: default_dir unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".max_time", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> max_time = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: max_time unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".default_spd", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> default_spd = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: default_spd unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".correlation_spd", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> correlation_spd = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: correlation_spd unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_TRACK_DEA_NAME, ".minimum_spd", &get_value );
  if( ret == 0 )
  {
    storm_cell_track -> minimum_spd = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_TRACK: minimum_spd unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: num_past_vols \t\t= %d\n",  storm_cell_track -> num_past_vols );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: num_intvls \t\t= %d\n",  storm_cell_track -> num_intvls );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: forecast_intvl \t\t= %d\n",  storm_cell_track -> forecast_intvl );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: allow_err \t\t= %d\n",  storm_cell_track -> allow_err );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: err_intvl \t\t= %d\n",  storm_cell_track -> err_intvl );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: default_dir \t\t= %d\n",  storm_cell_track -> default_dir );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: max_time \t\t= %d\n",  storm_cell_track -> max_time );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: default_spd \t\t= %4.1f\n",  storm_cell_track -> default_spd );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: correlation_spd \t\t= %4.1f\n",  storm_cell_track -> correlation_spd );
   LE_send_msg( GL_INFO, "STORM_CELL_TRACK: minimum_spd \t\t= %4.1f\n",  storm_cell_track -> minimum_spd );
#endif

  return 0;
}
