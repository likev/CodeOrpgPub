/********************************************************************************

        file: rdasim_construct_radials.c

        this file contains all the routines necessary to construct
        radial messages.

********************************************************************************/

/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/07/31 18:08:34 $
 * $Id: rdasim_construct_radials.c,v 1.58 2014/07/31 18:08:34 steves Exp $
 * $Revision: 1.58 $
 * $State: Exp $
 */  

#include <math.h>
#include <stdio.h>       /* fprintf calls */
#include <time.h>

#include <misc.h>
#include <rda_status.h>
#include <rpg_vcp.h>
#include <orpgvcp.h>

#include <rdasim_externals.h>
#include <rdasim_simulator.h>



#define DEFAULT_VELOCITY_RESOLUTION 2 /* ICD defined 0.5 m/s Doppler velocity resolution */
#define FAT_RADIAL_SIZE           2.1 /* size of a fat radial in degrees */
#define LOCAL_VCP                   1 /* vcp data is a local vcp */
#define REMOTE_VCP                  2 /* vcp data is a remote vcp */
#define VELOCITY_DATA               1 /* specifies the data is velocity data */
#define ANGULAR_ELEV_DATA           2 /* specifies the data is an elevation angle */
#define ANGULAR_AZM_DATA            3 /* specifies the data is an azimuth angle */
#define NUMBER_LOCAL_VCPS           5 /* # local/RDA VCPs defined */
#define NUMBER_PRFS_DEFINED         8 /* # PRFs defined for PRI #3 */
#define MAX_NUMBER_LOCAL_ELEV_CUTS 16 /* max number local/RDA elevation cuts
                                         for the local VCPs defined */
#define MAX_ELEV_CUTS             100 /* max number of elevation cuts allowed
                                        (this number was arbitrarily selected) */


typedef struct {              /* data for the next VCP to execute */
   short pattern_number;                     /* VCP number */
   short number_elev_cuts;                   /* # elev cuts in this VCP */
   short velocity_resolution;                /* Doppler velocity resolution */
   short surv_prf_number [MAX_ELEV_CUTS];    /* surviellance prf numbers */
   ushort surv_prf [MAX_ELEV_CUTS];          /* surv pulse count per second */
   short dopler_prf_number [MAX_ELEV_CUTS][3]; /* Doppler prfs segment numbers */
   ushort doppler_prf [MAX_ELEV_CUTS][3];    /* Doppler pulse count per second */
   float segment_angles [MAX_ELEV_CUTS] [3]; /* clockwise leading edge segment angles */
   float azimuth_rate [MAX_ELEV_CUTS];       /* azimuth rates (deg/sec) */
   float elev_angles [MAX_ELEV_CUTS];        /* elevation angles */
   float atmos_atten [MAX_ELEV_CUTS];        /* atmospheric attenuation */
   short waveform_type [MAX_ELEV_CUTS];      /* waveform types */
   short super_res [MAX_ELEV_CUTS];          /* super resolution types */
   short dual_pol [MAX_ELEV_CUTS];           /* dual pol requested */
   short ref_snr[MAX_ELEV_CUTS];	     /* SNR threshold for reflectivity (dB/8) */
   short vel_snr[MAX_ELEV_CUTS];	     /* SNR threshold for velocity (dB/8) */
   short sw_snr[MAX_ELEV_CUTS];	             /* SNR threshold for spectrum width (dB/8) */
   short zdr_snr[MAX_ELEV_CUTS];	     /* SNR threshold for ZDR (dB/8) */
   short phi_snr[MAX_ELEV_CUTS];	     /* SNR threshold for PHI (dB/8) */
   short rho_snr[MAX_ELEV_CUTS];	     /* SNR threshold for RHO (dB/8) */
} Vcp_data_t;


extern int           Interactive_mode; /* When non zero, simulator uses user defined data regions */
extern unsigned char Surv_fixed_1;   /* Default reflectivity data value region 1 */
extern unsigned char Surv_fixed_2;   /* Default reflectivity data value region 2 */
extern int           Surv_az1;       /* Azimuth limit for reflectivity region 1  */
extern int           Surv_az2;       /* Azimuth limit for relfectivity region 2  */
extern unsigned char Velo_fixed_1;   /* Default velocity data value region 1     */
extern unsigned char Velo_fixed_2;   /* Default velocity data value region 2     */
extern int           Velo_az1;       /* Azimuth limit for velocity region 1      */
extern int           Velo_az2;       /* Azimuth limit for velocity rgion 2       */
extern int           Phi_az1;        /* Azimuth limit for PHI region 1           */
extern int           Phi_az2;        /* Azimuth limit for PHI region 2           */
extern unsigned char Zdr_fixed_1;    /* Default ZDR data value region 1          */
extern unsigned char Zdr_fixed_2;    /* Default ZDR data value region 2          */
extern int           Zdr_az1;        /* Azimuth limit for ZDR region 1           */
extern int           Zdr_az2;        /* Azimuth limit for ZDR region 2           */
extern unsigned char Rho_fixed_1;    /* Default RHO data value region 1          */
extern unsigned char Rho_fixed_2;    /* Default RHO data value region 2          */
extern int           Rho_az1;        /* Azimuth limit for RHO region 1           */
extern int           Rho_az2;        /* Azimuth limit for RHO region 2           */

static Vcp_data_t Vcp_data;
static char   Surv_bins [MAX_SR_NUM_SURV_BINS];		/* Surveillance bin array increasing data */
static unsigned char Velocity_bins [MAX_SR_NUM_VEL_BINS];  	/* Velocity bin array increasing data */
static unsigned char Aliased_velocity [MAX_SR_NUM_VEL_BINS];/* aliased velocity radial */
static unsigned char  SW_bins [MAX_SR_NUM_VEL_BINS];	/* Spectrum width bin array decreasing data */
static unsigned char  Zdr_bins [MAX_NUM_ZDR_BINS];	/* ZDR bin array increasing data */
static unsigned short Phi_bins [MAX_NUM_PHI_BINS];	/* PHI bin array linear increasing data */
static unsigned short Phi_bins_mike [MAX_NUM_PHI_BINS];/* PHI bin array with Mike Istok's equation */
static unsigned short Phi_bins_john [MAX_NUM_PHI_BINS];/* PHI bin array with John Krause's equation */
static unsigned short Phi_bins_step [MAX_NUM_PHI_BINS];/* PHI bin array with single step increase */
static unsigned char  Rho_bins [MAX_NUM_RHO_BINS];	/* RHO bin array increasing data */
static float  Radial_sample_interval;				/* radial sample interval */
static float  Surv_sample_interval;				/* surveillance sample interval, in km */
static float  Doppler_sample_interval;				/* Doppler sample interval, in km */
static int    Radials_per_elevation;				/* # radials per elevation */
static int    Max_radials_per_elevation;			/* maximum # radials per elevation */
static short  New_pattern_selected;				/* New coverage pattern selected */
static int    New_vcp_selected = FALSE;				/* flag specifying a new vcp has been
								   selected */
static float  Fixed_azimuth_rate = 0.0;				/* user defined fixed azimuth 
								   rate (deg/sec) */
static short  Current_vcp = -11;				/* currently selected VCP - 
								   default for first scan = local 11 */
static float Surv_range;					/* surveillance range in kilometers */
static float Doppler_range;					/* Doppler range in kilometers */
static int Termination_cut = 25;					/* Supports AVSET simulation. */


/* the remaining initialized definitions represent the local/RDA VCPs */

/*  VCP Index for remaining array definitions:

               VCP  11 = 0
               VCP  21 = 1
               VCP  31 = 2
               VCP  32 = 3
               VCP 300 = 4 */

static short Number_elevation_cuts [NUMBER_LOCAL_VCPS] = {16, 11, 8, 7, 4};

static float Elevation_angles [NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS] = {

/*        array index: [VCP_index, elevation_cut] 

                                             Elevation Cut
                               1    2    3      4     5      6      7     8
                               9   10   11     12    13     14     15    16   */

      /* VCP 11 */           {0.5, 0.5, 1.45,  1.45, 2.40,  3.35,  4.3,  5.25,
                              6.2, 7.5, 8.70, 10.0, 12.00, 14.00, 16.7, 19.50},

      /* VCP 21 */           {0.5,  0.5, 1.45,  1.45, 2.4,  3.35,  4.3,  6.0,
                              9.9, 14.6, 19.5,  0.00, 0.0,  0.00,  0.0,  0.0},

      /* VCP 31 */           {0.5, 0.5, 1.50,  1.50, 2.50,  2.50,  3.5,  4.5,
                              0.0, 0.0, 0.00,  0.00, 0.0,   0.00,  0.0,  0.0},

      /* VCP 32 */           {0.5, 0.5, 1.50,  1.50, 2.50,  3.50,  4.5,  0.0,
                              0.0, 0.0, 0.00,  0.00, 0.0,   0.00,  0.0,  0.0},

      /* VCP 300 */          {0.5, 0.5, 2.40,  9.90, 0.00,  0.00,  0.0,  0.0,
                              0.0, 0.0, 0.00,  0.00, 0.0,   0.00,  0.0,  0.0}};

   /* the antenna rotation rates in deg/sec for each elevation cut per VCP.  */
static float Azimuth_rate [NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS] = {   

/*        array index: [VCP_index, elevation_cut] 

                                      Elevation Cut
                         1       2       3       4       5       6
                         7       8       9      10      11      12
                        13      14      15      16                    */

      /* VCP 11 */
                     {18.675, 19.224, 19.844, 19.225, 16.116, 17.893,
                      17.898, 17.459, 17.466, 25.168, 25.398, 25.421,
                      25.464, 25.515, 25.596, 25.696}, 

      /* VCP 21 */
                     {11.339, 11.360, 11.339, 11.360, 11.180, 11.182,
                      11.185, 11.189, 14.260, 14.322, 14.415,  0.000,
                       0.000,  0.000,  0.000,  0.000},

       /* VCP 31 */
                      {5.039,  5.061,  5.040,  5.062,  5.041,  5.062,
                       5.063,  5.065,  0.000,  0.000,  0.000,  0.000,
                       0.000,  0.000,  0.000,  0.000},

      /* VCP 32 */
                      {4.961,  4.544,  4.961,  4.544,  4.060,  4.061,
                       4.063,  0.000,  0.000,  0.000,  0.000,  0.000,
                       0.000,  0.000,  0.000,  0.000},

      /* VCP 300 */
                      {18.675, 19.224, 19.844,  19.225,  0.000,  0.000,
                        4.063,  0.000,  0.000,   0.000,  0.000,  0.000,
                        0.000,  0.000,  0.000,   0.000}};

   /* wave form type for each elevation cut */
static unsigned short WF_type[NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS] = {   

/*        array index: [VCP_index, elevation_cut] 

                                              ICD values for WF type:
                                              1 = CS (Continuos Surveillance)
                                              2 = CD (Continuos Doppler) with
                                                  Ambiguity Resolution
                                              3 = CD (Continuos Doppler) without
                                                  Ambiguity Resolution
                                              4 = B  (Batch)   */

      /* VCP 11 */           {1, 2, 1, 2, 4, 4, 4, 4,
                              4, 3, 3, 3, 3, 3, 3, 3},
   
      /* VCP 21 */           {1, 2, 1, 2, 4, 4, 4, 4,
                              3, 3, 3, 0, 0, 0, 0, 0},

      /* VCP 31 */           {1, 2, 1, 2, 1, 3, 3, 3, 
                              0, 0, 0, 0, 0, 0, 0, 0},
     
      /* VCP 32 */           {1, 2, 1, 2, 4, 4, 4, 0,
                              0, 0, 0, 0, 0, 0, 0, 0},
 
      /* VCP 300 */          {1, 4, 4, 4, 0, 0, 0, 0, 
                              0, 0, 0, 0, 0, 0, 0, 0}};

   /* surveillance prfs for each elev cut */
static unsigned short Surv_prf[NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS] = {

/*        array index: [VCP_index, elevation_cut] */

      /* VCP 11 */           {1, 0, 1, 0, 1, 2, 2, 3,
                              3, 6, 7, 7, 7, 7, 7, 7},
   
      /* VCP 21 */           {1, 0, 1, 0, 2, 2, 2, 3,
                              7, 7, 7, 0, 0, 0, 0, 0},

      /* VCP 31 */           {1, 0, 1, 0, 1, 2, 2, 2, 
                              0, 0, 0, 0, 0, 0, 0, 0},
     
      /* VCP 32 */           {1, 0, 1, 0, 2, 2, 2, 0,
                              0, 0, 0, 0, 0, 0, 0, 0},
 
      /* VCP 300 */          {1, 0, 5, 5, 0, 0, 0, 0, 
                              0, 0, 0, 0, 0, 0, 0, 0}};


   /* Doppler prfs for each elev cut per sector */
static unsigned short Dop_prf[NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS][3] = {   

/*        array index: [vcp_index][elevation_cut][sector number] */

      /* VCP 11 */          {{0,   0,   0},
                             {5,   5,   5},
                             {0,   0,   0},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {6,   6,   6},
                             {7,   7,   7},
                             {7,   7,   7},
                             {7,   7,   7},
                             {7,   7,   7},
                             {7,   7,   7},
                             {7,   7,   7}},

      /* VCP 21 */          {{0,   0,   0},
                             {5,   5,   5},
                             {0,   0,   0},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {7,   7,   7},
                             {7,   7,   7},
                             {7,   7,   7},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0}},

      /* VCP 31 */          {{0,   0,   0},
                             {2,   2,   2},
                             {0,   0,   0},
                             {2,   2,   2},
                             {0,   0,   0},
                             {2,   2,   2},
                             {2,   2,   2},
                             {2,   2,   2},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0}},

      /* VCP 32 */          {{0,   0,   0},
                             {5,   5,   5},
                             {0,   0,   0},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {5,   5,   5},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0}},

      /* VCP 300 */         {{0,   0,   0},
                             {2,   2,   2},
                             {5,   5,   5},
                             {5,   5,   5},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0},
                             {0,   0,   0}}};

static float Sector_angles [NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS][3] = {

/*        array index: [vcp_index][elevation_cut][sector number] */

      /* VCP 11 */   {{ 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0}},
     
      /* VCP 21 */   {{ 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0}},
     
      /* VCP 31 */   {{ 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0}},
     
      /* VCP 32 */   {{ 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0}},
     
      /* VCP 300 */  {{ 0.0,     0.0,     0.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      {30.0,   210.0,   335.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0},
                      { 0.0,     0.0,     0.0}}};

      /* pulse count per radial for local VCPs */
   short Surv_pulse_count [NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS] = {
         /* VCP 11 */    { 17,  0, 16,  0,  6,  6,  6, 10,
                           10,  0,  0,  0,  0,  0,  0,  0},

         /* VCP 21 */    { 28,  0, 28,  0,  8,  8,  8, 12, 
                            0,  0,  0,  0,  0,  0,  0,  0},

         /* VCP 31 */    { 63,  0, 63,  0, 63,  0,  0,  0,
                            0,  0,  0,  0,  0,  0,  0,  0},

         /* VCP 32 */    { 64,  0, 64,  0, 11, 11, 11, 0,
                            0,  0, 0,   0,  0,  0,  0, 0},

         /* VCP 300 */   { 16,  0,  0,  0,  0,  0,  0,  0,
                            0,  0,  0,  0,  0,  0,  0,  0}};

      /* pulse count per radial for local VCPs */
   short Dopler_pulse_count [NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS] = {
         /* VCP 11 */    {  0, 52,  0, 52, 41, 41, 41, 41,
                           41, 43, 46, 46, 46, 46, 46, 46},

         /* VCP 21 */    {  0, 88,  0, 88, 70, 70, 70, 70, 
                           82, 82, 82,  0,  0,  0,  0,  0},

         /* VCP 31 */    {  0, 87,  0, 87,  0, 87, 87, 87,
                            0,  0,  0,  0,  0,  0,  0,  0},

         /* VCP 32 */    {  0, 220, 0, 220, 220, 220, 220, 0,
                            0,   0, 0,   0,   0,   0,   0, 0},

         /* VCP 300 */   {  0, 64, 64, 64,  0,  0,  0,  0,
                            0,  0,  0,  0,  0,  0,  0,  0}};

   /* local functions */

static short  Determine_sector_number (float azimuth_angle, ushort elev_index);
static void   Compute_time (Radial_message_t *radial_msg);
static float  Convert_data_in (short data_to_convert, int data_type);
static ushort Convert_data_out (float data_to_convert, int data_type);
static void   Generate_aliased_velocities (float Va);
static int    Get_vcp_index (int vcp_selected);
       void   Initialize_moments (Radial_message_t *radial);
static void   Update_dynamic_radial_data (Radial_message_t *radial_msg, 
                     float elevation_angle, float azimuth_angle, unsigned short elev_cut);
static void   Update_rda_vcp_msg (int vcp_data_source);


/********************************************************************************

     Description: this routine returns the current vcp

           Input:

          Output:

          Return:

 ********************************************************************************/

short CR_get_current_vcp ()
{
   return (Current_vcp);
}


/********************************************************************************

     Description: this routine initializes the VCP data buffer for the first 
                  VCP to execute. The first VCP is always a local VCP.
                  The static radial data is also initialized here

           Input:

          Output:

          Return:

 ********************************************************************************/

void CR_initialize_first_vcp ()
{
   int   i,j;
   float temp;
   int   vcp_index;
   Radial_message_t *radial_msg;

   Surv_range = 0.5;                /* surveillance range in kilometers */
   Doppler_range = 0.125;           /* Doppler range in kilometers */
   
   Surv_sample_interval = 1.0;    
   Doppler_sample_interval = 0.25;

      /* retrieve radial msg pointer and initialize the moments */

   if ((radial_msg = RRM_get_radial_msg ()) == NULL) {
      fprintf (stderr, "Error retieving Radial Message Pointer\n");
      MA_terminate ();
   }

   Initialize_moments (radial_msg);

   temp = 360.0 / Radial_sample_interval;
   Radials_per_elevation = (int) (360.0 / Radial_sample_interval);
   if( Radial_sample_interval == HALF_DEG_RADIALS ){

      Max_radials_per_elevation = 800;
      radial_msg->hdr.base.azimuth_res = 1;
      radial_msg->hdr.base.azimuth_index = 25;
  
   }
   else{

      Max_radials_per_elevation = 400;
      radial_msg->hdr.base.azimuth_res = 2;
      radial_msg->hdr.base.azimuth_index = 50;

   }

   if ((temp - (float) Radials_per_elevation) != 0.0)
      ++Radials_per_elevation;


      /* initialize the static radial data in ICD compliant format */

   memcpy( radial_msg->hdr.base.radar_id, "KSIM", 4 );

   memcpy( radial_msg->vol_hdr.type, "RVOL", 4 );
   radial_msg->vol_hdr.len = sizeof(Generic_vol_t);
   radial_msg->vol_hdr.lat = 35.236f;
   radial_msg->vol_hdr.lon = -97.462f;
   radial_msg->vol_hdr.major_version = 2;
   radial_msg->vol_hdr.minor_version = 0;
   radial_msg->vol_hdr.height = 1255;
   radial_msg->vol_hdr.feedhorn_height = 1255;
   radial_msg->vol_hdr.calib_const = -54.0f;
   radial_msg->vol_hdr.horiz_shv_tx_power = 750.0f;
   radial_msg->vol_hdr.vert_shv_tx_power = 0.0f;
   radial_msg->vol_hdr.sys_diff_refl = 0.0f;
   radial_msg->vol_hdr.sys_diff_phase = 0.0f;
   radial_msg->vol_hdr.sig_proc_states = 0;

   memcpy( radial_msg->elv_hdr.type, "RELV", 4 );
   radial_msg->elv_hdr.len = sizeof(Generic_elev_t);
   radial_msg->elv_hdr.calib_const = -54.0f;

   memcpy( radial_msg->rad_hdr.type, "RRAD", 4 );
   radial_msg->rad_hdr.len = sizeof(Generic_rad_t) + sizeof(Generic_rad_dBZ0_t);
   radial_msg->rad_hdr.horiz_noise = -80.0f;
   radial_msg->rad_hdr.vert_noise = -80.0f;
   radial_msg->dBZ0_hdr.h_dBZ0 = -45.0f;
   radial_msg->dBZ0_hdr.v_dBZ0 = -45.0f;

   memcpy( radial_msg->ref_hdr.name, "DREF", 4 );
   radial_msg->ref_hdr.data_word_size = 8;
   radial_msg->ref_hdr.scale = 2.0;
   radial_msg->ref_hdr.offset = 66.0;
   radial_msg->ref_hdr.tover = 5.0;
   radial_msg->ref_hdr.first_gate_range = Surv_range * 1000;
   radial_msg->ref_hdr.bin_size = Surv_sample_interval * 1000;

   memcpy( radial_msg->vel_hdr.name, "DVEL", 4 );
   radial_msg->vel_hdr.data_word_size = 8;
   radial_msg->vel_hdr.scale = 2.0;
   radial_msg->vel_hdr.offset = 129.0;
   radial_msg->vel_hdr.tover = 5.0;
   radial_msg->vel_hdr.first_gate_range = Doppler_range * 1000;
   radial_msg->vel_hdr.bin_size = Doppler_sample_interval * 1000;

   memcpy( radial_msg->wid_hdr.name, "DSW ", 4 );
   radial_msg->wid_hdr.data_word_size = 8;
   radial_msg->wid_hdr.scale = 2.0;
   radial_msg->wid_hdr.offset = 129.0;
   radial_msg->wid_hdr.tover = 5.0;
   radial_msg->wid_hdr.first_gate_range = Doppler_range * 1000;
   radial_msg->wid_hdr.bin_size = Doppler_sample_interval * 1000;

   memcpy( radial_msg->zdr_hdr.name, "DZDR", 4 );
   radial_msg->zdr_hdr.data_word_size = 8;
   radial_msg->zdr_hdr.scale = 16.0;
   radial_msg->zdr_hdr.offset = 128.0;
   radial_msg->zdr_hdr.first_gate_range = Surv_range * 1000;
   radial_msg->zdr_hdr.bin_size = 250;
   radial_msg->zdr_hdr.tover = 5.0;

   memcpy( radial_msg->phi_hdr.name, "DPHI", 4 );
   radial_msg->phi_hdr.data_word_size = 16;
   radial_msg->phi_hdr.scale = 2.8361;
   radial_msg->phi_hdr.offset = 2.0;
   radial_msg->phi_hdr.first_gate_range = Surv_range * 1000;
   radial_msg->phi_hdr.bin_size = 250;
   radial_msg->phi_hdr.tover = 5.0;

   memcpy( radial_msg->rho_hdr.name, "DRHO", 4 );
   radial_msg->rho_hdr.data_word_size = 8;
   radial_msg->rho_hdr.scale = 300.0;
   radial_msg->rho_hdr.offset = -60.5;
   radial_msg->rho_hdr.first_gate_range = Surv_range * 1000;
   radial_msg->rho_hdr.bin_size = 250;
   radial_msg->rho_hdr.tover = 5.0;

      /* use VCP 21 as the default local VCP */
   
   Vcp_data.pattern_number = Current_vcp;
 
   Vcp_data.velocity_resolution = DEFAULT_VELOCITY_RESOLUTION;

   vcp_index = Get_vcp_index (Current_vcp);
   Vcp_data.number_elev_cuts = Number_elevation_cuts [vcp_index];

   for (i = 0; i < Number_elevation_cuts [vcp_index]; i++) {
 
      Vcp_data.azimuth_rate[i] = Azimuth_rate[vcp_index][i];
      Vcp_data.elev_angles[i] = Elevation_angles[vcp_index][i];
      Vcp_data.waveform_type[i] = WF_type[vcp_index][i];
      Vcp_data.super_res[i] = 0;

      if (Vcp_data.elev_angles[i] <= 20.0)
         Vcp_data.atmos_atten[i] = (0.00035 * Elevation_angles[vcp_index][i]) - 0.012;
      else
         Vcp_data.atmos_atten [i] = -0.003;

      Vcp_data.surv_prf_number[i] = Surv_prf[vcp_index][i];
      Vcp_data.surv_prf[i] = Surv_pulse_count [vcp_index][i] *
                          Vcp_data.azimuth_rate [i] / Radial_sample_interval;

      for (j = 0; j <= 2; j++) {
         Vcp_data.dopler_prf_number[i][j] = Dop_prf [vcp_index][i][j];
         Vcp_data.segment_angles[i][j] = Sector_angles [vcp_index][i][j];

             /* compute the Doppler prf per second for each sector */

         if (Dop_prf[vcp_index][i][j] != 0)
            Vcp_data.doppler_prf [i][j] = Dopler_pulse_count [vcp_index][i] * 
                                          Vcp_data.azimuth_rate [i] / Radial_sample_interval;
         else
            Vcp_data.doppler_prf [i][j] = 0;
      }

   }

   return;
}


/********************************************************************************

    Description: this routine initializes the vcp buffer for the next volume scan

   Input/Output: Vcp_data - structure that contians this volume scan's data

         Return:

 ********************************************************************************/


void CR_initialize_this_vol_scan_data ()
{
   int   i, j;
/*   float pulses_per_second [NUMBER_PRFS_DEFINED] = {    default PRF - Pulses per second 
                     PRF #                1          2          3          4
                                          5          6          7          8           

                                     321.888,   446.428,   643.777,   857.143,
                                    1013.151,  1094.890,  1181.000,  1282.050}; 
*/
                                    

      /* if new pattern # = 0, then run the remote VCP received from 
         the RPG */

   if (New_pattern_selected == 0) {
      char *data_ptr; 
      VCP_message_header_t *vcp_msg_hdr;
      VCP_elevation_cut_header_t *vcp_elev_hdr;

      data_ptr = (RRM_get_remote_vcp () + sizeof (RDA_RPG_message_header_t));
      vcp_msg_hdr = (VCP_message_header_t *) data_ptr;

      Current_vcp = vcp_msg_hdr->pattern_number; 
      Vcp_data.pattern_number = Current_vcp;
      RDA_status_msg.vcp_num = Current_vcp;

      vcp_elev_hdr = (VCP_elevation_cut_header_t *) (data_ptr + sizeof (VCP_message_header_t));

      Vcp_data.number_elev_cuts = vcp_elev_hdr->number_cuts;

      Vcp_data.velocity_resolution = vcp_elev_hdr->doppler_res;
      
      if (Vcp_data.number_elev_cuts > MAX_ELEV_CUTS) {
            /* do something...this isn't pretty but it keeps the simulator from crashing */
          fprintf (stderr, 
                   "\nExcessive # elevation cuts read from the RPG VCP msg (# read: %d)\n",
                   vcp_elev_hdr->number_cuts);
          Vcp_data.number_elev_cuts = MAX_ELEV_CUTS;
       }

       for (i = 0; i < Vcp_data.number_elev_cuts; i++) {

          Vcp_data.azimuth_rate [i] = 
                Convert_data_in (vcp_elev_hdr->data[i].azimuth_rate, VELOCITY_DATA);
          Vcp_data.elev_angles [i] = 
                Convert_data_in (vcp_elev_hdr->data[i].angle, ANGULAR_ELEV_DATA);
          Vcp_data.waveform_type[i] = vcp_elev_hdr->data[i].waveform;
          if( RDA_status_msg.super_res != SRS_ENABLED )
             vcp_elev_hdr->data[i].super_res &= (~VCP_HALFDEG_RAD & 0xffff); 

          Vcp_data.super_res [i] = (vcp_elev_hdr->data[i].super_res & VCP_SUPER_RES_MASK);

          if( vcp_elev_hdr->data[i].super_res & VCP_DUAL_POL_ENABLED )
             Vcp_data.dual_pol [i] = 1;
          else
             Vcp_data.dual_pol [i] = 0;

          Vcp_data.segment_angles [i][0] = 
               Convert_data_in (vcp_elev_hdr->data[i].edge_angle1, ANGULAR_AZM_DATA);
          Vcp_data.segment_angles [i][1] = 
               Convert_data_in (vcp_elev_hdr->data[i].edge_angle2, ANGULAR_AZM_DATA);
          Vcp_data.segment_angles [i][2] = 
               Convert_data_in (vcp_elev_hdr->data[i].edge_angle3, ANGULAR_AZM_DATA);

          Vcp_data.surv_prf_number [i] = vcp_elev_hdr->data[i].surv_prf_num;
          Vcp_data.surv_prf [i] = vcp_elev_hdr->data[i].surv_prf_pulse *
                                  Vcp_data.azimuth_rate [i] / Radial_sample_interval;

          Vcp_data.dopler_prf_number [i][0] = vcp_elev_hdr->data[i].dopp_prf_num1;
          Vcp_data.dopler_prf_number [i][1] = vcp_elev_hdr->data[i].dopp_prf_num2;
          Vcp_data.dopler_prf_number [i][2] = vcp_elev_hdr->data[i].dopp_prf_num3;

             /* compute the Doppler_prf per second for each sector.
                (convert the pulse count per radial to pulse count per second) */

          if (Vcp_data.waveform_type [i] & 0x000e) {
             Vcp_data.doppler_prf [i][0] = vcp_elev_hdr->data[i].dopp_prf_pulse1 * 
                                           Vcp_data.azimuth_rate [i] / Radial_sample_interval;
             Vcp_data.doppler_prf [i][1] = vcp_elev_hdr->data[i].dopp_prf_pulse2 * 
                                           Vcp_data.azimuth_rate [i] / Radial_sample_interval;
             Vcp_data.doppler_prf [i][2] = vcp_elev_hdr->data[i].dopp_prf_pulse3 * 
                                           Vcp_data.azimuth_rate [i] / Radial_sample_interval;
          } else {
             Vcp_data.doppler_prf [i][0] = 0;
             Vcp_data.doppler_prf [i][1] = 0;
             Vcp_data.doppler_prf [i][2] = 0;
          }

             /* define an atmospheric attenuation */

          if (Vcp_data.elev_angles [i] <= 20.0)
             Vcp_data.atmos_atten [i] = (0.00035 * Vcp_data.elev_angles[i]) - 0.012;
          else
             Vcp_data.atmos_atten [i] = -0.003;

             /* get the SNR ratios */
          Vcp_data.ref_snr [i] = vcp_elev_hdr->data[i].refl_thresh;
          Vcp_data.vel_snr [i] = vcp_elev_hdr->data[i].vel_thresh;
          Vcp_data.sw_snr [i] = vcp_elev_hdr->data[i].sw_thresh;
          Vcp_data.zdr_snr [i] = vcp_elev_hdr->data[i].diff_refl_thresh;
          Vcp_data.phi_snr [i] = vcp_elev_hdr->data[i].diff_phase_thresh;
          Vcp_data.rho_snr [i] = vcp_elev_hdr->data[i].corr_coeff_thresh;

       }

          /* populate the RDA VCP msg to be sent as part of the metadata */

           Update_rda_vcp_msg (REMOTE_VCP);

   }else {  /* a local/RDA VCP has been selected */

      int vcp_index;

         /* get the array index for the selected VCP. if the VCP is invalid,
            then just return and use the VCP executed last volume scan */

      if ((vcp_index = Get_vcp_index (New_pattern_selected)) == -1) {
          New_vcp_selected = FALSE;
          return;
      }

      Current_vcp = -New_pattern_selected;
      RDA_status_msg.vcp_num = Current_vcp;
      Vcp_data.pattern_number = Current_vcp;

      Vcp_data.velocity_resolution = DEFAULT_VELOCITY_RESOLUTION;

      Vcp_data.number_elev_cuts = Number_elevation_cuts [vcp_index];
      
      for (i = 0; i < Number_elevation_cuts [vcp_index]; i++) {
         Vcp_data.azimuth_rate [i] = Azimuth_rate[vcp_index][i];
         Vcp_data.elev_angles [i] = Elevation_angles[vcp_index][i];

         Vcp_data.surv_prf_number [i] = Surv_prf[vcp_index][i];
         Vcp_data.surv_prf [i] = Surv_pulse_count [vcp_index][i] *
                                 Vcp_data.azimuth_rate [i] / Radial_sample_interval;

             /* define an atmospheric attenuation */

         if (Elevation_angles [vcp_index][i] <= 20.0)
            Vcp_data.atmos_atten [i] = (0.00035 * Elevation_angles [vcp_index][i]) - 0.012;
         else
            Vcp_data.atmos_atten [i] = -0.003;

         Vcp_data.waveform_type [i] = WF_type[vcp_index][i];

         for (j = 0; j <= 2; j++) {
            Vcp_data.segment_angles [i][j] = Sector_angles[vcp_index][i][j];
            Vcp_data.dopler_prf_number [i][j] = Dop_prf[vcp_index][i][j];

                /* compute the Doppler_prf per second for each sector - use the same prf for all
                   sectors */

            if (Dop_prf[vcp_index][i][j] != 0)
               Vcp_data.doppler_prf [i][j] = Dopler_pulse_count [vcp_index][i] * 
                                             Vcp_data.azimuth_rate [i] / Radial_sample_interval;
            else
               Vcp_data.doppler_prf [i][j] = 0;

         }

         if( RDA_status_msg.super_res != SRS_ENABLED )
             Vcp_data.super_res[i] &= (~VCP_HALFDEG_RAD & 0xffff); 

          Vcp_data.super_res[i] &= VCP_SUPER_RES_MASK;

          if( Vcp_data.super_res[i] & VCP_DUAL_POL_ENABLED )
             Vcp_data.dual_pol [i] = 1;
          else
             Vcp_data.dual_pol [i] = 0;

      }

      /* populate the RDA VCP msg to be sent as part of the metadata */

      Update_rda_vcp_msg (LOCAL_VCP);

   }

   if (VErbose_mode >= 3) {
      char l_r_vcp;

         /* determine whether this is a local or remote VCP */

      strncpy (&l_r_vcp, (Vcp_data.pattern_number < 0 ? "L" : "R"), 1);

      fprintf (stdout, "Init next Vol Scan:\n");
      fprintf (stdout, "   VCP Selected: %c%d\n", l_r_vcp, abs(Vcp_data.pattern_number));
      fprintf (stdout, "   Number Elev Cuts: %d\n", Vcp_data.number_elev_cuts);

      for (i = 0; i < Vcp_data.number_elev_cuts; i++) {
         fprintf (stdout, "   Elev Cut: %3d   Elev Angle: %10.7f   Azimuth Rate: %10.7f\n",
                  i + 1, Vcp_data.elev_angles [i], Vcp_data.azimuth_rate[i]);
      }
   }

   if(BAD_start_vcp == TRUE){
      Current_vcp = 2;
      RDA_status_msg.vcp_num = (short) -Current_vcp;
      Vcp_data.pattern_number = Current_vcp;
   }

   New_vcp_selected = FALSE;

   RD_set_send_status_msg_flag ();   
   RD_set_send_rda_vcp_flag ();   

   return;
}


/********************************************************************************

    Description: This routine sets the "new vcp selected" flag received from the
                 RPG for the next volume scan

          Input: The new vcp number selected

         Output:
    
         Return:

 ********************************************************************************/

void CR_set_termination_cut (int termination_cut )
{
   Termination_cut = termination_cut;
}


/********************************************************************************

    Description: This routine sets the "new vcp selected" flag received from the
                 RPG for the next volume scan

          Input: The new vcp number selected

         Output:
    
         Return:

 ********************************************************************************/

void CR_new_vcp_selected (int new_pattern_number)
{
   New_vcp_selected = TRUE;
   if( new_pattern_number != INVALID_PATTERN_NUMBER )
      New_pattern_selected = new_pattern_number;

}


/********************************************************************************
  
    Description: This subroutine processes a radial of data.

         In/Out: *current_state - the current RDA processing state

         Output: *radial_msg - radial data message to send to the RPG

********************************************************************************/


#define ELEVATION_RATE             3.25  /* antenna elevation rate in deg/sec */
#define ANTENNA_RETRACE_LOCK_DELAY 1.25  /* time (in secs) it takes antenna to stabilize 
                                            and lock onto elevation after a retrace */
#define SPOT_BLANKING_IS_DISABLED     0  /* ICD defined value */
#define VOL_SCAN_HAS_SPOT_BLANKING    4  /*  "     "      "   */
#define ELEV_CUT_HAS_SPOT_BLANKING    6  /*  "     "      "   */
#define RADIAL_IS_SPOT_BLANKED        7  /*  "     "      "   */

                                          
void CR_process_radial (int *current_state)
{
   static short radial_number;             /* radial being processed */
   static unsigned short elevation_cut;    /* the elevation cut being processed */
   static float  elevation_angle;          /* the elevation angle */
   static float seconds_per_radial;        /* # seconds to process a radial */
   static int radial_delta_time = 0;       /* the delta time between radials */
   static int elapsed_time;                /* the total elapsed time since 
                                              the last radial */
   static int time_remaining;              /* time remaining to process next radial */
   static struct timeval start_time = {0, 0};/* the time this radial began 
                                                processing */
   static struct timeval current_time;     /* the current time */
   static float radial_angle;              /* the azimuth of this radial */
   static short first_pass = TRUE;         /* first pass flag */
   static short sig_proc_states = 0;       /* Signal Processor States variable. */
   float abs_delta;
   float  rotation_rate;                   /* the assigned antenna rotation rate */
   float  temp;			
   int radial_angle_i;
   Radial_message_t *radial_msg;           /* ptr to the ICD radial msg to construct */

       /* initialize the radial angle to some arbitrary value. attempt to 
          select a randomly unique starting angle without using the random 
          number generator (the generator has to be primed which would cause the 
          starting angle to be the same angle everytime the tool is started). */

   if (first_pass == TRUE) {
      gettimeofday (&current_time, NULL);
      gettimeofday (&start_time, NULL);
      radial_angle = (float) current_time.tv_usec;  /* use usec tick to init angle */
      radial_angle /= 1000.0;
      while (radial_angle > 360.0) {   
         radial_angle /= 2.0;
      }
      first_pass = FALSE;

   }

   radial_msg = RRM_get_radial_msg ();

   gettimeofday (&current_time, NULL);

   if (*current_state == RDASIM_START_OF_VOLUME_SCAN) {
       *current_state = START_OF_ELEVATION;  /* change processing states */

      if (SKIp_start_of_vol) { /* check for exception */
         radial_msg->hdr.base.status = *current_state;
         SKIp_start_of_vol = FALSE;
      }else
         radial_msg->hdr.base.status = RDASIM_START_OF_VOLUME_SCAN;

      elevation_cut = ZERO;

         /* if the RPG commanded a new vcp, then initialize the vcp data for
            this volume scan */

      if (New_vcp_selected == TRUE)
          CR_initialize_this_vol_scan_data ();

      if( sig_proc_states == 0 )
         sig_proc_states = 0x1;
      else if( sig_proc_states == 1 )
         sig_proc_states = 0x2;
      else if( sig_proc_states == 2 )
         sig_proc_states = 0x3;
      else
         sig_proc_states = 0;


   }

      /* find the time that has elapsed since the start of processing 
         the last radial - convert elapsed time to msec */

   elapsed_time = ((current_time.tv_sec - start_time.tv_sec) 
                  * MILLISECONDS_PER_SECOND)
                  + ((current_time.tv_usec - start_time.tv_usec)
                  / MILLISECONDS_PER_SECOND);

   time_remaining = radial_delta_time - elapsed_time;

       /* if there's time left before processing the next radial, then
          suspend.
           NOTE: If we are late, then just process the next radial; do not try
                 to make up the lost time for now (may change this later) */

   if (time_remaining > 0) {
      if (time_remaining > 1000) { /* if delta time > 1 sec, suspend for 1 sec & return */
         msleep (1000);
         return;
      }
      else
         msleep (time_remaining);
   }

      /* update the elevation cut and RDA status field */
    
   if (*current_state == START_OF_ELEVATION) {
      if (REstart_elevation_cut > ZERO) {
         elevation_cut = REstart_elevation_cut;
         REstart_elevation_cut = ZERO;
         radial_msg->hdr.base.status = START_OF_ELEVATION;
      } 
      else if (elevation_cut != ZERO) {  
         elevation_cut++;
         radial_msg->hdr.base.status = START_OF_ELEVATION;
         if( elevation_cut == Termination_cut ){
            radial_msg->hdr.base.status = START_OF_LAST_ELEVATION;
         }
      } 
      else
         elevation_cut++;

      if (BAD_elevation_cut == TRUE    &&      /* check for an exception condition */
         (elevation_cut + 1) <= Vcp_data.number_elev_cuts) { 
          elevation_cut++;
          BAD_elevation_cut = FALSE;
      }

         /* get the elevation angle for this cut */

      elevation_angle = Vcp_data.elev_angles[elevation_cut - ONE];

         /* get the azimuth rate for this elevation cut then compute the
            radial start angle and radial delta time */

      if (Fixed_azimuth_rate == 0.0)
          rotation_rate = Vcp_data.azimuth_rate[elevation_cut - ONE];
      else
          rotation_rate = Fixed_azimuth_rate;

      radial_angle += radial_delta_time * SECONDS_PER_MSEC * rotation_rate;

         /* Support exception handling. */

      if( NEG_start_angle ){

         NEG_start_angle = 0;
         radial_angle = -20.0;

      }

         /* Update the Radial Sample Interval if super resolution. */

      if( Vcp_data.super_res[elevation_cut - ONE] & VCP_HALFDEG_RAD )
         Radial_sample_interval = HALF_DEG_RADIALS;

      else
         Radial_sample_interval = ONE_DEG_RADIALS;

         /* Update the number of radials per elevation based on the new
            radial sample interval. */

      temp = 360.0 / Radial_sample_interval;
      Radials_per_elevation = (int) (360.0 / Radial_sample_interval);

      if ((temp - (float) Radials_per_elevation) != 0.0)
         ++Radials_per_elevation;

      seconds_per_radial = Radial_sample_interval / rotation_rate;
      radial_delta_time = seconds_per_radial * MILLISECONDS_PER_SECOND; 
      radial_number = ONE;

         /* Have the angles start on the 1/2 deg.  For super res, have the angles 
            start on the 1/4 deg. */
      radial_angle_i = radial_angle;

      if( Radial_sample_interval == HALF_DEG_RADIALS )
         radial_angle = (float) radial_angle_i + 0.25;

      else
         radial_angle = (float) radial_angle_i + 0.5;

         /* Update the Surveillance Sample Interval and Surveillance Range 
            if super resolution. */

      if( Vcp_data.super_res[elevation_cut - ONE] & VCP_QRTKM_SURV ){

         Surv_range = 0.125;
         Surv_sample_interval = 0.25;

      }
      else{

         Surv_range = 0.5;
         Surv_sample_interval = 1.0;

      }

   } else {  /* update the intermediate radial info */
      if (BAD_elevation_cut == TRUE    &&      /* check for an exception condition */
         (elevation_cut + 1) <= Vcp_data.number_elev_cuts) { 
          elevation_cut++;
          BAD_elevation_cut = FALSE;
      }
      if (FAT_forward) {
         radial_angle += FAT_RADIAL_SIZE;
         FAT_forward = FALSE;
      }
      else if (FAT_backward)
      {
         radial_angle -= FAT_RADIAL_SIZE;
         FAT_backward = FALSE;
      }
      else
         radial_angle += Radial_sample_interval;

      radial_number++;
   }

   if (radial_angle >= 360.0)
      radial_angle -= 360.0;  /* limit the angle to 0 - 359.9... degrees */

      /* populate the bins for this radial */

   Update_dynamic_radial_data (radial_msg, elevation_angle, radial_angle, elevation_cut );

       /* find start time for this radial */

   gettimeofday (&start_time, NULL);

       /* update remaining radial data */

   Compute_time (radial_msg);

   radial_msg->hdr.base.azimuth = radial_angle;
   if( radial_angle < 0.0 )
      radial_angle = 20.0;
   radial_msg->hdr.base.azi_num = radial_number;
   radial_msg->hdr.base.elevation = elevation_angle;
   radial_msg->hdr.base.elev_num = elevation_cut;
   radial_msg->vol_hdr.vcp_num = (short) abs(Vcp_data.pattern_number);
   radial_msg->vol_hdr.sig_proc_states = sig_proc_states;

   if( Radial_sample_interval == HALF_DEG_RADIALS ){

      radial_msg->hdr.base.azimuth_res = 1;
      radial_msg->hdr.base.azimuth_index = 25;

   }
   else{

      radial_msg->hdr.base.azimuth_res = 2;
      radial_msg->hdr.base.azimuth_index = 50;

   }

   if( Vcp_data.velocity_resolution == 2 )
      radial_msg->vel_hdr.scale = 2.0;
   else
      radial_msg->vel_hdr.scale = 1.0;

   if( Vcp_data.super_res[elevation_cut] & VCP_DUAL_POL_ENABLED )
      radial_msg->vol_hdr.vert_shv_tx_power = 750.0f;
   else
      radial_msg->vol_hdr.vert_shv_tx_power = 0.0f;

   /* Set the vertical noise level. */

   if( Vcp_data.super_res[elevation_cut] & VCP_DUAL_POL_ENABLED )
      radial_msg->rad_hdr.vert_noise = -60.0f;
   else
      radial_msg->rad_hdr.vert_noise = 0.0f;


   radial_msg->ref_hdr.bin_size = Surv_sample_interval * 1000;
   radial_msg->vel_hdr.bin_size = radial_msg->wid_hdr.bin_size = 
                                  Doppler_sample_interval * 1000;
   radial_msg->ref_hdr.first_gate_range = Surv_range * 1000;
   radial_msg->vel_hdr.first_gate_range = radial_msg->wid_hdr.first_gate_range = 
                                          Doppler_range * 1000;

      /* see if we're at the end of an elevation cut or volume scan */
   
   if (radial_number >= Radials_per_elevation) {
      if (MAX_radial_exceeded == FALSE) {  /* see if MAX_radial exception has been sent */
         if ( (elevation_cut == Vcp_data.number_elev_cuts) 
                             ||
              (elevation_cut == Termination_cut ) ){
            radial_msg->hdr.base.status = END_OF_VOLUME_SCAN;
            *current_state = END_OF_VOLUME_SCAN;

               /* compute time delay to reposition antenna to the first elev angle
                  of the next volume scan */

            radial_delta_time = ((elevation_angle / ELEVATION_RATE) +
                                  ANTENNA_RETRACE_LOCK_DELAY) * MILLISECONDS_PER_SECOND;

               /* if the RPG commanded a new vcp, then initialize the vcp data for
                  the next volume scan */
/*            if (New_vcp_selected == TRUE) {
               CR_initialize_next_vol_scan_data ();
               New_vcp_selected = FALSE;
            } */
         }else {
            radial_msg->hdr.base.status = END_OF_ELEVATION;
            *current_state = END_OF_ELEVATION;

               /* compute delta time to reposition antenna to next elev angle.
                  const 1.2 is used to ensure positive antenna angle rotation
                  from the end of one elev cut to the beginning of the next 
                  when delta elev angles are ~ 0 degs */

            abs_delta = fabs(Vcp_data.elev_angles [elevation_cut] - 
                             Vcp_data.elev_angles [elevation_cut - 1]);  
            radial_delta_time = ((abs_delta + 1.2) / ELEVATION_RATE) * MILLISECONDS_PER_SECOND;
         }
      }else {  /* Max radial exception is set, so keep generating radials */
         radial_msg->hdr.base.status = PROCESSING_RADIAL_DATA;
         *current_state = OPERATE;
         if(radial_number > Max_radials_per_elevation)
            MAX_radial_exceeded = FALSE;
      }
   }else if (radial_number != ONE) {
      radial_msg->hdr.base.status = PROCESSING_RADIAL_DATA;
      *current_state = OPERATE;
   }else
      *current_state = OPERATE;

      /* if spot blanking is enabled, spot blank the first and last radial of each
         elevation cut that is <= 6.0 degrees */

   if (RDA_status_msg.spot_blanking_status == SBS_ENABLED) {
      if (elevation_angle <= 6.0) {
         if ((radial_number == 1) || (radial_number == Radials_per_elevation))
            radial_msg->hdr.base.spot_blank_flag = RADIAL_IS_SPOT_BLANKED;
         else
            radial_msg->hdr.base.spot_blank_flag = ELEV_CUT_HAS_SPOT_BLANKING;
      }
      else
         radial_msg->hdr.base.spot_blank_flag = VOL_SCAN_HAS_SPOT_BLANKING;
   }
   else
      radial_msg->hdr.base.spot_blank_flag = SPOT_BLANKING_IS_DISABLED;

      /* write the radial message to the RPG */
   RRM_process_rda_message (GENERIC_DIGITAL_RADAR_DATA);
   return; 
}


/********************************************************************************
 
     Description: Set the command line option radar parameters

           Input: ant_rate - the antenna rotation rate - a rotation rate of 0.0
                             specifies that the user did not define a rate at
                             simulator startup and the default VCP rates
                             should be used.
              sample_interval - the radial sampling rate. the default is 0.95 deg

          Output:

          Return:

 ********************************************************************************/

void CR_set_radar_parameters (float ant_rate, float sample_interval)
{
   Fixed_azimuth_rate = ant_rate;
   Radial_sample_interval = sample_interval;
   return;
}


/********************************************************************************
 
     Description: Compute the radial julian date and millisecond time

           Input: Radial message header

          Output: Radial message header

          Return:

 ********************************************************************************/

static void Compute_time (Radial_message_t *radial_msg)
{
   time_t julian_time;   /* the current julian time in seconds */
   
   julian_time = time (NULL);

      /* compute the current julian date */

   radial_msg->hdr.base.date = (unsigned short) ((julian_time /
                                   (time_t) SECONDS_IN_A_DAY) + ONE);

      /* compute the elapsed milliseconds since midnight */

   radial_msg->hdr.base.time = (int) ((julian_time % (time_t) SECONDS_IN_A_DAY) *
                       MILLISECONDS_PER_SECOND);
   return;
}


/********************************************************************************
      
    Description: this routine converts a real angular or velocity measurement 
                 to its binary equivalent.
                 Refer Table III-A AND Table XI-D in the RDA-RPG ICD. 

              For velocity data:
                 bit 15 = sign bit
                 bit 14 = 22.5 degrees
                 bit 13 = 11.25 degrees
                 bit 12 = 5.625 degrees
                     .
                     .
                     .
                 bit 3 = 0.0109...
                 bits 0-2 - not used

              For angular data:
                 180 deg = bit 15
                  90 deg = bit 14
                  45 deg = bit 13
                         .
                         .

     Input:  data_to_convert - the real number to convert
             data_type       - type of data to convert (e.g. angular data
                               or velocity data)

     Output:

     Return: converted_data - the real number converted to its binary 
                              equivalent

********************************************************************************/

static ushort Convert_data_out (float data_to_convert, int data_type)
{
/*   short sign_bit; */
   unsigned short converted_data = 0;
   float scale;

   switch (data_type) {
      case VELOCITY_DATA:
         converted_data = (unsigned short) (((double)
                    (data_to_convert + ORPGVCP_RATE_HALF_BAM)) * ORPGVCP_RATE_DEG2BAMS);

      break;

      case ANGULAR_ELEV_DATA:
         if( data_to_convert < 0.0 ){

           if( (data_to_convert - ORPGVCP_HALF_BAM) < -1.0 )
              scale = 0.0;

           else
              scale = -ORPGVCP_HALF_BAM;

         }
         else{

            if( (data_to_convert + ORPGVCP_HALF_BAM) > 45.0 )
               scale = 0.0;

            else
               scale = ORPGVCP_HALF_BAM;

         }

         converted_data = (unsigned short) (((double) (data_to_convert + scale)) *
                                              ORPGVCP_ELVAZM_DEG2BAMS);

      break;

      case ANGULAR_AZM_DATA:

         scale = ORPGVCP_HALF_BAM;
         converted_data = (unsigned short) (((double) (data_to_convert + scale)) *
                                              ORPGVCP_ELVAZM_DEG2BAMS);

      break;

      default:
         fprintf (stdout, 
              "Convert_data_out: Error - received a non-velocity/angle value\n");
      break;
   }

   return( (unsigned short) (converted_data & 0xfff8) );
}


/********************************************************************************
      
    Description: this routine converts a binary angular or velocity measurement 
                 to its real equivalent.
                 Refer Table III-A AND Table XI-D in the RDA-RPG ICD. 

              For velocity data:
                 bit 15 = sign bit
                 bit 14 = 22.5 degrees
                 bit 13 = 11.25 degrees
                 bit 12 = 5.625 degrees
                     .
                     .
                     .
                 bit 3 = 0.0109...
                 bits 0-2 - not used

              For angular data:
                 180 deg = bit 15
                  90 deg = bit 14
                  45 deg = bit 13
                         .
                         .

     Input:  data_to_convert - the binary measurement to convert
             data_type       - type of data to convert (e.g. angular data
                               or velocity data)

     Output:

     Return: converted_data - the binary data converted to a real number

********************************************************************************/

static float Convert_data_in (short data_to_convert, int data_type)
{
   float converted_data = 0.0;

   switch (data_type) {
      case VELOCITY_DATA:
         converted_data = (float) 
                   (((double) (data_to_convert & 0xfff8)) * ORPGVCP_RATE_BAMS2DEG);
      break;

      case ANGULAR_ELEV_DATA:
         converted_data = (float) 
                   (((double) (data_to_convert & 0xfff8)) * ORPGVCP_ELVAZM_BAMS2DEG);

         if( converted_data > 180.0 )
            converted_data -= 360.0;

      break;

      case ANGULAR_AZM_DATA:
         converted_data = (float) 
                   (((double) (data_to_convert & 0xfff8)) * ORPGVCP_ELVAZM_BAMS2DEG);
      break;

      default:
         fprintf (stdout, 
              "Convert_data_in: Error - received a non-velocity/angle value\n");
      break;
   }

   return (converted_data);
}


/********************************************************************************

     Description: this routine determines the sector number for the Doppler 
                  elevation cuts

           Input: azimuth_angle - azimuth/radial angle being computed
                  elev_index    - elevation index (i.e. elev cut - 1)

          Output:

          Return: sector_num - sector number on success; -1 on error

 ********************************************************************************/

static short Determine_sector_number (float azimuth_angle, ushort elev_index)
{
   short sector_num = -1;

      /* determine the sector # for this radial */

   if ((azimuth_angle >= Vcp_data.segment_angles [elev_index][0])  &&
      (azimuth_angle < Vcp_data.segment_angles [elev_index][1]))
      sector_num = 1;
   else if ((azimuth_angle >= Vcp_data.segment_angles [elev_index][1]) &&
           (azimuth_angle < Vcp_data.segment_angles [elev_index][2]))
      sector_num = 2;
   else
      sector_num = 3;

   return sector_num;
}


/********************************************************************************

     Description: this routine generates the aliased velocity bins from the
                  array populated with all the possible velocity values 

           Input: Va - Nyquist Velocity

          Output: Aliased_velocity [] - the aliased velocity radial

 ********************************************************************************/

static void Generate_aliased_velocities (float Va)
{
   int i;
   short modulo;
   float Va2;        /* 2 times nyquist velocity */
   float velocity;   /* true velocity before aliasing */
   float result;     /* velocity after aliasing */

   Va2 = 2 * Va;

   for (i = 0; i < MAX_NUM_VEL_BINS; i++) {

      if (Vcp_data.velocity_resolution == 2)  /* velocity resolution = 0.5 m/s */
         velocity = (Velocity_bins [i] >> 1) - 64.5;
      else 
         velocity = Velocity_bins [i] - 129.0;

      modulo = (int)abs(velocity) % (int)Va2;

      if (modulo > Va) {
         if (velocity < 0)
             result =  Va2 - modulo;
         else
             result = modulo - Va2;
      } else {
           if (velocity >= 0)
              result = modulo;
           else
              result = -modulo;
      }

      if (Vcp_data.velocity_resolution == 2)  /* velocity resolution = 0.5 m/s */
         Aliased_velocity [i] = (unsigned char) (((result + 63.5) * 2) + 2);
      else
         Aliased_velocity [i] = (unsigned char) (result + 129.0);

   }

   return;
}


/********************************************************************************

     Description: determine the VCP index to use in the locally defined arrays
                  based on the local VCP selected

           Input: vcp - vcp number selected

          Output: 

          Return: the index for the local VCP selected on success; -1 on failure

 ********************************************************************************/

static int Get_vcp_index (int vcp_selected)
{
   int index;

   switch (abs(vcp_selected)) { 
      case 11:
         index = 0;
      break;

      case 21:
         index = 1;
      break;

      case 31:
         index = 2;
      break;

      case 32:
         index = 3;
      break;

      case 300:
         index = 4;
      break;

      default:
         index = -1;
         break;
   }
   return (index);
}


/********************************************************************************

     Description: initialize the radial bins to some static value

           Input: *radial - the radial (ie Digitqal Radar Data) message

          Output: radial  - the moment data fields in the radial message

          Return:

 ********************************************************************************/

#define SW_ICD_SCALING_FACTOR  2

void Initialize_moments (Radial_message_t *radial)
{
   int i;
   float number_of_surv_bins_per_value;
   float number_of_dop_bins_per_value;
   float number_of_sw_bins_per_value;
   float number_of_zdr_bins_per_value;
   float number_of_phi_bins_per_value;
   float number_of_rho_bins_per_value;

     /* initialize the surveillance bins */

     /* The surveillance data begins at data level 2. */
   number_of_surv_bins_per_value = MAX_SR_NUM_SURV_BINS / 254.0f;
/*   number_of_surv_bins_per_value = MAX_SR_NUM_SURV_BINS / 175.0f;*/

   if (VErbose_mode == 4) {
      fprintf (stdout, "\nSurveillance moment initialization:\n");
      fprintf (stdout, "----------------------------------\n\n");
   }

   for (i = 0; i < MAX_SR_NUM_SURV_BINS; i++) {

    Surv_bins [i] = (char) ((i / number_of_surv_bins_per_value) + 2);
/*    Surv_bins [i] = (char) ((i / number_of_surv_bins_per_value) + 86);*/

      if (VErbose_mode == 4) {
         if ((i % 8) == 0) {
            fprintf (stdout, "\n bin #: %3d", i);
            fprintf (stdout, "     %4d", Surv_bins[i]);
         }
         else
            fprintf (stdout, "      %4d", Surv_bins[i]);
      }

   }

     /* initialize the velocity and spectrum width bins */

   number_of_dop_bins_per_value = MAX_SR_NUM_VEL_BINS / 251.0f;
   number_of_sw_bins_per_value = 46; /* SW range: 149 - 129 = 20...ICD is wrong */

   if (VErbose_mode == 4) {
      fprintf (stdout, "\n\nVelocity and Spectrum Width moments initialization:\n");
      fprintf (stdout, "--------------------------------------------------\n\n");
   }

   for (i = 0; i < MAX_SR_NUM_VEL_BINS; i++) {

      Velocity_bins [i] = (char) ((float) i / number_of_dop_bins_per_value) + 2;
      SW_bins [i] = (unsigned char) (149 - ((float) i / number_of_sw_bins_per_value));
   
      if (VErbose_mode == 4) {
         if ((i % 4) == 0) {
            fprintf (stdout, "\n bin #: %3d", i);
            fprintf (stdout, "   dop: %4d  sw: %4d", Velocity_bins [i], SW_bins [i]);
         }
         else
            fprintf (stdout, "   dop: %4d  sw: %4d", Velocity_bins [i], SW_bins [i]);
      }
   }

   /* initialize the dual pol field bins (ZDR, PHI and RHO). */

   number_of_zdr_bins_per_value = (float) MAX_NUM_ZDR_BINS / 254.0f; 
   for( i = 0; i < MAX_NUM_ZDR_BINS; i++ ) {
      Zdr_bins [i] = (unsigned char) ((float) i / number_of_zdr_bins_per_value) + 2;
   }
      
   number_of_phi_bins_per_value = (float) MAX_NUM_PHI_BINS / 1021.0f;
   float nmi; 
   int j;
   short s;
   float modulo;
   s = 31;
   j = 31;

   for( i = 0; i < MAX_NUM_PHI_BINS; i++ ){
     /*nmi = (i*0.134989) + 1.0;*/  /* convert quarter kms to nmi */
     nmi = ((i+1)*0.134989);  /* convert quarter kms to nmi */
     Phi_bins [i] = (unsigned short) ((float) i / number_of_phi_bins_per_value) + 2;
     /*modulo = (int) ((nmi-30)*(nmi-30)/15.) % (int) 360;*/  /* Mike's equation */
     modulo = fmodf( ((nmi-30.0)*(nmi-30.)/15.) ,  360.0 );  /* Mike's equation */
     Phi_bins_mike [i] = (unsigned short) (modulo*2.8361+2.0);  
     modulo = fmodf( ((nmi+30.)*(nmi+30.)/30.) , 360.);  /* John's equation */
     Phi_bins_john [i] = (unsigned short) (modulo*2.8361+2.0);
     Phi_bins_step [i] = (unsigned short) (s); 
     s+=2; /* double step up each bin */
     if(s > 1079) s = 2;
/*     fprintf(stdout,"%d   %d   %d\n",Phi_bins_mike[i], Phi_bins_john[i], Phi_bins_step[i]);*/
   }

   number_of_rho_bins_per_value = (float) MAX_NUM_RHO_BINS / 254.0f;
/*   number_of_rho_bins_per_value = (float) MAX_NUM_RHO_BINS / 40.0f;*/
   for( i = 0; i < MAX_NUM_RHO_BINS; i++ ){
      Rho_bins [i] = (unsigned char) ((float) (MAX_NUM_RHO_BINS-1-i) / number_of_rho_bins_per_value) + 2;
   }

   if (VErbose_mode == 4) {
      fprintf (stdout,"\n\n");
      fflush (stdout);
   }

   return;
}


/********************************************************************************

     Description: this routine updates the dynamic radial data for each 
                  radial constructed

           Input:

          Output:

  ********************************************************************************/


#define BEGINNING_OF_MOMENT_DATA  	sizeof(Radial_message_t)  /* ICD defined radial 
                                                                     message offset */
#define VMAX_COEF			0.025  /* coefficient for computing nyquist 
                                                  velocity (1/4 * 10cm wavelength 
                                                  converted to metres) */
#define RMAX_COEFF            		149896 /* coefficient for computing unambiguous 
                                                  range (c/2 in km/second) */

static void Update_dynamic_radial_data (Radial_message_t *radial_msg, float elev_angle, 
                                        float azimuth_angle, unsigned short elev_cut) 
{
      /* the following table is the Nyquist velocity for the PRFs for Delta 
         Pulse Repitition Interval #3. the array is initialized in meters/second.

                   PRF #                1          2          3          4
                                        5          6          7          8

                   PRF (pulses/sec)  321.888    446.428    643.777    857.143
                                    1013.151   1094.890   1181.000   1282.050
                                    
                   Vn in kts          15.89      22.05      31.79      42.32
                                      50.02      54.06      58.31      63.30 */

   float nyquist_velocity [8] = {     8.18,      11.34,     16.35,     21.77,   
                                     25.73,      27.81,     30.00,     32.56 };

   float nyquist_vel_sprt [8] = {    30.20,      30.20,     35.10,     38.30,   
                                     42.00,      46.60,     52.30,     59.70 };


      /* the following table defines the PRFs ranges for Delta PRI #3.
         the array is initialized in km.

                   PRF #                1          2          3          4
                                        5          6          7          8
      
                   Range in nm       251.78      181.54     125.89     94.55
                                      79.97       74.02      68.62     63.22 */

   short range [8] = {                  460,        336,       233,      175,
                                        148,        137,       127,      117 };
   short range_d_sprt [8] = {           261,        261,       224,      206,
                                        188,        169,       151,      132 };
   short range_s_sprt [8] = {           392,        392,       337,      309,
                                        281,        254,       226,      198 };

   int i;
   unsigned short elevation_index;
   short sector_number;
   unsigned short sector_index = 0;
   unsigned short range_prf = 0;
   ushort unambiguous_range = 0;
   ushort number_doppler_bins = 0;
   ushort max_range = 511;        /* 511 km = max range allowed */
   float velocity;                /* computed nyquist velocity */
   static float n_1_nyquist_velocity = -1; /* last time's nyquist velocity */

   elevation_index = elev_cut - 1;

   /* Set the calib constant and noise levels for all radials. */
   radial_msg->elv_hdr.calib_const = -54.0f;

   /* 2.62 degrees = range clipping angle due to the 70k ft altitude limit */
   if (elev_angle > 2.62 ) {
      float alpha;                     /* elev angle in radians */
      float altitude_ceiling = 21.341; /* 70k ft altitidue ceiling in km */

      alpha = elev_angle * M_PI/180.0;
      max_range = (ushort) (altitude_ceiling * (1/tan (alpha)));
   }

   /* Initialize the moment pointers and number of bins.   These will be filled
      in according to waveform type and enabled/disabled moments. */
   radial_msg->ref_hdr.no_of_gates = 0;
   radial_msg->vel_hdr.no_of_gates = 0;
   radial_msg->wid_hdr.no_of_gates = 0;
   radial_msg->zdr_hdr.no_of_gates = 0;
   radial_msg->phi_hdr.no_of_gates = 0;
   radial_msg->rho_hdr.no_of_gates = 0;

   /* Assume a volume header, elevation header and radial header. */
   radial_msg->hdr.base.no_of_datum = 3;
   radial_msg->hdr.base.data[0] = RDASIM_VOL_DATA;
   radial_msg->hdr.base.data[1] = RDASIM_ELV_DATA;
   radial_msg->hdr.base.data[2] = RDASIM_RAD_DATA;

   for( i = 3; i < MAX_DATA_BLOCKS; i++ )
      radial_msg->hdr.base.data[i] = 0;

   radial_msg->hdr.base.sector_num = 0;
   radial_msg->rad_hdr.nyquist_vel = 0;

   /* if the waveform is surv or batch and the reflectivity moment
      is enabled, then update the survillience data (see ICD for bit mask) */

   /* if the waveform is Doppler w AB or SPRT, then update the survillience 
      data (see ICD for bit mask) */
   if ((Vcp_data.waveform_type [elevation_index] & 0x0005)  
                              ||
       ((Vcp_data.waveform_type [elevation_index] == 0x0002) 
                              &&
        (Vcp_data.super_res [elevation_index] & 0x0001))) {

      /* for CD w/o AB waveform, the surv PRF # is 0 in the RPG VCP msg 
         so use the Doppler PRF # for these elevation cuts. */
      if( (Vcp_data.waveform_type [elevation_index] == 0x0003)
                                 ||
          ((Vcp_data.waveform_type [elevation_index] == 0x0002) 
                                 &&
          (Vcp_data.super_res [elevation_index] & 0x0001)) ){
         if ((sector_number = Determine_sector_number (azimuth_angle, elevation_index)) == -1)
              sector_number = 1;  /* force a valid sector number */
         range_prf = Vcp_data.dopler_prf_number [elevation_index][sector_number - 1];
      }
      else
         range_prf = Vcp_data.surv_prf_number [elevation_index];

      radial_msg->hdr.base.data[radial_msg->hdr.base.no_of_datum] = RDASIM_REF_DATA; 
      radial_msg->hdr.base.no_of_datum++;

      if (Vcp_data.waveform_type [elevation_index] == 0x0005){

          if ((unambiguous_range = range_s_sprt [range_prf - 1]) > max_range)
              unambiguous_range = max_range;

      }
      else{

          if ((unambiguous_range = range [range_prf - 1]) > max_range)
              unambiguous_range = max_range;

      }

      if (unambiguous_range > 460 /* km */){

          radial_msg->ref_hdr.no_of_gates = MAX_NUM_SURV_BINS;  
          if( Vcp_data.super_res[elevation_index] & VCP_QRTKM_SURV )
             radial_msg->ref_hdr.no_of_gates = MAX_SR_NUM_SURV_BINS;  

      }
      else{

          radial_msg->ref_hdr.no_of_gates = unambiguous_range;
          if( Vcp_data.super_res[elevation_index] & VCP_QRTKM_SURV )
             radial_msg->ref_hdr.no_of_gates = unambiguous_range*4;  

      }
          
      /* populate the surv bins */
      for (i = 0; i < radial_msg->ref_hdr.no_of_gates; i++) {

         if( Vcp_data.super_res[elevation_index] & VCP_QRTKM_SURV ) {
            /* Use all data values for 1/4 KM bin resolution */
            if ( Interactive_mode == 0) {
               /* All azimuths are treated the same */
               radial_msg->ref_hdr.gate.b[i] = Surv_bins[i];
            }
            else {
               /* Apply user defined data values and regions */
               if ( azimuth_angle < Surv_az1 ) 
                  radial_msg->ref_hdr.gate.b[i] = Surv_fixed_1;
               else if ( azimuth_angle < Surv_az2 )
                  radial_msg->ref_hdr.gate.b[i] = Surv_fixed_2;
               else
                  radial_msg->ref_hdr.gate.b[i] = Surv_bins[i];
            }
         }
         else{
            /* Use very forth data value for 1 KM bin resolution */
            if ( Interactive_mode == 0) {
               /* All azimuths are treated the same */
               radial_msg->ref_hdr.gate.b[i] = Surv_bins[4*i];
            }
            else {
               /* Apply user defined data values and regions */
               if ( azimuth_angle < Surv_az1 ) 
                  radial_msg->ref_hdr.gate.b[i] = Surv_fixed_1;
               else if ( azimuth_angle < Surv_az2 )
                  radial_msg->ref_hdr.gate.b[i] = Surv_fixed_2;
               else
                  radial_msg->ref_hdr.gate.b[i] = Surv_bins[4*i];
            }
         }
      }

      /*  Insert high reflectivity at fixed ranges and azimuths for testing. */
      if( ((azimuth_angle > 90.0) && (azimuth_angle < 91))
                                || 
          ((azimuth_angle > 180.0) && (azimuth_angle < 181.0))
                                || 
          ((azimuth_angle > 270.0) && (azimuth_angle < 271.0)) ){

         radial_msg->ref_hdr.gate.b[0] = 200;
         radial_msg->ref_hdr.gate.b[1] = 150;
         radial_msg->ref_hdr.gate.b[2] = 100;
         radial_msg->ref_hdr.gate.b[185] = 200;
         radial_msg->ref_hdr.gate.b[370] = 200;

      }

      /* Insert the reflectivity threshold. */
      radial_msg->ref_hdr.SNR_threshold = Vcp_data.ref_snr[elevation_index];

   }
   else 
      range_prf = 4; /* pick a prf */

      /* if the waveform is Doppler or batch, then update the doppler data 
         (see ICD for bit mask) */

   if (Vcp_data.waveform_type [elevation_index] & 0x0006) {

      /* determine the sector # for this radial */
      if ((sector_number = Determine_sector_number (azimuth_angle, elevation_index)) == -1)
           sector_number = 1;  /* force a valid sector number */

      sector_index = sector_number - 1;

      radial_msg->hdr.base.sector_num = sector_number;

         /* determine the doppler prf and update the nyquist velocity */
/*      computing pulses/second from the azimuth rate and pulse count/radial introduced errors
        causing nyquist_velocity computational errors. This eq. has been left in encase
        we go back and use this method to compute the range and velocity */
/*      nyquist_velocity = (ushort) (VMAX_COEFF * 
                  Vcp_data.doppler_prf [elevation_index][sector_index]); */
      if (Vcp_data.waveform_type [elevation_index] == 0x0005)
         velocity = 
            nyquist_vel_sprt [range_prf - 1];
      else 
         velocity = 
            nyquist_velocity [Vcp_data.dopler_prf_number [elevation_index][sector_index] - 1];

      radial_msg->rad_hdr.nyquist_vel = (ushort) (velocity * 100);

      /* if the nyquist velocity has changed, generate new aliased velocity radial data */
      if (velocity != n_1_nyquist_velocity)
         Generate_aliased_velocities (velocity);

      n_1_nyquist_velocity = velocity;
      if (Vcp_data.waveform_type [elevation_index] != 0x0005)
         range_prf = Vcp_data.dopler_prf_number [elevation_index][sector_index];

/*      computing pulses/second from the azimuth rate and pulse count/radial introduced errors
        causing unambiguous_range computational errors. This eq. has been left in encase
        we go back and use this method to compute the range and velocity */
/*      unambiguous_range = (ushort) (RMAX_COEFF / Vcp_data.doppler_prf [elevation_index][sector_index]); */

      if (Vcp_data.waveform_type [elevation_index] == 0x0005)
         unambiguous_range = (ushort) 
              range_d_sprt [Vcp_data.surv_prf_number [elevation_index] - 1];
      else
         unambiguous_range = (ushort) 
              range [Vcp_data.dopler_prf_number [elevation_index][sector_index] - 1];

      if (Vcp_data.waveform_type [elevation_index] == 0x0003) {
         if (max_range < unambiguous_range)
            number_doppler_bins = max_range;
         else
            number_doppler_bins = unambiguous_range;
      } 
      else {

         if (max_range > unambiguous_range)
            number_doppler_bins = max_range;
         else
            number_doppler_bins = unambiguous_range;
      }

      number_doppler_bins *= 4;

      if (number_doppler_bins > MAX_NUM_VEL_BINS ){

         number_doppler_bins = MAX_NUM_VEL_BINS;
         if( Vcp_data.super_res[elevation_index] & VCP_300KM_DOP  )
            number_doppler_bins = MAX_SR_NUM_VEL_BINS;

      }


      radial_msg->vel_hdr.no_of_gates = number_doppler_bins;

      radial_msg->hdr.base.data[radial_msg->hdr.base.no_of_datum] = RDASIM_VEL_DATA;
      radial_msg->hdr.base.no_of_datum++;

      /* populate the velocity bins */
      for (i = 0; i < number_doppler_bins; i++) {
         if (Interactive_mode == 0) {
            /* All azimuths treated the same */
            radial_msg->vel_hdr.gate.b[i] = Aliased_velocity [i];
         }
         else{
            /* Apply user defined data values and regions */
            if ( azimuth_angle < Velo_az1 ) 
               radial_msg->vel_hdr.gate.b[i] = Velo_fixed_1;
            else if ( azimuth_angle < Surv_az2 )
               radial_msg->vel_hdr.gate.b[i] = Velo_fixed_2;
            else
               radial_msg->vel_hdr.gate.b[i] = Aliased_velocity [i];
         }               
      }

      /*  Insert test velocities at fixed ranges and azimuths for testing. */
      if( ((azimuth_angle > 90.0) && (azimuth_angle < 91.0))
                                  || 
          ((azimuth_angle > 180.0) && (azimuth_angle < 181.0))
                                  || 
          ((azimuth_angle == 270.0) && (azimuth_angle < 271.0)) ){

         radial_msg->vel_hdr.gate.b[0] = 200;
         radial_msg->vel_hdr.gate.b[4] = 150;
         radial_msg->vel_hdr.gate.b[8] = 100;
         radial_msg->vel_hdr.gate.b[12] = 50;
         radial_msg->vel_hdr.gate.b[185] = 200;
         radial_msg->vel_hdr.gate.b[370] = 200;

      }

      /* Insert the velocity threshold. */
      radial_msg->vel_hdr.SNR_threshold = Vcp_data.vel_snr[elevation_index];

      /* populate the spectrum width bins */
      radial_msg->hdr.base.data[radial_msg->hdr.base.no_of_datum] = RDASIM_WID_DATA;
      radial_msg->hdr.base.no_of_datum++;

      radial_msg->wid_hdr.no_of_gates = number_doppler_bins;

      for (i = 0; i < number_doppler_bins; i++) 
          radial_msg->wid_hdr.gate.b[i] = SW_bins [i];

      /*  Insert test spectrum width at fixed ranges and azimuths for testing. */
      if( ((azimuth_angle > 90.0) && (azimuth_angle < 91.0))
                                  || 
          ((azimuth_angle > 180.0) && (azimuth_angle < 181.0))
                                  || 
          ((azimuth_angle == 270.0) && (azimuth_angle < 271.0)) ){

         radial_msg->wid_hdr.gate.b[0] = 149;
         radial_msg->wid_hdr.gate.b[4] = 139;
         radial_msg->wid_hdr.gate.b[8] = 129;
         radial_msg->wid_hdr.gate.b[12] = 129;
         radial_msg->wid_hdr.gate.b[185] = 149;
         radial_msg->wid_hdr.gate.b[370] = 149;

      }

      /* Insert the spectrum width threshold. */
      radial_msg->wid_hdr.SNR_threshold = Vcp_data.sw_snr[elevation_index];

   }

   /* see if dual pol data to be included this radial. */
   if( Vcp_data.dual_pol [elevation_index] ){

      int max_range_m = max_range * 1000;

      radial_msg->zdr_hdr.no_of_gates = max_range_m / radial_msg->zdr_hdr.bin_size;
      if( radial_msg->zdr_hdr.no_of_gates > MAX_NUM_ZDR_BINS )
         radial_msg->zdr_hdr.no_of_gates = MAX_NUM_ZDR_BINS;

      radial_msg->rho_hdr.no_of_gates = max_range_m / radial_msg->rho_hdr.bin_size;
      if( radial_msg->rho_hdr.no_of_gates > MAX_NUM_RHO_BINS )
         radial_msg->rho_hdr.no_of_gates = MAX_NUM_RHO_BINS;

      radial_msg->phi_hdr.no_of_gates = max_range_m / radial_msg->phi_hdr.bin_size;
      if( radial_msg->phi_hdr.no_of_gates > MAX_NUM_PHI_BINS )
         radial_msg->phi_hdr.no_of_gates = MAX_NUM_PHI_BINS;

      radial_msg->hdr.base.data[radial_msg->hdr.base.no_of_datum] = RDASIM_ZDR_DATA;
      radial_msg->hdr.base.no_of_datum++;
      
      /* populate the ZDR bins */
      for (i = 0; i < radial_msg->zdr_hdr.no_of_gates; i++) {
         if ( Interactive_mode == 0 ) {
            /* All azimuths treated the same */
            radial_msg->zdr_hdr.gate.b[i] = Zdr_bins [i];
         }
         else {
            /* Apply user entered azimuths to create different ZDR data regions */
            if( azimuth_angle < Zdr_az1) {
               radial_msg->zdr_hdr.gate.b[i] = Zdr_fixed_1;
            }
            else if (azimuth_angle < Zdr_az2) {
               radial_msg->zdr_hdr.gate.b[i] = Zdr_fixed_2;
            }
            else {
               radial_msg->zdr_hdr.gate.b[i] = Zdr_bins [i];
            }
         }
      }
        
      radial_msg->hdr.base.data[radial_msg->hdr.base.no_of_datum] = RDASIM_PHI_DATA;
      radial_msg->hdr.base.no_of_datum++;

      /* populate the PHI bins */
      for (i = 0; i < radial_msg->phi_hdr.no_of_gates; i++) {
         if ( Interactive_mode == 0 ) {
            /* All azimuths treated the same */
            radial_msg->phi_hdr.gate.u_s[i] = Phi_bins [i];
         }
         else {
            /* Apply user entered azimuths to create different PHI data regions */
            if( azimuth_angle < Phi_az1) {
               radial_msg->phi_hdr.gate.u_s[i] = Phi_bins_mike [i];
            }
            else if (azimuth_angle < Phi_az2) {
               radial_msg->phi_hdr.gate.u_s[i] = Phi_bins_john [i];
            }
            else {
               radial_msg->phi_hdr.gate.u_s[i] = Phi_bins [i];
            }
         }
      }

      radial_msg->hdr.base.data[radial_msg->hdr.base.no_of_datum] = RDASIM_RHO_DATA;
      radial_msg->hdr.base.no_of_datum++;

      /* populate the RHO bins */
      for (i = 0; i < radial_msg->rho_hdr.no_of_gates; i++) {
         if ( Interactive_mode == 0 ) {
            /* All azimuths treated the same */
            radial_msg->rho_hdr.gate.b[i] = Rho_bins [i];
         }
         else {
            /* Apply user entered azimuths to create different RHO data regions */
            if( azimuth_angle < Rho_az1) {
               radial_msg->rho_hdr.gate.b[i] = Rho_fixed_1;
            }
            else if (azimuth_angle < Rho_az2) {
               radial_msg->rho_hdr.gate.b[i] = Rho_fixed_2;
            }
            else {
               radial_msg->rho_hdr.gate.b[i] = Rho_bins [i];
            }
         }
      }
   
      /* Insert the dual pol thresholds. */
      radial_msg->zdr_hdr.SNR_threshold = Vcp_data.zdr_snr[elevation_index];
      radial_msg->phi_hdr.SNR_threshold = Vcp_data.phi_snr[elevation_index];
      radial_msg->rho_hdr.SNR_threshold = Vcp_data.rho_snr[elevation_index];

   }

   /* enforce ICD range limits on the unambiguous range. Due to considering 
      the sample interval in the pulses/second computation, the computed count 
      may be slightly greater than/less than the actual count allowed so we'll
      just compensate for any range limit errors here */
   if (unambiguous_range < 115)      /* 115 km = lower range limit */
      unambiguous_range = 115;
   else if (unambiguous_range > 511) /* 511 km = upper range limit */
      unambiguous_range = 511;

   /* 10 & 1000 are ICD scaling factors */
   radial_msg->elv_hdr.atmos = Vcp_data.atmos_atten [elevation_index] * 1000;
   radial_msg->rad_hdr.unamb_range = unambiguous_range * 10;

   return;

}


/********************************************************************************

     Description: this routine updates the RDA VCP message in an Open RDA 
                  configuration. This msg is sent to the RPG as part of the 
                  metadata

           Input: vcp_data_source - specifies whether the VCP is a local or
                                    remote VCP

          Output:

 ********************************************************************************/

static void Update_rda_vcp_msg (int vcp_data_source)
{
   char *rda_vcp_buffer_ptr;
   int msg_size;

   short moment_thresholds [MAX_NUMBER_LOCAL_ELEV_CUTS] = 
                         { 16, 28, 16, 28, 28, 28, 28, 28,
                           28, 28, 16, 28, 28, 28, 28, 28};

   short surv_pulse_count [NUMBER_LOCAL_VCPS][MAX_NUMBER_LOCAL_ELEV_CUTS] = {
         /* VCP 11 */    { 17,  0, 16,  0,  6,  6,  6, 10,
                           10, 43, 46, 46, 46, 46, 46, 46},

         /* VCP 21 */    { 28,  0, 28,  0,  8,  8,  8, 12,
                           82, 82, 82,  0,  0,  0,  0,  0},

         /* VCP 31 */    { 63, 87, 63, 87, 63, 87, 87, 87,
                            0,  0,  0,  0,  0,  0,  0,  0},

         /* VCP 32 */    { 64,  0, 64,  0, 11, 11, 11,  0,
                            0,  0,  0,  0,  0,  0,  0,  0},

         /* VCP 300 */   { 64,  0, 64,  0,  0,  0,  0,  0,
                            0,  0,  0,  0,  0,  0,  0,  0}};

     /* ICD value for the pulse width */

   short pulse_width [NUMBER_LOCAL_VCPS] = {2, 2, 4, 2, 2};


      /* get the RDA VCP buffer pointer */

   rda_vcp_buffer_ptr = (char *)RRM_get_rda_vcp_buffer ();

      /* if the VCP is a remote VCP, then just copy the VCP msg to the RDA
         VCP msg buffer */

   if (vcp_data_source == REMOTE_VCP) {
      RDA_RPG_message_header_t *remote_vcp_buffer_ptr;

      remote_vcp_buffer_ptr = (RDA_RPG_message_header_t *) RRM_get_remote_vcp ();
      msg_size = (size_t)(sizeof (short) * remote_vcp_buffer_ptr->size);

      memcpy (rda_vcp_buffer_ptr, (char *)remote_vcp_buffer_ptr, msg_size);
      RRM_update_msg_size (RDA_VCP_MSG, msg_size);
   }
   else {  /* the next VCP to execute is a local VCP, so the RDA VCP
              msg must be manually updated */
      int i;
      int msg_offset;
      int vcp_index;
      RDA_RPG_message_header_t *rda_rpg_msg_hdr;
      VCP_message_header_t *msg_ptr;
      VCP_elevation_cut_header_t *vcp_elev_data;

         /* get the VCP index for the VCP selected. if the VCP isn't a locally
            defined VCP, then lie and just use VCP 11 to resolve some of the
            VCP msg data...we'll see if someone is actually using this info */

      if ((vcp_index = Get_vcp_index (Vcp_data.pattern_number)) == -1)
          vcp_index = 0;
    
      rda_rpg_msg_hdr = (RDA_RPG_message_header_t *) rda_vcp_buffer_ptr;

         /* assign the msg ptr to the beginning of the VCP msg data...skip
            the RDA/RPG ICD message header */

      msg_offset = sizeof (RDA_RPG_message_header_t);

      msg_ptr = (VCP_message_header_t *) (rda_vcp_buffer_ptr + msg_offset);
      msg_ptr->pattern_number = abs(Vcp_data.pattern_number); 
      msg_ptr->pattern_type = 2;   /* constant elevation cut */

      msg_offset += sizeof (VCP_message_header_t);

         /* assign the vcp elevation pointer to the beginning of the 
            elevation dependent data */

      vcp_elev_data = (VCP_elevation_cut_header_t *) (rda_vcp_buffer_ptr + msg_offset);

      vcp_elev_data->number_cuts = Vcp_data.number_elev_cuts;
      vcp_elev_data->doppler_res = Vcp_data.velocity_resolution;
      vcp_elev_data->pulse_width = pulse_width [vcp_index];
      vcp_elev_data->group = 1;  /* hardcode group number to 1 */

         /* update all the msg fields for each elevation cut */

      for (i = 0; i < vcp_elev_data->number_cuts; i++) {
/*         vcp_elev_data->data[i].phase = Vcp_data.phase[i];*/
         vcp_elev_data->data[i].waveform = Vcp_data.waveform_type [i];
         vcp_elev_data->data[i].refl_thresh = moment_thresholds [i];
         vcp_elev_data->data[i].vel_thresh = moment_thresholds [i];
         vcp_elev_data->data[i].sw_thresh = moment_thresholds [i];
         vcp_elev_data->data[i].super_res = Vcp_data.super_res [i];
         vcp_elev_data->data[i].surv_prf_num = Vcp_data.surv_prf_number [i];
         vcp_elev_data->data[i].surv_prf_pulse = surv_pulse_count [vcp_index][i];

         vcp_elev_data->data[i].angle = 
                        Convert_data_out (Vcp_data.elev_angles [i], ANGULAR_ELEV_DATA);
         vcp_elev_data->data[i].azimuth_rate = 
                        Convert_data_out (Vcp_data.azimuth_rate [i], VELOCITY_DATA);
         vcp_elev_data->data[i].edge_angle1 = 
                        Convert_data_out (Vcp_data.segment_angles [i][0], ANGULAR_AZM_DATA);
         vcp_elev_data->data[i].edge_angle2 = 
                        Convert_data_out (Vcp_data.segment_angles [i][1], ANGULAR_AZM_DATA);
         vcp_elev_data->data[i].edge_angle3 = 
                        Convert_data_out (Vcp_data.segment_angles [i][2], ANGULAR_AZM_DATA);

         vcp_elev_data->data[i].dopp_prf_pulse1 = Dopler_pulse_count [vcp_index][i]; /* use same prf for each sector */
         vcp_elev_data->data[i].dopp_prf_pulse2 = Dopler_pulse_count [vcp_index][i];
         vcp_elev_data->data[i].dopp_prf_pulse3 = Dopler_pulse_count [vcp_index][i];
         vcp_elev_data->data[i].dopp_prf_num1 = Vcp_data.dopler_prf_number [i][0];
         vcp_elev_data->data[i].dopp_prf_num2 = Vcp_data.dopler_prf_number [i][1];
         vcp_elev_data->data[i].dopp_prf_num3 = Vcp_data.dopler_prf_number [i][2];
      }

         /* compute the VCP msg size */

      msg_size = sizeof (VCP_message_header_t) + sizeof (VCP_elevation_cut_header_t) -
                 ((MAX_ELEVATION_CUTS * sizeof (VCP_elevation_cut_data_t)) -
                  (vcp_elev_data->number_cuts * sizeof (VCP_elevation_cut_data_t)));

         /* if odd # bytes in msg, round up to next halfword */

      if ((msg_size % 2) != 0)
          ++msg_size;

         /* update the msg size in the VCP msg and in the RDA/RPG message array */

      msg_ptr->msg_size = msg_size / sizeof (short);

      RRM_update_msg_size (RDA_VCP_MSG, msg_size + sizeof (RDA_RPG_message_header_t));

   }
   return;
}

