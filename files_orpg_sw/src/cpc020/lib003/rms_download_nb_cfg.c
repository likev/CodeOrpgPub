/**************************************************************************

   Module:  rms_download_nb_cfg.c

   Description:  This module builds authorized user message to be sent to RMMS.  Upon
   receipt of this command the specified user message is sent to RMMS for
   editing.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:04 $
 * $Id: rms_download_nb_cfg.c,v 1.26 2003/06/26 14:51:04 garyg Exp $
 * $Revision: 1.26 $
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

#define WORD_THREE   3
#define WORD_FOUR    4
#define WORD_FIVE    5
#define WORD_SIX     6


/*
* Static Globals
*/

extern   int   Num_users;
extern   char  *User_info;
extern   int   User_size;
extern   int   User_info_ptr   [USER_TBL_SIZE];
extern   int   User_line_index [USER_TBL_SIZE];
extern   int   rms_nb_user_lock;
extern   int   rms_nb_line_lock;

/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  This function reads the command from the message buffer.

   Input: nb_cfg_buf - Pointer to the message buffer.

   Output: Edited nb config information.

   Returns: 0 = Successful edit.

   Notes:

   **************************************************************************/
int rms_rec_download_nb_cfg_command (UNSIGNED_BYTE *nb_cfg_buf) {

   UNSIGNED_BYTE  *nb_cfg_buf_ptr;
   int            ret, i;
   int            num_lines;
   int            faa_redun, my_channel_status;
   int            other_channel_status, link_status;
   short          line_num;
   short          row_num;
   ushort         rate;
   ushort         time;
   short          uid;
   short          line_index;
   LB_info        info;
   Pd_distri_info *p_tbl;
   Pd_line_entry  *p_line_entry;
   Pd_user_entry  *user_profile;
   Pd_pms_entry   *user_entry;
   int            user_num = 0;
   char           temp_class_str[7];
   char           temp_type_str[7];
   char           temp_password_str[PASSWORD_LEN] = {NULL};
   char           temp_method_str[4];
   int            user_type;
   int            wb_link;
   char           apup[6]  = "APUP  ";
   char           napup[6] = "NAPUP ";
   char           other[6] = "Other ";
   char           rpgop[6] = "RPGOP ";
   char           rgdac[6] = "RGDAC ";
   char           rfc[6]   = "RFC   ";
   char           dialin[6] = "DIALIN";
   char           dedic[6]  = "DEDIC ";
   char           dinout[6] = "DINOUT";
   char           sset[4] = "SSET";
   char           rset[4] = "RSET";
   char           itim[4] = "1TIM";
   char           comb[4] = "COMB";

   /* Set pointer to begining of buffer */
   nb_cfg_buf_ptr = nb_cfg_buf;

   /* Get lock status of user profile LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_USER_PROFILES, 0);

   /* If user profile LB locked by someone other than RMS cancel 
      edit and return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && rms_nb_user_lock == 0){
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES is being edited");
      return (24);
   }

   /* Get lock status of line info LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID);

   /* If line info LB locked by someone other than RMS cancel edit and 
      return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && rms_nb_line_lock == 0){
      LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID is being edited");
      return (24);
   }

   /* Clear RMS edit lock */
   ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_USER_PROFILES, 0);

   if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlock unsuccessful.");
   }else{
      LE_send_msg(RMS_LE_ERROR,"ORPGDAT_USER_PROFILES unlocked.");
      rms_nb_user_lock = 0;
   }

   /* Clear RMS edit lock */
   ret = ORPGEDLOCK_clear_edit_lock(ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID);

   if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID unlock unsuccessful.");
   }else{
      LE_send_msg(RMS_LE_ERROR,"PD_LINE_INFO_MSG_ID unlocked.");
      rms_nb_line_lock = 0;
   }

   /* Set the pointer past the header */
   nb_cfg_buf_ptr += MESSAGE_START;

   /* Get the row number of the row to be edited */
   row_num = conv_shrt(nb_cfg_buf_ptr);
   nb_cfg_buf_ptr += PLUS_SHORT;

   /* Get the line number of the row to be edited */
   line_num = conv_shrt(nb_cfg_buf_ptr);
   nb_cfg_buf_ptr += PLUS_SHORT;

   /* Validation check */
   if ( line_num < 1 || line_num > 47){
      LE_send_msg(RMS_LE_LOG_MSG,"(%d) Not a valid line number", line_num);
      return (3);
   }

   /* Adjust line number for logical numbering */
   line_num -= 1;

   if(line_num <0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get user configuration line numbe %d", line_num);
      return (8);
   }

   /* Get line number of wideband line */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* If wideband line has been selected cancel edit and return error code */
   if ( wb_link == line_num){
      LE_send_msg(RMS_LE_ERROR,"Wideband line selected (line %d)", wb_link);
      return (24);
   }

   /* Get narrowband information */
   p_tbl = ORPGGST_get_prod_distribution_info();

   if (p_tbl == NULL){
      LE_send_msg(RMS_LE_ERROR,"Unable to find product distribution table");
      return (-1);
   }

   /* Set pointer to beginning of line list */
   p_line_entry = (Pd_line_entry*) (p_tbl + 1);

   /* Get the number of narrowband lines in use */
   num_lines = ORPGNBC_n_lines();

   /* Search for line to retrive configuration */
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

   /* Get user id */
   uid = rms_get_user_id(line_num);

   if ( uid < 0){
      LE_send_msg(RMS_LE_ERROR,"Unable to get user id for line number %d", line_num);
   }

   /* Get line index */
   line_index = rms_get_line_index(line_num);

   if ( line_index < 0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get line index for line number %d", line_num);
      return (-1);
   }

   /* Set edited user type */
   if(p_line_entry->line_type == DEDICATED){
      user_type = UP_DEDICATED_USER;
   }
   else if (p_line_entry->line_type == DIAL_IN){
      user_type = UP_DIAL_USER;
      line_index = 0;
   }
   else if (p_line_entry->line_type == DIAL_OUT){
      user_type = UP_CLASS;
      line_index = 0;
   }

   /* Get the user profile for this line */
   ret = rms_read_user_info();

   /* Find the user profile for line to be edited */
   for ( i=0; i < Num_users; i++){

      if(User_line_index[i] == line_index){
         user_num = i;
         break;
      }else {
         user_num = -1;
      }/*End else*/
   }/* End loop */

   if(user_num <0){
      LE_send_msg(RMS_LE_ERROR,
         "Unable to find user number for line number %d", line_index);
      return (-1);
   }/*End if */

   /* Set pointer to correct user profile */
   user_profile = (Pd_user_entry*) (User_info + User_info_ptr[user_num]);

   /* Set pointer to pms list for selected user profile */
   user_entry = (Pd_pms_entry*) user_profile + user_profile->pms_list;

   /* Set pointer past spares */
   nb_cfg_buf_ptr += PLUS_INT;

   /* Convert line class */
   for (i=0; i<WORD_SIX ; i++){
      conv_char (nb_cfg_buf_ptr, &temp_class_str[i],PLUS_BYTE);
      nb_cfg_buf_ptr += PLUS_BYTE;
   }

   /* Get the baud rate */
   rate = conv_shrt(nb_cfg_buf_ptr);
   nb_cfg_buf_ptr += PLUS_SHORT;

   /* Convert line type */
   for (i=0; i<WORD_SIX; i++){
      conv_char (nb_cfg_buf_ptr, &temp_type_str[i],PLUS_BYTE);
      nb_cfg_buf_ptr += PLUS_BYTE;
   }

   /* Convert password */
   for (i=0; i<WORD_FOUR; i++){
      conv_char (nb_cfg_buf_ptr, &temp_password_str[i],PLUS_BYTE);
      nb_cfg_buf_ptr += PLUS_BYTE;
   }

   /* Convert distribution method */
   for (i=0; i<WORD_FOUR; i++){
      conv_char (nb_cfg_buf_ptr, &temp_method_str[i],PLUS_BYTE);
      nb_cfg_buf_ptr += PLUS_BYTE;
   }

   /* Get the connect time */
   time = conv_shrt(nb_cfg_buf_ptr);
   nb_cfg_buf_ptr += PLUS_SHORT;

   /*Edit baud rate*/
   p_line_entry->baud_rate = 0;
   p_line_entry->baud_rate = (int)rate;

   /*Edit password*/
   memcpy(p_line_entry->port_password,&temp_password_str, PASSWORD_LEN);

   /* Save user type of selected user profile */
   user_type = user_profile->up_type;

   /*Edit line type*/
   if(strncmp(temp_type_str,dialin,WORD_SIX) == 0){
      user_profile->up_type = UP_DIAL_USER;
      uid = user_profile->user_id;
      user_profile->line_ind = 0;
   } /* End if */

   if(strncmp(temp_type_str,dedic, WORD_FIVE) == 0){
      user_profile->up_type = UP_DEDICATED_USER;
      user_profile->line_ind = line_num;
   } /* End if */

   if(strncmp(temp_type_str,dinout,WORD_SIX) == 0){
      user_profile->up_type = UP_DIAL_USER;
      user_profile->line_ind = 0;
   } /* End if */

   /*Edit line class*/
   if((strncmp(temp_class_str,apup, WORD_FOUR) == 0) && 
         p_line_entry->line_type == DEDICATED){
      user_profile->class = CLASS_I;
   };

   if((strncmp(temp_class_str,napup, WORD_FIVE) == 0) && 
          p_line_entry->line_type == DIAL_IN){
      user_profile->class = CLASS_II;
   };

   if(strncmp(temp_class_str,other, WORD_FIVE) == 0){
      user_profile->class = CLASS_IV;
   };

   if((strncmp(temp_class_str,rpgop, WORD_FIVE) == 0) && 
          p_line_entry->line_type == DEDICATED){
      user_profile->class = RPGOP_CLASS;
   };

   if(strncmp(temp_class_str,rgdac,WORD_FIVE) == 0){
      user_profile->class = CLASS_V_RGDAC;
   };

   if(strncmp(temp_class_str,rfc, WORD_THREE) == 0){
      user_profile->class = CLASS_V_RFC;
   };

   /*Edit distribution method*/
   if ( user_profile->class == CLASS_IV){
      user_entry->types = 0;

      if(strncmp(temp_method_str,sset,WORD_FOUR) == 0){
         user_entry->types |= PD_PMS_ROUTINE;
      };

      if(strncmp(temp_method_str,rset,WORD_FOUR) == 0){
         user_entry->types |= PD_PMS_ROUTINE;
      };

      if(strncmp(temp_method_str,itim,WORD_FOUR) == 0){
         user_entry->types |= PD_PMS_ONETIME;
      };

      if(strncmp(temp_method_str,comb,WORD_FOUR) == 0){
         user_entry->types |= PD_PMS_ROUTINE;
         user_entry->types |= PD_PMS_ONETIME;
      };
   }

   /*Edit max connect time*/
   user_profile->max_connect_time = 0;
   user_profile->max_connect_time = time;

   /* Get redundant type */
   faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);

   if (ORPGSITE_error_occurred()){
      faa_redun = 0;
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get reundant type for download narrowband config");
   }

   /* If not a redudundant site save the user and line information */
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
      }

      /* Post the change to the user profile to update the system */
      ret = EN_post( ORPGEVT_PD_USER, NULL,0,0);
   }else {

      /* Get other channel state */
      other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

      /* Get this channel state */
      my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);

      /* Get ORPG to ORPG link status */
      link_status = ORPGRED_rpg_rpg_link_state();

      /* If this channel active, or both channels inactive, or 
         ORPG to ORPG link down, save the information */
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
            "Unable to edit narrowband config in FAA redundant configuration.");
      }/*End else */
   } /* End else */

   return (0);

} /*End rms send cfg msg */
