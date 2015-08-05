

/******************************************************************

    Private header file for the ORPGADPT and PROP modules - The new
    implementation based on DEA.
 
******************************************************************/

/* 
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/04/09 14:09:15 $
 * $Id: orpgadpt_n_def.h,v 1.5 2004/04/09 14:09:15 cheryls Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef ORPGADPT_N_DEF_H
#define ORPGADPT_N_DEF_H


typedef struct {		/* DEA object definition table */
    char *f_name;
    char *value;
    int changed;
} ORPGADPT_field_t;

typedef struct {		/* DEA object definition table */
    char *call_name;
    char *struct_name;
    char *de_id;
    SMI_info_t *si;
    char *data;			/* stores C struct if si is not NULL */
    void *cb;
    unsigned short gid;		/* on_change group ID */
    char changed;
    char n_fs;			/* number of PROP fields */
    ORPGADPT_field_t *fs;	/* PROP fields */
} ORPGADPT_object_t;

int ORPGADPT_get_obj_table (ORPGADPT_object_t **objp);

#ifdef NEED_ADPT_OBJ_TABLE
static ORPGADPT_object_t Obj_entries[] = {
    {"Site Information", "Siteadp_adpt_t", 
		"site_info.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Redundant Information", "Redundant_info_t", 
		"Redundant_info.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"MLOS Information", "mlos_info_t", 
		"mlos_info.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Wideband Parameters", "rda_control_adapt_t", 
		"rda_control_adapt.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Precip Detection", "precip_detect1_t", 
		"precip_detect.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Layer Product Parameters", "layer_prod_params_t", 
		"layer_prod_params_t.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"RCM Product Parameters", "rcm_prod_params_t", 
		"rcm_prod_params_t.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Cell Product Parameters", "cell_prod_params_t", 
		"cell_prod_params_t.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"VAD/RCM Heights", "vad_rcm_heights_t", 
		"vad_rcm_heights_t.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"STP Data Levels", "", 
		"STP_data_levels.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"OHP/THP Data Levels", "", 
		"OHP/THP_data_levels.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Reflectivity Data Levels (Clear Air/16)", "", 
		"Reflectivity_data_levels_clear_16.", 
					NULL, NULL, NULL, 0,  1, 0, NULL},
/*    {"Reflectivity Data Levels (Clear Air/8)", "", 
		"Reflectivity_data_levels_clear_8.", 
					NULL, NULL, NULL, 0,  1, 0, NULL}, */
    {"Reflectivity Data Levels (Precip/16)", "", 
		"Reflectivity_data_levels_precip_16.", 
					NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Reflectivity Data Levels (Precip/8)", "", 
		"Reflectivity_data_levels_precip_8.", 
					NULL, NULL, NULL, 0,  1, 0, NULL},
    {"RCM Reflectivity Data Levels", "", 
	"RCM_reflectivity_data_levels.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Velocity Data Levels (Precip 16/0.97)", "", 
	"Vel_data_level_precip_16_97.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Velocity Data Levels (Precip 16/1.94)", "", 
	"Vel_data_level_precip_16_194.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Velocity Data Levels (Precip 8/0.97)", "", 
	"Vel_data_level_precip_8_97.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Velocity Data Levels (Precip 8/1.94)", "", 
	"Vel_data_level_precip_8_194.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Velocity Data Levels (Clear Air 16/0.97)", "", 
	"Vel_data_level_clear_16_97.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Velocity Data Levels (Clear Air 16/1.94)", "", 
	"Vel_data_level_clear_16_194.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Velocity Data Levels (Clear Air 8/0.97)", "", 
	"Vel_data_level_clear_8_97.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Velocity Data Levels (Clear Air 8/1.94)", "", 
	"Vel_data_level_clear_8_194.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Hail Detection", "hail_algorithm_t", 
		"alg.hail.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Storm Cell Tracking", "storm_cell_track_t", 
		"alg.storm_cell_track.", NULL, NULL, NULL, 0,  1, 0, NULL},
    {"Tornado Detection", "tda_t", 
		"alg.tda.", NULL, NULL, NULL, 0,  1, 0, NULL},
};	/* This static table may be read from a file in the future */

static int N_obj_entries = sizeof (Obj_entries) / sizeof (ORPGADPT_object_t);

#endif

#endif		/* #ifndef ORPGADPT_N_DEF_H */
