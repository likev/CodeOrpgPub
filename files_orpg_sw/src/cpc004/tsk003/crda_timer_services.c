/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:47:44 $
 * $Id: crda_timer_services.c,v 1.51 2014/03/13 19:47:44 steves Exp $
 * $Revision: 1.51 $
 * $State: Exp $
 */

#include <crda_control_rda.h>
#include <stdarg.h>
#include <missing_proto.h>

#define LOOPBACK_RETRY_LIMIT    4

/* File scope global variables. */

/* The following are timer activity flags.  A non-zero value denotes
   whether the timer is MALRM_ONESHOT_NTVL or some interval. */

/* Flag, if set, indicates that the Restart_timemout_active timer is
   active. */
static unsigned int Restart_timeout_active = 0;

/* Flag, if set, indicates that the LB_test_timemout_active timer is
   active. */
static unsigned int LB_test_timeout_active = 0;

/* Flag, if set, indicates that the Connection_retry_active timer is
   active. */
static unsigned int Connection_retry_active = 0;

/* Flag, if set, indicates that the Disconnection_retry_active timer is
   active. */
static unsigned int Disconnection_retry_active = 0;

/* Flag, if set, indicates that the Rda_comm_disc_active timer is
   active. */
static unsigned int Rda_comm_disc_active = 0;

/* Flag, if set, indicates that the Rda_status_active timer is
   active. */
static unsigned int Rda_status_active = 0;

/* Flag, if set, indicates the Loopback_periodic timer is active. */
static unsigned int Loopback_periodic_active = 0;

/* Flag, if set, indicates the channel control timer is active. */
static unsigned int Chan_ctrl_active = 0;

/* Flags, if set, indicates the particular timer has expired. */
static int Rda_comm_disc_expired = 0;
static int LB_test_timeout_expired = 0;
static int Connection_retry_expired = 0;
static int Disconnection_retry_expired = 0;
static int Restart_timeout_expired = 0;
static int Rda_status_expired = 0;
static int Loopback_periodic_expired = 0;
static int Chan_ctrl_expired = 0;

/* Function Prototypes. */
void Timer_callback( malrm_id_t timer_id );


/*\/////////////////////////////////////////////////////////////////
//
//   Description:
//     This module handles all timer registration for control_rda.
//
//     The timers registered are:
//        MALRM_RDA_COMM_DISC
//        MALRM_LB_TEST_TIMEOUT
//        MALRM_CONNECTION_RETRY 
//        MALRM_RESTART_TIMEOUT 
//        MALRM_DISCONNECTION_RETRY 
//        MALRM_RDA_STATUS 
//        MALRM_LOOPBACK_PERIODIC 
//        MALRM_CHAN_CTRL
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      There is no return values defined for this function.
//
//
//   Globals:
//      CR_msg_processed - see crda_control_rda.h
//      CR_communications_discontinuity - see crda_control_rda.h
//      CR_loopback_timeout - see crda_control_rda.h
//      CR_loopback_failure - see crda_control_rda.h
//      CR_loopback_retries - see crda_control_rda.h
//      CR_connect_wideband_after_loopback_failure - see crda_control_rda.h
//      CR_perform_loopback_test - see crda_control_rda.h
//      CR_connection_retries - see crda_control_rda.h
//      CR_connection_retries_exhausted - see crda_control_rda.h 
//      CR_disconnection_retries - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
//      See MALRM_register man page for details of this function.
//
//      As of this writing, the MALRM library only supports at most
//      8 timers on behalf of the user.  If control RDA requires 
//      additional timers, the library needs to be unpdated.
//
//////////////////////////////////////////////////////////////////\*/
void TS_register_timers(){

   int ret;

   /* Initialize all timer expiration flags. */
   CR_timer_expired = 0;
   Rda_comm_disc_expired = 0;
   LB_test_timeout_expired = 0;
   Connection_retry_expired = 0;
   Disconnection_retry_expired = 0;
   Restart_timeout_expired = 0;
   Rda_status_expired = 0;
   Loopback_periodic_expired = 0;
   Chan_ctrl_expired = 0;

   /* Register communication discontinuity timer and initialize message 
      counter. */
   if( (ret = ALARM_register( MALRM_RDA_COMM_DISC, 
                              Timer_callback )) < 0 ) 
      LE_send_msg( GL_MALRM(ret), 
                   "Unable To Register MALARM_RDA_COMM_DISC (%d).\n", ret );

   /* Initialize the number of radial messages processed and the timer
      active flag to inactive.  Indicate that the communications 
      discontinuity condition is not currently active. */
   CR_msg_processed = 0;
   Rda_comm_disc_active = 0;
   CR_communications_discontinuity = 0;

   /* Register RPG/RDA loopback test timeout timer. */
   if( (ret = ALARM_register( MALRM_LB_TEST_TIMEOUT, 
                              Timer_callback )) < 0 )
      LE_send_msg( GL_MALRM(ret), 
                   "Unable To Register MALRM_LB_TEST_TIMEOUT (%d).\n", ret );

   /* Initialize the timer active flag to inactive, clear the loopback test 
      timeout flag, and clear the flag indicating that a loopback test needs
      to be performed. */
   LB_test_timeout_active = 0;
   CR_loopback_timeout = 0;
   CR_loopback_failure = 0;
   CR_perform_loopback_test = 0;
   CR_loopback_retries = 0;
   CR_reconnect_wideband_line_after_loopback_failure = 0;

   /* Register RPG/RDA wideband connection retry timer. */
   if( (ret = ALARM_register( MALRM_CONNECTION_RETRY, 
                              Timer_callback )) < 0 )
      LE_send_msg( GL_MALRM(ret), 
                   "Unable To Register MALRM_CONNECTION_RETRY (%d).\n", ret );

   /* Initialize the number of connection retries it 0 and the timer active
      flag to inactive. */
   CR_connection_retries = 0;
   CR_connection_retries_exhausted = 0;
   Connection_retry_active = 0;

   /* Register RDA restart timeout timer. */
   if( (ret = ALARM_register( MALRM_RESTART_TIMEOUT, 
                              Timer_callback )) < 0 )
      LE_send_msg( GL_MALRM(ret), 
                   "Unable To Register MALRM_RESTART_TIMEOUT (%d).\n", ret );

   /* Initialize the timer active flag to inactive. */
   Restart_timeout_active = 0;

   /* Register the disconnect retry periodic. */
   if( (ret = ALARM_register( MALRM_DISCONNECTION_RETRY, 
                              Timer_callback )) < 0 )
      LE_send_msg( GL_MALRM(ret), 
                   "Unable To Register MALRM_DISCONNECTION_RETRY (%d)\n", ret );

   CR_disconnection_retries = 0;
   Disconnection_retry_active = 0;

   /* Register the RDA status timer. */
   if( (ret = ALARM_register( MALRM_RDA_STATUS, 
                              Timer_callback )) < 0 )
      LE_send_msg( GL_MALRM(ret), 
                   "Unable To Register MALRM_RDA_STATUS (%d).\n", ret );

   Rda_status_active = 0;

   /* Register the loopback periodic timer. */
   if( (ret = ALARM_register( MALRM_LOOPBACK_PERIODIC, 
                              Timer_callback )) < 0 )
      LE_send_msg( GL_MALRM(ret), 
                   "Unable To Register MALRM_LOOPBACK_PERIODIC (%d).\n", ret );
   
   Loopback_periodic_active = 0;

   /* Register the channel control timer. */
   if( (ret = ALARM_register( MALRM_CHAN_CTRL, 
                              Timer_callback )) < 0 )
      LE_send_msg( GL_MALRM(ret), 
                   "Unable To Register MALRM_CHAN_CTRL (%d).\n", ret );
   
   Chan_ctrl_active = 0;

/* End of TS_register_timers() */
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Generic timer expiration handler.
//
//   Input:
//      timer_id - ID of expired timer.
//
///////////////////////////////////////////////////////////////////////\*/
void Timer_callback( malrm_id_t timer_id ){

   if( timer_id == MALRM_RDA_COMM_DISC )
      Rda_comm_disc_expired = 1;

   else if( timer_id == MALRM_LB_TEST_TIMEOUT )
      LB_test_timeout_expired = 1;

   else if( timer_id == MALRM_CONNECTION_RETRY )
      Connection_retry_expired = 1;

   else if( timer_id == MALRM_DISCONNECTION_RETRY )
      Disconnection_retry_expired = 1;

   else if( timer_id == MALRM_RESTART_TIMEOUT )
      Restart_timeout_expired = 1;

   else if( timer_id == MALRM_RDA_STATUS )
      Rda_status_expired = 1;

   else if( timer_id == MALRM_LOOPBACK_PERIODIC )
      Loopback_periodic_expired = 1;

   else if( timer_id == MALRM_CHAN_CTRL )
      Chan_ctrl_expired = 1;

   CR_timer_expired = 1;

/* End of Timer_callback() */
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Service routine for timer handlers.
//
///////////////////////////////////////////////////////////////////////\*/
void TS_service_timer_expiration(){

   /* Each expiration flag must be checked ... Otherwise, the 
      service of an expired timer may happen unexpectedly at a
      future, unexpected time. */
   if( Rda_comm_disc_expired ){

      Rda_comm_disc_expired = 0;
      TS_handle_comm_discontinuity();

   }

   if( LB_test_timeout_expired ){

      LB_test_timeout_expired = 0;
      TS_handle_loopback_timeout();

   }

   if( Connection_retry_expired ){

      Connection_retry_expired = 0;
      TS_handle_connection_retry();

   }

   if( Disconnection_retry_expired ){

      Disconnection_retry_expired = 0;
      TS_handle_disconnection_retry();

   }

   if( Restart_timeout_expired ){

      Restart_timeout_expired = 0;
      TS_handle_restart_timeout();

   }

   if( Rda_status_expired ){

      Rda_status_expired = 0;
      TS_handle_RDA_status_timeout();

   }

   if( Loopback_periodic_expired ){

      Loopback_periodic_expired = 0;
      TS_handle_loopback_periodic();

   }

   if( Chan_ctrl_expired ){

      Chan_ctrl_expired = 0;
      TS_handle_channel_control();

   }

/* End of TS_service_timer_expiration() */
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Timer expiration handler for MALRM_RDA_COMM_DISC.
//
//      This functions performs the following actions:
//         1) If the number of radial messages processed is non-zero:
//            a)  update the last message processed time,
//            b)  reset the elapsed time (of no messages processed) 
//                and reset the message counter.
//         2) Checks if the status is OPERATE, the OPERABILITY_STATUS is 
//            OPERATIONAL, and all base data moments are enable.  If any 
//            condition not met, then:
//            a) the RDA communications discontinuity timer is cancelled,
//            b) the number of messages processed counter is initialized,
//            c) the last message process time is set to 0 and the module
//               exits.
//         3) The elapsed time since the last message processed is 
//            calculated.  If this value is greater than limit value, then:
//            a) reinitialize the last message processed time and the number
//               of messages processed.
//            b) the CR_communications_discontinuity flag is set. 
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_msg_processed - see crda_control_rda.h
//      CR_verbose_mode - see crda_control_rda.h
//      CR_communications_discontinuity - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
void TS_handle_comm_discontinuity()
{
   int			ret				= 0;
   static time_t	last_msg_processed_time		= 0;
   time_t		elapsed_time			= 0;

   /* Clear the active flag if timer is MALRM_ONESHOT_NTVL */
   if( Rda_comm_disc_active == (ALARM_SET|MALRM_ONESHOT_NTVL) )
      Rda_comm_disc_active = 0; 

   /* The communication discontinuity alarm was triggered.  If the 
      number of messages processed is non-zero, update the time the
      last message was processed. */
   if( CR_msg_processed != 0 ){

      last_msg_processed_time = MISC_systime( (int *) NULL ); 

      /* Initialize the number of RDA radial messages processed, and
         clear the elapsed time. */
      CR_msg_processed = 0;
      elapsed_time = 0;

   }
   else {

      /* Check if the status is OPERATE, the OPERATIOANL MODE is 
         OPERATIONAL, and all base data moments are enable.  If any 
         condition not met, cancel the RDA communications discontinuity 
         timer. */
      if( CR_operational_mode != OP_OPERATIONAL_MODE
                             ||
         (ret = ST_get_status(ORPGRDA_RDA_STATUS)) != RS_OPERATE
                             ||
         (ret=ST_get_status(ORPGRDA_DATA_TRANS_ENABLED))<=BD_ENABLED_NONE)
      {

         /* Cancel the communication discontinuity timer. */
         TS_cancel_timer( (int) 1, (malrm_id_t) MALRM_RDA_COMM_DISC );

         /* Reset the number of messages processed. */
         CR_msg_processed = 0;

         /* Re-initialize the time the last message was processed. */
         last_msg_processed_time = 0;

         /* Nothing left to do, so return. */
         return;

      }

      /* The number of messages processed is zero. Update the elapsed time 
         since the last message was received.  Ensure that the last msg 
         processed time is initialized.  This number must include the 
         number of seconds that have elapsed since the timer was set. */
      if( last_msg_processed_time == 0 )
         last_msg_processed_time = MISC_systime( (int *) NULL ) - RDA_COMM_DISC_VALUE;

      elapsed_time = MISC_systime( (int *) NULL ) - last_msg_processed_time;

   }

   /* If the elapsed time is greater than time-out value and the 
      communications discontinuity time is active, report error. */
   if( elapsed_time >= COMM_DISC_TIMEOUT_VALUE && Rda_comm_disc_active ){

      /* Re-initialize the last message processed time. */
      last_msg_processed_time = 0;

      /* Clear the number of radial messages processed. */
      CR_msg_processed = 0;

      /* Set the communications discontinuity condition active flag. */
      CR_communications_discontinuity = 1;

   }

/* End of TS_handle_comm_discontinuity() */
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Timer expiration handler for Loopback Test Timeout.
//
//      The number of loopback retries is incremented.  If the number
//      exceeds a limit, then the loopback timeout flag is set.  
//      Otherwise, another loopback test is attempted by virtue
//      of the perform loopback test flag being set.
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_loopback_retries - see crda_control_rda.h
//      CR_loopback_timeout - see crda_control_rda.h
//      CR_perform_loopback_test - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
void TS_handle_loopback_timeout(){

   /* Clear the active flag if timer is MALRM_ONESHOT_NTVL */
   if( LB_test_timeout_active == (ALARM_SET|MALRM_ONESHOT_NTVL) )
      LB_test_timeout_active = 0; 

   /* Increment the number of loopback retries. */
   CR_loopback_retries++;

   /* If the number of loopback retries exceeds limit, process loopback
      test timeout. */
   if( CR_loopback_retries >= LOOPBACK_RETRY_LIMIT ){

      /* Reinitialize the number of loopback retries. */
      CR_loopback_retries = 0;

      /* Set flag indicating loopback test timeout. */
      CR_loopback_timeout = 1;
      CR_return_to_previous_state = PREVIOUS_STATE_NO; 

   }
   else{

      /* Retry loopback again. */
      CR_perform_loopback_test = 1; 

   }

/* End of TS_handle_loopback_timeout() */
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Timer expiration handler for Connection Retry.  
//
//      If the number of connection retries is not over the limit, 
//      a connection request is posted to the line.  Otherwise,
//      set the connection retry exhausted flag.
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//      CR_connection_retries - see crda_control_rda.h
//      CR_connection_retries_exhausted - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
void TS_handle_connection_retry(){

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "MALRM_CONNECTION_RETRY Expired.\n" );
 
   /* Clear the active flag if timer is MALRM_ONESHOT_NTVL */
   if( Connection_retry_active == (ALARM_SET|MALRM_ONESHOT_NTVL) )
      Connection_retry_active = 0; 

   /* If the connection retries have not been exhausted and the wideband
      line still is not connected, try again.... */
   if( CR_connection_retries < CR_connection_retry_limit ){

      /* Issue wideband connection control command. */
      LC_connect_wb_line( );
      CR_connection_retries++;

   }

   /* If RDA is restarting, do exhaust the connection retries since the
      restart may take awhile.  The restart timeout will take care of
      any wideband alarms. */
   else if( !CR_rda_restarting ){

      /* Connection retry limit exceeded.  Wideband line has failed. */
      LE_send_msg( GL_INFO, 
                   "Wideband Connect Failed Because Retry Limit Meet.\n" );

      /* Set flag indicating that the connection retry was exhausted. */
      CR_connection_retries_exhausted = 1;

   }
   
   return;

/* End of TS_handle_connection_retry() */
} 

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Timer expiration handler for Disconnection Retry.  
//
//      If the number of disconnection retries is not over the limit, 
//      the number of disconnection retries is incremented.  Otherwise,
//      set the disconnection retry exhausted flag.
//
//      If the timeout occurs during shutdown processing, regardless
//      of whether the retries were exhausted, then set the disconnection
//      retry exhausted flag. 
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//      CR_disconnection_retries - see crda_control_rda.h
//      CR_disconnection_retries_exhausted - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
void TS_handle_disconnection_retry(){

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "MALRM_DISCONNECTION_RETRY Expired.\n" );
 
   /* Clear the active flag if timer is MALRM_ONESHOT_NTVL */
   if( Disconnection_retry_active == (ALARM_SET|MALRM_ONESHOT_NTVL) )
      Disconnection_retry_active = 0;

   /* If the disconnection retries have not been exhausted, increment the 
      disconnection retries. */
   if( CR_disconnection_retries < CR_disconnection_retry_limit )
      CR_disconnection_retries++;

   else {

      /* Connection retry limit exceeded.  Wideband line has failed. */
      LE_send_msg( GL_INFO, 
                   "Wideband Disconnect Failed Because Retry Limit Meet.\n" );

      /* Set flag indicating that the connection retry was exhausted. */
      CR_disconnection_retries_exhausted = 1;
      
   }

   /* If we are shutting down, then set the retries exhausted flag. */
   if( CR_shut_down_state == SHUT_DOWN_DISCONNECT_WAIT )
      CR_disconnection_retries_exhausted = 1;
   
   return;

/* End of TS_handle_disconnection_retry() */
}
 
/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Timer expiration handler for RDA restart timeout.
//
//      The RDA display blanking is set to RS_OPERABILITY_STATUS and
//      the RDA alarm RDA_LINK_BROKEN_ALARM is actived.
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_verbose_mode - see crda_control_rda.h
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
///////////////////////////////////////////////////////////////////////\*/
void TS_handle_restart_timeout(){

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "RDA Restart Timeout.\n" );

   /* Clear the active flag if timer is MALRM_ONESHOT_NTVL */
   if( Restart_timeout_active == (ALARM_SET|MALRM_ONESHOT_NTVL) ){

      LE_send_msg( GL_INFO, "Restart_timeout_active Flag Cleared\n" );
      Restart_timeout_active = 0; 

   }

   /* Did this alarm go off and Wideband Indicates CONNECTED? */
   if( (SCS_get_wb_status( ORPGRDA_WBLNSTAT )) == RS_CONNECTED )
      LE_send_msg( GL_INFO, "!!! Wideband Line Status: CONNECTED ?????\n" );

   /* Apply RDA status display blanking to all fields except the 
      OPERABILITY STATUS and the ALARM SUMMARY.  Post RDA alarm. */
   SCS_handle_wideband_alarm( (int) RS_WBFAILURE, (int) RDA_LINK_BROKEN_ALARM,
                              (int) RS_OPERABILITY_STATUS );

}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Timer expiration handler for RDA status.
//
//      If the status latch is not set, the RDA control status is not
//      local control or the control authorization is remote control
//      enabled, then the RDA is returned to its previous state.  
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_status_latch - see crda_control_rda.h
//      CR_status_protect - see crda_control_rda.h
//      CR_control_status - see crda_control_rda.h
//      CR_control_authority - see crda_control_rda.h
//      CR_verbose_mode - see crda_control_rda.h
//
//   Notes:
//      It is assumed once this timer expires after a wideband connection,
//      RDA status data has been received.
//
///////////////////////////////////////////////////////////////////////\*/
void TS_handle_RDA_status_timeout(){

   /* Clear the active flag if timer is MALRM_ONESHOT_NTVL */
   if( Rda_status_active == (ALARM_SET|MALRM_ONESHOT_NTVL) )
      Rda_status_active = 0; 

   /* 
     If Control RDA is in the process of shutting down and this
     timer expires, set disconnect wideband flag and return.
   */
   if( CR_shut_down_state != SHUT_DOWN_NO ){

      LE_send_msg( GL_INFO, "RDA Status Timed-out\n" );
      CR_shut_down_state = SHUT_DOWN_DISCONNECT;
      LE_send_msg( GL_INFO, "Control RDA in SHUT_DOWN_DISCONNECT State\n" );

      /* Clear the timer active flag. */
      Rda_status_active = 0;
      return;

   }
    
   /*
     If the RDA status latch is open, then we need to return to the 
     previous RDA state if the RDA control is not local.
   */
   if( (!CR_status_latch) 
              ||
       (CR_return_to_previous_state == PREVIOUS_STATE_STATUS_WAIT) ){
                
      if( CR_control_status != CS_LOCAL_ONLY ){

         LE_send_msg( GL_INFO, "Return To Previous RDA State\n" );
         CR_return_to_previous_state = PREVIOUS_STATE_PHASE_1;

      }

      /* If returning to previous state and the RDA is in local control 
         then skip the return previous state. */ 
      if( (CR_return_to_previous_state == PREVIOUS_STATE_STATUS_WAIT)
                             &&
          (CR_control_status == CS_LOCAL_ONLY) ){

         LE_send_msg( GL_INFO, 
                      "Skip Return To Previous RDA State.  RDA In Local Control\n" );    
         CR_return_to_previous_state = PREVIOUS_STATE_NO;
         CR_rpg_restarting = -1;

      }

      /*
        Close the status latch.
      */
      CR_status_latch = 1;

      /*
        Clear the status protect flag if not returning to previous 
        RDA state.
      */
      if( CR_return_to_previous_state == PREVIOUS_STATE_NO )
         CR_status_protect = 0;

   }
      
}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Timer expiration handler for channel control (FAA redundant sites
//      only)
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//     CR_return_to_previous_state - see crda_control_rda.h.
//
//   Notes:
//
///////////////////////////////////////////////////////////////////////\*/
void TS_handle_channel_control(){

   /* Clear the active flag if timer is MALRM_ONESHOT_NTVL */
   if( Chan_ctrl_active == (ALARM_SET|MALRM_ONESHOT_NTVL) )
      Chan_ctrl_active = 0; 

   /*
     Time to start phase 2 of returning to previous state.
   */
   CR_return_to_previous_state = PREVIOUS_STATE_PHASE_2;

}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Timer expiration handler for Loopback Periodic test.
//
//      When called, set flag to indicate a loopback test is to be performed.
//
//   Inputs:
//
//   Outputs:
//
//   Globals:
//      CR_perform_loopback_test - see crda_control_rda.h
//
//   Notes:
//
///////////////////////////////////////////////////////////////////////\*/
void TS_handle_loopback_periodic(){

   /* Clear the active flag if timer is MALRM_ONESHOT_NTVL */
   if( Loopback_periodic_active == (ALARM_SET|MALRM_ONESHOT_NTVL) )
      Loopback_periodic_active = 0; 

   /* 
     Set the flag to indicate a loopback test is to be performed.
   */
   CR_perform_loopback_test = 1;

}

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Cancels the specified timer(s).
//
//   Inputs:  
//      number_timers - The number of timers to cancel.
//      ...  - Variable argument list.  Contains the timer IDs of timers
//             to cancel. 
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Globals:
//      CR_msg_processed - see crda_control_rda.h
//      Rda_comm_disc_active - see Notes:
//      LB_test_timeout_active - see Notes:
//      Connection_retry_active - see Notes:
//      Restart_timeout_active - see Notes:
//      Disconnect_retry_active - see Notes:
//      Rda_status_active - see Notes:
//
//   Notes:
//      Definitions for file scope global variables can be found at the
//      beginning of this file.
//
//      See MALRM_cancel man page for details of this function.
//
///////////////////////////////////////////////////////////////////////\*/
void TS_cancel_timer( int number_timers, ... ){

   va_list arg_ptr;
   malrm_id_t id;
   int ret, i;

   /*
     Initialize the variable argument list.
   */
   va_start( arg_ptr, number_timers );

   if( arg_ptr == (va_list) NULL )
      LE_send_msg( GL_INFO, "Can Not Cancel Timer(s)\n" );

   else if( number_timers > 0 ){

      for( i = 0; i < number_timers; i++ ){

         /*
           Get next argument.
         */
         id = va_arg( arg_ptr, malrm_id_t );

         switch( id ){

            case MALRM_RESTART_TIMEOUT:
            {
               if( Restart_timeout_active ){

                  Restart_timeout_active = 0;
                  ret = ALARM_cancel( (malrm_id_t) MALRM_RESTART_TIMEOUT, (unsigned int *) NULL );

                  if( (ret == MALRM_ALARM_NOT_SET) && (Restart_timeout_expired) )
                     Restart_timeout_expired = 0;

               }

               break;
            } 

            case MALRM_LB_TEST_TIMEOUT:
            {
               if( LB_test_timeout_active ){

                  LB_test_timeout_active = 0;
                  ret = ALARM_cancel( (malrm_id_t) MALRM_LB_TEST_TIMEOUT, (unsigned int *) NULL );

                  if( (ret == MALRM_ALARM_NOT_SET) && (LB_test_timeout_expired) )
                     LB_test_timeout_expired = 0;

               }

               break;
            } 

            case MALRM_CONNECTION_RETRY:
            {
               if( Connection_retry_active ){

                  Connection_retry_active = 0;
                  ret = ALARM_cancel( (malrm_id_t) MALRM_CONNECTION_RETRY, (unsigned int *) NULL );

                  if( (ret == MALRM_ALARM_NOT_SET) && (Connection_retry_expired) )
                     Connection_retry_expired = 0;

                  CR_connection_retries = 0;
                  CR_connection_retries_exhausted = 0;

               }

               break;
            } 

            case MALRM_RDA_COMM_DISC:
            {
               if( Rda_comm_disc_active ){

                  Rda_comm_disc_active = 0;
                  ret = ALARM_cancel( (malrm_id_t) MALRM_RDA_COMM_DISC, (unsigned int *) NULL );

                  if( (ret == MALRM_ALARM_NOT_SET) && (Rda_comm_disc_expired) )
                     Rda_comm_disc_expired = 0;

                  CR_msg_processed = 0;

               }

               break;
            }

            case MALRM_DISCONNECTION_RETRY:
            {
               if( Disconnection_retry_active ){

                  Disconnection_retry_active = 0;
                  ret = ALARM_cancel( (malrm_id_t) MALRM_DISCONNECTION_RETRY, (unsigned int *) NULL );

                  if( (ret == MALRM_ALARM_NOT_SET) && (Disconnection_retry_expired) )
                     Disconnection_retry_expired = 0;

                  CR_disconnection_retries = 0;
                  CR_disconnection_retries_exhausted = 0;

               }

               break;
            }

            case MALRM_RDA_STATUS:
            {
               if( Rda_status_active ){

                  Rda_status_active = 0;
                  ret = ALARM_cancel( (malrm_id_t) MALRM_RDA_STATUS, (unsigned int *) NULL );

                  if( (ret == MALRM_ALARM_NOT_SET) && (Rda_status_expired) )
                     Rda_status_expired = 0;
               }

               break;
            }

            case MALRM_LOOPBACK_PERIODIC:
            {
               if( Loopback_periodic_active ){

                  Loopback_periodic_active = 0;
                  ret = ALARM_cancel( (malrm_id_t) MALRM_LOOPBACK_PERIODIC, (unsigned int *) NULL );

                  if( (ret == MALRM_ALARM_NOT_SET) && (Loopback_periodic_expired) )
                     Loopback_periodic_expired = 0;

               }

               break;
            }

            case MALRM_CHAN_CTRL:
            {
               if( Chan_ctrl_active ){

                  Chan_ctrl_active = 0;
                  ret = ALARM_cancel( (malrm_id_t) MALRM_CHAN_CTRL, (unsigned int *) NULL );

                  if( (ret == MALRM_ALARM_NOT_SET) && (Chan_ctrl_expired) )
                     Chan_ctrl_expired = 0;

               }

               break;
            }

            default:
               LE_send_msg( GL_CODE, "Cancelling Unknown Timer (%d).\n", id );

         }
 
      }

   }
   else if( number_timers < 0 ){

      /*
        Cancel all timers.
      */
      if( Restart_timeout_active ){

         Restart_timeout_active = 0;
         Restart_timeout_expired = 0;
         ALARM_cancel( (malrm_id_t) MALRM_RESTART_TIMEOUT, (unsigned int *) NULL );

      } 

      if( LB_test_timeout_active ){

         LB_test_timeout_active = 0;
         LB_test_timeout_expired = 0;
         ALARM_cancel( (malrm_id_t) MALRM_LB_TEST_TIMEOUT, (unsigned int *) NULL );

      } 

      if( Connection_retry_active ){

         Connection_retry_active = 0;
         Connection_retry_expired = 0;
         CR_connection_retries = 0;
         CR_connection_retries_exhausted = 0;
         ALARM_cancel( (malrm_id_t) MALRM_CONNECTION_RETRY, (unsigned int *) NULL );

      } 

      if( Rda_comm_disc_active ){

         Rda_comm_disc_active = 0;
         Rda_comm_disc_expired = 0;
         CR_msg_processed = 0;
         ALARM_cancel( (malrm_id_t) MALRM_RDA_COMM_DISC, (unsigned int *) NULL );

      }

      if( Disconnection_retry_active ){

         Disconnection_retry_active = 0;
         Disconnection_retry_expired = 0;
         CR_disconnection_retries = 0;
         CR_disconnection_retries_exhausted = 0;
         ALARM_cancel( (malrm_id_t) MALRM_DISCONNECTION_RETRY, (unsigned int *) NULL );

      }

      if( Rda_status_active ){

         Rda_status_active = 0;
         Rda_status_expired = 0;
         ALARM_cancel( (malrm_id_t) MALRM_RDA_STATUS, (unsigned int *) NULL );

      }

      if( Loopback_periodic_active ){

         Loopback_periodic_active = 0;
         Loopback_periodic_expired = 0;
         ALARM_cancel( (malrm_id_t) MALRM_LOOPBACK_PERIODIC, (unsigned int *) NULL );

      }

      if( Chan_ctrl_active ){

         Chan_ctrl_active = 0;
         Chan_ctrl_expired = 0;
         ALARM_cancel( (malrm_id_t) MALRM_CHAN_CTRL, (unsigned int *) NULL );

      }

   } 

   /*
     End variable argument list.
   */
   va_end( arg_ptr );

}
 
/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Sets the specified timer.
//
//   Inputs:  
//      id - The timer id.
//      time_in_future - Seconds from current time when timer is to 
//                       expire.
//      interval - The timer periodic rate, in seconds. 
//
//   Outputs:
//
//   Returns:
//      Status of setting the timer.  Negative values denote
//      unsuccessful attempt at setting timer.
//
//   Globals:
//      CR_msg_processed - see crda_control_rda.h
//      Rda_comm_disc_active - see Notes:
//      LB_test_timeout_active - see Notes:
//      Connection_retry_active - see Notes:
//      Restart_timeout_active - see Notes:
//      Disconnect_retry_active - see Notes:
//      Rda_status_active - see Notes:
//      Chan_ctrl_active - see Notes:
//
//   Notes:
//      Definitions for file scope global variables can be found at the
//      beginning of this file.
//
//      See MALRM_set man page for details of this function.
//
///////////////////////////////////////////////////////////////////////\*/
int TS_set_timer( malrm_id_t id, time_t time_in_future, 
                  unsigned int interval ){

   int ret;
   time_t start_time;

   /*
     Set the specified timer.
   */
   if( time_in_future != MALRM_START_TIME_NOW )
      start_time = MISC_systime( (int *) NULL ) + time_in_future;

   else
      start_time = MALRM_START_TIME_NOW;

   ret = ALARM_set( id, start_time, interval );

   /*
     Timer specific processing.
   */
   switch( id ){

      case MALRM_RESTART_TIMEOUT:
      {
         LE_send_msg( GL_INFO, "MALRM_RESTART_TIMEOUT Alarm Being Set .....\n" );
         if( ret == 0 ){

            LE_send_msg( GL_INFO, "MALRM_RESTART_TIMEOUT Alarm Acive Flag Set to %u\n",
                         ALARM_SET|interval );
            Restart_timeout_active = ALARM_SET|interval;

         }
         else{

            LE_send_msg( GL_INFO, "Restart_timeout_active Flag Cleared\n" );
            Restart_timeout_active = 0;

         }

         break;
      } 

      case MALRM_LB_TEST_TIMEOUT:
      {
         if( ret == 0 )
            LB_test_timeout_active = ALARM_SET|interval;

         else
            LB_test_timeout_active = 0;

         break;
      } 

      case MALRM_CONNECTION_RETRY:
      {
         if( ret == 0 ){

            CR_connection_retries = 0;
            CR_connection_retries_exhausted = 0;
            Connection_retry_active = ALARM_SET|interval;

         }
         else
            Connection_retry_active = 0;

         break;
      } 

      case MALRM_RDA_COMM_DISC:
      {
         if( ret == 0 )
            Rda_comm_disc_active = ALARM_SET|interval;

         else
            Rda_comm_disc_active = 0;

         CR_msg_processed = 0;

         break;
      }

      case MALRM_DISCONNECTION_RETRY:
      {
         if( ret == 0 )
            Disconnection_retry_active = ALARM_SET|interval;

         else
            Disconnection_retry_active = 0;

         break;
      }

      case MALRM_RDA_STATUS:
      {
         if( ret == 0 )
            Rda_status_active = ALARM_SET|interval;

         else
            Rda_status_active = 0;

         break;
        
      }

      case MALRM_LOOPBACK_PERIODIC:
      {
         if( ret == 0 )
            Loopback_periodic_active = ALARM_SET|interval;

         else
            Loopback_periodic_active = 0;

         break;
        
      }

      case MALRM_CHAN_CTRL:
      {
         if( ret == 0 )
            Chan_ctrl_active = ALARM_SET|interval;

         else
            Chan_ctrl_active = 0;

         break;
        
      }

      default:
      {
         LE_send_msg( GL_CODE, 
                      "Attempting To Set Unrecognized Timer (ID %d)\n", id );
         return( TS_FAILURE );
      }

   } 

   /*
     Determine success or failure of timer activation.
   */
   if( ret < 0 ){

      LE_send_msg( GL_MALRM(ret), 
                   "Unsucessful Attempt At Setting Timer (ID %d)\n", id );
      return( TS_FAILURE );

   }

   return( TS_SUCCESS );

} 

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Returns 0 if the specified timer is not active.
//      Returns non-zero value if the specified timer is active.
//      This value is the interval defined when the timer was activated.
//
//   Inputs:  
//      id - The timer id.
//
//   Outputs:
//
//   Returns:
//      See desciption. Also returns TS_FAILURE if unrecognized timer.
//
//   Globals:
//      CR_comm_disc_timer_active - see crda_control_rda.h
//      Rda_comm_disc_active - see Notes:
//      LB_test_timeout_active - see Notes:
//      Connection_retry_active - see Notes:
//      Restart_timeout_active - see Notes:
//      Disconnect_retry_active - see Notes:
//      Rda_status_active - see Notes:
//      Chan_ctrl_active - see Notes:
//
//   Notes:
//      Definitions for file scope global variables can be found at the
//      beginning of this file.
//
//      See MALRM_set man page for details of this function.
//
///////////////////////////////////////////////////////////////////////\*/
unsigned int TS_is_timer_active( malrm_id_t id ){

   /*
     Timer specific processing.
   */
   switch( id ){

      case MALRM_RESTART_TIMEOUT:
      {
         return (Restart_timeout_active);
      } 

      case MALRM_LB_TEST_TIMEOUT:
      {
         return (LB_test_timeout_active);
      } 

      case MALRM_CONNECTION_RETRY:
      {
         return (Connection_retry_active);
      } 

      case MALRM_RDA_COMM_DISC:
      {
         return (Rda_comm_disc_active);
      }

      case MALRM_DISCONNECTION_RETRY:
      {
         return (Disconnection_retry_active);
      } 
        
      case MALRM_RDA_STATUS:
      {
         return (Rda_status_active);
      }
        
      case MALRM_CHAN_CTRL:
      {
         return (Chan_ctrl_active);
      }

      default:
      {
         LE_send_msg( GL_CODE, 
                      "Attempting To Poll Unrecognized Timer (ID %d)\n", id );
         return (TS_FAILURE);

      }

   } 

} 
