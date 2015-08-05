/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:43 $
 * $Id: hydromet_acc_callback_fx.c,v 1.8 2007/01/30 23:12:43 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <hydromet_acc.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define hydromet_acc_callback_fx hydromet_acc_callback_fx_
#endif

#ifdef LINUX
#define hydromet_acc_callback_fx hydromet_acc_callback_fx__
#endif

#endif


int hydromet_acc_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  hydromet_acc_t *hydromet_acc = ( hydromet_acc_t * )common_block_address;


  /* Get hydromet accumulation data elements */

  ret = RPG_ade_get_values( HYDROMET_ACC_DEA_NAME,
                                 ".restart_time",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_acc -> restart_time = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ACC: restart_time unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ACC_DEA_NAME,
                                 ".max_interp_time",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_acc -> max_interp_time = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ACC: max_interp_time unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ACC_DEA_NAME,
                                 ".min_time_period",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_acc -> min_time_period = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ACC: min_time_period unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ACC_DEA_NAME,
                                 ".hourly_outlier",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_acc -> hourly_outlier = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ACC: hourly_outlier unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ACC_DEA_NAME,
                                 ".end_gage_time",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_acc -> end_gage_time = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ACC: end_gage_time unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ACC_DEA_NAME,
                                 ".max_period_acc",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_acc -> max_period_acc = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ACC: max_period_acc unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_ACC_DEA_NAME,
                                 ".max_hourly_acc",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_acc -> max_hourly_acc = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_ACC: max_hourly_acc unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "HYDROMET_ACC: restart_time \t\t= %d\n", hydromet_acc -> restart_time );
   LE_send_msg( GL_INFO, "HYDROMET_ACC: max_interp_time \t\t= %d\n", hydromet_acc -> max_interp_time );
   LE_send_msg( GL_INFO, "HYDROMET_ACC: min_time_period \t\t= %d\n", hydromet_acc -> min_time_period );
   LE_send_msg( GL_INFO, "HYDROMET_ACC: hourly_outlier \t\t= %d\n", hydromet_acc -> hourly_outlier );
   LE_send_msg( GL_INFO, "HYDROMET_ACC: end_gage_time \t\t= %d\n", hydromet_acc -> end_gage_time );
   LE_send_msg( GL_INFO, "HYDROMET_ACC: max_period_acc \t\t= %d\n", hydromet_acc -> max_period_acc );
   LE_send_msg( GL_INFO, "HYDROMET_ACC: max_hourly_acc \t\t= %d\n", hydromet_acc -> max_hourly_acc );
#endif

  return 0;
}
