/**************************************************************************
   
   Module:  rms_rec_load_shed_command.c   
   
   Description:  This module builds load_shed message to be sent to RMMS.  Upon
   receipt of this command the specified load shed message is sent to RMMS for
   editing.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2007/01/05 18:02:17 $
 * $Id: rms_edit_load_shed.c,v 1.18 2007/01/05 18:02:17 garyg Exp $
 * $Revision: 1.18 $
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
#define STATUS_TYPE  25
/*
* Static Globals
*/
int rms_load_shed_lock;

/*
* Static Function Prototypes
*/

static int rms_send_load_shed_msg(ushort line_num);

/**************************************************************************
   Description:  This function reads the command from the message buffer.

   Input: load_shed_buf - Pointer to the message buffer.

   Output: Sends loadshed information to the FAA/RMMS.

   Returns: 0 = Successful edit.

   Notes:

   **************************************************************************/

int rms_rec_load_shed_command (UNSIGNED_BYTE *load_shed_buf) {

   UNSIGNED_BYTE *load_shed_buf_ptr;
   int           ret, i;
   ushort        edit_flag;
   ushort        line_num;
   char          wx_mode[2];

   /* Set pointer to beginning of buffer */
   load_shed_buf_ptr = load_shed_buf;

   /* Place pointer past header */
   load_shed_buf_ptr += MESSAGE_START;

   /* Get command from input buffer */
   edit_flag = conv_ushrt(load_shed_buf_ptr);
   load_shed_buf_ptr += PLUS_SHORT;

   /* Get wx mode */
   for (i=0; i<=1; i++){
      conv_char (load_shed_buf_ptr, &wx_mode[i],PLUS_BYTE);
      load_shed_buf_ptr += PLUS_BYTE;
   }

   /* Get line number */
   line_num = conv_ushrt(load_shed_buf_ptr);
   load_shed_buf_ptr += PLUS_SHORT;

   /* Edit loadshed */
   if (edit_flag == 1) {

      /* Validation check */
      if ((line_num < 3) || (line_num > 5)) {
         LE_send_msg(GL_ERROR, 
                     "Invalid Load Shed Category Number recv'd from RMS (%d)", line_num);
         return (24);
      }

      /* Get lock status of load shed LB */
      ret = ORPGEDLOCK_get_edit_status(ORPGDAT_LOAD_SHED_CAT,LOAD_SHED_THRESHOLD_MSG_ID);

      /* If load shed LB locked cancel edit and return error code */
      if ( ret == ORPGEDLOCK_EDIT_LOCKED){
         LE_send_msg(RMS_LE_ERROR,"LOAD_SHED_THRESHOLD_MSG_ID is being edited");
         return (24);
      }

      /* Send load shed information to the FAA/RMMS */
      ret = rms_send_load_shed_msg (line_num);

      if (ret != 1)
         return (ret);

      return (0);

   } /* call send load shed */

   /* Cancel load shed edit */
   if (edit_flag == 2){

      /* Unlock the load shed LB */
      ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_LOAD_SHED_CAT,LOAD_SHED_THRESHOLD_MSG_ID);

      if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
         LE_send_msg(RMS_LE_ERROR,"LOAD_SHED_THRESHOLD_MSG_ID unlock unsuccessful.");
         return (ret);
      }else{
         LE_send_msg(RMS_LE_ERROR,
            "LOAD_SHED_THRESHOLD_MSG_ID unlocked edit cancelled.");
         rms_load_shed_lock = 0;
      }

      return (0);
   } /* stop load shed edit */

   return (24);

} /* End rms rec load shed command */


/**************************************************************************
   Description:  This function builds and sends the message with the requested
   load shed information.

   Input: line_num - The line number to be edited.

   Output:  User profile information.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/
static int rms_send_load_shed_msg(ushort line_num){

   UNSIGNED_BYTE msg_buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE *msg_buf_ptr;
   int           ret;
   int           rda_alarm, rda_warn;
   int           rpg_alarm, rpg_warn;
   int           loadshed_value;
   ushort        num_halfwords;
   short         temp_short;

   msg_buf_ptr = msg_buf;

   /* Place pointer past header */
   msg_buf_ptr += MESSAGE_START;

   /* Put line number in output buffer */
   conv_short_unsigned(msg_buf_ptr,(short*)&line_num);
   msg_buf_ptr += PLUS_SHORT;

   /* CPU Load Shed - no longer a valid category */
   if (line_num == 1){

      /* ORPG doesn't load shed memory so send zero values to the FAA/RMMS */
      temp_short = 0;

      /* Put warning value in output buffer */
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;

      /* Put alarm value in output buffer */  
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
   } /* End if */
         
   /* Memory Load Shed */
   if (line_num == 2){

      /* ORPG doesn't load shed memory so send zero values to the FAA/RMMS */
      temp_short = 0;
         
      /* Put warning value in output buffer */            
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
      
      temp_short = 0;
         
      /* Put alarm value in output buffer */  
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
              
   }/* End if */
         
   /* Distribution Load Shed */   
   if(line_num == 3){
      
      temp_short = 0;

      /* Get the current distribution load shed warning value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_PROD_DIST, LOAD_SHED_WARNING_THRESHOLD,&loadshed_value);   
         
      temp_short = (short) loadshed_value;
                     
      /* Put warning value in output buffer */            
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
      
      temp_short = 0;

      /* Get the current distribution load shed alarm value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_PROD_DIST, LOAD_SHED_ALARM_THRESHOLD,&loadshed_value);   
         
      temp_short = (short) loadshed_value;
                  
      /* Put alarm value in output buffer */  
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
   }/* End if */
         
   /* Storage Load Shed */   
   if (line_num == 4){
         
      temp_short = 0;

      /* Get the current storage  load shed warning value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_PROD_STORAGE, LOAD_SHED_WARNING_THRESHOLD,&loadshed_value);   
         
      temp_short = (short) loadshed_value;
                  
      /* Put warning value in output buffer */            
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
      
      temp_short = 0;

      /* Get the current storage  load shed alarm value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_PROD_STORAGE, LOAD_SHED_ALARM_THRESHOLD,&loadshed_value);   
         
      temp_short = (short) loadshed_value;
         
      /* Put alarm value in output buffer */  
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
   }/* End if */
         
   /* Input Buffer Load Shed */   
   if (line_num == 5){
      
      temp_short = 0;

      /* Get the current RDA radial  load shed warning value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_RDA_RADIAL, LOAD_SHED_WARNING_THRESHOLD,&rda_warn);

      /* Get the current RPG radial  load shed warning value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_RPG_RADIAL, LOAD_SHED_WARNING_THRESHOLD,&rpg_warn);
         
      /* Take the lesser of the two values to send to RMMS, if equal send rpg*/
      if ( rda_warn < rpg_warn){
         temp_short = (short)rda_warn;
      }else{
          temp_short = (short)rpg_warn;
      }
                  
      /* Put warning value in output buffer */            
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
      
      temp_short = 0;
         
      /* Get the current RDA radial  load shed alarm value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_RDA_RADIAL, LOAD_SHED_ALARM_THRESHOLD,&rda_alarm);

      /* Get the current RPG radial  load shed alarm value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_RPG_RADIAL, LOAD_SHED_ALARM_THRESHOLD,&rpg_alarm);
         
      /* Take the lesser of the two values to send to RMMS, if equal send rpg*/
      if ( rda_alarm < rpg_alarm){
         temp_short = (short)rda_alarm;
      }else{
          temp_short = (short)rpg_alarm;
      }   
         
      /* Put alarm value in output buffer */  
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
   }/* End if */
         
   /* Wideband Load Shed - no longer a valid category */
   if (line_num == 7){
         
      temp_short = 0;

      /* Get the current wideband user  load shed warning value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_WB_USER, LOAD_SHED_WARNING_THRESHOLD,&loadshed_value);   
         
      temp_short = (short) loadshed_value;
                  
      /* Put warning value in output buffer */            
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
      
      temp_short = 0;

      /* Get the current wideband user  load shed alarm value */
      ret = ORPGLOAD_get_data(LOAD_SHED_CATEGORY_WB_USER, LOAD_SHED_ALARM_THRESHOLD,&loadshed_value);   
         
      temp_short = (short) loadshed_value;
                   
      /* Put alarm value in output buffer */  
      conv_short_unsigned(msg_buf_ptr,(short*)&temp_short);
      msg_buf_ptr += PLUS_SHORT;
   }/* End if */

   /* Add terminator to the message */
   add_terminator(msg_buf_ptr);
   msg_buf_ptr += PLUS_INT;

   /*Compute the number of halfwords in the message */
   num_halfwords = ((msg_buf_ptr - msg_buf) / 2);

   /* Add the header to the message */
   ret = build_header(&num_halfwords, STATUS_TYPE, msg_buf, 0);
   
   if (ret != 1){
      LE_send_msg (RMS_LE_ERROR, 
         "RMS build header failed for authorized user (%d)", ret);
      return (-1);
   }
      
   /* Send the load shed information to the FAA/RMMS */
   ret = send_message(msg_buf,STATUS_TYPE,RMS_STANDARD);

   if (ret != 1){
       LE_send_msg (RMS_LE_ERROR, "Send message failed (ret %d) for load shed", ret);
       return (-1);
   }

   /* Lock the load shed LB until RMS is finished editing */
   ret = ORPGEDLOCK_set_edit_lock(ORPGDAT_LOAD_SHED_CAT,LOAD_SHED_THRESHOLD_MSG_ID);
      
   if ( ret == ORPGEDLOCK_LOCK_UNSUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"LOAD_SHED_THRESHOLD_MSG_ID lock unsuccessful.");
      return (ret);
   }else{   
      LE_send_msg(RMS_LE_ERROR,"LOAD_SHED_THRESHOLD_MSG_ID locked.");
      rms_load_shed_lock = 1;
   }
   
   return (0);
   
} /*End rms send user msg */
