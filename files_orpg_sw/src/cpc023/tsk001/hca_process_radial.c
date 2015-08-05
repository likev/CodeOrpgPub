/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/10 17:41:48 $
 * $Id: hca_process_radial.c,v 1.14 2014/12/10 17:41:48 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

/******************************************************************************
 *      Module:         Hca_process_radial                                    *
 *      Author:         Brian Klein, Yukuan Song                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    Controls the processing of a radial of input data     *
 *                      for the Hydrometeor Classificatioon Algorithm(HCA)    *
 *                                                                            *
 *      Change History:                                                       *
 *       Jan 2012: NA11-00387 Added partial beam blockage modification. BKlein*
 *       May 2012: NA12-00200 Prevent NBF and blockage in ZDR stats     BKlein*
 ******************************************************************************/

#include <rpgc.h> /* defined the dual-pol data type:RPGC_DREF...*/
#include <generic_basedata.h> /*--defined  Generic_moment_t--*/

#include <rpgcs.h>
#include <math.h>
#include "hca_local.h"
#include "hca_adapt.h"
#define EXTERN extern
#include "hca_adapt_object.h"

#define PI 3.1416
#define DEG_TO_RAD (PI/180.0)

#ifdef HCA_ZDR_ERROR_ESTIMATE
/* Variables for ZDR error calculation */
/* Holds HCA-computed ZDR error (DS based) for past 12 volumes */
float ZDR_error_DS[12] = {-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0};  
float ZDR_avg_error_DS;     /* Holds average of ZDR_error_DS array */
int   error_index_DS = 0;   /* Holds current pointer into ZDR_error_DS array */
int   error_exp_time_DS;    /* Time (seconds from midnight) when ZDR_error_DS expires */
unsigned short exp_date_DS; /* Julian date when ZDR_error_DS expires */

int    Bufsize[6] = {0,0,0,0,0,0}; /*Initial size of each refl category array */
float *AR_ZDR[6]; /* Pointers to arrays of ZDR values for each refl category */
int    BufsizeDS = 0; /* Initial size of dry snow method buffer */
float *DS_ZDR;    /* Pointer to array of ZDR values for dry snow method */
float ZDR_error_AR[6] = {-99.0,-99.0,-99.0,-99.0,-99.0,-99.0}; /* ZDR error(Ryshkov) for 6 refl categories */
float climoZDR[6] = {0.23, 0.27, 0.32, 0.38, 0.46, 0.55}; /* Climo ZDR for 20,22,24,26,28 and 30 dBZ */
int   error_index_AR = 0;    /* Current pointer into ZDR_avg_error_AR array */
float ZDR_avg_error_AR[12] = {-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0,-99.0};  /* Avg of all ZDR_error_AR arrays */
float medianZDR[6] = {-99.0,-99.0,-99.0,-99.0,-99.0,-99.0};
float medianDS_ZDR = -99.0;
float ZDR_running_avg_error_AR; /* Running average ZDR_avg_error from 12 volumes */
int   error_exp_time_AR;        /* Time (seconds from midnight) when ZDR_error_AR expires */
unsigned short exp_date_AR;     /* Julian date when ZDR_error_AR expires */
#endif

#ifdef CC_SNR_ANALYSIS
    int             cc_table[256][256];
    int             cc[1200], snr[1200];
    int             x, y;
#endif

/* Debug control.  Set DB_AZ < 0 to turn off debug output */
#define DB_AZ         -1
#define DB_SBIN       -1
#define DB_EBIN       -1
#define MINI_LKTP   -40 
#define ONE_BYTE      8 
#define TWO_BYTE     16 
#define KMTOM      1000.f 


/* Pointers to the input data headers */
static Generic_moment_t Zdr_hd, Phi_hd, Rho_hd, Smz_hd, Smv_hd,
                        Kdp_hd, Sdz_hd, Sdp_hd, Snr_hd,
                        QZdr_hd, QRho_hd, QSmz_hd, QKdp_hd,
                        QSdz_hd, QSdp_hd;

/* Function prototypes */
static int Get_data_fields (Hca_params_t *params, Base_data_header *bh, 
                            Hca_data_t *data);
static int Format_output_radial (Base_data_header *bh, Hca_params_t *params,
		                       Hca_data_t *data, char **out_radial);
static void Add_moment (char *buf, char *name, char f, Hca_params_t *params, 
	                   int word_size, unsigned short tover, short SNR_threshold, 
	                   float scale, float offset, float *data);
static void Hca_weightedMembershipAggregation(float weight[NUM_FL_INPUTS],
                                              float quality_i[NUM_FL_INPUTS],
                                              float fd_mem[NUM_FL_INPUTS],
                                              float *agg);
static void Print_unique_msg (char *msg);
static float Compute_range_from_height (float elev, float height);


/****************************************************************************
    Description:
       Create the output radial of the required dual pol and HCA data.
         The input radial is "bh", the processing parameters are "params",
         the processed fields are in "data" and the pointer to the output
         radial is returned with "out_radial". The function returns the
       size of the output radial.

    Input:

    Output:
    Returns:
    Globals:
    Notes:
  ***************************************************************************/
int Hca_process_radial(char *ibuf_ptr, char **output, int *length, float ML_top[MAXRADIALS], 
                       float ML_bottom[MAXRADIALS])
{

   /* local variables */
   float  agg_max;
   int    max_cal = NE;
   float  diff;
   float  top_diff;
/*   int    z_index;*/ /* REMOVED FOR NA11-00387 */
   int    blocked_percent;
   int    blockage_rng, blockage_azm;
   float  z_fshield, fshield = 1.0;
   int    atten_rad;  /* Flag set to 1 if high attenuation radial */

   int    bin;/* loop index for bin */
   int    h_class;/* loop index for class types */
   int    fl_input;/* loop index for class types */
   float  agg[NUM_CLASSES];
   float  points[NUM_X]; /* (OUT) Membership points   */
   float  fd_mem[NUM_FL_INPUTS];   /* Degree of membership F(Dj)   */
   float  weight_i[NUM_FL_INPUTS]; /* Weight for class i  */
   float  quality_i[NUM_FL_INPUTS];  /* Quality Index array for current bin */
   int    class_type[NUM_CLASSES] = {U0, U1, RA, HR, RH, BD, BI, GC, DS, WS, IC, GR, UK, NE}; 

   ML_bin_t Melting_layer;  /* Bin-based melting layer data */

   Base_data_header *bh;
   char  *out_radial;

   static Hca_params_t params;   /* Items extracted from Quality Index Algorithm output */
   static Hca_data_t   data;	 /* Structure for all input and output fields */

#ifdef HCA_ZDR_ERROR_ESTIMATE
   static int   DS_count, KDP_count, i;
   static float DS_Z_sum, DS_Z_avg, DS_ZDR_sum, DS_ZDR_avg, DS_ZDR_sum_sqr, DS_SD;
   static float DS_KDP_sum, DS_KDP_avg;
   static float DS_avg_med;
   static short DS_VCP;
   static int   DS_beg_vol_time, DS_vol_hr, DS_vol_min, oneKMabovelimit;

   static int   AR_count[6];
   static float AR_Z_sum[6], AR_Z_avg[6], AR_ZDR_sum[6], AR_ZDR_avg[6], AR_ZDR_sum_sqr[6], AR_SD[6];
   static float AR_avg_med[6];
   static short AR_VCP;
   static int   AR_beg_vol_time, AR_vol_hr, AR_vol_min, oneKMbelowlimit;
#endif

   bh = (Base_data_header *)ibuf_ptr;

   /* Reuse the input buffer for output */
   *output = ibuf_ptr;
   *length = bh->msg_len * sizeof (short);

   if (bh->n_surv_bins == 0)	       /* A spot blanked radial */
      return (bh->status);

   if (bh->azi_num == 1)            /* Reset the unique message tracker */
      Print_unique_msg ("");

#ifdef CC_SNR_ANALYSIS
   if(bh->status == BEG_ELEV){
       for (x = 0; x < 256; x++) 
          for (y = 0; y < 256; y++) 
             cc_table[x][y] = 0;
       for (x = 0; x < 1200; x++){
          cc[x] = 0;
          snr[x] = 0;
       }
   }
#endif

   /* Get the input data (this function also allocates all required memory) */
   if (Get_data_fields(&params, bh, &data) < 0)
      return (-1);

   if(bh->azi_num == DB_AZ){
     RPGC_log_msg(GL_INFO,"Starting HCA Elevation angle %f, #d_gates=%d",
                            bh->elevation,params.n_dgates);
     fprintf(stderr,"Starting HCA Elevation angle %f\n",bh->elevation);
   }

#ifdef CC_SNR_ANALYSIS
   for (x=0; x < 1200; x++)
       (cc_table[cc[x]][snr[x]])++;   
   if(bh->status == END_ELEV){
       fprintf(stderr,"START OF ELEVATION\n");
       for (x=0; x < 256; x++){
         for (y=0; y < 256; y++) {
          fprintf(stderr,"%d ",cc_table[x][y]);
         }
         fprintf(stderr,"\n");
       }
       fprintf(stderr,"END OF ELEVATION\n");
   }
#endif

#ifdef HCA_ZDR_ERROR_ESTIMATE
   for (i=0; i<6; i++) {
      if (Bufsize[i] == 0) {
         /*  Allocate memory for Rain method the first time */
         Bufsize[i] = sizeof(float*)*2000;
         AR_ZDR[i] = (float *)malloc(Bufsize[i]);
         if (AR_ZDR[i] == NULL) {
	   RPGC_log_msg(GL_MEMORY,"malloc failed in HCA, i=%d",i);
           RPGC_abort();
         }
      }
      else if (Bufsize[i] < (sizeof(float*)*(AR_count[i]+1000))){
         /* Reallocate to extend available space */
         Bufsize[i] = Bufsize[i] + (sizeof(float*)*3000);
         AR_ZDR[i] = (float *)realloc((void*)AR_ZDR[i],Bufsize[i]);
         if (AR_ZDR[i] == NULL) {
	   RPGC_log_msg(GL_MEMORY,"realloc failed in HCA, i=%d",i);
           RPGC_abort();
         }
      }
   }

   if (BufsizeDS == 0) {
      /*  Allocate memory for Dry Snow method the first time */
      BufsizeDS = sizeof(float*)*2000;
      DS_ZDR = (float *)malloc(BufsizeDS);
      if (DS_ZDR == NULL) {
        RPGC_log_msg(GL_MEMORY,"malloc failed in HCA for DS");
        RPGC_abort();
      }
   }
   else if (BufsizeDS < (sizeof(float*)*(DS_count+1000))){
      /* Reallocate to extend available space */
      BufsizeDS = BufsizeDS + (sizeof(float*)*3000);
      DS_ZDR = (float *)realloc((void*)DS_ZDR,BufsizeDS);
      if (DS_ZDR == NULL) {
         RPGC_log_msg(GL_MEMORY,"realloc failed in HCA for DS");
         RPGC_abort();
      }
   }

   if(bh->status == END_VOL){
      int   savedDS_count = 0;
      float savedDS_SD = 0.0;
      /* Reached end of volume. Output dry snow (DS) based results and reset */
      if (DS_count != 0) {
        DS_ZDR_avg = DS_ZDR_sum / DS_count;
        DS_SD = sqrtf(DS_ZDR_sum_sqr/DS_count - (DS_ZDR_avg * DS_ZDR_avg));
        DS_Z_avg = DS_Z_sum / DS_count;
      }
      if (KDP_count != 0) DS_KDP_avg = DS_KDP_sum / KDP_count;
      if (DS_count > 500 && DS_SD < 0.50){
         /* Require at least 500 bins and a low standard deviation for DS method */
         medianDS_ZDR = quick_select(DS_ZDR,DS_count);
         DS_avg_med = DS_ZDR_avg - medianDS_ZDR;
         ZDR_error_DS[error_index_DS] = DS_ZDR_avg - 0.2;
         error_exp_time_DS = DS_beg_vol_time + 3600000; /* add one hour */
         if (error_exp_time_DS > 86400000) {
            error_exp_time_DS -= 86400000;
            exp_date_DS++;
         }
      }
      else {
         /* Insufficient number of bins or too high a standard deviation */
         ZDR_error_DS[error_index_DS] = -99.;
      }

      int ZDR_count = 0;
      float ZDR_total = 0.0;
      for (i=0;i<12;i++){
        /* Average results from all saved volumes */
        if (ZDR_error_DS[i] != -99.0) {
           ZDR_total += ZDR_error_DS[i];
           ZDR_count++;
        }
      }
      if (ZDR_count > 0)
         ZDR_avg_error_DS = ZDR_total / ZDR_count;
      else
         ZDR_avg_error_DS = -99.0; 

      error_index_DS++;        
      if (error_index_DS > 11) error_index_DS = 0;
      DS_vol_hr = DS_beg_vol_time / 3600000; /* milliseconds per hour */
      DS_vol_min = (DS_beg_vol_time - (3600000*DS_vol_hr)) / 60000; /* milliseconds per minute */

      /*  Output the Dry Snow Method results to the hca log file */
      RPGC_log_msg(GL_INFO,"Volume %02d:%02d ZDR_error_DS avg= %4.1f Avg ZDR in DS above ML= %5.2f SD= %3.1f Z= %4.1f num DS= %d VCP= %d",
      DS_vol_hr, DS_vol_min, ZDR_avg_error_DS, DS_ZDR_avg, DS_SD, DS_Z_avg, DS_count, DS_VCP);
      /* Output the Dry Snow Method results to the hca.output file */
/*      fprintf(stderr,"Volume %02d:%02d ZDR_error_DS_avg= %4.1f Avg ZDR in DS above ML = %5.2f SD= %3.1f Z= %4.1f num DS= %d VCP= %d\n",
      DS_vol_hr, DS_vol_min, ZDR_avg_error_DS, DS_ZDR_avg, DS_SD, DS_Z_avg, DS_count, DS_VCP);*/

      savedDS_count = DS_count;
      savedDS_SD = DS_SD;
      DS_count = 0;
      KDP_count = 0;
      DS_Z_sum = 0.0;
      DS_ZDR_avg = 0.0;
      DS_Z_avg = 0.0;
      DS_SD = 0.0;
      DS_avg_med = -99.0;
      DS_KDP_avg = 0.0;
      DS_KDP_sum = 0.0;
      DS_ZDR_sum = 0.0;
      DS_ZDR_sum_sqr = 0.0;
      DS_beg_vol_time = -99;

      /* Reached end of volume. Output A. Ryzhkov based (AR) results and reset */
      for (i=0; i<6; i++) {
        /* Require at least 200 bins to compute a standard deviation and ZDR error for */
        /* this volume scan.  The ZDR error isn't considered valid until it passes the */
        /* next test */
        if (AR_count[i] >= 200) {
          AR_ZDR_avg[i] = AR_ZDR_sum[i] / AR_count[i];
          AR_SD[i] = sqrtf(AR_ZDR_sum_sqr[i] / AR_count[i] - (AR_ZDR_avg[i] * AR_ZDR_avg[i]));
        }
        if (AR_count[i] >= 200){
          medianZDR[i] = quick_select(AR_ZDR[i],AR_count[i]);
          ZDR_error_AR[i] = medianZDR[i] - climoZDR[i];
          AR_avg_med[i] = AR_ZDR_avg[i] - medianZDR[i];
        }
        else 
           ZDR_error_AR[i] = -99.;

      } /* end for all reflectivity categories */

      /* Finally, average the ZDR error from all 6 categories, if they have valid values */
      ZDR_count = 0;
      ZDR_total = 0.0;
      for (i=0; i<6; i++){
        /* Require at least 1000 bins before considering a ZDR error estimate */
        /* for a given reflectivity category valid.                           */
        if (ZDR_error_AR[i] != -99.0 && AR_count[i] > 1000) {
           ZDR_total += ZDR_error_AR[i];
           ZDR_count++;
        }
      }

      if (ZDR_count > 1) {
	 /* Require at least two valid reflectivity categories before making */
         /* a ZDR error estimate for this volume scan.                       */
         ZDR_avg_error_AR[error_index_AR] = ZDR_total / ZDR_count;
         error_exp_time_AR = AR_beg_vol_time + 3600000; /* add one hour */
         if (error_exp_time_AR > 86400000) {
            error_exp_time_AR -= 86400000;
            exp_date_AR++;
         }
      }
      else
         ZDR_avg_error_AR[error_index_AR] = -99.0; 
      
      ZDR_count = 0;
      ZDR_total = 0.0;
      for (i=0; i<12; i++) {
        /* Average the ZDR error estimates for the past 12 volume scans.  */
        if (ZDR_avg_error_AR[i] != -99.0) {
           ZDR_total += ZDR_avg_error_AR[i];
           ZDR_count++;
        }
      }

      if (ZDR_count > 0)
         ZDR_running_avg_error_AR = ZDR_total / ZDR_count;
      else
         ZDR_running_avg_error_AR = -99.0; 

      error_index_AR++;
      if (error_index_AR > 11) error_index_AR = 0;  
      AR_vol_hr = AR_beg_vol_time / 3600000; /* milliseconds per hour */
      AR_vol_min = (AR_beg_vol_time - (3600000*AR_vol_hr)) / 60000; /* milliseconds per minute */

      /*  Output the Ryshkov Method results to the hca log file */
      RPGC_log_msg(GL_INFO,"Volume %02d:%02d ZDR_error_AR avg= %4.1f Median ZDR Rain Method= %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f num AR= %d %d %d %d %d %d SD= %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f",
      AR_vol_hr, AR_vol_min, ZDR_running_avg_error_AR, medianZDR[0], medianZDR[1], medianZDR[2], medianZDR[3], medianZDR[4], medianZDR[5], AR_count[0], AR_count[1], AR_count[2], AR_count[3], AR_count[4], AR_count[5], AR_SD[0], AR_SD[1], AR_SD[2], AR_SD[3], AR_SD[4], AR_SD[5]);
      /* Output the Ryshkov Method results to the hca.output file */
/*      fprintf(stderr,"Volume %02d:%02d ZDR_error_AR avg= %4.1f Median ZDR Rain Method= %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f ", AR_vol_hr, AR_vol_min, ZDR_running_avg_error_AR, medianZDR[0], medianZDR[1], medianZDR[2], medianZDR[3], medianZDR[4], medianZDR[5]);*/
/*      fprintf(stderr,"num AR= %d %d %d %d %d %d  SD= %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f\n", AR_count[0], AR_count[1], AR_count[2], AR_count[3], AR_count[4], AR_count[5], AR_SD[0], AR_SD[1], AR_SD[2], AR_SD[3], AR_SD[4], AR_SD[5]);*/

      /* Output both Ryshkov and Dry Snow Method summaries to system status log */
      LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG,
          "ZDR Stats: %4.2f/%5.2f,%5.2f,%5.2f,%5.2f,%5.2f,%5.2f/%d/%3.1f,%3.1f,%3.1f,%3.1f,%3.1f,%3.1f DS %4.2f/%d/%3.1f\n",
           ZDR_running_avg_error_AR, medianZDR[0], medianZDR[1], medianZDR[2], medianZDR[3], medianZDR[4], medianZDR[5], (AR_count[0]+AR_count[1]+AR_count[2]+AR_count[3]+AR_count[4]+AR_count[5]),AR_SD[0], AR_SD[1],AR_SD[2],AR_SD[3],AR_SD[4],AR_SD[5], ZDR_avg_error_DS, savedDS_count, savedDS_SD);

      for (i=0; i<6; i++) {
        AR_count[i] = 0;
        AR_Z_sum[i] = 0.0;
        AR_ZDR_avg[i] = 0.0;
        AR_avg_med[i] = -99.0;
        AR_Z_avg[i] = 0.0;
        AR_SD[i] = 0.0;
        AR_ZDR_sum[i] = 0.0;
        AR_ZDR_sum_sqr[i] = 0.0;
        medianZDR[i] = -99.;
        AR_beg_vol_time = -99;
      }
   } 
#endif

   /* Compute beam intersection with melting layer for this elevation */
   /* CPT&E label A */
   Hca_beamMLintersection(bh->elevation, (int)(bh->azimuth), params.dg_size, ML_top, ML_bottom, &Melting_layer);

   /* Store the input from the Melting Layer Detection Algorithm to be output later. */
    data.ml[MLTT] = Melting_layer.bin_tt;
    data.ml[MLT] = Melting_layer.bin_t;
    data.ml[MLB] = Melting_layer.bin_b;
    data.ml[MLBB] = Melting_layer.bin_bb;
    

/*   if((bh->azi_num == DB_AZ) && (bin = DB_SBIN))
     RPGC_log_msg(GL_INFO,"Melting Layer bins: bb = %d, b = %d, t = %d, tt = %d",
       Melting_layer.bin_bb,Melting_layer.bin_b,
       Melting_layer.bin_t,Melting_layer.bin_tt);*/

   if (bh->msg_type & HIGH_ATTENUATION_TYPE){
     RPGC_log_msg(GL_INFO,"High attenuation radial Az: %f elev: %f",bh->azimuth,bh->elevation);
     atten_rad = 1;
   }
   else
     atten_rad = 0;

   /* Perform individual radial processing if radial not flagged 'BAD'. */
   for ( bin = 0; bin < params.n_dgates; bin++) {

/*      z_index = RPGCS_dBZ_to_reflectivity(data.smz[bin]);*/ /* REMOVED FOR NA11-00387 */

      /* The blockage azimuth angle (azm_angle) is in tenths of a degree.
      * The Blockage Algorithm computes beam blockage for every 1 km
      * range bin and for every 0.1 deg.
      *
      * NOTE: blockage_azm is deliberately rounded to the nearest integer
      * to ensure that the range of values falls within the array
      * limits of 0 to 3599. */
      blockage_azm = (int) RPGC_NINT (bh->azimuth * 10.0);

      /* Make sure array index not over boundary */
      if(blockage_azm > 3599)
         blockage_azm = 3599;

      /* Convert range bin index from 250m to 1km */
      blockage_rng = (int) (bin / 4);

      /* Blockage data only goes out to 230 km so keep using the old value beyond that */
      if (blockage_rng >= BLOCK_RNG) blockage_rng = BLOCK_RNG-1;

      /* Obtain the beam blockage value for each bin, convert from char to int.
       * NOTE: blocked_percent is an integer percentage (i.e 4 = 4%). */
      blocked_percent = (int) Beam_Blockage[blockage_azm][blockage_rng];

      if (blocked_percent > Min_blockage_thresh) {
         /* Compute the FShield adjustment based on blockage percentage */
         fshield = (0.5 * tanhf(0.0277 * (50.0 - blocked_percent))) + 0.5;

         /* Apply the correction.  Note that the correction is negative so to make
            the adjustment it needs to be subtracted from the original reflectivity. */
         z_fshield = data.smz[bin] - (10.0 * log10f(fshield));
      }
      else {
         /* Don't apply the FShield adjustment if blockage is below the threshold */
         z_fshield = data.smz[bin];
      }

/*if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN) && (bin <= DB_EBIN)) {
  RPGC_log_msg(GL_INFO,"az= %f bk_az=%d bk_rg=%d blocked_pct=%d smz[%d]=%f fshield=%f z_fshield=%f zdr=%f cc=%f",
             bh->azimuth,blockage_azm, blockage_rng,blocked_percent, bin, data.smz[bin], fshield, z_fshield, data.zdr[bin], data.rho[bin]);
}*/

      for(h_class=0;h_class<NUM_CLASSES;h_class++)
         agg[h_class]=0.0;

      if (data.snr[bin] < hca_adapt.min_snr){
         data.hca[bin] = NE;
      }
      else if ((data.zdr[bin] == HCA_RF_DATA) || (data.rho[bin] == HCA_RF_DATA) ||
               (data.phi[bin] == HCA_RF_DATA)) {
         data.hca[bin] = UK;
      }
      else {
         /* Determine which classes are allowed at this bin height relative to
            the melting layer height.  Disallowed classes have their aggregate
            totals set to INVALID_CLASS.                                       */
         /* CPT&E label B */
         Hca_allowedHydroClass(bin, data.smz[bin], data.zdr[bin], data.rho[bin],
                               data.phi[bin], data.smv[bin], atten_rad, agg, Melting_layer);

/*    if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN)){
     RPGC_log_msg(GL_INFO, "agg[RA]=%6.3f [HR]=%6.3f [RH]=%6.3f [BD]=%6.3f [BI]=%6.3f [GC]=%6.3f [DS]=%6.3f [WS]=%6.3f [IC]=%6.3f, [GR]=%6.3f",
                             agg[RA],agg[HR],agg[RH],agg[BD],agg[BI],agg[GC],agg[DS],
                             agg[WS],agg[IC],agg[GR]);
    }*/

         for(h_class = 0; h_class < NUM_CLASSES; h_class++){

            /* If this class isn't allowed at this height or because of
               some other hard threshold, skip to next bin */
            if (agg[h_class] == INVALID_CLASS) {
               agg[h_class] = 0.0;
               continue;
            }

            for(fl_input = 0; fl_input < NUM_FL_INPUTS; fl_input++) {
               /* Compute (Membership Function (Hydro Class Type)-- X1, X2, X3, X4 */
               /* CPT&E label C */
               Hca_setMembershipPoints(h_class, fl_input, z_fshield, points);

/*               if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN))
                 RPGC_log_msg(GL_INFO,"h_class = %d fl_input = %d z=%f  z_index = %d",
                                       h_class, fl_input,data.smz[bin],z_index); */

               /* For each fuzzy logic input, compute Degree of Membership, F(Dj) */
               switch( fl_input ){
                 case SMZ: /* CPT&E label D1 */
                   fd_mem[fl_input] = Hca_degreeMembership(z_fshield, points);        

/*                   if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN)){
                      RPGC_log_msg(GL_INFO,"bin %d class %d smz= %f snr= %f z_index= %d fd_mem[SMZ] = %f",
                           bin,h_class,data.smz[bin],data.snr[bin],z_index,fd_mem[SMZ]);
                      RPGC_log_msg(GL_INFO,"points = %f %f %f %f",points[0],points[1],points[2],
                                                                  points[3]);
                   }*/

                   break;
                 case ZDR: /* CPT&E label D2 */
                   fd_mem[fl_input] = Hca_degreeMembership(data.zdr[bin], points);

/*                   if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN)){
                      RPGC_log_msg(GL_INFO,"bin %d class %d zdr= %f fd_mem[ZDR] = %f",
                                            bin,h_class,data.zdr[bin],fd_mem[ZDR]);
                      RPGC_log_msg(GL_INFO,"points = %f %f %f %f",points[0],points[1],points[2],
                                                                  points[3]);
                   }*/

                   break;
                 case LKDP:
                   /* Input Kdp data is put in a log scale (LKdp) prior to processing */
                   /* CPT&E label D3 */
                   if (data.kdp[bin] >= 0.001 )
                     data.lkdp[bin] = 10.0*log10(data.kdp[bin]);
                   else
                     data.lkdp[bin] = MINI_LKTP;
                   /* CPT&E label D4 */
                   fd_mem[fl_input] = Hca_degreeMembership(data.lkdp[bin], points);

/*                   if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN)){
                      RPGC_log_msg(GL_INFO,"bin %d class %d lkdp= %f kdp = %f phi = %f fd_mem[LKDP] = %f",
                                bin,h_class,data.lkdp[bin],data.kdp[bin],data.phi[bin],fd_mem[LKDP]);
                      RPGC_log_msg(GL_INFO,"points = %f %f %f %f",points[0],points[1],points[2],
                                                                  points[3]);
                   }*/

                   break;
                 case RHO: /* CPT&E label D5 */
                   fd_mem[fl_input] = Hca_degreeMembership(data.rho[bin], points);

 /*                  if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN))
                      RPGC_log_msg(GL_INFO,"bin %d class %d rho= %f fd_mem[RHO] = %f",
                                            bin,h_class,data.rho[bin],fd_mem[RHO]);*/

                   break;
                 case SDZ: /* CPT&E label D6 */
                   fd_mem[fl_input] = Hca_degreeMembership(data.sdz[bin], points);

/*                   if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN))
                      RPGC_log_msg(GL_INFO,"bin %d class %d sdz= %f fd_mem[SDZ] = %f",
                                            bin,h_class,data.sdz[bin],fd_mem[SDZ]);*/

                   break;
                 case SDP: /*CPT&E label D7 */
                   fd_mem[fl_input] = Hca_degreeMembership(data.sdp[bin], points);

/*                   if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN))
                      RPGC_log_msg(GL_INFO,"bin %d class %d sdp= %f phi=%f fd_mem[SDP] = %f",
                                            bin,h_class,data.sdp[bin],data.phi[bin],fd_mem[SDP]);*/

                   break;
                 default:
                   break;
               } /* end switch */

               /* Each input field is assigned a weight (0 to 1), based on hydro class.  */
               weight_i[fl_input] = weight[fl_input][h_class];

            }/*END of for(fl_input = 0; ... */ 

            /* Each input field has a corresponding quality index (0 to 1) computed
               by the Quality Index Algorithm.                                      */
            quality_i[SMZ]  = data.qsmz[bin];
            quality_i[ZDR]  = data.qzdr[bin];
            quality_i[LKDP] = data.qkdp[bin];
            quality_i[RHO]  = data.qrho[bin];
            quality_i[SDZ]  = data.qsdz[bin];
            quality_i[SDP]  = data.qsdp[bin];

            /* For this hydro class compute the weighted membership aggregation of
               each fuzzy logic input. */
            /* CPT&E label E */
            Hca_weightedMembershipAggregation(weight_i, quality_i, fd_mem, &agg[h_class]);
         }/*END of for(h_class =0;...*/ 


/*         if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN)){
          RPGC_log_msg(GL_INFO,"bin %d smv= %f",bin,data.smv[bin]);
           RPGC_log_msg(GL_INFO,
           "agg[RA]=%6.3f [HR]=%6.3f [RH]=%6.3f [BD]=%6.3f [BI]=%6.3f [GC]=%6.3f [DS]=%6.3f [WS]=%6.3f [IC]=%6.3f, [GR]=%6.3f",
           agg[RA],agg[HR],agg[RH],agg[BD],agg[BI],agg[GC],agg[DS], agg[WS],agg[IC],agg[GR]);
           RPGC_log_msg(GL_INFO, "Bin above is %d at radial %f degrees", bin,bh->azimuth);
         }*/


         /* The hydro class with the largest aggregation is selected for this bin. */  
         /* CPT&E label F */
         agg_max = -2.0; 
         diff = 1.0;
         top_diff = 100.;

         for (h_class = 0; h_class < NUM_CLASSES; h_class++) {
            if(agg_max < agg[h_class]){
               agg_max = agg[h_class];
               data.hca[bin] = class_type[h_class];
               max_cal = h_class;
            }/*END of if(agg_max < ... */ 
         }/* END of for (h_class = 0; ... */

         for (h_class = 0; h_class < NUM_CLASSES; h_class++) {
            if(h_class !=max_cal){
               diff = agg_max - agg[h_class];
               if(diff < top_diff)
                  top_diff = diff;
            }/*END of if(h_class!= < ... */
         }/* END of for (h_class = 0; ... */

         /* Must have at least min_Agg and an aggregate score more than 0.001 over
            the next highest class or else we assign "Unknown." */
         if (agg_max < hca_adapt.min_Agg || top_diff < hca_adapt.min_Dif_Agg)
            data.hca[bin] = UK;

      }/*END of else Z above SNR */     

/*      if((bh->azi_num == DB_AZ) && (bin >= DB_SBIN && bin <= DB_EBIN))
        RPGC_log_msg(GL_INFO,"******HYDRO CLASS bin %d = %f",bin,data.hca[bin]);*/

#ifdef HCA_ZDR_ERROR_ESTIMATE
      /* Debug for average ZDR in dry snow above melting layer, an indication of calibration?? */
      /* Limit search to 1 km above top of melting layer to avoid ice crystal contamination.   */
      oneKMabovelimit = 4*(Compute_range_from_height(bh->elevation, ML_top[(int)bh->azimuth] + 1.0));
      if ((bin > data.ml[MLTT])  && bin < oneKMabovelimit &&
          (data.hca[bin] == DS) && 
          (data.zdr[bin] != HCA_NO_DATA) && (data.zdr[bin] != HCA_RF_DATA) &&
          (data.smz[bin] != HCA_NO_DATA) && (data.smz[bin] != HCA_RF_DATA) &&
          (data.smz[bin] >= 15. && data.smz[bin] <= 25.) &&
          (data.rho[bin] < 1.0)          && (data.rho[bin] > 0.98) &&
          (data.snr[bin] >= 20.0) &&
          (bh->elevation > 1.0) &&
          (data.phi[bin] < 100) &&
          (!atten_rad) &&
          (blocked_percent < 20)){
        DS_ZDR[DS_count] = data.zdr[bin];
	DS_count++;
        DS_ZDR_sum += data.zdr[bin];
	DS_ZDR_sum_sqr += (data.zdr[bin] * data.zdr[bin]);
        DS_Z_sum += data.smz[bin];
        if(data.kdp[bin] > 0.0) {
           DS_KDP_sum += data.kdp[bin];
           KDP_count++;
        }
      }

      exp_date_DS = bh->date;
      DS_VCP = bh->vcp_num; /* should be the same for all methods so just save it */
      DS_beg_vol_time = bh->begin_vol_time; /* should all be the same so just save it */

      /* Ryzhkov (Rain) section */
      /* Limit search to 1 km below bottom of melting layer to avoid frozen particle contamination */
      /* Also limit search to beyond 20 km from radar (bin 80) to avoid ground clutter.            */
      oneKMbelowlimit = 4*(Compute_range_from_height(bh->elevation, ML_bottom[(int)bh->azimuth] - 1.0));
      if ((bin < oneKMbelowlimit) && (bin > 80) &&
        /*  (data.hca[bin] == RA) &&*/ /* This line adds HCA input for help in determining rain */
          (data.zdr[bin] != HCA_NO_DATA) && (data.zdr[bin] != HCA_RF_DATA) &&
          (data.smz[bin] != HCA_NO_DATA) && (data.smz[bin] != HCA_RF_DATA) &&
          (data.rho[bin] > 0.98) &&
          (bh->elevation > 1.0) &&
          (data.snr[bin] > 20.0)){

          if (data.smz[bin] > 18.5 && data.smz[bin] <= 20.5) {
             AR_ZDR[0][AR_count[0]] = data.zdr[bin];
             AR_count[0]++;
             AR_ZDR_sum[0] += data.zdr[bin];
	     AR_ZDR_sum_sqr[0] += (data.zdr[bin] * data.zdr[bin]);
             AR_Z_sum[0] += data.smz[bin];
          }
          else if (data.smz[bin] > 20.5 && data.smz[bin] <= 22.5) {
             AR_ZDR[1][AR_count[1]] = data.zdr[bin];
             AR_count[1]++;
             AR_ZDR_sum[1] += data.zdr[bin];
	     AR_ZDR_sum_sqr[1] += (data.zdr[bin] * data.zdr[bin]);
             AR_Z_sum[1] += data.smz[bin];
          }
          else if (data.smz[bin] > 22.5 && data.smz[bin] <= 24.5) {
             AR_ZDR[2][AR_count[2]] = data.zdr[bin];
             AR_count[2]++;
             AR_ZDR_sum[2] += data.zdr[bin];
	     AR_ZDR_sum_sqr[2] += (data.zdr[bin] * data.zdr[bin]);
             AR_Z_sum[2] += data.smz[bin];
          }
          else if (data.smz[bin] > 24.5 && data.smz[bin] <= 26.5) {
             AR_ZDR[3][AR_count[3]] = data.zdr[bin];
             AR_count[3]++;
             AR_ZDR_sum[3] += data.zdr[bin];
	     AR_ZDR_sum_sqr[3] += (data.zdr[bin] * data.zdr[bin]);
             AR_Z_sum[3] += data.smz[bin];
          }
          else if (data.smz[bin] > 26.5 && data.smz[bin] <= 28.5) {
             AR_ZDR[4][AR_count[4]] = data.zdr[bin];
             AR_count[4]++;
             AR_ZDR_sum[4] += data.zdr[bin];
	     AR_ZDR_sum_sqr[4] += (data.zdr[bin] * data.zdr[bin]);
             AR_Z_sum[4] += data.smz[bin];
          }
          else if (data.smz[bin] > 28.5 && data.smz[bin] <= 30.5) {
             AR_ZDR[5][AR_count[5]] = data.zdr[bin];
             AR_count[5]++;
             AR_ZDR_sum[5] += data.zdr[bin];
	     AR_ZDR_sum_sqr[5] += (data.zdr[bin] * data.zdr[bin]);
             AR_Z_sum[5] += data.smz[bin];
          }
      }
      exp_date_AR = bh->date;
      AR_VCP = bh->vcp_num; /* should be the same for all methods so just save it */
      AR_beg_vol_time = bh->begin_vol_time; /* should all be the same so just save it */
#endif

   }/*END of for (bin = 0; bin < params.n_dgates; bin++) */

   /* Build the output radial */
   *length = Format_output_radial (bh, &params, &data, &out_radial);

   *output = out_radial;
   bh = (Base_data_header *)out_radial;

   /* Free the input memory allocated by RPGCS_radar_data_conversion() */
   free(data.smz);
   free(data.zdr);
   free(data.rho);
   free(data.kdp);
   free(data.sdz);
   free(data.sdp);
   free(data.phi);
   free(data.snr);
   free(data.smv);
   free(data.qsmz);
   free(data.qzdr);
   free(data.qrho);
   free(data.qkdp);
   free(data.qsdz);
   free(data.qsdp);
   return (bh->status);
}/* End of Hca_process_radial() */



/******************************************************************

    Extracts, converts and returns all data fields from radial "bh".
    The data fields are converted to type float in physical units. 
    This function returns 0 on success or -1 on failure (e.g. not all
    required fields are found or gate sizes do not match).
	
******************************************************************/

static int Get_data_fields (Hca_params_t *params, Base_data_header *bh,
                            Hca_data_t *data) {
    static int         buf_size = 0;
    static float      *buf = NULL;
    int n_all_gates, cnt, i;
    int ngates = 0, range = 0, bin_size = 0;
    unsigned char     *dp_data;
    unsigned short    *dp16_data;
    Generic_moment_t  *gm = NULL;

    /* Extract number of bins, gate size and range to center of first gate for
       the survellance and Doppler moments.                                      */
    params->n_zgates = bh->n_surv_bins;
    params->zg_size = (float)(.001) * bh->surv_bin_size;
    params->zr0 = (float)(.001) * bh->range_beg_surv + (float)(.5) * params->zg_size;

    params->n_vgates = bh->n_dop_bins;
    params->vg_size = (float)(.001) * bh->dop_bin_size;
    params->vr0 = (float)(.001) * bh->range_beg_dop + (float)(.5) * params->vg_size;

    /* Next, extract gate size and range to center of first gate for the dual pol fields. */
    cnt = 0;
    for (i = 0; i < bh->no_moments; i++) {	/* get DP field gate info */

	gm = (Generic_moment_t *)((char *)bh + bh->offsets[i]);
	if (strcmp (gm->name, "DZDR") == 0 ||
	    strcmp (gm->name, "DPHI") == 0 ||
	    strcmp (gm->name, "DRHO") == 0 ||
	    strcmp (gm->name, "DKDP") == 0 ||
         strcmp (gm->name, "DSMZ") == 0 ||
         strcmp (gm->name, "DSMV") == 0 ||
         strcmp (gm->name, "DSNR") == 0 ||
         strcmp (gm->name, "DSDZ") == 0 ||
         strcmp (gm->name, "DSDP") == 0 ||
         strcmp (gm->name, "DQSZ") == 0 ||
         strcmp (gm->name, "DQZD") == 0 ||
         strcmp (gm->name, "DQRO") == 0 ||
         strcmp (gm->name, "DQKD") == 0 ||
         strcmp (gm->name, "DQTZ") == 0 ||
         strcmp (gm->name, "DQTP") == 0 ){

	    if (cnt == 0) {
		ngates = gm->no_of_gates;
		range = gm->first_gate_range;
		bin_size = gm->bin_size;
          params->dr0 = (float)(.001) * range;
          params->n_dgates = ngates;
          params->dg_size = (float)(.001) * bin_size;
	    } /* end if */

	    cnt++;  /* Count the generic moments */

	} /* end if */
    } /*end for all generic moments */

    if (cnt == 0) {
	/* No dual-pol field found - return without processing */
	  Print_unique_msg ("No dual-pol fields found-Output without processing");
	return (-1);
    }
    if (params->n_vgates == 0) {
	/* No velocity field found - return without processing */
	  Print_unique_msg ("No velocity field found-Output without processing");
	return (-1);
    }
    if (cnt != 15) {
	  Print_unique_msg ("Unexpected number of dual-pol fields found-Output without processing");
	return (-1);
    }
    Print_unique_msg ("Dual-pol fields found-Processing");

    /* Allocate buffers for the internal log Kdp field, the four melting layer
       values (floats) and the one output field computed by this algorithm.
       The preprocessor allocates one more byte than need.                       */

    n_all_gates = (2 * (params->n_dgates + 1)) + (4 * sizeof(float));
    if (n_all_gates > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = MISC_malloc (n_all_gates * sizeof (float));
	buf_size = n_all_gates;
    }

    /* Set the pointers to their allocated memory */
    data->lkdp          = buf;
    data->hca           = data->lkdp + (params->n_dgates + 1);
    data->ml            = data->hca  + (params->n_dgates + 1);

    /* The preprocessor sets the last bin to a flag value. */

    data->hca[params->n_dgates]  = HCA_NO_DATA;
    data->lkdp[params->n_dgates] = HCA_NO_DATA;

    /* Read fuzzy logic input data and convert to internal format (float) */
    dp_data = (unsigned char *) RPGC_get_radar_data((void *)bh, RPGC_DZDR, &Zdr_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No Zdr data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&Zdr_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->zdr));

    dp_data = (unsigned char *) RPGC_get_radar_data((void *)bh, RPGC_DSMZ, &Smz_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No Smz data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&Smz_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->smz));

    dp16_data = (unsigned short *) RPGC_get_radar_data((void *)bh, RPGC_DKDP, &Kdp_hd);
    if (dp16_data == NULL)RPGC_log_msg(GL_ERROR," No Kdp data!!!");
    RPGCS_radar_data_conversion((void*)dp16_data,&Kdp_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->kdp));

    dp16_data = (unsigned short *) RPGC_get_radar_data((void *)bh, RPGC_DRHO, &Rho_hd);
#ifdef CC_SNR_ANALYSIS
/*fprintf(stderr,"Processing CC, ngates=%d",Rho_hd.no_of_gates);*/
    if (bh->azi_num == DB_AZ) fprintf(stderr,"cc scale=%f offset=%f size=%d",
                              Rho_hd.scale,Rho_hd.offset,Rho_hd.data_word_size);
    for (x = 0; x < Rho_hd.no_of_gates; x++){
       cc[x]=dp16_data[x]/256;
    }
/*fprintf(stderr,"Done CC\n");*/
#endif
    if (dp16_data == NULL)RPGC_log_msg(GL_ERROR," No Rho data!!!");
    RPGCS_radar_data_conversion((void*)dp16_data,&Rho_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->rho));

    dp_data = (unsigned char *) RPGC_get_radar_data((void *)bh, RPGC_DSDZ, &Sdz_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No Sdz data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&Sdz_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->sdz));

    dp_data = (unsigned char *) RPGC_get_radar_data((void *)bh, RPGC_DSDP, &Sdp_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No Sdp data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&Sdp_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->sdp));

    /* Read quality index input data and convert to internal format (float) */
    strcpy(QSmz_hd.name,"DQSZ");
    dp_data = (unsigned char *)RPGC_get_radar_data((void *)bh, RPGC_DANY, &QSmz_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No QSmz data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&QSmz_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->qsmz));

    strcpy(QZdr_hd.name,"DQZD");
    dp_data = (unsigned char *)RPGC_get_radar_data((void *)bh, RPGC_DANY, &QZdr_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No QZdr data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&QZdr_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->qzdr));

    strcpy(QRho_hd.name,"DQRO");
    dp_data = (unsigned char *)RPGC_get_radar_data((void *)bh, RPGC_DANY, &QRho_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No QRho data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&QRho_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->qrho));

    strcpy(QKdp_hd.name,"DQKD");
    dp_data = (unsigned char *)RPGC_get_radar_data((void *)bh, RPGC_DANY, &QKdp_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No QKdp data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&QKdp_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->qkdp));

    strcpy(QSdz_hd.name,"DQTZ");
    dp_data = (unsigned char *)RPGC_get_radar_data((void *)bh, RPGC_DANY, &QSdz_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No QSdz data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&QSdz_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->qsdz));

    strcpy(QSdp_hd.name,"DQTP");
    dp_data = (unsigned char *)RPGC_get_radar_data((void *)bh, RPGC_DANY, &QSdp_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No QSdp data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&QSdp_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->qsdp));

    /* Read the remaining input data and convert (Note that Phi has a 16 bit word size) */
    dp_data = (unsigned char *) RPGC_get_radar_data((void *)bh, RPGC_DSNR, &Snr_hd);
#ifdef CC_SNR_ANALYSIS
    for (x = 0; x < 1200; x++){
       snr[x]=dp_data[x];
/*       if (x < 100) fprintf(stderr,"snr[%d]=%d ",x,snr[x]);*/
    }
/*fprintf(stderr,"Done SNR\n");*/
#endif
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No Snr data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&Snr_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->snr));

    dp16_data = (unsigned short *) RPGC_get_radar_data((void *)bh, RPGC_DPHI, &Phi_hd);
    if (dp16_data == NULL)RPGC_log_msg(GL_ERROR," No Phi data!!!");
    RPGCS_radar_data_conversion((void*)dp16_data,&Phi_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->phi));

    dp_data = (unsigned char *) RPGC_get_radar_data((void *)bh, RPGC_DSMV, &Smv_hd);
    if (dp_data == NULL)RPGC_log_msg(GL_ERROR," No Smv data!!!");
    RPGCS_radar_data_conversion((void*)dp_data,&Smv_hd,HCA_NO_DATA,HCA_RF_DATA,&(data->smv));

/**START DEBUG CODE**/

   if (bh->azi_num == DB_AZ){
     int start = DB_SBIN;
	RPGC_log_msg(GL_INFO,"First radial of elevation, bins %d to %d",DB_SBIN,
                           DB_SBIN+4);

     RPGC_log_msg(GL_INFO,"q_smz:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data->qsmz[0+start],data->qsmz[1+start],data->qsmz[2+start],data->qsmz[3+start],data->qsmz[4+start]);
     RPGC_log_msg(GL_INFO,"q_rho:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data->qrho[0+start],data->qrho[1+start],data->qrho[2+start],data->qrho[3+start],data->qrho[4+start]);
     RPGC_log_msg(GL_INFO,"q_zdr:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data->qzdr[0+start],data->qzdr[1+start],data->qzdr[2+start],data->qzdr[3+start],data->qzdr[4+start]);
     RPGC_log_msg(GL_INFO,"q_kdp:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data->qkdp[0+start],data->qkdp[1+start],data->qkdp[2+start],data->qkdp[3+start],data->qkdp[4+start]);
     RPGC_log_msg(GL_INFO,"q_sdz:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data->qsdz[0+start],data->qsdz[1+start],data->qsdz[2+start],data->qsdz[3+start],data->qsdz[4+start]);
     RPGC_log_msg(GL_INFO,"q_sdp:");
     RPGC_log_msg(GL_INFO,"%6.3f %6.3f %6.3f %6.3f %6.3f",
        data->qsdp[0+start],data->qsdp[1+start],data->qsdp[2+start],data->qsdp[3+start],data->qsdp[4+start]);
    }

/* END DEBUG CODE */
    return (0);
} /* End Get_data_fields() */


/********************************************************************* 

   Description:
      Calculate aggregation value which is a function of membership
      function and its weight.
   Inputs:
   Returns:

******************************************************************* */
/* CPT&E label G */
void Hca_weightedMembershipAggregation(float weight[NUM_FL_INPUTS],
                                       float quality_i[NUM_FL_INPUTS],
                                       float fd_mem[NUM_FL_INPUTS],
                                       float *agg){
    
 /* Local variables */
 int fl_input; /* Loop index for FL Inputs */
 float s = 0.0; /* sum of W*Q */
 float sfd = 0.0; /* sum of W*Q*F(d) */


 for(fl_input = 0; fl_input < NUM_FL_INPUTS; fl_input++) {
  s += (weight[fl_input]*quality_i[fl_input]);
 }/* END of for(fl_input = 0;...*/


 for(fl_input = 0; fl_input < NUM_FL_INPUTS; fl_input++) {
  sfd += (weight[fl_input]*quality_i[fl_input]*fd_mem[fl_input])/(s+0.01); 
 }/* END of for(fl_input = 0;...*/

 *agg = sfd;

} /* End of Hca_weightedMembershipAggregation() */

/******************************************************************
 Description:
    Create the output radial of the required dual pol and HCA data.
    The input radial is "bh", the processing parameters are "params",
    the processed fields are in "data" and the pointer to the output
    radial is returned with "out_radial". The function returns the 
    size of the output radial.
	
******************************************************************/

static int Format_output_radial (Base_data_header *bh, Hca_params_t *params,
		Hca_data_t *data, char **out_radial) {
    static int buf_size = 0;
    static char *buf = NULL;
    int size1, out_size, z_size, v_size, d_size, ml_size;
    int rho_size, phi_size, zdr_size, kdp_size, sdz_size, sdp_size;

    Base_data_header *ohd;

    /* Size of the first part - basic 3 moments */
    size1 = ALIGNED_SIZE (bh->offsets[0]);

    /* Compute size, in bytes, of each additional output field */
    z_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + params->n_zgates);
    v_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + params->n_vgates);
    d_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + params->n_dgates);
    ml_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 4*sizeof(float));
    rho_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Rho_hd.data_word_size / 8) * params->n_dgates);
    kdp_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Kdp_hd.data_word_size / 8) * params->n_dgates);
    phi_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Phi_hd.data_word_size / 8) * params->n_dgates);
    zdr_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Zdr_hd.data_word_size / 8) * params->n_dgates);
    sdz_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Sdz_hd.data_word_size / 8) * params->n_zgates);
    sdp_size = ALIGNED_SIZE (sizeof (Generic_moment_t) + 
			(Sdp_hd.data_word_size / 8) * params->n_dgates);

    /* Seven 8-bit fields and four potential 16-bit fields(PHI, RHO, KDP and ML).
       Of these eleven fields, two are z_size (SMZ and SNR), one is v_size (SMV),
       one is d_size (HCA), six are sized on whatever is input (PHI, RHO, KDP,
       ZDR, SDZ and SDP), and one is ml_size (ML)    */
    out_size = size1 + z_size + z_size + v_size + d_size +
               phi_size + rho_size + kdp_size + zdr_size + sdz_size + sdp_size +
               ml_size;

    if (out_size > buf_size) {
	if (buf != NULL)
	    free (buf);
	buf = MISC_malloc (out_size);
	buf_size = out_size;
    }
#ifdef HCA_ZDR_ERROR_ESTIMATE
#ifdef HCA_ZDR_ERROR_DYNAMIC_ADJUST
    /* ZDR error correction ***TEST CODE***/
    int i;
    if (( (exp_date_DS == bh->date) && (error_exp_time_DS > bh->begin_vol_time) ) ||
          (exp_date_DS == (bh->date+1))) {
       for (i=0;i<params->n_dgates;i++){
          if (data->zdr[i] > HCA_NO_DATA && ZDR_avg_error_DS != -99.0) data->zdr[i] += ZDR_avg_error_DS;
       }
    } /* END TEST CODE */
#endif
#endif

    /* Flag the first displayable bin as an indicator of the
       NBF/Attenuation fix ON or OFF setting */
    if (hca_adapt.max_Z_BI == 60) data->hca[8] = BI; /* fix must be off */

    memcpy (buf, (char *)bh, bh->offsets[0]);
    ohd = (Base_data_header *)buf;
    ohd->msg_type |= PREPROCESSED_DUALPOL_TYPE;
    ohd->no_moments = (short)11;

    ohd->offsets[0] = size1;
    Add_moment (buf + size1, "DRHO", 'd', params, Rho_hd.data_word_size, Rho_hd.tover,
                Rho_hd.SNR_threshold, Rho_hd.scale, Rho_hd.offset, data->rho);
    size1 += rho_size;

    ohd->offsets[1] = size1;
    Add_moment (buf + size1, "DZDR", 'd', params, Zdr_hd.data_word_size, Zdr_hd.tover,
                Zdr_hd.SNR_threshold, Zdr_hd.scale, Zdr_hd.offset, data->zdr);
    size1 += zdr_size;

    ohd->offsets[2] = size1;
    Add_moment (buf + size1, "DSMZ", 'z', params, Smz_hd.data_word_size, Smz_hd.tover,
                Smz_hd.SNR_threshold, Smz_hd.scale, Smz_hd.offset, data->smz);
    size1 += z_size;

    ohd->offsets[3] = size1;
    Add_moment (buf + size1, "DSNR", 'z', params, Snr_hd.data_word_size, Snr_hd.tover,
                Snr_hd.SNR_threshold, Snr_hd.scale, Snr_hd.offset, data->snr);
    size1 += z_size;

    ohd->offsets[4] = size1;
    Add_moment (buf + size1, "DSMV", 'v', params, Smv_hd.data_word_size, Smv_hd.tover,
                Smv_hd.SNR_threshold, Smv_hd.scale, Smv_hd.offset, data->smv);
    size1 += v_size;

    ohd->offsets[5] = size1;
    Add_moment (buf + size1, "DKDP", 'd', params, Kdp_hd.data_word_size, Kdp_hd.tover,
                Kdp_hd.SNR_threshold, Kdp_hd.scale, Kdp_hd.offset, data->kdp);
    size1 += kdp_size;

    ohd->offsets[6] = size1;
    Add_moment (buf + size1, "DHCA", 'h', params, ONE_BYTE, 0,
                0, HCA_SCALE, HCA_OFFSET, data->hca);
    size1 += d_size;

    ohd->offsets[7] = size1;
    Add_moment (buf + size1, "DPHI", 'd', params, Phi_hd.data_word_size, Phi_hd.tover,
                Phi_hd.SNR_threshold, Phi_hd.scale, Phi_hd.offset, data->phi);
    size1 += phi_size;

    ohd->offsets[8] = size1;
    /* Note: Melting layer data is in bin number units. A floating point data type is used only*
     *       for compatibility with the Add_moment */
    Add_moment (buf + size1, "DML", 'm', params, TWO_BYTE, 0,
                0, ML_SCALE, ML_OFFSET, data->ml);
    size1 += ml_size;

    ohd->offsets[9] = size1;
    Add_moment (buf + size1, "DSDZ", 'z', params, Sdz_hd.data_word_size, Sdz_hd.tover,
                Sdz_hd.SNR_threshold, Sdz_hd.scale, Sdz_hd.offset, data->sdz);
    size1 += sdz_size;

    ohd->offsets[10] = size1;
    Add_moment (buf + size1, "DSDP", 'd', params, Sdp_hd.data_word_size, Sdp_hd.tover,
                Sdp_hd.SNR_threshold, Sdp_hd.scale, Sdp_hd.offset, data->sdp);
    size1 += sdp_size;
    
/*  Update the message length field of the base data header (its in shorts) */
    ohd->msg_len = size1 / sizeof(short);

    *out_radial = buf;
    return (size1);
} /* End of Format_output_radial() */

/******************************************************************
 Description:
    Creates a moment field of name "name" in "buf". The data is in
    "data". Other parameters provide the additional info for the 
    field.
	
******************************************************************/

static void Add_moment (char *buf, char *name, char f, Hca_params_t *params, 
	int word_size, unsigned short tover, short SNR_threshold, 
	float scale, float offset, float *data) {

    Generic_moment_t *hd;
    int i, up, low, n_gates;
    unsigned short *spt;
    unsigned char  *cpt;
    float dscale, doffset;

    hd = (Generic_moment_t *)buf;
    strcpy (hd->name, name);
    hd->info = 0;
    low = 0;
    n_gates = 0;
    if (f == 'z') {
	n_gates = params->n_zgates;
	hd->first_gate_range = RPGC_NINT((float)(params->zr0) * KMTOM);
	hd->bin_size = RPGC_NINT((float)(params->zg_size) * KMTOM);
     low = 2;
    }
    else if (f == 'd') {
	n_gates = params->n_dgates;
	hd->first_gate_range = RPGC_NINT((float)(params->dr0) * KMTOM);
	hd->bin_size = RPGC_NINT((float)(params->dg_size) * KMTOM);
     low = 2;
    }
    else if (f == 'v') {
	n_gates = params->n_vgates;
	hd->first_gate_range = RPGC_NINT((float)(params->vr0) * KMTOM);
	hd->bin_size = RPGC_NINT((float)(params->vg_size) * KMTOM);
     low = 2;
    }
    else if (f == 'h'){
	n_gates = params->n_dgates;
	hd->first_gate_range = RPGC_NINT((float)(params->dr0) * KMTOM);
	hd->bin_size = RPGC_NINT((float)(params->dg_size) * KMTOM);
     low = 0;
    }
    else if (f == 'm'){
	n_gates = 4;
	hd->first_gate_range = RPGC_NINT((float)(params->dr0) * KMTOM);
	hd->bin_size = RPGC_NINT((float)(params->dg_size) * KMTOM);
     low = 0;
    }

    hd->no_of_gates = n_gates;
    hd->tover = tover;
    hd->SNR_threshold = SNR_threshold;
    hd->control_flag = 0;
    hd->data_word_size = word_size;
    hd->scale = scale;
    hd->offset = offset;

    if (word_size == TWO_BYTE)
	up = 0xffff;
    else 
	up = 0xff;

    spt = hd->gate.u_s;
    cpt = hd->gate.b;
    dscale = scale;
    doffset = offset;

    for (i = 0; i < n_gates; i++) {
	int t;

	if (data[i] == HCA_NO_DATA)
	    t = 0;
        else if (data[i] == HCA_RF_DATA)
            t = 1;
	else {
	    float f;
	    f = data[i] * dscale + doffset;
	    if (f >= C0)
		t = f + Cp5;
	    else
		t = -(-f + Cp5);
	    if (t > up)
		t = up;
	    if (t < low)
		t = low;
	}
	if (word_size == TWO_BYTE)
	    spt[i] = (unsigned short)t;
	else
	    cpt[i] = (unsigned char)t;
    }
} /* End of Add_moment() */

/******************************************************************************
     Description:  Prints "msg" if it is not the same as the previous one.
     Input:    msg - message string to be printed.
     Output:   None.
     Returns:  Nothing.
     Notes:    Prints to task's log file.
******************************************************************************/
static void Print_unique_msg (char *msg) {
    static char *prev = NULL;

    if (prev != NULL && strcmp (msg, prev) == 0)
	return;
    if (msg[0] != '\0')
	LE_send_msg (GL_INFO, "%s", msg);
    prev = STR_copy (prev, msg);
} /* end of Print_unique_msg */

#ifdef HCA_ZDR_ERROR_ESTIMATE
/******************** Compute_range_from_height() *******************
**
**	Description:
**	Inputs:		elev - elevation angle in degree
**			height - height in km
**	Outputs:	
**	Return:		slant range(km) calculated from inputs.
**	Globals:	
**	Notes:
*/

static float Compute_range_from_height(float elev, float height)
{
  float sin_elev = sin(elev * DEG_TO_RAD);
  float range;
  const float IR = 1.21;
  const float RE = 6371;  /* earth radius in km */

    /* (CPT&E Label H) */
  range = IR * RE * (sqrt(sin_elev*sin_elev + 2*height/(IR * RE)) - sin_elev); 

  return range; 
}  /* end of Compute_range_from_height() */
#endif
