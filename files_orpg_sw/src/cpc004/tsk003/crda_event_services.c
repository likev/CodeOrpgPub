/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/11/20 22:17:38 $
 * $Id: crda_event_services.c,v 1.13 2006/11/20 22:17:38 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

#include <crda_control_rda.h>

/*
  Local functions.
*/
void Handle_event_control_command( int lbfd, int msg_id, int msg_info,
                                   void *arg );
void Handle_event_cpc4msg( int lbfd, int msg_id, int msg_info, void *msg);
void Handle_event_debug( en_t evtcd, void *msg, size_t msglen );

/*\/////////////////////////////////////////////////////////////////
//
//   Description:
//     This module handles all event registration and LB notification
//     for control_rda.
//
//     LB notification registration for the following LBs updated.
//
//        ORPGEVT_RDA_CONTROL_COMMAND 
//
//     The events registered are:
//
//        ORPGEVT_CONTROL_RDA_DEBUG
//
//   Inputs:
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
//////////////////////////////////////////////////////////////////\*/
void ES_register_for_events(){
 
   int ret;

   /* Register for LB Notification of ORPGEVT_RDA_CONTROL_COMMAND. */
   if( (ret = ORPGDA_UN_register( ORPGDAT_RDA_COMMAND, LB_ANY,
                                  Handle_event_control_command )) < 0 )
      LE_send_msg( GL_ERROR, 
         "Failed to Register ORPGDAT_RDA_COMMAND For LB Notification (%d).\n",
         ret );

   /* Register for event ORPGEVT_CONTROL_RDA_DEBUG */
   if( (ret = EN_register( ORPGEVT_CONTROL_RDA_DEBUG, 
                           (void *) Handle_event_debug )) < 0 )
      LE_send_msg( GL_ERROR, 
                   "Failed to Register ORPGEVT_CONTROL_RDA_DEBUG (%d).\n",
                   ret );

/* End of ES_register() */
}

/*\///////////////////////////////////////////////////////////////////
//
//   Description:   
//      Handler for the ORPGEVT_RDA_CONTROL_COMMAND update event.
//
//   Inputs:    
//      lbfd - file descriptor for this LB.
//      msg_id - message ID of message written.
//      msg_info - length of msg.
//      msg - optional passed argument (unused).
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
///////////////////////////////////////////////////////////////////\*/
void Handle_event_control_command( int lbfd, int msg_id, int msg_info,
                                   void *msg ){

   /* An event occurred.  Set flag indicating such. */
   CR_outstanding_control_command( (int) ORPGEVT_RDA_CONTROL_COMMAND );

/* End of Handle_event_control_command() */
}

/*\///////////////////////////////////////////////////////////////////
//
//   Description:   
//      Event handler for the ORPGEVT_CONTROL_RDA_DEBUG event.
//
//   Inputs:    
//      evtcd - event code associated with ORPGEVT_CONTROL_RDA_DEBUG
//              event.
//      msg - message associated with event.
//      msglen - length of msg.
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
///////////////////////////////////////////////////////////////////\*/
void Handle_event_debug( en_t evtcd, void *msg, size_t msglen ){

   /* An event occurred.  Toggle verbose mode flag. */
   if( CR_verbose_mode )
     CR_verbose_mode = 0;

   else
     CR_verbose_mode = 1;

/* End of Handle_event_debug() */
}

/*\/////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Post specified event.
//
//   Inputs:    
//      evtcd - Event code to be posted.
//      msg - Message associated with event.
//      msglen - Length of msg.
//      local_flag - Determines which nodes receive event (i.e., local
//                   verses broadcast ).
//   Outputs:
//
//   Returns:
//      Negative value on error, otherwise 0.  Error codes are defined
//      in global header file en.h.
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
/////////////////////////////////////////////////////////////////////\*/
int ES_post_event( en_t evtcd, void *msg, size_t msglen, 
                   int local_flag ){

   int ret;

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "Posting Event %d\n", evtcd );

   if( (ret = EN_post( evtcd, msg, msglen, local_flag )) < 0 )
      return ( ES_FAILURE );

   return ( ES_SUCCESS );

/* End of ES_post_event() */
}
