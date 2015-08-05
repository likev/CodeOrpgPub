/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/30 12:29:36 $
 * $Id: rpg_ldm.h,v 1.7 2012/10/30 12:29:36 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */ 

#ifndef RPG_LDM_H
#define RPG_LDM_H

/* Include files */

#include <orpg.h>
#include <infr.h>
#include <roc_level2.h>

/* Definitions */

#define	RPG_LDM_MAX_NUM_ICAOS			ROC_L2_NUM_ICAOS
#define	RPG_LDM_MAX_MSG_SIZE_OPTION_LEN		16
#define	RPG_LDM_MAX_FEED_TYPE_OPTION_LEN	32
#define	RPG_LDM_MAX_KEY_LEN			ROC_L2_MAX_LDM_KEY_LEN
#define	RPG_LDM_ICAO_LEN			ROC_L2_MAX_ICAO_LEN
#define	RPG_LDM_MIN_ICAO_LEN			4
#define	RPG_LDM_KB				1024
#define	RPG_LDM_MB				(RPG_LDM_KB*1000)
#define	RPG_LDM_EXP_FEED_TYPE			ROC_L2_FEED_TYPE_EXP
#define	RPG_LDM_NEXRAD_FEED_TYPE		ROC_L2_FEED_TYPE_NEXRAD
#define	RPG_LDM_EXP_FEED_TYPE_INDEX		ROC_L2_FEED_TYPE_EXP_INDEX
#define	RPG_LDM_NEXRAD_FEED_TYPE_INDEX		ROC_L2_FEED_TYPE_NEXRAD2_INDEX
#define	RPG_LDM_DEFAULT_MSG_SIZE		10000
#define	RPG_LDM_MAX_PRINT_MSG_LEN		1024
#define	RPG_LDM_MAX_MSG_SIZE			(RPG_LDM_MB*10)
#define	RPG_LDM_MAX_RPG_BUILD_STRING		32
#define	RPG_LDM_MAX_SERVER_INFO_STRING		8
#define	RPG_LDM_NUM_LE_LOG_LINES		2000
#define	RPG_LDM_MISSING_VALUE			0xffffffff
#define	RPG_LDM_PRODUCT_INDEX_MASK		0x0000ffff
#define	RPG_LDM_SUPPLEMENTAL_INDEX_MASK		0xffff0000
#define	RPG_LDM_SUCCESS				0
#define	RPG_LDM_FAILURE				-1
#define	RPG_LDM_MALLOC_FAILED			-50
#define	RPG_LDM_INVALID_COMMAND_LINE		-51
#define	RPG_LDM_FILE_NOT_EXIST			-52
#define	RPG_LDM_FILE_NOT_READABLE		-53
#define	RPG_LDM_FILE_NOT_OPENED			-54
#define	RPG_LDM_FILE_STAT_FAILED		-55
#define	RPG_LDM_READ_FAILED			-56
#define	RPG_LDM_COMPRESS_FAILED			-57
#define	RPG_LDM_LB_WRITE_FAILED			-58
#define	RPG_LDM_ORPGMISC_INIT_FAILED		-59
#define	RPG_LDM_UN_REGISTER_FAILED		-60
#define	RPG_LDM_NULL_LDM_KEY			-61
#define	RPG_LDM_TIME_INIT			ROC_L2_MISSING_TIMESTAMP
#define	RPG_LDM_PROD_FEED_TYPE_INIT		ROC_L2_FEED_TYPE_EXP
#define	RPG_LDM_PROD_FLAGS_CLEAR		ROC_L2_PROD_FLAGS_CLEAR
#define	RPG_LDM_PROD_FLAGS_PRINT_CW		ROC_L2_PROD_FLAGS_PRINT_CW
#define	RPG_LDM_MSG_FLAGS_CLEAR			ROC_L2_MSG_FLAGS_CLEAR
#define	RPG_LDM_MSG_FLAGS_CHANNEL1		ROC_L2_MSG_FLAGS_CHANNEL1
#define	RPG_LDM_MSG_FLAGS_CHANNEL2		ROC_L2_MSG_FLAGS_CHANNEL2
#define	RPG_LDM_MSG_FLAGS_COMPRESS_GZIP		ROC_L2_MSG_FLAGS_COMPRESS_GZIP
#define	RPG_LDM_MSG_FLAGS_COMPRESS_BZIP2	ROC_L2_MSG_FLAGS_COMPRESS_BZIP2
#define	RPG_LDM_MSG_CODE_INIT			99999
#define	RPG_LDM_MSG_SIZE_INIT			0
#define	RPG_LDM_MSG_SEG_NUM_INIT		0
#define	RPG_LDM_MSG_NUM_SEGS_INIT		0
#define	RPG_LDM_MAX_WMO_HDR_LEN			64
#define	RPG_LDM_CFC_PROD_SEG1_BITMASK		0x2000
#define	RPG_LDM_CFC_PROD_SEG2_BITMASK		0x1000
#define	RPG_LDM_CFC_PROD_SEG3_BITMASK		0x0800
#define	RPG_LDM_CFC_PROD_SEG4_BITMASK		0x0400
#define	RPG_LDM_CFC_PROD_SEG5_BITMASK		0x0200
#define	RPG_LDM_NUM_MILLISECS_IN_SECS		1000
#define	RPG_LDM_SECONDS_PER_DAY			86400
#define	RPG_LDM_BUILD_SCALE			ROC_L2_BUILD_SCALE
#define	RPG_LDM_PCT_IDLE_SCALE			ROC_L2_PCT_IDLE_SCALE
#define	RPG_LDM_LOAD_AVG_SCALE			ROC_L2_LOAD_AVG_SCALE
#define	RPG_LDM_NUM_L2_MSGS_SCALE		100
#define	RPG_LDM_RPGA_CH1_INDEX			ROC_L2_RPGA_CH1_INDEX
#define	RPG_LDM_RPGB_CH1_INDEX			ROC_L2_RPGB_CH1_INDEX
#define	RPG_LDM_RPGA_CH2_INDEX			ROC_L2_RPGA_CH2_INDEX
#define	RPG_LDM_RPGB_CH2_INDEX			ROC_L2_RPGB_CH2_INDEX
#define	RPG_LDM_MSCF_INDEX			ROC_L2_MSCF_INDEX
#define	RPG_LDM_RPGA_CH1_MASK			ROC_L2_RPGA_CH1_MASK
#define	RPG_LDM_RPGB_CH1_MASK			ROC_L2_RPGB_CH1_MASK
#define	RPG_LDM_RPGA_CH2_MASK			ROC_L2_RPGA_CH2_MASK
#define	RPG_LDM_RPGB_CH2_MASK			ROC_L2_RPGB_CH2_MASK
#define	RPG_LDM_MSCF_MASK			ROC_L2_MSCF_MASK
#define	RPG_LDM_NL2_PRIMARY_NODE1_MASK		ROC_L2_NL2_PRIMARY_NODE1_MASK
#define	RPG_LDM_NL2_PRIMARY_NODE2_MASK		ROC_L2_NL2_PRIMARY_NODE2_MASK
#define	RPG_LDM_NL2_SECONDARY_NODE1_MASK	ROC_L2_NL2_SECONDARY_NODE1_MASK
#define	RPG_LDM_NL2_SECONDARY_NODE2_MASK	ROC_L2_NL2_SECONDARY_NODE2_MASK
#define	RPG_LDM_NUM_LDM_PRODS			ROC_L2_NUM_LDM_PRODUCTS
#define	RPG_LDM_LDMPING_NL2_PROD		ROC_L2_RPG_LDMPING_NL2_PRODUCT_INDEX
#define	RPG_LDM_ADAPT_PROD			ROC_L2_RPG_ADAPTATION_DATA_PRODUCT_INDEX
#define	RPG_LDM_STATUS_LOG_PROD			ROC_L2_RPG_STATUS_LOG_PRODUCT_INDEX
#define	RPG_LDM_ERROR_LOG_PROD			ROC_L2_RPG_ERROR_LOG_PRODUCT_INDEX
#define	RPG_LDM_RDA_STATUS_PROD			ROC_L2_RPG_RDA_STATUS_PRODUCT_INDEX
#define	RPG_LDM_VOL_STATUS_PROD			ROC_L2_RPG_VOLUME_STATUS_PRODUCT_INDEX
#define	RPG_LDM_WX_STATUS_PROD			ROC_L2_RPG_WX_STATUS_PRODUCT_INDEX
#define	RPG_LDM_CONSOLE_MSG_PROD		ROC_L2_RPG_CONSOLE_MSG_PRODUCT_INDEX
#define	RPG_LDM_RPG_INFO_PROD			ROC_L2_RPG_RPG_INFO_PRODUCT_INDEX
#define	RPG_LDM_TASK_STATUS_PROD		ROC_L2_RPG_TASK_STATUS_PRODUCT_INDEX
#define	RPG_LDM_PROD_INFO_PROD			ROC_L2_RPG_PRODUCT_INFO_PRODUCT_INDEX
#define	RPG_LDM_SAVE_LOG_PROD			ROC_L2_RPG_SAVE_LOG_PRODUCT_INDEX
#define	RPG_LDM_BIAS_TABLE_PROD			ROC_L2_RPG_BIAS_TABLE_PRODUCT_INDEX
#define	RPG_LDM_RUC_DATA_PROD			ROC_L2_RPG_RUC_DATA_PRODUCT_INDEX
#define	RPG_LDM_L2_WRITE_STATS_PROD		ROC_L2_RPG_L2_WRITE_STATS_PRODUCT_INDEX
#define	RPG_LDM_L2_READ_STATS_PROD		ROC_L2_RPG_L2_READ_STATS_PRODUCT_INDEX
#define	RPG_LDM_L3_PROD				ROC_L2_RPG_L3_PRODUCT_INDEX
#define	RPG_LDM_SNMP_PROD			ROC_L2_RPG_SNMP_PRODUCT_INDEX
#define	RPG_LDM_RTSTATS_PROD			ROC_L2_RTSTATS_PRODUCT_INDEX
#define	RPG_LDM_SECURITY_PROD			ROC_L2_SECURITY_PRODUCT_INDEX
#define	RPG_LDM_HW_STATS_PROD			ROC_L2_HARDWARE_STATS_PRODUCT_INDEX
#define	RPG_LDM_MISC_FILE_PROD			ROC_L2_MISC_FILE_PRODUCT_INDEX
#define	RPG_LDM_REQUEST_PROD			ROC_L2_REQUEST_PRODUCT_INDEX
#define	RPG_LDM_COMMAND_PROD			ROC_L2_COMMAND_PRODUCT_INDEX

/* Use ROC_L2 header for LB/LDM products */
typedef ROC_L2_prod_hdr_t RPG_LDM_prod_hdr_t;
typedef ROC_L2_msg_hdr_t RPG_LDM_msg_hdr_t;
typedef ROC_L2_log_msg_t RPG_LDM_log_msg_t;
typedef ROC_L2_snmp_msg_t RPG_LDM_snmp_msg_t;
typedef ROC_L2_rtstats_t RPG_LDM_rtstats_t;
typedef ROC_L2_hw_stats_t RPG_LDM_hw_stats_t;
typedef ROC_L2_LDM_prod_stats_t RPG_LDM_LB_write_stats_t;
typedef ROC_L2_LDM_prod_stats_t RPG_LDM_LB_read_stats_t;
typedef ROC_L2_request_t RPG_LDM_request_t;
typedef ROC_L2_cmd_t RPG_LDM_cmd_t;

/* Enumerations */

enum { NO, YES };
enum
{
  RPG_LDM_RPGA_NODE_INDEX = 1,
  RPG_LDM_RPGB_NODE_INDEX,
  RPG_LDM_MSCF_NODE_INDEX
};
enum
{
  WMO_PACIFIC,
  WMO_NORTHEAST,
  WMO_SOUTHEAST,
  WMO_NORTHCENTRAL,
  WMO_SOUTHCENTRAL,
  WMO_ROCKY_MTS,
  WMO_WEST_COAST,
  WMO_SOUTHEAST_AK,
  WMO_CENTRAL_PACIFIC,
  WMO_NORTHERN_AK,
};

/* Function Prototypes */

void  RPG_LDM_parse_cmd_line( int, char *[] );
void  RPG_LDM_prod_hdr_init( RPG_LDM_prod_hdr_t * );
void  RPG_LDM_msg_hdr_init( RPG_LDM_msg_hdr_t * );
int   RPG_LDM_write_to_LB( char * );
void  RPG_LDM_print_debug( const char *, ... );
void  RPG_LDM_print_usage();
int   RPG_LDM_get_LDM_key_index_from_string( char * );
char *RPG_LDM_get_LDM_key_string_from_index( int );
int   RPG_LDM_get_local_ICAO_index();
char *RPG_LDM_get_local_ICAO();
int   RPG_LDM_get_ICAO_index_from_string( char * );
char *RPG_LDM_get_ICAO_string_from_index( int );
int   RPG_LDM_get_channel_number();
char *RPG_LDM_get_channel_string();
int   RPG_LDM_get_node_index();
char *RPG_LDM_get_node_string();
float RPG_LDM_get_RPG_build_number();
int   RPG_LDM_get_scaled_RPG_build_number();
char *RPG_LDM_get_RPG_build_string();
int   RPG_LDM_get_local_server_index();
int   RPG_LDM_get_local_server_mask();
char *RPG_LDM_get_local_server_string();
int   RPG_LDM_get_server_index_from_string( char * );
int   RPG_LDM_get_server_index_from_mask( int );
int   RPG_LDM_get_server_mask_from_string( char * );
int   RPG_LDM_get_server_mask_from_index( int );
char *RPG_LDM_get_server_string_from_index( int );
char *RPG_LDM_get_server_string_from_mask( int );
int   RPG_LDM_get_feed_type_index_from_string( char * );
int   RPG_LDM_get_feed_type_index_from_code( int );
int   RPG_LDM_get_feed_type_code_from_string( char * );
int   RPG_LDM_get_feed_type_code_from_code( int );
char *RPG_LDM_get_feed_type_string_from_index( int );
char *RPG_LDM_get_feed_type_string_from_code( int );
char *RPG_LDM_get_prod_hdr_flag_string( int );
char *RPG_LDM_get_msg_hdr_flag_string( int );
int   RPG_LDM_get_debug_mode();
char *RPG_LDM_get_timestring( time_t );
char *RPG_LDM_get_WMO_header( Graphic_product * );
void  RPG_LDM_set_LDM_key( RPG_LDM_prod_hdr_t *, int );
void  RPG_LDM_print_prod_hdr( char * );
void  RPG_LDM_print_msg_hdr( char * );
void  RPG_LDM_print_product( int, char * );
void  RPG_LDM_print_ICAO_ldmping_msg( char * );
void  RPG_LDM_print_adapt_msg( char * );
void  RPG_LDM_print_status_log_msg( char *, int );
void  RPG_LDM_print_error_log_msg( char *, int );
void  RPG_LDM_print_RDA_status_msg( char * );
void  RPG_LDM_print_volume_status_msg( char * );
void  RPG_LDM_print_wx_status_msg( char * );
void  RPG_LDM_print_console_msg_msg( char * );
void  RPG_LDM_print_RPG_info_msg( char * );
void  RPG_LDM_print_task_status_msg( char *, int );
void  RPG_LDM_print_product_info_msg( char * );
void  RPG_LDM_print_save_log_msg( char * );
void  RPG_LDM_print_bias_table_msg( char * );
void  RPG_LDM_print_RUC_data_msg( char * );
void  RPG_LDM_print_L2_write_stats_msg( char *, int );
void  RPG_LDM_print_L2_read_stats_msg( char *, int );
void  RPG_LDM_print_L3_msg( char *, int );
void  RPG_LDM_print_snmp_msg( char *, int );
void  RPG_LDM_print_rtstats_msg( char * );
void  RPG_LDM_print_security_msg( char * );
void  RPG_LDM_print_hw_stats_msg( char * );
void  RPG_LDM_print_misc_file_msg( char * );
void  RPG_LDM_print_request_msg( char * );
void  RPG_LDM_print_command_msg( char * );
# endif

