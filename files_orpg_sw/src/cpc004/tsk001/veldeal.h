#ifndef VELDEAL_H
#define VELDEAL_H

#include <rpgc.h>
#include <rpgcs.h>
#include <siteadp.h>
#include <alg_adapt.h>
#include <mpda_parameters.h>
#include <radazvd.h>
#include <itc.h>
#include <vcp.h>
#include <vdeal.h>

#define VELDEAL_NAME		"Velocity Dealiasing - "
#define SR_VELDEAL_NAME		"Super Res Velocity Dealiasing - "

#define NUM_INTRVL		6 	/* Maximum number of Nyquist 
					   intervals allowed. */
#define VELDEAL_MAX_SIZE	1200

#define BELOW_THR		0	/* Value indicating below SNR threshold. */
#define BAD			1	/* Value indicating range-folded. */

#define LARGE_VALUE		32767

#define NUM9PT_LKBK		4

#define NUM9PT_LKAHD		5

#define NUM_AVG			(NUM9PT_LKBK+NUM9PT_LKAHD)


/* Structure definition for A304DB. */
#define BAD_PREV_RADIAL		0
#define GOOD_PREV_RADIAL	1
typedef struct A304db {

   int lstgd_dpbin_prevaz;		/* Last good velocity on the 
					   previous saved radial. */

   int fstgd_dpbin_prevaz;		/* First good velocity on the 
					   previous saved radial. */

   int status;				/* Index containing whether the 
					   radial data is good or bad. */

   short *vel_prevaz[2];		/* Previous saved radial velocity
					   data. */

} A304db_t;


/* Nyquist information. */
short Nyq_intrvl;			/* The current Nyquist co-interval,
                                           in scaled (by velocity resolution)
                                           units. */

short Num_intrvl_chk;			/* Number of Nyquist co-intervals used
					   for dealiasing. */

short Nyvel;				/* The current Nyquist velocity. */

short Old_nyvel;			/* The previous Nyquist velocity. */


/* Structure definition for A304DD. */
typedef struct A304dd {

   int fst_good_dpbin;			/* First good bin on the current radial. */

   int lst_good_dpbin;			/* Last good bin on the current radial. */
 
} A304dd_t;


/* Rejected velocity information. */
#define MAX_CON_REJ		10
#define MAX_BINS_DELETED	1024
#define BIN_NUMBER              0
#define VELOCITY                1
int Con_bins_rej;			/* Number of consecutive bins rejected. */

short *Deleted_vel;			/* Array of deleted velocities. */
short *Deleted_bin;			/* Array of bin locations of deleted 
                                           velocities. */

short Num_deleted;			/* Number of deleted velocities. */


/* Velocity jump information. */
int Bin_lrg_azjump;			/* Number of consecutive bins with
					   large velocity jump in azimuth. */

int Num_jump_conrad;			/* Number of consecutive radials with  
					   large velocity jumps. */	

int Vel_jump;				/* Flag indicating whether a large 
					   velocity jump exists on the current
					   radial. */

/* Structure definition for A304DG.  These are  derived thresholds. */
typedef struct A304dg {

   int th_diff_unf_relax;		/* Threshold derived from th_diff_unfold. */
  
   int th_max_stdev;			/* The maximum standard deviation of 
					   velocities in the 9-pt average. */

   int tol_fstchk;			/* th_diff_unfold scaled by Doppler 
					   resolution. */

   int veljmp_mxaz;			/* The maximum velocity difference
					   in azimuth allowed in the final jump
					   check. */

   int veljmp_mxrad;			/* The maximum velocity difference
					   in range allowed in the final jump check. */

   int th_def_vel_diff;			/* Has same value as th_max_stdev. */

   int sounding_avail;			/* Flag indicating wind information 
					   available for dealiasing. */

} A304dg_t;


/* Structure definition for A304DI.  These are the adaptation parameters. */
typedef struct A304di {

   int num_bin_fstchk;

   int num_rep_lkahd;

   int num_rep_lkbk;

   int num_lkbk;

   int num_lkfor;

   int th_conbin_rej;

   int th_max_conazjmp;

   int th_mxmiss;

   int num_reunf_prazs;

   int th_mxbins_jmp;

   float th_diff_unfold;

   int th_bins_lrg_azjmp;

   int num_reunf_cazs;

   float th_vel_jmp_frad;

   float th_vel_jmp_faz;

   float th_scl_stdev;

   float th_scl_diff_unfold;

   int env_sound_to;

   int use_soundings_flag;

   int rep_rejected_vel;

   int disable_radaz_dealiasing;

   int use_sprt_replace_rej;

} A304di_t;


/* Structure definition for A304DJ. */
typedef struct A304dj {
 
   int no_chk_flg;

   int not_wibins_flg;

   int jmpchk;

} A304dj_t;

/* Global variables. */
A304db_t A304db;
A304dd_t A304dd;
A304dg_t A304dg;
A304di_t A304di;
A304dj_t A304dj;
Rdacnt   RDA_cntl;
mpda_adapt_params_t Mpda;
radazvd_t Radazvd;
A3cd97 Ewt;
Pct_obs Pct;
Siteadp_adpt_t Siteadp; 
Envvad_t Envvad;

int Env_wnd_tbl_updt;
int Valid_soundings;
int Min_ewt_entry;
int Max_ewt_entry;
int Waveform;
char *Function_name[64];

/* Lookup table definitions. */
#define VEL_BIAS                233     /* Corresponds to 116.5 m/s in 0.5 m/s
                                           increments.   Used to bias the 
                                           velocity in order to make the velocity
                                           interval larger than what is supported
                                           in the ICD. */
#define BIASED_ZERO             (129 + VEL_BIAS)
#define LOOKUP_SIZE             722     /* Corresponds to +/- 180 m/s in 0.5 m/s
                                           increments. */
#define MIN_VEL                 2       /* Minimum valid biased velocity value. */ 
#define MAX_VEL                 255     /* Maximum valid biased velocity value. */

short *Unbias_vel;        		/* Unbiased raw velocity data. */

int *Square;              		/* Array containing the square of
                                           the velocity. */


/* Function Prototypes for Legacy Dealiasing Algorithm. */
int A304d2_rad_azm_unfold( Base_data_header *radhdr, short *veloc );
int A304d3_min_vel_dif( short *velin, short velcmp, int veldif_tol );
int A304d4_comp_9ptavg( short *veloc, short *radhdr, int binnum,
                        int last_bin_gdvel );
int A304d8_jump_check( short *veloc, int binnum, int last_bin_gdvel );
int A304da_vd_buf_cntrl();
int A304db_update_adaptation( void *input_ptr );
int A304dc_process_radial( void *input_ptr );
int A304di_vd_local_copy( int pulse_width );
int A304dv_find_other_means( short *radhdr, short *veloc, int binnum,
                             int last_bin_gdvel );
int Define_edit_parm( );

/* Function Prototypes for MPDA. */
int mpda_buf_cntrl( int current_vcp );
int apply_mpda( void *input_ptr );
void build_ewt_struct();
void build_rad_header( Base_data_header *hdr, int *el_num, int *rpg_ind,
                       int *rad_stat, int length );
void build_rng_unf_arrys();
void check_for_mpda_vcp( int vcp, int *mpda_flg );
void get_adapt_params( void *struct_address );
void get_moments_status( int *ref_stat, int *vel_stat, int *wid_stat );
void initialize_data_arrys();
int output_mpda_data(int i, void *out_buf, int *size );
void update_ewt_status( void *input_buf );
int vcp_setup( int cuts_elv[], int cmode[], int lastflg[], int cur_vcp );

/* Function Prototypes for 2D Dealiasing Algorithm. */
void VD2D_realtime_processing( Vdeal_t *vdv );


#endif
