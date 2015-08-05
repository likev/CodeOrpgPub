/**************************************************************************
   
   Module:  rms_rec_edit_clutter_command.c   `
   
   Description:  This module builds clutter zones to be sent to RMMS.  Upon
   receipt of this command a censor zone is sent to RMMS for editing.
   

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/09 21:46:33 $
 * $Id: rms_edit_clutter.c,v 1.16 2007/01/09 21:46:33 ccalvert Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

  /* System Include Files/Local Include Files */

#include <rms_message.h>
#include <orpgedlock.h>
#include <rpg_clutter_censor_zones.h>


   /* Constant Definitions/Macro Definitions/Type Definitions */

#define MAX_CLUTTER_LENGTH   270
#define STATUS_TYPE          19


   /* Static Globals */

int clutter_zones_locked;


   /* Static Function Prototypes */

static int rms_send_legacy_clutter_zone_msg(ushort file_num);
static int rms_send_orda_clutter_zone_msg(ushort file_num);



/**************************************************************************
   Description:  This function reads the command from the message buffer.
      
   Input: rda_clutter_buf - Pointer to the message buffer.
      
   Output: Sends requested clutter zone information to the FAA/RMMS.

   Returns: 0 = Successful edit.

   Notes:

   **************************************************************************/

int rms_rec_clutter_command (UNSIGNED_BYTE *rda_clutter_buf) 
{
   UNSIGNED_BYTE *rda_clutter_buf_ptr;
   int           ret;
   ushort        rda_clutter_flag;
   ushort        file_num;


      /* Set pointer to beginning of buffer */
   rda_clutter_buf_ptr = rda_clutter_buf;

      /* Place pointer past header */
   rda_clutter_buf_ptr += MESSAGE_START;

      /* Get command */
   rda_clutter_flag = conv_ushrt(rda_clutter_buf_ptr);
   rda_clutter_buf_ptr += PLUS_SHORT;

      /* Get VCP number */
   file_num = conv_ushrt(rda_clutter_buf_ptr);
   rda_clutter_buf_ptr += PLUS_SHORT;

      /* Validation checks */
   if ( file_num  < 1 ){
      LE_send_msg (RMS_LE_ERROR,
           "Edit clutter file number less than one (file num = %d)", 
           file_num);
      return (24);
   }

   if ( file_num  > MAX_CLTR_FILES){
      LE_send_msg (RMS_LE_ERROR,
          "Edit clutter file number exceeds max (file num = %d)", 
          file_num);
      return (24);
   }

      /* Edit clutter command */
   if (rda_clutter_flag == 1) {

         /* Check to see if clutter zone LB locked */

      if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
         ret = ORPGCCZ_get_edit_status( ORPGCCZ_ORDA_ZONES );
      else  /* must be legacy configuration */
         ret = ORPGCCZ_get_edit_status( ORPGCCZ_LEGACY_ZONES );

      if ( ret == ORPGEDLOCK_EDIT_LOCKED) {
         LE_send_msg (RMS_LE_ERROR,"Can not edit CENSOR ZONES -  msg is locked");
         return (24);
      }

         /* Send Clutter Zone information to the FAA RMMS */
      if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
         ret = rms_send_orda_clutter_zone_msg (file_num);
      else
         ret = rms_send_legacy_clutter_zone_msg (file_num);
      
      if (ret != 1)
         return (ret);
         
      return (0);
      
   } /* call send clutter */

      /* Cancel edit clutter zones */
   if (rda_clutter_flag == 2) {

         /* Clear edit lock on clutter zone LB */
      if (ORPGRDA_get_rda_config (NULL) == ORPGRDA_ORDA_CONFIG)
         ret = ORPGCCZ_clear_edit_lock ( ORPGCCZ_ORDA_ZONES );
      else
         ret = ORPGCCZ_clear_edit_lock ( ORPGCCZ_LEGACY_ZONES );

      if ( ret == ORPGEDLOCK_UNLOCK_NOT_SUCCESSFUL) {
         LE_send_msg (RMS_LE_ERROR, "CENSOR ZONES unlock unsuccessful");
         return (ret);
      } else {
         LE_send_msg (RMS_LE_ERROR,
            "CENSOR ZONES unlock successful...edit cancelled.");
      }

      return (0);
   } /* stop clutter edit */

   return (24);

} /* End rms rec clutter command */


/**************************************************************************
   Description:  This function builds and sends the message with the current
   clutter censor zones.

   Input: file_num - The file number to be edited.

   Output:  Clutter censor zone message.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/

static int rms_send_orda_clutter_zone_msg (ushort file_num)
{
   ORPG_clutter_regions_msg_t *clutter_files;
   char                       *Clutter_data;
   UNSIGNED_BYTE              msg_buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE              *msg_buf_ptr;
   int                        msg_length = 0;
   int                        ret, i;
   ushort                     num_halfwords;
   short                      temp_short;

      /* Set pointer to beginning of buffer */
   msg_buf_ptr = msg_buf;

      /* Place pointer past header */
   msg_buf_ptr += MESSAGE_START;

      /* Put file number in output buffer */
   conv_ushort_unsigned (msg_buf_ptr, (ushort*)&file_num);
   msg_buf_ptr += PLUS_SHORT;

      /* Read clutter zone data from clutter zones LB */
   ret = ORPGCCZ_get_censor_zones( ORPGCCZ_ORDA_ZONES, (char **) &Clutter_data,
                                   ORPGCCZ_DEFAULT );

   if (ret != sizeof (ORPG_clutter_regions_msg_t)) {
      LE_send_msg (RMS_LE_ERROR,
         "ORPGCCZ_get_censor_zones (ORPGCCZ_ORDA_ZONES, ...) failed (ret: %d)", ret);
      return(-1);
   }

      /* Assign clutter data to clutter files structure */
   clutter_files = (ORPG_clutter_regions_msg_t *) Clutter_data;

      /* Temporarily assigning last file downloaded until coordination 
         with FAA is accomplished */
   file_num = clutter_files->last_dwnld_file;

      /* Place zones in output buffer */
   for (i = 0; i <= clutter_files->file[file_num].regions.regions; i++) {

         /* Place start range in message */
      temp_short = clutter_files->file[file_num].regions.data[i].start_range;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place stop range in message */
      temp_short = clutter_files->file[file_num].regions.data[i].stop_range;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place start azimuth in message */
      temp_short = clutter_files->file[file_num].regions.data[i].start_azimuth;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place stop azimuth in message */
      temp_short = clutter_files->file[file_num].regions.data[i].stop_azimuth;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place segment number in message */
      temp_short = clutter_files->file[file_num].regions.data[i].segment;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place select code in message */
      temp_short = clutter_files->file[file_num].regions.data[i].select_code;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

   } /* End for loop */

      /* Pad the message if necessary */
   if (i < MAX_NUMBER_CLUTTER_ZONES) {
      msg_length = msg_buf_ptr - msg_buf;
      pad_message(msg_buf_ptr, msg_length, MAX_CLUTTER_LENGTH);
   }

      /* Place pointer to end of message */
   msg_buf_ptr += (MAX_CLUTTER_LENGTH - msg_length);

      /* Add terminator to the message */
   add_terminator(msg_buf_ptr);
   msg_buf_ptr += PLUS_INT;

      /* Compute the number of halfwords in the message */
   num_halfwords = ((msg_buf_ptr - msg_buf) / 2);

      /* Add header to message */
   ret = build_header(&num_halfwords, STATUS_TYPE, msg_buf, 0);

   if (ret != 1){
      LE_send_msg (RMS_LE_ERROR,
         "Build RMMS msg header failed for clutter censor zone");
      return (-1);
   }

      /* Send the message to the FAA RMMS */
   ret = send_message (msg_buf, STATUS_TYPE, RMS_STANDARD);

   if (ret != 1) {
      LE_send_msg (RMS_LE_ERROR,
         "Send RMMS message failed (ret %d) for clutter censor zones", ret);
      return (-1);
   }

      /* Lock the clutter zone msg so someone else cannot 
         edit until RMS is done */
   ret = ORPGCCZ_set_edit_lock( ORPGCCZ_ORDA_ZONES );

   if ( ret == ORPGEDLOCK_LOCK_UNSUCCESSFUL) {
      LE_send_msg(RMS_LE_ERROR, "Unable to lock CENSOR ZONES ORDA");
      return (ret);
   } else{
      LE_send_msg(RMS_LE_ERROR, "CENSOR ZONES ORDA locked");
      clutter_zones_locked = 1;
   }

   return (1);

} /*End build clutter */


/**************************************************************************
   Description:  This function builds and sends the message with the current
   clutter censor zones.

   Input: file_num - The file number to be edited.

   Output:  Clutter censor zone message.

   Returns: Message sent = 1, Not sent = -1

   Notes:

   **************************************************************************/
static int rms_send_legacy_clutter_zone_msg(ushort file_num)
{
   RPG_clutter_regions_msg_t *clutter_files;
   char                      *Clutter_data;
   UNSIGNED_BYTE             msg_buf[MAX_BUF_SIZE];
   UNSIGNED_BYTE             *msg_buf_ptr;
   int                       msg_length = 0;
   int                       ret, i;
   ushort                    num_halfwords;
   short                     temp_short;

      /* Set pointer to beginning of buffer */
   msg_buf_ptr = msg_buf;

      /* Place pointer past header */
   msg_buf_ptr += MESSAGE_START;

      /* Put file number in output buffer */
   conv_ushort_unsigned(msg_buf_ptr,(ushort*)&file_num);
   msg_buf_ptr += PLUS_SHORT;

      /* Read clutter zone data from clutter zones LB */
   ret = ORPGCCZ_get_censor_zones( ORPGCCZ_LEGACY_ZONES, (char **) &Clutter_data,
                                   ORPGCCZ_DEFAULT );

   if (ret != sizeof (RPG_clutter_regions_msg_t)) {
      LE_send_msg (RMS_LE_ERROR,
         "ORPGCCZ_get_censor_zones failed (ORPGCCZ_LEGACY_ZONES) in edit clutter command (ret %d)",
         ret);
      return(-1);
   }

      /* Assign clutter data to clutter files structure */
   clutter_files = (RPG_clutter_regions_msg_t *) Clutter_data;

      /* Temporarily assigning last file downloaded until coordination 
         with FAA is accomplished */
   file_num = clutter_files->last_dwnld_file;

      /* Place zones in output buffer */
   for (i=0; i<=clutter_files->file[file_num].regions.regions; i++) {

         /* Place start range in message */
      temp_short = clutter_files->file[file_num].regions.data[i].start_range;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place stop range in message */
      temp_short = clutter_files->file[file_num].regions.data[i].stop_range;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place start azimuth in message */
      temp_short =clutter_files->file[file_num].regions.data[i].start_azimuth;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place stop azimuth in message */
      temp_short =clutter_files->file[file_num].regions.data[i].stop_azimuth;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place segment number in message */
      temp_short =clutter_files->file[file_num].regions.data[i].segment;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place select code in message */
      temp_short =clutter_files->file[file_num].regions.data[i].select_code;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

         /* Place doppler level in message */
      temp_short =clutter_files->file[file_num].regions.data[i].doppl_level;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;


         /* Place surviellance level in message */
      temp_short =clutter_files->file[file_num].regions.data[i].surv_level;
      conv_short_unsigned(msg_buf_ptr, &temp_short);
      msg_buf_ptr += PLUS_SHORT;

   } /* End for loop */

      /* Pad the message if necessary */
   if (i < MAX_NUMBER_CLUTTER_ZONES) {
      msg_length = msg_buf_ptr - msg_buf;
      pad_message(msg_buf_ptr, msg_length, MAX_CLUTTER_LENGTH);
   }

      /* Place pointer to end of message */
   msg_buf_ptr += (MAX_CLUTTER_LENGTH - msg_length);

      /* Add terminator to the message */
   add_terminator(msg_buf_ptr);
   msg_buf_ptr += PLUS_INT;

      /* Compute the number of halfwords in the message */
   num_halfwords = ((msg_buf_ptr - msg_buf) / 2);

      /* Add header to message */
   ret = build_header(&num_halfwords, STATUS_TYPE, msg_buf, 0);

   if (ret != 1){
      LE_send_msg (RMS_LE_ERROR,
         "Build RMMS msg header failed for clutter censor zone");
      return (-1);
   }

      /* Send the message to the FAA/RMMS */
   ret = send_message(msg_buf,STATUS_TYPE,RMS_STANDARD);

   if (ret != 1){
      LE_send_msg (RMS_LE_ERROR,
         "Send RMMS message failed (ret %d) for clutter censor zones", ret);
      return (-1);
   }

      /* Lock the clutter zone msg so someone else cannot 
         edit until RMS is done */
   ret = ORPGCCZ_set_edit_lock( ORPGCCZ_LEGACY_ZONES );

   if ( ret == ORPGEDLOCK_LOCK_UNSUCCESSFUL) {
      LE_send_msg(RMS_LE_ERROR,"Unable to lock CENSOR ZONES LGCY");
      return (ret);
   } else{
      LE_send_msg(RMS_LE_ERROR,"CENSOR ZONES LGCY locked");
      clutter_zones_locked = 1;
   }

   return (1);

} /*End build clutter */
