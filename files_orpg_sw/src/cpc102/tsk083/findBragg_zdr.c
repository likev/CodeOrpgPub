/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:29:32 $
 * $Id: findBragg_zdr.c,v 1.3 2014/11/07 21:29:32 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/*============================================================
 *
 * Module Name: findBragg.c 
 *
 * Module Version: 1.1
 *
 * Module Language: c
 *
 * Change History:
 *
 * Date    Version    Programmer  Notes
 * -------------------------------------------------------------
 * 04/25/14  1.0    Brian Klein  Creation
 * 09/30/14  1.1    Brian Klein  Eliminated highest and lowest 
 *                                ZDR values from histogram.
 *
 * Description:
 * Algorithm attempts to find the Bragg scattered signal from the data
 * presented. Various filters are applied.  First, the data filters
 * attempt to find radar bins that are a result of Bragg scatter.  Then,
 * a biota filter and precipitation filter is applied to help
 * eliminate volumes that may be contaminated with non-Bragg returns.
 * At the end of each volume scan statistics are computed.
 *
 */
 /*========================================================================*/

/*  Specification Files:  */

#include <float.h>
#include <math.h>
#include <rpgc.h>
#include <rpgcs.h>
#include <orpgsite.h>
#include <orpgred.h>
#include <time.h>
#include "dpprep.h"
#include "findBragg.h"

static int   HistoZDR[BRAGG_VOLUMES][HISTOGRAM_SIZE_ZDR];
static int   HistoPHI[BRAGG_VOLUMES][HISTOGRAM_SIZE_PHI];
static int   HistoZ[HISTOGRAM_SIZE_Z];
static int   bragg_cnt, precip_filter_cnt;
static int   vol = 0;
static int   Volume_bragg_cnt[BRAGG_VOLUMES] = {0,0,0,0,0,0,0,0,0,0,0,0};
static float Volume_zdr_bias[BRAGG_VOLUMES] = {-99.,-99.,-99.,-99.,-99.,-99.,
                                          -99.,-99.,-99.,-99.,-99.,-99.};

extern Bragg_data_t   Bragg_data;
extern int DPP_ZDR_DYNAMIC_ADJUST;
extern float DPP_ZDR_DYNAMIC_ADJUST_VALUE;

/* Function prototypes */
static void  Get_bragg_cfg(Bragg_cfg_t *Bragg_cfg);

/*========================================================================*/

void findBragg(Dpp_params_t *params, 
                      float *Z, 
                      float *V, 
                      float *SPW, 
                      float *RhoHV, 
                      float *Zdr, 
                      float *PhiDP,
                      float *SNR) {

int    i, j, last_bin, total_bins = 0, num_volumes = 0;
int    x25, x75, index_25, index_75, index_pct;
int    iZdr, iPhi, iZ, histoCount;
float  zdr_mode = -99., zdr_bias = -99., z_pct = -99.;
float  zdr_iqr = -99., total_zdr = -99;
int    yy = 1970, mm = 1, dd = 1, hr = 0, min = 0, sec = 0, mills = 0;
double yyd, mmd, ddd, hrd, mind;
char*  buffer;
char   adj_str[15];
int    redundant_type = NO_REDUNDANCY;
int    channel = 1;
Redundant_info_t redundant_info;
time_t curr_time;
struct tm  *t1;
static Bragg_cfg_t bragg_cfg;

int debug_flag = FALSE;

  if (params->status == BEG_VOL)
     Get_bragg_cfg(&bragg_cfg);

  if (params->vcp_num == 32 || params->vcp_num == 21){
    if (params->target_elev >= MIN_BRAGG_ELEV && params->target_elev <= MAX_BRAGG_ELEV) {
       /* Determine the last bin to process. */
       if (params->n_dgates < bragg_cfg.max_bragg_bin) {
          last_bin = params->n_dgates;
       }
       else {
          last_bin = bragg_cfg.max_bragg_bin;
       }
    }
    else {
       /* This elevation is not one we use for Bragg analysis */
       last_bin = 0;
    }

    for (i=bragg_cfg.min_bragg_bin; i<last_bin; i++) {
      /* First, save this integer Z value for use later as a precipitation filter */
      if (Z[i] > DPP_NO_DATA) {
         iZ = Z[i] * Z_SCALE + Z_OFFSET;
         HistoZ[iZ]++;
         precip_filter_cnt++;
      }

      /* Apply data filters characteristic of Bragg echo */
      if ((Z[i]       >  bragg_cfg.min_bragg_z   && Z[i]     < bragg_cfg.max_bragg_z)   &&
          (RhoHV[i]   >  bragg_cfg.min_bragg_rho && RhoHV[i] < bragg_cfg.max_bragg_rho) &&
          (PhiDP[i]   >= bragg_cfg.min_bragg_phi && PhiDP[i] < bragg_cfg.max_bragg_phi) &&
          (fabs(V[i]) >  bragg_cfg.min_bragg_v)  && 
          (V[i]      !=  DPP_NO_DATA             && V[i]    != DPP_ALIASED)             &&
          (SPW[i]     >  bragg_cfg.min_bragg_sw  && SPW[i]   < bragg_cfg.max_bragg_sw)  &&
          (SNR[i]     >  bragg_cfg.min_bragg_snr && SNR[i]   < bragg_cfg.max_bragg_snr)) {

         /* Add to histogram arrays after determining integer data value as index */
         iZdr = Zdr[i] * ZDR_SCALE + ZDR_OFFSET;
         iPhi = PhiDP[i] * PHI_SCALE + PHI_OFFSET;
   
         /* Do not use the highest or lowest integer encoded ZDR values in the  */
         /* histogram.  These bins are "catch-all" bins for extreme values that */
         /* may skew the statistics.                                            */
         if (iZdr >= MIN_ZDR_BIN && iZdr <= MAX_ZDR_BIN) {
            HistoZDR[vol][iZdr]++;
            HistoPHI[vol][iPhi]++;
            bragg_cnt++;  /* Number of valid histogram entries */
         }
      } /* end if Bragg bin */
    } /*end for all bins */
  } /* end if VCP 21 or 32 */

  /* Check if this is a non-operational configuration using the Bragg estimate to */
  /* adjust the ZDr data stream.                                                  */
  if (DPP_ZDR_DYNAMIC_ADJUST == 1)
     DPP_ZDR_DYNAMIC_ADJUST_VALUE = Bragg_data.Last_zdr_bias;

  /* Check if this is a non-operational configuration with a valid adjustment value */
  if (DPP_ZDR_DYNAMIC_ADJUST_VALUE != -99.0) {

     /* Apply the adjustment to the ZDR data stream and say so in the status message */
     for (i = 0;i < params->n_dgates; i++) {
        if (Zdr[i] > DPP_NO_DATA) Zdr[i] -= DPP_ZDR_DYNAMIC_ADJUST_VALUE;
     }
     strcpy(adj_str,"Bias applied");
  }
  else
     strcpy(adj_str,"");

  /* See if this is the last radial of the volume scan. If so, compute   */
  /* results from data collected in the volume scan.                     */

  if (params->status == END_VOL) {
     if (debug_flag) {
        fprintf(stderr,"Reached end of volume %d, bragg_cnt = %d Precip_cnt = %d\n",
                                         vol,bragg_cnt, precip_filter_cnt);
     }

     /* Initialize this volume's estimate to bad data */
     Volume_zdr_bias[vol] = -99;
     Volume_bragg_cnt[vol] = 0;

     /* Get Redundant Type. */
     if( ORPGSITE_get_redundant_data( &redundant_info ) >= 0 )
	redundant_type = redundant_info.redundant_type;

     /* Get channel number. */
     if( (channel = ORPGRDA_channel_num()) < 0 )
	channel = 1;

     if(debug_flag) fprintf(stderr,"Redundant_type = %d, channel = %d\n",redundant_type,channel);

     /*  Apply the first statistics filter (i.e. bin count) */
     if (bragg_cnt >= bragg_cfg.min_bragg_cnt_vol) {

       /* We have enough samples to work with.  Determine the precipitation filter, */
       /* which is currently the FILTER_Z_PCT percentile of Z at FILTER_Z_THRESH  */

       index_pct = precip_filter_cnt * bragg_cfg.filter_z_pct;

       /* Find the FILTER_Z_PCT percentile from the unfiltered Z histogram */
       int xpct = 0;

       histoCount = HistoZ[xpct];
       while (histoCount < index_pct) {
          xpct++;
          histoCount += HistoZ[xpct];
       }

       /* Convert the integer data back to real Z value */
       z_pct = (xpct - Z_OFFSET) / Z_SCALE;

       /* Apply the precipitation filter */
       if (z_pct <= bragg_cfg.filter_z_thresh) {

          /* We've passed the precipitation filter */
          /* Determine the 25th and 75th percentile of the number of Bragg bins collected. */
          index_25 = bragg_cnt * 0.25;
          index_75 = bragg_cnt * 0.75;

          /* Find the 25th and 75th percentiles from the ZDR histogram */
          x25 = 0;
          x75 = 0;
          float zdr_25, zdr_75;

          histoCount = HistoZDR[vol][x25];
          while (histoCount < index_25) {
             x25++;
             histoCount += HistoZDR[vol][x25];
          }
          histoCount = HistoZDR[vol][x75];
          while (histoCount < index_75) {
             x75++;
             histoCount += HistoZDR[vol][x75];
          }
 
          /* Convert the integer data back to real ZDR values */
          zdr_25 = (x25 - ZDR_OFFSET) / ZDR_SCALE;
          zdr_75 = (x75 - ZDR_OFFSET) / ZDR_SCALE;

          if (debug_flag) fprintf(stderr," zdr_25 = %f zdr_75 = %f z_pct = %2.3f\n",zdr_25,zdr_75,z_pct);
 
          /* Difference between the 25th and 75th percentiles is the IQR */
          zdr_iqr = zdr_75 - zdr_25;

          /* Apply the second statistics filter (i.e. ZDR interquartile range) */
          if (zdr_iqr <= bragg_cfg.zdr_iqr_thresh) {
 
             /* We've passed the statistics filters and the precipitation filter */         
             /* Find zdr mode from histogram */
             int max = 0;
             for (j = 1; j < HISTOGRAM_SIZE_ZDR; j++) {
                if (HistoZDR[vol][j] > HistoZDR[vol][max]) {
                  max = j;
                }
             }
             zdr_mode = (max - ZDR_OFFSET) / ZDR_SCALE;

             if (debug_flag) {
                fprintf(stderr," ZDR: iqr = %2.4f mode = %2.3f \n",zdr_iqr, zdr_mode);
             }

             /* Store this ZDR bias estimate and bin count.  The expected ZDR from Bragg */
             /* is 0 dB so there is no need to do a subtraction here.                    */
             Volume_zdr_bias[vol] = zdr_mode;
             Volume_bragg_cnt[vol] = bragg_cnt;
             LE_send_msg(GL_INFO, "%d Bragg bins. ZDR bias estimate this volume: %3.2f", bragg_cnt,zdr_mode);
          }
          else {
             LE_send_msg(GL_INFO,
             "%d Bragg bins. Not used due to possible biota contamination (zdr_iqr = %2.2f dB", bragg_cnt,zdr_iqr);
          }/* End if the ZDR IQR showed a small enough spread in the data to use it */
       }
       else {
          LE_send_msg(GL_INFO,
         "%d Bragg bins. Not used due to possible precip contamination (z_pct = %2.2f dB", bragg_cnt,z_pct);
       } /* End precipitation filter */
     }
     else {
        LE_send_msg(GL_INFO, "%d Bragg bins. Insufficent number for estimate",bragg_cnt);
     }/* End if there were enough bins this volume to make an estimate */

     /* If channel has changed, keep this volume's data but zero out all previous volumes */
     if (channel != Bragg_data.Bragg_channel) {
        if (debug_flag)
             fprintf(stderr," Redundant channel change from %d to %d\n", Bragg_data.Bragg_channel,channel);
        for (j = 0; j < BRAGG_VOLUMES; j++) {
           if (j != vol) Volume_bragg_cnt[vol] = 0;
        }
        Bragg_data.Bragg_channel = channel;
     }

     /* Compute the final ZDR bias estimate using all valid volume scans */
     total_bins = 0;
     total_zdr  = 0;
     for (j = 0; j < BRAGG_VOLUMES; j++) {
       if (Volume_bragg_cnt[j] > 0) {
          total_bins += Volume_bragg_cnt[j];
          total_zdr  += Volume_zdr_bias[j];
          num_volumes++;
       }
     }
     zdr_bias = total_zdr / num_volumes;

     /* Check to see if this is a non-operational configuration using a */
     /* fixed value ZDR adjustment */
     if (DPP_ZDR_DYNAMIC_ADJUST == 2) zdr_bias = DPP_ZDR_DYNAMIC_ADJUST_VALUE;

     /* Produce a system status message with a format based on whether the estimate */
     /* can be made considering the total bin count for all volume scans, as well   */
     /* as the redundant configuration.                                             */
     /* If this is a non-operational configuration using a fixed ZDR adjustment     */
     /* value always output a status message.                                       */
     if (total_bins >= bragg_cfg.min_bragg_cnt || DPP_ZDR_DYNAMIC_ADJUST == 2) {

        /* There are enough bins to make an overall ZDR bias estimate. */
        /* Save the date, time and RDA channel of last good estimate.  */
        Bragg_data.Last_zdr_bias = zdr_bias;
        Bragg_data.Last_zdr_bias_time = params->zdr_time;
        Bragg_data.Last_zdr_bias_date = params->zdr_date;
        Bragg_data.Bragg_channel = channel;

	if( (redundant_type == FAA_REDUNDANT)
                            ||
            (redundant_type == NWS_REDUNDANT) ) {
           RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, 
            "ZDR Stats (Bragg) [RDA:%d]: %3.2f/%d/%3.2f/%d/%3.2f/%2.1f/%d %s", 
	    channel, zdr_bias, total_bins, Volume_zdr_bias[vol], bragg_cnt, zdr_iqr, z_pct, params->vcp_num,adj_str);
        }
        else {
           RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, 
            "ZDR Stats (Bragg): %3.2f/%d/%3.2f/%d/%3.2f/%2.1f/%d %s", 
	    zdr_bias, total_bins, Volume_zdr_bias[vol], bragg_cnt, zdr_iqr, z_pct, params->vcp_num,adj_str);
        }
     }
     else {
        /* There are not enough bins to make an overall ZDR bias estimate  */
        /* Report the last computed estimate and the date/time it was made */
        /* but only report this message once per hour.                     */
        RPGCS_julian_to_date (Bragg_data.Last_zdr_bias_date,&yy,&mm,&dd);
	RPGCS_convert_radial_time ((Bragg_data.Last_zdr_bias_time),
			                   &hr, &min, &sec, &mills);
        curr_time = time(NULL);
        t1 = gmtime(&curr_time);

        if (Bragg_data.Last_zdr_bias_date > 0 && t1->tm_hour != Bragg_data.Last_report_hour) {

           /* Save this hour */
           Bragg_data.Last_report_hour = t1->tm_hour;

           /* Initialize the date and time fields */
           yyd = (double) yy;
	   mmd = (double) mm;
	   ddd = (double) dd;
	   hrd = (double) hr;
	   mind = (double) min;

  	   if( (redundant_type == FAA_REDUNDANT)
                               ||
               (redundant_type == NWS_REDUNDANT) ) {
              channel = Bragg_data.Bragg_channel;
	      RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG,
 	       "ZDR Stats (Bragg) [RDA:%d]: Last detection %02d/%02d/%02d %02d:%02dZ %3.2f %s", 
	       channel, (int)mmd, (int)ddd, (int)yyd, (int)hrd, (int)mind, Bragg_data.Last_zdr_bias,adj_str);
           }
           else {
	      RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG,
 	       "ZDR Stats (Bragg): Last detection %02d/%02d/%02d %02d:%02dZ %3.2f %s", 
	       (int)mmd, (int)ddd, (int)yyd, (int)hrd, (int)mind, Bragg_data.Last_zdr_bias, adj_str);
           }
        }
        else {
           if( t1->tm_hour != Bragg_data.Last_report_hour) {

              /* Save this hour */
              Bragg_data.Last_report_hour = t1->tm_hour;
              
    	      if( (redundant_type == FAA_REDUNDANT)
                                  ||
                  (redundant_type == NWS_REDUNDANT) ) {
                 /* Must be a clean startup and no past data file to read */
  	         RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG,
 	          "ZDR Stats (Bragg) [RDA:%d]: Unavailable", channel);
              }
              else {
  	         RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG,
 	          "ZDR Stats (Bragg): Unavailable");
              }
           }
        } 
     }

     /* Save this estimate (and the date/time/channel) for retrieval on system restarts */
     buffer = malloc(sizeof(Bragg_data_t));
     if (buffer != NULL) {
        int bytes_written;
        memcpy(&buffer, &Bragg_data, sizeof(Bragg_data_t));
        bytes_written = RPGC_data_access_write(DP_BRAGG,
                                               &buffer,
                                               sizeof(Bragg_data_t),
                                               (LB_id_t) DP_BRAGG_MSGID);
        if (bytes_written <=0) LE_send_msg (GL_INFO, "bragg.dat write failed");
     }
     else {
        LE_send_msg (GL_INFO, "bragg.dat write buffer NULL");
     }

     /* Increment the volume index, reset if needed */
     vol++;
     if (vol == BRAGG_VOLUMES) vol = 0;

     /* Reset the counter and histograms for the next volume scan */
     bragg_cnt = 0;
     precip_filter_cnt = 0;

     for (j = 0; j < HISTOGRAM_SIZE_ZDR; j++) {
        HistoZDR[vol][j] = 0;
     }
     for (j = 0; j < HISTOGRAM_SIZE_PHI; j++) {
        HistoPHI[vol][j] = 0;
     }
     for (j = 0; j < HISTOGRAM_SIZE_Z; j++) {
        HistoZ[j] = 0;
     }

  } /* end if end of volume */

  return;
}  /* end findBragg() */

/******************************************************************

    Retrieves the Bragg configuration data. If adaptation 
    data is not available, the default values are returned. The
    adaptation database is read at the beginning of each volume.
	
******************************************************************/

static void Get_bragg_cfg (Bragg_cfg_t *bragg_cfg) {


	int ret;
	double min_bragg_bin, max_bragg_bin;
	double min_bragg_z, max_bragg_z;
	double min_bragg_v;
	double min_bragg_sw, max_bragg_sw;
	double min_bragg_rho, max_bragg_rho;
	double min_bragg_snr, max_bragg_snr;
	double min_bragg_phi, max_bragg_phi;
	double min_bragg_cnt, min_bragg_cnt_vol;
	double zdr_iqr_thresh;
        double filter_z_pct, filter_z_thresh;

	if ((ret = DEAU_get_values ("alg.bragg.min_bragg_bin", 
						&min_bragg_bin, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.max_bragg_bin", 
						&max_bragg_bin, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.min_bragg_z", 
						&min_bragg_z, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.max_bragg_z", 
						&max_bragg_z, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.min_bragg_v", 
						&min_bragg_v, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.min_bragg_sw", 
						&min_bragg_sw, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.max_bragg_sw", 
						&max_bragg_sw, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.min_bragg_rho", 
						&min_bragg_rho, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.max_bragg_rho", 
						&max_bragg_rho, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.min_bragg_snr", 
						&min_bragg_snr, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.max_bragg_snr", 
						&max_bragg_snr, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.min_bragg_phi", 
						&min_bragg_phi, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.max_bragg_phi", 
						&max_bragg_phi, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.min_bragg_cnt", 
						&min_bragg_cnt, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.min_bragg_cnt_vol", 
						&min_bragg_cnt_vol, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.zdr_iqr_thresh", 
						&zdr_iqr_thresh, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.filter_z_pct", 
						&filter_z_pct, 1)) <= 0 ||
	    (ret = DEAU_get_values ("alg.bragg.filter_z_thresh", 
						&filter_z_thresh, 1)) <= 0) {
	    LE_send_msg (GL_INFO, 
		    "Bragg configuration data not available (DEAU_get_values %d)", ret);
	    LE_send_msg (GL_INFO, "Using default Bragg config values");
	    min_bragg_bin     =   40;
	    max_bragg_bin     =  320;
	    min_bragg_z       =  -32.0;
	    max_bragg_z       =   10.0;
	    min_bragg_v       =    2.0;
	    min_bragg_sw      =    0.0;
	    max_bragg_sw      =   30.0;
	    min_bragg_rho     =    0.98;
            max_bragg_rho     =    1.05;
            min_bragg_snr     =   -5.0;
            max_bragg_snr     =   15.0;
            min_bragg_phi     =    0.0;
            max_bragg_phi     =  400.0;
            min_bragg_cnt_vol =  600;
            min_bragg_cnt     = 10000;
            zdr_iqr_thresh    =    0.9;
            filter_z_pct      =    0.90;
            filter_z_thresh   =   -3.0;
	}

	bragg_cfg->min_bragg_bin     = (int)min_bragg_bin;
	bragg_cfg->max_bragg_bin     = (int)max_bragg_bin;
	bragg_cfg->min_bragg_z       = min_bragg_z;
	bragg_cfg->max_bragg_z       = max_bragg_z;
	bragg_cfg->min_bragg_v       = min_bragg_v;
	bragg_cfg->min_bragg_sw      = min_bragg_sw;
	bragg_cfg->max_bragg_sw      = max_bragg_sw;
	bragg_cfg->min_bragg_rho     = min_bragg_rho;
	bragg_cfg->max_bragg_rho     = max_bragg_rho;
	bragg_cfg->min_bragg_snr     = min_bragg_snr;
	bragg_cfg->max_bragg_snr     = max_bragg_snr;
	bragg_cfg->min_bragg_phi     = min_bragg_phi;
	bragg_cfg->max_bragg_phi     = max_bragg_phi;
	bragg_cfg->min_bragg_cnt     = (int)min_bragg_cnt;
	bragg_cfg->min_bragg_cnt_vol = (int)min_bragg_cnt_vol;
        bragg_cfg->zdr_iqr_thresh    = zdr_iqr_thresh;
	bragg_cfg->filter_z_pct      = filter_z_pct;
        bragg_cfg->filter_z_thresh   = filter_z_thresh;

    return;
}
