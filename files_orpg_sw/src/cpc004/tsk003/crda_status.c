/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:47:43 $
 * $Id: crda_status.c,v 1.78 2014/03/13 19:47:43 steves Exp $
 * $Revision: 1.78 $
 * $State: Exp $
 */


#include <stdio.h>
#include <string.h>
#include <crda_control_rda.h>
#include <rda_status.h>
#include <rda_alarm_table.h>
#include <orpgrat.h>
#include <orpgsite.h>

/* For status as a whole. */
#define STATUS_WORDS            26
#define MAX_STATUS_LENGTH       64

/* For processing Auxilliary Power/Generator status. */
#define MAX_PWR_BITS             5
#define COMSWITCH_BIT            4

/* For processing Archive II status. */
#define TAPE_NUM_SHIFT		12	
#define TAPE_NUM_MASK		0xf000

/* For date/time conversion. */
#define MILLS_PER_SEC		1000
#define SECS_PER_DAY		86400


static char *status[] = { "Start-Up", 
                          "Standby", 
                          "Restart", 
                          "Operate", 
                          "Playback", 
                          "Off-Line Operate", 
                          "??????" };

static char *moments[] = { "None", 
                           "All", 
                           "R", 
                           "V", 
                           "W", 
                           "??????" };

static char *mode[] = { "Operational",
                        "Maintenance", 
                        "      " };

static char *orda_mode[] = { "Operational", 
                             "??????", 
                             "Maintenance", 
                             "??????" };

static char *authority[] = { "Local", 
                             "Remote", 
                             "??????" };

static char *interference_unit[] = { "Enabled", 
                                     "Disabled", 
                                     "??????" };

static char *channel_status[] = { "Ctl", 
                                  "Non-Ctl", 
                                  "??????" };

static char *spot_blanking[] = { "Not Installed", 
                                 "Enabled", 
                                 "Disabled", 
                                 "??????" };

static char *operability[] = { "On-Line", 
                               "MAR", 
                               "MAM", 
                               "CommShut", 
                               "Inoperable", 
                               "WB Disc", 
                               "??????" };

static char *control[] = { "RDA", 
                           "RPG", 
                           "Eit", 
                           "??????" };

static char *set_aux_pwr[] = { " Aux Pwr=On",
                               " Util Pwr=Yes",
                               " Gen=On",
                               " Xfer=Manual",
                               " Cmd Pwr Switch",
                               "??????" };

static char *reset_aux_pwr[]        = { " Aux Pwr=Off",
                                        " Util Pwr=No",
                                        " Gen=Off",
                                        " Xfer=Auto",
                                        "",
                                        "??????" };

static char *perf_check[] = { "Auto", "Pending", "??????" };

static char *tps[] = { "Off", "Ok", "??????" };

static char *super_res[] = { "Enabled", "Disabled", "??????" };

static char *cmd[] = { "Enabled", "Disabled", "??????" };

static char *avset[] = { "Enabled", "Disabled", "??????" };

static char *archive_status[] = { "Installed", "Media Loaded", "Reserved",
                                  "Record", "Not Installed", "Plybk Avail",
                                  "Search", "Playback", "Check Label",
                                  "Fast Frwd", "Tape Xfer", " Tape #",
                                  "??????" };

/* Static Function Prototypes. */
static void Set_alarm_state( unsigned int *alarm_type, int alarm_state );
static void Rda_update_system_status( RDA_status_msg_t *status,
                                      RDA_status_msg_t *prev_status );
static void Orda_update_system_status( ORDA_status_msg_t *status,
                                       ORDA_status_msg_t *prev_status );
static void Process_status_field( char **buf, int *len, char *field_id, 
                                  char *field_val );
static void Update_rda_alarm_table( void *status, int rda_config );
static int Set_rda_status( int field, int val );
static int Set_orda_status( int field, int val );
static int Get_rda_status ( int element );
static int Get_orda_status ( int element );
static void Copy_current_to_previous();
static int Check_status_change( short *msg );

/*\////////////////////////////////////////////////////////////////////
//
//  Description:
//      Initialization for the ST module.  Reads the RDA status data
//      from the GSM LB.  

//   Inputs:
//
//   Outputs:
//
//   Returns:
//      Currently returns the return value from ORPGDA_read().
//
////////////////////////////////////////////////////////////////////\*/
int ST_init(){

   int ret             = 0;

   /* Read the latest status msg */
   ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &CR_ORDA_status,
                      sizeof (ORDA_status_t), RDA_STATUS_ID);

   if( ret > 0 ){

      /* Copy this to the legacy RDA status. */
      memcpy( &CR_RDA_status, &CR_ORDA_status, sizeof(RDA_status_t) );

      /* Make this also the previous RDA status. */
      memcpy( &CR_previous_ORDA_status, &CR_ORDA_status, sizeof(ORDA_status_t) );
      memcpy( &CR_previous_RDA_status, &CR_RDA_status, sizeof(RDA_status_t) );

   }
   else{

      LE_send_msg( GL_INFO, "ORPGDA_read( ORPGDAT_GSM_DATA, RDA_STATUS_ID ) Failed (%d)\n",
                   ret );

      /* Initialize the data elements to 0. */
      memset( &CR_ORDA_status, 0, sizeof(RDA_status_t) );
      memset( &CR_RDA_status, 0, sizeof(RDA_status_t) );
      memset( &CR_previous_ORDA_status, 0, sizeof(RDA_status_t) );
      memset( &CR_previous_RDA_status, 0, sizeof(RDA_status_t) );

   }

   /* Set the RDA Build number (just a guess). */
   CR_rda_build_num = 12.0f;

   return ret;

/* End of ST_init() */
}

/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//
//      This is the initialization module for RDA Status and Previous
//      RDA State.
//
//      Attempts to read RDA status data from Status Linear Buffer.
//      If the ORPGDA_read status is LB_TO_COME or LB_NOT_FOUND, the RDA
//      status is initialized to a default state, the default status
//      data is written to the rda status LB, and the event
//      ORPGEVT_RDA_STATUS_CHANGE is posted.  For all other negative
//      status values returned from ORPGDA_read, an error message is
//      posted and the module is exited.
//
//      This module also attempts to read the previous RDA status from
//      the Status Linear Buffer.  If this data is available, it is
//      read into the Previous_status structure.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//     ST_FAILURE on error, or ST_SUCCESS otherwise.
//
////////////////////////////////////////////////////////////////////\*/
int ST_init_rda_status( ){

   int          ret             = 0;
   int          new_vcp_num     = 0;
   char         *buf            = NULL;
   double       value1          = 0.0;


   /* Set operational mode to invalid values. */
   CR_operational_mode = 0;

   /* Call the lib routine to read the latest status msg */
   ret = ST_init();
   if ( ret <= 0 ){

      /* Send status log message */
      LE_send_msg( GL_ERROR, "Problem reading RDA Status Msg from Status LB.");

   }

   /* Set the status protect flag to TRUE and the status latch to FALSE.
      The status protect flag is used to determine whether to save
      previous state variables.  The status latch flag is used to
      determine if status data is first data received after a wideband
      connection. */
   CR_status_protect = 1;
   CR_status_latch = 0;


   /* Call lib routine to read and store the previous state.  This lib routine
      attempts to read the previous state info from the RDA status LB and store
      it in the lib's copy of the previous state struct.  If it fails it attempts
      to store the default previous state values in the previous state struct. */
   ret = ORPGRDA_read_previous_state();
   if ( ret != ORPGRDA_SUCCESS ){

      /* Set the current VCP number based on adaptation data. The default weather
         mode is defined in the site adaptation data group.  It can be either
         "Precipitation" or "Clear Air". */
      if( DEAU_get_string_values( "site_info.wx_mode", &buf ) > 0 ){

         if( !strncmp( buf, "Precipitation", 13 ) ){

            if( DEAU_get_values( "site_info.def_mode_A_vcp", &value1, 1 ) > 0 )
               new_vcp_num = (int) value1;

         }
         else{

            if( DEAU_get_values( "site_info.def_mode_B_vcp", &value1, 1 ) > 0 )
               new_vcp_num = (int) value1;

         }

      }

      ret = ORPGRDA_set_state( ORPGRDA_VCP_NUMBER, new_vcp_num );
      if ( ret != ORPGRDA_SUCCESS )
         LE_send_msg( GL_ERROR,
            "ST_init_rda_status: Could not set ORPGRDA_VCP_NUMBER!\n");

      ret = ORPGRDA_set_state_vcp( NULL, 0 );
      if( ret != ORPGRDA_SUCCESS )
         LE_send_msg( GL_ERROR, 
            "ST_init_rda_status: Could not set Previous State VCP data\n" );

      LE_send_msg( GL_INFO,
         "Previous RDA State Read Failed... Setting Defaults\n");

      /* Write out the Previous RDA Status Data and return */
      ORPGRDA_report_previous_state();
      return ( ST_SUCCESS );

      LE_send_msg( GL_ERROR | GL_INFO, "Read of Previous RDA State Failed.");

      return ( ST_FAILURE );

   }

   /* Write out the Previous RDA Status Data. */
   ORPGRDA_report_previous_state();

   return ( ST_SUCCESS );

} /* End of ST_init_rda_status() */


/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      Processes the RDA status data.  This includes:
//
//         1) The RDA status data is checked to determine if the
//            data has changed since the last status data received.
//            ST_update_rda_status is then called to:
//            a)  Post the ORPGEVT_RDA_STATUS_CHANGED event if the
//                status data has changed.
//            b)  The RDA status is written to the global status
//                Linear Buffer.
//
//         2) If a Remote Control Enabled control authorization
//            is received, a Remote Control Acknowledged is sent
//            to the RDA.
//
//         3) The control status, control authorization and operational
//            state are saved.
//
//         4) If rda control status is local control and rda control
//            authorization is not local control or status is not
//            protected, then save previous state variables.
//
//         5) If the RDA-RPG communications discontinuity timer is not
//            set, and the RDA status is OPERATE, any base data moment
//            is enabled, and the operational mode is OPERATIONAL, set
//            the RDA-RPG communications discontinuity timer.
//
//   Inputs:
//      rda_data - pointer to RDA status message data (including message header).
//
//   Outputs:
//
//   Returns:
//      Currently undefined.
//
////////////////////////////////////////////////////////////////////\*/
int ST_process_rda_status( short *rda_data ){

   int          ret                             = 0;
   int          status_changed                  = 0;
   int          rda_status                      = 0;
   int          rda_vcp_num                     = 0;
   int          rda_channel                     = 0;
   int          rda_contr_auth                  = 0;
   int          rda_contr_stat                  = 0;
   int          rda_int_suppr_unit              = 0;
   int          rda_spot_blank_stat             = 0;
   int          rda_chan_contr_stat             = 0;
   int          rda_data_trans_enbld            = 0;
   int          rda_super_res                   = 0;
   int          rda_cmd	                        = 0;
   int          rda_avset	                = 0;
   int          prev_rda_status                 = 0;
   int          prev_rda_vcp_num                = 0;
   int          prev_rda_control_auth           = 0;
   int          prev_rda_int_suppr_unit         = 0;
   int          prev_operational_mode           = 0;
   int          prev_rda_spot_blank_stat        = 0;
   int          prev_rda_chan_contr_stat        = 0;
   int          prev_rda_data_trans_enbld       = 0;

   static int   old_rda_channel                 = -1;
   static short previous_control_authorization  = -1;
   short*       msg_data_ptr                    = NULL;
   int          rda_config                      = ORPGRDA_ORDA_CONFIG;


   /* We must send a pointer to the message data itself, not the msg hdr */
   msg_data_ptr = rda_data + MSGHDRSZ;

   /* If any status value changed, set flag for sending an event. */
   status_changed = Check_status_change( msg_data_ptr );

   /* If the RDA status indicates OFFLINE MAINTENANCE, change the RDA status
      to indicate the RDA is shutting down.   Also clear the alarm summary. */
   if( (rda_config = ST_get_rda_config( rda_data )) == ORPGRDA_ORDA_CONFIG ){

      ORDA_status_msg_t *status = (ORDA_status_msg_t *) rda_data;

      CR_operational_mode = status->op_mode;
      if( (status->rda_build_num / 100) > 2 )
         CR_rda_build_num = (float) status->rda_build_num / 100.0f;

      else
         CR_rda_build_num = (float) status->rda_build_num / 10.0f;

      LE_send_msg( GL_INFO, "RDA Build Number: %6.2f\n", CR_rda_build_num );

      if ( CR_operational_mode == OP_OFFLINE_MAINTENANCE_MODE ){

         status->op_status = OS_COMMANDED_SHUTDOWN;
         status->rda_alarm = AS_NO_ALARMS;

      }

      /* Extract items to be used within this module. */
      rda_channel = (int) (status->msg_hdr.rda_channel & 0x03);
      rda_status = (int) status->rda_status;
      rda_contr_auth = (int) status->rda_control_auth;
      rda_contr_stat = (int) status->control_status;
      rda_vcp_num = (int) status->vcp_num;
      rda_spot_blank_stat = (int) status->spot_blanking_status;
      rda_chan_contr_stat = (int) status->channel_status;
      rda_data_trans_enbld = (int) status->data_trans_enbld;
      rda_super_res = (int) status->super_res;

      /* Bit 0 is the enabled/disabled bit. */
      rda_cmd = (int) status->cmd  & 0x1;
      rda_int_suppr_unit = 0;
      rda_avset = (int) status->avset;

   }
   else{

      RDA_status_msg_t *status = (RDA_status_msg_t *) rda_data;

      CR_operational_mode = status->op_mode;
      if ( CR_operational_mode == OP_OFFLINE_MAINTENANCE_MODE ){

         status->op_status = OS_COMMANDED_SHUTDOWN;
         status->rda_alarm = AS_NO_ALARMS;

      }

      rda_channel = (int) (status->msg_hdr.rda_channel & 0x03);
      rda_status = (int) status->rda_status;
      rda_contr_auth = (int) status->rda_control_auth;
      rda_contr_stat = (int) status->control_status;
      rda_vcp_num = (int) status->vcp_num;
      rda_spot_blank_stat = (int) status->spot_blanking_status;
      rda_chan_contr_stat = (int) status->channel_status;
      rda_data_trans_enbld = (int) status->data_trans_enbld;
      rda_int_suppr_unit = (int) status->int_suppr_unit;
      rda_super_res = 0;
      rda_cmd = 0;
      rda_avset = 0;
   }

   /* Update RDA status linear buffer. */
   ST_update_rda_status( status_changed, (char *) rda_data, (int) RDA_INITIATED );

   /* Update the system status log. */
   ST_update_system_status();

   /* Report RDA alarm types and command Acknowledgements. */
   ST_process_rda_alarms( (char *) rda_data );
   ST_process_command_ack( (char *) rda_data );

   /* Check to see if the RDA channel has changed.  If so, write a
      message to the system log indicating this. */
   if (rda_channel != old_rda_channel){

      old_rda_channel = rda_channel;
      if (CR_redundant_type != ORPGSITE_NO_REDUNDANCY)
         LE_send_msg( GL_STATUS,
               "Changed To RDA Channel %d\n", old_rda_channel );
   }

   /* If remote control enabled and RDA control is local, accept it. */
   if( rda_contr_auth == (int) CA_REMOTE_CONTROL_ENABLED ){

      if( ((rda_config == ORPGRDA_LEGACY_CONFIG)
                            &&
            (rda_contr_auth != previous_control_authorization))
                            ||
           ((rda_config == ORPGRDA_ORDA_CONFIG)
                            &&
            (rda_contr_stat == CS_LOCAL_ONLY)) )
         SWM_send_rda_command( (int) 1, (int) COM4_RDACOM,
                               (int) RDA_PRIMARY, (int) CRDA_ACCREMOTE );

   }

   /* If RDA control authorization is not remote enabled and RDA control is local,
      request remote control. */
   if( (rda_contr_auth != (int) CA_REMOTE_CONTROL_ENABLED)
                          &&
       (rda_contr_stat == CS_LOCAL_ONLY) )
      SWM_send_rda_command( (int) 1, (int) COM4_RDACOM, (int) RDA_PRIMARY,
                            (int) CRDA_REQREMOTE );

   /* If Super Res enabled state is different that current state reported by
      RDA, change the RDA state. */
   if( rda_config == ORPGRDA_ORDA_CONFIG ){

      int sr_state = ORPGINFO_is_super_resolution_enabled();

      if( sr_state && (rda_super_res == SR_DISABLED) )
         SWM_send_rda_command( (int) 1, (int) COM4_RDACOM, (int) RDA_PRIMARY,
                               (int) CRDA_SR_ENAB );

      else if( !sr_state && (rda_super_res == SR_ENABLED) )
         SWM_send_rda_command( (int) 1, (int) COM4_RDACOM, (int) RDA_PRIMARY,
                               (int) CRDA_SR_DISAB );

   }

   /* If CMD enabled state is different that current state reported by
      RDA, change the RDA state. */
   if( rda_config == ORPGRDA_ORDA_CONFIG ){

      int cmd_state = ORPGINFO_is_cmd_enabled();

      if( cmd_state && (rda_cmd == CMD_DISABLED) )
         SWM_send_rda_command( (int) 1, (int) COM4_RDACOM, (int) RDA_PRIMARY,
                               (int) CRDA_CMD_ENAB );

      else if( !cmd_state && (rda_cmd == CMD_ENABLED) )
         SWM_send_rda_command( (int) 1, (int) COM4_RDACOM, (int) RDA_PRIMARY,
                               (int) CRDA_CMD_DISAB );

   }

   /* If AVSET enabled state is different that current state reported by
      RDA, change the RDA state. */
   if( rda_config == ORPGRDA_ORDA_CONFIG ){

      int avset_state = ORPGINFO_is_avset_enabled();

      if( avset_state && (rda_avset == AVSET_DISABLED) )
         SWM_send_rda_command( (int) 1, (int) COM4_RDACOM, (int) RDA_PRIMARY,
                               (int) CRDA_AVSET_ENAB );

      else if( !avset_state && (rda_avset == AVSET_ENABLED) )
         SWM_send_rda_command( (int) 1, (int) COM4_RDACOM, (int) RDA_PRIMARY,
                               (int) CRDA_AVSET_DISAB );

   }

   /* Set the current control status. */
   CR_control_status = rda_contr_stat;

   /* Set the previous control authorization. */
   previous_control_authorization = rda_contr_auth;
   CR_control_authority = previous_control_authorization;


   /* If rda control status is local control and rda control
      authorization is not local control or status is not protected,
      then save previous state variables. */
   if ( (CR_control_status == CS_LOCAL_ONLY
                         &&
         CR_control_authority != CA_REMOTE_CONTROL_ENABLED)
                         ||
        !CR_status_protect ){

      status_changed = 0;

      /* Change status changed flag if necessary */
      prev_rda_status = ORPGRDA_get_previous_state(ORPGRDA_RDA_STATUS);
      prev_rda_vcp_num = ORPGRDA_get_previous_state(ORPGRDA_VCP_NUMBER);
      prev_rda_control_auth = ORPGRDA_get_previous_state(ORPGRDA_RDA_CONTROL_AUTH);
      prev_operational_mode = ORPGRDA_get_previous_state(ORPGRDA_OPERATIONAL_MODE);
      prev_rda_spot_blank_stat = ORPGRDA_get_previous_state(ORPGRDA_SPOT_BLANKING_STATUS);
      prev_rda_chan_contr_stat = ORPGRDA_get_previous_state(ORPGRDA_CHAN_CONTROL_STATUS);
      prev_rda_data_trans_enbld = ORPGRDA_get_previous_state(ORPGRDA_DATA_TRANS_ENABLED);
      prev_rda_int_suppr_unit = 0;
      if( rda_config == ORPGRDA_LEGACY_CONFIG )
         prev_rda_int_suppr_unit = ORPGRDA_get_previous_state(ORPGRDA_ISU);

      if( (prev_rda_status != rda_status)
                       ||
          (prev_rda_vcp_num != rda_vcp_num)
                       ||
          (prev_rda_control_auth != CR_control_authority)
                       ||
          (prev_rda_int_suppr_unit != rda_int_suppr_unit)
                       ||
          (prev_operational_mode != CR_operational_mode)
                       ||
          (prev_rda_spot_blank_stat != rda_spot_blank_stat)
                       ||
          (prev_rda_chan_contr_stat != rda_chan_contr_stat) )
            status_changed = 1;

      /* If the RDA is not restarting or starting up, then save the RDA
         status. */
      if( (rda_status != RS_RESTART) && (rda_status != RS_STARTUP) ){

         ret = ORPGRDA_set_state(ORPGRDA_RDA_STATUS, rda_status);
         if ( ret != ORPGRDA_SUCCESS )
            LE_send_msg(GL_ERROR,
               "Failure to set RDA status state.\n");

      }

      /* If the RDA state is operate or playback, save the base data
         moments enabled. */
      if ( (rda_status == RS_OPERATE) || (rda_status == RS_PLAYBACK) ){

         if( prev_rda_data_trans_enbld != rda_data_trans_enbld )
            status_changed = 1;

         ret = ORPGRDA_set_state(ORPGRDA_DATA_TRANS_ENABLED, rda_data_trans_enbld);
         if ( ret != ORPGRDA_SUCCESS )
            LE_send_msg(GL_ERROR,
               "Failure to set RDA data trans enabled state.\n");

      }

      /* Do not save the VCP number if it is zero. */
      if( rda_vcp_num != 0 ){

         ret = ORPGRDA_set_state(ORPGRDA_VCP_NUMBER, rda_vcp_num);
         if ( ret != ORPGRDA_SUCCESS )
            LE_send_msg(GL_ERROR,
               "Failure to set RDA current VCP num.\n");

      }

      ret = ORPGRDA_set_state(ORPGRDA_RDA_CONTROL_AUTH, CR_control_authority);
      if( ret != ORPGRDA_SUCCESS )
         LE_send_msg(GL_ERROR,
            "Failure to set RDA control authority.\n");

      if( rda_config == ORPGRDA_LEGACY_CONFIG ){

         ret = ORPGRDA_set_state(ORPGRDA_ISU, rda_int_suppr_unit);
         if ( ret != ORPGRDA_SUCCESS )
            LE_send_msg(GL_ERROR,
               "Failure to set RDA interf suppr unit.\n");

      }

      ret = ORPGRDA_set_state(ORPGRDA_SPOT_BLANKING_STATUS, rda_spot_blank_stat);
      if( ret != ORPGRDA_SUCCESS )
         LE_send_msg(GL_ERROR,
            "Failure to set RDA spot blank status.\n");

      ret = ORPGRDA_set_state(ORPGRDA_CHAN_CONTROL_STATUS, rda_chan_contr_stat);
      if( ret != ORPGRDA_SUCCESS )
         LE_send_msg(GL_ERROR,
            "Failure to set RDA channel control.\n");

      /* Checkpoint the previous state information if not in the process
         of shutting down and the status data has changed. */
      if ( (CR_shut_down_state == SHUT_DOWN_NO) && status_changed )
         ST_checkpoint_previous_state();

   }

   /* If wideband line is connected, then .... */
   if( SCS_get_wb_status( ORPGRDA_WBLNSTAT ) == RS_CONNECTED ){

      /* If communications discontinuity timer is not set and the state is
         OPERATIONAL, the operational mode is OPERATIONAL, and at least one
         moment is enabled, then activate the timer. */
      if( (!SCS_check_wb_alarm_status( (int) RDA_COMM_DISC_ALARM ))
                                &&
          (!CR_communications_discontinuity) ){

         if( (CR_operational_mode == OP_OPERATIONAL_MODE)
                               &&
             (rda_status == RS_OPERATE)
                               &&
             (rda_data_trans_enbld > BD_ENABLED_NONE) ){

            if( !TS_is_timer_active( (int) MALRM_RDA_COMM_DISC ) ){

               if( TS_set_timer( (malrm_id_t) MALRM_RDA_COMM_DISC,
                                 (time_t) RDA_COMM_DISC_VALUE,
                                 (unsigned int) RDA_COMM_DISC_VALUE ) == TS_SUCCESS ){

                  if ( CR_verbose_mode )
                     LE_send_msg( GL_INFO, "Set RDA_COMM_DISC Timer.\n" );

               }

            }

         }

      }

   } /* end if to check for wb connection */

   /* Return Normal. */
   return ( ST_SUCCESS );

} /* End of ST_process_rda_status() */

/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      The previous state data is written to the RDA Status Linear
//      Buffer.  On error, an error message is logged.
//
//   Inputs:
//
//   Outputs:
//
//   Returns:
//      ST_FAILURE if ORPGDA_write fails, ST_SUCCESS otherwise.
//
////////////////////////////////////////////////////////////////////\*/
int ST_checkpoint_previous_state( ){

   int ret;

   if( CR_verbose_mode )
      LE_send_msg( GL_INFO, "Checkpointing RDA Previous State.\n" );

   /* Write previous state data to linear buffer. This routine writes the
      contents of the previous state object in the library to the previous
      state LB */
   ret = ORPGRDA_write_state();

   if( ret != ORPGRDA_SUCCESS ){

      LE_send_msg( GL_ERROR, "Previous State Write Failed\n");
      return ( ST_FAILURE );
   }

   ORPGRDA_report_previous_state();

   return ( ST_SUCCESS );

} /* End of ST_checkpoint_previous_state() */


/*\//////////////////////////////////////////////////////////////////
//
//   Description:   
//      Reports any rda alarms (in plain text). This module only 
//      called if verbose mode is on.
//
//   Inputs:   
//      msg - pointer to the RDA status message.
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
//   Notes:         
//
//////////////////////////////////////////////////////////////////\*/
void ST_process_rda_alarms( char *msg ){

   int			i		= 0;
   int			chan_num	= 0;
   RDA_alarm_entry_t* 	alarm_data	= NULL;
   int			alarm_codes[MAX_RDA_ALARMS_PER_MESSAGE];
   unsigned int		alarm_state	= 0;
   int                  rda_config = ST_get_rda_config( msg );


   /* Retrieve and store the RDA channel number */
   if( rda_config == ORPGRDA_ORDA_CONFIG )
      chan_num = CR_ORDA_status.status_msg.msg_hdr.rda_channel & 0x03;

   else
      chan_num = CR_RDA_status.status_msg.msg_hdr.rda_channel & 0x03;

   /* Check the configuration ... then copy the alarm codes based on 
      the configuration. */
   if( rda_config == ORPGRDA_ORDA_CONFIG ){

      ORDA_status_msg_t *status = (ORDA_status_msg_t *) msg;
      int alarm_ind;

      for( alarm_ind = 0; alarm_ind < MAX_RDA_ALARMS_PER_MESSAGE; alarm_ind++ )
         alarm_codes[alarm_ind] = (int) status->alarm_code[alarm_ind];

   }
   else if( rda_config == ORPGRDA_LEGACY_CONFIG ){

      RDA_status_msg_t *status = (RDA_status_msg_t *) msg;
      int alarm_ind;

      for( alarm_ind = 0; alarm_ind < MAX_RDA_ALARMS_PER_MESSAGE; alarm_ind++ )
         alarm_codes[alarm_ind] = (int) status->alarm_code[alarm_ind];

   }
   else{

      LE_send_msg( GL_STATUS, 
          "Bad RDA Status Message Received.  Unable to Process RDA Alarms\n" );
      return;

   }
  
   /* Do For All Alarms in this RDA status message. */
   for( i = 0; i < MAX_RDA_ALARMS_PER_MESSAGE; i++ ){

      /* If valid alarm code, then ..... */
      if( (alarm_codes[i] & 0x7FFF) <= MAX_RDA_ALARM_NUMBER ){

         /* Alarm codes > 0 indicate new alarm. */
         if( alarm_codes[i] > 0 ){

            alarm_data = 
               (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data((int) alarm_codes[i]);

            /* Set the RDA alarm state for status log processing. */
            alarm_state = GL_STATUS;
            Set_alarm_state( &alarm_state, (int) alarm_data->state );
           
	    /* If redundant, indicate the RDA channel # */
	    if( chan_num != 0 ){
 
               /* Write new RDA alarm to system status log. */
               LE_send_msg( alarm_state, "%s [RDA:%d] %s\n",
                            ORPGINFO_RDA_ALARM_ACTIVATED, chan_num,
                            alarm_data->alarm_text );
            }
            else{

              /* Write new RDA alarm to system status log. */
              LE_send_msg( alarm_state, "%s %s\n",
                 ORPGINFO_RDA_ALARM_ACTIVATED, alarm_data->alarm_text );

            }

         }
         else if( alarm_codes[i] < 0 ){

            /* Negative alarm codes indicate the alarm was cancelled.  Mask off 
               most significant bit to get the alarm code. */
            alarm_codes[i] &= 0x7FFF;
            alarm_data =
               (RDA_alarm_entry_t *) ORPGRAT_get_alarm_data((int) alarm_codes[i]);

            /* Set the RDA alarm state for status log processing. */
            alarm_state = GL_STATUS | LE_RDA_AL_CLEARED;
            Set_alarm_state( &alarm_state, alarm_data->state );

	    /* If redundant, indicate the RDA channel # */
	    if( chan_num ){

              /* Write cancelled RDA alarm to system status log. */
              LE_send_msg( alarm_state, "%s [RDA:%d] %s\n",
                           ORPGINFO_RDA_ALARM_CLEARED, chan_num,
                           alarm_data->alarm_text );
            }
            else{

               /* Write cancelled RDA alarm to system status log. */
               LE_send_msg( alarm_state, "%s %s\n",
                            ORPGINFO_RDA_ALARM_CLEARED, alarm_data->alarm_text ); 

            }

         }

      }

   } /* End of "for" loop */

/* End of ST_process_rda_alarms() */
} 

/*\//////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Reports command acknowledgements from the rda.  
//
//   Inputs:   
//      msg - pointer to RDA Status message.
//
//   Outputs:
//
//   Returns:
//      There are no return valus defined for this function.
//
//   Notes:
//
/////////////////////////////////////////////////////////////////////\*/
void ST_process_command_ack( char *msg ){

   int command_stat = 0;
   int rda_config = ST_get_rda_config( msg );

   /* Check the configuration ... then get the command status. */
   if( rda_config == ORPGRDA_ORDA_CONFIG ){

      ORDA_status_msg_t *status = (ORDA_status_msg_t *) msg;
      command_stat = (int) status->command_status;

   }
   else if( rda_config == ORPGRDA_LEGACY_CONFIG ){

      RDA_status_msg_t *status = (RDA_status_msg_t *) msg;
      command_stat = (int) status->command_status;

   }
   else{

      LE_send_msg( GL_STATUS,
          "Bad RDA Status Message Received.  Unable to Process RDA Alarms\n" );
      return;

   }


   /* If command acknowledgement, what command? */
   switch( command_stat ){

      case RS_NO_ACKNOWLEDGEMENT:
         break;

      case RS_REMOTE_VCP_RECEIVED:
      {
         LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                      "RDA ACKNOWLEDGMENT: Remote VCP Received at RDA");
         break;
      }

      case RS_CLUTTER_BYPASS_MAP_RECEIVED:
      {
         LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS,
                      "RDA ACKNOWLEDGEMENT: Bypass Map Received at RDA");
         break;
      }

      case RS_CLUTTER_CENSOR_ZONES_RECEIVED:
      {
         LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS,
             "RDA ACKNOWLEDGEMENT: Clutter Censor Zones Received at RDA");
         break;
      }

      case RS_REDUND_CHNL_STBY_CMD_ACCEPTED:
      {
         LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                      "RDA ACKNOWLEDGEMENT: Redundant Channel Standby");
         break;
      }

      case 5:
      {
         LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, 
                      "RDA ACKNOWLEDGEMENT: Command Accepted by RDA" );
         break;
      }

      default:
      {
         LE_send_msg(GL_ERROR,
            "UNKNOWN RDA ACKNOWLEDGEMENT (%d)\n", command_stat);
         break; 
      }

   } /* End of "switch". */

/* End of ST_process_command_ack() */
} 

/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Writes RDA status data to system status log file in plain text
//      format.  Only status which is different than last reported is
//      written.
//
/////////////////////////////////////////////////////////////////////////////\*/
void ST_update_system_status( ){

   int rda_config = ST_get_rda_config( NULL );

   if( rda_config == ORPGRDA_ORDA_CONFIG )
      Orda_update_system_status( &CR_ORDA_status.status_msg, 
                                 &CR_previous_ORDA_status.status_msg );

   else if( rda_config == ORPGRDA_LEGACY_CONFIG )
      Rda_update_system_status( &CR_RDA_status.status_msg,
                                &CR_previous_RDA_status.status_msg );

/* End of ST_update_system_status() */
} 

/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Sets various fields in the RDA Status message based on the 
//      alarm_code.
//
//   Inputs:
//      alarm_code - The alarm code.  Should be either:
//                   RDA_LINK_BROKEN_ALARM
//                   RDA_COMM_DISC_ALARM
//
//   Returns:
//      void
//
/////////////////////////////////////////////////////////////////////////////\*/
void ST_set_communications_alarm( int alarm_code ){

   int rda_config = ST_get_rda_config( NULL );

   /* If alarm code is RDA_LINK_BROKEN_ALARM, set flag. */
   if( alarm_code == RDA_LINK_BROKEN_ALARM ){

      if( rda_config == ORPGRDA_ORDA_CONFIG ){

         /* Set the RDA operability status to INOPERABLE. */
         CR_ORDA_status.status_msg.op_status = OS_INOPERABLE;

         /* Set the wideband alarm bit in RDA status. */
         CR_ORDA_status.status_msg.rda_alarm = AS_RPG_COMMUN;

      }
      else if( rda_config == ORPGRDA_LEGACY_CONFIG ){

         /* Set the RDA operability status to INOPERABLE. */
         CR_RDA_status.status_msg.op_status = OS_INOPERABLE;

         /* Set the wideband alarm bit in RDA status. */
         CR_RDA_status.status_msg.rda_alarm = AS_RPG_COMMUN;

      }

   }
      /* If alarm code is RDA_COMM_DISC_ALARM, set flag. */
   else if ( alarm_code == RDA_COMM_DISC_ALARM ){

      if( rda_config == ORPGRDA_ORDA_CONFIG ){

         /* Set the RDA operability status INOPERABLE bit and clear the
            on-line bit.  This is what legacy does ... so we do it also. */
         CR_ORDA_status.status_msg.op_status |= OS_INOPERABLE;
         CR_ORDA_status.status_msg.op_status &= ~OS_ONLINE;

         /* Set the RDA CONTROL alarm summary bit in RDA status.
            For some reason, legacy does not do this for link broken
            alarms .... so we don't do it either! */
         CR_ORDA_status.status_msg.rda_alarm = AS_RDA_CONTROL;

      }
      else if( rda_config == ORPGRDA_LEGACY_CONFIG ){

         /* Set the RDA operability status INOPERABLE bit and clear the
            on-line bit.  This is what legacy does ... so we do it also. */
         CR_RDA_status.status_msg.op_status |= OS_INOPERABLE;
         CR_RDA_status.status_msg.op_status &= ~OS_ONLINE;

         /* Set the RDA CONTROL alarm summary bit in RDA status.
            For some reason, legacy does not do this for link broken
            alarms .... so we don't do it either! */
         CR_RDA_status.status_msg.rda_alarm = AS_RDA_CONTROL;

      }

   }

/* End of ST_set_communication_alarm() */
}

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
void ST_clear_communications_alarm( int alarm_code ){

   int rda_config = ST_get_rda_config( NULL );
   
   /* If RPG communications alarm set and alarm code is RDA communications
      discontinuity alarm, then check if the RDA CONTROL alarm bit is set
      in RDA status data.  If yes, reset it. */
   if( alarm_code == RDA_COMM_DISC_ALARM ){

      if( rda_config == ORPGRDA_ORDA_CONFIG ){

         if( CR_ORDA_status.status_msg.rda_alarm & AS_RDA_CONTROL )
            CR_ORDA_status.status_msg.rda_alarm &= ~AS_RDA_CONTROL;

      }
      else if( rda_config == ORPGRDA_LEGACY_CONFIG ){

         if( CR_RDA_status.status_msg.rda_alarm & AS_RDA_CONTROL )
            CR_RDA_status.status_msg.rda_alarm &= ~AS_RDA_CONTROL;

      }

   }

   /* If RDA Link Broken alarm set and alarm code is RDA Link Broken
      alarm, then check if the RPG COMMUNICATION alarm bit is set
      in RDA status data.  If yes, reset it. */
   else if( alarm_code == RDA_LINK_BROKEN_ALARM ){

      if( rda_config == ORPGRDA_ORDA_CONFIG ){

         if( (CR_ORDA_status.status_msg.rda_alarm & AS_RPG_COMMUN) )
            CR_ORDA_status.status_msg.rda_alarm &= ~AS_RPG_COMMUN;

      }
      else if( rda_config == ORPGRDA_LEGACY_CONFIG ){

         if( (CR_ORDA_status.status_msg.rda_alarm & AS_RPG_COMMUN) )
            CR_ORDA_status.status_msg.rda_alarm &= ~AS_RPG_COMMUN;

      }

   }

   /* Update RDA status. */
   ST_update_rda_status( (int) 1, /* post rda status changed event */
                         NULL,
                         (int) RPG_INITIATED );

} /* End of ST_clear_communications_alarm() */

/*///////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Clears all alarm codes in the RDA status data, and clears the
//      RDA alarm summary.
//
//   Inputs:
//
//   Returns:
//
/////////////////////////////////////////////////////////////////////////////\*/
void ST_clear_rda_alarm_codes(){

   int rda_config = ST_get_rda_config( NULL );
   int i;
   
   /* Process ORDA configuration. */
   if( rda_config == ORPGRDA_ORDA_CONFIG ){

      /* Clear the alarm code table. */
      for( i = 0; i < MAX_RDA_ALARMS_PER_MESSAGE; i++ )
         CR_ORDA_status.status_msg.alarm_code[i] = 0;

      /* Clear the alarm summary. */
      CR_ORDA_status.status_msg.rda_alarm = 0;

   }

   /* Process Legacy configuration. */
   else{

      /* Clear the alarm code table. */
      for( i = 0; i < MAX_RDA_ALARMS_PER_MESSAGE; i++ )
        CR_RDA_status.status_msg.alarm_code[i] = 0;

      /* Clear the alarm summary. */
      CR_RDA_status.status_msg.rda_alarm = 0;

   }

}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:
//      Writes to the RDA status Linear Buffer, and posts event
//      ORPGEVT_RDA_STATUS_CHANGE if LB write was successful and
//      post status change flag is set.
//
//      The RDA ALARMS Linear Buffer is updated if status message
//      contains any RDA alarms.
//
//   Inputs:
//      post_status_change_event - flag to indicate whether event
//                                 should be posted indicating RDA
//                                 status has changed.
//      msg_ptr - pointer to RDA Status message (includes message
//                header) or NULL if no message passed.
//      initiator - indicates whether the status update is RDA or
//                  RPG initiated.
//
//   Outputs:
//
//   Returns:
//      ST_FAILURE on error; otherwise, returns ST_SUCCESS.
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
int ST_update_rda_status( int post_status_change_event,
                          char *msg_ptr,
                          int initiator ){


   int                          ret                     = 0;
   static unsigned short        seq_num                 = 0;
   unsigned short               julian_date             = 0;
   int                          time_of_day_ms          = 0;
   int                          rda_config              = ORPGRDA_ORDA_CONFIG;
   short                        neg_seq_num             = 0;
   Redundant_info_t             red_info;

   static int			prev_channel_num	= 1;

   /* First need to identify the RDA configuration. */
   rda_config = ST_get_rda_config( msg_ptr );

   /* If this is an NWS Redundant Configuration, get the channel number. */
   if( (msg_ptr != NULL) 
               && 
       (ORPGSITE_get_redundant_data( &red_info ) >= 0) ){

      if( red_info.redundant_type == ORPGSITE_NWS_REDUNDANT ){
         
         RDA_RPG_message_header_t *msg_hdr = (RDA_RPG_message_header_t *) msg_ptr;

         /* Determine the RDA channel number. */
         if( msg_hdr->rda_channel & 0x1 )
            CR_channel_num = 1;

         else if (msg_hdr->rda_channel & 0x2 )
            CR_channel_num = 2;

         if( CR_channel_num != prev_channel_num )
            LE_send_msg( GL_INFO, "CR_channel_num Changed From %d To %d\n", 
                         prev_channel_num, CR_channel_num );

         prev_channel_num = CR_channel_num;

      }

   }

   /* Copy the current status to previous status before processing the 
      new status and making it current. */
   Copy_current_to_previous();

   /* If this is rpg initiated status data, need to set the sign
      bit on the sequence_num field to notify other processes. */
   if( initiator == RPG_INITIATED ){

      if( rda_config == ORPGRDA_ORDA_CONFIG ){

         seq_num++;
         /* Set the "sign bit" of the sequence number to indicate this
            status is "RPG initiated" */
         neg_seq_num = seq_num | 0x8000;
         CR_ORDA_status.status_msg.msg_hdr.sequence_num = neg_seq_num;

         SWM_get_date_time( (short *) &julian_date, (unsigned int *) &time_of_day_ms );
         CR_ORDA_status.status_msg.msg_hdr.julian_date = julian_date;

         CR_ORDA_status.status_msg.msg_hdr.num_segs = 1;
         CR_ORDA_status.status_msg.msg_hdr.seg_num = 1;

      }
      else if( rda_config == ORPGRDA_LEGACY_CONFIG ){

         seq_num++;
         /* Set the "sign bit" of the sequence number to indicate this
            status is "RPG initiated" */
         neg_seq_num = seq_num | 0x8000;
         CR_RDA_status.status_msg.msg_hdr.sequence_num = neg_seq_num;

         SWM_get_date_time( (short *) &julian_date, (unsigned int *) &time_of_day_ms );
         CR_RDA_status.status_msg.msg_hdr.julian_date = julian_date;

         CR_RDA_status.status_msg.msg_hdr.num_segs = 1;
         CR_RDA_status.status_msg.msg_hdr.seg_num = 1;

      }

   }

   if ( CR_verbose_mode )
      SCS_display_comms_line_status( );

   /* If RDA Status is passed (should only be passed if initiator is RDA_INITIATED), 
      then copy status data to current. */
   if( (initiator == RDA_INITIATED) && (msg_ptr != NULL) ){

      /* Copy the status message to local buffer. */
      if( rda_config == ORPGRDA_ORDA_CONFIG )
         memcpy( &CR_ORDA_status.status_msg, msg_ptr, sizeof(ORDA_status_msg_t) );

      else if( rda_config == ORPGRDA_LEGACY_CONFIG )
         memcpy( &CR_RDA_status.status_msg, msg_ptr, sizeof(RDA_status_msg_t) );

   }

   /* Write new status data to status LB. */
   ST_write_status_msg();

   /* Update RDA alarm table. */
   if( rda_config == ORPGRDA_ORDA_CONFIG )
      Update_rda_alarm_table( (char *) &CR_ORDA_status.status_msg, rda_config );

   else
      Update_rda_alarm_table( (char *) &CR_RDA_status.status_msg, rda_config );

   /* Do we need to post an event indicating the status has changed? */
   if( post_status_change_event ){

      /* Post status changed event. */
      if( (ret = ES_post_event( ORPGEVT_RDA_STATUS_CHANGE, (char *) &initiator,
                                sizeof(int), (int) 0 )) != ES_SUCCESS )
         return( ST_FAILURE );

   }

   /* Return normal. */
   return ( ST_SUCCESS );

} /* End of ST_update_rda_status() */

/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Sets the desired RDA status field (argument 1) to the desired
//      value (argument 2).
//
//   Inputs:
//      field_id - The status field to be set. 
//      value - The value to set status field to.
//
//   Return:
//      ST_FAILURE on error, or ST_SUCCESS otherwise.
//
/////////////////////////////////////////////////////////////////////////////\*/
int ST_set_status ( int field_id, int value ){

   int  status  = ST_SUCCESS;
   int rda_config = ST_get_rda_config( NULL );

   if ( rda_config == ORPGRDA_LEGACY_CONFIG )
      status = Set_rda_status( field_id, value );
    
   else if ( rda_config == ORPGRDA_ORDA_CONFIG )
      status = Set_orda_status( field_id, value );
    
   return status;

/* End ST_set_status() */
} 

/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Gets the desired RDA status field.
//
//   Inputs:
//      field_id - The status field to be get.
//
//   Return:
//      ST_FAILURE on error, or ST_SUCCESS otherwise.
//
/////////////////////////////////////////////////////////////////////////////\*/
int ST_get_status ( int field_id ){

   int  status  = ST_SUCCESS;
   int rda_config = ST_get_rda_config( NULL );

   if ( rda_config == ORPGRDA_LEGACY_CONFIG )
      status = Get_rda_status( field_id );

   else if ( rda_config == ORPGRDA_ORDA_CONFIG )
      status = Get_orda_status( field_id );

   return status;

/* End ST_get_status() */
}


/*\//////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Returns the RDA configuration.
//
//   Inputs:   
//
//   Outputs:
//
//   Returns:
//      The RDA configuration.
//
/////////////////////////////////////////////////////////////////////\*/
int ST_get_rda_config( void *msg ){

   int rda_config = ORPGRDA_get_rda_config( msg );

   if( (rda_config != ORPGRDA_ORDA_CONFIG)
                   &&
       (rda_config != ORPGRDA_LEGACY_CONFIG))
      rda_config = ORPGRDA_ORDA_CONFIG;

   return rda_config;

/* End of ST_get_rda_config() */
}

/*\//////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Writes the RDA Status Message.
//
//   Returns:
//      Status of the write operation.
//
/////////////////////////////////////////////////////////////////////\*/
int ST_write_status_msg(){

   int ret = 0;
   int rda_config = ST_get_rda_config( NULL );

   if( rda_config == ORPGRDA_ORDA_CONFIG )
      ret = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) &CR_ORDA_status,
                          sizeof(ORDA_status_t), RDA_STATUS_ID );

   else if( rda_config == ORPGRDA_LEGACY_CONFIG )
      ret = ORPGDA_write( ORPGDAT_GSM_DATA, (char *) &CR_RDA_status,
                          sizeof(RDA_status_t), RDA_STATUS_ID );

   return ret;

/* End of ST_write_status_msg() */
} 

/*//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Writes Legacy RDA status data to system status log file in plain text 
//      format.  Only status which is different than last reported is reported.
//
//   Inputs:
//      rda_status - pointer to RDA status message.
//
//   Returns:
//      void
//
/////////////////////////////////////////////////////////////////////////////\*/
static void Rda_update_system_status( RDA_status_msg_t *rda_status,
                                      RDA_status_msg_t *prev_rda_status ){

   int		stat			= 0;
   int		hw			= 0;
   int		len			= 0;
   int		rda_stat 		= 0;
   int		op_stat 		= 0;
   int		control_stat 		= 0;
   int		aux_pwr_stat 		= 0;
   int		data_trans_enab 	= 0;
   int		vcp 			= 0;
   int		rda_contr_auth 		= 0;
   int		opmode 			= 0;
   int		int_supp_unit 		= 0;
   int		arch_II_stat 		= 0;
   int		chan_stat 		= 0;
   int		spot_blank_stat 	= 0;
   int		tps_stat 		= 0;
   int		p_rda_stat 		= 0;
   int		p_op_stat 		= 0;
   int		p_control_stat 		= 0;
   int		p_aux_pwr_stat 		= 0;
   int		p_data_trans_enab 	= 0;
   int		p_vcp 			= 0;
   int		p_rda_contr_auth 	= 0;
   int		p_opmode 		= 0;
   int		p_int_supp_unit 	= 0;
   int		p_arch_II_stat 		= 0;
   int		p_chan_stat 		= 0;
   int		p_spot_blank_stat 	= 0;
   int		p_tps_stat 		= 0;
   char*	buf			= NULL;
   double	deau_ret_val		= 0.0;


   /* Get at status buffer. */
   buf = calloc( (MAX_STATUS_LENGTH+1), sizeof(char) );
   if( buf == NULL ){

      LE_send_msg( GL_INFO, "Unable to Process RDA Status Log Message\n" );
      return;

   }

   /* Place header string in buffer. */
   strcat( buf, "RDA STATUS:" );
   len = strlen( buf );

   rda_stat = rda_status->rda_status;
   op_stat = rda_status->op_status;
   control_stat = rda_status->control_status;
   aux_pwr_stat = rda_status->aux_pwr_state;
   data_trans_enab = rda_status->data_trans_enbld;
   vcp = rda_status->vcp_num;
   rda_contr_auth = rda_status->rda_control_auth;
   opmode = rda_status->op_mode;
   int_supp_unit = rda_status->int_suppr_unit;
   arch_II_stat = rda_status->arch_II_status;
   chan_stat = rda_status->channel_status;
   spot_blank_stat = rda_status->spot_blanking_status;
   tps_stat = rda_status->tps_status;

   /* Retrieve and store previous rda status fields */
   p_rda_stat = prev_rda_status->rda_status;
   p_op_stat = prev_rda_status->op_status;
   p_control_stat = prev_rda_status->control_status;
   p_aux_pwr_stat = prev_rda_status->aux_pwr_state;
   p_data_trans_enab = prev_rda_status->data_trans_enbld;
   p_vcp = prev_rda_status->vcp_num;
   p_rda_contr_auth = prev_rda_status->rda_control_auth;
   p_opmode = prev_rda_status->op_mode;
   p_int_supp_unit = prev_rda_status->int_suppr_unit;
   p_arch_II_stat = prev_rda_status->arch_II_status;
   p_chan_stat = prev_rda_status->channel_status;
   p_spot_blank_stat = prev_rda_status->spot_blanking_status;
   p_tps_stat = prev_rda_status->tps_status;

   /* Output status data is readable format. */
   for( hw = 0; hw <= STATUS_WORDS; hw++ ){

      switch( hw ){

         case RS_RDA_STATUS:
         {
            int i;

            LE_send_msg( GL_INFO, "--->RDA STATUS:  %x\n", rda_stat);

            /* If status has changed, process new value; */
            if( rda_stat == p_rda_stat )
               break;

            /* Process status. */
            if( (rda_stat & RS_STARTUP) )
               i = 0;

            else if( (rda_stat & RS_STANDBY) )
               i = 1;

            else if( (rda_stat & RS_RESTART) )
               i = 2;

            else if( (rda_stat & RS_OPERATE) )
               i = 3;

            else if( (rda_stat & RS_PLAYBACK) )
               i = 4;

            else if( (rda_stat & RS_OFFOPER) )
               i = 5;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 6;
               LE_send_msg( GL_INFO, "Unknown RDA Status: %d\n", rda_stat );

            }

            Process_status_field( &buf, &len, "Stat=", status[i] );

            break;
         }

         case RS_OPERABILITY_STATUS:
         {

            unsigned short rda_operability, auto_cal_disabled;
            int i;

            LE_send_msg( GL_INFO, "--->OPERABILITY STATUS:  %x\n", op_stat );

            /* If operability status has changed, process new value; */
            if( op_stat == p_op_stat )
               break;

            /* Process operability status. */
            auto_cal_disabled = 
                     (unsigned short) (op_stat & 0x01);
            rda_operability = 
                     (unsigned short) (op_stat & 0xfffe);

            if( (rda_operability & OS_ONLINE) )
               i = 0;

            else if( (rda_operability & OS_MAINTENANCE_REQ) )
               i = 1;

            else if( (rda_operability & OS_MAINTENANCE_MAN) )
               i = 2;

            else if( (rda_operability & OS_COMMANDED_SHUTDOWN) )
               i = 3;

            else if( (rda_operability & OS_INOPERABLE) )
               i = 4;

            else if( (rda_operability & OS_WIDEBAND_DISCONNECT) )
               i = 5;

            else
            {
               /* Unknown value.  Place value in status buffer. */
               i = 6;
               LE_send_msg( GL_INFO, "Unknown RDA Operability Status: %d\n", 
                            rda_operability );
            }

            if( auto_cal_disabled )
            {
               char *temp = calloc( strlen( operability[i] ) + strlen("/ACD" + 1), 
                                    sizeof(char) );
               if ( temp == NULL )
               {
                  if( buf != NULL )
                     free(buf);

                  return;
               }

               strcat( temp, operability[i] );
               strcat( temp, "/ACD" );

               Process_status_field( &buf, &len, "Oper=", temp );
               free( temp );

            }
            else
            {
               Process_status_field( &buf, &len, "Oper=", operability[i] );
            }

            break;
         }

         case RS_CONTROL_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->CONTROL STATUS:  %x\n", control_stat );

            /* If control status has changed, process new value; */
            if( control_stat == p_control_stat )
               break;

            /* Process Control Status. */
            if( (control_stat & CS_LOCAL_ONLY) )
               i = 0;

            else if( (control_stat & CS_RPG_REMOTE) )
               i = 1;

            else if( (control_stat & CS_EITHER) )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 3;
               LE_send_msg( GL_INFO, "Unknown RDA Control State: %d\n", control_stat );

            }

            Process_status_field( &buf, &len, "Cntl=", control[i] );

            break;

         }
         case RS_AUX_POWER_GEN_STATE: 
         {

            int i;
            short test_bit, curr_bit, prev_bit;
            char temp[MAX_STATUS_LENGTH];

            LE_send_msg( GL_INFO, "--->AUX PWR STATE:  %x\n", aux_pwr_stat );

            /* If auxilliary power source/generator status has changed, 
               process new value; */
            if( aux_pwr_stat == p_aux_pwr_stat )
               break;

            /* Clear out status buffer. */
            memset( temp, 0, MAX_STATUS_LENGTH ); 

            /* Check which power bits have changed. */
            for( i = 0; i < MAX_PWR_BITS; i++ ){

               test_bit = 1 << i;
               curr_bit = (aux_pwr_stat & test_bit);
               prev_bit = (p_aux_pwr_stat & test_bit);
               if( curr_bit == prev_bit )
                  continue;

               /* If bit is set. */
               if( curr_bit )
                  strcat( temp, set_aux_pwr[i] );

               else if( (i != COMSWITCH_BIT)
                                 &&
                        (strlen(reset_aux_pwr[i]) > 0) )
                  strcat( temp, reset_aux_pwr[i] );
                  
            }

            Process_status_field( &buf, &len, "", temp );

            break;
         }

         case RS_DATA_TRANS_ENABLED:
         {

            char moment_string[10];

            LE_send_msg( GL_INFO, "--->DATA TRANS ENABLED:  %x\n", data_trans_enab );

            /* If moments have changed, process new value; */
            if( data_trans_enab == p_data_trans_enab )
               break;

            /* Process Moments. */
            moment_string[0] = '\0';
            if( data_trans_enab == BD_ENABLED_NONE )
               strcat( moment_string, moments[0] );

            else if( data_trans_enab == (BD_REFLECTIVITY | BD_VELOCITY | BD_WIDTH) )
               strcat( moment_string, moments[1] );

            else{

               if( (data_trans_enab & BD_REFLECTIVITY) )
                  strcat( moment_string, moments[2] );

               if( (data_trans_enab & BD_VELOCITY) ){ 

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[3] );
    
               }

               if( (data_trans_enab & BD_WIDTH) ){ 

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[4] );
    
               }

            }

            Process_status_field( &buf, &len, "Data=", moment_string );

            break;
         }

         case RS_VCP_NUMBER:
         {
            short temp_vcp = vcp;
            char temp[10];

            LE_send_msg( GL_INFO, "--->VCP NUMBER:  %x\n", temp_vcp );

            /* If VCP number has changed, process new value. */
            if( temp_vcp == p_vcp )
               break;

            /* Clear temporary buffer. */
            memset( temp, 0, 10 );

            /* Determine if vcp is "local" or "remote" pattern. */
            if( temp_vcp < 0 )
            {
               temp_vcp = -vcp;
               temp[0] = 'L';
            }
            else
            {
               temp[0] = 'R';
            }

            /* Encode VCP number. */
            sprintf( &temp[1], "%d", temp_vcp );

            Process_status_field( &buf, &len, "VCP=", temp );

            break;
         }

         case RS_RDA_CONTROL_AUTH:
         {
            int i;

            LE_send_msg( GL_INFO, "--->RDA CONTROL AUTH:  %x\n", rda_contr_auth );

            /* If RDA control authority has changed, process new value; */
            if( rda_contr_auth == p_rda_contr_auth )
               break;

            if( rda_contr_auth == CA_NO_ACTION )
               break;

            if( rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
               i = 0;

            else if( rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown RDA Control Authority: %d\n", 
                            rda_contr_auth );

            }

            Process_status_field( &buf, &len, "Auth=", authority[i] );

            break;
         }

         case RS_OPERATIONAL_MODE:
         {

            int i;

            LE_send_msg( GL_INFO, "--->OPERATIONAL MODE:  %x\n", opmode );

            /* If operational mode has changed, process new value; */
            if( opmode == p_opmode )
               break;

            /* Process operational mode. */
            if( opmode == OP_MAINTENANCE_MODE )
               i = 1;

            else if( opmode == OP_OPERATIONAL_MODE )
               i = 0;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown Operational Mode: %d\n", opmode );

            }

            Process_status_field( &buf, &len, "Mode=", mode[i] );

            break;
         }

         case RS_ISU:
         {
            int i;

            LE_send_msg( GL_INFO, "--->ISU:  %x\n", int_supp_unit );

            /* If interference unit status has changed, process new value; */
            if( int_supp_unit == p_int_supp_unit )
               break;

            /* Process interference unit. */ 
            if( int_supp_unit == ISU_NOCHANGE )
               break;

            else if( int_supp_unit == ISU_ENABLED )
               i = 0;

            else if( int_supp_unit == ISU_DISABLED )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown ISU Value: %d\n", int_supp_unit );

            }

            Process_status_field( &buf, &len, "ISU=", interference_unit[i] );

            break;

         }

         case RS_ARCHIVE_II_STATUS:
         {
            int playback_avail = 0;
            short tape_num, tape_not_0;
            char tape_num_string[3], temp[MAX_STATUS_LENGTH];

            LE_send_msg( GL_INFO, "--->ARCHIVE II STATUS:  %x\n", arch_II_stat );

            /* If archive II status has changed, process new value. */
            if( arch_II_stat == p_arch_II_stat )
               break;

            /* Set the tape number. */
            memset( tape_num_string, 0, 3 );
            tape_num = (arch_II_stat & TAPE_NUM_MASK) >> TAPE_NUM_SHIFT;
            if( tape_num == 0 ){

               strcat( tape_num_string, "  " );
               tape_not_0 = 0;
              
            }
            else if( tape_num < 10 ){

               sprintf( tape_num_string, "%2.2d", tape_num );
               tape_not_0 = 1;

            }
            else{

               sprintf( tape_num_string, "%2d", tape_num );
               tape_not_0 = 1;

            }
            memset( temp, 0, MAX_STATUS_LENGTH );

            /* If PLAYBACK, then.... */
            if( (arch_II_stat & AR_PLAYBACK) ){

               playback_avail = 0;
               strcat( temp, archive_status[7] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );
 
               }
               
            }
            else if( (arch_II_stat & AR_PLAYBACK_AVAIL) )
                playback_avail = 1;
   
            /* Process other status. */
            if( (arch_II_stat & AR_TAPE_TRANSFER) ){

               strcat( temp, archive_status[10] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_CHECK_LABEL) ){

               strcat( temp, archive_status[8] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_FAST_FRWRD) )
            {
               strcat( temp, archive_status[9] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_SEARCH) )
            {
               strcat( temp, archive_status[6] ); 
               if( tape_not_0 ){

                  strcat( temp, tape_num_string );
                  strcat( temp, archive_status[11] );

               }

            }
            else if( (arch_II_stat & AR_RECORD) )
            {
               strcat( temp, archive_status[3] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_RESERVED) )
            {
               strcat( temp, archive_status[2] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_INSTALLED) )
            {
               strcat( temp, archive_status[0] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            else if( (arch_II_stat & AR_LOADED) )
            {
               strcat( temp, archive_status[1] ); 
               if( tape_not_0 ){

                  strcat( temp, archive_status[11] );
                  strcat( temp, tape_num_string );

               }

            }
            /* Is Archive II installed? */
            else if( (arch_II_stat == AR_NOT_INSTALLED) )
               strcat( temp, archive_status[4] ); 

            /* We do what legacy does ... If no bits are defined,
               it is assumed Archive II is not Installed. */
            else if( !(arch_II_stat & AR_PLAYBACK) )
               strcat( temp, archive_status[4] );                  

            if( playback_avail ){

               if( strlen( temp ) )
                  strcat( temp, ", ");
               strcat( temp, archive_status[5] );

            }

            Process_status_field( &buf, &len, "ArchII=", temp );

            break;

         }

         case RS_CHAN_CONTROL_STATUS:
         {

            int i = 0;

            LE_send_msg( GL_INFO, "--->CHANNEL CONTROL STATUS:  %x\n", chan_stat );

            /* If channel control status has changed, process new value; */
            if( chan_stat == p_chan_stat )
               break;

            /* Process channel control status if FAA Redundant. */ 
            if ( (stat =
              DEAU_get_values("Redundant_info.redundant_type", &deau_ret_val, 1))
              >= 0)
            {
               if( (int) deau_ret_val != ORPGSITE_FAA_REDUNDANT )
                  break;
               else if( chan_stat == RDA_IS_CONTROLLING )
                  i = 0;
               else if( chan_stat == RDA_IS_NON_CONTROLLING )
                  i = 1;
               else
               {
                  /* Unknown value.  Place value in status buffer. */
                  i = 2;
                  LE_send_msg( GL_INFO, "Unknown Channel Status: %d\n", chan_stat );
               }
            }
            else
            {
              LE_send_msg( GL_INFO | GL_ERROR,
               "Rda_update_system_status: call to DEAU_get_values returned error.\n");
            }

            Process_status_field( &buf, &len, "Chan Ctl=", channel_status[i] );

            break;
         }

         case RS_SPOT_BLANKING_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->SPOT BLANKING STATUS:  %x\n", spot_blank_stat );

            /* If spot blanking status has changed, process new value; */
            if( spot_blank_stat == p_spot_blank_stat )
               break;

            /* If spot blanking not installed, break. */
            if( spot_blank_stat == SB_NOT_INSTALLED )
               break;

            /* Process spot blanking status. */ 
            if( spot_blank_stat == SB_ENABLED )
               i = 1;

            else if( spot_blank_stat == SB_DISABLED )
               i = 2;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 3;
               LE_send_msg( GL_INFO, "Unknown Spot Blanking Status: %d\n", 
                            spot_blank_stat );

            }

            Process_status_field( &buf, &len, "SB=", spot_blanking[i] );
            break;

         }
         case RS_TPS_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->TPS STATUS:  %x\n", tps_stat );

            /* If TPS status has changed, process new value; */
            if( tps_stat == p_tps_stat )
               break;

            /* If TPS not installed, break. */
            if( tps_stat == TP_NOT_INSTALLED )
               break;

            /* Process TPS status. */ 
            if( tps_stat == TP_OFF )
               i = 0;

            else if( tps_stat == TP_OK )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown TPS Value: %d\n", tps_stat );

            }

            Process_status_field( &buf, &len, "TPS=", tps[i] );
            break;

         }
         default:
            break;

      /* End of "switch" statement. */
      }

   /* End of "for" loop. */
   }

   /* Write out last status message. */
   if( (buf != NULL) && (strcmp( buf, "RDA STATUS:" ) != 0) ){

      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", buf );
   }
   if (buf != NULL) {
       free(buf);
   }

} /* End of Rda_update_system_status() */


/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Writes Open RDA status data to system status log file in plain text 
//      format.  Only status which is different than last reported is reported.
//
//   Inputs:
//      rda_status - ORDA Status message.
//
//   Returns:
//      void
//
*******************************************************************************/
static void Orda_update_system_status( ORDA_status_msg_t *rda_status,
                                       ORDA_status_msg_t *prev_rda_status ){

   int		stat			= 0;
   int		hw			= 0;
   int		len			= 0;
   int		rda_stat 		= 0;
   int		op_stat 		= 0;
   int		control_stat 		= 0;
   int		aux_pwr_stat 		= 0;
   int		data_trans_enab 	= 0;
   int		vcp 			= 0;
   int		rda_contr_auth 		= 0;
   int		opmode 			= 0;
   int		chan_stat 		= 0;
   int		spot_blank_stat 	= 0;
   int		tps_stat 		= 0;
   int		sr_status		= 0;
   int		cmd_status		= 0;
   int		avset_status		= 0;
   int          perf_check_status       = 0;
   int		p_rda_stat 		= 0;
   int		p_op_stat 		= 0;
   int		p_control_stat 		= 0;
   int		p_aux_pwr_stat 		= 0;
   int		p_data_trans_enab 	= 0;
   int		p_vcp 			= 0;
   int		p_rda_contr_auth 	= 0;
   int		p_opmode 		= 0;
   int		p_chan_stat 		= 0;
   int		p_spot_blank_stat 	= 0;
   int		p_tps_stat 		= 0;
   int		p_sr_status		= 0;
   int		p_cmd_status		= 0;
   int		p_avset_status		= 0;
   int          p_perf_check_status     = 0;
   char*	buf			= NULL;
   double	deau_ret_val		= 0.0;


   /* Get at status buffer. */
   buf = calloc( (MAX_STATUS_LENGTH+1), sizeof(char) );
   if( buf == NULL ){

      LE_send_msg( GL_INFO, "Unable to Process RDA Status Log Message\n" );
      return;

   }

   /* Place header string in buffer. */
   strcat( buf, "RDA STATUS:" );
   len = strlen( buf );

   rda_stat = rda_status->rda_status;
   op_stat = rda_status->op_status;
   control_stat = rda_status->control_status;
   aux_pwr_stat = rda_status->aux_pwr_state;
   data_trans_enab = rda_status->data_trans_enbld;
   vcp = rda_status->vcp_num;
   rda_contr_auth = rda_status->rda_control_auth;
   opmode = rda_status->op_mode;
   chan_stat = rda_status->channel_status;
   spot_blank_stat = rda_status->spot_blanking_status;
   tps_stat = rda_status->tps_status;
   sr_status = rda_status->super_res;

   /* Bit 0 is the enabled/disabled bit. */
   cmd_status = rda_status->cmd & 0x1;

   avset_status = rda_status->avset;
   perf_check_status = rda_status->perf_check_status;

   /* Retrieve and store previous rda status fields */
   p_rda_stat = prev_rda_status->rda_status;
   p_op_stat = prev_rda_status->op_status;
   p_control_stat = prev_rda_status->control_status;
   p_aux_pwr_stat = prev_rda_status->aux_pwr_state;
   p_data_trans_enab = prev_rda_status->data_trans_enbld;
   p_vcp = prev_rda_status->vcp_num;
   p_rda_contr_auth = prev_rda_status->rda_control_auth;
   p_opmode = prev_rda_status->op_mode;
   p_chan_stat = prev_rda_status->channel_status;
   p_spot_blank_stat = prev_rda_status->spot_blanking_status;
   p_tps_stat = prev_rda_status->tps_status;
   p_sr_status = prev_rda_status->super_res;

   /* Bit 0 is the enabled/disabled bit. */
   p_cmd_status = prev_rda_status->cmd & 0x1;

   p_avset_status = prev_rda_status->avset;
   p_perf_check_status = prev_rda_status->perf_check_status;

   /*
      Output status data is readable format. NOTE: range for hw should start at
      the value of the macro representing the first field in the status msg
      (defined in rda_status.h)
   */
   for( hw = 1; hw <= STATUS_WORDS; hw++ ){

      switch( hw ){

         case RS_RDA_STATUS:
         {
            int i;

            LE_send_msg( GL_INFO, "--->RDA STATUS:  %x\n", rda_stat);

            /* If status has changed, process new value; */
            if( rda_stat == p_rda_stat )
               break;

            /* Process status. */
            if( (rda_stat & RS_STARTUP) )
               i = 0;

            else if( (rda_stat & RS_STANDBY) )
               i = 1;

            else if( (rda_stat & RS_RESTART) )
               i = 2;

            else if( (rda_stat & RS_OPERATE) )
               i = 3;

            else if( (rda_stat & RS_PLAYBACK) )
               i = 4;

            else if( (rda_stat & RS_OFFOPER) )
               i = 5;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 6;
               LE_send_msg( GL_INFO, "Unknown RDA Status: %d\n", rda_stat );

            }

            Process_status_field( &buf, &len, "Stat=", status[i] );

            break;
         }

         case RS_OPERABILITY_STATUS:
         {

            unsigned short rda_operability;
            int i;

            LE_send_msg( GL_INFO, "--->OPERABILITY STATUS:  %x\n", op_stat );

            /* If operability status has changed, process new value; */
            if( op_stat == p_op_stat )
               break;

            /* Process operability status. */
            rda_operability = (unsigned short) op_stat;

            if( (rda_operability & OS_ONLINE) )
               i = 0;

            else if( (rda_operability & OS_MAINTENANCE_REQ) )
               i = 1;

            else if( (rda_operability & OS_MAINTENANCE_MAN) )
               i = 2;

            else if( (rda_operability & OS_COMMANDED_SHUTDOWN) )
               i = 3;

            else if( (rda_operability & OS_INOPERABLE) )
               i = 4;

            else if( (rda_operability & OS_WIDEBAND_DISCONNECT) )
               i = 5;

            else
            {
               /* Unknown value.  Place value in status buffer. */
               i = 6;
               LE_send_msg( GL_INFO, "Unknown RDA Operability Status: %d\n", 
                            rda_operability );
            }

            Process_status_field( &buf, &len, "Oper=", operability[i] );

            break;
         }

         case RS_CONTROL_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->CONTROL STATUS:  %x\n", control_stat );

            /* If control status has changed, process new value; */
            if( control_stat == p_control_stat )
               break;

            /* Process Control Status. */
            if( (control_stat & CS_LOCAL_ONLY) )
               i = 0;

            else if( (control_stat & CS_RPG_REMOTE) )
               i = 1;

            else if( (control_stat & CS_EITHER) )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 3;
               LE_send_msg( GL_INFO, "Unknown RDA Control State: %d\n", 
                            control_stat );

            }

            Process_status_field( &buf, &len, "Cntl=", control[i] );

            break;

         }
         case RS_AUX_POWER_GEN_STATE: 
         {

            int i;
            short test_bit, curr_bit, prev_bit;
            char temp[MAX_STATUS_LENGTH];

            LE_send_msg( GL_INFO, "--->AUX PWR STATE:  %x\n", aux_pwr_stat );

            /* If auxilliary power source/generator status has changed, 
               process new value; */
            if( aux_pwr_stat == p_aux_pwr_stat )
               break;

            /* Clear out status buffer. */
            memset( temp, 0, MAX_STATUS_LENGTH ); 

            /* Check which power bits have changed. */
            for( i = 0; i < MAX_PWR_BITS; i++ ){

               test_bit = 1 << i;
               curr_bit = (aux_pwr_stat & test_bit);
               prev_bit = (p_aux_pwr_stat & test_bit);
               if( curr_bit == prev_bit )
                  continue;

               /* If bit is set. */
               if( curr_bit )
                  strcat( temp, set_aux_pwr[i] );

               else if( (i != COMSWITCH_BIT)
                                 &&
                        (strlen(reset_aux_pwr[i]) > 0) )
                  strcat( temp, reset_aux_pwr[i] );
                  
            }

            Process_status_field( &buf, &len, "", temp );

            break;
         }

         case RS_DATA_TRANS_ENABLED:
         {

            char moment_string[10];

            LE_send_msg( GL_INFO, "--->DATA TRANS ENABLED:  %x\n", data_trans_enab );

            /* If moments have changed, process new value; */
            if( data_trans_enab == p_data_trans_enab )
               break;

            /* Process Moments. */
            moment_string[0] = '\0';
            if( data_trans_enab == BD_ENABLED_NONE )
               strcat( moment_string, moments[0] );

            else if( data_trans_enab == (BD_REFLECTIVITY | BD_VELOCITY | BD_WIDTH) )
               strcat( moment_string, moments[1] );

            else{

               if( (data_trans_enab & BD_REFLECTIVITY) )
                  strcat( moment_string, moments[2] );

               if( (data_trans_enab & BD_VELOCITY) ){ 

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[3] );
    
               }

               if( (data_trans_enab & BD_WIDTH) ){ 

                  if( strlen( moment_string ) != 0 )
                     strcat( moment_string, "/" );
                  strcat( moment_string, moments[4] );
    
               }

            }

            Process_status_field( &buf, &len, "Data=", moment_string );

            break;
         }

         case RS_VCP_NUMBER:
         {
            short temp_vcp = vcp;
            char temp[10];

            LE_send_msg( GL_INFO, "--->VCP NUMBER:  %x\n", temp_vcp );

            /* If VCP number has changed, process new value. */
            if( temp_vcp == p_vcp )
               break;

            /* Clear temporary buffer. */
            memset( temp, 0, 10 );

            /* Determine if vcp is "local" or "remote" pattern. */
            if( temp_vcp < 0 )
            {
               temp_vcp = -vcp;
               temp[0] = 'L';
            }
            else
            {
               temp[0] = 'R';
            }

            /* Encode VCP number. */
            sprintf( &temp[1], "%d", temp_vcp );

            Process_status_field( &buf, &len, "VCP=", temp );

            break;
         }

         case RS_RDA_CONTROL_AUTH:
         {
            int i;

            LE_send_msg( GL_INFO, "--->RDA CONTROL AUTH:  %x\n", rda_contr_auth );

            /* If RDA control authority has changed, process new value; */
            if( rda_contr_auth == p_rda_contr_auth )
               break;

            if( rda_contr_auth == CA_NO_ACTION )
               break;

            if( rda_contr_auth & CA_LOCAL_CONTROL_REQUESTED )
               i = 0;

            else if( rda_contr_auth & CA_REMOTE_CONTROL_ENABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown RDA Control Authority: %d\n", 
                            rda_contr_auth );

            }

            Process_status_field( &buf, &len, "Auth=", authority[i] );

            break;
         }

         case RS_OPERATIONAL_MODE:
         {

            int i;

            LE_send_msg( GL_INFO, "--->OPERATIONAL MODE:  %x\n", opmode );

            /* If operational mode has changed, process new value
               OP_MAINTENANCE_MODE */
            if( opmode == p_opmode )
               break;

            /* Process operational mode. */
            if( opmode == OP_OPERATIONAL_MODE )
               i = 0;

            else if( opmode == OP_OFFLINE_MAINTENANCE_MODE )
               i = 2;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown Operational Mode: %d\n", opmode );

            }

            Process_status_field( &buf, &len, "Mode=", orda_mode[i] );

            break;
         }

         case RS_SUPER_RES:
         {

            int i;

            LE_send_msg( GL_INFO, "--->SUPER_RES STATUS:  %x\n", sr_status );

            /* If super res status has changed, process new value. */
            if( sr_status == p_sr_status )
               break;

            /* Process super res status. */
            if( sr_status == SR_ENABLED )
               i = 0;

            else if( sr_status == SR_DISABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown Super Resolution Status: %d\n",
                            sr_status );

            }

            Process_status_field( &buf, &len, "SR Status=", super_res[i] );

            break;
         }

         case RS_CMD:
         {

            int i;

            LE_send_msg( GL_INFO, "--->CMD STATUS:  %x\n", cmd_status );

            /* If CMD status has changed, process new value. */
            if( cmd_status == p_cmd_status )
               break;

            /* Process CMD status. */
            if( cmd_status == CMD_ENABLED )
               i = 0;

            else if( cmd_status == CMD_DISABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown CMD Status: %d\n",
                            cmd_status );

            }

            Process_status_field( &buf, &len, "CMD Status=", cmd[i] );

            break;
         }

         case RS_AVSET:
         {

            int i;

            LE_send_msg( GL_INFO, "--->AVSET STATUS:  %x\n", avset_status );

            /* If AVSET status has changed, process new value. */
            if( avset_status == p_avset_status )
               break;

            /* Process AVSET status. */
            if( avset_status == AVSET_ENABLED )
               i = 0;

            else if( avset_status == AVSET_DISABLED )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown AVSET Status: %d\n",
                            avset_status );

            }

            Process_status_field( &buf, &len, "AVSET Status=", avset[i] );

            break;

         }

         case RS_CHAN_CONTROL_STATUS:
         {

            int i = 0;

            LE_send_msg( GL_INFO, "--->CHANNEL CONTROL STATUS:  %x\n", chan_stat );

            /* If channel control status has changed, process new value; */
            if( chan_stat == p_chan_stat )
               break;

            if ( (stat =
              DEAU_get_values("Redundant_info.redundant_type", &deau_ret_val, 1))
              >= 0)
            {
               /* Process channel control status if FAA Redundant. */ 
               if( (int) deau_ret_val != ORPGSITE_FAA_REDUNDANT )
                  break;
               else if( chan_stat == RDA_IS_CONTROLLING )
                  i = 0;
               else if( chan_stat == RDA_IS_NON_CONTROLLING )
                  i = 1;
               else
               {
                  /* Unknown value.  Place value in status buffer. */
                  i = 2;
                  LE_send_msg( GL_INFO, "Unknown Channel Status: %d\n", chan_stat );
               }
            }
            else
            {
              LE_send_msg( GL_INFO | GL_ERROR,
               "Orda_update_system_status: call to DEAU_get_values returned error.\n");
            }

            Process_status_field( &buf, &len, "Chan Ctl=", channel_status[i] );

            break;
         }

         case RS_SPOT_BLANKING_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->SPOT BLANKING STATUS:  %x\n", spot_blank_stat );

            /* If spot blanking status has changed, process new value; */
            if( spot_blank_stat == p_spot_blank_stat )
               break;

            /* If spot blanking not installed, break. */
            if( spot_blank_stat == SB_NOT_INSTALLED )
               break;

            /* Process spot blanking status. */ 
            if( spot_blank_stat == SB_ENABLED )
               i = 1;

            else if( spot_blank_stat == SB_DISABLED )
               i = 2;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 3;
               LE_send_msg( GL_INFO, "Unknown Spot Blanking Status: %d\n", 
                            spot_blank_stat );

            }

            Process_status_field( &buf, &len, "SB=", spot_blanking[i] );
            break;

         }

         case RS_PERF_CHECK_STATUS:
         {
            int i;

            LE_send_msg( GL_INFO, "--->PERF CHECK STATUS:  %x\n", perf_check_status );

            /* If performance check status has changed, process new value. */
            if( perf_check_status == p_perf_check_status )
               break;

            /* Process performance check status. */
            if( perf_check_status == PC_AUTO )
               i = 0;

            else if( perf_check_status == PC_PENDING )
               i = 1;

            else{

               /* Unknown value.  Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown Performance Check Status: %d\n",
                            perf_check_status );

            }

            Process_status_field( &buf, &len, "Perf Check=", perf_check[i] );
            break;

         }

         case RS_TPS_STATUS:
         {

            int i;

            LE_send_msg( GL_INFO, "--->TPS STATUS:  %x\n", tps_stat );

            /* If TPS status has changed, process new value; */
            if( tps_stat == p_tps_stat )
               break;

            /* If TPS not installed, break. */
            if( tps_stat == TP_NOT_INSTALLED )
               break;

            /* Process TPS status. */ 
            if( tps_stat == TP_OFF )
               i = 0;

            else if( tps_stat == TP_OK )
               i = 1;

            else{

               /* Unknown value. Place value in status buffer. */
               i = 2;
               LE_send_msg( GL_INFO, "Unknown TPS Value: %d\n", tps_stat );

            }

            Process_status_field( &buf, &len, "TPS=", tps[i] );
            break;

         }
         default:
            break;

      /* End of "switch" statement. */
      }

   /* End of "for" loop. */
   }

   /* Write out last status message. */
   if( (buf != NULL) && (strcmp( buf, "RDA STATUS:" ) != 0) ){

      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", buf );
   }
   if (buf != NULL) {
       free(buf);
   }

/* End of Orda_update_system_status() */
} 

/*\///////////////////////////////////////////////////////////////////////////
//
//  Description:
//     Controls writing RDA status data to the system log file.  If status
//     to be written to status buffer causes the buffer to overflow, the buffer
//     is written to the status log before the new data is copied.
//
//  Inputs:
//     buf - address of status buffer.
//     len - address of length the status buffer string.
//     field_id - string containing the ID of the field to write to status
//                buffer.
//    field_val - string containing the field value.
//
//  Outputs:
//     buf - status buffer containing new status data.
//     len - length of the string with new status data.
//
//  Returns:
//     There is no return value define for this function.
//
///////////////////////////////////////////////////////////////////////////\*/
static void Process_status_field( char **buf, int *len, char *field_id, 
                                  char *field_val ){

   int comma_and_space;

   /* If there is previous status in the buffer, will need to
      append a comma and a space. */
   if( strcmp( *buf, "RDA STATUS:") != 0 )
      comma_and_space = 2;
   else
      comma_and_space = 0;

   /* Is the buffer large enoough to accommodate the new status. */
   if( (*len + strlen(field_id) + strlen(field_val) + comma_and_space) >
       MAX_STATUS_LENGTH )
   {
      /* Buffer not large enough.  Write the message to the status log. */
      LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", *buf );

      /* Initialize status buffer. */
      *buf = memset( *buf, 0, (MAX_STATUS_LENGTH+1) );

      /* Place header string in buffer. */
      strcat( *buf, "RDA STATUS:" );
      *len = strlen( *buf );
      comma_and_space = 0;
   }

   /* Append a comma and space, if required. */
   if( comma_and_space )
      strcat( *buf, ", " );

   /* Append status ID and status value to buffer. */
   strcat( *buf, field_id );
   strcat( *buf, field_val );

   /* Determine new length of status buffer. */
   *len = strlen( *buf );

/* End of Process_status_field() */
} 

/*\//////////////////////////////////////////////////////////////////////
//
//   Description:   
//      Sets the RDA Alarm Level bits for status log processing.
//
//   Inputs:   
//      alarm_type - pointer to where RDA Alarm Level is OR'd in.
//      alarm_state - RDA alarm state as defined in RDA Alarm Data.
//
//   Outputs:
//
//   Returns:
//      There are no return valus defined for this function.
//
//   Globals:
//
//   Notes:
//
/////////////////////////////////////////////////////////////////////\*/
static void Set_alarm_state( unsigned int *alarm_type, int alarm_state ){

   /* Set the RDA alarm level for this alarm. */
   switch( alarm_state ){

      case ORPGRDA_STATE_NOT_APPLICABLE:
         *alarm_type |= LE_RDA_AL_NOT_APP;
         break;

      case ORPGRDA_STATE_MAINTENANCE_MANDATORY:
         *alarm_type |= LE_RDA_AL_MAM;
         break;

      case ORPGRDA_STATE_MAINTENANCE_REQUIRED:
         *alarm_type |= LE_RDA_AL_MAR;
         break;

      case ORPGRDA_STATE_INOPERATIVE:
         *alarm_type |= LE_RDA_AL_INOP;
         break;

      case ORPGRDA_STATE_SECONDARY:
         *alarm_type |= LE_RDA_AL_SEC;
         break;

      default:
         *alarm_type |= LE_RDA_AL_SEC;
         break;

   }

/* End of Set_alarm_state() */
}


/*\////////////////////////////////////////////////////////////////
//
//   Description:
//      Writes the RDA alarm data to RDA ALARMS LB.
//
//   Inputs:
//      status - Pointer to RDA status message.
//
//   Outputs:
//
//   Returns:
//      There are no return values defined for this function.
//
////////////////////////////////////////////////////////////////\*/
static void Update_rda_alarm_table( void *status, int rda_config ){

   int year, month, day;
   int hour, minute, second;
   int alarm_ind, channel_num, ret, i;
   int millisec = 0;
   int jul_date = 0;
   time_t tm;
   RDA_alarm_t rda_alarm;
   int alarm_codes[MAX_RDA_ALARMS_PER_MESSAGE];

   /* Check RDA configuration and process accordingly ... */
   if( rda_config == ORPGRDA_ORDA_CONFIG ){

      ORDA_status_msg_t *rda_status = (ORDA_status_msg_t *) status;

      /* Get the milliseconds since midnight and julian date from the i
         message header. */
      millisec = rda_status->msg_hdr.milliseconds;
      jul_date = rda_status->msg_hdr.julian_date - 1;

      for ( alarm_ind = 0; alarm_ind < MAX_RDA_ALARMS_PER_MESSAGE; alarm_ind++ )
         alarm_codes[alarm_ind] = (int) rda_status->alarm_code[alarm_ind];

      channel_num = rda_status->msg_hdr.rda_channel & 0x03;

   }
   else{

      RDA_status_msg_t *rda_status = (RDA_status_msg_t *) status;

      /* Get the milliseconds since midnight and julian date from the i
         message header. */
      millisec = rda_status->msg_hdr.milliseconds;
      jul_date = rda_status->msg_hdr.julian_date - 1;

      for ( alarm_ind = 0; alarm_ind < MAX_RDA_ALARMS_PER_MESSAGE; alarm_ind++ )
         alarm_codes[alarm_ind] = (int) rda_status->alarm_code[alarm_ind];

      channel_num = rda_status->msg_hdr.rda_channel & 0x03;

   }

   /* Get the hour, minute, second and the year, month, and day the
      alarm was generated.  NOTE: The legacy Julian date reference
      year is one year earlier than the UNIX reference date.  The
      ST_get_status() function handles this by subtracting 1 from
      the date. */
   tm = (millisec / MILLS_PER_SEC) + (jul_date * SECS_PER_DAY);

   unix_time( &tm, &year, &month, &day, &hour, &minute, &second );

   /* Go through all alarms in the RDA status message. */
   for( i = 0; i < MAX_RDA_ALARMS_PER_MESSAGE; i++ ){

      if( alarm_codes[i] == 0 )
         continue;

      if( alarm_codes[i] < 0 )
         rda_alarm.code = 0;
       
      else
         rda_alarm.code = 1;

      rda_alarm.month = month;
      rda_alarm.day = day;
      rda_alarm.year = year;

      rda_alarm.hour = hour;
      rda_alarm.minute = minute;
      rda_alarm.second = second;

      rda_alarm.alarm = alarm_codes[i] & 0x7fff;

      /* Add the channel number from the message header. */
      rda_alarm.channel = channel_num;

      /* Write the alarm data structure to LB. */
      if ( (ret = ORPGDA_write( ORPGDAT_RDA_ALARMS, (char *) &rda_alarm, 
                                sizeof( RDA_alarm_t), LB_ANY )) < 0 )
         LE_send_msg( GL_ERROR, "ORPGDA_write( ORPGDA_RDA_ALARMS ) Failed (%d)\n", ret );
       
      else
         ret = ES_post_event( ORPGEVT_RDA_ALARMS_UPDATE, (char *) NULL, (size_t) 0,
                              (int) 0);
       
   } /* End of "for" loop */

} /* End of Update_rda_alarm_table() */

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Private function used for changing the values of the local RDA
//      status message store.
//
//   Inputs:
//      field - Int that indicates the field whose value is being changed.
//      val -  New value for the field.
//
//   Return:
//      status - ST_SUCCESS or ST_FAILURE
//
/////////////////////////////////////////////////////////////////////////\*/
static int Set_rda_status( int field, int val ){

   int status = ST_SUCCESS;

   switch ( field ){

      case ORPGRDA_RDA_STATUS :                 /* Halfword 1 */

         CR_RDA_status.status_msg.rda_status = (unsigned short) val;
         break;

      case ORPGRDA_OPERABILITY_STATUS :         /* Halfword 2 */

         CR_RDA_status.status_msg.op_status = (unsigned short) val;
         break;

      case ORPGRDA_RDA_ALARM_SUMMARY :          /* Halfword 15 */

         CR_RDA_status.status_msg.rda_alarm = (unsigned short) val;
         break;

      default :

         status = ST_FAILURE;
         break;
   }

   return status;

/* End Set_rda_status() */
} 

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Private function used for changing the values of the local ORDA
//      status message store.
//
//   Inputs:
//      field - Int that indicates the field whose value is being changed.
//      val -  New value for the field.
//
//   Return:
//      status - ST_SUCCESS or ST_FAILURE
//
/////////////////////////////////////////////////////////////////////////\*/
static int Set_orda_status( int field, int val ){

   int status = ST_SUCCESS;

   switch ( field ){

      case ORPGRDA_RDA_STATUS :                 /* Halfword 1 */

         CR_ORDA_status.status_msg.rda_status = (unsigned short) val;
         break;

      case ORPGRDA_OPERABILITY_STATUS :         /* Halfword 2 */

         CR_ORDA_status.status_msg.op_status = (unsigned short) val;
         break;

      case ORPGRDA_RDA_ALARM_SUMMARY :          /* Halfword 15 */

         CR_ORDA_status.status_msg.rda_alarm = (unsigned short) val;
         break;

      default :

         status = ST_FAILURE;
         break;
   }

   return status;

/* End Set_orda_status() */
} 

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Returns the specified status element.
//
//   Inputs:
//      element - Integer that indicates the element whose value is
//                being sought.
//
//   Return:
//      Value of the desired element.
//
/////////////////////////////////////////////////////////////////////////\*/
static int Get_rda_status ( int element ){

   int value = 0;

   switch (element){

      case RS_RDA_STATUS :                /* Halfword 1 */

         value = (int) CR_RDA_status.status_msg.rda_status;
         break;

      case RS_OPERABILITY_STATUS :        /* Halfword 2 */

          value = (int) CR_RDA_status.status_msg.op_status;
          break;

      case RS_CONTROL_STATUS :            /* Halfword 3 */

          value = (int) CR_RDA_status.status_msg.control_status;
          break;

      case RS_AUX_POWER_GEN_STATE :       /* Halfword 4 */

           value = (int) CR_RDA_status.status_msg.aux_pwr_state;
           break;

      case RS_AVE_TRANS_POWER :           /* Halfword 5 */

           value = (int) CR_RDA_status.status_msg.ave_trans_pwr;
           break;

      case RS_DATA_TRANS_ENABLED :        /* Halfword 7 */

         value = (int) CR_RDA_status.status_msg.data_trans_enbld;
         break;

      case RS_VCP_NUMBER :                /* Halfword 8 */

          value = (int) CR_RDA_status.status_msg.vcp_num;
          break;

      case RS_RDA_CONTROL_AUTH :          /* Halfword 9 */

          value = (int) CR_RDA_status.status_msg.rda_control_auth;
          break;

      case RS_OPERATIONAL_MODE :          /* Halfword 11 */

          value = (int) CR_RDA_status.status_msg.op_mode;
          break;

      case RS_ISU :                       /* Halfword 12 */

          value = (int) CR_RDA_status.status_msg.int_suppr_unit;
          break;

      case RS_RDA_ALARM_SUMMARY :         /* Halfword 15 */

          value = (int) CR_RDA_status.status_msg.rda_alarm;
          break;

      case RS_CHAN_CONTROL_STATUS :       /* Halfword 17 */

          value = (int) CR_RDA_status.status_msg.channel_status;
          break;

      case RS_SPOT_BLANKING_STATUS :      /* Halfword 18 */

          value = (int) CR_RDA_status.status_msg.spot_blanking_status;
          break;

      case RS_BPM_GEN_DATE :              /* Halfword 19 */

          value = (int) CR_RDA_status.status_msg.bypass_map_date;
          break;

      case RS_BPM_GEN_TIME :              /* Halfword 20 */

         value = (int) CR_RDA_status.status_msg.bypass_map_time;
         break;

      case RS_NWM_GEN_DATE :              /* Halfword 21 */

         value = (int) CR_RDA_status.status_msg.notchwidth_map_date;
         break;

      case RS_NWM_GEN_TIME :              /* Halfword 22 */

         value = (int) CR_RDA_status.status_msg.notchwidth_map_time;
         break;

      case RS_TPS_STATUS :                /* Halfword 24 */

         value = (int) CR_RDA_status.status_msg.tps_status;
         break;

      case RS_RMS_CONTROL_STATUS :        /* Halfword 25 */

         value = (int) CR_RDA_status.status_msg.rms_control_status;
         break;

      default :

         value = ST_FAILURE;
         break;

   } /* end switch */

   return value;

/* End Get_rda_status */
} 

/*\/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Returns the specified status element.
//
//   Inputs:
//      element - Integer that indicates the element whose value is
//                being sought.
//
//   Return:
//      Value of the desired element.
//
/////////////////////////////////////////////////////////////////////////\*/
static int Get_orda_status ( int element ){

   int value = 0;

   switch (element){

      case RS_RDA_STATUS :                /* Halfword 1 */

         value = (int) CR_ORDA_status.status_msg.rda_status;
         break;

      case RS_OPERABILITY_STATUS :        /* Halfword 2 */

          value = (int) CR_ORDA_status.status_msg.op_status;
          break;

      case RS_CONTROL_STATUS :            /* Halfword 3 */

          value = (int) CR_ORDA_status.status_msg.control_status;
          break;

      case RS_AUX_POWER_GEN_STATE :       /* Halfword 4 */

           value = (int) CR_ORDA_status.status_msg.aux_pwr_state;
           break;

      case RS_AVE_TRANS_POWER :           /* Halfword 5 */

           value = (int) CR_ORDA_status.status_msg.ave_trans_pwr;
           break;

      case RS_DATA_TRANS_ENABLED :        /* Halfword 7 */

         value = (int) CR_ORDA_status.status_msg.data_trans_enbld;
         break;

      case RS_VCP_NUMBER :                /* Halfword 8 */

         value = (int) CR_ORDA_status.status_msg.vcp_num;
         break;

      case RS_RDA_CONTROL_AUTH :          /* Halfword 9 */

         value = (int) CR_ORDA_status.status_msg.rda_control_auth;
         break;

      case RS_OPERATIONAL_MODE :          /* Halfword 11 */

         value = (int) CR_ORDA_status.status_msg.op_mode;
         break;

      case RS_SUPER_RES :                 /* Halfword 12 */

         value = (int) CR_ORDA_status.status_msg.super_res;
         break;

      case RS_CMD :                       /* Halfword 13 */

         value = (int) CR_ORDA_status.status_msg.cmd;
         break;

      case RS_AVSET :                     /* Halfword 14 */

         value = (int) CR_ORDA_status.status_msg.avset;
         break;

      case RS_RDA_ALARM_SUMMARY :         /* Halfword 15 */

         value = (int) CR_ORDA_status.status_msg.rda_alarm;
         break;

      case RS_CHAN_CONTROL_STATUS :       /* Halfword 17 */

         value = (int) CR_ORDA_status.status_msg.channel_status;
         break;

      case RS_SPOT_BLANKING_STATUS :      /* Halfword 18 */

         value = (int) CR_ORDA_status.status_msg.spot_blanking_status;
         break;

      case RS_BPM_GEN_DATE :              /* Halfword 19 */

         value = (int) CR_ORDA_status.status_msg.bypass_map_date;
         break;

      case RS_BPM_GEN_TIME :              /* Halfword 20 */

         value = (int) CR_ORDA_status.status_msg.bypass_map_time;
         break;

      case RS_NWM_GEN_DATE :              /* Halfword 21 */

         value = (int) CR_ORDA_status.status_msg.clutter_map_date;
         break;

      case RS_NWM_GEN_TIME :              /* Halfword 22 */

         value = (int) CR_ORDA_status.status_msg.clutter_map_time;
         break;

      case RS_TPS_STATUS :                /* Halfword 24 */

         value = (int) CR_ORDA_status.status_msg.tps_status;
         break;

      case RS_RMS_CONTROL_STATUS :        /* Halfword 25 */

         value = (int) CR_ORDA_status.status_msg.rms_control_status;
         break;

      default :

         value = ST_FAILURE;
         break;

   } /* end switch */

   return value;

/* End Get_orda_status */
}

/*\////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Copies the current RDA status to previous RDA status.
//
////////////////////////////////////////////////////////////////////////\*/
static void Copy_current_to_previous(){

   int rda_config = ST_get_rda_config( NULL );

   if( rda_config == ORPGRDA_ORDA_CONFIG ){

      /* Store the old data to the previous rda status message buffer */
      memcpy(&CR_previous_RDA_status, &CR_ORDA_status, sizeof(ORDA_status_t));
      memcpy(&CR_previous_ORDA_status, &CR_ORDA_status, sizeof(ORDA_status_t));

   }
   else{

      /* Store the old data to the previous rda status message buffer */
      memcpy(&CR_previous_RDA_status, &CR_RDA_status, sizeof(RDA_status_t));
      memcpy(&CR_previous_ORDA_status, &CR_RDA_status, sizeof(RDA_status_t));


   }

/* End of Copy_current_to_previous() */
}

/*\////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Checks if there are differences between the current and previous
//      RDA status messages.
//
//   Inputs:
//      new_msg_data - pointer to new RDA Status message.
//
//   Returns:
//      1 - there are differences, 0 - no differences.
//
////////////////////////////////////////////////////////////////////////\*/
static int Check_status_change( short *new_msg_data ){

   int field_index, rda_config = ST_get_rda_config( NULL );
   short *old_msg_data = NULL;

   /* Assign the data pointers.  Note: because we are interested in comparing
      only the RDA status data (not the msg hdr), we need to increment the
      pointer past the msg hdr.  Note: The "old" data is stored in CR_ORDA_status
      or CR_RDA_status since it hasn't been copied to previous yet. */
   if( rda_config == ORPGRDA_ORDA_CONFIG )
      old_msg_data = ((short *) &(CR_ORDA_status.status_msg)) +
                                 ORPGRDA_MSG_HEADER_SIZE;

   else if( rda_config == ORPGRDA_LEGACY_CONFIG )
      old_msg_data = ((short *) &(CR_RDA_status.status_msg)) +
                                 ORPGRDA_MSG_HEADER_SIZE;

   else
      return 0;

   /* Check each field of the status message to see if it has changed. If it has
      changed, set the return value appropriatly and return. */
   for( field_index = 0; field_index < ORPGRDA_RDA_STATUS_MSG_SIZE; field_index++ ){

      if( new_msg_data[field_index] != old_msg_data[field_index] )
         return 1;

   }

   return 0;

/* End of Check_status_change() */
}

