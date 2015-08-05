/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 23:01:40 $
 * $Id: pbd.h,v 1.94 2014/12/09 23:01:40 steves Exp $
 * $Revision: 1.94 $
 * $State: Exp $
 */
/**********************************************************************

	Header file for the pbd task.

**********************************************************************/



# ifndef PBD_H
# define PBD_H

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <infr.h>
#include <orpgerr.h>
#include <vcp.h>
#include <gen_stat_msg.h>
#include <basedata.h>
#include <generic_basedata.h>
#include <orpg.h>
#include <orpgrda.h>
#include <orpggdr.h>
#include <prod_gen_msg.h>
#include <alarm_services.h>
#include <siteadp.h>
#include <orpgsite.h>
#include <rda_status.h>
#include <orpgsails.h>


#define PBD_MAX_SCANS           	80 	/* Maximum volume scan number for
                                      		   volume scan number. */

/* Mask values for PBD_start_volume_required and PBD_start_elevation_required. */
/* PBD_DONT_SEND_RADIAL and PBD_SEND_RADIAL are only to be used if the
   least significant short is none zero.... otherwise PBD_PROCESS_NORMAL is
   only used.  PBD_SEND_RADIAL indicates subsequent radial messages should be 
   processed through the RPG.  PBD_DONT_SEND_RADIALS indicates subsequent radials
   should not be processed through RPG. */
#define PBD_SEND_RADIAL			0x00020000	
#define PBD_DONT_SEND_RADIAL   		0x00010000 
#define PBD_PROCESS_NORMAL     		0x00000000 

/* Return values from PH_radial_validation. */
#define PBD_NEW_FAILURE			0xfffffffe	
#define PBD_CONTINUING_FAILURE   	0xffffffff	
#define PBD_NORMAL             		0x00000000 

/* Macro definitions for ALARM services. */
#define PBD_INACTVTY_ALRM_ID     	1
#define PBD_INACTVTY_ALARM_INTERVAL     300
#define PBD_INACTVTY_CONT 	     	1800 

#define PBD_INBUF_CHECK_ALRM_ID		2
#define PBD_INBUF_CHECK_ALARM_INTERVAL	4

/* Macro definitions for related to azimuth resolution. */
#define HALF_DEG_RADIALS		1
#define ONE_DEG_RADIALS			2
#define LR_MAXN_RADIALS			400
#define HR_MAXN_RADIALS			800

#define PBD_MISSING_AZIMUTH		-999.0
#define PBD_MISSING_ELEVATION		-999.0

/* Macro definitions for flags used by the Move_additional_data function
   in pbd_process_data.c. */
#define PBD_DZDR_MOVED			1
#define PBD_DPHI_MOVED			2
#define PBD_DRHO_MOVED			4

#ifdef GLOBAL_DEFINED
#define EXTERN
#else
#define EXTERN extern
#endif

typedef struct Moment_info {

   int rda_surv_bin_off;                /* Offset to first good surveillance bin
                                           (RDA radial). */

   int rpg_surv_bin_off;                /* Offset to first good surveillance bin
                                           (RPG radial). */

   int rda_surv_bin_size;               /* Size, in meters, of a surveillance bin. */

   int num_surv_bins;                   /* Number of surveillance bins to copy. */

   int rda_dop_bin_off;                 /* Offset to first good Doppler bin
                                           (RDA radial). */

   int rpg_dop_bin_off;                 /* Offset to first good Doppler bin
                                           (RPG radial). */

   int num_dop_bins;                    /* Number of Doppler bins to copy. */

} Moment_info_t;

EXTERN Moment_info_t Info;               /* Bin offset information. */

EXTERN int PBD_verbose;                  /* Verbosity level. */
EXTERN int PBD_response_LB;		 /* Data ID for RDA response LB. */
EXTERN int PBD_radial_out_LB;		 /* Data ID for process basedata LB. */
EXTERN int PBD_weather_mode;		 /* Current weather mode to be advertised 
                                            with volume status. */
EXTERN int PBD_vcp_number;		 /* Current volume coverage pattern. */
EXTERN int PBD_super_res_this_elev;	 /* Set to 1 if this elevation is super res, 0 otherwise. */

/* Data structure used to save Reflectivity Moment data for split cut processing. */
typedef struct Moment {

   Generic_moment_t mom;

   unsigned char ref[MAX_BASEDATA_REF_SIZE];

   int has_zdr;				/* Flag, if set, indicates there is ZDR data. */

   Generic_moment_t zdr_mom;

   unsigned char zdr[BASEDATA_ZDR_SIZE];

   int has_phi;				/* Flag, if set, indicates there is PHI data. */

   Generic_moment_t phi_mom;

   unsigned short phi[BASEDATA_PHI_SIZE];

   int has_rho;				/* Flag, if set, indicates there is RHO data. */

   Generic_moment_t rho_mom;

   unsigned char rho[BASEDATA_PHI_SIZE];
   

} Z_t;

EXTERN Z_t PBD_saved_ref[HR_MAXN_RADIALS];  /* Save reflectivity data for split cut processing. */
EXTERN int PBD_n_saved;                 /* Number of radials saved */
EXTERN Generic_basedata_header_t PBD_Z_hdr[HR_MAXN_RADIALS];
                                        /* Generic basedata header. */
EXTERN Generic_elev_t PBD_Z_elev[HR_MAXN_RADIALS];
                                        /* Generic elevation header. */

EXTERN int PBD_saved_ind;		 /* Used of split cut processing: Match reflectivity radial. */
EXTERN int PBD_max_num_radials;	 	 /* Maximum number of radials as a function of azimuth
                                            resolution. */
EXTERN int PBD_rad_wx_mode;		 /* Current weather mode to be advertised 
                                            in radial. */
EXTERN int PBD_volume_scan_number;	 /* Current RPG volume scan number.  Value 
                                            varies between 1- 80. Used to support legacy. */
EXTERN int PBD_volume_seq_number;        /* Current RPG volume scan sequence number. 
                                            Monotonically increasing number.  Used to
                                            support ORPG. */
EXTERN int PBD_start_volume_required;    /* Start of volume scan required flag.  This 
                                            flag consists of two parts.  The most significant
                                            short indicates whether or not the current radial
                                            should be processed or not.  The least significant
                                            short is either TRUE (1) or FALSE (0).  */
EXTERN int PBD_start_elevation_required; /* Start of elevation scan required flag.  Definition
                                            is similar to PBD_start_volume_required flag. */
EXTERN int PBD_verify_elevation;         /* Verify elevation flag. */
EXTERN int PBD_expected_elev_num;        /* RDA elevation number expected for next start of 
                                            elevation. */  
EXTERN int PBD_current_elev_num;         /* The current RDA elevation number. */
EXTERN char PBD_alg_control;             /* Algorithm control flag. */
EXTERN char PBD_aborted_volume;          /* Aborted volume scan. */
EXTERN int PBD_rda_comm_inactivity;	 /* RDA communication inactivity flag. */
EXTERN time_t PBD_time_of_last_message;	 /* Time of last comm manager message received. */
EXTERN int PBD_spot_blank_bitmap;	 /* Spot blanking bitmap for Scan Summary. */

#define     RESTART_VOLUME               1
#define     RESTART_ELEVATION            2
EXTERN int  PBD_volume_restart_commanded; /* Flag, if set, indicates a volume restart command
                                             was received. */
EXTERN int  PBD_elevation_restart_commanded; /* Flag, if set, indicates an elevation restart
                                                command was received. */
EXTERN short PBD_rda_height;             /* RDA height, in meters MSL. */

EXTERN int PBD_sr_70000;		 /* Slant range to 70 Kft. */

EXTERN float PBD_latitude;               /* RDA latitude, in deg. */

EXTERN float PBD_longitude;              /* RDA latitude, in deg. */

#define PBD_RADAR_NAME_LEN               6
EXTERN char  PBD_radar_name[PBD_RADAR_NAME_LEN]; /* RDA ICAO. */

EXTERN int PBD_old_weather_mode;         /* Previous scans weather mode. */

EXTERN int PBD_old_vcp_number;           /* Previous scans VCP number. */

EXTERN VCP_ICD_msg_t PBD_rda_vcp_data;	 /* Local copy of the RDA VCP data. */

EXTERN short PBD_rda_rdccon[ECUTMAX];    /* RPG elevation numbers for RDA 
                                            VCP data. */

EXTERN int PBD_supplemental_scan_vcp;    /* Flag if set, indicates VCP has supplemental
                                            cuts ... i.e., SAILS-type cuts. */

EXTERN unsigned char PBD_sails_enabled;	 /* SAILs enabled (1) or disabled (0) flag. */

#define PBD_MAX_INSERTED_CUTS            3 /* Maximum number of SAILS cuts (assumes a SAILS
                                              cuts is actually a pair of cuts ... i.e., a
                                              split cut. ) */

#define PBD_DEFAULT_N_SAILS_CUTS	 1
EXTERN int PBD_N_sails_cuts;	         /* Number of SAILs cuts to add when SAILs enabled. */

EXTERN int PBD_N_sails_cuts_this_vol;    /* Number of SAILs cuts in the current volume scan. */

EXTERN int PBD_last_ele_flag;            /* Last elevation flag (1 - yes, 0 - no) */

EXTERN unsigned char PBD_auto_prf;	 /* Auto PRF enabled (1) or disabled (0) flag. */

EXTERN unsigned short PBD_VCP_suppl_flags[ECUTMAX];
                                         /* Supplement flags for VCP data. */

EXTERN int PBD_use_locally_defined_VCP;  /* Flag, if set, indicates to get information 
                                            about VCPs from adaptation data. */

EXTERN int PBD_volume_status_updated;    /* Flag, when set, indicates the Volume Status
                                            has been updated. */

EXTERN int PBD_RPG_info_updated;         /* Flag, when set, indicates the RPG Info
                                            has been updated. */

#define PBD_IS_PLAYBACK			0
#define PBD_IS_WB_SIMULATOR		1
#define PBD_IS_REAL_RDA			2
EXTERN int PBD_is_rda;                  /* Indicator for whether pbd is receiving
                                           data from a real RDA, simulator or playback. 
                                           This controls some quality control checks if 
					   nonzero. */

EXTERN int PBD_waveform_type;          /* Waveform type for the current cut. */

EXTERN int PBD_split_cut;              /* Split cut flag (1/0 - Yes/No Doppler split cut) */

EXTERN unsigned short PBD_data_trans_enabled;
				       /* Data transmission enabled from RDA Status. */

EXTERN int PBD_avset_status;           /* Avset status: 2 - Enabled, 4 - Disabled. */

EXTERN int PBD_RxRN_state;             /* Radial-by-radial noise state: 0 - Disabled, 1 - Enabled. */

EXTERN int PBD_CBT_state;              /* CBT state: 0 - Disabled, 1 - Enabled. */

EXTERN int PBD_last_commanded_vcp;     /* Last commanded VCP.  Needed for SAILS. */

EXTERN int PBD_test_SZ2_PRF_selection;
                                       /* Meant to be used in conjunction with SAILS, always
                                          sets the supplemental cuts to use the default SZ2 
                                          PRFs.   Thus the first split cut is what PRF Selection
                                          chooses, the second cut is the default. */

#define PBD_UNDEF_LAST_ELEV_ANG		195
#define PBD_MIN_LAST_ELEV_ANG		50
EXTERN int PBD_last_elevation_angle;   /* Angle, in deg*10, of last cut in VCP. */

EXTERN float PBD_vel_tover;            /* Velocity Tover from RDA Adaptation Data. */

EXTERN float PBD_spw_tover;            /* Velocity Tover from RDA Adaptation Data. */

#define PBD_UNDEFINED_SIG_PROC_STATE   0xffff
#define PBD_UNKNOWN_SIG_PROC_STATE     0xfffd
EXTERN unsigned short PBD_sig_proc_states;
                                       /* Signal Processing States.  See RDA/RPG ICD. */

/* The following are used for interference detection. */
EXTERN float PBD_h_shrt_pulse_noise;
EXTERN float PBD_h_shrt_pulse_noise_thr;
EXTERN float PBD_v_shrt_pulse_noise;
EXTERN float PBD_v_shrt_pulse_noise_thr;
EXTERN float PBD_h_long_pulse_noise;
EXTERN float PBD_h_long_pulse_noise_thr;
EXTERN float PBD_v_long_pulse_noise;
EXTERN float PBD_v_long_pulse_noise_thr;
EXTERN int PBD_is_short_pulse;
EXTERN int PBD_sun_spike_msgs;           

#define PBD_UNDEFINED_ANGLE		-999.0f
#define PBD_UNDEFINED_NOISE              999.0f

typedef struct {

   int h_noise_cnt;		/* Radial counter for H interference radials */

   int v_noise_cnt;		/* Radial counter for V interference radials */

   float h_sun_noise_lvl;	/* H Sun noise level */

   float h_sun_noise_azm;	/* H Sun noise level azm (deg) */

   float h_sun_noise_ele;	/* H Sun noise level ele (deg) */

   float v_sun_noise_lvl;	/* V Sun noise level */

   float v_sun_noise_azm;	/* V Sun noise level azm (deg) */

   float v_sun_noise_ele;	/* V Sun noise level azm (ele) */

   float h_max_noise_lvl;	/* H maximum noise level */

   float h_max_noise_azm;	/* H max noise level azm (deg) */

   float h_max_noise_ele;	/* H max noise level ele (deg) */

   float v_max_noise_lvl;	/* V maximum noise level */

   float v_max_noise_azm;	/* V max noise level azm (deg) */

   float v_max_noise_ele;	/* V max noise level ele (deg) */

} PBD_interf_detect_t;

EXTERN PBD_interf_detect_t PBD_i_detect;

/* Enables debugging code. */
EXTERN int PBD_DEBUG;

#ifdef AVSET_TEST

/* This is to support AVSET testing. */
typedef struct {

   int target_elev;

   float area;

} Cut_info_t;

typedef struct {

   int vcp_num;

   int num_cuts;

   Cut_info_t area[MAX_CUTS];

} Area_t;

Area_t Area_info[2];
int Area_info_ind;

/* Area lookup tables, as a function of bin number.  
   Values are in 100s of km^2.  One table for 
   normal resolution data, the other for super res. */
float Bintbl[460];
float Bintbl_sr[1840];

/* Flag used to denote AVSET thresholds meet. */
int Terminate_cut;

/* AVSET enable/disable flag. */
int Avset_enabled;

/* Used to store areal coverage for low and high reflectivity thresholds. */
float Area_low_refl;
float Area_high_refl;
float Area_small_core_refl;
float Area_low_refl_2nd_pass;
float Area_high_refl_2nd_pass;

/* Local copy of adaptable parameters. */
int Low_refl_thresh;
int High_refl_thresh;
int Small_core_refl_thresh;
float Low_refl_area_thresh;
float High_refl_area_thresh;
float Small_core_refl_area_thresh;
float Area_increase;
int Elev_tolerance;
int Max_time_difference;
int Low_refl_thresh_2nd_pass;
int High_refl_thresh_2nd_pass;
float Low_refl_area_thresh_2nd_pass;
float High_refl_area_thresh_2nd_pass;

/* State information. */
time_t Current_volume_time;
time_t Previous_volume_time;
int Current_AVSET_state;;

/* Possible states. */
#define AVSET_INITIAL_STATE		1
#define AVSET_2ND_PASS_STATE		2

/* Function Prototypes. */
int AVSET_compute_area_lookup();
int AVSET_compute_area( char *rpg_basedata );
void AVSET_read_adaptation_data();
float AVSET_find_closest_cut( Base_data_header *rpg_hd );
void AVSET_test_thresholds( Base_data_header *rpg_hd );
void AVSET_initialize_this_volume( Base_data_header *rpg_hd );

#endif

/* Values for RDA control status.  Control status value obtained from ORPGRDA API. */
#define RDA_CONTROL_UNKNOWN     0
#define RDA_CONTROL_REMOTE      CS_RPG_REMOTE
#define RDA_CONTROL_LOCAL       CS_LOCAL_ONLY
#define RDA_CONTROL_EITHER      CS_EITHER

/* Values for RDA status (Only one we care about).  Value obtained from ORPGRDA API. */
#define RDA_STATUS_UNKNOWN      0
#define RDA_STATUS_OPERATE	RS_OPERATE
#define RDA_STATUS_PLAYBACK     AR_PLAYBACK

/* Global Function Prototypes */

/* Modules defined in pbd_set_scan_summary.c */
void SSS_set_scan_summary( int volscan_num,  int rpgwmode, 
                           int rpg_num_elev_cuts, int rda_num_elev_cuts,
                           int last_elevation_angle,
                           Base_data_header *rpg_hd, Generic_basedata_t *gbd );
void SSS_init_read_scan_summary();

/* Modules defined in pbd_process_data.c */
int PD_process_incomplete_radial( char *rda_msg );
int PD_move_data( Generic_basedata_t *gbd, char *rpg_basedata );
int PD_process_rda_vcp_message( char *vcp_data );
void PD_process_rda_rpg_loopback_message( char *loopback_data );

/* Modules defined in pbd_process_header.h */
int PH_process_new_cut( Generic_basedata_t *gbd, Vol_stat_gsm_t *vs_gsm, 
                        int *rpg_num_elev_cuts );
int PH_process_header( Generic_basedata_t *gbd, char *rpg_basedata );
int PH_get_rda_status ( int item );
int PH_send_rda_control_command( int elev_num, int command, int reason );
int PH_restart_scan( int parameter_1, int parameter_2 );
int PH_process_restart_command( int command );
int PH_radial_validation ( Generic_basedata_t *gbd,
                           int *vol_aborted, int *unexpected_bov ); 
void PH_radial_accounting( Base_data_header *rpg_hd, char *rda_msg,
                           Vol_stat_gsm_t *vs_gsm );
int Round( float r );

/* Modules defined in pbd.c */
void PBD_abort (char *format, ... );

/* Modules defined in pbd_verbose.c */
void VM_write_vcp_data( Vcp_struct *vcp );

/* Modules defined in pbd_super_reso.c */
int SR_save_SuperRes_refl_data( char *rda_msg, unsigned short azm,
                                unsigned short elev );
int SR_restore_SuperRes_refl_data( char *rpg_msg, unsigned short azm, 
                                   unsigned short elev );

/* Modules defined in pbd_detect_interference.c */
void ID_site_init( char *radial );
void ID_init_interference_data( int vcp );
void ID_check_interference( char *radial );
void ID_process_rda_perf_maint_msg( char *pmd_data );
void ID_output_interference_msg( char *radial );

/* Modules defined in pbd_handle_sails.c */
int HS_update_vcp_for_sails( Vcp_struct *vcp, Vcp_struct **download_vcp );
int HS_strip_supplemental_cuts( Vcp_struct *vcp, Ele_attr *vcp_local,
                                int *n_cuts );
int HS_update_vcp( Vol_stat_gsm_t *Vs_gsm, int last_elevation_angle );
int HS_check_sails( int vcp_num, Vcp_struct *vcp, Vcp_struct **download_vcp );



#endif

