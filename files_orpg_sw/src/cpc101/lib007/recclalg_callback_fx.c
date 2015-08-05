/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:58 $
 * $Id: recclalg_callback_fx.c,v 1.5 2007/01/30 23:12:58 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/***************************************************************************/

/*** System Include Files ***/
#include <alg_adapt.h>
#include <recclalg_adapt.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define recclalg_callback_fx recclalg_callback_fx_
#endif

#ifdef LINUX
#define recclalg_callback_fx recclalg_callback_fx__
#endif

#endif


int recclalg_callback_fx( void *struct_address )
{
  double get_value = 0.0;  /* used to get data element's value */
  int ret = -1;            /* return status */
  rec_cl_target_t *recclalg = (rec_cl_target_t *)struct_address;


  /* Get recclalg data elements. */

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Ztxtr0", &get_value );
  if( ret == 0 )
  {
    recclalg -> Ztxtr0 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Ztxtr0 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Ztxtr1", &get_value );
  if( ret == 0 )
  {
    recclalg -> Ztxtr1 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Ztxtr1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Zsign0", &get_value );
  if( ret == 0 )
  {
    recclalg -> Zsign0 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Zsign0 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Zsign1", &get_value );
  if( ret == 0 )
  {
    recclalg -> Zsign1 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Zsign1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Zspin0", &get_value );
  if( ret == 0 )
  {
    recclalg -> Zspin0 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Zspin0 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Zspin1", &get_value );
  if( ret == 0 )
  {
    recclalg -> Zspin1 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Zspin1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".ZspinThr", &get_value );
  if( ret == 0 )
  {
    recclalg -> ZspinThr = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: ZspinThr unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".ZThr", &get_value );
  if( ret == 0 )
  {
    recclalg -> ZThr = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: ZThr unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Vmean0", &get_value );
  if( ret == 0 )
  {
    recclalg -> Vmean0 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Vmean0 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Vmean1", &get_value );
  if( ret == 0 )
  {
    recclalg -> Vmean1 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Vmean1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Vstdv0", &get_value );
  if( ret == 0 )
  {
    recclalg -> Vstdv0 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Vstdv0 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Vstdv1", &get_value );
  if( ret == 0 )
  {
    recclalg -> Vstdv1 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Vstdv1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Wmean0", &get_value );
  if( ret == 0 )
  {
    recclalg -> Wmean0 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Wmean0 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".Wmean1", &get_value );
  if( ret == 0 )
  {
    recclalg -> Wmean1 = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: Wmean1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".ZtxtrWt", &get_value );
  if( ret == 0 )
  {
    recclalg -> ZtxtrWt = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: ZtxtrWt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".ZsignWt", &get_value );
  if( ret == 0 )
  {
    recclalg -> ZsignWt = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: ZsignWt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".ZspinWt", &get_value );
  if( ret == 0 )
  {
    recclalg -> ZspinWt = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: ZspinWt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".VmeanWt", &get_value );
  if( ret == 0 )
  {
    recclalg -> VmeanWt = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: VmeanWt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".VstdvWt", &get_value );
  if( ret == 0 )
  {
    recclalg -> VstdvWt = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: VstdvWt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".WmeanWt", &get_value );
  if( ret == 0 )
  {
    recclalg -> WmeanWt = ( double )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: WmeanWt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".deltaAz", &get_value );
  if( ret == 0 )
  {
    recclalg -> deltaAz = ( short )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: deltaAz unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".deltaRng", &get_value );
  if( ret == 0 )
  {
    recclalg -> deltaRng = ( short )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: deltaRng unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( RECCLALG_DEA_NAME, ".deltaBin", &get_value );
  if( ret == 0 )
  {
    recclalg -> deltaBin = ( short )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "RECCLALG: deltaBin unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "RECCLALG: Ztxtr0 \t\t= %5.2f\n", recclalg -> Ztxtr0 );
   LE_send_msg( GL_INFO, "RECCLALG: Ztxtr1 \t\t= %5.2f\n", recclalg -> Ztxtr1 );
   LE_send_msg( GL_INFO, "RECCLALG: Zsign0 \t\t= %5.2f\n", recclalg -> Zsign0 );
   LE_send_msg( GL_INFO, "RECCLALG: Zsign1 \t\t= %5.2f\n", recclalg -> Zsign1 );
   LE_send_msg( GL_INFO, "RECCLALG: Zspin0 \t\t= %5.2f\n", recclalg -> Zspin0 );
   LE_send_msg( GL_INFO, "RECCLALG: Zspin1 \t\t= %5.2f\n", recclalg -> Zspin1 );
   LE_send_msg( GL_INFO, "RECCLALG: ZspinThr \t\t= %5.2f\n", recclalg -> ZspinThr );
   LE_send_msg( GL_INFO, "RECCLALG: ZThr \t\t= %5.2f\n", recclalg -> ZThr );
   LE_send_msg( GL_INFO, "RECCLALG: Vmean0 \t\t= %5.2f\n", recclalg -> Vmean0 );
   LE_send_msg( GL_INFO, "RECCLALG: Vmean1 \t\t= %5.2f\n", recclalg -> Vmean1 );
   LE_send_msg( GL_INFO, "RECCLALG: Vstdv0 \t\t= %5.2f\n", recclalg -> Vstdv0 );
   LE_send_msg( GL_INFO, "RECCLALG: Vstdv1 \t\t= %5.2f\n", recclalg -> Vstdv1 );
   LE_send_msg( GL_INFO, "RECCLALG: Wmean0 \t\t= %5.2f\n", recclalg -> Wmean0 );
   LE_send_msg( GL_INFO, "RECCLALG: Wmean1 \t\t= %5.2f\n", recclalg -> Wmean1 );
   LE_send_msg( GL_INFO, "RECCLALG: ZtxtrWt \t\t= %5.2f\n", recclalg -> ZtxtrWt );
   LE_send_msg( GL_INFO, "RECCLALG: ZsignWt \t\t= %5.2f\n", recclalg -> ZsignWt );
   LE_send_msg( GL_INFO, "RECCLALG: ZspinWt \t\t= %5.2f\n", recclalg -> ZspinWt );
   LE_send_msg( GL_INFO, "RECCLALG: VmeanWt \t\t= %5.2f\n", recclalg -> VmeanWt );
   LE_send_msg( GL_INFO, "RECCLALG: VstdvWt \t\t= %5.2f\n", recclalg -> VstdvWt );
   LE_send_msg( GL_INFO, "RECCLALG: WmeanWt \t\t= %5.2f\n", recclalg -> WmeanWt );
   LE_send_msg( GL_INFO, "RECCLALG: deltaAz \t\t= %d\n", recclalg -> deltaAz );
   LE_send_msg( GL_INFO, "RECCLALG: deltaRng \t\t= %d\n", recclalg -> deltaRng );
   LE_send_msg( GL_INFO, "RECCLALG: deltaBin \t\t= %d\n", recclalg -> deltaBin );
#endif

  return 0;
}
