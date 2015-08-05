/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/09/14 22:02:23 $
 * $Id: orpgsmi.h,v 1.22 2006/09/14 22:02:23 ryans Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */


#include <stdio.h>
#include <string.h>
#include <basedata.h>
#include <prod_gen_msg.h>
#include <gen_stat_msg.h>
#include <itc.h>
#include <prod_request.h>
#include <prod_status.h>
#include <hci.h>
#include <hci_up_nb.h>
#include <rda_alarm_table.h>
#include <le.h>
#include <rda_status.h>
#include <rda_rpg_loop_back.h>
#include <rda_rpg_message_header.h>
#include <rda_rpg_console_message.h>
#include <rda_notch_width_map.h>
#include <orda_clutter_map.h>
#include <rda_rpg_clutter_map.h>
#include <rda_control.h>
#include <rpg_request_data.h>
#include <mlos_info.h>
#include <rda_control_adapt.h>
#include <mode_select.h>
#include <layer_prod_params.h>
#include <rcm_prod_params.h>
#include <cell_prod_params.h>
#include <product_parameters.h>
#include <storm_cell_track.h>
#include <hail_algorithm.h>
#include <radazvd.h>
#include <tda.h>
#include <orda_pmd.h>
#include <orda_adpt.h>
#include <rpg_vcp.h>
#include <generic_basedata.h>


#ifndef ORPGSMI_H
#define ORPGSMI_H

#define ORPGSMI_EVENT_MESSAGE	0x700000

#define ORPGSMI_UNKNOWN_SIZE	100000

typedef struct {
    Base_data_header hd;
    short moments [BASEDATA_REF_SIZE + BASEDATA_DOP_SIZE * 2];
} Base_data_msg_t;

typedef struct {
    Prod_request items[ORPGSMI_UNKNOWN_SIZE];
} Prod_request_msg_t;

typedef struct {
    Prod_gen_status_header hd;
    Prod_gen_status items[ORPGSMI_UNKNOWN_SIZE];
} Prod_gen_status_msg_t;

typedef struct {
    Pd_distri_info hd;
    Pd_line_entry items[ORPGSMI_UNKNOWN_SIZE];
} Pd_distri_info_msg_t;

typedef struct {
    Pd_prod_entry items[ORPGSMI_UNKNOWN_SIZE];
} Pd_prod_entry_msg_t;

typedef struct {
    Pd_attr_entry items[ORPGSMI_UNKNOWN_SIZE];
} Pd_attr_entry_msg_t;

typedef struct {
    Mrpg_process_status_t items[ORPGSMI_UNKNOWN_SIZE];
} Mrpg_process_status_msg_t;

typedef struct {
    Mrpg_process_table_t items[ORPGSMI_UNKNOWN_SIZE];
} Mrpg_process_table_msg_t; 

typedef struct { 
    Mrpg_node_t items[ORPGSMI_UNKNOWN_SIZE];
} Mrpg_node_msg_t; 

typedef struct { 
    Mrpg_data_t items[ORPGSMI_UNKNOWN_SIZE];
} Mrpg_data_msg_t; 

typedef struct {
    int int_field;
} Int_message;
 
typedef struct {
    char char_field[1];
} String_msg_t;

/* SMI_struct Base_data_msg_t 55;
   SMI_struct Prod_header;
   SMI_struct RDA_basedata_header;
   SMI_struct ORDA_basedata_header;
   SMI_struct RDA_status_msg_t;
   SMI_struct ORDA_status_msg_t;
   SMI_struct RDA_RPG_loop_back_message_t;
   SMI_struct RDA_RPG_message_header_t;
   SMI_struct RDA_RPG_console_message_t;
   SMI_struct RDA_notch_map_filter_t;
   SMI_struct ORDA_clutter_map_filter_t;
   SMI_struct RDA_notch_map_suppr_t;
   SMI_struct ORDA_clutter_map_segment_t;
   SMI_struct RDA_notch_map_data_t;
   SMI_struct ORDA_clutter_map_data_t;
   SMI_struct RDA_notch_map_t;
   SMI_struct ORDA_clutter_map_t;
   SMI_struct RDA_notch_map_msg_t;
   SMI_struct ORDA_clutter_map_msg_t;
   SMI_struct RDA_bypass_map_segment_t;
   SMI_struct ORDA_bypass_map_segment_t;
   SMI_struct RDA_bypass_map_t;
   SMI_struct ORDA_bypass_map_t;
   SMI_struct RDA_bypass_map_msg_t;
   SMI_struct ORDA_bypass_map_msg_t;
   SMI_struct RDA_control_commands_t;
   SMI_struct ORDA_control_commands_t;
   SMI_struct RPG_request_data_struct_t;
   SMI_struct orda_pmd_t;
   SMI_struct transmitter_t;
   SMI_struct power_t;
   SMI_struct comms_t;
   SMI_struct tower_utilities_t;
   SMI_struct equipment_shelter_t;
   SMI_struct antenna_pedestal_t;
   SMI_struct rf_gnrtr_rcvr_t;
   SMI_struct calib_t;
   SMI_struct file_status_t;
   SMI_struct device_status_t;
   SMI_struct ORDA_adpt_data_msg_t;
   SMI_struct ORDA_adpt_data_t;
   SMI_struct VCP_ICD_msg_t;
   SMI_struct VCP_message_header_t;
   SMI_struct VCP_elevation_cut_header_t;
   SMI_struct Generic_vol_t;
   SMI_struct Generic_elev_t;
   SMI_struct Generic_rad_t;
   SMI_struct Generic_moment_t;
   SMI_struct Generic_any_t;
   SMI_struct Generic_record_t;
   SMI_struct Moment_unique_params_t;
   SMI_struct Generic_basedata_header_t;
   SMI_struct Generic_basedata_t;

   SMI_struct LE_message;

   SMI_struct sgmts09	100100.1;
   SMI_struct a315csad	100100.2;
   SMI_struct a315lock	100100.3;
   SMI_struct a315trnd	100100.4;
   SMI_struct a3cd09	100100.5;
   SMI_struct pvecs09	100100.6;
   SMI_struct a317ctad	100100.7;
   SMI_struct a317lock	100100.8;
   SMI_struct a3cd11	100100.9;

   SMI_struct cd07_vcpinfo	100200.1;
   SMI_struct Hrdb_date_time	100200.4;
   SMI_struct A3cd97	100400.1;
   SMI_struct A3136C3_t	100500.5;
   SMI_struct A3052t	100600.1;
   SMI_struct A3052u	100600.2;
   SMI_struct A304c2	100700.1;
   SMI_struct cd07_bypassmap	100700.3;

   RPG event messages:

   SMI_struct hci_child_started_event_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_HCI_CHILD_IS_STARTED;
   SMI_struct String_msg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_PROD_USER_STATUS;
   SMI_struct Prod_user_status ORPGSMI_EVENT_MESSAGE.ORPGEVT_PROD_USER_STATUS_DATA;
   SMI_struct String_msg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_PROCESS_READY;
   SMI_struct orpgevt_scan_info_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_SCAN_INFO;
   SMI_struct Orpgevt_radial_acct_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_RADIAL_ACCT;
   SMI_struct orpgevt_end_of_volume_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_END_OF_VOLUME;
   SMI_struct Orpgevt_task_exit_msg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_TASK_EXIT;
   SMI_struct Orpginfo_statefl_flag_evtmsg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_STATEFL_FLAG;
   SMI_struct Orpgevt_load_shed_msg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_LOAD_SHED_CAT;
   SMI_struct Orpgevt_adapt_msg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_ADAPT_UPDATE;
   SMI_struct Orpginfo_rpg_alarm_evtmsg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_RPG_ALARM;
   SMI_struct Orpginfo_rpg_opstat_evtmsg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_RPG_OPSTAT_CHANGE;
   SMI_struct Orpginfo_rpg_status_change_evtmsg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_RPG_STATUS_CHANGE;
   SMI_struct Int_message ORPGSMI_EVENT_MESSAGE.ORPGEVT_PROD_LIST;
   SMI_struct Pd_distri_cmd ORPGSMI_EVENT_MESSAGE.ORPGEVT_PD_LINE;
   SMI_struct String_msg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_HCI_COMMAND_ISSUED;
   SMI_struct String_msg_t ORPGSMI_EVENT_MESSAGE.ORPGEVT_SYSTEM_CONFIG_INFO;
   SMI_struct String_msg_t ORPGSMI_EVENT_MESSAGE.0xfffffff7;

   For ORPGADPT:

   SMI_struct RDA_RPG_console_message_t;
   SMI_struct Siteadp_adpt_t;
   SMI_struct Redundant_info_t;
   SMI_struct mlos_info_t;
   SMI_struct rda_control_adapt_t;
   SMI_struct mode_select_t;
   SMI_struct layer_prod_params_t;
   SMI_struct rcm_prod_params_t;
   SMI_struct cell_prod_params_t;
   SMI_struct vad_rcm_heights_t;
   SMI_struct hail_algorithm_t;
   SMI_struct storm_cell_track_t;
   SMI_struct tda_t;

*/

/* SMI_function ORPG_smi_info; */

#endif	/* #ifndef ORPGSMI_H */
