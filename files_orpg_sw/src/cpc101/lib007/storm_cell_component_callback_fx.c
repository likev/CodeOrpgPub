/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:13:04 $
 * $Id: storm_cell_component_callback_fx.c,v 1.7 2007/01/30 23:13:04 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <storm_cell_component.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define storm_cell_comp_callback_fx storm_cell_comp_callback_fx_
#endif

#ifdef LINUX
#define storm_cell_comp_callback_fx storm_cell_comp_callback_fx__
#endif

#endif


int storm_cell_comp_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  int i = 0;			/* looping variable */
  double get_array1[ STORM_CELL_MAX_REF_THRESH ];	/* temp array */
  double get_array2[ STORM_CELL_MAX_RADIUS_THRESH ];	/* temp array */
  storm_cell_comp_t *storm_cell_comp = ( storm_cell_comp_t * )common_block_address;


  /* Get storm cell comp data elements */

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".segment_overlap", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> segment_overlap = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: segment_overlap unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".max_pot_comp_per_elev", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> max_pot_comp_per_elev = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: max_pot_comp_per_elev unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".max_comp_per_elev", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> max_comp_per_elev = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: max_comp_per_elev unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".max_detect_cells", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> max_detect_cells = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: max_detect_cells unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".num_segs_per_comp", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> num_segs_per_comp = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: num_segs_per_comp unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".max_cells_per_vol", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> max_cells_per_vol = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: max_cells_per_vol unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".max_vil", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> max_vil = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: max_vil unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".comp_area", get_array1 );
  if( ret == 0 )
  {
    for( i = 0; i < STORM_CELL_MAX_REF_THRESH; i++ )
    {
      storm_cell_comp -> comp_area[ i ] = ( freal )get_array1[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: comp_area unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".search_radius", get_array2 );
  if( ret == 0 )
  {
    for( i = 0; i < STORM_CELL_MAX_RADIUS_THRESH; i++ )
    {
      storm_cell_comp -> search_radius[ i ] = ( freal )get_array2[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: search_radius unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".depth_delete", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> depth_delete = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: depth_delete unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".horiz_delete", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> horiz_delete = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: horiz_delete unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".elev_merge", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> elev_merge = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: elev_merge unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".height_merge", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> height_merge = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: height_merge unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".horiz_merge", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> horiz_merge = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: horiz_merge unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( STORM_CELL_COMP_DEA_NAME, ".azi_separation", &get_value );
  if( ret == 0 )
  {
    storm_cell_comp -> azi_separation = ( freal )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "STORM_CELL_COMP: azi_merge unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: segment_overlap \t\t= %d\n",  storm_cell_comp -> segment_overlap );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: max_pot_comp_per_elev \t\t= %d\n",  storm_cell_comp -> max_pot_comp_per_elev );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: max_comp_per_elev \t\t= %d\n",  storm_cell_comp -> max_comp_per_elev );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: max_detect_cells \t\t= %d\n",  storm_cell_comp -> max_detect_cells );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: num_segs_per_comp \t\t= %d\n",  storm_cell_comp -> num_segs_per_comp );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: max_cells_per_vol \t\t= %d\n",  storm_cell_comp -> max_cells_per_vol );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: max_vil \t\t= %d\n",  storm_cell_comp -> max_vil );
   for( i = 0; i < STORM_CELL_MAX_REF_THRESH; i++ )
   {
     LE_send_msg( GL_INFO, "STORM_CELL_COMP: comp_area[%d] \t\t= %4.1f\n", i, storm_cell_comp -> comp_area[ i ] );
   }
   for( i = 0; i < STORM_CELL_MAX_RADIUS_THRESH; i++ )
   {
     LE_send_msg( GL_INFO, "STORM_CELL_COMP: comp_area[%d] \t\t= %4.1f\n", i, storm_cell_comp -> search_radius[ i ] );
   }
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: depth_delete \t\t= %4.1f\n",  storm_cell_comp -> depth_delete );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: horiz_delete \t\t= %4.1f\n",  storm_cell_comp -> horiz_delete );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: elev_merge \t\t= %3.1f\n",  storm_cell_comp -> elev_merge );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: height_merge \t\t= %3.1f\n",  storm_cell_comp -> height_merge );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: horiz_merge \t\t= %4.1f\n",  storm_cell_comp -> horiz_merge );
   LE_send_msg( GL_INFO, "STORM_CELL_COMP: azi_separation \t\t= %3.1f\n",  storm_cell_comp -> azi_separation );
#endif

  return 0;
}
