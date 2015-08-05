/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:39 $
 * $Id: cell_prod_callback_fx.c,v 1.2 2007/01/30 23:12:39 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>
#include <cell_prod_params.h>


/*** Local Include Files ***/


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define cell_prod_callback_fx cell_prod_callback_fx_
#endif

#ifdef LINUX
#define cell_prod_callback_fx cell_prod_callback_fx__
#endif

#endif


int cell_prod_callback_fx( void *common_block_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  int ret = -1;			/* return status */
  cell_prod_params_t *cell_prod = ( cell_prod_params_t * )common_block_address;


  /* Get cell product parameters data elements */

  ret = RPG_ade_get_values( CELL_PROD_DEA_NAME, ".sti_alpha", &get_value );
  if( ret == 0 )
  {
    cell_prod -> sti_alpha = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "CELL_PROD: sti_alpha unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( CELL_PROD_DEA_NAME, ".ss_alpha", &get_value );
  if( ret == 0 )
  {
    cell_prod -> ss_alpha = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "CELL_PROD: ss_alpha unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( CELL_PROD_DEA_NAME, ".hail_alpha", &get_value );
  if( ret == 0 )
  {
    cell_prod -> hail_alpha = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "CELL_PROD: hail_alpha unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( CELL_PROD_DEA_NAME, ".sti_attrib", &get_value );
  if( ret == 0 )
  {
    cell_prod -> sti_attrib = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "CELL_PROD: sti_attrib unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( CELL_PROD_DEA_NAME, ".comb_attrib", &get_value );
  if( ret == 0 )
  {
    cell_prod -> comb_attrib = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "CELL_PROD: comb_attrib unavailable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( CELL_PROD_DEA_NAME, ".hail_attrib", &get_value );
  if( ret == 0 )
  {
    cell_prod -> hail_attrib = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "CELL_PROD: hail_attrib unavailable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "CELL_PROD: sti_alpha \t\t= %d\n",  cell_prod -> sti_alpha );
   LE_send_msg( GL_INFO, "CELL_PROD: ss_alpha \t\t= %d\n",  cell_prod -> ss_alpha );
   LE_send_msg( GL_INFO, "CELL_PROD: hail_alpha \t\t= %d\n",  cell_prod -> hail_alpha );
   LE_send_msg( GL_INFO, "CELL_PROD: sti_attrib \t\t= %d\n",  cell_prod -> sti_attrib );
   LE_send_msg( GL_INFO, "CELL_PROD: comb_attrib \t\t= %d\n",  cell_prod -> comb_attrib );
   LE_send_msg( GL_INFO, "CELL_PROD: hail_attrib \t\t= %d\n",  cell_prod -> hail_attrib );
#endif

  return 0;
}
