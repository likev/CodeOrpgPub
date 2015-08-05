/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/10/12 20:40:49 $
 * $Id: vcp_cs.h,v 1.7 2012/10/12 20:40:49 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */


#ifndef VCP_CS_H

#define VCP_CS_H

#include <cs.h>
#include <vcp.h>
#include <orpgvcp.h>

/* Macro definitions for CS format */

#define VCP_CS_DFLT_FNAME		"vcp_xxx"
#define VCP_CS_COMMENT			'#'
#define VCP_CS_MAXLEN			80

/* Macro definitions for VCP_attr section. */
#define VCP_CS_ATTR_KEY			"VCP_attr"
#define VCP_CS_MSG_SIZE_KEY		"msg_size_shorts"
#define VCP_CS_MSG_SIZE_TOK		(1 | (CS_SHORT))
#define VCP_CS_PAT_TYPE_KEY		"pattern_type"
#define VCP_CS_PAT_TYPE_TOK		1
#define VCP_CS_PAT_NUM_KEY		"pattern_num"
#define VCP_CS_PAT_NUM_TOK		(1 | (CS_SHORT))
#define VCP_CS_WX_MODE_KEY		"wx_mode"
#define VCP_CS_WX_MODE_TOK		1 
#define VCP_CS_NUM_ELEVS_KEY		"num_elev_cuts"
#define VCP_CS_NUM_ELEVS_TOK		(1 | (CS_SHORT))
#define VCP_CS_CLUT_GRP_KEY		"clutmap_grp"
#define VCP_CS_CLUT_GRP_TOK		(1 | (CS_SHORT))
#define VCP_CS_DOP_RESO_KEY		"velocity_reso"
#define VCP_CS_DOP_RESO_TOK		(1 | (CS_FLOAT))
#define VCP_CS_PULSE_WID_KEY		"pulse_width"
#define VCP_CS_PULSE_WID_TOK		1
#define VCP_CS_WHERE_DEF_KEY    	"where_defined"
#define VCP_CS_WHERE_DEF_TOK    	1
#define VCP_CS_ALLOW_SAILS_KEY		"allow_sails"
#define VCP_CS_ALLOW_SAILS_TOK		(1 | (CS_SHORT))

/* If flag is set, the token associated with the flag has 
   3 possible (allowable) values:
   RPG:  RPG Site-specific
   RDA:  RDA Site-specific
   BOTH: RDA/RPG Site-specific
*/
#define VCP_CS_SITE_VCP_KEY		"site_specific_vcp"
#define VCP_CS_SITE_VCP_TOK		1 


#define VCP_CS_ELEV_ATTR_KEY		"Elev_attr"
#define VCP_CS_ELEV_ANG_KEY		"elev_ang_deg"
#define VCP_CS_ELEV_ANG_TOK		(1 | (CS_FLOAT))
#define VCP_CS_WF_KEY			"waveform_type"
#define VCP_CS_WF_TOK			1
#define VCP_CS_PHASE_KEY		"phase"
#define VCP_CS_PHASE_TOK		1
#define VCP_CS_SNR_KEY			"SNR_thresh_dB"
#define VCP_CS_SNRZ_TOK			(1 | (CS_FLOAT))
#define VCP_CS_SNRV_TOK			(2 | (CS_FLOAT))
#define VCP_CS_SNRW_TOK			(3 | (CS_FLOAT))
#define VCP_CS_SNRDZ_TOK		(4 | (CS_FLOAT))
#define VCP_CS_SNRP_TOK			(5 | (CS_FLOAT))
#define VCP_CS_SNRC_TOK			(6 | (CS_FLOAT))
#define VCP_CS_SURV_PRF_KEY		"surv_prf"
#define VCP_CS_SURV_PRF_TOK		(1 | (CS_SHORT))
#define VCP_CS_SURV_PULSES_KEY		"surv_pulses"
#define VCP_CS_SURV_PULSES_TOK		(1 | (CS_SHORT))
#define VCP_CS_SECT_ANG_KEY		"edge_angle"
#define VCP_CS_SECT_ANG_TOK		(1 | (CS_FLOAT))
#define VCP_CS_DOP_PRF_KEY		"dop_prf"
#define VCP_CS_DOP_PRF_TOK		(1 | (CS_SHORT))
#define VCP_CS_DOP_PULSES_KEY		"dop_pulses"
#define VCP_CS_DOP_PULSES_TOK		(1 | (CS_SHORT))
#define VCP_CS_SCAN_RATE_KEY		"scan_rate_dps"
#define VCP_CS_SCAN_RATE_TOK		(1 | (CS_FLOAT))
#define VCP_CS_SCAN_PERIOD_KEY		"scan_period_s"
#define VCP_CS_SCAN_PERIOD_TOK		(1 | (CS_SHORT))
#define VCP_CS_ALWB_PRF_KEY		"allowable_prfs"
#define VCP_CS_RPG_ELEV_NUM_KEY		"rpg_elev_num"
#define VCP_CS_RPG_ELEV_NUM_TOK		(1 | (CS_SHORT))
#define VCP_CS_RPG_SUPER_RES_KEY	"super_res"
#define VCP_CS_RPG_SUPER_RES_TOK	(1 | (CS_SHORT))
#define VCP_CS_RPG_DUAL_POL_KEY		"dual_pol"
#define VCP_CS_RPG_DUAL_POL_TOK		(1 | (CS_SHORT))


#endif
