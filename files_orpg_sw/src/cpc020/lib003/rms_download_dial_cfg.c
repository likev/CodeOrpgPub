/**************************************************************************

   Module:  rms_download_dial_cfg.c

   Description:  This module receives the edited informaiton from RMMS.  
   Upon receipt of this command the specified information is palced in the 
   ORPG system.

   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:00 $
 * $Id: rms_download_dial_cfg.c,v 1.24 2003/06/26 14:51:00 garyg Exp $
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

#define WORD_FOUR   4
#define WORD_SIX    6

/*
* Static Globals
*/

extern  int  Num_users;
extern  char *User_info;
extern  int  User_size;
extern  int  User_info_ptr   [USER_TBL_SIZE];
extern  int  User_line_index [USER_TBL_SIZE];
extern  int  User_id [USER_TBL_SIZE];
extern  int  rms_dial_user_lock;
extern  int  rms_dial_line_lock;


/*
* Static Function Prototypes
*/

/**************************************************************************
   Description:  This function downloads the information from the message buffer.

   Input: dial_cfg_buf - Pointer to the message buffer.

   Output:

   Returns:

   Notes:

   **************************************************************************/

int rms_rec_download_dial_cfg_command (UNSIGNED_BYTE *dial_cfg_buf) {

   UNSIGNED_BYTE  *dial_cfg_buf_ptr;
   int            ret, i;
   int            user_num = 0;
   int            num_lines;
   int            faa_redun, my_channel_status;
   int            other_channel_status, link_status;
   int            wb_link;
   short          line_num;
   short          time;
   short          uid;
   LB_info        info;
   Pd_distri_info *p_tbl;
   Pd_line_entry  *p_line_entry;
   Pd_user_entry  *user_profile;
   Pd_pms_entry   *user_entry;
   char           temp_method_str[5];
   char           temp_password_str[PASSWORD_LEN] = {NULL};
   char           sset[4] = "SSET";
   char           rset[4] = "RSET";
   char           itim[4] = "ITIM";
   char           comb[4] = "COMB";

   /* Set pointer to beginning of buffer */
   dial_cfg_buf_ptr = dial_cfg_buf;

   /* Get lock status of user profile LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_USER_PROFILES, 0);

   /* If LB locked by someone other than RMS cancel edit and return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && rms_dial_user_lock == 0){
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES is being edited");
      return (24);
   }

   /* Get lock status of line info LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID);

   /* If LB locked by someone other than RMS cancel edit and return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && rms_dial_line_lock == 0){
      LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID is being edited");
      return (24);
   }

   /* Go ahead and clear locks in case there is an edit problem */
   ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_USER_PROFILES, 0);

   if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlock unsuccessful.");
   }else{   
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlocked.");
      rms_dial_user_lock = 0;
   }
         
   ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID);
   
   if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID unlock unsuccessful.");
   }else{   
      LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID unlocked.");
      rms_dial_line_lock = 0;
   }
      
   /* Set the pointer past the header */
   dial_cfg_buf_ptr += MESSAGE_START;
   
   /* Place pointer past spare in message */
   dial_cfg_buf_ptr += PLUS_SHORT;
      
   /* Get the line number of the row to be edited */
   line_num = conv_shrt(dial_cfg_buf_ptr);      
   dial_cfg_buf_ptr += PLUS_SHORT;

   /* Validation check */
   if ( line_num < 1 || line_num > 47){
      LE_send_msg(RMS_LE_LOG_MSG,"(%d) Not a valid line number", line_num);
      return (3);
   }

   /* Adjust line number for logical numbering */
   line_num -= 1;

   if(line_num <0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get user configurarion line number %d", line_num);
      return (8);
   }

   /* Get wideband line number */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* If the wideband line has been selected cancel edit and return error message */
   if ( wb_link == line_num){
      LE_send_msg(RMS_LE_ERROR,"Wideband line selected (line %d)", wb_link);
      return (24);
   }
      
   /* Get narrowband information */
   p_tbl = ORPGGST_get_prod_distribution_info();
   
   if (p_tbl == NULL)
      return (-1);
      
   /* Set pointer to beginning of line list */   
   p_line_entry = (Pd_line_entry*) (p_tbl + 1);

   num_lines = ORPGNBC_n_lines();
   
   /* Search for line to retrive configuration */
   for ( i = 0; i <= num_lines; i++) {
      /* Get each line entry */
      if ( p_line_entry->line_ind == (line_num)) {
         break;
      } /*end if */

      p_line_entry = (p_line_entry + 1);

      if (i == num_lines)
         p_line_entry = NULL;

   } /* end loop */

   if(p_line_entry == NULL){
      LE_send_msg(RMS_LE_ERROR,"Unable to find dial up line (%d)",line_num);
      return (8);
   }

   /* Get user id */
   uid = rms_get_user_id(line_num);

   if ( uid < 0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get user id for line number %d", line_num);
      return (-1);
   }

   /* Get the user profiles */
   ret = rms_read_user_info();

   /* If user profile matches line number then set user number */
   for ( i=0; i < Num_users; i++){
      if(User_line_index[i] == 0 && User_id[i] == uid){
         user_num = i;
         break;
      }else {
         user_num = -1;
      }/*End else*/
   }/* End loop */

   if(user_num <0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to find user number for line number %d", line_num);
      return (-1);
   }/*End if */

   /* Set the pointer to the correct user profile */
   user_profile = (Pd_user_entry*) (User_info + User_info_ptr[user_num]);

   /* Set the pms list pointer using the user profile */
   user_entry = (Pd_pms_entry*) user_profile + user_profile->pms_list;

   /* Convert password */
   for (i=0; i<WORD_FOUR; i++){
      conv_char (dial_cfg_buf_ptr, &temp_password_str[i],PLUS_BYTE);
      dial_cfg_buf_ptr += PLUS_BYTE;
   }

   /*Edit password*/
   memcpy(p_line_entry->port_password,&temp_password_str,PASSWORD_LEN);

   /* Convert distribution method */
   for (i=0; i<WORD_FOUR; i++){
      conv_char (dial_cfg_buf_ptr, &temp_method_str[i],PLUS_BYTE);
      dial_cfg_buf_ptr += PLUS_BYTE;
   }

   /* Set user class to IV */
   user_profile->class = CLASS_IV;

   /*Edit distribution method*/
   user_entry->types = 0;

   if(strncmp(temp_method_str,sset,WORD_FOUR) == 0){
      user_entry->types |= PD_PMS_ROUTINE;
   };  /* SSET single set */

   if(strncmp(temp_method_str,rset,WORD_FOUR) == 0){
      user_entry->types |= PD_PMS_ROUTINE;
   };  /* RSET routine set */

   if(strncmp(temp_method_str,itim,WORD_FOUR) == 0){
      user_entry->types |= PD_PMS_ONETIME;
   };  /* 1TIM onetime request */

   if(strncmp(temp_method_str,comb,WORD_FOUR) == 0){
      user_entry->types |= PD_PMS_ROUTINE;
      user_entry->types |= PD_PMS_ONETIME;
   };  /* COMB combination */

   /* Edit the connect time */
   time = conv_shrt(dial_cfg_buf_ptr);
   dial_cfg_buf_ptr += PLUS_SHORT;

   user_profile->max_connect_time = 0;
   user_profile->max_connect_time = time;

   /* Get redundant type */
   faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);

   if (ORPGSITE_error_occurred()){
      faa_redun = 0;
      LE_send_msg(RMS_LE_ERROR,"Unable to get reundant type for download dial up config");
   }

   /* If not a redundant site then save prod info and user info */
   if(faa_redun != ORPGSITE_FAA_REDUNDANT){

      /* Save prod distribution information */

      ret = ORPGDA_info (ORPGDAT_PROD_INFO,PD_LINE_INFO_MSG_ID,&info);

      if(ORPGGST_save_prod_distribution_info(p_tbl, info.size)!=1){
         LE_send_msg(RMS_LE_ERROR,"Unable to save prod distribution info");
         return (-1);
      }/* End if */

      /* Save user profile information */
      ret = ORPGDA_write (ORPGDAT_PROD_INFO, (char *)User_info,
                          User_size, PD_USER_INFO_MSG_ID);

      if (ret <= 0) {
         LE_send_msg (GL_ORPGDA (ret),
            "ORPGGST: ORPGDA_write PD_USER_INFO_MSG_ID failed (ret %d)\n", ret);
         return (-1);
      } /*End if */

      /* Post the change to the user profile to update the system */
      ret = EN_post( ORPGEVT_PD_USER, NULL,0,0);

   }else {

      /* Get other channel state */
      other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

      /* Get this channel state */
      my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);

      /* Get the ORPG to ORPG link status */
      link_status = ORPGRED_rpg_rpg_link_state();

      /* If this channel is active, or both channels are inactive, or the 
         ORPG to ORPG link is down save prod info and user info */
      if(my_channel_status == ORPGRED_CHANNEL_ACTIVE || 
         link_status == ORPGRED_CHANNEL_LINK_DOWN    ||
         (my_channel_status == ORPGRED_CHANNEL_INACTIVE && 
          other_channel_status == ORPGRED_CHANNEL_INACTIVE)){

         /* Save prod distribution information */
         ret = ORPGDA_info (ORPGDAT_PROD_INFO,PD_LINE_INFO_MSG_ID,&info);

         if(ORPGGST_save_prod_distribution_info(p_tbl, info.size)!=1){
            LE_send_msg(RMS_LE_ERROR,"Unable to save prod distribution info");
            return (-1);
         }/* End if */

         /* Save user profile information */
         ret = ORPGDA_write (ORPGDAT_PROD_INFO, (char *)User_info,
                             User_size, PD_USER_INFO_MSG_ID);

         if (ret <= 0) {
            LE_send_msg (GL_ORPGDA (ret),
               "ORPGGST: ORPGDA_write PD_USER_INFO_MSG_ID failed (ret %d)\n", ret);
            return (-1);
         } /*End if */
      }else {

          LE_send_msg (RMS_LE_ERROR,
              "Unable to edit dial up config in FAA redundant configuration.");
      }/*End else */
   }/* End else */

   return (0);
   
} /*End rms rec download dial cfg command*/
