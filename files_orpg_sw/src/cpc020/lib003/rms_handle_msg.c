/**************************************************************************

   Module:  rms_handle_msg.c

   Description:  This routine is called when a message from RMMS is received.
   it strips the header and places the infromation into a structure for use
   by other functions.  It then reads the message number and calls the
   appropiate routine to handle the message.


   Assumptions:

   **************************************************************************/
/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/03 22:53:06 $
 * $Id: rms_handle_msg.c,v 1.18 2008/01/03 22:53:06 aamirn Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/
#include <rms_message.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/
#define   BAD_MESSAGE          1
#define INVALID_CMD_RECEIVED  19

/*
* Static Globals
*/

/*
* Static Function Prototypes
*/

static int validate_msg (UNSIGNED_BYTE *buf);


/**************************************************************************
   Description:  This function send the message to the appropriate routine to handle
   it.

   Input: None

   Output: None

   Returns: 0 = Successful message handling.

   Notes:

   **************************************************************************/
void rms_handle_msg(){

   UNSIGNED_BYTE buf[MAX_BUF_SIZE];
   int           buf_count = 0;
   int           ret_val;
   struct header *header_ptr;
   struct header msg_header;
   extern int    in_lbfd;
   extern ushort reject_flag;

   /* Main interface function that directs messages to the library routines
       that process the individual messages */

   /* Read the input buffer for the latest message. */
   buf_count = LB_read (in_lbfd, (char*)&buf, MAX_BUF_SIZE, LB_NEXT);

   /* Continue reading messages until there are none left in the input buffer */
   while(buf_count != LB_TO_COME){

      /* Validate the message */
      ret_val = validate_msg (buf);

      /* If the return size is less than MAX_BUF_SIZE or a bad message 
         was received an error has occured */
      if (buf_count < 0 || ret_val == BAD_MESSAGE) {

         if (buf_count != LB_TO_COME){
            LE_send_msg (RMS_LE_ERROR,
                "LB read rms handle message failed (ret %d)", buf_count);
         } /* Check for LB read errors */

         if (ret_val == BAD_MESSAGE){
            LE_send_msg (RMS_LE_ERROR,"RMS: Bad message received from RMMS");
         }
      } else {
         /* Set the header pointer equal to the header structure */
         header_ptr = &msg_header;

         /* Get header information and place it in the message header structure. */
         ret_val = strip_header((UNSIGNED_BYTE*)buf,header_ptr);

         if (ret_val == 1){

            /* Process message */
            switch(msg_header.message_type){

               case 4: /* RDA state command */
                  /* Command sent from the FAA/RMMS to control the RDA */
                  if ((ret_val = rms_rec_rda_state_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                          "RMMS RDA state change command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 5: /* RDA channel channel command */
                  /* Command from the FAA/RMMS to change RDA channel control */
                  if ((ret_val = rms_rec_rda_channel_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS RDA channel control command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 6: /* RDA mode command */
                  /* Command from the FAA/RMMS to change the RDA operating mode */
                  if ((ret_val = rms_rec_rda_mode_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS RDA change mode command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 7: /* Archive II control */
/*                 Archive II functionality has been removed in Build 7 - 
                   Central Data Collection is now the archiving mechanism 

                  if ((ret_val = rms_rec_arch_command((UNSIGNED_BYTE*)buf)) != 0) {
                      LE_send_msg (RMS_LE_ERROR,
                         "RMMS RDA Archive II command failed"); 

                     reject_flag = ret_val;
                  } */
                  LE_send_msg (RMS_LE_ERROR,
                      "RMMS Archive II cmd no longer supported, rejecting cmd");

                  reject_flag = 19;  /* Set Msg ack to "Invalid command received" */

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message");

               break;

               case 8: /* RPG control command */
                  /* Command from the FAA/RMMS to change the RPG operating mode */
                  if ((ret_val = rms_rec_rpg_control_command ((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS RPG control command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 9: /* Force adaptation update */
                  /* Cmd from the RMMS to force adapt data to the redundant channel */
                  if ((ret_val = rms_rec_adaptation_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                       "RMMS Force adaptation command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 10: /* Narrowband interface command */
                  /* Cmd from the FAA/RMMS to connect/disconnect narrowband lines */
                  if ((ret_val = rms_rec_nb_inter_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS narrow band interface command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 11: /* Wideband Interface command */
                  /* Command from the FAA/RMMS to connect/disconnect wideband line */
                  if ((ret_val = rms_rec_wb_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                       "RMMS wide band command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                  LE_send_msg (RMS_LE_ERROR,
                    "Error sending acknowledgement message" );

               break;

               case 12: /* Wideband Loopback*/
                  /* Cmd from the FAA/RMMS to start/stop wideband loopback test */
                  if ((ret_val = rms_rec_loopback_command ((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                       "RMMS loopback command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 14: /* Send free text command*/
                  /* Recieved a free text message from the FAA/RMMS */
                  if ((ret_val = rms_rec_free_text_command ((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS free text command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 15: /* Edit PUP ID/Comline Assignments */
                  /* Recieved a request for PUP information from the FAA/RMMS */
/*//                  if ((ret_val = rms_rec_pup_id_command ((UNSIGNED_BYTE*)buf)) != 0) {*/
/*//                     LE_send_msg (RMS_LE_ERROR,*/
/*//                       "RMMS pup id command failed");*/
                     /* Return value is error code for failed messages*/
/*//                     reject_flag = ret_val;*/
/*//                  }*/
                  LE_send_msg (GL_ERROR,
                      "Edit PUP ID/Comline Assignments cmd no longer supported, rejecting cmd");

                  reject_flag = INVALID_CMD_RECEIVED;

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 17: /* Download PUP ID/Comline Assignments */
                  /* Recieved cmd from RMMS to download edited PUP info into ORPG */
/*//                  if ((ret_val = rms_rec_download_pup_id_command((UNSIGNED_BYTE*)buf)) != 0) {*/
/*//                     LE_send_msg (RMS_LE_ERROR,*/
/*//                        "RMMS download pup id command failed");*/
                     /* Return value is error code for failed messages*/
/*//                     reject_flag = ret_val;*/
/*//                  }*/
                  LE_send_msg (GL_ERROR,
                      "Download PUP ID/Comline Assignments cmd no longer supported, rejecting cmd");

                  reject_flag = INVALID_CMD_RECEIVED;

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 18: /* Edit clutter suppression regions */
                  /* Recieved a request for clutter zone information from the FAA/RMMS */
                  if ((ret_val = rms_rec_clutter_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS clutter command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 20: /* Download clutter suppression regions */
                  /* Recieved  cmd from FAA/RMMS to download edited 
                     clutter zone info into ORPG  */
                  if ((ret_val = rms_rec_download_clutter_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS download clutter command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 21: /* Edit bypass map */
/*                  Editing the Bypass Map functionality has been removed
                    in Build 7.
                  if ((ret_val = rms_rec_bypass_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                       "RMMS bypass command failed");

                     reject_flag = ret_val;
                  } */
                  LE_send_msg (RMS_LE_ERROR,
                      "RMMS Edit Bypass Map cmd no longer supported, rejecting cmd");

                  reject_flag = 19;  /* Set Msg ack to "Invalid command received" */

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 23: /* Download bypass map */
/*                  Editing the Bypass Map functionality has been removed
                    in Build 7.

                  if ((ret_val = rms_rec_download_bypass_command((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS bypass command failed");

                     reject_flag = ret_val;
                  } */
                  LE_send_msg (RMS_LE_ERROR,
                     "RMMS Download Bypass Map cmd no longer supported, rejecting cmd");

                  reject_flag = 19;  /* Set Msg ack to "Invalid command received" */

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 24: /* Edit load shed */
                  /* Recieved a request for load shed information from the FAA/RMMS */
                  if ((ret_val = rms_rec_load_shed_command ((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                       "RMMS load_shed command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 26: /* Download load shed */
                  /* Recieved cmd from RMMS to download edited load shed info into ORPG*/
                  if ((ret_val = rms_rec_download_load_shed_command ((UNSIGNED_BYTE*)buf)) != 0) {
                     LE_send_msg (RMS_LE_ERROR,
                       "RMMS download load_shed command failed");
                     /* Return value is error code for failed messages*/
                     reject_flag = ret_val;
                  }

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );
               break;

               case 27: /* Edit narrowband configuration */
                  /* Recieved a request for nb config info from the FAA/RMMS */
/*//                  if((ret_val = rms_rec_nb_cfg_command ((UNSIGNED_BYTE*)buf)) != 0) {*/
/*//                     LE_send_msg (RMS_LE_ERROR, */
/*//                       "RMMS narrowband configuration failed");*/
                     /* Return value is error code for failed messages*/
/*//                     reject_flag = ret_val;*/
/*//                  }*/
                  LE_send_msg (GL_ERROR,
                      "Edit Narrowband Configuration cmd no longer supported, rejecting cmd");

                   reject_flag = INVALID_CMD_RECEIVED;

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 29: /* Download narrowband configuration */
                  /* Recieved cmd from RMMS to download edited nb config info into ORPG*/
/*//                  if((ret_val = rms_rec_download_nb_cfg_command ((UNSIGNED_BYTE*)buf)) != 0) {*/
/*//                     LE_send_msg (RMS_LE_ERROR,*/
/*//                        "RMMS narrowband download configuration failed");*/
                     /* Return value is error code for failed messages*/
/*//                     reject_flag = ret_val;*/
/*//                  }*/
                  LE_send_msg (GL_ERROR,
                      "Download Narrowband Configuration cmd no longer supported, rejecting cmd");

                  reject_flag = INVALID_CMD_RECEIVED;

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 30: /* Edit narrowband dial in line configuration */
                  /* Recieved a request for nb dial in information from the FAA/RMMS */
/*//                  if((ret_val = rms_rec_dial_cfg_command ((UNSIGNED_BYTE*)buf)) != 0) {*/
/*//                     LE_send_msg (RMS_LE_ERROR,*/
/*//                        "RMMS dial in configuration failed");*/
                     /* Return value is error code for failed messages*/
/*//                     reject_flag = ret_val;*/
/*//                  }*/
                  LE_send_msg (GL_ERROR,
                      "Edit Narrowband Dial-in Line Configuration cmd no longer supported, rejecting cmd");

                  reject_flag = INVALID_CMD_RECEIVED;

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 32: /* Download narrowband dial in line configuration */
                  /* Recieved cmd from RMMS to download edited nb dial in info into ORPG*/
/*//                  if((ret_val = rms_rec_download_dial_cfg_command ((UNSIGNED_BYTE*)buf)) != 0) {*/
/*//                     LE_send_msg (RMS_LE_ERROR,*/
/*//                       "RMMS dial in download configuration failed");*/
                     /* Return value is error code for failed messages*/
/*//                     reject_flag = ret_val;*/
/*//                  }*/
                  LE_send_msg (GL_ERROR,
                      "Download Narrowband Dial-in Line Configuration cmd no longer supported, rejecting cmd");

                  reject_flag = INVALID_CMD_RECEIVED;

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 33: /* Edit authorized user */
                  /* Recieved a request for auth user information from the FAA/RMMS */
/*//                  if((ret_val = rms_rec_auth_user_command ((UNSIGNED_BYTE*)buf)) != 0) {*/
/*//                     LE_send_msg (RMS_LE_ERROR,*/
/*//                        "RMMS authorized user failed");*/
                     /* Return value is error code for failed messages*/
/*//                     reject_flag = ret_val;*/
/*//                  }*/
                  LE_send_msg (GL_ERROR,
                      "Edit Authorized User cmd no longer supported, rejecting cmd");

                  reject_flag = INVALID_CMD_RECEIVED;

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );

               break;

               case 35: /* Download authorized user */
                  /* Recieved cmd from RMMS to download edited auth user info into ORPG*/
/*//                  if((ret_val = rms_rec_download_auth_user_command ((UNSIGNED_BYTE*)buf)) != 0) {*/
/*//                     LE_send_msg (RMS_LE_ERROR,*/
/*//                       "RMMS download authorized user failed");*/
                     /* Return value is error code for failed messages*/
/*//                     reject_flag = ret_val;*/
/*//                  }*/
                  LE_send_msg (GL_ERROR,
                      "Download Authorized User cmd no longer supported, rejecting cmd");

                  reject_flag = INVALID_CMD_RECEIVED;

                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1)
                     LE_send_msg (RMS_LE_ERROR,
                       "Error sending acknowledgement message" );

               break;

               case 37: /* Acknowledgement message */
                  /* Recieved from FAA/RMMS to acknowledge the reciept of a message */
                  /* Do not send acknowledgement msg for an acknowledgement msg */
                  if(receive_ack((UNSIGNED_BYTE*)buf) != 1) {
                     LE_send_msg (RMS_LE_ERROR,
                        "RMMS received acknowledgment failed");
                  }
               break;

               case 39: /* RMS up */
                  /* Recieved from FAA/RMMS to indicate interface was down */
                  /* Message successful send acknowledgement */
                  ret_val = build_ack(&msg_header);

                  if (ret_val != 1){
                     LE_send_msg (RMS_LE_ERROR,
                        "Error sending acknowledgement message" );
                  }

               default:

               break;
            } /*End switch*/
         } else {
              LE_send_msg (RMS_LE_ERROR,
                 "Bad RMMS message %d reason unknown", msg_header.seq_num);
           }/* End else */
      } /* End if statement*/

      /* Read next message in the input buffer */
      buf_count = LB_read (in_lbfd, (char*)&buf, MAX_BUF_SIZE, LB_NEXT);

   }/* End Loop */

}/* End of rms handle msg */

/**************************************************************************
   Description:  This function validates the message by comparing checksums.

   Input: Message buffer

   Output: None

   Returns: 0 = Successful message validation.

   Notes:

   **************************************************************************/
static int validate_msg (UNSIGNED_BYTE *buf){

   UNSIGNED_BYTE *buf_ptr;
   int msg_checksum, checksum;
   ushort msg_seq_num, msg_type, ack_msg_seq_num, return_code;

   /* Set pointer to the start of the buffer */
   buf_ptr = buf;

   /* Place pointer at sequence number */
   buf_ptr += PLUS_SHORT;
   buf_ptr += PLUS_SHORT;
   buf_ptr += PLUS_SHORT;

   /* Get incoming message sequence number */
   msg_seq_num = conv_ushrt (buf_ptr);
   buf_ptr += PLUS_SHORT;

   /* Get incoming message message_type */
   msg_type = conv_ushrt (buf_ptr);
   buf_ptr += PLUS_SHORT;

   /* Place pointer at checksum */
   buf_ptr += PLUS_SHORT;
   buf_ptr += PLUS_SHORT;
   buf_ptr += PLUS_INT;

   /* Get incoming message checksum */
   msg_checksum = conv_intgr(buf_ptr);
   buf_ptr += PLUS_INT;

   /* Set pointer to beginning of message */
   buf_ptr = buf;

   /* Compute the checksum for comparison with message checksum */
/*   checksum = rms_checksum(buf_ptr); *******don't checksum with the TCP/IP connection */

   /* Compare the checksum received in the message with the computed checksum 
      ********* Force the checksum to always pass for the Ethernet interface. Again,
                not a pretty change but schedules are priority over coding right now */
   if (1) {
/*   if(msg_checksum == checksum){ */
      /* If the checksums are good process log messages */
      if (msg_type != 37 ) {
         /* Not an acknowledgement message */
         LE_send_msg (RMS_LE_LOG_MSG,
            "RMMS message %u received (type %d)",msg_seq_num, msg_type);
      }else{ /* Acknowledgement message */
         /* Set pointer past header */
         buf_ptr += MESSAGE_START;

         /* Get message type */
         msg_type = conv_ushrt (buf_ptr);
         buf_ptr += PLUS_SHORT;

         /* Get message sequence number being acknowleged*/
         ack_msg_seq_num = conv_ushrt (buf_ptr);
         buf_ptr += PLUS_SHORT;

         /* Get message return code*/
         return_code = conv_ushrt (buf_ptr);
         buf_ptr += PLUS_SHORT;

         LE_send_msg(RMS_LE_LOG_MSG,
           "Ack for message %u received (type %d).", ack_msg_seq_num, msg_type);

         if(return_code != 0){
            LE_send_msg(RMS_LE_LOG_MSG,
              "Ack return code =  %u.", return_code);
         }

      }/* End else */
   }else{
      LE_send_msg(RMS_LE_ERROR,
         "Bad checksum for RMMS message type %d sequence number %u",msg_type,msg_seq_num);

      /* Increment RMS message errors */
      interface_errors ++;

      /* Return BAD_MESSAGE */
      return (1);

   }/* End else*/

   return (0);

} /* End validate msg */
