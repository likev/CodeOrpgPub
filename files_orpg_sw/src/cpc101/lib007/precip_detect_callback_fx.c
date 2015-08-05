/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:55 $
 * $Id: precip_detect_callback_fx.c,v 1.8 2007/01/30 23:12:55 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <precip_detect.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define precip_detect_callback_fx precip_detect_callback_fx_
#endif

#ifdef LINUX
#define precip_detect_callback_fx precip_detect_callback_fx__
#endif

#endif


int precip_detect_callback_fx( void *common_block_address )
{
  int ret = -1;			/* return status */
  int i = 0;			/* looping variable */
  int j = 0;			/* looping variable */
  double get_array[ PRECIP_DETECT_NUMROWS ];	/* temp array */
  precip_detect_t *precip_detect = ( precip_detect_t * )common_block_address;


  /* Get precip detect data elements */

  ret = RPG_ade_get_values( PRECIP_DETECT_DEA_NAME, ".min_elev", get_array );
  if( ret >= 0 )
  {
    for( i = 0; i < PRECIP_DETECT_NUMROWS; i++ )
    {
      precip_detect -> precip_thresh[ 0 ][ i ] = ( fint )get_array[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "PRECIP_DETECT: min_elev unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( PRECIP_DETECT_DEA_NAME, ".max_elev", get_array );
  if( ret >= 0 )
  {
    for( i = 0; i < PRECIP_DETECT_NUMROWS; i++ )
    {
      precip_detect -> precip_thresh[ 1 ][ i ] = ( fint )get_array[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "PRECIP_DETECT: max_elev unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( PRECIP_DETECT_DEA_NAME, ".rate", get_array );
  if( ret >= 0 )
  {
    for( i = 0; i < PRECIP_DETECT_NUMROWS; i++ )
    {
      precip_detect -> precip_thresh[ 2 ][ i ] = ( fint )get_array[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "PRECIP_DETECT: rate unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( PRECIP_DETECT_DEA_NAME, ".nominal_clutter_area", get_array );
  if( ret >= 0 )
  {
    for( i = 0; i < PRECIP_DETECT_NUMROWS; i++ )
    {
      precip_detect -> precip_thresh[ 3 ][ i ] = ( fint )get_array[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "PRECIP_DETECT: nominal_clutter_area unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( PRECIP_DETECT_DEA_NAME, ".precip_area_thresh", get_array );
  if( ret >= 0 )
  {
    for( i = 0; i < PRECIP_DETECT_NUMROWS; i++ )
    {
      precip_detect -> precip_thresh[ 4 ][ i ] = ( fint )get_array[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "PRECIP_DETECT: precip_area_thresh unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( PRECIP_DETECT_DEA_NAME, ".precip_category", get_array );
  if( ret >= 0 )
  {
    for( i = 0; i < PRECIP_DETECT_NUMROWS; i++ )
    {
      precip_detect -> precip_thresh[ 5 ][ i ] = ( fint )get_array[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "PRECIP_DETECT: precip_category unavailable, abort task\n" );
    RPG_abort_task();
  }

  j = 0; /* Prevent compiler warning about unused "j" */
#ifdef DEBUG
   for( i = 0; i < PRECIP_DETECT_NUMCATS; i++ )
   {
     for( j = 0; j < PRECIP_DETECT_NUMROWS; j++ )
     {
       LE_send_msg(GL_INFO, "PRECIP_DETECT: precip_thresh[%d][%d] \t\t= %d\n", i, j, precip_detect -> precip_thresh[ i ][ j ] );
     }
   }
#endif

  return 0;
}
