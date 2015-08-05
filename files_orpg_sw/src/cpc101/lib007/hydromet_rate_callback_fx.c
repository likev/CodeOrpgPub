/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:47 $
 * $Id: hydromet_rate_callback_fx.c,v 1.9 2007/01/30 23:12:47 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <hydromet_rate.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define hydromet_rate_callback_fx hydromet_rate_callback_fx_
#endif

#ifdef LINUX
#define hydromet_rate_callback_fx hydromet_rate_callback_fx__
#endif

#endif


int hydromet_rate_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  hydromet_rate_t *hydromet_rate = ( hydromet_rate_t * )common_block_address;


  /* Get hydromet rate data elements */

  ret = RPG_ade_get_values( HYDROMET_RATE_DEA_NAME,
                                 ".range_cutoff",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_rate -> range_cutoff = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_RATE: range_cutoff unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_RATE_DEA_NAME,
                                 ".range_coef1",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_rate -> range_coef1 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_RATE: range_coef1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_RATE_DEA_NAME,
                                 ".range_coef2",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_rate -> range_coef2 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_RATE: range_coef2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_RATE_DEA_NAME,
                                 ".range_coef3",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_rate -> range_coef3 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_RATE: range_coef3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_RATE_DEA_NAME,
                                 ".min_precip_rate",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_rate -> min_precip_rate = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_RATE: min_precip_rate unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_RATE_DEA_NAME,
                                 ".max_precip_rate",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_rate -> max_precip_rate = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_RATE: max_precip_rate unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_RATE_DEA_NAME,
                                 ".zr_mult",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_rate -> zr_mult = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_RATE: zr_mult unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_RATE_DEA_NAME,
                                 ".zr_exp",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_rate -> zr_exp = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_RATE: zr_exp unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "HYDROMET_RATE: range_cutoff \t\t= %d\n", hydromet_rate -> range_cutoff );
   LE_send_msg( GL_INFO, "HYDROMET_RATE: range_coef1 \t\t= %3.1f\n", hydromet_rate -> range_coef1 );
   LE_send_msg( GL_INFO, "HYDROMET_RATE: range_coef2 \t\t= %4.1f\n", hydromet_rate -> range_coef2 );
   LE_send_msg( GL_INFO, "HYDROMET_RATE: range_coef3 \t\t= %3.1f\n", hydromet_rate -> range_coef3 );
   LE_send_msg( GL_INFO, "HYDROMET_RATE: min_precip_rate \t\t= %4.1f\n", hydromet_rate -> min_precip_rate );
   LE_send_msg( GL_INFO, "HYDROMET_RATE: max_precip_rate \t\t= %6.1f\n", hydromet_rate -> max_precip_rate );
   LE_send_msg( GL_INFO, "HYDROMET_RATE: zr_mult \t\t= %6.1f\n", hydromet_rate -> zr_mult );
   LE_send_msg( GL_INFO, "HYDROMET_RATE: zr_exp \t\t= %3.1f\n", hydromet_rate -> zr_exp );
#endif

  return 0;
}
