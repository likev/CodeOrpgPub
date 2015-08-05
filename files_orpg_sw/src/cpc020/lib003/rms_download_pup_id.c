/**************************************************************************
   
   Module: rms_download_pup_id.c    
   
   Description:  This module takes the authorized user sent from RMS
   and places the edited information into the .
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2003/06/26 14:51:05 $
 * $Id: rms_download_pup_id.c,v 1.21 2003/06/26 14:51:05 garyg Exp $
 * $Revision: 1.21 $
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

#define ID_LENGTH   4
#define BYTE_2      2
#define BYTE_1      1


/*
* Static Globals
*/

extern   int   Num_users;
extern   int   User_line_index [USER_TBL_SIZE];
extern   int   User_class [USER_TBL_SIZE];
extern   int   rms_pup_user_lock;


/*
* Static Function Prototypes
*/

static int rms_edit_pup_id_info (UNSIGNED_BYTE *buf_ptr, short user, 
                                 int index, int line);

/**************************************************************************
   Description:  This function recieved the message and calls the routine to
   place the edited pup id information into the system.

   Input: pup_id_buf - Pointer to the message buffer.

   Output: Edited PUP id information.

   Returns: 0 = Successful edit.

   Notes:

   **************************************************************************/
int rms_rec_download_pup_id_command (UNSIGNED_BYTE *pup_id_buf) {
   int           ret, ret_val, i;
   int           wb_link;
   UNSIGNED_BYTE *pup_id_buf_ptr;
   short         line_num;
   short         line_index;
   int           user_num = 0;

   pup_id_buf_ptr = pup_id_buf;
   pup_id_buf_ptr += MESSAGE_START;

   /* Line number of the PUP to be edited */
   line_num = conv_shrt(pup_id_buf_ptr);
   pup_id_buf_ptr += PLUS_SHORT;

   /* Validation check */
   if ( line_num < 1 || line_num > 47){
      LE_send_msg(RMS_LE_LOG_MSG,"RMS: (%d) Not a valid line number", line_num);
      return (3);
   }

   /* Get lock status for user profile LB */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_USER_PROFILES, 0);

   /* If user profile LB locked by someone other than RMS cancel edit 
      and return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && rms_pup_user_lock == 0){
      LE_send_msg(RMS_LE_ERROR,"RMS: ORPGDAT_USER_PROFILES is being edited");
      return (24);
   }

   /* Adjust line number for logical numbering */
   line_num -= 1;

   if(line_num <0){
      LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get pup id line number %d", line_num);
      return (8);
   }

   /* Get wideband line number */
   wb_link = ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE;

   /* If wideband line selected cancel edit and return error code */
   if ( wb_link == line_num){
      LE_send_msg(RMS_LE_ERROR,"RMS: Wideband line selected (line %d)", wb_link);
      return (24);
   }

   /* Get line index */
   line_index = rms_get_line_index(line_num);

   if ( line_index < 0){
      LE_send_msg(RMS_LE_ERROR,
         "RMS: Unable to get line index for line number %d", line_num);
      return (-1);
   }

   /* Get the user profile for the PUP */
   ret = rms_read_user_info();

   /* Search for line to retrieve user information */
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
         "RMS: Unable to find user number for line number %d", line_index);
      return (-1);
   }/*End if */

   /* Change the PUP name in the profile */
   ret = rms_edit_pup_id_info( pup_id_buf, user_num, line_index, line_num);

   /* Unlock the PUP edit */
   ret_val = ORPGEDLOCK_clear_edit_lock(ORPGDAT_USER_PROFILES, 0);

   if ( ret_val == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL){
      LE_send_msg(RMS_LE_ERROR,"RMS: ORPGDAT_USER_PROFILES unlock unsuccessful.");
   }else{
      LE_send_msg(RMS_LE_ERROR,"RMS: ORPGDAT_USER_PROFILES unlocked.");
      rms_pup_user_lock = 0;
   }

   if ( ret != 1){
      LE_send_msg(RMS_LE_ERROR,"RMS: PUP id edit failed (ret (%d))\n", ret);
      return (ret);
   }

   return (0);
} /* End rms rec download pup id command */

/************************************************************************
    Function:     rms_edit_pup_id_info               
                            
    Description:  This function edits the authorized user info.         
                                                                       
 ************************************************************************/

static int rms_edit_pup_id_info( UNSIGNED_BYTE *buf_ptr, short user, 
                                 int index, int line) {

   char           temp_string[PASSWORD_LEN] = {NULL};
   Pd_distri_info *p_tbl;
   Pd_line_entry  *p_line_entry;
   LB_info        info;
   int            ret, i, num_lines;
   int            faa_redun, my_channel_status;
   int            other_channel_status, link_status;

    /* Get product distribution line information */
   p_tbl = ORPGGST_get_prod_distribution_info();

   if (p_tbl == NULL){
      LE_send_msg(RMS_LE_ERROR,"RMS: Unable to find product distribution table");
      return (-1);
   }

   /* Set pointer to beginning of line list */
   p_line_entry = (Pd_line_entry*) (p_tbl + 1);

   /* Get number of narrowband lines in use */
   num_lines = ORPGNBC_n_lines();

   /* Search for line to retrieve line information */
   for ( i = 0; i <= num_lines ; i++) {
      /* Get each line entry */

      if ( p_line_entry->line_ind == (line)) {
         break;
      } /*end if */

      p_line_entry = (p_line_entry + 1);

      if (i == num_lines)
         p_line_entry = NULL;

   } /* end loop */

   if(p_line_entry == NULL){
      LE_send_msg(RMS_LE_ERROR,"RMS: Unable to find line (%d)",line);
      return (8);
   }

   /* Set pointer past line number */
   buf_ptr += MESSAGE_START;
   buf_ptr += PLUS_SHORT;

   /* Convert the new PUP name */
   for (i=0; i<=ID_LENGTH -1; i++){
      conv_char (buf_ptr, &temp_string[i],PLUS_BYTE);
      buf_ptr += PLUS_BYTE;
   }

   if(p_line_entry->line_type == DEDICATED && User_class[user] !=  CLASS_IV){
      /* Place PUP id in output buffer */
      memcpy (p_line_entry->port_password,&temp_string, PASSWORD_LEN);
   }else {
      LE_send_msg(RMS_LE_ERROR,"RMS: Not a PUP line( %d)\n", index);
      return (24);
   }

   /* Get redundant type */
   faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);

   if (ORPGSITE_error_occurred()){
      faa_redun = 0;
      LE_send_msg(RMS_LE_ERROR,
          "RMS: Unable to get reundant type for download pup id");
   }

   /* If not a redundant site save the line info */
   if(faa_redun != ORPGSITE_FAA_REDUNDANT){

      /* Save prod distribution information */
      ret = ORPGDA_info (ORPGDAT_PROD_INFO,PD_LINE_INFO_MSG_ID,&info);

      if(ORPGGST_save_prod_distribution_info(p_tbl, info.size)!=1){
         LE_send_msg(RMS_LE_ERROR,"RMS: Unable to save prod distribution info");
         return (-1);
      }/* End if */
   }else {
      /* Get other channel state */
      other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

      /* Get this channel state */
      my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);

      /* Get ORPG to ORGP link status */
      link_status = ORPGRED_rpg_rpg_link_state();

      /*  If this channel active, or both channels inactive, or ORPG 
          to ORPG link down the save the information */
      if(my_channel_status == ORPGRED_CHANNEL_ACTIVE || 
         link_status == ORPGRED_CHANNEL_LINK_DOWN    ||
         (my_channel_status == ORPGRED_CHANNEL_INACTIVE && 
          other_channel_status == ORPGRED_CHANNEL_INACTIVE)){

          /* Save prod distribution information */
          ret = ORPGDA_info (ORPGDAT_PROD_INFO,PD_LINE_INFO_MSG_ID,&info);

          if(ORPGGST_save_prod_distribution_info(p_tbl, info.size)!=1){
             LE_send_msg(RMS_LE_ERROR,"RMS: Unable to save prod distribution info");
             return (-1);
          }/* End if */
      }else {
           LE_send_msg (RMS_LE_ERROR,
              "RMS: Unable to edit pup id in FAA redundant configuration");
              return (24);
      }/* End else */
   }/* End else */
      
   return (1);  
         
} /*End rms pup id info*/
