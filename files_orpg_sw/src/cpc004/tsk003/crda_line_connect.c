/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/03/07 19:04:53 $
 * $Id: crda_line_connect.c,v 1.54 2007/03/07 19:04:53 steves Exp $
 * $Revision: 1.54 $
 * $State: Exp $
 */

/* File scope global variables. */

/* Flag, if set, indicates the wideband disconnect request was commanded, 
   vice owing to a wideband failure. */
static int Disconnect_commanded = 0;

/* Flag, if set, indicates an attempt to reconnect the wideband line
   should be made after a wideband failure. */
static int Reconnect_wideband_line_after_failure = 0;

#include <crda_control_rda.h>
#include <gen_stat_msg.h>
#include <orpgrda.h>

/*\/////////////////////////////////////////////////////////////////
//   
//  Description:
//
//     Initialization function for LC module.
//
//     Calls SCS initialization module.
//
//  Inputs:
//
//  Outputs:
//
//  Returns:
//     There are no return values defined for this function.
//
//  Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//////////////////////////////////////////////////////////////////\*/
void LC_init(){

   /* Initialize SCS module. */
   SCS_init( );

   /* Initialize flags. */
   Reconnect_wideband_line_after_failure = 0;
 
/* End of LC_init() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description: 
//       This module processes the wideband line connection 
//       command.  The following actions are taken:
//
//        1) If the wideband line status is not RS_CONNECT_PENDING but
//           the wideband line has not failed, then the wideband line 
//           status is update to RS_CONNECT_PENDING.
//        2) If the MALRM_CONNECTION_RETRY timer is not active, 
//           then this timer is activated.
//        3) If the MALRM_DISCONNECTION_RETRY timer is active, then 
//           deactivate the timer.
//        4) A connection request is posted.
//
//   Inputs:  
//
//   Outputs:
//
//   Returns: 
//      Returns LC_FAILURE if connection request fails for some 
//      reason, or LC_SUCCESS otherwise.  
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int LC_connect_wb_line(){

   int ret;

   if( SCS_get_wb_status( ORPGRDA_WBLNSTAT ) != RS_CONNECT_PENDING ){
   
      /* If the wideband link has failed, do not set the wideband line
         status.  Keep it as failed. */
      if( !SCS_check_wb_alarm_status( RDA_LINK_BROKEN_ALARM ) ){ 

         /* Set the wideband line status to CONNECT_PENDING and update 
            wideband line status linear buffer.  Set the display blanking
            value to RS_OPERABILTIY_STATUS only. */
         SCS_update_wb_line_status( (int) RS_CONNECT_PENDING,
                                    (int) RS_OPERABILITY_STATUS, /* display blanking */
                                    (int) 1  /* post wideband line
                                                status changed event */ );

      }
   
   }
 
   /* Is the connection retry timer active?  If not, set it. */
   if( !TS_is_timer_active( MALRM_CONNECTION_RETRY ) )
         TS_set_timer( (malrm_id_t) MALRM_CONNECTION_RETRY,
                       (time_t) CR_connect_retry_value,
                       (unsigned int) RDA_CONNECT_RETRY_VALUE );

   LE_send_msg( GL_INFO, "Requesting Connection Of Wideband Line.\n" );

   /* If the disconnection retry timer is active, deactive it. */
   if( TS_is_timer_active( (malrm_id_t) MALRM_DISCONNECTION_RETRY ) )
      TS_cancel_timer( (int) 1, MALRM_DISCONNECTION_RETRY );

   /* Request connection of wideband line. */
   if( (ret = SWM_request_operation( RQ_CONNECT, /* type of request */ 
                                     (int) 0, /* parameter associated with type */
                                     (int) DONT_WAIT_FOR_RESP, (int) 0 )) < SWM_SUCCESS ){
   
      /* Connection failed for some reason. */
      return ( LC_FAILURE );

   }

   return ( LC_SUCCESS );

/* End of LC_connect_wb_line() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:  
//       This module processes the wideband line disconnection 
//       command.  The following actions are taken:
//
//        1) If the wideband line is not in a disconnected state,
//           then the wideband line status is update to RS_DISCONNECT_PENDING.
//           If the line is already disconnected, there is nothing
//           more to do.
//        2) The MALRM_RDA_COMM_DISC timer, the MALRM_LB_TEST_TIMEOUT timer, 
//           the MALRM_LOOPBACK_PERIODIC, and the MALRM_CONNECTION_RETRY
//           timers are cancelled, if active.
//        3) Various control flags are cleared to prevent future
//           processing.
//        4) The request queue is clear of any outstanding requests.
//        5) Command the wideband line to disconnect.
//        6) Activate the Disconnection Retry timer.  If the RPG is 
//           shutting down, make the time one shot.  Otherwise, make it
//           periodic.
//
//   Inputs:  
//
//   Outputs:
//
//   Returns:   
//      Returns status of disconnection request, or LC_SUCCESS.  
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//      CR_loopback_timeout - see crda_control_rda.h
//      CR_loopback_failure - see crda_control_rda.h
//      CR_communications_discontinuity - see crda_control_rda.h
//      CR_perform_loopback_test - see crda_control_rda.h
//
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int LC_disconnect_wb_line( int whodunit ){

   int ret;
   int wblnstat;

   /* If wideband line is not already disconnected, update wideband line
      status. */
   wblnstat = SCS_get_wb_status( ORPGRDA_WBLNSTAT );
   if( (wblnstat != RS_DISCONNECTED_HCI)
                    &&
       (wblnstat != RS_DISCONNECTED_CM)
                    &&
       (wblnstat != RS_DISCONNECTED_SHUTDOWN) 
                    &&
       (wblnstat != RS_DISCONNECTED_RMS) ){
   
      /* Set the wideband line status to DISCONNECT_PENDING and
         update wideband line status. */
      SCS_update_wb_line_status( (int) RS_DISCONNECT_PENDING,
                                 (int) -1, /* display blanking no change */
                                 (int) 1 /* post wideband line
                                            status changed event */ );
   
   }
   else{

      /* Line is already disconnected.  There is nothing to do. */
      if( CR_verbose_mode )
         LE_send_msg( GL_INFO, 
                 "Disconnection Request Received.  Line Already Disconnected.\n" ); 
   
       return (LC_SUCCESS);

   }

   /* Cancel the communication discontinuity timer, the loopback test timeout
      timer, the connection retry timer, and the loopback periodic timer,
      if active. */
   TS_cancel_timer( (int) 4, (malrm_id_t) MALRM_RDA_COMM_DISC,
                    (malrm_id_t) MALRM_LB_TEST_TIMEOUT,
                    (malrm_id_t) MALRM_CONNECTION_RETRY,
                    (malrm_id_t) MALRM_LOOPBACK_PERIODIC );
   
   /* Clear loopback timeout flag, communications discontinuity flag, 
      and perform loopback test flag.  These flags need to be cleared to
      prevent processing which might otherwise be performed. */
   CR_loopback_timeout = CR_loopback_failure = 0;
   CR_communications_discontinuity = 0;
   CR_perform_loopback_test = 0;
   CR_loopback_retries = 0;

   /*
     Set the Disconnect_commanded to true.
   */
   Disconnect_commanded = whodunit;

   /*
     Do not attempt to reconnect line after a failure.
   */
   Reconnect_wideband_line_after_failure = 0;

   /*
     Clear any outstanding requests in the outstanding request queue.
   */
   QS_clear_request_queue();


   /*
     Disconnect the wideband line.
   */
   LE_send_msg( GL_INFO, "Requesting Disconnection Of Wideband Line.\n" );
   if( (ret = SWM_request_operation( RQ_DISCONNECT, /* type of request */
                                     (int) 0, /* parmeter associated with type */
                                     (int) DONT_WAIT_FOR_RESP, (int) 0 )) >= SWM_SUCCESS ){
      /*
        Set the disconnection retry timer if retry timer is not active.
      */
      if( !TS_is_timer_active( MALRM_DISCONNECTION_RETRY ) ){

         if( CR_shut_down_state != SHUT_DOWN_NO )
            TS_set_timer( (malrm_id_t) MALRM_DISCONNECTION_RETRY,
                          (time_t) RDA_DISCONNECT_SHUTDOWN,
                          (unsigned int) MALRM_ONESHOT_NTVL );
         else
            TS_set_timer( (malrm_id_t) MALRM_DISCONNECTION_RETRY,
                          (time_t) RDA_DISCONNECT_RETRY_VALUE,
                          (unsigned int) RDA_DISCONNECT_RETRY_VALUE );

      }

   }
   else

      /*
        Disconnection failed for some reason.
      */
      return ( ret );
   
   return ( LC_SUCCESS );

/* End of  LC_disconnect_wb_line() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:  
//      Process wideband line connection.  This module is called after
//      a connection command was posted and the comm manager has 
//      responded to the request.  
//
//      The following actions are taken:
//
//      If line connection was successful, then:
//         a) the MALRM_CONNECTION_RETRY timer is cancelled,
//         b) display blanking is cleared,
//         c) the wideband line status is updated to RS_CONNECTED,
//         d) the Alarms RDA_LINK_BROKEN_ALARM and RDA_COMM_DISC_ALARM
//            are cleared,
//         e) the CR_status_latch flag is cleared, the 
//            CR_status_protect flag is set, the CR_rda_restarting flag
//            is cleared, and the CR_return_to_previous_state phase is
//            set to PREVIOUS_STATE_PERFORM_LOOPBACK.
//
//      If the line connection was terminated by the comm manager, then:
//         a) cancel the connection retry timer.  
//
//      All other failures, return error.
//
//   Inputs:  
//      comm_manager_returned - Communications Manager return value from 
//                              connection request.
//
//   Outputs:
//
//   Returns:   
//      Returns CM_CONNECTED if line is connected, or LC_FAILURE if
//      connection timeout. 
//      
//
//   Globals:
//      CR_status_latch - see crda_control_rda.h
//      CR_status_protect - see crda_control_rda.h
//      CR_verbose_mode - see crda_control_rda.h
//      CR_rda_is_restarting - see crda_control_rda.h
//
//   Notes:
//     Currently, only one wideband line is supported: 
//     RDA_PRIMARY.  
//
//     All global variables are defined and described in 
//     crda_control_rda.h.  These will begin with CR_.  All file 
//     scope global variables are defined and described at the 
//     top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int LC_process_wb_line_connection( int comm_manager_returned ){

   /* Line is connected ..... */
   if ( comm_manager_returned == CM_SUCCESS
                       || 
        comm_manager_returned == CM_CONNECTED ){

      unsigned char value = 0;

      /* Cancel the connection retry timer and the RDA restart timer. */
      TS_cancel_timer( (size_t) 2, 
                       (malrm_id_t) MALRM_CONNECTION_RETRY,
                       (malrm_id_t) MALRM_RESTART_TIMEOUT );

      /* Clear RDA display blanking.  That is, tell the HCI to display both
         state and operability status of the RDA.  Set the wideband line status 
         to RS_CONNECTED and update wideband line status linear buffer. */
      SCS_update_wb_line_status( (int) RS_CONNECTED,  /* wideband line status */
                                 (int) 0, /* disable display blanking */
                                 (int) 1 /* post wideband line 
                                            status changed event */ );

      /* Clear RPG Communication alarms which may be active. */
      SCS_clear_communications_alarm( RDA_LINK_BROKEN_ALARM );
      SCS_clear_communications_alarm( RDA_COMM_DISC_ALARM );

      /* Check the state data to see if the wideband alarm is still
         active.  This might happen if the wideband alarm was active
         and the RPG was restarted. If alarm set, clear it now. */
      if( ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_WBFAILRE,
                                      ORPGINFO_STATEFL_GET, &value ) >= 0 ){

         if( value )
            ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_WBFAILRE,
                                        ORPGINFO_STATEFL_CLR, &value );

      }

      /* Clear the RDA status latch and set the status protect flag. */
      CR_status_latch = 0;
      CR_status_protect = 1;

      /* Clear the RDA is restarting flag. */
      CR_rda_restarting = 0;

      /* Schedule loopback test on wideband line. */
      CR_return_to_previous_state = PREVIOUS_STATE_PERFORM_LOOPBACK;

   }
   
   /* Connection request was terminated by disconnection request. */
   else if( comm_manager_returned == CM_TERMINATED ){

      if( CR_verbose_mode )
         LE_send_msg( GL_INFO, 
                      "Connection Request Terminated!  Cancel Connection Retry Timer.\n" );

      /* Cancel the connection retry timer, if active. */
      TS_cancel_timer( (size_t) 1, (malrm_id_t) MALRM_CONNECTION_RETRY );

   }

   /* All other comm manager returned values not already processed.
      Line does not connect.  Currently no action is taken except to
      report an error. */
   else{

      if( CR_verbose_mode )
         LE_send_msg( GL_INFO, "Comm Manager Returned %d From Connection Request.\n",
                      comm_manager_returned );  
      /* Return error. */
      return ( LC_FAILURE );

   }

   /* Return line connected. */
   return ( CM_CONNECTED );
        
/* End of LC_process_wb_line_connection() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:  
//      Processes results wideband line disconnection command.  It is
//      assumed a disconnection command was already posted.  This module
//      services the comm manager response to this command.
//
//      The following actions are taken:
//
//      If the line disconnection was successful, then:
//         a) the MALRM_DISCONNECT_RETRY timer is cancelled,
//         
//         If the disconnection was operator commanded, then:
//            a) RDA display blanking is set to RS_OPERABILITY_STATUS,
//            b) the wideband line status is set to RS_DISCONNECTED_HCI 
//               if operator commanded disconnect, RS_DISCONNECTED_RMS
//               if RMS commanded disconnect or RS_DISCONNECTED_CM
//               otherwise,
//            c) the MALRM_RDA_COMM_DISC timer is cancelled, 
//            d) all RDA alarms are cleared and the RDA status is
//               updated,
//            e) the request queue is clear of outstanding requests,
//            f) return disconnected.
//
//      If the line disconnection was not successful, then 
//         If the disconnection was not operator commanded, then:
//            a) the wideband line status is set to RS_WBFAILURE,
//            b) the disconnection commanded flag is cleared.
//         a) return error.
//
//   Inputs:  
//      comm_manager_returned - return value from Communications Manager 
//                              associated with the disconnection request.
//
//   Outputs:
//
//   Return:  
//      LC_FAILURE on failure, otherwise CM_DISCONNECTED
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//      CR_shutdown_state - see crda_control_rda.h
//
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int LC_process_wb_line_disconnection( int comm_manager_returned )
{
   int ret = 0;

   /* Line is disconnected .....  */
   if ( comm_manager_returned == CM_SUCCESS 
                      || 
        comm_manager_returned == CM_DISCONNECTED ){

      /* Cancel disconnect retry timer, if active. */
      TS_cancel_timer( (int) 1, (malrm_id_t) MALRM_DISCONNECTION_RETRY );

      /* If wideband disconnect was operator commanded (i.e., it was not in response
         to an CM_LOST_CONN event), then.... */
      if( Disconnect_commanded ){

         /* Set RDA display blanking to only allow OPERABILITY STATUS display.
            Set the wideband line status and update wideband line status 
            linear buffer. */
         if( CR_shut_down_state == SHUT_DOWN_NO ){

            if( Disconnect_commanded != RS_DISCONNECTED_RMS )
               SCS_update_wb_line_status( (int) RS_DISCONNECTED_HCI, /* wideband line status */ 
                                          (int) RS_OPERABILITY_STATUS, /* display blanking */
                                          (int) 1 /* post wideband line
                                                     status changed event */ );
     
            else
               SCS_update_wb_line_status( (int) RS_DISCONNECTED_RMS, /* wideband line status */ 
                                          (int) RS_OPERABILITY_STATUS, /* display blanking */
                                          (int) 1 /* post wideband line
                                                     status changed event */ );
         }
         else{

            SCS_update_wb_line_status( (int) RS_DISCONNECTED_SHUTDOWN, /* wideband line status */ 
                                       (int) RS_OPERABILITY_STATUS, /* display blanking */
                                       (int) 1 /* post wideband line
                                                  status changed event */ );
         }

         /* Clear the Disconnect_commanded flag (i.e., set it to false). */
         Disconnect_commanded = 0;

         /* Cancel the RDA-RPG communications discontinuity timer. */
         TS_cancel_timer( (int) 1, (malrm_id_t) MALRM_RDA_COMM_DISC );

         /* Clear any RDA alarms. */
         ST_clear_rda_alarm_codes();

         /* Set the operability status to OS_WIDEBAND_DISCONNECT */
         ret = ST_set_status(ORPGRDA_OPERABILITY_STATUS,OS_WIDEBAND_DISCONNECT);
         if ( ret < 0 )
         {
            LE_send_msg( GL_STATUS | GL_ERROR, "Failed to set op_status.\n");
            return ( LC_FAILURE );
         }

         /* Update RDA status. */
         ST_update_rda_status( (int) 1, /* post rda status changed event */
                               NULL,
                               (int) RPG_INITIATED );

         /* Clean up outstanding request queue and re-initialize outstanding 
            request list. */
         QS_clear_request_queue();
      }

      /* Return line disconnected. */
      return ( CM_DISCONNECTED );
   }
   else
   {
      if( CR_verbose_mode )
      {
         LE_send_msg( GL_INFO, 
              "THIS SHOULD NEVER HAPPEN! Disconnect Request Failed. (%d)\n", 
               comm_manager_returned );
      }

      /* If wideband disconnect was operator commanded (i.e., it was not in response
         to an CM_LOST_CONN event), then.... */
      if( Disconnect_commanded ){

         /* Clear the disconnect commanded flag. */
         Disconnect_commanded = 0;

         /* Post RDA alarm. */ 
         SCS_handle_wideband_alarm( (int) RS_WBFAILURE, (int) RDA_LINK_BROKEN_ALARM,
                                    (int) RS_OPERABILITY_STATUS /* display blanking */ );
      }

      /* Return error. */
      return ( LC_FAILURE );
   }
   
} /* End of LC_process_wb_line_disconnection() */ 


/*\//////////////////////////////////////////////////////////////////
//
//   Description:  
//      Request status about wideband line.  The following actions
//      are taken:
//
//      If the status request returns CM_CONNECTED, then:
//         a) process the unexpected line connection,
//         b) return wideband line is connected.
//
//      If the status request returns CM_DISCONNECTED, then:
//         a) return wideband line is disconnected.
//
//      All other status values returned considered errors.
//
//   Inputs: 
//
//   Outputs:
//
//   Returns:  
//      Wideband line status.  Value is negative if failure 
//      occurred.  Otherwise, either RS_CONNECTED or 
//      RS_DISCONNECTED_CM.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//
//   Notes:
//      This function is intended only to be called at control_rda
//      start-up.  To get the wideband line status at other times,
//      should call SCS_get_wb_status().
//
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
/////////////////////////////////////////////////////////////////\*/
int LC_check_wb_line_status( ){

   int status_request;

   /* Post a request status on this line. */
   status_request = SWM_request_operation( RQ_STATUS, /* type of request */
                                           (int) 1, /* parameter associated with type */
                                           (int) WAIT_FOR_RESP, 
                                           (int) 10000 /* wait time, in millsecs */ );

   if ( status_request == CM_CONNECTED ){

      LE_send_msg( GL_INFO, "Requested Wideband Line Status: CONNECTED\n" );

      /* Process unexpected wideband line connection. */
      LC_process_wb_line_connection( CM_CONNECTED );

      /* Return line is connected. */
      return ( RS_CONNECTED );

   }

   else if( status_request == CM_DISCONNECTED ) {

      if( CR_verbose_mode )
         LE_send_msg( GL_INFO, "Requested Wideband Line Status: DISCONNECTED\n" );

      /* Return line is disconnected. */
      return ( RS_DISCONNECTED_CM );

   }

   /* Treat all other status values as failures. */
   else{

      if( CR_verbose_mode )
         LE_send_msg( GL_INFO, "Requested Wideband Line Status Failed. (%d)\n", 
                      status_request );

      /* Return error. */
      return ( LC_FAILURE );
 
   }

/* End of LC_check_wb_line_status() */
}
 

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Processes unexpected wideband line disconnection.
//
//      1) Cancel the MALRM_RDA_COMM_DISC, MALRM_LB_TEST_TIMEOUT,
//         MALRM_CONNECTION_RETRY, and MALRM_LOOPBACK_PERIODIC timers.
//      2) Clear global flags 
//            CR_loopback_failure 
//            CR_loopback_timeout
//            CR_communications_discontinuity
//            CR_perform_loopback_test
//            CR_loopback_retries
//
//      3) Set alarm code to no RDA alarms
//           
//      The following actions are performed if the wideband line 
//      status is not disconnect_pending, disconnected by hci, 
//      disconnected by comm manager, disconnected by shutdown or failed:
//         1) Set the wideband line status to disconnected by comm 
//            manager.
//         2) If line drop was due to RS_RESTART or RS_COMMANDED SWITCHOVER
//            and the comm manager did not die:
//            a)  The RESTART timer is activated.
//            b)  RDA display blanking is set to RS_RDA_STATUS.
//            d)  Set the RDA restarting flag.
//         3) If line drip was due to OS_COMMANDED_SHUTDOWN and the comm
//            manager did not die:
//            a)  Set RDA display blanking to RS_OPERABILITY_STATUS.
//         4) If the line drop was unexpected:
//            a)  Set RDA display blanking to RS_OPERABILITY_STATUS. 
//            b)  Set the wideband line status to RS_WBFAILURE.
//            c)  Set the RDA alarm RDA_LINK_BROKEN_ALARM.
//         5) Post RDA alarm code.
//         6) Cancel any previous requests.
//         7) If the communications manager did not die, then:
//            a) Attempts to disconnect the wideband line.
//            b) Set the Reconnect_wideband_line_disconnection flag.
//            b) Sets the MALRM_DISCONNECT_RETRY timer. 
//
//   Inputs:
//      event_type - Communications manager event code, or NULL.
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//      CR_loopback_failure - see crda_control_rda.h
//      CR_loopback_timeout - see crda_control_rda.h
//      CR_communications_discontinuity - see crda_control_rda.h
//      CR_perform_loopback_test - see crda_control_rda.h
//      CR_loopback_retries - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
void LC_process_unexpected_line_disconnection( int event_type )
{
   int alarm_code		= 0;
   int wideband_line_status	= 0;
   int display_blanking		= 0;
   int ret			= 0;
   int wblnstat 		= 0;
   int rda_stat 		= 0;
   int aux_pwr_stat 		= 0;
   int op_stat	 		= 0;
   

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "Process Unexpected Line Disconnection.\n" );

   /* Cancel the communications discontinuity timer, the loopback test 
      timeout timers, the connection retry timer, the RDA status wait timer,
      and the periodic loopback timer, if active. */
   TS_cancel_timer( (int) 5, (malrm_id_t) MALRM_RDA_COMM_DISC, 
                             (malrm_id_t) MALRM_LB_TEST_TIMEOUT,
                             (malrm_id_t) MALRM_CONNECTION_RETRY,
                             (malrm_id_t) MALRM_RDA_STATUS,
                             (malrm_id_t) MALRM_LOOPBACK_PERIODIC );

   /* Clear loopback timeout flag, communications discontinuity flag, 
      and perform loopback test flag. */
   CR_loopback_timeout = CR_loopback_failure = 0;
   CR_communications_discontinuity = 0;
   CR_perform_loopback_test = 0;
   CR_loopback_retries = 0;
   CR_return_to_previous_state = PREVIOUS_STATE_NO;

   /* Clear rda alarm code to indicate no new alarm. */
   alarm_code = RDA_ALARM_NONE;

   /* If wideband line is already in a disconnect pending state, or the wideband
      line is already disconnected or has failed, return. */
   wblnstat = SCS_get_wb_status( ORPGRDA_WBLNSTAT );
   if( (wblnstat == RS_WBFAILURE) || (wblnstat == RS_DISCONNECT_PENDING) ||
       (wblnstat == RS_DISCONNECTED_CM) || (wblnstat == RS_DISCONNECTED_HCI) ||
       (wblnstat == RS_DISCONNECTED_SHUTDOWN) || (wblnstat == RS_DISCONNECTED_RMS) ) 
      return;
 
   /* Set wideband line status to disconected by Comm Manager. */
   wideband_line_status = RS_DISCONNECTED_CM;
   
   /* Retrieve and store RDA status data */
   rda_stat = ST_get_status( ORPGRDA_RDA_STATUS );
   aux_pwr_stat = ST_get_status( ORPGRDA_AUX_POWER_GEN_STATE );
   op_stat = ST_get_status( ORPGRDA_OPERABILITY_STATUS );

   /* If the RDA is restarting or there is a commanded power switchover, 
      then .. */
   if ( ( ( rda_stat == RS_RESTART ) ||
          ( aux_pwr_stat == RS_COMMANDED_SWITCHOVER) ) && 
          ( event_type != CM_TERMINATE )){
 
      if( CR_verbose_mode ){

         if (rda_stat == RS_RESTART)
            LE_send_msg( GL_STATUS, "RDA is Restarting.\n" );
          
         else if (aux_pwr_stat == RS_COMMANDED_SWITCHOVER)
            LE_send_msg( GL_STATUS, "RDA Commanded Power Switchover.\n" );
          
      }

      /* Set RDA restart timer. */
      TS_set_timer( (malrm_id_t) MALRM_RESTART_TIMEOUT,
                    (time_t) RDA_RESTART_VALUE,
                    (unsigned int) MALRM_ONESHOT_NTVL );

      /* Set RDA display blanking to display only the RDA STATUS. */
      display_blanking = RS_RDA_STATUS;

      /* Set the RDA is restarting flag. */
      CR_rda_restarting = 1;

   }
   
   /* If the RDA commanded shutdown, then .. */
   else if ( ( ( op_stat & 0xfffe) == OS_COMMANDED_SHUTDOWN ) &&
             ( event_type != CM_TERMINATE ) ){

      /* Set the RDA display blanking to display only OPERABILITY STATUS. */
      display_blanking = RS_OPERABILITY_STATUS;

   }
  
   /* If the line unexpectedly dropped, then .. */
   else{

      /* Set the RDA display blanking to display only OPERABILITY STATUS. */
      display_blanking = RS_OPERABILITY_STATUS;

      /* Set the rda alarm code to indicate link was broken unexpectedly,
         and set wideband line status to wideband failure. */
      alarm_code = RDA_LINK_BROKEN_ALARM;
      wideband_line_status = RS_WBFAILURE;

   }
   
   /* Post RDA alarm. */
   SCS_handle_wideband_alarm( wideband_line_status, alarm_code,
                              display_blanking );

   /* Clear the outstanding request queue and re-initialize the request list. */
   QS_clear_request_queue();

   /* If the comm manager did not die, then ..... */
   if( event_type != CM_TERMINATE ){

      /* Set the flag to reconnect the wideband line after the wideband 
         disconnects. */
      Reconnect_wideband_line_after_failure = 1;

      /* Post a disconnect to be sure line is really disconnected. */
      if( (ret = SWM_request_operation( RQ_DISCONNECT, /* type of request */
                                        (int) 0, /* parm assoc with type */
                                        (int) DONT_WAIT_FOR_RESP, (int) 0 )) == SWM_SUCCESS){
 
         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "Disconnect Operation Requested.\n" );
          
         /* Set disconnect retry timer. */
         if( TS_set_timer( (malrm_id_t) MALRM_DISCONNECTION_RETRY,
                           (time_t) RDA_DISCONNECT_RETRY_VALUE,
                           (unsigned int) RDA_DISCONNECT_RETRY_VALUE ) == TS_FAILURE ){

            /* An error occurred setting the disconnection retry timer.  Treat
               this the same as if the disconnection retry exhausted. */
            LC_process_disconnection_retry_exhausted();

         }

      }

   }

} /* End of LC_process_unexpected_line_disconnection() */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      Processes connection retry limit exceeded.
//
//      This module performs the following actions:
//         1) Cancels connection retry timer.
//         2) Sets RDA display blanking to OPERABILITY status only.
//         3) Posts RDA alarm (RDA_LINK_BROKEN_ALARM) and sets the 
//            communications line status to wideband failure.
//         4) Attempts reconnection of wideband line.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      Returns return values from LC_line_connect call.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////////\*/
int LC_process_connection_retry_exhausted()
{
   /* Cancel connection retry timer. */
   TS_cancel_timer( (int) 1, (malrm_id_t) MALRM_CONNECTION_RETRY );

   /* Set RDA display blanking to only allow OPERABILITY STATUS display.
      Post wideband alarm if not already active. */
   SCS_handle_wideband_alarm( (int) RS_WBFAILURE /* wideband line status */, 
                              (int) RDA_LINK_BROKEN_ALARM /* alarm code */,
                              (int) RS_OPERABILITY_STATUS /* display blanking */ );

   /* Attempt reconnection of wideband line. */
   return( LC_connect_wb_line() );

} /* End of LC_process_connection_retry_exhausted() */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      Processes disconnection retry limit exceeded.
//
//      The disconnection retry timer is cancelled.
//
//      This module performs the following actions if RPG is shutting
//      down when retry limit exceeded:
//         1) Sets RDA display blanking to OPERABILITY status only,
//            set wideband line status to disconnected by shutdown,
//            and clears all RDA alarms.
//         2) Set the shutdown state to shutdown exit state.
//
//      If the RPG is not shutting down:
//         1) Sets RDA display blanking to OPERABILITY status only,
//            sets wideband line status to wideband failure, and
//            sets the RDA link broken alarm.
//
//
//      Attempts to reconnect wideband line after wideband failure.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
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
////////////////////////////////////////////////////////////////////\*/
void LC_process_disconnection_retry_exhausted()
{
   /* Cancel disconnection retry timer. */
   TS_cancel_timer( (int) 1, (malrm_id_t) MALRM_DISCONNECTION_RETRY );

   /* If in shutdown state, set shutdown state to exit. */
   if( CR_shut_down_state != SHUT_DOWN_NO )
   {
      CR_shut_down_state = SHUT_DOWN_EXIT;

      /* Set RDA display blanking to only allow OPERABILITY STATUS display.
         Change wideband line status and post wideband alarm if alarm not 
         already set. */
      SCS_handle_wideband_alarm( (int) RS_DISCONNECTED_SHUTDOWN, (int) RDA_ALARM_NONE,
                                 (int) RS_OPERABILITY_STATUS );
      return;
   }
   else
   {
      /* Set RDA display blanking to only allow OPERABILITY STATUS display.
         Change wideband line status and post wideband alarm if alarm not 
         already set. */
      SCS_handle_wideband_alarm( (int) RS_WBFAILURE, (int) RDA_LINK_BROKEN_ALARM,
                                 (int) RS_OPERABILITY_STATUS );
   }

   /* Try and reconnect the wideband line if after wideband line failure. */
   LC_reconnect_line_after_failure();

} /* End of LC_process_disconnection_retry_exhausted() */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      This module performs a wideband line connection attempt if
//      the Reconnect_wideband_line_after_failure flag is set.
//
//      Checks if the Disconnection retry timer is active, and if active,
//      cancels the timer.  
//      
//      Attempts wideband connection.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      Returns return value from LC_connect_wb_line call, or 0.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////////\*/
int LC_reconnect_line_after_failure()
{
   /* If the disconnection was in response to a wideband failure,
      try and reconnect the wideband line. */
   if( Reconnect_wideband_line_after_failure )
   {
      Reconnect_wideband_line_after_failure = 0;

      /* If disconnection retry timer is active, deactivate it. */
      if( TS_is_timer_active( (malrm_id_t ) MALRM_DISCONNECTION_RETRY ) )
         TS_cancel_timer( (int) 1, (malrm_id_t ) MALRM_DISCONNECTION_RETRY );
  
      /* Attempt wideband connection. */
      LE_send_msg( GL_INFO, "Reconnecting Wideband Line After Failure.\n" );
      return( LC_connect_wb_line() );
   }
 
   return ( LC_SUCCESS );

} /* End of LC_reconnect_line_after_failure() */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      This module sets Reconnect_wideband_line_after_failure flag.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      There is no return value define for this module.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////////\*/
void LC_set_reconnect_after_failure_flag()
{
     Reconnect_wideband_line_after_failure = 1;

} /* End of LC_set_reconnect_after_failure_flag() */
