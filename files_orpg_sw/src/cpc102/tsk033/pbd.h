/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/17 21:40:08 $
 * $Id: pbd.h,v 1.1 2010/03/17 21:40:08 ccalvert Exp $
 * $Revision: 1.1 $
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

EXTERN float PBD_latitude;               /* RDA latitude, in deg. */

EXTERN float PBD_longitude;              /* RDA latitude, in deg. */

#define PBD_RADAR_NAME_LEN               6
EXTERN char  PBD_radar_name[PBD_RADAR_NAME_LEN]; /* RDA ICAO. */

EXTERN int PBD_old_weather_mode;         /* Previous scans weather mode. */

EXTERN int PBD_old_vcp_number;           /* Previous scans VCP number. */

EXTERN VCP_ICD_msg_t PBD_rda_vcp_data;	 /* Local copy of the RDA VCP data. */

EXTERN int PBD_volume_status_updated;   /* Flag, when set, indicates the Volume Status
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

#ifdef SNR_TEST

EXTERN int PBD_allow_snr_thresholding; /* Allow SNR thresholding. */

#define QRTKM                                   0
#define ONEKM                                   1
EXTERN double PBD_sensitivity_loss[2]; /* Amount to add to reflectivity SNR threshold. 
                                          Defined for 1/4 km and 1 km reflectivity. */

EXTERN double PBD_z_snr_threshold[VCP_MAXN_CUTS];     
                                       /* SNR threshold for Z and DP data. */

EXTERN double PBD_d_snr_threshold[VCP_MAXN_CUTS];     
                                       /* SNR threshold for Doppler data. */

EXTERN int PBD_apply_speckle_filter;   /* If TRUE, apply speckle filter to censored data. */

EXTERN double PBD_log_range[MAX_BASEDATA_REF_SIZE];      
                                       /* Lookup table of 20logR values, where R in 250 m increments. */

EXTERN double PBD_log_range_1km[BASEDATA_REF_SIZE];      
                                       /* Lookup table of 20logR values, where R in 1 km increments. */

EXTERN double PBD_z_table[256];        /* Lookup table of Z dBZ converted to Reflectivity Factor. */
#endif


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
float Area_low_refl, Area_high_refl;
float Area_low_refl_2nd_pass, Area_high_refl_2nd_pass;

/* Local copy of adaptable parameters. */
int Low_refl_thresh;
int High_refl_thresh;
float Low_refl_area_thresh;
float High_refl_area_thresh;
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
#define RDA_STATUS_PLAYBACK     AR_PLAYBACK

/* Global Function Prototypes */

/* Modules defined in pbd_set_scan_summary.c */
void SSS_set_scan_summary( int volscan_num,  int rpgwmode, 
                           int rpg_num_elev_cuts, int rda_num_elev_cuts,
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
void PH_radial_accounting( Base_data_header *rpg_hd, char *rda_msg );
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


#endif

