/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/03/08 18:27:56 $
 * $Id: rpg_site_info_callback_fx.c,v 1.5 2007/03/08 18:27:56 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <rpg.h>
#include <siteadp.h>
#include <orpgsite.h>


/*** Local Include Files ***/


/*
  This function is called whenever the site information adaptation
  data has been modified. The struct address is a pointer to the 
  site information struct passed by the algorithm when it called
  RPG_reg_site_info(). When the struct below is filled with new values,
  those new values are automatically reflected in the algorithm that
  registered.
*/

int RPG_site_info_callback_fx( void *struct_address )
{
  double get_value = 0.0;	/* used to get data element's value */
  char *get_string_value = NULL;/* used to get data element's value */
  int ret = -1;			/* return status */
  int i = 0;			/* looping variable */
  Siteadp_adpt_t *site_adapt = ( Siteadp_adpt_t * )struct_address;

  /* Get site info data elements */

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_RDA_LATITUDE,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> rda_lat = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: rda_lat unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_RDA_LONGITUDE,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> rda_lon = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: rda_lon unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_RDA_ELEVATION,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> rda_elev = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: rda_elev unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_RPG_ID,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> rpg_id = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: rpg_id unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_WX_MODE,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> wx_mode = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: wx_mode unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_DEF_MODE_A_VCP,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> def_mode_A_vcp = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: def_mode_A_vcp unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_DEF_MODE_B_VCP,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> def_mode_B_vcp = ( fint )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: def_mode_B_vcp unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_HAS_MLOS,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> has_mlos = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: has_mlos unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_HAS_RMS,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> has_rms = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: has_rms unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_HAS_BDDS,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> has_bdds = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: has_bdds unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_HAS_ARCHIVE_III,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> has_archive_III = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: has_archive_III unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_IS_ORDA,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> is_orda = ( int )get_value; 
  
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: is_orda unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_values( "",
                            ORPGSITE_DEA_PRODUCT_CODE,
                            &get_value );
  if( ret == 0 )
  {
    site_adapt -> product_code = ( int )get_value;
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: product_code unavailalable, abort task\n" );
    RPG_abort_task();
  }

  ret = RPG_ade_get_string_values( "",
                                   ORPGSITE_DEA_RPG_NAME,
                                   &get_string_value );
  if( ret >= 0 )
  {
    for( i = 0; i < SITE_INFO_CHAR_LENGTH; i++ )
    {
      site_adapt -> rpg_name[ i ] = ( char )get_string_value[ i ];
    }
  }
  else
  {
    LE_send_msg( GL_ERROR, "SITE_INFO: rpg_name unavailalable, abort task\n" );
    RPG_abort_task();
  }

#ifdef DEBUG
   LE_send_msg( GL_INFO, "SITE_INFO: rda_lat \t\t= %d\n", site_adapt -> rda_lat );
   LE_send_msg( GL_INFO, "SITE_INFO: rda_lon \t\t= %d\n", site_adapt -> rda_lon );
   LE_send_msg( GL_INFO, "SITE_INFO: rda_elev \t\t= %d\n", site_adapt -> rda_elev );
   LE_send_msg( GL_INFO, "SITE_INFO: rpg_id \t\t= %d\n", site_adapt -> rpg_id );
   LE_send_msg( GL_INFO, "SITE_INFO: wx_mode \t\t= %d\n", site_adapt -> wx_mode );
   LE_send_msg( GL_INFO, "SITE_INFO: def_mode_A_vcp \t\t= %d\n", site_adapt -> def_mode_A_vcp );
   LE_send_msg( GL_INFO, "SITE_INFO: def_mode_B_vcp \t\t= %d\n", site_adapt -> def_mode_B_vcp );
   LE_send_msg( GL_INFO, "SITE_INFO: has_mlos \t\t= %d\n", site_adapt -> has_mlos );
   LE_send_msg( GL_INFO, "SITE_INFO: has_rms \t\t= %d\n", site_adapt -> has_rms );
   LE_send_msg( GL_INFO, "SITE_INFO: has_bdds \t\t= %d\n", site_adapt -> has_bdds );
   LE_send_msg( GL_INFO, "SITE_INFO: has_archive_III \t\t= %d\n", site_adapt -> has_archive_III );
   LE_send_msg( GL_INFO, "SITE_INFO: is_orda \t\t= %d\n", site_adapt -> is_orda ); 
   LE_send_msg( GL_INFO, "SITE_INFO: product_code \t\t= %d\n", site_adapt -> product_code );
   LE_send_msg( GL_INFO, "SITE_INFO: rpg_name \t\t= %s\n", site_adapt -> rpg_name );
#endif

  return 0;
}
