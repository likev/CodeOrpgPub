#ifndef PRFSELECT_H
#define PRFSELECT_H

#include <a309.h>
#include <basedata.h>
#include <vcp.h>
#include <rpgc.h>
#include <rpgcs.h>
#include <itc.h>
#include <siteadp.h>
#include <prfselect_buf.h>

#define DOP_PRF_BEG                  5
#define DOP_PRF_END             PRFMAX

#define DELPRF_MAX                   5
#define MAX_VCPID                   20

#define DTR		      DEGTORAD

#define MAX_PROC_RNG               230
#define MAX_FOLD_BIN  (4*MAX_PROC_RNG)
#define MAX_UR                     230
#define MAX_RNG                    460
#define EPWR_SIZE          (4*MAX_RNG)   /* Accounts for the possibility of 250m bins to 460 km */
#define MAX_REF_SIZE         EPWR_SIZE  
#define RADIUS			 20.0f
#define RADIUS2			400.0f

/* Assumes an Unsigned Integer is 32 bits in length.  */
#define BITMAP_WORDS               38      
#define BITMAP_CHARS (BITMAP_WORDS*sizeof(unsigned int))

#define FLAG_NO_PWR            -9999.0
#define RNG_MISSING		-999.0f

#define A3CD09_DATAID           ((A3CD09/ITC_IDRANGE)*ITC_IDRANGE)
#define A3CD09_MSGID            (A3CD09%ITC_IDRANGE)

a3cd09 A3cd09;			/* ITC containg storm motion data. */

typedef struct {

   int rpgvcpid;		

   int rpgwmode;

   Vcp_struct current_vcp_table;

} Vcpinfo_t;


/* Information for cell tracking. */
typedef struct {

   char storm_id[4];		/* 2 character storm ID. */

   float cent_rng;		/* Projected range to storm centroid, in km. */

   float cent_azm;		/* Projected azimuth to storm centroid, in deg. */
   
} Storm_info_t;
   
Storm_info_t Storm_info[MAX_STORMS];

typedef struct Points {

   float r1;

   float r2;

} Points_t;


/* Number of storms to examine .... Must be less than or equal to 
   Max_num_storms. */
int Num_storms;

/* Number of cells to examine ... will be either 0 or 1. */
int Num_cells;

/* Adaptation Data for number of storms to examine ... range [1, 5]. */
int Adapt_max_num_storms;

/* Adaptation Data for minimum storm VIL to examine ... range [10.0, 30.0]. */
float Adapt_min_vil;

/* Maximum number of storms to examine ... will be either Adapt_num_storms 
   (for Storm-based PRF selection) or 1 (for Cell-based PRF selection). */
int Max_num_storms;

/* Adaptation Data for minimum PRF ... range [4, 8]. */
int Min_PRF;

/* Storm based PRF selection enabled [1]/disabled [0] flag... . */
int Storm_based_PRF_selection;
int Local_storm_based_PRF_selection;

/* Storm based PRF selection enabled [1]/disabled [0] flag... . */
int Cell_based_PRF_selection;

/* Counter for bins that overlap within the Num_storms 20 km radius of influence. */
int Set_count;

/* Bitmap of bins to check each radial.  Constructed from storms to examine.
   Based on storm centroid and radius of influence. */
unsigned int Bitmap[BITMAP_WORDS];

/* Flag set when A3CD09 ITC is updated. */
int Read_itc;

/* Flag set when PRF Selection Information updated. */
int Read_prf_command;


/* Function prototypes. */
int A30531_prf_init( Base_data_header *rad_hdr, void *outbuf_epwr, void *outbuf_pwr_lookup );
int A30532_echo_overlay( Base_data_header *rad_hdr, unsigned short *radial, float *epwr, 
                         float *pwr_lookup );
int A30533_change_vcp( int vcpatnum, int vcp_num_cuts, Ele_attr *elev_attr, int *disposition );
int A3053A_buffer_control();
int A3053B_get_buffers( char **outbuf, char **scratchbuf, int *prfbuf );
int A3053J_dummy_processor( );
int ST_init();
int ST_identify_storms();
int ST_start_end_bin( Base_data_header *bhd, int *first, int *last );
int CT_init();
int CT_identify_storm( char *id );
int CT_update_storm();
int CT_start_end_bin( Base_data_header *bdh, int *first, int *last );
int CF_init();
int CF_read_adapt();
int CF_update_storm_info();
void CF_write_informational_messages( char *header );
Points_t Find_points( float r0, float az0, float az1 );
int SZ2_read_clutter();
int SZ2_echo_overlay( Base_data_header *rad_hdr, unsigned short *radial, float *epwr, 
                      float *pwr_lookup );


/* Global variable definitions. */
#ifdef PRFSELECT_C
int PS_rpgvcpid;
int Operational;
Vcpinfo_t PS_vcp_info;
Siteadp_adpt_t PS_site_info;
Vol_stat_gsm_t Vol_stat;           /* Volume Status. */

int PS_rpgwmode;                   /* Defines the currernt weather mode. */
Vcp_struct  PS_curr_vcp_tab;       /* Defines the current VCP. */
Vol_stat_gsm_t Vol_stat;           /* Volume Status. */
Prf_command_t Prf_command;
Prf_status_t Prf_status;
unsigned long Del_time;
#endif

#ifdef A30531_C
int Max_proc_bin;
float Bin_range[MAX_REF_SIZE+1];

extern int PS_rpgvcpid;
extern int PS_delta_pri;
extern int SZ2_prf_selection;
extern Vcp_struct PS_curr_vcp_tab;

float Overlaid[DOP_PRF_END+1];
int  Overlaid_cnt[DOP_PRF_END+1];
static int Validprf[DOP_PRF_END+1];
static short Folded_bin1[DOP_PRF_END+1][MAX_FOLD_BIN+1];
static short Folded_bin2[DOP_PRF_END+1][MAX_FOLD_BIN+1];
static short Folded_bin3[DOP_PRF_END+1][MAX_FOLD_BIN+1];
#endif

#define DEF_Z_SNR		2.0
#define DEF_V_SNR		3.5

#ifdef A3053A_C
int PS_delta_pri;
int SZ2_prf_selection;
float Z_snr;
float V_snr;

extern Vcpinfo_t PS_vcp_info;
extern Vcp_struct PS_curr_vcp_tab;
extern Prf_command_t Prf_command;
extern Prf_status_t Prf_status;
#endif

#ifdef COMMON_FUNCS_C
extern Vol_stat_gsm_t Vol_stat;           /* Volume Status. */
extern unsigned long Del_time;
extern Prf_command_t Prf_command;
extern Prf_status_t Prf_status;
extern int Operational;
#endif

#ifdef CELL_PRF_C
extern Prf_command_t Prf_command;
extern Prf_status_t Prf_status;
extern unsigned long Del_time;
#endif

#ifdef STORM_PRF_C
extern unsigned long Del_time;
extern Prf_status_t Prf_status;
#endif

#ifdef SZ2PRF_C
extern int PS_rpgvcpid;
extern int PS_delta_pri;
extern Vcp_struct PS_curr_vcp_tab;
extern int Max_proc_bin;
extern float Bin_range[MAX_REF_SIZE+1];
extern float Z_snr;
extern float V_snr;

extern float Overlaid[DOP_PRF_END+1];
extern int  Overlaid_cnt[DOP_PRF_END+1];
#endif
#endif
