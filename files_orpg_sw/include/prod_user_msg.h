/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/07/06 14:58:49 $
 * $Id: prod_user_msg.h,v 1.73 2012/07/06 14:58:49 steves Exp $
 * $Revision: 1.73 $
 * $State: Exp $
 */  

/***********************************************************************

    Description: header file defining data structures for product user
		messages. Refer to RPG/APUP ICD.

***********************************************************************/


#ifndef PROD_USER_MSG_H

#define PROD_USER_MSG_H

/* for c++ compatability */
#ifdef __cplusplus
extern "C"
{
#endif

typedef short halfword;
typedef int fullword;

#define NUM_PROD_DEPENDENT_PARAMS	6
				/* number of product dependent parameters
				   for difining a product */
#define ELEV_ANGLE_INDEX	2
				/* elevation angle entry, if exiting, in 
				   product dependent parameter array */

#define HALFWORD_SHIFT(a) (1 << (15 - (a)))
				/* macro for setting the RPG user message 
				   bit fields */
#define FULLWORD_SHIFT(a) (1 << (31 - (a)))
				/* macro for setting the RPG user message 
				   bit fields */


enum {				/* product distribution message code */
    MSG_PROD_REQUEST,		/* (0) product request:
				   Pd_msg_header + Pd_request_products
				   + ... 
				   The array of Pd_request_products starts at
				   ALIGNED_SIZE (sizeof (Pd_msg_header)).
				   A MSG_PROD_REQUEST message without any prod
				   (Pd_msg_header only) is used in ORPG for 
				   canceling all requests from the user. */
    MSG_SPARE1,			/* not used */
    MSG_GEN_STATUS,		/* general status: Pd_general_status_msg */
    MSG_REQ_RESPONSE,		/* request response message: 
				   Pd_request_response_msg */
    MSG_MAX_CON_DISABLE,	/* maximum connect time disable request: 
				   Pd_max_conn_time_disable_msg */
    MSG_EXTERNAL_DATA,		/* External Data message */
    MSG_ALERT_PARAMETER,	/* alert adaptation parameter message: 
				   Pd_alert_adaptation_parameter_msg +
				   Pd_alert_adaptation_parameter_entry + ...
				   The array of 
				   Pd_alert_adaptation_parameter_entry starts 
				   at ALIGNED_SIZE (sizeof 
				   (Pd_alert_adaptation_parameter_msg)). */
    MSG_ALERT_REQUEST,		/* alert request message */
    MSG_PROD_LIST,		/* product list: 
				   Pd_prod_list_msg  + 
				   Pd_prod_list_entry + ...
				   The array of Pd_prod_list_entry starts at
				   ALIGNED_SIZE (sizeof (Pd_prod_list_msg)). */
    MSG_ALERT,			/* alert message */
    MSG_SPARE10,		/* not used */
    MSG_SIGN_ON,		/* sign-on request message: Pd_sign_on_msg */
    MSG_SPARE12,		/* not used. */
    MSG_PROD_REQ_CANCEL,	/* product request cancel */
    MSG_SPARE14,		/* not used */
    MSG_ENVIRONMENTAL_DATA	/* Environmental Data message */  
};

#define MSG_FREE_TEXT_MSG	75
				/* Free Text Message product code. */

typedef struct {		/* message header block */
    short msg_code;		/* message code: -131 - -16; 0 - 211 */
    short date;			/* date of the message; modified Julian date */
    int time;			/* time of the message; GMT; seconds after 
				   midnight, 1/1/1970 */
    int length;			/* message length; number of bytes including 
				   this header; Note that this is the length 
				   in the C structure format. */
    short src_id;		/* ID of the sender: 0 - 999 */
    short dest_id;		/* ID of the receiver: 0 - 999 */
    short n_blocks;		/* header block plus the product description 
				   blocks in the message */
    short line_ind;		/* WAN line index */
} Pd_msg_header;

/* the following message header block is for legacy products */
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
    short line_ind;		/* WAN line index */
} Pd_prod_header;

#define SIZE_OF_PD_MSG_HEADER	18
				/* size of Pd_msg_header */

/* bit numbers for error_code in Pd_request_response_msg */
#define RR_NO_SUCH_MSG		0	/* ICD: no such message */
#define RR_NO_SUCH_PROD		1	/* ICD: no such product */
#define RR_NOT_GENERATED 	2	/* ICD: product not generated (not 
			  	 	   available in data base) */
#define RR_PROD_EXPIRED		2	/* generated but expired in LB; */
#define RR_GEN_TIMED_OUT	2	/* generation timed out ... product
                                           generation AWOL */
#define RR_ONETIME_GEN_FAILED	3
					/* ICD: one-time req gen process faulted */
#define RR_NB_LOADSHED		4	/* ICD: narrow band load shed */
#define RR_ILLEGAL_REQ		5	/* ICD: illegal request */
#define RR_MEM_LOADSHED		6	/* ICD: RPG memory load shed */
#define RR_CPU_LOADSHED		7	/* ICD: RPG cpu load shed */
#define RR_SLOT_FULL		8	/* ICD: unavailability of slots (Class I) */
#define RR_TASK_FAILED 		9	/* ICD: task failed */
#define RR_TASK_UNLOADED	10	/* ICD: unavailable (task not loaded upon start up) */
#define RR_AVAIL_NEXT_VOL 	11	/* ICD: available next volume scan (class I) */
#define RR_MOMENT_DISABLED 	12	/* ICD: moment disabled */
#define RR_INVALID_PASSWD 	13	/* ICD: invalid password (dial-up user only) */
#define RR_NOT_USED		14	/* ICD: not used */
#define RR_VOLUME_ABORTED       15      /* ICD: aborted volume scan */
#define RR_INVALID_PROD_PARAMS  16      /* ICD: invalid product parameters */
#define RR_DATA_SEQUENCE_ERROR  17      /* ICD: product not generated (data sequence
                                           error) */
#define RR_TASK_SELF_TERM       18      /* ICD: task failed (self-terminated) */

typedef struct {		/* Request Response Message */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1 */
    short length;		/* block length = 26: Number of bytes to 
				   follow */
    int error_code;		/* error code. See ICD */
    short seq_number;		/* sequence number. -13, 0 to 32767. */
    short msg_code;		/* product/message code. -16 to -131, 
				   1 to 131. */
    short elev_angle;		/* elevation angle in .1 degrees */
    short vol_date;		/* volume scan start date (Modified Julian) */
    short vol_time_msw; 	/* volume scan start time (secs since midnight) MSW */
    short vol_time_lsw;		/* volume scan start time (secs since midnight) LSW */
    short spare3;		/* spare */
    short spare4;		/* spare */
    short spare5;		/* spare */
    short spare6;		/* spare */
    short spare7;		/* spare */
} Pd_request_response_msg;

#define SIZE_OF_REQUEST_RESPONSE_MSG	48
				/* size of REQUEST RESPONSE MESSAGE including 
				   the header block */

#define MAX_GS_N_ELEV	20	/* elevation angle table size in 
				   Pd_general_status_msg */
#define MAX_GS_ADD_ELEV	 5	/* Additional elevation angle table size in 
				   Pd_general_status_msg */

typedef struct {		/* General Status Message */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1 */
    short length;		/* block length = 178: Number of bytes to 
				   follow */
    short wx_mode;		/* weather mode: 0 - maintenance; 
				   1 - clear air; 2 - precip */
    short rda_op_status;	/* bit fields; ICD Figure 3-17 */
    short vcp;			/* VCP number */
    short n_elev;		/* number of elev cuts */
    short elev_angle[MAX_GS_N_ELEV];
				/* elevation angles; in .1 degrees; unused
				   entries should be 0; */
    short rda_status;		/* bit fields; ICD Figure 3-17 */
    short rda_alarms;		/* bit fields; ICD Figure 3-17 */
    short data_trans_enable;	/* bit fields; ICD Figure 3-17 */
    short rpg_op_status;	/* bit fields; ICD Figure 3-17 */
    short rpg_alarms;		/* bit fields; ICD Figure 3-17 */
    short rpg_status;		/* bit fields; ICD Figure 3-17 */
    short rpg_nb_status;	/* bit fields; ICD Figure 3-17 */
    short ref_calib;		/* horizontal reflectivity calibration 
                                   correction (difference from adaptation 
                                   data); -40 to 40 */
    short prod_avail;		/* bit fields; ICD Figure 3-17 */
    short super_res;		/* bit map indicating which elevation cuts
				   have super res enabled.  Bit 0 = cut 1 */
    short cmd;			/* Clutter Mitigation Decision Bit Map. */
    short v_ref_calib;		/* vertical reflectivity calibration
                                   correction (difference from adaptation
                                   data); -40 to 40 */
    short RDA_build_num;	/* RDA Build Number */
    short RDA_channel_num;	/* RDA Channel Number - 0, 1, or 2 */
    short max_connect_time;	/* the max connection time in minutes. 0 
				   indicates no time limit. */
    short distri_method;	/* code for distribution method [1 - 4] */
    short build_version;	/* RPG Build Number */
    short add_elev_angle[MAX_GS_ADD_ELEV];	
				/* Additional elevation angles.  These 
				   will/will not be defined based on value 
				   of n_elev. */
    short vcp_supp_data;	/* Bit map for supplemental VCP data. */
    short spares[42];		/* Spares for future expansion. */
} Pd_general_status_msg;

#define MAX_N_ERROR_MSG 30      /* number of pup/rpgop error status.         */

typedef struct {		/* PUP_RPGOP_to_RPG_status_msg.              */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* 20 to 78, Number of bytes to follow. 
				   18 + n_elements_in_pup_rpgop_err_status * 2
				     */
    short pup_rpgop_state;  	/* 0/1/2, operatonal/training/shut_down_mode */
    short spare0;		/* spare, field 13                           */
    short spare1;		/* spare, field 14                           */
    short spare2;		/* spare, field 15                           */
    short spare3;		/* spare, field 16                           */
    short spare4;		/* spare, field 17                           */
    short spare5;		/* spare, field 18                           */
    short spare6;		/* spare, field 19                           */
    short spare7;		/* spare, field 20                           */
    short pup_rpgop_err_status[1];
    				/* error msg. 32769 to 32798, and 0 to 30; See
				   ICD for details    */
} Pd_pup_rpgop_to_rpg_status_msg;

/* NOTES on product parameters:

   1. When requesting (specifying) a product, one can specify certain product
   dependent parameters (at most NUM_PROD_DEPENDENT_PARAMS parameters). These
   parameters will be passed to the algorithm/product tasks for generating
   the specified product. These parameters will also be used for idenfifying
   the right product for distribution.

   2. When a parameter is used for specifying an elevation, it is used as 
   follows. A positive value represents an angle and a negative value 
   represents a slice (elevation index). Angles are coded as 
   value = angle * 10 if angle >= 0, or value = (angle + 360) * 10 if angle < 0. 

   3. Special values defined following these notes can be used for any of the 
   parameters. The ORPG product distribution software may use these special
   values in distribution control.

   4. Since the legacy users may not use the above conventions in their product 
   request messages (See ICD Table II-A), p_server must perform product 
   dependent parameter conversions for legacy user requests. ps_routine may 
   need to convert them back when generating the product generation control 
   msgs to be used by the legacy algorithm/product tasks.

*/
/* special values for params in the product request structures */
/* NOTE: PARAM_ALG_SET must remain a negative value in order for the 
         SRM and SRR products to be generated when the algorithm determines
         the storm speed and direction.  If this value is changed to a 
         positive value, librpg needs to handle this. */
#define PARAM_UNUSED -32768	/* This parameter is not used (UNU) */
#define PARAM_ANY_VALUE -32767	/* This parameter can be of any value (ANY) */
#define PARAM_ALG_SET -32766	/* This parameter value is (to be) set by the
				   algorithm/product task (ALG) */
#define PARAM_ALL_VALUES -32765	/* This specifies multiple requests: products 
				   of all possible parameter values. This will
				   affect generation scheduling. (ALL) */
#define PARAM_ALL_EXISTING -32764
				/* This specifies multiple requests: All 
				   existing products regardless of the value 
				   of this parameter. This does not affect
				   generation scheduling. (EXS) */
#define PARAM_MAX_SPECIAL PARAM_ALL_EXISTING
				/* the maximum special parameter value */

/* special values for "flag_bits" in the request message */
/* when the most significant bit of flag_bits is set, it is a high priority request
   the following defines the mask for that bit. */
#define PRIORITY_FLAG_BIT 0x8000				   
/* when the second most significant bit of flag_bits is set, it indicates a map
   has been requested.  The following defines the mask for that bit. */
#define MAP_FLAG_BIT 0x4000
/* the least significant bit will be set if the requestor is prohibited from
   scheduling a product generation, and the request must be fulfilled from
   the database.  The following defines the mask for that bit */
#define NON_SCHEDULING_REQUEST_BIT 	1

/* rpg internal bits */
#define ALERT_SCHEDULING_BIT		2
#define SUPPLEMENTAL_SCAN_BIT	   	4   /* Set if request is for a supplemental scan. */

typedef struct {		/* Request weather products                  */
    short divider;		/* block devider = -1                        */
    short length;		/* block length = 32: Number of bytes to 
				   follow.                                   */
    short prod_id;		/* buffer number of the product; prod_code
				   before permission check. */
    short flag_bits;		/* high/low priority or map request. 0,1/bit.*/
    short seq_number;		/* sequence number. 1 to 32767.              */
    short num_products;		/* number of products. -1, 1 to 9. 0 indicates
				   an empty request.          */
    short req_interval;		/* request interval. 1 to 9.                 */
    short VS_date;		/* volumn scan date. 1 to 32767. julian date.*/
    int VS_start_time;		/* volumn scan start time. -2 to 86399. 
                                   seconds.                                  */
    short params[NUM_PROD_DEPENDENT_PARAMS];
				/* product dependent parameters. Refer to 
				   section "NOTES on product parameters" in 
				   prod_user_msg.h. (See also Table II-A) */
} Pd_request_products;

#define ALERT_OT_REQ_MSGID      10000 
                                /* Alert-paired product message ID for 
                                         one-time product request            */

#define USER_PASSWD_SIZE 8	/* user passwd buffer size	     */
#define PORT_PASSWD_SIZE 8	/* port passwd buffer size	     */
				   
typedef struct {		/* Sign-on message.                          */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* block length = 18: Number of bytes from -1 
				   divider to end of block.                  */
    short disconn_override_flag;
                  		/* 0, 1 (yes). override automatic disconnect 
				   feature.*/
    char user_passwd[USER_PASSWD_SIZE];
                  		/* 6 chraracter user password.               */
    char port_passwd[PORT_PASSWD_SIZE];
                  		/* 4 chraracter user password.               */
    short spare;            	/* spare. 				     */         
} Pd_sign_on_msg;

typedef struct {		/* Max connect time disable request.         */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* block length = 10: Number of bytes including 
				   divider and length field.                 */
    short add_conn_time;	/* additional connect time. 0 to 1440.minutes */
    short spare0;		/* spare.				     */
    short spare1;		/* spare.				     */
} Pd_max_conn_time_disable_msg;


/* External Data Message (msg 5) block IDs */
#define RUC_MODEL_DATA_BLOCK_ID     4


/* Product List message (msg 8) */
typedef struct {		/* Product list message */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* 4 to 998. Number of bytes to follow.      */ 
    short num_products;		/* number of products on list. 0 to 71.      */
    short distribution_method;
                                /* single/repeate/one-time product. 1 to 4.  */
} Pd_prod_list_msg;

typedef struct {		/* Product list message entry */
    short prod_id;		/* buffer number of the product */
    short params[NUM_PROD_DEPENDENT_PARAMS];
				/* product dependent parameters. Refer to 
				   section "NOTES on product parameters" in 
				   prod_user_msg.h. See also Table II-A */
    short distribution_class;	/* distribution class for individual products
                                   0 to 3.                                   */
} Pd_prod_list_entry;


typedef struct {		/* Alert msg.         */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1                        */
    short alert_status;		/* 1 or 2 */
    short alert_area_num;	/* Area number of alert as defined per user  */
    short alert_category;	/* 1 to 41				     */
    short thd_code;		/* 1 to 6				     */
    int   thd_value;		/* see Table LXVIII				     */
    int   exd_value;		/* see Table LXVIII				     */
    short azim;			/* 0 to 359.9				     */
    short range;                /* 0 to 186.0				     */
    char  c1;			/* A to Z				     */
    char  c2;                   /* 0 - 9 				     */
    short vol_num;
    short vol_date;
    int   vol_time;
} Pd_alert_msg;
 
typedef struct {		/* alert request message. */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* up to 820. Number of bytes to follow */ 
    short alert_area_num;	/* 1 or 2 */ 
    short num_category;	        /* 0 or 10 */ 
} Pd_alert_request_msg;		
 
typedef struct {		/* alert category_def message. */
    short alert_category;	/* block devider = -1                        */
    short thd_code;		/* up to 820. Number of bytes to follow */ 
    short prod_req_flag;	/* 1 or 2 */ 
} Pd_alert_category_def_msg;		
 
typedef struct {		/* alert box message. */
    short b1;	
    short b2;	
    short b3;	
    short b4;	
} Pd_alert_box_msg;		

 
typedef struct {		/* alert adaptation parameter message. */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1                        */
    short length;		/* up to 820. Number of bytes to follow */ 
} Pd_alert_adaptation_parameter_msg;

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
} Pd_alert_adaptation_parameter_entry;
   

typedef struct {		/* Request Response Message */
    Pd_msg_header mhb;		/* message header block */
    short divider;		/* block devider = -1 */
    short length;		/* block length = 12: Number of bytes to 
				   follow */
    short date;			/* volume date */
    int time;			/* volume time */
    short seq_number;		/* sequence number. -13, 0 to 32767. */
    short v_num;		/* volume scan number */
    short edit_flag;		/* 0 - not editing; 1 - PUP will edit msg */
} Pd_RCM_edit_noedit_msg;

/* Structure definition for Environmental Data. */
#define BIAS_TABLE_BLOCK_ID     1

/* Environmental data block header. */
typedef struct {

    short divider;		/* block devider = -1 */
    short block_id;		/* block ID  =  1 */
    short version;		/* version number */
    short length;		/* block length */

} Block_id_t;

/* For a complete description of data, see RPG/APUP ICD. */
typedef struct {

   short mem_span_msw;		/* period of gage-radar analysis, MSW */
   short mem_span_lsw;		/* period of gage-radar analysis, LSW */
   short n_pairs_msw;		/* effective sample size, MSW */
   short n_pairs_lsw;		/* effective sample size, LSW */
   short avg_gage_msw;		/* avg. hourly gage accumulation, MSW */
   short avg_gage_lsw;		/* avg. hourly gage accumulation, LSW */
   short avg_radar_msw;		/* Avg. hourly radar accumulation, MSW */
   short avg_radar_lsw;		/* Avg. hourly radar accumulation, LSW */
   short bias_msw;		/* mean field bias, MSW */
   short bias_lsw;		/* mean field bias, LSW */

} Memory_span_t;

typedef struct {		/* Bias Table Environmental Data Message */
    Pd_msg_header mhb;		/* message header block */
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
} Pd_bias_table_msg;

#ifdef __cplusplus
}
#endif
 
#endif		/* #ifndef PROD_USER_MSG_H */


