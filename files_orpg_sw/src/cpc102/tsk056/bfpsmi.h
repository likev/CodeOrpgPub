/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2005/06/02 19:31:37 $
 * $Id: bfpsmi.h,v 1.4 2005/06/02 19:31:37 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
*/


#include <basedata.h>
#include <prod_gen_msg.h>
#include <gen_stat_msg.h>
#include <itc.h>
#include <prod_request.h>
#include <prod_status.h>
#include <hci.h>
#include <hci_up_nb.h>
#include <hci_rda_control.h>
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


#define MAX_STR_SIZE    100



/*
   SMI_struct MyShortStruct;
   SMI_struct MyIntStruct; 
   SMI_struct MyCharStruct; 
   SMI_struct MyComboStruct;
   SMI_struct RDA_RPG_message_header_t;
   SMI_struct RDA_status_msg_t;
   SMI_struct Base_data_msg_t;
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
   SMI_struct LE_message;
   SMI_struct sgmts09;
   SMI_struct a315csad;
   SMI_struct a315lock;
   SMI_struct a315trnd;
   SMI_struct a3cd09;
   SMI_struct pvecs09;
   SMI_struct a317ctad;
   SMI_struct a317lock;
   SMI_struct a3cd11;
   SMI_struct cd07_vcpinfo;
   SMI_struct Hrdb_date_time;
   SMI_struct A3cd97;
   SMI_struct A3136C3_t;
   SMI_struct A3052t;
   SMI_struct A3052u;
   SMI_struct A304c2;
   SMI_struct cd07_bypassmap;
   SMI_struct hci_child_started_event_t;
   SMI_struct String_msg_t;
   SMI_struct Prod_user_status;
   SMI_struct String_msg_t;
   SMI_struct orpgevt_scan_info_t;
   SMI_struct Orpgevt_radial_acct_t;
   SMI_struct orpgevt_end_of_volume_t;
   SMI_struct Orpgevt_task_exit_msg_t;
   SMI_struct Orpginfo_statefl_flag_evtmsg_t;
   SMI_struct Orpgevt_load_shed_msg_t;
   SMI_struct Orpgevt_adapt_msg_t;
   SMI_struct Orpginfo_rpg_alarm_evtmsg_t;
   SMI_struct Orpginfo_rpg_opstat_evtmsg_t;
   SMI_struct Orpginfo_rpg_status_change_evtmsg_t;
   SMI_struct Int_message;
   SMI_struct Pd_distri_cmd;
   SMI_struct String_msg_t;
   SMI_struct String_msg_t;
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
   SMI_struct rda_command_t;
*/

/* SMI_function BFP_smi_info; */

