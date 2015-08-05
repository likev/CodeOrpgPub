/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:46 $
 * $Id: hydromet_prep_callback_fx.c,v 1.8 2007/01/30 23:12:46 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <hydromet_prep.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define hydromet_prep_callback_fx hydromet_prep_callback_fx_
#endif

#ifdef LINUX
#define hydromet_prep_callback_fx hydromet_prep_callback_fx__
#endif

#endif


int hydromet_prep_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  hydromet_prep_t *hydromet_prep = ( hydromet_prep_t * )common_block_address;


  /* Get hydromet preprocessing data elements */

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".beam_width",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> beam_width = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: beam_width unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".block_thresh",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> block_thresh = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: block_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".clutter_thresh",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> clutter_thresh = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: clutter_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".weight_thresh",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> weight_thresh = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: weight_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".full_hys_thresh",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> full_hys_thresh = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: full_hys_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".low_dbz_thresh",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> low_dbz_thresh = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: low_dbz_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".rain_dbz_thresh",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> rain_dbz_thresh = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: rain_dbz_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".rain_area_thresh",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> rain_area_thresh = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: rain_area_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".rain_time_thresh",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> rain_time_thresh = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: rain_time_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".min_refl_rate",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> min_refl_rate = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: min_refl_rate unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".max_refl_rate",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> max_refl_rate = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: max_refl_rate unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".num_zone",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> num_zone = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: num_zone unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm1",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm1 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm1",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm1 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng1",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng1 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng1",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng1 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl1",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl1 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl1 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm2",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm2 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm2",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm2 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng2",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng2 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng2",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng2 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl2",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl2 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl2 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm3",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm3 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm3",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm3 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng3",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng3 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng3",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng3 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl3",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl3 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl3 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm4",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm4 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm4 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm4",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm4 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm4 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng4",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng4 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng4 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng4",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng4 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng4 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl4",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl4 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl4 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm5",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm5 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm5 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm5",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm5 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm5 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng5",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng5 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng5 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng5",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng5 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng5 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl5",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl5 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl5 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm6",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm6 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm6 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm6",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm6 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm6 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng6",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng6 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng6 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng6",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng6 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng6 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl6",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl6 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl6 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm7",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm7 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm7 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm7",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm7 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm7 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng7",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng7 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng7 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng7",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng7 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng7 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl7",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl7 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl7 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm8",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm8 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm8 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm8",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm8 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm8 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng8",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng8 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng8 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng8",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng8 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng8 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl8",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl8 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl8 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm9",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm9 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm9 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm9",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm9 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm9 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng9",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng9 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng9 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng9",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng9 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng9 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl9",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl9 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl9 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm10",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm10 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm10 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm10",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm10 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm10 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng10",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng10 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng10 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng10",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng10 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng10 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl10",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl10 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl10 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm11",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm11 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm11 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm11",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm11 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm11 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng11",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng11 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng11 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng11",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng11 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng11 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl11",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl11 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl11 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm12",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm12 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm12 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm12",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm12 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm12 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng12",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng12 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng12 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng12",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng12 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng12 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl12",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl12 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl12 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm13",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm13 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm13 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm13",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm13 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm13 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng13",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng13 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng13 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng13",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng13 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng13 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl13",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl13 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl13 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm14",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm14 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm14 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm14",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm14 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm14 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng14",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng14 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng14 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng14",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng14 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng14 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl14",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl14 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl14 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm15",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm15 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm15 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm15",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm15 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm15 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng15",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng15 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng15 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng15",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng15 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng15 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl15",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl15 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl15 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm16",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm16 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm16 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm16",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm16 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm16 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng16",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng16 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng16 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng16",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng16 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng16 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl16",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl16 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl16 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm17",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm17 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm17 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm17",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm17 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm17 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng17",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng17 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng17 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng17",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng17 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng17 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl17",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl17 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl17 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm18",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm18 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm18 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm18",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm18 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm18 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng18",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng18 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng18 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng18",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng18 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng18 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl18",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl18 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl18 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm19",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm19 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm19 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm19",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm19 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm19 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng19",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng19 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng19 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng19",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng19 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng19 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl19",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl19 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl19 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_azm20",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_azm20 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_azm20 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_azm20",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_azm20 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_azm20 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Beg_rng20",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Beg_rng20 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Beg_rng20 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".End_rng20",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> End_rng20 = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: End_rng20 unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( HYDROMET_PREP_DEA_NAME,
                                 ".Elev_agl20",
                                 &get_value );
  if( ret == 0 )
  {
    hydromet_prep -> Elev_agl20 = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "HYDROMET_PREP: Elev_agl20 unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "HYDROMET_PREP: beam_width \t\t= %3.1f\n", hydromet_prep -> beam_width );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: block_thresh \t\t= %5.1f\n", hydromet_prep -> block_thresh );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: clutter_thresh \t\t= %d\n", hydromet_prep -> clutter_thresh );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: weight_thresh \t\t= %5.1f\n", hydromet_prep -> weight_thresh );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: full_hys_thresh \t\t= %5.1f\n", hydromet_prep -> full_hys_thresh );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: low_dbz_thresh \t\t= %5.1f\n", hydromet_prep -> low_dbz_thresh );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: rain_dbz_thresh \t\t= %4.1f\n", hydromet_prep -> rain_dbz_thresh );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: rain_area_thresh \t\t= %d\n", hydromet_prep -> rain_area_thresh );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: rain_time_thresh \t\t= %d\n", hydromet_prep -> rain_time_thresh );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: min_refl_rate \t\t= %5.1f\n", hydromet_prep -> min_refl_rate );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: max_refl_rate \t\t= %4.1f\n", hydromet_prep -> max_refl_rate );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: num_zone \t\t= %d\n", hydromet_prep -> num_zone );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm1 \t\t= %5.1f\n", hydromet_prep -> Beg_azm1 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm1 \t\t= %5.1f\n", hydromet_prep -> End_azm1 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng1 \t\t= %d\n", hydromet_prep -> Beg_rng1 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng1 \t\t= %d\n", hydromet_prep -> End_rng1 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl1 \t\t= %4.1f\n", hydromet_prep -> Elev_agl1 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm2 \t\t= %5.1f\n", hydromet_prep -> Beg_azm2 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm2 \t\t= %5.1f\n", hydromet_prep -> End_azm2 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng2 \t\t= %d\n", hydromet_prep -> Beg_rng2 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng2 \t\t= %d\n", hydromet_prep -> End_rng2 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl2 \t\t= %4.1f\n", hydromet_prep -> Elev_agl2 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm3 \t\t= %5.1f\n", hydromet_prep -> Beg_azm3 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm3 \t\t= %5.1f\n", hydromet_prep -> End_azm3 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng3 \t\t= %d\n", hydromet_prep -> Beg_rng3 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng3 \t\t= %d\n", hydromet_prep -> End_rng3 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl3 \t\t= %4.1f\n", hydromet_prep -> Elev_agl3 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm4 \t\t= %5.1f\n", hydromet_prep -> Beg_azm4 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm4 \t\t= %5.1f\n", hydromet_prep -> End_azm4 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng4 \t\t= %d\n", hydromet_prep -> Beg_rng4 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng4 \t\t= %d\n", hydromet_prep -> End_rng4 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl4 \t\t= %4.1f\n", hydromet_prep -> Elev_agl4 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm5 \t\t= %5.1f\n", hydromet_prep -> Beg_azm5 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm5 \t\t= %5.1f\n", hydromet_prep -> End_azm5 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng5 \t\t= %d\n", hydromet_prep -> Beg_rng5 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng5 \t\t= %d\n", hydromet_prep -> End_rng5 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl5 \t\t= %4.1f\n", hydromet_prep -> Elev_agl5 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm6 \t\t= %5.1f\n", hydromet_prep -> Beg_azm6 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm6 \t\t= %5.1f\n", hydromet_prep -> End_azm6 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng6 \t\t= %d\n", hydromet_prep -> Beg_rng6 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng6 \t\t= %d\n", hydromet_prep -> End_rng6 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl6 \t\t= %4.1f\n", hydromet_prep -> Elev_agl6 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm7 \t\t= %5.1f\n", hydromet_prep -> Beg_azm7 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm7 \t\t= %5.1f\n", hydromet_prep -> End_azm7 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng7 \t\t= %d\n", hydromet_prep -> Beg_rng7 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng7 \t\t= %d\n", hydromet_prep -> End_rng7 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl7 \t\t= %4.1f\n", hydromet_prep -> Elev_agl7 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm8 \t\t= %5.1f\n", hydromet_prep -> Beg_azm8 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm8 \t\t= %5.1f\n", hydromet_prep -> End_azm8 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng8 \t\t= %d\n", hydromet_prep -> Beg_rng8 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng8 \t\t= %d\n", hydromet_prep -> End_rng8 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl8 \t\t= %4.1f\n", hydromet_prep -> Elev_agl8 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm9 \t\t= %5.1f\n", hydromet_prep -> Beg_azm9 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm9 \t\t= %5.1f\n", hydromet_prep -> End_azm9 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng9 \t\t= %d\n", hydromet_prep -> Beg_rng9 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng9 \t\t= %d\n", hydromet_prep -> End_rng9 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl9 \t\t= %4.1f\n", hydromet_prep -> Elev_agl9 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm10 \t\t= %5.1f\n", hydromet_prep -> Beg_azm10 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm10 \t\t= %5.1f\n", hydromet_prep -> End_azm10 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng10 \t\t= %d\n", hydromet_prep -> Beg_rng10 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng10 \t\t= %d\n", hydromet_prep -> End_rng10 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl10 \t\t= %4.1f\n", hydromet_prep -> Elev_agl10 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm11 \t\t= %5.1f\n", hydromet_prep -> Beg_azm11 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm11 \t\t= %5.1f\n", hydromet_prep -> End_azm11 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng11 \t\t= %d\n", hydromet_prep -> Beg_rng11 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng11 \t\t= %d\n", hydromet_prep -> End_rng11 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl11 \t\t= %4.1f\n", hydromet_prep -> Elev_agl11 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm12 \t\t= %5.1f\n", hydromet_prep -> Beg_azm12 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm12 \t\t= %5.1f\n", hydromet_prep -> End_azm12 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng12 \t\t= %d\n", hydromet_prep -> Beg_rng12 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng12 \t\t= %d\n", hydromet_prep -> End_rng12 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl12 \t\t= %4.1f\n", hydromet_prep -> Elev_agl12 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm13 \t\t= %5.1f\n", hydromet_prep -> Beg_azm13 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm13 \t\t= %5.1f\n", hydromet_prep -> End_azm13 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng13 \t\t= %d\n", hydromet_prep -> Beg_rng13 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng13 \t\t= %d\n", hydromet_prep -> End_rng13 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl13 \t\t= %4.1f\n", hydromet_prep -> Elev_agl13 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm14 \t\t= %5.1f\n", hydromet_prep -> Beg_azm14 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm14 \t\t= %5.1f\n", hydromet_prep -> End_azm14 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng14 \t\t= %d\n", hydromet_prep -> Beg_rng14 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng14 \t\t= %d\n", hydromet_prep -> End_rng14 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl14 \t\t= %4.1f\n", hydromet_prep -> Elev_agl14 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm15 \t\t= %5.1f\n", hydromet_prep -> Beg_azm15 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm15 \t\t= %5.1f\n", hydromet_prep -> End_azm15 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng15 \t\t= %d\n", hydromet_prep -> Beg_rng15 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng15 \t\t= %d\n", hydromet_prep -> End_rng15 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl15 \t\t= %4.1f\n", hydromet_prep -> Elev_agl15 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm16 \t\t= %5.1f\n", hydromet_prep -> Beg_azm16 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm16 \t\t= %5.1f\n", hydromet_prep -> End_azm16 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng16 \t\t= %d\n", hydromet_prep -> Beg_rng16 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng16 \t\t= %d\n", hydromet_prep -> End_rng16 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl16 \t\t= %4.1f\n", hydromet_prep -> Elev_agl16 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm17 \t\t= %5.1f\n", hydromet_prep -> Beg_azm17 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm17 \t\t= %5.1f\n", hydromet_prep -> End_azm17 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng17 \t\t= %d\n", hydromet_prep -> Beg_rng17 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng17 \t\t= %d\n", hydromet_prep -> End_rng17 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl17 \t\t= %4.1f\n", hydromet_prep -> Elev_agl17 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm18 \t\t= %5.1f\n", hydromet_prep -> Beg_azm18 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm18 \t\t= %5.1f\n", hydromet_prep -> End_azm18 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng18 \t\t= %d\n", hydromet_prep -> Beg_rng18 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng18 \t\t= %d\n", hydromet_prep -> End_rng18 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl18 \t\t= %4.1f\n", hydromet_prep -> Elev_agl18 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm19 \t\t= %5.1f\n", hydromet_prep -> Beg_azm19 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm19 \t\t= %5.1f\n", hydromet_prep -> End_azm19 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng19 \t\t= %d\n", hydromet_prep -> Beg_rng19 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng19 \t\t= %d\n", hydromet_prep -> End_rng19 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl19 \t\t= %4.1f\n", hydromet_prep -> Elev_agl19 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_azm20 \t\t= %5.1f\n", hydromet_prep -> Beg_azm20 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_azm20 \t\t= %5.1f\n", hydromet_prep -> End_azm20 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Beg_rng20 \t\t= %d\n", hydromet_prep -> Beg_rng20 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: End_rng20 \t\t= %d\n", hydromet_prep -> End_rng20 );
   LE_send_msg( GL_INFO, "HYDROMET_PREP: Elev_agl20 \t\t= %4.1f\n", hydromet_prep -> Elev_agl20 );
#endif

  return 0;
}
