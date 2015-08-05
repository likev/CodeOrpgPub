/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/10/07 19:47:14 $
 * $Id: superob_callback_fx.c,v 1.4 2010/10/07 19:47:14 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <superob_adapt.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define superob_callback_fx superob_callback_fx_
#endif

#ifdef LINUX
#define superob_callback_fx superob_callback_fx__
#endif

#endif


int superob_callback_fx( void *struct_address )
{
  double get_value = 0.0;  /* used to get data element's value */
  int ret = -1;            /* return status */
  superob_adapt_t *superob = (superob_adapt_t *)struct_address;


  /* Get superob data elements. */

  ret = RPG_ade_get_values( SUPEROB_DEA_NAME,
                             ".rangemax",
                             &get_value );
  if( ret == 0 )
  {
    superob -> rangemax = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SUPEROB: rangemax unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SUPEROB_DEA_NAME,
                             ".deltr",
                             &get_value );
  if( ret == 0 )
  {
    superob -> deltr = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SUPEROB: deltr unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SUPEROB_DEA_NAME,
                             ".offset_min",
                             &get_value );
  if( ret == 0 )
  {
    superob -> offset_min = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SUPEROB: offset_min unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SUPEROB_DEA_NAME,
                             ".deltaz",
                             &get_value );
  if( ret == 0 )
  {
    superob -> deltaz = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SUPEROB: deltaz unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SUPEROB_DEA_NAME,
                             ".deltt",
                             &get_value );
  if( ret == 0 )
  {
    superob -> deltt = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SUPEROB: deltt unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( SUPEROB_DEA_NAME,
                             ".min_sample_size",
                             &get_value );
  if( ret == 0 )
  {
    superob -> min_sample_size = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SUPEROB: min_sample_size unavailable, abort task\n" );
    RPG_abort_task();
  }


#ifdef DEBUG
   LE_send_msg( GL_INFO, "SUPEROB: rangemax \t\t= %d\n", superob -> rangemax );
   LE_send_msg( GL_INFO, "SUPEROB: deltr \t\t= %d\n", superob -> deltr );
   LE_send_msg( GL_INFO, "SUPEROB: deltaz \t\t= %d\n", superob -> deltaz );
   LE_send_msg( GL_INFO, "SUPEROB: deltt \t\t= %d\n", superob -> deltt );
   LE_send_msg( GL_INFO, "SUPEROB: min_sample_size \t\t= %d\n", superob -> min_sample_size );
#endif

  return 0;
}
