/********************************************************************************

        file: rdasim_driver.c

        Description:  This file contains the main RDA simulator driver routines.

 ********************************************************************************/       

/* 
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:49:31 $
 * $Id: rdasim_driver.c,v 1.51 2014/03/18 18:49:31 jeffs Exp $
 * $Revision: 1.51 $
 * $State: Exp $
 */  


#include <stdio.h>

#include <rdasim_simulator.h>
#include <rdasim_externals.h>
#include <rda_control.h>
#include <rda_status.h>


#include <errno.h>


extern int errno;  /* error number passed back from system calls */

        /*  file scope globals  */

    /* send status message flag */
static short Send_status_msg = TRUE;

    /* send performance data flag */
static unsigned short Send_performance_data;

    /* send bypass map flag */
static unsigned short Send_bypass_map_data;

    /* send notchwidth map flag */
static unsigned short Send_notchwidth_map_data;

    /* send RDA Adaptation Data flag (ORDA configuration only) */
static unsigned short Send_rda_adapt_data;

    /* send VCP Message/Data flag */
static unsigned short Send_vcp_data;

    /* commanded change of super resolution state */
static short Change_super_res = FALSE;

    /* commanded super resolution value */
static short Commanded_super_res = SRS_ENABLED;

    /* commanded change of CMD state */
static short Change_cmd = FALSE;

    /* commanded CMD value */
static short Commanded_cmd = CMDS_ENABLED;

    /* commanded change of AVSET state */
static short Change_avset = FALSE;

    /* commanded AVSET value */
static short Commanded_avset = AVSETS_DISABLED;

    /* commanded change of CMD state */
static short Change_perf_check = FALSE;

    /* commanded CMD value */
static short Commanded_perf_check = PC_PENDING;

    /* new commanded state for the RDA */
static int Commanded_state = NO_COMMAND_PENDING;

    /* current processing state of the simulator */
static int Processing_state = STANDBY;

    /* user commanded power source */
static int Commanded_power_state = NO_COMMAND_PENDING;

    /* the following are commands sent to the simulator from the interface
       test tool (rdasim_tst) */

static int Toggle_channel_number;    /* toggle channel number (1 or 2)...is
                                        only functional in redundant mode */
static int Change_chanl_status;      /* change channel control status (ctl/non-ctl).
                                        is functional only in redundant mode */
static int RDA_loopback_ack = FALSE; /* RDA Loopback acknowledgement flag */

static int Offline_maintenance_mode = FALSE;
static int Toggle_offline_maintenance_mode = FALSE;


    /* local functions */

static void Beginning_of_vol_scan ();
static void Change_pwr_source (struct timeval curent_time);
static void End_of_elevation (int last_time_processing_state);
static void End_of_vol_scan (int last_time_processing_state);
static void Operate_state (int last_time_processing_state);
static void Process_rda_state_change_cmd (unsigned short command);
static void Rda_restart (struct timeval curent_time, 
                         int *last_time_processing_state);
static void Standby_state (struct timeval curent_time, 
                           int last_time_processing_state);
static void Startup_state ();
static void Vcp_elevation_restart (struct timeval curent_time);


/**************************************************************************

    Description: This routine performs the wideband connection

          Input:

         Output:

         Return: 0 on success, or -1 on failure.

**************************************************************************/

int RD_connect_link ()
{
      /* return if the RDA is "restarting" */

   if (Processing_state == RDA_RESTART)
      return (-1);

    msleep (10);   /* wait 10 msec before connecting */

    LINk.link_state = LINK_CONNECTED;

       /* set the metadata message flags that are dependent on being sent
          after a wideband connection is made */

    Send_rda_adapt_data = TRUE;
    Send_status_msg = TRUE;
    Send_performance_data = TRUE;
    Send_bypass_map_data = TRUE;
    Send_notchwidth_map_data = TRUE;
    Send_vcp_data = TRUE;

    PR_send_response(&(LINk.req[(int)LINk.conn_req_ind]),
                     CM_SUCCESS);
    return (0);
}


/**************************************************************************

    Description: This routine performs the wideband disconnection

          Input:

         Output:

         Return: 0 on success, or -1 on failure.

**************************************************************************/

int RD_disconnect_link ()
{
       /* reset all active write pending requests */

    PR_disconnect_cleanup ();

    msleep (10);   /* wait 10 msec before connecting */

    LINk.link_state = LINK_DISCONNECTED;

      /* reset RDA Loopback acknowledgement flag */

    RDA_loopback_ack = 0; 

    PR_send_response(&(LINk.req[(int)LINk.conn_req_ind]),
                     CM_SUCCESS);

    return (0);
}


/**************************************************************************

    Description: Return the simulator's current processing state

          Input:
 
         Output:

         Return: The current simulator processing state

**************************************************************************/

int RD_get_processing_state ()
{
   return (Processing_state);
}


/********************************************************************************
  
    Description: Process all the RPG-to-RDA control commands

          Input:  

         Output: modified fields of the RDA status message +
                 processing state variables

         Return:

 ********************************************************************************/

#define NUMBER_HALFWORDS 26  /* # halfwords in a control command */

void RD_process_control_command ()
{
   unsigned short control_cmd_buffer [NUMBER_HALFWORDS];
   int index;
   unsigned short command_value;
   unsigned short default_rda_control [NUMBER_HALFWORDS] = 
                                         { 0, 0, 0, 0, 0, 32767,
                                       32767, 0, 0, 0, 0,     0,
                                           0, 0, 0, 0, 0,     0, 
                                           0, 0, 0, 0, 0,     0,
                                           0, 0 };

      /* align the control command buffer to halfword boundary */

   memcpy (&control_cmd_buffer, LINk.w_buf + sizeof (CTM_header_t) +
           sizeof (RDA_RPG_message_header_t), (NUMBER_HALFWORDS * sizeof (short)));

   if (VErbose_mode >= 3)
      fprintf (stdout, "     Process RPG control commands:\n");

   for (index = ZERO; index < NUMBER_HALFWORDS; index++) {
         /* once a difference is found, break out of the loop and process the
            command then continue checking the rest of the message fields */

      command_value = control_cmd_buffer [index];

      if (default_rda_control[index] == command_value)
            continue;

        /* if the RDA is in local and the command is not a request for remote
           control then send back an alarm */

      if (RDA_status_msg.control_status == RCS_RDA_LOCAL_ONLY) {
         if ((index != RDC_CONTROL)    ||
            (index == RDC_CONTROL && command_value != RCOM_REQREMOTE)) {
                RRM_set_rda_alarm (RDA_ALARM_INVALID_RPG_COMMAND_RECEIVED);
         }
         return;
      }

         /* if a control cmd is received and the control status is in 
            either, set the control status to remote */

      if ((RDA_status_msg.control_status == RCS_RDA_EITHER)  &&
          (RDA_status_msg.rda_status == RDS_OPERATE)) {
          RDA_status_msg.control_status = RCS_RDA_REMOTE_ONLY;
          Send_status_msg = TRUE;
      }

      switch (index) {
         case RDC_STATE:           /*  RDA State                   */
            Process_rda_state_change_cmd (command_value);
            break;

         case RDC_AUXGEN:          /*  Auxilliary Power/Generator  */
            Commanded_power_state = command_value;

            if (VErbose_mode >= 3)
                fprintf (stdout, 
                         "         change Aux Pwr Generator Control: %d\n",
                         command_value);
            break;

         case RDC_CONTROL:         /*  Control Authorization       */
            if (command_value == RCOM_REQREMOTE) {
                 RDA_status_msg.rda_control_auth = RCA_REMOTE_CONTROL_ENABLED;
                 RDA_status_msg.control_status = RCS_RDA_REMOTE_ONLY;
            }

            if (command_value ==  RCOM_ENALOCAL) {
                 RDA_status_msg.rda_control_auth = RCA_NO_ACTION;
                 RDA_status_msg.control_status = RCS_RDA_EITHER;
            }

            Send_status_msg = TRUE;

            if (VErbose_mode >= 3)
                fprintf (stdout, 
                         "         change Control Authorization: %d\n",
                         command_value);
            break;

         case RDC_RESCAN:          /*  Restart VCP/Elevation Cut   */
         
            if (IGNore_volume_elevation_restart == TRUE) {
               IGNore_volume_elevation_restart = FALSE;
               break;
            }
            Commanded_state = VCP_ELEVATION_RESTART;
            REstart_elevation_cut = command_value & 127;
    
            if (VErbose_mode >= 3)
                fprintf (stdout, 
                         "         Restart VCP/Elevation Cut %d\n",
                         REstart_elevation_cut);
            break;

         case RDC_SELVCP:          /*  Select VCP for Next Scan    */
            if ((RRM_get_remote_vcp () == NULL) && (command_value == 0))
               RRM_set_rda_alarm (RDA_ALARM_REMOTE_VCP_NOT_DOWNLOADED);
            else {
               CR_new_vcp_selected (command_value);

               if (VErbose_mode >= 3)
                   fprintf (stdout, 
                         "         Select VCP %d for Next Vol Scan\n", 
                          command_value);
            }
            break;

         case RDC_AUTOCAL:
            /* Place holder to avoid error message. */
            break;

         case RDC_SUPER_RES:       /* Super Resolution Control     */
	    if (RDA_status_msg.rda_status == RDS_STANDBY) {

	         RDA_status_msg.super_res = command_value;
		 Send_status_msg = TRUE;

	    }

            Change_super_res = TRUE;
            Commanded_super_res = command_value;

            CR_new_vcp_selected( INVALID_PATTERN_NUMBER );

            if (VErbose_mode >= 3)
                fprintf (stdout, 
                         "         change Super Resolution State: %d\n",
                         command_value);

            break;

         case RDC_CMD:     /* Clutter Mitigation Decision Control  */
	    if (RDA_status_msg.rda_status == RDS_STANDBY) {

                 if( command_value == RCOM_ENABLE_CMD )
	            RDA_status_msg.cmd = 0x7;

                 else if( command_value == RCOM_DISABLE_CMD )
	            RDA_status_msg.cmd = 0;

		 Send_status_msg = TRUE;

	    }

            Change_cmd = TRUE;
            Commanded_cmd = command_value;

            if (VErbose_mode >= 3)
                fprintf (stdout, 
                         "         change Clutter Mitigation Decision State: %d\n",
                         command_value);

             break;

         case RDC_AVSET:     /* Automated Volume Scan Evaluation and Termination. */
            if (RDA_status_msg.rda_status == RDS_STANDBY) {

                RDA_status_msg.avset = command_value;
                Send_status_msg = TRUE;

            }

            Change_avset = TRUE;
            Commanded_avset = command_value;

            if (VErbose_mode >= 3)
                fprintf (stdout,
                         "         change AVSET State: %d\n",
                         command_value);

             break;

         case RDC_SELOPMODE:       /*  Select Operating Mode       */
             
               /* if maintenance selected, set the status mode but keep 
                  the simulator operational */

            if ((RDA_status_msg.op_mode & command_value) != command_value) {
               RDA_status_msg.op_mode = command_value;
               Send_status_msg = TRUE;
            }

            if (VErbose_mode >= 3)
                fprintf (stdout, 
                         "         change Operating Modes: %d\n",
                         command_value);
             break;

         case RDC_CCTL_STATUS:     /*  Channel Control Command     */
             if ((command_value == 1) &&
                 (RDA_status_msg.channel_status != CCS_CONTROLLING)) {
                RDA_status_msg.channel_status = CCS_CONTROLLING;
                Send_status_msg = TRUE;
             }
             else if ((command_value == 2) &&
                      (RDA_status_msg.channel_status != CCS_NON_CONTROLLING)) {
                RDA_status_msg.channel_status = CCS_NON_CONTROLLING;
                Send_status_msg = TRUE;
             }

             msleep (5000); /* suspend some arbitrary time to emulate xsition time */

             if (VErbose_mode >= 3)
                fprintf (stdout, 
                         "         change Channel Control: %d\n",
                         command_value);
             break;

         case RDC_PERF_CHECK:     /* Perform Performance Check. */

            RDA_status_msg.perf_check_status = PC_PENDING;
            Send_status_msg = TRUE;

            Change_perf_check = TRUE;
            Commanded_perf_check = PC_AUTO;

            if (VErbose_mode >= 3)
                fprintf (stdout,
                         "         execute Perf Check: %d\n",
                         command_value);

             break;

         case RDC_SB:              /*  Spot Blanking               */
            if ((command_value == RCOM_SB_ENAB) &&
                (RDA_status_msg.spot_blanking_status != SBS_ENABLED)) {
               RDA_status_msg.spot_blanking_status = SBS_ENABLED;
               Send_status_msg = TRUE;
            }
            else if ((command_value == RCOM_SB_DIS) &&
                     (RDA_status_msg.spot_blanking_status != SBS_DISABLED)) {
               RDA_status_msg.spot_blanking_status = SBS_DISABLED;
               Send_status_msg = TRUE;
            }

            if (VErbose_mode >= 3)
                fprintf (stdout, 
                         "         change Spot Blanking: %d\n",
                         command_value);

            break;
  
         default:
            fprintf (stderr, "     invalid control command received: ");
            fprintf (stderr, "     cmd: %x; index: %d\n", command_value, index);
            RRM_set_rda_alarm (RDA_ALARM_INVALID_RPG_COMMAND_RECEIVED);
      }
   }

   return;
}


/********************************************************************************

    Description: This routine processes the commands received from the rdasim_tst
                 interface tool

          Input: event - the event received from the test tool

         Output:

         Return:

********************************************************************************/

void RD_process_tst_tool_cmd (int event)
{
   switch (event)
   {
      case COMMAND_RDA_TO_LOCAL:
         RDA_status_msg.control_status = RCS_RDA_LOCAL_ONLY;
         Send_status_msg = TRUE;
         break;

      case COMMAND_RDA_TO_REMOTE:
         RDA_status_msg.control_status = RCS_RDA_REMOTE_ONLY;
         Send_status_msg = TRUE;
         break;

      case TOGGLE_CHANNEL_NUMBER:
         if (RDA_channel_number != 0) {
            Toggle_channel_number = TRUE;
            Commanded_state = STANDBY;
            msleep (3000); 
         }
         break;

      case CHANGE_CHANNEL_CONTROL_STATUS:
         if (RDA_channel_number != 0) {
            Change_chanl_status = TRUE;
            Commanded_state = STANDBY;
            msleep (3000);
         }
         break;

      case TOGGLE_MAINTENANCE_MODE:
         Toggle_offline_maintenance_mode = TRUE;
         if( Offline_maintenance_mode == FALSE ){

            Offline_maintenance_mode = TRUE;
            RDA_status_msg.op_mode = OP_OFFLINE_MAINTENANCE_MODE;
            Send_status_msg = TRUE;

         }
         else{

            Offline_maintenance_mode = FALSE;
            RDA_status_msg.op_mode = ROM_OPERATIONAL;

         }

         break;
   }
   return;
}


/**************************************************************************

    Description: This routine sets the "request for data" flag for the data
                 requested by the rpg.

          Input:

         Output:     

         Return:     

**************************************************************************/

void RD_set_request_for_data_flag (int request)
{
   switch (request)
   {
      case REQUEST_FOR_STATUS_DATA:
         Send_status_msg = TRUE;
         break;

      case REQUEST_FOR_PERFORMANCE_DATA:
         Send_performance_data = TRUE;
         break;

      case REQUEST_FOR_BYPASS_MAP_DATA:
         Send_bypass_map_data = TRUE;
         break;

      case REQUEST_FOR_NOTCHWIDTH_MAP_DATA:
         Send_notchwidth_map_data = TRUE;
         break;

      default:
         break;
   }
   return;
}


/**************************************************************************

    Description: This routine sets the RDA loopback ack flag

          Input:

         Output:     

         Return:     

**************************************************************************/

void RD_set_rda_loopback_ack_flag ()
{
   RDA_loopback_ack = TRUE;
   return;
}


/**************************************************************************

    Description: This routine sets the "send status msg" flag

          Input:

         Output:     

         Return:     

**************************************************************************/

void RD_set_send_status_msg_flag ()
{
   Send_status_msg = TRUE;
   return;
}

/**************************************************************************

    Description: This routine sets the "send rda vcp" flag

          Input:

         Output:     

         Return:     

**************************************************************************/

void RD_set_send_rda_vcp_flag ()
{
   Send_vcp_data = TRUE;
   return;
}


/**************************************************************************

    Description: This routine performs most of the RDA functionality.

          Input:

         Output: RDA_status_msg   - rda status msg fields
                 Processing_state - the current simulator processing state

         Return:

**************************************************************************/

void RD_run_simulator ()
{
       /* the current time of day */
   static struct timeval current_time = {0, 0};  

       /* last pass's processing state - used for printing state info only */
   static int n_1_Processing_state = -9999;


      /* if the link disconnects and the simulator is not in a "Restart" state,
         then set the current processing state to standby */

   if ((LINk.link_state == LINK_DISCONNECTED)  &&
       (Processing_state != RDA_RESTART)       &&
       (Processing_state != STANDBY)) {
       Processing_state = STANDBY;
       Commanded_state = STANDBY;
   }

      /* a new command will either be processed or ignored 
         depending on the current processing state of the RDA */

   if ((Commanded_state != NO_COMMAND_PENDING) &&
       (Commanded_state != Processing_state)) {
      if ((Processing_state != START_UP) &&
          (Processing_state != RDA_RESTART)) {
         if (Commanded_state == OPERATE)
            Processing_state = START_UP;
         else
            Processing_state = Commanded_state;

         Send_status_msg = TRUE;
      }
   }

   Commanded_state = NO_COMMAND_PENDING;

   gettimeofday (&current_time, NULL);  /* get the current time of day */

       /* see if another power source has been commanded */

   if (Commanded_power_state != NO_COMMAND_PENDING)
       Change_pwr_source (current_time);

   if (VErbose_mode >= 2) {
      if (Processing_state != n_1_Processing_state)
         fprintf (stdout, "Processing_state changed to %d\n", 
                  Processing_state);
   }

       /* continue from last time's processing state, or
          change simulator states due to a new control command */

   switch (Processing_state) {
      case STANDBY:
         Standby_state (current_time, n_1_Processing_state);
         break;

      case RDA_RESTART:
         Rda_restart (current_time, &n_1_Processing_state);
         break;

      case START_UP:
           Startup_state ();

      case RDASIM_START_OF_VOLUME_SCAN:
            /* perform the start of volume scan duties then
               fall through to the next state */
         Beginning_of_vol_scan ();

           /* fall through to next step */

      case START_OF_ELEVATION:
      case START_OF_LAST_ELEVATION:
           
           /* fall through to next step */

      case OPERATE:
         if (RDA_status_msg.channel_status == CCS_NON_CONTROLLING)
            break; /* can't process radials if we're not controlling */

         Operate_state (n_1_Processing_state);
         break;

      case VCP_ELEVATION_RESTART:
         Vcp_elevation_restart (current_time);
         break;

      case END_OF_ELEVATION:
         End_of_elevation (n_1_Processing_state);
         break;

      case END_OF_VOLUME_SCAN:
         End_of_vol_scan (n_1_Processing_state);
         break;
   }

       /* send a status msg if the status msg flag is set */

   if (Send_status_msg == TRUE) {
       RRM_process_rda_message (RDA_STATUS_DATA);
       Send_status_msg = FALSE;
   }

   if( Toggle_offline_maintenance_mode ){

      if (Offline_maintenance_mode == TRUE){
         fprintf (stdout, "Process Entering Offline Maintenance Mode\n" );

         msleep(1000);

         LINk.conn_activity = NO_ACTIVITY;
         LINk.link_state = LINK_CONNECTED;

         PR_process_exception();

         /* This prevents the wideband link from reconnecting. */
         LINk.conn_activity = CONNECTING;

      }
      else{
         fprintf (stdout, "Process Leaving Offline Maintenance Mode\n" );
         LINk.conn_activity = NO_ACTIVITY;
      }

      Toggle_offline_maintenance_mode = FALSE;

   }

   if (VErbose_mode >= 2) {
      if (Processing_state != n_1_Processing_state)
          fprintf (stdout, "Processing state change: %d\n", 
                   Processing_state);
   }

   n_1_Processing_state = Processing_state;
   return;
}


/********************************************************************************
  
    Description: Process beginning of volume scan

          Input: 

         Output:

         Return:

 ********************************************************************************/

static void Beginning_of_vol_scan ()
{

      /* send the RDA Adaptation Data if required */

   if (Send_rda_adapt_data == TRUE) {
          RRM_process_rda_message (RDA_ADAPTATION_DATA);
          Send_rda_adapt_data = FALSE;
   }
   
      /* send notchwidth map if required */

   if (Send_notchwidth_map_data == TRUE) {
      RRM_process_rda_message (NOTCHWIDTH_MAP_DATA);
      Send_notchwidth_map_data = FALSE;
   }

      /* send clutter filter bypass map if required */

   if (Send_bypass_map_data == TRUE) {
      RRM_process_rda_message (CLUTTER_FILTER_BYPASS_MAP);
      Send_bypass_map_data = FALSE;
   }

      /* send the Meta Data type msgs if required */

      RRM_process_rda_message (PERFORMANCE_MAINTENANCE_DATA);
      Send_performance_data = FALSE;

       /* starting with Build 14, all moments are enabled. */ 
   DATa_trans_enbld = DTE_ALL_ENABLED;
   RDA_status_msg.data_trans_enbld = DATa_trans_enbld;

      /* change the super resolution state if commanded */

   if (Change_super_res == TRUE) {

      RDA_status_msg.super_res = Commanded_super_res;
      Change_super_res = FALSE;

   }

      /* change the Clutter Mitigation Decision state if commanded */

   if (Change_cmd == TRUE) {

      if( Commanded_cmd == RCOM_ENABLE_CMD )
         RDA_status_msg.cmd = 0x7;
   
      else if ( Commanded_cmd == RCOM_DISABLE_CMD )
         RDA_status_msg.cmd = 0;

      Change_cmd = FALSE;

   }

      /* change the Automated Volume Scan Evaluation and Termination. */

   if (Change_avset == TRUE) {

      if (VErbose_mode >= 3)
         fprintf (stdout, "         RDA Status: change AVSET State: %d\n",
                  Commanded_avset);

      RDA_status_msg.avset = Commanded_avset;
      Change_avset = FALSE;

   }

      /* change the Performance Check status. */
   
   if (Change_perf_check == TRUE) {

      if (VErbose_mode >= 3)
         fprintf (stdout, "         RDA Status: change Performance Check Status: %d\n",
                  Commanded_perf_check);

      RDA_status_msg.perf_check_status = Commanded_perf_check;
      Change_perf_check = FALSE;

   }

   /* Status Message and VCP message will be sent in the following call. */
   CR_initialize_this_vol_scan_data ();

   if (Send_vcp_data == TRUE) {

      RRM_process_rda_message (RDA_VCP_MSG);
      Send_vcp_data = FALSE;

   }

   return;
}


/********************************************************************************
 
     Description: Change the shelter power source

           Input: curent_time - the current time of day

          Output:

          Return:

 ********************************************************************************/

static void Change_pwr_source (struct timeval curent_time)
{
       /* start time to xfer pwr from one source to another */
   static struct timeval power_xfer_start_time = {0, 0}; 

       /* elapsed time in seconds power has been switching */
   static int power_transition_elapsed_time = 0; 


   switch (Commanded_power_state) {
      case RCOM_UTIL:   /* switch to utility power */

            /* if power source is in utility power, then disregard the 
               command. it's assumed that if power is not switched to 
               aux power, then the power source must be utility power */

         if (((RDA_status_msg.aux_pwr_state & APGS_AUXILLARY_POWER)
               == ZERO)   &&
             ((RDA_status_msg.aux_pwr_state & APGS_UTILITY_PWR_AVAILABLE)
               != ZERO)) {
            Commanded_power_state = NO_COMMAND_PENDING;
            break;
         }

             /* initiate power switchover sequence */

         if (power_xfer_start_time.tv_sec == ZERO)
            power_xfer_start_time.tv_sec = curent_time.tv_sec;

         power_transition_elapsed_time = curent_time.tv_sec - 
                                         power_xfer_start_time.tv_sec;

         if ((power_transition_elapsed_time == 5) &&
             (RDA_status_msg.aux_pwr_state & APGS_UTILITY_PWR_AVAILABLE) 
             == ZERO) {
            RDA_status_msg.aux_pwr_state |= APGS_UTILITY_PWR_AVAILABLE;
            Send_status_msg = TRUE;
         } else if (power_transition_elapsed_time > 8) {
            RDA_status_msg.aux_pwr_state = APGS_UTILITY_PWR_AVAILABLE;
            Send_status_msg = TRUE;
            Commanded_power_state = NO_COMMAND_PENDING;
            power_transition_elapsed_time = ZERO;
            power_xfer_start_time.tv_sec = ZERO;
         }
        
         break;

      case RCOM_AUXGEN:

            /* if power source is in generator power, then disregard the
               command */

         if ((RDA_status_msg.aux_pwr_state & APGS_AUXILLARY_POWER)
              != ZERO) {
            Commanded_power_state = NO_COMMAND_PENDING;
            break;
         }

             /* initiate power switchover sequence */

         if (power_xfer_start_time.tv_sec == ZERO)
            power_xfer_start_time.tv_sec = curent_time.tv_sec;

         power_transition_elapsed_time = curent_time.tv_sec - 
                                         power_xfer_start_time.tv_sec;

         if ((power_transition_elapsed_time == 10) &&
             (RDA_status_msg.aux_pwr_state & APGS_GENERATOR_ON) == ZERO) {
            Send_status_msg = TRUE;
            RDA_status_msg.aux_pwr_state |= APGS_GENERATOR_ON;
         } else if (power_transition_elapsed_time > 20) {
            RDA_status_msg.aux_pwr_state |= APGS_AUXILLARY_POWER;
            Send_status_msg = TRUE;
            Commanded_power_state = NO_COMMAND_PENDING;
            power_transition_elapsed_time = ZERO;
            power_xfer_start_time.tv_sec = ZERO;
         }
        
         break;
   }
   return;
}


/********************************************************************************
  
    Description: Process end of elevation

          Input: last_time_processing_state - last time's processing state

         Output: Processing_state - the current processing state

         Return:

 ********************************************************************************/

static void End_of_elevation (int last_time_processing_state)
{

      /* send performance/maintenance data if required */

   if (Send_performance_data == TRUE) {
      RRM_process_rda_message (PERFORMANCE_MAINTENANCE_DATA);
      Send_performance_data = FALSE;
   }

   if (VErbose_mode >= 2) {
      if (Processing_state != last_time_processing_state)
          fprintf (stdout, "Processing_state = %d\n", 
                   Processing_state);
   }
         
   Processing_state = START_OF_ELEVATION;
   return;
}


/********************************************************************************
  
    Description: Process end of volume scan

          Input: last_time_processing_state - last time's processing state

         Output:

         Return:

 ********************************************************************************/

static void End_of_vol_scan (int last_time_processing_state)
{

   if (VErbose_mode >= 2) {
      if (Processing_state != last_time_processing_state)
          fprintf (stdout, "Processing_state = %d\n", 
                   Processing_state);
   }
         
   Processing_state = RDASIM_START_OF_VOLUME_SCAN;
   return;
}


/********************************************************************************
  
    Description: Process RDA in operate state

          Input: last_time_processing_state - last time's processing state
                 UNExpected_elevation       - exception from the rdasim_tst tool
                 UNExpected_volume          -    "        "   "     ""        "

         Output:  
 
         Return:

 ********************************************************************************/

static void Operate_state (int last_time_processing_state)
{
   if (VErbose_mode >= 2) {
      if (Processing_state != last_time_processing_state)
          fprintf (stdout, "Processing_state = %d\n", 
                   Processing_state);
   }

       /* construct the radial and write it to the response lb */
   CR_process_radial (&Processing_state);

   if(UNExpected_elevation == TRUE) {
      Processing_state = START_OF_ELEVATION;
      UNExpected_elevation  = FALSE;
   }

   if(UNExpected_volume == TRUE) {
      Processing_state = RDASIM_START_OF_VOLUME_SCAN;
      UNExpected_volume = FALSE;
   }
   return;
}


/********************************************************************************
  
    Description: Process the RDA restarting

          Input: curent_time                - current time of day
                 last_time_processing_state - last time's processing state

         Output:

         Return:

 ********************************************************************************/

static void Rda_restart (struct timeval curent_time, int *last_time_processing_state)
{

       /* RDA operability status */
   static short ops_status;

       /* start time used to suspend RDA processing */
   static struct timeval suspend_start_time = {0, 0};  

       /* length of time to suspend RDA processing */
   static unsigned int suspend_time = 0;  


   if (RDA_status_msg.rda_status != RDS_RESTART) {
      RDA_status_msg.rda_status = RDS_RESTART;
      ops_status = RDA_status_msg.op_status;
      RDA_status_msg.op_status = ROS_RDA_COMMANDED_SHUTDOWN;
              
         /* update the suspend time & the suspend timer 
            start time */

      suspend_start_time.tv_sec = curent_time.tv_sec;
      suspend_time = 150;  /* suspend for 150 seconds */ 

         /* write a status msg */

      RRM_process_rda_message (RDA_STATUS_DATA);
      msleep (1500);   /* suspend for 1.5 seconds */
      PR_process_exception();
      LINk.link_state = LINK_DISCONNECTED;
      *last_time_processing_state = Processing_state;
   }

      /* if the suspend time has not expired, then return */

   if ((curent_time.tv_sec - suspend_start_time.tv_sec) < 
        suspend_time)
      return;
   else {
      suspend_time = ZERO;  /* reset the suspend time */
           
      if ((LINk.link_state == LINK_DISCONNECTED) &&
          (LINk.conn_activity == CONNECTING)) {
         PR_send_response(&(LINk.req[(int)LINk.conn_req_ind]),
                          CM_SUCCESS);
         LINk.link_state = LINK_CONNECTED;
         LINk.conn_activity = NO_ACTIVITY;
      }

      RDA_status_msg.op_status = ops_status;  /* set to last known op status */
      Processing_state = STANDBY;
   }
   return;
}


/********************************************************************************
  
    Description: Process command to change RDA states

          Input: command - the RDA commanded state to change to

         Output:  

         Return:

 ********************************************************************************/

static void Process_rda_state_change_cmd (unsigned short command)
{
   if (VErbose_mode >= 3)
      fprintf (stdout, "         change RDA state to ");

   switch (command) {
      case RCOM_STANDBY:    /*  Command RDA Standby         */
          Commanded_state = STANDBY;
          msleep (3000);  /* wait a few seconds before continuing */

          if (VErbose_mode >= 3)
             fprintf (stdout, "Standby\n");
          break;

      case RCOM_OFFOPER:    /*  Command RDA Offline Operate */
          Commanded_state = OFFLINE_OPERATE; 

          if (VErbose_mode >= 3)
             fprintf (stdout, "Offline Operate\n");
          break;

      case RCOM_OPERATE:    /*  Command RDA Operate         */
          if (RDA_status_msg.channel_status == CCS_NON_CONTROLLING) {
             RRM_set_rda_alarm (RDA_ALARM_UNABLE_TO_CMD_OPER_REDUN_CHAN_ONLINE);
             RDA_status_msg.rda_alarm = RAS_RDA_CONTROL;
             break;
          }
          else
             Commanded_state = OPERATE;

          if (VErbose_mode >= 3)
             fprintf (stdout, "Operate\n");
          break;

      case RCOM_RESTART:    /*  Command RDA Restart         */
          Commanded_state = RDA_RESTART;

          if (VErbose_mode >= 3)
             fprintf (stdout, "RDA Restart\n");
          break;

      case RCOM_PLAYBACK:   /*  Command Archive II Playback */
/*          Commanded_state = PLAYBACK;  this command is not currently implemented */

          if (VErbose_mode >= 3)
             fprintf (stdout, "Playback\n");
          break;

      default:
          RRM_set_rda_alarm (RDA_ALARM_INVALID_RPG_COMMAND_RECEIVED);
          fprintf (stderr, 
                   "     invalid RDA_STATE command received (cmd = %d)\n",
                   command);
          break;
   }
   return;
}


/********************************************************************************
  
    Description: Process the RDA while in standby state

          Input: curent_time                - current time of day
                 last_time_processing_state - last time's processing state

         Output:

         Return:

 ********************************************************************************/

static void Standby_state (struct timeval curent_time, 
                           int last_time_processing_state)
{

       /* Bypass map msg time delay between the request and the time sent */
   int bypass_map_msg_delta_time = 3;

       /* start time for Bypass map request when in Standby */
   static struct timeval bypass_map_msg_start_time = {0, 0};

       /* Perf/Maint msg time delay between the request and the time sent */
   int perf_maint_msg_delta_time = 3;

       /* start time for Perf/Maint msg request when in Standby */
   static struct timeval perf_maint_msg_start_time = {0, 0};

       /* delay time between loopback msg writes in standby mode */
   int loopback_msg_delta_time = 300;

       /* start time used to periodically send a loopback msg */
   static struct timeval loopback_msg_start_time = {0, 0}; 


      /* set the rda status and standby timers for stanby mode */

   if (RDA_status_msg.rda_status != RDS_STANDBY) {
      RDA_status_msg.rda_status = RDS_STANDBY;
      RDA_status_msg.data_trans_enbld = DTE_NONE_ENABLED;
      RDA_status_msg.control_status = RCS_RDA_EITHER;
   }

      /* send a loopback msg every xx seconds */

   if (LINk.link_state == LINK_CONNECTED) {
      if ((curent_time.tv_sec - loopback_msg_start_time.tv_sec) >
           loopback_msg_delta_time) {
         if (RDA_status_msg.control_status != RCS_RDA_LOCAL_ONLY) {
            RRM_process_rda_message (LOOPBACK_TEST_RDA_RPG);
            loopback_msg_start_time = curent_time;
         }
      }

         /* do not process any messages until the RDA loopback msg has
            been returned */

      if (RDA_loopback_ack == FALSE) {
          msleep (1000);
          return;
      }

         /* send a RDA status message */

      if (Send_status_msg == TRUE) {
         RRM_process_rda_message (RDA_STATUS_DATA);
         Send_status_msg = FALSE;
      }

         /* send performance/maintenance data if required */

      if (Send_performance_data == TRUE) {
         if (perf_maint_msg_start_time.tv_sec == ZERO)
            perf_maint_msg_start_time.tv_sec = curent_time.tv_sec;

         if ((curent_time.tv_sec - perf_maint_msg_start_time.tv_sec) >
             perf_maint_msg_delta_time) {
            RRM_process_rda_message (PERFORMANCE_MAINTENANCE_DATA);
            perf_maint_msg_start_time.tv_sec = ZERO;
            Send_performance_data = FALSE;
         }
      }

          /* send the Bypass map if required */

      if (Send_bypass_map_data == TRUE) {
         if (bypass_map_msg_start_time.tv_sec == ZERO)
            bypass_map_msg_start_time.tv_sec = curent_time.tv_sec;

         if ((curent_time.tv_sec - bypass_map_msg_start_time.tv_sec) >
             bypass_map_msg_delta_time) {
            RRM_process_rda_message (CLUTTER_FILTER_BYPASS_MAP);
            bypass_map_msg_start_time.tv_sec = ZERO;
            Send_bypass_map_data = FALSE;
         }
      }
   
         /* send notchwidth map if required */

      if (Send_notchwidth_map_data == TRUE) {
         RRM_process_rda_message (NOTCHWIDTH_MAP_DATA);
         Send_notchwidth_map_data = FALSE;
      }

         /* send the RDA Adaptation Data if required */

      if (Send_rda_adapt_data == TRUE) {
             RRM_process_rda_message (RDA_ADAPTATION_DATA);
             Send_rda_adapt_data = FALSE;
      }

      msleep (1000); 

      if (last_time_processing_state == STANDBY) {
         if (Toggle_channel_number == TRUE) {
             RDA_channel_number = 3 - RDA_channel_number;
             Toggle_channel_number = FALSE;
             Send_status_msg = TRUE;
         }

         if ( Change_chanl_status == TRUE) {
             RDA_status_msg.channel_status = (0x0001 & 
                                          (~RDA_status_msg.channel_status));
             Send_status_msg = TRUE;
             Change_chanl_status = FALSE;
         }
      }
   } else {
      loopback_msg_start_time.tv_sec = ZERO;
      perf_maint_msg_start_time.tv_sec = ZERO;
      bypass_map_msg_start_time.tv_sec = ZERO;
   }
   return;
}


/********************************************************************************
  
    Description: Process startup state

          Input:

         Output:

         Return:

 ********************************************************************************/

static void Startup_state ()
{
      /* write a loopback test msg */

   if (RDA_status_msg.control_status != RCS_RDA_LOCAL_ONLY)
      RRM_process_rda_message (LOOPBACK_TEST_RDA_RPG);

   RDA_status_msg.rda_status = RDS_STARTUP;
   RDA_status_msg.op_mode = ROM_OPERATIONAL;
   RDA_status_msg.control_status = RCS_RDA_REMOTE_ONLY;
         
   if (RDA_status_msg.rda_alarm == RAS_NO_ALARMS)
           RDA_status_msg.op_status = ROS_RDA_ONLINE;         

       /* write a status msg */
         
   RRM_process_rda_message (RDA_STATUS_DATA);

   msleep (6000);  /* suspend for 6 seconds - seems like a good 
                      thing to do */

     /* go to operate, enable the moments and send another status message */
         
   RDA_status_msg.rda_status = RDS_OPERATE;
   RDA_status_msg.data_trans_enbld = DTE_ALL_ENABLED; 

      /* write a status msg & change processing states */

   RRM_process_rda_message (RDA_STATUS_DATA);
   Processing_state = RDASIM_START_OF_VOLUME_SCAN;

      /* starting with Build 14, all moments are enabled. */

   DATa_trans_enbld = DTE_ALL_ENABLED;
   RDA_status_msg.data_trans_enbld = DATa_trans_enbld;

   return;
}


/********************************************************************************
  
    Description: Process vcp/elevation restart

          Input: curent_time - current time of day

         Output:

         Return:

 ********************************************************************************/

static void Vcp_elevation_restart (struct timeval curent_time)
{
       /* start time used to suspend RDA processing */
   static struct timeval suspend_start_time = {0, 0};  

       /* length of time to suspend RDA processing */
   static unsigned int suspend_time = 0;  


      /* set the suspend time to 6 seconds for VCP restart and 2 
         seconds for an elevation cut restart */

   if (suspend_time == ZERO) {
      if (REstart_elevation_cut > ZERO)  /* restart elevation cut */
         suspend_time = 2;
      else
         suspend_time = 6;  

      suspend_start_time.tv_sec = curent_time.tv_sec;
   }
         
   if ((curent_time.tv_sec - suspend_start_time.tv_sec) > 
        suspend_time) {
      suspend_time = ZERO;

      if (REstart_elevation_cut > ONE)  /* restart elevation cut */
         Processing_state = START_OF_ELEVATION;
      else {    /* restart VCP Scan */
         Processing_state = RDASIM_START_OF_VOLUME_SCAN;
         Beginning_of_vol_scan ();
      }
   }
   return;
}
