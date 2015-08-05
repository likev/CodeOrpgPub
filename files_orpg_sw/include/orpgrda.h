/****************************************************************
 *								*
 *  Module: orpgrda.h						*
 *								*
 *   Description: This is the header file associated with the	*
 *		 RDA status function group in libORPG.		*
 *		 RDA status section of libORPG.			*
 *								*
 ****************************************************************/

/*
 * RCS info 
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/08/07 19:33:38 $
 * $Id: orpgrda.h,v 1.43 2013/08/07 19:33:38 steves Exp $
 * $Revision: 1.43 $
 * $State: Exp $
 * $Log: orpgrda.h,v $
 * Revision 1.43  2013/08/07 19:33:38  steves
 * CCR NA13-00182
 *
 * Revision 1.42  2013/06/03 14:09:11  steves
 * CCR NA12-00145
 *
 * Revision 1.41  2013/05/31 19:11:58  steves
 * CCR NA12-00145
 *
 * Revision 1.40  2012/09/25 18:00:27  steves
 * issue 4-052
 *
 * Revision 1.39  2012/08/21 17:09:13  steves
 * issue 3-981
 *
 * Revision 1.38  2010/12/01 17:15:10  steves
 * issue 3-803
 *
 * Revision 1.37  2009/06/01 21:43:46  steves
 * issue 3-580
 *
 * Revision 1.36  2008/02/04 20:02:50  steves
 * Issue 3-411
 *
 * Revision 1.34  2007/03/07 18:59:34  steves
 * issue 3-241
 *
 * Revision 1.33  2007/01/11 19:34:30  steves
 * issue 3-046
 *
 * Revision 1.32  2007-01-11 09:27:45-06  steves
 * issue 3-046
 *
 * Revision 1.31  2006/10/03 17:20:15  steves
 * issue 3-046
 *
 * Revision 1.30  2005/11/16 20:57:47  ryans
 * Update macros for "who_sent_it" field of control cmd
 *
 * Revision 1.29  2005-10-18 15:11:28-05  steves
 * issue 2-868
 *
 * Revision 1.28  2005/03/25 16:09:43  steves
 * issue 2-591
 *
 * Revision 1.27  2004/12/21 19:34:00  steves
 * issue 2-566
 *
 * Revision 1.25  2004/07/20 15:07:01  ryans
 * Add RDA Version Num macro
 *
 * Revision 1.24  2004/06/24 17:12:52  ccalvert
 * fix bug in RDA Alarm gui and make "hot buttons" work with ORDA
 *
 * Revision 1.23  2004/06/22 23:31:43  ccalvert
 * add new ORDA devices COM, SIG, and RCV
 *
 * Revision 1.22  2004/06/10 19:28:17  steves
 * issue 2-394
 *
 * Revision 1.21  2004/06/09 17:11:29  steves
 * issue 2-394
 *
 * Revision 1.20  2004/06/04 19:03:46  steves
 * issue 2-394
 *
 * Revision 1.18  2004/02/04 22:54:26  ryans
 * Make the Set_rda_config_flag() a public function
 *
 * Revision 1.17  2004/01/21 22:41:18  ryans
 * Update for ORDA support
 *
 * Revision 1.16  2003/07/25 22:21:15  ryans
 * Header file for ORPGRDA lib needs updating.
 *
 * Revision 1.15  2003/06/23 21:47:47  ryans
 * Header file for ORPGRDA lib needs updating.
 *
 * Revision 1.14  2003/06/12 21:39:01  ryans
 * ORPGRDA library header file needs update to support ORDA.
 *
 * Revision 1.13  2003/06/12 20:24:34  ryans
 * Modifications to support new ORDA system.
 *
 * Revision 1.12  2000/11/02 21:24:43  priegni
 * Issue 1-055, 1-082
 *
 * Revision 1.11  2000/05/05 16:44:16  steves
 * Add SB support
 *
 * Revision 1.10  2000/02/02 19:02:25  garyg
 * Add function prototype ORPGRDA_read_alarms ()
 *
 * Revision 1.9  2000/01/28 21:28:10  cm
 * fix
 *
 * Revision 1.8  1999/11/17 18:57:24  priegni
 * add channel number to data structure for redundancy requirement
 *
 * Revision 1.7  1999/11/10 14:46:35  priegni
 * add new macro for wb failure
 *
 * Revision 1.6  1999/11/01 19:31:54  priegni
 * Add function to return RDA channel number
 *
 * Revision 1.5  1999/08/26 21:17:02  priegni
 * replace ORPGRDA_DEVICE_TOW with ORPGRDA_DEVICE_UTL
 *
 * Revision 1.4  1999/08/23 15:36:10  priegni
 * Add RDA Controlling/Non-Controlling to definitions
 *
 * Revision 1.3  1999/06/17 13:04:28  eforren
 * Add iostatus function
 *
 * Revision 1.2  1999/05/07 20:07:46  priegni
 * Add update time functions
 *
*/

#ifndef ORPGRDA_STATUS_H
#define ORPGRDA_STATUS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <rda_status.h>
#include <gen_stat_msg.h>
#include <rda_control.h>
#include <rss_replace.h>
#include <orpgvcp.h>


/* RDA Status message size in halfwords */
#define ORPGRDA_RDA_STATUS_MSG_SIZE	40

/* Message Header size in halfwords */
#define ORPGRDA_MSG_HEADER_SIZE		8


/*	Various return status values for RDA status functions	*/
#define ORPGRDA_SUCCESS			0
#define	ORPGRDA_DATA_NOT_FOUND		-9999
#define	ORPGRDA_ERROR			-9999


#define	ORPGRDA_UNLOCK			0
#define	ORPGRDA_LOCK			1

/*	Variable nmae for the RDA configuration adaptation parameter.	*/
#ifndef RDA_CONFIGURATION
#define RDA_CONFIGURATION		"site_info.is_orda"
#define RDA_CONFIGURATION_GROUP		"site_info"
#endif


/*	Macro definitions for RDA Alarm state codes			*/
enum   { ORPGRDA_STATE_NOT_APPLICABLE=0,
	 ORPGRDA_STATE_MAINTENANCE_MANDATORY,
	 ORPGRDA_STATE_MAINTENANCE_REQUIRED,
	 ORPGRDA_STATE_INOPERATIVE,
	 ORPGRDA_STATE_SECONDARY
};


/*	Macro definitions for RDA Alarm type codes.			*/
enum   { ORPGRDA_TYPE_EDGE_DETECTED=1,
	 ORPGRDA_TYPE_FILTERED_OCCURRENCE,
	 ORPGRDA_TYPE_OCCURRENCE	
};


/*	Macro definitions for RDA Alarm deice codes.			*/
enum   { ORPGRDA_DEVICE_XMT=1,
	 ORPGRDA_DEVICE_UTL,
	 ORPGRDA_DEVICE_RSP,
	 ORPGRDA_DEVICE_CTR,
	 ORPGRDA_DEVICE_PED,
	 ORPGRDA_DEVICE_ARC,
	 ORPGRDA_DEVICE_USR,
	 ORPGRDA_DEVICE_RPG,
	 ORPGRDA_DEVICE_WID,
	 ORPGRDA_DEVICE_RCV=3, 
	 ORPGRDA_DEVICE_SIG=6, 
	 ORPGRDA_DEVICE_COM=9 
};


/*	Macro definitions for RDA_alarm_t structure elements.		*/
enum   { ORPGRDA_ALARM_MONTH=1,
	 ORPGRDA_ALARM_DAY,
	 ORPGRDA_ALARM_YEAR,
	 ORPGRDA_ALARM_HOUR,
	 ORPGRDA_ALARM_MINUTE,
	 ORPGRDA_ALARM_SECOND,
	 ORPGRDA_ALARM_CODE,
	 ORPGRDA_ALARM_ALARM,
	 ORPGRDA_ALARM_CHANNEL
};


/*	Macro definitions for RDA configuration types */
enum   { ORPGRDA_LEGACY_CONFIG = 0,
	 ORPGRDA_ORDA_CONFIG = 1
};


/*	Macro definitions for RDA status change indicators */
enum   { ORPGRDA_STATUS_UNCHANGED	= 0,
	 ORPGRDA_STATUS_CHANGED		= 1
};


/*	Macro definitions for RDA status fields */
enum   { ORPGRDA_RDA_STATUS			= RS_RDA_STATUS,
	 ORPGRDA_OPERABILITY_STATUS		= RS_OPERABILITY_STATUS,
	 ORPGRDA_CONTROL_STATUS			= RS_CONTROL_STATUS,
	 ORPGRDA_AUX_POWER_GEN_STATE		= RS_AUX_POWER_GEN_STATE,
	 ORPGRDA_AVE_TRANS_POWER		= RS_AVE_TRANS_POWER,
	 ORPGRDA_REFL_CALIB_CORRECTION		= RS_REFL_CALIB_CORRECTION,
	 ORPGRDA_DATA_TRANS_ENABLED		= RS_DATA_TRANS_ENABLED,
	 ORPGRDA_VCP_NUMBER			= RS_VCP_NUMBER,
	 ORPGRDA_RDA_CONTROL_AUTH		= RS_RDA_CONTROL_AUTH,
	 ORPGRDA_INTERFERENCE_DETECT_RATE 	= RS_INTERFERENCE_DETECT_RATE,
	 ORPGRDA_RDA_BUILD_NUM			= RS_RDA_BUILD_NUM,
	 ORPGRDA_OPERATIONAL_MODE		= RS_OPERATIONAL_MODE,
	 ORPGRDA_ISU				= RS_ISU,
	 ORPGRDA_SUPER_RES			= RS_SUPER_RES,
	 ORPGRDA_ARCHIVE_II_STATUS		= RS_ARCHIVE_II_STATUS,
	 ORPGRDA_CMD				= RS_CMD,
	 ORPGRDA_ARCHIVE_II_CAPACITY		= RS_ARCHIVE_II_CAPACITY,
	 ORPGRDA_AVSET				= RS_AVSET,
	 ORPGRDA_RDA_ALARM_SUMMARY		= RS_RDA_ALARM_SUMMARY,
	 ORPGRDA_COMMAND_ACK			= RS_COMMAND_ACK,
	 ORPGRDA_CHAN_CONTROL_STATUS		= RS_CHAN_CONTROL_STATUS,
	 ORPGRDA_SPOT_BLANKING_STATUS		= RS_SPOT_BLANKING_STATUS,
	 ORPGRDA_BPM_GEN_DATE			= RS_BPM_GEN_DATE,
	 ORPGRDA_BPM_GEN_TIME			= RS_BPM_GEN_TIME,
	 ORPGRDA_NWM_GEN_DATE			= RS_NWM_GEN_DATE,
	 ORPGRDA_NWM_GEN_TIME			= RS_NWM_GEN_TIME,
	 ORPGRDA_VC_REFL_CALIB_CORRECTION	= RS_VC_REFL_CALIB_CORRECTION,
	 ORPGRDA_SPARE2				= RS_SPARE2,
	 ORPGRDA_TPS_STATUS			= RS_TPS_STATUS,
	 ORPGRDA_SPARE3				= RS_SPARE3,
	 ORPGRDA_RMS_CONTROL_STATUS		= RS_RMS_CONTROL_STATUS,
	 ORPGRDA_SPARE4				= RS_SPARE4,
	 ORPGRDA_PERF_CHECK_STATUS		= RS_PERF_CHECK_STATUS,
	 ORPGRDA_ALARM_CODE1			= RS_ALARM_CODE1,
	 ORPGRDA_ALARM_CODE2			= RS_ALARM_CODE2,
	 ORPGRDA_ALARM_CODE3			= RS_ALARM_CODE3,
	 ORPGRDA_ALARM_CODE4			= RS_ALARM_CODE4,
	 ORPGRDA_ALARM_CODE5			= RS_ALARM_CODE5,
	 ORPGRDA_ALARM_CODE6			= RS_ALARM_CODE6,
	 ORPGRDA_ALARM_CODE7			= RS_ALARM_CODE7,
	 ORPGRDA_ALARM_CODE8			= RS_ALARM_CODE8,
	 ORPGRDA_ALARM_CODE9			= RS_ALARM_CODE9,
	 ORPGRDA_ALARM_CODE10			= RS_ALARM_CODE10,
	 ORPGRDA_ALARM_CODE11			= RS_ALARM_CODE11,
	 ORPGRDA_ALARM_CODE12			= RS_ALARM_CODE12,
	 ORPGRDA_ALARM_CODE13			= RS_ALARM_CODE13,
	 ORPGRDA_ALARM_CODE14			= RS_ALARM_CODE14
};


/*	Macros for defining wideband status elements.		*/
#define	ORPGRDA_WBLNSTAT		0
#define ORPGRDA_DISPLAY_BLANKING	1
#define	ORPGRDA_WBFAILED		2


/*	Macro definitions for wideband line status values */
enum   { ORPGRDA_NOT_IMPLEMENTED		= RS_NOT_IMPLEMENTED,
	 ORPGRDA_CONNECT_PENDING		= RS_CONNECT_PENDING,
	 ORPGRDA_DISCONNECT_PENDING		= RS_DISCONNECT_PENDING,
	 ORPGRDA_DISCONNECTED_HCI		= RS_DISCONNECTED_HCI,
	 ORPGRDA_DISCONNECTED_CM		= RS_DISCONNECTED_CM,
	 ORPGRDA_DISCONNECTED_SHUTDOWN		= RS_DISCONNECTED_SHUTDOWN,
	 ORPGRDA_CONNECTED			= RS_CONNECTED,
	 ORPGRDA_DOWN				= RS_DOWN,
	 ORPGRDA_WBFAILURE			= RS_WBFAILURE,
	 ORPGRDA_DISCONNECTED_RMS		= RS_DISCONNECTED_RMS
};


#define RDA_COMMAND_MAX_MESSAGE_SIZE 2000

/*	Define the originators of the RDA control command.              */
#define HCI_INITIATED_RDA_CTRL_CMD 	-100    /* HCI initiated RDA control	*
					           command			*/
#define PBD_INITIATED_RDA_CTRL_CMD 	-200	/* PBD initiated RDA control	*
					            command			*/
#define RMS_INITIATED_RDA_CTRL_CMD 	-300	/* RMS initiated RDA control	*
					           command			*/
#define MSF_INITIATED_RDA_CTRL_CMD 	-400	/* Mode Selection Function 	*
					         * initiated RDA control 	*
					         * command			*/
#define RED_INITIATED_RDA_CTRL_CMD 	-500	/* Redundant mgr initiated	*
					           RDA control command          */
#define APP_INITIATED_RDA_CTRL_CMD 	-600    /* Application initiated        */
#define HCI_VCP_INITIATED_RDA_CTRL_CMD 	-700    /* Application initiated        */
#define TST_INITIATED_RDA_CTRL_CMD 	-999	/* RDA control command issued   *
                                                   from testing tool            */

/*	Definitions for buffer elements of RDA command data written	*
 *	to RDA control linear buffer.					*/

#define	CPC4M_COMMAND		 0	/*  Command type element	*/
#define	CPC4M_LINENO		 1	/*  Channel #			*/
#define	CPC4M_P1		 2	/*  RDA Command number		*/
#define	CPC4M_P2		 3	/*  Command Data		*/
#define	CPC4M_P3		 4	/*  Command Data		*/
#define	CPC4M_MSGDATA		 5	/*  Message Data 		*/

/*	Command Type codes						*/

#define COM4_LBTEST_RDAtoRPG   403	/*  RDA/RPG Loopback Message    */ 
#define COM4_LBTEST	       404	/*  RPG/RDA Loopback Message    */ 
#define COM4_WBMSG	       406	/*  RPG/RDA Console Message     */ 
#define COM4_WBENABLE	       407	/*  Wideband Line Enable        */ 
#define COM4_WBDISABLE	       408	/*  Wideband Line Disable       */ 
#define COM4_WMVCPCHG          410	/*  Weather Mode Change         */ 
#define	COM4_RDACOM	       411	/*  RDA Control Command Code	*/
#define	COM4_REQRDADATA	       412	/*  Request RDA Data        	*/
#define	COM4_DLOADVCP  	       413	/*  Download VCP        	*/
#define	COM4_SENDEDCLBY	       414	/*  Download Clutter Bypass Map */
#define	COM4_SENDCLCZ  	       415	/*  Download Clutter Sensor Zones*/
#define COM4_SB_ENAB           420      /*  Spot Blanking Enabled       */
#define COM4_SB_DIS            421      /*  Spot Blanking Disabled      */
#define COM4_VEL_RESO          422      /*  VCP Velocity Resolution     */
#define	COM4_HEARTBEATLB       500	/*  Send Heartbeat Loopback Msg */

#define COM4_PBD_TEST         1000      /*  For PBD Testing Purposes    */

/*	RDA command codes (COM4_RDACOM parameter_1)			*/

#define	CRDA_STANDBY		 1	/*  RDA Standby State		*/
#define	CRDA_OFFOPER		 2	/*  RDA Offline Operate State	*/
#define	CRDA_OPERATE		 3	/*  RDA Operate State		*/
#define	CRDA_RESTART		 4	/*  RDA Restart State		*/
#define	CRDA_BDENABLE		 5	/*  Enable All Data fields	*/
#define	CRDA_REQREMOTE		10	/*  Request RDA Remote Control	*/
#define	CRDA_ACCREMOTE		11	/*  Accept RDA Remote Control	*/
#define	CRDA_ENALOCAL		12	/*  Enable RDA Local Control	*/
#define	CRDA_AUTOCAL		13	/*  Calibration Override Cmd 
                                            (Legacy Only)	        */
#define	CRDA_CTLISU		14	/*  Interference Suppression	*/
#define	CRDA_AUXGEN		15	/*  Switch to Auxiliary Power	*/
#define	CRDA_RESTART_VCP	16	/*  Volume Restart Command	*/
#define	CRDA_RESTART_ELEV	17	/*  Elevation Restart Command	*/
#define	CRDA_SELECT_VCP		18	/*  Select new VCP		*/
#define	CRDA_UTIL		20	/*  Switch to Utility Power	*/
#define	CRDA_ARCH_COLLECT	21	/*  Start Archive II collection	*/
#define	CRDA_ARCH_REPLAY	22	/*  Playback Archive II data	*/
#define	CRDA_ARCH_STOP		23	/*  Stop Archive II collection	*/
#define	CRDA_FORCE_RED_STANDBY	24	/*  Force Redundant Standby	*/
#define	CRDA_MODE_OP		25	/*  Switch to Operational Mode	*/
#define	CRDA_MODE_MNT		26	/*  Switch to Maintenance Mode	*/
#define	CRDA_CHAN_CTL		27	/*  Cmd RDA to Controlling	*/
#define	CRDA_CHAN_NONCTL	28	/*  Cmd RDA to Non-Controlling	*/
#define	CRDA_SB_ENAB		29	/*  Enable Spot Blanking	*/
#define	CRDA_SB_DIS		30	/*  Disable Spot Blanking	*/
#define CRDA_SR_ENAB		31	/*  Super Res Enable		*/
#define CRDA_SR_DISAB		32	/*  Super Res Disable		*/
#define CRDA_CMD_ENAB		33	/*  CMD Enable			*/
#define CRDA_CMD_DISAB		34	/*  CMD Disable			*/
#define CRDA_AVSET_ENAB		35	/*  AVSET Enable		*/
#define CRDA_AVSET_DISAB	36	/*  AVSET Disable		*/
#define CRDA_PERF_CHECK		37	/*  Perform Performance Check   */

/*	Velocity Resolution codes (COM4_VEL_RESO parameter_1)		*/

#define CRDA_VEL_RESO_LOW       ORPGVCP_VEL_RESOLUTION_LOW
                                        /*  Velocity Resolution Low     */
#define CRDA_VEL_RESO_HIGH      ORPGVCP_VEL_RESOLUTION_HIGH
                                        /*  Velocity Resolution High    */

#define CRDA_UNEX_VOL_START   1000      /*  For Testing Unexpected      */
                                        /*  Volume Start                */
#define CRDA_UNEX_ELV_START   1001      /*  For Testing Unexpected      */
                                        /*  Elevation Start             */

/*	RDA Data Request Types						*/

#define	ORPGRDA_MAX_CMD_MSG_LEN	1600

typedef struct {

   int cmd;
   int line_num;
   int param1;
   int param2;
   int param3;
   int param4;
   int param5;
   char msg[ORPGRDA_MAX_CMD_MSG_LEN];

} Rda_cmd_t;

/*	The following header files are for RDA alarm processing.	*/

#include <rda_alarm_data.h>

/*	Prototypes for RDA status functions.				*/

int	ORPGRDA_alarm_io_status ();
int	ORPGRDA_read_status_msg ();
int	ORPGRDA_write_status_msg (char *buf);
int	ORPGRDA_get_status (int item);
int	ORPGRDA_channel_num ();
int	ORPGRDA_status_io_status ();
int	ORPGRDA_status_update_flag ();
time_t	ORPGRDA_status_update_time ();
double	ORPGRDA_last_status_update ();
int	ORPGRDA_get_rda_config ( void* msg_ptr );
int	ORPGRDA_set_rda_config( void* msg_ptr );
int	ORPGRDA_get_alarm_codes (int* alarm_code_ptr);
int	ORPGRDA_check_status_change ( short* new_msg_data );
int	ORPGRDA_set_wb_status ( int field_id, int value );
int	ORPGRDA_set_status ( int field_id, int value );
int	ORPGRDA_read_previous_state ();
int	ORPGRDA_set_state ( int field_id, int value );
int	ORPGRDA_write_state ();
int	ORPGRDA_get_previous_state ( int field_id );
int	ORPGRDA_get_previous_state_vcp ( char *vcp, int *size );
int	ORPGRDA_set_state_vcp ( char *vcp, int size );
int	ORPGRDA_get_previous_status ( int field_id );
void	ORPGRDA_update_system_status();
void	ORPGRDA_report_previous_state();


/*	Prototypes for RDA-RPG wideband functions.			*/

int	ORPGRDA_get_wb_status (int item);

/*	Prototypes for HCI RDA Alarm data functions.			*/

int	ORPGRDA_read_alarm_msg  ();
int	ORPGRDA_write_alarm_msg (char *buf);
int	ORPGRDA_get_num_alarms  ();
int	ORPGRDA_get_alarm       (int indx, int item);
void	ORPGRDA_clear_alarms();
int	ORPGRDA_read_alarms();
int	ORPGRDA_alarm_update_flag ();
time_t	ORPGRDA_alarm_update_time ();
int	ORPGRDA_clear_alarm_codes ();

/*	Prototypes for RDA control command functions.			*/

int	ORPGRDA_send_cmd( int cmd, int who_sent_it, int p1, int p2,
		          int p3, int p4, int p5, char *msg, ... );

#ifdef __cplusplus
}
#endif

#endif

