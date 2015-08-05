/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/11 17:21:47 $
 * $Id: orpgdat.h,v 1.170 2014/08/11 17:21:47 steves Exp $
 * $Revision: 1.170 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpdat.h

 Description: Global Open Systems Radar Product Generator (ORPG) data
              header file.
       Notes: All constants defined in this file begin with the prefix
              ORPGDAT_.
 **************************************************************************/


#ifndef ORPGDAT_H
#define ORPGDAT_H

#include <rda_performance_maintenance.h>
#include <rda_rpg_console_message.h>
#include <alg_cpu_stats.h>

#define ORPGDAT_BASE		3000
				/* This must be larger than any defined RPG 
				   buffer type numbers */

#define ORPGSC_LBNAME_COL	1
				/* column number of the LB names in the system 
				   configuration text */

#define ORPGDAT_ADAPTATION	3000
				/* working adaptation data LB */

#define ORPGDAT_PROD_REQUESTS	3001
				/* ORPG product requests 		*/
				/* re: prod_request.h			*/
	/* SMI_struct Prod_request_msg_t ORPGDAT_PROD_REQUESTS; */

#define ORPGDAT_PROD_GEN_MSGS	3002
				/* product generation message LB */
				/* re: prod_gen_msg.h            */
	/* SMI_struct Prod_gen_msg ORPGDAT_PROD_GEN_MSGS; */

#define ORPGDAT_SCAN_SUMMARY    3003 
                               	/* RPG Scan Summary data. */

	/* SMI_struct Summary_Data ORPGDAT_GSM_DATA.SCAN_SUMMARY_ID; */

#define ORPGDAT_HYOCCULT        3004
                               /* Occultation Adaptation Data File */

#define ORPGDAT_HYSECTRS        3005
                               /* Hybrid Sectors Adaptation Data File*/

#define ORPGDAT_ACCDATA         3006
				/* Volume and elevation accounting data	*/
				/* re: basedata.h			*/
	/* SMI_struct Radial_accounting_data	ORPGDAT_ACCDATA; */

#define ORPGDAT_RDA_STATUS      3007
#define ORPGDAT_GSM_DATA        ORPGDAT_RDA_STATUS
                               	/* RDA Status data, volume-based status, and */
				/* RPG status data for General Status Message.*/
				/* re: gen_stat_msg.h			*/

	/* SMI_struct RDA_status_t ORPGDAT_GSM_DATA.RDA_STATUS_ID;
	   SMI_struct Vol_stat_gsm_t ORPGDAT_GSM_DATA.VOL_STAT_GSM_ID;
	   SMI_struct Previous_state_t ORPGDAT_GSM_DATA.PREV_RDA_STAT_ID; */

#define ORPGDAT_CLUTTERMAP        3008
                               /* Clutter Filter Map data, Clutter 
				  Filter Bypass Map data, and Censor Zone 
				  Data. */
	/* 
   SMI_struct RDA_bypass_map_msg_t
      ORPGDAT_CLUTTERMAP.LBID_BYPASSMAP_LGCY;
   SMI_struct ORDA_bypass_map_msg_t
      ORPGDAT_CLUTTERMAP.LBID_BYPASSMAP_ORDA;
   SMI_struct RDA_bypass_map_msg_t
      ORPGDAT_CLUTTERMAP.LBID_EDBYPASSMAP_LGCY;
   SMI_struct ORDA_bypass_map_msg_t
      ORPGDAT_CLUTTERMAP.LBID_EDBYPASSMAP_ORDA;
   SMI_struct RDA_notch_map_msg_t
      ORPGDAT_CLUTTERMAP.LBID_CLUTTERMAP_LGCY;
   SMI_struct ORDA_clutter_map_msg_t
      ORPGDAT_CLUTTERMAP.LBID_CLUTTERMAP_ORDA;
   SMI_struct RPG_clutter_regions_msg_t
      ORPGDAT_CLUTTERMAP.LBID_CENSOR_ZONES_LGCY;
   SMI_struct ORPG_clutter_regions_msg_t
      ORPGDAT_CLUTTERMAP.LBID_CENSOR_ZONES_ORDA;
   SMI_struct B9_ORPG_clutter_regions_msg_t
      ORPGDAT_CLUTTERMAP.LBID_CENSOR_ZONES_ORDA;
   SMI_struct RPG_clutter_regions_msg_t
      ORPGDAT_CLUTTERMAP.LBID_BASELINE_CENSOR_ZONES_LGCY; 
   SMI_struct ORPG_clutter_regions_msg_t
      ORPGDAT_CLUTTERMAP.LBID_BASELINE_CENSOR_ZONES_ORDA; 
   SMI_struct B9_ORPG_clutter_regions_msg_t
      ORPGDAT_CLUTTERMAP.LBID_BASELINE_CENSOR_ZONES_ORDA; 
	*/ 

#define ORPGRAT_ALARMS_TBL_MSG_ID  1
#define ORPGRAT_ORDA_ALARMS_TBL_MSG_ID  2
#define ORPGDAT_RDA_ALARMS_TBL   3009
			       /* RDA Alarms Table */
	/* SMI_struct RDA_alarm_entry_t	ORPGDAT_RDA_ALARMS_TBL.ORPGRAT_ALARMS_TBL_MSG_ID; */
	/* SMI_struct RDA_alarm_entry_t	ORPGDAT_RDA_ALARMS_TBL,ORPGRAT_ORDA_ALARMS_TABLE_MSG_ID; */

#define	ORPGDAT_RDA_ALARMS	3010
			       /* RDA Alarms data */
	/* SMI_struct RDA_alarm_t	ORPGDAT_RDA_ALARMS; */

#define	ORPGDAT_RDA_COMMAND	3011
			       /* RDA Command data */
	/* SMI_struct Rda_cmd_t		ORPGDAT_RDA_COMMAND; */

#define	ORPGDAT_RDA_PERF_MAIN   3012
			       /* RDA Performance/Maintenance data 	*/
				/* re: rda_performance_maintenance.h	*/
	/* SMI_struct rda_performance_t	ORPGDAT_RDA_PERF_MAIN; */

#define	ORPGDAT_RDA_CONSOLE_MSG 3013
			       /* RDA Console Message data */
	/* SMI_struct RDA_RPG_console_message_t ORPGDAT_RDA_CONSOLE_MSG; */

#define ORPGDAT_SUPPL_VCP_INFO  3014
			       /* Supplemental VCP Information ....
			          VCP translation table .. */

#define ORPGDAT_RDA_ADAPT_MSG_ID  1
#define	ORPGDAT_RDA_ADAPT_DATA	3015
			       /* RDA Adaptation data			*/

#define ORPGDAT_WX_ALERT_REQ_MSG  3016
				/* Weather Alert Request Message	*/

#define ORPGDAT_TERRAIN         3017
                                /* Terrain data file 			*/

#define ORPGDAT_BLOCKAGE        3018
                                /* Beam Blockage data file		*/

#define ORPGDAT_RDA_VCP_MSG_ID  1
#define	ORPGDAT_RDA_VCP_DATA	3019
			       /* RDA VCP data				*/

/*
 * 3020 - 3039: Manage RPG
 */
#define ORPGDAT_SYSLOG	        3020
				/* writer: many                         */
				/* reader: HCI				*/
				/* Critical LE System Log Msgs		*/
				/* generated by various RPG tasks       */
				/* re: le.h (TBD) 	                */
	/* SMI_struct LE_critical_message ORPGDAT_SYSLOG; */

#define ORPGDAT_RPG_INFO	3021
				/* writer: Control RPG                  */
				/* reader: many				            */
				/* includes State File, Task Attribute  */
				/* Table (TAT), RPG Node List, MSCF RPG */
				/* Command, RMMS RPG Command, Admin RPG */
				/* Command                              */
				/* re: orpginfo.h, orpgtat.h */
	/*
   SMI_struct Orpginfo_statefl_t ORPGDAT_RPG_INFO.ORPGINFO_STATEFL_MSGID;
   SMI_struct Orpginfo_statefl_shared_t 
				ORPGDAT_RPG_INFO.ORPGINFO_STATEFL_SHARED_MSGID;
   SMI_struct Int_message ORPGDAT_RPG_INFO.ORPGINFO_ENDIANVALUE_MSGID;
	*/

#define ORPGDAT_TASK_STATUS	3023
				/* RPG Task Status LB File              */
				/* writer: Monitor RPG                  */
				/* reader: Control RPG; other RPG procs */
				/* re: orpgstat.h                       */
	/*
   SMI_struct Mrpg_state_t ORPGDAT_TASK_STATUS.MRPG_RPG_STATE_MSGID;
   SMI_struct Mrpg_process_status_msg_t ORPGDAT_TASK_STATUS.MRPG_PS_MSGID;
   SMI_struct Mrpg_process_table_msg_t ORPGDAT_TASK_STATUS.MRPG_PT_MSGID;
   SMI_struct Mrpg_node_msg_t ORPGDAT_TASK_STATUS.MRPG_RPG_NODE_MSGID;
   SMI_struct Mrpg_data_msg_t ORPGDAT_TASK_STATUS.MRPG_RPG_DATA_MSGID;
	*/

#define ORPGDAT_ERRLOG	        3024
				/* writer: many                         */
				/* reader: HCI				*/
				/* Critical LE System Error Msgs	*/
				/* generated by various RPG tasks       */
				/* re: le.h (TBD) 	                */
	/* SMI_struct LE_critical_message ORPGDAT_ERRLOG; */

#define ORPGDAT_SYSLOG_SHADOW	3025
				/* writer: status_prod                  */
				/* reader: status_prod			*/
				/* Critical LE System Log Msgs		*/
				/* re: le.h (TBD) 	                */
	/* SMI_struct LE_critical_message ORPGDAT_SYSLOG_SHADOW; */

#define ORPGDAT_PAT	        3026
				/* RPG Product Attribute Table LB File  */

   	/*
   SMI_struct Pd_attr_entry_msg_t ORPGDAT_PAT.PROD_ATTR_MSG_ID;
   	*/

#define ORPGDAT_TAT	        3027
				/* RPG Task Attribute Table LB File     */
				/* Each LB message corresponds to a     */
				/* given Task (msgid == Task ID)        */
				/* re: orpgtat.h                        */
	/* SMI_struct Orpgtat_entry_t ORPGDAT_TAT; */

#define ORPGDAT_MRPG_CMDS	3028
				/* mrpg command queue: RPG applications
				   write and mrpg reads. The message is
				   defined in mrpg.h.			*/
	/* SMI_struct Mrpg_cmd_t ORPGDAT_MRPG_CMDS; */

#define ORPGDAT_UNUSED_329	3029

#define ORPGDAT_UNUSED_330      3030

#define ORPGDAT_MNGRPG_332      3032
#define ORPGDAT_MNGRPG_333      3033
#define ORPGDAT_MNGRPG_334      3034
#define ORPGDAT_MNGRPG_335      3035
#define ORPGDAT_MNGRPG_336      3036
#define ORPGDAT_MNGRPG_337      3037
#define ORPGDAT_MNGRPG_338      3038
#define ORPGDAT_MNGRPG_339      3039

/***
 *** END OF Manage RPG Data IDs
 ***/


#define ORPGDAT_REDMGR_CMDS	3040
				/* Redundant Manager ORPG derived cmds	*
				 * This is a single msg LB that		*
				 * contains commands generated from 	*
				 * orpg processes commanding the active	*
				 * channel redundant mgr to transmit	*
				 * specific data/cmds to the inactive	*
				 * channel.				*
				 * writers: HCI, RMS, Control_RDA	*
				 * refer: orpgred.h			*/

#define ORPGDAT_REDMGR_CHAN_MSGS 3041
				/* Redundant Manager Channel messages	*
				 * This LB contains three replacable	*
				 * messages.				*
				 * msg 1 contains channel status info.	*
				 * msg 2 contains the redundant 	*
				 * channel's channel status info.	*
				 * msg 3 contains inter-process 	*
				 * commands sent between the two 	*
				 * channels' redundant mgrs		*
				 * writer: Redundant Manager		*
				 * refer: orpgred.h			*/

#define	ORPGDAT_LOAD_SHED_CAT	3050
				/* Load Shed Category information file.	*
				 * writer: ORPGLSC library		*
				 * 1st message contains thresholds.	*
				 * These values should only be changed	*
				 * by the HCI and read by others.	*
				 * 2nd message contains current level.	*
				 * These should be changed by applic.   *
				 * and read by HCI.			*
				 * refer to orpglsc.h for msg IDs.	*/
	/*
   SMI_struct load_shed_threshold_t 
		ORPGDAT_LOAD_SHED_CAT.LOAD_SHED_THRESHOLD_MSG_ID;
   SMI_struct load_shed_current_t 
		ORPGDAT_LOAD_SHED_CAT.LOAD_SHED_CURRENT_MSG_ID;
   SMI_struct load_shed_threshold_t 
		ORPGDAT_LOAD_SHED_CAT.LOAD_SHED_THRESHOLD_BASELINE_MSG_ID;
	*/

#define	ORPGDAT_PRODUCT_PARAMS	3051
				/* Selectable Product Parameter adapt	*
				 * data file.  Refer to the header	*
				 * file orpgadpt.h for specific msg	*
				 * IDs.					*/

#define ORPGDAT_NODE_VERSION            3054
                                /* contains the node/version info       *
                                 * for the current version of adapt     *
                                 * data.                                */

#define ORPGDAT_ARCHIVE_II_INFO            3055
                                /* contains the data that is used to    *
                                 * status information across hci and    *
                                 * bdds  				*/

#define ORPGDAT_LDM_WRITER_INPUT       3059
                                /* contains data to be written to LDM.  */
#define ORPGDAT_LDM_READER_INPUT       3060
                                /* contains data to be read from LDM.  */

#define	ORPGDAT_RECOMBINED_RAWDATA	55
				/* Id of the output buffer used by the	*/
				/* Super Resolution recombine alg.	*/
				/* There's no macro for product ids.	*/

#define ORPGDAT_LEGACY_HYOCCULT_DAT 3096
#define ORPGDAT_LEGACY_HYSECTRS_DAT 3097

#define	ORPGDAT_HCI_CCZ_BREF	3098
				/* id of the data buffer used by the	*/
				/* HCI CCZ editor for getting background*/
				/* base reflectivity data. 		*/

#define	ORPGDAT_HCI_DATA	3099
				/* HCI data */
	/*
   SMI_struct Hci_gui_t        ORPGDAT_HCI_DATA.HCI_GUI_INFO_MSG_ID;
   SMI_struct Hci_rms_status_t ORPGDAT_HCI_DATA.HCI_RMS_STATUS_MSG_ID;
   SMI_struct Hci_task_t       ORPGDAT_HCI_DATA.HCI_TASK_INFO_MSG_ID;
   SMI_struct Hci_ccz_data_t   ORPGDAT_HCI_DATA.HCI_CCZ_TASK_DATA_MSG_ID;
   SMI_struct Hci_prf_data_t   ORPGDAT_HCI_DATA.HCI_PRF_TASK_DATA_MSG_ID;
   SMI_struct Precip_status_t  ORPGDAT_HCI_DATA.HCI_PRECIP_STATUS_MSG_ID;
	*/


#define ORPGDAT_PROD_INFO 	4000
				/* product generation and distribution	*/
				/* info (adaptation) LB 		*/
				/* re: prod_distri_info.h		*/
	/* 
   SMI_struct Pd_distri_info_msg_t ORPGDAT_PROD_INFO.PD_LINE_INFO_MSG_ID;
   SMI_struct Pd_prod_entry_msg_t ORPGDAT_PROD_INFO.PD_CURRENT_PROD_MSG_ID;
   SMI_struct Pd_prod_entry_msg_t ORPGDAT_PROD_INFO.PD_DEFAULT_A_PROD_MSG_ID;
   SMI_struct Pd_prod_entry_msg_t ORPGDAT_PROD_INFO.PD_DEFAULT_B_PROD_MSG_ID;
   SMI_struct Pd_prod_entry_msg_t ORPGDAT_PROD_INFO.PD_DEFAULT_M_PROD_MSG_ID;
	*/

#define ORPGDAT_PROD_STATUS 	4001
				/* ORPG Product Status			*/
				/* writer: Schedule Routine Products    */
				/* re: prod_status.h			*/
	/* SMI_struct Prod_gen_status_msg_t 
				ORPGDAT_PROD_STATUS.PROD_STATUS_MSG; */

#define ORPGDAT_ALG_CPU_STATS   4002
				/* Algorithm CPU statistics.		*/
				/* writer:  librpg.			*/
				/* re:  alg_cpu_stats.h			*/
	/* SMI_struct alg_cpu_stats_t ORPGDAT_ALG_CPU_STATS; */

#define ORPGDAT_HARDWARE_CONFIG	4003
				/* Hardware configuration data.		*/
				/* writer:  hci_hardware_config.	*/
				/* re:  hardware_config.h		*/
	/* SMI_struct hardware_config_t ORPGDAT_HARDWARE_CONFIG; */

#define ORPGDAT_CM_REQUEST	4010
				/* comm_manager request data store. We need 
				   to reserve 100 number */

#define ORPGDAT_CM_RESPONSE	5010
				/* comm_manager response data store. We need 
				   to reserve 100 number */


#define ORPGDAT_OT_RESPONSE	6011
				/* one-time product scheduler response data 
				   store. We need to reserve 100 number */

#define ORPGDAT_RT_RESPONSE	7011
				/* routine product scheduler response data 
				   store. We need to reserve 100 number */

#define ORPGDAT_OT_REQUEST	8011
				/* data store for messages from p_server to 
				   ps_onetime */

#define ORPGDAT_RT_REQUEST	8012
				/* data store for messages from p_server to 
				   ps_routine */

#define ORPGDAT_PROD_USER_STATUS	8013
				/* product user (line) status 		*/
				/* writer: Serve Products 		*/
				/* re: prod_status.h			*/
				/* re: prod_user_msg.h			*/

#define ORPGDAT_PRODUCTS	8014
				/* The ORPG product LB. ALL final products
				   are written into this LB by librpg. */

#define ORPGDAT_PSQ_REQUEST	8015
				/* Product storage query request LB. Any ORPG
				   process can write a query request in this
				   LB and expect an response in 
				   ORPGDAT_PSQ_RESPONSE */
#define ORPGDAT_PSQ_RESPONSE	8016
				/* LB for Product storage query response msgs.
				   A response will have the same msg ID as its
				   request msg */
				   
#define ORPGDAT_USER_PROFILES		8017
					/* The ORPG user profile data base LB */	/* SMI_struct Pd_user_entry ORPGDAT_USER_PROFILES; */

#define ORPGDAT_FACTORY_USER_PROFILES	8019
					/* The factory default ORPG user 
					   profile data base LB */
	/* SMI_struct Pd_user_entry ORPGDAT_FACTORY_USER_PROFILES; */

#define ORPGDAT_REPLAY_REQUESTS 8020
				/* one-time request from replay product
				   generation requests. */
	/* SMI_struct Prod_request_msg_t ORPGDAT_REPLAY_REQUESTS; */

#define ORPGDAT_REPLAY_RESPONSES 8021
				/* one-time response from replay product
				   generation. */
	/* SMI_struct Prod_gen_msg ORPGDAT_REPLAY_RESPONSES; */

#define ORPGDAT_BASEDATA_REPLAY 8022
                                /* Data store for BASEDATA replay data. */

#define ORPGDAT_BASEDATA_ACCT   8023
                                /* Data store for BASEDATA replay 
                                   accounting data. */

#define ORPGDAT_SYSLOG_LATEST  8092  
				/*  Abbreviated syslog data store that
				   only contains the latest status
				  and alarm messages.  Used to
				  improve the performance of low
				  bandwith HCI */
	/* SMI_struct LE_critical_message ORPGDAT_SYSLOG_LATEST.1;
	   SMI_struct LE_critical_message ORPGDAT_SYSLOG_LATEST.2; */

#define ORPGDAT_PRF_COMMAND_INFO 	9000
#define ORPGDAT_PRF_COMMAND_MSGID	1
#define ORPGDAT_PRF_STATUS_MSGID	2

#define ORPGDAT_RMS_TEXT_MSG	9004
				/* RMS free text message 		*/
				/* writer: HCI console callback		*/

#define STORM_1D_DATA           9010
                                /* Stores the data from storm1d */
#define STORM_2D_DATA           9011
                                /* stores the data from storm2d */
#define STORM_2D_GROUP_DATA     9012
                                /* Stores the data from storm2dgroup */
#define STORM_3D_DATA           9013
                                /* Stores the data from storm3d  */
#define STORM_4D_DATA           9014
                                /* Stores the data from storm4d  */

#define ORPGDAT_ADAPT_DATA	9090
				/* ORPG adaptation data */

/*  Product IDs 1000-1999 are being reserved for externally generated	*
 *  products.								*/

#define	ORPGDAT_PUP_TXT_MSG	1000
				/* PUP free text message (Product 77)	*/

typedef struct {  /* struct for a high shear feature detected by veldeal */
    int vol_seq;	/* volume sequence number */
    int time;		/* volume time (UNIX time) */
    short elev_num;
    short range, azi;	/* feature location: start range bin index and azi
			   index (both start with 1) */
    short n_bins, n_rs;	/* feature size: number of bins, number of radials */
    short n;		/* number of high shear borders */
    short maxs, mins;	/* the maximum and minimum high shear in the feature. 
			   In .5 m/s per gate size */
    short nyq, unamb_range;	/* Nyquist velocity (each unit is .5 m/s) and
				   unamb_range as in Base_data_header */
    short type;		/* type of the record: 0 for dealiase-failure feature
			   and 1 for high shear feature */
    short min_size;	/* minimum size (number of gates) used in detection */
    float threshold;	/* threshold (m/s) used in detection */
} ORPG_hsf_t;

#define ORPGDAT_BIAS_TABLE_MSG_ID     1
#define ORPGDAT_RUC_DATA_MSG_ID       2
#define ORPGDAT_VELDEAL_HSF_MSG_ID       3
	/* The message contains an array of ORPG_hsf_t which are the 
	   dealiase-failure features and high shear features detected in the 
	   latest two volumes. The elements in the array are in detection 
	   time order with the latest being the first. */
#define ORPGDAT_ENVIRON_DATA_MSG  10002
				/* Bias table message (Message 15)	*/

#define ORPGDAT_GAGEDATA        10003
#define GAGEDATA                ORPGDAT_GAGEDATA
				/* Gage Database. */

#define ORPGDAT_HYUSRSEL         10004
#define HYUSRSEL                 ORPGDAT_HYUSRSEL
				/* User Selectable Precipitation Database. */

#define	ORPGDAT_ADAPT		  15000	  /*  ORPG main adaptation data file */
#define ORPGDAT_BASELINE_ADAPT	  15001	  /*  ORPG baseline adaptation data file */

/*  These stores are obsolete - provided for backward compatibility */
#define ORPGDAT_PROD_PARAMS_ADAPT 15000    /*  Product Parameters Adaptation Data Store */
#define ORPGDAT_MISC_ADAPT        15000    /*  Miscelleneous Adaptation Data Store */
#define ORPGDAT_ALGORITHM_ADAPT  15000    /*  Algorithms data store */


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* NOTE:  Data Store IDs in the 300000 - 400000 range are reserved for algorithm use */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/* Data store to support Snow Accumulation Algorithm User Selectable. */
#define SAAUSERSEL                300000

/* Data stores to support DP Long Term Accumulation Algorithm */
#define DP_OLD_RATE               300001 /* stores 1 scan-to-scan accum       */
#define DUAUSERSEL                300002 /* stores 336 DUA scan-to-scan accums*/
#define DP_HRLY_BACKUP            300003 /* stores 2 hourly circular queues   */
#define DP_STORM_BACKUP           300004 /* stores 2 storm totals             */
#define DP_HRLY_ACCUM             300005 /* stores 30 hourly accum bufs       */
#define DP_DIFF_ACCUM             300006 /* stores 30 hourly diff accum bufs  */

#define DP_ISDP_EST_MSGID         1
#define DP_ISDP_EST               300007 /* stored estimated ISDP             */
#define DP_BRAGG_MSGID            2
#define DP_BRAGG                  300008 /* stored Bragg scatter state data   */

/* Data stores to support model data extensions */
#define MODEL_FRZ_GRID            300100 /* stores number and heights of 0 degC crossovers */
#define MODEL_FRZ_ID              1
#define MODEL_CIP_GRID            300101 /* stores final CIP calculations in grid          */
#define MODEL_CIP_ID              2

#endif /*DO NOT REMOVE!*/
