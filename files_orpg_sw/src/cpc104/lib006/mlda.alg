# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/07/22 18:38:42 $
# $Id: mlda.alg,v 1.13 2013/07/22 18:38:42 steves Exp $
# $Revision: 1.13 $
# $State: Exp $

###########################################################
##
##	File:		MLDA.alg
##	Author:		Ning Shen
##	Date:		May 1, 2007
##	version:	1.0
##
##	Description:
##	------------
##
##	This file contains all the adaptation data used by 
##	Melting Layer Detection Algorithm.
##
###########################################################

  alg_name		value = MLDA


  Melting_Layer_Source	value = Model_Enhanced;
			range = {RPG_0C_Hgt, Radar_Based, Model_Enhanced};
			enum = 0, 1, 2;
			name = Melting Layer Source;
			type = string;
			permission = [ROC, URC];
			description = Melting layer top and bottom heights are defined by radar-only MLDA when selecting Radar_Based and model-enhanced radar when selecting Model_Enhanced.  Select RPGC_0C_Hght to override and use the Environmental Data 0-degree C height.;	

  ml_depth		value = 0.5;
			name = Melting Layer Depth;
			type = float;
			range = [0.5, 1.0];
			unit = km;
			accuracy = [0.1];
			description = vertical distance above a sample volume;

  max_top_height	value = 8.0;
			name = max ML top height;
			type = float;
			range = [1.0, 9.0];
			unit = km;
			accuracy = [0.1];
			description = max height for ML top;

  height_interval	value = 100;
			name = vertical height interval
			type = int;
			range = [100, 500];
                        unit = meter;
			accuracy = 10;
			description = height interval;

  upper_RhoHV_limit	value = 0.97;
			name = upper limit of RhoHV in ML;
			type = float;
			range = [0.85, 0.97];
			accuracy = [0.01];
			description = upper limit of RhoHV in ML;

  lower_RhoHV_limit	value = 0.90;
			name = lower limit of RhoHV in ML;
			type = float;
			range = [0.85, 0.92];
			accuracy = [0.01];
			description = lower limit of RhoHV in ML;

  low_RhoHV_profile	value = 0.85;
			name = low RhoHV profile value;
			type = float;
			range = [0.80, 0.90];
			accuracy = [0.01];
			description = low RhoHV profile value in ML;

  upper_Zmax_profile	value = 47.0;
			name = upper limit of Zmax profile;
			type = float;
			range = [43, 50];
			unit = dBZ;
			accuracy = [1];
			description = upper limit of Zmax profile value in ML;

  lower_Zmax_profile	value = 30.0;
			name  = lower limit of Zmax profile;
			type = float;
			range = [28, 35];
			unit = dBZ;
			accuracy = [1];
			description = lower limit of Zmax profile value in ML;

  upper_Z_limit		value = 47.0;
			name = upper limit of Z in ML;
			type = float;
			range = [43, 50];
			unit = dBZ;
			accuracy = [1];
			description = upper limit of Z in ML;

  lower_Z_limit		value = 15.0;
			name = lower limit of Z in ML;
			type = float;
			range = [12, 20];
			unit = dBZ;
			accuracy = [1];
			description = lower limit of Z in ML;

  upper_ZDRmax_pro	value = 2.2;
			name = upper limit of ZDRmax profile value;
			type = float;
			range = [2.0, 3.0];
			unit = dB;
			accuracy = [0.1];	
			description = upper limit of ZDRmax profile value in ML;

  lower_ZDRmax_pro	value = 0.8;
			name = lower limit of ZDRmax profile value;
			type =  float;
			range = [0.6, 1.0];
			unit = dB;
			accuracy = [0.1];
			description = lower limit of ZDRmax profile value in ML;

  half_window_size	value = 10;
			name = azimuth size in both side of a given azimuth;
			type = int;
			range = [8, 15];
			unit = degree;
			accuracy = [1];
			description =  azimuth size in both side of a given azimuth;

  upper_elev_limit	value = 10.0;
 			name = upper limit for the elevation angle in ML;
			type = float;
			range = [8.0, 11.0];
			unit = degree;
			accuracy = [0.1];
			description = upper limit for the elevation angle in ML;

  lower_elev_limit	value = 4.0;
			name = lower limit for the elevation angle in ML;
			type = float;
			range = [3.0, 5.0];
			unit = degree;
			accuracy = [0.1];
			description = lower limit for the elevation angle in ML;

  high_percentile	value = 0.80;
			name = percentile value;
			type = float;
			range = [0.80, 0.90];
			accuracy = [0.1];
			description = percentile value of a normalized histogram;

  low_percentile	value = 0.20;
			name = percentile value;
			type = float;
			range = [0.10, 0.20];
			accuracy = [0.1];
			description = percentile value of a normalized histogram;

  min_wet_snow_sum	value = 1500;
			name = min wet snow weight sum;
			type = int;
			range = [1400, 1600];
			accuracy = [10];
			description = min wet snow weight summation for a given azimuth;

  low_snow_thresh	value = 0;
			name = low snow thresh for model merging;
			type = int;
			range = [0, 1400];
			accuracy = [100];
			description = low snow weight for model merging for a given azimuth;

  min_rad_mod_wt	value = 0.0;
			name = minimum ratio of radar data for model merging;
			type = float;
			range = [0.0, 0.5];
			accuracy = [0.01];
			description = minimum ratio for interpolated radar data merging with model;

  min_snr		value = 5.0;
			name = minimum value of snr;
			type = float;
			range = [-12.0, 110.0];
			unit = dB;
			accuracy = [0.5];
			description = minimum value of snr;

  vol_time_gap		value = 30;
			name = minutes elapsed between volumes;
			type = int;
			unit = minute;
			range = [20, 60];
			accuracy = 5;
			description = volume time gap allowed;

  num_vol_clear		value = 6;
			name = minimum number of volume needed for clear air mode;
			type = int;
			range = [4, 8];
			description = number of volume needed for clear air mode;

  num_vol_precip	value = 3;
			name = minimum number of volume needed for precip mode;
			type = int;
			range = [3, 5];
			description = number of volume needed for precip mode;

  mdl_time_gap		value = 240;
			name = Allowable minutes elapsed between volume and model time;
			type = int;
			unit = minute;
			range = [90, 900];
			accuracy = 5;
			description = model time gap allowed;

  mdl_half_window_size	value = 8;
			name = half width for model based data;
			type = int;
			range = [1, 15];
			unit = degree;
			accuracy = [1];
			description =  half width of radials to average for model based calculations;

  mdl_min_range		value = 0;
			name = minimum model based range;
			type = int;
			range = [0, 50];
			unit = km;
			accuracy = [1];
			description =  Minimum range for model based calculations;

  mdl_max_range		value = 150;
			name = maximum model based range;
			type = int;
			range = [100, 400];
			unit = km;
			accuracy = [1];
			description =  Maximum range for model based calculations;

  radar_maxgap_interval value = 15;
			name = max gap between good radar based radials;
			type = int;
			range = [0, 359];
			unit = degrees;
			accuracy = [1];
			description =  use interpolated radar values if gaps are smaller than this value;

  radar_radonly_thr	value = 320;
			name = Threshold value of valid radar-based azimuths;
			type = int;
			range = [0, 359];
			unit = degree;
			accuracy = [1];
			description =  Use all radar based data if valid radials exceeds threshold;

  mdl_min_bot_delta	value = 0.5;
			name = Average distance between top and bottom freezing heights;
			type = float;
			range = [0.0, 2.0];
			unit = km;
			accuracy = [0.1];
			description =  Distance used to build bottom of freezing height for model based calculations;

  mdl_max_bot_delta	value = 1.0;
			name = Maximum distance between top and bottom freezing heights;
			type = float;
			range = [0.0, 8.0];
			unit = km;
			accuracy = [0.1];
			description =  Maximum delta allowed to build bottom of freezing height for model based calculations;

  use_avg_top_clipping	value = 1;
			name = Boolean to indicate whether to use average top or alternative;
			range = [0, 1];
			type = int;
			description =  Use last average top or prev radial by radial top for clipping wet snow;
