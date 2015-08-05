/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:13:15 $
 * $Id: vil_echo_tops_callback_fx.c,v 1.7 2007/01/30 23:13:15 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <vil_echo_tops.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define vil_echo_tops_callback_fx vil_echo_tops_callback_fx_
#endif

#ifdef LINUX
#define vil_echo_tops_callback_fx vil_echo_tops_callback_fx__
#endif

#endif


int vil_echo_tops_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  vil_echo_tops_t *vil_echo_tops = ( vil_echo_tops_t * )common_block_address;


  /* Get vil/echo tops data elements */

  ret = RPG_ade_get_values( VIL_ECHO_TOPS_DEA_NAME, ".beam_width", &get_value );
  if( ret == 0 )
  {
    vil_echo_tops -> beam_width = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VIL_ECHO_TOPS: beam_width unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( VIL_ECHO_TOPS_DEA_NAME, ".min_refl", &get_value );
  if( ret == 0 )
  {
    vil_echo_tops -> min_refl = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VIL_ECHO_TOPS: min_refl unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( VIL_ECHO_TOPS_DEA_NAME, ".max_vil", &get_value );
  if( ret == 0 )
  {
    vil_echo_tops -> max_vil = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VIL_ECHO_TOPS: max_vil unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "VIL_ECHO_TOPS: beam_width \t\t= %3.1f\n",  vil_echo_tops -> beam_width );
   LE_send_msg( GL_INFO, "VIL_ECHO_TOPS: min_refl \t\t= %5.1f\n",  vil_echo_tops -> min_refl );
   LE_send_msg( GL_INFO, "VIL_ECHO_TOPS: max_vil \t\t= %d\n",  vil_echo_tops -> max_vil );
#endif

  return 0;
}
