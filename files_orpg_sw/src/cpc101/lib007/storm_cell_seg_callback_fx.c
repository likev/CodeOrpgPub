/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:13:05 $
 * $Id: storm_cell_seg_callback_fx.c,v 1.8 2007/01/30 23:13:05 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <storm_cell_seg.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define storm_cell_seg_callback_fx storm_cell_seg_callback_fx_
#endif

#ifdef LINUX
#define storm_cell_seg_callback_fx storm_cell_seg_callback_fx__
#endif

#endif


int storm_cell_seg_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  int i = 0;			/* looping variable */
  double get_array1[ STORM_CELL_MAX_REF_THRESH ];	/* temp array */
  storm_cell_seg_t *storm_cell_seg = ( storm_cell_seg_t * )common_block_address;


  /* Get storm cell segments data elements */

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".refl_threshold", get_array1 );
  if( ret == 0 )
  {
    for( i = 0; i < STORM_CELL_MAX_REF_THRESH; i++ )
    {
      storm_cell_seg -> refl_threshold[ i ] = ( fint )get_array1[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: refl_threshold unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".seg_length", get_array1 );
  if( ret == 0 )
  {
    for( i = 0; i < STORM_CELL_MAX_REF_THRESH; i++ )
    {
      storm_cell_seg -> seg_length[ i ] = ( freal )get_array1[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: seg_length unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".dropout_refl_diff", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> dropout_refl_diff = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: dropout_refl_diff unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".dropout_count", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> dropout_count = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: dropout_count unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".num_refl_levels", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> num_refl_levels = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: num_refl_levels unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".max_segment_range", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> max_segment_range = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: max_segment_range unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".max_segs_per_radial", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> max_segs_per_radial = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: max_segs_per_radial unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".max_segs_per_elev", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> max_segs_per_elev = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: max_segs_per_elev unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".refl_ave_factor", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> refl_ave_factor = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: refl_ave_factor unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".mass_weight_factor", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> mass_weight_factor = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: mass_weight_factor unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".mass_mult_factor", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> mass_mult_factor = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: mass_mult_factor unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".mass_coef_factor", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> mass_coef_factor = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: mass_coef_factor unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".filter_kernel_size", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> filter_kernel_size = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: filter kernel size  unavailable, abort task\n" );
    RPG_abort_task();
  }
  
  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".filter_fract_req", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> filter_fract_req = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: fraction required  unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_SEG_DEA_NAME, ".filter_on", &get_value );
  if( ret == 0 )
  {
    storm_cell_seg -> filter_on = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_SEG: filter_on flag  unavailable, abort task\n" );
    RPG_abort_task();
  }


#ifdef DEBUG
   for( i = 0; i < STORM_CELL_MAX_REF_THRESH; i++ )
   {
     LE_send_msg( GL_INFO, "STORM_CELL_SEG: refl_threshold[%d] \t\t= %d\n", i, storm_cell_seg -> refl_threshold[ i ] );
   }
   for( i = 0; i < STORM_CELL_MAX_REF_THRESH; i++ )
   {
     LE_send_msg( GL_INFO, "STORM_CELL_SEG: seg_length[%d] \t\t= %3.1f\n", i, storm_cell_seg -> seg_length[ i ] );
   }
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: dropout_refl_diff \t\t= %d\n",  storm_cell_seg -> dropout_refl_diff );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: dropout_count \t\t= %d\n",  storm_cell_seg -> dropout_count );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: num_refl_levels \t\t= %d\n",  storm_cell_seg -> num_refl_levels );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: max_segment_range \t\t= %d\n",  storm_cell_seg -> max_segment_range );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: max_segs_per_radial \t\t= %d\n",  storm_cell_seg -> max_segs_per_radial );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: max_segs_per_elev \t\t= %d\n",  storm_cell_seg -> max_segs_per_elev );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: refl_ave_factor \t\t= %d\n",  storm_cell_seg -> refl_ave_factor );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: mass_weight_factor \t\t= %7.1f\n",  storm_cell_seg -> mass_weight_factor );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: mass_mult_factor \t\t= %5.1f\n",  storm_cell_seg -> mass_mult_factor );
   LE_send_msg( GL_INFO, "STORM_CELL_SEG: mass_coef_factor \t\t= %4.2f\n",  storm_cell_seg -> mass_coef_factor );
#endif

  return 0;
}
