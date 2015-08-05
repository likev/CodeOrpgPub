/**************************************************************************
   
   Module: rms_download_auth_user.c    
   
   Description:  This module takes the authorized user sent from RMS
   and places the edited information into the .
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:50:56 $
 * $Id: rms_download_auth_user.c,v 1.24 2003/06/26 14:50:56 garyg Exp $
 * $Revision: 1.24 $
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
#define ID_LENGTH   6
#define BYTE_2      2
#define BYTE_1      1
#define MAX_ID      4
/*
* Static Globals
*/
extern int   Num_users;
extern char  *User_info;
extern int   User_size;
extern int   User_line_index [USER_TBL_SIZE];
extern int   User_info_ptr   [USER_TBL_SIZE];
extern int   rms_auth_user_lock;
/*
* Static Function Prototypes
*/
/* Function to edit the authorized user info */

static int rms_edit_auth_user_info( UNSIGNED_BYTE *buf_ptr, int user);

/**************************************************************************
   Description:  This function recieved the message and calls the routine to
   place the edited censor zone information into the system.

   Input: auth_user_buf - Pointer to the message buffer.

   Output: Authorized user information for the requested line.

   Returns: != 0 Indicates an error condition

   Notes:

   **************************************************************************/
int rms_rec_download_auth_user_command (UNSIGNED_BYTE *auth_user_buf) {

   int           ret, ret_val, i;
   int           user_num = 0;
   int           wb_link;
   short         line_num;
   short         line_index;
   short         uid;
   UNSIGNED_BYTE *auth_user_buf_ptr;

   /* Set pointer equal to beginning of buffer */
   auth_user_buf_ptr = auth_user_buf;

   /* Place pointer past header */
   auth_user_buf_ptr += MESSAGE_START;

   /* Get line number to be edited */
   line_num = conv_shrt(auth_user_buf_ptr);
   auth_user_buf_ptr += PLUS_SHORT;

   /* Validation check for line number */
   if ( line_num < 1 || line_num > 47){
      LE_send_msg(RMS_LE_LOG_MSG,"(%d) Not a valid line number", line_num);
      return (3);
   }

   /* Get lock status for LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_USER_PROFILES, 0);

   /* If LB locked, print message and return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && rms_auth_user_lock == 0){
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES is being edited");
      return (24);
   }

   /* Reduce line number by one to account for logical numbering of ORPG lines */
   line_num -= 1;

   if(line_num <0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get auth user line number %d", line_num);
      return (8);
   }

   /* Get line number of wideband */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* If wideband line selected cancel the edit and return error code */
   if ( wb_link == line_num){
      LE_send_msg(RMS_LE_ERROR,"Wideband line selected (line %d)", wb_link);
      return (24);
   }

   /* Get line index of selected line */
   line_index = rms_get_line_index(line_num);

   if ( line_index < 0){
         LE_send_msg(RMS_LE_ERROR,
            "Unable to get line index for line number %d", line_num);
         return (-1);
   }

   /* Get user id of selected line */
   uid = rms_get_user_id(line_index);

   if ( uid < 0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get user id for line number %d", line_index);
   }

   /* Get the user profile for the input line number */
   ret = rms_read_user_info();

   if (ret < 0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to read user profiles( %d)\n", line_index);
      return (8);
   }

   /* If line index found them set user profile array number */
   for ( i=0; i < Num_users; i++){
      if(User_line_index[i] == line_index){
         user_num = i;
         break;
      }
      else
         /* Unable to match line index with user profile array */
         user_num = -1;
   }

   if (user_num < 0){
      LE_send_msg(RMS_LE_ERROR,"Unable to find user profile( %d)\n", line_index);
      return (8);
   }

   /* Send the authorized user information */
   ret = rms_edit_auth_user_info(auth_user_buf, user_num);

   /* Unlock the auth user edit */
   ret_val = ORPGEDLOCK_clear_edit_lock(ORPGDAT_USER_PROFILES, 0);

   if ( ret_val == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlock unsuccessful.");
   }
   else{
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlocked.");
      rms_auth_user_lock = 0;
   }

   if ( ret != 1){
      LE_send_msg(RMS_LE_ERROR,
        "Authorized user edit failed (ret (%d))\n", ret);
      return (-1);
   }

   return (0);

} /* End rms rec download auth user command */

/************************************************************************
    Function:     rms_edit_auth_user_info

    Description:  This function edits the authorized user info.

 ************************************************************************/

static int rms_edit_auth_user_info(UNSIGNED_BYTE *buf_ptr, int user){

   char          temp_char;
   char          line_end = '\0';
   char          tmp_string[4];
   Pd_user_entry *user_profile;
   int           ret,i;
   int           faa_redun, my_channel_status, other_channel_status, link_status;


   /* Set pointer past header and line number */
   buf_ptr += MESSAGE_START;
   buf_ptr += PLUS_SHORT;

   /* Set the user profile pointer to the user being edited */
   user_profile = (Pd_user_entry*) (User_info + User_info_ptr [user]);

   /* Get user id in temporary string*/
   for(i=0; i < MAX_ID; i++){
      conv_char (buf_ptr, &tmp_string[i],PLUS_BYTE);
      buf_ptr += PLUS_BYTE;
   }

   /* Convert temporary id string to an integer*/
   user_profile->user_id = atoi(tmp_string);

   /* Put the password into a string */
   for (i=0; i<=ID_LENGTH; i++){
      if ( i == ID_LENGTH){
         strcat(&user_profile->user_password[i], &line_end);
      }
      else{
         conv_char (buf_ptr, &user_profile->user_password[i],PLUS_BYTE);
         buf_ptr += PLUS_BYTE;
      }
   }

   /* Get override command */
   conv_char (buf_ptr, &temp_char, PLUS_BYTE_2);

   /* Set override command to upper case */
   temp_char = toupper(temp_char);

   /* If override command determine if already set if not set it */
   if (temp_char == 'Y') {
      user_profile->cntl |= UP_CD_OVERRIDE;
      user_profile->defined |= UP_CD_OVERRIDE;
   }

   /* If cancel override command determine if already cleared if not clear it */
   if (temp_char == 'N') {
      user_profile->cntl &= ~UP_CD_OVERRIDE;
      user_profile->defined &= ~UP_CD_OVERRIDE;
   }

   /* Get redundant type */
   faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);

   if (ORPGSITE_error_occurred()){
      faa_redun = 0;
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get reundant type for download authorized user");
   }

   if(faa_redun != ORPGSITE_FAA_REDUNDANT){

      /* Save the new user profile */

      ret = ORPGDA_write (ORPGDAT_PROD_INFO, (char *)User_info,
               User_size, PD_USER_INFO_MSG_ID);
          if (ret <= 0) {
             LE_send_msg (GL_ORPGDA (ret),
                "ORPGDA_write PD_USER_INFO_MSG_ID failed (ret %d)\n", ret);
          }

          /* Post the change to the user profile to update the system */
          ret = EN_post( ORPGEVT_PD_USER, NULL,0,0);

   }else {

       /* Get the other ORPG channel's state */
       other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

       /* Get this ORPG channel's state */
       my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);

       /* Get the ORPG to ORPG link status */
       link_status = ORPGRED_rpg_rpg_link_state();

       /* If this channel is active, or the ORPG to ORPG link is down, 
          or both ORPG channels are down, save the information */
       if(my_channel_status == ORPGRED_CHANNEL_ACTIVE || 
          link_status == ORPGRED_CHANNEL_LINK_DOWN    ||
          (my_channel_status == ORPGRED_CHANNEL_INACTIVE && 
           other_channel_status == ORPGRED_CHANNEL_INACTIVE)){

          /* Save the new user profile */
          ret = ORPGDA_write (ORPGDAT_PROD_INFO, (char *)User_info,
                              User_size, PD_USER_INFO_MSG_ID);

          if (ret <= 0) {
            LE_send_msg (GL_ORPGDA (ret),
            "ORPGDA_write PD_USER_INFO_MSG_ID failed (ret %d)\n", ret);
            return (-1);
          }

          /* Post the change to the user profile to update the system */
          ret = EN_post( ORPGEVT_PD_USER, NULL,0,0);
       }else {
           LE_send_msg (RMS_LE_ERROR,
             "Unable to edit auth user in FAA redundant configuration.");
       }/*End else */
    } /* End else */

    return (1);

} /*End rms edit auth user info*/
