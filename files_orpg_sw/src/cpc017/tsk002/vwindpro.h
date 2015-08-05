/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/25 17:19:05 $
 * $Id: vwindpro.h,v 1.6 2014/04/25 17:19:05 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef VWINDPRO
#define VWINDPRO

#include <rpgc.h>
#include <rpgcs.h>
#include <prodsel.h>
#include <alg_adapt.h>
#include <coldat.h>
#include <vad.h>
#include <itc.h>
#include <a309.h>
#include <basedata.h>
#include <a317buf.h>

/* Constants. */

/* Sizing parameters. */
#define NELVEWT			2
#define NVOL			11
#define MAX_NUM_AMBRNG		3	
#define NAZIMS			400		
#define NAZIMS_EWT		NAZIMS
#define NPTS		 	(NVOL*MAX_VAD_HTS)	
#define MAX_VAD_ELVS		20
#define MAX_NELVEWT		2
#define VADRMS			12

/* Missing value indicator. */
#define MISSING			-666.0f

/* Earth's radius in km and m. */
#define RE_KM			6371.0f
#define RE_M			6371000.0f

/* Index of refraction. */
#define IR			1.21f

/* Used in height equations. */
#define RE_IR_KM		(RE_KM*IR)
#define RE_IR_M			(RE_M*IR)
#define SLCON			(2.0/RE_IR_KM)
#define HTS_CONST		(2.0*RE_IR_M)

/* Supplemental VAD range scaling factor. */
#define SCALE_RNG_EWT		0.35f

/* Minimum and maximum processing range. */
#define MINSLR			0.0
#define MAXSLR			230.0
#define CLIPPED_RANGE		60.0

/* Product Buffer Sizes. */
#define VADTMHGT_SIZE		(VAD_OBUF_SZ*sizeof(int))
#define VADPARAM_SIZE		(LEN_VAD_ALERT*sizeof(int))
#define HPLOTS_SIZE		32000

/* Complex number. */
typedef struct complex {

   double real;

   double img;

} Complex_t;


/* AVSET information. */
typedef struct {

   int active;                  /* Active/Inactive flag. */

   int vcp;                     /* VCP in operation. */

   int last_elev_cut;           /* RDA cut number when AVSET terminated 
                                   the volume. */

   int last_rpg_elev_cut;       /* RPG cut number when AVSET terminated 
                                   the volume. */

   float last_elev_angle;       /* Elevation angle when AVSET terminated 
                                   the volume. */

   int prev_elev_cut;           /* RDA cut number when AVSET terminate 
                                   the volume, minus 1. */
    
   int prev_rpg_elev_cut;       /* RPG cut number when AVSET terminate 
                                   the volume, minus 1. */

   float prev_elev_angle;       /* Elevation angle cooresponding to 
                                   prev_elev_cut. */

} Avset_t;


/* ... Adaptation Data ... */

/* Product Selectable Parameters. */
Prodsel_t Prodsel;

/* Site Adaptation Data. */
Siteadp_adpt_t Siteadp;

/* Color Table. */
Coldat_t Color_data;

/* VAD Adaptation Data. */
vad_t Vad;

/* Product Selectable Adaptation Data. */
Prodsel_t Prodsel;

/* ... Status Data ... */

/* Volume Status. */
Vol_stat_gsm_t Vol_stat;

/* Scan Summary. */
Scan_Summary *Summary;

/* Environmental Wind Table. */
A3cd97 Ewt;

/* VAD flags. */
Envvad_t Envvad;


/* ... Product IDs ... */
int Vadtmhgt_id;
int Vadparam_id;
int Hplots_id;

/* Global Variables. */
int Ceminht;
int Cemaxht;
int Ceminht_s;
int Cemaxht_s;
int Cvol;
int Volno;
int Vcpno;

/* Structure Defintions (corresponds to legacy A317VI common. */
typedef struct a317vi {

   int celv;				/* Index within the VAD output arrays
                                           indicating the current elevation 
                                           scan number. */ 

   int irbin;				/* Index within the reflectivity radial to
                                           VAD_RNG. */

   int ivbin;				/* Index within the velocity radial to 
                                           VAD_RNG. */

   int aztst;				/* Flag indicating whether or not the 
                                           selected azimuth sector contains the
                                           0/360 degree crossover. */

   int nradials;			/* Number of radials available for least-
                                           squares fitting. */

   float sum_elv;			/* Summation of elevation angles of the 
                                           radials contained with the elevation 
                                           scan. */

   float sum_ref;			/* Summation of reflectivities at slant 
                                           range VAD_RNG of the radials contained 
                                           within "azm" and "ve". */

   float azm[BASEDATA_MAX_RADIALS];	/* Azimuth angles of the radials between 
                                           AZM_DEG and AZM_END. */

   float ve[BASEDATA_MAX_RADIALS];	/* Doppler velocities at slant range VAD_RNG 
                                           and between AZM_DEG and AZM_END. */

   float dec_last;			/* Distance to the center of the earth for 
                                           most recent elevation scan within the
                                           same volume as the current elevation 
                                           scan. */

   float svw_last;			/* Vertical velocity for the most recent 
                                           elevation scan, within the same volume
                                           scan as the current elevation scan. */

   int tnradials;			/* Total number of radials within an 
                                           elevation scan. */

} A317vi_t;


/* Data Structure Definition for the Supplemental VADs. */
typedef struct a317ec {

   int irbin_ewt;			/* Index within the reflectivity radial to
                                           VAD_RNG_EWT. */

   int ivbin_ewt;			/* Index within the velocity radial to
                                           VAD_RNG_EWT. */

   int nradials_ewt;			/* Number of radials of data available for
                                           least square fitting. Used solely for
                                           supplemental VADS in the Environmental
                                           Winds Table. */

   float sum_elv_ewt;			/* Sum of elevation angles of the radials
                                           contained within the elevation scan.
                                           Used solely for supplemental VADS in the
                                           Environmental Winds Table. */

   float azm_ewt[NAZIMS_EWT];		/* Azimuth angles of the radials over a
                                           full circle. */

   float ve_ewt[NAZIMS_EWT];		/* Doppler velocities at slant range
                                           VAD_RNG_EWT and azimuth AZM_EWT within
                                           the current elevation scan. */

   int tnradials_ewt;			/* Total number of radials in the elevation
                                           scan. Used solely for the supplemental
                                           VADS in the Environmental Winds Table. */

   float vad_rng_ewt;			/* The slant range at which the
                                           supplemental VAD, in the Enviornmental
                                           Winds Table, is performed. */

} A317ec_t;


/* Data Structure Definition for Local Copy of Adaptation Data. */
typedef struct a317va {

   int wmode;				/* Flag which indicates whether the weather
                                           is clear air(1), rain(2) or snow(3). */

   float vad_rng;			/* Slant range at which the VAD analysis is
                                           performed. */

   float azm_beg;			/* Beginning azimuth angle for VAD
                                           analysis. AZM_BEG is the counter
                                           clockwise limit of the selected sector
                                           (Range [0,360]). */

   float azm_end;			/* Ending azimuth angle for VAD analysis.
                                           AZM_END is the clockwise limit of the
                                           selected sector (Range [0,360]). */

   int fit_tests;			/* Number of times through the loop which
                                           performs the least squares fit and then
                                           removes low magnitude outlier data
                                           points. */

   float th_rms;			/* Maximum value RMS can be and still be
                                           accepted as a good wind estimate. */

   float tsmy;				/* ADAPTABLE PARAMETER: The maximum value
                                           CF1 can be and still be accepted as a
                                           good wind estimate, in m/s. Rng:[0,10]. */

   float rh;				/* Height of the radar antenna above sea
                                           level. */

   int minpts;				/* Minimum number of sample points. */

} A317va_t;


/* VAD output definition for the VADs at constant range. */
typedef struct a317vd {

   float htg[MAX_VAD_ELVS];		/* The height of the vad data values for all 
					   of the saved volume and elevation scans, 
                                           in meters. */

   float rms[MAX_VAD_ELVS];		/* The rms values for each of the saved volume 
					   and elevation scans, m/s. */

   float shw[MAX_VAD_ELVS];		/* The horizontal wind speed values for each of 
					   the saved volume and elevation scans, in m/s. */

   float svw[MAX_VAD_ELVS];		/* The vertical wind speed values for each of the
					   saved volume and elevation scans, in m/s. */

   float div[MAX_VAD_ELVS];		/* The divergence values for each of the saved
					   volume and elevation scans, in 1/s. */


   float hwd[MAX_VAD_ELVS];		/* The horizontal wind direction values for each of
					   the saved volume and elevation scans, degrees. */
   
   int date;				/* The date of the current volume scan, 
                                           JULIAN days. */

   int time[NVOL];			/* The time of each of the volume scans, hhmmss. */

} A317vd_t;


/* Data output definition for the VADs at selected heights. */
typedef struct a317ve {

   int nrads[MAX_VAD_HTS];		/* Number of radials of data available for
                                           least square fitting. */

   int elcn[MAX_VAD_HTS];		/* Elevation cut number. */

   float slrn[MAX_VAD_HTS];		/* Slant range in km.   Indexed by height. */

   int velbin[MAX_VAD_HTS];		/* Velocity bin. */

   int refbin[MAX_VAD_HTS];		/* Reflectivity bin. */

   int ambig_range[MAX_VAD_HTS][MAX_NUM_AMBRNG];	
					/* Range of ambiguous ranges. */

   int ambig_range_ptr[MAX_VAD_HTS];	/* Number of ambiguous ranges. */

   int ambig_azlim[MAX_VAD_HTS][MAX_NUM_AMBRNG];
					/* Azimuth of ambiguous ranges. */

   float hcf1[MAX_VAD_HTS];		/* Fourier coefficient. */

   float hcf2[MAX_VAD_HTS];		/* Fourier coefficient. */

   float hcf3[MAX_VAD_HTS];		/* Fourier coefficient. */

   float hvel[MAX_VAD_HTS][NAZIMS];	/* Velocity values for each data point. */

   float href[MAX_VAD_HTS][NAZIMS];	/* Reflectivity values for each data point. */

   float hazm[MAX_VAD_HTS][NAZIMS];	/* Azimith angles for each data point, in degrees. */

   float vhtg[NVOL][MAX_VAD_HTS];	/* Height of the VAD data for all saved volume and 
					   elevation scans.  Indexed by volume, height. */

   float vrms[NVOL][MAX_VAD_HTS];	/* RMS for each saved volume and elevation scan. */

   int vnpt[NVOL][MAX_VAD_HTS];		/* Number of data points used in VAD processing. */

   float vhwd[NVOL][MAX_VAD_HTS];	/* Horizontal wind directions for all saved
					   volume and elevation scans.  Indexed by
					   volume, height. */

   float vshw[NVOL][MAX_VAD_HTS];	/* Horizontal wind speed for all saved volume
					   and elevation scans.  Indexed by volume, height. */

   int vnhts[NVOL];			/* VAD heights. */

} A317ve_t;


typedef struct a318ci {

   float maxht;				/* Maximum height of current elevation. */

   int yposition[MAX_VAD_HTS];		/* Velocity Azimuth Display grid current Y axis. */

} A318ci_t;


/* Data output definition for the VADs at selected heights (NEW). */
typedef struct a317vs {

   int nrads[ECUTMAX][MAX_VAD_HTS];     /* Number of radials of data available for
                                           least square fitting. */

   int elcn[ECUTMAX][MAX_VAD_HTS];      /* Elevation cut number. */

   float slrn[ECUTMAX][MAX_VAD_HTS];    /* Slant range in km.   Indexed by height. */

   int velbin[ECUTMAX][MAX_VAD_HTS];    /* Velocity bin. */

   int refbin[ECUTMAX][MAX_VAD_HTS];    /* Reflectivity bin. */

   int ambig_range[MAX_VAD_HTS][ECUTMAX][MAX_NUM_AMBRNG];
                                        /* Range of ambiguous ranges. */

   int ambig_range_ptr[ECUTMAX][MAX_VAD_HTS];    
                                        /* Number of ambiguous ranges. */

   int ambig_azlim[MAX_VAD_HTS][ECUTMAX][MAX_NUM_AMBRNG];
                                        /* Azimuth of ambiguous ranges. */

   float hcf1[ECUTMAX][MAX_VAD_HTS];    /* Fourier coefficient. */

   float hcf2[ECUTMAX][MAX_VAD_HTS];    /* Fourier coefficient. */

   float hcf3[ECUTMAX][MAX_VAD_HTS];    /* Fourier coefficient. */

   float hvel[ECUTMAX][MAX_VAD_HTS][NAZIMS];     
                                        /* Velocity values for each data point, in m/s. */

   float href[ECUTMAX][MAX_VAD_HTS][NAZIMS];     
                                        /* Reflectivity values for each data point, in 
                                           scaled/biased (ICD) units. */

   float href_avg[ECUTMAX][MAX_VAD_HTS]; /* Average Reflectivity for each cut/height pair, 
                                           in dBZ. */

   float hazm[ECUTMAX][MAX_VAD_HTS][NAZIMS];     
                                        /* Azimith angles for each data point, in degrees. */

   float vhtg[ECUTMAX][MAX_VAD_HTS];    /* Height of the VAD data for all saved volume and 
                                           elevation scans.  Indexed by volume, height. */

   float vrms[ECUTMAX][MAX_VAD_HTS];    /* RMS for each saved volume and elevation scan. */

   float vhwd[ECUTMAX][MAX_VAD_HTS];    /* Horizontal wind directions for all saved
                                           volume and elevation scans.  Indexed by
                                           volume, height. */

   float vshw[ECUTMAX][MAX_VAD_HTS];    /* Horizontal wind speed for all saved volume
                                           and elevation scans.  Indexed by volume, height. */

   float velv[ECUTMAX][MAX_VAD_HTS];    /* Elevation angle for the wind estimate. */

   int vnpt[ECUTMAX][MAX_VAD_HTS];      /* Number of data points used in VAD processing. */

   int vnhts;                           /* VAD heights. */

   int vnels;			        /* Number of elevation cuts. */

} A317vs_t;


/* Data Structures Instantiations. */
A317ec_t *A317ec;
A317vi_t *A317vi;
A317va_t *A317va;
A317vs_t *A317vs;
A317ve_t *A317ve;
A317vd_t *A317vd;
A318ci_t *A318ci;

/* Mapping table from RPG Elevation Index W/SAILS to RPG Elevation 
   Index assuming no sails cuts. */
#define UNDEFINED_INDEX		-1
short Remapped_rpg_elev_ind[ECUTMAX];
short Rpg_elev_ind[ECUTMAX];

/* ... Function Prototypes. */

/* The following files found in vwindpro_alg.c. */
int A317a2_vad_buffer_control();
int A317b2_calc_opt_slrng( Base_data_header *bdh );
int A317c2_vad_proc_hts( int ht );
int A317d2_vad_data_hts( char *ipr, int ht );
int A317e2_vad_init( char *ipr, int *nrstat, float *htg_ewt, int *npt_ewt, 
                     float *rms_ewt, float *hwd_ewt, float *shw_ewt );
int A317f2_vad_data( char *ipr );
int A317g2_vad_proc();
int A317h2_vad_lsf( int nradials, float *azm, float *ve, int *dnpt, float *cf1,
                    float *cf2, float *cf3 );
int A317i2_vad_rms( int nradials, float *azm, float *ve, float dhwd,  
                    float cf1, float cf2, float cf3, float *drms );
int A317j2_fit_test( int nradials, float *azm, float *ve, float cf1, 
                     float cf2, float cf3, float dhwd, float drms );
int A317k2_sym_chk( float cf1, float cf2, float cf3, float tsmy,
                    int *sym );
int A317l2_vv_div( float avg_elv, float cf1, float pfv, float dhtg,
                   float *dec_last, float *svw_last, float vad_rng,
                   float rh, float *dsvw, float *ddiv );
int A317n2_density( float htsea, float *rho, float *drho );
int A317o2_vad_alert( int *optr );
int A317p2_vad_tmhgt( char *optrv );
int A317u2_vad_data_ewt( char *ipr );
int A317v2_vad_proc_ewt( float *htg_ewt, float *rms_ewt, int *npt_ewt,
                         float *hwd_ewt, float *shw_ewt );
void A317w2_update_avset_info( Base_data_header *bdh );
int A317x2_interpolate_ewtab(void);
void A317y2_get_avset_info( int *active, int *vcp, float *elev_angle,
                            float *prev_elev_angle ); 
int A317z2_envwndtab_entries( float *htg_ewt, float *hwd_ewt, float *shw_ewt,
                              int *statl );

/* The following files defined in vwindpro_prod.c. */
int A31831_vad_prod( float *htg_ewt, float *rms_ewt, float *hwd_ewt,
                     float *shw_ewt, char *ipr, char *optr );
int A31833_vad_grid( int *bptr, short *vadbuf );
int A31834_vad_axis_lbl( int *bptr, short *vadbuf );
int A31835_vad_winds( int *bptr, short *vadbuf );
int A31836_vad_adapt_data( float *htg_ewt, float *rms_ewt, float *hwd_ewt,
                           float *shw_ewt, int offsad, int *bptr, short *vadbuf );
int A31837_vad_header( short *vadbuf, int maxspd, int maxdir, int htmaxspd, 
                       int endptr, int offsab, int lenpsd );
int A31838_vad_adapt_hdr( short *vadbuf, int bptr, int lensab );
int A31839_counlv_pkt( int *new, float xl, float yt, float xr, float yb,
                       int c, int *bptr, short *vadbuf );
int A3183a_cnvtime( int msctime, int *hmtime );
int A3183b_cochr_pkt( int tflg, float x, float y, int c, int value, 
                      int *bptr, short *vadbuf );
int A3183c_max_levels( int maxspd, int maxdir, int htmaxspd, 
                       short *vadbuf );
int A3183d_wbarb_pkt( float x, float y, float wd, float ws,
                      float rmsl, int *bptr, short *vadbuf );
int A3183e_no_data( float x, float y, int c, int *bptr, short *vadbuf );
int A3183f_vad_alpha_winds( int beg, int *end, short *vadbuf, int i4date,
                            int i4time, float *hts, float *u, float *v,
                            float *hwd, float *hws, float *rms, float *vws,
                            float *div, float *slr, int *eidx, short n,
                            int *pages );
int A3183g_vad_winds_info( float *htg_ewt, float *rms_ewt, float *hwd_ewt,
                           float *shw_ewt, float vad_slrng, int beg, int *end,
                           short *bufptr, int *extra_pages );

/* The following files defined in vwindpro_aux.c. */
void Cmplx_abs( Complex_t *z, double *c );
void Cmplx_mult( Complex_t *z1, Complex_t *z2, Complex_t *c );
void Cmplx_div( Complex_t *z1, Complex_t *z2, Complex_t *c );

/* The following files defined in vwindpro_s_alg.c. */
int A317t2_vad_proc_hts( int cvht );
int A317s2_vad_data_hts( char *ipr, int ht );
int A317r2_choose_height();
#endif
