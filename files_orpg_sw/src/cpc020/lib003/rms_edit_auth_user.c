/**************************************************************************

   Module:  rms_rec_auth_user_command.c

   Description:  This module builds authorized user message to be sent to RMMS.  Upon
   receipt of this command the specified user message is sent to RMMS for
   editing.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:07 $
 * $Id: rms_edit_auth_user.c,v 1.19 2003/06/26 14:51:07 garyg Exp $
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

#define MAX_PASSWORD  6
#define STATUS_TYPE   34
#define MAX_ID        4
/*
* Static Globals
*/
extern int   Num_users;
extern int   User_line_index [USER_TBL_SIZE];
extern int   User_id [USER_TBL_SIZE];
extern char  User_password [USER_TBL_SIZE][PASSWORD_LEN];
extern uint  User_defined [USER_TBL_SIZE];
int          rms_auth_user_lock;

/*
* Static Function Prototypes
*/

static int rms_send_user_msg (ushort line_num);

/**************************************************************************
   Description:  This function reads the command from the message buffer.

   Input: auth_user_buf - Pointer to the message buffer.

   Output: Sends current user information to the FAA/RMMS.

   Returns: 0 = successful edit.

   Notes:

   **************************************************************************/

int rms_rec_auth_user_command (UNSIGNED_BYTE *auth_user_buf) {

   UNSIGNED_BYTE *auth_user_buf_ptr;
   int ret;
   int wb_link;
   ushort edit_flag;
   ushort line_num;

   /* Set pointer to beginning of buffer */
   auth_user_buf_ptr = auth_user_buf;

   /* Set the pointer past the header */
   auth_user_buf_ptr += MESSAGE_START;

   /* Determine if this is an edit or cancel edit */
   edit_flag = conv_ushrt(auth_user_buf_ptr);
   auth_user_buf_ptr += PLUS_SHORT;

   /* Get line number of user to be edited */
   line_num = conv_ushrt(auth_user_buf_ptr);
   auth_user_buf_ptr += PLUS_SHORT;

   /* Get line number of wideband */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* Edit auth user */
   if (edit_flag == 1){

      /* Validation check */
      if ( line_num < 1 || line_num > 47){
         LE_send_msg(RMS_LE_LOG_MSG,"RMS: (%d) Not a valid line number", line_num);
         return (3);
      }

      /* If wideband selected cancel edit and return error code */
      if ( wb_link == (line_num - 1)){
         LE_send_msg(RMS_LE_ERROR,"RMS: Wideband line selected (line %d)", wb_link);
         return (24);
      }

      /* Get lock status of user profile LB */
      ret = ORPGEDLOCK_get_edit_status(ORPGDAT_USER_PROFILES, 0);

      /* If user profiles vLB locked cancel edit and return error code */
      if ( ret == ORPGEDLOCK_EDIT_LOCKED){
         LE_send_msg(RMS_LE_ERROR,"RMS: ORPGDAT_USER_PROFILES is being edited");
         return (24);
      }

      ret = rms_send_user_msg (line_num);

      if (ret != 1)
         return (ret);

      return (0);
   } /* call send clutter */

   /* Cancel auth user edit */
   if (edit_flag == 2){

      /* Clear the edit lock for user profile */
      ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_USER_PROFILES, 0);

      if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
         LE_send_msg(RMS_LE_ERROR,"RMS: ORPGDAT_USER_PROFILES unlock unsuccessful.");
         return (ret);
      }
      else{
         LE_send_msg(RMS_LE_ERROR,"RMS: ORPGDAT_USER_PROFILES unlocked edit cancelled.");
         rms_auth_user_lock = 0;
      }

      return (0);
   } /* stop clutter edit */

   return (24);

} /* End rms rec auth user command */


/**************************************************************************
   Description:  This function builds and sends the message with the requested
   authorized user information.

   Input: line_num - The line number to be edited.

   Output:  User profile information.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/
static int rms_send_user_msg (ushort line_num) {

   UNSIGNED_BYTE msg_buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE *msg_buf_ptr;
   int           ret, i;
   ushort        num_halfwords;
   short         search_line;
   short         line_index;
   short         uid;
   int           user_num = 0;
   char          Yes = 'Y';
   char          No = 'N';
   char          blank = ' ';
   char          temp_string[4];

   /* Set pointer to beginning of buffer */
   msg_buf_ptr = msg_buf;

   /* Adjust line number for logical numbering */
   search_line = (short)(line_num - 1);

   if(search_line <0) {
      LE_send_msg(RMS_LE_ERROR,
          "RMS: Unable to get user configuration line number %d", search_line);
      return (8);
   }

   /* Get line index */
   line_index = rms_get_line_index(search_line);

   if ( line_index < 0) {
         LE_send_msg(RMS_LE_ERROR,
            "RMS: Unable to get line index for line number %d", search_line);
         return (-1);
   }

   /* Get user id */
   uid = rms_get_user_id(line_index);

   if ( uid < 0) {
      LE_send_msg(RMS_LE_ERROR,
          "RMS: Unable to get user id for line number %d", line_index);
   }

   /* Get the user profile for this line */
   ret = rms_read_user_info();

   if (ret < 0){
      LE_send_msg(RMS_LE_ERROR,
         "RMS: Unable to read user profiles( %d)\n", line_index);
      return (ret);
   }

   /* Search for user profile for line being edited */
   for (i = 0; i < Num_users; i++) {
      if (User_line_index[i] == line_index) {
         user_num = i;
         break;
      }
      else
         user_num = -1;
   }

   if (user_num < 0) {
      LE_send_msg(RMS_LE_ERROR,
         "RMS: Unable to find user profile( %d)\n", line_index);
      return (8);
   }

   /* Place pointer past header */
   msg_buf_ptr += MESSAGE_START;

   /* Put line number in output buffer */
   conv_ushort_unsigned(msg_buf_ptr,(ushort*)&line_num);
   msg_buf_ptr += PLUS_SHORT;

   /* Put user id in temporary string */
   sprintf(temp_string,"%d",User_id[user_num]);

   /* Put user id in output buffer */
   for (i = 0; i < MAX_ID; i++) {
      conv_char_unsigned (msg_buf_ptr, &temp_string[i], PLUS_BYTE);
      msg_buf_ptr += PLUS_BYTE;
   } /*End Loop */

   /* Put user password in output buffer */
   for (i=0; i<MAX_PASSWORD; i++) {
      conv_char_unsigned (msg_buf_ptr, &User_password[user_num][i], PLUS_BYTE);
      msg_buf_ptr += PLUS_BYTE;
   }/* End loop */

   /* Put override permission in output buffer */
   if (User_defined [user_num] & UP_CD_OVERRIDE) {
      conv_char_unsigned (msg_buf_ptr, &Yes, PLUS_BYTE);
      msg_buf_ptr += PLUS_BYTE;
      conv_char_unsigned (msg_buf_ptr, &blank, PLUS_BYTE);
      msg_buf_ptr += PLUS_BYTE;
   }/* End if */
   else {
      conv_char_unsigned (msg_buf_ptr, &No, PLUS_BYTE);
      msg_buf_ptr += PLUS_BYTE;
      conv_char_unsigned (msg_buf_ptr, &blank, PLUS_BYTE);
      msg_buf_ptr += PLUS_BYTE;
   }/* End else */

   /* Add the message terminators */
   add_terminator(msg_buf_ptr);
   msg_buf_ptr += PLUS_INT;

   /* Compute number of halfword in the message */
   num_halfwords = ((msg_buf_ptr - msg_buf) / 2);

   /* Add header to the message */
   ret = build_header(&num_halfwords, STATUS_TYPE, msg_buf, 0);

   if (ret != 1) {
      LE_send_msg (RMS_LE_ERROR,
         "RMS: RMS build header failed for authorized user");
      return (-1);
   }

   /* Send the message to the FAA/RMMS */
   ret = send_message(msg_buf,STATUS_TYPE,RMS_STANDARD);

   if (ret != 1) {
      LE_send_msg (RMS_LE_ERROR, 
          "RMS: Send message failed (ret %d) for authorized user", ret);
      return (-1);
   }

   /* Lock the user profile LB until a cancel edit or download messsage is received */
   ret = ORPGEDLOCK_set_edit_lock(ORPGDAT_USER_PROFILES, 0);

   if ( ret == ORPGEDLOCK_LOCK_UNSUCCESSFUL) {
      LE_send_msg(RMS_LE_ERROR,
         "RMS: ORPGDAT_USER_PROFILES lock unsuccessful.");
      return (ret);
   }
   else {
      LE_send_msg(RMS_LE_ERROR,"RMS: ORPGDAT_USER_PROFILES locked.");

      /* Set the locked flag */
      rms_auth_user_lock = 1;
   }

   return (1);
} /*End rms send user msg */

