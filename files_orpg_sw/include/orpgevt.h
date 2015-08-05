/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 16:31:41 $
 * $Id: orpgevt.h,v 1.120 2014/10/03 16:31:41 steves Exp $
 * $Revision: 1.120 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpgevt.h

 Description: Global Open Systems Radar Product Generator (ORPG) events
              header file.
       Notes: All constants defined in this file begin with the prefix
              ORPGEVT_.  All typedef's defined in this file SHOULD begin
              with the prefix Orpgevt_.
 **************************************************************************/


#ifndef ORPGEVT_H
#define ORPGEVT_H

#include <en.h>
#include <lb.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 *
 */
#define ORPGEVT_EVTCD_BASE	((EN_POST_MAX_RESERVED_EVTCD) + 1)

/**
  * Global ORPG Event Codes
  */
                               /** Used by orpg library to indicate no event
                                  registered. */
#define ORPGEVT_NULL_EVENT      0

                               /** ORPG Configuration Change               */
#define ORPGEVT_CFG_CHANGE	17


#define ORPGEVT_LE_BASE 18
#define ORPGEVT_LE_CRITICAL	((ORPGEVT_LE_BASE) + 0)
#define ORPGEVT_LE_VL_CHNG	((ORPGEVT_LE_BASE) + 1)
				/* Here we simply reserve these events; */
				/* always use the LE_DIR_EVENT env var,	*/
				/* which for ORPG will have the value of*/
				/* ORPGEVT_LE_BASE.                     */
				/* re: le.h                             */
				/* 	LE_event_message                    */
				/* 	LE_critical_message                 */

                               /** Event code to be used to indicate that
                                 * new RDA control command data was put
                                 * in the RDA command linear buffer
                                 *
                                 * NOTE: Control RDA now uses LB notification.
                                 */
#define ORPGEVT_RDA_CONTROL_COMMAND	100

                               /** Event code to be used to indicate that
                                 * a CPC4MSG was generated with an associated
                                 * event.
                                 */
#define ORPGEVT_CPC4MSG			101

/** RDA Status Changed Event
  *
  * POSTED BY: cpc004 tsk003 control_rda 
  */
#define ORPGEVT_RDA_STATUS_CHANGE	102

/** RDA Comms Status Changed Event
  *
  * POSTED BY: cpc004 tsk003 control_rda 
  */
#define ORPGEVT_RDA_RPG_COMMS_STATUS    ORPGEVT_RDA_STATUS_CHANGE

/** Scan Information Event Definition
  *
  * POSTED BY: cpc004 tsk002 pbd 
  */
#define ORPGEVT_SCAN_INFO		103

#define ORPGEVT_BEGIN_VOL		  1
#define ORPGEVT_END_VOL			  2
#define ORPGEVT_BEGIN_ELEV  		  3
#define ORPGEVT_END_ELEV  		  4


typedef struct {
   int          vol_scan_number;	/* 1 - 80 */

   int          elev_cut_number;	/* 1 - 20 */

   unsigned int date;			/* Radial date, Modified Julian */

   unsigned int time;			/* Radial time, milliseconds since
					   midnight */

   int          vcp_number;		/* 1 - ? */

   int 		super_res;		/*  Super Resolution flag.
                                            1 - 1/2 deg azimuth. */

} orpgevt_scan_info_data_t;

typedef orpgevt_scan_info_data_t Orpgevt_scan_info_data_t ;

typedef struct {
   int key;
   orpgevt_scan_info_data_t data;
} orpgevt_scan_info_t;

typedef orpgevt_scan_info_t Orpgevt_scan_info_t ;

#define ORPGEVT_SCAN_INFO_DATA_LEN 	sizeof(int)+sizeof(orpgevt_scan_info_t)


/** Event posted when RPG status has changed.
  */
#define ORPGEVT_RPG_STATUS_CHANGE	104

/** Event code to be used to indicate that
  * a clutter filter bypass map has been
  * received from the RDA.
  */
#define ORPGEVT_BYPASS_MAP_RECEIVED	105

/** Event posted upon receipt of clutter filter notchwidth map from
  * RDA.
  *
  */
#define ORPGEVT_CLUTTER_MAP_RECEIVED	106

/** Event posted at start of volume scan.
  *
  * POSTED BY: cpc004 tsk002 pbd
  */
#define ORPGEVT_START_OF_VOLUME		107

/** Amplification data associated with ORPGEVT_START_OF_VOLUME event. */
#define SOV_VCP_SUPPL_SCANS             1
typedef struct {

   unsigned int flags;          /* Bit map of various flags. */

} orpgevt_start_of_volume_t;

#define ORPGEVT_START_OF_VOLUME_DATA_LEN   sizeof(orpgevt_start_of_volume_t)


/** Event posted upon receipt of ... from RDA.
  *
  * POSTED BY: cpc004 tsk003 control_rda
  */
#define ORPGEVT_PERF_MAIN_RECEIVED	108

/** Event posted at end of volume scan.
  *
  * POSTED BY: cpc004 tsk002 pbd
  */
#define ORPGEVT_END_OF_VOLUME           109

/** Amplification data associated with ORPGEVT_END_OF_VOLUME event.        */ 
typedef struct {

   int vol_aborted; 		/* If set, volume scan did not complete. */

   unsigned int vol_seq_num; 	/* Volume scan sequence number. */

   unsigned int expected_vol_dur; /* Expected volume scan duration, in 
                                     seconds.  Set to 0 if volume aborted. */
   
}  orpgevt_end_of_volume_t;
   
#define ORPGEVT_END_OF_VOLUME_DATA_LEN   sizeof( orpgevt_end_of_volume_t )

#define ORPGEVT_PBD_DEBUG   		110

/**
  *
  * POSTED BY: cpc004 tsk003 control_rda
  */
#define ORPGEVT_CONTROL_RDA_DEBUG       111

/** Event posted upon precipitation category change.
  *
  * POSTED BY: cpc005 tsk002 pcipdalg
  */
#define ORPGEVT_PRECIP_CAT_CHNG		112

/**
  *
  * POSTED BY: cpc004 tsk003 control_rda
  */
#define ORPGEVT_RDA_ALARMS_UPDATE	113

/** Event posted when alerting adaptation data have been updated or
  * otherwise modified.
  *
  * POSTED BY: cpc001 tsk001 hci
  */
#define ORPGEVT_WX_ALERT_ADAPT_UPDATE   114

/** Event posted when an alert message has been generated.
  *
  * POSTED BY: cpc008 tsk001 alerting
  */
#define ORPGEVT_WX_ALERT_MESSAGE        115

/** Event posted when an alert paired product request
  * has been made.
  *
  * POSTED BY: cpc008 tsk001 alerting
  */
#define ORPGEVT_WX_ALERT_OT_REQ         116

/** Event posted upon generation of a user alert
  * message.
  *
  * POSTED BY: cpc008 tsk001 alerting
  */
#define ORPGEVT_WX_USER_ALERT_MSG	117

/** Event posted when precip threshold adapt data
  * have been modified. 
  *
  * POSTED BY: cpc001 tsk001 hci
  */
#define	ORPGEVT_PRECIP_THRESH_DATA	118

/** Event posted by mrpg when any ORPG global data store has been created. 
  *
  * POSTED BY: cpc111 tsk001 mrpg
  */
#define	ORPGEVT_DATA_STORE_CREATED	119

/** Event posted at start of volume scan data flow.
  *
  * POSTED BY: cpc004 tsk002 pbd
  */
#define ORPGEVT_START_OF_VOLUME_DATA	120

#define ORPGEVT_RADIAL_ACCT             121

#define RADIAL_ACCT_REFLECTIVITY	1
#define RADIAL_ACCT_VELOCITY		2
#define RADIAL_ACCT_WIDTH		4
#define RADIAL_ACCT_DUALPOL		8

/* ORPGEVT_RADIAL_ACCT message structure. */
typedef struct {

   short azimuth;               /* Azimuth angle (deg*10) */

   short azi_num;               /* Azimuth number */

   short elevation;             /* Elevation angle (deg*10) */

   unsigned char elev_num;      /* Elevation number (1-25) */

   unsigned char last_ele_flag; /* Last elevation flag */

   short radial_status;         /* Radial status */

   unsigned char n_sails_cuts;  /* Number of SAILS cuts this VCP. */

   unsigned char sails_cut_seq; /* SAILS cut sequence number. */

   short super_res;             /* Super Resolution flag.
                                    1 - 1/2 deg azimuth
                                    2 - 1/4 km reflectivity
                                    4 - 300 km Doppler. */

   short start_elev_azm;        /* Start of elevation azimuth angle
                                   (deg*10) */

   short moments;               /* Moments/data available this cut.
                                           1 - Reflectivity
                                           2 - Velocity 
                                           4 - Spectrum width
                                           8 - Dual Pol */

   short spare1;		/* Reserved for future expansion. */

   short spare2;		/* Reserved for future expansion. */
 
   short spare3;		/* Reserved for future expansion. */

} Orpgevt_radial_acct_t ;

#define ORPGEVT_RADIAL_ACCT_LEN        sizeof(Orpgevt_radial_acct_t)


/** Event for querying mrpg about sys_cfg info. Posted by any process as the
    request and by mrpg as the response. The response msg is an ASCII string -
    the "Version line" of sys_cfg. In order to be able to detect without 
    knowing the redundant type, we must use a group-independent event number.
    */
#define	ORPGEVT_SYSTEM_CONFIG_INFO	(122 + 0x1000000)

/** Event posted when a State File Flag has been updated.
  *
  * The event message is defined in orpginfo.h
  */
#define ORPGEVT_STATEFL_FLAG 123

/** Posted at the start of last elevation when AVSET is active
  * and AVSET terminates the volume scan.  This event includes
  * a Scan_Summary structure message.
  *
  * POSTED BY: cpc004 tsk002 pbd 
  */
#define ORPGEVT_LAST_ELEV_CUT	124

/** Posted every five minutes in absence of
  * System Status Messages being reported.
  *
  * POSTED BY: cpc107 tsk002 Monitor RPG
  */
#define ORPGEVT_NO_STATUS_CHANGE      129


/** Event posted when the contents of one of the Load Shed category
  * messages has changed.
  *
  * The message ID is passed in the event message.
  */
#define	ORPGEVT_LOAD_SHED_CAT	130

/** ORPGEVT_LOAD_SHED_CAT message */
typedef struct {
    int msg_id ;
} Orpgevt_load_shed_msg_t ;

/** EVENT CODE 131 IS AVAILABLE
  */
#define ORPGEVT_UNUSED_131      131

/** Event posted when one of the ORPG adaptation data messages has been
  * updated.
  *
  * The event msg indicates the ORPG data store ID
  * (see orpgdat.h) and the adaptation	data message ID (see orpgadpt.h)
  * which has been updated.
  */
#define	ORPGEVT_ADAPT_UPDATE		132

/** ORPGEVT_ADAPT_UPDATE message */
typedef struct {
    LB_id_t data_id;	/* ORPGDAT data ID which contains message that	*
			 * has been updated.				*/
    int msg_id ;	/* ID of message containing modified adaptation	*
			 * data.					*/
} Orpgevt_adapt_msg_t ;

/***
 *** ???
 ***/
/* This event indicates that a message	*
 * in the LB containing selectable prod	*
 * parameters (ORPGDAT_PRODUCT_PARAMS)	*
 * has been updated.  			*/

/** Event posted when a State File RPG Alarms bitflag has been updated.
  *
  * The event message is defined in orpginfo.h.
  * POSTED BY: cpc101 lib003 ORPGINFO sub-library
  */
#define ORPGEVT_RPG_ALARM 133

/** Event posted when a State File RPG Operability Status bitflag has been
  * updated.
  *
  * The event message is defined in orpginfo.h.
  * POSTED BY: cpc101 lib003 ORPGINFO sub-library
  */
#define ORPGEVT_RPG_OPSTAT_CHANGE 134

/** Event posted when a process comletes its initialization and is ready to
  * operate.
  *
  * The event message is the process's PID in ASCII form.
  * POSTED BY: all control tasks and tasks specified in task table.
  */
#define ORPGEVT_PROCESS_READY 135

/* Event posted manually. */
#define ORPGEVT_MSCF_PWR_CTRL_VERBOSE	151
#define ORPGEVT_MSCF_PWR_CTRL_GETNAMES  152


#define ORPGEVT_NB_COMM_REQ		200
				/* a NB comm_manager request is sent by 
				   a p_server instance;
				   ORPGEVT_NB_COMM_REQ + comm_manager_index
				   is posted.
				   We need to reserve 100 numbers here. 
				   (max number of comm_manager instances) */
#define ORPGEVT_NB_COMM_RESP		300
				/* a NB comm_manager response is sent by 
				   a comm_manager instance; 
				   ORPGEVT_NB_COMM_RESP + link_index is posted;
				   We need to reserve 100 numbers here. 
				   (max number of comm links) */

#define ORPGEVT_PROD_USER_STATUS	400
				/* A product user status message in
				   ORPGDAT_PROD_USER_STATUS is updated by 
				   p_server. The event message (a byte) is the 
				   line index (i.e. the message ID) */

#define ORPGEVT_PD_LINE			401
				/* product distribution line info updated 
				   by HCI */
#define ORPGEVT_PD_USER			402
				/* product distribution user profile updated 
				   by HCI */
#define ORPGEVT_PROD_LIST		403
				/* default product generation list updated 
				   by HCI.
				   NOTE:  Since there are 4 different tables (one
				   for each Wx mode and the current table, the ID
				   of the table which is being updated is passed
				   as data in the event message.  One should typecast
				   the message to an int.  The values should be
				   one of the following: ORPGPGT_DEFAULT_M_TABLE,
				   ORPGPGT_DEFAULT_B_TABLE, ORPGPGT_DEFAULT_A_TABLE,
				   ORPGPGT_CURRENT_TABLE			*/

/** Event posted by p_server when status changes for a narrowband line.	*
  * The data that is written to ORPGDAT_PROD_USER_STATUS is also sent	*
  * with this event.  See Prod_user_status type in header file		*
  * prod_status.h.							*/

#define ORPGEVT_PROD_USER_STATUS_DATA   405


#define ORPGEVT_OT_REQUEST		408
				/* one-time product request is sent from 
				   p_server */
#define ORPGEVT_RT_REQUEST		409
				/* routine product request is sent from 
				   p_server */
#define ORPGEVT_OT_RESPONSE		410
				/* one-time product response is sent from 
				   ps_onetime.
				   ORPGEVT_OT_RESPONSE + p_server_index is
				   posted.
				   We need to reserve 100 numbers here. 
				   (max number of p_server instances) */

#define ORPGEVT_PROD_STATUS		510
				/* product status is updated by ps_routine */

#define ORPGEVT_WX_UPDATED		511
				/* product schedule updated by ps_routine
				   upon a weather mode change */

#define ORPGEVT_PROD_GEN_CONTROL        512
				/* product gen. control is updated by 
				   ps_routine */

#define ORPGEVT_OT_SCHEDULE_LIST	513
				/* a new list of products to be scheduled is 
				   sent from ps_onetime to ps_routine */

#define	ORPGEVT_ENVWND_UPDATE		600
				/* a part of the environmental wind table
				   has been changed (either data or VAD
				   update flag). */

#define	ORPGEVT_AUTOPRF_FLAG_CHANGE	601
				/* The auto prf flag has been changed by
				   the hci. */

#define ORPGEVT_HCI_COMMAND_ISSUED	602
				/* A command has been issued by the HCI	*/

#define ORPGEVT_EDITED_RCM_READY	603
				/* An edited RCM product is ready for 
				   distribution. This is a p_server private 
				   event. */

/* ORPGEVT_HCI_CHILD_IS_STARTED event message. */				   
typedef struct {

        pid_t parent_pid;       /*  Pid of the parent HCI process */
        int child_id;           /*  Child ID associated with the HCI child process */

} hci_child_started_event_t;

#define ORPGEVT_HCI_CHILD_IS_STARTED 	604	
				/*  An HCI child process has completed its startup operations */

#define	ORPGEVT_FREE_TXT_MSG		605
				/*  A Free text message has been created by the
				    HCI and is ready to be sent to narrowband
				    users.  */
#define	ORPGEVT_PUP_TXT_MSG		606
				/*  A Free text message has been created by the
				    PUP and has been received by the ORPG */
#define ORPGEVT_TERM_PRECIP_ALGS	607
				/* Event serviced by mnnttsk_hydromet. */
#define ORPGEVT_TERM_LDM		608
				/* Event serviced by mnnttsk_lem. */

#define ORPGEVT_STATISTICS_PERIOD	610
		/* Event for changing product distribution statistics report 
		   period. The event message carries the period (in number 
		   seconds) in the null-terminated ASCII string format. */

#define ORPGEVT_RMS_MSG_RECEIVED	700
				/* A message has been received from RMS	*/				
				
#define ORPGEVT_RMS_SEND_MSG 		701
				/* A message needs to be sent to RMS */
				
#define ORPGEVT_RMS_INHIBIT_MSG 	702
				/* An inhibit message needs to be sent to RMS */	
				
#define ORPGEVT_RMS_TEXT_MSG 		703
				/* A free text message needs to be sent to RMS */

#define ORPGEVT_REPLAY_RESPONSES        704
                                /* a response to a replay request has been posted on the LB */
                                
#define ORPGEVT_RMS_CHANGE		705
				/* A change has occured to the RMS interface status */
				
#define ORPGEVT_RESET_RMS		706
				/* Reset the RMS interface */
				
#define ORPGEVT_RMS_SOCKET_MGR_FAILED   707
				/* Notify parent the socket mgr process failed */
				
#define ORPGEVT_ARCHIII_STATUS 801
        /* send archive status to HCI */

#define ORPGEVT_ARCHIII_CMD 802
        /* A message is sent from HCI to archiveIII,
				 * the structure used is archIII_command */

#define ORPGEVT_PROD_DB_DELETE 803
        /* A message is sent to product database to delete expired prodcts */

/* 20111207 Ward CCR NA11-00373: Added ORPGEVT_RESTART_LT_DIFF event */

#define ORPGEVT_RESTART_LT_ACCUM 1000 /* Restart DP long term accum      */
#define ORPGEVT_RESTART_LT_DIFF  1001 /* Restart DP long term difference */

#define ORPGEVT_RESET_SAAACCUM         300000

typedef struct Avset_info {

   int RDA_cut_num;

   float elevation;

} Avset_info_t;

#define ORPGEVT_AVSET_INFO_LEN         sizeof(Avset_info_t)

#define ORPGEVT_AVSET_TERMINATE        400000

#ifdef __cplusplus
}
#endif

#endif /*DO NOT REMOVE!*/
