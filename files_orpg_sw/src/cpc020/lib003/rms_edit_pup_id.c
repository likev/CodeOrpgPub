/**************************************************************************

   Module:  rms_rec_pup_id_command.c

   Description:  This module builds pup id message to be sent to RMMS.  Upon
   receipt of this command the pup id message is sent to RMMS for 
   editing.
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:16 $
 * $Id: rms_edit_pup_id.c,v 1.19 2003/06/26 14:51:16 garyg Exp $
 * $Revision: 1.19 $
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

#define MAX_ID  4
#define STATUS_TYPE  16


/*
* Task Level Globals
*/

extern   int   Num_users;
extern   int   User_line_index [USER_TBL_SIZE];
extern   int   User_class [USER_TBL_SIZE];
int            rms_pup_user_lock;


/*
* Static Function Prototypes
*/

static int rms_send_pup_id(ushort line_num);

/**************************************************************************
   Description:  This function reads the command from the message buffer.
      
   Input: pup_id_buf - Pointer to the message buffer.
      
   Output: Sends PUP information to the FAA/RMMS.

   Returns: 0 = Successful edit.
   
   Notes:  

   **************************************************************************/
   
int rms_rec_pup_id_command (UNSIGNED_BYTE *pup_id_buf) {

   UNSIGNED_BYTE *pup_id_buf_ptr;
   int           ret;
   int           wb_link;
   ushort        edit_flag;
   ushort        line_num;

   /* Set pointer to beginning of buffer */
   pup_id_buf_ptr = pup_id_buf;

   /* Place pointer past header */
   pup_id_buf_ptr += MESSAGE_START;

   /* Get edit command */
   edit_flag = conv_ushrt(pup_id_buf_ptr);
   pup_id_buf_ptr += PLUS_SHORT;

   /* Get line number */
   line_num = conv_ushrt(pup_id_buf_ptr);
   pup_id_buf_ptr += PLUS_SHORT;

   /* get wideband line number */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* edit command */
   if (edit_flag == 1) {
      /* Validation check */
      if ( line_num < 1 || line_num > 47){
         LE_send_msg(RMS_LE_LOG_MSG,
             "(%d) Not a valid line number", line_num);
         return (3);
      }

      /* If wideband line selected cancel edit and return error code */
      if ( wb_link == (line_num - 1)){
         LE_send_msg(RMS_LE_ERROR,"Wideband line selected (line %d)", wb_link);
         return (24);
      }

      /* Get lock status of user profile LB */
      ret = ORPGEDLOCK_get_edit_status(ORPGDAT_USER_PROFILES, 0);

      /* if user profile LB locked cancel edit and return error code */
      if ( ret == ORPGEDLOCK_EDIT_LOCKED){
         LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES is being edited");
         return (24);
      }

      /* Send PUP id information */
      ret = rms_send_pup_id (line_num);

      if (ret != 1)
         return (ret);

      return (0);
   } /* call send pup id */

   /* Cancel edit */
   if (edit_flag == 2){

      /* Unlock user profile LB */
      ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_USER_PROFILES, 0);

      if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
         LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlock unsuccessful.");
         return (ret);
      }else{
         LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlocked edit cancelled.");
         rms_pup_user_lock = 0;
      }

      return (0);
   } /* stop pup id edit */

   return (24);

} /* End rms rec pup id command */


/**************************************************************************
   Description:  This function builds and sends the message with the requested
   pup id information.

   Input: line_num - The line number to be edited.

   Output:  User profile information.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/
static int rms_send_pup_id(ushort line_num){

   Pd_distri_info *p_tbl;
   Pd_line_entry  *p_line_entry;
   UNSIGNED_BYTE  msg_buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE  *msg_buf_ptr;
   int            ret, i;
   int            num_lines;
   ushort         num_halfwords;
   short          line_index;
   int            user_num = 0;

   /* Set pointer to the beginning  of the buffer */
   msg_buf_ptr = msg_buf;

   /* Adjust line number to account for logical numbering */
   if ((line_num - 1) < 0) {
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get user configuration line number %d", line_num);
      return (8);
   }
   else
      line_num -= 1;

   /* Get narrowband information */
   p_tbl = ORPGGST_get_prod_distribution_info();

   if (p_tbl == NULL){
      LE_send_msg(RMS_LE_ERROR,"Unable to find product distribution table");
      return (-1);
   }

   /* Set pointer to beginning of line list */
   p_line_entry = (Pd_line_entry*) (p_tbl + 1);

   /* Get number of lines in use */
   num_lines = ORPGNBC_n_lines();

   /* Search for line to retrieve configuration */
   for ( i = 0; i <= num_lines ; i++) {
      /* Get each line entry */
      if ( p_line_entry->line_ind == (line_num)) {
         break;
      } /*end if */

      p_line_entry = (p_line_entry + 1);

      if (i == num_lines)
         p_line_entry = NULL;

   } /* end loop */

   if(p_line_entry == NULL){
      LE_send_msg(RMS_LE_ERROR,"Unable to find line (%d)",line_num);
      return (8);
   }

   /* Get line index */
   line_index = rms_get_line_index(line_num);

   if ( line_index < 0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get line index for line number %d", line_num);
      return (-1);
   }

   /* Get user profile */
   ret = rms_read_user_info();

   if (ret < 0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to read user profiles( %d)\n", line_num);
      return (ret);
   }

   /* Find user profile of the requested line */
   for ( i=0; i < Num_users; i++){
      if(User_line_index[i] == line_index){
         user_num = i;
         break;
      }
      else
         user_num = -1;
   }

   if (user_num < 0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to find user profile( %d)\n", line_index);
      return (ret);
   }

   /* Place pointer past header */
   msg_buf_ptr += MESSAGE_START;

   /* reset line number */
   line_num ++;

   /* Place line number in output buffer */
   conv_ushort_unsigned(msg_buf_ptr,(ushort*)&line_num);
   msg_buf_ptr += PLUS_SHORT;

   /* If line is dedicated and not CLASS IV then get the PUP id */
   if(p_line_entry->line_type == DEDICATED && User_class[user_num] !=  CLASS_IV){

      /* Place PUP id in output buffer */
      for (i=0; i<=MAX_ID -1; i++){
         conv_char_unsigned (msg_buf_ptr, &p_line_entry->port_password[i], PLUS_BYTE);
         msg_buf_ptr += PLUS_BYTE;
      }
   }else {
      LE_send_msg(RMS_LE_ERROR,"Not an APUP line( %d)\n", line_index);
      return (24);
   }/* End else */

   /* Add terminator to the message */
   add_terminator(msg_buf_ptr);
   msg_buf_ptr += PLUS_INT;

   /* compute the number of halfwords in the message */
   num_halfwords = ((msg_buf_ptr - msg_buf) / 2);

   /* Add the header to the message */
   ret = build_header(&num_halfwords, STATUS_TYPE, msg_buf, 0);

   if (ret != 1){
      LE_send_msg (RMS_LE_ERROR, "RMS build header failed for PUP id");
      return (-1);
   }

   /* Send the message to the FAA/RMMS */
   ret = send_message(msg_buf,STATUS_TYPE,RMS_STANDARD);

   if (ret != 1){
      LE_send_msg (RMS_LE_ERROR, 
          "Send message failed (ret %d) for PUP id", ret);
      return (-1);
   }

   /* Lock the user profile LB until RMS is done editing */
   ret = ORPGEDLOCK_set_edit_lock(ORPGDAT_USER_PROFILES, 0);

   if ( ret == ORPGEDLOCK_LOCK_UNSUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES lock unsuccessful.");
      return (ret);
   }else{
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES locked.");
      rms_pup_user_lock = 1;
   }

   return (1);
} /*End rms send pup id */
