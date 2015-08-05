/**************************************************************************
   
   Module:  rms_rec_dial_cfg_command.c   
   
   Description:  This module builds authorized user message to be sent to RMMS.  Upon
   receipt of this command the specified user message is sent to RMMS for 
   editing.
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:11 $
 * $Id: rms_edit_dial_cfg.c,v 1.20 2003/06/26 14:51:11 garyg Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */


/*
* System Include Files/Local Include Files
*/

#include <rms_message.h>
#include <orpgedlock.h>


/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

#define WORD_FOUR    4
#define WORD_SIX     6
#define STATUS_TYPE  31


/*
* Static Globals
*/

int    rms_dial_user_lock;
int    rms_dial_line_lock;


/*
* Static Function Prototypes
*/

static int rms_send_dial_msg(ushort line_num);


/**************************************************************************
   Description:  This function reads the command from the message buffer.
      
   Input: dial_cfg_buf - Pointer to the message buffer.
      
   Output: Send the line configuration to the FAA/RMMS.

   Returns: 0 = Successful edit.
   
   Notes:  

   **************************************************************************/
   
int rms_rec_dial_cfg_command (UNSIGNED_BYTE *dial_cfg_buf) {

   UNSIGNED_BYTE *dial_cfg_buf_ptr;
   int           ret;
   int           wb_link;
   ushort        edit_flag;
   ushort        line_num;
   
   /* Set pointer to beginning of buffer */
   dial_cfg_buf_ptr = dial_cfg_buf;

   /* Set the pointer past the header */
   dial_cfg_buf_ptr += MESSAGE_START;

   /* Determine if this is an edit or cancel edit */
   edit_flag = conv_ushrt(dial_cfg_buf_ptr);
   dial_cfg_buf_ptr += PLUS_SHORT;

   /* Get line number of user to be edited */
   line_num = conv_ushrt(dial_cfg_buf_ptr);
   dial_cfg_buf_ptr += PLUS_SHORT;

   /* Get line number of wideband line */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* Edit command */
   if (edit_flag == 1){

      /* If wideband line selected cancel edit and return error code */
      if ( wb_link == (line_num - 1)){
         LE_send_msg(RMS_LE_ERROR,
            "Wideband line selected (line %d)", wb_link);
         return (24);
      }

      /* Validation check */
      if ( line_num < 1 || line_num > 47){
         LE_send_msg(RMS_LE_LOG_MSG,
            "(%d) Not a valid line number", line_num);
         return (3);
      }

      /* Get lock status of user profile LB */
      ret = ORPGEDLOCK_get_edit_status(ORPGDAT_USER_PROFILES, 0);

      /* If user profile LB locked then cancel edit and return error code */
      if ( ret == ORPGEDLOCK_EDIT_LOCKED){
         LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES is being edited");
         return (24);
      }

      /* Get lock status of line info LB */
      ret = ORPGEDLOCK_get_edit_status(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID);

      /* If line info LB locked then cancel edit and return error code */
      if ( ret == ORPGEDLOCK_EDIT_LOCKED){
         LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID is being edited");
         return (24);
      }

      /* Send the configuration information to the FAA/RMMS */
      ret = rms_send_dial_msg (line_num);

      if (ret != 1)
         return (ret);

      return (0);
   } /* call send dial cfg */

   /* Cancel edit */
   if (edit_flag == 2){

      /* Clear the edit lock on the user profile LB */
      ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_USER_PROFILES, 0);
      
      if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
         LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlock unsuccessful.");
      }else{   
         LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlocked edit cancelled.");
         rms_dial_user_lock = 0;
      }

      /* Clear the edit lock on the line info LB */
      ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID);
      
      if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
         LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID unlock unsuccessful.");
         return (ret);
      }else{   
         LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID unlocked edit cancelled.");
         rms_dial_line_lock = 0;
      }   
      return (0);
   } /* stop dial cfg edit */
   
   return (24);
      
} /* End rms edit dial cfg command */


/**************************************************************************
   Description:  This function builds and sends the message with the requested
   dial in user information.
      
   Input: line_num - The line number to be edited.
      
   Output:  User profile information.
      
   Returns: Message sent = 1, Not sent = -1 
   
   Notes:  

   **************************************************************************/
static int rms_send_dial_msg(ushort line_num){

   Nb_cfg        nb_cfg;
   Nb_cfg        *cfg_ptr;
   UNSIGNED_BYTE msg_buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE *msg_buf_ptr;
   char          sset[4] = "SSET";
   char          rset[4] = "RSET";
   char          itim[4] = "ITIM";
   char          comb[4] = "COMB";
   char          char_blank = ' ';
   int           ret, i;
   short         temp_short;
   short         uid;
   ushort        num_halfwords;
   ushort        blank = 0;
   short         search_line;

   /* Set the narrowband configuration pointer to the beginning 
      of the  nb_cfg array */
   cfg_ptr = &nb_cfg;

   /* Set the message pointer to the beginning of the message */
   msg_buf_ptr = msg_buf;

   /* Adjust the line number to account for logical number */
   search_line = (short)(line_num - 1);

   if(search_line <0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get user configuration line number %d", search_line);
      return (8);
   }

   /* Get the user id */
   uid = rms_get_user_id(search_line);

   if ( uid < 0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get user id for line number %d", search_line);
         return (-1);
   }

   /* Get the user configuration for this line */
   if(rms_get_cfg_ptr (cfg_ptr,search_line, uid) != 1){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get user configuration line number %d", search_line);
      return (8);
   }

   /* Place pointer past header */
   msg_buf_ptr += MESSAGE_START;

   /* Put spare in output buffer */
   conv_ushort_unsigned(msg_buf_ptr,&blank);
   msg_buf_ptr += PLUS_SHORT;

   /* Put line number in output buffer */
   conv_ushort_unsigned(msg_buf_ptr,(ushort*)&line_num);
   msg_buf_ptr += PLUS_SHORT;

   /* Put user password in output buffer */
   for (i=0; i<=WORD_FOUR -1; i++){
      conv_char_unsigned (msg_buf_ptr, &cfg_ptr->password[i], PLUS_BYTE);
      msg_buf_ptr += PLUS_BYTE;
   }/* End loop */

   /* Put distribution method in output buffer */
   if (cfg_ptr->distri_type & PD_PMS_ROUTINE &&  cfg_ptr->distri_type & PD_PMS_ONETIME){
      for (i=0; i<=WORD_FOUR - 1; i++){
         conv_char_unsigned (msg_buf_ptr, &comb[i], PLUS_BYTE);
         msg_buf_ptr += PLUS_BYTE;
      }
   }
   else if (cfg_ptr->distri_type & PD_PMS_ONETIME){
      for (i=0; i<=WORD_FOUR - 1; i++){
         conv_char_unsigned (msg_buf_ptr, &itim[i], PLUS_BYTE);
         msg_buf_ptr += PLUS_BYTE;
      }
   }
   else if (cfg_ptr->distri_type & PD_PMS_ROUTINE){
      for (i=0; i<=WORD_FOUR - 1; i++){
         conv_char_unsigned (msg_buf_ptr, &rset[i], PLUS_BYTE);
         msg_buf_ptr += PLUS_BYTE;
      }
   }
   else if (cfg_ptr->distri_type & PD_PMS_ROUTINE){
      for (i=0; i<=WORD_FOUR - 1; i++){
         conv_char_unsigned (msg_buf_ptr, &sset[i], PLUS_BYTE);
         msg_buf_ptr += PLUS_BYTE;
      }
   }
   else {
      for (i=0; i<=WORD_FOUR - 1; i++){
         conv_char_unsigned (msg_buf_ptr, &char_blank, PLUS_BYTE);
         msg_buf_ptr += PLUS_BYTE;
      }
   }/*End else*/

   /* Put time limit in output buffer */
   temp_short = cfg_ptr->connect_time;
   conv_ushort_unsigned(msg_buf_ptr,(ushort*)&temp_short);
   msg_buf_ptr += PLUS_SHORT;

   /* Add terminator to the message */
   add_terminator(msg_buf_ptr);
   msg_buf_ptr += PLUS_INT;

   /* Compute the number of halfwords in the message */
   num_halfwords = ((msg_buf_ptr - msg_buf) / 2);

   /* Add the header to the message */
   ret = build_header(&num_halfwords, STATUS_TYPE, msg_buf, 0);

   if (ret != 1){
      LE_send_msg (RMS_LE_ERROR,
         "RMS build header failed for edit dial in user");
      return (-1);
   }

   /* Send the message to the FAA/RMMS */
   ret = send_message(msg_buf,STATUS_TYPE,RMS_STANDARD);

   if (ret != 1){
      LE_send_msg (RMS_LE_ERROR, 
         "Send message failed (ret %d) for dial in user", ret);
      return (-1);
   }

   /* Lock the user profile LB until RMS is finished editing */
   ret = ORPGEDLOCK_set_edit_lock(ORPGDAT_USER_PROFILES, 0);

   if ( ret == ORPGEDLOCK_LOCK_UNSUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES lock unsuccessful.");
      return (ret);
   }else{
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES locked.");
      rms_dial_user_lock = 1;
   }

   /* Lock the line info LB until RMS is finished editing */
   ret = ORPGEDLOCK_set_edit_lock(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID);

   if ( ret == ORPGEDLOCK_LOCK_UNSUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID lock unsuccessful.");
      rms_dial_user_lock = 0;
      return (ret);
   }else{   
      LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID locked.");
      rms_dial_line_lock = 1;
   }
   
   return (1);
} /*End rms send dial msg */
