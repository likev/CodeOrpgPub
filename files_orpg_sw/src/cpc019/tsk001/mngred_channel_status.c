/********************************************************************************
 
             file:  mngred_channel_status.c

      Description:  This file contains the routines which update and process 
                    channel status data and misc state data. The principal 
                    responsibilities are to update and log the channel status 
                    message, transmit the channel status to the other channel, 
                    save previous channel state before process termination, and 
                    process misc channel state changes not processed anywhere else.

 ********************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/05/14 20:58:32 $
 * $Id: mngred_channel_status.c,v 1.13 2013/05/14 20:58:32 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

#include <math.h>

#include <mngred_globals.h>
#include <gen_stat_msg.h>
#include <mrpg.h>
#include <orpggst.h>

/* file scope variables */

   /*  flag specifying that the misc state data managed in this file needs to 
       be updated on the other channel */
static int Update_misc_state_data_req = MNGRED_FALSE; 

/* local functions */

static void Update_misc_states ();


/********************************************************************************

    Description: This routine prints the channel status to the message log

          Input: msg - the message header to print 

         Output:

         Return:

        Globals: CHAnnel_status     - see mngred_globals.h & orpgred.h
                 CHAnnel_link_state - see mngred_globals.h & orpgred.h

          Notes:

 ********************************************************************************/

void CST_print_channel_status (char *msg)
{
   char *ch_link_state[3];     /* states of the channel link */
   char *ch_state [2];         /* RPG channel states */
   char *rda_control_state[2]; /* RDA channel control states */
   char *comms_relay [3];      /* states of the comms relay */
   char *rda_status[7];        /* RDA status (RDA-RPG ICD defined) */
   int  index;                 /* rda_status array index */
   

   ch_link_state [0] = "Unknown";
   ch_link_state [ORPGRED_CHANNEL_LINK_UP] = "Up";
   ch_link_state [ORPGRED_CHANNEL_LINK_DOWN] = "Down";

   ch_state [ORPGRED_CHANNEL_INACTIVE] = "Inactive";
   ch_state [ORPGRED_CHANNEL_ACTIVE] = "Active";

   rda_control_state [ORPGRED_RDA_CONTROLLING] = "Controlling";
   rda_control_state [ORPGRED_RDA_NON_CONTROLLING] = "Non-Controlling";

   comms_relay [ORPGRED_COMMS_RELAY_UNKNOWN] = "Unknown";
   comms_relay [ORPGRED_COMMS_RELAY_ASSIGNED] = "Assigned";
   comms_relay [ORPGRED_COMMS_RELAY_UNASSIGNED] = "Unassigned";

   rda_status [0] = "Unknown";
   rda_status [1] = "Start-up";
   rda_status [2] = "Standby";
   rda_status [3] = "Restart";
   rda_status [4] = "Operate";
   rda_status [5] = "Playback";
   rda_status [6] = "Off-line Operate";

   if (msg != NULL)
      LE_send_msg (MNGRED_OP_VL, "%s", msg);
   
   LE_send_msg (MNGRED_OP_VL, "        RPG Channel Number: %d", 
                CHAnnel_status.rpg_channel_number);
   
   if ((CHAnnel_status.rpg_channel_state < 0) ||
       (CHAnnel_status.rpg_channel_state > 1))
      LE_send_msg (MNGRED_OP_VL, "             Channel State: Uninitialized");
   else
      LE_send_msg (MNGRED_OP_VL, "             Channel State: %s", 
                   ch_state[CHAnnel_status.rpg_channel_state]);

   if ((CHAnnel_status.rda_control_state < 0)   ||
       (CHAnnel_status.rda_control_state > 1))
         LE_send_msg (MNGRED_OP_VL, "         RDA Control State: Unknown");
   else
      LE_send_msg (MNGRED_OP_VL, "         RDA Control State: %s", 
                   rda_control_state[CHAnnel_status.rda_control_state]);

   if ((CHAnnel_status.comms_relay_state < 0)  ||
       (CHAnnel_status.comms_relay_state > 2))
       LE_send_msg (MNGRED_OP_VL, "               Comms Relay: Unknown");
   else
      LE_send_msg (MNGRED_OP_VL, "               Comms Relay: %s", 
           comms_relay [CHAnnel_status.comms_relay_state]);

   if ((CHAnnel_link_state < 0)   || (CHAnnel_link_state > 2))
         LE_send_msg (MNGRED_OP_VL, "              Channel Link: Unknown");
   else
         LE_send_msg (MNGRED_OP_VL, "              Channel Link: %s",
                      ch_link_state [CHAnnel_link_state]);
   
   if (CHAnnel_status.rda_rpg_wb_link_state == RS_CONNECTED)
      LE_send_msg (MNGRED_OP_VL, "           RDA-RPG WB Link: Connected");
   else
      LE_send_msg (MNGRED_OP_VL, "           RDA-RPG WB Link: Disconnected");

      /* compute the index for the rda state array based off the RDA-RPG 
         ICD state value. 
             the RDA-RPG ICD RDA state value = 2 exp n
             to find the array index (where the index = n):
                      index = (log10 (RDA state value)) / log10 (2) */

   index = (int) ((double)  (log10 ((double) CHAnnel_status.rda_status)) /
                  (double) 0.3);   /* 0.3 = log10 (2.0) */
   
   if ((index < 0) || (index > 6))
         index = 0;

   LE_send_msg (MNGRED_OP_VL, "                RDA Status: %s", rda_status [index]);

   LE_send_msg (MNGRED_OP_VL, "                 RPG State: %d", CHAnnel_status.rpg_state);

   LE_send_msg (MNGRED_OP_VL, "                  RPG Mode: %d", CHAnnel_status.rpg_mode);
   
   LE_send_msg (MNGRED_OP_VL, "   RDA Stat Msg Sequence #: %d", 
                CHAnnel_status.stat_msg_seq_num);


   return;
}


/********************************************************************************

    Description: This routine prints the channel's previous state

          Input:  previous_state - see orpgred.h

         Output:

         Return:

        Globals: CHAnnel_status - see mngred_globals.h & orpgred.h

          Notes:

 ********************************************************************************/

void CST_print_previous_state (Previous_channel_state_t previous_state)
{
   char *ch_state [2];         /* RPG channel states */
   char *rda_control_state[2]; /* RDA channel control states */

   LE_send_msg (MNGRED_OP_VL, "*****Previous State Conditions*****");

   ch_state [ORPGRED_CHANNEL_INACTIVE] = "Inactive";
   ch_state [ORPGRED_CHANNEL_ACTIVE] = "Active";

   rda_control_state [ORPGRED_RDA_CONTROLLING] = "Controlling";
   rda_control_state [ORPGRED_RDA_NON_CONTROLLING] = "Non-Controlling";
   
   if ((previous_state.rpg_channel_state < 0)   ||
       (previous_state.rpg_channel_state > 1))
      LE_send_msg (MNGRED_OP_VL, "         Channel State: Uninitialized");
   else
      LE_send_msg (MNGRED_OP_VL, "         Channel State: %s", 
                   ch_state[previous_state.rpg_channel_state]);

   if ((previous_state.rda_control_state < 0)   ||
       (previous_state.rda_control_state > 1))
         LE_send_msg (MNGRED_OP_VL, "  RDA Control State: Unknown");
   else
      LE_send_msg (MNGRED_OP_VL, "     RDA Control State: %s", 
                   rda_control_state[previous_state.rda_control_state]);
   return;
}

   
/********************************************************************************

    Description: This routine transmits this channel's status to the redundant 
                 channel.

          Input:

         Output:

         Return: 0 on success or when the update has been blocked due to the
                   channel software verion numbers not matching;
                -1 on failure;

        Globals:

          Notes:

 ********************************************************************************/

int CST_transmit_channel_status ()
{
              /* previous transmission failure flag */
   static int previous_transmission_failure = MNGRED_FALSE; 
   int  ret;  /* function call return value */

      /* write the channel status to the redundant channel */

   ret = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                                     (char *) &CHAnnel_status,
                                      sizeof (Channel_status_t),
                                      ORPGRED_REDUN_CHANL_STATUS_MSG, 
                                      NULL, NULL);

  if (ret >= 0)
      LE_send_msg (MNGRED_DEBUG_VL, "Channel Status written to other channel");

   if (ret == -1)
   {
      if (previous_transmission_failure == MNGRED_FALSE)
      {
         LE_send_msg (GL_STATUS | GL_ERROR,
               "%s Failure writing channel status data to redundant channel",
               MNGRED_WARN_ACTIVE);
         previous_transmission_failure = MNGRED_TRUE;
      }
   }
   else if (previous_transmission_failure == MNGRED_TRUE)
   {
      LE_send_msg (GL_STATUS,
            "%s Failure writing channel status data to redundant channel",
            MNGRED_WARN_CLEAR);

      previous_transmission_failure = MNGRED_FALSE;
   }
   return (ret);
}


/********************************************************************************

    Description: This routine updates the channel status information

          Input:

         Output:

         Return:

        Globals: CHAnnel_status     - see mngred_globals.h & orpgred.h
                 CONfiguration_type - see mngred_globals.h

          Notes:

 ********************************************************************************/

int CST_update_channel_status ()
{
   int  ret;                /* function calls return value */
   int status;              /* temp status field for various status reads */
   int status_msg_seq_num;  /* status msg sequence number */
   Mrpg_state_t rpg_state;  /* the state of the rpg */
   static Channel_status_t previous_status; /* previous channel status */
   static time_t last_time_relay_checked = 0;
   static time_t last_time_status_logged;
 
 
   CHAnnel_status.rpg_configuration = CONfiguration_type;

      /* get the RDA status data */

   status = ORPGRDA_get_wb_status (ORPGRDA_WBLNSTAT);
  
      /* update the status of the RDA-RPG WB link (if the header date is 0, 
         then we have not received a valid RDA status message yet) */

   if ((status == RS_CONNECTED) && 
       (ORPGRDA_get_status (-RDA_RPG_MSG_HDR_DATE) != 0))
   {
      CHAnnel_status.rda_rpg_wb_link_state = RS_CONNECTED;
         
         /* get the current RDA status msg sequence number */
   
      status_msg_seq_num = ORPGRDA_get_status (-RDA_RPG_MSG_HDR_SEQ_NUM);

         /* update the RDA Channel Control state after a current status msg has
            been received. A sequence number < 0 specifies that the status
            message was generated by the RPG and not the RDA....we're only 
            interested in RDA updates */

      if ((status_msg_seq_num != CHAnnel_status.stat_msg_seq_num) &&
          (status_msg_seq_num >= 0))
      {
         status = ORPGRDA_get_status (RS_CHAN_CONTROL_STATUS);

         if (status >= 0)  /* ensure the control status is valid */
            CHAnnel_status.rda_control_state = status;
         else
            CHAnnel_status.rda_control_state = ORPGRED_RDA_CONTROL_STATE_UNKNOWN;

         CHAnnel_status.stat_msg_seq_num = status_msg_seq_num;
      }
   }
      /* if the RDA-RPG WB link is not connected, set the wb link state to
         down, and the RDA control state to "unknown" */
   else  
   {
      CHAnnel_status.rda_rpg_wb_link_state = RS_DOWN;
      CHAnnel_status.rda_control_state = ORPGRED_RDA_CONTROL_STATE_UNKNOWN;
   }

      /* update the RDA status */

   status = ORPGRDA_get_status (RS_RDA_STATUS);

   CHAnnel_status.rda_status = status;

      /* update the RDA operability status */

   status = ORPGRDA_get_status (RS_OPERABILITY_STATUS);

   CHAnnel_status.rda_operability_status = status;

      /* update the RPG state and mode */

   ret = ORPGMGR_get_RPG_states (&rpg_state);

   if (ret == 0)
   {
      CHAnnel_status.rpg_state = rpg_state.state;
      CHAnnel_status.rpg_mode = rpg_state.test_mode;
   }

      /* update the comms relay state */

   if ((time (NULL) - last_time_relay_checked) >= 3) {
      CHAnnel_status.comms_relay_state = DIO_read_comms_relay_state ();
      last_time_relay_checked = time (NULL);
   }

      /* update the channel link state */

   CHAnnel_status.rpg_rpg_link_state = CHAnnel_link_state;

      /* update the channel state */

   CHAnnel_status.rpg_channel_state = CHAnnel_state;

      /* On the active channel, update the misc channel state data 
         that is not updated anywhere else */

   if (CHAnnel_state == ORPGRED_CHANNEL_ACTIVE)
       Update_misc_states ();

      /* do not update the channel status message if nothing has changed. 
         We're not considering the last integer in the message because it
         is just the RDA status message sequence number and we're not
         interested when it changes...we're only interested when the
         data changes */

   if (((memcmp((char *) &previous_status, (char *) &CHAnnel_status, 
        (sizeof (Channel_status_t) - sizeof (int)))) == 0)  &&
        ((time(NULL) - last_time_status_logged) < 1800))
         return (0);   

   memcpy (&previous_status, &CHAnnel_status, sizeof (Channel_status_t));

      /* update the local and redundant channel status lbs */

   ret = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS, (char *) &CHAnnel_status, 
                       sizeof (Channel_status_t), 
                       ORPGRED_CHANNEL_STATUS_MSG);

   CST_transmit_channel_status ();

      /* log the changed channel status */

   last_time_status_logged = time (NULL);
   
   CST_print_channel_status ("Channel Status:");

   if (ret < 0)
      LE_send_msg (MNGRED_OP_VL, 
            "Error updating channel status msg LB (err %d)", ret);

   return (0);
}


/********************************************************************************

    Description: This routine saves the previous channel state before terminating

          Input:

         Output:

         Return:

        Globals: CHAnnel_status - see mngred_globals.h & orpgred.h
                 CHAnnel_state  - see mngred_globals.h & orpgred.h

          Notes:

 ********************************************************************************/

void CST_save_channel_state ()
{
   int ret;
   Previous_channel_state_t previous_state;  /* the previous channel state */

       /* save the previous state information */

   previous_state.rda_control_state = CHAnnel_status.rda_control_state;
   previous_state.rpg_channel_state = CHAnnel_state;

      /* write the previous state msg */

   ret = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS, (char *) &previous_state,
                       sizeof (Previous_channel_state_t), 
                       ORPGRED_PREVIOUS_CHANL_STATE);

      /* reset channel link status data for default startup conditions */

   CHAnnel_status.rpg_rpg_link_state = ORPGRED_CHANNEL_LINK_DOWN;
   CHAnnel_status.rda_control_state = ORPGRED_RDA_CONTROL_STATE_UNKNOWN;
   CHAnnel_status.rpg_channel_state = ORPGRED_CHANNEL_INACTIVE;
   CHAnnel_status.rda_rpg_wb_link_state = RS_DOWN;

      /* get the latest rda status message sequence number */
   
   CHAnnel_status.stat_msg_seq_num = ORPGRDA_get_status (-RDA_RPG_MSG_HDR_SEQ_NUM);

      /* update the local channel status lb */

   ret = ORPGDA_write (ORPGDAT_REDMGR_CHAN_MSGS, (char *) &CHAnnel_status, 
                       sizeof (Channel_status_t), 
                       ORPGRED_CHANNEL_STATUS_MSG);

   if (ret < 0)
      LE_send_msg (GL_ERROR, 
                "Error saving channel state to data_id %d, msg_id %d (err %d)",
                ORPGDAT_REDMGR_CHAN_MSGS, ORPGRED_CHANNEL_STATUS_MSG, ret);
   return;
}


/********************************************************************************

    Description: This routine sets the flag to update the misc state data.

          Input:

         Output:

         Return:

        Globals: Update_misc_state_data_req - see file scope global section

          Notes:

 ********************************************************************************/

void CST_set_misc_state_data_flag ()
{
   Update_misc_state_data_req = MNGRED_TRUE;

   return;
}


/********************************************************************************

    Description: This routine checks misc channel state data that is not 
                 checked anywhere else. Any state data that changes or if
                 the global state data update flag is set, then the data is 
                 updated on the redundant channel via IPC commands.

          Input:

         Output:

         Return:

        Globals: Update_misc_state_data_req - see file scope global section

          Notes:

 ********************************************************************************/

static void Update_misc_states ()
{
      /* previous Spot Blanking selection */
   static int previous_spot_blanking = MNGRED_SPOT_BLANKING_NOT_INSTALLED;
      /* previous Super Res state */
   static int previous_sr = (unsigned char) MNGRED_UNINITIALIZED;
      /* previous CMD state */
   static int previous_cmd = (unsigned char) MNGRED_UNINITIALIZED;
      /* previous SAILS state */
   static int previous_sails = (unsigned char) MNGRED_UNINITIALIZED;
      /* previous AVSET state */
   static int previous_avset = (unsigned char) MNGRED_UNINITIALIZED;
      /* current Spot Blanking selection */
   int current_spot_blanking;
      /* current Super Res state */
   int current_sr;
      /* current CMD state */
   int current_cmd;
      /* current SAILS state */
   int current_sails;
      /* current AVSET state */
   int current_avset;

      /* check for a change in spot blanking state */

   current_spot_blanking = ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS);
   
   if (((current_spot_blanking != ORPGRDA_DATA_NOT_FOUND)  &&
       (current_spot_blanking != previous_spot_blanking)) ||
       (Update_misc_state_data_req == MNGRED_TRUE))
   {
      DC_set_IPC_cmd (ORPGRED_UPDATE_SPOT_BLANKING, current_spot_blanking, 
                      0, 0, 0, 0);
      previous_spot_blanking = current_spot_blanking;
   }

      /* check Super Resolution for a state change */

   current_sr = ORPGINFO_is_super_resolution_enabled();

   if ((current_sr != previous_sr) ||
       (Update_misc_state_data_req == MNGRED_TRUE)) {

       if (current_sr)
          DC_set_IPC_cmd (ORPGRED_UPDATE_SR, CRDA_SR_ENAB, 0, 0, 0, 0);
       else
          DC_set_IPC_cmd (ORPGRED_UPDATE_SR, CRDA_SR_DISAB, 0, 0, 0, 0);

       previous_sr = current_sr;
   }

      /* check CMD for a state change */

   current_cmd = ORPGINFO_is_cmd_enabled();

   if ((current_cmd != previous_cmd) ||
       (Update_misc_state_data_req == MNGRED_TRUE)) {

       if (current_cmd)
          DC_set_IPC_cmd (ORPGRED_UPDATE_CMD, CRDA_CMD_ENAB, 0, 0, 0, 0);
       else
          DC_set_IPC_cmd (ORPGRED_UPDATE_CMD, CRDA_CMD_DISAB, 0, 0, 0, 0);

       previous_cmd = current_cmd;
   }

      /* check SAILS for a state change */

   current_sails = ORPGINFO_is_sails_enabled();

   if ((current_sails != previous_sails) ||
       (Update_misc_state_data_req == MNGRED_TRUE)) {

       DC_set_IPC_cmd (ORPGRED_UPDATE_SAILS, current_sails, 0, 0, 0, 0);

       previous_sails = current_sails;
   }

      /* check AVSET for a state change */

   current_avset = ORPGINFO_is_avset_enabled();

   if ((current_avset != previous_avset) ||
       (Update_misc_state_data_req == MNGRED_TRUE)) {

       if (current_avset)
          DC_set_IPC_cmd (ORPGRED_UPDATE_AVSET, CRDA_AVSET_ENAB, 0, 0, 0, 0);
       else
          DC_set_IPC_cmd (ORPGRED_UPDATE_AVSET, CRDA_AVSET_DISAB, 0, 0, 0, 0);

       previous_avset = current_avset;
   }

   Update_misc_state_data_req = MNGRED_FALSE;

   return;
}
