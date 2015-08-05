/********************************************************************************
 
          file:  mngred_download_cmds.c
          
   Description:  This file contains the routines to process RPG-to-RPG and RDA 
                 download commands between the active and inactive channels in 
                 FAA redundant and NWS redundant configurations.

 ********************************************************************************/
   
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/05/14 20:58:33 $
 * $Id: mngred_download_cmds.c,v 1.17 2013/05/14 20:58:33 steves Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */

#include <mngred_globals.h>
#include <rpg_clutter_censor_zones.h>
#include <orpgsite.h>


/* file scope global variables */
       /* Note: These variables are overloaded and used concurrently 
                on both the active and inactive channels */

   /* flag denoting the bypass map needs to be downloaded to the RDA on the 
      inactive channel */
static int Dnld_bypass_map_req = MNGRED_FALSE;

   /* flag denoting the clutter censor zones need to be downloaded to 
      the RDA on the inactive channel */
static int Dnld_clutter_censor_zones_req = MNGRED_FALSE;

   /* the clutter file id to download to the RDA */
static int Clutter_file_id;

   /* the number of clutter zones defined for the file to download to the RDA */
static int Number_clutter_zones;

   /* flag specifying the "select_vcp" cmd needs to be downloaded to 
      the RDA on the inactive channel */
static int Select_vcp_commanded = MNGRED_FALSE;

   /* the selected vcp number to download to the RDA */
static int Vcp_to_select = 0;

   /* flag specifying the "download_vcp" cmd needs to be downloaded to 
      the RDA on the inactive channel */
static int Download_vcp_commanded = MNGRED_FALSE;

   /* the vcp number to download to the RDA */
static int Vcp_to_download = 0;

   /* flag specifying to command inactive channel to update its Adaptation Data
      updated time */
static int Update_adapt_dat_time = MNGRED_FALSE;

   /* the time to update the Adaptation Data updated time to */
static time_t Adapt_dat_time = 0;

   /* flag specifying to update the spot blanking state on the inactive channel */
static int Update_spot_blanking = MNGRED_FALSE;

   /* state to set the spot blanking to */
static int Spot_blanking_state = 0;

   /* flag specifying to update the Super Resolution state on the inactive channel */
static int Update_super_res = MNGRED_FALSE;

   /* state to set Super Resolution to */
static int Super_res_state = 0;

   /* flag specifying to update the Clutter Mitigation Decision (CMD) state on the 
      inactive channel */
static int Update_clutter_md = MNGRED_FALSE;

   /* state to set CMD to */
static int Clutter_md_state = 0;

   /* flag specifying to update AVSET state on the inactive channel */
static int Update_avset = MNGRED_FALSE;

   /* state to set AVSET to */
static int Avset_state = 0;

   /* flag specifying to update SAILS state on the inactive channel */
static int Update_sails = MNGRED_FALSE;

   /* state to set SAILS to */
static int Sails_state = 0;

/********************************************************************************

    Description: This routine checks for any commands pending 

          Input: data_type - type of data we are inquiring about (State Data
                             or Adaptation Data)

         Output:

         Return: TRUE if any commands are pending; otherwise, FALSE is 
                 returned

        Globals: Dnld_bypass_map_req           - see file scope global section
                 Dnld_clutter_censor_zones_req - see file scope global section
                 Download_vcp_commanded        - see file scope global section
                 Select_vcp_commanded          - see file scope global section
                 Update_adapt_dat_time         - see file scope global section
                 Update_spot_blanking          - see file scope global section
                 Update_super_res              - see file scope global section
                 Update_clutter_md             - see file scope global section
                 Update_avset                  - see file scope global section
                 Update_sails                  - see file scope global section

          Notes:

 ********************************************************************************/

int DC_are_cmds_pending (int data_type)
{
   int ret_code = MNGRED_FALSE;
   
   switch (data_type)
   {
      case MNGRED_STATE_DAT:
         if ((Select_vcp_commanded == MNGRED_TRUE)   || 
             (Download_vcp_commanded == MNGRED_TRUE) ||
             (Update_spot_blanking == MNGRED_TRUE)   || 
             (Update_super_res == MNGRED_TRUE)       ||
             (Update_clutter_md == MNGRED_TRUE)      ||
             (Update_avset == MNGRED_TRUE)           ||
             (Update_sails == MNGRED_TRUE))
               ret_code = MNGRED_TRUE;
         else
               ret_code = MNGRED_FALSE;

         break;
         
      case MNGRED_ADAPT_DAT:
         if ((Update_adapt_dat_time == MNGRED_TRUE) ||
             (Dnld_bypass_map_req == MNGRED_TRUE)   || 
             (Dnld_clutter_censor_zones_req == MNGRED_TRUE))
            ret_code = MNGRED_TRUE;
         else
            ret_code = MNGRED_FALSE;

         break;
   }

   return (ret_code);
}


/********************************************************************************

    Description:  This routine checks for unsent IPC channel commands and 
                  unprocessed RDA commands at channel switchover. If any are 
                  found, they are reported then all commands are cleared.

          Input:

         Output:

         Return:

        Globals: CHAnnel_state                 - see mngred_globals.h & orpgred.h
                 RDA_download_required         - see mngred_globals.h
                 SENd_ipc_cmd                  - see mngred_globals.h
                 Dnld_bypass_map_req           - see file scope global section
                 Dnld_clutter_censor_zones_req - see file scope global section
                 Download_vcp_commanded        - see file scope global section
                 Select_vcp_commanded          - see file scope global section
                 Update_spot_blanking          - see file scope global section
                 Update_adapt_dat_time         - see file scope global section
                 Update_super_res              - see file scope global section
                 Update_clutter_md             - see file scope global section
                 Update_avset                  - see file scope global section
                 Update_sails                  - see file scope global section

          Notes:

 ********************************************************************************/

void DC_clear_channel_cmds ()
{
   LE_send_msg (MNGRED_TEST_VL, "IPC channel commands cleared");

   switch (CHAnnel_state)
   {
         /* these commands were not processed before this channel transitioned 
            from active to inactive */

      case ORPGRED_CHANNEL_INACTIVE:

         if (Dnld_bypass_map_req == MNGRED_TRUE)
            LE_send_msg (GL_STATUS | GL_ERROR,
                  "Failed to send \"Download Bypass Map\" command to other channel");

         if (Dnld_clutter_censor_zones_req == MNGRED_TRUE)
            LE_send_msg (GL_STATUS | GL_ERROR,
                  "Failed to send \"Download Clutter Censor Zones\" cmd to other chanl");

         if ((Select_vcp_commanded == MNGRED_TRUE)   ||
             (Download_vcp_commanded == MNGRED_TRUE))
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "Failed to send current VCP data to other channel");

         if (Update_spot_blanking == MNGRED_TRUE)
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "Failed to send Spot Blanking state to other channel");

         if (Update_super_res == MNGRED_TRUE)
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "Failed to send Super Res state to other channel");

         if (Update_clutter_md == MNGRED_TRUE)
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "Failed to send CMD state to other channel");

         if (Update_avset == MNGRED_TRUE)
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "Failed to send AVSET state to other channel");

         if (Update_sails == MNGRED_TRUE)
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "Failed to send SAILS state to other channel");

         SENd_ipc_cmd = MNGRED_FALSE;
                         
         break;
         
         /* these commands were not processed before this channel transitioned 
            from inactive to active */

      case ORPGRED_CHANNEL_ACTIVE:

         if (Dnld_bypass_map_req == MNGRED_TRUE)
            LE_send_msg (GL_ERROR | GL_STATUS, 
                  "Failed to download Bypass Map to the RDA");

         if (Dnld_clutter_censor_zones_req == MNGRED_TRUE)
            LE_send_msg (GL_ERROR | GL_STATUS, 
                  "Failed to download Clutter Censor Zones to the RDA");

         if ((Select_vcp_commanded == MNGRED_TRUE)   ||
             (Download_vcp_commanded == MNGRED_TRUE))
               LE_send_msg (GL_ERROR | GL_STATUS, 
                            "Failed to download current VCP data to the RDA");

         if (Update_spot_blanking == MNGRED_TRUE)
               LE_send_msg (GL_ERROR | GL_STATUS, 
                         "Failed to download Spot Blanking status to the RDA");

         if (Update_super_res == MNGRED_TRUE)
               LE_send_msg (GL_ERROR | GL_STATUS, 
                            "Failed to download Super Res state to the RDA");

         if (Update_clutter_md == MNGRED_TRUE)
               LE_send_msg (GL_ERROR | GL_STATUS, 
                            "Failed to download CMD state to the RDA");

         if (Update_avset == MNGRED_TRUE)
               LE_send_msg (GL_STATUS | GL_ERROR,
                            "Failed to download AVSET state to the RDA");

         RDA_download_required = MNGRED_FALSE;
                         
         break;
   }
   
      /* clear all the channel commands */

   Dnld_bypass_map_req = MNGRED_FALSE;
   Dnld_clutter_censor_zones_req = MNGRED_FALSE;
   Select_vcp_commanded = MNGRED_FALSE;
   Download_vcp_commanded = MNGRED_FALSE;
   Update_adapt_dat_time = MNGRED_FALSE;
   Update_spot_blanking = MNGRED_FALSE;
   Update_super_res = MNGRED_FALSE;
   Update_clutter_md = MNGRED_FALSE;
   Update_avset = MNGRED_FALSE;
   Update_sails = MNGRED_FALSE;

   return;
}


/********************************************************************************

    Description:  This routine processes RDA download commands on the inactive 
                  channel. The download commands are initiated from the active 
                  channel.

          Input:

         Output:

         Return:

        Globals: CHAnnel_status                - see mngred_globals.h & orpgred.h
                 CONfiguration_type            - see mngred_globals.h
                 RDA_download_required         - see mngred_globals.h
                 Clutter_file_id               - see file scope global section
                 Dnld_bypass_map_req           - see file scope global section
                 Dnld_clutter_censor_zones_req - see file scope global section
                 Download_vcp_commanded        - see file scope global section
                 Number_clutter_zones          - see file scope global section
                 Select_vcp_commanded          - see file scope global section
                 Spot_blanking_state           - see file scope global section
                 Update_spot_blanking          - see file scope global section
                 Vcp_to_download               - see file scope global section
                 Vcp_to_select                 - see file scope global section
                 Update_super_res              - see file scope global section
                 Super_res_state               - see file scope global section
                 Update_clutter_md             - see file scope global section
                 Clutter_md_state              - see file scope global section

          Notes:

 ********************************************************************************/

void DC_process_download_commands ()
{
   int return_val = 0;      /* function call return value */
   int rda_control_status;  /* the control status of the rda */

      /* get the current rda control status */

   rda_control_status = ORPGRDA_get_status(RS_CONTROL_STATUS);

      /* if the RDA-RPG WB link is not connected, or the rda control status
         is not in "either" or "remote" then return */

   if ((CHAnnel_status.rda_rpg_wb_link_state != RS_CONNECTED) ||
      ((rda_control_status != CS_RPG_REMOTE)  &&
       (rda_control_status != CS_EITHER)))
         return;
                
      /* The "download clutter censor zones, downlaod VCP, & select VCP" 
         commands will only be processed when the channel is transitioning 
         from Inactive to Active - this prevents a CMD mismatch problem and
         prevents the Adaptation Data from being updated by control_rda when
         the downloaded VCPs change from a Non-SZ2 to a SZ2 VCP. */

   if ((CHAnnel_status.rda_control_state == ORPGRED_RDA_CONTROLLING) &&
       (CHAnnel_state == ORPGRED_CHANNEL_ACTIVE))  {

         /* see if the clutter censor zones have been commanded for download */

      if (Dnld_clutter_censor_zones_req == MNGRED_TRUE)
      {
         return_val = ORPGRDA_send_cmd (COM4_SENDCLCZ, RED_INITIATED_RDA_CTRL_CMD,
            Number_clutter_zones, Clutter_file_id, 0, 0, 0, (char *) NULL);
         if (return_val < 0)
            LE_send_msg (GL_ERROR,
              "Download Clutter Zones write failed, file #: %d, # zones: %d, (ret %d)", 
                 Clutter_file_id, Number_clutter_zones, return_val);
         else
         {
            LE_send_msg (MNGRED_OP_VL,
           "Download Clutter Censor Zones (file %d, zones %d) cmd sent to RDA",
           Clutter_file_id, Number_clutter_zones);
         
            Dnld_clutter_censor_zones_req = MNGRED_FALSE;
         }
      }

         /* see if the "download_vcp" has been commanded.
            Note:  The sequence of processing the vcp commands is order specific.
                   Always process the "download_vcp" command before processing the 
                   "Select_vcp" command.  */

      if (Download_vcp_commanded == MNGRED_TRUE)
      {
         return_val = ORPGRDA_send_cmd (COM4_DLOADVCP, RED_INITIATED_RDA_CTRL_CMD,
            Vcp_to_download, 0, 0, 0, 0, (char *) NULL);

         if (return_val < 0)
            LE_send_msg (GL_ERROR, "Download VCP write failed, VCP #: %d (ret %d)", 
                         Vcp_to_download, return_val);
         else
         {
            LE_send_msg (MNGRED_OP_VL, "\"Download VCP\" cmd sent to RDA");
            Download_vcp_commanded = MNGRED_FALSE;
         }
      }

         /* see if the "select_vcp" has been commanded */

      if ((Select_vcp_commanded == MNGRED_TRUE)      &&
          (Download_vcp_commanded == MNGRED_FALSE))
      {
         return_val = ORPGRDA_send_cmd (COM4_RDACOM, RED_INITIATED_RDA_CTRL_CMD,
            CRDA_SELECT_VCP, Vcp_to_select, 0, 0, 0, (char *) NULL);

         if (return_val < 0)
            LE_send_msg (GL_ERROR,
                  "Select VCP write failed, VCP %d (ret %d)", 
                  Vcp_to_select, return_val);
         else
         {
            LE_send_msg (MNGRED_OP_VL, "\"Select VCP\" cmd sent to RDA");
            Select_vcp_commanded = MNGRED_FALSE;
         }
      }
   }

      /* see if the Bypass Map has been commanded for download */

   if (Dnld_bypass_map_req == MNGRED_TRUE)
   {
      return_val = ORPGRDA_send_cmd (COM4_SENDEDCLBY,
         RED_INITIATED_RDA_CTRL_CMD, 0, 0, 0, 0, 0, (char *) NULL);

      if (return_val < 0)
         LE_send_msg (GL_ERROR, "Download Bypass Map command write failed (ret %d)", 
                      return_val);
      else
      {
         LE_send_msg (MNGRED_OP_VL, "Download Bypass Map cmd sent to RDA");
         Dnld_bypass_map_req = MNGRED_FALSE;
      }
   }

      /* see if the spot blanking state has changed. if the
         commanded state and current state are different, then send 
         the commanded state to the RDA */

   if (Update_spot_blanking)
   {
      if ((Spot_blanking_state != ORPGRDA_get_status (RS_SPOT_BLANKING_STATUS))  &&
          (Spot_blanking_state != MNGRED_SPOT_BLANKING_NOT_INSTALLED))
      {
         if (Spot_blanking_state == MNGRED_SPOT_BLANKING_ENABLED)
               return_val = ORPGRDA_send_cmd (COM4_RDACOM,
	          RED_INITIATED_RDA_CTRL_CMD, CRDA_SB_ENAB, 0, 0, 0, 0, NULL);
         else if (Spot_blanking_state == MNGRED_SPOT_BLANKING_DISABLED)
               return_val = ORPGRDA_send_cmd (COM4_RDACOM,
	          RED_INITIATED_RDA_CTRL_CMD, CRDA_SB_DIS, 0, 0, 0, 0, NULL);

         if (return_val < 0)
            LE_send_msg (GL_ERROR,
             "Failure sending \"change spot blanking\" RDA cmd (cmd state: %d, ret_val: %d)",
             Spot_blanking_state, return_val);
         else
         {
            LE_send_msg (MNGRED_OP_VL,
                         "Change Spot Blanking cmd sent to RDA (cmded state: %d)",
                         Spot_blanking_state);
         
            Update_spot_blanking = MNGRED_FALSE;
         }
      }
      else
         Update_spot_blanking = MNGRED_FALSE;
   }

      /* see if the Super Res state needs downloading */

   if (Update_super_res == MNGRED_TRUE)
   {
      return_val = ORPGRDA_send_cmd (COM4_RDACOM,
         RED_INITIATED_RDA_CTRL_CMD, Super_res_state, 0, 0, 0, 0, (char *) NULL);

      if (return_val < 0)
         LE_send_msg (GL_ERROR, "Download Super Res state write failed (ret %d)", 
                      return_val);
      else
      {
         LE_send_msg (MNGRED_OP_VL, "Download Super Res state cmd sent to RDA (state: %d)",
                      Super_res_state);
         Update_super_res = MNGRED_FALSE;
      }
   }

      /* see if the CMD state needs downloading */

   if (Update_clutter_md == MNGRED_TRUE)
   {
      return_val = ORPGRDA_send_cmd (COM4_RDACOM,
         RED_INITIATED_RDA_CTRL_CMD, Clutter_md_state, 0, 0, 0, 0, (char *) NULL);

      if (return_val < 0)
         LE_send_msg (GL_ERROR, "Download CMD state write failed (ret: %d)", 
                      return_val);
      else
      {
         LE_send_msg (MNGRED_OP_VL, "Download CMD state cmd sent to RDA (state: %d)",
                      Clutter_md_state);
         Update_clutter_md = MNGRED_FALSE;
      }
   }

      /* see if the AVSET state needs downloading */

   if (Update_avset == MNGRED_TRUE)
   {
      return_val = ORPGRDA_send_cmd (COM4_RDACOM,
         RED_INITIATED_RDA_CTRL_CMD, Avset_state, 0, 0, 0, 0, (char *) NULL);

      if (return_val < 0)
         LE_send_msg (GL_ERROR, "Download AVSET state write failed (ret: %d)", 
                      return_val);
      else
      {
         LE_send_msg (MNGRED_OP_VL, "Download AVSET state cmd sent to RDA (state: %d)",
                      Avset_state);
         Update_avset = MNGRED_FALSE;
      }
   }

      /* update the download required flag */

   if (CONfiguration_type == ORPGSITE_NWS_REDUNDANT)
      RDA_download_required = Dnld_clutter_censor_zones_req |
                              Dnld_bypass_map_req;
                               
   else  /* FAA redundancy is assumed */
      RDA_download_required = Dnld_clutter_censor_zones_req |
                              Dnld_bypass_map_req           |
                              Select_vcp_commanded          |
                              Download_vcp_commanded        |
                              Update_spot_blanking          |
                              Update_clutter_md             |
                              Update_super_res;

   return;
}


/********************************************************************************

    Description: This routine sends the IPC command(s) from the active 
                 channel to the inactive channel

          Input:

         Output:

         Return: 0 on success; negative value on error.

        Globals: SENd_ipc_cmd                  - see mngred_globals.h
                 Adapt_dat_time                - see file scope global section
                 Clutter_file_id               - see file scope global section
                 Dnld_bypass_map_req           - see file scope global section
                 Dnld_clutter_censor_zones_req - see file scope global section
                 Download_vcp_commanded        - see file scope global section
                 Number_clutter_zones          - see file scope global section
                 Select_vcp_commanded          - see file scope global section
                 Spot_blanking_state           - see file scope global section
                 Update_adapt_dat_time         - see file scope global section
                 Update_spot_blanking          - see file scope global section
                 Vcp_to_download               - see file scope global section
                 Vcp_to_select                 - see file scope global section
                 Update_super_res              - see file scope global section
                 Update_clutter_md             - see file scope global section
                 Update_avset                  - see file scope global section
                 Avset_state                   - see file scope global section
                 Update_sails                  - see file scope global section
                 Sails_state                   - see file scope global section

          Notes:

 ********************************************************************************/

int DC_send_IPC_cmds ()
{
   int error = 0;                       /* error flag */
   Redundant_channel_msg_t channel_cmd; /* the IPC channel command */
   Lb_table_entry_t *table_entry;       /* LB lookup table ptr */
   int rda_config;

      /* retrieve the RDA configuration we're in...Legacy or ORDA */

   rda_config = ORPGRDA_get_rda_config (NULL);

      /* see if the download clutter censor zones cmd needs to be sent */

   if ((Dnld_clutter_censor_zones_req == MNGRED_TRUE) &&
       (rda_config != ORPGRDA_DATA_NOT_FOUND))
   {

         /* find the Clutter Censor Zones table entry in the lookup table */

      table_entry = MLT_find_table_entry (ORPGDAT_ADAPT_DATA, -1);

      if (table_entry == NULL)
      {
         LE_send_msg (GL_ERROR, 
               "Error locating Clutter Censor Zones entry in lookup table");
         LE_send_msg (GL_ERROR | GL_STATUS,
           "Error sending Download Clutter Censor Zones command to redundant channel");
         Dnld_clutter_censor_zones_req = MNGRED_FALSE;
      }
         /* ensure the clutter zones have been sent to the other channel 
            before sending the download clutter zone cmd */
      else if (table_entry->update_required == MNGRED_FALSE)
      {
            /* block all notifications until all conditions have been
               satisfied for this write */

         LB_NTF_control (LB_NTF_BLOCK);
            
            /* construct the command message then write the command to the redundant
               channel */

         channel_cmd.parameter1 = Number_clutter_zones;
         channel_cmd.parameter2 = Clutter_file_id;

         error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                                (char *) &channel_cmd, 
                                sizeof (Redundant_channel_msg_t), 
                                ORPGRED_DNLOAD_CLUTTER_CENSOR_ZONES, 
                                NULL, NULL);

         if (error < 0)
         {
            LE_send_msg (GL_ERROR, 
             "Error sending \"Dnld Clutter Censor Zones\" cmd to redundant channel");

               /* resume notifications */

            LB_NTF_control (LB_NTF_UNBLOCK);
         }
         else
         {
            LE_send_msg (MNGRED_OP_VL, 
               "\"Download Clutter Censor Zones\" command sent to redundant channel");
         
               /* resume notifications */

            LB_NTF_control (LB_NTF_UNBLOCK);

            Dnld_clutter_censor_zones_req = MNGRED_FALSE;
            
         }
      }
   }

      /* see if the download_vcp cmd needs to be sent */
   
   if (Download_vcp_commanded == MNGRED_TRUE)
   {
         /* block all notifications until all conditions have been
            satisfied for this write */

      LB_NTF_control (LB_NTF_BLOCK);
            
         /* construct the command message then write the command to the redundant
            channel */

      channel_cmd.parameter1 = Vcp_to_download;

      error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                            (char *) &channel_cmd, 
                            sizeof (Redundant_channel_msg_t), 
                            ORPGRED_DOWNLOAD_VCP, 
                            NULL, NULL);

      if (error < 0)
      {
         LE_send_msg (GL_ERROR,
               "Error sending \"download vcp\" cmd to redundant channel");

           /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);
      }
      else
      {
         LE_send_msg (MNGRED_TEST_VL, 
               "\"Download VCP\" command sent to redundant channel");
         
            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);

         Download_vcp_commanded = MNGRED_FALSE;
      }
   }

      /* see if the select_vcp cmd needs to be sent */
   
   if (Select_vcp_commanded == MNGRED_TRUE)
   {
         /* block all notifications until all conditions have been
            satisfied for this write */

      LB_NTF_control (LB_NTF_BLOCK);
            
         /* construct the command message then write the command to the redundant
            channel */

      channel_cmd.parameter1 = Vcp_to_select;

      error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                            (char *) &channel_cmd, 
                            sizeof (Redundant_channel_msg_t), 
                            ORPGRED_SELECT_VCP, 
                            NULL, NULL);

      if (error < 0)
      {
         LE_send_msg (GL_ERROR,
               "Error sending \"select vcp\" cmd to redundant channel");

           /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);
      }
      else
      {
         LE_send_msg (MNGRED_TEST_VL, 
               "\"Select VCP\" command sent to redundant channel");
         
            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);

         Select_vcp_commanded = MNGRED_FALSE;
      }
   }

      /* see if the "update adaptation data time" cmd needs to be sent */
   
   if (Update_adapt_dat_time == MNGRED_TRUE) 
   {
         /* write this channel's channel status to the redundant channel
            now so the "adaptation data mismatch" flag will be reset as
            soon as the adaptation times are updated */

      CST_transmit_channel_status ();

         /* block all notifications until all conditions have been
            satisfied for this write */

      LB_NTF_control (LB_NTF_BLOCK);
            
         /* construct the command message then write the command to the redundant
            channel */

      channel_cmd.parameter1 = Adapt_dat_time;

      error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                            (char *) &channel_cmd, 
                            sizeof (Redundant_channel_msg_t), 
                            ORPGRED_UPDATE_ADAPT_DATA_TIME, 
                            NULL, NULL);

      if (error < 0)
      {
         LE_send_msg (GL_ERROR, 
            "Error sending \"update Adapt Dat time\" cmd to redundant channel");

               /* resume notifications */

            LB_NTF_control (LB_NTF_UNBLOCK);
      }
      else
      {
         LE_send_msg (MNGRED_OP_VL, 
               "\"Update Adapt Dat Time\" command sent to redundant channel");
         
            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);

         Update_adapt_dat_time = MNGRED_FALSE;
      }
   }

      /* see if the "update spot blanking" cmd needs to be sent */
   
   if (Update_spot_blanking == MNGRED_TRUE)
   {
         /* block all notifications until all conditions have been
            satisfied for this write */

      LB_NTF_control (LB_NTF_BLOCK);
            
         /* construct the command message then write the command to the redundant
            channel */

      channel_cmd.parameter1 = Spot_blanking_state;

      error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                            (char *) &channel_cmd, 
                            sizeof (Redundant_channel_msg_t), 
                            ORPGRED_UPDATE_SPOT_BLANKING, 
                            NULL, NULL);

      if (error < 0)
      {
         LE_send_msg (GL_ERROR, 
            "Error sending \"update Spot Blanking\" cmd to redundant channel");

            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);
      }
      else
      {
         LE_send_msg (MNGRED_OP_VL, 
               "\"Update Spot Blanking\" command sent to redundant channel");
         
            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);

         Update_spot_blanking = MNGRED_FALSE;
      }
   }

      /* see if the "update Super Res" cmd needs to be sent */
   
   if (Update_super_res == MNGRED_TRUE)
   {
         /* block all notifications until all conditions have been
            satisfied for this write */

      LB_NTF_control (LB_NTF_BLOCK);
            
         /* construct the command message then write the command to the redundant
            channel */

      channel_cmd.parameter1 = Super_res_state;

      error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                            (char *) &channel_cmd, 
                            sizeof (Redundant_channel_msg_t), 
                            ORPGRED_UPDATE_SR,
                            NULL, NULL);

      if (error < 0)
      {
         LE_send_msg (GL_ERROR, 
            "Error sending \"update Super Res\" cmd to redundant channel");

            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);
      }
      else
      {
         LE_send_msg (MNGRED_OP_VL, 
               "\"Update Super Res\" command sent to redundant channel");
         
            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);

         Update_super_res = MNGRED_FALSE;
      }
   }

      /* see if the "update CMD" cmd needs to be sent */
   
   if (Update_clutter_md == MNGRED_TRUE)
   {
         /* block all notifications until all conditions have been
            satisfied for this write */

      LB_NTF_control (LB_NTF_BLOCK);
            
         /* construct the command message then write the command to the redundant
            channel */

      channel_cmd.parameter1 = Clutter_md_state;

      error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                            (char *) &channel_cmd, 
                            sizeof (Redundant_channel_msg_t), 
                            ORPGRED_UPDATE_CMD,
                            NULL, NULL);

      if (error < 0)
      {
         LE_send_msg (GL_ERROR, 
            "Error sending \"update CMD\" cmd to redundant channel");

            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);
      }
      else
      {
         LE_send_msg (MNGRED_OP_VL, 
               "\"Update CMD\" command sent to redundant channel");
         
            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);

         Update_clutter_md = MNGRED_FALSE;
      }
   }

      /* see if the "update AVSET" cmd needs to be sent */
   
   if (Update_avset == MNGRED_TRUE) 
   {
         /* block all notifications until all conditions have been
            satisfied for this write */

      LB_NTF_control (LB_NTF_BLOCK);
            
         /* construct the command message then write the command to the redundant
            channel */

      channel_cmd.parameter1 = Avset_state;

      error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                            (char *) &channel_cmd, 
                            sizeof (Redundant_channel_msg_t), 
                            ORPGRED_UPDATE_AVSET, 
                            NULL, NULL);

      if (error < 0)
      {
         LE_send_msg (GL_ERROR,
            "Error sending \"update AVSET\" cmd to redundant channel");

            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);
      }
      else
      {
         LE_send_msg (MNGRED_OP_VL, 
               "\"Update AVSET\" command sent to redundant channel");

            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);

         Update_avset = MNGRED_FALSE;
      }
   }

      /* see if the "update SAILS" cmd needs to be sent */
   
   if (Update_sails == MNGRED_TRUE) 
   {
         /* block all notifications until all conditions have been
            satisfied for this write */

      LB_NTF_control (LB_NTF_BLOCK);
            
         /* construct the command message then write the command to the redundant
            channel */

      channel_cmd.parameter1 = Sails_state;

      error = WCD_write_redundant_lb_data (ORPGDAT_REDMGR_CHAN_MSGS, 
                            (char *) &channel_cmd, 
                            sizeof (Redundant_channel_msg_t), 
                            ORPGRED_UPDATE_SAILS, 
                            NULL, NULL);

      if (error < 0)
      {
         LE_send_msg (GL_ERROR,
            "Error sending \"update SAILS\" cmd to redundant channel");

            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);
      }
      else
      {
         LE_send_msg (MNGRED_OP_VL, 
               "\"Update SAILS\" command sent to redundant channel");

            /* resume notifications */

         LB_NTF_control (LB_NTF_UNBLOCK);

         Update_sails = MNGRED_FALSE;
      }
   }
 
      /* clear the global flag if all download commands have been sent */

   if ((Dnld_bypass_map_req == MNGRED_FALSE)           &&
       (Dnld_clutter_censor_zones_req == MNGRED_FALSE) &&
       (Select_vcp_commanded == MNGRED_FALSE)          &&
       (Download_vcp_commanded == MNGRED_FALSE)        &&
       (Update_adapt_dat_time == MNGRED_FALSE)         &&
       (Update_spot_blanking == MNGRED_FALSE)          &&
       (Update_super_res == MNGRED_FALSE)              &&
       (Update_clutter_md == MNGRED_FALSE)             &&
       (Update_avset == MNGRED_FALSE)                  &&
       (Update_sails == MNGRED_FALSE))
          SENd_ipc_cmd = MNGRED_FALSE;

   return (error);
}


/********************************************************************************

    Description:  This routine sets the RDA download commands/variables on the 
                  inactive channel. These commands will be sent to this channel's
                  RDA by another routine.

          Input: channel_cmd - the RDA command received from the active
                               channel

         Output:

         Return:

        Globals: RDA_download_required         - see mngred_globals.h
                 Clutter_file_id               - see file scope global section
                 Dnld_bypass_map_req           - see file scope global section
                 Dnld_clutter_censor_zones_req - see file scope global section
                 Download_vcp_commanded        - see file scope global section
                 Number_clutter_zones          - see file scope global section
                 Select_vcp_commanded          - see file scope global section
                 Spot_blanking_state           - see file scope global section
                 Update_spot_blanking          - see file scope global section
                 Vcp_to_download               - see file scope global section
                 Vcp_to_select                 - see file scope global section
                 Update_super_res              - see file scope global section
                 Update_clutter_md             - see file scope global section
                 Update_avset                  - see file scope global section

          Notes:

 ********************************************************************************/

void DC_set_download_cmd (Redundant_channel_msg_t channel_cmd, LB_id_t msgid)
{
   switch (msgid)
   {
      case ORPGRED_SELECT_VCP:  /* cmd to select new VCP */
         Select_vcp_commanded = MNGRED_TRUE;
         Vcp_to_select = channel_cmd.parameter1;

         LE_send_msg (MNGRED_TEST_VL,
            "Select VCP cmd rec'd from other channel");
         break;

      case ORPGRED_DOWNLOAD_VCP: /* cmd to download new VCP to RDA */
         Download_vcp_commanded = MNGRED_TRUE;
         Vcp_to_download = channel_cmd.parameter1;

         LE_send_msg (MNGRED_TEST_VL,
            "Download VCP cmd rec'd from other channel");
         break;

      case ORPGRED_DNLOAD_CLUTTER_CENSOR_ZONES:/* cmd to dnld censor zones to RDA */
         Dnld_clutter_censor_zones_req = MNGRED_TRUE;
         Number_clutter_zones = channel_cmd.parameter1;
         Clutter_file_id = channel_cmd.parameter2;

         LE_send_msg (MNGRED_OP_VL,
            "Dnld Clutter Zones cmd rec'd from other channel (file id %d, #zones %d)",
            Clutter_file_id, Number_clutter_zones);
         break;

      case ORPGRED_UPDATE_SPOT_BLANKING:  /* cmd to change the spot blanking state */
         Update_spot_blanking = MNGRED_TRUE;
         Spot_blanking_state = channel_cmd.parameter1;

         LE_send_msg (MNGRED_TEST_VL,
            "Update Spot Blanking cmd rec'd from other channel");
         break;
 
      case ORPGRED_UPDATE_SR:  /* cmd to change the Super Res state */
         Update_super_res = MNGRED_TRUE;
         Super_res_state = channel_cmd.parameter1;

         LE_send_msg (MNGRED_TEST_VL,
            "Update Super Res cmd rec'd from other channel");
         break;

      case ORPGRED_UPDATE_CMD:  /* cmd to change the CMD state */
         Update_clutter_md = MNGRED_TRUE;
         Clutter_md_state = channel_cmd.parameter1;

         LE_send_msg (MNGRED_TEST_VL,
            "Update CMD cmd rec'd from other channel");
         break;

      case ORPGRED_UPDATE_AVSET:  /* cmd to change the AVSET state */
         Update_avset = MNGRED_TRUE;
         Avset_state =  channel_cmd.parameter1;

         LE_send_msg (MNGRED_TEST_VL,
            "Update AVSET cmd rec'd from other channel");
         break;

      default:
         LE_send_msg (MNGRED_OP_VL, 
               "Invalid download command received from active channel (cmd %d)",
               msgid);
         return;
   }

   RDA_download_required = MNGRED_TRUE;

   return;
}


/********************************************************************************

    Description: This routine sets the active channel IPC commands and command 
                 parameters that are to be sent to the inactive channel.

          Input: cmd        - the IPC command to send to the inactive channel
                 parameter1 - additional command parameter
                 parameter2 - additional command parameter
                 parameter3 - additional command parameter
                 parameter4 - additional command parameter
                 parameter5 - additional command parameter

         Output:

         Return:

        Globals: SENd_ipc_cmd                  - see mngred_globals.h
                 Adapt_dat_time                - see file scope global section
                 Clutter_file_id               - see file scope global section
                 Dnld_bypass_map_req           - see file scope global section
                 Dnld_clutter_censor_zones_req - see file scope global section
                 Download_vcp_commanded        - see file scope global section
                 Number_clutter_zones          - see file scope global section
                 Select_vcp_commanded          - see file scope global section
                 Spot_blanking_state           - see file scope global section
                 Update_adapt_dat_time         - see file scope global section
                 Update_spot_blanking          - see file scope global section
                 Vcp_to_download               - see file scope global section
                 Vcp_to_select                 - see file scope global section
                 Update_super_res              - see file scope global section
                 Super_res_state               - see file scope global section
                 Update_clutter_md             - see file scope global section
                 Clutter_md_state              - see file scope global section
                 Update_avset                  - see file scope global section
                 Avset_state                   - see file scope global section
                 Update_sails                  - see file scope global section
                 Sails_state                   - see file scope global section

          Notes: 1. Not all commands utilize the command parameters.
                 2. Refer to orpgred.h for the list of IPC channel command 
                    definitions.

 ********************************************************************************/

void DC_set_IPC_cmd (int cmd, int parameter1, int parameter2, int parameter3, 
                     int parameter4, int parameter5)
{
   switch (cmd)
   {
      case ORPGRED_DNLOAD_CLUTTER_CENSOR_ZONES:
         Dnld_clutter_censor_zones_req = MNGRED_TRUE;
         Number_clutter_zones = parameter1;
         Clutter_file_id = parameter2;

         break;
         
      case ORPGRED_SELECT_VCP:
         Select_vcp_commanded = MNGRED_TRUE;
         Vcp_to_select = parameter1;

         break;
         
      case ORPGRED_DOWNLOAD_VCP:
         Download_vcp_commanded = MNGRED_TRUE;
         Vcp_to_download = parameter1;

         break;

      case ORPGRED_UPDATE_ADAPT_DATA_TIME:
         Update_adapt_dat_time = MNGRED_TRUE;
         Adapt_dat_time = parameter1;

         break;

      case ORPGRED_UPDATE_SPOT_BLANKING:
         Update_spot_blanking = MNGRED_TRUE;
         Spot_blanking_state = parameter1;

         break;

      case ORPGRED_UPDATE_SR:
         Update_super_res = MNGRED_TRUE;
         Super_res_state = parameter1;

         break;

      case ORPGRED_UPDATE_CMD:
         Update_clutter_md = MNGRED_TRUE;
         Clutter_md_state = parameter1;

         break;

      case ORPGRED_UPDATE_AVSET:
         Update_avset = MNGRED_TRUE;
         Avset_state = parameter1;

         break;

      case ORPGRED_UPDATE_SAILS:
         Update_sails = MNGRED_TRUE;
         Sails_state = parameter1;

         break;

      default:
         LE_send_msg (MNGRED_OP_VL, 
                      "Invalid set_IPC_command received (cmd %d)",
               cmd);
         return;
         break;
   }

   SENd_ipc_cmd = MNGRED_TRUE;

   return;
}


/********************************************************************************

    Description: This routine sets the bypass map and clutter censor zones
                 download required flags in a NWS redundant configuration.

          Input: cmd_msg - the download bypass map or download clutter
                           censor zones command to execute on channel switchover

         Output:

         Return:

        Globals: Clutter_file_id               - see file scope global section
                 Dnld_bypass_map_req           - see file scope global section
                 Dnld_clutter_censor_zones_req - see file scope global section
                 Number_clutter_zones          - see file scope global section

          Notes: This routine is used only in NWS redundant configurations

 ********************************************************************************/

void DC_set_redun_rda_dnld_cmd (Redundant_cmd_t cmd_msg)
{
   switch (cmd_msg.cmd)
   {
      case ORPGRED_DOWNLOAD_CLUTTER_ZONES:
         Dnld_clutter_censor_zones_req = MNGRED_TRUE;

         Number_clutter_zones = cmd_msg.parameter1;
         Clutter_file_id = cmd_msg.parameter2;

         break;
      default:
         LE_send_msg (MNGRED_OP_VL, "Invalid download command received (cmd %d)",
                      cmd_msg.cmd);
         return;
         break;
   }

   RDA_download_required = MNGRED_TRUE;

   return;
}
