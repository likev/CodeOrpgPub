/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 23:12:53 $
 * $Id: mode_select_callback_fx.c,v 1.11 2007/01/30 23:12:53 ccalvert Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/***************************************************************************/


/*** System Include Files ***/
#include <alg_adapt.h>


/*** Local Include Files ***/
#include <mode_select.h>


#ifndef C_NO_UNDERSCORE

#ifdef SUNOS
#define mode_select_callback_fx mode_select_callback_fx_
#endif

#ifdef LINUX
#define mode_select_callback_fx mode_select_callback_fx__
#endif

#endif


int mode_select_callback_fx( void *common_block_address ){

   int ret = -1;			/* return status */
   double value;			/* temp value */
   Mode_select_entry_t *mode_select = (Mode_select_entry_t *) common_block_address;

   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".precip_mode_zthresh", &value );
   if( ret >= 0 )
      mode_select->precip_mode_zthresh = (float) value;
   
   else{

      LE_send_msg( GL_ERROR, "MODE_SELECT: precip_mode_zthresh unavailable, abort task\n" );
      RPG_abort_task();

   }

   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".precip_mode_area_thresh", &value );
   if( ret >= 0 )
      mode_select->precip_mode_area_thresh = (int) value;
   
   else{

      LE_send_msg( GL_ERROR, "MODE_SELECT: precip_mode_area_thresh unavailable, abort task\n" );
      RPG_abort_task();

   } 

   /* Get VCP select data elements */
   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".auto_mode_A", &value );
   if( ret >= 0 )
      mode_select->auto_mode_A = (int) value;
   
   else{

      LE_send_msg( GL_ERROR, 
          "MODE_SELECT: auto_mode_A unavailable, abort task\n" );
      RPG_abort_task();

   }

   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".auto_mode_B", &value );
   if( ret >= 0 )
      mode_select->auto_mode_B = (int) value;
   
   else{

      LE_send_msg( GL_ERROR, 
         "MODE_SELECT: auto_mode_B unavailable, abort task\n" );
      RPG_abort_task();

   }

   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".mode_B_selection_time", &value );
   if( ret >= 0 )
      mode_select->mode_B_selection_time = (int) value;
     
   else{

      LE_send_msg( GL_ERROR, "MODE_SELECT: mode_B_selection_time unavailable, abort task\n" );
      RPG_abort_task();

   }

   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".ignore_mode_conflict", &value );
   if( ret >= 0 )
      mode_select->ignore_mode_conflict = (int) value;
     
   else{

      LE_send_msg( GL_ERROR, "MODE_SELECT: ignore_mode_conflict unavailable, abort task\n" );
      RPG_abort_task();

   }

   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".mode_conflict_duration", &value );
   if( ret >= 0 )
      mode_select->mode_conflict_duration = (int) value;
     
   else{

      LE_send_msg( GL_ERROR, "MODE_SELECT: mode_conflict_duration unavailable, abort task\n" );
      RPG_abort_task();

   }

   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".use_hybrid_scan", &value );
   if( ret >= 0 )
      mode_select->use_hybrid_scan = (int) value;

   else{

      LE_send_msg( GL_ERROR, "MODE_SELECT: use_hybrid_scan unavailable, abort task\n" );
      RPG_abort_task();

   }

   ret = RPG_ade_get_values( MODE_SELECT_DEA_NAME, ".clutter_thresh", &value );
   if( ret >= 0 )
      mode_select->clutter_thresh = (int) value;

   else{

      LE_send_msg( GL_ERROR, "MODE_SELECT: clutter_thresh unavailable, abort task\n" );
      RPG_abort_task();

   }


   return 0;
}
