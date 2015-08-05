/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/25 18:00:27 $
 * $Id: rda_status.h,v 1.48 2012/09/25 18:00:27 steves Exp $
 * $Revision: 1.48 $
 * $State: Exp $
 */  
/***********************************************************************

	Header defining the data structures and constants used for
	RDA Status messages.

***********************************************************************/


#ifndef	RDA_STATUS_MESSAGE_H
#define	RDA_STATUS_MESSAGE_H


#include "rda_rpg_message_header.h"


#define	MAX_RDA_ALARMS_PER_MESSAGE	 14
#define	MAX_RDA_ALARM_NUMBER		800

/* Define RDA status halfword locations. Some HW locations have 
   duplicate macro definitions for one reason or another, including
   different meaning in ORDA. */
#define RS_RDA_STATUS                     1	/* also for display blanking */
#define RS_OPERABILITY_STATUS             2	/* also for display blanking */
#define RS_CONTROL_STATUS                 3
#define RS_AUX_POWER_GEN_STATE            4
#define RS_AVE_TRANS_POWER                5
#define RS_REFL_CALIB_CORRECTION          6	/* Horizontal Channel        */
#define RS_DATA_TRANS_ENABLED             7
#define RS_VCP_NUMBER                     8
#define RS_RDA_CONTROL_AUTH               9
#define RS_INTERFERENCE_DETECT_RATE      10	/* not valid for ORDA        */
#define RS_RDA_BUILD_NUM		 10	/* not valid for Legacy      */
#define RS_OPERATIONAL_MODE              11
#define RS_ISU                           12	/* not valid for ORDA        */
#define RS_SUPER_RES                     12	/* Only valid for ORDA       */
#define RS_ARCHIVE_II_STATUS             13	/* not valid for ORDA        */
#define RS_CMD                           13     /* Only valid for ORDA       */
#define RS_AVSET  		         14	/* Only valid for ORDA       */
#define RS_ARCHIVE_II_CAPACITY           14	/* not valid for ORDA        */
#define RS_RDA_ALARM_SUMMARY             15
#define RS_COMMAND_ACK                   16
#define RS_CHAN_CONTROL_STATUS           17
#define RS_SPOT_BLANKING_STATUS          18
#define RS_BPM_GEN_DATE                  19
#define RS_BPM_GEN_TIME                  20
#define RS_NWM_GEN_DATE                  21
#define RS_NWM_GEN_TIME                  22 
#define RS_SPARE1                        23	/* Only valid for Legacy     */
#define RS_VC_REFL_CALIB_CORRECTION      23	/* Vertical Channel          */
#define RS_SPARE2                        24     /* Only valid for Legacy     */
#define	RS_TPS_STATUS			 24
#define RS_SPARE3                        25
#define RS_RMS_CONTROL_STATUS            25
#define RS_SPARE4                        26     /* Only valid for Legacy     */
#define RS_PERF_CHECK_STATUS             26     /* Only valid for ORDA       */
#define	RS_ALARM_CODE1                   27
#define	RS_ALARM_CODE2                   28
#define	RS_ALARM_CODE3                   29
#define	RS_ALARM_CODE4                   30
#define	RS_ALARM_CODE5                   31
#define	RS_ALARM_CODE6                   32
#define	RS_ALARM_CODE7                   33
#define	RS_ALARM_CODE8                   34
#define	RS_ALARM_CODE9                   35
#define	RS_ALARM_CODE10                  36
#define	RS_ALARM_CODE11                  37
#define	RS_ALARM_CODE12                  38
#define	RS_ALARM_CODE13                  39
#define	RS_ALARM_CODE14                  40


/* Define Wideband Line Status Values. */
#define RS_NOT_IMPLEMENTED             0
#define RS_CONNECT_PENDING             1
#define RS_DISCONNECT_PENDING          2
#define RS_DISCONNECTED_HCI            3
#define RS_DISCONNECTED_CM             4
#define RS_DISCONNECTED_SHUTDOWN       5
#define RS_CONNECTED                   6
#define RS_DOWN                        7
#define RS_WBFAILURE                   8
#define RS_DISCONNECTED_RMS            9


/* Define Command Acknowledgement status values	*/
#define	RS_NO_ACKNOWLEDGEMENT			0
#define	RS_REMOTE_VCP_RECEIVED			1
#define	RS_CLUTTER_BYPASS_MAP_RECEIVED		2
#define	RS_CLUTTER_CENSOR_ZONES_RECEIVED	3
#define	RS_REDUND_CHNL_STBY_CMD_ACCEPTED	4


/* Define RDA status states. */
#define RS_STARTUP                   2
#define RS_STANDBY                   4
#define RS_RESTART                   8
#define RS_OPERATE                  16
#define RS_PLAYBACK                 32		/* Not valid for ORDA */
#define RS_OFFOPER                  64


/* Define RDA operability status. */
#define OS_INDETERMINATE             0
#define OS_ONLINE                    2
#define OS_MAINTENANCE_REQ           4
#define OS_MAINTENANCE_MAN           8
#define OS_COMMANDED_SHUTDOWN       16
#define OS_INOPERABLE               32
#define OS_WIDEBAND_DISCONNECT     128



/* TBD - the values defined below don't appear to agree with what's 
   defined in the ICD.  Perhaps we're not comparing the values
   directly.  Need to find out.  For instance, if the transfer switch
   manual bit is set in the msg, the value would be 8.  But the defined
   value below is 6. ????  But keep in mind that the Legacy ICD defines
   this field the same as the new ICD.  Apparently the software was
   working before, so probably will now?????? */

/* Auxilliary Power/Generator states. */
#define RS_COMMANDED_SWITCHOVER      8
#define AP_UTILITY_PWR_AVAIL         2
#define AP_GENERATOR_ON              4
#define AP_TRANS_SWITCH_MAN          6
#define AP_COMMAND_SWITCHOVER        RS_COMMANDED_SWITCHOVER
#define AP_SWITCH_AUX_PWR           16


/* Define Data Transmission Enabled states. */
#define BD_ENABLED_NONE              2
#define BD_REFLECTIVITY              4
#define BD_VELOCITY                  8
#define BD_WIDTH                    16
#define BD_ENABLED_ALL              28


/* Interference Suppression Unit - valid for Legacy RDA only */
#define ISU_ENABLED                  2
#define ISU_DISABLED                 4
#define ISU_NOCHANGE             32767


/* Super Resolution ... valid for ORDA only		     */
#define SR_NOCHANGE                 0
#define SR_ENABLED                  2
#define SR_DISABLED                 4

/* Clutter Mitigation Decision ... valid for ORDA only.  Note: 
   The value is a bit map, bit 0 is the enabled/disabled bit.*/
#define CMD_ENABLED                 1
#define CMD_DISABLED                0

/* Automated Volume Evaluation and Termination ... valid for ORDA only */
#define AVSET_ENABLED               2
#define AVSET_DISABLED              4

/* Define RDA Control Authorization states. */
#define CA_NO_ACTION                 0
#define CA_LOCAL_CONTROL_REQUESTED   2
#define CA_REMOTE_CONTROL_ENABLED    4

/* Define Performance Check status values. */
#define PC_AUTO			     0
#define PC_PENDING		     1

/* Define Control Status states. */
#define CS_LOCAL_ONLY                2
#define CS_RPG_REMOTE		     4
#define CS_EITHER		     8


/*
   TBD - the operational modes may be undergoing a change for ORDA.
   The action is pending review at the time of writing this note.
   The new values would be: 2=Maintenance (Test), 4=Operational, 8=Offline-Maintenance.
*/

/* Define Operational Modes */
#define OP_MAINTENANCE_MODE          2
#define OP_OPERATIONAL_MODE          4
#define OP_OFFLINE_MAINTENANCE_MODE  8


/*
   TBD - Can't find this "Auto Cal status" in the ICD.  Is this used locally?
   Or is it obsolete and subject to removal?
*/

/* Auto Cal status */
#define AC_AUTOCAL_NOCHANGE      32767


/* Define Redundant channel control status */
#define	RDA_IS_CONTROLLING           0
#define	RDA_IS_NON_CONTROLLING       1


/* Define Spot Blanking status */
#define SB_NOT_INSTALLED	     0
#define SB_ENABLED		     2
#define SB_DISABLED		     4


/* Define TPS status */
#define TP_NOT_INSTALLED	     0
#define TP_OFF			     1
#define TP_OK			     3


/* Define Archive II status - not valid for ORDA */
#define AR_NOT_INSTALLED             0
#define AR_INSTALLED                 1
#define AR_LOADED                    2
#define AR_RESERVED_WP               4
#define AR_RESERVED                  8
#define AR_RECORD                   16
#define AR_PLAYBACK_AVAIL           32
#define AR_SEARCH                   64
#define AR_PLAYBACK                128
#define AR_FAST_FRWRD              256
#define AR_CHECK_LABEL             512
#define AR_TAPE_TRANSFER          1024  


/* RDA Alarm Summary Categories.  Some values may have multiple 
   macro definitions due to differences between Legacy and Open RDA message
   definitions. */
#define AS_NO_ALARMS                 0
#define AS_TOW_UTIL                  2
#define AS_PEDESTAL                  4
#define AS_TRANSMITTER               8
#define AS_RECV_SIGPROC             16 /* Lgcy only */
#define AS_RECV			    16 /* ORDA only */
#define AS_RDA_CONTROL              32
#define AS_RPG_COMMUN               64 /* Lgcy only */
#define AS_RPG_COMMUN               64 /* ORDA only */
#define AS_USR_COMMUN              128 /* Lgcy only */
#define AS_SIGPROC		   128 /* ORDA only */
#define AS_ARCHIVE_II              256 /* Lgcy only */


/* Defined below are the alarm codes for the RDA alarm messages that
   are of particular interest to the RPG. */

#define	RDA_ALARM_NONE					0
#define	RDA_ALARM_RPG_LINK_MINOR_ALARM			24
#define	RDA_ALARM_RPG_LINK_FUSE_ALARM			25
#define	RDA_ALARM_RPG_LINK_MAJOR_ALARM			26
#define	RDA_ALARM_ROG_LINK_REMOTE_ALARM			27 
#define	RDA_ALARM_UNAUTHORIZED_SITE_ENTRY		144
#define RDA_ALARM_INVALID_REMOTE_VCP_RECEIVED		393
#define RDA_ALARM_REMOTE_VCP_NOT_DOWNLOADED		394
#define RDA_ALARM_INVALID_RPG_COMMAND_RECEIVED		395
#define RDA_ALARM_UNABLE_TO_CMD_OPER_REDUN_CHAN_ONLINE	552
#define	RDA_ALARM_INVALID_CENSOR_ZONE_MESSAGE_RECEIVED	679


/* Legacy RDA Status Message structure */
typedef	struct {

    RDA_RPG_message_header_t	msg_hdr;
    unsigned short	rda_status;		/*  Status of RDA as indicated:
						   <BIT> Set = Yes
						     1 = Start-Up
						     2 = Standby
						     3 = Restart
						     4 = Operate
						     5 = Playback
						     6 = Off-line Operate
						*/
    unsigned short	op_status;		/*  RDA Operability Status as indicated:
						   <BIT> Set = Yes
						     1 = On-line
						     2 = Maintenance Required
						     3 = Maintenance Mandatory
						     4 = Commanded Shut-down
						     5 = Inoperable
						    Above codes + 1 = Auto-calibration
						    disabled
						*/
    unsigned short	control_status; 	/*  RDA Control Status as indicated:
						   <BIT> Set = Yes
						     1 = Local Only
						     2 = Remote Only (RPG)
						     3 = Either
						     (Mutually Exclusive)
						*/
    unsigned short	aux_pwr_state;		/*  Auxilliary Power Generator State:
						   <BIT> Set = Yes
						     1 = Utility Power Available
						     2 = Generator On
						     3 = Transfer Switch (Manual)
						     4 = Commanded Switchover
						    Above codes + 1 = Switched to
						    Auxilliary Power
						*/
    short		ave_trans_pwr;		/*  Average transmitter power
						    (Range 0 to 9999)
						*/
    short		ref_calib_corr;		/*  Reflectivity calibration correction
						    (difference from adaptation data)
						    (Range -10 to 10 scaled by 4).
						*/
    unsigned short	data_trans_enbld;	/*  Data transmission enabled:
						   <BIT> Set = Yes
						     1 = None
						     2 = Reflectivity
						     3 = Velocity
						     4 = Spectrum Width
						*/
    short		vcp_num;		/*  Volume Coverage Pattern:
						     0       = No pattern
						     1 to 99 = Constant Elevation Types
						     <=255   = Operational
						     >255    = Maintenance/Test
						     Positive: RDA remote pattern (RPG)
						     Negative: RDA local pattern
						*/
    unsigned short	rda_control_auth;	/*  RDA control authorization:
						     0 = No action
						     2 = Local Control Requested
						     4 = Remote Control Enabled
						*/
    unsigned short	int_detect_rate;	/*  Interference detection rate. Number
						    of pulses detected per second.
						    (Range 0 to 32767)
						*/
    unsigned short	op_mode;		/*  Operational Mode:
						     2 = Maintenance
						     4 = Operational
						*/
    unsigned short	int_suppr_unit;		/*  Interference suppression unit:
						     2 = Enabled
						     4 = Disabled
						*/
    unsigned short	arch_II_status;		/*  Archive II Status:
						   <BIT> Set = Yes
						     0 = Installed
						     1 = Loaded
						     2 = Reserved (Write Protected)
						     3 = Reserved
						     4 = Record
						     5 = Playback Available (can be paired
							 with other codes)
						     6 = Search
						     7 = Playback
						     8 = Fast Forward
						     9 = Check Label
						     10 = Tape Transfer
						     12       = Tape#1
						     13       = Tape#2
						     12&13    = Tape#3
						     14       = Tape#4
						     12&14    = Tape#5
						     13&14    = Tape#6
						     12&13&14 = Tape#7
						     15       = Tape#8
						     12&15    = Tape#9
						     .....
						     Tape # bits may be paired
						     with bits 1-10.
						*/
    unsigned short	arch_II_capacity;	/*  Estimated number of volume scans
						    remaining on archive II tape
						    (worst case).
						    (Range 1 to 200)
						*/
    unsigned short	rda_alarm;		/*  RDA Alarm Summary:
						   <BIT> Set = Yes
						     0 = Alarms
						     1 = Tower/Utilities
						     2 = Pedestal
						     3 = Transmitter
						     4 = Receiver/Signal Processor
						     5 = RDA Control
						     6 = RPG Communication
						     7 = User Communication
						     8 = Archive II
						*/
    unsigned short	command_status;		/*  Command Acknowledgement:
						     0 = No acknowledgement
						     1 = Remote VCP Received
						     2 = Clutter bypass map received
						     3 = Clutter sensor zones received
						     4 = Redundant channel standby
							 command accepted
						*/
    unsigned short	channel_status;		/*  Channel control status.  Identifies
						    whether channel is controlling
						    channel.
						     0 = Controlling
						     1 = Non-controlling
						*/
    unsigned short	spot_blanking_status;	/*  Spot blanking status:
						     0 = Not installed
						     2 = Enabled
						     4 = Disabled
						*/
    unsigned short	bypass_map_date;	/*  Bypass map generation date (julian
						    date referenced from January 1, 1970
						    at 0000 UT; starts with 1).
						    (Range 1 to 65535)
						*/
    unsigned short	bypass_map_time;	/*  Bypass map generation time.  Number
						    of minutes since midnight (UT).
						    (Range 0 to 1440)
						*/
    unsigned short	notchwidth_map_date;	/*  Notchwidth map generation date
						    (julian date referenced from
						    January 1, 1970 at 0000 UT; starts
						    with 1).
						    (Range 1 to 65535)
						*/
    unsigned short	notchwidth_map_time;	/*  Notchwidth map generation time.
						    Number of minutes since midnight (UT).
						    (Range 1 to 1440)
						*/
    short		spare23;
    unsigned short	tps_status;		/*  Transition Power Source
						    status: 
							0 - Not Installed
							1 - OFF
							3 - OK
						*/
    unsigned short	rms_control_status;	/*  RMS Control Status:
							0 - Non-RMS system
							2 - RMS in control
							4 - RDA MMI in control
						*/
    short		spare26;
    short		alarm_code [MAX_RDA_ALARMS_PER_MESSAGE];
						/*  Alarm code.  One condition per
						    halfword with a maximum of 14
						    alarms sent at a time.  Code
						    ranges from 0 to 800.  MSB set
						    indicates alarm has been removed.
						*/
} RDA_status_msg_t;



/* Open RDA (ORDA) Status Message structure */
typedef	struct {

    RDA_RPG_message_header_t	msg_hdr;
    unsigned short	rda_status;		/*  Status of RDA as indicated:
						   <BIT> Set = Yes
						     1 = Start-Up
						     2 = Standby
						     3 = Restart
						     4 = Operate
						     5 = Spare
						     6 = Off-line Operate
						*/
    unsigned short	op_status;		/*  RDA Operability Status as indicated:
						   <BIT> Set = Yes
						     1 = On-line
						     2 = Maintenance Required
						     3 = Maintenance Mandatory
						     4 = Commanded Shut-down
						     5 = Inoperable
						    Above codes + 1 = Auto-calibration
						    disabled
						*/
    unsigned short	control_status; 	/*  RDA Control Status as indicated:
						   <BIT> Set = Yes
						     1 = Local Only
						     2 = Remote Only (RPG)
						     3 = Either
						     (Mutually Exclusive)
						*/
    unsigned short	aux_pwr_state;		/*  Auxilliary Power Generator State:
						   <BIT> Set = Yes
						     1 = Utility Power Available
						     2 = Generator On
						     3 = Transfer Switch (Manual)
						     4 = Commanded Switchover
						    Above codes + 1 = Switched to
						    Auxilliary Power
						*/
    short		ave_trans_pwr;		/*  Average transmitter power
						    (Range 0 to 9999)
						*/
    short		ref_calib_corr;		/*  Reflectivity calibration
						    correction, Horizontal Channel 
                                                    (delta dBZ0, difference from 
                                                     adaptation data)
						    (Range -198dB to 198dB).
						*/
    unsigned short	data_trans_enbld;	/*  Data transmission enabled:
						   <BIT> Set = Yes
						     1 = None
						     2 = Reflectivity
						     3 = Velocity
						     4 = Spectrum Width
						*/
    short		vcp_num;		/*  Volume Coverage Pattern:
						     0       = No pattern
						     1 to 99 = Constant Elevation Types
						     <=255   = Operational
						     >255    = Maintenance/Test
						     Positive: RDA remote pattern (RPG)
						     Negative: RDA local pattern
						*/
    unsigned short	rda_control_auth;	/*  RDA control authorization:
						     0 = No action
						     2 = Local Control Requested
						     4 = Remote Control Enabled
						*/
    short		rda_build_num;		/* RDA Build Number, scaled.
                                                   Major and minor build info.
						   Valid Range: 0 - 999
                                                   (ex.  71 = 7.1)
						*/


/* TBD - the op_mode has changed in the latest ORDA ICD.  However,
   the changes may be erased pending analysis.  If they are not
   erased, the values would be 2=Test, 4=Operational, 8=Maint. */

    unsigned short	op_mode;		/*  Operational Mode:
						     2 = Maintenance
						     4 = Operational
						*/
    unsigned short	super_res;		/*  Super Resolution:
						     2 = Enabled
						     4 = Disabled
						*/
    unsigned short	cmd;			/*  Clutter Mitigation Decision:
						     2 = Enabled
						     4 = Disabled
						*/
    unsigned short	avset;			/*  Automated Volume Evaluation and 
						    Termination:
						     2 = Enabled
						     4 = Disabled
						*/
    unsigned short	rda_alarm;		/*  RDA Alarm Summary:
                                                    No bits set = No alarms
						   <BIT> Set = Yes
						     1 = Tower/Utilities
						     2 = Pedestal
						     3 = Transmitter
						     4 = Receiver
						     5 = RDA Control
						     6 = Communication
						     7 = Signal Processor
						*/
    unsigned short	command_status;		/*  Command Acknowledgement:
						     0 = No acknowledgement
						     1 = Remote VCP Received
						     2 = Clutter bypass map received
						     3 = Clutter sensor zones received
						     4 = Redundant channel standby
							 command accepted
						*/
    unsigned short	channel_status;		/*  Channel control status.  Identifies
						    whether channel is controlling
						    channel.
						     0 = Controlling
						     1 = Non-controlling
						*/
    unsigned short	spot_blanking_status;	/*  Spot blanking status:
						     0 = Not installed
						     2 = Enabled
						     4 = Disabled
						*/
    unsigned short	bypass_map_date;	/*  Bypass map generation date (julian
						    date referenced from January 1, 1970
						    at 0000 UT; starts with 1).
						    (Range 1 to 65535)
						*/
    unsigned short	bypass_map_time;	/*  Bypass map generation time.  Number
						    of minutes since midnight (UT).
						    (Range 0 to 1440)
						*/
    unsigned short	clutter_map_date;	/*  Clutter filter map generation date
						    (julian date referenced from
						    January 1, 1970 at 0000 UT; starts
						    with 1).
						    (Range 1 to 65535)
						*/
    unsigned short	clutter_map_time;	/*  Clutter map generation time.
						    Number of minutes since midnight (UT).
						    (Range 1 to 1440)
						*/
    short		vc_ref_calib_corr;	/*  Reflectivity calibration
						    correction, Vertical Channel 
                                                    (delta dBZ0, difference from 
                                                     adaptation data)
						    (Range -198dB to 198dB).
						*/
    unsigned short	tps_status;		/*  Transition Power Source
						    status: 
							0 - Not Installed
							1 - OFF
							3 - OK
						*/
    unsigned short	rms_control_status;	/*  RMS Control Status:
							0 - Non-RMS system
							2 - RMS in control
							4 - RDA in control
						*/
    short		perf_check_status;	/*  Performance check status.
                                                        0 - Automatic
                                                        1 - Pending 
                                                */
    short		alarm_code [MAX_RDA_ALARMS_PER_MESSAGE];
						/*  Alarm code.  One condition per
						    halfword with a maximum of 14
						    alarms sent at a time.  Code
						    ranges from 0 to 800.  MSB set
						    indicates alarm has been removed.
						*/
} ORDA_status_msg_t;


#endif
