
/******************************************************************

    This is the private header file for dpprep - the dual-pol 
    pre-processing program.
	
******************************************************************/

/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/06/23 19:30:25 $ */
/* $Id: dpprep.h,v 1.12 2014/06/23 19:30:25 steves Exp $ */
/* $Revision:  */
/* $State: */

#ifndef DPPREP_H
#define DPPREP_H

#ifndef DPPREP_TEST
#define USE_FLOAT 1
#endif

/* Internal data type, math functions and constants - float */
#ifdef USE_FLOAT
typedef float DPP_d_t;
#define LOG10 log10f
#define EXP10 exp10f
#define SQRT sqrtf
#define DPP_NO_DATA -1.234567e20f
#define DPP_ALIASED -1.334567e20f	/* must < DPP_NO_DATA */
#define Csmall .000000001f
#define Cp1 0.1f
#define Cp5 0.5f
#define Cp01 0.01f
#define Cp04 0.04f
#define Cp004 0.004f
#define C10 10.f
#define C1 1.f
#define C0 0.f
#else
/* Internal data type, math functions and constants - double */
typedef double DPP_d_t;
#define LOG10 log10
#define EXP10 exp10
#define SQRT sqrt
#define DPP_NO_DATA -1.234567e30
#define DPP_ALIASED -1.334567e30	/* must < DPP_NO_DATA */
#define Csmall .000000001
#define Cp1 0.1
#define Cp5 0.5
#define Cp01 0.01
#define Cp04 0.04
#define Cp004 0.004
#define C10 10.
#define C1 1.
#define C0 0.
#endif

#define MAX_WIN 32
#define SECS_IN_HOUR	3600

typedef struct {		/* parameters for pre-processing */
    DPP_d_t atmos;		/* atmospheric attenuation factor (-.02 to 
				   -.002 dB/km) */
    DPP_d_t z_syscal;		/* z calibration adjustment (-99 to 99 dB) */
    DPP_d_t dbz0;		/* dBZ0 (-99 to 99 dB) */
    DPP_d_t zdr_syscal;		/* zdr calibration adjustment (-7.875 to 7.75
				   dB) */
    DPP_d_t init_fdp;		/* initial system differential phase (deg) */
    DPP_d_t zdr_thr;		/* RDA threshold for valid Zdr data (-12 to 
				   20 dB) */
    DPP_d_t cor_thr;		/* RDA threshold for valid correlation data 
				   (-12 to 20 dB) */
    DPP_d_t n_h;		/* horizontal channel noise level (-100 to -50
				   dBm) */
    DPP_d_t n_v;		/* vertical channel noise level (-100 to -50
				   dBm) */

    /* RPG adaptaion data */
    DPP_d_t dbz_thresh;		/* Minimum reflectivity for accepting short 
				   gate Kdp (default 40 dBZ) */
    DPP_d_t corr_thresh;	/* Correlation threshold for Kdp calculation 
				   (default 0.9) */
    DPP_d_t md_snr_thresh;	/* SNR threshold for meteo data identification 
				   (default 5.0) */
    DPP_d_t max_diff_phidp;	/* Maximum Phidp difference for acceptable 
				   data (default 100.) */
    DPP_d_t max_diff_dbz;	/* Maximum DBZ difference for acceptable data
				   (default 50.) */
    int dbz_window;		/* Window size for DBZ smoothing (default 3) */
    int window;			/* Window size for smoothing other fields 
				   (default 5) */
    int short_gate;		/* Window size for short gate Kdp calculation 
				   (default 9) */
    int long_gate;		/* Window size for long gate Kdp calculation 
				   (default 25) */

    DPP_d_t zr0;		/* range of the center of the first sample 
				   volume (km) */
    int n_zgates;		/* number of Z sample volumes in the radial */
    DPP_d_t zg_size;		/* Z sample volume size (.25 km) */
    DPP_d_t vr0;		/* Doppler range 0 */ 
    int n_vgates;		/* number of V sample volumes in the radial */
    DPP_d_t vg_size;		/* V sample volume size (.25 km) */
    DPP_d_t dr0;		/* DP range 0 */ 
    int n_dgates;		/* number of DP sample volumes in the radial */
    DPP_d_t dg_size;		/* DP sample volume size (.25 km) */

    int *dz_ind;		/* index in z gates of index in d gates */
    int *zd_ind;		/* index in d gates of index in z gates */

    DPP_d_t alpha;		/* a deduced parameter for efficiency */

    /* parameters for identifying high-attenuation-present-radial testing */
    int art_start_bin;		/* starting bin (180) */
    int art_count;		/* minimum count of the high att bins (10) */
    DPP_d_t art_min_z;		/* minimum z for high att bins (30.0) */
    DPP_d_t art_max_z;		/* maximum z for high att bins (50.0) */
    DPP_d_t art_v;		/* v threshold for high att bins (1.0) */
    DPP_d_t art_corr;		/* corr threshold for high att bins (0.7) */
    DPP_d_t art_min_sw;         /* minimum sw for high att bins (2.0) */

    /* parameters for estimation of initial system differential phase */
    int     isdp_est;           /* Estimate of initial system diff phase */
    int     isdp_apply;         /* 1= apply ISDP estimate to base data */

    /* parameters for estimation of ZDR from Bragg scatter */
    int     vcp_num;            /* Volume coverage pattern.  See basedata.h */
    int     status;             /* Radial status. See basedata.h */
    float   target_elev;        /* Target elevation angle. See basedata.h */
    int     zdr_time;           /* Volume time of ZDR bias estimate */
    unsigned short zdr_date;    /* Volume Julian date of ZDR bias estimate */
} Dpp_params_t;

typedef struct {		/* structure for pre-processing output */
    DPP_d_t *rho_prcd; 		/* correlation coefficient smoothed and 
				   corrected for noise (0 to 1) */
    DPP_d_t *phi_long_gate; 	/* differential phase smoothed by a 5 gate 
				   median filter, a 25-gate running average 
				   filter and finally interpolated between 
				   meteorological targets (0 to 720 degrees) */
    DPP_d_t *zdr_prcd;		/* differential reflectivity processed by 
				   calibration adjustment and noise and 
				   attenuation correction (-8 to 8 dB) */
    DPP_d_t *z_prcd; 		/* horizontal reflectivity processed by 
				   calibration adjustment, smoothing and 
				   attenuation correction (-32 to 94 dBZ) */
    DPP_d_t *snr; 		/* derived horizontal signal noise ratio (-12 
				   to 110 dB) */
    DPP_d_t *vh_smd; 		/* smoothed horizontal velocity (m/s) */
    DPP_d_t *kdp; 		/* specific differential phase (degrees/km) */
    DPP_d_t *sd_phi;		/* standard deviation (texture) of the 
				   differential phase (0 to 100 degrees) */
    DPP_d_t *sd_zh;		/* standard deviation (texture) of the 
				   horizontal reflectivity (0 to 30 dBZ) */
} Dpp_out_fields_t;

int DPPF_process_radial (char *input, char **output, int *length);
int DPPP_process_data (Dpp_params_t *params, int elev_num, 
		DPP_d_t *ref, DPP_d_t *vel, DPP_d_t *spw,
		DPP_d_t *rho, DPP_d_t *phi, DPP_d_t *zdr,
		Dpp_out_fields_t *out, int *is_ha);
void DPPL_test ();
void DPPT_average_filter (int n, int w, DPP_d_t *in, DPP_d_t *out);
void DPPT_median_filter (int n, int w, DPP_d_t *in, DPP_d_t *out);
void DPPT_std_filter (int n, int w, DPP_d_t std_thresholds, 
				DPP_d_t *in, DPP_d_t *in_smd, DPP_d_t *out);
void DPPT_lls_filter (int n, int w, DPP_d_t *in, DPP_d_t *out);
DPP_d_t DPPT_med_filter (DPP_d_t *arr, int n);
void DPPP_test ();
float DPPC_calc_system_PhiDP(const int    num_bins, const float  no_data,
                             const int    elev_num, const float* RhoHV,
                             const float* PhiDP,    const float* Z);
int DPPC_end_of_elev_proc(const int vol_num,  const int radial_status,
                            const float isdp);

double Round_signif_digits (double v, int n_digits);
#endif
