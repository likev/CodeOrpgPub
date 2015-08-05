/****************************************************************
		
    Module: orpgmisc.h
				
    Description: This is the header file used for miscellaneous
		 functions in libORPG.

****************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/02 15:41:10 $
 * $Id: orpgmisc.h,v 1.36 2013/07/02 15:41:10 steves Exp $
 * $Revision: 1.36 $
 * $State: Exp $
 */

#ifndef ORPGMISC_OPTIONS_H
#define ORPGMISC_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

enum {ORPGMISC_IS_RPG_STATUS_OPER,
      ORPGMISC_IS_RPG_STATUS_RESTART,
      ORPGMISC_IS_RPG_STATUS_STANDBY,
      ORPGMISC_IS_RPG_STATUS_TEST} ;
unsigned char ORPGMISC_is_rpg_status(int check_status) ;

#define ORPGMISC_rpg_oper() \
    (ORPGMISC_is_rpg_status((ORPGMISC_IS_RPG_STATUS_OPER)))
#define ORPGMISC_rpg_restart() \
    (ORPGMISC_is_rpg_status((ORPGMISC_IS_RPG_STATUS_RESTART)))
#define ORPGMISC_rpg_standby() \
    (ORPGMISC_is_rpg_status((ORPGMISC_IS_RPG_STATUS_STANDBY)))
#define ORPGMISC_rpg_test() \
    (ORPGMISC_is_rpg_status((ORPGMISC_IS_RPG_STATUS_TEST)))

/* Macros for rpg_install.info file. */

#define	INSTALL_TYPE_TAG		"TYPE:"
#define	INSTALL_ICAO_TAG		"ICAO:"
#define	INSTALL_CHANNEL_TAG		"CHANNEL:"
#define	INSTALL_SUBNET_TAG		"SUBNET:"
#define	INSTALL_NETMASK_TAG		"NETMASK:"
#define	INSTALL_REDUNDANT_TAG		"RPG_REDUNDANT:"
#define	INSTALL_BDDS_TAG		"BDDS:"
#define	INSTALL_NWS_TAG			"NWS:"
#define	ADAPT_LOADED_TAG		"ADAPTATION_DATA_LOADED:" /* YES,NO */
#define	DEV_CONFIGURED_TAG		"HARDWARE_CONFIGURED:" /* YES,NO */
#define	ALL_DEVICES_TIME_TAG		"ALL_DEVICES_CONFIGURED_TIME:"
#define	CONSERV_DEVICE_TIME_TAG		"CONSERV_DEVICE_CONFIGURED_TIME:"
#define	LAN_DEVICE_TIME_TAG		"LAN_DEVICE_CONFIGURED_TIME:"
#define	RPG_ROUTER_DEVICE_TIME_TAG	"RPG_ROUTER_DEVICE_CONFIGURED_TIME:"
#define	PWR_ADMIN_DEVICE_TIME_TAG	"PWR_ADMIN_DEVICE_CONFIGURED_TIME:"
#define	UPS_DEVICE_TIME_TAG		"UPS_DEVICE_CONFIGURED_TIME:"
#define	PTIA_DEVICE_TIME_TAG		"PTIA_DEVICE_CONFIGURED_TIME:"
#define	PTIB_DEVICE_TIME_TAG		"PTIB_DEVICE_CONFIGURED_TIME:"
#define	PTIC_DEVICE_TIME_TAG		"PTIC_DEVICE_CONFIGURED_TIME:"
#define	DIO_MODULE_DEVICE_TIME_TAG	"DIO_MODULE_DEVICE_CONFIGURED_TIME:"
#define	FR_ROUTER_RPG_DEVICE_TIME_TAG	"FR_ROUTER_RPG_DEVICE_CONFIGURED_TIME:"
#define	FR_ROUTER_MSCF_DEVICE_TIME_TAG	"FR_ROUTER_MSCF_DEVICE_CONFIGURED_TIME:"
#define	COPY_AUDIT_LOGS_TIME_TAG	"COPY_AUDIT_LOGS_TIME:"


 int ORPGMISC_is_low_bandwidth();
 int ORPGMISC_read_options(int argc, char **argv);
void ORPGMISC_set_compression(int data_id);
 int ORPGMISC_system_endian_type ();
 int ORPGMISC_local_endian_type  ();
 int ORPGMISC_change_endian_type (char *buf, int data_id);
void ORPGMISC_send_multi_line_le (int code, char *msg);

int ORPGMISC_init (int argc, char *argv[], int n_msgs, 
		   int lb_type, int instance, int no_system_log);
unsigned char
     ORPGMISC_deliverable(void) ;
pid_t ORPGMISC_get_pid(int node_id, char *task_name, int instance) ;
int ORPGMISC_deau_init ();
  int ORPGMISC_le_init (int argc, char **argv, int instance);
int ORPGMISC_LE_init (int argc, char *argv[], int n_msgs, 
		int lb_type, int instance, int no_system_log);

int ORPGMISC_vol_scan_num( unsigned int vol_seq_num );
unsigned int ORPGMISC_vol_seq_num( int vol_quotient, int vol_scan_num );
int ORPGMISC_is_operational ();
char *ORPGMISC_get_site_name (char *field);
int ORPGMISC_get_site_value (char *variable, char *value, int v_size);
char *ORPGMISC_crypt (char *str);
int ORPGMISC_RPG_build_number ();
int ORPGMISC_RPG_adapt_version_number ();
int ORPGMISC_get_install_info (char *tag, char *buf, int buf_size);
int ORPGMISC_set_install_info (char *tag, char *value);


int ORPGMISC_pack_ushorts_with_value( void *loc, void *value );
int ORPGMISC_unpack_value_from_ushorts( void *loc, void *value );

int ORPGMISC_max_products();

#define LDM_VERSION_UNKN -1 /* Unknown. */
#define LDM_VERSION_0	0   /* Does not have LDM. */
#define LDM_VERSION_1	1   /* Msg 1  - Legacy. */
#define LDM_VERSION_2	2   /* Msg 31 - SR disabled at RDA. */
#define LDM_VERSION_3	3   /* Msg 31 - SR enabled for LDM. */
#define LDM_VERSION_4	4   /* Msg 31 - SR disabled for LDM. */
#define LDM_VERSION_5	5   /* Msg 31 - DP SR disabled at RDA. */
#define LDM_VERSION_6	6   /* Msg 31 - DP SR enabled for LDM. */
#define LDM_VERSION_7	7   /* Msg 31 - DP SR disabled for LDM.*/

int ORPGMISC_get_LDM_version();

#ifdef __cplusplus
}
#endif

#endif

