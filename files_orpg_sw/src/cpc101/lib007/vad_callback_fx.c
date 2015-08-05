/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2010/10/19 16:25:23 $
 * $Id: vad_callback_fx.c,v 1.9 2010/10/19 16:25:23 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <vad.h>


/*** Local Include Files ***/

#define DEBUG 1

#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define vad_callback_fx vad_callback_fx_
#endif

#ifdef LINUX
#define vad_callback_fx vad_callback_fx__
#endif

#endif


int vad_callback_fx( void *common_block_address )
{
  double get_value = 0.0;  /* used to get data element's value */
  int ret = -1;            /* return status */
  vad_t *vad = ( vad_t * )common_block_address;


  /* Get vad data elements */

  ret = RPG_ade_get_values( VAD_DEA_NAME, ".thresh_velocity", &get_value );
  if( ret == 0 )
  {
    vad -> thresh_velocity = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VAD: thresh_velocity unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( VAD_DEA_NAME, ".num_fit_tests", &get_value );
  if( ret == 0 )
  {
    vad -> num_fit_tests = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VAD: num_fit_tests unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( VAD_DEA_NAME, ".min_samples", &get_value );
  if( ret == 0 )
  {
    vad -> min_samples = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VAD: min_samples unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( VAD_DEA_NAME, ".anal_range", &get_value );
  if( ret == 0 )
  {
    vad -> anal_range = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VAD: anal_range unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( VAD_DEA_NAME, ".start_azimuth", &get_value );
  if( ret == 0 )
  {
    vad -> start_azimuth = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VAD: start_azimuth unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( VAD_DEA_NAME, ".end_azimuth", &get_value );
  if( ret == 0 )
  {
    vad -> end_azimuth = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VAD: end_azimuth unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( VAD_DEA_NAME, ".symmetry", &get_value );
  if( ret == 0 )
  {
    vad -> symmetry = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "VAD: symmetry unavailable, abort task\n" );
    RPG_abort_task();
  }

  /* Enable Enhanced VAD logic VAD. */
  ret = RPG_ade_get_values( VAD_DEA_NAME, ".enhanced_vad", &get_value );
  if( ret == 0 ){

      vad -> enhanced_vad = (int) get_value;

  }
  else{

      LE_send_msg( GL_ERROR, "VAD: enhanced_vad unavailable, abort task\n" );
      RPG_abort_task();

  }

  /* Minimum number of points to accept a supplemental VAD. */
  ret = RPG_ade_get_values( VAD_DEA_NAME, ".min_points", &get_value );
  if( ret == 0 ){

      vad -> min_points = (int) get_value;

  }
  else{

      LE_send_msg( GL_ERROR, "VAD: min_points unavailable, abort task\n" );
      RPG_abort_task();

  }

  /* Minimum symmetry value to accept a supplemental VAD. */
  ret = RPG_ade_get_values( VAD_DEA_NAME, ".min_symmetry", &get_value );
    if( ret == 0 ){

      vad -> min_symmetry = (float) get_value;

  }
  else{

      LE_send_msg( GL_ERROR, "VAD: min_symmetry unavailable, abort task\n" );
      RPG_abort_task();

  }

  /* RMS scaling factor to accept a supplemental VAD. */
  ret = RPG_ade_get_values( VAD_DEA_NAME, ".scale_rms", &get_value );
  if( ret == 0 ){

      vad -> scale_rms = (float) get_value;

  }
  else{

      LE_send_msg( GL_ERROR, "VAD: scale_rms unavailable, abort task\n" );
      RPG_abort_task();

  }

  /* Minimum processing range for supplemental VAD. */
  ret = RPG_ade_get_values( VAD_DEA_NAME, ".min_proc_range", &get_value );
  if( ret == 0 ){

      vad -> min_proc_range = (float) get_value;

  }
  else{

      LE_send_msg( GL_ERROR, "VAD: min_proc_range unavailable, abort task\n" );
      RPG_abort_task();

  }

  /* Maximum processing range for supplemental VAD. */
  ret = RPG_ade_get_values( VAD_DEA_NAME, ".max_proc_range", &get_value );
  if( ret == 0 ){

      vad -> max_proc_range = (float) get_value;

  }
  else{

      LE_send_msg( GL_ERROR, "VAD: max_proc_range unavailable, abort task\n" );
      RPG_abort_task();

  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "VAD: thresh_velocity \t\t= %3.1f\n",  vad -> thresh_velocity );
   LE_send_msg( GL_INFO, "VAD: num_fit_tests \t\t= %d\n",  vad -> num_fit_tests );
   LE_send_msg( GL_INFO, "VAD: min_samples \t\t= %d\n",  vad -> min_samples );
   LE_send_msg( GL_INFO, "VAD: anal_range \t\t= %5.1f\n",  vad -> anal_range );
   LE_send_msg( GL_INFO, "VAD: start_azimuth \t\t= %5.1f\n",  vad -> start_azimuth );
   LE_send_msg( GL_INFO, "VAD: end_azimuth \t\t= %5.1f\n",  vad -> end_azimuth );
   LE_send_msg( GL_INFO, "VAD: symmetry \t\t= %4.1f\n",  vad -> symmetry );
   LE_send_msg( GL_INFO, "VAD: min_points \t\t= %d\n", vad -> min_points );
   LE_send_msg( GL_INFO, "VAD: min_symmetry\t\t= %5.1f\n", vad -> min_symmetry );
   LE_send_msg( GL_INFO, "VAD: scale_rms\t\t= %5.1f\n", vad -> scale_rms );
   LE_send_msg( GL_INFO, "VAD: enhanced_vad\t\t= %d\n", vad -> enhanced_vad );
   LE_send_msg( GL_INFO, "VAD: min_proc_range\t\t= %5.1f\n", vad -> min_proc_range );
   LE_send_msg( GL_INFO, "VAD: max_proc_range\t\t= %5.1f\n", vad -> max_proc_range );
#endif

  return 0;
}
