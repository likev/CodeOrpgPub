/**************************************************************************

   Module: rms_rec_download_clutter_command.c

   Description:  This module takes the clutter zones sent from RMS
   and places the edited information into the ORPGDAT_CLUTTERMAP LB.

    Assumptions:

**************************************************************************/
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/03/04 19:35:55 $
 * $Id: rms_download_clutter.c,v 1.26 2005/03/04 19:35:55 steves Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 */


   /* System Include Files/Local Include Files */

#include <rms_message.h>
#include <orpgedlock.h>


   /* Constant Definitions/Macro Definitions/Type Definitions */

#define MAX_CLUTTER_LENGTH   242
#define MAX_CLUTTER_ZONES    14


   /* Static Globals */

extern int clutter_zones_locked;


   /* Static Function Prototypes */

static int rms_edit_legacy_clutter_zone_info(int num, UNSIGNED_BYTE *buf_ptr);
static int rms_edit_orda_clutter_zone_info(int num, UNSIGNED_BYTE *buf_ptr);


/**************************************************************************
   Description:  This function recieved the message and calls the routine to
   place the edited censor zone information into the system.

   Input: rda_clutter_buf - Pointer to the message buffer.

   Output: Edited Clutter Censor Zones.

   Returns:

   Notes:

   **************************************************************************/
int rms_rec_download_clutter_command (UNSIGNED_BYTE *rda_clutter_buf) {

   UNSIGNED_BYTE *rda_clutter_buf_ptr;
   int           ret, ret_val;
   int           return_code = 0;
   short         num;

      /* Place pointer at beginning of buffer */
   rda_clutter_buf_ptr = rda_clutter_buf;

      /* Place pointer past header */
   rda_clutter_buf_ptr += MESSAGE_START;

      /* Get file number of clutter zone */
   num = conv_shrt(rda_clutter_buf_ptr);
   rda_clutter_buf_ptr += PLUS_SHORT;

      /* Validation checks */
   if ( num  < 1 ){
      LE_send_msg(RMS_LE_ERROR,
          "Download clutter file number less than one (file num = %d)", num);
      return_code = 24;
   }

   if ( num  > MAX_CLTR_FILES){
      LE_send_msg(RMS_LE_ERROR,
          "Download clutter file number exceeds max (file num = %d)", num);
      return_code = 24;
   }

      /* Get lock status of clutter zone LB */
   if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
      ret = ORPGCCZ_get_edit_status( ORPGCCZ_ORDA_ZONES );
   else
      ret = ORPGCCZ_get_edit_status( ORPGCCZ_LEGACY_ZONES );

      /* If the LB is locked by someone other than RMS cancel edit and 
         return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED && clutter_zones_locked == 0){
         LE_send_msg(RMS_LE_ERROR,
                 "CENSOR_ZONES msg is locked, download cmd rejected"); 
         return_code = 24;
   }

      /* Get lock status of RDA command buffer */
   ret = ORPGEDLOCK_get_edit_status(ORPGDAT_RDA_COMMAND, 0);

      /* If command buffer locked cancel edit and return error code */
   if ( ret == ORPGEDLOCK_EDIT_LOCKED){
      LE_send_msg(RMS_LE_ERROR, 
               "RDA Commands LB is locked, unable to download clutter zones");
      return_code = 24;
   }

      /* If validation and lock checks passed then edit clutter maps */
   if (return_code == 0) {
      if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
         ret = rms_edit_orda_clutter_zone_info (num, rda_clutter_buf_ptr);
      else
         ret = rms_edit_legacy_clutter_zone_info (num, rda_clutter_buf_ptr);

      if ( ret != 1){
         return_code = ret;
      }
   }

      /* Unlock clutter zone LB */
   if (clutter_zones_locked){
      if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
         ret_val = ORPGCCZ_clear_edit_lock( ORPGCCZ_ORDA_ZONES );
      else
         ret_val = ORPGCCZ_clear_edit_lock( ORPGCCZ_LEGACY_ZONES );

      if ( ret_val == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL) {
         LE_send_msg(RMS_LE_ERROR, 
            "Unlock CENSOR_ZONES msg failed (ret: %d)", ret_val);
      } else {
         LE_send_msg(RMS_LE_ERROR, "CENSOR_ZONES msg unlocked");
         clutter_zones_locked = 0;
      }
   }

   return (return_code);

} /* End rms rec download clutter command */


/************************************************************************

    Description:  This function saves the RMMS edited ORDA clutter zone 
                  file to the Clutter MAP LB then sends it to the ORDA

              Inputs: File number, Message buffer

              Outputs: Edited clutter zones.

              Returns: 1 = Successful edit.

 ************************************************************************/

static int rms_edit_orda_clutter_zone_info(int num, UNSIGNED_BYTE *buf_ptr)
{
   ORPG_clutter_regions_msg_t *clutter_files;
   char                       *Clutter_data;
   Redundant_cmd_t            redun_cmd;
   int                        faa_redun;
   int                        my_channel_status;
   int                        other_channel_status;
   int                        link_status;
   int                        ret, i;
   short                      temp_short = 1;
   int                        file_num;
   int                        num_zones = 0;

      /* Set the file number */
   file_num = num;

      /* Read current clutter data */
   ret = ORPGCCZ_get_censor_zones( ORPGCCZ_ORDA_ZONES, (char **) &Clutter_data,
                                   ORPGCCZ_DEFAULT );

   if (ret != sizeof (ORPG_clutter_regions_msg_t)) {
        LE_send_msg (RMS_LE_ERROR,
           "ORPGCCZ_get_censor_zones ( ORPGCCZ_ORDA_ZONES, ...) failed (ret %d)", ret);
        free (Clutter_data);
        return(-1);
   }

      /* Set the clutter regions pointer */
   clutter_files = (ORPG_clutter_regions_msg_t *) Clutter_data;

      /* Temporarily assigning last file downloaded until coordination 
         with FAA is accomplished */
   file_num = clutter_files->last_dwnld_file;

      /* Update the clutter zones */
   for ( i = 0; i <= MAX_CLUTTER_ZONES; i++) {

         /* Get start range from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

         /*Validate start range */
      if ((temp_short < 2 && temp_short != 0) || temp_short > 510) {
          LE_send_msg (RMS_LE_ERROR,
              "Clutter Zone start range out of bounds ( %d)", temp_short);
          free (Clutter_data);
          return(24);
      }

         /* Place start range in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].start_range = temp_short;

         /* Get stop range from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

         /*Validate stop range */
      if ((temp_short < 2 && temp_short != 0) || temp_short > 510 ){
         LE_send_msg (RMS_LE_ERROR,
            "Clutter Zone stop range out of bounds ( %d)", temp_short);
         free (Clutter_data);
         return(24);
      }

         /* Place stop range in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].stop_range = temp_short;

         /* Get start azimuth from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

         /*Validate start azimuth */
      if ( temp_short < 0 || temp_short > 360) {
         LE_send_msg (RMS_LE_ERROR,
            "Clutter Zone start azimuth out of bounds ( %d)", temp_short);
         free (Clutter_data);
         return(24);
      }

         /* Place start azimuth in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].start_azimuth = temp_short;

         /* Get stop azimuth from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

         /* Validate stop azimuth */
      if ( temp_short < 0 || temp_short > 360){
         LE_send_msg (RMS_LE_ERROR,
            "Clutter Zone stop azimuth out of bounds ( %d)", temp_short);
         free (Clutter_data);
         return(24);
      }

         /* Place stop azimuth in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].stop_azimuth = temp_short;

         /* Get segment number from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

         /* Validate segment number */
      if ((temp_short < 1 && temp_short != 0) || temp_short > 5) { 
         LE_send_msg (RMS_LE_ERROR,
            "Clutter Zone segment out of bounds ( %d)", temp_short);
         free (Clutter_data);
         return(24);
      }

         /* Place segment number in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].segment = temp_short;

         /* Get select code from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

         /* Validate select code */
      if ( temp_short < 0 || temp_short > 2){
         LE_send_msg (RMS_LE_ERROR,
            "Clutter Zone select code out of bounds ( %d)", temp_short);
         free (Clutter_data);
         return(24);
      }

         /* Place select code in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].select_code = temp_short;

         /* If valid segment then increment number of clutter zones */
      if (clutter_files->file[ file_num ].regions.data[i].segment != 0 )
          num_zones++;

   } /* End for loop */

      /* Set up number of regions downloaded*/
   clutter_files->file[ file_num ].regions.regions = num_zones;

      /* Get redundant type */
   faa_redun = ORPGSITE_get_int_prop (ORPGSITE_REDUNDANT_TYPE);

   if (ORPGSITE_error_occurred()){
      faa_redun = 0;
      LE_send_msg(RMS_LE_ERROR,
         "Unable to get reundant type for download clutter zones");
   }

      /* Reject command if not a FAA redundant configuration...this
         should never happen */
   if(faa_redun != ORPGSITE_FAA_REDUNDANT) {
      LE_send_msg (RMS_LE_ERROR,
              "RMMS command received in a non-FAA redundant configuration");

      free (Clutter_data);
      return (28);  /* Reject cmd due to non-redundant configuration */
   } else {

         /* Get other channel state */
      other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

         /* Get this channel state */
      my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);

         /* Get ORPG to ORPG link status */
      link_status = ORPGRED_rpg_rpg_link_state();

         /* If this channel active, or both channels inactive, or the 
            ORPG to ORPG link is down save clutter zones */
      if (my_channel_status == ORPGRED_CHANNEL_ACTIVE || 
          link_status == ORPGRED_CHANNEL_LINK_DOWN    ||
          (my_channel_status == ORPGRED_CHANNEL_INACTIVE && 
           other_channel_status == ORPGRED_CHANNEL_INACTIVE)) {

            /* Write new region data.*/
         ret = ORPGCCZ_set_censor_zones( ORPGCCZ_ORDA_ZONES, (char *) Clutter_data,
                                         sizeof (ORPG_clutter_regions_msg_t),
                                         ORPGCCZ_DEFAULT );

         if (ret != sizeof (ORPG_clutter_regions_msg_t)) {
            LE_send_msg (RMS_LE_ERROR,
              "ORPGCCZ_set_clutter_zones( ORPGCCZ_ORDA_ZONES, ...) failed (ret %d)", ret);
            free (Clutter_data);
            return(-1);
         }

            /* Send a command to RDA to update the clutter zones */
         ret = ORPGRDA_send_cmd (COM4_SENDCLCZ, RMS_INITIATED_RDA_CTRL_CMD, 
                                 MAX_CLUTTER_ZONES + 1, file_num, 0, 0, 0, NULL);

         if (ret < 0 ) {
            free (Clutter_data);
            return (ret);
         }

            /* Set up commands to force redundant update */
         redun_cmd.cmd = ORPGRED_DOWNLOAD_CLUTTER_ZONES;
         redun_cmd.lb_id  = 0;
         redun_cmd.msg_id = 0;
         redun_cmd.parameter1 = clutter_files->file [file_num].regions.regions;
         redun_cmd.parameter2 = file_num;

            /* Send the command to update the redundant channel */
         if((ret = ORPGRED_send_msg (redun_cmd)) < 0){
              LE_send_msg (RMS_LE_ERROR,
                 "Unable to download clutter on redundant channel (%d)", ret);
              free (Clutter_data);
              return (ret);
         }
      }
      else {
         LE_send_msg (RMS_LE_ERROR,
                 "Unable to edit clutter in FAA redundant configuration.");
      }/*End else */

   }/* End else */

      /* Free file data. */
   free (Clutter_data);

   return (1);

} /*End rms edit clutter zone info*/


/************************************************************************
    Function:     rms_edit_clutter_zone_info

    Description:  This function sends the clutter
             zone message to RMMS.

              Inputs: File number, Message buffer

              Outputs: Edited clutter zones.

              Returns: 1 = Successful edit.

 ************************************************************************/

static int rms_edit_legacy_clutter_zone_info(int num, UNSIGNED_BYTE *buf_ptr) {

   RPG_clutter_regions_msg_t *clutter_files;
   char                      *Clutter_data;
   Redundant_cmd_t           redun_cmd;
   int                       faa_redun;
   int                       my_channel_status;
   int                       other_channel_status;
   int                       link_status;
   int                       ret, i;
   short                     temp_short = 1;
   int                       file_num;
   int                       num_zones = 0;

   /* Set the file number */
   file_num = num;

   /* Read current clutter data */
   ret = ORPGCCZ_get_censor_zones( ORPGCCZ_LEGACY_ZONES, (char **) &Clutter_data,
                                   ORPGCCZ_DEFAULT );

   if (ret != sizeof (RPG_clutter_regions_msg_t)) {
        LE_send_msg (RMS_LE_ERROR,
           "RMS: ORPGCCZ_get_censor_zones failed (ORPGCCZ_LEGACY_ZONES ) in download clutter zones (ret %d)",ret);
        free (Clutter_data);
        return(-1);
   }

   /* Set the clutter regions pointer */
   clutter_files = (RPG_clutter_regions_msg_t *) Clutter_data;

   /* Temporarily assigning last file downloaded until coordination 
      with FAA is accomplished */
   file_num = clutter_files->last_dwnld_file;

   /* Update the clutter zones */
   for ( i=0; i<=MAX_CLUTTER_ZONES; i++) {

      /* Get start range from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

      /*Validate start range */
      if ((temp_short < 2 && temp_short != 0) || temp_short > 510 ){
          LE_send_msg (RMS_LE_ERROR,
              "RMS:  Clutter Zone start range out of bounds ( %d)",temp_short);
          free (Clutter_data);
          return(24);
      }

      /* Place start range in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].start_range = temp_short;

      /* Get stop range from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

      /*Validate stop range */
      if ((temp_short < 2 && temp_short != 0) || temp_short > 510 ){
         LE_send_msg (RMS_LE_ERROR,
            "RMS:  Clutter Zone stop range out of bounds ( %d)",temp_short);
         free (Clutter_data);
         return(24);
      }

      /* Place stop range in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].stop_range = temp_short;

      /* Get start azimuth from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

      /*Validate start azimuth */
      if ( temp_short < 0 || temp_short > 360){
         LE_send_msg (RMS_LE_ERROR,
            "RMS:  Clutter Zone start azimuth out of bounds ( %d)",temp_short);
         free (Clutter_data);
         return(24);
      }

      /* Place start azimuth in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].start_azimuth = temp_short;

      /* Get stop azimuth from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

      /* Validate stop azimuth */
      if ( temp_short < 0 || temp_short > 360){
         LE_send_msg (RMS_LE_ERROR,
            "RMS:  Clutter Zone stop azimuth out of bounds ( %d)",temp_short);
         free (Clutter_data);
         return(24);
      }

      /* Place stop azimuth in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].stop_azimuth = temp_short;

      /* Get segment number from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

      /* Validate segment number */
      if ((temp_short < 1 && temp_short != 0) || temp_short > 2){
         LE_send_msg (RMS_LE_ERROR,
            "RMS:  Clutter Zone segment out of bounds ( %d)",temp_short);
         free (Clutter_data);
         return(24);
      }

      /* Place segment number in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].segment = temp_short;

      /* Get select code from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

      /* Validate select code */
      if ( temp_short < 0 || temp_short > 2){
         LE_send_msg (RMS_LE_ERROR,
            "RMS:  Clutter Zone select code out of bounds ( %d)",temp_short);
         free (Clutter_data);
         return(24);
      }

      /* Place select code in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].select_code = temp_short;

      /* Get channel D width from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

      /* Validate channel D width */
      if ((temp_short < 1 && temp_short != 0) || temp_short > 3 ){
         LE_send_msg (RMS_LE_ERROR,
            "RMS:  Clutter Zone channel D width out of bounds ( %d)",temp_short);
         free (Clutter_data);
         return(24);
      }

      /* Place channel D width in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].doppl_level = temp_short;

      /* Get channel S width from message */
      temp_short = conv_shrt(buf_ptr);
      buf_ptr += PLUS_SHORT;

      /*Validate channel S width */
      if ((temp_short < 1 && temp_short != 0) || temp_short > 3 ){
         LE_send_msg (RMS_LE_ERROR,
            "RMS:  Clutter Zone channel S width out of bounds ( %d)",temp_short);
         free (Clutter_data);
         return(24);
      }

      /* Place channel S width in clutter zone */
      clutter_files->file[ file_num ].regions.data[i].surv_level = temp_short;

      /* If valid segment then increment number of xlutter zones */
      if (clutter_files->file[ file_num ].regions.data[i].segment != 0 )
          num_zones++;

   } /* End for loop */

   /* Set up number of regions downloaded*/
   clutter_files->file[ file_num ].regions.regions = num_zones;

   /* Get redundant type */
   faa_redun = ORPGSITE_get_int_prop(ORPGSITE_REDUNDANT_TYPE);

   if (ORPGSITE_error_occurred()){
      faa_redun = 0;
      LE_send_msg(RMS_LE_ERROR,"RMS: Unable to get reundant type for download clutter zones");
   }

   /* If not a redundant configuration save clutter zones */
   if(faa_redun != ORPGSITE_FAA_REDUNDANT) {

      /* Write new region data.*/
      ret = ORPGCCZ_set_censor_zones( ORPGCCZ_LEGACY_ZONES, (char *) Clutter_data,
                                      sizeof (RPG_clutter_regions_msg_t),  ORPGCCZ_DEFAULT );

      if (ret != sizeof (RPG_clutter_regions_msg_t)) {
         LE_send_msg (RMS_LE_ERROR,
           "RMS: ORPGCCZ_set_censor_zones failed (ORPGCCZ_LEGACY_ZONES) in download clutter zones (ret %d)",ret);
          free (Clutter_data);
          return(-1);
      }

      /* Send a command to RDA to update the clutter zones */
      ret = ORPGRDA_send_cmd (COM4_SENDCLCZ, RMS_INITIATED_RDA_CTRL_CMD, 
                              MAX_CLUTTER_ZONES + 1, file_num, 0, 0, 0, NULL);

      if (ret < 0 ){
         free (Clutter_data);
         return (ret);
      }
   }/* End if*/
   else {

      /* Get other channel state */
      other_channel_status = ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL);

      /* Get this channel state */
      my_channel_status = ORPGRED_channel_state(ORPGRED_MY_CHANNEL);

      /* Get ORPG to ORPG link status */
      link_status = ORPGRED_rpg_rpg_link_state();

      /* If this channel active, or both channels inactive, or the ORPG to ORPG 
         link is down save clutter zones */
      if (my_channel_status == ORPGRED_CHANNEL_ACTIVE || 
          link_status == ORPGRED_CHANNEL_LINK_DOWN    ||
          (my_channel_status == ORPGRED_CHANNEL_INACTIVE && 
           other_channel_status == ORPGRED_CHANNEL_INACTIVE)) {

         /* Write new region data.*/
         ret = ORPGCCZ_set_censor_zones( ORPGCCZ_LEGACY_ZONES, (char *) Clutter_data,
                                         sizeof (RPG_clutter_regions_msg_t), ORPGCCZ_DEFAULT );

         if (ret != sizeof (RPG_clutter_regions_msg_t)) {
            LE_send_msg (RMS_LE_ERROR,
              "RMS: ORPGCCZ_set_censor_zones failed (ORPGCCZ_LEGACY_ZONES) in download clutter zones (ret %d)",ret);
            free (Clutter_data);
            return(-1);
         }

         /* Send a command to RDA to update the clutter zones */
         ret = ORPGRDA_send_cmd (COM4_SENDCLCZ, RMS_INITIATED_RDA_CTRL_CMD, 
                                 MAX_CLUTTER_ZONES + 1, file_num, 0, 0, 0, NULL);

         if (ret < 0 ) {
            free (Clutter_data);
            return (ret);
         }

         /* Set up commands to force redundant update */
         redun_cmd.cmd = ORPGRED_DOWNLOAD_CLUTTER_ZONES;
         redun_cmd.lb_id  = 0;
         redun_cmd.msg_id = 0;
         redun_cmd.parameter1 = clutter_files->file [file_num].regions.regions;
         redun_cmd.parameter2 = file_num;

         /* Send the command to update the redundant channel */
         if((ret = ORPGRED_send_msg (redun_cmd)) < 0){
              LE_send_msg (RMS_LE_ERROR,
                 "RMS: Unable to download clutter on redundant channel (%d)", ret);
              free (Clutter_data);
              return (ret);
         }
      }/* End if */
      else {
         LE_send_msg (RMS_LE_ERROR,
                 "RMS: Unable to edit clutter in FAA redundant configuration.");
      }/*End else */

   }/* End else */

   /* Free file data. */
   free (Clutter_data);

   return (1);
} /*End rms edit clutter zone info*/
