/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:47:43 $
 * $Id: crda_set_comms_status.c,v 1.74 2014/03/13 19:47:43 steves Exp $
 * $Revision: 1.74 $
 * $State: Exp $
 */

#include <crda_control_rda.h>
#include <orpgrat.h>
#include <rdacnt.h>
#include <orpgsite.h>
#include <mrpg.h>
#include <orpgrda.h>
#include <orpgadpt.h>


/* Flag, if set, indicates the RDA_LINK_BROKEN_ALARM is set. */
static int Link_broken_alarm_set = 0;

/* Flag, if set, indicates the RDA_COMM_DISC_ALARM is set. */
static int Rpg_communications_alarm_set = 0;


/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//      Initialization for the SCS module.  Reads the RDA status data
//      from the GSM LB.  If the read fails, sets the wideband line
//      status to DISCONNECTED_CM and sets display blanking to 
//      operability status only.
//
//      Clears the Communications Link failure alarm flags.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      Currently returns SCS_SUCCESS
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
int SCS_init(){

   int			ret		= 0;
   int			new_vcp_num	= 0;
   double 		value1		= 0.0;
   char			*buf;
   unsigned char        value		= 0;


   /* Read the latest status msg */
   ret = ST_init();
   if( ret <= 0 ){

      /* Send status log message */
      LE_send_msg( GL_ERROR, "Problem reading RDA Status Msg from GSM LB.");

      /* Set wideband line status to disconnected by Comm Manager and set
         rda display blanking to only allow operability status display. */
      SCS_update_wb_line_status( (int) RS_DISCONNECTED_CM, /* wideband line status */
                                 (int) RS_OPERABILITY_STATUS, /* display blanking */
                                 (int) 0 /* Do not post wideband
                                            line status changed event */ );
   }

   /* Clear communications alarm flags. */
   Link_broken_alarm_set = 0;
   Rpg_communications_alarm_set = 0;

   /* Check the state data to see if the wideband alarm is still
      active.  This might happen if the wideband alarm was active
      and the RPG was restarted. If alarm set, clear it now. */
   if( ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_WBFAILRE,
                                   ORPGINFO_STATEFL_GET, &value ) >= 0 ){

      if( value )
         ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_WBFAILRE,
                                     ORPGINFO_STATEFL_CLR, &value );
   }
 
   if( CR_rpg_restarting == MRPG_STARTUP ){

      /* Use the library to read the previous state values.  This will use the
         default previous state values in leiu of real data. */
      ret = ORPGRDA_read_previous_state();
      if ( ret != ORPGRDA_SUCCESS )
         LE_send_msg(GL_ERROR, "control_rda: Could not read default previous state values.\n");

      /* Save the system default VCP in case we can't get the site data */
      new_vcp_num = ORPGRDA_get_previous_state( ORPGRDA_VCP_NUMBER );

      /* The default weather mode is defined in the site adaptation data
         group.  It can be either Precipitation or Clear Air. */
      if( DEAU_get_string_values( "site_info.wx_mode", &buf ) > 0 ){

         if( !strncmp( buf, "Precipitation", 13 ) ){

            if( DEAU_get_values( "site_info.def_mode_A_vcp", &value1, 1 ) > 0 )
               new_vcp_num = (int) value1;

            else
               LE_send_msg( GL_ERROR, "DEAU_get_values( site_info.def_mode_A_vcp ) Failed\n" );
         }
         else{

            if( DEAU_get_values( "site_info.def_mode_B_vcp", &value1, 1 ) > 0 )
               new_vcp_num = (int) value1;
            else
               LE_send_msg( GL_ERROR, "DEAU_get_values( site_info.def_mode_B_vcp ) Failed\n" );

         }

         /* Change the previous state VCP to the default VCP */
         ret = ORPGRDA_set_state( ORPGRDA_VCP_NUMBER, new_vcp_num );
         if ( ret != ORPGRDA_SUCCESS )
            LE_send_msg( GL_ERROR, "control_rda: Could not set state for ORPGRDA_VCP_NUMBER.\n");

      }
      else
         LE_send_msg( GL_ERROR, "DEAU_get_values( site_info.wx_mode ) Failed\n" );

   }

   /* Return success. */
   return( SCS_SUCCESS );

} /* End of SCS_init() */


/*\/////////////////////////////////////////////////////////////////////
//
//   Description:
//      Handle event type CM_EVENT from the comm manager.
//
//      The event types handled are:
//
//         CM_LOST_CONN
//         CM_START
//         CM_TERMINATE
//
//   Inputs:
//      event_type - event type associated with CM_EVENT.
//
//   Outputs:
//
//   Returns:
//      There are no return values associated with this module.
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
//////////////////////////////////////////////////////////////////////\*/
void SCS_handle_comm_manager_event( int event_type ){

   switch( event_type ){

      case CM_LOST_CONN:
      {
         if( CR_verbose_mode )
            LE_send_msg( GL_COMMS, 
                         "CM_LOST_CONN: Connection Lost By Remote Action.\n" );

         /* Process unexpected line drop. */
         LC_process_unexpected_line_disconnection( (int) CM_LOST_CONN );

         break;
      }

      case CM_START:
      {
         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "CM_START: Comm Manager Is Starting.\n" );

         /* If CM_START received without a CM_TERMINATE and control rda is not 
            restarting, set the reconnect wideband line after failure flag. */
         if( !CR_control_rda_restarting )
            LC_set_reconnect_after_failure_flag();

         /* Cancel all timers which are active. */
         TS_cancel_timer( (int) -1 );

         /* Post a line connection command if wideband failure is active. */
         LC_reconnect_line_after_failure( );

         break;
      }

      case CM_TERMINATE:
      {
         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "CM_TERMINATE: Comm Manager Is Terminating.\n" );

         /* Process unexpected wideband disconnection. */
         LC_process_unexpected_line_disconnection( (int) CM_TERMINATE );

         break;
      }
      
      default:
      {
         if( CR_verbose_mode )
            LE_send_msg( GL_INFO, "CM_EVENT (%d) Not Supported or Ignored.\n", 
                         event_type );
         break;
      }

   } /* End of "switch" */

} /* End of SCS_handle_comm_manager_event() */


/*\////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Service routine for wideband alarms generated by ORPG.
//
//      This module performs the following actions:
//         1) The timers MALRM_LB_TEST_TIMEOUT, MALRM_LOOPBACK_PERIODIC,
//            and MALRM_RDA_COMM_DISC are cancelled.
//
//         2) If the wideband line status has changed or the RDA display
//            blanking has changed, the wideband line status is updated,
//            and written to the status linear buffer via a call to 
//            SCS_update_wb_line_status.
//
//         3) The appropriate alarm code and summary value is set in RDA 
//            status.  The operability status is set to INOPERABLE.  The 
//            RDA status linear buffer is then updated.
//           
//         4) The RPG status is updated to activated the Wideband Failure
//            Maintenance Mandatory alarm, and a message is written to
//            the system status log.
//
//   Inputs:
//      wideband_line_status - Current wideband line status.
//      alarm_code - alarm code associated with wideband alarm.
//      display_blanking - determines which parts of status are not 
//                         reported.
//
//   Outputs:
//
//   Returns:
//      There are no return values associated with the module.
//
/////////////////////////////////////////////////////////////////////////\*/
void SCS_handle_wideband_alarm( int wideband_line_status, int alarm_code,
                                int display_blanking ){

   int			curr_wb_line_stat	= 0;
   int			rda_chan_num		= 0;
   int 			rda_config		= ST_get_rda_config( NULL);
   static int		old_display_blanking	= 0;
   unsigned char	value			= 0;
   RDA_alarm_entry_t*	alarm_data		= 0;


   /* Cancel the loopback test timeout timer, the RDA->RPG discontinuity 
      timer, and the periodic loopback timer, if active. */
   TS_cancel_timer( (int) 3, (malrm_id_t) MALRM_LB_TEST_TIMEOUT,
                             (malrm_id_t) MALRM_RDA_COMM_DISC,
                             (malrm_id_t) MALRM_LOOPBACK_PERIODIC );
 
   /* Get wb line status */  
   curr_wb_line_stat = SCS_get_wb_status( ORPGRDA_WBLNSTAT );

   /* Update the wideband line status only if it has changed or the rda
      display blanking has changed, and write wideband status linear buffer. */
   if( (curr_wb_line_stat != wideband_line_status) 
                          ||
       ((display_blanking != old_display_blanking) 
                          &&
        (display_blanking >= 0 ))){

      SCS_update_wb_line_status( wideband_line_status, 
                                 (int) display_blanking,
                                 (int) 1 /* post wideband line
                                            status changed event */ );

      /* Sleep 1 seconds.  This is to allow all interested parties to 
         service the line status change prior to posting an RDA alarm. */
      sleep(1);

   }

   /* Alarm already set or invalid alarm code, so ignore. */
   if ( (alarm_code == RDA_LINK_BROKEN_ALARM && Link_broken_alarm_set) 
                                    || 
        (alarm_code == RDA_COMM_DISC_ALARM && Rpg_communications_alarm_set))
      return;

   /* Clear all RDA alarm codes and alarm categories. */
   ST_clear_rda_alarm_codes();

   /* Set the communications alarm associated with alarm_code. */
   SCS_set_communications_alarm( alarm_code );

   /* Update RDA status linear buffer. */
   ST_update_rda_status( (int) 1, /* post RDA status changed event */
                         NULL,    
                         (int) RPG_INITIATED );

   if ( alarm_code <= RDA_ALARM_NONE )
      return;

   /* Update the system status log with new RDA status. */
   ST_update_system_status( );

   /* Set RPG alarm for wideband failure. */
   ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_WBFAILRE,
                               ORPGINFO_STATEFL_SET, &value );

   /* Report communications alarm. */
   alarm_data = (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data( (int) alarm_code );

   /* Retrieve the RDA channel number */
   if( rda_config == ORPGRDA_ORDA_CONFIG )
      rda_chan_num = CR_ORDA_status.status_msg.msg_hdr.rda_channel & 0x03;

   else
      rda_chan_num = CR_RDA_status.status_msg.msg_hdr.rda_channel & 0x03;

   if ( rda_chan_num == ORPGRDA_DATA_NOT_FOUND )
      LE_send_msg(GL_ERROR,
         "SCS_handle_wideband_alarm: Could not retrieve channel number!\n");

   /* If redundant, include channel number in message. */
   if ( rda_chan_num )
      LE_send_msg( GL_STATUS | GL_ERROR | LE_RPG_AL_MAM, "%s [RDA:%d] %s\n", 
         ORPGINFO_RPG_ALARM_ACTIVATED, rda_chan_num, alarm_data->alarm_text );
   else
      LE_send_msg( GL_STATUS | GL_ERROR | LE_RPG_AL_MAM, 
         "%s %s\n", ORPGINFO_RPG_ALARM_ACTIVATED, alarm_data->alarm_text );

   return;
        
} /* End of SCS_handle_wideband_alarm() */



/*\//////////////////////////////////////////////////////////////////
//
//   Description:  
//      Writes to the Wideband Line status Linear Buffer, and posts 
//      event ORPGEVT_RDA_RPG_COMMS_STATUS if LB write was successful 
//      and the post status change flag is set.
//
//   Inputs: 
//      wideband_line_status - new wideband line status.
//      post_status_change_event - flag indicating whether to post
//                                 wideband line status changed 
//                                 event.
//
//   Outputs:
//
//   Returns:  
//      If value negative, failure occurred.   Failure is either 
//      the Linear Buffer Write failed or the Event Notification
//      failed.  Otherwise, returns 0.
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
/////////////////////////////////////////////////////////////////\*/
int SCS_update_wb_line_status( int wideband_line_status, 
                               int display_blanking,
                               int post_status_change_event ){

   int		ret		= 0;
   int		wb_line_stat	= 0;
   int		rda_config	= ORPGRDA_ORDA_CONFIG;

   /* Get wideband line status value from ORPGRDA lib */
   wb_line_stat = SCS_get_wb_status( ORPGRDA_WBLNSTAT );

   /* Write line status to status log. */
   if ( wb_line_stat != wideband_line_status ){

      switch( wideband_line_status ){

         case RS_CONNECT_PENDING:

            if ( !Link_broken_alarm_set )
               LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "Wideband Line has CONNECT PENDING\n" );

            break;

         case RS_CONNECTED:
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "Wideband Line is CONNECTED\n" );
            break;

         case RS_DISCONNECT_PENDING:
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "Wideband Line has DISCONNECT PENDING\n" );
            break;

         case RS_DISCONNECTED_HCI:
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "Wideband Line DISCONNECTED by HCI\n" );
            break;

         case RS_DISCONNECTED_CM:
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "Wideband Line Experienced Unsolicated DISCONNECT\n" );
            break;

         case RS_DISCONNECTED_SHUTDOWN:
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "Wideband Line DISCONNECTED by RPG SHUTDOWN\n" );
            break;

         case RS_DISCONNECTED_RMS:
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "Wideband Line DISCONNECTED by RMS\n" );
            break;

         case RS_WBFAILURE:
            
            if ( !Link_broken_alarm_set )
               LE_send_msg( GL_STATUS | GL_ERROR, "Wideband Line has FAILED\n" );

            break;

         case RS_DOWN:
            LE_send_msg( GL_STATUS, "Wideband Line is DOWN\n" );
            break;

         default:
            LE_send_msg( GL_INFO, "Unknown Wideband Line Status (%d)\n",
                         wideband_line_status );
            break;

      } /* End of "switch" */
   }

   /* Update the wideband line status */
   if( (rda_config = ST_get_rda_config( NULL )) == ORPGRDA_ORDA_CONFIG ){

      CR_ORDA_status.wb_comms.wblnstat = wideband_line_status;

      /* Update the display blanking value. A display blanking value less than 0
         indicates no change. */
      if ( display_blanking >= 0 )
         CR_ORDA_status.wb_comms.rda_display_blanking = display_blanking;

   }
   else{

      CR_RDA_status.wb_comms.wblnstat = wideband_line_status;

      /* Update the display blanking value. A display blanking value less than 0
         indicates no change. */
      if ( display_blanking >= 0 )
         CR_RDA_status.wb_comms.rda_display_blanking = display_blanking;

   }

   if ( CR_verbose_mode )
      SCS_display_comms_line_status( );

   /* Write new status data to status linear buffer. */
   if ( (ret = ST_write_status_msg()) < 0 ){

      /* Report error and return. */
       LE_send_msg( GL_ERROR, "ORPGRDA GSM Write Failed\n");

       return( SCS_FAILURE );

   }
   else{

      if( post_status_change_event ){

         if( (ret = ES_post_event( ORPGEVT_RDA_RPG_COMMS_STATUS, 
                                   (char *) NULL, (size_t) 0, 
                                   (int) 0  )) != ES_SUCCESS ){

            LE_send_msg( GL_EN(ret), "Posting Event ORPGEVT_RDA_RPG_COMMS_STATUS Failed\n" );
            return ( SCS_FAILURE );

         }

      }

   }

   /* Return normal. */
   return ( SCS_SUCCESS );

} /* SCS_update_wb_line_status() */


/*\//////////////////////////////////////////////////////////////////
//
//   Description:
//      Sets the Link_broken_alarm_set flag or the 
//      Rpg_communications_alarm_set flag depending on the value of
//      alarm_code.
//
//   Inputs:
//      alarm_code - The alarm code.  Should be either:
//                   RDA_LINK_BROKEN_ALARM
//                   RDA_COMM_DISC_ALARM
//
//   Outputs:
//
//   Returns:
//      There are no return values currently defined.
//
//////////////////////////////////////////////////////////////////\*/
void SCS_set_communications_alarm( int alarm_code ){

   int	rda_config	= ORPGRDA_ORDA_CONFIG;

   /* Set the wideband failed flag in comms status. */
   if( (rda_config = ST_get_rda_config( NULL )) == ORPGRDA_ORDA_CONFIG )
      CR_ORDA_status.wb_comms.wb_failed = 1;
      
   else
      CR_RDA_status.wb_comms.wb_failed = 1;

   /* Update the RDA Status. */
   ST_set_communications_alarm( alarm_code );

   /* If alarm code is RDA_LINK_BROKEN_ALARM, set flag. */
   if( alarm_code == RDA_LINK_BROKEN_ALARM )
      Link_broken_alarm_set = 1;

   /* If alarm code is RDA_COMM_DISC_ALARM, set flag. */
   else if ( alarm_code == RDA_COMM_DISC_ALARM )
      Rpg_communications_alarm_set = 1;

   else{

      /* Unknown or unsupported communications alarm. */
      return;

   }

} /* End of SCS_set_communications_alarm() */

/*\/////////////////////////////////////////////////////////////////
//
//   Description:  
//      Clears the communications alarm associated with alarm_code.
//      The alarm is cleared in RPG state and a message is written to
//      the system status log.
//
//   Inputs: 
//      alarm_code - code of alarm to clear.
//
//   Outputs:
//
//   Returns:  
//      There is no return values defined for this function.
//
//   Globals:
//
//   Notes:         
//      All global variables are defined and described in 
//      crda_control_rda.h.  These will begin with CR_.  All file 
//      scope global variables are defined and described at the 
//      top of the file.
//
////////////////////////////////////////////////////////////////\*/
void SCS_clear_communications_alarm( int alarm_code ){

   RDA_alarm_entry_t *alarm_data;
   unsigned char value;
   int ret = 0;
   int rda_chan = 0;
   int rda_config = ORPGRDA_ORDA_CONFIG;

   /* Clear the wideband failure flag. */
   if( (rda_config = ST_get_rda_config( NULL )) == ORPGRDA_ORDA_CONFIG )
      CR_ORDA_status.wb_comms.wb_failed = 0;

   else
      CR_RDA_status.wb_comms.wb_failed = 0;

   /* Update the RDA Status message. */
   if( (Rpg_communications_alarm_set && (alarm_code == RDA_COMM_DISC_ALARM))
                                     ||
       (Link_broken_alarm_set && (alarm_code == RDA_LINK_BROKEN_ALARM)) )
   ST_clear_communications_alarm( alarm_code );

   /* Retrieve the current RDA channel num */
   if( rda_config == ORPGRDA_ORDA_CONFIG )
      rda_chan = CR_ORDA_status.status_msg.msg_hdr.rda_channel & 0x03;

   else
      rda_chan = CR_RDA_status.status_msg.msg_hdr.rda_channel & 0x03;

   /* If the Link Broken alarm is set, clear it. */
   if ( Link_broken_alarm_set && (alarm_code == RDA_LINK_BROKEN_ALARM ) ){

      /* Report RDA communications alarm cleared. */
      alarm_data = 
         (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data( (int) RDA_LINK_BROKEN_ALARM );

      /* If redundant, include channel number in message. */
      if ( rda_chan ) 
         LE_send_msg( GL_STATUS | LE_RPG_AL_MAM | LE_RPG_AL_CLEARED, "%s [RDA:%d] %s\n", 
            ORPGINFO_RPG_ALARM_CLEARED, rda_chan, alarm_data->alarm_text );
      else
         LE_send_msg( GL_STATUS | LE_RPG_AL_MAM | LE_RPG_AL_CLEARED, "%s %s\n", 
                      ORPGINFO_RPG_ALARM_CLEARED, alarm_data->alarm_text );

      /* Link broken alarm was set, so clear the alarm set flag. */
      Link_broken_alarm_set = 0;

      /* Clear the RPG alarm for wideband failure. */
      ret = ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_WBFAILRE,
                                        ORPGINFO_STATEFL_CLR, &value );
      if( ret < 0 )
         LE_send_msg( GL_INFO, 
                      "Unable to Clear RPG Alarm (RDA LINK: RDA_LINK_BROKEN_ALARM)\n" );
   }

   /* If the RPG Communications alarm is set, clear it. */
   if ( Rpg_communications_alarm_set && (alarm_code == RDA_COMM_DISC_ALARM) ){

      /* Report RDA communications alarm cleared. */
      alarm_data = (RDA_alarm_entry_t *)
         ORPGRAT_get_alarm_data( (int) RDA_COMM_DISC_ALARM );

      /* If redundant, include channel number in message. */
      if ( rda_chan )
         LE_send_msg( GL_STATUS | LE_RPG_AL_MAM | LE_RPG_AL_CLEARED, "%s [RDA:%d] %s\n", 
            ORPGINFO_RPG_ALARM_CLEARED, rda_chan, alarm_data->alarm_text );
      else
         LE_send_msg( GL_STATUS | LE_RPG_AL_MAM | LE_RPG_AL_CLEARED, "%s %s\n",
            ORPGINFO_RPG_ALARM_CLEARED, alarm_data->alarm_text );

      /* RDA Communications Discontinuity Alarm is not active. */
      Rpg_communications_alarm_set = 0;

      /* Clear the RPG alarm for wideband failure. */
      ret = ORPGINFO_statefl_rpg_alarm( ORPGINFO_STATEFL_RPGALRM_WBFAILRE,
                                        ORPGINFO_STATEFL_CLR, &value );

      if( ret < 0 )
         LE_send_msg( GL_INFO, 
            "Unable to Clear RPG Alarm (RDA LINK: RDA_LINK_BROKEN_ALARM)\n" );
   }

} /* End of SCS_clear_communications_alarm() */


/*\/////////////////////////////////////////////////////////////////////
//
//   Description:
//      Returns the status of a Wideband Communications alarm.
//
//   Inputs:
//      Alarm code for which status is requested.
//
//   Outputs:
//
//   Returns: 
//      1 - if alarm associated with alarm code is active, otherwise 0.
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
int SCS_check_wb_alarm_status( int wb_alarm_code ){

   if( Link_broken_alarm_set 
                  &&
       wb_alarm_code == RDA_LINK_BROKEN_ALARM )
      return(1);

   if ( Rpg_communications_alarm_set
                   &&
        wb_alarm_code == RDA_COMM_DISC_ALARM )
      return(1);

   return(0);

/* End of SCS_check_wb_alarm_status() */
}

/*\//////////////////////////////////////////////////////////////
//
//   Description:
//      Write communications line status data to task log file.
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
/////////////////////////////////////////////////////////////\*/
void SCS_display_comms_line_status(){

   static int	old_wblnstat			= -1;
   static int	old_rda_display_blanking	= -1;
   static int	old_wb_failed			= -1;
   int		wb_line_stat			= 0;
   int		disp_blank			= 0;
   int		wb_failed			= 0;
   int		rda_stat			= 0;
   int		op_stat				= 0;

   /* Retrieve the necessary wb parameters */
   wb_line_stat = SCS_get_wb_status( ORPGRDA_WBLNSTAT );
   disp_blank = SCS_get_wb_status( ORPGRDA_DISPLAY_BLANKING );
   wb_failed = SCS_get_wb_status( ORPGRDA_WBFAILED );

   /* Retrieve the necessary status parameters */
   rda_stat = ST_get_status( RS_RDA_STATUS );
   if ( rda_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg( GL_ERROR,
         "SCS_display_comms_line_status: Could not retrieve RS_RDA_STATUS!\n");
      rda_stat = 0; /* Reset to 0 */
   }
   op_stat = ST_get_status( RS_OPERABILITY_STATUS );
   if ( op_stat == ORPGRDA_DATA_NOT_FOUND )
   {
      LE_send_msg( GL_ERROR,
         "SCS_display_comms_line_status: Could not retrieve RS_OPERABILITY_STATUS!\n");
      op_stat = 0; /* Reset to 0 */
   }

   if( (old_wblnstat == wb_line_stat)
                        &&
       (old_rda_display_blanking == disp_blank)
                        &&
       (old_wb_failed == wb_failed ) )
      return;

   old_wblnstat = wb_line_stat;
   old_rda_display_blanking = disp_blank;
   old_wb_failed = wb_failed;
  
   LE_send_msg( GL_INFO, "Wideband Communications Status: \n" );

   switch( wb_line_stat )
   {
      case RS_NOT_IMPLEMENTED:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  NOT_IMPLEMENTED\n" );
         break;

      case RS_CONNECT_PENDING:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  CONNECT PENDING\n" );
         break;

      case RS_DISCONNECT_PENDING:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  DISCONNECT PENDING\n" );
         break;

      case RS_DISCONNECTED_HCI:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  DISCONNECTED HCI\n" );
         break;

      case RS_DISCONNECTED_CM:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  DISCONNECTED CM\n" );
         break;

      case RS_DISCONNECTED_SHUTDOWN:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  DISCONNECTED SHUTDOWN\n" );
         break;

      case RS_DISCONNECTED_RMS:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  DISCONNECTED RMS\n" );
         break;

      case RS_CONNECTED:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  CONNECTED\n" );
         break;

      case RS_DOWN:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  DOWN\n" );
         break;

      case RS_WBFAILURE:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  WBFAILURE\n" );
         break;

      default:
         LE_send_msg( GL_INFO, "---> Wideband Line Status:  %d\n", wb_line_stat );
   }

   switch( disp_blank )
   {
      case 0:
         LE_send_msg( GL_INFO, "---> RDA Display Blanking:  NONE (%d, %d)\n",
            rda_stat, op_stat );
         break;

      case RS_RDA_STATUS:
         LE_send_msg( GL_INFO,
            "---> RDA Display Blanking:  RDA State Only (%d)\n", rda_stat );
         break;

      case RS_OPERABILITY_STATUS:
         LE_send_msg( GL_INFO,
            "---> RDA Display Blanking:  RDA Operability Status Only (%d)\n",
            op_stat );
         break;

      default:
         LE_send_msg( GL_INFO, "---> RDA Display Blanking:  %d\n", disp_blank );

   }

   if( wb_failed )
      LE_send_msg( GL_INFO, "---> Wideband Failure Flag:  Set\n" );
    
   else
      LE_send_msg( GL_INFO, "---> Wideband Failure Flag:  Not Set\n" );

} /* End of SCS_display_comms_line_status() */

/*\//////////////////////////////////////////////////////////////
//
//   Description:
//      Get communications line status data.
//
//   Inputs:
//      field - field element.
//
//   Outputs:
//
//   Returns:
//      field element value.
//
/////////////////////////////////////////////////////////////\*/
int SCS_get_wb_status( int field ){

   int rda_config;

   /* ORDA configuration */
   if( (rda_config = ST_get_rda_config( NULL )) == ORPGRDA_ORDA_CONFIG ){

      switch( field ){

         case ORPGRDA_WBLNSTAT:
            return( CR_ORDA_status.wb_comms.wblnstat );

         case ORPGRDA_DISPLAY_BLANKING:
            return( CR_ORDA_status.wb_comms.rda_display_blanking );

         case ORPGRDA_WBFAILED:
            return( CR_ORDA_status.wb_comms.wb_failed );

      }

   }
   else{

      switch( field ){

         case ORPGRDA_WBLNSTAT:
            return( CR_RDA_status.wb_comms.wblnstat );

         case ORPGRDA_DISPLAY_BLANKING:
            return( CR_RDA_status.wb_comms.rda_display_blanking );

         case ORPGRDA_WBFAILED:
            return( CR_RDA_status.wb_comms.wb_failed );

      }

   }

   return 0;

/* End of SCS_get_wb_status() */
}
