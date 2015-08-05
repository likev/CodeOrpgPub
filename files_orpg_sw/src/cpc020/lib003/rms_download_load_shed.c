/**************************************************************************

   Module:  rms_download_load_shed_command.c

   Description:  This module receives the new loadshed percentages from
   RMMS and places them in the system.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/05/25 13:30:04 $
 * $Id: rms_download_load_shed.c,v 1.21 2004/05/25 13:30:04 steves Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>
#include <orpgload.h>
#include <orpgedlock.h>
/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

/*
* Static Globals
*/
extern int rms_load_shed_lock;
/*
* Static Function Prototypes
*/

static int rms_edit_load_shed_info(ushort line_num, UNSIGNED_BYTE *buf);

/**************************************************************************
   Description:  This function reads the command from the message buffer.

   Input: load_shed_buf - Pointer to the message buffer.

   Output: Edited load shed warning and alarm values.

   Returns: 1 = Successful edit.

   Notes:

   **************************************************************************/

int rms_rec_download_load_shed_command (UNSIGNED_BYTE *load_shed_buf) {

   UNSIGNED_BYTE *load_shed_buf_ptr;
   int           ret;
   ushort        line_num;

   load_shed_buf_ptr = load_shed_buf;

   /* Place the pointer past the header */
   load_shed_buf_ptr += MESSAGE_START;

   /* Get the number of area to set loadshed */
   line_num = conv_ushrt(load_shed_buf_ptr);

   /* Validation check */
   if (line_num < 1 || line_num > 7 || line_num == 6){
      LE_send_msg(RMS_LE_ERROR,"Invalid line number (%d)", line_num);
      return (24);
   }

   /* Get lock status for the load shed LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_LOAD_SHED_CAT,LOAD_SHED_THRESHOLD_MSG_ID);

   /* If load shed LB locked by someone other than RMS cancel edit and 
      return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && rms_load_shed_lock == 0){
      LE_send_msg(RMS_LE_ERROR,"LOAD_SHED_THRESHOLD_MSG_ID is being edited");
      return (24);
   }

   /* Call routine to edit load shed */
   ret = rms_edit_load_shed_info(line_num, load_shed_buf);

   if (ret < 0){
      LE_send_msg(RMS_LE_ERROR,"Unable to download LOAD_SHED_THRESHOLD_MSG_ID.");
      return (ret);
   }
   
   return (0);
      
} /* End rms rec download load shed command */


/**************************************************************************
   Description:  This function replaces the alarm and warning percentages
   for load shed.
      
   Input: line_num - The line number to be edited.
      
   Output:  User profile information.
      
   Returns: Message sent = 1, Not sent = -1 
   
   Notes:  

   **************************************************************************/
static int rms_edit_load_shed_info(ushort line_num, UNSIGNED_BYTE *buf) {

   UNSIGNED_BYTE *buf_ptr;
   int           ret, ret_val;
   int           faa_redun, my_channel_status, other_channel_status, link_status;
   short         temp_warning, temp_alarm;
      
   /* Set pointer to beginnig of buffer */
   buf_ptr = buf;
   
   /* Unlock the load shed LB in case of an edit error */
   ret_val = ORPGEDLOCK_clear_edit_lock(ORPGDAT_LOAD_SHED_CAT,LOAD_SHED_THRESHOLD_MSG_ID);
      
   if ( ret_val == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"LOAD_SHED_THRESHOLD_MSG_ID unlock unsuccessful.");
   }else{
      LE_send_msg(RMS_LE_ERROR,"LOAD_SHED_THRESHOLD_MSG_ID unlocked.");
      rms_load_shed_lock = 0;
   }
   
   /* Place the pointer past message header */   
   buf_ptr += MESSAGE_START;
   buf_ptr += PLUS_SHORT;
      
   /* Get the edited warning value from message buffer*/
   temp_warning = conv_shrt(buf_ptr);
   buf_ptr += PLUS_SHORT;

   /* Get the edited alarm value from message buffer */
   temp_alarm = conv_shrt(buf_ptr);
   buf_ptr += PLUS_SHORT;

   /* Check if either load shed values are less than 1% or greater than 100% */
   if (temp_warning < 1 || temp_warning > 100 || temp_alarm < 1 || temp_alarm > 100){
      LE_send_msg(RMS_LE_ERROR, 
         "Settings must be between 1 and 100 (warning level (%d) alarm level (%d))", 
         temp_warning, temp_alarm);
      return (24);
   } /* End if */
             
   /* CPU Load Shed */   
   if(line_num == 1){

      /* ORPG doesn't do CPU loadshed */
      LE_send_msg(RMS_LE_ERROR, "CPU load shed no longer an option");
      return (24);

   }/* End if */

   /* Memory Load Shed */
   if(line_num == 2){

      /* ORPG doesn't do memory loadshed */
      LE_send_msg(RMS_LE_ERROR, "Memory load shed no longer an option");
      return (24);
   }/* End if */

   /* Distribution Load Shed */
   if(line_num == 3){

      /* Set Distribution warning threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_PROD_DIST, LOAD_SHED_WARNING_THRESHOLD,(int)temp_warning);

      /* Set Distribution alarm threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_PROD_DIST, LOAD_SHED_ALARM_THRESHOLD,(int)temp_alarm);

   }/* End if */
          
   /* Storage Load Shed */   
   if(line_num == 4){

      /* Set Storage warning threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_PROD_STORAGE, LOAD_SHED_WARNING_THRESHOLD,(int)temp_warning);
         
      /* Set Storage alarm threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_PROD_STORAGE, LOAD_SHED_ALARM_THRESHOLD,(int)temp_alarm);

   }/* End if */
   
   /* Input Buffer */   
   if(line_num == 5){

      /* Set RDA radial warning threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_RDA_RADIAL, LOAD_SHED_WARNING_THRESHOLD,(int)temp_warning);

      /* Set RPG radial warning threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_RPG_RADIAL, LOAD_SHED_WARNING_THRESHOLD,(int)temp_warning);

      /* Set RDA radial alarm threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_RDA_RADIAL, LOAD_SHED_ALARM_THRESHOLD,(int)temp_alarm);

      /* Set RPG radial alarm threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_RPG_RADIAL, LOAD_SHED_ALARM_THRESHOLD,(int)temp_alarm);

   }/* End if */
             
   /* Wideband Load Shed */   
   if(line_num == 7){

      /* Set Wideband warning threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_WB_USER, LOAD_SHED_WARNING_THRESHOLD,(int)temp_warning);   

      /* Set Wideband alarm threshold to edited value */
      ret = ORPGLOAD_set_data(LOAD_SHED_CATEGORY_WB_USER, LOAD_SHED_ALARM_THRESHOLD,(int)temp_alarm);   

   }/* End if */
         
   /* Get redundant type */
   faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);
   
   if (ORPGSITE_error_occurred()){
      faa_redun = 0;
      LE_send_msg(RMS_LE_ERROR,"Unable to get reundant type for download load shed");
   }

   /* If not a redundant site save edited information */
   if(faa_redun != ORPGSITE_FAA_REDUNDANT){

      /* Write the edited load shed information */
      if((ret = ORPGLOAD_write(LOAD_SHED_THRESHOLD_MSG_ID)) < 0 ){
         LE_send_msg(RMS_LE_ERROR, "Unable to save loadshed changes (ret %d)", ret);
         return (ret);
      }/* End if */
   }else{
      /* Get other channel state */
      other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

      /* Get this channel state */
      my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);

      /* Get ORPG to ORPG link status */
      link_status = ORPGRED_rpg_rpg_link_state();

      /* If this channel active, or both channels inactive, or ORPG to 
         ORPG link down then save the information */
      if(my_channel_status == ORPGRED_CHANNEL_ACTIVE || 
         link_status == ORPGRED_CHANNEL_LINK_DOWN    ||
         (my_channel_status == ORPGRED_CHANNEL_INACTIVE && 
          other_channel_status == ORPGRED_CHANNEL_INACTIVE)){

         /* Write the edited load shed information */
         if((ret = ORPGLOAD_write(LOAD_SHED_THRESHOLD_MSG_ID)) < 0 ){
            LE_send_msg(RMS_LE_ERROR, 
              "Unable to save loadshed changes (ret %d)", ret);
            return (ret);
         }/* End if */
      }else{
           LE_send_msg (RMS_LE_ERROR, 
              "Unable to edit load shed in FAA redundant configuration.");
      }/*End else */   
           
   } /* End else */
      
   return (0);
} /*End rms send user msg */
