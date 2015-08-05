/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 16:03:12 $
 * $Id: findBragg.h,v 1.3 2014/10/03 16:03:12 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/*==================================================================
 *
 * Module Name: findBragg.h 
 *
 * Module Version: 1.0
 *
 * Module Language: c
 *
 * Change History:
 *
 * Date    Version    Programmer  Notes
 * ----------------------------------------------------------------------
 * 04/25/14  1.0    Brian Klein   Creation
 * 
 *
 * Description:
 *  This code determines which signals are from Bragg scattering;
 *
 *=============================================================================*/
#ifndef _FINDBRAGG_H
#define _FINDBRAGG_H 1

#define BRAGG_VOLUMES   12  /* Number of scans to average for final estimate */
#define Z_OFFSET        66.
#define Z_SCALE          2.
#define ZDR_OFFSET     128.
#define ZDR_SCALE       16.
#define PHI_OFFSET       2.
#define PHI_SCALE       60.678707
#define HISTOGRAM_SIZE_ZDR 256   /* ZDR is 8-bit data */
#define HISTOGRAM_SIZE_PHI 65536 /* PHI is 16-bit data */
#define HISTOGRAM_SIZE_Z   256   /* Z is 8-bit data */
#define MIN_BRAGG_ELEV   24    /* lowest RPG elevation angle to use (times 10) */
#define MAX_BRAGG_ELEV   45    /* highest RPG elevation angle to use (times 10) */
#define MIN_ZDR_BIN       3    /* lowest integer encoded ZDR to be used in histobram */
#define MAX_ZDR_BIN     254    /* highest integer encoded ZDR to be used in histogram */

typedef struct
{
  int	Last_zdr_bias_time;  /* Time of last good Bragg-based ZDR bias estimate */
  int   Last_zdr_bias_date;  /* Date of last good Bragg-based ZDR bias estimate */
  float Last_zdr_bias;       /* Last good Bragg-based ZDR bias estimate         */
  int   Bragg_channel;       /* For redundant systems, the applicable channel   */
  int   Last_report_hour;    /* Hour of last status message when no estimate is made */
} Bragg_data_t;


typedef struct
{
 int    min_bragg_bin;       /* Start bin for Bragg scatter analysis */
 int    max_bragg_bin;	     /* End bin for Bragg scatter anaylsis */
 float  min_bragg_z;	     /* Reflectivity value above which may be Bragg scatter */
 float  max_bragg_z;	     /* Reflectivity value below which may be Bragg scatter */
 float  min_bragg_v;	     /* Absolute value of velocity above which may be Bragg scatter */
 float  min_bragg_sw;	     /* Spectrum width value above which may be Bragg scatter */
 float  max_bragg_sw;	     /* Spectrum width value below which may be Bragg scatter */
 float  min_bragg_rho;	     /* Correlation coefficient value above which may be Bragg scatter */
 float  max_bragg_rho;	     /* Correlation coefficient value below which may be Bragg scatter */
 float  min_bragg_snr;	     /* Signal-to-noise ratio value above which may be Bragg scatter */
 float  max_bragg_snr;	     /* Signal-to-noise ratio value below which may be Bragg scatter */
 float  min_bragg_phi;	     /* Differential phase value above which may be Bragg scatter */
 float  max_bragg_phi;	     /* Differential phase value below which may be Bragg scatter */
 int    min_bragg_cnt_vol;   /* Minimum number of Bragg bins for a volume scan to be usable in analysis */
 int    min_bragg_cnt;	     /* Minimum number of Bragg bins for all volumes considered */
 float  zdr_iqr_thresh;	     /* Differential reflectivity interquartile range threshold (biota filter) */
 float  filter_z_pct;	     /* Reflectivity percentile to be used for precipitation filter */
 float  filter_z_thresh;     /* Reflectivity threshold for precipitation filter */
} Bragg_cfg_t;

void findBragg(Dpp_params_t *params,
                      float *Z,
                      float *V,
                      float *SPW,
                      float *RhoHV,
                      float *Zdr,
                      float *PhiDP,
                      float *SNR);

#endif           
