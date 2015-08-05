/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:44 $
 * $Id: hydromet_adj_callback_fx.c,v 1.8 2007/01/30 23:12:44 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <hydromet_adj.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define hydromet_adj_callback_fx hydromet_adj_callback_fx_
#endif

#ifdef LINUX
#define hydromet_adj_callback_fx hydromet_adj_callback_fx__
#endif

#endif


int hydromet_adj_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  hydromet_adj_t *hydromet_adj = ( hydromet_adj_t * )common_block_address;


  /* Get hydromet accumulation data elements*/

  ret = RPG_ade_get_values( HYDROMET_ADJ_DEA_NAME,
                                 ".time_bias",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_adj -> time_bias = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ADJ: time_bias unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ADJ_DEA_NAME,
                                 ".num_grpairs",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_adj -> num_grpairs = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ADJ: num_grpairs unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ADJ_DEA_NAME,
                                 ".reset_bias",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_adj -> reset_bias = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ADJ: reset_bias unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ADJ_DEA_NAME,
                                 ".longst_lag",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_adj -> longst_lag = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ADJ: longst_lag unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ADJ_DEA_NAME,
                                 ".bias_flag",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_adj -> bias_flag = ( flogical )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ADJ: bias_flag unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "HYDROMET_ADJ: time_bias \t\t= %d\n", hydromet_adj -> time_bias );
   LE_send_msg( GL_INFO, "HYDROMET_ADJ: num_grpairs \t\t= %d\n", hydromet_adj -> num_grpairs );
   LE_send_msg( GL_INFO, "HYDROMET_ADJ: reset_bias \t\t= %3.1f\n", hydromet_adj -> reset_bias );
   LE_send_msg( GL_INFO, "HYDROMET_ADJ: longst_lag \t\t= %d\n", hydromet_adj -> longst_lag );
   LE_send_msg( GL_INFO, "HYDROMET_ADJ: bias_flag \t\t= %d\n", hydromet_adj -> bias_flag );
#endif

  return 0;
}
