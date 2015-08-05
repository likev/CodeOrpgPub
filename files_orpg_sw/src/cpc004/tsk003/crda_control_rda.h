/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/13 19:47:43 $
 * $Id: crda_control_rda.h,v 1.89 2014/03/13 19:47:43 steves Exp $
 * $Revision: 1.89 $
 * $State: Exp $
 */
#ifndef CONTROL_RDA_H
#define CONTROL_RDA_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>
#include <itc.h>
#include <orpg.h>
#include <misc.h>
#include <malrm.h>
#include <en.h>
#include <comm_manager.h>
#include <rda_control.h>
#include <orpgrda.h>
#include <gen_stat_msg.h>
#include <clutter.h>
#include <rda_alarm_data.h>
#include <rda_rpg_message_header.h>
#include <rpg_vcp.h>
#include <orpgerr.h>
#include <alarm_services.h>

/*
  Define Adaptation Data items.
*/
#define ADAPTATION_BLOCKS 0

/*
  Define the number of connection/disconnection retries.  (NOTE: This 
  will be part of adaptation data.  These are defaults.)
*/
#define CONNECTION_RETRY_LIMIT       4
#define DISCONNECTION_RETRY_LIMIT    CONNECTION_RETRY_LIMIT

/*
  Define timer ids.
*/
#define MALRM_RDA_COMM_DISC        1
#define MALRM_LB_TEST_TIMEOUT      2
#define MALRM_CONNECTION_RETRY     3
#define MALRM_RESTART_TIMEOUT      4
#define MALRM_DISCONNECTION_RETRY  5
#define MALRM_RDA_STATUS           6
#define MALRM_LOOPBACK_PERIODIC    7
#define MALRM_CHAN_CTRL            8

/*
 Connection/Disconnection retries.
*/
#define RDA_NUM_CONN_RETRIES       2

/*
  Timeout values (all define in seconds).
*/
#define LB_TEST_TIMEOUT_VALUE       4
#define RDA_COMM_DISC_VALUE        15 
#define RDA_CONNECT_RETRY_VALUE    10
#define RDA_DISCONNECT_RETRY_VALUE 10
#define RDA_DISCONNECT_SHUTDOWN     5
#define RDA_RESTART_VALUE         120 
#define RDA_STATUS_TIMEOUT         15
#define LOOPBACK_PERIODIC_RATE     60
#define CHAN_CTRL_TIMEOUT_VALUE    15


/* 
  Alarm Set Flag. 
*/
#define ALARM_SET                  0x80000000

/*
  Define RPG Generated RDA alarm codes for RDA status.
*/
#define RDA_LINK_BROKEN_ALARM   1
#define RDA_COMM_DISC_ALARM     2

/* 
Communications discontinuity timeout value, in seconds. 
*/
#define COMM_DISC_TIMEOUT_VALUE 160

/*
  Define line number for RDA Primary.
*/
#define RDA_PRIMARY       1

/*
  RDA to RPG, RPG to RDA message buffer sizes.
*/
#define CTM_HEADER_LENGTH 6     /* Number of halfwords in CTM header. */
#define INPUT_DATA_SIZE   (32768+CTM_HEADER_LENGTH+(sizeof(CM_resp_struct)/2))  
                                /* maximum size of RDA message, in halfwords. 
                                   Includes the message itself plus the CTM header
                                   and Comm Manager respoonse structure. */
#define OUTPUT_DATA_SIZE  (1208+CTM_HEADER_LENGTH+(sizeof(CM_req_struct)/2))
                                /* maximum size of RPG message, in halfwords.  
                                   Includes the message itself plus the CTM header 
                                   and Comm Manager request structure. */
#define MSGSIZMAX         1200  /* maximum message size, in halfwords */


/*
  Define VCP data constants and offsets.
*/
#define PFS1AZAN	 11
#define PFS2AZAN	 15
#define PFS3AZAN	 19
#define DEG_TO_BAM        0.0439453f
#define REMOTE_VCP	 0


/*
  RDA status data defintions.
*/
#define MAX_ALARMS             40    /* Maximum number of rda alarms */
#define MAX_RDA_ALARM_CODE     800   /* Maximum rda alarm code */
#define RPG_INITIATED          0     /* RPG initiated status data */
#define RDA_INITIATED          1     /* RDA initiated status data */


/* Message Header size, in shorts. */
#define MSGHDRSZ          8

/* RDA Status Message size, in shorts. */
#define MSIZ_RDASTATUS   ORPGRDA_RDA_STATUS_MSG_SIZE 

/* RPG Loopback Message size, in shorts. */
#define MSIZ_RPGLB       50     


/*
  Outstanding Request Macro definition.
*/
#define MAXN_OUTSTANDING   20

#define WAIT_FOR_RESP      1
#define DONT_WAIT_FOR_RESP 0

#define MSGID_TO_REQ_BIAS  -100
#define REQ_TO_MSGID_BIAS   100

/*
  Defined Types.
*/
typedef struct request_list{

   unsigned int sequence_number;
   int operation;
   int wait_for_resp;
   int wait_time;
   int message_id;
   time_t message_time;
   int message_size;
   malrm_id_t timer_id;
   time_t timeout_value;
   char *rpg_to_rda_msg;

} Request_list_t;

/* Keep track of downloaded information.   Currently, we only track
   VCP.  Other information can be added when needed. */
typedef struct request_info {

   int last_downloaded_vcp;

   time_t last_download_vcp_time;

} Request_info_t;

#ifdef GLOBAL_DEFINED_CONTROL_RDA
#define EXTERN
#else
#define EXTERN extern
#endif

/* Copy of ORDA Status message and Wideband Line Communications Status. */
ORDA_status_t CR_ORDA_status;
ORDA_status_t CR_previous_ORDA_status;

/* Copy of RDA Status message and Wideband Line Communications Status. */
RDA_status_t CR_RDA_status;
RDA_status_t CR_previous_RDA_status;

/* Flag, if set, forces wideband connection on receipt of radial data. */
EXTERN int CR_force_connection;

/* Flag, if set, indicates that control rda is restarting. */
EXTERN int CR_control_rda_restarting;

/* Last commanded Velocity Resolution for VCP. Used whenever VCP downloaded. */
EXTERN int CR_velocity_resolution;

/* Flag, if set, indicates that we need to return RDA to previous state. */
EXTERN int CR_return_to_previous_state;

/* Values for CR_return_to_previous_state. */
enum{ PREVIOUS_STATE_NO, PREVIOUS_STATE_PERFORM_LOOPBACK,
      PREVIOUS_STATE_LOOPBACK_WAIT, PREVIOUS_STATE_STATUS_WAIT, 
      PREVIOUS_STATE_PHASE_1, PREVIOUS_STATE_PHASE_2 };

/* Flag, if set, indicates that Control RDA is shutting down. */
EXTERN int CR_shut_down_state;

/* Values for CR_shut_down_state. */
enum{ SHUT_DOWN_NO, SHUT_DOWN_STANDBY, SHUT_DOWN_STANDBY_WAIT, 
      SHUT_DOWN_DISCONNECT, SHUT_DOWN_DISCONNECT_WAIT, SHUT_DOWN_EXIT };

/* If non-negative, specifies the command mrpg sent to get it in its 
   current state. */
EXTERN int CR_rpg_restarting;

/* Flag, if set, indicates the RDA is restarting */
EXTERN int CR_rda_restarting;

/* Counter for number of connections retries after a line connection
   attempt has been made.  Also the limiting value and the timeout
   value before initiating another retry. */
EXTERN int CR_connection_retries;
EXTERN int CR_connection_retry_limit;
EXTERN int CR_connect_retry_value;

/* Flag, if set, indicates the connection retry limit was reached. */
EXTERN int CR_connection_retries_exhausted;

/* Flag, if set, indicates to attempt wideband reconnection after a
   loopback test failure. */
EXTERN int CR_reconnect_wideband_line_after_loopback_failure;

/* Counter for number of disconnections retries after a line disconnection
   attempt has been made.  Also the limiting value and the timeout
   value before initiating another retry. */
EXTERN int CR_disconnection_retries;
EXTERN int CR_disconnection_retry_limit;
EXTERN int CR_disconnect_retry_value;

/* Flag, if set, indicates the disconnection retry limit was reached. */
EXTERN int CR_disconnection_retries_exhausted;

/* The Linear Buffer data ID for the Request Linear Buffer. */
EXTERN int CR_request_LB;

/* The Linear Buffer data ID for the Response Linear Buffer. */
EXTERN int CR_response_LB;

/* The line index for the wideband line. */
EXTERN int CR_link_index;

/* Counter for number of radial messages received during the 
   MALRM_RDA_COMM_DISC period. */
EXTERN int CR_msg_processed;

/* Condition variable, if set, indicates the communications discontinuity
   alarm should be actived. */
EXTERN int CR_communications_discontinuity;

/* The current RDA control status.  Status values are either:
   CS_LOCAL_ONLY (RDA control of RDA)
   CS_RPG_REMOTE (RPG control of RDA)
   CS_EITHER (RDA Control is Either RDA or RPG). */
EXTERN int CR_control_status;

/* The current RDA control authority.  Control authority values
   are either:
   CA_LOCAL_CONTROL_REQUESTED (RDA Requests Local RDA Control)
   CA_REMOTE_CONTROL_ENABLED  (Remote Control of RDA Enabled). */ 
EXTERN int CR_control_authority;

/* Current operational mode.  This is either:
   OP_MAINTENANCE_MODE = 2	   (For backward compatibility)
   OP_OPERATIONAL_MODE = 4
   OP_OFFLINE_MAINTENANCE_MODE = 8 (Open RDA only)
*/
EXTERN int CR_operational_mode;

/* RDA status protection flag. If set, RDA status is not to be saved 
   as "previous status". */
EXTERN int CR_status_protect;

/* RDA status latch flag.  If cleared, the first time the 
   MALRM_RDA_STATUS alarm is triggered, the RDA is commanded to its
   previous state (if the RDA is not in local control or the control
   authority is not rda remote control enabled). */
EXTERN int CR_status_latch;

/* Flag, if set, indicates the loopback test failed due to timeout. */
EXTERN int CR_loopback_timeout;

/* Flag, if set, indicates the loopback test failed due to mismatch. */
EXTERN int CR_loopback_failure;

/* Flag, if set, indicates the loopback test needs to be re-initiated. Also
   the loopback periodic rate, in seconds. */
EXTERN int CR_perform_loopback_test;
EXTERN int CR_loopback_periodic_rate;

/* Counter for loopback retries. */
EXTERN int CR_loopback_retries;

/* These flags, if set, indicates HCI user issued request for data. */
EXTERN int CR_hci_requested_status;
EXTERN int CR_hci_requested_pmd;
EXTERN int CR_hci_requested_vcp;

/* Verbosity switch.  Used to turn on LE informational Messages. */
EXTERN int CR_verbose_mode;

/* Redundant configuration type and channel number. */
EXTERN int CR_redundant_type;
EXTERN int CR_channel_num;

/* Flag indicating if timer expired. */
EXTERN int CR_timer_expired;

/* Flag indicating if DP PMD to be included in RDA CAL. */
EXTERN int CR_include_dp_pmd;
EXTERN float CR_rda_build_num;

/* To keep track of last VCP downloaded. */
EXTERN int CR_last_commanded_vcp;
EXTERN Vcp_struct CR_last_downloaded_vcp_data;

/*
  Function Prototypes
*/

/*             crda_main.c                         */
void CR_outstanding_control_command( int command_type );

/*             crda_send_wideband_msg.c             */
int SWM_check_response_LB( int waiting_for_response, int wait_time );
int SWM_RDAtoRPG_loopback_message( short *rda_to_rpg_loopback, int message_size );
int SWM_process_rda_command( Rda_cmd_t *rda_command );
int SWM_request_operation( int operation, int parameter, int wait_for_response,
                           int wait_time );
int SWM_process_weather_mode_change( Rda_cmd_t *rda_command );
int SWM_process_vcp_download( Rda_cmd_t *rda_command );
int SWM_process_vcp_change( Rda_cmd_t *rda_command );
int SWM_download_vcp( Rda_cmd_t *rda_command );
void SWM_get_date_time( short *date, unsigned int *current_time );
int SWM_rda_msg_header( char *rda_msg, int type, short seglen, short segnum,
                        short totsegs );
int SWM_send_rda_command( int num_arg, int command, int wideband_line, ... );
void SWM_init_outstanding_requests();
int  SWM_return_to_previous_state( int cold_startup );

#define SWM_SUCCESS                                   0
#define SWM_FAILURE                                  -1

/*             crda_line_connect.c                  */
void LC_init();
int  LC_connect_wb_line();
int  LC_disconnect_wb_line( int whodunit );
int  LC_process_wb_line_connection( int return_value );
int  LC_process_wb_line_disconnection( int return_value );
void LC_process_unexpected_line_disconnection( int event_type );
int  LC_process_connection_retry_exhausted();
void LC_process_disconnection_retry_exhausted();
int  LC_reconnect_line_after_failure( );
int  LC_check_wb_line_status();
void LC_set_reconnect_after_failure_flag();

#define LC_SUCCESS                                  0
#define LC_FAILURE                                 -1


/*             crda_download_vcp.c                  */
int DV_process_vcp_download( Rda_cmd_t *rda_command, short **message_data, int *message_size );
int DV_get_default_vcp_for_wxmode( int weather_mode );
int DV_init_adaptation_data();

#define DV_SUCCESS                                   0
#define DV_FAILURE                                  -1


/*             crda_proc_rdamsg.c                   */
void PR_process_rda_status_data( short *rda_data );
void PR_init_RPGtoRDA_loopback_message( );
int  PR_process_bypass_map( short *rda_data, int message_size, int *last_segment );
int  PR_process_clutter_map( short *rda_data, int message_size, int *last_segment );
int  PR_process_performance_data( short *rda_data );
int  PR_process_rda_console_msg( short *rda_data );
int  PR_validate_RPGtoRDA_loopback_message( short *rda_data, int message_size );
int  PR_RPGtoRDA_loopback_message( short **message_data, int *message_size );
int  PR_process_adapt_data( short *rda_data, int message_size, int *last_seg );
int  PR_process_rda_vcp_data( short *rda_data );

#define PR_SUCCESS                                   0
#define PR_FAILURE                                  -1

/*             crda_rda_control.c                    */
int RC_orda_control( Rda_cmd_t *rda_command, short **message_data,
                    int *message_size );

#define RC_SUCCESS                                   0
#define RC_FAILURE                                  -1


/*             crda_receive_wb_data.c                */
int RW_receive_wideband_data( int *RDA_to_RPG_msg, short format );
int RW_get_current_elev_index( );

/*             crda_status.c                          */
int  ST_init();
int  ST_init_rda_status( );
void ST_process_rda_alarms( char *msg );
void ST_process_command_ack( char *msg );
void ST_update_system_status( );
void ST_set_communications_alarm( int alarm_code );
void ST_clear_communications_alarm( int alarm_code );
void ST_clear_rda_alarm_codes();
int  ST_update_rda_status( int post_status_change_event, char *msg, int initiator );
int  ST_process_rda_status( short *rda_data );
int  ST_checkpoint_previous_state( );
int  ST_set_status( int field_id, int value );
int  ST_get_status( int field_id );
int  ST_get_rda_config( void *msg );
int  ST_write_status_msg( );

#define ST_SUCCESS                                   0
#define ST_FAILURE                                  -1

/*             crda_set_comms_status.c                */
int  SCS_init();
void SCS_handle_comm_manager_event( int event_type );
void SCS_handle_wideband_alarm( int wideband_line_status, int alarm_code,
                                int display_blanking );
int  SCS_update_wb_line_status( int wideband_line_status, 
                                int display_blanking,
                                int post_status_change_event );
void SCS_clear_communications_alarm( int alarm_code );
void SCS_set_communications_alarm( int alarm_code );
int  SCS_check_wb_alarm_status( int wb_alarm_code );
void SCS_display_comms_line_status();
int  SCS_get_wb_status( int field );

#define SCS_SUCCESS                                   0
#define SCS_FAILURE                                  -1

/*             crda_timer_services.c                  */
void TS_register_timers();
void TS_service_timer_expiration();
void TS_handle_comm_discontinuity();
void TS_handle_loopback_timeout();
void TS_handle_connection_retry();
void TS_handle_disconnection_retry();
void TS_handle_restart_timeout();
void TS_handle_RDA_status_timeout();
void TS_handle_loopback_periodic();
void TS_handle_channel_control();
int  TS_set_timer( malrm_id_t id, time_t time_in_future, unsigned int interval );
void TS_cancel_timer( int number_timers, ... );
unsigned int  TS_is_timer_active( malrm_id_t id );

#define TS_SUCCESS                                    0
#define TS_FAILURE                                   -1

/*            crda_queue_services.c                  */
void QS_create_request_queue();
int  QS_request_queue_empty();
int  QS_request_queue_full();
void QS_clear_request_queue();
int  QS_insert_rear_request_queue( Request_list_t *list_ptr );
int  QS_insert_front_request_queue( Request_list_t *list_ptr );
int  QS_remove_request_queue( Request_list_t **list_ptr );

#define QS_SUCCESS                                   0
#define QS_QUEUE_FULL                           -13100
#define QS_QUEUE_EMPTY                          -13200


/*            crda_event_services.c                  */
void ES_register_for_events();
int ES_post_event( en_t evtcd, void *msg, size_t msglen, int local_flag );

#define ES_SUCCESS                                   0
#define ES_FAILURE                                  -1


RDA_alarm_data_t Get_rda_alarm_data( int code );

#endif
