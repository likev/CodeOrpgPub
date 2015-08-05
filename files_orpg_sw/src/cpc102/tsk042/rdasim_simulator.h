/********************************************************************************

     file: rdasim_simulator.h

     this file contains all the header definitions for the rda simulator.

********************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/31 17:59:06 $
 * $Id: rdasim_simulator.h,v 1.33 2014/07/31 17:59:06 steves Exp $
 * $Revision: 1.33 $
 * $State: Exp $
 */  


#ifndef RDA_SIMULATOR_H
#define RDA_SIMULATOR_H

#include <orpg.h>
#include <comm_manager.h>
#include <rpg_vcp.h>


   /* locally defined RDA->RPG ICD message types */

#define RDA_ADAPTATION_DATA      18
#define RDA_VCP_MSG               5


#define CCS_CONTROLLING      0x0000  /* RDA-RPG ICD setting for this channel controlling */
#define CCS_NON_CONTROLLING  0x0001  /* RDA-RPG ICD setting for this channel not controlling */

#define ALIGNMENT_SIZE                    4  /* # of bytes for alignment */

#define FALSE                             0
#define TRUE                              1

#define ZERO                              0
#define ONE                               1

#define REQUEST_FOR_STATUS_DATA         129  /* request for status data */
#define REQUEST_FOR_PERFORMANCE_DATA    130  /* request for performance data */
#define REQUEST_FOR_BYPASS_MAP_DATA     132  /* request for clutter bypass map */
#define REQUEST_FOR_NOTCHWIDTH_MAP_DATA 136  /* request for notchwidth map */

#define MAX_MESSAGE_SIZE               2416  /* maximum message length */
#define MAX_MESSAGE_31_SIZE           65535  /* maximum message length */
#define MAX_BUFFER_SIZE               66000  /* maximum length of a data buffer (size is padded) */
#define MAX_NAME_SIZE                   128  /* maximum size of a string */
#define MAX_NUM_SURV_BINS               460  /* max # of surveillance bins */
#define MAX_SR_NUM_SURV_BINS           1840  /* max # of surveillance bins (super res)*/
#define MAX_NUM_VEL_BINS                920  /* max # of velocity & spec width bins */
#define MAX_SR_NUM_VEL_BINS            1200  /* max # of velocity & spec width bins (super res) */
#define MAX_NUM_ZDR_BINS               BASEDATA_ZDR_SIZE
#define MAX_NUM_PHI_BINS               BASEDATA_PHI_SIZE
#define MAX_NUM_RHO_BINS               BASEDATA_RHO_SIZE
#define HALF_DEG_RADIALS               0.5f
#define ONE_DEG_RADIALS                1.0f
#define MAX_NUM_RADIALS                360   /* max # of radials */ 
#define MAX_SR_NUM_RADIALS             720   /* max # of radials for 1/2 deg radials */ 

#define NO_COMMAND_PENDING               -1  /* state where a new command has not
                                                been received and a command to
                                                execute is not pending */
#define MILLISECONDS_PER_SECOND        1000  /* # of milliseconds in a second */
#define SECONDS_PER_MSEC              0.001  /* # seconds in a millisecond */
#define SECONDS_IN_A_DAY              86400  /* # seconds in a day */
#define SECONDS_IN_A_HOUR              3600  /* # seconds in an hour */
#define SECONDS_IN_A_MINUTE              60  /* # seconds in a minute */

    /* define the commanded states and processing states */

#define START_OF_ELEVATION                0  /* processing at beginning of elevation cut */
#define PROCESSING_RADIAL_DATA            1  /* digital radar data being processed */
#define END_OF_ELEVATION                  2  /* processing at the end of elevation cut */
#define RDASIM_START_OF_VOLUME_SCAN       3  /* processing at beginning of vol scan */
#define END_OF_VOLUME_SCAN                4  /* processing at end of vol scan */
#define START_OF_LAST_ELEVATION           5
#define NO_PENDING_COMMAND                6  /* a command is not pending */
#define START_UP                          7
#define STANDBY                           8
#define RDA_RESTART                       9
#define OPERATE                          10
#define OFFLINE_OPERATE                  11
#define PLAYBACK                         12
#define VCP_ELEVATION_RESTART            13


         /* RDA Status Message Info */

   /* RDA Status (RDS_) */

#define RDS_STARTUP                   0x0002  /* RDA in startup */
#define RDS_STANDBY                   0x0004  /* RDA in stand-by */
#define RDS_RESTART                   0x0008  /* RDA in restart */
#define RDS_OPERATE                   0x0010  /* RDA in operate */
#define RDS_PLAYBACK                  0x0020  /* RDA in playback */
#define RDS_OFFLINE_OPERATE           0x0040  /* RDA in offline operate */


   /* RDA Operability Status (ROS_) */

#define ROS_RDA_ONLINE                0x0002  /* RDA online */
#define ROS_RDA_MAINTENANCE_REQUIRED  0x0004  /* maintenance required */
#define ROS_RDA_MAINTENANCE_MANDATORY 0X0008  /* maintenance mandatory */
#define ROS_RDA_COMMANDED_SHUTDOWN    0x0010  /* commanded shutdown */
#define ROS_RDA_INOPERABLE            0x0020  /* RDA inoperable */
#define ROS_RDA_AUTOMATIC_CALIBRATION_DISABLED  0x0001  /* auto cal disabled */

   /* RDA Control Status (RCS_) */

#define RCS_RDA_LOCAL_ONLY            0x0002  /* RDA in local only */
#define RCS_RDA_REMOTE_ONLY           0x0004  /* RDA in remote only */
#define RCS_RDA_EITHER                0x0008  /* RDA in either */


   /* Aux Power Generator State (APGS_) */

#define APGS_AUXILLARY_POWER          0x0001  /* gen sw to aux power */
#define APGS_UTILITY_PWR_AVAILABLE    0x0002  /* utility power available */
#define APGS_GENERATOR_ON             0x0004  /* aux generator is on */
#define APGS_XFER_SWITCH_IN_MANUAL    0x0008  /* gen xfer switch is in manual */
#define APGS_COMMANDED_SWITCHOVER     0x0010  /* gen in commanded switchover */

   /* Data Transmission Enabled (DTE_) */

#define DTE_NONE_ENABLED              0X0002  /* no data is enabled */
#define DTE_REFLECTIVITY_ENABLED      0X0004  /* xmission of reflectivity enabled */
#define DTE_VELOCITY_ENABLED          0X0008  /* xmission of velocity data enabled */
#define DTE_SPECTRUM_WIDTH_ENABLED    0x0010  /* xmission of s.w. data enabled */
#define DTE_ALL_ENABLED               0x001c  /* xmission all enabled */

   /* RDA Control Authorization (RCA_) */

#define RCA_NO_ACTION                 0x0000  /* no action */
#define RCA_LOCAL_CONTROL_REQUESTED   0x0002  /* local control requested */
#define RCA_REMOTE_CONTROL_ENABLED    0x0004  /* remote control is enabled */

   /* RDA Operational Mode (ROM_) */

#define ROM_MAINTENANCE               0x0002  /* RDA in maintenance mode */
#define ROM_OPERATIONAL               0x0004  /* RDA in operational mode */

   /* Super Resolution Status     */

#define SRS_ENABLED                   0x0002  /* Super Res enabled */
#define SRS_DISABLED                  0x0004  /* Super Res disabled */

   /* Clutter Mitigation Decision */

#define CMDS_ENABLED                  0x0002  /* CMD enabled */
#define CMDS_DISABLED                 0x0004  /* CMD disabled */

   /* Automatic Volume Scan Evaluation and Termination. */

#define AVSETS_ENABLED                0x0002  /* AVSET enabled */
#define AVSETS_DISABLED               0x0004  /* AVSET disabled */

   /* Archive II Status (A2S_) */

#define A2S_NOT_INSTALLED             0x0000  /* archive II not installed */
#define A2S_INSTALLED                 0x0001  /* archive II installed */
#define A2S_LOADED                    0x0002  /* archive II tape loaded */
#define A2S_WRITE_PROTECTED           0x0004  /* archive II tape write protected */
#define A2S_RESERVED                  0x0008  /* reserved bit */
#define A2S_RECORD                    0x0010  /* archive II is recording */
#define A2S_PLAYBACK_AVAILABLE        0x0020  /* archive II playback available */
#define A2S_GENERATE_DIRECTORY        0x0040  /* archive II generating tape dir */
#define A2S_POSITION                  0x0080  /* tape is being positioned */

   /* RDA Alarm Summary (RAS_) */

#define RAS_NO_ALARMS                 0x0000
#define RAS_TOWER_UTILITIES           0x0002
#define RAS_PEDESTAL                  0x0004
#define RAS_TRANSMITTER               0x0008
#define RAS_RECEIVER_SIGNAL_PROCESSOR 0x0010
#define RAS_RECEIVER                  0x0010  /* ORDA only */
#define RAS_RDA_CONTROL               0x0020
#define RAS_RPG_COMMUNICATIONS        0x0040
#define RAS_USER_COMMUNICATION        0x0080
#define RAS_SIGNAL_PROCESSOR          0x0080  /* ORDA only */
#define RAS_ARCHIVE_II                0x0100

   /* Command Acknowledgement (CA_) */

#define CA_NO_ACKNOWLEDGEMENT                      0x0000
#define CA_REMOTE_VCP_RECEIVED                     0x0001
#define CA_CLUTTER_BYPASS_MAP_RECEIVED             0X0002
#define CA_CLUTTER_CENSOR_ZONES_RECEIVED           0x0003
#define CA_REDUNDANT_CHANNEL_STANDBY_CMD_ACCEPTED  0x0004

   /* Spot Blanking Status (SBS_) */

#define SBS_NOT_INSTALLED             0x0000
#define SBS_ENABLED                   0x0002
#define SBS_DISABLED                  0x0004



typedef struct {
   short word1;
   short word2;
   short word3;
   short word4;
   short word5;
   short word6;
} CTM_header_t;


#define RDASIM_VOL_DATA			-3
#define RDASIM_ELV_DATA			-2
#define RDASIM_RAD_DATA			-1

#define RDASIM_REF_DATA			1
#define RDASIM_VEL_DATA			2
#define RDASIM_WID_DATA			3
#define RDASIM_ZDR_DATA			4
#define RDASIM_PHI_DATA			5
#define RDASIM_RHO_DATA			6

#define MAX_DATA_BLOCKS			9

   /* The radial message structure.  */

typedef struct {
   Generic_basedata_t		hdr;		/* RDA/RPG Message and Generic 
						   Basedata headers. */

   int				data_offs[MAX_DATA_BLOCKS];	
						/* Reserved space for block data 
                                                   offsets.  We need space for
                                                   Generic_vol_t, Generic_elev_t,
                                                   Generic_rad_t, and all the moments.*/

   Generic_vol_t		vol_hdr;	/* Generic Basedata header. */

   Generic_elev_t		elv_hdr;	/* Generic Elevation header. */

   Generic_rad_t		rad_hdr;	/* Generic Radial header. */

   Generic_rad_dBZ0_t           dBZ0_hdr;	/* Additional information for Message 31, 
                                                   Major Version 2. */

   Generic_moment_t		ref_hdr;	/* Reflectivity header. */

   unsigned char		ref[MAX_BASEDATA_REF_SIZE];
						/* Reflectivity data. */

   Generic_moment_t		vel_hdr;	/* Velocity header. */

   unsigned char		vel[BASEDATA_DOP_SIZE];
						/* Velocity data. */

   Generic_moment_t		wid_hdr;	/* Specturm width header. */

   unsigned char		wid[BASEDATA_DOP_SIZE];
						/* Spectrum width data. */

   Generic_moment_t		zdr_hdr;	/* Diff. Reflectivity header. */

   unsigned char		zdr[BASEDATA_ZDR_SIZE];
						/* Diff. Reflectivity data. */

   Generic_moment_t		phi_hdr;	/* Diff. Phase header. */

   unsigned short		phi[BASEDATA_PHI_SIZE];
						/* Diff. phase data. */

   Generic_moment_t		rho_hdr;	/* Diff. Correlation header. */

   unsigned char		rho[BASEDATA_RHO_SIZE];
						/* Diff. Correlation data. */


} Radial_message_t;


enum {                         /* processing state ("state" in Req_struct or
                                  Resp_struct) */
    CM_NEW,                    /* new and unprocessed */
    CM_DONE                    /* processing finished and response sent */
};


enum {                         /* values for link_state */
    LINK_DISCONNECTED,         /* the link is disconnected */
    LINK_CONNECTED             /* the link is connected */
};

enum {                         /* values for conn_activity */
    NO_ACTIVITY,               /* no connect/disconnect request is being
                                  processed */
    CONNECTING,                /* a connect request is being processed */
    DISCONNECTING              /* a disconnect request is being processed */
};

typedef struct {               /* internal request message struct */
    int state;                 /* CM_NEW, CM_DONE or start time */
    char *data;                /* CM_WRITE only; pointer to the buffer 
                                  holding the original message including 
                                  header CM_req_struct and the data. */
    LB_id_t msg_id;            /* LB msg id of this request */

    /* the following is identical to the CM_req_struct */
    int type;                  /* request type; for CM_WRITE type, the data 
                                  will following this structure */
    int req_num;               /* request number; unique for the link */
    int link_ind;              /* link index */

    int time_out;              /* time out value in seconds for this request;
                                  0 means no time out specified */
    int parm;                  /* request parameter; 
                                  CM_WRITE: write priority (1 - 3) with
                                            3 meaning the lowest priority;
                                  CM_CANCEL: request number to cancel; -1 
                                             means all previous requests */
    int data_size;             /* CM_WRITE only; data size */
} Req_struct;

typedef struct {               /* link configuration */
    char link_ind;             /* link index */
    char link_state;           /* link connection state */
    char conn_activity;        /* current connection activity: CONNECTING,
                                  DISCONNECTING or NO_ACTIVITY; There can 
                                  be only one connect/disconnect request being 
                                  processed at any time */
    char conn_req_ind;         /* req index in "req" of the current 
                                  connect/disconnect request being processed */
    int packet_size;           /* maximum packet size */
    int n_added_bytes;         /* number of bytes added in front of each
                                  message written to the RDA. 10 bytes must 
                                  be added for the legacy RDA */
    int reqfd;                 /* LB file descriptor for requests */
    int respfd;                /* LB file descriptor for responses */
    char *r_buf;               /* buffer for incoming data */
    int r_cnt;                 /* bytes read */
    int r_buf_size;            /* read buffer size */
    unsigned int r_seq_num;    /* user data sequence number */
    char *w_buf;               /* buffer for outgoing data */
    int w_size;                /* outgoing message sizes */
    int w_cnt;                 /* bytes written */
    int w_ack;                 /* bytes acknowledged */
    char w_req_ind;            /* the request index in "req" array */
    char n_reqs;               /* number of pending requests to be processed */
    char st_ind;               /* index of the first request in "req" */
    Req_struct req[MAX_N_REQS];/* stores unfinished and pending requests */
} link_t;

typedef struct {
    unsigned short      rda_status;             /*  Status of RDA as indicated:
                                                   <BIT> Set = Yes
                                                     1 = Start-Up
                                                     2 = Standby
                                                     3 = Restart
                                                     4 = Operate
                                                     5 = Spare
                                                     6 = Off-line Operate
                                                */
    unsigned short      op_status;              /*  RDA Operability Status as indicated:
                                                   <BIT> Set = Yes
                                                     1 = On-line
                                                     2 = Maintenance Required
                                                     3 = Maintenance Mandatory
                                                     4 = Commanded Shut-down
                                                     5 = Inoperable
                                                    Above codes + 1 = Auto-calibration
                                                    disabled
                                                */
    unsigned short      control_status;         /*  RDA Control Status as indicated:
                                                   <BIT> Set = Yes
                                                     1 = Local Only
                                                     2 = Remote Only (RPG)
                                                     3 = Either
                                                     (Mutually Exclusive)
                                                */
    unsigned short      aux_pwr_state;          /*  Auxilliary Power Generator State:
                                                   <BIT> Set = Yes
                                                     1 = Utility Power Available
                                                     2 = Generator On
                                                     3 = Transfer Switch (Manual)
                                                     4 = Commanded Switchover
                                                    Above codes + 1 = Switched to
                                                    Auxilliary Power
                                                */
    short               ave_trans_pwr;          /*  Average transmitter power
                                                    (Range 0 to 9999)
                                                */
    short               ref_calib_corr;         /*  Horizontal Channel Reflectivity calibration 
                                                    correction (difference from adaptation data)
                                                    (Range -10 to 10 scaled by 4).
                                                */
    unsigned short      data_trans_enbld;       /*  Data transmission enabled:
                                                   <BIT> Set = Yes
                                                     1 = None
                                                     2 = Reflectivity
                                                     3 = Velocity
                                                     4 = Spectrum Width
                                                */
    short               vcp_num;                /*  Volume Coverage Pattern:
                                                     0       = No pattern
                                                     1 to 99 = Constant Elevation Types
                                                     <=255   = Operational
                                                     >255    = Maintenance/Test
                                                     Positive: RDA remote pattern (RPG)
                                                     Negative: RDA local pattern
                                                */
    unsigned short      rda_control_auth;       /*  RDA control authorization:
                                                     0 = No action
                                                     2 = Local Control Requested
                                                     4 = Remote Control Enabled
                                                */
    unsigned short      spare10;                /* Unused
                                                */
    unsigned short      op_mode;                /*  Operational Mode:
                                                     2 = Maintenance
                                                     4 = Operational
                                                */
    unsigned short      super_res;              /*  Super Resolution (Int Suppression Unit 
                                                    in Legacy):
                                                     2 = Enabled
                                                     4 = Disabled
                                                */
    unsigned short      cmd;                    /*  Clutter Mitigation Decision (spare 
                                                    in Legacy):
                                                     2 = Enabled
                                                     4 = Disabled
                                                */
    unsigned short      spare14;                /*  Unused
                                                */
    unsigned short      rda_alarm;              /*  RDA Alarm Summary:
                                                   <BIT> Set = Yes
                                                     0 = Alarms
                                                     1 = Tower/Utilities
                                                     2 = Pedestal
                                                     3 = Transmitter
                                                     4 = Receiver/Signal Processor
                                                     5 = RDA Control
                                                     6 = RPG Communication
                                                     7 = User Communication
                                                */
    unsigned short      command_status;         /*  Command Acknowledgement:
                                                     0 = No acknowledgement
                                                     1 = Remote VCP Received
                                                     2 = Clutter bypass map received
                                                     3 = Clutter sensor zones received
                                                     4 = Redundant channel standby
                                                         command accepted
                                                */
    unsigned short      channel_status;         /*  Channel control status.  Identifies
                                                    whether channel is controlling
                                                    channel.
                                                     0 = Controlling
                                                     1 = Non-controlling
                                                */
    unsigned short      spot_blanking_status;   /*  Spot blanking status:
                                                     0 = Not installed
                                                     2 = Enabled
                                                     4 = Disabled
                                                */
    unsigned short      bypass_map_date;        /*  Bypass map generation date (julian
                                                    date referenced from January 1, 1970
                                                    at 0000 UT; starts with 1).
                                                    (Range 1 to 65535)
                                                */
    unsigned short      bypass_map_time;        /*  Bypass map generation time.  Number
                                                    of minutes since midnight (UT).
                                                    (Range 0 to 1440)
                                                */
    unsigned short      notchwidth_map_date;    /*  Notchwidth map generation date
                                                    (julian date referenced from
                                                    January 1, 1970 at 0000 UT; starts
                                                    with 1).
                                                    (Range 1 to 65535)
                                                */
    unsigned short      notchwidth_map_time;    /*  Notchwidth map generation time.
                                                    Number of minutes since midnight (UT).
                                                    (Range 1 to 1440)
                                                */
    short               vc_ref_calib_corr;       /*  Vertical Channel Reflectivity calibration 
                                                    correction (difference from adaptation data)
                                                    (Range -10 to 10 scaled by 4).
                                                */
    unsigned short      tps_status;             /*  Transition Power Source
                                                    status:
                                                        0 - Not Installed
                                                        1 - OFF
                                                        3 - OK
                                                */
    unsigned short      rms_control_status;     /*  RMS Control Status:
                                                        0 - Non-RMS system
                                                        2 - RMS in control
                                                        4 - RDA MMI in control
                                                */
    short               spare26;
    short               alarm_code [MAX_RDA_ALARMS_PER_MESSAGE];
                                                /*  Alarm code.  One condition per
                                                    halfword with a maximum of 14
                                                    alarms sent at a time.  Code
                                                    ranges from 0 to 800.  MSB set
                                                    indicates alarm has been removed.
                                                */
} RDA_status_ICD_msg_t;


   /* Define the user/gui commands and exceptions that can sent to the simulator 
      during operation */

#define	RDA_SIMULATOR_LB	"rdasim.lb"

      /* RDA Simulator Exception definitions */

#define FAT_RADIAL_FORWARD                     1
#define FAT_RADIAL_BACK                        2
#define BAD_ELEVATION_CUT                      3
#define MAX_RADIAL                             4
#define UNEXPECTED_BEGINNING_ELEVATION         5
#define UNEXPECTED_BEGINNING_VOLUME            6
#define BAD_SEGMENT_BYPASS                     7
#define BAD_SEGMENT_NOTCHWIDTH                 8
#define BAD_START_VCP                          9
#define IGNORE_VOLUME_ELEVATION_RESTART       10
#define SKIP_START_OF_VOLUME_MSG              11
#define LOOPBACK_TIMEOUT                      12
#define LOOPBACK_SCRAMBLE                     13
#define RDA_ALARM_TEST                        14
#define NEGATIVE_START_ANGLE                  15


      /* RDA Simulator Command definitions */

#define START_OF_COMMAND_DEFINITIONS         100
#define COMMAND_RDA_TO_LOCAL                 101
#define COMMAND_RDA_TO_REMOTE                102
#define TOGGLE_CHANNEL_NUMBER                103
#define CHANGE_CHANNEL_CONTROL_STATUS        104
#define CHANGE_REFL_CALIB_CORR               105
#define TOGGLE_MAINTENANCE_MODE              106
#define AVSET_TERMINATION_CUT                107


   /* the internal LB structure used to send the simulator commands 
      and exceptions from the GUI */

#define	LB_PARAMETER_LENGTH	80
#define	RDASIM_GUI_MSG_ID     0

enum { COMMAND, EXCEPTION };

typedef struct {
   int cmd_type;   /* specifies the command type - command or an exception */
   int command;    /* the command/exception to execute */
   char parameters[ LB_PARAMETER_LENGTH ]; /* parameters to pass */
}  Rdasim_gui_t;


   /* function prototypes */

int CO_read_link_config (int cm_index, char *link_conf_name,
                         int *device_number);

short CR_get_current_vcp ();
void  CR_initialize_first_vcp ();
void  CR_initialize_this_vol_scan_data ();
void  CR_process_radial (int *current_state);
void  CR_set_radar_parameters (float antenna_rate, float sample_interval);
void  CR_set_termination_cut( int termination_cut );

#define INVALID_PATTERN_NUMBER		-99999
void  CR_new_vcp_selected (int new_pattern_number);

int  MA_align_bytes (int n_bytes);
void MA_terminate ();

   /*  Message conversion/byte swapping prototypes   */

int  MC_RDAtoRPG_message_convert_to_external(int data_type, char* data);
int  MC_RPGtoRDA_message_convert_to_internal(int data_type, char* data);
int  MC_RDAtoRPG_message_header_convert(char* data);
void MC_convert_comm_req_to_internal(CM_req_struct* data);
void MC_convert_comm_resp_to_external(CM_resp_struct* data);

void PR_disconnect_cleanup ();
void PR_process_cancel (Req_struct *req);
void PR_send_data (char *data, int message_type, int len);
void PR_send_response (Req_struct *req, int ret_value);
void PR_process_exception ();
void PR_process_requests ();

int  RD_connect_link ();
int  RD_disconnect_link ();
int  RD_get_processing_state ();
void RD_process_control_command ();
void RD_process_tst_tool_cmd (int event);
void RD_set_request_for_data_flag (int request);
void RD_set_rda_loopback_ack_flag ();
void RD_set_send_status_msg_flag ();
void RD_set_send_rda_vcp_flag ();
void RD_run_simulator ();

void RR_get_requests ();

void *RRM_get_radial_msg ();
void *RRM_get_rda_vcp_buffer ();
void *RRM_get_remote_vcp ();
int  RRM_initialize_messages ();
void RRM_init_chanl_control_state (short control_state);
void RRM_set_rda_alarm (int alarm_code);
void RRM_process_rda_alarms (int start_alarm, int end_alarm, 
                             int set_clear_cmd);
void RRM_process_rda_message (short message_type);
void RRM_process_rpg_msg ();
void RRM_update_msg_size (short msg_type, int msg_size);
void RRM_set_refl_calib_corr (int refl_calib_val);

#endif 
