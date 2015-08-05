/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/24 16:17:54 $
 * $Id: crda_main.c,v 1.117 2013/07/24 16:17:54 steves Exp $
 * $Revision: 1.117 $
 * $State: Exp $
 */

/*******************************************************************

	Main module for RDA Control program

*******************************************************************/

#define GLOBAL_DEFINED_CONTROL_RDA
#include <crda_control_rda.h>
#include <sys/time.h>
#include <stdlib.h>
#include <lb.h>
#include <infr.h>
#include <orpg.h>
#include <prod_gen_msg.h>
#include <mrpg.h>
#include <orpgadpt.h>
#include <orpgred.h>
#include <orpgsite.h>

/* File Scope Global Variables. */
/* Flag, if set, indicates there are control commands waiting to
   be processed for LB data ID ORPGDAT_RDA_COMMAND. */
static int Outstanding_control_command = 0;

/* Previous value of periodic loopback rate. */
static int Previous_periodic_loopback_rate = 0;

/* Flag, if set, indicates the wideband interface parameters have
   been updated. */
static int Wideband_interface_parm_update = 0;

/* Log file number of LE messages. */
static int Log_file_nmsgs = 1000;

/* Local functions. */
static void Init_control_rda();
static int Check_rda_commands();
static int Process_rda_commands( Rda_cmd_t *rda_commandi, int type );
static int Read_options ( int argc, char **argv );
static int Init_cd07_clutter_map_data();
static void Process_communications_discontinuity();
static void Process_return_to_previous_state();
static void Process_loopback_failure();
static void Init_interface_parameters();
void WB_interface_callback( int fd, LB_id_t msgid, int msg_info, void *arg );
void Update_WB_interface_parms();
void Redundant_commands( Rda_cmd_t *rda_command );
int Cleanup_fxn( int signal, int sig_type );
static void Shutdown_processing();
static void Open_lb();


/*\///////////////////////////////////////////////////////////////////
//
//   Description:
//      The main function for the RDA Control Function.  This
//      module performs all initialization to start the control
//      RDA process.  Once initialization is complete, all 
//      control RDA commands buffers received by this 
//      function are serviced. Special processing for timer 
//      expiration are also serviced, and the Comms Manager response
//      linear buffer is read to service any Comm Manager data
//      or RDA data. 
//
//   Inputs:
//      argc - the number of command line arguments.     
//      argv - the command line arguments.
//
//   Outputs:
//  
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_request_LB - see crda_control_rda.h
//      CR_control_rda_restarting - see crda_control_rda.h
//      CR_connection_retries_exhausted - see crda_control_rda.h
//      CR_disconnection_retries_exhausted - see crda_control_rda.h
//      CR_reconnect_wideband_line_after_loopback_failure - 
//          see crda_control_rda.h
//      CR_perform_loopback_test - see crda_control_rda.h
//      CR_loopback_failure - see crda_control_rda.h
//      CR_loopback_timeout - see crda_control_rda.h
//      CR_return_to_previous_state - see crda_control_rda.h
//      CR_verbose_mode - see crda_control_rda.h
//      CR_shutdown_state - see crda_control_rda.h
//
//   Notes:   
//      All global variables are defined and described in
//      crda_control_rda.h.  These will begin with CR_.  All file
//      scope global variables are defined and described at 
//      the top of the file.
//
////////////////////////////////////////////////////////////////////\*/
int main (int argc, char *argv[]){

   int ret, empty;

   /* Read command line options. */
   Read_options (argc, argv);

   /* Set up Log-Error services. */
   if( ORPGMISC_init( argc, argv, Log_file_nmsgs, 0, -1, 0 ) < 0 ){

      LE_send_msg( GL_INFO, "ORPGMISC_init Failed\n" );
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* Register termination handler. */
   ret = ORPGTASK_reg_term_handler (Cleanup_fxn);
   if (ret < 0) {

      LE_send_msg(GL_ERROR, "ORPGTASK_reg_term_handler failed: %d", ret) ;
      ORPGTASK_exit(GL_EXIT_FAILURE) ;

   }

   /* Open all LB's with the proper access. */
   Open_lb();

   /* Register for events. */
   ES_register_for_events();

   /* Register for multiple alarm services. */ 
   TS_register_timers();

   /* Tell the ORPG manager that control RDA is ready. */
   ORPGMGR_report_ready_for_operation();

   /* Wait for the RPG to enter operational state.  Wait no more
      than 120 seconds before continuing on. */
   if( ORPGMGR_wait_for_op_state( (time_t) 120 ) < 0 )
      LE_send_msg( GL_INFO, "Waiting For Operational State Timed Out\n" );
   else
      LE_send_msg( GL_INFO, "The RPG is in the OPERATE state\n" );

   /* Perform all necessary initialization for the Control RDA task. */
   Init_control_rda();


   /* If wideband line status does not indicate connected, try to 
      connect it. */
   if( ((ret = LC_check_wb_line_status()) != LC_SUCCESS) 
                          ||
       (SCS_get_wb_status( ORPGRDA_WBLNSTAT ) != RS_CONNECTED) )
      LC_connect_wb_line();

   /* Clear flag indicating that control_rda is finished restarting. */
   CR_control_rda_restarting = 0;

   /* Do Forever ..... */
   while(1){

      /* Check for alarms ..... */
      ALARM_check_for_alarm();

      /* Check if any timers expired. */
      if( CR_timer_expired ){

         CR_timer_expired = 0;
         TS_service_timer_expiration();

      }

      /* Check if the connection retries have been exhausted.  In this
         case, we need to try and reconnect the wideband line. */
      if( CR_connection_retries_exhausted ){

         /* Clear the connection retries exhausted flag and attempt a 
            wideband line reconnection. */
         CR_connection_retries_exhausted = 0;
         LC_process_connection_retry_exhausted();

      }

      /* Check if the disconnection retries have been exhausted.  In this
         case, we need to try and disconnect the wideband line. */
      else if( CR_disconnection_retries_exhausted ){

         /* Clear the disconnection retries exhausted flag and attempt a 
            wideband line disconnection. */
         CR_disconnection_retries_exhausted = 0;
         LC_process_disconnection_retry_exhausted();

      }

      /* Check all other flags and process accordingly. */
      else{

         /* Was the wideband line disconnected after a loopback
            test timeout or loopback test failure.  If so, 
            the line needs to be reconnected. */
         if( CR_reconnect_wideband_line_after_loopback_failure ){

            int ret = SCS_get_wb_status( ORPGRDA_WBLNSTAT );

            if( (ret == RS_DISCONNECTED_HCI) || (ret == RS_WBFAILURE) ||
                (ret == RS_DISCONNECTED_CM) || (ret == RS_DISCONNECTED_RMS) ){
   
               LC_connect_wb_line(); 
               CR_reconnect_wideband_line_after_loopback_failure = 0;

            }
            else if( ret == RS_CONNECTED )
               CR_reconnect_wideband_line_after_loopback_failure = 0;

         }

         /* Check if a loopback test needs to be performed. */
         if( CR_perform_loopback_test ){

            /* Clear flag for perform test and issue loopback test command. */
            CR_perform_loopback_test = 0;
            if( SWM_send_rda_command( (int) 0, (int) COM4_LBTEST,
                                      (int) RDA_PRIMARY ) < 0 ){

               CR_loopback_failure = 1;

            }

         }

         /* Check if loopback test failed due to timeout or failure. */
         if( CR_loopback_timeout || CR_loopback_failure )
            Process_loopback_failure();

         /* Check if we need to return RDA to previous state. */
         if( CR_return_to_previous_state != PREVIOUS_STATE_NO ){

            if( CR_return_to_previous_state == PREVIOUS_STATE_PERFORM_LOOPBACK )
               LE_send_msg( GL_INFO, "Initiating Return To Previous State\n" );

            Process_return_to_previous_state( );

         }

         /*
           If the RDA has not sent any data in a while and the status is operate,
           moments are enabled, and the operational mode is operational, we have
           a wideband communications discontinuity.
         */
         if( CR_communications_discontinuity ){

            /* Clear the communications discontinuity flag. */
            CR_communications_discontinuity = 0;
        
            /* Handle communications discontinuity condition. */
            Process_communications_discontinuity();

         }

         /* Check if wideband interface parameters have been updated. */
         if( Wideband_interface_parm_update ){

            LE_send_msg( GL_INFO, "Wideband Interface Parameters Updated\n" );

            /* Clear flag indicating update. */
            Wideband_interface_parm_update = 0;

            /* Get the new updates. */
            Update_WB_interface_parms();

         }

         /* Check if there are any rda commands pending. If so, process 
            them.  If control_rda is in the process of returning to 
            previous RDA state, then wait until return to previous state
            is complete before processing control commands. */
         if( Outstanding_control_command && 
             (CR_return_to_previous_state == PREVIOUS_STATE_NO) ){

            if( CR_verbose_mode )
               LE_send_msg( GL_INFO, "Servicing RDA Control Command Event.\n" );

            /* Reset Outstanding_control_command flag. */
            Outstanding_control_command = 0;

            /* Process command(s). */
            Check_rda_commands();

         }

         /* Check if control rda is shutting down. */
         if( CR_shut_down_state != SHUT_DOWN_NO )
            Shutdown_processing();

         /* Check if there are any Outstanding requests.  If so, process them
            now. */
         while( (empty = QS_request_queue_empty()) != QS_QUEUE_EMPTY ){

            Request_list_t *list_ptr = NULL;

            /* Remove item from Outstanding request queue. */
            QS_remove_request_queue( &list_ptr );

            /* Write this data to request linear buffer. */
            ret = ORPGDA_write( CR_request_LB, 
                                (char *) list_ptr->rpg_to_rda_msg,
                                list_ptr->message_size, LB_ANY );
            
            /* Free memory associated with this message. */
            if( list_ptr->rpg_to_rda_msg != NULL ){

               free( list_ptr->rpg_to_rda_msg );
               list_ptr->rpg_to_rda_msg = NULL;

            }

            /* If the write failed, report error and break out of loop. */
            if( ret < 0 ){

               LE_send_msg( GL_INFO, "ORPGDA_write To Request LB Failed.  Ret = %d\n",
                            ret );
               break;

            }
            else{

               /* Track the time this messsage was sent. */
               list_ptr->message_time = (time_t) time(NULL);

            }

            /* Check for a response to the request. */
            SWM_check_response_LB( list_ptr->wait_for_resp, list_ptr->wait_time );

         }

      /* End of "while" */
      }

      /* Sleep for 1/2 second, then try again. */
      msleep( (unsigned int) 500 );

      /* Process any messages in response linear buffer.  Data or otherwise. */
      SWM_check_response_LB( (int) DONT_WAIT_FOR_RESP, (int) 0  /* no wait time */ );

   /* End of "while" */
   }        
   
/* End of main() */
}

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      This module handles all initialization for the control rda 
//      function.
//
//   Inputs:
//      None.
//
//   Outputs:
//      None.
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_request_LB - see crda_control_rda.h
//      CR_response_LB - see crda_control_rda.h
//      CR_link_index -  see crda_control_rda.h
//      CR_shut_down_state -  see crda_control_rda.h
//      CR_control_rda_restarting -  see crda_control_rda.h
//      CR_return_to_previous_state -  see crda_control_rda.h
//
//   Notes:
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////////////\*/  
static void Init_control_rda(){

   int ret;
   Mrpg_state_t rpg_state;
   Redundant_info_t redundant_info;

   /* Initialize shutdown flag to NOT shutting down. */
   CR_shut_down_state = SHUT_DOWN_NO;
   
   /* Initialize the HCI requested flags to false. */
   CR_hci_requested_status = 0;
   CR_hci_requested_pmd = 0;
   CR_hci_requested_vcp = 0;

   /* Set flag indicating that control rda is restarting.  Set 
      the initial state for return to NO previous state. */
   CR_control_rda_restarting = 1;
   CR_return_to_previous_state = PREVIOUS_STATE_NO;

   /* For tracking the last VCP commanded. */
   CR_last_commanded_vcp = 0;
   memset( &CR_last_downloaded_vcp_data, 0, sizeof(Vcp_struct) );

   /* Check if the RPG is restarting. This flag is only used for 
      FAA redundant systems. */
   if( (ret = ORPGMGR_get_RPG_states( &rpg_state )) < 0 ){

      /* Assume that the rpg is starting up. */
      LE_send_msg( GL_INFO, "RPG Command In Progress Failed\n" );
      LE_send_msg( GL_INFO, "The RPG is Starting Up\n" );
      CR_rpg_restarting = MRPG_STARTUP;

   }
   else{

      CR_rpg_restarting = rpg_state.cmd;
      LE_send_msg( GL_INFO, "Setting RPG Start Up State to %d\n", rpg_state.cmd );

   }

   CR_redundant_type = NO_REDUNDANCY;
   CR_channel_num = 1;
   if( ORPGSITE_get_redundant_data( &redundant_info ) >= 0 ){

      CR_redundant_type = redundant_info.redundant_type;
      CR_channel_num = redundant_info.channel_number;

   }

   LE_send_msg( GL_INFO, "Channel Number At Startup: %d\n", CR_channel_num );

   /* Initialize RPG_RDA loopback test data, and RDA status data. */
   PR_init_RPGtoRDA_loopback_message();
   ST_init_rda_status();

   /* Register Adaptation Data Block RDACNT. */
   CR_velocity_resolution = CRDA_VEL_RESO_HIGH;
   DV_init_adaptation_data();

   /* Initialize bypass map and notchwidth map generation dates and times. */
   Init_cd07_clutter_map_data();

   /* Initialize the Wideband line handling module. */
   LC_init();

   /* Initialize the outstanding request queue. */
   QS_create_request_queue();

   /* Initialize the outstanding request list. */
   SWM_init_outstanding_requests();

   /* Initially read the Connection Retry limit and loopback periodic rate. 
      Also, register for updates to these values. */
   Init_interface_parameters();

/* End of Init_control_rda() */
}

#define RDA_CTRL_CMD          1

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Reads commands from the command Linear Buffer.  These commands 
//      are processed by the command processor.  Typically, each linear 
//      buffer contains a single command.  However, a weather mode 
//      change command must be handled differently.
//
//      The Linear Buffer data ID is ORPGDAT_RDA_COMMAND.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:    
//      Returns error is Linear Buffer Read failed; otherwise 0.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
static int Check_rda_commands(){

   int ret;
   Rda_cmd_t rda_command;

   /* Do Forever .... */
   while(1){

      /* Read the RDA Commands Linear Buffer.  If command LB is
         exhausted of any new messages, then return. */
      if( (ret = ORPGDA_read( ORPGDAT_RDA_COMMAND,
                              (char *) &rda_command,
                              (int) sizeof( Rda_cmd_t ),
                              LB_NEXT )) == LB_TO_COME ) break;
    
      /* On read error, report it and return.  Otherwise, process the
         control command. */
      if( ret < 0 ){

         LE_send_msg( GL_ORPGDA(ret),
                      "RDA Command Linear Buffer Read Failed (%d).\n", ret );

         /* If the error message was LB_EXPIRED, go to the first 
            unread message in LB, then try and re-read the LB. */
         if( ret == LB_EXPIRED ){

            ORPGDA_seek( ORPGDAT_RDA_COMMAND, 0, LB_FIRST, NULL );
            continue;

         }
         else
            return( ret );

      }
      else
         Process_rda_commands( &rda_command, (int) RDA_CTRL_CMD );


   /* End of "while" */
   }

   return (0);

/* End of Check_rda_commands() */
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      RDA control commands are processed. 
//
//   Inputs:
//      rda_command - pointer to RDA command buffer.
//      type - either RDA_CTRL_CMD or CP4MSG_CTRL_CMD.
//
//   Outputs:
//
//   Returns:    
//      Allows returns 0.
//
//   Globals: 
//      CR_hci_requested_status - see crda_control_rda.h
//      CR_hci_requested_pmd - see crda_control_rda.h
//      CR_hci_requested_vcp - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
static int Process_rda_commands( Rda_cmd_t *rda_command, int type ){

   int ret;

   /* Set return value to failure. */
   ret = -1;

   switch( rda_command->cmd ){

      case COM4_WMVCPCHG:

         /* Weather Mode change is unique, since a weather mode change 
            involves downloading the default VCP for that mode, selecting 
            that VCP for next start of volume, then possible restarting 
            the volume scan. */
         LE_send_msg( GL_INFO, "Process Weather Mode Change.\n" );
         ret = SWM_process_weather_mode_change( rda_command );
 
         break;

      case COM4_DLOADVCP:

         /* The download command needs to be processed by pbd first. */
         if( rda_command->line_num == PBD_INITIATED_RDA_CTRL_CMD ){

            /* The VCP Download command is unique since the VCP must be
               downloaded, then a control command issued to use that VCP
               at the start of the next volume scan. */
            LE_send_msg( GL_INFO, "Process VCP Download Command.\n" );
            ret = SWM_process_vcp_download( rda_command );

         }

         break;
        
      case COM4_RDACOM:

         /* The RDA control command for elevation/volume restart may be unique 
            depending on which process initiated the command.  If the command was 
            HCI initiated, simply ignore it. */
         if( ((rda_command->param1 == CRDA_RESTART_ELEV)
                              ||
              (rda_command->param1 == CRDA_RESTART_VCP))
                              &&
              (rda_command->line_num != PBD_INITIATED_RDA_CTRL_CMD) )
            return(0);

         /* If the RDA control command was CRDA_SELECT_VCP, handle this special. */
         if( rda_command->param1 == CRDA_SELECT_VCP ){

            LE_send_msg( GL_INFO, "Process VCP Change Command.\n" );
            ret = SWM_process_vcp_change( rda_command );

         }
         else
            ret = SWM_process_rda_command( rda_command );

         break;

      case COM4_VEL_RESO:

         /* Set the velocity resolution for all subsequent VCP downloads. */
         LE_send_msg( GL_INFO, "Process VCP Velocity Resolution Command.\n" );

         CR_velocity_resolution = rda_command->param1;
         if( (rda_command->param1 != CRDA_VEL_RESO_LOW)
                                  &&
             (rda_command->param1 != CRDA_VEL_RESO_HIGH) ){

            LE_send_msg( GL_INFO, "Invalid Velocity Resolution in Command: %d\n",
                         rda_command->param1 );
            CR_velocity_resolution = CRDA_VEL_RESO_HIGH;

         }

         LE_send_msg( GL_INFO, "--->VCP Velocity Resolution: %d\n",
                      CR_velocity_resolution );

         /* Do we need to download a new VCP definitionr?   We determine this
            by comparing the CR_last_vcp_data VCP number number against the 
            CR_last_vcp.  If different, do nothing.  If the same, download 
            a new VCP. */
         if( CR_last_downloaded_vcp_data.vcp_num == CR_last_commanded_vcp ){

            Rda_cmd_t rda_command;

            /* Fill in fields as necessary to download VCP data 
               (see rda_commands.5 man page). */
            rda_command.cmd = COM4_DLOADVCP;
            rda_command.line_num = APP_INITIATED_RDA_CTRL_CMD;
            rda_command.param1 = CR_last_commanded_vcp;
            rda_command.param2 = 1;
            rda_command.param3 = VCP_DO_NOT_TRANSLATE;
            rda_command.param4 = 0;
            rda_command.param5 = 0;
            memcpy( &rda_command.msg[0], &CR_last_downloaded_vcp_data, 
                    sizeof(Vcp_struct) );

            /* Download the VCP data. */
            SWM_process_rda_command( &rda_command );

            /* Fill in fields as necessary to command next VCP. */
            rda_command.cmd = COM4_RDACOM;
            rda_command.line_num = APP_INITIATED_RDA_CTRL_CMD;
            rda_command.param1 = CRDA_SELECT_VCP;
            rda_command.param2 = RCOM_USE_REMOTE_PATTERN;
            rda_command.param3 = 0;
            rda_command.param4 = 0;
            rda_command.param5 = 0;

            /* Issue the RDA Control Command. */
            SWM_process_rda_command( &rda_command );

         }

         break;
        
      /* Note we purposely fall through this case so as to process the
         control command in the "default" case. */
      case COM4_REQRDADATA:
      {
         /* NOTE: TBD - The VCP request is an anticipated future enhancement. 
            For now we're just adding hooks in the code so it's easy to implement
            in the future.  Until that time DREQ_VCP is defined here.  Later
            it will have to be added to the necessary header files like the
            other DREQ macros. */

         int DREQ_VCP = -99999;

         /* If this is a request for certain data, set flag indicating the 
            data was requested at the HCI. */
         if ( (rda_command->param1 == DREQ_STATUS) &&
              (type == RDA_CTRL_CMD) )
         {
            CR_hci_requested_status = 1;
         }
         else if ( (rda_command->param1 == DREQ_PERFMAINT) &&
                   (type == RDA_CTRL_CMD) )
         {
            CR_hci_requested_pmd = 1;
         }
         else if ( (rda_command->param1 == DREQ_VCP) &&
                   (type == RDA_CTRL_CMD) )
         {
            CR_hci_requested_vcp = 1;
         }
      }
      default:
         ret = SWM_process_rda_command( rda_command );
   
         break;

   /* End of "switch" statement. */
   }

   /* Does redundant need to know about this command? */
   if( (ret >= 0) && (CR_redundant_type != ORPGSITE_NO_REDUNDANCY) )
      Redundant_commands( rda_command );

   return (0);

/* End of Process_rda_commands() */
}

/*\////////////////////////////////////////////////////////////////////
//
//   Description:  
//      Initializes the bypass map generation date/times in 
//      ITC DATAID_CD07_BYPASSMAP.  Initializes ITC DATAID_A304C2 
//      elements.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:    
//      Currently undefined.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////////\*/
static int Init_cd07_clutter_map_data(){

   int ret;

   cd07_bypassmap bm_map_date_time_stamp;  
   A304c2 clutter_map_generation_flags;

   /* Read in A304c2 from linear buffer.  If no buffer exits 
      initialize values, and write initialized values back 
      to linear buffer. */
   if( (ret = ORPGDA_read( DATAID_A304C2, 
                           (char *) &clutter_map_generation_flags, 
                           sizeof( A304c2 ), 
                           LBID_A304C2 ) ) < 0 ){

      if( ret == LB_NOT_FOUND ){

         /* Initialize structure elements, then write data to linear 
            buffer. */
         clutter_map_generation_flags.nw_map_request_pending = 0;
         clutter_map_generation_flags.bypass_map_request_pending = 0;
         clutter_map_generation_flags.unsolicited_nw_received = 0;

         if( (ret = ORPGDA_write( DATAID_A304C2, 
                                  (char *) &clutter_map_generation_flags, 
                                  sizeof( A304c2 ), 
                                  LBID_A304C2 )) < 0 )
            LE_send_msg( GL_ORPGDA(ret), 
                         "Init of DATAID_A304C2 (%d) Failed (%d).\n",
                         DATAID_A304C2, ret );

      }

   }

   /* Read in CD07_BYPASSMAP DATA from linear buffer.  If no buffer 
      exists, initialize values, and write initialized values back 
      to linear buffer. */
   if( (ret = ORPGDA_read( DATAID_CD07_BYPASSMAP, 
                           (char *) &bm_map_date_time_stamp, 
                           sizeof( cd07_bypassmap ), 
                           LBID_CD07_BYPASSMAP ) ) < 0 ){

      if( ret == LB_NOT_FOUND ){

         cd07_bypassmap bm_map_date_time_stamp;  

         /* Initialize structure elements, then write data to linear buffer. */
         bm_map_date_time_stamp.bm_gendate = 0;
         bm_map_date_time_stamp.bm_gentime = 0;

         if( (ret = ORPGDA_write( DATAID_CD07_BYPASSMAP, 
                                  (char *) &bm_map_date_time_stamp, 
                                  sizeof( cd07_bypassmap ), 
                                  LBID_CD07_BYPASSMAP )) < 0 )
            LE_send_msg( GL_ORPGDA(ret), 
                         "Init of DATAID_CD07_BYPASSMAP (%d) Failed (%d).\n",
                         DATAID_CD07_BYPASSMAP, ret );

      }

   }

   return 0;

/* End of Init_cd07_clutter_map_data() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:   
//      Sets the appropriate flag (Outstanding_control_command)
//      based on the input argument.
//
//   Inputs:   
//      command_type - Event ID associated with a command request. 
//                     Only command type ORPGEVT_RDA_CONTROL_COMMAND
//                     is supported.
//
//   Outputs:
//
//   Returns:  
//      There are not return values defined for this module.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/
void CR_outstanding_control_command( int event_id ){

   if( event_id == ORPGEVT_RDA_CONTROL_COMMAND ){

      /* Set the outstanding RDA Control Command flag. */
      Outstanding_control_command = 1;

   }

/* End of CR_outstanding_control_command() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:
//      Services the communications discontinuity alarm condition.
//
//      Must first cancel the MALRM_RDA_COMM_DISC timer, then update 
//      the wideband line status, and post the RDA_COMM_DISC_ALARM.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/
static void Process_communications_discontinuity(){

   int wideband_line_status;
   int ret;
   
   /* Check the communications line status to see if line is still
      connected.  If not, post new line status. */
   if( (ret = SCS_get_wb_status( ORPGRDA_WBLNSTAT )) != RS_CONNECTED ){
   
      /* Set the new wideband line status. */
      if( (ret == RS_DISCONNECTED_CM) || (ret == RS_DISCONNECTED_HCI) ||
          (ret == RS_DISCONNECTED_SHUTDOWN) || (ret == RS_DISCONNECTED_RMS) )
         wideband_line_status = ret;
   
      else
         wideband_line_status = RS_WBFAILURE;
   
   }
   else
      wideband_line_status = RS_CONNECTED;
   
   /* 1) Apply RDA status display blanking to all fields except the
         OPERABILITY STATUS.  

      2) Set the RDA operability status to INOPERABLE.
      3) Clear the alarm summary.
      4) Set the RDA_COMM_DISC_ALARM.

      These are all set in SCS_handle_wideband_alarm. */
   SCS_handle_wideband_alarm( wideband_line_status,
                              (int) RDA_COMM_DISC_ALARM,
                              (int) RS_RDA_STATUS );

/* End of Process_communications_discontinuity() */
}

/*\////////////////////////////////////////////////////////////////////
//
// Description:
//    Performs the necessary processing in order to return 
//    the RDA to it previous processing state.  These steps
//    are performed after a wideband line connection.
//
// Inputs:
//
// Outputs:
//
// Returns:
//    This function has no define return values.
//
// Globals:
//    CR_return_to_previous_state - see crda_control_rda.h
//    CR_loopback_failure - see crda_control_rda.h
//    CR_perform_loopback_test - see crda_control_rda.h
//    CR_loopback_periodic_rate - see crda_control_rda.h
//    CR_rpg_restarting - see crda_control_rda.h
//
//  Notes:         
//    All global variables are defined and described in 
//    crda_control_rda.h.  These will begin with CR_.  All file 
//    scope global variables are defined and described at the 
//    top of the file.
//
////////////////////////////////////////////////////////////////////\*/
static void Process_return_to_previous_state(){

   static time_t start_time, current_time;

   /* Processing based on current state. */
   switch( CR_return_to_previous_state ){

      case PREVIOUS_STATE_PERFORM_LOOPBACK:
      {

         LE_send_msg( GL_INFO, "Scheduling RPG/RDA Loopback Test\n" );

         /* Verify the link. */
         CR_perform_loopback_test = 1;
         CR_return_to_previous_state = PREVIOUS_STATE_LOOPBACK_WAIT;

         /* Set the time that the loopback command  was issued.  There is 
            a chance that the loopback timer never gets set ... It is set
            only after the loopback message is sent and acknowledged by the 
            comm manager. */
         start_time = MISC_systime(NULL);

         break;
      }

      case PREVIOUS_STATE_LOOPBACK_WAIT:
      {

         int ret;

         /* The loopback test fails if the timer is not set within 
            MALRM_LB_TEST_TIMEOUT seconds. */
         if( !TS_is_timer_active( MALRM_LB_TEST_TIMEOUT ) ){

            current_time = MISC_systime(NULL);
            if( (current_time - start_time) > LB_TEST_TIMEOUT_VALUE ){
               
               CR_loopback_failure = 1;
               LE_send_msg( GL_INFO, "Loopback test timer not set within %d seconds\n",
                            LB_TEST_TIMEOUT_VALUE );
               CR_return_to_previous_state = PREVIOUS_STATE_NO;
               CR_rpg_restarting = -1;
               break;

            }

         }

         /* Request RDA status data. */
         SWM_send_rda_command( (int) 1, (int) COM4_REQRDADATA,
                               (int) RDA_PRIMARY, (int) DREQ_STATUS );

         /* Set the RDA status time-out.  When the timer expires,
            we return to the previous RDA operating state. */
         ret = TS_set_timer( (malrm_id_t) MALRM_RDA_STATUS, 
                             (time_t) RDA_STATUS_TIMEOUT, 
                             (unsigned int) MALRM_ONESHOT_NTVL );

         if( ret == TS_FAILURE ){
   
            LE_send_msg( GL_INFO, "Unable To Set RDA_STATUS_TIMEOUT timer \n" );
            CR_return_to_previous_state = PREVIOUS_STATE_PHASE_1;

         }
         else
            CR_return_to_previous_state = PREVIOUS_STATE_STATUS_WAIT;

         break;
      }

      case PREVIOUS_STATE_STATUS_WAIT:
      {

         /* Nothing to do .... just wait for the status timer
            to expire. */
         break;
      }

      case PREVIOUS_STATE_PHASE_1:
      {

         /* Do phase 1 of returning to previous state. */
         LE_send_msg( GL_INFO, "Performing Phase 1 Return To Previous State\n" );
         SWM_return_to_previous_state( PREVIOUS_STATE_PHASE_1 );

         break;
      }

      case PREVIOUS_STATE_PHASE_2:
      {
 
         int ret;

         /* Do phase 2 of returning to previous state. */
         LE_send_msg( GL_INFO, "Performing Phase 2 Return To Previous State\n" );
         SWM_return_to_previous_state( PREVIOUS_STATE_PHASE_2 );

         /* If the periodic loopback is enabled, set the periodic 
            loopback timer. */
         if( CR_loopback_periodic_rate > 0 ){

            ret = TS_set_timer( (malrm_id_t) MALRM_LOOPBACK_PERIODIC,
                                (time_t) CR_loopback_periodic_rate,
                                (int) MALRM_ONESHOT_NTVL );

            if( ret == TS_FAILURE )
               LE_send_msg( GL_INFO, "Unable To Activate MALRM_LOOPBACK_PERIODIC\n" );

            else{

               if( Previous_periodic_loopback_rate == 0 )
                 LE_send_msg( GL_INFO, "Periodic Loopback Test Initiated\n" );

            }

            /* All done with return to previous state processing. */
            CR_return_to_previous_state = PREVIOUS_STATE_NO;

         }
         break;

      }
   
   /* End of "switch" */
   }

/* End of Process_return_to_previous_state() */
}

/*\//////////////////////////////////////////////////////////////////////
//
// Description:
//    This modules handles the loopback test failure and timeout.
//    Loopback failures and timeout cause the wideband line to be
//    disconnected .... after which the wideband line is reconnected.
//
// Inputs:
//
// Outputs:
//
// Returns:
//    There is no return value for this function.
//
// Globals:
//    CR_loopback_timeout - see crda_control_rda.h
//    CR_loopback_failure - see crda_control_rda.h
//    CR_loopback_retries - see crda_control_rda.h
//    CR_reconnect_wideband_line_after_loopback_failure - see 
//        crda_control_rda.h
//
// Notes:         
//    All global variables are defined and described in 
//    crda_control_rda.h.  These will begin with CR_.  All file 
//    scope global variables are defined and described at the 
//    top of the file.
//
//////////////////////////////////////////////////////////////////////\*/
static void Process_loopback_failure(){

   if( CR_loopback_failure )
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "RPG/RDA Loopback Test Failed\n" );

   else if( CR_loopback_timeout )
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "RPG/RDA Loopback Test Timed-out\n" );

   /*
     Clear the loopback test timeout and failure flags, 
     and loopback retries flag.
   */
   CR_loopback_timeout = 0;
   CR_loopback_failure = 0;
   CR_loopback_retries = 0;

   /* If returning to previous state, clear previous state. */
   CR_return_to_previous_state = PREVIOUS_STATE_NO;
 
   /*
     Disconnect the wideband line.
   */
   SWM_send_rda_command( (int) 0, (int) COM4_WBDISABLE,
                         (int) RDA_PRIMARY );

   /* Reconnect the wideband line. */
   CR_reconnect_wideband_line_after_loopback_failure = 1;

/* End of Process_loopback_failure() */
}

/*\/////////////////////////////////////////////////////////////////////////
//
//  Description:
//     Initializes wideband interface parameters and registers for updates
//     to these interface parameters.
//
//  Inputs:
//
//  Outputs:
//
//  Returns:
//     There is no return value for this function.
//
//  Globals:
//     CR_connection_retry_limit - see crda_control_rda.h
//     CR_disconnection_retry_limit - see crda_control_rda.h
//     CR_connect_retry_value - see crda_control_rda.h
//     CR_disconnect_retry_value - see crda_control_rda.h
//     CR_loopback_periodic_rate - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////////////\*/
static void Init_interface_parameters(){

   int ret;
   double value1, value2;

   CR_connection_retry_limit = RDA_NUM_CONN_RETRIES;
   CR_disconnection_retry_limit = CR_connection_retry_limit;
   CR_connect_retry_value = RDA_CONNECT_RETRY_VALUE;
   CR_disconnect_retry_value = RDA_DISCONNECT_RETRY_VALUE;

   LE_send_msg( GL_INFO, "Wideband Connection/Disconnection Retry Limit (%d)\n", 
                CR_connection_retry_limit );
   LE_send_msg( GL_INFO, "Wideband Connection/Disconnection Timeout Value (%d)\n", 
                CR_connect_retry_value );

   /* Read initial value of loopback periodic rate. */
   if( (DEAU_get_values( "rda_control_adapt.loopback_disabled", &value1, 1 ) > 0)
                                    &&
       (DEAU_get_values( "rda_control_adapt.loopback_rate", &value2, 1 ) > 0) ){

      if( (int) value1 /* loopback disabled. */)
         CR_loopback_periodic_rate = 0;
      else
         CR_loopback_periodic_rate = (int) value2;

      /* Register for updates to interface parameters. */
      if( (ret = DEAU_UN_register( "rda_control_adapt", 
                                    WB_interface_callback )) == 0 )
         LE_send_msg( GL_ERROR, "Unable To Register for Wideband Interface Parameter Updates\n" );

      LE_send_msg( GL_INFO, "Periodic Loopback Rate (%d)\n", CR_loopback_periodic_rate );
      return;
            
   }
   else{

      /* If read falls, set these values to defaults. */
      LE_send_msg( GL_INFO, "Wideband Interface Parameters Read Failed\n" );
      LE_send_msg( GL_INFO, "Setting Interface Parameters To Default Values\n" ); 

      CR_connection_retry_limit = CONNECTION_RETRY_LIMIT;
      CR_disconnection_retry_limit = DISCONNECTION_RETRY_LIMIT;
      CR_connect_retry_value = RDA_CONNECT_RETRY_VALUE;
      CR_disconnect_retry_value = RDA_DISCONNECT_RETRY_VALUE;
      CR_loopback_periodic_rate = LOOPBACK_PERIODIC_RATE;

      LE_send_msg( GL_INFO, "Wideband Connection Retry Limit (%d)\n", 
                   CR_connection_retry_limit );
      LE_send_msg( GL_INFO, "Periodic Loopback Rate (%d)\n", 
                   CR_loopback_periodic_rate );

   }

/* End of Init_interface_parameters() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:
//      Callback function for updating wideband interface parameters.
//
//   Inputs:
//      see lb man page for a description of argument list.
//
//   Outputs:
//      Set new interface parameter values.
//
//   Notes:
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/ 
void WB_interface_callback( int fd, LB_id_t msgid, int msg_info, void *arg ){

   Wideband_interface_parm_update = 1;

/* End of WB_interface_callback() */
}

/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//     Function to update wideband interface parameters because of
//     operator modification.
//
//  Inputs:
//
//  Outputs:
//
//  Returns:
//
//  Notes:
//
//  Globals:
//
//     CR_connection_retry_limit - see crda_control_rda.h
//     CR_disconnection_retry_limit - see crda_control_rda.h
//     CR_loopback_periodic_rate - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////////\*/
void Update_WB_interface_parms(){

   double value1, value2;

   /* Read the data. */
   if( (DEAU_get_values( "rda_control_adapt.loopback_disabled", &value1, 1 ) > 0)
                                   &&
       (DEAU_get_values( "rda_control_adapt.loopback_rate", &value2, 1 ) > 0) ){

      Previous_periodic_loopback_rate = CR_loopback_periodic_rate;

      if ( (int) value1 /* loopback disabled */)
         CR_loopback_periodic_rate = 0;
      else
         CR_loopback_periodic_rate = (int) value2;

      /* If previous value of periodic loopback rate is not the same as
         new value, then set the periodic loopback rate timer. */
      if( (Previous_periodic_loopback_rate != CR_loopback_periodic_rate)
                       &&
          (CR_loopback_periodic_rate > 0) ){

         /* Activate the timer only if the wideband is connected. */
         if( SCS_get_wb_status( ORPGRDA_WBLNSTAT ) == RS_CONNECTED ){

            if( TS_set_timer( (malrm_id_t) MALRM_LOOPBACK_PERIODIC,
                              (time_t) CR_loopback_periodic_rate,
                              (int) MALRM_ONESHOT_NTVL ) == TS_FAILURE )
               LE_send_msg( GL_INFO, "Unable To Activate MALRM_LOOPBACK_PERIODIC\n" );

            else{

               if( Previous_periodic_loopback_rate == 0 )
                  LE_send_msg( GL_INFO, "Periodic Loopback Test Initiated\n" );

            }

         }

      }
      else if( (Previous_periodic_loopback_rate != 0)
                               &&
               (CR_loopback_periodic_rate == 0) ){

         /* Cancel loopback test and test timeout timer. */
         TS_cancel_timer( (int) 2, (malrm_id_t) MALRM_LB_TEST_TIMEOUT,
                          (malrm_id_t) MALRM_LOOPBACK_PERIODIC );
         LE_send_msg( GL_INFO, "Periodic Loopback Test Disabled\n" );

      }

      return;

   }

   LE_send_msg( GL_ERROR, "control_rda: Wideband Interface Update Failed\n" );

/* End of Update_WB_interface_parms() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:
//      Commands which redundant needs to know something about.
//
//   Inputs:
//      rda_command - pointer to RDA control command buffer.
//
//   Outputs:
//
//   Returns:
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////\*/
void Redundant_commands( Rda_cmd_t *rda_command ){

   Redundant_cmd_t red_cmd;
   int vcp_num, error = 0, ret = 0;

   /* Initialize all red_cmd parameters to 0. */
   red_cmd.parameter1 = 0;
   red_cmd.parameter2 = 0;
   red_cmd.parameter3 = 0;
   red_cmd.parameter4 = 0;
   red_cmd.parameter5 = 0;

   /* Is this a command redundant needs to know about? */
   switch( rda_command->cmd ){

      case COM4_RDACOM:

         /* Process select VCP for next restart. */
         if( rda_command->param1 == CRDA_SELECT_VCP ){

            LE_send_msg( GL_INFO, "Processing CRDA_SELECT_VCP command for REDUNDANT\n" );
            red_cmd.cmd = ORPGRED_VCP_RDA_CONTROL;
            red_cmd.parameter1 = COM4_RDACOM;
            red_cmd.parameter2 = CRDA_SELECT_VCP;
            red_cmd.parameter3 = rda_command->param2;

            if( (ret = ORPGDA_write( ORPGDAT_REDMGR_CMDS, (char *) &red_cmd, 
                                    sizeof( Redundant_cmd_t), LB_ANY )) < 0 )
               error = 1;

         }
         else if( (rda_command->param1 == CRDA_SR_ENAB) 
                                  ||
                  (rda_command->param1 == CRDA_SR_DISAB) ){

            LE_send_msg( GL_INFO, "Processing CRDA_SR_* command for REDUNDANT\n" );
            red_cmd.cmd = ORPGRED_VCP_RDA_CONTROL;
            red_cmd.parameter1 = COM4_RDACOM;
            red_cmd.parameter2 = rda_command->param1;
            red_cmd.parameter3 = 0;

            if( (ret = ORPGDA_write( ORPGDAT_REDMGR_CMDS, (char *) &red_cmd, 
                                    sizeof( Redundant_cmd_t), LB_ANY )) < 0 )
               error = 1;

         }
         else if( (rda_command->param1 == CRDA_CMD_ENAB)
                                  ||
                  (rda_command->param1 == CRDA_CMD_DISAB) ){

            LE_send_msg( GL_INFO, "Processing CRDA_CMD_* command for REDUNDANT\n" );
            red_cmd.cmd = ORPGRED_VCP_RDA_CONTROL;
            red_cmd.parameter1 = COM4_RDACOM;
            red_cmd.parameter2 = rda_command->param1;
            red_cmd.parameter3 = 0;

            if( (ret = ORPGDA_write( ORPGDAT_REDMGR_CMDS, (char *) &red_cmd, 
                                    sizeof( Redundant_cmd_t), LB_ANY )) < 0 )
               error = 1;

         }
 
         break; 

      case COM4_WMVCPCHG:

         /* Get the default VCP associated with this weather mode. */
         vcp_num = DV_get_default_vcp_for_wxmode( rda_command->param1 );

         if( vcp_num != DV_FAILURE ){

            LE_send_msg( GL_INFO, "Processing COM4_DLOADVCP command for REDUNDANT\n" );
            /* Tell Redundant to tell Control RDA to download the VCP. */
            red_cmd.cmd = ORPGRED_VCP_RDA_CONTROL;
            red_cmd.parameter1 = COM4_DLOADVCP;
            red_cmd.parameter2 = vcp_num;
  
            if( (ret = ORPGDA_write( ORPGDAT_REDMGR_CMDS, (char *) &red_cmd,
                                    sizeof( Redundant_cmd_t ), LB_ANY )) < 0 )
               error = 1;

         }
         else
            LE_send_msg( GL_ERROR, 
                         "Unable to Inform Redundant Manager of Wx Mode Change\n" );
         break;
 
      case COM4_DLOADVCP:

         vcp_num = rda_command->param1;

         /* Tell Redundant to tell Control RDA to download the VCP. */
         LE_send_msg( GL_INFO, "Processing COM4_DLOADVCP command for REDUNDANT\n" );
         red_cmd.cmd = ORPGRED_VCP_RDA_CONTROL;
         red_cmd.parameter1 = COM4_DLOADVCP;
         red_cmd.parameter2 = vcp_num;

         /* If local VCP ..... first check if vcp data came with command.   If not, 
            get the current vcp from Volume Status data. */
         if( vcp_num == 0 ){

            LE_send_msg( GL_INFO, "--->REDUNDANT: Commanded Download of Current VCP\n" );
            if( rda_command->param2 != 0 ){

               Vcp_struct *vcp = NULL;

               /* VCP data is passed with command. */
               vcp = (Vcp_struct *) rda_command->msg;
               vcp_num = vcp->vcp_num;

               
            }
            else{

               /* Get the VCP number from Volume Status. */
               vcp_num = ORPGVST_get_vcp();
               if( vcp_num == ORPGVST_DATA_NOT_FOUND ){

                  LE_send_msg( GL_ERROR, 
                         "Unable to Inform Redundant Manager of VCP Download\n" );

                  error = 1;
                  break;

               }

            }

            LE_send_msg( GL_INFO, "--->REDUNDANT: Command Download of VCP %d\n", vcp_num );
            red_cmd.parameter2 = vcp_num;

         }
  
         if( (ret = ORPGDA_write( ORPGDAT_REDMGR_CMDS, (char *) &red_cmd, 
                                 sizeof( Redundant_cmd_t ), LB_ANY )) < 0 )
            error = 1;

         break;

      default:
         break;

   /* End of "switch" */
   }

   /* Did we encounter an error? */
   if( error )
      LE_send_msg( GL_INFO, "ORPGDA_write of Redundant Command Failed (%d)\n",
                   ret );

/* End of Redundant_commands() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:   
//      Cleanup function.  Invoked by ORPGTASK_ library termination
//      handler.
//
//   Inputs:   
//      signal - UNIX signal number
//      sig_type - either ORPGTASK_EXIT_NORMAL_SIG or
//                 ORPGTASK_EXIT_ABNORMAL_SIG
//
//   Outputs:
//
//   Returns:  
//      If value returned is non_zero, the process does not terminate
//      after module exit.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//      CR_shutdown_state - crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/
int Cleanup_fxn( int signal, int sig_type ){

   LE_send_msg( GL_INFO, "CONTROL RDA is Shutting Down.\n" );  

   /* Checkpoint previous RDA state. */
   ST_checkpoint_previous_state();

   /* Set global flag indicating that control rda is to shut down. */
   CR_shut_down_state = SHUT_DOWN_STANDBY;

   /* A non-zero return value prevents this task from exiting on return. */
   return (1);

/* End of Cleanup_fxn() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:   
//      This function performs shutdown processing for Control RDA.
//
//      If the RDA was commanded to STANDBY and the RDA is in 
//      STANDBY, issue wideband disconnect.
//
//      If the RDA was not commanded to STANBY, disconnect the wideband
//      line.
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:    
//
//   Globals:
//      CR_shut_down_state - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/
static void Shutdown_processing(){

   int 	wblnstat;
   int	rda_stat	= 0;

   /* Check the wideband line status ... If wideband ever disconnects,
      exit immediately. */
   wblnstat = SCS_get_wb_status( ORPGRDA_WBLNSTAT );
   if( (wblnstat != RS_CONNECTED)
                 &&
       (wblnstat != RS_DISCONNECT_PENDING) ){
 
      CR_shut_down_state = SHUT_DOWN_EXIT;
      
      /* Clear all wideband alarms. */
      SCS_clear_communications_alarm( RDA_COMM_DISC_ALARM );
      SCS_clear_communications_alarm( RDA_LINK_BROKEN_ALARM );

      if( (wblnstat == RS_WBFAILURE)
                    ||
          (wblnstat == RS_DISCONNECTED_HCI) 
                    ||
          (wblnstat == RS_DISCONNECTED_CM) 
                    ||
          (wblnstat == RS_DISCONNECTED_RMS) ) 

         SCS_update_wb_line_status( (int) RS_DISCONNECTED_SHUTDOWN, /* wideband line status */
                                    (int) RS_OPERABILITY_STATUS, /* display blanking */
                                    (int) 1 /* post wideband line status changed event */ ); 
   }

   /* Get the rda_status value */
   rda_stat = ST_get_status(ORPGRDA_RDA_STATUS);

   /* Shutdown processing based on shut down state. */
   switch( CR_shut_down_state ){

      case SHUT_DOWN_STANDBY:
      {
      
         /* Clear all wideband alarms. */
         SCS_clear_communications_alarm( RDA_COMM_DISC_ALARM );
         SCS_clear_communications_alarm( RDA_LINK_BROKEN_ALARM );

         /* Put the RDA in standby. */
         if( (rda_stat != RS_STANDBY) 
                               &&
             (CR_control_status != CS_LOCAL_ONLY) ){

            Rda_cmd_t rda_command;

            LE_send_msg( GL_INFO, "Commanding RDA to STANDBY\n" );

            memset( &rda_command, 0, sizeof( Rda_cmd_t ) );
            rda_command.cmd = COM4_RDACOM;
            rda_command.param1 = CRDA_STANDBY;
      
            if( SWM_process_rda_command( &rda_command ) ==  SWM_FAILURE ){

               LE_send_msg( GL_INFO, "Commanded RDA Standby Failed\n" );
               CR_shut_down_state = SHUT_DOWN_DISCONNECT;
               LE_send_msg( GL_INFO, "Control RDA in SHUT_DOWN_DISCONNECT State\n" );

            }
            else{

               CR_shut_down_state = SHUT_DOWN_STANDBY_WAIT;
               LE_send_msg( GL_INFO, "Control RDA in SHUT_DOWN_STANDBY_WAIT State\n" );

               /* Set rda status timer. */
               if( TS_set_timer( (malrm_id_t) MALRM_RDA_STATUS, 
                                 (time_t) RDA_STATUS_TIMEOUT,
                                 (unsigned int) MALRM_ONESHOT_NTVL ) == TS_FAILURE ){

                  LE_send_msg( GL_INFO, "RDA Status Timeout Timer Failed\n" );
                  CR_shut_down_state = SHUT_DOWN_DISCONNECT;
                  LE_send_msg( GL_INFO, "Control RDA in SHUT_DOWN_DISCONNECT State\n" );

               }
  
            }

         }
         else{

            /* Clear flag so we do not wait for RDA status and set flag to 
               indicate we need to post wideband disconnect command. */
            CR_shut_down_state = SHUT_DOWN_DISCONNECT;
            LE_send_msg( GL_INFO, "Control RDA in SHUT_DOWN_DISCONNECT State\n" );

         }

         break;

      }  

      case SHUT_DOWN_STANDBY_WAIT:
      {
      
         /* If waiting for RDA standby state, then .... */
         if( rda_stat == RS_STANDBY ){ 

            CR_shut_down_state = SHUT_DOWN_DISCONNECT;
            LE_send_msg( GL_INFO, "RDA is in STANDBY\n" );
            LE_send_msg( GL_INFO, "Control RDA in SHUT_DOWN_DISCONNECT State\n" );

         }

         break;

      }

      case SHUT_DOWN_DISCONNECT:
      {

         LE_send_msg( GL_INFO, "Disconnecting Wideband\n" );

         /* Disconnect the wideband line. */
         if( LC_disconnect_wb_line( RS_DISCONNECTED_SHUTDOWN ) != LC_SUCCESS ){

            LE_send_msg( GL_INFO, "Wideband Disconnect Attempt Failed\n" );
            CR_shut_down_state = SHUT_DOWN_EXIT;
            LE_send_msg( GL_INFO, "Control RDA in SHUT_DOWN_EXIT State\n" );

         }
         else
            CR_shut_down_state = SHUT_DOWN_DISCONNECT_WAIT;

         break;

      }

      case SHUT_DOWN_DISCONNECT_WAIT:
      {

         int wblnstat;

         /* Check the wideband line status. */
         wblnstat = SCS_get_wb_status( ORPGRDA_WBLNSTAT );
         if( (wblnstat != RS_CONNECTED) 
                       && 
             (wblnstat != RS_DISCONNECT_PENDING) ){

            LE_send_msg( GL_INFO, "Wideband Is Not Connected." );    
            CR_shut_down_state = SHUT_DOWN_EXIT;
            LE_send_msg( GL_INFO, "Control RDA in SHUT_DOWN_EXIT State\n" );

         }

         break;

      }

      case SHUT_DOWN_EXIT:
      {

         LE_send_msg( GL_INFO, "Control RDA is Exiting NOW\n" );
         exit(0);

         break;

      }

   /* End of "switch" */
   }

/* End of Shutdown_processing() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:   
//      This function reads and processes command line arguments.
//
//   Inputs:   
//      argc - number of command line arguments.
//      argv - command line arguments.
//
//   Outputs:
//
//   Returns:    
//      0 on success or -1 on failure
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h.
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/
static int Read_options (int argc, char **argv){

    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */
    int err;                /* error flag */

    /* Initialize verbose mode to off. */
    CR_verbose_mode = 0;

    /* Initialize include DP data in RDA CAL message. */
    CR_include_dp_pmd = 0;

    /* Initialize the force connection flag. */
    CR_force_connection = 0;

    /* Process all command line arguments. */
    err = 0;

    /* Force connection on RPG NON-OPERATIONAL.  We then can define
       the "-f" command line option to be a toggle in the event the 
       user does not want the line to connect on playback. */
    if( !ORPGMISC_is_operational() )
       CR_force_connection = 1;

    /* Process all command line arguments. */
    while ((c = getopt (argc, argv, "fdl:vh")) != EOF) {
	switch (c) {

            case 'f':
            {
               if( CR_force_connection == 1 )
                  CR_force_connection = 0;

               else
                  CR_force_connection = 1;
               break;
            }
            case 'd':
            {
               CR_include_dp_pmd = 1;
               break;
            }
            case 'l':
            {
                Log_file_nmsgs = atoi( optarg );
                if( Log_file_nmsgs < 0 || Log_file_nmsgs > 5000 )
                   Log_file_nmsgs = 1000;
                break;
            }
	    case 'v':
            {
		CR_verbose_mode = 1;
		break;
            }
	    case 'h':
	    case '?':
            {
		err = 1;
		break;
            }

        /* End of "switch" */
	}

    /* End of "while" */
    }

   /* Error occurred.  Print usage message. */
   if (err == 1) { 			
	printf ("Usage: %s [options]\n", argv[0]);
	printf ("       Options:\n");
        printf ("       -l Log File Number LE Messages (%d)\n", Log_file_nmsgs);
        printf ("       -d Include DP PMD Data in RDA CAL message\n" );
        printf ("       -f Force WB Connection (Testing Only)\n" );
	printf ("       -h (print usage)\n");
	printf ("       -v (verbose mode)\n");
	exit (1);
    }

    return (0);

/* End of Read_options() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:   
//      This function opens all LB accessed by Control RDA. 
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:    
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/
static void Open_lb(){

   int retval, data_id;
   char task_name[ORPG_TASKNAME_SIZ];
   Orpgtat_entry_t *task_entry = NULL;

   /* Get the Link information from ORPGCMI. */
   CR_response_LB = ORPGCMI_rda_response();
   CR_request_LB = ORPGCMI_rda_request();
   CR_link_index = CR_response_LB - ORPGDAT_CM_RESPONSE;

   /* Get my task name .... this will be used to access the task table entry. */
   if( (ORPGTAT_get_my_task_name( (char *) &task_name[0], ORPG_TASKNAME_SIZ ) >= 0)
                                   &&
       ((task_entry = ORPGTAT_get_entry( (char *) &task_name[0] )) != NULL) ){

      /* Check for match on Response data name. */
      if( (data_id = ORPGTAT_get_data_id_from_name( task_entry, "RESPONSE_LB" )) >= 0){

         CR_response_LB = data_id;
         LE_send_msg( GL_INFO,
                      "CR_response_LB (%d) Defined By Task Attribute Table (Link: %d)\n",
                      CR_response_LB, CR_link_index );

      }

      /* Check for match on Request data name. */
      if( (data_id = ORPGTAT_get_data_id_from_name( task_entry, "REQUEST_LB" )) >= 0){

         CR_request_LB = data_id;
         LE_send_msg( GL_INFO,
                      "CR_request (%d) Defined By Task Attribute Table\n",
                      CR_request_LB );
      }

      if( task_entry != NULL )
         free( task_entry );

   }

   /* Check if CR_request_LB is defined. */
   if( CR_request_LB < 0 ){

      /* Failure occurred. */
      LE_send_msg( GL_ERROR, "Error Setting REQUEST_LB\n" );
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* Check if CR_response_LB is defined. */
   if( CR_response_LB < 0 ){

      /* Failure occurred. */
      LE_send_msg( GL_ERROR, "Error Setting RESPONSE_LB\n" );
      ORPGTASK_exit(GL_EXIT_FAILURE);

   }

   /* Open all READ access LBs */
   if( (retval = ORPGDA_open( ORPGDAT_RDA_COMMAND, LB_READ )) < 0 ||
       (retval = ORPGDA_open( CR_response_LB, LB_READ )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_open for READ Failed (%d)\n", retval );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   /* Open all WRITE access LBs. */
   if( (retval = ORPGDA_open( CR_request_LB, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( DATAID_A304C2, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( DATAID_CD07_BYPASSMAP, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_REDMGR_CMDS, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_CLUTTERMAP, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_RDA_PERF_MAIN, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_RDA_CONSOLE_MSG, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_GSM_DATA, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_RDA_STATUS, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_ADAPTATION, LB_WRITE )) < 0 ||
       (retval = ORPGDA_open( ORPGDAT_RDA_ALARMS, LB_WRITE )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_open for WRITE Failed (%d)\n", retval );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }

   /* End of Open_lb() */
}

