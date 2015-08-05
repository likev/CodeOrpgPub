/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:49 $
 * $Id: layer_reflectivity_callback_fx.c,v 1.8 2007/01/30 23:12:49 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <layer_reflectivity.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define layer_dbz_callback_fx layer_dbz_callback_fx_
#endif

#ifdef LINUX
#define layer_dbz_callback_fx layer_dbz_callback_fx__
#endif

#endif


int layer_dbz_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  layer_dbz_t *layer_dbz = ( layer_dbz_t * )common_block_address;


  /* Get layer dbz data elements */

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".min_cltr_dbz", &get_value );
  if( ret == 0 )
  {
    layer_dbz -> min_cltr_dbz = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: min_cltr_dbz unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".altitude_omit", &get_value );
  if( ret == 0 )
  {
    layer_dbz -> altitude_omit = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: altitude_omit unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".altitude_accept", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> altitude_accept = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: altitude_accept unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".distance_omit", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> distance_omit = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: distance_omit unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".distance_accept", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> distance_accept = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: distance_accept unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".distance_reject", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> distance_reject = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: distance_reject unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".elevation_accept", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> elevation_accept = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: elevation_accept unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".elevation_reject", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> elevation_reject = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: elevation_reject unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".velocity_reject", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> velocity_reject = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: velocity_reject unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".width_reject", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> width_reject = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: width_reject unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".velocity_accept", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> velocity_accept = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: velocity_accept unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".width_accept", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> width_accept = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: width_accept unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".cbd_phase", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> cbd_phase = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: cbd_phase unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".cbd_bins", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> cbd_bins = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: cbd_bins unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".cbd_dbz", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> cbd_dbz = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: cbd_dbz unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".median_phase", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> median_phase = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: median_phase unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".median_bins", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> median_bins = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: median_bins unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".median_range", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> median_range = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: median_range unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( LAYER_REF_DEA_NAME, ".median_dbz", &get_value
);
  if( ret == 0 )
  {
    layer_dbz -> median_dbz = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "LAYER_REF: median_dbz unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "LAYER_REF: min_cltr_dbz \t\t= %4.1f\n",  layer_dbz -> min_cltr_dbz );
   LE_send_msg( GL_INFO, "LAYER_REF: altitude_omit \t\t= %d\n",  layer_dbz -> altitude_omit );
   LE_send_msg( GL_INFO, "LAYER_REF: altitude_accept \t\t= %d\n",  layer_dbz -> altitude_accept );
   LE_send_msg( GL_INFO, "LAYER_REF: distance_omit \t\t= %d\n",  layer_dbz -> distance_omit );
   LE_send_msg( GL_INFO, "LAYER_REF: distance_accept \t\t= %d\n",  layer_dbz -> distance_accept );
   LE_send_msg( GL_INFO, "LAYER_REF: distance_reject \t\t= %d\n",  layer_dbz -> distance_reject );
   LE_send_msg( GL_INFO, "LAYER_REF: elevation_accept \t\t= %3.1f\n",  layer_dbz -> elevation_accept );
   LE_send_msg( GL_INFO, "LAYER_REF: elevation_reject \t\t= %4.1f\n",  layer_dbz -> elevation_reject );
   LE_send_msg( GL_INFO, "LAYER_REF: velocity_reject \t\t= %3.1f\n",  layer_dbz -> velocity_reject );
   LE_send_msg( GL_INFO, "LAYER_REF: width_reject \t\t= %3.1f\n",  layer_dbz -> width_reject );
   LE_send_msg( GL_INFO, "LAYER_REF: velocity_accept \t\t= %3.1f\n",  layer_dbz -> velocity_accept );
   LE_send_msg( GL_INFO, "LAYER_REF: width_accept \t\t= %3.1f\n",  layer_dbz -> width_accept );
   LE_send_msg( GL_INFO, "LAYER_REF: cbd_phase \t\t= %d\n",  layer_dbz -> cbd_phase );
   LE_send_msg( GL_INFO, "LAYER_REF: cbd_bins \t\t= %d\n",  layer_dbz -> cbd_bins );
   LE_send_msg( GL_INFO, "LAYER_REF: cbd_dbz \t\t= %4.1f\n",  layer_dbz -> cbd_dbz );
   LE_send_msg( GL_INFO, "LAYER_REF: median_phase \t\t= %d\n",  layer_dbz -> median_phase );
   LE_send_msg( GL_INFO, "LAYER_REF: median_bins \t\t= %d\n",  layer_dbz -> median_bins );
   LE_send_msg( GL_INFO, "LAYER_REF: median_range \t\t= %d\n",  layer_dbz -> median_range );
   LE_send_msg( GL_INFO, "LAYER_REF: median_dbz \t\t= %5.1f\n",  layer_dbz -> median_dbz );
#endif

  return 0;
}
