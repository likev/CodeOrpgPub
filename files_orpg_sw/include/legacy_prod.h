
/***********************************************************************

    Description: header file defining data structures for product user
		messages. Refer to RPG/APUP ICD.

***********************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/10/12 19:09:19 $
 * $Id: legacy_prod.h,v 1.18 2012/10/12 19:09:19 steves Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */  

#ifndef LEGACY_PROD_H

#define LEGACY_PROD_H

/* for c++ compatability */
#ifdef __cplusplus
extern "C"
{
#endif

/* Product Message Header. */
typedef struct {		/* message header block */
    short msg_code;		/* message code: -131 - -16; 0 - 211 */
    short date;			/* date of the message; modified Julian date */
    short timem;		/* the most significant short of time of the 
				   message; GMT; seconds after 
				   midnight, 1/1/1970 */
    short timel;		/* the least significant short of time of 
				   the message; GMT; seconds after 
				   midnight, 1/1/1970 */
    short lengthm;		/* the most significant short of message 
				   length; number of bytes including 
				   this header */
    short lengthl;		/* the least significant short of message 
				   length; number of bytes including 
				   this header */
    short src_id;		/* ID of the sender: 0 - 999 */
    short dest_id;		/* ID of the receiver: 0 - 999 */
    short n_blocks;		/* header block plus the product description 
				   blocks in the message */
} Prod_msg_header_icd;




/* Type Definitions For Incoming PUP/APUP Messages. */

#define USER_PASSWD_LEN 6	/* user passwd buffer size	     */
#define PORT_PASSWD_LEN 4	/* port passwd buffer size	     */
				   
/* Sign-on Request Message. */
typedef struct {		/* Sign-on message.                          */
    Prod_msg_header_icd mhb;	/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* block length = 18: Number of bytes from -1 
				   divider to end of block.                  */
    char user_passwd[USER_PASSWD_LEN];
                  		/* 6 chraracter user password.               */
    char port_passwd[PORT_PASSWD_LEN];
                  		/* 4 chraracter user password.               */
    short disconn_override_flag;
                  		/* 0, 1 (yes). override automatic disconnect 
				   feature.*/
    short spare;            	/* spare. 				     */         
} Prod_sign_on_msg_icd;



/* Maximum Connect Time Disable Request Message. */
typedef struct {		/* Max connect time disable request.         */
    Prod_msg_header_icd mhb;	/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* block length = 10: Number of bytes including 
				   divider and length field.                 */
    short add_conn_time;	/* additional connect time. 0 to 1440.minutes */
    short spare0;		/* spare.				     */
    short spare1;		/* spare.				     */
} Prod_max_conn_time_disable_msg_icd;



/* RCM Edit/Noedit Message. */
typedef struct {                /* RCM Edit/Noedit Message */
   Prod_msg_header_icd mhb;    /* message header block */
   short divider;              /* block devider = -1 */
   short length;               /* block length = 12: Number of bytes to follow */
   short date;                 /* volume date */
   short timem;                /* volume time (most significant short) */
   short timel;                /* volume time (least significant short) */
   short seq_number;           /* sequence number. -13, 0 to 32767. */
   short v_num;                /* volume scan number */
   short edit_flag;            /* 0 - not editing; 1 - PUP will edit msg */
} Prod_RCM_edit_noedit_msg_icd;  



/* Type Definitions For Outgoing PUP/APUP Messages. */

/* Request Response Message Format. */
typedef struct {		/* Request Response Message */
    Prod_msg_header_icd mhb;    /* message header block */
    short divider;              /* divider = -1 */
    short length;               /* block length = 26: Number of bytes to follow */
    short error_codem;		/* error code (most significant 2 bytes). See ICD */
    short error_codel;		/* error code (least significant 2 bytes). See ICD */
    short seq_number;		/* sequence number. -13, 0 to 32767. */
    short msg_code;		/* product/message code. -16 to -131, 1 to 131. */
    short elev_angle;		/* elevation angle in .1 degrees */
    short vol_date;		/* volume scan start date (Modified Julian) */
    short vol_time_msw;		/* volume scan start time (secs since midnight) MSW */
    short vol_time_lsw;		/* volume scan start time (secs since midnight) LSW */
    short spare3;		/* spare */
    short spare4;		/* spare */
    short spare5;		/* spare */
    short spare6;		/* spare */
    short spare7;		/* spare */
} Prod_request_response_msg_icd;

#define RR_NO_SUCH_MESSAGE      0x80000000 
#define RR_NO_SUCH_PRODUCT      0x40000000 
#define RR_PRODUCT_NOT_GEN      0x20000000 
#define RR_ONE_TIME_FAULT       0x10000000 
#define RR_NARROWBAND_LS        0x08000000 
#define RR_ILLEGAL_REQUEST      0x04000000 
#define RR_MEMORY_LOADSHED      0x02000000 
#define RR_RPG_CPU_LOADSHED     0x01000000 
#define RR_SLOT_UNAVAIL         0x00800000 
#define RR_TASK_FAILURE         0x00400000 
#define RR_TASK_UNAVAIL         0x00200000 
#define RR_AVAIL_NEXT_SCAN      0x00100000 
#define RR_DISABLED_MOMENT      0x00080000 
#define RR_INVALID_PASSWORD     0x00040000
#define RR_UNUSED               0x00020000
#define RR_VOLUME_SCAN_ABORT    0x00010000
#define RR_INVLD_PROD_PARAMS    0x00008000
#define RR_DATA_SEQ_ERROR       0x00004000
#define RR_TASK_TERM            0x00002000


#define MAX_NUM_ELEV_GSM        20
#define MAX_ADD_ELEV_GSM        5 

/* General Status Message Format. */
typedef struct {		/* General Status Message */
    Prod_msg_header_icd mhb;	/* message header block */
    short divider;		/* block devider = -1 */
    short length;		/* block length = 82: Number of bytes to follow */
    short wx_mode;		/* weather mode: 0 - maintenance; 
				   1 - clear air; 2 - precip */
    short rda_op_status;	/* bit fields; ICD Figure 3-17 */
    short vcp;			/* VCP number */
    short n_elev;		/* number of elev cuts */
    short elev_angle[MAX_NUM_ELEV_GSM];
				/* elevation angles; in .1 degrees; unused
				   entries should be 0; */
    short rda_status;		/* bit fields; ICD Figure 3-17 */
    short rda_alarms;		/* bit fields; ICD Figure 3-17 */
    short data_trans_enable;	/* bit fields; ICD Figure 3-17 */
    short rpg_op_status;	/* bit fields; ICD Figure 3-17 */
    short rpg_alarms;		/* bit fields; ICD Figure 3-17 */
    short rpg_status;		/* bit fields; ICD Figure 3-17 */
    short rpg_nb_status;	/* bit fields; ICD Figure 3-17 */
    short ref_calib;		/* H channel reflectivity calibration 
                                   correction (difference from adaptation 
                                   data); -40 to 40 */
    short prod_avail;		/* bit fields; ICD Figure 3-17 */
    short super_res;		/* bit fields; ICD Figure 3-17 */
    short cmd;			/* bit fields; ICD Figure 3-17 */
    short v_ref_calib;		/* V channel reflectivity calibration 
                                   correction (difference from adaptation 
                                   data); -40 to 40 */
    short RDA_build_num;	/* RDA Build Number */
    short RDA_channel_num;      /* RDA Channel Number - 0, 1, or 2 */
    short max_connect_time;	/* the max connection time in minutes. 0 indicates
                                   no time limit. */
    short distri_method;        /* code for distribution method [1 - 4] */
    short build_version;        /* RPG Build Number */
    short add_elev_angle[MAX_ADD_ELEV_GSM];
                                /* Additional elevation angles.  These 
                                   will/will not be defined based on value 
                                   of n_elev. */
    short vcp_supp_data;        /* Bit map for supplemental VCP data. */
    short spares[42];           /* Spares for future expansion. */

} Prod_general_status_msg_icd;

/* GSM RDA Operability Status */
#define GSM_RDA_OPER_AUTO_CAL_DIS  0x0001
#define GSM_RDA_OPER_ON_LINE       0x0002
#define GSM_RDA_OPER_MR            0x0004
#define GSM_RDA_OPER_MM            0x0008
#define GSM_RDA_OPER_CMD_SHUTDOWN  0x0010
#define GSM_RDA_OPER_INOPERABLE    0x0020
#define GSM_RDA_OPER_WB_DISC       0x0080
#define GSM_RDA_OPER_INDETERMINATE 0x00bf

/* GSM RDA Status. */
#define GSM_RDA_STAT_STARTUP       0x0002
#define GSM_RDA_STAT_STANDBY       0x0004
#define GSM_RDA_STAT_RESTART       0x0008
#define GSM_RDA_STAT_OPERATE       0x0010
#define GSM_RDA_STAT_PLAYBACK      0x0020
#define GSM_RDA_STAT_OFFLINE       0x0040
#define GSM_RDA_STAT_INDETERMINATE 0x007e

/* GSM RDA Alarms. */
#define GSM_RDA_ALARMS_INDETERMINATE 0x0001
#define GSM_RDA_ALARMS_TOWER_UTILS   0x0002
#define GSM_RDA_ALARMS_PEDESTAL      0x0004
#define GSM_RDA_ALARMS_TRANSMITTER   0x0008
#define GSM_RDA_ALARMS_RECEIVER      0x0010
#define GSM_RDA_ALARMS_RDA_CONTROL   0x0020
#define GSM_RDA_ALARMS_COMMUNICATION 0x0040
#define GSM_RDA_ALARMS_SIG_PROC      0x0080
#define GSM_RDA_ALARMS_NO_ALARMS     0x01ff

/* GSM Data Transmission Enabled. */
#define GSM_MOMENTS_NONE    0x0002
#define GSM_MOMENTS_REF     0x0004
#define GSM_MOMENTS_VEL     0x0008
#define GSM_MOMENTS_WID     0x0010

/* GSM RPG Alarms. */
#define GSM_RPG_ALARMS_NO_ALARMS        0x0001
#define GSM_RPG_ALARMS_NODE_CONNECT     0x0002
#define GSM_RPG_ALARMS_WIDEBAND_FAILRE  0x0004
#define GSM_RPG_ALARMS_RPG_CNTL         0x0008
#define GSM_RPG_ALARMS_BASEDATA_FAILURE 0x0010
#define GSM_RPG_ALARMS_SPARE26          0x0020
#define GSM_RPG_ALARMS_INPUT_BUFFER_LS  0x0040
#define GSM_RPG_ALARMS_SPARE24          0x0080
#define GSM_RPG_ALARMS_PROD_STORAGE_LS  0x0100
#define GSM_RPG_ALARMS_SPARE22          0x0200
#define GSM_RPG_ALARMS_SPARE21          0x0400
#define GSM_RPG_ALARMS_INTER_LINK       0x1000
#define GSM_RPG_ALARMS_RED_CHAN         0x2000
#define GSM_RPG_ALARMS_TSK_FAIL         0x4000
#define GSM_RPG_ALARMS_MEDIA_FAILURE    0x8000

/* GSM RDA Operability Status */
#define GSM_RPG_OPER_LOADSHED      0x0001
#define GSM_RPG_OPER_ON_LINE       0x0002
#define GSM_RPG_OPER_MR            0x0004
#define GSM_RPG_OPER_MM            0x0008
#define GSM_RPG_OPER_CMD_SHUTDOWN  0x0010

/* GSM RPG Status. */
#define GSM_RPG_STAT_RESTART       0x0001
#define GSM_RPG_STAT_OPERATE       0x0002
#define GSM_RPG_STAT_STANDBY       0x0004

/* Narrowband Status. */
#define GSM_NB_STATUS_COMM_DISC    0x0001
#define GSM_NB_STATUS_NB_LS        0x0002

/* Product Availability. */
#define GSM_PA_PROD_AVAIL          0x0001  
#define GSM_PA_DEGRADED_AVAIL      0x0002 
#define GSM_PA_PROD_NOT_AVAIL      0x0004       




/* Product List Message Format */
typedef struct {		/* Product list message */
    Prod_msg_header_icd mhb;	/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* 4 to 998. Number of bytes to follow.      */ 
    short num_products;		/* number of products on list. 0 to 71.      */
    short reserved;             /* single/repeate/one-time product. 1 to 4.  */
} Prod_list_msg_icd;

#define NUM_PROD_LIST_PARM      4
/* Product List Entry Format. */
typedef struct {		/* Product list message entry                */
    short prod_id;		/* buffer number of the product              */
    short elevation;            /* Elevation angle.                          */
    short params[NUM_PROD_LIST_PARM];
				/* product dependent parameters. Refer to 
				   section "NOTES on product parameters" in 
				   prod_user_msg.h. See also Table II-A */
    short distribution_class;	/* distribution class for individual products
                                   1 to 20.                                   */
} Prod_list_entry_icd;




/* Alert Message Format. */
typedef struct {		/* Alert msg.         */
    Prod_msg_header_icd mhb;	/* message header block */
    short divider;		/* block devider = -1                        */
    short alert_status;		/* 1 or 2 */
    short alert_area_num;	/* Area number of alert as defined per user  */
    short alert_category;	/* 1 to 41				     */
    short thd_code;		/* 1 to 6				     */
    short thd_valuem;		/* see Table LXVIII (most significant short) */
    short thd_valuel;		/* see Table LXVIII (least significant short)*/
    short exd_valuem;		/* see Table LXVIII (most significant short) */
    short exd_valuel;		/* see Table LXVIII (least significant short)*/
    short azim;			/* 0 to 359.9				     */
    short range;                /* 0 to 186.0				     */
    char  c1;			/* A to Z				     */
    char  c2;                   /* 0 - 9 				     */
    short vol_num;
    short vol_date;
    short vol_timem;
    short vol_timel;
} Prod_alert_msg_icd;
 


/* Alert Adaptation Message Format. */
typedef struct {		/* alert adaptation parameter message. */
    Prod_msg_header_icd mhb;	/* message header block */
    short divider;		/* block devider = -1 */
    short length;		/* up to 820. Number of bytes to follow */ 
} Prod_alert_adaptation_parameter_msg_icd;




/* Alert Request Area Definition. */
typedef struct {		/* alert request message. */
    Prod_msg_header_icd mhb;	/* message header block */
    short divider;		/* block divider = -1 */
    short length;		/* up to 532.  Number of bytes to follow */
    short area_num;		/* alert area number */
    short num_cats;		/* number of categories */
} Prod_alert_req_msg_icd;




/* Alert Request Category Definition. */
typedef struct {
    short category;		/* alert category */
    short threshold;		/* alert threshold */
    short request_flag;		/* product request flag */
} Prod_alert_cat_entry_icd;




typedef struct {		/* alert adaptation parameter message entry */
    short alert_group;		/* 0 to 3; alert group: 1 = Grid Group; 
				   2 = Volume Group; 3 = Forecast Group */
    short alert_category;	/* alert category; 0 to 41; as defined in 
				   Table 5-7 */
    short max_n_thresholds;	/* number of allowable thresholds; 0 to 6;
				   See Table 5-7 */
    short threshold1;		/* Parameter dependent data value corresponding 
				   to the user defined threshold code; see 
				   table 5-7 */
    short threshold2;		/*  */
    short threshold3;		/*  */
    short threshold4;		/*  */
    short threshold5;		/*  */
    short threshold6;		/*  */
    short prod_code;		/* product code. 0, 16 to 131; Also see
				   table 5-9 */
} Prod_alert_adaptation_parameter_entry_icd;
   



/* Bias Table Environmental Data Format. */
typedef struct {		/* Bias Table Environmental Data Message */
    Prod_msg_header_icd mhb;	/* message header block */
    Block_id_t block;		/* environmental data block header */
    char awips_id[4];           /* AWIPS site ID ... NOT NULL TERMINATED */
    char radar_id[4];           /* radar site ID ... NOT NULL TERMINATED */
    short obs_yr;		/* observation year (1970 - 2099) */
    short obs_mon;		/* observation month (1 - 12) */
    short obs_day;		/* observation day (1 - 31) */
    short obs_hr;		/* observation hour (0 - 23) */
    short obs_min;		/* observation minute (0 - 59) */
    short obs_sec;		/* observation second (0 - 59) */
    short gen_yr;		/* generation year (1970 - 2099) */
    short gen_mon;		/* generation month (1 - 12) */
    short gen_day;		/* generation day (1 - 31) */
    short gen_hr;		/* generation hour (0 - 23) */
    short gen_min;		/* generation minute (0 - 59) */
    short gen_sec;		/* generation second (0 - 59) */
    short n_rows;		/* number of bias table rows (2 - 12) */
    Memory_span_t span[12];	/* memory span data. */ 
} Prod_bias_table_msg;


#ifdef __cplusplus
}
#endif
 
#endif		/* #ifndef LEGACY_PROD_H */


